# R2-01 — The battle-path roof, and what it cost

**Date:** 2026-07-27
**Phase:** R2-01 (`Smash64DS_Runtime2_SwitchPlan.md` §7).
**Status:** **KEEP, gate met, default off.** Boundary green; frame cost within
the measured build-placement noise floor.
**Standing rules apply.**

**Branch:** `codex/r2-runtime2`. **Flag:** `NDS_R2_PATH` (default 0).
**Arms:** `builds/build-r2-01-tickhud-off` (`E09028F2…`) and
`builds/build-r2-01-on` (`C449FED3…`), both
`smash64ds-battle-playable-tickhud-hwtri`, one tree, flag off versus on.
**Evidence:** `artifacts/r2-01-ab/`.

---

## 1. What R2-01 is for

Plan §7: "isolates the scene-flow seam once so later phases swap renderers under
a stable roof." It is explicitly not a performance change, and its gate is that
it does not become one.

The Runtime 1 battle loop lives inside a ~30-condition
`#if NDS_DEV_SCENE_HARNESS == ...` chain in `src/port/taskman_seam.c:7621`,
shared with every mp-collision development harness, carrying two runtime flags
(`is_battle_playable`, `use_realtime_presentation`) that it branches on about a
dozen times per iteration. Under Boundary both are compile-time constants.

`src/nds/r2/nds_r2_battle.c` is that loop with the constants folded: **116 bytes
of text** driving eight named Runtime 1 operations. Every branch removed was
provably not taken in the shipped configuration.

## 2. Built so the default arm cannot move

This mattered more than the feature. Tasks 87, 88, 89, 94 and 95 all regressed
for one shared reason — editing a hot translation unit re-addresses its
neighbours — so the 0 arm had to be untouched, not merely equivalent.

- Every `taskman_seam.c` edit is inside `#if NDS_R2_PATH`.
- The new translation unit enters `CFILES` **only when the flag is 1**, so the
  0 arm's link input set is unchanged. An empty object still enters a link, and
  that was not worth risking. Verified: `nds_r2_battle.o` is absent from the
  control build.
- The only unconditional change is one inert `#define` in the generated config
  header.

Fails closed rather than silently: the C rejects any harness but
`battle_playable`, rejects `NDS_HARNESS_FAST_LOGIC`, and `#error`s on the
`NDS_SCENE_MIP_CACHE_LAB` seed path rather than quietly dropping it.
`NDS_R2_PATH` joins `print-benchmark-flags`, so `check-tickhud-parity.ps1`
guards it against drift between the published and tick-HUD targets.

## 3. Gates

**Correctness — Boundary verifier on the R2 path: PASS.** Built with
`NDS_R2_PATH=1` through the harness's own make environment; "Boundary
verification profile passed." Every static check green, including
`check-harness-registry.ps1`, which caught a real defect first (see §5).

**Frame cost — matched 128-sample A/B, one tree, flag off versus on:**

| bucket | control P50 | candidate P50 | ΔP50 | ΔP95 |
|---|---|---|---|---|
| `ALL` | 1,680,064 | 1,680,000 | **−64** | 0 |
| `WORK` | 1,326,080 | 1,330,368 | +4,288 | +5,504 |
| `WORK-H` | 1,321,728 | 1,326,464 | +4,736 | +24,512 |
| `FTR` | 543,104 | 543,168 | +64 | −960 |
| `STG` | 351,488 | 351,872 | +384 | +1,216 |
| `SRC` | 324,224 | 325,952 | +1,728 | +1,728 |

VBlank interval share, normalized (never min-FPS):

| | 3-VBI | 4-VBI | 5+-VBI |
|---|---|---|---|
| control | 90.3 | 8.8 | 0.9 |
| candidate | **90.6** | **8.5** | 0.9 |

Cadence violations: 0 both arms.

**Verdict: within noise.** `WORK` P50 +4,288 sits under the 5,000–7,000
build-placement floor Task 100 measured directly. The paired per-frame test
distinguishes placement from mechanism, and it says placement: the delta is
positive on 81 frames and negative on 47, median +1,728. A real regression
appears on ~128 of 128 with a consistent sign — that is how Task 79 caught a
6,880-tick table-sharing defect. This does not have that shape. The VBlank
histogram, which `AGENTS.md` names as the actual pacing signal, is marginally
better on the candidate.

**Do not read the `WORK-H` P95 +24,512 as a result.** R2-00a established that
the HUD under-counts `WAIT` on tail frames and that the shortfall relocates into
whichever phase is running, so `WORK-H` P95 is the least trustworthy number in
this table until that bracket is fixed. `WORK` P95 (+5,504) and the VBlank
histogram are the ones to read.

## 4. What it bought

Nothing, and that is the correct outcome. R2-01 exists so that R2-02 and R2-03
have one place to swap the renderer instead of each re-deriving where the battle
loop lives. It cost approximately nothing measurable to get it, which is the
gate.

## 5. One defect the tooling caught, and it was right

`check-harness-registry.ps1` failed the first Boundary attempt:
`src/nds/r2/nds_r2_battle.c` referenced a scene-harness macro without including
`nds_scene_harness_config.h`. The reference was in a comment, so the checker was
text-matching — but the complaint was correct in substance: the file *is*
harness-specific by construction, since the constants it folds are only constant
under `battle_playable`. It now includes the canonical config and asserts that
precondition itself rather than inheriting the macros by luck. Rewording the
comment would have passed the check and left the coupling undocumented.

## 6. Cost

Four builds, one Boundary run, one 128-sample A/B pair.
