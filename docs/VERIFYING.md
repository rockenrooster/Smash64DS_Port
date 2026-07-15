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

Codex-owned single runs use melonDS GDB ARM9/ARM7 ports `4333/4334`; ports
`3333/3334` are reserved for a user-opened manual melonDS instance. Runner
slots keep isolated mappings: FGM `3343/3344`, capture `3363/3364`, audio
`3373/3374`, M4 `3413/3414`, and countdown `4463/4464`. Scripted launches
normalize their selected TOML through the central profile. DevFast runs
`check-melonds-policy.ps1` to validate that profile and reject external emulator
paths without auditing mutable local TOMLs. Use `-AuditLocalConfigs` only for an
explicit drift audit; the all-worktree setter is creation/repair, not a recurring
check. Use scripted emulator/GDB/capture automation only.
For subjective play behavior, build the verifier-covered ROM and ask the user
to test it rather than controlling their desktop.
Keep Codex-owned runner-slot emulator volume at `0`; this mutes only host
playback and does not disable or bypass ROM audio channels/counters. Never
change the user's manual melonDS audio configuration.

Use `NDS_DEV_SCENE_HARNESS=normal` for the shared `smash64ds.nds` target.
Harness builds should use their own `TARGET=` and `BUILD=` directories. The
Makefile stores plain `BUILD=build*` outputs under `builds/` so root stays
readable. The profile `-Build` path forces a normal rebuild of the shared
target so runtime verification cannot accidentally sample a harness-flavored ROM.
If a compiler is killed after writing a dependency file, the Makefile repairs
malformed Windows `C:devkitPro` paths before parsing existing `.d` files; do
not clean the build directory merely for that recoverable signature.

## Static Checks

Run these for docs, tooling, registry, and renderer decode changes:

```powershell
.\scripts\check-docs.ps1
.\scripts\check-architecture.ps1
.\scripts\check-harness-registry.ps1
.\scripts\check-gbi-decode-fixtures.ps1
.\scripts\check-ft-hitstatus-fixtures.ps1
.\scripts\check-audio-id-fixtures.ps1
.\scripts\check-one-minute-match-verifier.ps1
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
.\scripts\verify-battle-playable-one-minute-match.ps1 -RunnerSlot 3
.\scripts\verify-p1-gate.ps1
.\scripts\verify-boundary.ps1
.\scripts\verify-current.ps1
.\scripts\verify-regression.ps1
```

`verify-current.ps1` is the Latest profile wrapper. Do not run both
`verify-current.ps1` and `verify-all.ps1 -Profile Latest` unless you are
testing profile plumbing.

`verify-dev-fast.ps1` runs melonDS policy, GBI, fighter hit-status, audio-ID,
and registry checks, then incrementally builds
only `smash64ds-battle-playable-canonical-hwtri` unless `-NoBuild` is passed,
and invokes its realtime verifier with `FastIteration`. It does not force a
normal-ROM `-B` rebuild. That path uses a minimum 25-second post-GO smoke and
then holds ARM9 at startup, installs exact frame-complete breakpoints, and
captures adjacent GO frames 438/439 while paused. Both frames must prove the
running timer, unlocked controls, native OAM/no-conversion state, visibility,
motion, named regions, detail, and sky before `latest.png` rotates. After the
canonical build, `check-battle-playable-rom-parity.ps1` requires identical byte
length and SHA-256 for canonical and shipped ROM names. Runner-slot captures use
that slot's emulator/config; stable alias rotation is serialized and atomic.

Store every generated screenshot under `artifacts/visibility/`; do not leave
captures in the repo root or temporary runner directories. Dated evidence names
must identify the date, ROM/configuration, and synchronized frame window.
`latest.png` and `previous.png` are stable aliases only, not substitutes for the
dated evidence file.

Every `verify-all` invocation of `battle_playable_realtime` uses that same exact
438/439 `FastIteration` capture and calibrated 50% / 45 pairwise limits. This
keeps Boundary/Regression decisions synchronized and repeatable. Invoke the
realtime script directly without `-FastIteration` only for a separate long live-
capture diagnostic; its host-delay pair is not canonical comparison evidence.

The shipped/canonical target is renderer profile 0. Renderer changes also need
the internal profile-2 correctness run; this is the same mode-163 scene and
source configuration, not another harness mode:

```powershell
.\scripts\verify-battle-playable-renderer-forensic.ps1 -DelaySeconds 3
```

