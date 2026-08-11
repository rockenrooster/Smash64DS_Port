#!/usr/bin/env python3
"""Prove cycle 117's float-multiply deletions in the floor-crossing kernel are
BIT-IDENTICAL, not merely close.

`ndsMPFCSegmentCrossesKernel` decides whether a moving point crossed a collision
segment. It is gameplay, so unlike the render-side work it gets no error budget
at all -- the claim being made is equality, and the right instrument for an
equality claim is an equality check.

The three edits under test all rest on the same observation: `side` and `orient`
are both exactly +-1.0f, so `side * X` and `orient * X` are sign flips, and
`orient * sx` is `fabsf(sx)`. Multiplying an IEEE-754 binary32 by +-1.0f IS the
sign flip for every finite value, both zeroes and both infinities, so the
rewrite is equal by construction -- but "by construction" is what the strips
shipped on, so this compiles the REFERENCE (reconstructed here with the original
float expressions) and the SHIPPED header side by side and sweeps them.

The domain is the one the caller actually produces: `ud` in {+1, -1}, segments
drawn from Dream Land's own geometry scale, and motion vectors that straddle,
touch, miss and run parallel to the segment. Degenerate inputs are deliberately
included -- zero-length motion, vertical and horizontal segments, exactly-on-line
positions, and both signed zeroes -- because those are where a sign-flip rewrite
would differ if it differed anywhere.

Usage:
    python scripts/check_mp_floor_crossing_exact.py [--verbose]
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HEADER = os.path.join(ROOT, "include", "nds", "nds_mp_floor_crossing.h")
FCMP = os.path.join(ROOT, "include", "nds", "nds_fcmp.h")


def inline_header(path: str, drop_includes: bool) -> str:
    """Strip the include guard and the `#include`s so the body can be pasted
    into the harness. The closing `#endif` is bare in these headers, so it is
    found by matching the guard rather than by its own text -- dropping the
    LAST `#endif` is what balances the `#ifndef` that was removed."""
    lines = open(path, encoding="utf-8", errors="replace").read().splitlines()
    last_endif = max(
        (i for i, line in enumerate(lines) if line.strip().startswith("#endif")),
        default=-1,
    )
    out = []
    for i, line in enumerate(lines):
        s = line.strip()
        if s.startswith("#ifndef SSB64_") or s.startswith("#define SSB64_"):
            continue
        if i == last_endif:
            continue
        if drop_includes and s.startswith("#include"):
            continue
        out.append(line)
    return "\n".join(out)


HARNESS = r"""
#include <stdio.h>
#include <string.h>
#include <math.h>

typedef float f32;
typedef unsigned int u32;
typedef int s32;
typedef int sb32;

__FCMP__

__SHIPPED__

