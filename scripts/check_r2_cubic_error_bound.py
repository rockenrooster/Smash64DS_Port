#!/usr/bin/env python3
"""Bound the error the fixed-point animation path introduces into joint values.

R2-03 E64b replaced `gcGetInterpValueCubic`'s single-precision evaluation with a
Q12 one. That is authorized (`PROJECT_GOAL.md` requires mechanical equivalence,
not bit exactness) but it was graduated with its numerical deviation UNMEASURED,
and the instrument `KNOWN_ISSUES.md` named for it -- the Task 9 state hash -- is
a bit-exactness hash. It *cannot* pass a deliberately non-bit-exact change, so it
would only ever report "differs", which carries no information about whether
gameplay moved.

The right instrument for a non-bit-exact change is an error bound. This is it.

It extracts the shipped kernels out of `src/import/battleship_sys_objanim.c`
between the `NDS_R2_CUBIC_FIXED_KERNEL_BEGIN/END` markers, prepends
`include/nds/nds_anim_fixed.h`, takes the reference out of the decomp verbatim,
compiles them on the host, and sweeps the input domain the animation data
actually produces. Extraction, not a copy, so the bound is always measured
against the code that ships.

Cycle 116 (Requirement 4) added a second candidate: `ndsR2AnimValueQ`, which
reads an AObj that is ALREADY fixed point, and the parser half that writes it.
The parser half is checked EXHAUSTIVELY rather than bounded -- over all 65,536
s16 arguments on each of the six power-of-two tracks, `ndsR2AnimArgToQ` must
produce the same Q12 integer the shipped float path produces by converting
`arg * 2^-k`. If that holds, the Q form's four value inputs are not an
approximation at all, and the only thing left to bound is `t` and `length`.

The host models the N64 faithfully for this purpose: both are IEEE-754 single
precision, round-to-nearest.

Usage:
    python scripts/check_r2_cubic_error_bound.py [--json PATH] [--verbose]
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
KERNEL_SRC = os.path.join(ROOT, "src", "import", "battleship_sys_objanim.c")
DECOMP_SRC = os.path.join(
    ROOT, "decomp", "BattleShip-main", "decomp", "src", "sys", "objanim.c"
)
ANIM_HEADER = os.path.join(ROOT, "include", "nds", "nds_anim_fixed.h")
FTANIM_PORT = os.path.join(ROOT, "src", "import", "battleship_ftanim.c")
FTANIM_DECOMP = os.path.join(
    ROOT, "decomp", "BattleShip-main", "decomp", "src", "ft", "ftanim.c"
)
BEGIN = "NDS_R2_CUBIC_FIXED_KERNEL_BEGIN"
END = "NDS_R2_CUBIC_FIXED_KERNEL_END"

# The gate. A joint value reaches gameplay only through
# gmCollisionGetFighterPartsWorldPosition (gm/gmcollision.c:489), which places
# hitboxes in world units. Fighter hitbox radii on Dream Land are single-digit
# world units and the smallest gameplay-relevant separation is well above 0.1,
# so a deviation under 0.02 world units cannot flip a hit decision. Rotations
# are radians: 0.02 rad is 1.1 degrees at a joint, and the arm length that
# amplifies it is a few units, so it lands in the same place.
BOUND_ABS = 0.02


def extract_kernel() -> str:
    text = open(KERNEL_SRC, encoding="utf-8", errors="replace").read()
    begin = text.find(BEGIN)
    end = text.find(END)
    if begin < 0 or end < 0:
        sys.exit(
            "FAIL: kernel markers missing from %s. The extraction fence is how "
            "this checker stays honest -- restore %s / %s rather than deleting "
            "the checker." % (KERNEL_SRC, BEGIN, END)
        )
    # The BEGIN marker sits inside a comment block; start after that block
    # closes, or the remaining prose leaks in as code.
    body = text[text.index("*/", begin) + 2 : text.rfind("/*", begin, end)]
    for name in ("ndsR2CubicValueFixed", "ndsR2AnimValueQ"):
        if name not in body:
            sys.exit("FAIL: extracted region does not contain %s." % name)
    return body


def extract_header() -> str:
    """`include/nds/nds_anim_fixed.h`, inlined.

    The header is the shipped definition of every Q scale, kind and helper, so
    the harness must compile the real file rather than a transcription. Only the
    include guard and the `extern` on the saturation counter come out -- the
    harness owns that symbol.
    """
    text = open(ANIM_HEADER, encoding="utf-8", errors="replace").read()
    out = []
    for line in text.splitlines():
        stripped = line.strip()
        if stripped.startswith("#ifndef NDS_ANIM_FIXED_H"):
            continue
        if stripped.startswith("#define NDS_ANIM_FIXED_H"):
            continue
        if stripped.startswith("#endif /* NDS_ANIM_FIXED_H */"):
            continue
        if stripped.startswith("extern volatile u32 gNdsR2CubicSaturations"):
            continue
        out.append(line)
    body = "\n".join(out)
    for name in ("ndsR2AQLoad", "ndsR2AQStore", "ndsR2AnimArgToQ",
                 "ndsR2F32ToFixed", "NDS_R2_AQ_IF"):
        if name not in body:
            sys.exit("FAIL: %s missing from %s." % (name, ANIM_HEADER))
    return body


def extract_frac_tables():
    """The decomp's `fracs[]` and the port's `sNdsR2AnimFracShift[]`, read from
    source. Hardcoding either here would let the shipped table drift away from
    the one this checker proves exhaustive agreement against, which is the only
    way this check could silently stop meaning anything."""
    decomp = open(FTANIM_DECOMP, encoding="utf-8", errors="replace").read()
    match = re.search(r"f32 fracs\[[^\]]*\]\s*=\s*\{(.*?)\};", decomp, re.S)
    if not match:
        sys.exit("FAIL: fracs[] not found in %s." % FTANIM_DECOMP)
    fracs = [
        re.sub(r"//.*", "", entry).strip()
        for entry in match.group(1).split(",")
    ]
    fracs = [entry for entry in fracs if entry]
    if len(fracs) != 8:
        sys.exit("FAIL: fracs[] has %d entries, expected 8." % len(fracs))

    port = open(FTANIM_PORT, encoding="utf-8", errors="replace").read()
    match = re.search(
        r"sNdsR2AnimFracShift\[8\]\s*=\s*\{([^}]*)\}", port
    )
    if not match:
        sys.exit("FAIL: sNdsR2AnimFracShift not found in %s." % FTANIM_PORT)
    shifts = [
        int(entry.strip().rstrip("u"))
        for entry in match.group(1).split(",") if entry.strip()
    ]
    if len(shifts) != 8:
        sys.exit("FAIL: sNdsR2AnimFracShift has %d entries." % len(shifts))
    return fracs, shifts


def extract_reference() -> str:
    text = open(DECOMP_SRC, encoding="utf-8", errors="replace").read()
    match = re.search(
        r"^f32 gcGetInterpValueCubic\(.*?^\}", text, re.S | re.M
    )
    if not match:
        sys.exit("FAIL: gcGetInterpValueCubic not found in %s." % DECOMP_SRC)
    return match.group(0).replace("SQUARE(length_invert)", "(length_invert * length_invert)").replace(
        "SQUARE(length)", "(length * length)"
    )


HARNESS = r"""
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef float f32;
typedef unsigned int u32;
typedef unsigned char u8;
typedef int s32;
typedef long long s64;
typedef int sb32;

