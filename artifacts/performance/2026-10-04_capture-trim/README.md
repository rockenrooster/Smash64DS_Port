# Camera-loop capture trims, 2026-10-04

Two per-GObj costs in the battle camera loop, found in the late-window lab
profile (`artifacts/task37-census/sz-lateprof02`); both are exact.

- `ndsStageGCDrawAllLoopRecordCapturedDisplay` ran, for every display GObj of
  every camera, a chain of five recognisers (selected fighter, Jungle barrel,
  Yoshi's Island cloud, Peach's Castle Lakitu, Dream Land Bronto -- two of them
  DObj-tree walks) whose only effect is a diagnostic counter no code reads
  (~8K cycles a frame). Lab builds (`NDS_LAB_FOURCPU_SWEEP`) keep the census.
- `ndsRendererAdapterCommitNativeStageDisplay` compared every display GObj of
  the stage camera with each stage segment GObj (~302 compares a frame for
  ~42 GObjs). The workspace now keeps a 32-bit bloom of its segment pointers,
  rebuilt by `ndsRendererAdapterCollectNativeStageTopology` (the only writer of
  `segments[]`) on every return; a GObj whose bit is clear matches no segment.

## Gate (official target, ring dump, 1,960 samples)

Compile-time changes, so the comparison is across ROMs (`build-gate-1004e`, both
hot-stack words on, `../2026-10-04_ndl-entry-hot/gate-on.*`, against
`build-gate-1004f`, `gate-trim.*`):

| | WORK P50 | WORK P95 | > 1.12M | MCAM P50 | STG P50 | two-VBlank |
|---|---:|---:|---:|---:|---:|---:|
| before | 934,144 | 1,280,448 | 311 | 69,440 | 152,448 | 1,639 / 1,961 |
| after | **929,216** | **1,272,000** | **297** | **66,496** | **151,232** | **1,653 / 1,961** |

Replay digest identical on all 1,960 rows. The single-run spread on one ROM is
~0.4K at P50 and ~5.6K at P95, and a layout change adds its own; the median
moved by more than either.
