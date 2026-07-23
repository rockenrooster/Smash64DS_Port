# TASK 49 — `NDS_BATTLE_PROFILE` axis + GX equivalence differ

**Standing rules apply in full: read `docs/optimization/TASK_STANDING_RULES.md`
first.**

Branch: `codex/task49-battle-profile-axis` — **branch from current master**, not
from the Task 48 branch. Task 49 does not depend on Task 48's instrument.

Campaign: `docs/optimization/PROFILE0_NATIVE_CAMPAIGN.md` §4, §5.
Predecessor: Task 48 passed its kill criterion (H1 100.000%, H2 42/42); Tasks 51
and 52 are authorized and **this task builds the oracle that will judge them**.

This task ships no rendering change. It ships a build axis that is a no-op at its
default, and a differ. Its value is entirely in being trustworthy, so the two
control experiments in §4 are not optional garnish — they are the deliverable.

---

## Part 1 — The `NDS_BATTLE_PROFILE` axis

The owner ruled on 2026-07-23: today's Profile 0 is demoted to Profile 1 and
becomes the correctness oracle; today's Profile 1 becomes Profile 2; the new
Profile 0 is the DS-native precompiled path. **The renumbering is additive.**

Do **not** renumber `NDS_RENDERER_PROFILE_LEVEL`. It appears at ~700 sites (391
in `src/nds/nds_renderer.c`, 159 in `src/port/reloc_backend_renderer_dl.c`, 32
in `src/port/taskman_seam.c`, 32 in `scripts/check-gbi-decode-fixtures.ps1`,
plus `nds_platform.c`, `nds_ifcommon_oam.c`, `sprite_preview_backend.c`), many
of them ordinal (`>= 1`, `!= 1`, `== 2`). A literal renumber changes no behavior
and risks the published ROM for zero measured gain.

1. Declare the flag beside the other renderer flags (`Makefile:40-53`, next to
   `NDS_TASK48_MODELSPACE_CENSUS`):

   ```make
   # Battle pipeline selector. Orthogonal to NDS_RENDERER_PROFILE_LEVEL, which
   # is the *instrumentation* level within profiles 1 and 2 and keeps its
   # existing values (0 lean, 1 phase timers, 2 full oracle).
   #   0 = DS-native precompiled path (Tasks 51/52; not implemented yet)
   #   1 = today's shipping translation path — the correctness oracle
   #   2 = instrumented / semantic oracle build of that same path
   NDS_BATTLE_PROFILE ?= 1
   ```

2. Reject anything outside 0/1/2 with `$(error)`.

3. **`NDS_BATTLE_PROFILE=0` must fail the build with a clear `$(error)` until
   Task 51 lands.** It must never silently fall through to the profile-1 path.
   This project has paid for silent no-ops three times — Task 37's `:=`-vs-`=`
   shipped a byte-identical ROM while claiming a change (`Makefile:100-118`
   records it), and Task 46's census was garbage-collected because its callers
   were empty stubs. A selector whose unimplemented value quietly builds
   something else is the same failure with a new name.

4. Emit it into `nds_build_config.h` beside the others (`Makefile:1340-1352`)
   and into `print-benchmark-flags` (`Makefile:1593-1607`) as
   `BENCH_MAKE_BATTLE_PROFILE`.

5. Add `override NDS_BATTLE_PROFILE := 1` to the published target block
   (`Makefile:168`) **and to the tick-HUD/proof block** (`Makefile:219` and
   `:228`). Standing rule: any flag on one is on both. Adding it to
   `print-benchmark-flags` means `scripts/check-tickhud-parity.ps1` covers it
   automatically from that moment on — verify that it does by running the
   checker and confirming the flag count rises from 42.

6. Document the two axes once, in `docs/optimization/NATIVE_RENDERER_PLAN.md`
   (the renderer contract doc named by `docs/goal-objective.md:114`), so the
   next reader is not left inferring it from the Makefile.

**Gate for Part 1: the published ROM must still hash `1818AA77…` and the
tick-HUD ROM `60C68AFF…`, byte-for-byte.** The axis is a pure no-op at default
and must be provable as one.

---

## Part 2 — The GX equivalence differ

Tasks 51 and 52 will replace how the stage and the fighters reach the GPU. The
only defensible way to accept them is to prove the new stream is equivalent to
the one the demoted path emits. That is this differ.