/* ---- reference: the pre-cycle-117 body, float multiplies intact ---- */
static int refKernel(
    float position_x, float position_y,
    float translate_x, float translate_y,
    float v1_x, float v1_y, float v2_x, float v2_y,
    int ud,
    float *hit_x, float *hit_y)
{
    const float epsilon = 0.001F;
    const float side = (float)ud;
    float sx, sy, min_x, max_x, min_y, max_y;

    if ((hit_x == 0) || (hit_y == 0)) { return 0; }
    sx = v2_x - v1_x;
    sy = v2_y - v1_y;
    if (NDS_FCMP_EQ0(sx)) { return 0; }
    min_x = (v1_x < v2_x) ? v1_x : v2_x;
    max_x = (v1_x > v2_x) ? v1_x : v2_x;
    min_y = (v1_y < v2_y) ? v1_y : v2_y;
    max_y = (v1_y > v2_y) ? v1_y : v2_y;

    if (NDS_FCMP_EQ0(sy)) {
        float delta_y, x;
        if (NDS_FCMP_LE0(side * (position_y - translate_y)) ||
            ((side * (position_y - v1_y)) < -epsilon) ||
            NDS_FCMP_LE0(side * (v1_y - translate_y))) { return 0; }
        delta_y = position_y - translate_y;
        x = (((v1_y - position_y) / delta_y) * (position_x - translate_x)) +
            position_x;
        if ((x < min_x) || (x > max_x)) { return 0; }
        *hit_x = x; *hit_y = v1_y; return 1;
    } else {
        float motion_dx = position_x - translate_x;
        float motion_dy = position_y - translate_y;
        float raw_prev, raw_curr, orient, extent_epsilon;
        float prev_height_scaled, curr_height_scaled, surface_prev;

        if (NDS_FCMP_GT0(motion_dy)) {
            if (((max_y + epsilon) < translate_y) ||
                (position_y < (min_y - epsilon))) { return 0; }
        } else if (((max_y + epsilon) < position_y) ||
                   (translate_y < (min_y - epsilon))) { return 0; }
        if (NDS_FCMP_GT0(motion_dx)) {
            if ((max_x < translate_x) || (position_x < min_x)) { return 0; }
        } else if ((max_x < position_x) || (translate_x < min_x)) { return 0; }

        raw_prev = (sx * (position_y - v1_y)) - (sy * (position_x - v1_x));
        raw_curr = (sx * (translate_y - v1_y)) - (sy * (translate_x - v1_x));
        orient = NDS_FCMP_GT0(sx) ? 1.0F : -1.0F;
        extent_epsilon = epsilon * (orient * sx);
        prev_height_scaled = side * (orient * raw_prev);
        curr_height_scaled = side * (orient * raw_curr);
        if (curr_height_scaled > -extent_epsilon) { return 0; }
        surface_prev = v1_y + (((position_x - v1_x) / sx) * sy);
        if (prev_height_scaled < extent_epsilon) {
            if ((prev_height_scaled > -extent_epsilon) &&
                (position_x >= min_x) && (position_x <= max_x)) {
                *hit_x = position_x; *hit_y = surface_prev; return 1;
            }
            return 0;
        } else {
            float denominator = raw_prev - raw_curr;
            float numerator, t, u;
            if (NDS_FCMP_EQ0(denominator)) { return 0; }
            t = raw_prev / denominator;
            numerator = ((v1_x - position_x) * (translate_y - position_y)) -
                        ((v1_y - position_y) * (translate_x - position_x));
            u = numerator / denominator;
            if ((t < -epsilon) || NDS_FCMP_GT_C(t, 1.0F + epsilon) ||
                (u < -epsilon) || NDS_FCMP_GT_C(u, 1.0F + epsilon)) { return 0; }
            if (NDS_FCMP_LT0(u)) { u = 0.0F; }
            else if (NDS_FCMP_GT_C(u, 1.0F)) { u = 1.0F; }
            *hit_x = v1_x + (sx * u); *hit_y = v1_y + (sy * u); return 1;
        }
    }
}

static long long cases, hits, mismatch_ret, mismatch_hit;
static double first_bad[9];
static int have_bad;

static u32 bits(float v) { u32 b; memcpy(&b, &v, sizeof b); return b; }

static void probe(float px, float py, float tx, float ty,
                  float ax, float ay, float bx, float by, int ud)
{
    float rhx = 12345.0f, rhy = -54321.0f, shx = 12345.0f, shy = -54321.0f;
    int r, s;

    cases++;
    r = refKernel(px, py, tx, ty, ax, ay, bx, by, ud, &rhx, &rhy);
    s = ndsMPFCSegmentCrossesKernel(px, py, tx, ty, ax, ay, bx, by, ud,
                                    &shx, &shy);
    if (r != s) {
        mismatch_ret++;
    } else if (r) {
        hits++;
        /* Bit patterns, not tolerances: the claim is equality. */
        if (bits(rhx) != bits(shx) || bits(rhy) != bits(shy)) {
            mismatch_hit++;
        }
    }
    if ((mismatch_ret + mismatch_hit) && !have_bad) {
        have_bad = 1;
        first_bad[0] = px; first_bad[1] = py; first_bad[2] = tx;
        first_bad[3] = ty; first_bad[4] = ax; first_bad[5] = ay;
        first_bad[6] = bx; first_bad[7] = by; first_bad[8] = ud;
    }
}

