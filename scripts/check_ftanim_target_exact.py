#!/usr/bin/env python3
"""Prove the port parser's `ftAnimGetTargetValue` replacement is BIT-EXACT.

`ftAnimGetTargetValue` is `(f32)arg * fracs[id]` -- an `__aeabi_i2f` followed by
an `__aeabi_fmul`, inlined at twelve sites inside
`ftAnimParseDObjFigatree`. Six of the eight `fracs` entries are exact powers of
two, so for those the whole thing is an exponent subtraction on a value that
always fits f32's mantissa. `src/import/battleship_ftanim.c` does exactly that.

"Exact" is a strong claim about animation data that drives hitboxes, so it is
proven here over EVERY s16 for every power-of-two track id rather than argued
from the comment. The two non-power-of-two ids (TraI, `1/16384 - 3e-12`) keep the
original expression in the shipped code and are checked here too -- they must
come out bit-identical because that arm is textually unchanged.

Extraction, not a copy: the candidate is lifted out of the shipped TU, so this
cannot drift away from the code that ships.

SCOPE, and why it is not a narrowing. `69ce92e279f` (Requirement 4) gave
`ndsR2AnimTargetValue` a second output representation behind a `q` argument, and
the extracted body has carried its `q != 0u` branches ever since -- which is what
broke this checker: the lifted tail stopped compiling against a harness that
knows nothing about them. This file covers the FLOAT route, `q == 0`, which is
the route that ships (`NDS_R2_CUBIC_FIXED ?= 0`, `Makefile:455`) and is exactly
the claim above. The Q route is already proven exhaustively, over the same
65,536 s16 on the same six power-of-two tracks, by
`check_r2_cubic_error_bound.py` -- which is wired into
`check-gbi-decode-fixtures.ps1` -- so duplicating it here would buy nothing. The
Q helpers are therefore linked in as POISONED stubs: if `q == 0` ever reaches
one, this checker aborts instead of quietly comparing something else.

Usage:
    python scripts/check_ftanim_target_exact.py [--verbose]
"""

import argparse
import os
import re
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src", "import", "battleship_ftanim.c")

HARNESS = r"""
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef float f32;
typedef unsigned int u32;
typedef int s32;
typedef short s16;
typedef unsigned char u8;
typedef int sb32;

/* The shipped body's `q != 0u` arms, present but unreachable at q == 0. They are
 * poisoned rather than emulated: reaching one would mean this checker is no
 * longer measuring the float route it claims to measure. The Q route itself is
 * proven by check_r2_cubic_error_bound.py, not here. */
#define NDS_R2_AQ_VF 12
static f32 ndsR2AQStore(s32 v)
{
    (void)v;
    fprintf(stderr, "RED: ndsR2AQStore reached at q == 0 -- the extracted "
                    "float route is not the float route.\n");
    exit(2);
}
static s32 ndsR2F32ToFixed(f32 v, s32 bits)
{
    (void)v; (void)bits;
    fprintf(stderr, "RED: ndsR2F32ToFixed reached at q == 0.\n");
    exit(2);
}
static s32 ndsR2AnimArgToQ(s32 arg, s32 shift)
{
    (void)arg; (void)shift;
    fprintf(stderr, "RED: ndsR2AnimArgToQ reached at q == 0.\n");
    exit(2);
}

/* The decomp's own table, verbatim from ftanim.c:28-38. */
static f32 fracs[8] = {
    1.0F / 512.0F,
    1.0F / 4.0F,
    1.0F / 4096.0F,
    1.0F / 16384.0F - (3.0F / 1000000000000.0F),
    1.0F / 512.0F,
    1.0F / 32.0F,
    1.0F / 8192.0F,
    1.0F / 16384.0F - (3.0F / 1000000000000.0F)
};

static f32 reference(s16 arg, int id)
{
    f32 ret = arg;              /* __aeabi_i2f  */
    ret *= fracs[id];           /* __aeabi_fmul */
    return ret;
}

/* ---- candidate, extracted from the shipped TU ---- */
__CANDIDATE__

int main(void)
{
    long long checked = 0, mismatched = 0;
    int id;
    long a;

    for (id = 0; id < 8; id++) {
        for (a = -32768; a <= 32767; a++) {
            s16 arg = (s16)a;
            f32 want = reference(arg, id);
            f32 got = candidate(arg, id, 0u);   /* the shipping route */
            u32 wb, gb;
            memcpy(&wb, &want, 4);
            memcpy(&gb, &got, 4);
            checked++;
            /* Bit patterns, not ==: this is an exactness claim, and == would
             * let +0.0f pass for -0.0f. */
            if (wb != gb) {
                if (mismatched < 8) {
                    printf("  MISMATCH id=%d arg=%6d want=%.9g (%08x) "
                           "got=%.9g (%08x)\n",
                           id, (int)arg, (double)want, wb, (double)got, gb);
                }
                mismatched++;
            }
        }
    }
    printf("checked %lld (all 65,536 s16 x 8 track ids)\n", checked);
    if (mismatched) {
        printf("RED: %lld bit mismatches.\n", mismatched);
        return 1;
    }
    printf("GREEN: bit-identical on every input, including both signed zeros\n"
           "       and the two non-power-of-two TraI ids.\n");
    return 0;
}
"""


