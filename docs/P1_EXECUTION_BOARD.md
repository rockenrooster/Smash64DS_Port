# P1 Execution Board

Updated: 2026-07-27 22:45 Central

Boundary: `battle_playable_realtime`, mode `163`

This is the only dynamic P1 queue. `PROJECT_GOAL.md` owns the milestone and
fidelity contract. `HANDOFF.md` owns restart commands, `KNOWN_ISSUES.md` owns
durable gaps, `PERF_LEDGER.md` owns measurements and rejected experiments, and
`PORTING.md` is append-only history.

## Artifact Identity

Pinned public-build identity from `README.md`:

```text
smash64ds-battle-playable-hwtri.nds
11,428,864 bytes
SHA-256 4D795B4E83B335598B20A3B5953FDB1821797CC5E0A825FA96A0643ABBA4A090
```

Current local root artifact:

```text
smash64ds-battle-playable-hwtri.nds
11,421,696 bytes
SHA-256 A9ED45BC5DEF9DE71E00850E83DEB34AE46F4CB9B2CE19113E0548273C56F574
```

The worktree is dirty, so the local identity is informational only. It is not a
release candidate until the relevant verifier passes and the public-build pin is
updated in the same kept change.

## Bug #10 — closed and folded in (2026-07-28)

`06992f10812` "Fix Mario pelvis texture clamp", cherry-picked from `2cbc6189d15`
on `codex/fix-mario-bottom-rendering` onto the R2 branch so authorship is
preserved. Epoch 0 loads a 32x24 CI4 source into a 32x32 DS texture; its N64 T
axis is CLAMP with mask 5, so coordinates 24..31 resolve to row 23, while the DS
sampler wrapped through the eight zero-padded transparent rows — the aperture
was *inside* textured pelvis triangles, not at a geometry or culling seam, which
is why five earlier causes were eliminated. One line in
`ndsRendererHardwareTextureMaskedClampNeedsWrap` disables wrap when the logical
clamp edge is at or before the mask period.

It arrives with its own gates rather than needing new ones: a host fixture for
the exact 32x24 case, a structural pin in `check-gbi-decode-fixtures.ps1` so the
line cannot be silently reverted, the `pause_under20` camera oracle, and the
controller-playback DTCM move that oracle needs in order to write pads over GDB.
The DTCM layout checker was not relaxed to accommodate it — every Calico
boundary assertion survives, parameterised by the new 32 bytes, with added
all-or-none and per-symbol address/size/alignment pins.

Folding it in did surface a real harness defect, fixed in the same cycle.
Boundary failed twice on the locked-30 pacing gate reading
`logic/present = 422/212` with a phase histogram summing to 211. The ROM was
right: taskman's own counter and the fighter route both read 424 updates for 212
presents, an exact 2:1. Two terms compared counters incremented at *different*
instructions of one iteration, so they were asserting where the debugger stopped.
Both are now a four-state stop-phase model that rejects five of the eight sign
combinations — strictly stronger than the equality it replaced — with taskman's
independent counter disambiguating the one aliasing pair. E8 did not create the
window; it changed where in the frame the stop lands. Full derivation in
`docs/optimization/TASK_STANDING_RULES.md`.

The opt-in Task 25R trace carried the third instance of the same defect and is
now fixed too (`6221406`). Its rows all come from one fixed marker, so the skew
is constant and the contract is *stronger* than the four-state model: take the
skew from the first row, require it reachable, require every later row to agree
— a dropped or doubled update then disagrees with its neighbours instead of
hiding inside a tolerance. The final reconcile runs the BPLAY_PACE snapshot
through `Test-BattlePlayablePacingStopPhase` rather than a logic-only bound.
Eight synthetic cases cover it with no ROM or emulator; the registry pins the
new contract and bans the old equality.

## R2-03 gate MISSED 2.00x, and the 56% nobody had measured (E13/E14, 2026-07-28)

The owner observation below is now measured, and it turned over the phase.

**The gate.** Fighter draw, both fighters, bracketed on the tick-HUD ROM over 479
frames: **501,624 ticks/frame against §7's 250,000 for the pair.** Over by a
factor of 2.00. Mario alone measures 237,219 per draw call; either fighter on his
own very nearly exhausts the budget written for both. That budget was set in
R2-00b without a measured per-fighter cost.

R2-03 has shipped -47,486 (E9+E10, E12) against a 250,833 gap — 19% of it.

**Where the draw actually goes** (per frame, both fighters):

| phase | ticks | share |
|---|---:|---:|
| Walk / Validate / Reset | 20,595 | 4.1% |
| OwnerPrep (matrices + materials) | 143,684 | 28.6% |
| Build production inputs | 37,292 | 7.4% |
| **`...ExecuteNativeFighterOwnerProduction`** | **279,617** | **55.7%** |
| tail | 20,436 | 4.1% |

Both shipped cuts landed in the 28.6%. The 55.7% had never been bracketed, so it
was never a candidate — E3's split stopped at the point the owner inputs are
built, and everything past it went into one unnamed remainder.

**The 3D hardware is idle.** `GXSTAT` sampled either side of 946 fighter
submissions: FIFO entries 0 entering, 0 leaving, max 0, geometry engine busy on
0 of 946. Positive control passes (OR of raw words `0x06009F00`, bit 26 =
FIFO-empty set, so the register is live and the zeros are real).

**The ARM9 is the whole cost.** Cutting fighter polygons is the *wrong* lever: it
spends visual fidelity to work around a CPU failing to feed hardware that has
headroom. `PROJECT_GOAL.md` permits the trade; this says we have not earned it.

**E15 corrects what comes next.** E14 read "447 ticks per hardware triangle" off
this bracket and recommended a captured command stream on R2-02 E2's precedent.
That statistic divided an inclusive bracket by the wrong denominator — most of
the bracket is not per-triangle work. **The emit is ~99 ticks/triangle and 20% of
the execute**, so a DMA'd stream caps out near 62,693 against a 250,833 gap. The
recommendation is withdrawn; see the E15 section below for the real ranking.

Full write-ups: `docs/optimization/ClaudeOpus5_R203_E13_FighterPriceAndGate_20260728.md`,
`..._R203_E14_SubmitSplitAndGxIdle_20260728.md`.

## R2-03 E15 — the fighter is a per-epoch machine (2026-07-28)

