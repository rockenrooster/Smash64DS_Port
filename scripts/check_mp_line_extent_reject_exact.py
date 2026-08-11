#!/usr/bin/env python3
"""Prove the cycle-117 whole-line collision rejects cannot hide a hit.

Two rejects were added to `src/port/reloc_backend_mp_collision.c`, both of the
same shape: look at a precomputed span for the WHOLE line and skip its segment
loop entirely. That is the biggest structural saving available in the collision
lane -- an object is over at most one floor line and the loops visit them all --
and it is also the most dangerous, because a wrong reject is BUGS.md's fighters
floating under the stage. So it is proven here rather than argued in a comment.

  * `ndsMPLineExtentRejects` (the `mpCollisionGetFCCommon*` point query) claims
    that if `object_x` is outside the line's x span, every segment fails the
    loop's own "object_x lies between this segment's two x" gate. That one is
    exact by inspection and is checked here anyway.

  * `ndsMPLineExtentSweepRejects` (the floor and ceiling sweeps) claims that if
    the sweep's y span misses the line's y span by more than 0.001, every
    segment is rejected by `ndsMPFCSegmentCrossesKernel`, AND the caller's
    `saw_flat_ascending_sweep` side effect stays false. That one spans the
    kernel's tilt branch (which gates on y with a 0.001 slack) and its FLAT
    branch (which does not gate on y at all and had to be reasoned about
    separately), so it gets the real work here.

The test is the implication, not the verdict: for every case where the reject
fires, run the loop the reject skipped and assert it produced nothing. A reject
that never fires would pass trivially, so the fire rate is printed and a run
with no fires is a FAILURE.

X is deliberately not part of the sweep reject -- see the comment on
`ndsMPLineExtentSweepRejects` -- and this checker would catch it if it were:
the flat branch's hit x is an extrapolation that can leave the sweep's x span.
"""

from __future__ import annotations

import pathlib
import re
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
KERNEL = ROOT / "include" / "nds" / "nds_mp_floor_crossing.h"
FCMP = ROOT / "include" / "nds" / "nds_fcmp.h"


def inline_header(path: pathlib.Path) -> str:
    """Strip the include guard and any #include so the body can be pasted."""
    text = path.read_text(encoding="utf-8", errors="replace")
    text = re.sub(r"^\s*#\s*(ifndef|define)\s+SSB64_\w+\s*$", "", text,
                  flags=re.MULTILINE)
    text = re.sub(r"^\s*#\s*include\s+.*$", "", text, flags=re.MULTILINE)
    # The closing #endif is bare, so match the LAST one rather than its text.
    idx = text.rfind("#endif")
    if idx < 0:
        sys.exit("no #endif in %s" % path)
    return text[:idx] + text[idx + len("#endif"):]


