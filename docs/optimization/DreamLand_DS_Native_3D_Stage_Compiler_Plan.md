# Smash64DS — Dream Land DS-Native 3D Stage Compiler Plan

## Objective

Replace the current N64-derived Dream Land visual-stage representation with a **programmatically generated, DS-native 3D mesh and GX stream** that preserves Dream Land's recognizable 3D appearance while radically reducing geometry-engine work.

This plan is **visual-stage only**.

Gameplay collision, platform behavior, blast zones, ledges, Whispy wind, camera behavior, and other gameplay semantics remain authoritative and separate.

### Hard rules

- Dream Land remains a **real 3D stage**. No 2D impostor replacement for the fighting stage.
- Preserve the existing gameplay collision independently from the visual mesh.
- Build tooling may be generic; runtime should be aggressively Dream-Land-specific.
- Move expensive work to host-side generation whenever possible.
- Prefer fewer submitted DS vertices over prettier source topology.
- Prefer lower visual fidelity over missing the stable 30 FPS target.
- Do not preserve N64 DObj/binding/material abstractions when they are not needed at runtime.
- Every runtime experiment must be flag-gated until measured and visually accepted.
- Do not claim a performance win from STG alone; use frame-level `ALL` / P95 evidence.
- The pause-orbit camera may use a separate higher-detail 3D mesh if that materially helps gameplay performance.
- Keep the current source-driven renderer available as a development oracle until the replacement is accepted.

---

# Success criteria

The generated gameplay mesh should aim for:

- **<= 150 submitted stage vertices per presented gameplay frame** as the primary stretch target.
- **<= 200 submitted stage vertices** as the first useful acceptance band.
- A large reduction from the current 606 stage vertex submissions.
- Real DS depth and perspective.
- No material gameplay change.
- Dream Land remains immediately recognizable during play.
- No obvious holes, severe texture corruption, or unacceptable silhouette damage.
- Representative active-gameplay `ALL` and P95 improve materially.
- The replacement should be simpler to render than the current 8-binding / 54-run source-derived stage path.

The generator should produce a candidate report such as:

```text
candidate   tris   submitted_verts   gx_words   max_gameplay_error_px
source      202    606               ...        0.00
A           130    260               ...        0.35
B            95    185               ...        0.75
C            70    140               ...        1.20
D            50    105               ...        2.90
```

The selected mesh should be the cheapest candidate that still looks acceptable in normal gameplay.

---

# TASK 57 — Extract a canonical world-space Dream Land visual mesh

```text
/task TASK 57 — Build the host-side canonical Dream Land world-space mesh IR.

Goal:
Create a new host-only generator stage that consumes the already-validated
Dream Land extraction produced by scripts/generate_nds_native_stage.py and
turns the static visual stage into one ordinary world-space mesh suitable for
retopology and simplification.

Do NOT implement runtime rendering yet.

Requirements:

1. Reuse the existing exact extraction machinery rather than reparsing the
   original stage independently.
   - Consume the generator's StageBinding / DenseVertex / corner / baked-world
     data directly where practical.
   - Do not duplicate BattleShip decoding logic unless unavoidable.

2. Identify the currently rendered static gameplay stage subset.
   - Separate static layer-0 fighting-stage geometry from dynamic stage actors.
   - Whispy face/animated parts, flowers, and other genuinely dynamic pieces
     must remain separately identifiable.
   - Do not accidentally bake dynamic actor animation into the static mesh.

3. Bake each static vertex into final Dream Land world space on the host:
       world_position = baked_world_matrix[binding] * local_position
   Preserve:
   - source triangle membership
   - source material / texture identity
   - UVs
   - vertex color
   - original binding/source provenance for diagnostics only

4. Emit a new host IR, for example:
   scripts/generated/dreamland_world_mesh.json
   or an equivalent deterministic intermediate form containing:
   - world-space vertices
   - triangle indices
   - UVs
   - color/material identity
   - source provenance
   - flags for protected surfaces/edges where determinable

5. Produce a census:
   - source triangles
   - source submitted corners
   - unique exact positions
   - unique position+UV+material vertices
   - bounds
   - material count
   - connected components
   - boundary edges
   - non-manifold edges
   - degenerate triangles
   - static vs dynamic excluded geometry

6. Add a host checker proving:
   - deterministic output
   - all source static triangles represented exactly once
   - no invalid indices
   - no NaN/overflow
   - world-space transform matches the current runtime world result for sampled
     vertices / all vertices if cheap enough
   - the static/dynamic partition is explicit and documented

7. Do not modify gameplay collision.

Deliverables:
- new host-side world-mesh compiler module/script
- deterministic IR
- census report
- checker
- short Results section with exact counts

STOP conditions:
- If the existing generator cannot expose enough information without major
  duplication, refactor the generator minimally so both outputs share one
  extraction pipeline.
- Do not proceed to simplification until the world-space mesh is proven correct.
```

