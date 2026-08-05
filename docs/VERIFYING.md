# Verifying

Use the least work that can falsify the change. Do not stack overlapping suites.

## Environment

Run every repository command from PowerShell 7 (`pwsh`). Do not use Windows
PowerShell 5.1. This is not a preference: `scripts/lib/melonds.ps1:349` uses a
PS7 ternary, so *every* harness script that dot-sources it is a parse error
under 5.1 — including any launched indirectly, which is how a probe spending
`powershell` inside a gdb `shell` line lost a run on 2026-08-01. The failure
reads as `UnexpectedToken` in the innocent caller, never in the file that
actually holds the PS7 syntax.

```powershell
$env:DEVKITPRO = 'C:/devkitPro'
$env:DEVKITARM = 'C:/devkitPro/devkitARM'
```

Use only `emulators/melonds/melonDS.exe` for manual launch and repo-owned
`emulators/melonds-runners/slotN/melonDS.exe` copies for automation. Never a
system, PATH, or package-manager melonDS. After replacing the source executable,
refresh every slot with `.\scripts\New-MelonDSRunnerSlots.ps1 -Count <N> -Force`;
`check-melonds-policy.ps1` fails if any slot binary is not that exact build, so
manual and sharded runs can never disagree about which emulator ran. Every TOML
uses the 416x664 outer-window profile: its 400x600 content viewport is the exact
2:3 aspect of two stacked 256x192 screens, with no capture bars, equal sizing,
zero gap, no swap, nearest filtering, and OSD off. Ports `3333/3334` are manual-only; slot 0 uses
`4323/4324`, phase-FGM slot 1 uses `3343/3344`, and slot 2 uses `4463/4464`.
Lab outputs stay under `builds/`; exactly two ROMs publish at the repo root.

## Building For P1

P1 ships `smash64ds-battle-playable-hwtri.nds` and is measured on its
flag-identical tick-HUD sibling. `make p1-tick` builds the measuring ROM (the
cheap compile check during iteration); `make p1` also rebuilds the published
battle ROM and its root pair. Bare `make` builds the P2 `smash64ds.nds` the
milestone does not ship — the Makefile says so when TARGET was defaulted. The
harnesses still build for themselves; the aliases use their exact TARGET/BUILD
pairs, so alias and harness builds stay incremental with each other. One build
at a time, never `-j`, never touch `MAKEFLAGS`.

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
6. When a finding exposes a repeatable mistake or inefficiency, improve the
   existing shared helper, checker, or owning doc that prevents recurrence. If
   that is not safe and in scope, record one concise actionable item there.

Do not require routine A/B/A, 32-frame, or 128-frame promotion runs. Increase
sample count only when the eight-frame decision is genuinely inconclusive.
Historical experiments in `PERF_LEDGER.md` remain evidence, not current policy.

R2-07 iteration rules (cycle 79):

- **Gate readings are whole-match only** — `sample-tick-hud-buckets.ps1
  -RingDump`, 1,600 samples, frames 440–2040, DLDI-on. A 128-frame window
  reads the cheapest 6% of the match. Reserve the whole-match run for banked
  baselines and KEEP decisions.
- **Prefer one dual-route binary over two linked ROMs for A/B.** Route the
  candidate at runtime behind a gdb-settable flag (the
  `NDS_R2_STAGE_ROUTE_PROBE` pattern): one build, both arms in one run, zero
  placement noise. Separately linked A/B ROMs have already confused two
  comparisons on this placement-sensitive ROM.
- **Per-list constants make short probes valid for iteration.** Effect-submit
  cost is ~102,730 ticks/list; ticks-per-list from a few stops is a sound
  progress metric for board lane G3. The P95 verdict still needs the
  whole-match run.
- **New tables or code: state the byte cost and run the 8-sample
  `-StartFrame 60` boot probe (~50 s) before any measuring run.** The ROM is
  ~1.4–2.2 KB from a boot cliff, and text counts as much as bss.

Useful existing commands:

```powershell
# Retained Mode-8 fighter-owner comparison
.\scripts\compare-renderer-fast-raw.ps1 -FastRunMode 8 `
  -RendererBenchmarkSamples 8 -RendererBenchmarkTimeoutSeconds 120 `
  -RunnerSlot 3

# Mode-9 stage timing/capture arm
.\scripts\benchmark-renderer-fast-raw.ps1 -FastRunMode 9 `
  -StaticTextureAotMode 1 -IFCommonHybridOamMode 0 `
  -RendererProfileLevel 1 -RendererBenchmarkSamples 8 `
  -RendererBenchmarkStartFrame 438 -RunnerSlot 3 `
  -RendererBenchmarkExportPath artifacts/performance/m3.json `
  -RendererBenchmarkScreenshot artifacts/visibility/m3.png

# Natural source-event timing (run once with KO, once with Rebirth)
.\scripts\benchmark-renderer-fast-raw.ps1 -FastRunMode 9 `
  -StaticTextureAotMode 1 -FoxCpuMode 1 -RendererProfileLevel 1 `
  -RendererBenchmarkSamples 8 -RendererBenchmarkStartEvent KO `
  -RendererBenchmarkTimeoutSeconds 300 -RunnerSlot 3
```

The Mode-8 comparator accepts integer-array or space-delimited exported rows
and fails closed when a projected semantic field is missing.

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

