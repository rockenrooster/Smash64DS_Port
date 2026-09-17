> **Legacy imported package copy — not the active implementation plan.**
> The repository-root path `docs/p2/native-optimization/` is canonical and
> `docs/P2_EXECUTION_BOARD.md` owns the execution cursor and live status.
> Preserve this delivery copy as reference; do not execute its install patch,
> synchronize its PLANNED statuses, or use its historical next steps to restart work.

# P2-2p8 native-runtime implementation campaign

**Plan date:** September 15, 2026. **Revision 2:** all legal four-fighter lineups on every selectable VS stage. **Original research baseline:** `75f7f6b4b4864c82c01872d0fd2771d171005272`. **Repository rechecked for this revision:** `e67e5871ba8c4ae972f4826bfeb89757d3686401` on `master`. Old measurements remain historical; implementation must rebase against the current board.

This is an implementation specification, not an implemented optimization or a new benchmark. All task states begin **PLANNED**. Proposed paths, APIs, formats and test IDs are explicitly new work; existing anchors refer to the pinned repository. No ROM was built and no GitHub branch was modified while preparing this package.

## Start and authority

Read [00_MASTER.md](00_MASTER.md), then the assigned package, relevant contracts in [01_CONTRACTS.md](01_CONTRACTS.md), and its tests in [14_VALIDATION.md](14_VALIDATION.md). Do not reread the whole research report on every restart.

`PROJECT_GOAL.md` remains the product authority. `docs/VERIFYING.md` remains the execution procedure. `docs/P2_EXECUTION_BOARD.md` remains the **only live queue**. Register this campaign beneath existing **P2-2p8**, not as a second competing milestone. The task graph here is a static implementation breakdown; record current status only in the existing board/evidence, or generate a view from it. Do not manually maintain two status ledgers.

The owner's requested endpoint is a smaller, fixed-point DS runtime, substantially lower CPU time, native rendering in every ROM, and stable 30 FPS for **every legal four-fighter lineup on every selectable VS stage**, including repeated fighters and legal slot assignments. Runtime includes menus, audio control, transitions and rare game states, not just the measured battle loop. ROM-derived source assets, source data and reference code remain read-only; build tools may use floating point. Integer code that emulates binary32 arithmetic is not fixed point.

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

## Install without overwriting active work

These files are additive under `docs/p2/native-optimization/`; the supporting plan validator is under `planning-tools/`. For a first install, copy into a clean review branch or apply `INSTALL_ADDITIVE.patch` after `git apply --check`. For an exact original-v1 installation, use `UPGRADE_FROM_V1.patch` instead, after its own `git apply --check`. Do not apply both. Refuse collisions or hand-edited v1 context mismatches rather than overwrite active work; review/merge changes when necessary. Inspect `git status --short` first. The package does not edit `decomp/`, existing generated outputs, existing P2 plans, or the root ROM.

Add a link from the existing P2-2p8 board row and one pointer from the handoff when adopting the campaign. Keep the original research document as supporting analysis, not as a second execution queue.

The standalone `CHANGES_v2.md` at the package root summarizes revisions. The current repository already carries earlier research under `docs/optimization/`; preserve it as historical analysis and link this campaign from the existing board on adoption. No existing repository plans or research files are overwritten.

A standalone combined Markdown edition is also provided for reading. Edit the split source documents and regenerate the combined view; do not maintain both by hand.