The execute partitions completely. Per frame, both fighters (instrumented build;
brackets cost ~31,165/frame, so absolutes are inflated ~10-20% and the ranking is
the finding):

| phase | ticks/frame | share |
|---|---:|---:|
| Preflight | 3,247 | 1.0% |
| Per-root: bind, composed matrix, `glStoreMatrix`, light preamble | 40,785 | 13.1% |
| Per-epoch: two state spans + material | 52,065 | 16.8% |
| **Per-epoch: shade actions** | **86,207** | **27.7%** |
| Run prepare | 42,520 | 13.7% |
| Raw emit | 56,873 | 18.3% |
| Cross emit | 5,820 | 1.9% |
| residual | 18,487 | 5.9% |

**48.5 epochs and 66.2 runs per frame, averaging 12.7 triangles per epoch.** Each
epoch pays ~2,850 ticks of state and shade *before a triangle is emitted*, against
~1,255 of prepare-and-emit. **~70% of the execute is per-epoch and per-root setup;
20% is geometry.**

Ranked leverage:

1. **Shade actions, 86,207.** E1 refuted memoising it *across frames* and that
   stands — but E1 never asked whether the shade recomputes **per epoch** what is
   constant **per root**, which a cross-frame memo cannot see. 48.5 epochs against
   ~28 roots is the shape that makes it worth asking.
2. **Epoch state spans, 52,065.** R2-02 F found adjacent-run redundancy in the
   stage's spans; the fighter's have never been checked.
3. **Per-root 40,785** over ~28 roots — contains the GX matrix load and
   `glStoreMatrix`. Whether every root needs its own palette store is unasked.
4. Run prepare 42,520 — already cut by E12, diminishing.
5. Emit 62,693 — ordinary, and the least promising per unit of risk.

Write-up: `docs/optimization/ClaudeOpus5_R203_E15_ExecuteSplit_20260728.md`.

## R2-03 E16 — the shade pass IS the DS's hardware lighting (2026-07-28)

Premise proven without exception, and it is the largest cut identified in R2-03.

`ndsRendererHardwareLitShadeColorPrepared` computes, per vertex,
`ambient + diffuse * dot(normal, light_dir) / 127` — with `light_color_1` as
diffuse, `light_color_2` as ambient, and the `rgba` field of the dense vertex
holding the **normal** (F3DEX packs normals there for lit vertices). That is,
term for term, the Nintendo DS geometry engine's hardware lighting equation.

Measured over 479 frames, both fighters:

| counter | per frame |
|---|---:|
| **lit epochs** | **48.5** |
| **unlit epochs** | **0** |
| epochs on the LUT path | 48.5 (100%) |
| epochs applying a material | 27.7 (57%) |
| **vertices lit** | **513.1** |
| vertices copied from a shared source | 21.5 |

**Not one fighter epoch in a match is unlit**, at ~169 ticks per shaded vertex.

**Why E1's refutation is explained rather than worked around.** E1 found the
shade output changes on 1,796 of 1,835 frames. It does: the light direction is
transformed into each root's local space by that root's modelview, the fighter
animates, so every dot product changes every frame. It is unmemoisable for a
structural reason — and that is exactly the problem DS hardware solves, by
setting the light vector once in view space and applying the current matrix per
vertex in silicon.

**`GFX_LIGHT_VECTOR`, `GFX_LIGHT_COLOR`, `glLight` and `POLY_FORMAT_LIGHT` appear
nowhere in `src/nds` or `src/port`.** The renderer has never used DS hardware
lighting, while E14 measured the geometry engine idle on 946 of 946 fighter
submissions.

Design: pack normals into `GFX_NORMAL` words at load time; set light and material
per root (~28/frame) instead of per vertex (~534/frame), folding `color_modulate`
into the material; emit the precomputed normal word instead of the computed
colour word — **one FIFO word either way, traffic unchanged**. Expected: most of
90,295 ticks/frame.

**Prerequisite the design does not survive without.** The fighter loads an
identity projection plus the **CPU-composed MVP** as the modelview, through
`ndsRendererHardwareSetMatrixMode(GL_MODELVIEW)` — mode 1, position only, which
**never updates the vector matrix**. Normals are transformed by the vector
matrix, so a naive `GFX_NORMAL` would light against whatever was left there, and
loading the composed MVP into it instead is equally wrong because normals must
not be rotated by the projection.

Fix: load projection into `GL_PROJECTION` and modelview into
`GL_MODELVIEW_VECTOR` (mode 2), and **delete
`ndsRendererAdapterComposeNativeRootMatrix`** — the hardware performs that
multiply. The adapter already carries both matrices separately, so this is a
removal, and it takes a 4x4 multiply per root out of the 120,407 MatrixPrep
bracket as a side effect. The light vector is then written once per frame in view
space while the vector matrix is identity.

**NOT IMPLEMENTED.** It touches the matrix mode, the load-time table format, the
emit's per-vertex word, and per-root light/material state. Being a rendering-side
change it gates on a screenshot pair plus **the owner's visual approval**: the DS
light model is not bit-identical to the N64's and colours will shift slightly.
`PROJECT_GOAL.md` lists "simplified lighting" among the allowed compromises, but
the call is the owner's.

Write-up: `docs/optimization/ClaudeOpus5_R203_E16_ShadeIsHardwareLighting_20260728.md`.

**Next: implement E16 behind a flag, capture the A/B screenshot pair, and put it
in front of the owner.**

### Open, not chased: GXSTAT bit 15 is set

Matrix stack overflow/underflow error latched at least once during a normal
match. It is a sticky flag and may date from init or teardown rather than
gameplay, and nothing observable is wrong. Recorded because it is an error bit
that is on.

## One fighter is worth ~400,000 ticks (owner observation, 2026-07-28)

**Superseded by the section above — measured at 271,424 WORK P50, not ~400,000.
Kept for the reasoning trail.** The inference below was sound but read the
quantization boundary as the whole cost; the actual transition needed less than
the boundary implied because `WAIT` absorbed part of it.


The owner noticed that knocking Fox off-screen, so he stops rendering, takes the
build to **~29 FPS** from ~20. That is not a small effect and it is arithmetically
informative.