int main(void)
{
    /* Dream Land's own scale, plus the degenerate values a sign-flip rewrite
     * would break on if it broke anywhere. */
    static const float coords[] = {
        -0.0f, 0.0f, 1e-7f, -1e-7f, 0.0005f, -0.0005f, 0.001f, -0.001f,
        0.5f, -0.5f, 1.0f, -1.0f, 7.5f, -7.5f, 60.0f, -60.0f,
        180.0f, -180.0f, 1200.0f, -1200.0f
    };
    static const float seg[] = {
        -180.0f, -60.0f, -7.5f, -0.001f, 0.0f, 0.001f, 7.5f, 60.0f, 180.0f
    };
    size_t i, j, k, l, m;
    int ud;

    for (ud = -1; ud <= 1; ud += 2) {
        for (i = 0; i < sizeof(seg) / sizeof(seg[0]); i++) {
            for (j = 0; j < sizeof(seg) / sizeof(seg[0]); j++) {
                for (k = 0; k < sizeof(seg) / sizeof(seg[0]); k++) {
                    for (l = 0; l < sizeof(coords) / sizeof(coords[0]); l++) {
                        for (m = 0; m < sizeof(coords) / sizeof(coords[0]); m++) {
                            float ax = seg[i], ay = seg[j];
                            float bx = seg[k], by = seg[(j + k) % 9];
                            float px = coords[l], py = coords[m];
                            /* four motion vectors per configuration: through,
                             * away, parallel, and stationary */
                            probe(px, py, px - 1.0f, py - 3.0f, ax, ay, bx, by, ud);
                            probe(px, py, px + 1.0f, py + 3.0f, ax, ay, bx, by, ud);
                            probe(px, py, px + (bx - ax), py + (by - ay),
                                  ax, ay, bx, by, ud);
                            probe(px, py, px, py, ax, ay, bx, by, ud);
                        }
                    }
                }
            }
        }
    }
    printf("cases %lld hits %lld ret_mismatch %lld hit_mismatch %lld\n",
           cases, hits, mismatch_ret, mismatch_hit);
    if (have_bad) {
        printf("first bad: p=(%g,%g) t=(%g,%g) v1=(%g,%g) v2=(%g,%g) ud=%g\n",
               first_bad[0], first_bad[1], first_bad[2], first_bad[3],
               first_bad[4], first_bad[5], first_bad[6], first_bad[7],
               first_bad[8]);
    }
    return (mismatch_ret + mismatch_hit) ? 1 : 0;
}
"""


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args()

    compiler = None
    for candidate in ("gcc", "cc", "clang"):
        found = shutil.which(candidate)
        if found:
            compiler = found
            break
    if compiler is None:
        sys.exit("FAIL: no host C compiler (gcc/cc/clang) on PATH.")

    shipped = inline_header(HEADER, drop_includes=True)
    if "ndsMPFCSegmentCrossesKernel" not in shipped:
        sys.exit("FAIL: %s no longer defines the kernel." % HEADER)
    fcmp = inline_header(FCMP, drop_includes=True)
    if "ndsFcmpGtC" not in fcmp:
        sys.exit("FAIL: %s no longer defines the fcmp predicates." % FCMP)

    source = HARNESS.replace("__FCMP__", fcmp).replace("__SHIPPED__", shipped)
    workdir = tempfile.mkdtemp(prefix="mpfc-")
    try:
        csrc = os.path.join(workdir, "mpfc.c")
        exe = os.path.join(workdir, "mpfc.exe")
        with open(csrc, "w", encoding="utf-8") as handle:
            handle.write(source)
        build = subprocess.run(
            [compiler, "-O1", "-fno-fast-math", "-fexcess-precision=standard",
             "-o", exe, csrc, "-lm"],
            capture_output=True, text=True,
        )
        if build.returncode != 0:
            sys.stderr.write(build.stdout + build.stderr)
            sys.exit("FAIL: host build of the extracted kernel failed.")
        run = subprocess.run([exe], capture_output=True, text=True)
    finally:
        shutil.rmtree(workdir, ignore_errors=True)

    print("MP floor-crossing kernel -- shipped vs the pre-cycle-117 float body")
    print("  " + run.stdout.strip().replace("\n", "\n  "))
    if run.returncode != 0:
        print("RED: the rewrite is NOT bit-identical.")
        return 1
    print("GREEN: every case returns the same verdict and the same hit "
          "BIT PATTERNS.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