/* Only the fields the kernels read, plus `kind`, which is the Q discriminator.
 * Field names must match the decomp's AObj or the extracted kernels will not
 * compile, which is the point. */
typedef struct AObj {
    f32 length_invert;
    f32 length;
    f32 value_base;
    f32 value_target;
    f32 rate_base;
    f32 rate_target;
    u8 kind;
} AObj;

static unsigned int gNdsR2CubicEvals;
static volatile u32 gNdsR2CubicSaturations;

/* The standing-rule-7 A/B route gate the kernels read. Defined the way a
 * PUBLISHED build defines it -- constant 1 -- so the bound is measured on the
 * FUSED path, the only arm whose error needs bounding: route 0 is the decomp's
 * own float expression, whose deviation from itself is zero by construction. */
#define NDS_R2_ANIM_CUT_ON(bit) (1)

/* ---- include/nds/nds_anim_fixed.h, inlined from disk ---- */
__HEADER__

/* ---- reference, verbatim from the decomp ---- */
__REFERENCE__

/* ---- candidates, extracted from the shipped TU ---- */
__KERNEL__

/* The decomp's own `fracs[]` and the port's `sNdsR2AnimFracShift[]`, both read
 * out of source by the driver rather than retyped here. */
static const f32 FRACS[8] = { __FRACS__ };
static const int FRAC_SHIFT[8] = { __FRACSHIFT__ };

