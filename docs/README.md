# Documentation Index

Use this file as the table of contents for project-owned docs. The dynamic P1
queue is `P1_EXECUTION_BOARD.md`; current summaries stay in `STATUS.md` and
`HANDOFF.md`, and historical detail stays in `PORTING.md`.

| Doc | Status | Purpose |
|---|---|---|
| `ARCHITECTURE.md` | Maintained | Port architecture, ownership boundaries, and large-file split plan. |
| `AUDIO_BACKEND_SCOUT.md` | Reference | Dated source-backed scout for DS audio backend work. |
| `DECOMP_MAP.md` | Maintained | Read-only upstream reference map for `decomp/`. |
| `DIAGNOSTIC_REFERENCE.md` | Maintained | Runtime marker and diagnostic contracts. |
| `EMULATOR_STRATEGY.md` | Maintained | melonDS/no$gba usage boundaries. |
| `FT_ANIM_STATUS_SCOUT.md` | Reference | Dated dependency scout for fighter animation/status graduation. |
| `FTSTRUCT_PARITY.md` | Maintained | Field-by-field FTStruct parity report against BattleShip source layout. |
| `GOAL_DEBUGGING.md` | Maintained | Debugging and verifier workflow notes. |
| `goal-objective.md` | Objective contract | User-owned delivery objective and acceptance criteria. |
| `HANDOFF.md` | Current truth | Detailed current handoff and latest proof evidence. |
| `HARNESSES.md` | Maintained | Harness naming, registry, and generated-index policy. |
| `HW_RENDERER_VISIBILITY_FINDINGS.md` | Reference | Dated July 8 HW realtime visibility findings. |
| `KNOWN_ISSUES.md` | Maintained | Known blockers, stubs, and deferred systems. |
| `MEMORY_OVERLAY_PLAN.md` | Maintained | Source-backed memory budget and reloc eviction plan. |
| `MP_PASS_THROUGH_SCOUT.md` | Reference | Dated source-order pass-through floor/platform scout. |
| `OPTIMIZATION_ROADMAP.md` | Reference | Concise renderer status and rejected-experiment synopsis; not a queue. |
| `optimization/NATIVE_RENDERER_PLAN.md` | Technical contract | Exact source/semantic/performance gates for renderer milestones M2–M4. |
| `P1_EXECUTION_BOARD.md` | Current truth | P1 acceptance matrix, lane ownership, dated gates, and integration decisions. |
| `PERF_LEDGER.md` | Maintained | Reproducible renderer benchmark identities, measurements, gates, and decisions. |
| `PORTING.md` | Append-only | Chronological porting history. Do not use as primary planning. |
| `ROADMAP.md` | Stable | P1/P2 milestone and dependency map; dynamic priority stays on the execution board. |
| `SPECIALS_WEAPONS_SCOUT.md` | Reference | Dated Mario/Fox specials and weapon-manager scout. |
| `STATUS.md` | Current truth | Short active planning state. |
| `VERIFYING.md` | Maintained | Build, static-check, verifier, emulator, and snapshot workflow. |

## Freshness Rules

- `STATUS.md` and `HANDOFF.md` must reference the latest Boundary profile from
  `scripts/lib/harness-registry.ps1`.
- New docs must be added to this index.
- Harness details belong in `HARNESSES.md` and the registry, not in
  `AGENTS.md`.
- Verification policy belongs in `VERIFYING.md`, not in `AGENTS.md`.
- Historical entries belong at the end of `PORTING.md`.
