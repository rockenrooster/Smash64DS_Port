# Documentation Index

Use this file as the table of contents for project-owned docs. Current planning
starts in `STATUS.md` and `HANDOFF.md`; historical detail stays in
`PORTING.md`.

| Doc | Status | Purpose |
|---|---|---|
| `ARCHITECTURE.md` | Maintained | Port architecture, ownership boundaries, and large-file split plan. |
| `DECOMP_MAP.md` | Maintained | Read-only upstream reference map for `decomp/`. |
| `DIAGNOSTIC_REFERENCE.md` | Maintained | Runtime marker and diagnostic contracts. |
| `EMULATOR_STRATEGY.md` | Maintained | melonDS/no$gba usage boundaries. |
| `FTSTRUCT_PARITY.md` | Maintained | Field-by-field FTStruct parity report against BattleShip source layout. |
| `GOAL_DEBUGGING.md` | Maintained | Debugging and verifier workflow notes. |
| `HANDOFF.md` | Current truth | Detailed current handoff and latest proof evidence. |
| `HARNESSES.md` | Maintained | Harness naming, registry, and generated-index policy. |
| `KNOWN_ISSUES.md` | Maintained | Known blockers, stubs, and deferred systems. |
| `MP_PASS_THROUGH_SCOUT.md` | Maintained | Source-order scout for the next pass-through floor/platform boundary. |
| `NEXT_BOUNDARY_QUEUE.md` | Current truth | Compact next-boundary candidate queue. |
| `PORTING.md` | Append-only | Chronological porting history. Do not use as primary planning. |
| `ROADMAP.md` | Maintained | Milestone-level project roadmap. |
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
