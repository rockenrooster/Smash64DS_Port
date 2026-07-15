# Smash 64 DS Port

This repository ports the BattleShip Super Smash Bros. 64 decompilation to
Nintendo DS:

```text
Original BattleShip game code + Nintendo DS backend = playable port
```

`decomp/` is read-only upstream reference material. Original runtime imports
live under `src/import`; Nintendo DS backends and compatibility seams live under
`src/nds`, `src/port`, and `include`.

## Current P1

The active release target is a verifier-covered Mario-versus-level-3-Fox match
on Dream Land, one-minute Time mode, items off, by **2026-07-19 23:59
America/Chicago**. Canonical `battle_playable_realtime`, mode `163`, is the
active Boundary profile. It is a natural BattleShip battle scene, not a
scripted combat proof.

Cut G renderer milestone 1 and the lower-screen text HUD are accepted. The
remaining release blockers include renderer milestones 2–4, natural recovery
coverage, fighter voices and audible Dream Land BGM, a dated canonical
one-minute soak, Full Regression, and final exact-ROM qualification. Current
acceptance state, lane ownership, dated gates, and evidence are maintained only
in `docs/P1_EXECUTION_BOARD.md`.

Current verifier-covered user ROM:

```text
smash64ds-battle-playable-hwtri.nds
14,368,768 bytes
SHA-256 F8EFEE10ED15457CD79A9B71B9766B5247BE870C332FB12316431F8301A0A94A
```

Profile-1 and special benchmark ROMs are laboratory evidence and must never be
published under that filename.

## Build And Verify

On Windows PowerShell:

```powershell
$env:DEVKITPRO = 'C:/devkitPro'
$env:DEVKITARM = 'C:/devkitPro/devkitARM'
make NDS_DEV_SCENE_HARNESS=normal -j16
.\scripts\verify-all.ps1 -Profile Boundary -List
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```

Use `docs/VERIFYING.md` for the tiered workflow and release gates. Automated
Codex runs use GDB ports `4333/4334`; `3333/3334` remain available for the
user's manual melonDS instance.

## Start Here

- `AGENTS.md` — source-port rules, ownership, verification, and snapshot policy.
- `docs/P1_EXECUTION_BOARD.md` — the only dynamic P1 queue.
- `docs/HANDOFF.md` — exact current commands and artifact handoff.
- `docs/STATUS.md` — concise current-truth summary.
- `docs/optimization/NATIVE_RENDERER_PLAN.md` — renderer M2–M4 technical contract.
- `docs/PERF_LEDGER.md` — measured optimization evidence and decisions.
- `docs/README.md` — complete documentation index.

Do not begin P2 implementation while a required P1 acceptance row is red. After
P1 passes, is documented, and is snapshotted, the project continues toward the
complete source-faithful port.
