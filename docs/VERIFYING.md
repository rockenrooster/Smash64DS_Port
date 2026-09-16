# Verifying

Use the least work that can falsify the change. This file owns build, verifier,
measurement, capture and publication procedure. `PROJECT_GOAL.md` owns acceptance;
`P2_EXECUTION_BOARD.md` owns current work, decisions and artifact identity.
`p2/native-optimization/13_AGENT_EXECUTION.md` owns the P2-2p8 batch assignment.
The September 15 workflow revision changes test scheduling, not game requirements.

## One coherent batch, not one full verifier per task card

A batch removes one substantial repeated operation or converts a bounded
producer-to-consumer chain. It may cover several task IDs and several commits.
List its actual edit boundaries, inputs, first falsifier and acceptance coverage
in its existing board/evidence record. Do not create a second queue or a mandatory
new report per small edit. Source/behavior/asset contracts still apply throughout.

| Boundary | Required work | Do not do |
|---|---|---|
| Continue/recover | Intact context: execute the cursor. Lost context: reconcile its scoped inputs/job once | Repeat all startup or completed checks for a new turn, or treat an uncommitted result as unperformed |
| Slice implementation | Relevant source/host/negative fixtures and a discriminating target check where needed | Full Boundary/Latest and an entire new profile for each small commit |
| Frozen integrated batch | One widest relevant profile; compatible whole-match work, cadence, native, memory, pixels/audio together; remaining required scenario proofs | Stack overlapping profiles or duplicate separate runs for already-collected compatible evidence |
| Release | Qualified final hard-on natural-input ROM, no-float/retirement and full required content/lifecycle/roster-stage coverage | Promote a short probe, correctness GREEN, incomplete matrix or unqualified candidate |

A full run is repeated after an invalidating change, incomplete required coverage,
or a failure/conflict that requires it; batching never excuses missing proof.
Stop unsafe execution on corruption. Fix the cause rather than collecting more
invalid output. Independent work can continue without changing frozen inputs.

## Status and checkpoints

`IMPLEMENTED_NOT_ACCEPTED` preserves coherent candidate code, tests and producers.
It may be committed with explicit paths, checks completed/owed, known defects,
temporary routes and next command. It is not accepted, not a closed task and not
permission to publish. Preserve an incoherent edit as a recoverable diff rather
than falsely claiming a verified boundary. Never sweep unrelated owner changes.

A local KEEP says the scoped mechanism is retained; it is not universal acceptance.
Record whether a change is a measured win, a neutral enabling dependency or merely
implemented. A batch's measured gain must not be assigned to every constituent.
Full bug FIXED conditions remain in `BUG_FIXING_PROCESS.md`.

## Reuse evidence by the property it proves

Before launching a command, consult the cursor's pending/completed action and
linked receipt. Same relevant inputs and operation with valid completed evidence
means advance to the next unfinished action, not run again. Record the specific
invalidator, missing coverage or conflicting result when repeating is necessary.
After a result, persist its verdict and next action before a context handoff;
commit failure does not erase the observation. Timing on a changed executable
still needs new qualification. No receipt means unknown, not an assumed pass.

Before repeating completed work, state the actual invalidator or missing question.
Use existing manifests/receipts; do not retype every identity into every document.

| Evidence | Reuse rule |
|---|---|
| Source/host semantic fixture | Same relevant source, generator, constants, oracle and contract; changed outputs invalidate affected proof |
| Baseline ROM/ELF/config/rows | Exact matched retained inputs and valid population; historical rows remain a baseline, never become a new candidate result |
| Timing/cadence | New linked code/layout, runtime inputs, assets, SDK/emulator, instrumentation or relevant service behavior requires new qualification; a source-only proof cannot exempt timing |
| Lifetime/resource proof | Same admitted identities, capacities, transition paths and generations; changes invalidate affected scenarios |
| Pixels/audio | Same candidate state/output producer and qualified capture path; pose/material/cue changes invalidate affected evidence |
| Completed job | Writer exited, exit status and full required output valid, identity matches; file existence alone is insufficient |

An unchanged documentation edit does not by itself require a ROM rebuild.
If a harness rebuilds a different binary, record that identity and do not attach
old runtime receipts to it. Same-ROM experiment results establish a mechanism;
removing the experiment route still needs final hard-on qualification.