---

# TASK 58 — Build a gameplay-camera visual-error oracle

```text
/task TASK 58 — Build the Dream Land gameplay-camera screen-space error oracle.

Goal:
Create an offline visual-fidelity metric so later mesh simplification is judged
by what the player actually sees, not by arbitrary 3D geometric distance.

No runtime renderer changes yet.

Requirements:

1. Enumerate representative legal gameplay camera states from the current
   Smash64DS camera implementation / original Dream Land behavior:
   - left / center / right framing
   - close zoom
   - medium zoom
   - maximum normal gameplay zoom-out
   - high / low framing produced by normal battle movement
   - representative KO / respawn framing if materially different

2. Keep pause-orbit OUT of the gameplay acceptance oracle.
   Pause-orbit may receive a separate detail mesh later.

3. Implement host-side projection using the same effective DS camera/projection
   conventions needed to compare source and candidate meshes.

4. Render or analytically compare at minimum:
   - outer stage silhouette
   - three pass-through platform silhouettes
   - main fighting-surface top edge
   - visible front lip / underside outline
   - Whispy trunk/static silhouette if included in the static mesh

5. Produce metrics per camera:
   - maximum silhouette displacement in pixels
   - P95 silhouette displacement
   - protected-platform edge displacement
   - projected bounding-box delta
   - optional coarse image mask difference

6. Add configurable hard limits, initially:
   - platform/fighting-surface protected edges: <= 0.5 px preferred
   - major silhouette: <= 1.5 px target
   - decorative low-priority surfaces: <= 2.0 px target
   These are starting thresholds, not sacred constants.

7. The tool must support comparing:
   source world mesh vs candidate mesh.

8. Save deterministic camera fixtures and results so future tasks cannot
   silently move the goalposts.

Deliverables:
- host camera fixture set
- source projection reference
- candidate comparison command
- JSON/Markdown error report
- checker

STOP condition:
Do not permit automatic simplification to ship without this oracle or an
equivalent deterministic gameplay-view comparison.
```

---

# TASK 59 — Constrained DS-specific mesh simplifier

