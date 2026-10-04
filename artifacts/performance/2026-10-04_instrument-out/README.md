# Instrument out of WORK (owner ruling D12a), 2026-10-04

## Why

Owner ruling D12 (docs/p2/FOUR_FIGHTER_30FPS_ARCHITECTURE.md section 8)
approved taking the instrument's measured time out of the gate quantity. Two
pieces of the measuring ROM are not the game:

- the replay digest (`ndsReplayDigestTick`, two folds a presented frame,
  published as DGSA/DGSB): ~4.0K ticks a frame (P50 4,032, P95 4,800);
- the tick HUD's fine per-bucket span clocks (FTR, STG, MISC and its splits,
  the foreground spans): ~120 `cpuGetTiming` reads a presented frame.

## Change

- WORK = ALL - WAIT - digest ticks (`taskman_seam_battle_host.c`). The digest
  stays in HUD, so the harness-side WORK-H drops it twice; the gate reads WORK.
- `NDS_TICK_HUD_SPAN_CLOCK()` (`include/nds/nds_startup.h`) replaces the clock
  read at 26 high-frequency span sites. It reads the clock only while
  `gNdsTickHudSpans` is non-zero, else 0: those buckets read 0 and WORK stays
  exact. The word's boot value is `NDS_TICK_HUD_SPANS_DEFAULT`: 1 everywhere,
  0 on the four-CPU gate target. A run that wants the breakdown on the gate
  ROM passes `-BootSetGlobals 'gNdsTickHudSpans=1'`.

## Result (official gate, `build-gate-1004p`)

| | WORK P50 | WORK P95 | P99 | > 1.12M | two-VBlank |
|---|---:|---:|---:|---:|---:|
| `capture-cuts/gate-cc1` (before) | 897,408 | 1,236,352 | 1,559,232 | 234 | 1,722 / 1,961 |
| `gate-spans1` (digest out, spans on) | 897,728 | 1,235,776 | 1,562,624 | 234 | 1,718 / 1,961 |
| `gate-spans0` (spans poked 0) | 895,360 | 1,232,576 | 1,561,280 | 226 | 1,722 / 1,961 |
| `gate-default0` (gate default 0) | 895,360 | 1,232,576 | 1,561,280 | 226 | 1,722 / 1,961 |

Paired by frame against cc1: spans1 WORK median -576 (ALL - WAIT +3,456: the
new build's layout cost about what the digest exclusion returned), default0
WORK median -3,200 with 1,774 of 1,960 frames better (ALL - WAIT +768: the
span reads were ~2.7K). `gate-default0` is byte-identical to `gate-spans0`
(same code, only the word's initial value differs). Replay digest IDENTICAL to
cc1 on all three runs.

Run: `scripts/sample-tick-hud-buckets.ps1 -Target
smash64ds-p2-fourcpu-tickhud-hwtri -Build build-gate-1004p -NoBuild -RingDump
-Samples 1960 -StartFrame 2 -RunnerSlot 5 -TimeoutSeconds 3000`.
