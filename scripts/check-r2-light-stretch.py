#!/usr/bin/env python3
"""Host oracle for the fighter hardware light's normalisation point (K02/P04/J01).

Runs nothing on hardware and writes no files.  It re-implements, in the same
integer arithmetic the ROM uses, the three quantities the r25 face/body seam
turns on, and asserts the five claims the repair in
``src/nds/nds_renderer_native_common.c`` rests on::

    software   level = dot(N, M L) / |M L|      PrepareLitDirection normalises
                                                the light AFTER the transform
    hardware   level = dot(N, M L) / |L|        r25: normalised BEFORE it, and
                                                the engine never renormalises
                                                N x M
    ratio      level_hw / level_sw = |M L| / |L|

CLAIM 1  A pure rotation -- which is ALL a facing change is; ftmain.c:4477 makes
         ``fp->lr`` a +-90 degree Y rotation on the TopN joint and no lr-driven
         negative scale exists in decomp/ or src/import/ -- leaves the ratio at
         exactly 1.  Facing alone therefore cannot produce the reported seam.
         If the owner's witness shows row_norm == 4096 on both facings, this
         whole mechanism is refuted and the residual is elsewhere.
CLAIM 2  With a rigid 3x3 the repaired packed light word is BIT-IDENTICAL to
         r25's.  That is the safety argument for defaulting the repair on.
CLAIM 3  With a scaled 3x3 the repaired level tracks the software level, while
         r25's is off by the stretch.
CLAIM 4  The repair never drives a packed component outside the DS's 10-bit
         signed field, which NDS_R2_NORMAL_PACK masks (wraps) rather than
         saturates.  That is why the repair declines when |M L| < |L|.
CLAIM 5  (negative, and it cost a wrong hypothesis to find)  A scale is NOT by
         itself enough.  A facing flip is diag(-1, 1, -1) on the light, and
         |A w|^2 = w' (A'A) w, so the flip changes the stretch only through the
         xy and yz cross terms of A'A.  An anisotropic scale composed straight
         onto the facing joint, or with only a further Y rotation below it,
         leaves those terms zero and is facing-INVARIANT.  It takes a scaled
         joint with an X/Z-rotated joint BELOW it before the stretch differs
         between facings -- and then it differs by up to 2.7x.  So the witness
         must show a non-unity row_norm AND a facing-dependent stretch; either
         alone does not implicate this mechanism.

Usage:  python scripts/check-r2-light-stretch.py [-v]
Exit 0 on success, 1 on the first failed claim.
"""

import math
import sys

FRAC = 12
ONE = 1 << FRAC          # 20.12 unity, NDS_RENDERER_DS_MTX_FRAC_BITS
LIGHT_UNIT = 511         # ndsRendererR2WriteLightVector's hardware scale
PACK_MIN = -512
PACK_MAX = 511


def isqrt64(value):
    """ndsR2HwMathSqrt64: DS SQRT unit, 64-bit mode, truncating."""
    if value <= 0:
        return 0
    return math.isqrt(int(value))


def div64(numerator, denominator):
    """ndsR2HwMathDiv64: DS DIV unit, DIV_64_32, truncates toward zero."""
    if denominator == 0:
        return 0
    q = abs(numerator) // abs(denominator)
    return -q if (numerator < 0) != (denominator < 0) else q


def mat_rot_y(degrees):
    """Row-major m[i][j] in 20.12, the layout MATRIX_LOAD4x4 and
    MATRIX_READ_VECTOR both use."""
    c = math.cos(math.radians(degrees))
    s = math.sin(math.radians(degrees))
    return [int(round(v * ONE)) for v in
            (c, 0.0, s,
             0.0, 1.0, 0.0,
             -s, 0.0, c)]


def mat_rot_x(degrees):
    c = math.cos(math.radians(degrees))
    s = math.sin(math.radians(degrees))
    return [int(round(v * ONE)) for v in
            (1.0, 0.0, 0.0,
             0.0, c, -s,
             0.0, s, c)]


def mat_rot_z(degrees):
    c = math.cos(math.radians(degrees))
    s = math.sin(math.radians(degrees))
    return [int(round(v * ONE)) for v in
            (c, -s, 0.0,
             s, c, 0.0,
             0.0, 0.0, 1.0)]


