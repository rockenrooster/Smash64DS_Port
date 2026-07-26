# Task 60 — DS-Native Primitive Topology: Results

Host-only. Compiles Task 59's mesh candidates into DS-native primitive
topology (GL_QUAD + GL_TRIANGLE_STRIP, residual GL_TRIANGLES) and emits a
submission-cost report. No runtime changes.

This is where Task 59's measured ceiling converts into a real submitted-vertex
reduction. Task 59 found that geometry simplification tops out at ~21% before
the silhouette breaks, and recommended redirecting the 50% ambition to
stripification (this task).

## Deliverables

- `scripts/dreamland_primitive_compiler.py` — quad/strip compiler + cost
  estimator + topology validator + `--check`.
- `scripts/generated/primitive_streams/*.json` — per-candidate primitive-group
  IR (Task 62's runtime input).
- `scripts/generated/dreamland_primitive_cost_report.json` — IoU-aware cost
  report + recommendation.
- `scripts/generated/candidates/c120.json` — the fine-grained candidate Task 60
  added (119 tris, the lowest tri-count still visually acceptable).

## Approach

Per binding group (strips/quads never cross a material boundary):
1. **GL_QUAD** formation: two triangles sharing one edge with 4 distinct verts
   → one GL_QUAD (4 verts for 2 tris). The shared edge is the diagonal (v0,v2);
   the two unshared verts go at v1,v3 so the expansion `(v0,v1,v2),(v0,v2,v3)`
   reproduces both source triangles exactly.
2. **GL_TRIANGLE_STRIP** of the remainder: longest-strip greedy heuristic with
   an **ordered** active edge (last two emitted verts). The DS hardware flips
   winding per triangle, so a triangle extends iff it contains the active edge;
   the new active edge is (second, new). All 3 cyclic seed orderings are tried
   to maximize extension.
3. Residual single triangles → GL_TRIANGLES.

Reuses the Task 56 winding model (`_strip_extend`); the ordered-active-edge
tracking is the fix that makes emission reproduce the exact source triangle
multiset (a frozenset active edge loses ordering and drops triangles).

## Cost model

Per the plan: transformed vertex submissions (the prize), BEGIN count (one per
group), GX words (COLOR + TEXCOORD + VERTEX16 = 4 per vertex + 1 BEGIN per
group), material transitions (binding changes between consecutive groups).

## Result — the candidate ladder (sorted by submitted verts)

Source baseline: 525 submitted verts (GL_TRIANGLES, 3 per corner).

```text
candidate      tris  quads strips  sub_verts    IoU  reduction  acceptable
c40              39     13     13        91  0.237     82.7%      no
c55              55     22     11       121  0.519     77.0%      no
c70              69     29     11       149  0.519     71.6%      no
c90              90     39     12       192  0.841     63.4%      no
c110            110     44     22       242  0.895     53.9%      no
c120            119     48     23       261  0.959     50.3%   RECOMMENDED
r_keep9         125     54     17       267  0.873     49.1%      no
r_keep6         137     60     17       291  0.873     44.6%      no
c140            139     58     23       301  0.987     42.7%      OK (safest)
r_keep3         157     67     23       337  0.987     35.8%      OK
source_welded   175     74     27       377  1.000     28.2%      OK (baseline)
```

## Verdict

- **Recommended candidate: c120** — 261 submitted verts (50.3% reduction) at
  IoU 0.959 (visually acceptable). The cheapest acceptable option per the
  plan's "select the cheapest candidate that still looks acceptable."
- **Safest acceptable: c140** — 301 submitted verts (42.7%) at IoU 0.987, for
  risk-averse integration.
- **The ≤200 / ≤150 stretch targets are NOT met by any visually-acceptable
  candidate.** They ARE reachable (c90 = 192 verts) but only at IoU 0.841,
  which fails the 0.95 visual gate. The owner is the visual oracle; these
  lower-fidelity candidates are preserved for Task 62/63 in case the owner
  accepts the visual delta on device.

## The key win

Task 59 proved geometry simplification tops out at ~21% (c140, 417 submitted
verts at IoU 0.987) before the silhouette breaks. Task 60's strip/quad
conversion turns that 21% geometry cut into a **42.7% submitted-vertex cut**
(c140: 417 → 301) at the same fidelity — and pushing to c120 reaches
**50.3%** (503 → 261). Stripification is pure topology reordering: zero
geometric change, so IoU is identical to Task 59's. This is the reduction the
plan was built to unlock.

## Checker (all gates pass)

`python scripts/dreamland_primitive_compiler.py --check`:
1. **Topology equivalence** — every candidate's expanded primitive groups
   reproduce the source triangle multiset exactly (no missing/extra/dup).
2. **Strip parity** — each strip of k verts expands to k-2 triangles.
3. **Quad integrity** — every quad has exactly 4 verts.
4. **Determinism** — rebuilt report sha256-matches the stored report.

Mutation-tested: corrupting a stream's vertex index is caught (multiset
mismatch); falsifying `target_200_met` in the stored report is caught
(determinism drift).

## Notes for Task 61/62

- **c120 is the candidate to integrate** (Task 62): 48 quads + 23 strips, 0
  residual triangles, 261 submitted verts. Its primitive-stream IR is at
  `scripts/generated/primitive_streams/c120.json`.
- Task 61 (coordinate quantization / cheaper vertex opcodes) can further cut
  GX *words* per vertex (VERTEX10/VERTEX_DIFF vs VERTEX16) on top of this —
  that reduces cost-per-vertex, complementary to the vertex-count cut here.
- c140 (301 verts, IoU 0.987) is the conservative fallback if c120's 0.959 IoU
  reads as too loose on device; the owner is the visual oracle.
