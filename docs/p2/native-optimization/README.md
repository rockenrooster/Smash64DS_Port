# P2-2p8 native-runtime implementation campaign

**Plan date:** September 15, 2026. **Revision 2:** all legal four-fighter lineups on every selectable VS stage. **Original research baseline:** `75f7f6b4b4864c82c01872d0fd2771d171005272`. **Repository rechecked for this revision:** `e67e5871ba8c4ae972f4826bfeb89757d3686401` on `master`. Old measurements remain historical; implementation must rebase against the current board.

This is an implementation specification, not an implemented optimization or a new benchmark. The **PLANNED** fields are static specification metadata, not live execution state. Proposed paths, APIs, formats and test IDs are explicitly new work; existing anchors refer to the pinned repository. No ROM was built and no GitHub branch was modified while preparing this package.

## Current execution amendment

The owner requested lower workflow overhead and larger runtime changes on
September 15. The implementation scope and 81-card dependency/coverage graph are
unchanged. Reviewed workflow baseline: `941f4daab5611e9d86f835a6e4bd8ec61e0156df`.
Current status/metrics still come from the board, not this static plan.

## Start and authority

Continue the **Execution cursor** in `docs/P2_EXECUTION_BOARD.md`. In an intact
context do not repeat startup or completed phases. After context loss use
`docs/HANDOFF.md` and the linked receipt once, then read only the contracts required
by Next. [13_AGENT_EXECUTION.md](13_AGENT_EXECUTION.md) defines continuation;
[00_MASTER.md](00_MASTER.md) defines scope, not a per-turn checklist.

`PROJECT_GOAL.md` owns product acceptance, `docs/VERIFYING.md` test/build/publication,
and the board the only live queue. This specification belongs to existing P2-2p8,
not a second milestone. The board's focus may change as P2 advances. Record current
work/evidence there; never infer that implemented work is undone from static metadata.

The fixed-point DS runtime, native rendering in every ROM, substantially lower CPU
cost and stable 30 FPS for **every legal four-fighter lineup on every selectable
VS stage**, including repetitions and legal slots, remain required. Runtime includes
menus, audio control, transitions and rare states. Host tools may use floating point;
integer binary32 emulation is not the fixed-point endpoint. References stay read-only.

This directory is canonical. The nested delivery package under
`docs/optimization/Smash64DS_Implementation_Plan/` is a legacy imported copy, not
another active plan/queue. Preserve it as reference; do not reinstall or update its
static statuses during ordinary work.

## Documents

| File | Purpose |
|---|---|
| [00_MASTER.md](00_MASTER.md) | Decisions, sequence, budgets, first commits and integration rules |
| [01_CONTRACTS.md](01_CONTRACTS.md) | Native numeric, ownership, mutation, resource and packet interfaces |
| [02_BASELINE_AND_GATES.md](02_BASELINE_AND_GATES.md) | Reproducible measurement; explicit product gate |
| [03_TCM_AND_BUILD.md](03_TCM_AND_BUILD.md) | Bounded early ITCM reclamation, build/telemetry cleanup |
| [04_RESIDENCY_AND_ASSETS.md](04_RESIDENCY_AND_ASSETS.md) | Memory recovery and deterministic pre-GO asset admission |
| [05_BOUND_RENDERER.md](05_BOUND_RENDERER.md) | Binding and dynamic-update separation; eliminate source-shaped prep |
| [06_PACKET_COMPILER.md](06_PACKET_COMPILER.md) | Generated native GX streams, patching and hardware lifetime |
| [07_FIXED_NUMERICS.md](07_FIXED_NUMERICS.md) | Numeric graph, fixed primitives, constants and no-float closure |
| [08_EVENTS_AND_POSE.md](08_EVENTS_AND_POSE.md) | Event timing, compact pose, required sockets and transform reuse |
| [09_GAMEPLAY_COLLISION_AI.md](09_GAMEPLAY_COLLISION_AI.md) | Fixed gameplay, collision, shared query facts and ordering |
| [10_STAGE_VFX_UI.md](10_STAGE_VFX_UI.md) | Static/dynamic stage split, particles and native UI |
| [11_AUDIO_AND_OFFLOAD.md](11_AUDIO_AND_OFFLOAD.md) | Audio/storage bounds and justified DMA/ARM7/math offload |
| [12_FINAL_PACK_AND_RETIREMENT.md](12_FINAL_PACK_AND_RETIREMENT.md) | Whole-runtime cleanup, TCM tuning and release closure |
| [13_AGENT_EXECUTION.md](13_AGENT_EXECUTION.md) | Task handoffs, source ownership, safe integration and example commands |
| [14_VALIDATION.md](14_VALIDATION.md) | Concrete regression, negative, coverage and acceptance tests |
| [15_SOURCE_INDEX.md](15_SOURCE_INDEX.md) | Immutable evidence anchors; what is known versus proposed |
| [16_ALL_ROSTERS_ALL_STAGES.md](16_ALL_ROSTERS_ALL_STAGES.md) | Universal scope, exhaustive base/ordered cases, resource and timing proof, added tasks |
| `coverage-contract.json` | Machine-readable coverage rules; planning specification, not results |
| `tasks.json`, `tasks.csv` | Static dependency graph and implementation register |
| `templates/` | Unmeasured baseline, experiment and admission templates |

## Revising an adopted plan

This plan is already adopted in the reviewed repository. Do not apply the original
additive installer again or reset all task statuses. The static PLANNED fields in
`tasks.json` are not the live board; use mapped current implementation/evidence.
The workflow update modifies existing documents and four task implementation steps,
not the task graph or universal release contract. Preserve unrelated owner edits;
review a patch against its pinned base and reject conflicts instead of overwriting.

Edit split source documents and synchronize any task text also represented in
`tasks.json`/`tasks.csv`. Combined editions are derived reading copies, not another
source of instructions. Supporting research under `docs/optimization/` stays
lookup-only; `13_AGENT_EXECUTION.md` and `docs/VERIFYING.md` own the workflow.
