# Verifying

Use the least work that can falsify the change. This file owns build, verifier,
measurement and capture procedure. [PROJECT_GOAL.md](../PROJECT_GOAL.md) owns
acceptance; [P2_EXECUTION_BOARD.md](P2_EXECUTION_BOARD.md) owns current packages,
configurations, deferrals and evidence. Historical experiments are not defaults.

## How a P2 row runs

1. **Scope the outcome.** Read the board and [HANDOFF.md](HANDOFF.md). Reuse the
   existing package and source contract; finish the affected feature and reachable
   siblings, not just its first rejected root. Resolve discoverable facts yourself.
   Batch necessary owner decisions before builds; independent ready work continues.
2. **Freeze the inputs.** Main owns shared generators and one build at a time.
   Record the baseline, intended dirty overlay, generator inputs and configuration.
   Preserve unrelated work. Preflight dependencies and actual probe parameters;
   historical commands may no longer match a script's parameter block.
3. **Discriminate cheaply.** Use a focused source/host check or natural trigger.
   Read the complete available failure record before rebuilding. First-cause
   latches do not enumerate every defect. Add bounded diagnostics only when needed;
   do not bypass a failure or continue unsafe work to collect more output.
4. **Verify the integrated batch.** Choose one widest relevant profile. Collect
   compatible state, positive native engagement, pixels/audio, resources and timing
   together. Do not repeat a long focused window for counters the wide run can
   collect. Batching does not waive required per-unit, sibling or lifecycle coverage.
5. **Keep the report bounded.** Save full logs; return exit/verdict, relevant errors,
   warning summary, evidence paths and command wall time when available. Distinguish
   source presence, build health, scoped runtime proof and publication acceptance.
6. **Land reproducibly.** Commit coherent implementation, producers, dependencies,
   tests and required tracked outputs together. Record ignored asset prerequisites.
   Push confirmed progress under the active task's rules; label acceptance still
   owed. Periodically build `smash64ds.nds`, and deliver its verifier-covered
   natural-input configuration after accepted fix batches, not individual edits.

Follow [BUG_FIXING_PROCESS.md](BUG_FIXING_PROCESS.md) for bug closure. Preserve
owner CPU-optimization/raster deferrals and active campaign work. A deferral does
not cancel the final gate; a historical code-first list is not a current build ban.

## Environment and build identity

Run from the repository root in PowerShell 7 (`pwsh`), not Windows PowerShell 5.1.
Invoke Python explicitly rather than relying on Windows `.py` associations.
Use the configured devkitPro/devkitARM installation; typical paths are:

```powershell
$env:DEVKITPRO = 'C:/devkitPro'
$env:DEVKITARM = 'C:/devkitPro/devkitARM'
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

One build at a time, including across different `BUILD` directories: generated
outputs are shared. Never pass `-j`, request a `-Jobs` override, or alter
`MAKEFLAGS`; the Makefile owns parallelism. `make NDS_JOBS=1` is the deliberate
build-order diagnostic. Freeze generated inputs while any consumer builds or runs.

A fresh checkout also needs ignored derived inputs: O2R, extracted relocData,
converted assets and generated includes. The acquisition/extraction entry point is:

```powershell
.\build.ps1 -Rom 'D:\path\to\baserom.us.z64'
```

Inspect its prerequisites, pin checks and logs; this command is not a promise that
an unprepared host builds. On a prepared tree, `make TARGET=smash64ds` builds the
P2 target. A publish needs clean-source reproducibility with declared derived
inputs, not merely a successful dirty incremental build. Shared input junctions
are acceptable only for identified generated subtrees kept immutable during use.
`decomp/` and `artifacts/` contain tracked files: inspect `git ls-files -- <path>`
before cleanup; never replace an entire reference tree to supply missing assets.

| Output | Role |
|---|---|
| Root `smash64ds.nds` | P2 human-input ROM; no scripted walk or fast logic. |
| Root `smash64ds-battle-playable-hwtri.nds` | Frozen P1 artifact; no routine rebuild. |
| Other targets under `builds/` | Lab instruments, not published ROMs. |

A published target name still writes the root output with a custom `BUILD`.
Use lab targets for experiments and resolve matching per-build ROM/ELF paths.
`DECOMP_PIN.txt` still pins the P1 output at this revision; do not use that hash
as P2 acceptance. Record the identity actually built and covered by verification.

Evidence identity includes commit/dirty overlay, ROM and ELF hashes, build path,
`nds_build_config.h`, source/generated asset versions, emulator/verifier versions,
input/seed, save/DLDI state, instrumentation and guest window. Equal config headers
alone cannot expose a diagnostic branch compiled into source. Reuse prior proof
only while its inputs and expected contract remain valid, not just its ROM hash.

## Checkpoint choice and coverage

[scripts/lib/harness-registry.ps1](../scripts/lib/harness-registry.ps1) is the
membership authority. `verify-all.ps1 -Profile Boundary -List` and
`verify-all.ps1 -Profile Latest -List` show the selected runtime entries.
[HARNESSES.md](HARNESSES.md) owns naming. The driver's static preflights are separate.

Use a focused checker while editing. For a kept coherent batch, choose Boundary
for battle-only work or Latest for normal/shared startup; do not stack DevFast,
Boundary and Latest. Example, with full output capture as described below:

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -RunnerSlot 2
```

Use `-Profile Latest -Build` instead when the normal target needs rebuilding.
`-Build` rebuilds it only when the selected plan includes that target. Boundary's
children build lab ROMs; a green Boundary does not refresh the root P2 ROM.
`-NoBuild` requires matching existing ROM/ELF pairs and is not a freshness check.
`-Only` selects named registry entries; `-From` selects the profile suffix. Neither
proves the full profile by itself: report actual coverage despite the wrapper's
pass-message label. Never pass `-Build` and `-NoBuild` together.

| Boundary entry | Configuration and coverage |
|---|---|
| `p2_shell_loop` | `smash64ds-p2-shell-loop-hwtri` / `build-p2-shell-loop`: shell transitions, scene/input trace and arena checks. Defaults to one lap; fast logic is not performance evidence. |
| `p2_battle_realtime` | `smash64ds-p2-shell-hwtri` / `build-p2-shell`: mode 163 via the shell, Mario human vs level-3 Fox, Dream Land, items off, one-minute Time. |
| `p2_fourcpu_stress` | `smash64ds-p2-fourcpu-tickhud-hwtri` / `build-p2-fourcpu-tickhud`: direct battle, observed four-CPU roster, native-output and resource checks. |

Latest adds `runtime` to these three entries. The realtime arm's counters and
screenshots must describe the same candidate. A shell timeout before battle is
not battle coverage; inspect the last reached scene rather than assuming an abort.

For four-CPU runs, derive the expected roster from the built configuration and
compare it with the observed slots. Preserve the source item spawn law and zero
`gNdsItemRateOverride` / `gNdsItemTogglesOverride` at the checked runtime stops.
A forced-item diagnostic changes the workload and RNG history; label it diagnostic.
The default timing window is 1,972 samples at frames 2–1,973, with identity at
frame 1 and clock coverage 60→1. It does not cover Time Up, Results or rematch.

The four-CPU script asserts native/resource/correctness conditions, **not the
product tick/cadence target**. Animation cache misses/rejects are reported data,
not a guarantee of zero streaming. Capacity proof may survive an independent
native rejection, but the complete verifier and publication remain unaccepted.
For residency acceptance, prove the resource-class deltas, admitted sets and epoch
rules in [P2-texture-residency.md](p2/P2-texture-residency.md), including zero mandatory
post-GO demand reads. Report declared BGM service separately; no blanket I/O claim.

## Focused checks

Select only the affected surface; these examples are not an obligatory suite:

| Surface | Existing checks |
|---|---|
| Docs/imports/ABI | `check-docs.ps1`, `check-architecture.ps1`, `check-decomp-header-mirror.py` |
| Rendering | `check-gbi-decode-fixtures.ps1`, `check-battle-playable-static-textures.ps1`, `check-native-owner-wiring.py` |
| Fighter production | `check-fighter-production-manifest.ps1`, `fighters/check_native_owner_geometry_closure.py` |
| Collision | `check-mp-floor-crossing-fixtures.ps1`, `check-mp-topology-fixtures.ps1`, `check-ft-hitstatus-fixtures.ps1` |
| Audio/menu assets | `check-audio-fgm-phase-pack.ps1`, `check-audio-bgm-derived-assets.ps1`, `check-mn-screen-coverage.ps1` |
| Build inputs | `check-untracked-dependencies.py`, `check-generator-staleness.ps1` |

Paths above are relative to `scripts/`; run `.ps1` directly in PowerShell and
`.py` with `python`. `check-generator-staleness.ps1 -IncludeSlow` is opt-in
exhaustive regeneration, not an every-edit requirement. Actual preflight wiring
is in `verify-all.ps1`; do not assume every check in this table runs there.

Run the architecture check when adding an import wrapper; it checks the literal
BattleShip/overlay provenance. Fix stale generated outputs at their producer and
build dependency. Menu coverage checks source/kit inventory, not every visible
state; an open allowlist entry is not visual acceptance. Parse structured-file
edits with the relevant parser/build check rather than trusting text inspection.

## Performance evidence

Use one synchronized eight-frame A/B for initial iteration, with matching content,
input, cadence, instrumentation and guest window. Require positive route engagement,
a screenshot with automated analysis and relevant state/geometry guards. Presented-
work counters credit equivalent native work; CPU-work counters do not credit work
that was bypassed. Stop on a decisive KEEP/REVERT; add A2 or more samples only for
noise, near-gate results or conflicting statistics/state/pixels. No routine A/B/A.

Short probes are not release P95 readings. Bank representative whole-match evidence
for the standing stress configuration, re-derived over landed content under P2 law.
The retained 1,600-frame rank-80/WORK-H instrument sizes candidates; it does not
replace four-CPU coverage or all-presented-frame cadence. Memory and CPU worst
cases may be different rosters. Respect the standing ≥14,080 cross-build P95
floor; small deltas need supporting evidence, not a forced verdict. Historical
exact repeatability and old frame windows are not new mandatory run schedules.

Report P50/P95 with bucket definitions, population and coverage; distinguish WORK-H
work cost from ALL/pacing. Include FPS, the 2/3/4/5+ VBlank histogram and maximum
interval. Product acceptance remains P95 approximately ≤1.12M ticks and ≥95%
two-VBlank cadence under the standing contract. Menus also target 30 Hz.
`gNdsBattlePlayablePacingCadenceViolationCount` detects early, not late, presents:
zero `TICKSLIP` or `cadenceViolations` does not establish 30 FPS.

Instrument safeguards:

- Debugger RAM reads/writes can disagree with dirty ARM9 cache lines. Use the
  existing coherent publication/flush seam and guest route-hit witnesses; a
  successful poke/readback or `volatile` declaration is not sufficient.
- Prefer the ring collector over per-frame stops, which can disturb pacing and
  timer-driven work. `-AllowRepeatedFrames` relaxes duplicate labels, not ring-wrap
  safety or valid per-presented-frame percentiles; resolve the population first.
- Price intermittent work from window totals and event counts. Medians or trimming
  can discard the event itself. For SRC attribution in
  `analyze-tick-hud-excursion.ps1`, use `-LoadFrameSrcMultiple 0`.

After capacity changes, prove actual startup/admission and the affected lifetime
before a long measurement. Static headroom cannot prove a runtime allocation.
Long both-CPU freeze soaks are deliberate final stability qualification, not
routine iteration. Save measurements in [PERF_LEDGER.md](PERF_LEDGER.md).