HARNESS = r"""
#include <stdio.h>
#include <string.h>

typedef float f32;
typedef signed int s32;
typedef unsigned int u32;

@@FCMP@@
@@KERNEL@@

/* ---- the two rejects, transcribed from reloc_backend_mp_collision.c ---- */

static int extent_rejects(const f32 *vx, int n, f32 object_x)
{
    f32 min_x = vx[0];
    f32 max_x = vx[0];
    int k;

    for (k = 1; k < n; k++)
    {
        if (NDS_FCMP_LT(vx[k], min_x)) { min_x = vx[k]; }
        if (NDS_FCMP_GT(vx[k], max_x)) { max_x = vx[k]; }
    }
    return NDS_FCMP_LT(object_x, min_x) || NDS_FCMP_GT(object_x, max_x);
}

static int sweep_rejects(const f32 *vy, int n, f32 py, f32 ty)
{
    const f32 epsilon = 0.001F;
    f32 min_y = vy[0];
    f32 max_y = vy[0];
    f32 smin;
    f32 smax;
    int k;

    for (k = 1; k < n; k++)
    {
        if (NDS_FCMP_LT(vy[k], min_y)) { min_y = vy[k]; }
        if (NDS_FCMP_GT(vy[k], max_y)) { max_y = vy[k]; }
    }
    if (NDS_FCMP_LT(py, ty)) { smin = py; smax = ty; }
    else                     { smin = ty; smax = py; }
    return NDS_FCMP_LT(max_y + epsilon, smin) ||
           NDS_FCMP_LT(smax, min_y - epsilon);
}

/* The caller's flat-ascending side effect, from ndsStageMPSweepFloorLoopSweep.
 * It runs BEFORE the kernel call and is not gated by it, so a whole-line
 * reject has to keep it false too. */
static int flat_ascending(f32 v1x, f32 v1y, f32 v2x, f32 v2y,
                          f32 px, f32 py, f32 tx, f32 ty)
{
    return (v1y == v2y) && (ty > py) && (py <= v1y) && (ty >= v1y) &&
           (((px < tx) ? px : tx) <= ((v1x > v2x) ? v1x : v2x)) &&
           (((px > tx) ? px : tx) >= ((v1x < v2x) ? v1x : v2x));
}

/* ---- deterministic case generation ---- */

static unsigned long long g_state = 0x9E3779B97F4A7C15ull;

static unsigned int rnd(void)
{
    g_state ^= g_state << 13;
    g_state ^= g_state >> 7;
    g_state ^= g_state << 17;
    return (unsigned int)(g_state >> 32);
}

/* Stage vertices are s16 in the real data; sweeps are fighter positions, which
 * are arbitrary floats. Coordinates are kept small and quantised so that the
 * 0.001 boundary is actually probed instead of being lost in float noise. */
static f32 coord(void)
{
    return (f32)((int)(rnd() % 4001u) - 2000) * 0.0625F;
}

static f32 near_value(f32 base)
{
    static const f32 nudge[9] = {
        0.0F, 0.0005F, -0.0005F, 0.001F, -0.001F,
        0.0015F, -0.0015F, 0.05F, -0.05F
    };
    return base + nudge[rnd() % 9u];
}

int main(void)
{
    const int CASES = @@CASES@@;
    long long extent_fired = 0, extent_bad = 0;
    long long sweep_fired = 0, sweep_bad = 0, sweep_flat_bad = 0;
    long long segments = 0;
    int c;

    for (c = 0; c < CASES; c++)
    {
        f32 vx[9];
        f32 vy[9];
        int n = 2 + (int)(rnd() % 8u);   /* 2..9 vertices = 1..8 segments */
        f32 px, py, tx, ty, ox;
        int ud = (rnd() & 1u) ? 1 : -1;
        int j;

        for (j = 0; j < n; j++)
        {
            vx[j] = coord();
            vy[j] = coord();
        }
        /* Half the lines get a FLAT run, so the kernel's sy == 0 branch -- the
         * one with no y bounding gate -- is actually exercised. */
        if (rnd() & 1u)
        {
            f32 flat = vy[0];
            for (j = 0; j < n; j++) { vy[j] = flat; }
        }
        px = coord();  py = coord();
        tx = coord();  ty = coord();
        /* Aim the sweep at the line's span often enough that the reject sits
         * on its boundary rather than far away from it. */
        if (rnd() & 1u) { py = near_value(vy[rnd() % (unsigned)n]); }
        if (rnd() & 1u) { ty = near_value(vy[rnd() % (unsigned)n]); }
        ox = (rnd() & 1u) ? near_value(vx[rnd() % (unsigned)n]) : coord();

        if (extent_rejects(vx, n, ox))
        {
            extent_fired++;
            for (j = 0; j + 1 < n; j++)
            {
                f32 a = vx[j], b = vx[j + 1];

                if (((a <= ox) && (b >= ox)) || ((b <= ox) && (a >= ox)))
                {
                    extent_bad++;
                }
            }
        }

        if (sweep_rejects(vy, n, py, ty))
        {
            sweep_fired++;
            for (j = 0; j + 1 < n; j++)
            {
                f32 hx = 0.0F, hy = 0.0F;

                segments++;
                if (ndsMPFCSegmentCrossesKernel(px, py, tx, ty,
                        vx[j], vy[j], vx[j + 1], vy[j + 1], ud, &hx, &hy))
                {
                    sweep_bad++;
                }
                if ((ud > 0) && flat_ascending(vx[j], vy[j], vx[j + 1],
                                               vy[j + 1], px, py, tx, ty))
                {
                    sweep_flat_bad++;
                }
            }
        }
    }

    printf("cases %d\n", CASES);
    printf("extent_fired %lld extent_missed_hits %lld\n",
           extent_fired, extent_bad);
    printf("sweep_fired %lld segments_replayed %lld "
           "sweep_missed_hits %lld sweep_missed_flat %lld\n",
           sweep_fired, segments, sweep_bad, sweep_flat_bad);
    return 0;
}
"""


def main() -> int:
    cases = 400000
    src = (HARNESS
           .replace("@@FCMP@@", inline_header(FCMP))
           .replace("@@KERNEL@@", inline_header(KERNEL))
           .replace("@@CASES@@", str(cases)))
    with tempfile.TemporaryDirectory() as td:
        c = pathlib.Path(td) / "extent.c"
        exe = pathlib.Path(td) / "extent.exe"
        c.write_text(src, encoding="utf-8")
        cp = subprocess.run(
            ["gcc", "-O1", "-fno-fast-math", "-o", str(exe), str(c)],
            capture_output=True, text=True)
        if cp.returncode != 0:
            print(cp.stdout)
            print(cp.stderr)
            return 1
        run = subprocess.run([str(exe)], capture_output=True, text=True)
        print(run.stdout.strip())
        if run.returncode != 0:
            print(run.stderr)
            return 1

    nums = {}
    for token in run.stdout.split():
        if nums.get("_k"):
            nums[nums["_k"]] = int(token)
            nums["_k"] = None
        elif not token.lstrip("-").isdigit():
            nums["_k"] = token
    nums.pop("_k", None)

    if nums.get("extent_fired", 0) == 0 or nums.get("sweep_fired", 0) == 0:
        print("FAIL: a reject never fired, so nothing was actually proven")
        return 1
    bad = (nums.get("extent_missed_hits", 1) or 0) + \
          (nums.get("sweep_missed_hits", 1) or 0) + \
          (nums.get("sweep_missed_flat", 1) or 0)
    if bad != 0:
        print("FAIL: a whole-line reject skipped a segment that would have hit")
        return 1
    print("MP_LINE_EXTENT_REJECT_EXACT=PASS  every fired reject was replayed "
          "segment by segment and produced no hit and no flat-ascending flag")
    return 0


if __name__ == "__main__":
    sys.exit(main())
