# 2026-10-05 Collision: segment windows on long ordered lines; wall static miss in one pass

`src/port/reloc_backend_mp_collision.c`, exact (replay digest identical in
every pair below).

1. **Segment window** (`gNdsMPSweepSegmentWindow`). The floor and ceiling
   sweeps bound the x interval a segment must meet to be hit: the sweep's
   truncated x span, less/plus one unit, widened by |dx| + 2 units when the
   motion falls (rises, for a ceiling) more than one unit. A flat segment's
   crossing is extrapolated and lands outside the sweep's span by at most
   0.001 |dx| / dy; with dy <= 0 the flat branch never hits; in between the
   line is walked whole. On a line of at least 12 vertices whose x never
   reverses (`sNdsMPLineXOrder`, filled with the line's extent): a line whose
   integer x extent misses the interval is skipped, else two binary searches
   give the run of segments to walk, in the original order.
2. **Point queries** (`gNdsMPPointSegmentStart`): mpCollisionGetFCCommonFloor
   / ...Ceil start their walk at the segment ending at the first vertex past
   the point on such lines (the earlier segments cannot bracket it).
3. **Wall static miss in one pass** (`gNdsMPWallMissOnePass`): each group is
   proved rejected and booked (`reject_misses = 0`) in the same pass instead
   of a second pass after all are; the field only paces the reject's backoff.

Clean lab ROMs, same ROM, all words 0 -> default:

| ROM / stage | P50 | P95 | over 1.12M | paired median |
|---|---|---|---|---|
| final (`build-lab-clean1005s`), Sector Z (`c0-g1` -> `c1-g1`) | 915,200 -> 910,144 | 1,277,952 -> 1,272,256 | 229 -> 211 | -5,440 |
| final, Yoshi's Island (`c0-g5` -> `c1-g5`) | 935,296 -> 933,056 | 1,249,024 -> 1,249,984 | 266 -> 262 | -1,536 |
| first cut (`../2026-10-05_collision-window`, `1005n`), Saffron | 931,456 -> 929,536 | 1,254,464 -> 1,254,592 | 231 -> 230 | -1,024 |
| first cut, Jungle | 900,928 -> 900,480 | 1,264,320 -> 1,264,768 | 241 -> 238 | -256 |
| first cut, Mushroom Kingdom | 918,528 -> 918,912 | 1,196,864 -> 1,197,440 | 175 -> 173 | -384 |
| window alone (`../2026-10-05_segment-window`, `1005l`), Dream Land | 823,040 -> 823,360 | 1,131,712 -> 1,132,672 | 105 -> 105 | 0 |

The first cut applied the bounds and the whole-line reject to every line;
Yoshi's Island paid +2.9K a frame for them (`../2026-10-05_collision-window3`:
window alone +2,944, one pass alone -1,472; a same-ROM profile put
`ndsMPSweepWindowBounds` at 3.4K cycles a frame with no segment saved). Only
long ordered lines take the window now. `../2026-10-05_collision-window4`:
with the point-query start off and the rest on, Sector Z reads -3.0K, so the
start carries the remaining ~2.5K there.

**Official gate** (`build-gate-1005u`: this change, the heavy-frame particle
LOD of `../2026-10-05_particle-lod`, and the day's earlier commits), same ROM,
their words 0 -> default (`gate-u0` -> `gate-u`): P50 819,904 -> 818,240, P95
1,124,992 -> 1,121,728, over 98 -> 96, two-VBlank presents 1,854 -> 1,858,
paired median -1,408, digest identical. Against the morning's official build
(`../2026-10-05_more-routes/gate-s1`, `build-gate-1005s`) the same ROM reads
P95 +5.4K, P50 +4.4K: the build's layout moved the gate +8.7K (words off).
