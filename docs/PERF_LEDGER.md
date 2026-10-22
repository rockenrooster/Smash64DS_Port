# Smash64DS Performance Ledger

The measurement record. Do not delete reverted or inconclusive rows.

**Where the rows are.** Every row from 2026-07-12 to 2026-09-28 (238
sections: the July tasks and cuts, the T0-T2 experiments, the tick-HUD fork
baseline, P2-2p8's phases, the September RAM and ITCM rounds) is in
[archive/PERF_LEDGER_2026-07_to_09.md](archive/PERF_LEDGER_2026-07_to_09.md),
unchanged, with its own experiment index. Since 2026-10, measurements live
with the phase that made them: P7's gate work in
[P7/P7_STATUS.md](P7/P7_STATUS.md), P8's RAM and CPU levers in
[P8/P8_STATUS.md](P8/P8_STATUS.md) (one log line a batch: commit, proof,
numbers).

**Emulator, since 2026-07-22.** Tick counts come from the owner's
cache-modelling melonDS fork, which reports roughly 40% more ticks than stock
melonDS for the same work; rows before that date are not comparable in
absolute terms (the archive's `TICK-HUD BASELINE / FORK EMULATOR` section).

**The gate today.** WORK P95 and the two-VBlank share come from
`scripts/tools/worst_case_sweep.ps1` on a gate ROM
(`scripts/tools/build_gate_rom.ps1`: the four-CPU sweep target in the ship
layout); its CSV rows are the replay digest. Baselines: `build-sweep-s56`
(the worst-case search) and `build-sweep-s61` (digests), in
[P8/P8_STATUS.md](P8/P8_STATUS.md)'s baseline table.
