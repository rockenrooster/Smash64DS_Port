# Jump A/B/C Task Blocks — 2026-07-15 23:26 (Claude Fable 5)

## User override — 2026-07-16

The Jump A `<=500K` stack keep gate and Jump C `>=80K` / `<=336,576`
pre-code keep gates are withdrawn. Those numbers remain milestone targets, but
every repeatable correctness-preserving gain is kept and accumulated. The task
blocks below retain their historical wording; this override governs reruns.

Supersedes the Jump A stack and Jump C blocks in
`ClaudeFable5_JumpA_Stack_And_716_Decision_20260715_2140.md`. Written after
verifying tonight's ledger rows and live code. Two corrections to the prior
plan, both verified in-tree:

1. **Old CUT 3 is already done.** The raw-Z current-matrix stage class already
   loads `raw_composed` and submits raw vertices for hardware transform
   (`ndsRendererNativeStageBeginRun`, src/nds/nds_renderer.c:15812-15816), and
   the ten formerly range-rejected triangles now ride `scaled_raw_modelview`
   (:15818-15827). Do not re-propose hardware T&L for the raw class.
2. **Old CUT 2's premise was stale.** The projection divide already uses the
   DS hardware divider (`div64`, src/nds/nds_renderer.c:4493). There is no
   soft-float divide to remove in the stage projection path. The ">=150K from
   this cut alone" estimate is withdrawn; the cut is redefined below and must
   be bounded by a bucket profile before coding.

The new dominant Jump A lever: extend the proven scaled-raw hardware path to
the **no-Z painter classes** with a constant-Z matrix row (CUT 3-NEW below).

Jump B (M4) is closed — no task. Its only obligations are re-proof on the
final published CPU-on ROM (already on the board) and defending its >=128 KiB
reserve against the crowd-audio bake (handled in the audio task).

Task order: TASK 1 (depth-band fix landing) -> TASK 2 (Jump A stack v2) ->
TASK 3 (Jump C v2). Task 1 changes no-Z depth semantics, so no A/B baseline
for Task 2 may be sampled before Task 1 lands and the owner signs off.

---

## TASK 1 — Land and verify the stage depth-band fix (paste to codex)

```
/task Finish, verify, and land the uncommitted stage depth-band work in
src/nds/nds_renderer.c. This is a correctness task, not a perf task; it also
re-baselines M3 for the next Jump A stack, so it must land first.

The working tree already contains (do not revert; reconcile per
OPTIMIZATION_ROADMAP R0 and finish it):
- ndsRendererHardwareNextProjectedDepth now consumes one full
  NDS_RENDERER_HW_PROJECTED_DEPTH_STEP per painter primitive instead of 1/6th
  of a step (src/nds/nds_renderer.c:~10528). Root cause of the grass/bush
  overlap: six consecutive no-Z triangles shared one post-division depth, so
  an earlier stage triangle rejected a later grass/bush draw.
- ndsRendererHardwareSourceDepthToV16 clamps real source Z to
  [NDS_RENDERER_HW_SOURCE_DEPTH_MIN, NDS_RENDERER_HW_SOURCE_DEPTH_MAX],
  reserving 128 strictly ordered painter depths at each v16 endpoint so
  camera extremes cannot push perspective Z into a painter band (:~4514).
- New stage depth-trace instrumentation hooked into the semantic event path
  (ndsRendererStageDepthTraceEvent): per-class counts, background/foreground
  painter min/max/count, a no-Z collision counter, and a running hash.

Gates (all must pass before commit):
1. Profile-2 lab build: gNdsRendererStageDepthTraceNoZCollisionCount == 0
   across the 8-frame window; background and foreground painter bands strictly
   monotonic and fully inside their reserved 128-depth endpoint bands; publish
   the per-class triangle counts (this class census is also Phase 0 input for
   the next task — save it).
2. Exact semantics unchanged: 8 callbacks / mask 255 / 57 DObjs / 42 bindings
   / 54 runs / 202 stage triangles / 49 epochs / 4 material commits, owner
   121/828, zero fallback, M4 22/131072 with zero post-GO fence classes.
3. Same-ROM A/B/A ticks vs the committed 645,248/645,440 stage baseline:
   expected roughly neutral; publish the delta either way in the ledger row.
   If the fix costs >10K stage P50, report before committing.
4. Visual evidence for the owner: frames 438/439 plus the pause-orbit camera set
   (normal / front / +33.6 degrees, same deterministic gate as the clip fix)
   showing (a) grass/bushes no longer overlapped by stage floor triangles and
   (b) whether the "main floor path texture moving around" symptom under
   camera motion is gone — the shared-depth collision was Z-fighting, which
   reads as texture swimming, so this fix is the most likely cure. Do NOT
   mark the PLAYTESTING_Review.md pause line FIXED yourself; the owner confirms
   by eyeball and marks it.

Ledger row (correctness class) with ROM SHA, JSON, screenshots. Commit as its
own change. Constraints: decomp/ read-only; no gate weakening; no unrelated
edits; snapshot with New-Smash64DSSnapshot.ps1 -Mode Lean as the final action.
```

---

## TASK 1B — Grass/bush overlap persists after the painter fix (paste to codex)

Added 2026-07-16 after the owner's retest: commit 87e04fa5d3 landed with zero
painter collisions, strict order, and source citations — and the visual
overlap is UNCHANGED. The counter bug was real but was not this defect. This
task replaces the old "Bug 1" block's diagnosis tree.

```
/task Stage grass/bush overlap persists after 87e04fa5d3 (strict painter
depths, zero collisions, profile-2 order pass) — the painter counter was a
real but different bug. Diagnose with one-variable isolation probes BEFORE
proposing any fix. CORRECTNESS task, own commit, NOT perf-gated, no
optimization bundled. Reconcile first (R0).

PRIME SUSPECT — the cross-producer depth-scale seam. The depth buffer has two
producers since the M3 owner landed (ef65ef541c): the 126 no-Z triangles
submit CPU-packed v16 painter depths under identity matrices
(ndsRendererLoadHardwareMatrices(NULL, FALSE)), while the 66 source-Z and 10
range triangles get depths computed BY GX HARDWARE from raw_composed
(src/nds/nds_renderer.c:15813-15814, built :16135-16139) and the scaled-raw
matrix (:15818-15827). glFlush uses GL_TRANS_MANUALSORT Z-buffering
(src/nds/nds_platform.c:1983). The ledger's "real stage Z 3605..3728" is the
CPU ORACLE's projected value — nothing yet proves the HARDWARE's actual
depth-buffer values for those 76 triangles land strictly BETWEEN the
foreground painter band (-3969..-4022) and background band (4095..4024) in
depth-buffer units. If hardware depths come out nearer than the foreground
band, real geometry covers the grass/bush decals — exactly the symptom.
Timeline fits: stage fidelity was accepted ~7/12 (all-CPU depth), M3 moved
these triangles to hardware depth, overlap was reported 7/15. Also audit the
scaled-raw compensation specifically: halved coordinates with a compensating
matrix must not halve or rescale depth relative to the unscaled raw class.

PHASE 1 — identify, do not guess (all probes are build-flag or config
toggles, one variable each, screenshot per probe):
1. Reproduce the offending composition. The existing frame 438/501 pixel
   gates PASS while the owner sees the defect, so the current gates look at the
   wrong pixels. Capture the grass-top edge and side-bush regions at the
   camera position matching the owner's report; Claude reads the captures.
2. Class isolation: render once with the 10 range triangles skipped, once
   with the 66 source-Z skipped (or forced through the CPU-projected
   fallback path the M3 contract guarantees), once with the 126 no-Z
   skipped. Whichever toggle makes the overlap vanish names the offending
   producer. Publish all captures.
3. Pre-M3 A/B: run a pre-ef65ef541c ROM at the same view. If the overlap is
   absent there, the M3 hardware-depth migration introduced it; if present,
   the defect predates M3 and classification becomes the lead.

PHASE 2 — root-cause per Phase 1 outcome:
- If the hardware classes are the producer: derive the DS Z-buffering
  depth-value formula (GBATEK; depth from z/w through the viewport mapping)
  and compute the actual depth-buffer values raw_composed and the scaled-raw
  matrix produce for the offending triangles vs the values the identity-
  matrix CPU path produces for packed painter depths. Fix the normalization
  so, in DEPTH-BUFFER UNITS, foreground band < every hardware source-Z depth
  < background band across the match camera envelope. Then PROVE it: extend
  the stage depth trace to record the derived hardware depth per raw/range
  triangle alongside the CPU submitted_z, and gate foreground/background
  band separation on those numbers.
- If order/classification is the producer: verify the offending grass/bush
  DObjs' class membership against the source layers (BattleShip
  grdisplay.c:52-63,86-95,111-118,134-141 no-Z/Z/no-Z/no-Z and
  grpupupu.c:637-690 construction order — read only, cite exact lines) and
  correct the port classification to match.

PHASE 3 — gates: new ROI pixel gate ON THE DEFECT REGION (grass-top edge +
side bushes at the reproducing camera), kept as a permanent ratchet; the owner
eyeball confirms before the playtest line is marked FIXED; profile-2
202-triangle order and class census unchanged (66/126/10); painter bands
still strict/zero-collision; exact 121/828; M4 fence classes zero; DevFast +
Boundary green. Ledger correctness row citing which hypothesis Phase 1
confirmed and which it excluded. Separate commit; do not start Jump A stack
work in this task — but note: the depth-scale derivation produced here is a
PREREQUISITE artifact for Jump A CUT 3-NEW, so save the mapping math in the
ledger row. Snapshot last.
```

