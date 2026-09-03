# Handoff

Current: 2026-09-03 — **P2-4 stage production: all eight VS stages have their
gameplay half landed opt-in.** None has been accepted on a running ROM.
`smash64ds.nds` rebuilds green at default flags; the board owns per-stage state.

Landed today: ITCommonData resident; the item maker generalized to
`dITManagerProcMakeList` plus the spawn law and container drops; GBumper; the
ground-HAZARD seam (`ftMainCheckAddGroundHazard`, `ftMainClearHazard`,
`ftMainUpdateDamageStatGround`, a real `ftMainSearchGroundHit`); Congo Jungle's
barrel-cannon fighter half; seven BGM tracks; all eight stages pinned in
`docs/p2/stages/`. Instruments must flush the data cache and publish their
caller's return address -- an unflushed witness reads back as zero.

**Boundary 2026-09-03:** `p2_shell_loop` PASS, `p2_battle_realtime` PASS.
`-Profile Boundary -List` is the membership authority; P2-2p8 is parked.

## Next

1. **P2-4h1 — Peach's Castle hangs inside its battle.** Control: the same ROM
   on Dream Land passes a full lap in 105 s; Castle reaches scene entry 7,
   confirms with `gkind=00`/`walkloops=1`, then no further entry in 3,000 s --
   so not the scripted walk and not renderer speed. Ranked candidates: the
   GBumper spawn's data-dependent `DObjDesc` walk
   (`battleship_item_link_core.c:729`) and the platform joint animation.
   Instrument: `scripts/probe-trace-symbols.ps1 -Build
   build-p2-shell-loop-castle` over `grCastleInitAll`, `itManagerMakeItem`,
   `grCastleBumperProcUpdate`.
2. **Boot the other seven stages.** `gNdsSCVSBattleStageGKind` plus the
   `gNdsSCVSBattleStage*` mask bits prove each loaded its own ground data.
3. **P2-4n1, the native stage packet (law 8)** — one pipeline job, sized and
   ordered in `docs/p2/P2-4-stage-production.md:297`. Generator first.
4. **P2-5 items** — 43 kinds remain; order in `docs/p2/P2-5-items.md`.
5. **P2-3f47**: ten-flag both-CPU smokes, CSS capture, Boundary, stress arm.

Congo Jungle and Sector Z music is rendered but **held**: both loop starts sit
near the track midpoint (50.1% and 53.6%), the signature of a doubled decode.

Owner decision owed: `lbRelocGetForceExternHeapFile` hands back a raw heap
pointer on a miss instead of failing closed. Counted now
(`gNdsRelocForceFighterAnimFallbackCount`), so visible rather than silent.

## Delegation

OpenCode is the active skill (owner, 2026-09-03: "opencode-agent is back").
`opencode run --agent swarm-build|swarm-probe --variant Xhigh --auto --dir
<repo>`; permissions are tool-enforced, so prompts carry scope, not rules.
Build agents write only their own new files and REPORT the Makefile and header
deltas — those files are shared and are the orchestrator's to apply.

## Context discipline

Restart reads this file + `docs/P2_EXECUTION_BOARD.md` only. CodeGraph first,
then bounded reads of the returned seams. `PORTING.md`, `PERF_LEDGER.md`,
`VERIFYING.md`, `KNOWN_ISSUES.md` and the fighter/stage plans are lookup-only.
Bank verbose output to files and bring back status plus the failure window.
One build at a time; never pass `-j` or override `MAKEFLAGS`.
Owner directives: **no snapshot**; no new worktrees.

Start of cycle: `scripts/verify-all.ps1 -Profile Boundary -List`, `git status --short`.