def mat_mul(a, b):
    out = [0] * 9
    for i in range(3):
        for j in range(3):
            acc = 0
            for k in range(3):
                acc += a[i * 3 + k] * b[k * 3 + j]
            out[i * 3 + j] = acc >> FRAC
    return out


def mat_scale(sx, sy, sz):
    return [int(round(sx * ONE)), 0, 0,
            0, int(round(sy * ONE)), 0,
            0, 0, int(round(sz * ONE))]


def transform_light(m, light):
    """t_i = sum_j m[i][j] * L_j -- byte for byte
    ndsRendererHardwarePrepareLitDirection and
    ndsRendererR2SampleVectorMatrix."""
    return [sum(m[i * 3 + j] * light[j] for j in range(3)) for i in range(3)]


def row_norms(m):
    return [isqrt64(sum(m[i * 3 + j] ** 2 for j in range(3))) for i in range(3)]


def det_20p12(m):
    det = (m[0] * (m[4] * m[8] - m[5] * m[7])
           - m[1] * (m[3] * m[8] - m[5] * m[6])
           + m[2] * (m[3] * m[7] - m[4] * m[6]))
    return det >> (2 * FRAC)


def pack_r25(light):
    """ndsRendererR2WriteLightVector as shipped in r25."""
    length = isqrt64(sum(c * c for c in light))
    if length == 0:
        return (0, 0, 0)
    return tuple(div64(-c * LIGHT_UNIT, length) for c in light)


def light_len_q12(light):
    """|L| in 20.12.  sqrt(n << 24) == sqrt(n) << 12, so this is the precise
    reference `isqrt64(sum) << 12` is NOT -- the DS sqrt truncates, and the
    shipped light is ~100 units long, so the truncated form is up to 1% short."""
    return isqrt64(sum(c * c for c in light) << (2 * FRAC))


def pack_repaired(light, m):
    """The repair: divide by |M L| instead of |L|, only when the chain
    stretches (so a component can never leave the 10-bit field) and only
    outside a 1/32 deadband (so a rigid chain is bit-identical to r25)."""
    length = isqrt64(sum(c * c for c in light))
    if length == 0:
        return (0, 0, 0), False
    ref = light_len_q12(light)
    t = transform_light(m, light)
    stretched = isqrt64(sum(v * v for v in t))
    if ref > 0 and stretched > ref + (ref >> 5):
        scale = LIGHT_UNIT << FRAC
        return tuple(div64(-c * scale, stretched) for c in light), True
    return tuple(div64(-c * LIGHT_UNIT, length) for c in light), False


def hardware_level(packed_light, normal, m):
    """What the geometry engine computes: diffuse = max(0, -dot(Lstored, N x M)),
    with Lstored in 1.0 == 512 units and the normal in the same units."""
    nm = [sum(normal[i] * m[i * 3 + j] for i in range(3)) / ONE for j in range(3)]
    dot = sum(packed_light[j] / 512.0 * nm[j] / 512.0 for j in range(3))
    return max(0.0, -dot)


def software_level(light, normal, m):
    """dot(N, M L) / |M L| -- PrepareLitDirection renormalises to 127, then
    LitDiffuseNumer divides the dot by 127."""
    t = transform_light(m, light)
    tlen = math.sqrt(sum(float(v) ** 2 for v in t))
    if tlen == 0.0:
        return 0.0
    unit = [v / tlen for v in t]
    return max(0.0, sum(normal[i] / 512.0 * unit[i] for i in range(3)))


# ftDisplayLightsDrawReflect writes dir[] = vec * 100.0F into an s8 Light.
LIGHT = [-40, -70, -55]
# A handful of object-space normals in the DS's 1.0 == 512 units.
NORMALS = [
    (511, 0, 0), (-511, 0, 0), (0, 511, 0), (0, 0, 511), (0, 0, -511),
    (295, 295, 295), (-295, 295, -295), (180, -420, 200),
]
CAMERA = mat_rot_y(18.0)          # an ordinary orthonormal view rotation


def report(verbose, *args):
    if verbose:
        print(*args)