The frame is VBlank-quantized at 560,190 ticks. Wall is **1,531,768** = 2.73
VBlanks, which rounds up to 3 → 20 FPS. Landing on 2 VBlanks needs wall
**≤ 1,120,380**, a saving of **~411,000**. Removing one fighter produced exactly
that transition, so **one fighter costs on the order of 400,000 ticks/frame** —
render, pose, matrices and everything downstream.

Two consequences.

**`PROJECT_GOAL.md` §7's budget table looks mis-proportioned.** It allots 250,000
to *combined* fighter rendering and 100,000 to fighter pose. Two fighters at
~400,000 each is ~800,000 of a 1,120,000 frame. Either the budget or the
implementation is wrong by a factor of two, and the budget was never validated
against a measured per-fighter cost.

**It is also a free instrument.** Suppressing one fighter's draw is a controlled
A/B that the tick-HUD reads directly, and it partitions the per-fighter cost into
render versus pose versus matrix without any new probe. Queued as the next
measurement after the R2-03 gate, reporting the 2/3/4/5+ VBlank histogram and max
interval per `AGENTS.md` — never min FPS.

Recorded as an owner observation, not a measurement: the FPS figure is a HUD
reading, and the ~411,000 is inferred from the quantization boundary rather than
bracketed.

**Outcome (E13).** Built as `NDS_R2_DRAW_SUPPRESS_MASK` and run. The observation
reproduces exactly — the median frame moves from three VBlank intervals to two,
and the 2-interval share goes 217/566 to 458/566 (histogram `2:458 3:102 4:5
5+:1`, max 17). The cost is **271,424 WORK P50**, not ~400,000.

The frame is also **CPU-bound, and this pair is what proves it**: `WAIT` went
*up* when Fox stopped drawing, 246,720 to 271,232. A rasterizer-bound frame that
loses a quarter of its pixel load waits less; a CPU-bound frame that loses
271,424 ticks of ARM9 work finishes earlier and waits longer.

## R2-03 E11/E12 — the fighter had no key for the cache that already existed

`ClaudeOpus5_R203_E11_PrepareRunSplit_20260728.md`,
`ClaudeOpus5_R203_E12_RunTextureMemo_20260728.md`.

**`PrepareProductionRun` 82,042 → 49,318 ticks/frame; the texture prepare inside
it 45,952 → 12,362.** Graduated to the published block.

E5 proved this function is a pure function of `run_index` and then declined to
build the memo because "~119 UV writes/frame can't explain 21,504 ticks". The
arithmetic was right and the premise was wrong: **a census row is self time.**
E5's bracket read ~21,500, the frame census row read 22,205, and four brackets
inside the function read **82,042** — the difference being the texture resolver
it calls out to, which the census charges separately to
`ResolveOrBindTexture` (18,803) and `SyncTextureTile` (12,004).

The cut is not a new mechanism. The resolver already opens with a site cache
keyed on `state->source_command_site`; the native fighter path does not
interpret display lists, so it has no site and has **never once hit that cache**.
The memo is the same cache re-keyed on `run_index`. R2-05 gets it for free.

| counter | value |
|---|---|
| memo hits | 1,074 (8.4/frame — every textured call) |
| fills | **9 in total, not per frame** |
| stale entries | 0 |
| mismatches, level 2 | **0 of 1,083** |

Nine distinct textured runs, resolved once each for the whole match. Predicted
35,000–45,000 in E11 before building; delivered **−32,724**, recorded as
measured rather than rounded into the band.

Three rules added to `TASK_STANDING_RULES.md`: check whether an instrument
measured the symbol or the work before rejecting a candidate as too small; ask
of every shared cache a native path inherits what its key is and whether this
caller has one; and a default-off `#if` does not hide a probe from a
source-level checker.

## R2-02 F — generic emit split, and the stage target moved

`ClaudeOpus5_R202_F_GenericEmitSplit_20260728.md`, `ea6b1fc`. The per-segment
counters existed and had never been read. **Segments 1/2/3/6 — Whispy's eyes and
mouth, both flower beds — cost 43,998 ticks/frame for 21 triangles**, against
segment 4's 22,843 for 76. At 2,095 ticks per triangle they, not segment 4, are
the largest remaining stage lever; the "segment 4 is the largest" line below is
superseded. R2-02's plan already named them ("small specialized update+draw
path") and that path was never built.

Three cuts refuted on the way, each with a number:

| candidate | measurement | verdict |
|---|---|---|
| merge adjacent runs | 1.0 of 21 repeats the previous state; 18 rebind a texture | dead, ~1,200 |
| revive Task 51 | 0.0 triangles take the path, 1,634 ticks/frame failing, emit +4,754 | structurally dead |
| guard the texture bind | 21,978/frame over 54 runs, both guards already present | near the floor |

Task 51's 2026-07-23 kill named its own revisit condition — find a scene where
bindings 20–29/33–38 submit GX — and that condition is now met. It still fails,
for a *different* reason: `Task51EnsureWorld` rejects on `task36_segment_active`,
and only a rigid binding opens that bracket, which an actor segment does not
have. Pinned so the next reader does not repeat the three builds.

The texture-bind floor caps the actor rewrite near **30,000**, not 44,000.

## R2-03 E5 — the premise is proven, the obvious cut is not worth building

Three counters over one canonical match settle whether R2-03's baked-facts
submit is possible (`ClaudeOpus5_R203_E5_RunFactMemo_20260728.md`,
`de34e051181`, `12968f83dd2`, `fad10d4cf91`):

| question | measurement | answer |
|---|---|---|
| do a run's facts ever change? | 0 misses / 112,300 calls | no |
| does the function ever reject? | entry 112,367 == success 112,367 | no |
| does a UV write ever change anything? | 0 changes / 208,874 writes | no |

`ndsRendererNativePrepareProductionRun` is a pure function of `run_index` in the
canonical configuration. The switch plan's "consuming only baked facts, no
policy re-checks, no per-frame texture identity proof" is achievable, and the
table can be generated rather than discovered.

**But the obvious implementation banks nothing.** The UV loop is only ~119
writes/frame — about 13 of the 67 runs are textured, touching 106 of 541 dense
vertices — which is low thousands of ticks against the bucket's 21,504. The cost
is spread across per-call validation and `texture_prepare_*` bookkeeping, and
`texture_prepare_valid` is already a cache with 44 invalidation sites. Building
a memo for the arithmetic was dropped on the measurement rather than attempted.

