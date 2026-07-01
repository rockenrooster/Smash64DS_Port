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
- Keep DS/backend behavior in `src/nds` or `src/port`.
- Keep compatibility declarations in `include/`.
- Do not edit generated build outputs or local emulator payloads.
- Keep changes bounded, source-backed, buildable, and verifier-backed. Larger
  slices are acceptable when they follow one original-code path and end at a
  clear verifier boundary.
- After successful verified progress, run `.\scripts\New-Smash64DSSnapshot.ps1` as the final project action/tool call. Finish docs, static checks, verifiers, and status inspection first; never run commands, probes, or status checks after the snapshot.
- Update docs at meaningful boundary or handoff points. Do not churn docs for
  every internal helper, probe, or mechanical seam when the boundary summary
  already covers it.

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
| Append-only history | `docs/PORTING.md` |

`docs/PORTING.md` is history, not the planning surface.

## Current Boundary

Current active boundary summary; the registry wins if this drifts:

```powershell
.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1
```

Modes `161/162` prove bounded selected Fox Jab2 live-hit activation -> damage
lifecycle -> damage-status follow-through on the Pupupu Mario/Fox battle root,
inheriting the current MP, cliff, passive/recover, wall/rebound, catch/throw,
ledge, dash-run damage setup, and `mpDamageRecover` proofs. Summary:
`status=17->52/45`, `hitlag=6->0`, callbacks `1/6/1`, search `0xf`, repeat
gate `1/1 gate=0x3f`, statusMask `0xffffffff`, damage `0xffffffff`, hbdmg `0x7ffff`, shc `0x7fffff`, catchSearch `0xffffffff`.
See `docs/STATUS.md` and `docs/HANDOFF.md`.
Use `-DelaySeconds 3` for boundary/current verifier profiles.

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

For parallel melonDS regression shards, create local gitignored runner slots,
prebuild once, then run shards with `-NoBuild`:

```powershell
.\scripts\New-MelonDSRunnerSlots.ps1 -Count 4
.\scripts\build-verify-profile.ps1 -Profile Regression
.\scripts\verify-all.ps1 -Profile Regression -ShardCount 4 -ShardIndex 0 -RunnerSlot 0 -NoBuild
```

Do not commit runner slots, emulator configs/binaries, logs, or shard artifacts.

Run Full only when the risk requires it or the user asks:

```powershell
.\scripts\verify-all.ps1 -Profile Full
```

## Emulator Policy

melonDS is the automated GDB verifier path. no$gba is for interactive DS
hardware/register/VRAM/OAM/palette/timing debugging. See
`docs/EMULATOR_STRATEGY.md`.

## Snapshot Policy
Run a snapshot after successful verified progress, always as the final project
action after docs, verification, static checks, and status inspection. Run no
commands after it; only the final response may follow.

```powershell
.\scripts\New-Smash64DSSnapshot.ps1
```

## Slice And Doc Policy

Prefer one coherent source-backed slice over many tiny proof-only edits when
the original BattleShip path is understood and the verifier boundary remains
bounded. A good slice may import or route several adjacent original helpers
together, but it must keep runtime restored after the proof and avoid broad
subsystem imports.

Keep docs lean during implementation. Update `docs/STATUS.md` and
`docs/HANDOFF.md` when the current boundary, latest proof, verifier command, or
known blocker changes. Append `docs/PORTING.md` only after a verified milestone
or important architectural decision. Avoid duplicating full marker strings
across multiple docs unless a verifier or reviewer needs the exact value.

## Parallel Agents

Use parallel agents for bounded scouting, docs synthesis, verifier/tooling work,
or collision/renderer research. The main agent owns merge decisions.

## Editing Policy

- Use `apply_patch` for manual source/doc edits.
- Do not revert user changes unless explicitly requested.
- Do not add broad compatibility headers or broad original imports.
- Do not treat a stub as a completed subsystem.
- Remove temporary probes before handoff; keep verified diagnostics only.