## Environment and builds

Use PowerShell 7 (`pwsh`), never Windows PowerShell 5.1. Invoke Python explicitly.
Use the configured devkitPro/devkitARM installation. Typical prepared-host setup:

```powershell
$env:DEVKITPRO = 'C:/devkitPro'
$env:DEVKITARM = 'C:/devkitPro/devkitARM'
git status --short
git diff --stat
```

Use `verify-all.ps1 -Profile Boundary -List` or `-Profile Latest -List` when
selecting coverage or when the registry changed. Do not repeat it during polling.
The acquisition/extraction entry for an unprepared tree is:

```powershell
.\build.ps1 -Rom 'D:\path\to\baserom.us.z64'
```

Inspect prerequisites, pin checks and ignored O2R/relocData/converted inputs;
this is not a promise that an unprepared host builds. On a prepared tree,
`make TARGET=smash64ds` builds P2. Never replace `decomp/` to supply missing assets.
Inspect `git ls-files -- <path>` before cleanup: references and artifacts are tracked.

**One build/producer writer at a time, across all BUILD directories/worktrees.**
Generators share paths. Freeze source/generated inputs consumed by a build or run.
The Makefile owns internal parallelism: never pass `-j`, override `-Jobs`, or
alter `MAKEFLAGS`. `make NDS_JOBS=1` is the deliberate build-order diagnostic.

Reuse warm configuration-qualified build directories for iteration. Before reuse,
preserve the matching baseline ROM/ELF/map/config and hashes in existing lab
storage. A new name is not evidence of a clean or correctly configured build.
Fresh builds remain required for clean-source reproducibility, invalid dependency
state or a deliberate diagnostic—not for every candidate by default.

### Shared generated configuration

Resolve actual expanded flags, producer arguments and generated-input versions,
not an inferred configuration from a target name. Before a target switch, prepare
the bounded producer output AND its stamp for the incoming configuration. Keep
these stable until consumers finish. If a gate expects canonical shipping outputs,
prepare that state through the producer before running the gate.

The September 15 rollouts alternated Yoster-off lab data with Yoster-on shipping
expectations. Treat a recurrence as a concrete dependency/preflight-order defect:
inspect the exact invocation, correct its owner once, test the mismatch/repair,
then return to runtime work. Do not hand-edit outputs/stamps, run a whole fresh
shell build merely to change a stamp when a supported producer target suffices,
or remove Yoster/required content. Distinct BUILD directories alone are not isolation.

| Output | Role |
|---|---|
| Root `smash64ds.nds` | Verifier-covered natural-input P2, no scripted walk/fast logic |
| Root `smash64ds-battle-playable-hwtri.nds` | Frozen P1; no routine rebuild |
| Lab targets under `builds/` | Experiments with matching per-build ROM/ELF/config |

A publish target can overwrite the root output even with a custom BUILD. Use lab
targets for experiments. `DECOMP_PIN.txt` still contains a P1 hash at the reviewed
baseline; identify the actual P2 artifact instead. Preserve accepted artifacts.

## Identity and coverage

Evidence includes commit/intended dirty overlay, ROM/ELF hashes, build path,
`nds_build_config.h`, source/generated assets and producer versions, SDK/linker,
emulator/verifier, seed/input, save/DLDI state, instrumentation and guest window.
Equal config headers do not reveal a diagnostic branch compiled into source.

`scripts/lib/harness-registry.ps1` owns membership; `HARNESSES.md` owns naming.
Use Boundary for a battle-only integrated batch, Latest for normal/shared startup.
Do not stack DevFast, Boundary and Latest or restore retired diagnostic fleets.

### Four-CPU optimization runs the match and nothing else (owner, 2026-09-16)

**Four-CPU performance work runs `p2_fourcpu_stress` alone.** No shell loop, no
realtime arm, no menu walking, no extra lab builds. Invoke
`verify-p2-four-fighter-stress.ps1` directly against the candidate; do not spend
a Boundary profile on a tick measurement. The target already boots straight into
the source VSBattle path — `Makefile` overrides `NDS_P2_FOUR_CPU_STRESS` for
exactly this reason — so all 1,972 samples are gameplay frames with no CSS walk
and no load frames in the percentiles.