`verify-all.ps1` runs three of the forty-seven `check-*.ps1` scripts itself:
`check-gbi-decode-fixtures`, `check-harness-registry`, and (since 2026-08-01)
`check-nds-particle-banks`. The other forty-four are hand-run, which on
2026-08-01 meant the particle-bank pins sat stale across a commit and cost seven
failing runs of arrears to clear. **Actionable:** when a checker pins numbers
that a generator can move, wire it into `verify-all.ps1` at the point of the
change rather than trusting anyone to remember it. The generic version of that
fix -- a static-checker aggregator -- is not worth building until a second
checker has actually gone stale, because most of the forty-four need a specific
ROM or build and would turn one wrapper into a fleet.

## Checkpoint Choice

Choose one widest relevant wrapper:

```powershell
# Battle-only source/backend change
.\scripts\verify-boundary.ps1 -DelaySeconds 3 -RunnerSlot 2

# Normal launch or shared startup/runtime change; replaces Boundary
.\scripts\verify-current.ps1 -Build -DelaySeconds 3 -RunnerSlot 2
```

**`smash64ds.nds` is not part of P1** (owner, 2026-08-02), so `-Build` is the
wrong default reflex. P1 ships `smash64ds-battle-playable-hwtri.nds`, and
Boundary above exercises it. Use `verify-current.ps1 -Build` only when a change
genuinely touches normal or shared startup: it rebuilds the default
configuration, which costs a full cycle on a ROM the milestone does not ship.
Several consecutive cycles went to it during the 2026-08-02 `BUGS.md` queue
before that was caught.

TWO GATES IN ONE SESSION FAILED FROM WINDOW OCCLUSION, NOT FROM THE ROM
(2026-08-02). `assert-melonds-horizontal-detail` threw on `left_bush`, and
`soak-freeze-watch` returned a FREEZE verdict — and `soak-freeze-watch` then
contradicted its own verdict in the same report: *"the guest presented 9184
frames across 22358 VBlanks — 2.4 VBlanks per frame, which is a normally paced
ROM, not a stopped one. Suspect the CAPTURE before the ROM."* It also warns that
an attached GDB halts a RUNNING core at an arbitrary PC, so the backtrace it
prints beside a false freeze is not evidence of a hang. **Read the harness's own
contradiction block before acting on its verdict**, and treat a picture-frozen
verdict with healthy VBlank pacing as a capture failure until proven otherwise.
The cost of not doing so is high: the first of these nearly reverted a correct
fix, and the second aborted a run before its counter dump.

A SCREENSHOT GATE'S FIRST FAILURE IS A MEASUREMENT, NOT A VERDICT.
`assert-melonds-horizontal-detail.ps1` samples a named region of a captured
frame, and capture runs on an interactive desktop — a foregrounded window, or a
fighter standing in the sampled region, lowers its variation. On 2026-08-02 it
threw `left_bush variation 22.379%` against a 40% floor and a correct change was
nearly reverted on that one arm; re-running the same candidate passed. Re-run
before believing it, the same way an A/B would.

When `-Build` is genuinely warranted: it is the **only** routine command that
builds the default configuration, and the default is the published
`smash64ds.nds`:
`NDS_RENDERER_HW_TRIANGLES ?= 0` with `NDS_R2_PARTICLE_RUNTIME ?= 1`. Every lab,
tickhud, and `-hwtri` build overrides the first to `1`, so a function defined
inside `#if NDS_RENDERER_HW_TRIANGLES` and called from an unguarded caller links
everywhere except the ROM that ships. That is not hypothetical: on 2026-08-02
`ndsRendererSetParticleCamera` had been in that state, and `make` with no
overrides failed at link on that one symbol while the whole campaign stayed
green. A linker is the only sound checker for this, so there is no static guard
-- run this wrapper before any commit that publishes. When adding a symbol
inside that `#if`, define its twin in the `#else` in the same edit.

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

Concurrency: measuring runs read guest-deterministic counters, so concurrent
runs on separate slots should not move each other's tick series — but that is
unproven until the board's parked calibration row passes (one solo-vs-paired
run of the same ROM, identical ticks). Until then keep measuring runs solo.
Screenshot-gated runs must never overlap another emulator window regardless:
capture runs on the interactive desktop, and occlusion has already produced
two false failures. Wall-clock liveness verdicts (STALLED / TOO SLOW) read
observed frames/s, which host contention lowers — read the harness's own
contradiction block before believing one.

Use automated emulator/GDB/capture scripts only. For subjective play behavior,
build the verifier-covered ROM and ask the user to test it. Use no$gba only for
a specific VRAM/OAM/palette/DMA/register question melonDS cannot answer.

`capture-melonds.ps1` uses screen capture in an interactive desktop and native
`PrintWindow` only when that fails in a disconnected session. The fallback is
evidence only when the unchanged visibility, region, motion, and detail gates
pass; a successful API call alone never qualifies an image.

The P1 timer is one minute (`3600` source ticks). Never launch the obsolete
five-minute configuration.

## Snapshot

After docs, the chosen verifier, static checks, `git status` inspection, and commit:

```powershell
.\scripts\New-Smash64DSSnapshot.ps1
```

The snapshot is the final project command. Run nothing afterward.