/* Worst observed deviation, and the inputs that produced it. */
static double worst_abs;
static AObj worst_at;
static double worst_slope;      /* L*max|rate| at the worst case: the amplifier */
static double sum_sq;
static double sum_signed;
static long long samples;
static long long over_bound;
static long long saturating;
static int use_q;               /* 0 = E64's float-fed kernel, 1 = the Q one */

static void reset_stats(void)
{
    worst_abs = 0.0;
    worst_slope = 0.0;
    sum_sq = 0.0;
    sum_signed = 0.0;
    samples = 0;
    over_bound = 0;
    saturating = 0;
    memset(&worst_at, 0, sizeof(worst_at));
}

static void account(f32 ref, f32 got, const AObj *at, double slope,
                    unsigned int sat_before)
{
    double err;

    if (!isfinite((double)ref)) {
        return;
    }
    if (gNdsR2CubicSaturations != sat_before) {
        saturating++;
        return;             /* saturation is its own report, not an error bar */
    }
    err = (double)got - (double)ref;
    samples++;
    sum_sq += err * err;
    sum_signed += err;
    if (fabs(err) > __BOUND__) {
        over_bound++;
    }
    if (fabs(err) > worst_abs) {
        worst_abs = fabs(err);
        worst_at = *at;
        worst_slope = slope;
    }
}

static void probe(f32 li, f32 L, f32 vb, f32 vt, f32 rb, f32 rt)
{
    AObj a;
    AObj shown;
    f32 ref, got;
    unsigned int sat_before = gNdsR2CubicSaturations;
    double slope = fabs((double)rb) > fabs((double)rt) ?
        fabs((double)rb) : fabs((double)rt);

    shown.length_invert = li;
    shown.length = L;
    shown.value_base = vb;
    shown.value_target = vt;
    shown.rate_base = rb;
    shown.rate_target = rt;
    shown.kind = 0;

    ref = gcGetInterpValueCubic(li, L, vb, vt, rb, rt);
    if (use_q) {
        /* Built the way the Q PARSER builds it: a Q30 reciprocal, Q12 frames,
         * Q12 values. This measures the STORAGE change and the kernel together,
         * which is the only honest way to price a representation change. */
        a.kind = (u8)NDS_R2_AQ_KIND_CUBIC;
        a.length_invert = ndsR2AQStore(ndsR2F32ToFixed(li, NDS_R2_AQ_IF));
        a.length = ndsR2AQStore(ndsR2F32ToFixed(L, NDS_R2_AQ_LF));
        a.value_base = ndsR2AQStore(ndsR2F32ToFixed(vb, NDS_R2_AQ_VF));
        a.value_target = ndsR2AQStore(ndsR2F32ToFixed(vt, NDS_R2_AQ_VF));
        a.rate_base = ndsR2AQStore(ndsR2F32ToFixed(rb, NDS_R2_AQ_VF));
        a.rate_target = ndsR2AQStore(ndsR2F32ToFixed(rt, NDS_R2_AQ_VF));
        got = ndsR2AnimValueQ(&a);
    } else {
        a = shown;
        got = ndsR2CubicValueFixed(&a);
    }
    account(ref, got, &shown, slope * (double)L, sat_before);
}

