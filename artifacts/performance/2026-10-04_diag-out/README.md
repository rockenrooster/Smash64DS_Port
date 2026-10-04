# Development tallies compiled out of the shipped configuration, 2026-10-04

## Why

A late-window lab profile (`artifacts/task37-census/sz-lateprof05`, frames
1300-1940) put ~50.8K cycles a frame on source lines that update `gNds*`
globals: development tallies (draw counts, memo hit counts, seen masks,
published copies) that only gdb probes and verifiers read. Each is a
read-modify-write of a main-RAM global that the 4 KB data cache has usually
evicted. An audit of every hot site (scratchpad `counteraudit.py`) found 109
whose name no code reads -- 23.6K cycles a frame at 60+ cycles a site.

## Change

- `NDS_DIAG(...)`, defined in the generated build config:
  `do { __VA_ARGS__; } while (0)` when `NDS_DIAG_COUNTERS` is 1, nothing when
  0. `NDS_DIAG_COUNTERS` defaults to 0 on the published ROM (`smash64ds`) and
  the four-CPU gate target -- the configuration that ships -- and 1 on every
  other target; a probe that needs the tallies on a shipping build passes
  `NDS_DIAG_COUNTERS=1`.
- 106 write-only tally statements in 24 files are wrapped (a wrapped statement
  holds no call; native-failure records, state and the tick HUD are not
  touched).

## Result (official gate, `build-gate-1004p`)

| | WORK P50 | WORK P95 | P99 | > 1.12M | two-VBlank |
|---|---:|---:|---:|---:|---:|
| `attr-dedup/gate-ad1` | 882,496 | 1,222,848 | 1,549,568 | 209 | 1,734 / 1,961 |
| `gate-dg0` (tallies out) | 873,344 | 1,215,616 | 1,540,544 | 199 | 1,754 / 1,961 |

Paired by frame: median -7,872, 1,889 of 1,960 frames better (p10 -12.7K,
p90 -3.0K). Replay digest IDENTICAL. The shipping build (`make
TARGET=smash64ds`) compiles with the tallies out.

## Second batch

28 more sites whose names the audit flagged as read turned out to be read
only by their own declarations (multi-name or attributed `volatile u32`
lines, `NDS_FT_POSE_COUNTER`, the `NDS_DIAG_WORD` dump table), by other
tallies, by a debug print, or by lab tours (`NDS_P2_NESS_SPECIAL_TOUR`,
`NDS_P2_LINK_BOMB_TOUR`) that are not compiled on the gate or the published
ROM. Kept as they were: engagement-mask inputs, `gNdsFtrDrawMemoSkipRoot`,
`gNdsDtcmHotStackCalls`, `gNdsR2FighterFacingLr`,
`gNdsFighterDisplayContractSelectedCount`, BGM state and the tick HUD.

| | WORK P50 | WORK P95 | P99 | > 1.12M | two-VBlank |
|---|---:|---:|---:|---:|---:|
| `gate-dg0` | 873,344 | 1,215,616 | 1,540,544 | 199 | 1,754 / 1,961 |
| `gate-dg1` (second batch) | 870,592 | 1,210,752 | 1,532,416 | 192 | 1,759 / 1,961 |

Paired by frame: median -3,456, 1,757 of 1,960 frames better. Replay digest
IDENTICAL.
