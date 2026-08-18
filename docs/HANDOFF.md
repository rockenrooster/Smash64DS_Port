# Handoff

Current: 2026-08-17 — **P1 IS COMPLETE AND P2 IS PLANNED AND OWNER-APPROVED.**

## State

- **P1 closed.** R2-08 done, all five SwitchPlan §6 acceptance items
  discharged, gate met on the shipping configuration under the owner's
  population ruling (stress arm 95.20% / Boundary arm 98.38% two-VBlank;
  battlepack flip committed `603238b168e`). Published P1 ROM frozen:
  `smash64ds-battle-playable-hwtri.nds`, 12,530,688 B, SHA-256
  `2F47C8AC…CB2F`, source state `843fe40f4d2`. Full P1 detail:
  `docs/archive/HANDOFF_P1_FINAL.md` (verbatim final P1 handoff),
  `docs/archive/P1_EXECUTION_BOARD.md`, `artifacts/performance/`.
- **P2 contract** is in `PROJECT_GOAL.md` (milestone section rewritten
  2026-08-17): 7 phases — VS shell → 4-fighter engine → fighters → stages →
  items → 1P Game → modes/meta; standing 4-CPU stress gate (P95 ≤ 1.12M and
  ≥95% two-VBlank over all presented frames on the measured-hardest config,
  all items on). Wireless multiplayer is P3
  (`docs/P3_Multiplayer/Multiplayer.md`).
- **P2 plan tree**: `docs/P2_PLAN.md` (order, standing laws, owner decisions
  of 2026-08-17: bottom-screen battle HUD, intro cinematic deferred to P2-7,
  fighter/stage orders) + `docs/p2/` (7 phase subplans + per-fighter,
  per-stage, per-item-class unit files). Queue:
  `docs/P2_EXECUTION_BOARD.md` — the only dynamic queue; P2-1 rows seeded,
  all red/unowned.
- Boundary is unchanged until P2-1 closes: `battle_playable_realtime`, mode
  `163`, one-minute match, and the P1 configuration stays green as a
  regression guard for all of P2.

## Next

1. Take `P2-1a` (match-config seam) from the board — first on purpose:
   everything else plugs into the descriptor it creates.
2. Before implementing: read `docs/p2/P2-1-vs-shell.md`, then BattleShip
   `src/sc/` and `src/gm/` (how VS settings reach battle) and `src/mn/`
   (menu scenes). CodeGraph first per repo rules.

## Standing operational facts (carried from P1 — still load-bearing)

- Clean checkout builds through `build.ps1`, not bare `make` (four of six
  `.inc` are gitignored and `build.ps1`'s generator is not run by `make`).
  `make p1-tick` = measuring ROM, `make p1` = published P1 pair. Never pass
  `-j`, never override `MAKEFLAGS`, one build at a time, never build a
  published target name for lab work.
- Preserve: mode 163, renderer mode 9, mip 0, static textures, source
  countdown, Dream Land water frame 0, Task 16 `1/1/1`. Never edit `decomp/`.
- Push hygiene: owner-name scan is `git grep -l -i -e <owner-given-name>
  HEAD`; the single surviving hit (sm64 IDO `usr/lib/copt`) is a false
  positive. History's 16 scrubbed Cargo blobs are owner-accepted.
- Run `New-Smash64DSSnapshot.ps1` after verified progress.

## Start-of-cycle commands

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

Then read `docs/P2_EXECUTION_BOARD.md` and take the highest unowned red row.