def main(argv):
    verbose = "-v" in argv
    failures = []

    # ---- CLAIM 1: facing alone is a rotation; the stretch stays at 1.0 ----
    for lr, degrees in ((+1, +90.0), (-1, -90.0), (0, 0.0)):
        m = mat_mul(mat_rot_y(degrees), CAMERA)
        norms = row_norms(m)
        det = det_20p12(m)
        t = transform_light(m, LIGHT)
        ref = light_len_q12(LIGHT)
        stretch = div64(isqrt64(sum(v * v for v in t)) << FRAC, ref)
        report(verbose,
               "lr=%+d rows=%s det=%d stretch=%d (unity %d)"
               % (lr, norms, det, stretch, ONE))
        if det <= 0:
            failures.append(
                "CLAIM 1: lr=%+d gave a non-positive determinant %d; the "
                "facing rotation is not a mirror" % (lr, det))
        for n in norms:
            if abs(n - ONE) > 2:
                failures.append(
                    "CLAIM 1: lr=%+d row norm %d is not unity" % (lr, n))
        if abs(stretch - ONE) > 2:
            failures.append(
                "CLAIM 1: lr=%+d stretch %d is not unity -- a rigid chain "
                "cannot make the hardware light facing-dependent" % (lr, stretch))

    # ---- CLAIM 2: rigid chain => repaired word is bit-identical to r25 ----
    for degrees in (+90.0, -90.0, 0.0, 37.0):
        m = mat_mul(mat_rot_y(degrees), CAMERA)
        old = pack_r25(LIGHT)
        new, applied = pack_repaired(LIGHT, m)
        report(verbose, "rigid %+.0f deg: r25=%s repaired=%s applied=%s"
               % (degrees, old, new, applied))
        if new != old:
            failures.append(
                "CLAIM 2: rigid chain at %+.0f deg changed the packed light "
                "%s -> %s; the repair must be a no-op here" % (degrees, old, new))

    # ---- CLAIM 3: scaled chain => repaired level tracks the software level ----
    for sx, sy, sz, label in ((2.0, 2.0, 2.0, "isotropic 2x"),
                              (2.5, 1.0, 1.0, "anisotropic x"),
                              (1.0, 1.0, 3.0, "anisotropic z")):
        for degrees, lr in ((+90.0, +1), (-90.0, -1)):
            m = mat_mul(mat_mul(mat_scale(sx, sy, sz), mat_rot_y(degrees)),
                        CAMERA)
            old = pack_r25(LIGHT)
            new, applied = pack_repaired(LIGHT, m)
            if not applied:
                failures.append(
                    "CLAIM 3: %s lr=%+d did not apply the repair" % (label, lr))
                continue
            for normal in NORMALS:
                sw = software_level(LIGHT, normal, m)
                hw_old = hardware_level(old, normal, m)
                hw_new = hardware_level(new, normal, m)
                if sw > 0.05 and abs(hw_new - sw) > 0.02:
                    failures.append(
                        "CLAIM 3: %s lr=%+d normal=%s repaired level %.4f != "
                        "software %.4f" % (label, lr, normal, hw_new, sw))
                if sw > 0.05 and abs(hw_old - sw) <= 0.02:
                    failures.append(
                        "CLAIM 3: %s lr=%+d normal=%s r25 level %.4f already "
                        "matched software %.4f -- the test case proves nothing"
                        % (label, lr, normal, hw_old, sw))
            report(verbose, "%s lr=%+d: repaired=%s (r25=%s)"
                   % (label, lr, new, old))

    # ---- CLAIM 5: a scale BESIDE the facing joint is facing-INVARIANT ----
    # This one is a negative result and it cost a wrong hypothesis to find, so
    # it is asserted rather than left in a comment.  A facing flip is a 180
    # degree Y rotation, which maps (x, y, z) -> (-x, y, -z).  A diagonal
    # scale's quadratic form 6.25x^2 + y^2 + z^2 is EVEN in every coordinate,
    # so |S R L| is identical for both facings no matter how anisotropic S is.
    # An anisotropic joint scale alone therefore CANNOT make the hardware light
    # facing-dependent.  Neither can one with a further Y rotation between it
    # and the facing joint, because Y rotations commute and the total is still
    # a Y rotation.  It takes a rotation about X or Z between the facing joint
    # and the scale -- i.e. a scaled joint further up a POSED chain.  So the
    # witness has to show BOTH a non-unity row_norm AND a stretch that differs
    # between facings before this mechanism is the one in play; either alone
    # is not enough.
    for sx, sy, sz, label in ((2.5, 1.0, 1.0, "aniso x"),
                              (1.0, 1.0, 3.0, "aniso z")):
        beside = []
        yaw_between = []
        tilt_between = []
        ref = light_len_q12(LIGHT)
        for degrees in (+90.0, -90.0):
            facing = mat_mul(mat_rot_y(degrees), CAMERA)
            scale = mat_scale(sx, sy, sz)
            # Order matters and it is the whole finding.  |A w|^2 = w' (A'A) w,
            # so only the symmetric part Q = A'A decides, and a rotation to the
            # LEFT of the scale cancels out of Q entirely (R'S'SR vs S'R'RS).
            # In `m L` the chain evaluates root-first, so "left of the scale"
            # means a joint nearer the LEAF and "right of it" a joint nearer
            # the ROOT.  Only a rotation BELOW the scaled joint -- a scaled
            # joint higher up a posed chain -- puts xy/yz terms into Q, and
            # those terms are exactly what the facing flip's diag(-1, 1, -1)
            # negates.
            cases = (
                (mat_mul(scale, facing), beside),
                (mat_mul(mat_mul(scale, mat_rot_y(41.0)), facing),
                 yaw_between),
                (mat_mul(mat_mul(scale,
                                 mat_mul(mat_rot_z(35.0), mat_rot_x(41.0))),
                         facing),
                 tilt_between),
            )
            for m, sink in cases:
                t = transform_light(m, LIGHT)
                sink.append(div64(isqrt64(sum(v * v for v in t)) << FRAC, ref))
        report(verbose, "%s: beside=%s yaw_between=%s tilt_between=%s"
               % (label, beside, yaw_between, tilt_between))
        if beside[0] != beside[1]:
            failures.append(
                "CLAIM 5: %s scale beside the facing joint gave different "
                "stretches %s -- the invariance argument is wrong"
                % (label, beside))
        if abs(yaw_between[0] - yaw_between[1]) > 4:
            failures.append(
                "CLAIM 5: %s scale with only a YAW below it gave different "
                "stretches %s -- a Y rotation contributes an xz cross term, "
                "and the facing flip diag(-1, 1, -1) leaves xz alone, so this "
                "must stay invariant" % (label, yaw_between))
        if abs(tilt_between[0] - tilt_between[1]) <= 4:
            failures.append(
                "CLAIM 5: %s scale with a Z-then-X tilt below it still gave "
                "equal stretches %s -- then nothing in this mechanism is "
                "facing-dependent at all" % (label, tilt_between))

    # ---- CLAIM 4: packed components stay inside the 10-bit signed field ----
    shrink_seen = False
    for sx, sy, sz in ((0.25, 0.25, 0.25), (0.4, 1.0, 1.0), (3.0, 0.2, 1.5),
                       (1.0, 1.0, 1.0), (9.33, 9.33, 9.33)):
        for degrees in (+90.0, -90.0, 0.0):
            m = mat_mul(mat_mul(mat_scale(sx, sy, sz), mat_rot_y(degrees)),
                        CAMERA)
            new, applied = pack_repaired(LIGHT, m)
            if not applied:
                shrink_seen = True
            for c in new:
                if c < PACK_MIN or c > PACK_MAX:
                    failures.append(
                        "CLAIM 4: scale (%.2f,%.2f,%.2f) at %+.0f deg packed "
                        "%d, outside [%d,%d]; NDS_R2_NORMAL_PACK would wrap it"
                        % (sx, sy, sz, degrees, c, PACK_MIN, PACK_MAX))
    if not shrink_seen:
        failures.append(
            "CLAIM 4: no shrinking chain was exercised, so the decline path "
            "is untested")

    if failures:
        for f in failures:
            print("FAIL:", f)
        print("%d failure(s)" % len(failures))
        return 1
    print("check-r2-light-stretch: OK (5 claims)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
