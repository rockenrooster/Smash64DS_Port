# Smash64DS — Dream Land DS-Native 3D Stage Compiler Plan (Revision 2)

**Revision 2 — 2026-07-25.** Rewritten after Task 62 was REVERTED at the owner
visual gate. Revision 1 (Tasks 57–65 as originally specified) is preserved in
git history and in the per-task Results docs under `docs/optimization/archive/`.

Revision 1 was not wrong about the opportunity. It was wrong about **what a
simplified stage is allowed to discard**, and it built its acceptance gate on a
metric that could not see the thing that broke. This revision fixes both, keeps
everything that was actually proven, and states plainly which of Revision 1's
targets are now known to be unreachable.

Planner-model prefix for new task docs bumps to **`ClaudeOpus5_`** (was
`ClaudeOpus48_` through Task 62).

---

# Part I — Evidence ledger: what Tasks 57–62 actually proved

| task | deliverable | verdict |
|---|---|---|
| 57 | `dreamland_world_mesh.json` — 175 static tris, 255 dense verts, exact world bake | **Sound, but diagnostic only.** Bakes *descriptor* world matrices, not the live DObj transforms. Drawing its coordinates directly puts the three pass-through platforms near ground level. It is a analysis IR, never a runtime source of truth. |
| 58 | gameplay-camera screen-space error oracle (region IoU) | **Disqualified as a gate.** Rasterizes bare projected triangles with no texture, no alpha, no depth class, no clipping. On a stage that is mostly alpha-textured no-Z cards, its IoU cannot qualify a runtime-visible result. IoU 0.959 was scoring geometry that renders as opaque white rectangles. |
| 59 | constrained quadric simplifier + reconstruction | **PROCEED gate NOT met, and correctly so.** Geometry simplification tops out at ~21% before the silhouette breaks. Separately, it emitted *averaged* vertices with no source identity — the direct cause of Task 62's failure. |
| 60 | strip/quad primitive compiler → c120 at 262 submitted verts | **Deferred, not banked.** Architecturally incompatible with the per-run emit machinery that makes materials correct (see Part II.4). |
| 61 | VERTEX10 + axis-reuse → 261 vertex words | **Deferred, not banked.** Same reason, plus the runtime rebasis produced geometry explosions and was already reverted to VERTEX16 in `fca00cdbd`. |
| 62 | runtime `DreamLand_DrawStatic3D` | **REVERTED at owner visual gate.** "Just Mario and Fox on invisible platforms." Follow-up captures confirmed opaque white cards, not a capture artifact. |

## The one measurement that survived

A **material-complete exact control** — retaining every source run, segment,
dense vertex, texture, UV, colour, alpha, submit class, and the *live* binding
transform — produced a frame-438 top-screen capture **pixel-identical to the
flag-0 control (0 changed pixels)**. That proves the runtime plumbing is
correct and gives us the gate this campaign should have had from the start
(Part II.2). It submits all 525 source vertices, so it carries no performance
win by itself.

## What the working tree already contains

The uncommitted working set is a rewrite toward the right architecture: the
runtime now drives the existing per-run stage machinery
(`ndsRendererNativeStageBeginRun` / `EmitVertex` / `EmitNoZTriangle`) using
source-run and source-dense identities carried through the candidate, instead
of blitting baked world verts flat-white. **That direction is correct and is
retained.** It has three defects, diagnosed and quantified in Part IV, Task 63.

---

# Part II — Corrected principles

These amend the Revision 1 "Hard rules". Rules not restated here still stand
(Dream Land remains a real 3D stage; gameplay collision is untouched and
authoritative; every runtime experiment stays flag-gated; expensive work moves
host-side; the source-driven renderer stays available as an oracle).

### 1. Render identity is a hard invariant of simplification

Every corner the runtime emits **must resolve to exactly one source dense
vertex**. No averaged, interpolated, or synthesized vertices — ever. A vertex
with no source identity has no defensible UV, colour, alpha, or texture epoch,
and reconstructing those by averaging is precisely what turned Dream Land into
white cards.