## Logs, runners and captures

`verify-all.ps1` writes child output directly to the console handle. Capture with
OS-level redirection, not `Tee-Object` or a PowerShell redirect around the driver:

```powershell
New-Item -ItemType Directory -Force builds | Out-Null
cmd /c "pwsh -NoProfile -ExecutionPolicy Bypass -File scripts\verify-all.ps1 -Profile Boundary -RunnerSlot 2 > builds\verify-boundary.log 2>&1"
$LASTEXITCODE
```

Require successful exit, expected completed checks and the final pass message.
Accounting/infrastructure errors are not ROM verdicts and must not be bypassed.
Keep the full failure context; do not rerun reproducible failures until one passes.
Use unique log/result names per run. Wait for the writer process to exit: file
existence is not completion, and buffered `.gdb.out` may be from a previous run.
Use supported incremental logging for long probes; interrupted output is partial.

Pass array parameters directly inside PowerShell or through `pwsh -Command`, not
as a numeric list through native `pwsh -File`. The redirect example uses scalar
arguments; respect scripts that explicitly accept comma-separated strings.

Use only repo-local accuracy-focused melonDS: `emulators/melonds/melonDS.exe`
for manual use and numbered runner copies for automation. Keep interpreter/JIT-
disabled profiling policy from boot; never alter the owner's manual instance.
Provision idle slots with `New-MelonDSRunnerSlots.ps1`; refresh idle copies with
`-Force` after changing the source executable. Use `check-melonds-policy.ps1`;
`-AuditLocalConfigs` is for deliberate local audit/repair, not every run.

Slotted launches default to private `slotN/storage` FAT/save/state paths; explicit
storage overrides win and non-slotted runs retain canonical storage. Check the
resolved paths, not merely the slot number. Guest read-only DLDI does not prevent
host staging from changing a shared image. Keep folder sync off for frozen-input
runs. Persistence tests require disposable writable storage, never the user's save.

Correctness diagnostics may use up to 12 supported isolated slots: freeze the
ROM/ELF/inputs and give every case unique ports, logs, captures and storage. The
batch runner additionally leases slots and checks hashes. This permits neither
parallel builds nor concurrent tick/FPS/VBlank or exact visual acceptance. A
capture mutex serializes windows, not guest execution. Recheck suspected
contention-induced stalls in isolation. Host muting must leave guest audio active.

Capture at a guest scene/event anchor. Menu probes/captures live under
`scripts/menus/`; inspect their parameters before targeting a state. Match pixels
to the ROM and sampled state; validate the native crop and unobscured capture.
Healthy guest progress with a frozen image calls for capture diagnosis, not an
assumed hang. Do not delete runner TOMLs or weaken image thresholds to get green.

Every built ROM, including diagnostics, must exclude forbidden graphics paths from
actual build inputs and linked binaries before packaging. A native-only flag or
hardware triangles alone is not enforcement; reference rendering stays host-side.
Native acceptance also needs positive engagement, required geometry/material/state
coverage and source-comparable visible output—not only zero failure counters.
Keep source-mandated hidden/transparent states distinct from missing output.
A pose/material fix invalidates evidence derived from its old state. Save accepted
screens and measurements under `artifacts/visibility` and `artifacts/performance`;
raw shard logs, configs, images of storage and binaries are not committed.

## Publish and checkpoint

Qualify the delivered human-input ROM, not a relabeled lab artifact. Recheck its
config and SHA-256 after the last build, update the existing board/evidence and
state unrun or failed gates. Subjective owner testing supplements measurable proof.
Retain coverage owed by old code-first commits in their existing phase/unit owners;
removing their historical command list does not complete those requirements.
After documentation, the chosen verifier and static checks, inspect `git status`,
commit reproducible changes and push under the active task's rules.

Snapshots are obsolete (owner, 2026-09-12). Do not create one or restore the deleted
snapshot script/final-command requirement. A successful build or commit alone is
not publication acceptance.