For a warm 8-frame level-0/1/2 median and p95 comparison, add
`-RendererBenchmarkSamples 8`; invoke profile 1 through
`verify-battle-playable-harness.ps1 -RealtimePresentation
-RendererProfileLevel 1`. Ordinary DevFast does not run the forensic build, so
the visible one-ROM edit loop remains fast.

The realtime/canonical mode-163 build uses O2 for latency. The larger scripted
and timer/Results diagnostic builds use Os because O2 reduced their measured
scene headroom below the reserve gate. Profile 1 additionally requires
`RENDER_TOPOLOGY`, `RENDER_COST`, `RENDER_CI4LUT`, and `RENDER_HWDIV`: immutable reloc spans
must coexist with dynamic-list validation, and only profiles 0/1 may use the
adjacent-TRI replay/derived-value fast paths. Profile 2 retains the independent
generic interpreter, exact shade path, and old-C-division oracle.

FastIteration gates left shrub/stage body/pond at `40%/32px`, `18%/112px`, and
`20%/96px`; the normal path uses `45%/12px`, `18%/80px`, and `35%/80px`.
Fixed fighter-color crops are not gates because
the immediately preceding GDB verifier hard-proves both fighters' selected,
submitted, and in-bounds display contracts.

`P1Gate` is a shadow compact checkpoint with four legs: compact normal-opening
smoke, canonical realtime `FastIteration`, supplemental deterministic/scripted
mode-163 battle coverage, and the one-minute timer-to-Results lifecycle. It is
additive: Boundary, Regression, and Full profile memberships remain unchanged,
and legacy harnesses remain diagnostic. Passing it does not prove P1 completion
or replace the required canonical one-minute full-match soak. Unless `-NoBuild`
is passed, its wrapper incrementally prepares the normal opening ROM first;
explicit `-Build` retains the existing forced-normal-rebuild behavior.

Use DevFast for ordinary visual/renderer iteration, P1Gate for integrated
scene checkpoints, and Boundary once before handoff. Keep the older registry
fleet available for failure localization; do not run Full Regression merely to
repeat assertions already owned by those integrated gates.

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
.\scripts\verify-all.ps1 -Profile P1Gate -List
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

For faster visual iteration, pass `-Unthrottled` to `capture-melonds.ps1`.
It temporarily disables melonDS FPS limiting, keeps JIT disabled, and restores
the config afterward. The native software renderer remains the marker/geometry
baseline. The canonical texture-detail pixel ratchet explicitly passes
`-SoftwareRenderer -MaximizeVertical`, then normalizes the displayed top
screen back to native `256x192`; user-local window/OpenGL settings therefore
cannot move its source-backed regions or turn a known-good ROM white. Its
frame-delta gate registers only bounded source-camera motion (plus or minus
2% scale and 2px translation) and requires at least 95% overlap before applying
the unchanged 25% flashing ceiling. `-OpenGL4x` remains an optional secondary
inspection mode, not a pixel oracle; never compare its percentages with
software-renderer captures.

To capture an internal fast selector without changing a ROM's compiled
default, pass `-RendererFastRunMode 0..8` with its matching ELF present beside
the ROM. The script verifies the selected global through GDB, detaches, captures
the still-running emulator, and restores the original melonDS configuration.
After resizing an OpenGL window, allow at least one presented frame before a
comparison capture; a 100ms sample can catch the resize transition at 12fps.
Verifier launches use the same unthrottled interpreter policy automatically;
their assertions are tied to emulated frames/timers, not host wall time. The
non-runner config is restored after each verifier, while dedicated runner-slot
configs remain verifier-owned.
The match-lifecycle gate uses a one-minute harness setting (`3600` source
ticks) while retaining the original timer-expiry and Results transition path.
Effective 2026-07-14, one minute is the P1 ROM rule and the automated
iteration/soak duration. Do not launch the obsolete `18000`-tick five-minute
configuration; the full-duration acceptance gate is the natural `3600`-tick
expiry-to-Results path.

See `docs/EMULATOR_STRATEGY.md` for the emulator decision boundary.

## Snapshots

Use Lean snapshots for normal handoff:

```powershell
.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
```

Lean excludes generated build products, root ROM/ELF outputs, emulator logs and
payloads, upstream decomp build output, duplicate nested O2R payloads, and tool
caches while keeping build-critical source/reference context.

Use CodeOnly only for small review handoffs that do not need `decomp/`:

```powershell
.\scripts\New-Smash64DSSnapshot.ps1 -Mode CodeOnly
```