Sequencing amendment: TASK 1B replaces TASK 1 as the blocker — the Jump A
stack (TASK 2) must not baseline until 1B lands, because CUT 3-NEW moves the
no-Z class onto hardware depth and would inherit a broken seam.

---

## TASK 2 — Jump A stack v2: stage owner 645K -> <=500K (paste to codex)

```
/task Jump A stack v2 on the M3 stage owner. Baseline is the post-depth-fix
stage P50/P95 from the previous task (do not reuse 645,248 if the depth fix
moved it). Target: stage P50 <=500K, then push toward 450K.

THE OWNER'S AUTHORIZATION — ledger rule amendment: the PERF_LEDGER
M3-DENSE-PREPARE-ONCE row's "do not retry or widen dense-only preparation
reuse" is amended to: dense-index preparation reuse may be re-landed ONLY as
the base of this measured stack, and the keep gate applies to the STACK TOTAL
(combined <=500K), not to each component alone. Nothing below the stack gate
ships. Record this amendment in the new ledger row and cite this task.

Forbidden (proven dead, do not revive): whole-owner FIFO copy/patch/DMA
packet; any new per-root interpreter; per-joint GX hierarchy matrices;
widening dense reuse beyond the previously measured design. Cross-matrix
stage classes (5/10/15) stay untouched. Reconcile first (R0).

PHASE 0 — bucket profile before any cut (R2). On the new baseline, publish
the stage owner's internal composition: per-submit-class triangle counts
(from the depth trace census in the previous task), count of CPU projections
per frame and their measured ticks, div64 call count, attribute-preparation
ticks, and run/batch overhead. Give each cut below a pre-code bound from
these numbers. If the bounds cannot jointly reach -145K, STOP and report
instead of coding.

CUT 1 — re-land dense-index prepare-once as the stack base.
- Reconstruct the previously measured design exactly: 606 corner references
  -> 312 dense vertices; 408 projected references -> 246 unique; removes 294
  repeated attribute preparations, 162 repeated transforms, 486 projections.
  Measured -108,960 P50 with exact semantics and zero conflicts
  (PERF_LEDGER M3-DENSE-PREPARE-ONCE; artifacts/visibility/
  m3-dense-prepare-8frames.json). It was hand-reverted, not git-reverted.
- Gate: profile-2 semantic oracle zero-mismatch; same-ROM A/B/A reproduces
  approximately -109K on the new baseline; exact 121/828; reserve >=128 KiB.

CUT 2 (redefined) — no-Z projection overhead, bounded by Phase 0.
- CORRECTION: the divide is already on the hardware divider (div64,
  src/nds/nds_renderer.c:4493). Do not propose replacing a soft divide.
- Remaining levers, in order of expected value: overlap divider latency by
  issuing the x/y/z divides of a vertex (or the three vertices of a triangle)
  back-to-back and doing clamp/attribute work while the divider runs, instead
  of div64's blocking spin per call; hoist repeated s64 scaling
  (NDS_RENDERER_HW_PROJECTED_VERTEX multiplies) where operands repeat after
  CUT 1's dedup; flatten per-vertex call overhead in the class-3 path.
- Exactness: quotients must remain bit-identical (same hardware divider, same
  truncation) — this cut may not change any projected value, only its cost.
  Profile-2 oracle zero-mismatch is the gate.
- Bank only if the measured saving matches the Phase 0 pre-code bound within
  reason; publish both numbers.

CUT 3-NEW — move the no-Z painter classes onto GX with a constant-Z scaled
raw matrix (the dominant lever; mechanism already proven in-tree).
- Mechanism: the PROJECTED_RANGE_OR_MATRIX path already loads an identity
  projection plus scaled_raw_modelview and lets GX transform and clip
  (src/nds/nds_renderer.c:15818-15827, matrix at :2470). Extend it: for the
  class-3 no-Z painter triangles, submit model-space vertices with a raw
  composed matrix whose Z row is [0, 0, 0, band_constant] so hardware
  transforms X/Y (and clips) while every vertex of the primitive lands on the
  exact painter depth. The band constant comes from the SAME
  ndsRendererHardwareNextProjectedDepth counter — painter ordering semantics
  are unchanged by construction.
- Transport bound (compute pre-code, publish): one raw 4x3 matrix reload per
  painter step (~13 FIFO words); at ~126 painter triangles that is ~1.6K
  words per frame — verify against measured FIFO headroom before coding. If
  measured transport exceeds the CPU projection ticks it removes, STOP (this
  is the T2B lesson; do not brute-force it).
- Depth mapping proof: the hardware-produced depth for a band constant must
  reproduce exactly the v16 painter band values the CPU path writes today.
  Derive the viewport/depth mapping, then PROVE it with the stage depth
  trace: submitted_z recorded per triangle must equal the CPU-path values for
  the same frame. Zero tolerance on depth; the bands are the bug fix you just
  landed.
- X/Y drift: hardware transform may differ sub-pixel from CPU div64
  projection. Presentation follows the 90% DS rule (NATIVE_RENDERER_PLAN
  Shared Rules) — publish max per-vertex X/Y deviation and hold the captures
  for the owner's sign-off, exactly as the scaled-raw clip fix did. Fence-over-
  floor layering across background/foreground phases must be visually intact
  in the pause-orbit camera set.
- Fallback: keep the CPU class-3 path compiled and selectable; any triangle
  the mechanism cannot serve exactly falls back per-run, fail-closed.

Stack gate and closeout: combined stage P50 <=500K on same-ROM A/B/A
(8 frames, then 32/128 falsifier for keeps); exact 121/828 and all M3
semantic counters; M4 fence classes zero on a one-minute natural CPU-on run
with exactly one teardown; reserve >=128 KiB; screenshots + image analysis;
one ledger row PER CUT including any reverts, plus a stack-total row. Then
re-profile the whole mode-163 frame (update/audio/HUD/flush/VBlank/residual)
and publish new draw/present/loop P50/P95. Land keeps as separate commits;
snapshot with New-Smash64DSSnapshot.ps1 -Mode Lean as the final action.
```

---

## TASK 3 — Jump C v2: fighter compute cut, M2 416K -> <=336K (paste to codex)

```
/task Jump C v2 on the M2 fighter owner (Mode 8 production path). Baseline:
combined Mario+Fox 416,576/416,704 P50/P95 (ledger M2-MODE8-ITCM-PLACEMENT
A-arm). First gate per the board: >=80K combined saving AND <=336,576.

Evidence framing: the ITCM placement experiment moved the same instructions
to zero-wait memory and saved only 18,080 — the owner is compute-bound, not
fetch-bound. The compute is the target: soft-float matrix construction and
per-vertex work. Keep the pre-GX active-animlock/dynamic-matrix fail-closed
guard (commit 6d6fba9685) exactly as is.

Forbidden (proven dead): whole-owner FIFO copy/patch/DMA packet (+124K);
per-joint GX hierarchy matrices (84 restores); DS hardware lighting (misses
102/413 exact RGB15 cases); any new per-root interpreter; retrying ITCM
placement ALONE. Combination rule from the board: the measured 18K ITCM
placement may be re-added only combined with a compute cut whose pre-code
bound plus 18K clears the 80K gate.

PHASE 0 — bound before coding (R2). Publish the current matrix-construction
and lighting bucket ticks on this baseline (prior measurements: matrix
~72,896; lighting ~79,648 — re-measure, do not trust stale numbers). If
matrix + eligible lighting residual + 18K ITCM cannot reach 80K, STOP and
report; do not code a cut that cannot clear the gate.

CUT 1 — fighter local-matrix construction in fixed point / source tables.
- The per-joint locals are built in software float and quantized to 16.16 via
  syMatrixF2LFixedW: syMatrixTraRotRpyRSca / syMatrixRotPyrR / syMatrixRotRpyR
  / syMatrixTraRotPyrRSca at src/port/reloc_backend_renderer_dl.c:333,
  504-519, 526, 544, 595, 764, 887; syMatrixF2LFixedW at :489, :589. ARM9
  has no FPU; every f32 op is a library call.
- If the builders reach libm sinf/cosf, route rotation through the BattleShip
  source's own u16-angle sin/cos tables — read the source first and cite
  file:line for the exact table and matrix functions; do not hand-author
  trig. Where the source already quantizes to 16.16, do the arithmetic in
  fixed point rather than float.
- Exactness: the output contract is the composed 16.16 matrix the RSP-side
  consumer sees. Gate: profile-2 matrix oracle bit-exact against the float
  path, OR publish max per-element deviation and hold captures for the owner's
  sign-off (display math; sub-LSB drift is acceptable ONLY if gated and
  signed off). Do not regress the fixed-W quantization boundary.
- Keep the float path compiled as the shadow oracle during qualification.

CUT 2 — fighter lighting: measure, then touch only if real residual exists.
- The path already has prepared per-part light direction and a 2,096-byte
  shade LUT cache (ndsRendererHardwareLitShadeColorLut nds_renderer.c:2041,
  prepared_light_direction :2105, cache :2004-2010). If Phase 0 shows the
  bucket is already a bare LUT lookup, record that and STOP — do not
  manufacture a cut. Lighting stays CPU and source-exact.

CUT 3 (only if time after 1+2) — extend the KRAW shared kernel to more
fighter raw runs than it covers today (246 Mario + 234 Fox). Stacked, not a
rewrite.

Gates: combined fighter P50 <=336,576 with >=80K saved (ITCM combo counts
only per the rule above); exact geometry, owner state, texture traffic,
screenshot, reserve >=128 KiB; no fallback, no allocation, no ledger-on-only
win; 32-root/49-epoch/67-run/626-triangle contract intact. Same-ROM A/B/A,
8-frame falsifier then 32/128 for keeps. One ledger row per cut including
reverts. Coordinate nds_renderer.c edits with the stage lane (one-writer).
Snapshot with New-Smash64DSSnapshot.ps1 -Mode Lean as the final action.
```