```text
/task TASK 59 — Implement constrained Dream Land visual-mesh simplification.

Goal:
Automatically generate much lower-poly 3D Dream Land candidates while
protecting gameplay-important visual surfaces.

Host-only generation. No runtime path yet.

Requirements:

1. Implement or integrate a deterministic constrained mesh simplifier.
   Preferred approach:
   - quadric-error edge collapse or equivalent
   - deterministic tie-breaking
   - no external GUI/manual modeling dependency

2. Preserve hard constraints:
   - main fighting-surface outline
   - platform outlines
   - ledge-adjacent visual edges
   - intentional holes/openings
   - material seams that cannot safely be merged
   - UV seams where merging would visibly corrupt textures
   - sharp silhouette-critical creases

3. Give strong weights to:
   - visible island silhouette
   - front lip
   - underside silhouette
   - Whispy trunk / major recognizable forms

4. Give low weights to:
   - hidden/rear decorative surfaces
   - tiny bumps
   - small curvature details that textures can carry instead
   - areas with negligible gameplay-camera screen coverage

5. Generate a candidate ladder automatically, for example:
   - source / 180 tris / 140 / 110 / 90 / 70 / 55 / 40
   or a vertex-budget-driven ladder.

6. Run TASK 58's gameplay-camera oracle against every candidate.

7. Emit a Pareto report containing at least:
   - triangles
   - unique vertices
   - estimated submitted vertices before stripification
   - material groups
   - max/P95 gameplay-camera error
   - protected-edge error
   - rejected reason if outside threshold

8. Select no final candidate yet unless one is overwhelmingly obvious.
   Preserve several useful candidates for TASK 60.

9. Do not touch gameplay collision.

Deliverables:
- deterministic simplifier
- candidate meshes
- Pareto/error report
- host checker
- exact source-to-candidate provenance where useful

PROCEED gate:
At least one candidate should reduce estimated submitted vertices by >= 50%
while remaining visually acceptable under the gameplay-camera oracle.

STOP / reframe:
If automatic simplification cannot achieve that, switch from generic collapse
to stage-aware procedural reconstruction of the island/platform/trunk surfaces
using the source mesh as a fitting target. Do not fall back to hand editing.
```

---

# TASK 60 — Rebuild topology for DS primitives, not N64 triangles

```text
/task TASK 60 — Compile simplified Dream Land candidates into DS-native primitive topology.

Goal:
Turn the selected low-poly 3D candidates into topology designed for the Nintendo
DS geometry engine rather than preserving N64 triangle ordering.

Host-only first.

Requirements:

1. For each surviving TASK 59 candidate:
   - build adjacency
   - detect strip opportunities globally within compatible material/state groups
   - reorder triangles freely where render-equivalent
   - generate GL_TRIANGLE_STRIP where beneficial
   - generate GL_QUAD_STRIP / GL_QUADS where a safe quad representation exists
   - retain GL_TRIANGLES only for residual geometry

2. This is NOT Task 55-style lossless stripification of the original topology.
   The input is the new DS-native mesh and may be reordered aggressively.

3. Build a DS submission-cost estimator using actual primitive vertex counts:
   - transformed vertex submissions
   - BEGIN count
   - texture/material state transitions
   - expected GX words by command class

4. Optimize primarily for:
   a. fewer transformed vertices
   b. fewer total GX words
   c. fewer state/primitive breaks
   in that order unless measurement later proves otherwise.

5. Preserve winding/culling correctness.

6. Build deterministic topology validation:
   - rendered triangle set equivalent to candidate mesh
   - no missing/duplicated triangles
   - orientation correct
   - strip parity correct
   - material/UV assignment preserved

7. Produce a report for every candidate:
   - triangle count
   - DS primitive groups
   - submitted vertices
   - BEGINs
   - estimated GX words
   - texture/material transitions

Target:
Find at least one gameplay candidate <= 200 submitted vertices.
Stretch target <= 150.

Deliverables:
- DS primitive compiler
- primitive-stream IR
- checker
- cost report
- recommended candidate(s)

STOP:
If strip/quad conversion increases total cost or creates correctness ambiguity,
keep the simpler primitive form. Fewer transformed vertices is the prize, not
stripification for its own sake.
```

---

# TASK 61 — DS coordinate quantization and cheapest vertex opcode selection

