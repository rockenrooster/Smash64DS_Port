# Handoff

Current: 2026-09-04 — **P2-5 items: 44 of the 45 kinds are in the ROM and
fighters can pick them up.** All 20 common items, all 13 Poké Ball Pokémon,
7 of 8 stage-spawned. All eight VS stages boot and play. `smash64ds.nds`
rebuilds green at default flags; the board owns per-slice state and
`docs/p2/P2-5-items.md` owns the item detail and its traps.

**Boundary 2026-09-04:** `p2_shell_loop` PASS, `p2_battle_realtime` PASS.
`p2_fourcpu_stress` fails as the board records it (parked, P2-2p8).
`-Profile Boundary -List` is the membership authority.

## Next

1. **The Item Switch and VS Options screens** — the last P2-5 slice with no
   code. State and commit rule already exist
   (`ndsMatchConfigItemTogglesFromRows`, transcribed from
   `mnVSItemSwitchSetItemToggles`); the 37 surfaces are sized and sourced in
   `docs/p2/P2-5-items.md`. Entry is the VS screen's OPTIONS row, which
   refuses today (`nds_menu_shell_mode_vs.c:612`).
2. **The bonus-stage Target** is written and compiles but is not linked: it
   needs `sc1PBonusStageUpdateTargetCount` and `gSC1PBonusStageItemFile`,
   which are P2-6 scene state. It links when the campaign lands.
3. **P2-4n1 steps 3-5** — checker parameterisation, then a second stage
   packet. What is still Dream Land-hardcoded: commit `aa1ba3949b1`.
4. **P2-3f47** — ten-flag both-CPU smokes, CSS capture, Boundary, stress arm.

Held: Congo Jungle and Sector Z music (loop starts near the track midpoint, the
signature of a doubled decode). Owner decision owed:
`lbRelocGetForceExternHeapFile` returns a raw heap pointer on a miss instead of
failing closed (`gNdsRelocForceFighterAnimFallbackCount` counts it).

## Delegation

OpenCode is the active skill (owner, 2026-09-03: "opencode-agent is back").
`opencode run --agent swarm-build|swarm-probe --variant Xhigh --auto`;
permissions are tool-enforced, so prompts carry scope, not rules. Build agents
write only their own new files and REPORT Makefile/header deltas. Require each
to syntax-check its own files. Before calling one stalled, check
`Get-Process opencode` CPU — a backgrounded run piped through `Select-Object`
writes nothing until it finishes, which reads exactly like the real stall.

## Context discipline

Restart reads this file + `docs/P2_EXECUTION_BOARD.md` only. CodeGraph first,
then bounded reads of the returned seams. `PORTING.md`, `PERF_LEDGER.md`,
`VERIFYING.md`, `KNOWN_ISSUES.md` and the phase plans are lookup-only.
Bank verbose output to files; bring back status plus the failure window. One
build at a time; never pass `-j` or override `MAKEFLAGS`, and run a plain
`make` before `verify-all.ps1` if the last build used lab flags — the
generators write shared untracked packs and the verifier tests the default
configuration. Owner directives:
**no snapshot**; no new worktrees.

Start of cycle: `scripts/verify-all.ps1 -Profile Boundary -List`, `git status --short`.
