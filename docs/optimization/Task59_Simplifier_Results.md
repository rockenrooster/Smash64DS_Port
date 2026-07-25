# Task 59 — Constrained Dream Land Mesh Simplifier: Results

Host-only. Implements a deterministic constrained mesh simplifier + stage-aware
reconstruction, judges every candidate against the Task 58 gameplay-camera
oracle, and emits a Pareto report. **No runtime changes. No gameplay collision
change.**

## Deliverables

- `scripts/dreamland_simplifier.py` — quadric-error edge-collapse simplifier +
  position-weld component reconstructor + candidate ladder + Pareto report +
  `--check`.
- `scripts/generated/candidates/*.json` — candidate mesh IRs (Task 57 schema;
  Task 60/62 consume these).
- `scripts/generated/dreamland_candidate_pareto.json` — Pareto report.
- **Task 58 oracle metric upgrade**: replaced the point-Hausdorff silhouette
  (which over-penalized the 48-fragment mesh's internal boundaries) with a
  rasterized region IoU — literally "what the player sees" (the plan's "coarse
  image mask difference"). Identity probe = IoU 1.0; mutation-tested.

## Two simplification strategies, both measured

### 1. Generic quadric edge collapse (with constraints)

Deterministic (sorted-key ties), hard-locks protected edges, penalizes
UV/seam/boundary edges, weights front (camera-facing) geometry higher than rear.

### 2. Stage-aware reconstruction (plan STOP/reframe path)

Generic collapse hit the plan's STOP condition (see below), so the reconstructor
welds exact position-duplicates (the source fragments one surface into 48
UV/binding-split components, but only ~26 are true geometric components), then
drops tiny decorative fragments and collapses within the survivors.

## Measured result — the candidate ladder

Source: 175 tris, 255 dense verts, **525 submitted verts** (GL_TRIANGLES,
3 per corner).

```text
candidate        tris  verts  submitted  reduction  worst IoU  verdict
source_welded     175    249       525        0%      1.000   OK
c140              139    231       417       21%      0.987   OK    <- best collapse
c110              110    213       330       37%      0.895   REJECT
c90                90    166       270       49%      0.841   REJECT
c70                69    155       207       61%      0.519   REJECT
r_weld            175    182       525        0%      1.000   OK    (free position-weld)
r_keep3           157    138       471       10%      0.987   OK    <- best reconstruction
r_keep6           137    106       411       22%      0.873   REJECT
r_keep6_c70        69     72       207       61%      0.760   REJECT
```

Acceptable candidates (IoU ≥ 0.95, protected edges intact): **c140** (21%
reduction, IoU 0.987) and **r_keep3** (10% reduction, IoU 0.987). Both preserved
for Task 60.

## PROCEED gate: NOT met

The plan's PROCEED gate requires **≥50% submitted-vertex reduction while
visually acceptable (IoU ≥ 0.95)**. This is **not met**, and measurement shows
it is **genuinely unreachable by any geometry-preserving method**:

- **Generic collapse** tops out at **21%** (c140) before the silhouette breaks
  (c110 drops to IoU 0.895). This triggered the plan's explicit STOP condition.
- **Reconstruction** (position-weld + component filter) tops out at **10%**
  (r_keep3); dropping components with ≥3 tris collapses IoU to 0.873.
- **Stacking** the two (filter then collapse) does not compose — r_keep3+c140
  drops to IoU 0.929.

### Why the gate is unreachable

The Dream Land static mesh's gameplay-camera silhouette is **carried by many
small fragments**, not a few large simplifiable surfaces. The source is 48
UV-fragmented components (26 true geometric after position-weld); the biggest
structural component is 47 tris but the silhouette also depends on dozens of
3-5-tri fragments (Whispy bits, platform edges, decorative forms). Dropping
*any* of them costs ≥12% IoU. The mesh is already near the visual minimum for
its silhouette complexity — there is no redundant geometry to cut.

The plan's STOP condition is explicit:
> *"If automatic simplification cannot achieve that, switch from generic collapse
> to stage-aware procedural reconstruction... Do not fall back to hand editing."*

Stage-aware reconstruction was implemented and measured; it does not reach the
gate either. The honest conclusion: **geometry-level simplification cannot halve
this stage at acceptable fidelity.** The 50% reduction must come from a
different runtime axis — Task 61's cheaper DS vertex opcodes (VERTEX10/DIFF,
which cut *GX words* per vertex, not vertex count) or Task 60's stripification
(which cuts *submitted* verts via sharing, not unique verts).

## Checker (all gates pass)

`python scripts/dreamland_simplifier.py --check`:
1. **Determinism** — rebuilt Pareto sha256-matches the stored report.
2. **Constraint preservation** — every source protected position survives
   position-welding (protected edges never collapsed away).
3. **Candidate validity** — no invalid triangle indices; all finite positions.
4. **PROCEED gate consistency** — the stored `proceed_gate_met` matches the
   rebuilt verdict.

Mutation-tested: falsifying `proceed_gate_met=true` in the stored report is
caught (determinism drift + gate mismatch). The Task 58 oracle's own
mutation tests confirm candidate displacement is detected.

## Recommendation for Task 60/61

- **Carry c140 and r_keep3 forward** as the acceptable candidates. c140 is the
  best (21% fewer corners).
- **Do not pursue further geometry simplification** expecting to reach 50%;
  the silhouette floor is measured at ~21%. Redirect the 50% ambition to
  Task 60 (stripification: submitted-vert cut via strip sharing) and Task 61
  (cheaper vertex opcodes: GX-word cut). Both reduce the *cost per vertex*
  rather than the vertex count, which is where this stage's remaining budget
  lives.
- The Task 58 oracle (now region-IoU based) is the correct, LOD-stable visual
  gate for all of this; it already passes identity and rejects destructive
  candidates.
