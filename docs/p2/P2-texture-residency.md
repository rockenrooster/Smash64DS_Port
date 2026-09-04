# P2 texture and VRAM residency — decision and Phase 0

`docs/reviews/Review_DS_Texture_VRAM_Residency.md` answers
`docs/reviews/Ask_ds_texture_residency.md`. This file carries the decision and
the work it authorises. Read the review for the reasoning; this does not repeat
it.

## Decision

The fixed `44 static / 79 dynamic` partition **does not survive as the residency
policy**, and it is **not** replaced by a gameplay-time LRU — an LRU can choose
which texture loses, but it cannot make an overfull simultaneous working set
fit, and its miss path reintroduces exactly the allocation, upload, conversion
and global invalidation this project has been removing from gameplay.

The replacement is a **deterministic generated scene-residency plan, committed
before `GO`, with stable texture handles for the whole residency epoch** and a
small explicitly-bounded optional-presentation region. Residency locks at `GO`:
required content may not allocate, upload, evict, convert or touch NitroFS
afterwards. A missing *required* resource prevents the scene from starting and
names the failed constraint — it never demotes the stage to the generic
renderer.

The existing fixed arrays, direct slot indexing, generated static records and
prepared-run handles are good machinery and stay. What goes is the idea that an
anonymous global high-water partition decides admission *while rendering*.

## The two questions I asked, answered

**The `82` vs `79` arithmetic is history, not a shortage.** The review
reconstructed it from commit history:

| State | Total | Static | Dynamic |
| --- | ---: | ---: | ---: |
| before `8da5257c2528` | 114 | 32 | **82** ← the "78 measured + 4 headroom" state |
| `8da5257c2528` | 114 | 35 | 79 (three live Dream Land keys moved to static) |
| `6178b43d052d` | 123 | 44 | 79 (nine DeadExplode variants preloaded; total grew by nine) |
| current | 123 | 44 | 79 |

So nothing is three entries short. **But 79 is not validated as a P2 capacity
either** — the conclusion lives in commit history and contradictory prose, not
in a generated set proof, and Ness, Kirby, items, Pokémon or one new texture
view can invalidate it without moving any counter the checker pins.

**"Texture capacity" is at least five independent constraints**, which is why a
high-water count can pass while a picture silently changes: texture storage
bytes; texture view/key slots; palette bytes and bases in banks F/G;
format-specific auxiliary placement (DS 4x4-compressed blocks have coupled
placement requirements); and atlas geometry plus shared-palette quality, where
enough free texels does not imply a required rectangle can be placed.

## Corrections to what this repo believed

- **Banks C and D are not "one full-screen background".** `nds_platform.c` makes
  BG2 and BG3 each a 256x256 16-bit bitmap at 128 KB, used as general
  scene-owned overlay layers with direct pixel writes, affine transforms and
  title-fire use. Reclaiming them is a compositor and scene-bank-ownership
  change, not a free remap. The review's answer is **scene-specific VRAM
  profiles** — battle can plausibly take D for texture (384 KB) or both C and D
  (512 KB) while menu/title keeps the compositor layers.
- **The stage failure boundary is a containment bug**, not degradation: one
  run's failed texture resolve invalidates an owner carrying thousands of static
  stage commands. Generic rendering must never be a resource-pressure fallback.
- **The particle packer is not the central problem.** Keep atlases — few texture
  names is an advantage on this hardware — but replace best-effort membership
  with a required-closure contract that has **no eviction operation**, and use
  **scene-specific atlas variants** rather than growing one global atlas toward
  the union of all P2 content.

## The ImpactShock case becomes a build error

A required Yoshi's Island cell may not replace required texture 30 just because
the totals read the same. The solver's only acceptable outcomes are: find
another valid placement, use a pre-approved different representation, select a
larger allocation the complete plan proves, or **fail and report the 128-byte
witness**.

Note the review's caution on the fifth-sheet answer already in `docs/BUGS.md`: a
dedicated 32x32 texture is not automatically better, because it trades atlas
geometry pressure for one more resident view/name and its own palette state. The
planner should compare those axes explicitly rather than assume.

**The three Stock effects enter through the same gate.** If
`efManagerStockSnapMakeEffect`, `efManagerStockStealStartMakeEffect` and
`efManagerStockStealEndMakeEffect` are restored, scripts `0x26`, `0x75` and
`0x76` become *required roots* and their complete script and texture closure must
be marked required before the forwarders are enabled. If that closure does not
fit, **the build must fail** — wiring a forwarder while its scripts are absent
just converts a known inert stub into another silent visual failure.

## Phase 0 — contain the two silent failures, before any planner

These are authorised now and do not depend on the rest of the architecture:

1. **Correct the cache-sizing history** in `src/nds/nds_renderer_preamble.c` and
   generate/assert the arithmetic from one source of truth instead of prose.
2. **Publish the complete nested first texture fault unconditionally.** Keep
   outer reason `6`, but carry the inner reason, run index, requested view,
   requested bytes, and the first-failure cache/bank census. Today the inner
   reason needs `NDS_TASK36_REJECT_TRACE`, so the shipped ROM reports a code
   that names nothing.
3. **Stop letting a texture-resource failure invalidate the whole stage native
   owner.** Preserve an independently prepared stage-core validity proof.
4. **Make the particle checker compare exact required/admitted/excluded sets per
   configuration variant, not counts.** Half-done 2026-09-04: the excluded set is
   now pinned per bake; required and admitted are still counts.
5. **Add permanent counters and gates for post-`GO`** texture creation, upload,
   deletion, eviction, conversion and NitroFS reads.

Later phases (generated manifest, host admission checker
`scripts/generate_nds_texture_residency.py` +
`check-nds-texture-residency.ps1` + `NDS_TEXTURE_RESIDENCY.generated.json`,
stable pre-`GO` handles) are in the review's migration plan and are not started
until Phase 0 lands.