```text
/task TASK 61 — Quantize the generated stage and choose the cheapest legal DS vertex encoding.

Goal:
After geometry reduction, shrink the generated DS command stream further without
increasing transformed-vertex count or causing visible corruption.

Host-only first.

Requirements:

1. Rebase / rescale DS-native local coordinates where useful so geometry can use
   cheaper DS vertex commands while preserving the same final world-space shape
   through a compensating hardware matrix.

2. Evaluate, per primitive/run, the legal use of:
   - VERTEX10
   - VERTEX_XY
   - VERTEX_XZ
   - VERTEX_YZ
   - VERTEX_DIFF
   - VERTEX16 fallback

3. Do not repeat Task 55's assumption that the old N64-scale coordinates must be
   preserved. This task is allowed to choose a DS-native coordinate basis.

4. Search scale/origin choices programmatically.
   Candidate basis must:
   - fit required coordinate ranges
   - preserve protected gameplay-camera edges within TASK 58 thresholds
   - avoid overflow in matrix math
   - remain deterministic

5. Generate exact DS FIFO command/parameter counts.

6. Verify host-side decode/reconstruction of every encoded vertex against the
   chosen candidate mesh.

7. Measure screen-space quantization error with TASK 58.

8. Keep VERTEX16 where a cheaper form is not actually safe.

Deliverables:
- coordinate-basis search
- opcode-selection compiler
- encoded primitive stream
- exact GX word census
- error report
- checker

PROCEED:
Keep only encodings that materially reduce GX words with no visible regression.
Do not accept an encoding merely because it is theoretically smaller.
```

---

# TASK 62 — Generate the runtime DreamLand_DrawStatic3D path

```text
/task TASK 62 — Add the generated DS-native Dream Land static 3D renderer.

Goal:
Introduce a flag-gated runtime path that draws the generated gameplay mesh with
minimal ARM9 work and normal DS 3D depth/perspective.

Requirements:

1. Add a generated include/data blob containing:
   - primitive groups
   - packed vertex/GX parameters
   - material/texture bindings
   - any precomputed static matrix data
   - compact metadata only where runtime truly needs it

2. Runtime structure should be approximately:
       load current camera/projection
       establish static Dream Land transform
       submit generated static geometry
       draw dynamic Whispy/flower pieces separately
   No generic N64 stage interpreter in this path.

3. Use real DS 3D depth for the replacement static stage unless a specific
   surface demonstrably requires a special treatment.
   Avoid carrying PROJECTED_NO_Z purely for legacy compatibility.

4. Eliminate runtime requirements for the static mesh where possible:
   - DObj traversal
   - per-binding matrix composition
   - source DL interpretation
   - cross-matrix ownership
   - runtime strip finding
   - runtime mesh simplification
   - generic material decoding

5. Keep collision/gameplay stage state unchanged.

6. Add selector:
       NDS_DREAMLAND_DS_MESH=0  current shipping stage
       NDS_DREAMLAND_DS_MESH=1  generated gameplay 3D mesh

7. Instrument:
   - submitted stage vertices
   - primitive groups
   - GX words
   - stage owner ticks
   - ALL ticks
   - P50/P95 where existing harness permits

8. Visual gates:
   - normal gameplay side-by-side captures
   - no holes
   - no severe UV/material errors
   - platform silhouettes correct
   - main island recognizable
   - camera motion stable
   - specifically check whether the prior pause-orbit stage "swimming" is reduced

9. Correctness:
   - gameplay collision must remain identical
   - camera/game rules untouched
   - dynamic stage actors still function

10. Performance acceptance:
   - Do NOT keep based on STG reduction alone.
   - Require material ALL/P95 improvement.
   - Record submitted-vertex reduction explicitly.

Target:
<= 200 stage submitted vertices in gameplay.
Stretch <= 150.

Deliverables:
- runtime renderer
- flag
- generated data
- visual A/B
- perf A/B
- Results doc

KEEP gate:
Keep only if visual quality is acceptable and frame-level performance improves
materially.
```

---

# TASK 63 — Automatic candidate shootout on real gameplay

