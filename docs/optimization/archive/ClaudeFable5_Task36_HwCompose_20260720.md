# TASK 36 — Stage static lane: hardware matrix compose, then bake

**Standing rules apply in full: read `docs/optimization/TASK_STANDING_RULES.md`
first** (process, fidelity doctrine, device-test economy). One prior-decision note:
the owner CANCELLED the Task 35 attribution census (2026-07-20) — the evidence below
already answers it; do not build census instrumentation.

**Supersession notice:** Task 34's closure ("do not start E2/E3") was correct for
the CURRENT architecture — CPU-composed camera×model matrices make the stage stream
0.000% frame-conserved. This task changes the architecture first, then re-derives
that census. Task 30/31 closures are untouched; do not reopen them.

## Evidence this stands on (cite, don't re-derive)

- Stage owner ≈ 461-465K melonDS ticks, CONSTANT across every phase window
  including Whispy animation (`artifacts/performance/2026-07-18_task25r-phase-matrix.md`)
  — the dynamic elements (Whispy's eyes/nose/mouth, 4 flower sets) are not the cost.
- Task 32 PC census (`artifacts/performance/2026-07-20_task32-draw-hot-text.md`):
  top main-RAM draw samples are `ndsRendererMtxMul20p12` (198/900),
  `ndsRendererLoadHardwareMatrixPair`, `ndsRendererNativeStageLoadNoZMatrix`,
  `ndsRendererMtxLoadN64ToDS20p12`, `ndsRendererMtxMulAffine20p12`,
  `ndsRendererCommitNativeStageSegment`, `ndsRendererNativeStagePrepareRun` —
  the static-path matrix-compose and emission machinery.
- Task 34 E1 (`artifacts/performance/2026-07-20_task34-e1-stage-stream.md`): 2,557
  commands / 6,894 words per frame; ALL 42 display-bearing DObjs vary every frame
  because the camera is CPU-folded into every matrix upload.
- 23R Phase 0 certificate: 43 live-camera accesses + 42 composed matrices; manifest
  `docs/optimization/NDS_NATIVE_STAGE_CONSUMED_FIELDS.generated.json`.

## Thesis

The static stage costs ~460K because the CPU re-derives and re-emits all 42 DObjs
every frame, folding the live camera into every matrix upload. Move the compose into
the GX — load the view matrix ONCE per frame, then per rigid DObj multiply its
CONSTANT local matrix in hardware — and (a) the census-top CPU matrix work
disappears, (b) per-DObj uploads become frame-constant, which re-opens stream
conservation and the bake. Phase A alone should cut a triple-digit-K slice off the
stage owner in melonDS (CPU-work removal — melonDS SEES it, unlike Tasks 17/32);
Phase B compounds it. Fidelity cost: 20.12 hardware-multiply low-bit differences —
sub-pixel vertex wobble, covered by the fidelity doctrine.

## Step 0 — codify the doctrine

Add to AGENTS.md (Hard Rules, adjacent to the retail-engagement rule):
> "Rendering-side changes may approximate: they gate on a reported fidelity budget
> (synchronized screenshot diffs + the owner's visual approval), not pixel-exactness.
> Gameplay/source behavior remains bit-exact and verifier-gated. Engagement proof is
> a counter on the shared engagement HUD row, confirmed by batched device smoke
> boots — per-feature retail runs are reserved for cache/TCM/DMA/IO-class claims,
> which melonDS cannot referee; CPU-work-removal claims may KEEP on melonDS typed
> A/B evidence behind their flag until the next device checkpoint."
Commit separately with a one-line HANDOFF note.

## Phase A — hardware compose for rigid DObjs

1. Map the current per-DObj matrix flow first and write it down in the task report:
   where the camera×model compose happens (`ndsRendererMtxMul20p12` callers,
   `ndsRendererAdapterBuildDObjWorldMatrix`, `ndsRendererLoadHardwareMatrixPair`,
   the no-Z variant `ndsRendererNativeStageLoadNoZMatrix`), which GX matrix mode the
   renderer runs (position/vector pair semantics — lighting normals consume the
   vector matrix; get both right or lighting shifts), and where the raw-Z vs no-Z
   vs range classes diverge.
2. At stage load, precompute each rigid DObj's constant LOCAL matrix in 20.12 once
   (from the same source data the per-frame path uses today). Add a one-soak debug
   check that re-derives per frame and asserts constancy for the rigid set. The
   dynamic set = every DObj that FAILS the check (expect Whispy eyes/nose/mouth +
   the 4 flower sets; report any surprise, e.g. water ST scroll) — it stays on the
   existing CPU path untouched in every phase.
3. Per frame: upload the camera/view matrix once; for each rigid DObj, MTX_MULT its
   stored local matrix in hardware (restore/push-pop or reload-view+mult per DObj —
   pick by measuring both if unsure, report which and why), then submit the same
   vertex/material words as today. CPU compose functions must vanish from the rigid
   path (verify: re-run the Task-32-style PC census afterward and show
   `ndsRendererMtxMul20p12` samples collapse; note the .text.hot.draw set may
   deserve re-derivation later — flag it, don't do it in this task).
4. Everything behind `NDS_TASK36_HW_COMPOSE` (default 0; A/B pair buildable).
5. **Gates:**
   - Gameplay bit-exactness: continuous-runtime verifier + boundary green; source
     markers/trace identical to control (rendering-only diff).
   - Fidelity: synchronized A/B screenshots for the standard windows
     (countdown438-445, early600-607, whispy1398-1405); report changed-pixel counts
     + mean delta; attach PNGs; **the owner approves visuals**. Watch specifically for
     lighting shifts (vector matrix) and no-Z painter-band misorders (band×W depth
     rows must stay coherent with hardware-composed W).
   - Perf: melonDS synchronized A/B on the stage owner typed counter (must drop;
     report exact deltas + the calibration-predicted device delta). This is
     melonDS-sufficient class: KEEP on melonDS evidence. Add a hardware-composed-
     DObjs-per-frame counter to the shared engagement HUD row and BUILD the retail
     A/B pair into `builds/device-queue/` for the next batched checkpoint — do not
     ask the owner to run it now.
   - Full-match soak, zero fallbacks/asserts, results screen clean.
   **Kill criterion:** fidelity artifacts the owner rejects, or painter/no-Z depth
   breaks structurally, or melonDS stage saving < ~40K (not worth the added
   pipeline) → checkpoint on WIP branch, report, stop.

## Phase B — conservation census, then bake (only after Phase A KEEP)

1. Re-run the Task 34 E1 census against the Phase-A renderer, same three windows,
   same 60% gate: per-DObj words should now be frame-conserved for the rigid set
   (matrix uploads constant, vertex/material words already constant).
2. If ≥60% conserved: bake the conserved rigid stream at match start; per-frame
   prologue (one view matrix) + replay baked words + CPU path for dynamic DObjs and
   fighters. ARENA GUARD mandatory: allocate the bake buffer from a static array or
   before the taskman arena reservation, then assert post-boot that the adaptive
   arena (src/port/diagnostics.c:7306-7350) still received its full size and print
   it on the HUD — a degraded arena is a FAILED gate even if the game boots (the
   affine OOM lesson). Fail-closed invalidation (overlay/mask/viewport change, any
   rigid DObj failing constancy) → full CPU path that frame + fallback counter.
   Engagement/fallback counters on the shared engagement HUD row.
3. Fidelity: replay output should be EXACT vs Phase A (same words) — pixel-compare
   against Phase A and report; any divergence is a bug, not an approximation.
4. Transport: CPU word-copy replay ONLY (melonDS-sufficient class). GX-FIFO DMA is
   device-only class and OPT-IN: do not build it unless the owner explicitly grants a
   device session. If the copy loop itself measures ≥~80K in the typed counter,
   REPORT that as the DMA opportunity and stop there.
5. Device: nothing per-phase. Queue the Phase A and Phase B A/B pairs plus the
   updated smoke-boot ROM in `builds/device-queue/` with a one-page ordered
   checklist; the whole campaign is confirmed in ONE batched session whenever the owner
   chooses.
   **Kill criterion:** conservation <60% after hardware compose (report WHY — which
   words still vary), or replay instability in soak → keep Phase A, stop B.

## Fences

Rigid stage DObjs only — fighters, items, effects, wallpaper, HUD untouched.
Dynamic set (Whispy face, flowers, plus any constancy-check surprises) stays on the
current path in every phase. No decomp edits. No gameplay-side caching. Task 30/31
stay closed. Separate commits per phase; snapshot at the end.
