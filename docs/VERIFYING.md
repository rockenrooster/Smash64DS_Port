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

**Capturing a verifier's output needs an OS-level redirect, not a PowerShell
one.** `verify-all.ps1` prints each child verifier's stdout with
`[Console]::Out.Write` (`Invoke-VerifyScriptOnce`), which writes straight to the
console handle — so `| Tee-Object`, `> file` and `*> file` all capture the
*driver's* one progress line and none of the run. On 2026-08-15 that produced a
90-byte log for a failing Boundary run and cost two repeats. Use:

```powershell
cmd /c "pwsh -NoProfile -ExecutionPolicy Bypass -File scripts\verify-all.ps1 -Profile Boundary > %TEMP%\b.log 2>&1"
```

`cmd`'s redirection is a handle the whole process tree inherits. Note `pwsh`,
not `powershell`, for the reason in the paragraph above — spelling it
`powershell` inside the `cmd` line reintroduces 5.1 and fails at
`melonds.ps1:349`.

**Edit every structured file with Read/Edit, not a heredoc or `\n` escapes.**
`CLAUDE.md` records this for `.ps1`; on 2026-08-15 the same trap ate a backslash
in a **Makefile** recipe continuation. The rule is not about PowerShell quoting —
it is about any file whose meaning depends on exact line endings, escapes or
continuations: `.ps1`, `Makefile`, `.mk`, linker scripts, `.S`, `.toml`, `.json`.