---

## TASK 4 — Mario black right pant leg + missing underside polys (paste to codex)

Added 2026-07-16. the owner confirmed against the original: the underside is
closed and blue on N64 — both symptoms are port defects, no source-accurate
branch. Reference: the owner's melonDS capture, Mario airborne from behind/below.

```
/task Mario fighter visual defect, two co-located symptoms: (1) right pant
leg renders black (left is correct blue); (2) underside/crotch is missing
one or two polygons. the owner verified the original: underside is CLOSED and
BLUE — both are port defects. Longstanding and stable ("for a while"),
likely the documented "Mario facing/light A/B" visual-debt item.
CORRECTNESS task, own commit, NOT perf-gated, no optimization bundled, no
hand-authored geometry. Reconcile first (R0).

PRIME SUSPECT — a mishandled cross-matrix run at the hip. Both symptoms
sit where cross-part vertex-cache sharing concentrates (persistent vertex
cache + cross-matrix semantics in the M2 contract; the fighter cross-matrix
triangles live at joints like the pelvis). ONE bad cross-matrix run there
explains BOTH at once: lit colors/normals evaluated under the wrong part
state render black, and positions read from a stale or overwritten cache
slot produce degenerate (invisible) triangles. Runner-up suspect: a
run->epoch binding off-by-one hitting exactly one part's material/light
state.

PHASE 1 — locate, then discriminate (one A/B, no fix yet):
1. Map the defect to runs: use the profile-2 oracle's projected vertices to
   identify which root/part/run owns the black screen region and which
   source triangles cover the underside. Check FIRST whether those runs are
   cross-matrix class. Do not guess the part index.
2. Owner A/B: build one ROM with the Mode-8 native fighter owner disabled
   (source-driven fallback path), same scene, scripted-jump deterministic
   airborne capture, both ROMs to the owner / captures for Claude.
   - Defect only with owner ON -> owner bug: dump the offending runs'
     vertex-cache epochs and run->epoch->material/light bindings vs the
     recorded contract; fix the binding/cache handling, not the lighting
     math.
   - Defect in BOTH -> shared decode bug predating the owner: dump the
     part's light direction/color/ambient and shade-LUT inputs
     (ndsRendererHardwareLitShadeColorLut nds_renderer.c:2041,
     prepared_light_direction :2105) and compare against the BattleShip
     fighter display/lighting source for that DObj (read-only, cite
     file:line; suspects: normal-transform matrix for that part,
     G_MW_LIGHTCOL decode, per-part G_LIGHTING geometry-mode toggle,
     G_CULL_BACK mapping for the underside triangles).
3. Missing-poly census check: count the source DL triangles for the
   affected parts against the owner's recorded runs — the owner census is
   self-referential (it verifies what was RECORDED, not that recording
   captured every source triangle). If the recorder missed triangles,
   re-record/extend the contract; census and checker numbers update ONLY
   together with the source citation justifying the new count.

Gates: side-by-side captures (owner on/off/fixed) — the owner eyeballs leg
color AND closed blue underside before any FIXED claim; M2 contract counts
unchanged unless a recorder gap is proven with source citation; stage owner
untouched; M4 fence classes zero; exact vertex-cache/cross-matrix semantics
preserved; DevFast + Boundary green. Ledger correctness row stating which
hypothesis was confirmed/excluded. Separate commit; coordinate
nds_renderer.c edits with the depth-fix lane (one-writer). Snapshot last.
```

---

## TASK 5 — Locked-30 fixed-two source scheduler (completed)

Added and completed 2026-07-16. the owner's amended checkpoint decision selects a
locked-30 cap with exactly two unchanged source updates per presented frame and
no catch-up. A phase that fits two vblanks reaches 60 source updates/s; an
overloaded phase slows uniformly like the source console while audio remains
hardware-paced. Canonical natural qualification measured 4,084 updates / 2,042
presents and phase rates 39.9/37.9/39.5/n.a./58.2 updates/s.

```
/task Implement the locked-30 scheduler: presentation is capped at a 2-vblank
cadence and each presented frame owns exactly two source updates. Never repay
a slipped slot. DECISION RECORD: this is the owner's amended July-16 checkpoint
outcome — target is locked 30 when the budget permits; 60 FPS presentation is
not claimed, and source-faithful slowdown is accepted under load. Update the P1
board explicitly (never silently). Reconcile first (R0).

ORIGINAL FINDING: the realtime battle loop ran input -> source update -> draw
once per presented frame. The amended owner now runs the input/tick/update path
twice before one draw/present. Game speed is therefore exactly twice present
rate, with no elapsed-vblank debt and no 2/3-update judder.

ARCHITECTURE (behavior-sacred: each update remains the unmodified source
60Hz tick — NO delta scaling, NO half-rate logic, NO skipped logic frames):
1. Fixed source batching: each loop iteration runs exactly two source updates,
   with no vblank-owed debt, catch-up, or cap-4 path. Run both source updates
   back-to-back, EACH preceded by its own input
   read (ndsPlatformReadInput + syControllerReadDeviceData/
   syControllerUpdateGlobalData) so input sampling stays 60 Hz like the
   original per-retrace read.
2. Present cadence: one draw+flush per iteration, presented on a 2-vblank
   boundary. Never present faster than every 2 vblanks (locked cap). If a
   frame overruns its slot, present at the next vblank boundary (slip to 3
   vblanks accepted and counted per phase). Never repay the missed wall-time
   tick: uniform slowdown matches source-console lag behavior and the next
   frame returns immediately to the fixed two-update batch.
3. Preserve the existing terminal-update rule (LoadScene break — BattleShip
   syTaskmanRunTask never draws the terminal update, keep the cited
   comment), the Wait->GO texture-arm hook, and the mip-cache lab seeding
   path. Fast-logic verifier mode is untouched.
4. Audio: BGM refills stay hardware-timer paced (unchanged). FGM triggers
   now fire at correct wall time as a side effect of correct update rate —
   note in the ledger row that crowd/fade timing complaints must be
   re-ear-checked AFTER this lands, before further audio work.
5. Debug HUD (debug builds): show updates/s next to presented FPS.

BUDGET DECISION (R2): two updates plus draw are about 1,081K ticks against the
1,120,380-tick two-vblank budget. A catch-up third update makes a slipped slot
about 1,360K and therefore cannot fit back under the two-vblank boundary; one
transient spike can lock that design near 20 FPS. Fixed-two self-recovers on
the next frame and keeps motion uniform rather than alternating 2/3 ticks.

GATES (hardware-timer anchored, per the standing real-time rule):
- HARD everywhere: updates equal exactly two times presents over every sampled
  window. No debt/catch-up accounting is permitted.
- HARD in phases that hold 30 presents/s: updates per wall-second 59.0-61.0 and
  the in-game timer advances one source second per wall second.
- Elsewhere publish per-phase updates/s; this is the direct slowdown percentage
  (40/s = 67% speed) and identifies where Jump A CUT 3-NEW margin remains.
- HARD immediately: presents never exceed one per 2 vblanks; per-phase slip
  counts (countdown / early combat / late combat / KO / Results) published.
- Per-phase "holds 30" gates start as PUBLISHED METRICS and are promoted to
  hard gates as the Jump A / Jump C cuts bring each phase's
  (updates+draw+flush) under the 1,120,380-tick 2-vblank budget. Publish
  per-phase P50/P95 against that budget — this doubles as the board's
  July-16 phase-evidence table.
- Audit every mode-163 verifier expectation that assumed 1 update == 1
  present (frame/update counters, fps assertions, the demoted 59.3-60.3
  pacing gate) and retarget them to the locked-30 contract: 60 Hz updates +
  exact 2:1 update/present ratio plus phase slowdown metrics, default-hard.
  Pacing is port presentation policy — cite
  this decision row for expectation changes; per-update source behavior is
  unchanged and stays source-gated. Never weaken a semantic gate to pass.

BOARD/DOCS: update P1_EXECUTION_BOARD July-16 checkpoint row with the
explicit decision (locked-30 chosen; 60 not claimed); retarget acceptance
rows referencing 60 FPS; PERF_LEDGER row records the new pacing mode and
notes ALL future A/B baselines must state pacing mode and resample (present
now includes multiple updates — old baselines are not comparable).

Surfaces: taskman_seam.c, nds_platform.c, verifiers, docs — disjoint from
nds_renderer.c, so this may run in a parallel lane to the stage-depth task
with one-writer coordination. Rebuild the canonical ROM; the owner playtest
gate: correct game speed by feel/stopwatch, stable cadence. Separate
commit(s); snapshot with New-Smash64DSSnapshot.ps1 -Mode Lean last.
```

---

## TASK 6 — Combat draw cut: CUT 3-NEW + dense re-land, fast-loop banked (paste to codex)

Added 2026-07-16 after the locked-30 scheduler landed. Target arithmetic:
combat slot = 2 updates + draw + flush <= 1,120,380 for locked-30 at full
speed; measured combat runs ~20 presents/s (~2/3 speed). With CPU-on updates
near ~165K each, draw+flush must reach ~<=790K; the measured windows show
draw 936K-1,014K. Gap: roughly 225K, owned by the stage's remaining CPU
projection work and continued accumulation. Governed by the user override:
bank every repeatable correctness-preserving gain, own ledger row each.

