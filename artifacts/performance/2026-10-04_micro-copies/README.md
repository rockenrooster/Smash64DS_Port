# Collision readiness inline; inline matrix copies rejected, 2026-10-04

Two compile-time micro-changes from the late-window lab profile
(`artifacts/task37-census/sz-lateprof02`), measured against a same-session rerun
of the previous ROM (`build-gate-1004f`, `gate-base.*`).

## Kept: `ndsStageCollisionLoopGeometryReady` inline

~1,000 calls a frame from main-RAM Thumb reached this six-load ITCM body through
an interworking call (11.6K cycles a frame in the profile). It is now
`static inline __attribute__((always_inline))` (`build-gate-1004h`, `gate-geo.*`):

| | WORK P50 | WORK P95 | > 1.12M | STG P50 | two-VBlank |
|---|---:|---:|---:|---:|---:|
| base (`build-gate-1004f`, rerun) | 930,496 | 1,274,496 | 296 | 151,168 | 1,653 / 1,961 |
| inline readiness (`build-gate-1004h`) | **925,888** | **1,271,488** | **288** | 151,552 | **1,666 / 1,961** |

Replay digest identical on all 1,960 rows (compile-time change, across ROMs).

## Rejected: inline 16-word copies in `ndsStageGxCommitFast`

The fast pass makes ~98 64-byte matrix copies a frame through `memcpy` (~120
cycles a call in the profile). Replacing them with straight-line word copies
(`stagegx-copy16`, `build-gate-1004g`, `gate-micro.*`) took STG P50
151,168 -> 159,872 (+8.7K) and WORK P50 +2.7K, P95 +4.5K, over 296 -> 309.
`memcpy` is the ARM LDM/STM body in ITCM (`src/nds/nds_fast_mem.c`); the inline
form put the same stores into main-RAM code inside an already large function.
Reverted.
