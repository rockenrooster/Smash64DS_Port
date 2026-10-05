# Third development-tally batch out of ship and gate (NDS_DIAG), 2026-10-05

The same write-only scan as `../2026-10-05_diag-collision` over eight hot
files: 1,061 statement increments of write-only `gNds*` counters now go
through `NDS_DIAG(...)` (nds_renderer_native_owners.c 90,
nds_renderer_native_common.c 106, renderer_adapter_matrix.c 61,
reloc_backend_compat_shims.c 247, nds_ft_pose.c 2, renderer_adapter_stage.c
280, nds_renderer_dl_core.c 9, reloc_backend_movement.c 266). Counters any
code reads stay as they were.

Replay digest identical everywhere.

Official gate (`build-gate-1005a` against `gate-s1-diagmp`): P50/P95
829,824/1,152,256 -> 824,384/1,143,616, over 126 -> 116, paired median
-5,376, mean -5,426.

Lab sweep ROM (`build-lab-sweepall5` -> `-sweepall6`), paired medians: Castle
-2,240, Sector Z -4,032, Jungle -4,352, Zebes -2,560, Hyrule -320, Yoshi's
Island -1,728, Dream Land -1,408, Saffron -3,008, Mushroom Kingdom -1,600.
