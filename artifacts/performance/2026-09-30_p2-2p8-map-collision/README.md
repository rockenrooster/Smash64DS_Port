# P2-2p8: map-collision sweeps skip line groups they cannot touch (2026-09-29/30)

Solo, no subagents. Lab = the NDS_LAB_FOURCPU_SWEEP ROM (default roster
Donkey/Samus/Link/Kirby unless named), one full match per run; gate = the
four-CPU tick-HUD ROM on Dream Land. Run files are in
`artifacts/performance/2026-09-26_p2-2p8-ftr-item-tail/` under the arm names
below; `ab.txt` and `lab-baseline.txt` hold every summary line quoted here.

## Where the stages' extra time is

The lab sweep at `85b87d19b3f` (`lab-baseline.txt`, arms `l12_g*`) puts every
stage but Dream Land (P95 1,256K) and Congo Jungle (1,286K) at 1.34-1.63M:
Sector Z 1,625K, Yoshi's Island 1,477K, Saffron 1,450K, Zebes 1,430K,
Mushroom Kingdom 1,419K, Peach's Castle 1,386K. The tick-HUD buckets say the
difference is mostly flat sim time: SRC medians 483-562K against Dream Land's
408K. A whole-match profile of Peach's Castle (`p2p8-prof-lab-g0`) puts
map collision at the top of that premium: `__aeabi_fadd` 100K a frame (47K on
Dream Land), of which `ndsStageMPAdjustFloorLoopWallSweep` alone makes 1,850
soft-float calls a frame (283 on Dream Land) and 48K of its own time.

The wall sweep is called ~99 times a frame (every map-collision query of every
object, both wall kinds) and visits every wall group of its kind -- one group
per yakumono. Each visit paid the float preamble (the yakumono offset, the
+/-0.001 bounds, the integer range) before its line loop examined nothing: a
line is only examined when its `[coll_pos_prev, coll_pos_next]` meets that
range, and most groups are nowhere near the sweeping object.

## Banked (`src/port/reloc_backend_mp_collision.c`)

- **Group reject.** Each kind group now carries its lines' span
  (`ext_lo`/`ext_hi`, the min/max of their `coll_pos`). A superset of the
  sweep's range is built from the operands' truncations with 4 units of slack
  (each truncation is within 1, the bounds add 0.001, and the float
  subtractions round far below a unit below 2^20); a group whose span misses
  it cannot examine a line, and is skipped before its preamble. The yakumono
  translate and speed truncations are cached per yakumono by their float bits.
  The floor and ceiling sweeps take the same test on the y spans (a line
  they examine is one whose y span meets the sweep's within 0.001, which is
  their per-line extent reject). A group whose checks keep failing (a moving
  platform the object stands near: Mushroom Kingdom's walls rejected ~6%)
  stops being checked for 32 visits after 8 misses; whether a group is
  checked never changes what the sweep finds. Words
  `gNdsMPWallSweepGroupReject`, `gNdsMPSweepGroupReject`.
- **Group-level offsets.** The floor and ceiling sweeps computed a moving
  group's offsets (four subtractions, two additions) once per line; they are
  the group's. Word `gNdsMPSweepGroupHoist`.
- **Extent bounds at fill.** The per-line extent reject added and subtracted
  0.001 on every call; the two bounds are now stored when the extent is filled
  (the same two float operations, made once).
- ITCM: `ndsFighterDisplayContractSetLight` (88 B) and
  `ndsRendererProfileSetOwner` (116 B; the lab build already evicted it) left
  ITCM for the wall sweep's growth (`linker/nds_hot_text.ld`).

Same ROM (lab `cb`), every new word off (`cb0`) against on (`cb1`):

| Stage | P50 off / on | P95 off / on | two-VBlank off / on | group rejects |
|---|---:|---:|---:|---:|
| Peach's Castle (a) | 1,060,416 / **1,009,088** | 1,400,320 / **1,341,184** | 59.7% / **70.7%** | 503,508 wall, 114,039 floor/ceil |
| Peach's Castle (b) | 1,060,160 / **1,008,576** | 1,402,496 / **1,339,200** | 59.7% / **70.6%** | |
| Sector Z | 1,161,536 / 1,159,424 | 1,631,232 / 1,628,800 | 43.4% / 43.1% | 160,473 / 29,236 |
| Zebes | 1,035,968 / 1,035,328 | 1,429,376 / 1,431,360 | 63.2% / 63.8% | 0 / 34,330 |
| Yoshi's Island | 1,141,696 / **1,129,600** | 1,487,552 / **1,478,592** | 39.8% / 42.3% | 192,263 / 130,486 |
| Dream Land | 941,824 / 939,968 | 1,279,424 / **1,274,048** | 81.4% / 81.7% | 166,065 / 25,510 |
| Saffron | 1,064,384 / **1,055,808** | 1,525,312 / **1,513,536** | 56.9% / 58.6% | 97,871 / 86,871 |
| Mushroom Kingdom | 1,117,760 / **1,114,304** | 1,436,032 / **1,427,392** | 43.7% / 45.0% | 8,851 / 76,421 |

Replay IDENTICAL on every pair (on against off, same ROM). Before the
backoff, Mushroom Kingdom read P50 +4.3K (its checks were pure cost) and
Zebes +5.5K (no wall groups, but the call-level truncations ran anyway; they
are now lazy).

## Found: the simulation depended on the build (resolved)

The `cb` build's all-off arm differs from the previous builds' (`cq0`,
`l12`) on Dream Land (from frame 1370) and Saffron (from 1137), though every
new behaviour is off -- while within every build tested, on and off, the
hurtbox kernel's modes 0/1/2 (`hr0`, `sh`, `qk0-2`) and every word pair agree,
and shadow mode reports 0 flips. Rebuilding the previous source (`qk*`)
reproduces its old digest; kernel off, the two layouts' float paths still
differ. An inert 64-byte BSS pad on HEAD changes nothing on six stages
(`pad_g*`), but skipping one no-op helper call when the words are off
(`st*`) moves Dream Land back to the old digest and leaves Saffron on the new
one. Something in the simulation reads memory whose contents depend on heap
placement or stack history (the kind-group array grew by 4 bytes a group
here, shifting every later allocation of the stage load).

Resolved (same day, `2026-09-30_p2-2p8-layout-digest`): not an
uninitialised read but a stale loaded-file pointer. A fighter clip's full
load finalized a record that an animation-cache pin rescue had moved, so the
clip kept its raw relocation chain and played no animation; whether the
cache's ring reached a pin depended on the arena size, hence on the build.

## Gate

The gate ROM (Dream Land, `b12m0`/`b12m1`, same ROM): P50 913,920 / 913,920 ->
913,472 / 913,984, P95 1,228,416 / 1,230,464 -> 1,229,440 / 1,229,120 --
neutral there; replay IDENTICAL between the arms and against the previous
commit's gate runs (`b11i1_a`).