**Why this rule cannot be made structural by content inspection, and what would
work.** An eaten `\` leaves a *syntactically valid* file: there is no residue in
the bytes, so no grep, escape scan or line-ending rule can catch the class. The
only gate that can is one that **parses the result** —
`make --dry-run --no-print-directory <target>` fails on a broken recipe
continuation in seconds without building anything. **ACTION (unowned):** add that
to `scripts/check-architecture.ps1`, which already sweeps the tracked tree and is
registry-wired, for the two published targets. Not done 2026-08-15 — a new
failure mode in a checker that gates Boundary needs its own cycle.

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

The realtime profile's GDB arm builds an isolated proof target; it does not
refresh the root P1 ROM before the later visual check. For a promotion or
release, run `make p1` explicitly before treating that root visual check as
evidence, then confirm the canonical `nds_build_config.h` contains the promoted
flags. Otherwise a green profile can have inspected an older root ROM.

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
  `sample-tick-hud-buckets.ps1 -SetGlobals name=value[,name=value]` is the
  mechanism (added cycle 79, G1): it pokes the globals once at the first
  frame-complete marker — past bss init, before the sample window — so both
  arms come from one build. Until it existed the rule was not expressible on
  the gate instrument and every "dual-route" A/B quietly degraded into the
  two-build form the rule forbids, paying the ±5,376 cross-build P95 floor for
  nothing.
- **A poke that does not land still prints a full, plausible bucket table.**
  `-SetGlobals` shipped broken for its first two runs: the gdb command lines
  were spliced in as a NESTED array, and a nested array piped into
  `Where-Object` is emitted as one object that stringifies to a single
  space-joined line. gdb rejected that one malformed line, `-batch` printed
  the error and carried on, and the run reached its window and produced a
  complete percentile table with the route never applied — indistinguishable,
  from the output table alone, from a candidate that engaged and saved
  nothing. It was caught only because the arm carried its own engagement
  counters (`-ExtraGlobals`) and they read 0 where the census had already
  proved they must read 10,330. **Every routed arm carries a counter that
  proves the route took, and that counter is checked before its ticks are
  read.** Do not diagnose this class from console output — the console is
  exactly where it hides.
- **THE WHOLE-MATCH NOISE FLOOR, calibrated cycle 100. The ±5,376 quoted
  elsewhere is a 128-frame-era number and is far too tight.**
  - *Same binary, same invocation: **zero*** — the run reproduces
    bit-identically (six runs, three binaries, rows-CSV SHA256 equal across
    every repeat pair). A figure that fails to reproduce exactly means
    something in the invocation or the binary changed.
  - *Cross-build `WORK-H` P95: **≥14,080, sign unreliable*** — one change,
    three A/B pairs, P95 moved −8,832, −2,368 and **+5,248**. This holds even
    when both arms link at the identical `fake_heap_start` with identical
    text/data/bss.
  - *Cross-build `WORK-H` P50: ~5,700*, and P50 kept its sign in all three
    pairs. **Rank an A/B on P50, mean and over-gate count.** P95 is the gate's
    definition; it is not a usable discriminator at these magnitudes.
  - So the protocol is: run each arm twice and require each to reproduce
    itself. Two self-reproducing arms give an exact delta; judging that delta
    still needs the cross-build floors above, because layout-identical is not
    execution-identical.
- **A buffered child's stdout is lost to ANY abrupt parent termination — force-kill,
  tool cap, or timeout alike (2026-08-15, second door in two cycles).** The
  previous cycle lost a 25-minute `probe-battlepack-pacing.ps1` capture by
  force-killing gdb, and fixed that one script. The identical loss then happened
  to a 1,600-sample gate run through a different door: the run outlived a
  10-minute harness/tool timeout, was terminated, and its `Tee-Object` log was
  0 bytes because PowerShell block-buffers into a redirected handle. **Fixing one
  script did not fix the class.** So: anything expected to run past a few minutes
  is launched **detached with an OS-level redirect**
  (`cmd /c "pwsh -NoProfile -File … > log 2>&1"`), and anything that may be
  killed also sets its own incremental logging (`set logging enabled on` for
  gdb). Wait on the **writer's process handle** (`Wait-Process -Id`), never on
  the result file — `Test-Path` on a result JSON has already read one mid-write
  and flipped a KEEP verdict.
- **GATE AN ALLOCATOR ARM ON THE SOAK BEFORE THE GATE RUN (2026-08-15).** A
  2,400 s gate run was spent on an arm a 5-minute `soak-freeze-watch.ps1` would
  have refused: it reported `NEVER-STARTED`, zero presented battle frames, and
  `general heap free bytes 6,076` against the 32,768 floor. Any change that moves
  `NDS_TASKMAN_ARENA_SIZE`, a reservation, or a pool gets the soak first, and the
  checks are `gNdsTaskmanArenaChosenSize == requested`,
  `gNdsTaskmanArenaAllocFailCount == 0`, `…ReserveFailCount == 0`,
  `gNdsR2AnimCacheRejects == 0` and a completed match. **`check-boot-headroom.ps1`
  cannot stand in for this** — it meters the static image against a boot
  threshold, not the heap a runtime `calloc` can be *given*: an arm with 319,840 B
  of "proven headroom" was granted only **188,416** of a 258,048 B arena growth,
  and the reservation *inside* the short arena still succeeded, so every allocator
  guard passed and the battle simply never started.
- **A `-SetGlobals` poke can land and still not be seen (cycle 100).** The stub
  writes main RAM; the ARM9 keeps its own copy. When the target shares its
  32-byte D-cache line with anything the guest writes, the line stays dirty, the
  guest reads its stale value forever, and each writeback stamps that value back
  over the poke — while the readback still reports success. Measured on
  `gNdsFtrPlanRoute`: poked 7, read back 7, **0** hits over 1,216 draws, 0 at end
  of run; a sibling in the previous line survived the same batch and one 12 bytes
  higher in the *same* line died with it. Route a flag at runtime only if it owns
  a clean line — otherwise put it at build time. The harness now records every
  poke's readback in the JSON (`setGlobals`) and throws instead of printing a
  percentile table when a poke did not take, but note it **cannot** catch this
  case: the readback comes from RAM, which is exactly where the write did land.
- **THE SAME BLINDNESS RUNS THE OTHER WAY: A GDB *READ* ALSO MISSES THE D-CACHE
  (cycle 2026-08-15).** `ARMv5::ReadMem` (`melonDS-Accurate/src/ARM.cpp:1545`)
  special-cases ITCM and DTCM and otherwise falls through to `ARM::ReadMem` →
  `BusRead32`; there is no DCache lookup on that path. So a global still dirty in
  the ARM9 data cache reads **stale** over GDB, and a group of globals published
  together can be read **torn** — because ARM946E-S does not write-allocate, a
  store to a non-resident line reaches RAM while the next store to the same line,
  after any load has filled it, only marks it dirty and aborts the bus write.
  That is the whole of the two-year-old R2-04 E2 "rolling FPS counter did not
  sample actual presentation cadence" assert, measured frame by frame in
  `artifacts/verification/2026-08-15_fpshud-publication.txt` and fixed by
  `DC_FlushRange` at the publication seam (`nds_platform.c`,
  `ndsPlatformPublishBattleFpsHudGroup`). **A counter written right up to the
  stop can therefore under-read.** Whole-run totals are usually safe (the line is
  evicted long before the stop); anything sampled per frame, or any group that
  must be self-consistent, needs the publisher to clean its own line.
- **THAT RULE IS NOW STRUCTURAL, NOT ADVISORY (2026-08-15).** After the third
  diagnosis of the same defect, a debugger-read counter group is declared ONCE
  as an X-macro list beside its externs (`NDS_BATTLE_PLAYABLE_PACING_GROUP`,
  `NDS_GCRUNALL_TASKMAN_GROUP`, `NDS_BATTLE_FPS_HUD_GROUP`) and the publish is
  GENERATED from it (`NDS_PUBLISH_DEBUGGER_GROUP`, `nds_platform.h`), so a
  member cannot be added without its flush. `check-gbi-decode-fixtures.ps1`
  requires each list and its marker `printf` to be the same set in both
  directions, and Boundary runs it. **The test for "does this group need the
  seam" is not "is it printed" but "does a harness compare one of its members
  to another live counter at a stop that can land mid-update"** — publishing
  one side of such a comparison and not the other is not a fix, it only moves
  which counter is free to read stale (`…/2026-08-15_pacing-publication/`).
- **The general form: ANY construct between the harness and your eyes hides
  its failures.** This has now cost four cycles as `Select-Object -First`, as
  `Where-Object`, and (cycle 92) as the redirect itself — `2>&1 | Out-File`
  writes a **zero-byte file** when the pipeline throws, so a failing run and a
  silent one are indistinguishable. Let a harness write to the console or to
  its own artifact; do not wrap it. A **default argument** is the same class:
  see the exclusion rule below.
- **Never rank `SRC` with the load-frame exclusion on, and the default is ON.**
  `analyze-tick-hud-excursion.ps1` defaults `-LoadFrameSrcMultiple 2.0`, and
  that rule thresholds on the very bucket being attributed, so it is circular
  for `SRC` (cycle 81). On the banked c86 gate arm it reports `SGCO` **81,595
  instead of 153,291 — understated 1.88x**, and both tables look equally
  plausible. Pass `-LoadFrameSrcMultiple 0`. The script now warns loudly rather
  than leaving this to be remembered.
- **`-AllowRepeatedFrames` relaxes the duplicate-label gate only.** It is
  consulted at `sample-tick-hud-buckets.ps1:857`, long after stitching; the
  ring-wrap proof at `:738-759` (hard-fail on `presentedDelta > 128` and on
  `delta == 0 && presentedDelta > 0`) runs regardless and is **not** weakened.
  So the flag is safe for an excursion *ranking* — each repeat is two genuinely
  distinct iterations with differing payloads, not one frame counted twice —
  and wrong for a per-**presented**-frame P50/P95, where that frame's true cost
  is the sum of its iterations. Do not re-bank a baseline from a run that used
  it.
- **Repeated presented frames are stop-aligned, so a quieter host will not fix
  them.** Cycle 92 gate arm: repeats at frames 534, 822, 1111, 1302, 1591 —
  deltas 288, 289, 191, 289, and **288 = 3 x 96 = exactly three ring stops**.
  The GDB stop stretches the frame until the guest's own pacing drops a
  present, which is a host-induced change to *guest* behaviour, not a host-side
  double-sample. Do not chase host quiet as the remedy; budget ~3-5 repeats per
  1,600-sample run (measured 5, 5, 3 over three runs) and use the flag.
- **A per-unit constant makes a short probe valid for iteration.** Where a cost
  is genuinely constant per unit of work (per list, per instance, per call),
  cost-per-unit read from a few stops is a sound iteration metric even though
  the window is short — the constant, not the window, is what carries it. The
  P95 verdict still needs the whole-match run, and a **short window is never a
  gate reading** (it samples the cheapest ~6% of the match).
  **Label the constant with its arm**: effect-submit cost is ~102,730
  ticks/list on **Boundary** and 80,394–83,632 on the **both-CPU gate arm**, a
  gap wide enough to invert a decision. (The G3 lane this was written for is
  closed — cycle 89–91 proved the packet path and the N64 painter order are
  mutually exclusive — so do not restart it on the strength of this metric.)
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

## Run Economics — the both-CPU soak is final-acceptance only

**The both-CPU soak is the most expensive run in this project. Run it ONCE, when
you believe the goal is complete** (owner, 2026-08-05). It is not an iteration
instrument and it is not a regression check. The cost is the whole chain: it
builds its own `build-r2-bothcpu` stress ROM, then watches a real-time match
long enough for a freeze to have somewhere to happen — and one game minute is
~136 s of wall clock (`soak-freeze-watch.ps1`), which is why `-MinutesToRun`
caps at 7.0 *wall* minutes and defaults to 2.5.

Nothing in ordinary development needs it. Use instead:

- **freeze/stability during iteration** — the Boundary soak, or a short
  `NDS_R2_SOAK_MATCH_MINUTES` run; both catch hangs far cheaper.
- **the performance gate** — the both-CPU *tick* run, which is a 60-second match
  since the 2026-08-05 reseed and is a different and much cheaper thing than the
  soak. Do not conflate the two because they share `NDS_R2_BOTH_CPU`.

Budget a soak deliberately, say why the cheaper form will not do, and never
launch one to "check something quickly".

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

**THE "IT SCALES WITH CAPTURE RESOLUTION" DIAGNOSIS IS RETRACTED, AND THE GATE
IS FIXED (cycle 93).** This section previously said the gate "scales with
capture RESOLUTION" because "a window captured at a lower effective scale
averages neighbouring texels". **That is wrong twice over**: the enforced
profile pins *nearest* filtering, so rescaling replicates blocks and averages
nothing — and the real defect was not in the metric at all.

`Convert-MelonDSWindowTopToNativeBitmap` derived `scale` and `left` from the
window but **hard-coded the content origin at `top = TopY` (56)**, i.e. the top
of the *client area* rather than the top of the *content*. melonDS aspect-fits
the stacked 256x192 pair into the client area and centres it on both axes, so
any window taller than 256:384 letterboxes — and the crop then started inside
the black bar and ran off the bottom of the top screen. Measured content top
edge: **y=76 in an 877x1400 capture (24px letterbox), y=175 in a 620x1212
capture (123px letterbox)**, chrome ending at y=51 in both. The second crop
began ~119 source px (~50 native rows) too high, so `left_bush` read 29.6%
against a 40% floor while the frame was rendered correctly.

`top` is now derived from that layout. **The metric is resolution-independent by
construction, not by normalisation**: the same stored frames now measure
`left_bush` **278/496 = 56.048%** at 877x1400, at 620x1212, *and* at the 600x957
the fixed harness produces — identical to three decimals across a 1.43x scale
range. Normalising the metric or pinning the window would both have papered over
a crop that was reading the wrong pixels.

The varying resolution had its own cause, also fixed:
`verify-battle-playable-realtime-harness.ps1` passes **`-MaximizeVertical`**, so
`Set-MelonDSCaptureWindow` ignores the canonical 416x664 and sizes the window
from `Screen.PrimaryScreen.WorkingArea` — a **host** property, which is why
`check-melonds-policy.ps1` passes (it audits the TOML) while captures arrive at
whatever the desktop allows. It sized off the work area's *height* alone, so a
600x1212 work area asked for 759x1212, the window came back clamped to 620x1212
with the 416:664 aspect destroyed, and ~13 columns of the guest hung off the
screen edge where `CopyFromScreen` photographs desktop black. It now fits both
axes. **A capture whose aspect is not ~0.6265 means the window was clamped**;
the failure message prints the measured aspect beside the canonical one.

Two things this did not change. Every threshold in the region specs was
calibrated against the *misaligned* crop, so the corrected numbers move: the
`-FastIteration` set Boundary runs (caps 32/112/96) passes with margin, but the
non-fast `$textureDetailRegions` **`pond` flat-run cap of 80 now measures 81**.
**Actionable:** re-calibrate that one cap from a fresh non-`-FastIteration`
capture the next time that path is run; it is not on the Boundary path, so it
was not retuned blind here. And **do not delete `emulators/melonds/melonDS.toml`
to "reset" the window** — it carries required paths, the emulator then never
reaches the GDB listener, and the run dies at `gdb-markers.ps1` with a
connection timeout that looks nothing like the problem you were chasing.

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
