# R2-02 E0 — What the stage prepares every frame, and why none of it changes

**Date:** 2026-07-27
**Phase:** R2-02 (`Smash64DS_Runtime2_SwitchPlan.md` §7), sizing only.
**Status:** E0 read-only. No runtime change, no build. Sizing and design; E1 is
the implementation.
**Standing rules apply.** This exists because of "size a rewrite before you
scope it" (Task 96) — the arithmetic below is one read, not one session.

---

## 1. The claim being sized

Plan §7 R2-02: replace the stage's
`discover / validate / rebuild / resolve / prepare / submit` shape with a direct
owned path — "the runtime shape is `DreamLand_Run17()`". The question E0 has to
answer before anything is written: **how much of what the stage does per frame
is actually frame-invariant, and what is removing it worth?**

## 2. What `ndsRendererNativeStagePrepareRun` does, per run, per frame

Read at `src/nds/nds_renderer.c:19669`. For each of the 21 runs, every frame:

| step | what it does | frame-invariant? |
|---|---|---|
| read `sNdsNativeStageRuns[run_index]` | generated table | **yes — already baked** |
| read `sNdsNativeStageTextureEpochs[...]` | generated table | **yes — already baked** |
| read `sNdsNativeStageStatePolicies[...]` | generated table | **yes — already baked** |
| `ndsRendererNativeStagePolicyMatches(policy, stats)` | re-verifies the baked policy against live traversal state | yes, given unchanged topology |
| `memset(prepared, 0, sizeof(*prepared))` | blanket clear | — |
| `memset(&resolved, 0, sizeof(resolved))` | blanket clear | — |
| `ndsRendererHardwareUseTexture(stats)` | re-derives texture enable | yes |
| `ndsRendererHardwareTextureImplicitStateOn(stats)` | re-derives implicit state | yes |
| `ndsRendererHardwareResolveStageSourceFrameTexture(...)` | **per-frame texture resolution** | yes |
| scale/tile/offset reads from `stats` | re-derives | yes |
| write 10 fields of `NDSNativeStagePreparedRun` | the actual output | — |

Three observations, in order of how much they matter:

1. **Every input except `stats` is a generated table already in ROM.** The run,
   the epoch and the state policy are all baked. The function re-reads baked
   data and re-derives a conclusion the generator could have written down.

2. **`stats` is the only live input, and Task 44 already proved it does not
   change.** Steady-state stage admission is a Dream Land asset-mutation
   generation compare plus an eight-segment structural guard, shipped and
   enabled. The invariant R2-02 needs is therefore not a new assumption — it is
   an invariant the tree already validates every frame and already trusts
   enough to ship.

3. **The two `memset`s are the Task 104 pattern, unfixed.** A blanket clear of a
   struct followed by writing the fields that matter. Task 104 measured exactly
   this shape on `NDSRendererStats` and took `STG` P50 −22,016 by removing both
   accesses to the same lines. §3.4 of the plan bans the shape outright.

## 3. Sizing

Two attributions, quoted deliberately (R2-00b §3 — never mix them):

**By bracket** (`scripts/census-stage-run-phases.ps1`, board-recorded):

| block | ticks/frame | calls |
|---|---|---|
| `PrepareRun` head | **67,119** | 21 |
| `ndsRendererAdapterPrepareNativeStageMatrices` | **55,077** | 1 |
| combined | **122,196** | |

**By location** (R2-00b census, exclusive self-time):

| symbol | ticks/frame |
|---|---|
| `ndsRendererNativeStagePrepareRun.constprop.0` | 24,868 |
| `ndsRendererNativeStageBeginRun` | 13,533 |
| `ndsRendererNativeStageApplyStateSpan.constprop.0` | 11,204 |
| `ndsRendererPrepareNativeStageOwner` | 9,115 |
| `ndsRendererAdapterStageWorldSourceKeyMatches` | 7,713 |
| `ndsRendererAdapterPrepareNativeStageOwner` | 4,260 |

The bracket figure is the right one for a phase gate because it includes the
callees the elision also removes (policy match, texture resolve, both clears).
The location figure is the right one for knowing which function to edit. They
disagree by ~2.7× on `PrepareRun` for exactly the reason R2-00b §3 records.

**Target:** `STG` is ~366K after Task 104 against a frozen 180K budget, so
R2-02 must find ~186K. The two blocks above are 122,196 of it — 66%. That is
enough to make the phase worth starting and not enough to finish it, which is
the honest position to start from.

## 4. Why this is not the memo that failed before

Two previous attempts in this area were reverted, and the difference matters:

- **Task 79** (texture site memo) and **Task 81** (stage texture-identity memo
  at the bind seam) were *per-bind* memos. Task 81 measured **zero stage texture
  binds in battle**, so it was inert; Task 79's transfer cost exceeded its
  saving. Both memoized a lookup while leaving the surrounding work in place.
- R2-02 does not memoize a lookup. It **removes an entire per-frame phase**
  whose output is already known, under a guard the tree already computes for
  another purpose. There is no new per-run transfer, and with one call per frame
  for the matrices there is no per-run transfer problem of the kind that killed
  Task 79 E1.

The board's own reading agrees: "(1) and (2) are per-frame preparation over a
topology Task 44 has already proven unchanged, which is the shape an incremental
update attacks."

## 5. The design E1 should implement

Preferred, in plan order (§2: "extend the generators downward, delete the
scaffolding upward"):

1. **Bake `NDSNativeStagePreparedRun` at build time** in
   `generate_nds_native_stage.py`, for all 21 runs — `poly_fmt`, texture params,
   dimensions, format, `alpha_test`/`alpha_ref`, `textured`. All are functions
   of the generated run/epoch/policy tables plus the state the generator already
   models.
2. **Resolve the one genuinely runtime field, `texture_entry`, once at match
   load** and store it beside the baked table. §3.8: match loading may be
   expensive, gameplay may not.
3. **Delete `PrepareRun` from the frame** behind `NDS_R2_STAGE_DIRECT`, keeping
   the Task 44 guard as the admission test. If the guard fires, fall back to the
   Runtime 1 path for that frame — the fallback must stay, because §3.8 forbids
   *unpredictable* work, not *guarded* work.

A cheaper intermediate exists — run the existing prepare once and cache — and it
should be resisted as the endpoint, because it is the runtime discovery the plan
says R2 deletes rather than schedules. It is acceptable only as an E1a
measurement step to confirm the saving is where §3 says it is before spending
generator work.

## 6. Gate for E1

- `STG` P50/P95 from a matched 8-frame A/B, control vs candidate, one tree, flag
  off versus on (Task 79: vary the build, not the run).
- Screenshot-identical static stage, owner as the visual oracle.
- Boundary green.
- **Kill line:** if the elision lands under ~40,000 on `STG` P50 — roughly a
  third of what the brackets predict — the bracket attribution is wrong and the
  phase should be re-sized rather than pushed. Task 103 E7 realised 28% of its
  prediction for a related reason and that is the precedent to respect.

## 7. Cost

One read of one function and two existing measurements. No build, no emulator
run.