Next on this phase is an internal cost split of the function, not an
implementation. Two side effects any memo must preserve are recorded in §4d of
the writeup: the GX bind at `:17313` and the harness-visible texture-prepare
counters.

**The bigger fighter lever is MatrixPrep at 91,338/frame** — four times this
bucket, moving every frame, and where the bulk of the ~460K gap to the 1.12M
gate has to come from.

## Runtime 2 (2026-07-27)

The owner approved `Smash64DS_Runtime2_SwitchPlan.md` and it is now the live
renderer direction; `optimization/archive/NATIVE_RENDERER_PLAN.md` is history.
R2 phases are rows here, measured under `TASK_STANDING_RULES.md`.

| phase | state | evidence |
|---|---|---|
| R2-00a stall attributor | **done, gate met** | `optimization/ClaudeOpus5_R200a_StallAttributor_20260727.md` |
| R2-00b re-baseline + budgets | **done** | `optimization/ClaudeOpus5_R200b_BaselineAndBudgets_20260727.md` |
| R2-01 battle-path skeleton | **done, gate met** | `NDS_R2_PATH`, `src/nds/r2/`; Boundary green |
| R2-02 Dream Land direct runtime | **stage budget MET — 177,088 vs 180,000; E1a/E2/E7/E8 shipping** | `optimization/ClaudeOpus5_R202_E8_PreflightElision_20260728.md` |
| R2-03 fighter direct draw | **unowned — not started** | |
| (R1 harvest) hardware sqrt | done, KEEP | `optimization/ClaudeOpus5_R203_E1_HardwareSqrt_20260728.md` (filename mislabels it R2-03) |

**R2-02's stage budget is MET.** `STG` P50 is **177,088** against the 180,000
provisional budget — 2,912 under — after E1a, E2, E7 and E8. E3 is retracted and
E4 refuted its whole approach; neither contributed. The two soft-float files
named `R2-03` are Runtime 1 harvest, not that phase; they are corrected in place
per the never-rename rule.

```text
STG P50   351,488  baseline
          256,704  after E1a  (-94,784)  prepare-run elision      -- clean
          224,320  after E2   (-30,912)  GXFIFO DMA rigid replay  -- clean
          212,480  after E7   (-11,840)  view-projection hoist    -- bit-exact
          177,088  after E8   (-35,392)  preflight elision        -- bit-exact
          180,000  budget
          -------
           -2,912  UNDER

         (173,120  E3 and E4-C both  (-51,200)  BOTH REVERTED: that number is
                                                the price of not drawing the
                                                flowers, not of drawing them
                                                faster)
```

**E8 is the first arm that followed §7 rather than optimising around it.** For
the five segments the Task 36 replay does not serve, the owner preflight cleared
a 1,292-byte `NDSRendererStats`, initialised a traversal state, and replayed 21
run-level and 16 binding-level state spans to produce a `preflight_stats` and a
traversal state that **nothing reads** once E1a's prepared run table is valid:
`CapturePreparedSegment` early-returns for an ineligible segment, and
`sNdsNativeStageOwnerExecution.traversal` is referenced nowhere outside the
function. The one member that escapes the loop, `sync_command_count`, is now
memoised beside `epoch_mask`. Task 104 had written that sentence down already,
one level lower and for three segments; it was true of the other five and of the
whole loop body. Engagement reads exactly **5 elisions per frame**, and the Task
36 replay stays READY at its full 3,916 words.

Pacing: **2-VBlank frames 13 → 198 of 565**, `WAIT` P50 −202,368, `WORK` P95
−77,504. The DS top screen is **pixel-identical** to the pre-E8 arm — 0 of
121,600 pixels — at presented frame 500 and at the `time_remain` 1800
simulation-clock lock, against a control arm proven reproducible run-to-run.

**All three kept cuts now ship.** `NDS_R2_STAGE_DIRECT`, `NDS_R2_STAGE_DMA` and
`NDS_R2_STAGE_VIEWPROJ` are default-on in the published
`smash64ds-battle-playable-hwtri` block and in the `tickhud`/`proof` block —
`STG` P50 **351,488 → 212,480, −40%**, and the frame moves off the 3 VBlanks the
previous shipping ROM sat at. None of the three spends the fidelity budget, so
none of them needed the owner's visual-oracle call: that clause governs
approximations, and these are exactness-preserving. E7 is bit-identical to its
control on all 42 composed matrices at six frames spanning the camera's range of
motion. The tick-HUD block sets the three *without* `override`, deliberately —
they are the live A/B surface for the rest of the phase — and the graduated
default tick-HUD build hashes `DFBE1ED0E2BB97DB`, byte-identical to the explicit
lab build, so measurement and shipping are the same binary.

**E7 also corrected a wrong rationale that had already been written down twice.**
The cut was designed as an associativity hoist that would spend the Task 49
Tier-2 pixel budget. Dumping `binding_composed` out of both ROMs showed no delta
at all: `ndsRendererAdapterBuildCameraMatrices` already returns
`projection = MtxMul(lookat, persp)` with `modelview_valid` FALSE for the battle
camera, so the compose was `world × (lookat × persp)` — **one multiply per
binding, never two** — and the −11,840 is the per-binding camera-cache lookup and
three 64-byte `MTXCOPY` memcpys that stopped happening. Both E6 and E7 were
designed against arithmetic and both resolved to memory traffic. **Do not size
the next stage matrix lever by counting multiplies.**

**The mechanism, established by E4** —
`optimization/ClaudeOpus5_R202_E4_ActorSegmentsRefuted_20260728.md`. A **rigid**
binding's captured stream is `PUSH` + `MULT4x4` of a constant world under the
camera the segment bracket loads live every frame, so it replays. A **dynamic**
binding's stream is a `MATRIX_LOAD4x4` per triangle of projection × view × model,
so replaying it pins that geometry to the camera the capture frame happened to
have — which is exactly why the flowers sat in a fixed screen band under every
camera. Hence the invariant, now written into both masks:

> `NDS_TASK36_REPLAY_SEGMENT_MASK` must name exactly the segments whose every
> binding is in `NDS_RENDERER_TASK36_RIGID_BINDING_MASK`.

