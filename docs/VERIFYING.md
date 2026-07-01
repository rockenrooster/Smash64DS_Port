# Verifying

This project uses a tiered verifier workflow. Use the smallest profile that
proves the changed boundary, then widen only when the risk justifies it.

## Environment

Use Windows PowerShell:

```powershell
$env:DEVKITPRO = 'C:/devkitPro'
$env:DEVKITARM = 'C:/devkitPro/devkitARM'
make NDS_DEV_SCENE_HARNESS=normal -j16
```

Use `NDS_DEV_SCENE_HARNESS=normal` for the shared `smash64ds.nds` target.
Harness builds should use their own `TARGET=` and `BUILD=` directories. The
profile `-Build` path forces a normal rebuild of the shared target so runtime
verification cannot accidentally sample a harness-flavored ROM.

## Static Checks

Run these for docs, tooling, registry, and renderer decode changes:

```powershell
.\scripts\check-docs.ps1
.\scripts\check-architecture.ps1
.\scripts\check-harness-registry.ps1
.\scripts\check-gbi-decode-fixtures.ps1
.\scripts\clean-generated.ps1 -DryRun
```

`check-docs.ps1` validates the docs index, `AGENTS.md` size budget, current
boundary references, and prospective stub/deferred markers.

`check-architecture.ps1` validates the important repo invariants that should
not rely on memory: `decomp/` is read-only, generated outputs are not tracked,
original imports stay in `src/import`, and DS/backend source stays under
`src/nds` or `src/port`.

## Verifier Profiles

Use the wrapper profiles while iterating:

```powershell
.\scripts\verify-dev-fast.ps1
.\scripts\verify-boundary.ps1
.\scripts\verify-current.ps1
.\scripts\verify-regression.ps1
```

`verify-current.ps1` is the Latest profile wrapper. Do not run both
`verify-current.ps1` and `verify-all.ps1 -Profile Latest` unless you are
testing profile plumbing.

After source changes that can affect normal boot/runtime, run:

```powershell
.\scripts\verify-current.ps1 -Build -DelaySeconds 3
```

Without `-Build`, the runtime leg can sample a stale shared `smash64ds.nds`
even when harness-specific targets were rebuilt.

Use Full only for major risk:

```powershell
.\scripts\verify-all.ps1 -Profile Full
```

Full is appropriate when adding/removing harness modes, changing Makefile source
lists, changing shared ABI or headers, changing taskman/object-manager/
controller/reloc/display shared code, changing registry/checker behavior,
preparing a major snapshot/release, or when explicitly requested.

If Full or Regression times out, report the timeout and resume with `-From`
through `verify-all.ps1`:

```powershell
.\scripts\verify-all.ps1 -Profile Regression -From battle_mariofox_stage_mpcliffwait_damage_loop
```

## Harness Selection

Harness names, modes, scripts, build directories, and profile membership come
from `scripts/lib/harness-registry.ps1`. List a profile without running it:

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
.\scripts\verify-all.ps1 -Profile Regression -List
```

Use `docs/HARNESSES.md` for naming rules and adding a new direct/menu-chain
pair.

## Emulator Choice

melonDS remains the machine-readable pass/fail verifier path. Use no$gba only
when melonDS cannot answer a hardware question such as VRAM, OAM, palettes,
BG/3D registers, DMA, or timing.

```powershell
.\scripts\debug-melonds.ps1 -Build
.\scripts\capture-melonds.ps1 -Build
.\scripts\debug-nogba.ps1 -Build
.\scripts\capture-nogba.ps1 -Build -AllWindows
.\scripts\verify-nogba-smoke.ps1 -Build
```

See `docs/EMULATOR_STRATEGY.md` for the emulator decision boundary.

## Snapshots

Use Lean snapshots for normal handoff:

```powershell
.\scripts\New-Smash64DSSnapshot.ps1
```

Lean excludes generated build products, root ROM/ELF outputs, emulator logs and
payloads, upstream decomp build output, duplicate nested O2R payloads, and tool
caches while keeping build-critical source/reference context.

Use CodeOnly only for small review handoffs that do not need `decomp/`:

```powershell
.\scripts\New-Smash64DSSnapshot.ps1 -Mode CodeOnly
```
