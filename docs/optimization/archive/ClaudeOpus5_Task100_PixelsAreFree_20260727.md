# Task 100 — A quarter of the pixels costs 320 ticks: the raster thesis is refuted at its first test

**Date:** 2026-07-27
**Status:** **STOP on the raster axis.** Probe reverted, no runtime change.
The campaign's own kill criterion (`RASTER_AXIS_CAMPAIGN.md` §3.1) fires.
**Authorized by:** the owner — "execute the plan RASTER_AXIS_CAMPAIGN.md".
**Inputs:** `artifacts/task100-A.json`, `-B.json`, `-C.json`; three builds under
`builds/build-task100-{A,B,C}`, 128 ring samples each, frames 439–566, identical
ROM target / window / runner. Probe applied is shown in
`artifacts/visibility/2026-07-27_task100-{A,B}.png`.

## 1. What was tested

The campaign opened on the observation that the stage bucket's ~331,300 fixed
ticks have never been attributed, and that the two quantities GX throughput
scales with were both refuted as the currency — command words (Task 55 E2: −355
→ **+64**) and triangles (Task 99: −50% → **−19,584**, ~5%). Its thesis was that
the missing quantity is **pixels**: rasterization time reaching the CPU as GX
FIFO backpressure.

`NDS_TASK100_VIEWPORT_SHRINK=2` divides the `glViewport` rectangle at
`src/nds/nds_platform.c:318` by two in each axis. The viewport maps normalized
device coordinates onto the screen, so the scene rasterizes into 128×96 instead
of 256×192 — **a quarter of the pixels, with the GX command stream, the triangle
count and every vertex transform bit-identical.** That is the isolation Task 99
could not achieve: decimating triangles left screen coverage untouched, which is
why it only moved 5%.

`NDS_TASK100_DISABLE_AA=1` drops `glEnable(GL_ANTIALIAS)` at
`src/nds/nds_platform.c:315`, enabled at init and never examined in 99 tasks.

## 2. The probe demonstrably applied

`artifacts/visibility/2026-07-27_task100-B.png` shows Dream Land and both
fighters rendered into the bottom-left quarter of the top screen. This is
recorded because a null result from an inert probe is worthless, and the build
config alone (`NDS_TASK100_VIEWPORT_SHRINK 2` in
`builds/build-task100-B/nds_build_config.h`) proves only that it compiled.

## 3. Result

| bucket | A (full) | B (¼ pixels) | Δ B | C (no AA) | Δ C |
|---|---|---|---|---|---|
| **`STG` P50** | 370,368 | 370,048 | **−320** | 375,488 | +5,120 |
| `STG` P95 | 376,960 | 376,064 | −896 | 382,336 | +5,376 |
| `FTR` P50 | 543,360 | 543,552 | +192 | 542,336 | −1,024 |
| `OTHR` P50 | 376,640 | 375,552 | −1,088 | 369,728 | −6,912 |
| **`WORK-H` P50** | 1,320,512 | 1,320,768 | **+256** | 1,327,360 | +6,848 |
| `WORK-H` P95 | 1,732,608 | 1,733,888 | +1,280 | 1,727,296 | −5,312 |
| `ALL` P50 | 1,680,000 | 1,680,064 | +64 | 1,680,064 | +64 |
| VBlank 3-interval | 511 | 510 | −1 | 510 | −1 |

**Three quarters of the frame's pixels stopped being drawn and the frame cost
320 ticks less — 0.09% of the stage bucket.** The kill criterion required
≥40,000. It is not close, and the sign of the `WORK-H` delta is wrong.

Anti-aliasing is free on the same evidence, and arm C is worth more as a noise
calibration (§5).

## 4. Why this was predictable, and should have been predicted

The refutation is not a surprise about the DS — it is a mistake in the campaign's
own §2, and the mechanism was available before a single build ran.

The DS 3D pipeline is **decoupled at the polygon RAM**:

```text
CPU stores -> GXFIFO (256) -> geometry engine -> polygon/vertex RAM
                                                        |
                                              (swap at SwapBuffers)
                                                        v
                                       rasterizer, per scanline during HDraw
```

The CPU can stall on the FIFO being full, on polygon/vertex RAM filling, or on a
swap waiting for the previous frame. It **cannot** stall on rasterization
throughput, because rasterization consumes the already-swapped buffer during
scanout. When the rasterizer cannot keep up, the DS drops polygons past its
per-scanline limit — it does not push back on the CPU.

`RASTER_AXIS_CAMPAIGN.md` §2 quoted `src/port/taskman_seam.c:4925` correctly —
"GX backpressure is not pooled here but distributed through the named buckets as
memory stall on the write that could not retire" — and then over-read it. That
sentence describes **FIFO** backpressure, which is geometry-engine throughput and
scales with vertices. It says nothing about fill, and fill is on the other side
of the decoupling. Two different things were merged into one thesis on the
strength of one sentence.

**The rule this earns:** before building a probe for a hardware mechanism, state
the datapath and say where the proposed cost enters the CPU's critical path. The
campaign's own §8.1 already says "find the tick anchor before proposing a lever";
this is its hardware counterpart, and it would have cost ten minutes against
three builds and three measurement runs.

## 5. By-product: an empirical noise floor

Arm C differs from arm A by one `glEnable` call that the CPU executes once at
init. It cannot change per-frame CPU work. It moved `STG` P50 by **+5,120** and
`WORK-H` P50 by **+6,848**.

That is a clean, direct measurement of build-to-build placement noise on this
instrument, from a build whose per-frame work is provably identical. It confirms
the ±8,000 figure the campaign has been carrying, and it is the first time that
number has been measured rather than inherited.

It also retroactively sharpens Task 99: its −19,584 for half the stage's
triangles was only ~2.5× this floor.

## 6. What is now closed, and what is not

**Closed — the pixel axis.** Coverage joins words and triangles. Three
quantities, three refutations, and now an architectural reason why the third one
could never have worked.

**Not closed — the 331,300.** The stage bucket's fixed cost is still
unattributed, and this task narrows where it can be. It is not pixels, not
words, not triangles, and Task 54 proved it is not stage CPU work in the sense of
*more instructions* (removing 187,648 ticks of it moved `STG+OTHR` by −1.8%).

What remains is **per-operation scaffolding**, which is exactly the currency Task
99 §4 named and no task has yet isolated:

```
331,296 fixed ticks / 54 stage runs  =  ~6,135 ticks per run
of which ~1,621 x 25 binds = 40,525 is texture bind (Task 98 §3)
```

Task 99 arm C tried to measure this by culling runs and got +109,888, because
culling changed the run set and disarmed the Task 36 capture-once replay. The
measurement is still owed, and it needs an instrument that varies run count
without invalidating the capture.

That is `RASTER_AXIS_CAMPAIGN.md`'s fork-B task, and it is now the live one.

## 7. State

`WORK-H` P95 1,732,608 on arm A against the 1,120,000 gate — 612,608 over.
No runtime change; probe flags reverted from `Makefile` and
`src/nds/nds_platform.c`, tree clean.
