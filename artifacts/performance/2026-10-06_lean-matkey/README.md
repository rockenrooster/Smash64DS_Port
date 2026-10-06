# Lean events reuse keyed material snapshots; AI floor snapshot reads only floor owners (2026-10-06, q21 -> q23)

Changes (all exact):

- `ndsFtrLeanEvent` (renderer_fighter_lean.c): the material rows are checked
  against the old path's per-row keys (`sNdsRendererAdapterNativeOwnerMaterialKeys`:
  MObj, heap generation, hash of the complete input set) before
  `ndsRendererAdapterBuildNativeMaterialSnapshot` runs. A lean event rebuilt
  every material of every root; the snapshot is a pure function of the
  hashed inputs, so a matching key means the row already holds the bytes a
  build would write. The material identity is folded from the same hashes in
  the same loop (it was a second pass over the chains).
- `ndsMPAiFloorSnapshot` (reloc_backend_mp_collision.c): only the yakumono
  groups that own floor lines can change the AI's floor answer, so the
  snapshot keeps their ids per geometry and compares only them, instead of
  re-reading every group's line-info halfwords on every ask (~14K cycles a
  frame on Peach's Castle). The floor test's `< 0.001F` is the exact bit
  compare.
- `ndsMPCollisionEnsureLineGroups`: a geometry/ground-data pair proven
  complete returns at once; every topology reset clears it.
- Two pure tallies in hot paths are `NDS_DIAG` (sweep line visits, fighter
  light direction).

q22 also put three map-collision sweeps in ARM state; that read Sector Z
+4.9K paired (bigger main-RAM code) and was reverted before q23.

Lab four-CPU ROM, one run each; WORK-H (`runsum.py`), paired (`pairab.py`)
against q21 (`../2026-10-06_flat-learn`):

| config | P50 q21 -> q23 | P95 q21 -> q23 | paired median | digest |
|---|---|---|---|---|
| gate (Dream Land) | 777,088 -> 776,320 | 1,064,128 -> 1,062,272 | -128 | identical |
| Castle (gkind 0) | 820,672 -> 810,752 | 1,172,608 -> 1,168,192 | -7,360 | identical |
| Saffron (gkind 7) | 852,160 -> 849,664 | 1,183,744 -> 1,181,952 | -2,048 | identical |
| Sector Z (gkind 1) | 859,968 -> 858,368 | 1,165,888 -> 1,165,248 | -1,024 | identical |
| 4 x Yoshi | 849,664 -> 849,664 | 1,195,264 -> 1,191,040 | +704 | identical |
