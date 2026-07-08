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
- Keep DS/backend behavior in `src/nds` or `src/port`.
- Keep compatibility declarations in `include/`.
- Do not edit generated build outputs or local emulator payloads.
- Keep changes source-backed, buildable, and verifier-backed.
- User-facing ROMs must be verifier-covered build configurations.
- After successful verified progress, run
  `.\scripts\New-Smash64DSSnapshot.ps1` as the final project action/tool call.
  Finish docs, static checks, verifiers, and status inspection first; never run
  commands, probes, or status checks after the snapshot.

## Start Here

Use `.\scripts\verify-all.ps1 -Profile Boundary -List` as the authority for
current Boundary/Latest membership, then read these human summaries:

| Need | File |
|---|---|
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

Registry modes `161/162` remain the active legacy proof boundary:

```powershell
.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
```

They prove a bounded selected Fox Jab2 live-hit damage lifecycle and
damage-status follow-through on the Pupupu Mario/Fox battle root, inheriting
the current MP, ledge, damage, catch/throw, shield, rebound, and TaruCannon
hazard setup/physics proofs. Full marker strings live only in
`docs/DIAGNOSTIC_REFERENCE.md`.

This boundary is a regression anchor, not the template for future one-bit work.

## Common Commands

```powershell
$env:DEVKITPRO = 'C:/devkitPro'
$env:DEVKITARM = 'C:/devkitPro/devkitARM'
make NDS_DEV_SCENE_HARNESS=normal -j16
```

Static checks:

```powershell
.\scripts\check-docs.ps1
.\scripts\check-architecture.ps1
.\scripts\check-harness-registry.ps1
.\scripts\check-gbi-decode-fixtures.ps1
.\scripts\clean-generated.ps1 -DryRun
```

Tiered verifiers:

```powershell
.\scripts\verify-dev-fast.ps1
.\scripts\verify-boundary.ps1
.\scripts\verify-current.ps1
.\scripts\verify-regression.ps1
```

Use `verify-dev-fast.ps1 -Build -DelaySeconds 3` while iterating, and
`verify-boundary.ps1 -DelaySeconds 3` when a runtime slice appears done.
For shared-TU work, gate during the session on `RegressionCore`; run one full
fresh Regression prebuild plus four sharded `-NoBuild` runs at the end. Builds
expected to exceed 90 seconds should use `build-verify-profile.ps1 -Detach`,
then confirm completion with `build-verify-profile.ps1 -VerifyStamp`.

Do not commit runner slots, emulator configs/binaries, logs, or shard artifacts.

## Slice And Doc Policy

Future gameplay progress should move by runtime-first subsystem groups, not
one-bit markers: import original TUs, wire narrow seams, prove with continuous
natural-runtime/capture gates, then graduate live. Legacy bounded modes are
migrate-or-delete when superseded. Mechanical file splits and docs/tooling may
stay smaller when they do not claim gameplay progress.

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
- Do not add broad compatibility headers or broad original imports.
- Do not treat a stub as a completed subsystem.
- Remove temporary probes before handoff; keep verified diagnostics only.
