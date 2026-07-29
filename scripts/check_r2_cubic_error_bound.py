#!/usr/bin/env python3
"""Bound the error E64b's fixed-point cubic introduces into joint values.

R2-03 E64b replaced `gcGetInterpValueCubic`'s single-precision evaluation with a
Q12 one. That is authorized (`PROJECT_GOAL.md` requires mechanical equivalence,
not bit exactness) but it was graduated with its numerical deviation UNMEASURED,
and the instrument `KNOWN_ISSUES.md` named for it -- the Task 9 state hash -- is
a bit-exactness hash. It *cannot* pass a deliberately non-bit-exact change, so it
would only ever report "differs", which carries no information about whether
gameplay moved.

The right instrument for a non-bit-exact change is an error bound. This is it.

It extracts the shipped kernel out of `src/import/battleship_sys_objanim.c`
between the `NDS_R2_CUBIC_FIXED_KERNEL_BEGIN/END` markers and the reference out
of the decomp verbatim, compiles both on the host, and sweeps the input domain
the animation data actually produces. Extraction, not a copy, so the bound is
always measured against the code that ships.

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
    if "ndsR2CubicValueFixed" not in body:
        sys.exit("FAIL: extracted region does not contain ndsR2CubicValueFixed.")
    return body


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
typedef int s32;
typedef long long s64;

/* Only the six fields the kernel reads. Field names must match the decomp's
 * AObj or the extracted kernel will not compile, which is the point. */
typedef struct AObj {
    f32 length_invert;
    f32 length;
    f32 value_base;
    f32 value_target;
    f32 rate_base;
    f32 rate_target;
} AObj;

static unsigned int gNdsR2CubicEvals;
static unsigned int gNdsR2CubicSaturations;

/* ---- reference, verbatim from the decomp ---- */
__REFERENCE__

/* ---- candidate, extracted from the shipped TU ---- */
__KERNEL__

/* Worst observed deviation, and the inputs that produced it. */
static double worst_abs;
static AObj worst_at;
static double worst_slope;      /* L*max|rate| at the worst case: the amplifier */
static double sum_sq;
static double sum_signed;
static long long samples;
static long long over_bound;
static long long saturating;

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

static void probe(f32 li, f32 L, f32 vb, f32 vt, f32 rb, f32 rt)
{
    AObj a;
    f32 ref, got;
    double err;
    unsigned int sat_before = gNdsR2CubicSaturations;

    a.length_invert = li;
    a.length = L;
    a.value_base = vb;
    a.value_target = vt;
    a.rate_base = rb;
    a.rate_target = rt;

    ref = gcGetInterpValueCubic(li, L, vb, vt, rb, rt);
    got = ndsR2CubicValueFixed(&a);
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
        double slope = fabs((double)rb) > fabs((double)rt) ?
            fabs((double)rb) : fabs((double)rt);
        worst_abs = fabs(err);
        worst_at = a;
        worst_slope = slope * (double)L;
    }
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

    printf("[\n");
    sweep(rot_mags, sizeof(rot_mags) / sizeof(rot_mags[0]),
          rate_mults, sizeof(rate_mults) / sizeof(rate_mults[0]));
    report("rotation", 1);
    printf(",\n");
    sweep(tra_mags, sizeof(tra_mags) / sizeof(tra_mags[0]),
          rate_mults, sizeof(rate_mults) / sizeof(rate_mults[0]));
    report("translation", 1);
    printf(",\n");
    sweep(wide_mags, sizeof(wide_mags) / sizeof(wide_mags[0]),
          wide_rates, sizeof(wide_rates) / sizeof(wide_rates[0]));
    report("conservative", 0);
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

    source = (
        HARNESS.replace("__REFERENCE__", extract_reference())
        .replace("__KERNEL__", extract_kernel())
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
