# Lean lab instruments out of the gate ROM (owner ruling D12a), 2026-10-04

## Change

`include/nds/renderer_fighter_lean.h`: `NDS_FTR_LEAN_LAB` and
`NDS_VRAM_CENSUS_LIVE` were tied to `NDS_TICK_HUD`, so the four-CPU gate ROM
(a tick-HUD build) carried the lean renderer's lab instrument -- the
`gNdsFtrLean` counters on every fighter draw, the oracle-route branches and
the outlined route 1 entry -- plus the VRAM census's per-upload tagging and
frame publish, none of which the published ROM has. Both now also need
`NDS_DIAG_COUNTERS` (0 on the published ROM and the gate, 1 in every lab
build), so the gate compiles the shipping image's lean path. Gate ELF: text
-20.5 KB, BSS -14 KB (`gNdsFtrLean` 2.5 KB, `gNdsVramCensus` 6.9 KB gone).
Probes that read `gNdsFtrLean` on a gate ROM need `NDS_DIAG_COUNTERS=1`.

## Result

Gate target, 1,960 presented frames.

| arm | build | WORK P50 | WORK P95 | > 1.12M | two-VBlank |
|---|---|---:|---:|---:|---:|
| control (`../2026-10-04_lean-relaxed/gate-r1`) | build-gate-1004r | 850,176 | 1,184,960 | 160 | 1,788/1,961 |
| instruments out (`gate-l1`) | build-gate-1004l | 845,504 | 1,181,952 | 157 | 1,792/1,961 |

Paired by frame: median -3,584, mean -3,828, 1,757 of 1,960 frames better.
Replay digest identical over 1,960 samples. No render or game change.
