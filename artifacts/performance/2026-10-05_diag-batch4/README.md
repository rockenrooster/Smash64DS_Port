# Fourth development-tally batch (NDS_DIAG), 2026-10-05

Every tracked `src/**/*.c|.inc` (generated files and the owner's two modified
diagnostics files excepted): 875 statement increments of 652 write-only
`gNds*` counters go through `NDS_DIAG(...)`. The scan (scratchpad
`diagwrap.py`) now also keeps any counter a script under `scripts/` names
(gdb verifiers and probes) live on every build. Note: the earlier batches
(`2026-10-05_diag-collision`, `2026-10-05_diag-batch3`) compiled out 292
counters that probe scripts name; a probe that reads one on the published
ROM or the gate passes `NDS_DIAG_COUNTERS=1` (Makefile note, owner ruling
D12a). The native-failure record (`gNdsRendererNativeFailure`) was never
wrapped.

Replay digest identical everywhere.

Lab sweep ROM (`build-lab-sweepall6` -> `-sweepall7`), paired medians:
Castle -3,072, Sector Z +448, Jungle -3,712, Zebes -2,112, Hyrule -3,648,
Yoshi's Island -7,040, Dream Land -3,008, Saffron -3,200, Mushroom Kingdom
-6,016.

Official gate (`build-gate-1005b` against `gate-s1-diag8`): P50/P95
824,384/1,143,616 -> 827,520/1,149,056, over 116 -> 121, paired median +2,880.
The gate binary (Dream Land only staged) moved the other way from every lab
layout but one; kept on the eight lab layouts, which share the published
ROM's all-stage content, and recorded as layout drift on the gate binary.