```
/task Land combat draw cuts toward the locked-30 full-speed budget
(draw+flush ~<=790K; publish the real number from Phase 0). Governed by the
user no-discard override: every correctness-preserving gain is banked with
its own ledger row. Reconcile first (R0). Forbidden: FIFO packet
copy/patch/DMA caching, new interpreters, run reordering/merging (draw order
is source-sacred), any poly-ID/translucency semantic change (that belongs to
the open grass/bush investigation), decomp/ edits.

FAST ITERATION PROTOCOL (use for every cut; full match only at the end):
- A/B lab profile-1 ROM, synchronized 8-frame window (stage window 438..445
  and fighter window 600..607 as applicable): stage/draw/active P50/P95.
- Quick visual: the deterministic native 256x192 top-screen compare
  (expect 0/49,152 changed pixels when exactness is claimed; publish any
  nonzero with the exact-aspect capture for the owner).
- Fast FPS check: canonical smoke
  verify-battle-playable-realtime-harness.ps1 -RequireLocked30Pacing
  -SkipScreenshot -> presents/s + updates/s + cadence violations in ~1 min.
- Only after the last banked cut: full one-minute natural CPU-on match
  (phase updates/s + slip counts + fences + teardown + reserve) and one
  30-second the owner eyeball ROM. Snapshot last.

PHASE 0 — current-HEAD bucket profile (no code): publish the stage owner's
internal composition (per-class emit ticks: 66 raw / 10 range / 126 no-Z,
projection+transform ticks, attribute prep, run-transition/texture-bind
overhead, accounting) and the CPU-on combat numbers: per-update tick cost
with live Fox AI and combat-phase draw P50. Give each cut below a pre-code
bound from these numbers; report the bounds before coding.

CUT A — constant-depth GX submission for the 126 no-Z painter triangles
(Jump A CUT 3-NEW; the dominant lever).
- Mechanism: submit their model-space vertices through the proven scaled-raw
  hardware path (same X/Y transform as the range class, which already passes
  pixel gates). Constant post-divide depth per primitive: build the per-
  primitive matrix with its Z row set to band_constant x W_row, so
  z_clip = c * w and z/w == band_constant exactly for every vertex. The band
  constant comes from the SAME painter counter — order semantics unchanged
  by construction.
- Depth exactness (zero tolerance): derive the DS viewport depth mapping so
  the hardware depth-buffer value for band_constant equals what the CPU path
  writes today for the same packed v16 depth; prove per-triangle with the
  stage depth trace (record the hardware-path depth beside the CPU
  submitted_z; every band value must match). X/Y may drift sub-pixel under
  the 90% presentation rule — publish the pixel delta; first CUT-A ROM gets
  a the owner eyeball including: grass/bush overlap UNCHANGED (known open bug —
  parity against the current baseline, defect included), fence-over-floor
  layering intact, water intact, pause-orbit set clean.
- Transport bound pre-code: ~126 per-primitive Z-row matrix updates/frame
  (MTX_LOAD_4x3 ~13 FIFO words -> ~1.6K words, negligible; a cheaper partial
  load is fine if provably equivalent). If measured transport exceeds the
  CPU work removed, STOP and record (T2B rule).
- Predicted saving: the dense experiment measured 486 projections + 162
  transforms + 294 preps = 108,960; CUT A removes the projection+transform
  share for these triangles plus the remaining EmitNoZTriangle s64 work.
  Publish the pre-code bound; bank whatever lands.
- Fallback: per-run fail-closed to the CPU path; zero fallback in the final
  banked state.

CUT B — re-land dense-index prepare-once on the remaining CPU work (own
row). The previously measured design (606->312 dense, 408->246 unique, zero
conflicts, exact semantics) was reverted only for a now-withdrawn gate.
After CUT A its saving shrinks (projection dedup overlaps) — expect mainly
the repeated attribute-preparation share. Re-measure honestly; bank the
remainder under the override.

CUT C (only if Phase 0 shows it is real) — overlap hardware-divider latency
for whatever CPU projections remain (cross-matrix classes), and/or the
largest remaining Phase-0 bucket with a stated pre-code bound. Do not
manufacture a cut; record and stop if the buckets are bare.

Per-cut gates: exact stage census 8/57/42/54/202 + cross 5/10/15, owner
121/828, zero fallback/fence/conservation error, M4 22/131072 intact,
reserve >=128 KiB, ledger row per cut (KEEP or REVERT with numbers). End of
task: canonical rebuild, one-minute natural match with per-phase updates/s
and slip counts published (combat updates/s vs the 37.9-39.9 baseline IS the
user-visible result), the owner playtest ROM, board/handoff updates, snapshot
with New-Smash64DSSnapshot.ps1 -Mode Lean as the final action.
```

---

## TASK 7 — Freeze forensics kit + scripted repro (paste to codex)

Added 2026-07-16. the owner reports periodic random freezes; latest trigger:
Mario down-air connecting on Fox while airborne. Hit events fire three
freeze-shaped paths at once — FGM ARM7 ACK spin-wait, hit-effect poly spawn
(GX poly/vertex RAM overflow -> permanent swap-buffer stall), and the
board's open P0 damage/throw collision stall. DS freezes are usually stalls,
not aborts, so the kit pairs an exception screen with a watchdog that
reports the interrupted PC.

```
/task Freeze forensics: build the diagnostic kit for the periodic random
freeze (latest repro: Mario down-air connecting on Fox while airborne),
then attempt scripted reproduction. DIAGNOSTIC task first — do NOT fix
anything until the kit names the stall; likely related to the board's open
P0 "damage/throw map collision" stall. Own commit(s). Reconcile first (R0).

KIT (all shipping in the canonical ROM; sub-screen output, near-zero
steady-state cost):
1. ARM9 exception handler rendering to the bottom debug console: fault type,
   PC, LR, fault address, register dump, and the last breadcrumb. (Catches
   aborts — necessary but not sufficient.)
2. Watchdog: a hardware-timer IRQ (~2 s period) checks a main-loop heartbeat
   counter. If unchanged, print a STALL report to the bottom screen from the
   IRQ: interrupted PC and LR (from the IRQ return frame — this is the
   smoking gun for any spin-wait), last N breadcrumbs, and the discriminator
   state: GXSTAT (FIFO full/busy, box/pos/vec test busy), polygon and vertex
   RAM counts, swap-buffer pending flag, IPC/FIFO queue state, audio FGM
   command/ACK counters and the channel owner map, current update phase and
   frame/update counters. The report must stay on screen (freeze persists) so
   the owner can photograph it.
3. Breadcrumbs: a tiny fixed ring buffer of phase markers written at cheap
   boundaries — update start, hit-search/damage entry, effect spawn, FGM
   play/ACK wait entry+exit, draw start, flush, vblank wait, present done.
   One u32 store each; no formatting in the hot path.
4. Audit every unbounded spin-wait in the port (grep while-loops around: GX
   FIFO status, swap buffer, FGM/ARM7 ACK (src/nds/nds_audio_fgm.c ACK
   paths), IPC sends, vblank waits). Add a bounded-timeout counter to each:
   on timeout, increment a named safety counter and emit the breadcrumb —
   do NOT change behavior yet (no silent recovery); the goal is naming the
   loop, not masking it.

REPRO HARNESS: extend the scripted-input machinery into a soak: Mario
repeatedly jumps and lands down-air on Fox (vary spacing/timing per
iteration), full hit/effect/SFX path live, CPU-on, running until freeze or N
thousand updates. Run it in melonDS with the GDB stub enabled; on watchdog
STALL or hang, capture PC/backtrace via the stub and screenshot the
sub-screen report. If it reproduces, publish: stuck PC, breadcrumb trail,
GXSTAT/poly/vertex counts, FGM ACK state — then STOP and report the named
root cause before any fix.

Gates: kit adds zero measurable cost to the fast-loop A/B (publish the
delta; heartbeat+breadcrumbs are plain stores); canonical ROM rebuilt with
kit enabled; DevFast + Boundary green; existing safety counters unchanged;
one-minute natural match still passes with zero watchdog trips in normal
play. Ledger note (diagnostic class). Docs: DIAGNOSTIC_REFERENCE.md gains
the STALL report field key so the owner's photos are decodable. Snapshot last.
```

---

## TASK 8 — Combat draw cuts round 2: preflight, residual, CUT A carried (paste to codex)

Added 2026-07-16 after TASK 6 closed with Cuts C+D banked (-51K) and a stop.
Measured truth (TASK6-R0 Phase 0, window 600..607, live Fox): draw+flush
1,245,664 P50; two-update batch 314,144 (~157K/update); full-speed locked-30
needs draw+flush <=~800K -> gap ~445K. Where it lives: preflight 286,848;
prepare 215,616 (calibrated attr bound 56,448 remains); no-Z inclusive
98,816 (matrix 50,432) — CUT A never attempted; begin/bind 39,808; commit
170,528; and ~370K of draw is UNATTRIBUTED outside the stage+fighter owners
(stage 489K + fighters ~387K vs draw 1,245K). Fighter per-joint matrix
construction remains soft-float (~50-70K; only F2L quantization landed).
Update-path floats are source gameplay math — IEEE-exact, untouchable.