Two conditions come with it, because the cheaper loop removes the cross-checks
that used to catch these:

1. **Build into `build-p2-fourcpu-tickhud`, the baseline's own directory, and
   check the NitroFS payload matches.** `NITRO_FILES := $(NITROFS_DIR)` hands the
   whole build directory to the packer, so **a build ships every file present in
   its `nitrofs/`, not the set its own flags produce** — the Makefile says so at
   the `NDS_AUDIO_OBSOLETE_DERIVED_FILES` comment, and `prune-obsolete-audio` and
   `prune-streamed-ftanim` are the only prunes that exist. A long-lived build
   directory therefore accumulates payload from every earlier flag set, and a
   fresh one carries only what the current flags produce.

   On 2026-09-16 the identical N04.08 source measured WORK-H P50 **1,587,328**
   from a fresh directory against **1,639,808** from `build-p2-fourcpu-tickhud`,
   whose `nitrofs/` held 710 files to the fresh one's 365 — 1,005,221 B of Kirby
   reloc animations, CSS preview FPCs, Pikachu/Yoshi images and a shield pose,
   dated 2026-09-11 and 2026-09-13, with only 3 files written that day. Nothing
   failed: native failures stayed 0/0 and the engagement counter was correct.
   The whole difference sat in **STG** (~54,500 ticks/frame) while GCRA and FTR
   matched to within 1,500. **Whether the stale payload causes that STG
   difference is not established** — an unopened file costs ROM size and FAT
   chain length, not stage time — so treat this as "these two ROMs are not
   comparable", not as a measured cost of staleness.

   **Before quoting any cross-build delta, check the candidate's NitroFS file
   count and total size against the baseline's.** See
   `artifacts/performance/2026-09-16_p2-2p8-mobj-stable-skip/`.
2. **Carry the candidate's own engagement counter in the run.** Without the
   realtime and shell arms, this measurement is the only thing standing between
   a lever that fires and a lever that silently stopped firing.

Correctness, presentation and publication gates are unchanged and still owed
before acceptance — this rules the measurement loop, not the acceptance bar. Run
Boundary when a batch is being integrated or published, not per tick experiment.

| Boundary entry | Actual coverage |
|---|---|
| `p2_shell_loop` | `smash64ds-p2-shell-loop-hwtri`, `build-p2-shell-loop`; scene/input/arena transitions; fast logic is not performance evidence |
| `p2_battle_realtime` | `smash64ds-p2-shell-hwtri`, `build-p2-shell`; mode 163 via shell, Mario human/level-3 Fox, Dream Land, items off, one-minute Time |
| `p2_fourcpu_stress` | `smash64ds-p2-fourcpu-tickhud-hwtri`, `build-p2-fourcpu-tickhud`; observed four-CPU slots, native/resources/correctness and timing report |

Latest adds `runtime`. The four-CPU script's correctness/resource pass does NOT
assert the product tick/cadence target. Evaluate those independently. A full profile
also does not cover every move, lifecycle state or legal roster-stage combination.
`p2/native-optimization/16_ALL_ROSTERS_ALL_STAGES.md` retains the full release set.
Development screening is not release coverage; do not average away a failing case.

`-Build` rebuilds the normal target only when the selected plan includes it.
Boundary's child lab builds do not refresh the root P2 ROM. `-NoBuild` requires
verified matching existing artifacts; never combine it with `-Build`.
`-Only`/`-From` select actual entries, not the full profile. A resumed subset may
complete missing coverage with compatible prior receipts, but report that scope;
do not claim the failed/incomplete umbrella invocation exited GREEN.

Preserve actual source item spawn law and zero runtime diagnostic item overrides.
Forced-item runs change workload/RNG and are diagnostic. Observe slot identities,
CPU activity and native output, not flags alone. The default stress has frame-1
identity and 1,972 timing samples at frames 2–1,973, clock 60→1 at the reviewed
baseline. It excludes Time Up/Results/rematch. As cadence changes, prove actual
source-clock/population coverage rather than assuming historical frame counts
still describe a whole match. Required lifecycle tests remain separate.

Residency acceptance follows `p2/P2-texture-residency.md`: exact required/admitted
sets and zero mandatory post-GO demand reads. BGM is a separately declared service;
reported cache misses or a capacity pass do not prove zero streaming or full content.