Consequence: edge collapse must use **subset placement** (collapse onto one
surviving endpoint), not midpoint/optimal placement. This is a standard quadric
variant and costs some reduction. Pay it.

### 2. The visual gate is pixel-diff plus the owner — never a geometric metric

The acceptance gate is:

1. **Deterministic pixel-diff** at a fixed frame against the flag-0 capture
   (the frame-438 method that proved the material-complete control), with an
   explicit changed-pixel budget; and
2. **The owner's on-device A/B.** The owner is the visual oracle.

Task 58's IoU is demoted to a **screening heuristic** for ranking candidates
cheaply during host iteration. It may never appear in a KEEP argument.

### 3. World-space baking is diagnostic; the runtime uses live state

The runtime must obtain binding transforms from live prepared-run state, not
from baked host matrices. Task 57's IR stays useful for census, adjacency, and
simplification — not for coordinates that reach the GX FIFO.

### 4. Cut vertex *count* first; cut *words per vertex* only after

Strips/quads (Task 60) and cheap opcodes (Task 61) both require emitting
vertices outside the per-run machinery that guarantees material correctness.
They are re-admitted only after a materially-correct reduced path has been
measured, and only if that measurement says word count is still the binding
constraint. See Task 66.

### 5. Reduce the 3D surfaces; do not re-triangulate the alpha cards

The static stage is ~66 raw-Z triangles (the actual 3D island and platforms),
~99 projected-no-Z triangles (alpha-textured decorative cards), and ~10
range/matrix triangles. These need different treatment:

- **raw-Z / range runs** — edge collapse onto surviving source vertices is
  allowed. This is real 3D geometry and quadric error is meaningful on it.
- **no-Z runs** — the only permitted operation is **dropping a whole triangle
  (and preferably a whole card)**. Never re-index, never move a vertex. A
  billboard's silhouette *is* its texture; collapsing its corners destroys it
  while the geometric metric registers almost nothing.

This resolves two of Task 63's three defects by construction rather than by
patching.

---

# Part III — Restated success criteria

Revision 1's targets are withdrawn as **measured-unreachable**:

> ~~≤150 submitted stage vertices (stretch)~~ · ~~≤200 (acceptance band)~~

No visually acceptable candidate reaches either. c90 hits 187 verts at IoU
0.841 — and IoU 0.841 on a metric that already over-reports quality. These
numbers should never have survived Task 59's STOP.