```
/task Combat draw cuts round 2, toward draw+flush <=~800K (current 1,245,664
P50, window 600..607, live Fox). Governed by the no-discard override: bank
every correctness-preserving gain, own ledger row each. Use TASK 6's fast
iteration protocol verbatim (8-frame A/B + native 256x192 compare +
-RequireLocked30Pacing smoke; full match only at the end). Forbidden
unchanged: FIFO packet caching, new interpreters, run reordering, poly-ID/
translucency changes, gameplay float conversion (update math stays
IEEE-exact), decomp/ edits. Reconcile first (R0).

PHASE 0.5 — profile the unowned draw residual FIRST (no code): stage 489K +
fighters ~387K leave roughly 370K of draw+flush unattributed. Extend the
Phase-0 stopwatch to cover everything outside the two owners: begin-frame,
foreground/HUD submission, texture-state churn between owners, semantic/
accounting shells, flush preparation, present residuals. Publish the table;
the largest named bucket becomes a cut candidate with a pre-code bound.

CUT E — generation-gated preflight (bound: large share of 286,848). The
whole-owner preflight revalidates immutable topology every frame. Split it:
validate immutable structures ONCE at arm/scene-setup and stamp a generation
counter; per-frame preflight checks only live-bound state (matrices,
materials, colors, selection, animlock) plus the generation stamps, and
fails closed to the full validator on ANY mismatch. The fail-closed
guarantee and every rejection path must remain provably reachable — add a
lab-only fault-injection check that flips one stamped byte and confirms the
full validator re-engages. Zero visual change; native compare 0/49,152.

CUT A (carried from TASK 6, still open) — constant-depth GX submission for
the no-Z painter class (bound: ~60-90K of the measured 98,816 inclusive;
no-Z matrix bucket alone is 50,432). Mechanism unchanged from TASK 6: Z row
= band_constant x W_row so post-divide depth is exactly the band constant;
zero-tolerance depth proof via the stage depth trace; X/Y sub-pixel drift
published + one the owner eyeball; parity against current baseline including
the known grass/bush defect; per-run fail-closed fallback; transport bound
before coding (T2B rule).

CUT F — fighter local-matrix construction in fixed point / source tables
(Jump C CUT 1 proper; bound: re-measure, previously ~72,896, only the F2L
quantization has landed). Route rotation through the BattleShip source's
own u16-angle sin/cos tables (cite file:line; no hand-authored trig); do
the concat in fixed point where the source already quantizes to 16.16.
DISPLAY math only — the shadow float oracle stays compiled; gate bit-exact
OR publish max per-element deviation for the owner sign-off. Do not regress the
fixed-W quantization boundary.

CUT G2 — idempotent GX state elision in begin/bind (bound: share of
39,808). Track last-written texture params, matrix mode, and poly state;
skip writes whose value is unchanged. The final GX state per run must be
provably identical — assert via the existing semantic/texture counters.

CUT H (only if profile confirms it is real) — sub-screen console dirty
updates: rewrite only changed characters per present instead of full lines.

AUDIT (no-code deliverable): confirm zero draw-side derivation executes
inside the update pair (recorder stubs, display prep) — anything found is
per-present work running twice and gets its own bounded cut row.

Per-cut gates: exact censuses (owner 121/828, stage 8/255/57/42/54/202/49/4,
cross 5/10/15, M2 32/49/67/626, M4 22/131072), zero fallback/fence/
conservation error, reserve >=128 KiB, native compare 0/49,152 for exact
cuts, ledger row per cut incl. reverts. End of task: canonical rebuild,
one-minute natural CPU-on match with per-phase updates/s + slip counts
(combat updates/s vs the current baseline IS the user-visible result),
the owner playtest ROM, board/handoff updates, snapshot last.
```

---

## TASK 9 — Bit-exact soft-float acceleration for gameplay updates (paste to codex)

Added 2026-07-16. Fixed-point conversion of update math is forbidden
(feedback-loop divergence + destroys the source-exactness oracle), but the
same bits can be produced faster. Build facts: all code compiles -mthumb
(correct for the 16-bit main-RAM bus); no custom float routines exist —
every gameplay float op funnels through stock libgcc __aeabi_* in waitstated
main RAM; ITCM is 25,824/32,768 (~6.9K free) and runs ARM code on a
zero-wait 32-bit bus. Reference semantics for "exact" = current libgcc
outputs (the established port-exact baseline all behavior gates verify).

```
/task Bit-exact soft-float acceleration for the gameplay update pair
(baseline: two-update batch 314,144 P50, window 600..607, live Fox). The
HARD RULE for this task: every gameplay float result stays bit-identical to
the CURRENT libgcc soft-float outputs — that is the established port-exact
reference all behavior gates already verify against. No -ffast-math, no
flush-to-zero, no NaN/denormal/rounding semantic change, no libm
replacement, no fixed-point conversion of update math. Governed by the
no-discard override; own ledger row per phase. Reconcile first (R0).

PHASE 0 — float-call census (lab only, no shipping change): count per-update
calls to each __aeabi_* routine (fadd/fsub/fmul/fdiv/fcmp*/f2iz/i2f/f2d/
d2f/d* doubles if any) via linker --wrap counting shims in a lab build, and
record each routine's linked MODE (ARM vs Thumb, objdump the ELF) and
section placement. Publish the table with a per-routine tick bound
(count x measured routine cost). This names the Phase 1/2 targets and the
honest ceiling.

PHASE 1 — relocate hot libgcc float routines to ITCM, code unchanged
(bit-identical by construction). Extract the relevant libgcc objects,
rename their text sections into the ITCM region, link ahead of libgcc.
Budget: current ITCM is 25,824/32,768 and the renderer owns what is placed
— fit within free space, report exact bytes, and do NOT evict renderer
ITCM; if contention, report instead of evicting. Measure the two-update
batch A/B; bank whatever lands.

PHASE 2 — optimized ARM-mode IEEE-754-single routines in ITCM for the
census leaders (typically fmul/fadd/fcmp/f2iz/i2f). Reference = current
libgcc bit behavior. Proof ladder, all required before shipping:
1. Host-side: exhaustive comparison over all 2^32 inputs for every unary
   routine; for binary routines, directed edge vectors (every rounding
   boundary class, NaN payloads, +-0, denormals, infinities, overflow/
   underflow) plus >=10^8 randomized vectors, against a libgcc golden.
2. On-device supreme gate: the deterministic scripted match, thousands of
   updates, per-update full game-state hash A/B against the baseline ROM —
   ANY single-bit divergence at any update = revert, no exceptions.
3. Every existing behavior verifier green (they assert source-exact values
   and double as the regression net).

ALSO (report-only): confirm the ARM9 stack lives in DTCM for the update
path and that hot update frames do not spill to main RAM; note findings,
no change in this task.

Gates: state-hash identity A/B (the supreme gate), two-update batch P50
delta published per phase, DevFast + Boundary green, censuses/fences/
reserve unchanged, fast iteration protocol from TASK 6 for measurement,
ledger row per phase incl. reverts. End: canonical rebuild, one-minute
natural match with per-phase updates/s + slip counts, snapshot with
New-Smash64DSSnapshot.ps1 -Mode Lean last.
```

---

## TASK 16 — Extended Task 9 Phase 2 (census-scoped)

```text
/task Extend TASK 9 Phase 2 only across the census leaders: fmul, fadd,
fsub, the fcmp family, and i2f. Every replacement must match the selected
GCC libgcc result bit-for-bit, live in ARM ITCM, retain a renamed literal
stock golden for proof, and pass the host/ARM9/SUPREME ladder. Keep fdiv,
f2iz, f2uiz, f2d, d2f, dmul, and ddiv in relocated stock libgcc. Do not
touch the renderer TU or decomp source, and keep each shipping selector
default OFF until integration chooses otherwise.
```

Closure (2026-07-18): compare, i2f, and add/sub are qualified and coexist in
one exact link. The fmul exact-zero specialization was decisively reverted
after correctness passed but both its representative ARM9 microbenchmark and
natural source-update timing regressed. The combined 0/0/0 versus 1/1/1
SUPREME gate has exactly 3,892 matching six-field rows per side with zero
overflow, and the synchronized natural A/B keeps the three accepted
candidates. `PERF_LEDGER.md` owns the measurements and artifact identities;
`verify-task16-combined-state-hash-ab.ps1` owns the repeatable combined gate.
The global selectors remain zero so the exact A/B controls stay available;
the published and release-equivalent freeze targets now override all three to
one. Canonical Boundary passed in mode 163 with 1/1/1, stock 408-byte fmul,
and ITCM 28,820/32,768 with 3,948 bytes free and zero fill. TASK 16 is
shipped; `P1_EXECUTION_BOARD.md` owns the current ROM identity, while the
ledger owns the publication measurement. Its authoritative combined gate
always rebuilds both sides and binds exact build, ROM, ELF, and full six-field
row identity.

## Sequencing and interactions

- TASK 1 before TASK 2, hard: the depth fix changes class-3 semantics and the
  painter bands that CUT 3-NEW must reproduce; its class census is TASK 2's
  Phase 0 input.
- TASK 3 is independent of TASK 1/2 surfaces except shared nds_renderer.c
  edits — one-writer rule; run it after TASK 2 or in a coordinated lane.
- The crowd-audio bake task (separate, already endorsed) threatens M4's
  >=128 KiB reserve: audio-adjusted reserve is 163,312 and the full 6.9 s cue
  adds ~38K. If TASK 2/3 qualification shows reserve below 131,072 after the
  audio bake lands, the audio task trims (bake the audible ~6.18 s), not M4.
- Against locked 30: present must reach <=~1,120K. TASK 2 fully landed is
  roughly -145K to -195K from draw; the remainder must come from TASK 3.
  Neither alone is sufficient.

---

# 2026-07-17 — Hardware-reality addendum (TASKS 10–12)

Context: the owner observed real-DS hardware running ≈0.75× melonDS on this build.
The multiplier is almost certainly per-resource, not uniform (ITCM ALU ~1.0×,
waitstated main RAM worse, GX FIFO stalls likely unmodeled in melonDS), so it
must be calibrated before re-ranking anything. Agreed strategy pivot: the big
remaining wins are (a) visual economy at DS resolution — 256×192 is exactly
0.8× N64 linear per axis; CPU draw cost scales with triangles, not pixels —and
(b) memory-system placement tuned for real hardware. Gameplay stays bit-exact;
the budget being spent is the 90% visual target, via measured per-owner pixel
ratchets. Budget lines: melonDS locked-30 needs the frame pair ≤ 1,120,380
ticks; provisional hardware-30 ≈ ≤840K melonDS-equivalent until TASK 10
replaces 0.75× with per-bucket multipliers. Current heavy-combat truth:
draw P50 ~1,149K, update owner ~260K (post TASK9-P1), loop 1,680K (3 vblanks).