### The design constraint that decides the whole thing

Profile 0 will emit `MTX_PUSH` → `MTX_MULT4x3(constant model)` → `MTX_POP` under
a once-loaded view. Profile 1 emits `MATRIX_LOAD4X4` of a CPU-composed
`projection × view × model` product (`src/nds/nds_renderer.c:12849-12868`;
the stage's live loads come from `binding_composed[]` via
`ndsRendererNativeStageLoadNoZMatrix`, `:20257`).

**These two will not be bit-identical and must not be gated as if they could
be.** The DS composes the clip matrix in hardware; we compose in 20.12 fixed
point on the CPU; 4×3 drops a row the hardware reconstructs. Demanding
bit-identity here is exactly the failure mode
`TASK_STANDING_RULES.md` §"Exactness is within reason" describes: a gate
asserting a quantity the port never promised.

So the differ has two tiers with **different** standards, and the task's report
must state which tier each result came from.

### Tier 1 — non-matrix stream, bit-exact, no tolerance

Task 48 measured this partition directly: 2,858 words per frame, **100.000%
bit-identical across 24 frames**, in every window
(`artifacts/performance/2026-07-23_task48-modelspace-census.md`). Classes
`CONTROL`, `ALPHA_TEST`, `TEXTURE_PARAM`, `TEXTURE_BIND`, `POLY_FORMAT`,
`BEGIN`, `COLOR`, `TEX_COORD`, `VERTEX16`
(`include/nds/nds_renderer.h:293-318`).

Geometry, colours, texture coordinates, polygon formats and texture binds must
match **word for word**. There is no float-to-fixed argument available here —
both paths read the same baked source data. Any divergence in Tier 1 is a defect
in the native path, full stop.

### Tier 2 — matrix equivalence by effective transform, with a stated tolerance

Recompose on the host and compare the *result*, not the operands:

- from the profile-1 stream, take each binding's `LOAD4X4` composed matrix;
- from the profile-0 stream, take the once-loaded view/projection and the
  binding's `MULT4x3` model matrix and compose them the way the DS geometry
  engine would;
- transform each binding's 8 bounding-box corners under both and compare.

**Report the deviation in screen-space pixels after projection**, because that
is the unit the fidelity doctrine is written in and the only one the owner's
eyeball test can be reconciled against. Propose a threshold, justify it, and
state it in the certificate. Do **not** pick a number that makes the gate pass —
if you cannot defend a threshold, say so and stop; that is a legitimate outcome
of this task.

### Capture mechanism — reuse, do not rebuild

- **Tier 0, free, already exists.** `NDS_TASK29_GX_CENSUS` records per-class and
  per-owner command/word counts and FNV stream hashes
  (`include/nds/nds_renderer.h:321-347`; owners are STAGE / MARIO / FOX at
  `:284-288`). Use it as the cheap "did anything change, and in which owner and
  class" screen before any word-level capture.
- **Word-level capture** follows the Task 34 recorder's shape
  (`src/nds/nds_renderer.c:460-580`) but brackets **per owner** rather than per
  stage segment. Note the capacity: Task 34's buffer is 2,800 entries / 7,000
  words (`include/nds/nds_renderer.h:350-351`) and the stage alone uses
  2,557 / 6,894 — a whole-frame capture does not fit. Capture one owner per run,
  selected by a flag, and size the buffer from a measured count rather than a
  guess. Report overflow and fault counters in every certificate, as Task 34
  and Task 48 both do.

### Build constraints you will hit

- `NDS_TASK29_GX_CENSUS` requires `NDS_RENDERER_PROFILE_LEVEL == 1`
  (`include/nds/nds_renderer.h:77`), so differ targets are profile-1 lab
  targets. The published profile-0 configuration cannot host the differ; that is
  expected and is why profile 1 is the oracle.
- **Profile-1 builds must set `NDS_FAST_WALLPAPER_AFFINE=0`** — profile 1 plus
  affine OOMs the taskman arena. Known, unfixed, accepted
  (`TASK_STANDING_RULES.md` §Process). Getting this wrong costs a session to a
  boot hang that looks like a code defect.
