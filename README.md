# Smash 64 DS Port

Nintendo DS source port of the BattleShip Super Smash Bros. 64 decompilation:

```text
Original BattleShip game code + Nintendo DS backend = playable port
```

`decomp/` is read-only. Original runtime imports live under `src/import`; DS
backends and compatibility seams live under `src/nds`, `src/port`, and `include`.

## P1

The active target is a verifier-covered Mario-versus-level-3-Fox match on Dream
Land, one-minute Time mode, items off, due **2026-07-19 23:59 America/Chicago**.
Canonical `battle_playable_realtime`, mode `163`, is the natural-runtime boundary.

Current artifact identity, blockers, gates, and ownership live only in
[`docs/P1_EXECUTION_BOARD.md`](docs/P1_EXECUTION_BOARD.md). Profile-1 and forensic
ROMs are laboratory evidence and never replace the two published root ROMs.

Presentation targets roughly 90% overall likeness under DS limits. After one
measured cosmetic attempt, keep the cheapest recognizable source-derived result;
gameplay semantics stay source-faithful. Screenshots belong under
`artifacts/visibility`.

## Work

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

Use [`docs/HANDOFF.md`](docs/HANDOFF.md) for the exact restart command and
[`docs/VERIFYING.md`](docs/VERIFYING.md) for the focused A/B verification loop.
The documentation index is [`docs/README.md`](docs/README.md).