## TASK 10 — hardware calibration + on-device phase-tick HUD

```
/task TASK 10 — Hardware calibration lab ROM + profile-1 phase-tick HUD.
Real DS hardware measures ≈0.75× melonDS on this build (the owner, 2026-07-17).
Before re-ranking any optimization we need per-resource multipliers and an
on-device measurement loop. Zero behavior change anywhere in this task.

PHASE A — phase-tick HUD (profile-1 only).
The sub-screen console and battle FPS HUD already exist: consoleInit on the
sub engine src/nds/nds_platform.c:274-276, row-positioned print helper
:1351-1355, half-second HUD sample window :1361-1390 with
NDS_BATTLE_FPS_HUD_SAMPLE_TICKS = BUS_CLOCK/2 (:47), tick source
cpuStartTiming(0)/cpuGetTiming() (:208) — the same tick currency as the
ledger. Add HUD rows, sampled off the hot path and formatted only inside
the existing HUD window, reusing the profile-1 phase accumulators that
already feed the perf artifacts:
  UPD (source-update owner), DRW (draw), ACT (active), LOOP,
  PRE/PRP/CMT (stage preflight/prepare/commit, if rows allow),
  FPS (presents/s), SLIP (delta of the locked-30 cadence counter
  gNdsBattlePlayablePacingCadenceViolationCount).
Raw decimal ticks, fixed rows, labels ≤4 chars — readable from a photo of
the device. Print the short git hash once at boot for provenance.
Constraints: compiled out of profile-0 entirely (production ROM
byte-identical); HUD cost inside the known profile-1 observer allowance;
no allocation or formatting on the frame hot path.

PHASE B — calibration lab ROM (standalone; no new harness-registry mode;
lab-only symbols absent from the production ELF per established lab
discipline). Boot straight into a fixed bench sequence, print label +
ticks per bench on the sub console; the identical ROM runs in melonDS and
on hardware. Benches, fixed work sizes, each ~0.5-1.0 s on hardware:
  ALU-ITCM  1,000,000-iteration dependent-add chain, ARM code in .itcm
            (reuse the attribute pattern at src/nds/nds_renderer.c:33).
  MEM-THMB  streaming u32 sum over a 256 KiB main-RAM buffer, Thumb.
  MEM-ARM   the identical C loop compiled target("arm"), main RAM.
  CACHE4K   the same loop over a 4 KiB buffer, pass count scaled to the
            same total load count (dcache-resident control).
  GX-BRST   fixed synthetic stream: 10,000 flat triangles as immediate
            GXFIFO writes with normal poly attrs, swap every 2,048;
            sustained ticks including FIFO-full stalls (GXSTAT).
  CARD      optional, informational: timed 512 KiB sequential card read
            if trivially available on the flashcart path.
End with a fixed-order summary table so one photo captures everything.

PHASE C — documentation with data. Record melonDS-side numbers for every
bench under the canonical harness config; record the melonDS version and
any timing/cache-accuracy options (if such an option exists, run the
bench under it too and record the delta). Add to docs/P1_EXECUTION_BOARD.md
a "Hardware reality (2026-07-17)" section: the bench table with melonDS
values filled and hardware cells left TBD for the owner's photo numbers, plus
the provisional rule "hardware-30 budget ≈ 840K melonDS-equivalent until
calibrated". Add a PERF_LEDGER.md methodology note defining per-bucket
hardware projection (CPU-ITCM / CPU-mainRAM / GX buckets × multipliers).

Gates: profile-0 ROM byte-identical; DevFast + Boundary green; fixtures
untouched; no decomp/gameplay/renderer-behavior edits; lab ROM not
registered as a harness mode. Deliver: lab ROM path + SHA-256, HUD ROM
path + SHA-256, and one-paragraph instructions for the owner (what to run,
what to photograph). Ledger row for the task. End: canonical rebuild,
snapshot New-Smash64DSSnapshot.ps1 -Mode Lean last.
```

## TASK 11 — screen-space census + background economy (stage only)

```
/task TASK 11 — Screen-space over-tessellation census + background economy.
Stage only; fighter LOD is a later task fed by this census. The DS top
screen is 256×192 = 49,152 px, exactly 0.8× N64 linear per axis: 1 DS px
= 1.25 N64 px, so near-sub-pixel N64 geometry is fully sub-pixel here.
CPU draw cost scales with triangles, not pixels. The capture harness's
pixel diff at 256×192 is the perceptual referee. Gameplay is untouched;
hitboxes are bone-anchored spheres in gameplay code and cannot be
affected by draw-side cuts.

PHASE A — census (measurement only; profile-1/lab; compiled out of
profile-0). At triangle submit time the pipeline already holds projected
coordinates. Accumulate per stage owner/DL (across the 66 raw, 126 no-Z
painter, and 10 range classes) and per fighter part: triangle count,
triangles with projected screen area <1 px² and <4 px², and area sum.
Integer/fixed-point math only — no new soft-float on the frame path.
Windows: canonical frames 600..607 plus one ~600-frame natural-match
window (realistic camera range). Export through the existing perf
artifact JSON path. Deliverable: a ranked over-tessellation table
(owner ticks × sub-pixel fraction) in the ledger row. Fighters appear in
the census for later LOD planning but are NOT cut in this task.

PHASE B — background economy behind NDS_RENDER_ECONOMY (new build flag,
default OFF; default builds byte-identical). From the census ranking, cut
the top background/far stage owners (expected: sky, clouds, trees, far
scenery on Dream Land) by whole-owner skip or a statically reduced
variant. Rules, per owner cut, each its own ledger ratchet row:
  - A/B ticks via the fast-iteration protocol: stage/draw/active P50,
    flag ON vs OFF.
  - Pixel tolerance gate at 256×192 on canonical frames: meaningful
    changed pixels ≤ 500/49,152 per owner cut, zero changes inside
    required regions, and every existing visibility / named-region /
    horizontal-detail / required-region gate stays green with the flag
    ON. Existing gates are one-way ratchets — none may be weakened.
  - Painter strict-order semantics for all remaining owners unchanged
    (contract per the grdisplay.c:52-141 / grpupupu.c:637-690 citations
    already in the ledger).
  - A failed owner reverts alone; keep the rest.
Pause rule: economy applies only during live gameplay. While the battle
is paused, render the full owner set — pause is frame-frozen with no perf
budget, and it is the only time the player can inspect closely. Bind to
the real pause/process-pause state; select per frame, never per triangle.

Gates: DevFast + Boundary green with the flag OFF (default) AND an
economy smoke run with the flag ON; fixtures green; no decomp edits; no
gameplay reads/writes changed; per-owner ledger rows with tick deltas,
pixel numbers, and capture evidence paths. Pool context: stage P50 is
470,784 post CUT-E. End: canonical rebuild, snapshot -Mode Lean last.
```

## TASK 12 — renderer Thumb conversion + hot/cold text grouping

```
/task TASK 12 — Renderer main-RAM Thumb conversion + hot/cold grouping.
Pure codegen/placement task: zero behavior change; canonical frames stay
pixel-exact 0/49,152.

Facts: global ARCH is -mthumb (Makefile:185) but nds_renderer.o adds
-marm (Makefile:933; bench echo mirror at :1006). So all non-ITCM
renderer code executes ARM opcodes from waitstated 16-bit main RAM — two
fetches per instruction on every icache miss — while the ITCM macros
already force ARM + placement via __attribute__((hot, optimize("O3"),
target("arm"), section(".itcm"))) (src/nds/nds_renderer.c:33, :38).
melonDS may under-model the 8 KiB icache; TASK 9 Phase 1 proved main-RAM
placement is heavily charged. Real hardware is the primary referee here,
via the TASK 10 HUD.

PHASE A — Thumb conversion.
1. Audit first: objdump every symbol in .itcm* — each must be ARM-state
   and reached via the annotated macros. Any bare section(".itcm")
   function missing target("arm") gets the full macro BEFORE the flag
   change (silently becoming Thumb-in-ITCM would waste the 32-bit bus).
2. Remove -marm from nds_renderer.o CFLAGS (Makefile:933) and mirror the
   bench echo at :1006 so benchmarks build identically. armv5te has blx;
   interworking veneers are automatic — fix any fallout, expect none.
3. Verify: objdump state audit (main-RAM renderer symbols Thumb, .itcm
   ARM); ITCM bytes unchanged (28,052/32,768 baseline); report the
   main-RAM renderer text size delta (expect roughly −25-30%).
4. Exactness: GBI fixtures, parity corpus, canonical frames raw delta
   0/49,152, DevFast + Boundary green.
5. Measure: melonDS A/B (report even if a wash) AND hardware A/B — build
   a control/candidate profile-1 ROM pair carrying the TASK 10 HUD and
   hand the owner one-paragraph instructions (which ROM, which HUD rows to
   photograph). KEEP rule for this task (new precedent, memory-system
   changes only): hardware numbers are primary; a melonDS wash does not
   veto a hardware win ≥1% of loop; a melonDS regression >1% alongside a
   hardware win → REWORK, understand it before banking. Record both
   columns in the ledger; mark hardware cells PENDING-HW if the owner's
   device run lands later — never guess them.

PHASE B — hot/cold grouping (after Phase A lands).
1. Add an NDS_HOT_TEXT(nn) macro → section(".text.hot.nn"). Annotate the
   hottest main-RAM-resident per-frame functions chosen from ledger owner
   attribution + map-file sizes; total hot region ≤ 8 KiB (icache size);
   never move ITCM functions.
2. Placement: supplementary INSERT-only linker script —
     SECTIONS { .text.hot : { *(SORT_BY_NAME(.text.hot.*)) } }
     INSERT BEFORE .text;
   passed via -T alongside the default script if the toolchain accepts
   augmentation; otherwise fork the default ds_arm9 script into the repo
   with the one added line and a provenance comment. Verify hot-region
   contiguity and ordering in the -Wl,-Map output.
3. Same exactness gates and the same hardware-primary A/B protocol as
   Phase A.

Hard rules: no decomp edits, no gameplay edits, no DTCM changes, no GX
packet/semantic changes; one-writer — this task owns only the Makefile
lines, the linker-script addition, and attribute/macro annotations in
the renderer TU. Ledger row per phase with both melonDS and hardware
numbers. End: canonical rebuild, snapshot -Mode Lean last.
```

