# Handoff

Current: 2026-09-03 — **P2-4 stage production is running.** Yoshi's Island and
Peach's Castle have their gameplay half landed opt-in; neither has been
accepted on a running ROM. `smash64ds.nds` rebuilds green; board owns its hash.

Landed today: ITCommonData resident (`45d5fead788`) unblocking P2-5; Castle
behind `NDS_P2_STAGE_CASTLE` (`65f3ac0edc4`); all eight stages pinned in
`docs/p2/stages/`, P2-5 in `docs/p2/P2-5-items.md`.

Instruments flush the data cache and publish their caller's return address; an
unflushed witness reads back as zero because the stub reads behind the cache.

**Boundary 2026-09-03:** `p2_shell_loop` PASS, `p2_battle_realtime` PASS,
`p2_fourcpu_stress` FAILS on the tick-HUD sampler's 3,600 s ceiling — the
parked P2-2p8 cost, measured, not a regression. Board row has the evidence.

## Next

1. **P2-5i1**, the item maker: `battleship_item_link_core.c:532-537` refuses
   every kind but the Link bomb. It gates its own phase *and* three stages —
   Castle's bumper, Mushroom Kingdom's Piranhas and POW, Saffron's Pokémon are
   all items. Castle will not link with its flag on until it defines
   `itManagerMakeItemSetupCommon`.
2. **Accept the two landed stages on a ROM.** Neither Yoster nor Castle has
   booted; `gNdsSCVSBattleStageGKind` plus the `gNdsSCVSBattleStage*` mask bits
   are what would prove each loaded its own ground data.
3. **P2-4s1 remainder**: Yoster's native stage packet (law 8), stage-select
   art, music, asset rows. Castle owes the same four.
4. **P2-3f47**: ten-flag both-CPU smokes, CSS capture, Boundary, stress arm.

Two standing P2-4 facts live in `docs/p2/P2-4-stage-production.md`: a measured
stage-order proposal, and that a map-object miscount is a silent boot hang.

Owner decision owed: `lbRelocGetForceExternHeapFile` hands back a raw heap
pointer on a miss instead of failing closed. Counted now
(`gNdsRelocForceFighterAnimFallbackCount`), so visible rather than silent.

## Delegation

Command Code (`cmdc`), model `meta/muse-spark-1.3-contributor` — no `--effort`
flag on that model. Guardrails are in `.commandcode/settings.json` and are
tool-enforced: `decomp/`, the owner's docs and `make` are denied. OpenCode is
capped until the owner re-enables it; do not run both skills.

## Active gates

`scripts/verify-all.ps1 -Profile Boundary -List` is the membership authority:
`p2_shell_loop`, `p2_battle_realtime` (mode 163), `p2_fourcpu_stress`.
P1 stays frozen. Owner directive: **no snapshot**; no new worktrees.

## Context discipline

Restart reads this file + `docs/P2_EXECUTION_BOARD.md` only. CodeGraph first,
then bounded reads of the returned seams. `PORTING.md`, `PERF_LEDGER.md`,
`VERIFYING.md`, `KNOWN_ISSUES.md` and the fighter/stage plans are lookup-only.
Bank verbose output to files and bring back status plus the failure window.
One build at a time; never pass `-j` or override `MAKEFLAGS`.

Start of cycle: `scripts/verify-all.ps1 -Profile Boundary -List`, `git status --short`.