## Focused checks and measurements

Choose tests for the changed owner and its consumers. Existing anchors include:
`check-docs.ps1`, `check-architecture.ps1`, `check-decomp-header-mirror.py`,
`check-gbi-decode-fixtures.ps1`, `check-native-owner-wiring.py`,
`check-fighter-production-manifest.ps1`,
`fighters/check_native_owner_geometry_closure.py`,
`check-mp-floor-crossing-fixtures.ps1`, `check-mp-topology-fixtures.ps1`,
`check-ft-hitstatus-fixtures.ps1`, `check-audio-fgm-phase-pack.ps1`,
`check-audio-bgm-derived-assets.ps1`, `check-mn-screen-coverage.ps1`,
`check-battle-playable-static-textures.ps1`, `check-untracked-dependencies.py`
and `check-generator-staleness.ps1`, all relative to `scripts/`.

Use current parameter blocks. `-IncludeSlow` generation checks are deliberate
exhaustive regeneration, not every edit. Fix staleness at its producer/dependency.
Run architecture checks after import changes. Inventory checkers do not establish
visible output or complete allowlisted content. Parse structured-file edits.

For an unresolved timing hypothesis, use one discriminating short synchronized
A/B (the existing eight-frame probe is an option), with matching inputs/window
and route engagement. A ready valid comparison need not be repeated. Extend for
noise, an unobserved intermittent workload, new semantic effects or conflict—not
because every candidate must pass an eight/128/full-match ladder. No routine A/B/A.

Bank retained integrated gains with representative whole-match evidence from the
frozen final batch, collected with compatible native/state/resource/output checks.
Do not repeat a separate whole match for fields the integrated run can collect.
Additional scenario/rare-state proofs remain required. Native pixels and audio
must describe the same candidate. Stop when a decisive rejection is already known.

Keep these reporting distinctions explicit:

- Label **WORK-H P50 and WORK-H P95 separately**, with units/population/rank;
  label ALL/pacing separately. Never substitute the median into the gate gap.
- Reconcile per-row WORK−HUD/accounting; parent SRC/GCRA/SINT/SCPU buckets are
  nested. Independent percentiles or package savings do not add.
- Include FPS, 2/3/4/5+ VBlank histogram, maximum interval and requested tail data.
  Product gate remains approximately P95 ≤1.12M and ≥95% two-VBlank; menus 30 Hz.
  Zero `TICKSLIP`/cadenceViolation counters alone do not prove timely presents.
- The documented cross-build floor is ≥14,080 ticks; smaller deltas need supporting
  paired/same-ROM/owner-local evidence, not a forced win. A combined batch gain
  does not establish a separate causal gain for every patch.

Reuse current exclusive attribution for selection. Re-profile the affected owner
only when missing/stale data changes the decision. A profile is a means to select
runtime work, not an ongoing deliverable loop. Rank-80 over 1,600 gameplay frames
is candidate sizing, not a replacement for current four-CPU/cadence coverage.

Use coherent guest publication/flush for debugger access; `volatile`/poke readback
is not cache coherence. Prefer ring collection to per-frame stops. Repeated-label
allowances do not waive ring-wrap/population safety. Account for intermittent work
from totals and event counts; do not trim away the event. For SRC attribution in
`analyze-tick-hud-excursion.ps1`, use `-LoadFrameSrcMultiple 0`.
After capacity changes prove actual startup/admission/lifetime before a long run.
Long both-CPU freeze soaks are final stability work, not routine iteration.

## Command lifecycle, logs and runner isolation

Capture stdout/stderr from launch. `verify-all.ps1` writes via the console handle;
use OS redirection, not `Tee-Object` or a PowerShell redirect around that driver:

```powershell
New-Item -ItemType Directory -Force builds | Out-Null
cmd /c "pwsh -NoProfile -ExecutionPolicy Bypass -File scripts\verify-all.ps1 -Profile Boundary -RunnerSlot 2 > builds\verify-boundary.log 2>&1"
$LASTEXITCODE
```