- Non-publishing A/B targets get their **own** Makefile block. Appending targets
  to the Task 41 tickhud/proof block breaks
  `scripts/check-gbi-decode-fixtures.ps1:1847`, which pins that block
  structurally (exact filter list and inner `ifeq` text).
- `override` in a target block beats a command-line variable, so an A/B pair
  cannot be built by passing a flag to a target that forces it. Two targets, not
  one with a flag.

---

## Part 3 — Controls. This is the deliverable.

A differ that has never reported a divergence is not known to work. Both of
these run before any result from this task is quoted anywhere.

1. **Positive control (self-identity).** Run the differ with profile 1 against
   profile 1 — the same ROM, two runs, same window. It must report **0
   divergences in Tier 1 and zero deviation in Tier 2**. If it does not, the
   capture is non-deterministic and nothing downstream is interpretable. Task
   45's plan flagged exactly this control as the one that should have run first
   and did not.
2. **Negative control (known-divergence injection).** In a throwaway lab build,
   perturb exactly one thing — one vertex word, then separately one binding's
   model matrix by one LSB — and confirm the differ names it: correct owner,
   correct class, correct entry, and for the matrix case a non-zero screen-space
   deviation. Revert the perturbation; it never merges. **Report both injections
   and what the differ said.**

---

## Gates

- Published ROM `1818AA77…` and tick-HUD ROM `60C68AFF…` byte-identical.
- `scripts/check-tickhud-parity.ps1` green, flag count risen from 42 to 43.
- `NDS_BATTLE_PROFILE=0` fails the build with the intended error message
  (show the message in the report).
- Both controls in Part 3 pass and are reported with their actual output.
- `.\scripts\verify-dev-fast.ps1` then `.\scripts\verify-boundary.ps1`. The
  `battle_playable` locked-30 pacing red and `check-architecture.ps1`'s failure
  on tracked `artifacts/performance/*.json` are **pre-existing** — see
  `docs/HANDOFF.md` and `docs/PERF_LEDGER.md:14-22`. Do not chase them; do not
  report them as new.

## Stop criterion

If the Tier 2 tolerance cannot be stated in screen-space pixels with a defensible
justification, **stop and report**. Part 1 and Tier 1 still land and are useful
on their own; a Tier 2 gate with an unjustified magic number is worse than no
Tier 2 gate, because Tasks 51 and 52 will both be judged by it.

## Constraints

- `decomp/` is read-only. `decomp/sm64ds-decomp` has **no headers** — it is
  10,914 single-function files, most still `func_020XXXXX`. Read
  `Matrix4x3_*.c`, `Geometry_MatrixMultiply3x3.c` and
  `_Z13CopyToViewMatPK9Matrix4x3.c` for the 4×3 and view-matrix conventions the
  Tier 2 recomposition must model. `decomp/sm64-nds/src/nds/nds_renderer.c`
  (1,285 lines) is the readable DS-side reference.
- One writer on `src/nds/nds_renderer.c`; prefer a new TU for the recorder.
- New flags default to the no-op value.
- Long builds detached (Start-Process → log → poll a completion stamp), never
  foreground.
- Time-box open-ended debugging to ~10 emulator runs or ~1 hour, then checkpoint
  to the branch and report.
- Separate commits: (1) axis + no-op proof, (2) differ capture + analyzer,
  (3) controls + results, (4) docs.
- Cite `file:line` for anything asserted about existing behavior.
- Part 1 is a KEEP candidate and may merge `--no-ff` once green, since it is a
  proven no-op. Part 2 merges with it if the controls pass. **Never push.**

## Deliverables

1. `NDS_BATTLE_PROFILE` in the Makefile, config header, `print-benchmark-flags`,
   published block and tick-HUD/proof block; the two-axis note in
   `docs/optimization/NATIVE_RENDERER_PLAN.md`.
2. The differ: capture instrument (default off) + host analyzer, with Tier 1 and
   Tier 2 reported separately.
3. `artifacts/performance/2026-07-23_task49-gx-differ.json` + `.md` certificate
   in the Task 34 certificate format (`docs/PERF_LEDGER.md:159-170`), including
   both controls and both ROM hashes.
4. Results section appended to this file; one `PERF_LEDGER.md` entry; brief
   `HANDOFF.md` update.
5. `.\scripts\verify-dev-fast.ps1` green, then
   `.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` as the final action.