E3 broke it by widening one mask. E4 arm C restored it by widening both — and
lost the flower beds anyway, for an unrelated reason: **the rigid emit path is
single-binding by construction.** `ndsRendererNativeStageEmitNoZTriangle` drops
a triangle whose corners are not all bound to the run's own binding, and the two
flower beds are the only cross-matrix geometry on Dream Land — 10 of their 15
triangles. That is the `cross_matrix_triangles=10` that
`M3_NATIVE_STAGE_CHECK_OK` prints on every Boundary run, and it had been on
screen the whole time.

It is also why the flowers are expensive: a cross-matrix triangle falls to the
generic tail, which loads a composed matrix **once per vertex**. 15 flower
triangles cost 35 matrix loads a frame; Whispy's 12 single-binding triangles cost
12.

Two hypotheses died cheaply on the host and should have died before E3 landed:
every actor triangle carries coordinate shift 0 (so Task 51's missing shift
compensation is irrelevant), and `NDS_TASK51_STAGE_NATIVE` defaults to 0 and is
compiled out of every ROM measured (so E3's premise — "Task 51 already baked
those world matrices" — was false).

**Nothing shipped.** `NDS_R2_STAGE_ACTORS` is deleted. The published ROMs are at
defaults and Boundary-green at **62.750%**, `stage_body` green 44.848% / detail
52.242%. One real defect was found and kept: replay asserted
`task36_local_pushed = TRUE` for every run, so each admitted actor segment bought
an unmatched `glPopMatrix(1)`. Capture now records the run's actual `PUSH`/`POP`
balance.

**The stage partition, re-measured on the graduated program 2026-07-28**
(`census-stage-run-phases.ps1`, frames 439–499, `build-r2-02-census-e7`, total
242,574). This is what E8 was aimed from, and what the *next* stage arm must be
aimed from — the majority is no longer preflight:

```text
prepare owner                 111,849   46.1%
  prepare matrices             42,557          (54,901 before E7)
  renderer prepare owner        49,840
    apply state span             20,370   21 calls @   970   <- E8 elides
    init stats + traversal       13,565    5 calls @ 2,713   <- E8 elides
    unattributed                 13,721          (16 binding-level state spans)
    prepare run                     995   21 calls @    47   (E1a: was 98,828)
  validate task36 world          8,588
  prepare materials              5,623
display commit                130,219   53.7%
  generic emit                   67,126   21 runs @ 3,196, 103 tris @ 652
  replay                         29,124   33 runs @   883
  loop overhead                  13,120   54 iterations
  per-segment scaffolding        13,852    8 commits @ 1,732
```

**The next stage lever is `generic emit`, 67,126 ticks/frame** — the 21 runs and
103 triangles the Task 36 replay does not serve, at 3,196 per run against the
replay's 883 and 652 per triangle against ~294. E4 established it cannot be
reached by widening the replay masks. layer1 (segment 4) is 76 of those 103
triangles across only 6 of the 21 runs, so the cost is per-run dominated and the
15 actor-segment runs are the expensive half.

The older defaults-build partition below (total 401,506) is retained only as the
pre-E1a reference; do not aim new work from it.

```text
prepare owner (preflight)     238,609   59.4%
  renderer prepare owner        165,045
    prepare run                  98,828   21 calls @ 4,706  <- E1a takes this
      head policy/memset/tex       69,379
      dense vertex loop            22,339   143 dense @ 156
    apply state span             30,117   21 calls @ 1,434  <- NOT elided by E1a
    init stats + traversal       16,793    5 calls @ 3,359  <- 1,292-byte clear
    task36 reuse check              693
    validate topology               610
    unattributed                 18,004
  prepare matrices               54,242   16 dynamic bindings @ ~3,390
  validate task36 world           8,577
  prepare materials               5,675
  config / frame setup            2,523
display commit (actual submit) 162,399   40.4%
finish owner                       498
```

**Read this against §7's actual instruction, which has not been followed.**
R2-02 says the static majority becomes *"a fully direct owned path: no generic
preflight, no stats temporaries, no per-frame texture resolution; the runtime
shape is `DreamLand_Run17()`, not discover/validate/rebuild/resolve/prepare/
submit"*. E1a, E2, E3, E4 and E5 all optimised the discover/validate/prepare
pipeline instead of replacing it. Segment 0 already has the prescribed shape —
`ndsRendererNativeStagePrepareGeneratedSegment0`, gated by
`NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE`. Segments 1–7 do not. **Extending
that generated program to the remaining segments is the phase's own design and
is the credible lever §8 asks for before any budget is relaxed.**

Ranked by size, and all of it is preflight the direct path deletes rather than
optimises:

1. `prepare matrices` **54,242** — `ndsRendererAdapterPrepareInitialMatrices`
   walks each dynamic binding's DObj parent chain every frame. The flowers'
   worlds are provably constant (E4 arm C: the runtime rigid-constancy check
   accepted them), so this is recomputing a known-constant world and then
   composing the camera onto it. Splitting camera from world is what Task 36
   already does for the 26 rigid bindings via `MULT4x4` under a once-loaded
   camera. Largest single item and the most clearly structural.
2. `apply state span` **30,117** — 21 calls E1a's `r2_reuse` memo does not
   cover, because it sits outside that guard. Careful: it mutates the running
   `state` that later runs consume, so it cannot be skipped per-run without
   also proving the successor's incoming state.
3. `init stats + traversal` **16,793** — a 1,292-byte blanket clear plus
   traversal init, 5× a frame, for the five segments Task 104's elision does not
   reach. §3.4 names this shape explicitly; extend Task 104's pattern.
4. `unattributed` **18,004** inside the owner span — uncensused, and bigger than
   item 3. Bracket it before assuming it is small.

Items 1–3 total 101,152 against a 44,320 requirement, so the budget is reachable
without relaxing it — the work is structural, not another memo.

**Also on this row — de-cross the flowers in the generator.** For each of the 15
foreign corners emit a duplicate dense vertex pre-transformed into the run's
binding space (`v' = W_run⁻¹ · W_foreign · v`, a compile-time transform because
both worlds are constant). That is +15 dense vertices, no new runs or triangles,
and it makes every flower triangle single-binding. Only then widen
`NDS_RENDERER_TASK36_RIGID_BINDING_MASK` and `NDS_TASK36_REPLAY_SEGMENT_MASK`
together; gate the transform on the Task 49 Tier-2 differ (the inverse-multiply
is where fixed-point error enters); verify with a frame-locked crop of segments 3
and 6 plus a triangle count and `task36_runtime_rigid_mask` read from the run
that produced the buckets. Whispy (20–24) is out of scope — materially animated,
and at 12 single-binding triangles it was never the expensive half.

**Closed 2026-07-28: the R2-02 flags are graduated and the published ROMs carry
them.** This row previously asked the owner to make that call. It was the wrong
ask — the owner is the visual oracle for changes that *spend the fidelity
budget*, and all three of these are exactness-preserving, so the decision was
the verifier's and not a matter of taste.

**What else is left in the stage.** layer1 (segment 4) is 22,738 ticks/frame for
76 triangles and is still generic: its six runs submit through the raw composed
matrix (binding 29, submit classes 0 and 6), which is the camera and genuinely
moves. Moving it onto the segment-bracket path is generator work worth ~19,000 —
now the *largest* remaining stage lever, and still not enough for the gate alone.

**R2-03 is owned; E0–E3 are done and the target is named.** The frame is
re-baselined on the post-R2-02 program: **REAL WORK 1,264,844 against the
1,120,000 budget, gap 144,844** (it was 407,000 at Task 65).

**The sizing is finished and the target is one span: fighter matrix
preparation, 91,338 ticks/frame — 63% of the gap.**
`ndsFighterMarioFoxDLAllDrawForSlot` costs 497,231 ticks/frame inclusive (the
census's 37,206 is self time); the split is walk 3,138, reset 6,675,
revalidation 9,916, owner prep 113,855 (**matrix 91,338** + material 21,504),
submit and tail 361,936. Two independent builds agree to 0.6%.

Three named mechanisms cover 93% of the gap: fighter matrix prep 91,338,
fighter material prep 21,504, stage layer1 22,738. The matrix half is not a
memo — the pose moves every frame — it is §7's generated per-epoch submit
consuming baked facts, and soft-float's 177,503 is largely the same ticks
counted another way (the source's joint transforms are float; the render side
converts them to 20.12 every frame). The material half *may* be a memo and
deserves the E3 falsifier first, at one build for ~21,500.
`optimization/ClaudeOpus5_R203_E4_MatrixPrepIsTheTarget_20260728.md`.

Two candidates were closed on the way, both with evidence rather than opinion:
the shade loop is **not** a memo (inputs and outputs both changed on 1,796 of
1,835 frames), and walk + revalidation is 4% of the function, not the 37% a
self-time-versus-inclusive mix-up made it look like.

**R2-03 E1 took `sqrtf` from 15,760 to 9,720 ticks/frame, −6,040**, bit-exact
against IEEE over 8.7M checked inputs, Boundary green. The 8-frame A/B read
**flat on every bucket** — the saving sits inside the 5,000–7,000 placement
floor, and the symbol census is what resolved it. That constructive half is now
in `TASK_STANDING_RULES.md`: when the predicted saving is near the floor, gate
on the census, which times the function directly and has no placement term.

Only 38% though, not the 17× the hardware's 13-cycle latency suggests: libnds's
`sqrt64` is write / **poll-busy** / write / **poll-busy** / read, and the I/O
polling costs about what the software root did. **On this hardware a coprocessor
is only worth it if the result can be collected without spinning on it.**

**R2-02 E1a took `STG` P50 −94,784, down on 128/128 frames**, and 4-VBlank
frames fell 50 → 12 out of 566. Boundary green; required-region detail 62.792%
vs 62.778%. `NDS_R2_STAGE_DIRECT`, default 0, owner visual approval outstanding.
`STG` is now 256,704 against the frozen 180K budget — gap 76,704, was 171,488.
E2 is `ndsRendererAdapterPrepareNativeStageMatrices` (55,077 bracketed), which
is **not** frame-invariant: the camera moves, so a reuse key will not work
there and E0's sizing method does not transfer.

### CLOSED — the gate metric is sound (R2-00c, 2026-07-27)

**R2-00a's phantom-work finding is refuted, and the row it opened is closed.**
It compared halt measured in a profile ROM against `WAIT` measured in a
different tick-HUD ROM; placement differs between builds, so a frame index does
not name the same workload in both. One ROM carrying both instruments
(`NDS_TASK37_PROFILE_PER_FRAME_REGION=1`, new) settles it over 128 frames of one
run: `ALL` agrees to **0.04%**, `WAIT` to a constant **−851 ticks/frame**, and
the 27 excursion frames (median −860) are no different from the other 100
(median −847). `WORK-H` P95 is not inflated by a mis-scoped bracket; the 1.12M
gap is real. Evidence:
`optimization/ClaudeOpus5_R200c_WaitBracketAudit_20260727.md`.

**What replaced it is a real optimization row.** The excursion is genuine
execution — `armWaitForIrq` falls 323,450 ticks/frame and **+286,619** of work
takes its place, on 21% of frames — and the same per-frame regions attribute it:
softfloat ~49,600, **the tick HUD measuring itself ~44,300**, cart read +
relocation + bulk copy ~36,000, geometry submission ~14,500, collision ~5,700,
animation ~2,700, then a diffuse tail over ~59,000 PCs. Four unrelated causes on
the same frames, which is why five previous tasks found no single mechanism.

Two consequences worth acting on:

- **The frames are not load-free.** `_ntrcardRecvByCpu` + `ntrcardRomRead` are
  12,639 ticks/frame higher there. Task 75's preload targets something real, but
  its ~103,488 estimate must be re-derived against the measured ~36,000.
- **`WORK-H` cannot remove all of the instrument.** `ndsPlatformTickHudSample()`
  runs after the buckets are latched, so the percentile sort (19,605
  ticks/frame) lands in the *next* frame's `ALL`. ~2% of the P95, not 33%, but
  it is the metric charging the ROM for being measured.

R2-00a's other findings stand: no GX, DMA or cart *stall*; ledger closed;
bit-identical reproduction of the prior census.

### The frame, re-ranked on attribution that holds (R2-00c §7)

`task65_subsystem_census.py` named functions with `addr2line -f`, which resolves
through DWARF — and DWARF still describes functions the linker
garbage-collected. It charged 24,240 ticks/frame to `ndsRendererTask29GXRecord`,
which is not in the binary. The census now bisects the ELF symbol table and
overrides addr2line; that **renames 18,987 of 59,366 PCs, 32%**. Aggregates
survive (REAL WORK 1,446,638 vs R2-00b's 1,446,348, 0.02%); the per-symbol table
did not, and that is what targets are picked from.

| group | ticks/frame | % of work | cyc/insn |
|---|---|---|---|
| soft-float | **177,857** | **12.3%** | 1.19 |
| matrix | **156,627** | **10.8%** | 2.35 |
| gx-submit | 144,852 | 10.0% | 2.72 |
| texture-resolve | 108,681 | 7.5% | 4.91 |
| `mem*` | 98,207 | 6.8% | 2.60 |

**Soft-float is the largest block and it is not stalled** — 1.19 cyc/insn, and
`__aeabi_fadd` is already hand-written ITCM assembly. Nothing to win by making
it faster; the only lever is calling it less, i.e. float→fixed at the call sites
in imported gameplay and animation. **Matrix construction is 156,627, not the
55,077 R2-02 E2 was sized at** — the bracket saw one call, the census sees seven
symbols across stage and fighter. Re-scope E2 against that.

The attributor is installed repo-local at
`emulators/melonds-attributor/melonDS.exe` (`D81FC0BF…`) rather than replacing
`emulators/melonds/melonDS.exe`, so measurements taken with `DE80E46B…` stay
comparable. `check-melonds-policy.ps1` passes with it present.

**R2-00b replaced the stale Task 65 baseline.** REAL WORK is **1,446,348**
ticks/frame, not 1,527,277; the gap to the 1.12M gate is **326,348**, not
407,277. Stall is 62.1% of work (memory 555,943, non-memory 342,494), so the
architectural premise is unchanged — memory stall alone still exceeds the whole
gap.

It also corrected an attribution defect Task 65 shipped: `task65_subsystem_census.py`
filed `src/port/reloc_backend_renderer_dl.c` under `PORT/reloc`, charging
**147,777 ticks/frame of renderer adapter work to a bucket named after
loading.** Corrected, **the renderer is 723,554 ticks/frame — 50.0% of the
frame's work** — and all gameplay is 190,649 (13.2%). Any plan built on Task
65's §2 table under-counted the renderer by that amount.

Note for every future phase gate: the census attributes by where code lives and
the tick-HUD buckets attribute by bracket. They are not interchangeable, and the
shared kernels (616,701 ticks/frame) are what differ between them. State which
view a gate quotes.

## Red Queue

1. **Stable 30 FPS:** qualify representative active gameplay at
   P95 <= 1.12M ARM9 ticks per presented frame on the accuracy-focused custom
   melonDS fork. Hardware remains the final check for mechanisms the emulator
   cannot referee.
2. **Mario/Fox completeness:** replace battle-reachable weak status callbacks
   with source-backed behavior and prove both complete movesets naturally.
3. **Dream Land completeness:** close the remaining Whispy material/animation
   presentation debt without reintroducing gameplay-time texture conversion.
4. **Audio completeness:** implement or explicitly qualify every reachable
   voice, pitch schedule, composite cue, and overlapping match-audio path.
5. **Final acceptance:** run the CPU-on one-minute match, complete-match capture,
   owner play/listen pass, reserve gate, Results transition, and teardown proof
   on the exact candidate ROM.

**Performance lane (2026-07-28):** `WORK-H` P95 **1,579,584** after R2-02 E8,
against the 1,120,000 gate — gap **459,584**. (It was 1,647,424 after Task 104;
E7 and E8 took the rest.) `WORK` P50 is 1,163,328 and P95 1,592,320. VBlank
intervals 2:198 3:349 4:14 5+:4 of 565, max 18 — the median frame is still three
intervals, but 35% now present in two where 2% did before Runtime 2. Two search spaces are closed by measurement — exactness-preserving
(Tasks 78–96) and visual approximation in its payload form (Tasks 98–99). The
raster axis was opened in `optimization/RASTER_AXIS_CAMPAIGN.md` and **Task 100
closed it at the first test** — a quarter of the frame's pixels stopped being
drawn and `STG` moved −320 against a ≥40,000 criterion, for the architectural
reason that the DS rasterizer consumes already-swapped polygon RAM and cannot
stall the CPU. Pixels join words and triangles; do not propose another fill,
coverage, AA or overdraw lever.

**Task 103 ran and moved the lane.** Partitioning `STG` in place found that
Tasks 51–55, 99 and 100 all worked the run loop, which is only 35% of the
bucket; **61% (238,254 ticks/frame) is outside the segment commit entirely, in
the owner prepare path, and has never been profiled.** It also found the 21
generic runs the Task 36 replay does not serve cost 63,903 ticks for 103
triangles, and that GX words cost 9.51 ticks each — retiring Task 55 E2's "words
are free" as a below-noise null.

E3/E4 then closed the attribution exactly — all four writers of
`gNdsTickHudStageTicks` tapped with zero added instrument, partition closing to
192 ticks (0.05%) against the build's own `STG`.

**Task 104 took the first cut out of it — KEEP, default on, Boundary green.**
On each of the three Task 36 replay-hit segments the owner cleared a 1,292-byte
`NDSRendererStats` and then overwrote all 1,292 bytes with a copy, to transport
**four live bytes** (`sync_command_count`, the only member read after the segment
loop). Eliding both accesses: `STG` P50 **−22,016**, `WORK-H` P50 **−26,240**,
P95 **−28,352**, VBlank 4-interval **39 → 28**, `FTR` flat. `WORK-H` P95 is now
**1,647,424**. Detail in `optimization/ClaudeOpus5_Task104_FourLiveBytes_20260727.md`.

That result also explains Task 103 E7's 28% realisation and produced a standing
rule: **size a memory lever by bytes that stop being touched, not instructions
that stop executing** — removing one of two accesses to the same cache lines
relocates the misses rather than eliminating them.

**Task 105 then closed the rest of that axis at E0, for one census run and no
builds.** `memset`'s residue is ~16,018 ticks split five ways (Task 84 E1.3
priced `InitStats` at 72% of the family's time), and a re-attributed `memcpy`
census found ~294 matrix copies/frame across five sites worth 3,300–10,600
nominal each — every one discounting to 1,000–3,000 under Task 104's own rule,
below the floor. Two rows in that census are inlined-range artifacts and are
marked as such. **The memory-traffic axis is harvested;** the residue is
structural, in `NDSRendererMatrix20p12` being 4×4/64 B for affine transforms the
DS loads as 4×3/48 B, and is not worth a Runtime 1 refactor.

**Task 106/107 E0 then sized the last untested large lever and re-aimed the
lane.** A 30 Hz simulation (`NDS_TASK106_UPDATES_PER_PRESENT=1`, default 2,
nothing shipped) is worth `WORK-H` P50 −158,592, taking the median to
**1,119,616 — 384 ticks under the gate**. But `WORK-H` P95 falls only −119,744,
because the `SRC` excursion above its own median is **+518,016 on the control
and +522,720 on the candidate — unchanged**. Halving the update rate halves
median `SRC` and leaves its tail intact: the excursion is asset loading driven
by animation events, and fewer update ticks do not reduce how many distinct
animations a match loads.

**The gate is a tail statistic, and Task 75 E0 has now measured what owns it.**
A load counter at `ndsRelocFinalizeLoadedFile`, ringed per frame, discharges
Task 71 §5's obligation — and answers it **no**. All 5 load frames in the window
are `SRC` excursions, so a load is *sufficient*; but **2 of the 7 excursions
carry no cartridge activity at all** (frames 453 and 454, at 2.0× and 1.9× the
`SRC` median), so a load is *not necessary*. The counter cross-validates against
the independent native-owner counter exactly (7 loads, `animLoad:7`).

Sized against the distribution rather than one frame: `WORK-H` P95 is 1,656,896
over all frames and **1,553,408 over load-free frames only**, so eliminating
on-demand loading is worth **~103,488** — 19% of the 536,896 gap, against Task 71
§5's extrapolated ~170,000. And the resulting P95 would be frame 454, a load-free
excursion, so the preload buys 103,488 and hands the gate to an unidentified
cause.

**Highest-value unowned row: profile a load-free `SRC` excursion.** Frame 453 —
single-frame spike, `SRC` 636,096, zero loads, no fallback, `FTR`/`STG` at
median. Task 71's per-PC census windowed on the frame is the instrument; its own
window (469–470) contained a load, so this population has never been profiled.
Whether the residual shares a cause with the loads (relocation, figatree parse)
decides whether one fix serves both and whether the preload's ceiling is higher
than 103,488. Row 51's preload bridge is real but must not start as a subsystem
against 19% of the gap.

Stage levers, still unowned, now second in priority:

1. **The `PrepareRun` head — 67,119 ticks/frame over 21 calls**, the largest
   block inside `ndsRendererPrepareNativeStageOwner` (now ~138,600 after Task
   104) that nothing has attacked. Long span, so the sizing is trustworthy.
   Task 81's closed stage memo does **not** cover it: that was a texture-identity
   memo at the bind seam, and Task 81 measured zero stage texture binds in
   battle. **Highest-value unowned row on the board.**
2. **`ndsRendererAdapterPrepareNativeStageMatrices` — 55,077 ticks/frame at one
   call per frame.** Never profiled; same in-place span method.
3. **Bring the 21 generic stage runs under the Task 36 replay** — 63,607
   ticks/frame for 103 triangles, less the replay's own ~1,785/run. Note this
   cannot be done by widening `NDS_TASK36_REPLAY_SEGMENT_MASK`, which would
   freeze dynamic stage geometry; mode 2 replays complete rigid segments only.

(1) and (2) are per-frame preparation over a topology Task 44 has already proven
unchanged, which is the shape an incremental update attacks. With one call per
frame there is no per-run transfer problem of the kind that killed Task 79 E1.

Task 62's reduced DS-native static mesh remains a **REVERT**. A source-exact
follow-up now preserves material/UV/color/alpha and matches the flag-0 top
screen pixel-for-pixel, but submits the same 525 static vertices. The reduced
candidates have no run/material provenance, so the corrected Task 60/61 gates
recommend none. Keep `NDS_DREAMLAND_DS_MESH=0`; details and the earlier
CPU/GX reduction remain rejected-experiment evidence in
`optimization/archive/Task62_AB_Results.md`.

## Lane Ownership

| Surface | Owner |
|---|---|
| Goal, fidelity, milestone, definition of done | `PROJECT_GOAL.md` |
| Dynamic queue, artifact identity, blockers | this file |
| Exact restart surface and next packet | `HANDOFF.md` |
| Stable architecture | `ARCHITECTURE.md` |
| Verification workflow | `VERIFYING.md` |
| Durable unresolved gaps | `KNOWN_ISSUES.md` |
| Measurements and rejected experiments | `PERF_LEDGER.md` |
| Chronological history | `PORTING.md` |

The current dirty Task 62 follow-up/runtime files are user-owned. Preserve them;
do not infer qualification or overwrite them during documentation cleanup.

## Acceptance Matrix

| Acceptance condition | State | Current evidence / blocker |
|---|---|---|
| Mario human vs original level-3 Fox CPU, Dream Land, one-minute Time, items off | Pass configuration | Boundary registry exposes only canonical mode 163 |
| Original Wait -> countdown -> GO, timer, scoring, Time Up, Results | Focused gates pass | Final exact-ROM CPU-on owner run remains red |
| Mario and Fox complete source-equivalent gameplay behavior | Red | Battle-reachable weak callbacks remain |
| Dream Land collision, platforms, blast zones, wind, camera | Pass for current P1 stage | Dynamic presentation debt remains red separately |
| Recognizable Dream Land presentation and required animation | Red | Whispy material/animation debt; Task 62 candidate rejected |
| Complete overlapping BGM, FGM, voices, announcer, crowd | Red | Exact pitch/composite/voice coverage and listen gates remain |
| Stable 30 FPS, representative P95 <= 1.12M ticks | Red | No current qualifying full-match result |
| Stable reserve, no corruption, clean teardown | Focused gates pass | Requalify after the final content/performance candidate |
| Reproducible public artifact | Red | Current local root ROM differs from the pinned public identity |

## Integration Rule

Keep only correctness-preserving, verifier-covered progress. Rendering may use
the fidelity budget in `PROJECT_GOAL.md`; gameplay must remain mechanically
equivalent to the original. Run the smallest relevant check, then one widest
relevant verifier for a kept checkpoint.
