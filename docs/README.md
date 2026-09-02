# Documentation Index

One fact, one owner. Current state lives only in `P2_EXECUTION_BOARD.md`;
historical detail stays in `PORTING.md` and `PERF_LEDGER.md`. P1 surfaces
(`P1_EXECUTION_BOARD.md`, `Smash64DS_Runtime2_SwitchPlan.md`, the final P1
`HANDOFF.md`) are archived under `archive/`.

| Doc | Owner |
|---|---|
| `P2_EXECUTION_BOARD.md` | Dynamic queue, artifact identity, blockers, dates, decisions |
| `P2_PLAN.md` + `p2/` | P2 execution order, cross-cutting laws, phase subplans, per-unit plans |
| `P3_Multiplayer/Multiplayer.md` | Owner's P3 wireless multiplayer design |
| `HANDOFF.md` | Restart surface and exact next command |
| `VERIFYING.md` | A/B iteration, verifier, emulator, capture, snapshot workflow |
| `HARNESSES.md` | Registry authority and harness naming |
| `ARCHITECTURE.md` | Stable source/backend/component boundaries |
| `KNOWN_ISSUES.md` | Unresolved durable gaps only |
| `BUGS.md` | User-reported playtest bugs |
| `BACKLOG.md` | Minor deferred bugs the owner parked for after P1-critical work |
| `BUG_FIXING_PROCESS.md` | Intake, diagnosis, proof, and closure process for `BUGS.md` |
| `archive/Smash64DS_Runtime2_SwitchPlan.md` | (archived) Runtime 2 charter — R2-08 complete 2026-08-17 |
| `RAM_RECOVERY_PLAN.md` | Main-RAM recovery and cache-residency plan for a bounded, pre-resident battle working set |
| `archive/SRC_REFACTOR_PLAN.md` | (archived) Completed source-organization refactor rationale, ownership boundaries, and verification constraints |
| `OPTIMIZE_LIST.md` | Owner's standing wish-list of subsystems to specialize, bake or offload |
| `optimization/OPTIMIZATION_IDEAS.md` | Dated optimization reviews and idea bin for the P95 gate (corrections at top govern) |
| `optimization/archive/TASK_STANDING_RULES.md` | (archived) Historical performance-task rules still cited by retained P1-era diagnostics; current measurement law lives in `VERIFYING.md` + the board |
| `PERF_LEDGER.md` | Reproducible measurements and rejected experiments |
| `DIAGNOSTIC_REFERENCE.md` | Marker definitions and manual diagnostics |
| `PORTING.md` | Append-only chronological history |
| `../PROJECT_GOAL.md` | Authoritative product, fidelity, milestone, and definition-of-done contract |
| `../scripts/README.md` | scripts/ directory layout, area-folder workflow, and Python path convention |
| `DECOMP_MAP.md` | Read-only upstream map |
| `FTSTRUCT_PARITY.md` | BattleShip/DS fighter ABI parity |
| `archive/AUDIO_BACKEND_SCOUT.md` | (archived) Dated audio reference |
| `HW_RENDERER_VISIBILITY_FINDINGS.md` | Dated renderer visibility reference |

Do not add a new planning or workflow document. Extend the existing owner or
delete obsolete material. New top-level docs must be indexed here.
