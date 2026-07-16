# Verifying

Use the least work that can falsify the change. Do not stack overlapping suites.

## Environment

```powershell
$env:DEVKITPRO = 'C:/devkitPro'
$env:DEVKITARM = 'C:/devkitPro/devkitARM'
```

Use only `emulators/melonds/melonDS.exe` for manual launch and repo-owned
`emulators/melonds-runners/slotN/melonDS.exe` copies for automation. Every TOML
uses the 488x675 vertical, equal-size, native-aspect, zero-gap, unswapped,
unfiltered, OSD-off profile. Ports `3333/3334` are manual-only; slot 0 uses
`4323/4324`, phase-FGM slot 1 uses `3343/3344`, and slot 2 uses `4463/4464`.
Lab outputs stay under `builds/`; exactly two ROMs publish at the repo root.

## Fast Iteration

1. Run the checker/build that directly covers the edited surface.
2. For performance, capture eight synchronized baseline frames (A) and eight
   candidate frames (B) with the same ROM configuration and frame window.
3. Compare P50/P95 ticks, FPS, a screenshot from each arm, automated screenshot
   analysis, and the cheap semantic/state/geometry/texture counters.
4. Stop on a decisive KEEP or REVERT.
5. Run A2 only when A/B is near the gate, median and P95 disagree, host drift is
   plausible, or counters/screenshots disagree. A2 must reproduce A; B must beat
   both controls.

Do not require routine A/B/A, 32-frame, or 128-frame promotion runs. Increase
sample count only when the eight-frame decision is genuinely inconclusive.
Historical experiments in `PERF_LEDGER.md` remain evidence, not current policy.

Useful existing commands:

```powershell
# Retained Mode-8 fighter-owner comparison
.\scripts\compare-renderer-fast-raw.ps1 -FastRunMode 8 `
  -RendererBenchmarkSamples 8 -RunnerSlot 3

# Mode-9 stage timing/capture arm
.\scripts\benchmark-renderer-fast-raw.ps1 -FastRunMode 9 `
  -StaticTextureAotMode 1 -IFCommonHybridOamMode 0 `
  -RendererProfileLevel 1 -RendererBenchmarkSamples 8 `
  -RendererBenchmarkStartFrame 438 -RunnerSlot 3 `
  -RendererBenchmarkExportPath artifacts/performance/m3.json `
  -RendererBenchmarkScreenshot artifacts/visibility/m3.png
```

All screenshots go under `artifacts/visibility`. A screenshot is evidence only
when the matching runtime counters and image-analysis gates pass.

## Focused Checks

Run only the relevant group:

```powershell
# Renderer
.\scripts\check-gbi-decode-fixtures.ps1
.\scripts\check-battle-playable-static-textures.ps1

# Collision/gameplay
.\scripts\check-mp-floor-crossing-fixtures.ps1
.\scripts\check-mp-topology-fixtures.ps1
.\scripts\check-ft-hitstatus-fixtures.ps1

# Audio
.\scripts\check-audio-id-fixtures.ps1
.\scripts\check-audio-bgm-derived-assets.ps1
.\scripts\check-audio-fgm-phase-pack.ps1

# Tooling/docs
.\scripts\check-docs.ps1
.\scripts\check-harness-registry.ps1
.\scripts\check-melonds-policy.ps1
```

Do not run all groups merely because they are cheap. `verify-dev-fast.ps1` is a
cross-domain checkpoint helper, not an every-edit command.

## Checkpoint Choice

Choose one widest relevant wrapper:

```powershell
# Battle-only source/backend change
.\scripts\verify-boundary.ps1 -DelaySeconds 3 -RunnerSlot 2

# Normal launch or shared startup/runtime change; replaces Boundary
.\scripts\verify-current.ps1 -Build -DelaySeconds 3 -RunnerSlot 2
```

If an unchanged ROM hash already passed the chosen wrapper, do not rerun it.
Use the one-minute gate only for timer/lifecycle/CPU/memory/M4-residency work or
release qualification. Use renderer forensic checks only when renderer semantics
changed. The retired profiles and modes no longer exist.

`verify-all.ps1 -Profile Boundary -List` is the membership authority. Boundary
currently contains only `battle_playable_realtime`, mode `163`.

## Emulator And Captures

Scripted launches normalize the selected runner TOML. Do not audit mutable TOMLs
on every run; `check-melonds-policy.ps1 -AuditLocalConfigs` is repair-only.
Runner volume is zero for host silence, while ROM audio channels/counters remain
live. Never alter the user's manual melonDS instance.

Use automated emulator/GDB/capture scripts only. For subjective play behavior,
build the verifier-covered ROM and ask the user to test it. Use no$gba only for
a specific VRAM/OAM/palette/DMA/register question melonDS cannot answer.

The P1 timer is one minute (`3600` source ticks). Never launch the obsolete
five-minute configuration.

## Snapshot

After docs, the chosen verifier, static checks, `git status` inspection, and commit:

```powershell
.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
```

The snapshot is the final project command. Run nothing afterward.
