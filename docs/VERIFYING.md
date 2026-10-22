# Verifying

Use the least work that can falsify the change. This file owns build,
measurement, capture and publication procedure; the active plan's status file
(`docs/P8/P8_STATUS.md`) owns current work, its targets and artifact identity.
The tools are in `scripts/tools/` (its README lists them). The P1/P2 verifier
framework (`verify-all.ps1`, the harness registry, the `verify-battle-playable-*`
verifiers) was deleted on 2026-10-22; `docs/OLD_MACHINERY_DELETED.md` records it.

## One rule a batch

A batch is one rule, proved: the proof that fits the change, not a fixed ladder.

| change | proof |
|---|---|
| Text only (splits, comments, renames inside one unit) | the preprocessed translation units are identical (`scripts/tools/pp_run.py`, `pp_cmp.py`) in the five live configs |
| Code that must not change behaviour | the objects or function sizes, then the gate ROM's replay digests (`worst_case_sweep.ps1` CSVs, `scripts/compare-replay-digest.py`) |
| A performance lever | the gate ROM's worst-case search against the last gate, same layout, digests identical |
| Anything visible or audible | the owner's OK first, then captures |

A full run is repeated after an invalidating change, incomplete coverage, or a
failure that requires it. Stop on corruption; fix the cause rather than collecting
more invalid output.

## Reuse evidence by the property it proves

Same relevant inputs and operation with valid completed evidence means advance,
not run again; state the invalidator when repeating. No receipt means unknown,
not an assumed pass.

| Evidence | Reuse rule |
|---|---|
| Source/host fixture | Same relevant source, generator, constants and oracle |
| Baseline ROM/ELF/config | Exact matched retained inputs; historical rows stay a baseline |
| Timing | New linked code or layout, runtime inputs, assets, SDK or emulator requires a new run; a source-only proof cannot exempt timing |
| Pixels/audio | Same candidate state and capture path |
| Completed job | Writer exited, exit status and full output valid, identity matches; file existence alone is not completion |

## Environment and builds

Use PowerShell 7 (`pwsh`), never Windows PowerShell 5.1, and launch builds from
PowerShell, not Git Bash (the msys2 `make` path differs). Invoke Python
explicitly. Use the configured devkitPro/devkitARM installation:

```powershell
$env:DEVKITPRO = 'C:/devkitPro'
$env:DEVKITARM = 'C:/devkitPro/devkitARM'
```

An unprepared tree starts with `.\build.ps1 -Rom 'D:\path\to\baserom.us.z64'`;
never replace `decomp/` to supply missing assets. Inspect `git ls-files -- <path>`
before cleanup: references and artifacts are tracked.

**One build writer at a time, across all build directories.** Generators share
paths; the `scripts/tools` builds hold one named mutex. Never edit sources while
a build runs. The Makefile owns internal parallelism: never pass `-j` or alter
`MAKEFLAGS`.

Reuse warm build directories for iteration (`-Seed` copies one with its mtimes
kept). A build ships every file present in its `nitrofs/`, not the set its own
flags produce, so a long-lived directory accumulates payload from earlier flag
sets: before quoting a cross-build delta, compare the two ROMs' NitroFS file
count and size.

Before a target switch, prepare the bounded producer output and its stamp for
the incoming configuration (`make TARGET=<target> BUILD=<build>
prepare-particle-banks` for the particle banks), and keep them stable until the
consumers finish. A generated-file goal passed directly to the root Makefile can
report success without running its inner recipe; verify the stamp.

Measure the config that ships: the gate ROM (`build_gate_rom.ps1`) is the ship
layout with the lab's sweep driver; the play ROM is what the owner plays.
Instruments and fault injection stay out of both.

## Identity

Evidence names the commit and any intended dirty overlay, the ROM/ELF hashes and
build path, `nds_build_config.h`, the generated inputs, the emulator, the seed
and input, the save/DLDI state and the instrumentation. Equal config headers do
not reveal a diagnostic branch compiled into source.

## Focused checks and measurements

Choose tests for the changed owner and its consumers: the host tests next to it
(`python -m pytest scripts/<area>`), and the host checks under `scripts/`, for
example `check-decomp-header-mirror.py`, `check-audio-fgm-phase-pack.ps1`,
`check-audio-bgm-derived-assets.ps1`, `check-fighter-production-manifest.ps1`,
`check-mp-topology-fixtures.ps1`, `check-r2-collision-mtx.ps1` and
`check-untracked-dependencies.py`. Inventory checkers do not establish visible
output. Parse structured-file edits.

For an unresolved timing hypothesis, use one discriminating short A/B with
matching inputs, window and route engagement. Extend for noise or an unobserved
intermittent workload, not by habit.

Keep these reporting distinctions explicit:

- Label **WORK P50 and WORK P95 separately**, with units and population; never
  substitute the median into the gate gap. The targets are the plan's.
- Parent buckets in the tick HUD are nested: independent percentiles or package
  savings do not add.
- Include the 2/3/4/5+ VBlank histogram and the maximum interval. Zero slip
  counters alone do not prove timely presents.
- The cross-build noise floor is about 14,080 ticks; a smaller delta needs paired
  or same-ROM evidence.

Use coherent guest publication for debugger reads: a `volatile` read or poke is
not cache coherence. Prefer ring collection to per-frame stops. Account for
intermittent work from totals and event counts; do not trim the event away.

## Command lifecycle, logs and runner isolation

Capture stdout and stderr from launch, one log name per run, and keep the launch,
log path and expected completion marker with the batch's record. Require writer
exit, exit status and complete output: a stale `.gdb.out` or a filtered success
line is not completion. Infrastructure failures are not ROM verdicts; repair the
demonstrated cause before rerunning. A stalled command is investigated, not
automatically killed and restarted.

Pass PowerShell array parameters directly or via `pwsh -Command`, not a numeric
list through native `pwsh -File` (`-Frames 900,960` becomes `900960`).

Use only the repo-local accuracy-focused melonDS, interpreter/JIT disabled from
boot, in numbered runner slots with private ports, logs and storage
(`New-MelonDSRunnerSlots.ps1` provisions them; `check-melonds-policy.ps1` audits
them). Never alter the owner's own instance or stop melonDS by name. Up to 12
isolated correctness slots may run at once, but no concurrent authoritative
timing or exact visual acceptance. Emulator runs must not take focus.

Capture at a guest scene or event anchor. Healthy guest progress with a frozen
image needs capture diagnosis, not an assumed hang. Do not weaken image
thresholds to get green.

## Acceptance and publication

Every built ROM, lab diagnostics included, excludes the N64 graphics
interpreters, generic compatibility renderers and software scene compositors. A
native flag alone does not prove it: positive native engagement and the intended
output are required, not zero counters alone.

Save permanent proof under `artifacts/performance` and `artifacts/visibility`;
raw logs, runner configs, storage images and binaries are not committed (the
repository is public: no ROM-derived assets). `builds/` holds generated output
only.

A commit or build is not acceptance, and the owner's delivery target is not
proof of completion. Report failed, blocked, stale and unrun checks.