**Revised criteria.** The reachable band must be re-derived under Part II's
constraints (that is Task 63's E0 census), but the honest anchor today is:

- source: **525 submitted corners** (175 tris × 3, `GL_TRIANGLES`)
- c120 re-expressed as source-exact `GL_TRIANGLES`: **360 corners — a 31% cut**

So the working expectation is a **25–35% submitted-vertex reduction** at
full material fidelity, not 50–70%.

KEEP requires **all** of:

1. Pixel-diff within the agreed budget **and** owner visual acceptance;
2. **Material `ALL` / P95 improvement** — not STG, not FTR/STG, not GX word
   count. Tasks 53–55 established that stage-CPU reductions relocate rather
   than remove cost; only frame-level evidence counts;
3. Gameplay, collision, and camera bit-unchanged;
4. Deterministic generator + checker green; flag defaults to 0 and the flag-0
   ROM stays byte-identical.

**A KILL is a valid and valuable outcome.** If a materially-correct 31% vertex
cut does not move `ALL`/P95, that is the decisive answer to the question this
campaign has circled since Task 51 — the frame is not bound by stage vertex
count — and it retires the entire stage-geometry axis. Record it and stop.

---

# Part IV — Remaining tasks

Supersedes Revision 1's Tasks 63, 64, and 65.

## TASK 63 — Repair the runtime candidate path and constrain the simplifier

**Goal:** turn the in-flight working tree into a materially-correct *reduced*
stage that renders. Host + runtime. Flag stays default-off.

### The three defects (all quantified against the current working tree)

**Defect A — the source-dense subscript is wrong; 328 of 360 emissions read the
wrong vertex.**

The generator emits `sNdsDreamLandDSSourceFirst[360]` and
`sNdsDreamLandDSSourceCount[360]` mapping each emission into a flattened
406-entry `sNdsDreamLandDSSourceDense[]`. The runtime
(`src/nds/nds_renderer.c:20924`) ignores both and subscripts the flat array by
emission index:

```c
u32 dense_index = sNdsDreamLandDSSourceDense[idx];   /* idx = 0..359 */
```

24 emissions carry more than one source index, so the arrays desynchronize at
emission 18 and never recover. Every corner past that reads a foreign position,
UV, and colour.

*Fix:* not a subscript patch. Enforce Part II.1 upstream so
`source_count == 1` for every emission, making the mapping 1:1 and collapsing
`SourceFirst`/`SourceCount` away entirely. The generator then hard-asserts
one-source-per-emission and emits a single `sNdsDreamLandDSSourceDense[N]`
parallel to the emission stream.

**Defect B — the validation gate rejects exactly the triangles the simplifier
reduced.**

```c
if ((native_run->submit_class != submit_class) ||
    (native_run->triangle_count != triangle_count))  { continue; }
```

A reduced run has *fewer* triangles than its source run by definition. This
drops 4 of 33 groups — 20 of 120 triangles — including the two largest
reductions (run 36: 38→12, run 37: 16→6). The reduction is silently discarded
and the mesh is holed.

*Fix:* replace the equality with the actual invariants — matching submit class,
`triangle_count <= native_run->triangle_count`, dense indices in range, and
(for no-Z) every source triangle offset in range. Keep the
`gNdsRendererM3PostArmFailureCount` accounting so a silent degrade is visible.

**Defect C — the no-Z path ignores candidate topology; that is 80 of 120
triangles.**

`ndsRendererNativeStageEmitNoZTriangle(native_run, prepared_run,
triangle_offset, …)` reads corners from
`sNdsNativeStageCorners[run->first_corner + triangle_offset * 3 + k]` — the
**source** run's triangle list. Passing the candidate's loop counter draws the
first N source triangles instead of the candidate's chosen ones: holes where
counts differ, no reduction where they don't.

*Fix:* per Part II.5, no-Z reduction is triangle *dropping* only. The blob
carries a `source_triangle_offset` per retained no-Z triangle and the runtime
passes that, leaving `Task36TriangleShift`, the binding-uniformity check,
`EnsureWorld`, and `LoadNoZProjection` operating on exactly the inputs they
were written for. No new emit variant is required.

### Work items

1. **`scripts/dreamland_simplifier.py`** — switch `simplify_to_tri_budget` to
   subset placement (collapse onto one surviving endpoint; the survivor keeps
   its own source dense index and attributes). Drop `_mean_rgba` /
   `source_attribute_mean` from `mesh_to_ir` and hard-fail if any vertex would
   need it. Restrict no-Z runs to whole-triangle removal, preserving each
   retained triangle's source offset.
2. **`scripts/generate_dreamland_ds_mesh.py`** — emit `source_dense[N]` 1:1
   with emissions plus `source_triangle_offset[]` for no-Z groups; drop
   `SourceFirst`/`SourceCount`; drop the now-dead
   `sNdsDreamLandDSVertexX/Y/Z` local-coordinate arrays (never read by the
   runtime); refresh the certificate.
3. **`src/nds/nds_renderer.c`** — apply Defect B and C fixes in
   `ndsRendererDreamLandDrawStatic3D`.
4. **Checkers** — extend `--check` on both scripts to assert: exactly one
   source dense index per emission; every candidate triangle's run/submit class
   matches its source run; every no-Z triangle offset is within its source run;
   candidate triangle count ≤ source triangle count per run; determinism
   (rebuilt output sha256-matches).

### E0 census (do this first, before any code change)

Under Part II's constraints, re-derive the candidate ladder and report:
submitted corners, per-submit-class breakdown, and reduction vs 525. **If no
candidate clears ~15% reduction at full material fidelity, STOP and report** —
the axis is not worth the runtime risk.

### Deliverables

Repaired runtime, constrained generator, checkers, E0 census, and
`docs/optimization/ClaudeOpus5_Task63_DreamLandRepair_<date>.md`.

### Housekeeping (do before the code work)

The working tree carries 95 uncommitted changes mixing a large
`docs/optimization/` → `archive/` reorg with the live renderer work, plus an
unrelated `glEnd()` removal in `ndsRendererNativeEmitProductionPrimitiveGroups`
(the Task 56 fighter path). Commit the docs reorg separately; separate or drop
the fighter-path edit. A failed experiment has already overwritten the
published ROM once in this campaign.

---

## TASK 64 — Gate the repaired path

**Goal:** decide whether it looks right, before anyone measures it.

1. Build flag-0 and flag-1 arms of `smash64ds-battle-playable-proof-hwtri` from
   one tree.
2. Capture the fixed frame (438, the canonical post-GO steady-state gate) on
   both arms and produce a deterministic changed-pixel diff. Record the count
   and a diff image, not a verdict adjective.
3. Confirm flag-0 byte-identity against the published ROM
   (`4d795b4e83b335598b20a3b5953fdb1821797cc5e0a825fa96a0643abba4a090`) so the
   override-trap still holds.
4. Hand the flag-1 ROM to the owner for the on-device A/B: main island
   recognizable, three platform silhouettes correct, no holes, no texture
   corruption, camera motion stable.

**No performance measurement happens in this task.** Revision 1 measured first
and gated second, which is how a −29.6% number ended up in the ledger attached
to a stage that did not render.

Keep `check-published-roms.ps1`'s Task-62-payload rejection in place; extend it
to the new payload signature rather than removing it.

---

## TASK 65 — Measure honestly, then KEEP or KILL

Only entered if Task 64 passes both gates.

1. Same 8-frame synchronized A/B methodology, canonical Boundary configuration.
2. Report **P50 and P95** for `ALL`, `STG`, `FTR`, `OTHR`, plus VBlank/pacing
   share and submitted stage vertices. Lead with percentiles and the spread
   ratio; never headline the mean.
3. The decision rests on **`ALL`/P95**. A stage-CPU-only improvement is a KILL
   under Part III, and should be reported as the campaign-closing answer it is.
4. Record the measured relationship `submitted stage vertices → ALL/P95` so the
   cost model outlives this task either way.

---

## TASK 66 — *Conditional* — re-admit strips/quads and cheap opcodes

**Only if Task 65 KEEPs and identifies GX words as the remaining constraint.**

Revives Tasks 60 and 61 on top of the source-exact candidate, one axis at a
time, each with its own pixel-diff gate. Note that Task 61's VERTEX10 rebasis
already produced geometry explosions once (`fca00cdbd`); it re-enters as an
experiment, not as banked work.

---

## Deferred (unchanged in intent)

- **Pause-orbit detail mesh** — only if the shipped gameplay mesh looks too
  coarse under orbit. Skip otherwise.
- **Generalize the compiler pipeline** — only after Dream Land ships. Do not
  introduce a generic runtime stage abstraction that raises Dream Land's cost.

---

# Part V — Execution order and checkpoints

```text
63  Repair + constrain   ── E0 census gate: <15% reduction ⇒ STOP
 ↓
64  Visual gate          ── pixel-diff + owner A/B ⇒ no pass, no measurement
 ↓
65  Measure + decide     ── ALL/P95 only; KILL is a valid, valuable outcome
 ↓
66  (conditional) words  ── strips/quads/VERTEX10, one axis at a time
```

**The checkpoint that matters** is now Task 63's E0 census, not Task 60's
paper vertex count. Revision 1 placed its checkpoint after a host-side number
that turned out to describe geometry the runtime could not draw. The census
must be expressed in corners the repaired runtime will actually emit, at full
material fidelity, or it is measuring the wrong thing again.
