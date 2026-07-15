# AGENTS.md

## Mission

This repo is a Nintendo DS source port of the BattleShip Smash 64
decompilation:

```text
Original Smash 64 decomp game code + Nintendo DS backend = playable port
```

Do not turn it into a handwritten Smash clone or DS-native gameplay rewrite.

## Hard Rules

- Treat `decomp/` as read-only reference source. Never edit it.
- Inspect relevant BattleShip source before importing or replacing behavior.
- Inspect `decomp/sm64-nds` before DS backend architecture changes.
- Prefer importing original BattleShip translation units through `src/import`.
- Runtime-first gameplay slices import coherent original subsystem TU groups
  for a scene-level capability, then graduate them live.
- Ban new one-bit proof-mask increments. Do not add per-branch seed/restore
  proofs when the whole function or TU can run naturally.
- Do not add proof code whose only purpose is to rerun one already-imported
  branch with synthetic state. Convert proven paths to live runtime instead.
- When a subsystem's TUs are fully imported, remove the guarded seam and let
  the original code run live in-scene. Keep the old proof as a regression
  marker only; no permanent state-restore around proven runtime.
- Legacy bounded modes are migrate-or-delete. When a runtime slice obsoletes
  a marker stack, delete its mode/verifier and leave one `[coverage-reduced]`
  `KNOWN_ISSUES` line instead of reproducing old markers.
- New harness modes are only for scene-level capabilities such as
  `battle_playable`; otherwise use Boundary/Latest or continuous natural-runtime
  verifier plus captures.
- Keep DS/backend behavior in `src/nds` or `src/port`; compatibility declarations in `include/`.
- Do not edit generated build outputs or emulator payloads; keep changes source-backed, buildable, and verifier-backed.
- User-facing ROMs must be verifier-covered build configurations.
- Publish exactly two root ROMs: `smash64ds.nds` for the original launch path and
  `smash64ds-battle-playable-hwtri.nds` for P1; all other outputs stay in `builds/`.
- After successful verified progress, run
  `.\scripts\New-Smash64DSSnapshot.ps1` as the final project action/tool call.
  Finish docs, static checks, verifiers, and status inspection first; never run
  commands, probes, or status checks after the snapshot.

## Start Here

Use `.\scripts\verify-all.ps1 -Profile Boundary -List` as the authority for
current Boundary/Latest membership, then read these human summaries:

| Need | File |
|---|---|
| Only dynamic P1 queue, ownership, dated gates | `docs/P1_EXECUTION_BOARD.md` |
| Active handoff and exact current commands | `docs/HANDOFF.md` |
| Current boundary summary, latest proof, blockers | `docs/STATUS.md` |
| Verification workflow | `docs/VERIFYING.md` |
| Harness registry and naming rules | `docs/HARNESSES.md` |
| Architecture and split plans | `docs/ARCHITECTURE.md` |
| Known blockers and deferred work | `docs/KNOWN_ISSUES.md` |
| Read-only upstream map | `docs/DECOMP_MAP.md` |
| Full marker strings and diagnostics | `docs/DIAGNOSTIC_REFERENCE.md` |
| Append-only history | `docs/PORTING.md` |

`docs/PORTING.md` is history, not the planning surface.

## Current Boundary

Canonical `battle_playable_realtime` mode `163` is the active natural-runtime
boundary:

```powershell
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```

It proves the source one-minute Mario-human/Fox-CPU scene, exact Wait-to-GO
control/timer behavior, retained affine BG2 wallpaper, and live stage/fighter
hardware submission. Legacy modes `161/162` remain diagnostic-only because
their bounded input driver assumes pre-GO movement. Full marker strings live
only in `docs/DIAGNOSTIC_REFERENCE.md`.

P1 timer policy (2026-07-14): the canonical/user-facing ROM and automated
expiry/stability gates use the original one-minute (`3600` tick) rule. Do not
launch the obsolete five-minute configuration.

## Common Commands

```powershell
$env:DEVKITPRO = 'C:/devkitPro'
$env:DEVKITARM = 'C:/devkitPro/devkitARM'
make TARGET=smash64ds BUILD=build NDS_DEV_SCENE_HARNESS=normal NDS_HARNESS_FAST_LOGIC=0 -j16
make TARGET=smash64ds-battle-playable-hwtri BUILD=build-battle-playable-canonical-hwtri-harness -j16
.\scripts\check-published-roms.ps1 -RequireBoth
```

Tiered verifiers:

```powershell
.\scripts\verify-dev-fast.ps1
.\scripts\verify-boundary.ps1
.\scripts\verify-current.ps1 -Build
```

Use `verify-dev-fast.ps1 -DelaySeconds 3` while iterating, `verify-boundary.ps1
-DelaySeconds 3` for a finished runtime slice, and `verify-current.ps1 -Build`
when the original launch path can be affected.
`Full`, `Regression*`, and `P1Gate` are list-only legacy inventories until their
checks are migrated onto the two runtime ROMs; never execute or prebuild them.
Focused lab builds may run only with their non-published output inside `builds/`.

Use only repo-local `./emulators` melonDS binaries and the canonical window profile in every TOML; do not commit runner slots, emulator configs/binaries, logs, or shard artifacts.

## Work Selection And Doc Policy

Select the highest-impact unowned red P1 row from `P1_EXECUTION_BOARD.md`.
Gameplay progress moves by runtime-first subsystem groups: import original TUs,
wire narrow seams, prove natural runtime/captures, then graduate live.

Keep all three subagent slots active when three useful P1 packets exist and
reassign completed slots. Agents share the live tree, so each lane claims a
disjoint file set, build directory, and runner slot. Only integration/release edits
central files/docs. Define a hypothesis, exclusive counters, and a
keep/revert threshold; do not begin P2 while a required P1 row is red.

Legacy bounded modes are migrate-or-delete when superseded. Mechanical splits
and tooling may stay smaller when they do not claim gameplay progress.

New harness modes are only for scene-level milestones. Registry changes go
through `scripts/lib/harness-registry.ps1` and
`.\scripts\check-harness-registry.ps1`.

Docs stay short. Keep `docs/STATUS.md` and `docs/HANDOFF.md` at or under 150
lines each. Put full marker strings in `docs/DIAGNOSTIC_REFERENCE.md`; append
increment history to `docs/PORTING.md`; do not duplicate previous-increment
stacks across active docs.

## Snapshot Policy

Run a snapshot after successful verified progress, always as the final project
action after docs, verification, static checks, and status inspection. Run no
commands after it; only the final response may follow.

```powershell
.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
```

## Editing Policy

- Use `apply_patch` for manual source/doc edits.
- Do not revert user changes unless explicitly requested.
- Do not add broad compatibility headers/imports or treat a stub as a completed subsystem.
- Remove temporary probes before handoff; keep verified diagnostics only.
