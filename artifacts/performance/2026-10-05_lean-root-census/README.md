# 2026-10-05 Lean root-reuse census (lab)

`NDS_LAB_LEAN_ROOT_CENSUS=1` (Makefile knob; `ndsFtrLeanRootCensus` in
`src/nds/nds_renderer_native_common.c`, called from
`src/port/renderer_fighter_lean.c` after a materialization and before
`ndsFtrLeanLearnVariant`): for each root of the fresh list, is its block (its
LOAD4x3 seed to the next root's) the held list's block for the same root word
for word, patch sites masked on both sides (`ndsFtrLeanPatchMask`)?

Clean lab ROM with the census (`build-lab-census1005`), Sector Z (gkind 1),
one 1,960-frame match (`census-g1.*`), `gNdsLabLeanRootCensus`:

| | value |
|---|---|
| lists compared (a valid held entry of the same root count) | 48 |
| roots | 654 |
| roots of equal length | 542 (83%) |
| identical roots | 365 (56%) |
| words | 82,170 |
| words in identical roots | 46,702 (57%) |
| leading identical roots (sum) | 204 |
| lists differing in at most two roots | 24 (50%) |

A new state usually changes a few roots: most of a materialization re-emits
words the held list already has. `ndsRendererNativeBindProductionRoot` resets
the texture prepare and vertex masks at every root, so a root's words depend
on its own inputs plus the carried N64 state, the packer's header slot and the
running metadata (sites, textures, texgen, tint binds, light): the shape an
incremental materializer would have to fingerprint and rebase.
