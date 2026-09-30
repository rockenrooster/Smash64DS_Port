# P2-2p8: the event32 ledger margin, and a Poke Ball crash it caused (2026-09-30)

## The crash

The baseline sweep of this session (lab ROM, full match, items on) died on
Kongo Jungle with Captain/Yoshi/Kirby/DK (`x0_cy_g2`) at presented frame 937:
data abort in `itMBallOpenProcUpdate` (`battleship_item_mball.c`), writing a
Poke Ball's position into its rays effect. The control ROM without this
session's collision change crashes at the same frame (`x0c_cy_g2`), so it is
not new with it.

gdb on the fix lab ROM (`tools/mballprobe*.ps1`; `mball-fix-cy-g2*.txt`):

- f=936 `itMBallOpenInitVars` makes the rays effect (`efManagerMBallRaysMakeEffect`).
- f=937 its first update, `efManagerNoStructProcUpdate`, finds the GObj's
  anim_frame <= 0 and ejects it: all four of its DObjs have `event32 = NULL`
  and `anim_wait = AOBJ_ANIM_NULL`. The animation was never attached.
- The ball's next update writes through the ejected GObj, whose object pointer
  the port's eject clears: NULL + 0x1C.

Why no animation: the event32 normalize ledger was full. At f=936
`sNdsAObjEvent32NormalizedCount` 3,287 of limit 3,288, no holes, 35 scripts
already refused (reason 12, capacity). `gcAddAnimAll` normalizes the whole
table first and skips the bind when any script is refused.

## Sizing

The limit is max(stage bound, stage part + the roster's entry clips + margin)
(`ndsAObjEvent32RosterLimit`, from the 09-28 S1 fix). A ledger curve with the
lab override at 7,000 (`ledger-cy_g2_o7000.txt`) shows live entries (count
less holes) of ~3,100-3,250 through the match: a working set at the limit,
not a leak. The entry-clip term undercounts rosters whose every motion is an
event32 clip (Ness, Yoshi, Pikachu, Purin); the elastic motion cache and the
09-30 idle-time clip prefetch keep more of them normalized than the 09-28
census saw.

Full-match rows at the old margin (384) and with the 7,000 override
(`sizing-old-margin.txt`; lab ROM `8220F668`, runs `n0_*` / `n7_*`):

| Arm | limit | high-water | refused / detached | high-water at 7,000 |
|---|---:|---:|---:|---:|
| Dream Land, DK/Samus/Link/Kirby | 3,072 | 2,954 | 0 / 0 | 2,954 |
| Jungle, Captain/Yoshi/Kirby/DK | 3,288 | full | crash f=937 | 4,713 |
| Mushroom Kingdom, same | 3,262 | 3,257 | 0 / 0 | 3,685 |
| Dream Land, Fox/Pikachu/Ness/Samus | 3,072 | 3,072 | **84 / 23** | 4,236 |
| Saffron, same | 3,586 | 3,572 | 0 / 0 | 4,089 |
| Castle, Luigi/Kirby/Ness/Purin | 2,825 | 2,823 | 0 / 0 | 2,929 |
| Yoshi's Island, same | 3,826 | 3,823 | 0 / 0 | 4,399 |
| Zebes, Pikachu x4 | 6,532 | 6,532 | **149 / 56** | 6,836 |
| Hyrule, Kirby x4 | 1,536 | 1,474 | 0 / 0 | 1,474 |
| Sector Z, Kirby/Fox/Yoshi/Pikachu | 3,829 | 3,828 | 0 / 0 | 3,967 |
| Sector Z, DK/Samus/Link/Kirby | 3,358 | 3,350 | 0 / 0 | 3,418 |

(High-water counts holes until an append compacts them; with the override no
compaction happens, so the 7,000 column bounds the live set from above.)

## Fix

- The margin keeps 384 (`NDS_AOBJ_EVENT32_ROSTER_MARGIN`) and adds 768
  (`NDS_AOBJ_EVENT32_MOTION_MARGIN`) for each Ness, Yoshi, Pikachu or Purin in
  the match; the DK/Samus/Link/Kirby-style rosters keep their old limits.
- `gNdsAObjEvent32LiveHighWater`: the live high-water (count less holes), the
  number a limit has to cover.
- The ball follows its rays only while the effect still has its DObj
  (`ndsITMBallRaysFollow`): N64 leaves an ended effect's DObj in the pool, so
  the source's late write lands in a dead object; the port's eject clears the
  pointer.

A flat margin of 2,048 was tried first (`m1_*`, lab ROM `7098D615`;
`verify-flat-2048.txt`, `perf-flat-2048.txt`). It removed every refusal, but
its ledger heap on Sector Z's default roster pushed the Arwing's flight table
and DMA arena under their free-heap checks: STG +25K a frame (P50 137,984 ->
163,072) and WORK P50 +37K on `m1_d_g1`.

## Verification

Runs `m2_*` on the roster-margin lab ROM (`428F0827`, which also carries the
uncommitted scene-transition change), full match, items on
(`verify-roster-margin.txt`, `perf-roster-margin.txt`; baseline `x0_*` in
`perf-baseline.txt`):

| Arm | limit | live high-water | refused | heap low-water | WORK P50 x0 -> m2 |
|---|---:|---:|---:|---:|---|
| Dream Land, DK/Samus/Link/Kirby | 3,072 | 2,546 | 0 | 161,452 | 929,664 -> 933,056 |
| Jungle, Captain/Yoshi/Kirby/DK | 4,056 | 3,406 | 0 | 74,896 | crash -> 963,712 |
| Mushroom Kingdom, same | 4,030 | 2,717 | 0 | 146,640 | 1,106,816 -> 1,106,304 |
| Dream Land, Fox/Pikachu/Ness/Samus | 4,082 | 3,303 | 0 (84) | 278,404 | 971,264 -> 966,848 |
| Saffron, same | 5,122 | 2,897 | 0 | 212,908 | 1,121,024 -> 1,119,680 |
| Castle, Luigi/Kirby/Ness/Purin | 4,361 | 1,922 | 0 | 242,260 | 958,720 -> 958,080 |
| Yoshi's Island, same | 5,362 | 3,223 | 0 | 163,928 | 1,065,152 -> 1,064,320 |
| Zebes, Pikachu x4 | 8,191 | 6,449 | 0 (149) | 397,988 | 981,888 -> 981,440 |
| Hyrule, Kirby x4 | 1,536 | 1,474 | 0 | 462,380 | 856,640 -> 857,280 |
| Sector Z, Kirby/Fox/Yoshi/Pikachu | 5,365 | 3,320 | 0 | 67,464 | 1,040,512 -> 1,041,024 |
| Sector Z, DK/Samus/Link/Kirby | 3,358 | 2,752 | 0 | 61,248 | 1,065,792 -> 1,065,216 |

(Refusals in parentheses are the old margin's.) Sector Z's STG bucket is back
to baseline: default roster 262.4M -> 265.6M over the match (m1: 312.0M),
the Kirby/Fox/Yoshi/Pikachu roster 261.1M -> 261.0M. Every heap low-water
stays above the 25,600 floor. The lab sweep carries no replay digest columns;
the gate ROM's digest is checked separately.