/* Block lengths the animation data uses. `length_invert = 1/payload` and
 * `length` walks 0..payload by `anim_speed` (1.0 for every node E61's census
 * saw), so t sweeps [0,1] in payload steps. 90 is past anything measured; it is
 * there to show how the bound scales with L. */
static const int payloads[] = {1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 20, 30, 45, 60, 90};

/* The error scales with L*|rate|, which is the curve's own steepness in value
 * units per t. The Q12 basis functions have a 1/4096 quantum, so the deviation
 * is roughly (L*|rate|)/4096 -- which is why the domain has to be stated rather
 * than assumed: a 600-unit swing crossed in 15 frames deviates 100x as much as a
 * one-radian joint rotation, from identical arithmetic. */
static void sweep(const float *mags, size_t mag_count,
                  const float *rate_mults, size_t rate_count)
{
    size_t pi, bi, ti, ri, rj;
    int step, sb, st;

    reset_stats();
    for (pi = 0; pi < sizeof(payloads) / sizeof(payloads[0]); pi++) {
        int payload = payloads[pi];
        float li = 1.0f / (float)payload;
        for (bi = 0; bi < mag_count; bi++) {
            for (ti = 0; ti < mag_count; ti++) {
                for (sb = 0; sb < 2; sb++) {
                    for (st = 0; st < 2; st++) {
                        float vb = sb ? -mags[bi] : mags[bi];
                        float vt = st ? -mags[ti] : mags[ti];
                        float chord = (vt - vb) * li;
                        for (ri = 0; ri < rate_count; ri++) {
                            for (rj = 0; rj < rate_count; rj++) {
                                float rb = chord * rate_mults[ri];
                                float rt = chord * rate_mults[rj];
                                for (step = 0; step <= payload; step++) {
                                    probe(li, (float)step, vb, vt, rb, rt);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

/* Requirement 4's Linear arm. The float parser divides `(vt-vb)` by the payload
 * in f32 and stores the quotient; the Q parser divides the two Q12 integers and
 * rounds the magnitude. Both are then evaluated at every step of the block, so
 * this bounds the rounding of the stored rate amplified by `length`, which is
 * where a Linear slip would actually show. */
static void sweep_linear(const float *mags, size_t mag_count)
{
    size_t bi, ti, pi;
    int step, sb, st;

    reset_stats();
    for (pi = 0; pi < sizeof(payloads) / sizeof(payloads[0]); pi++) {
        int payload = payloads[pi];
        for (bi = 0; bi < mag_count; bi++) {
            for (ti = 0; ti < mag_count; ti++) {
                for (sb = 0; sb < 2; sb++) {
                    for (st = 0; st < 2; st++) {
                        float vb = sb ? -mags[bi] : mags[bi];
                        float vt = st ? -mags[ti] : mags[ti];
                        float rate = (vt - vb) / (float)payload;
                        s32 qvb = ndsR2F32ToFixed(vb, NDS_R2_AQ_VF);
                        s32 qvt = ndsR2F32ToFixed(vt, NDS_R2_AQ_VF);
                        s32 d = (qvt - qvb) << (NDS_R2_AQ_RF - NDS_R2_AQ_VF);
                        u32 h = (u32)payload >> 1;
                        s32 qr = (d < 0) ?
                            -(s32)(((u32)(-d) + h) / (u32)payload) :
                             (s32)(((u32)d + h) / (u32)payload);
                        for (step = 0; step <= payload; step++) {
                            AObj a;
                            AObj shown;
                            unsigned int sat_before = gNdsR2CubicSaturations;
                            f32 L = (f32)step;
                            f32 ref = vb + (L * rate);

                            shown.length_invert = 1.0f / (float)payload;
                            shown.length = L;
                            shown.value_base = vb;
                            shown.value_target = vt;
                            shown.rate_base = rate;
                            shown.rate_target = 0.0f;
                            shown.kind = 0;

                            a = shown;
                            a.kind = (u8)NDS_R2_AQ_KIND_LINEAR;
                            a.length = ndsR2AQStore(
                                ndsR2F32ToFixed(L, NDS_R2_AQ_LF));
                            a.value_base = ndsR2AQStore(qvb);
                            a.value_target = ndsR2AQStore(qvt);
                            a.rate_base = ndsR2AQStore(qr);
                            a.rate_target = ndsR2AQStore(0);
                            a.length_invert = ndsR2AQStore(0);
                            account(ref, ndsR2AnimValueQ(&a), &shown,
                                    fabs((double)rate) * (double)L, sat_before);
                        }
                    }
                }
            }
        }
    }
}

/* Step selects one of two stored values on a frame-count compare. Both sides
 * are exact integers in Q form, so this is not a bound so much as a proof that
 * the field's second meaning survived the format change. */
static long long step_mismatches(const float *mags, size_t mag_count,
                                 long long *total)
{
    size_t bi, ti, pi;
    int step, sb, st;
    long long bad = 0;

    *total = 0;

    for (pi = 0; pi < sizeof(payloads) / sizeof(payloads[0]); pi++) {
        int payload = payloads[pi];
        for (bi = 0; bi < mag_count; bi++) {
            for (ti = 0; ti < mag_count; ti++) {
                for (sb = 0; sb < 2; sb++) {
                    for (st = 0; st < 2; st++) {
                        float vb = sb ? -mags[bi] : mags[bi];
                        float vt = st ? -mags[ti] : mags[ti];
                        for (step = 0; step <= payload + 1; step++) {
                            AObj a;
                            f32 L = (f32)step;
                            f32 ref = ((float)payload <= L) ? vt : vb;
                            f32 refq = ndsR2FixedToF32(
                                ndsR2F32ToFixed(ref, NDS_R2_AQ_VF),
                                NDS_R2_AQ_VF);

                            a.kind = (u8)NDS_R2_AQ_KIND_STEP;
                            a.length_invert =
                                ndsR2AQStore(payload << NDS_R2_AQ_LF);
                            a.length = ndsR2AQStore(
                                ndsR2F32ToFixed(L, NDS_R2_AQ_LF));
                            a.value_base = ndsR2AQStore(
                                ndsR2F32ToFixed(vb, NDS_R2_AQ_VF));
                            a.value_target = ndsR2AQStore(
                                ndsR2F32ToFixed(vt, NDS_R2_AQ_VF));
                            a.rate_base = ndsR2AQStore(0);
                            a.rate_target = ndsR2AQStore(0);
                            (*total)++;
                            if (ndsR2AnimValueQ(&a) != refq) {
                                bad++;
                            }
                        }
                    }
                }
            }
        }
    }
    return bad;
}

/* The parser half, EXHAUSTIVELY. For every s16 argument on each of the six
 * power-of-two tracks, the Q parser's `arg << (12-k)` must equal the Q12
 * integer the shipped float path reaches by converting `arg * 2^-k`. If this is
 * zero, the four value inputs to the cubic did not move at all and the sweeps
 * above are bounding `t` and `length` alone. */
static long long parser_mismatches(void)
{
    long long bad = 0;
    int id;
    long arg;

    for (id = 0; id < 8; id++) {
        if ((id == 3) || (id == 7)) {
            continue;       /* not a power of two: keeps the float expression */
        }
        for (arg = -32768; arg <= 32767; arg++) {
            s32 viaf = ndsR2F32ToFixed((f32)arg * FRACS[id], NDS_R2_AQ_VF);
            s32 viaq = ndsR2AnimArgToQ((s32)arg,
                                       NDS_R2_AQ_VF - FRAC_SHIFT[id]);

            if (viaf != viaq) {
                bad++;
            }
        }
    }
    return bad;
}

static void report(const char *name, int gated)
{
    printf("  {\"domain\": \"%s\", \"gated\": %d,\n", name, gated);
    printf("   \"samples\": %lld,\n", samples);
    printf("   \"max_abs_error\": %.9g,\n", worst_abs);
    printf("   \"rms_error\": %.9g,\n",
           samples ? sqrt(sum_sq / (double)samples) : 0.0);
    printf("   \"mean_signed_error\": %.9g,\n",
           samples ? sum_signed / (double)samples : 0.0);
    printf("   \"over_bound\": %lld,\n", over_bound);
    printf("   \"saturating_inputs\": %lld,\n", saturating);
    printf("   \"worst_steepness\": %.9g,\n", worst_slope);
    printf("   \"worst_at\": {\"length_invert\": %.9g, \"length\": %.9g, "
           "\"value_base\": %.9g, \"value_target\": %.9g, "
           "\"rate_base\": %.9g, \"rate_target\": %.9g}}",
           (double)worst_at.length_invert, (double)worst_at.length,
           (double)worst_at.value_base, (double)worst_at.value_target,
           (double)worst_at.rate_base, (double)worst_at.rate_target);
}

static void report_exact(const char *name, long long bad, long long total)
{
    printf("  {\"domain\": \"%s\", \"gated\": 1,\n", name);
    printf("   \"samples\": %lld,\n", total);
    printf("   \"max_abs_error\": 0,\n");
    printf("   \"rms_error\": 0,\n");
    printf("   \"mean_signed_error\": 0,\n");
    printf("   \"over_bound\": %lld,\n", bad);
    printf("   \"saturating_inputs\": 0,\n");
    printf("   \"worst_steepness\": 0,\n");
    printf("   \"worst_at\": {\"length_invert\": 0, \"length\": 0, "
           "\"value_base\": 0, \"value_target\": 0, "
           "\"rate_base\": 0, \"rate_target\": 0}}");
}

int main(void)
{
    /* Rotation tracks. RotX/RotY/RotZ are radians, so a full turn is the
     * magnitude ceiling and the ones below it are the joint deflections a
     * fighter animation actually uses. */
    static const float rot_mags[] = {0.0f, 0.05f, 0.5f, 1.0f, 3.14159265f, 6.2831853f};
    /* Translation and scale tracks, in world units. A fighter's furthest bone
     * offset from its root is tens of units; 60 covers it with room. */
    static const float tra_mags[] = {0.0f, 0.05f, 0.5f, 1.0f, 5.0f, 20.0f, 60.0f};
    /* Conservative: a 300-unit endpoint is past anything in the data and is
     * here only to expose the scaling law. */
    static const float wide_mags[] = {0.0f, 1.0f, 10.0f, 60.0f, 300.0f};
    /* Rate multipliers, as a factor of the chord slope (vt-vb)/L. An exported
     * smooth curve has these near 1; 2 already overshoots. */
    static const float rate_mults[] = {0.0f, 0.5f, 1.0f, -1.0f, 2.0f, -2.0f};
    /* Conservative: 4x the chord is a rate no exported animation should hold. */
    static const float wide_rates[] = {0.0f, 1.0f, -1.0f, 2.0f, -2.0f, 4.0f, -4.0f};
    long long step_bad;
    long long step_total = 0;

    printf("[\n");
    for (use_q = 0; use_q < 2; use_q++) {
        sweep(rot_mags, sizeof(rot_mags) / sizeof(rot_mags[0]),
              rate_mults, sizeof(rate_mults) / sizeof(rate_mults[0]));
        report(use_q ? "rotation-Q" : "rotation", 1);
        printf(",\n");
        sweep(tra_mags, sizeof(tra_mags) / sizeof(tra_mags[0]),
              rate_mults, sizeof(rate_mults) / sizeof(rate_mults[0]));
        report(use_q ? "translation-Q" : "translation", 1);
        printf(",\n");
        sweep(wide_mags, sizeof(wide_mags) / sizeof(wide_mags[0]),
              wide_rates, sizeof(wide_rates) / sizeof(wide_rates[0]));
        report(use_q ? "conservative-Q" : "conservative", 0);
        printf(",\n");
    }
    sweep_linear(tra_mags, sizeof(tra_mags) / sizeof(tra_mags[0]));
    report("linear-Q", 1);
    printf(",\n");
    step_bad = step_mismatches(tra_mags, sizeof(tra_mags) / sizeof(tra_mags[0]),
                               &step_total);
    report_exact("step-Q-exact", step_bad, step_total);
    printf(",\n");
    report_exact("parser-Q-exact", parser_mismatches(), 6LL * 65536LL);
    printf("\n]\n");
    return 0;
}
"""


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--json", dest="json_path", default=None)
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

    fracs, frac_shift = extract_frac_tables()
    source = (
        HARNESS.replace("__HEADER__", extract_header())
        .replace("__REFERENCE__", extract_reference())
        .replace("__KERNEL__", extract_kernel())
        .replace("__FRACS__", ", ".join(fracs))
        .replace("__FRACSHIFT__", ", ".join(str(v) for v in frac_shift))
        .replace("__BOUND__", repr(BOUND_ABS))
    )

    workdir = tempfile.mkdtemp(prefix="r2cubic-")
    try:
        csrc = os.path.join(workdir, "bound.c")
        exe = os.path.join(workdir, "bound.exe")
        with open(csrc, "w", encoding="utf-8") as handle:
            handle.write(source)
        # -ffloat-store and no fast-math: the reference must round exactly like
        # single-precision hardware, which is what the N64 did.
        build = subprocess.run(
            [compiler, "-O1", "-fno-fast-math", "-fexcess-precision=standard",
             "-o", exe, csrc, "-lm"],
            capture_output=True, text=True,
        )
        if build.returncode != 0:
            sys.stderr.write(build.stdout + build.stderr)
            sys.exit("FAIL: host build of the extracted kernel failed.")
        run = subprocess.run([exe], capture_output=True, text=True)
        if run.returncode != 0:
            sys.stderr.write(run.stdout + run.stderr)
            sys.exit("FAIL: sweep did not run.")
        report = json.loads(run.stdout)
    finally:
        shutil.rmtree(workdir, ignore_errors=True)

    if args.json_path:
        with open(args.json_path, "w", encoding="utf-8") as handle:
            json.dump(report, handle, indent=2)
            handle.write("\n")

    print("R2-03 E64b/E65 fixed-point cubic -- host error bound vs the decomp "
          "float")
    print("  gameplay bound %.4f world units / radians" % BOUND_ABS)
    print("")
    print("  %-13s %9s %10s %10s %10s %9s %s"
          % ("domain", "samples", "max|err|", "rms", "bias", "steepness",
             "gate"))
    failed = []
    for row in report:
        print("  %-13s %9d %10.6f %10.6f %+10.6f %9.1f %s"
              % (row["domain"], row["samples"], row["max_abs_error"],
                 row["rms_error"], row["mean_signed_error"],
                 row["worst_steepness"],
                 ("RED" if row["over_bound"] else "green") if row["gated"]
                 else "(reported)"))
        if row["gated"] and row["over_bound"]:
            failed.append(row)
        if args.verbose or (row["gated"] and row["over_bound"]):
            worst = row["worst_at"]
            print("      worst at L=%g 1/L=%g vb=%g vt=%g rb=%g rt=%g; "
                  "%d over bound, %d saturating"
                  % (worst["length"], worst["length_invert"],
                     worst["value_base"], worst["value_target"],
                     worst["rate_base"], worst["rate_target"],
                     row["over_bound"], row["saturating_inputs"]))
    print("")
    print("  'steepness' is L*max|rate| at the worst case -- the amplifier on the")
    print("  Q12 basis quantum, and the whole reason the domain is stated.")

    if failed:
        print("RED: %s deviate past %.4f."
              % (", ".join(r["domain"] for r in failed), BOUND_ABS))
        return 1
    print("GREEN: every gated domain stays inside %.4f. Worst gated deviation "
          "%.6f." % (BOUND_ABS,
                     max(r["max_abs_error"] for r in report if r["gated"])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