```text
/task TASK 63 — Benchmark multiple generated Dream Land meshes and choose the fastest acceptable one.

Goal:
Use actual runtime performance to choose between several visually acceptable
generated meshes instead of guessing the best fidelity/performance point.

Requirements:

1. Compile at least 3 candidate gameplay meshes from TASK 59-61, for example:
   - QUALITY
   - BALANCED
   - AGGRESSIVE

2. All candidates use the same gameplay/collision code.

3. Measure using the same representative active-gameplay capture:
   - ALL P50
   - ALL P95
   - STG
   - FTR
   - OTHR
   - VBlank/pacing counters
   - submitted stage vertices
   - GX words

4. Capture normal gameplay visuals for each.

5. Rank candidates by:
   1. stable 30 FPS / P95 objective
   2. gameplay readability / recognizable Dream Land
   3. visual fidelity

6. Choose the lowest-cost candidate that still passes owner visual acceptance.
   Do not automatically choose the prettiest mesh.

7. Record the observed relationship:
   submitted stage vertices -> STG+OTHR / ALL
   so future stage compilers have a measured cost model.

8. Promote the winner behind the normal shipping configuration only after:
   - DevFast / relevant verification green
   - normal gameplay visual approval
   - no collision/gameplay regression
   - deterministic generator check passes

Deliverables:
- three-or-more candidate ROMs
- benchmark table
- visual comparison captures
- winner recommendation
- measured DS stage vertex-cost model
```

---

# TASK 64 — Optional separate 3D pause-orbit/detail mesh

```text
/task TASK 64 — Add a higher-detail 3D Dream Land mesh only for pause-orbit / inspection mode, if needed.

Goal:
Keep the aggressively simplified gameplay stage while preserving a nicer 3D
appearance when the game is paused and the orbit camera is used.

Only do this if TASK 63's winning gameplay mesh looks noticeably too coarse in
pause orbit.

Requirements:

1. Reuse the same generator pipeline with a looser performance budget and tighter
   arbitrary-view geometric error.

2. Generate a separate detail mesh, still fully 3D.

3. Switch to the detail mesh only while paused/orbiting or another proven
   non-performance-critical inspection state.

4. Gameplay collision remains unchanged.

5. Do not let detail-mesh memory use endanger runtime stability.

6. Ensure entering/exiting pause does not corrupt GX state or textures.

7. No requirement that the pause mesh meet gameplay-frame performance.

Deliverables:
- optional detail candidate
- pause-mode selector
- visual comparison

STOP:
Skip this task entirely if the gameplay mesh already looks good under orbit.
```

---

# TASK 65 — Generalize the compiler pipeline without slowing Dream Land

```text
/task TASK 65 — Refactor the successful Dream Land compiler into reusable stage-build tooling.

Goal:
After the Dream Land path is proven, generalize the OFFLINE generation pipeline
for future SSB64 stages without replacing the fast Dream Land runtime with a
generic renderer.

Requirements:

1. Extract reusable host modules for:
   - source mesh import
   - world-space baking
   - protected-feature tagging
   - camera-fixture evaluation
   - simplification
   - topology rebuilding
   - DS primitive generation
   - coordinate quantization/opcode selection
   - candidate reporting

2. Keep DreamLand_DrawStatic3D specialized.

3. Do not introduce a generic runtime stage abstraction that increases Dream Land
   cost.

4. Document the minimum per-stage configuration needed for future stages:
   - dynamic/static partition
   - protected surfaces
   - camera fixtures
   - visual-error thresholds
   - target budgets

5. Preserve deterministic generation and checkers.

Deliverables:
- reusable host compiler modules
- Dream Land remains byte/perf equivalent to the accepted implementation
- short guide for adding the next stage
```

---

# Recommended execution order

```text
57  Canonical world-space mesh
 ↓
58  Gameplay-camera error oracle
 ↓
59  Constrained simplifier
 ↓
60  DS-native topology / strips / quads
 ↓
61  Coordinate quantization + vertex opcode selection
 ↓
62  Runtime generated 3D renderer
 ↓
63  Runtime candidate shootout
 ↓
64  Optional pause-orbit detail mesh
 ↓
65  Generalize build tooling
```

## Highest-value checkpoint

**Do not wait until TASK 62 to learn whether this architecture has enough upside.**

By the end of TASK 60 the host tools should already tell us approximately:

```text
source:     606 submitted vertices
candidate:  ??? submitted vertices
```

If the best visually acceptable candidate is still above roughly 300 submitted
vertices, stop and reassess the simplification/reconstruction strategy before
doing runtime integration.

If the generator finds a good candidate near 100-200 submitted vertices, proceed
aggressively: that is the architecture this plan is trying to unlock.