def extract_candidate() -> str:
    """Lift the shipped conversion out of battleship_ftanim.c.

    Takes the frac-shift table and the body of ndsR2AnimTargetValue, and rewrites
    the enum-driven switch into the `id` the harness passes directly -- the switch
    is a verbatim copy of the decomp's and is not what this checker is about.
    """
    text = open(SRC, encoding="utf-8", errors="replace").read()

    m = re.search(r"static const u8 sNdsR2AnimFracShift\[8\] = \{[^}]*\};", text)
    if not m:
        sys.exit("FAIL: sNdsR2AnimFracShift table not found in %s." % SRC)
    table = m.group(0)

    body = re.search(
        r"if \(value_or_step != 0\)\s*\{\s*id \+= 4;\s*\}(.*?)\n\}", text, re.S
    )
    if not body:
        sys.exit("FAIL: ndsR2AnimTargetValue tail not found in %s." % SRC)
    tail = body.group(1)
    if "sNdsR2AnimFracShift" not in tail or "__builtin_clz" not in tail:
        sys.exit(
            "FAIL: extracted tail is missing the shift table or the CLZ; the "
            "shipped conversion changed shape and this checker needs updating "
            "rather than deleting."
        )
    # The `q` route must still be PRESENT and still be a route this file does not
    # take. If it vanishes, the poisoned stubs stop being a control and the
    # docstring's scope note becomes a lie -- fail rather than silently widen.
    if "q != 0u" not in tail:
        sys.exit(
            "FAIL: the extracted tail no longer branches on `q`. Either the Q "
            "route was removed (drop the stubs and this guard) or it was "
            "renamed; either way the scope note in this file is now wrong."
        )

    helper = re.search(
        r"static inline f32 ndsR2BitsToF32\(u32 bits\)\s*\{.*?\n\}", text, re.S
    )
    if not helper:
        sys.exit("FAIL: ndsR2BitsToF32 not found in %s." % SRC)

    return "%s\n\n%s\n\nstatic f32 candidate(s16 arg, int id, u32 q)\n{\n" \
           "    u32 mag;\n    u32 shift;\n    u32 k;\n%s\n}\n" % (
               helper.group(0), table, tail)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--verbose", action="store_true")
    args = ap.parse_args()

    src = HARNESS.replace("__CANDIDATE__", extract_candidate())
    if args.verbose:
        print(src)

    cc = None
    for cand in ("gcc", "cc", "clang"):
        try:
            subprocess.run([cand, "--version"], capture_output=True, check=True)
            cc = cand
            break
        except (OSError, subprocess.CalledProcessError):
            continue
    if cc is None:
        print("SKIP: no host C compiler found.")
        return 0

    tmp = tempfile.mkdtemp(prefix="ftanim-exact-")
    cpath = os.path.join(tmp, "check.c")
    exe = os.path.join(tmp, "check.exe")
    with open(cpath, "w", encoding="utf-8") as fh:
        fh.write(src)

    # No fast-math: the point is IEEE semantics, and the shipped build has none
    # either. -O2 so the candidate is compiled the way it ships.
    build = subprocess.run(
        [cc, "-O2", "-std=gnu99", "-Wall", "-o", exe, cpath, "-lm"],
        capture_output=True, text=True)
    if build.returncode != 0:
        print("FAIL: host build of the extracted conversion failed.")
        print(build.stdout)
        print(build.stderr)
        return 1

    print("ftAnimGetTargetValue replacement -- exhaustive bit-exactness")
    run = subprocess.run([exe], capture_output=True, text=True)
    sys.stdout.write(run.stdout)
    sys.stderr.write(run.stderr)
    return run.returncode


if __name__ == "__main__":
    sys.exit(main())