Use a unique log name per actual run. Keep the original launch, handle type/ID,
log path and expected completion marker in the existing batch record. An outer
execution-cell ID, shell `session_id` and OS PID are different. Continue through
the originating API; on a missing-cell error inspect the original response and
actual process/log once. Handles are scoped to the originating session/API and
are not assumed usable in a different session. Resume only when supported; otherwise
observe the owned process/output safely and mark uncertain completion as unknown.
Do not repeatedly wait on the wrong ID or duplicate a job.
Use supported bounded waits; poll for progress/stall/cancel decisions rather than
repeated tiny Get-Process/Get-Content calls. Do not invent tool methods.

Require writer exit, exit status, expected completed checks and complete output.
File existence, stale `.gdb.out` or a filtered success line is not completion.
Read full failure context. Infrastructure/accounting failures are not ROM verdicts;
repair the demonstrated cause before rerunning. Interrupted results remain partial.
A stalled command is investigated, not automatically killed and restarted.
Separate underlying command exit/errors from wrapper success; a successful tool
call or a later shell command cannot prove an earlier `git add`/build succeeded.
Retain stdout/stderr and relevant exit statuses. Do not infer that a commit exists
from intent, a wrapper exit zero, or a dirty file's presence.

A denied Git write, unavailable approval service or helper initialization failure
is a capability result. Record its evidence and legitimate retry condition in the
cursor/receipt. Do not retry unchanged on each goal continuation, bypass denial,
remove a lock without verified ownership, or invent trusted runtime metadata.
Continue permitted independent implementation; report commit/push/verification
still owed. Retry only after a relevant observed environment/authorization change;
repair an optional helper transport outside the game's main implementation lane.
An exhausted/lost handle does not by itself prove the child command stopped.

Pass PowerShell array parameters directly or via `pwsh -Command`, not a numeric
list through native `pwsh -File`. Avoid fragile nested quoting and Bash heredocs
in PowerShell; reuse the known-working invocation. Do not delete the user's inputs.

Use only repo-local accuracy-focused melonDS, interpreter/JIT disabled from boot.
Do not alter the owner's manual instance. Use numbered runner copies and private
ports/logs/storage. `New-MelonDSRunnerSlots.ps1` provisions idle slots; `-Force`
refreshes idle copies after emulator changes. Run `check-melonds-policy.ps1`;
`-AuditLocalConfigs` is deliberate repair, not every run. No retail-test prerequisite.

Check resolved storage paths: overrides can defeat slot isolation. Keep folder
sync off on frozen runs; guest read-only DLDI does not protect a shared host image.
Persistence uses disposable writable saves. Up to 12 supported isolated correctness
slots may run when inputs/leases/hashes permit, but no concurrent authoritative
tick/FPS/VBlank or exact visual acceptance. A capture mutex does not serialize
emulators. Keep CPU-heavy host tests/other emulators off the measurement interval;
read-only or isolated edit work must not mutate its inputs or create contention.
Host muting must leave guest audio running.

Capture at a guest scene/event anchor; menu tools live under `scripts/menus/`.
Check the native crop, unobscured capture and state. Healthy guest progress with a
frozen image needs capture diagnosis, not an assumed game hang. Do not weaken
image thresholds or delete runner TOMLs to get green.

## Acceptance and publication

Every built ROM, including lab diagnostics, must exclude N64 graphics interpreters,
generic compatibility renderers and software scene compositors from actual inputs
and linked code before packaging. A native flag alone does not prove enforcement.
Positive native engagement, source-comparable output, required geometry/material
states and intended hidden/transparent behavior are required—not zero counters alone.

Save permanent proof under `artifacts/performance` and `artifacts/visibility`;
raw shard logs, runner configs, storage images and binaries are not committed.
Update `PERF_LEDGER.md` for retained/rejected measurements and the existing board
for status; the handoff is only a restart pointer. Do not rewrite history.

Qualify the final hard-on natural-input `smash64ds.nds`, not a renamed lab ROM.
Recheck configuration and SHA-256 after the last build. Preserve required owner
acceptance. Report failed, blocked, stale and unrun gates. All legal four-fighter
lineups/stages, native coverage, fixed-runtime and final release obligations stay
open until actually proved; no pooled result or safe refusal closes a legal case.

Commit reproducible qualified batches under the active task's rules. Candidate
checkpoints are explicitly distinguished above. Snapshots are obsolete; do not
restore a snapshot script or final-command requirement. A commit/build is not
publication acceptance, and an owner's delivery target is not proof of completion.
