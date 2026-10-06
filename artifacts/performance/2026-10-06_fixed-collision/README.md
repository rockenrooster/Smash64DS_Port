# Fixed point, second pass: code size, the attack range tests, collision in Q12 parked (2026-10-06, q28 -> q32)

## What the profile showed

`castle-prof30` (Castle, the q30 code) against `castle-prof24`: soft float
106K -> 69K cycles a profile region, but the new ARM fixed-point functions
ran at CPI 5-8 -- cold main-RAM code bloated by inlined float <-> fixed
converters (camera 3.0 KB, particle TRS 3.1 KB, wallpaper 1.6 KB). Shared
out-of-line converters (`include/nds/nds_fixed_convert.h`) halved them; the
same trajectory (digest identical) q29b -> q30 read paired -6.7K gate,
-10.1K Castle, -5.2K Sector Z (that pair still carried the Q12 collision
below, whose own size dropped too).

## Collision in Q12: parked

Floor and ceiling sweeps with a Q12 crossing kernel (ARM, out of line) and
the sloped floor height in Q12 (`parked/collision-q12.patch`,
`parked/nds_mp_floor_crossing_q.h`). Host check of the kernel against the
float one (300,000 random motions near random segments): every float hit is
a Q hit, 26 more Q hits on the 0.001 epsilon boundary, hit points within
0.1 units. On Dream Land the replay digest stays identical, so the A/B is
clean there: q31f (float sweeps) -> q31 (Q12 sweeps) paired +2,304. The
compact Thumb sweep calling the ITCM soft-float routines beats a main-RAM
ARM kernel; a fixed collision needs ITCM residency, i.e. soft float evicted
first. Reverted; the patch is kept.

## Kept (q32, digest identical to q28 on every stage)

- shared converters; camera, particle, wallpaper, Mod1 helpers on them
- kind-48 helpers `noinline` ARM (they had been inlined into the Thumb caller
  as 13 `__aeabi_lmul` a frame)
- wallpaper perspective memo on its inputs (the frame's present asks with
  the tick's eye and target)
- the three attack-near-fighter tests with their bounds formed once and the
  order-key compares (exact)

Same-trajectory reads (paired medians):

| step | gate | Castle | Sector Z |
|---|---|---|---|
| q31k (float kind 48) -> q31f (Q kind 48) | -2,048 | -3,136 | +2,240 |
| q31f -> q32 (range tests) | +448 | +448 | -512 |
| q28 -> q32 (all of the above) | -960 | -960 | -- |
| q26 -> q32 (Sector Z, with q27 kind 48 and q28 pose clock) | -- | -- | +4,160 |

Sector Z bisect: q26 -> q27 (kind 48 in Q, then inlined into Thumb) +3,584,
q27 -> q28 (pose clock integer path) +1,024; q28 -> q32 +128. Absolute (q32):
gate P95 1,049,344, Castle 1,140,736, Sector Z 1,156,352.
