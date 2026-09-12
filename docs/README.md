# Documentation Index

One fact, one owner. `PROJECT_GOAL.md` owns product/fidelity/milestone acceptance. Current implementation state, blockers, artifact identity and next action live in `P2_EXECUTION_BOARD.md`. Static plans describe required outcomes and dependencies, not duplicate live completion counts.

## Active routing

| Document | Responsibility |
|---|---|
| `P2_EXECUTION_BOARD.md` | Only dynamic queue, current owner decisions, blockers, states and artifact identity |
| `HANDOFF.md` | Short restart route and immediate package pointer; no second live measurement table |
| `P2_PLAN.md` + `p2/` | Seven phases, shared dependencies, bounded outcome packages and source-backed unit contracts |
| `VERIFYING.md` | Environment/build, focused checks, A/B, registry use, capture and publish/checkpoint procedure |
| `HARNESSES.md` | Registry authority and harness naming; actual membership comes from the registry |
| `BUGS.md` | Owner-reported playtest wording/order; short permitted annotations only |
| `BUG_FIXING_PROCESS.md` | Bug diagnosis, native-only rule, observable proof and closure |
| `p2/BUG_NOTES.md` | Append-only investigation evidence, eliminated hypotheses and scoped measurements |
| `ARCHITECTURE.md` | Stable component/backend/source boundaries |
| `RAM_RECOVERY_PLAN.md` | RAM recovery/resource work; current P2 capacity contract is linked to the semantic estimator |
| `KNOWN_ISSUES.md` | Durable unresolved gaps |
| `BACKLOG.md` | Owner-deferred minor bugs |
| `OPTIMIZE_LIST.md` | Owner specialization/baking/offload wish list, not permission to override current priorities |
| `optimization/OPTIMIZATION_IDEAS.md` | Dated research/idea bin; corrections and current owner direction govern |
| `PERF_LEDGER.md` | Reproducible measurements and rejected experiments, lookup-only during ordinary work |
| `PORTING.md` | Append-only chronology, lookup-only |
| `DIAGNOSTIC_REFERENCE.md` | Diagnostic marker definitions and manual lookup |
| `DECOMP_MAP.md`, `FTSTRUCT_PARITY.md` | Source map and source/DS ABI reference |
| `../.agents/skills/n64-to-nds-porting/` | Project-specific source translation rules and semantic fixtures; not workflow policy |
| `../.agents/skills/nds-coding-practices/` | Project-specific DS hardware/API recipes; not scheduling, verification cadence or closure policy |
| `P3_Multiplayer/` | P3 wireless plans; outside P2 |
| `P4/` | Later new-character plans; outside original-content P2 |
| `../scripts/README.md` | Repository script layout/path convention |

## P2 owners

The existing `P2-1` through `P2-7` files remain the phase entry points. Existing `p2/fighters`, `p2/stages` and `p2/items` files own per-unit source/behavior/output requirements. Do not create another queue, one document per commit or an alternative mandatory workflow.

`p2/P2-2-pack-estimator.md` owns semantic set/cost verdicts; `p2/P2-texture-residency.md` owns required texture/palette/atlas admission; `p2/P2-1c-vram-map.md` owns scene bank/OBJ claims; `p2/RESULTS_OAM_DESIGN.md` owns Results composition. The stage parent owns Dream Land/PupupuSmall regression; the Yoshi stage unit owns YosterSmall; other campaign venues and all bonus boards have existing unit owners.

`p2/P2-1k-source-notes.md` and `p2/stages/stage-actor-census-2026-09-05.md` are dated reference indexes, not active instructions. Unsupported proposals and old “no route/not started” observations do not override current source/evidence.

## Retained history

The documentation bundle's guarded installer preserves each changed file at its reviewed baseline under `archive/P2_PLAN_BASELINE_2026-09-10/`, with paths relative to `docs/`. This is immutable lookup evidence, not another project workflow or live state owner. Until installed, each replacement document also links its immutable GitHub baseline.

Keep existing `archive/P2_CLOSED_ROWS.md`, archived P1 execution/Runtime2 plans and historical performance references. Do not erase useful failed-theory evidence or rewrite append-only investigations as current fact. Never bulk-delete `decomp/`, `artifacts/` or an uncertain worktree: parts are tracked; follow the actual repository cleanup policy.

| Document | Responsibility |
|---|---|
| `HW_RENDERER_VISIBILITY_FINDINGS.md` | Historical July 2026 hardware renderer visibility and texture-color checkpoint; lookup-only evidence, not current P2 acceptance |

## Editing rules

Prefer focused changes to the owning active document, preserving source pins and valid proof. Status/owner/next action go to the board; current source requirements go to the unit; detailed evidence goes to its existing owner. A source-present/compiled/runtime-verified/accepted distinction must survive every summary.

Protected `AGENTS.md`/`CLAUDE.md` and the product contract are not replaced by this documentation bundle. Follow their current owner permissions and explicit newer directions. Any remaining conflict in protected text is reported, not silently edited. New top-level documents require explicit need and an entry here; further splitting should normally use sections inside existing owners.
