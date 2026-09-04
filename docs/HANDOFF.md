# Handoff

Current: 2026-09-03 — **P2-5 items: all twenty common items, five of the
thirteen Poké Ball Pokémon, and the fighter half of pickup are in the ROM and
exercised on a running ROM.** All eight VS stages boot and play.
`smash64ds.nds` rebuilds green at default flags; the board owns per-slice
state and `docs/p2/P2-5-items.md` owns the item detail and its traps.

Landed today: the eight-stage sweep closed (P2-4h1); nineteen common items plus
the Poké Ball; the monster bus and five Pokémon; P2-4n1 step 2 with Dream Land
output byte-identical (`sha256=eda2dbd6…`); `ftcommonget.c`, so a fighter can
finally hold an item.

**Boundary 2026-09-03:** `p2_shell_loop` PASS, `p2_battle_realtime` PASS.
`-Profile Boundary -List` is the membership authority; P2-2p8 is parked.

## Next

1. **The remaining eight Pokémon** — Iwark, Lizardon, Spear, Kamex, MLucky,
   Starmie, Sawamura, Pippi. Verbatim TUs under `src/it/itmonster/`; follow
   `battleship_item_nyars.c`, and register each maker in
   `sNdsITManagerProcMakeList` or the kind is unreachable however it is rolled.
   Arena is tight by ~1 page; `docs/p2/P2-5-items.md` carries the measurement.
2. **The VS Options and Item Switch screens.** State and commit rule already
   exist (`ndsMatchConfigItemTogglesFromRows`); what is missing is the two
   screens and their art. Entry point is the VS screen's OPTIONS row, which
   currently refuses (`nds_menu_shell_mode_vs.c:612`). The thirty-seven
   surfaces are sized and sourced in `docs/p2/P2-5-items.md`.
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