## Sequencing (TASKS 10–12)

- Relay order: TASK 10 first (small; establishes the device measurement
  loop), then TASK 11 (independent, melonDS-measurable), then TASK 12
  (its keep decision consumes TASK 10's HUD on hardware).
- TASK 8 CUT A and TASK 9 Phase 2 remain staged and are not preempted.
  TASK 11/12 touch the renderer TU — one-writer: do not run them in the
  same codex lane as TASK 8's renderer cuts.
- Demo window: TASK 10 and 11 are bankable before 7/19; TASK 12's
  hardware referee depends on the owner's device availability.
- The decimation-pack task (fighter + stage-solid LOD, the biggest
  visual-economy payoff) is deliberately NOT written yet: its targets
  come straight from TASK 11's census table.

---

# 2026-07-18 — post-calibration additions

TASK 10 hardware numbers are in (board table now filled): ITCM ×1.03,
streaming main-RAM ×1.50, cache-resident ×0.88 (hardware faster; melonDS has
no dcache model), MEM-ARM ×1.20 with the ARM/Thumb ranking inverted vs
emulator, GX ×0.87 (hardware faster). TASK 11's census aims decimation at
fighters: natural-window <4 px² fractions are 76.99% (Mario) / 77.63% (Fox)
against a combined fighter draw P50 of 385,312/pair; stage layers are not
sub-pixel and stage economy is closed at owner 5 (−7.1K).

## TASK 13 — fighter decimation pack (derived-asset LOD)

```
/task TASK 13 — Fighter decimation pack: tool-derived LOD meshes for Mario
and Fox part meshes, substituted at draw time under NDS_RENDER_ECONOMY.
Census facts this task is built on (TASK11-SCREEN-CENSUS ledger row):
natural frames 438..1037 put 76.99% of Mario's and 77.63% of Fox's
triangles under 4 px² on the 256×192 screen; combined fighter draw P50 is
385,312 ticks/pair. Fighter meshes are N64-era segmented RIGID parts (one
DL per joint segment, no skinning), and hitboxes are bone-anchored spheres
in gameplay code — visual decimation cannot affect collision or feel.
Calibration note: fighter prep is main-RAM CPU work, so every melonDS tick
saved here is worth ~×1.2-1.5 on retail hardware (board, Hardware reality).

PHASE 0 — extraction + tool. Host-side deterministic tool (Python, same
derived-asset pattern as assets/audio/fgm_phase_pack_ima.bin + .json):
read each fighter part's source Vtx/tri payload via the existing decode
machinery, run quadric-error edge collapse PER PART with hard constraints:
  - joint-boundary ring vertices preserved bit-exact (no seam cracks);
  - collapses only within a single texture/material run — never across
    a texture switch; winding preserved; degenerate output forbidden;
  - vertex colors/UVs recomputed only for interior vertices;
  - deterministic: pinned ordering, no RNG; same input → same output.
Emit pack + manifest: per part, source payload SHA-256, original and
decimated tri/vtx counts, achieved ratio. Target ratio per part from the
census: aggressive where <4 px² fraction is high; parts may individually
ship at 1.0 (no cut) if gates fail. Tool + pack + regeneration script
checked in; manifest SHA pinned in the ledger row.

PHASE 1 — runtime substitution. Under NDS_RENDER_ECONOMY (default OFF),
the prepared/AOT fighter path substitutes decimated part payloads keyed
by source payload hash; any key miss FAILS OPEN to the source mesh.
Close-up guard: LOD applies only while the fighter's projected scale is
below a threshold — KO/rebirth camera zoom, magnify, and Pause render
source meshes (reuse the per-present game_status sampling TASK 11 added,
plus a projected-scale check from data the pipeline already computes; the
KO window frames 708..715 baseline must show source meshes). Default-OFF
rebuild must remain byte-identical to the frozen control ROM, same
discipline as TASK 11.

PHASE 2 — gates, per part (each its own ratchet row; failed part ships
full-res, rest keep):
  - Pixel gate at 256×192: idle frames 600..607 AND ≥8 natural-window
    captures spanning 438..1037 (attack/jump/hit poses). Per capture:
    meaningful changed pixels ≤500/49,152 per fighter, zero changes in
    required regions, all existing visibility/named-region/detail gates
    green with the flag ON. Flag-OFF gates untouched.
  - Counter contract: flag-ON smoke uses expected triangle/owner counts
    DERIVED FROM THE MANIFEST (e.g. 828-triangle partition adjusted by
    the pack totals); flag-OFF expectations are not weakened.
  - Ticks: combined fighter / draw / active P50 A/B via the fast-iteration
    protocol, flag ON vs OFF; report melonDS values and the ×1.2-1.5
    hardware-weighted projection. Optional: profile-1 HUD ROM pair for a
    retail-DS spot check.
  - the owner feel/eyeball gate: flag-ON ROM plays a natural match; visual
    sign-off before default-ON is even discussed (not in this task).

Hard rules: no decomp edits; no gameplay reads/writes; stage meshes are
OUT OF SCOPE (census shows stage layers are not sub-pixel; stage economy
is closed); one-writer — renderer TU edits limited to the loader hookup,
and this task must NOT run concurrently with TASK 12's renderer changes.
Ledger row per part plus a pack-summary row. End: canonical rebuild,
snapshot New-Smash64DSSnapshot.ps1 -Mode Lean last.
```

## Sequencing amendment (2026-07-18)

- TASK 13 relays only after TASK 12 lands (shared renderer TU, one-writer).
- TASK 12 steer per calibration: melonDS will overvalue blanket Thumb (it
  charges ARM main-RAM fetches continuously) and cannot see icache/dcache
  wins at all — the hardware-primary keep rule is not optional there. Hot
  icache-resident functions prefer ARM: if blanket Thumb regresses on
  hardware, retry with hot functions pinned ARM via the existing
  target("arm") attribute before abandoning Phase A.
- GX FIFO DMA feeding is DEPRIORITIZED (GX measured ×0.87 — hardware is
  faster than emulator at transport; no stall tax to recover).

---

# 2026-07-18 — overnight queue (TASKS 14–18)

## TASK 14 — revive prepare-once under the accumulation policy

```
/task TASK 14 — Dense-preparation reuse, revived. AUTHORIZATION: the ledger
row M3-DENSE-PREPARE-ONCE (2026-07-15) says "Do not retry or widen
dense-only preparation reuse." That instruction is SUPERSEDED by the owner's
2026-07-16 policy override recorded at the head of this document: the
threshold gates (>=164,544 required saving, <=500K stage) are withdrawn
and every repeatable correctness-preserving gain is kept and accumulated.
The 7/15 experiment was exactness-clean — counters 8/57/42/54/202 exact,
owner 121/828, M4 22/131072, zero fallback/fence faults, screenshot
correct — and saved 108,960/108,864 P50/P95. It failed ONLY the withdrawn
threshold. Re-land that class of win under current policy and the CUT E
fail-closed pattern. This is main-RAM CPU work: melonDS savings count
x1.2-1.5 on retail hardware (board, Hardware reality).

PHASE 0 — re-baseline. The 7/15 numbers predate CUT E and the other
banked cuts (stage was 664K then; stage P50 is 470,784 now, prepare-runs
213,408/213,504 per the TASK8-CUT-E row). Re-measure the prepare bucket
composition on current source before changing anything; expect the
recoverable pool at −60K..−110K. Bank whatever is clean — no minimum.

PHASE 1 — generation-gated prepared-run reuse, on the TASK8-CUT-E
template: cache the dense/prepared run state that is immutable for static
stage topology; keep matrices, materials, colors, texture selection,
alpha, and near-plane work live every frame (same live/immutable split
CUT E proved for preflight). Any generation/stamp mismatch fails closed
through the original full preparation before any GX mutation.

PHASE 2 — fail-closed lab: one-shot lab mutation flips the cached stamp;
publish the full/hit/mismatch/inject/revalidate census exactly like the
CUT E row; lab-only symbols absent from the production ELF.

Gates: canonical frames raw delta 0/49,152; owner/partition counters
exact at current values; GBI fixtures + parity corpus; DevFast + Boundary
green; fast-iteration protocol for A/B (stage/draw/active P50); ledger
row either way. Hard rules: no decomp edits; no gameplay reads; renderer
TU lane — must NOT run concurrently with other renderer-TU tasks.
End: canonical rebuild, snapshot New-Smash64DSSnapshot.ps1 -Mode Lean.
```

Result (2026-07-18): **KEEP**. The current-source rebaseline is
904,928/905,088 stage ticks. A generation-gated 55-offset/312-index immutable
first-visit plan removes the 606-corner per-frame mask walk while every live
matrix/material/texture/alpha/color/UV/near-plane operation stays per-frame.
Stage becomes 895,872/896,000 (-9,056/-9,088); draw and active improve
-9,280/-8,704 and -9,248/-8,704. All eight paired frames win, the native
top-screen delta is 0/49,152, the one-shot fault lab is exactly
full2/hit436..443/mismatch1/inject1/revalidate1, and production is
full1/hit437..444 with no fault symbols. Cost is 736 B BSS, 568 B text, zero
ITCM, and 40 B less hit-path stack. The new host/static proofs pin all 55
offsets, the exact 312-ID permutation, uniform source alpha per run, unique-bit
construction, terminal count, and fail-closed cache publication order.

## TASK 15 — CUT A: no-Z painter class onto the GX

```
/task TASK 15 — Offload the no-Z painter class to hardware transform
(TASK 8's carried CUT A, now standalone). Pool per TASK6-R0 Phase 0:
no-Z inclusive 98,816 ticks (matrix share 50,432); re-measure first on
current source. Calibration bonus: GX transport measured x0.87 on retail
hardware — CPU→GX shifts are worth MORE on-device than melonDS shows.

IDEA: the 126-owner no-Z painter class computes vertex transforms on the
CPU so it can pack the synthetic monotonic painter-band depth into the
v16 Z channel. Move it to the GX matrix path with the Z row replaced by
band_constant × W row: hardware then emits z/w = band_constant per
polygon — the exact same band value the CPU path packs — while the
hardware does the XY transform work. Painter semantics are unchanged
because the band constant per run is unchanged.

REQUIREMENTS:
1. Reproduce the depth-band contract exactly: bg 4095..4024 / fg
   −3969..−4022 bands with real-Z 3605..3728 between (strict-order note;
   BattleShip grdisplay.c:52-141 callback order, grpupupu.c:637-690 owner
   map). Verify band values A/B with the existing
   ndsRendererStageDepthTraceEvent instrumentation before/after.
2. Do NOT regress the banked TASK 1/1B depth and translucency work:
   translucent poly-ID cycling and the grass/bush behavior must be
   pixel-identical on the canonical frames and the depth-probe scenes.
3. Respect GX resource ceilings: report poly/vertex RAM headroom before
   and after (2048 poly / 6144 vtx hardware limits; current partition is
   828 stage+fighter triangles — ample, but publish the numbers).
4. Fail closed: if any no-Z owner cannot express its band through the
   matrix Z-row form (unexpected vertex-level depth variation), leave
   that owner on the CPU path and report it — partial offload is a valid
   KEEP; per-owner accounting in the ledger row.

Gates: canonical frames raw delta 0/49,152; stage depth trace bands
identical; owner/partition counters exact; fixtures + parity corpus;
DevFast + Boundary; A/B stage/draw/active P50 via fast-iteration
protocol; ledger row per outcome including per-owner offload census.
Hard rules: no decomp edits; renderer TU lane — not concurrent with
other renderer-TU tasks. End: canonical rebuild, snapshot -Mode Lean.
```

## TASK 16 — GO for TASK 9 Phase 2 (census-scoped)

```
/task TASK 16 — Execute TASK 9 PHASE 2 as specified in this document
(optimized ARM-mode IEEE-754-single routines in ITCM, bit-exact, full
proof ladder), with the census-informed scope from the
TASK9-LIBGCC-FLOAT-ITCM ledger row:
  IMPLEMENT: fmul (1,172.4 calls/update), fadd (821.1), fsub (576.4),
  the fcmp family (~896 combined; medians 40-48 ticks/call — the largest
  relative win: an ARM-state compare is ~10-15 instructions), i2f (174.4).
  KEEP LIBGCC (already in ITCM from Phase 1): fdiv, f2iz, f2uiz, f2d,
  d2f, dmul, ddiv — together <2% of float ticks; not worth proof surface.
The proof ladder and SUPREME GATE are unchanged from the TASK 9 block:
host exhaustive 2^32 for unary, directed + >=10^8 random vectors for
binary against the libgcc golden, then the deterministic scripted match
with per-update full game-state hash A/B vs the baseline ROM — any
single-bit divergence at any update = revert, no exceptions. The
nds_task9_state_hash machinery from Phase 1 is already in the tree.
ITCM budget: replacing the six Phase-1 libgcc objects (1,952 bytes)
frees their space; optimized routines must fit within that plus current
free bytes (baseline 28,052/32,768). Report exact before/after bytes; do
not evict renderer ITCM. Expected yield: −20..−35K per update
(−40..−70K per pair) on top of Phase 1's −51.5K. Not a renderer-TU task;
surfaces are the new routine objects, link order, and lab shims only.
Gates and end sequence exactly as the TASK 9 block specifies.
```

## TASK 17 — hot-text grouping round 2: the update path

```
/task TASK 17 — Extend TASK 12's hot-text mechanism to the update path.
PREREQUISITE: TASK 12 Phase B (NDS_HOT_TEXT macro + .text.hot linker
placement) must be landed; this task reuses its mechanism and its
hardware-primary keep rule. melonDS cannot see cache-locality wins at
all (board: no icache/dcache model), so device photos are the referee.

MECHANISM FOR DECOMP-COMPILED CODE (no source edits — decomp/ is
read-only): -ffunction-sections is already global (Makefile:213), so
every function has its own .text.<name> section. Place gameplay/library
hot functions BY SECTION NAME in the supplementary linker script's hot
region — zero source changes. Port-side functions may use NDS_HOT_TEXT
directly.

SELECTION: from the TASK 9 Phase 0 census callers (scVSBattleFuncUpdate
scope) and per-owner update attribution, list the hottest main-RAM
update-path functions with map-file sizes. Combined .text.hot (renderer
set from TASK 12 + this update set) target <=12 KiB with the hottest
<=8 KiB placed first — the ARM946E-S icache is 8 KiB, 4-way; publish the
final region size and ordering from the map.

MEASURE: melonDS A/B (expect a wash — report it anyway) + hardware A/B:
control/candidate profile-1 HUD ROM pair for the owner's device, one
paragraph of instructions, UPD row is the number that decides. PENDING-HW
ledger cells until the photos exist — never guessed. KEEP on hardware
win; REVERT on hardware wash (dead weight in the linker script helps
nobody).

Hard rules: no decomp edits; no behavior change; linker-script surface
shared with TASK 12 — run only after TASK 12 is fully landed; not
concurrent with any renderer-TU task if it also annotates renderer
functions. Ledger row with both columns. End: canonical rebuild,
snapshot -Mode Lean.
```

## TASK 18 — KO-window wallpaper re-render spike

```
/task TASK 18 — Diagnose and bound the KO-window wallpaper spike.
FACT (NATURAL-KO-REBIRTH-PHASE-20260717 ledger row): during KO frames
708..715, wallpaper costs 547,584/547,648 P50/P95 — nearly half of the
1,256,544 KO-window draw — while steady-state wallpaper is retained and
cheap. Hypothesis: the KO camera zoom invalidates the retained wallpaper
every frame, forcing a full re-render exactly when the camera moment is
most visible. An affine wallpaper path already exists
(gNdsSceneWallpaperAffineLastTicks, src/nds/nds_platform.c:193 and
:942-968).

PHASE 0 — diagnose (profile-1 counters, no behavior change): during the
KO window record per frame: invalidation cause (scale change? scroll?
palette?), full vs partial re-render, pixels/tiles re-rendered, and
whether the affine path is engaged. Publish an 8-frame trace in the
ledger row. Do not guess the cause — name it from the counters.

PHASE 1 — bounded fix, chosen from the Phase 0 evidence, preference
order:
  a. EXACT: render the wallpaper source once per zoom sequence and let
     the 2D affine hardware scale it per frame (the affine path exists;
     if zoom currently re-renders the source instead of reusing the
     affine matrix, this is a pure win with pixel-identical output —
     gate: KO-window captures raw delta 0/49,152).
  b. If (a) is structurally unavailable: reduced re-render cadence
     during zoom with affine interpolation between renders. This changes
     intermediate-frame pixels: gate becomes named KO-window captures
     with meaningful delta <=500/49,152 per frame plus the owner's explicit
     eyeball sign-off on the KO moment — flag-gated, default OFF until
     his sign-off.
  Steady-state behavior byte-identical either way; only the zoom-window
  path may change. No gameplay reads; camera values are consumed, never
  written.

Gates: steady-state canonical frames raw delta 0/49,152 (unchanged
path); KO-window gate per the chosen branch above; DevFast + Boundary;
A/B of the KO window (draw/wallpaper P50) + one full natural KO
sequence; ledger row. Surface: wallpaper/platform path — NOT the
renderer stage TU; may interleave with the renderer lane. End: canonical
rebuild, snapshot -Mode Lean.
```

## Overnight relay order (2026-07-18)

Single codex lane, after in-flight TASK 12 completes:
  13 → 14 → 16 → 15 → 17 → 18
Rationale: 13 and 14 are the two biggest pools; 16 is bit-exact and
independent of the renderer TU (safe mid-queue breather with its own
supreme gate); 15 after 14 because both are renderer-TU and 15 carries
more semantic risk (depth bands); 17 strictly after 12 (shared linker
mechanism, hardware-primary); 18 anytime — it is surface-disjoint and
can also slot earlier if a renderer task blocks.
Two lanes, if run: lane A (renderer TU) 13 → 14 → 15; lane B
(non-renderer) 16 → 18, then 17 after 12 lands. One-writer holds per
lane; nothing in lane B touches the renderer TU.
Morning deliverables to the owner: TASK 12/17 HUD ROM pairs for device
photos; TASK 13 flag-ON ROM for the feel/eyeball gate; TASK 18 KO
captures if branch (b) was taken.
