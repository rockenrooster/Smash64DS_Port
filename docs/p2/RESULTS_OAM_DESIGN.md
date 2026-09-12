# P2 — Native Results Composition and Acceptance

Qualify the existing/native Results owner against source scene 24. Retain the source draw-order and sprite/fill inventory from the prior design, but replace invalid OBJ-size arithmetic and any silent-output shortcuts. This document does not assume the current implementation contains every old design defect.

## Source composition contract

Read `mnvsresults.c` and actual native call sites. Source camera order places wallpaper, wallpaper-only tints, emblem, another tint, fighters, player tags, winner text, a further tint, table/header and transition at distinct depths. The final table is not darkened by the same set of fills as the background/fighters. A single global fade is not automatically equivalent to these layers.

Preserve source player count, FFA/team ranking, ties/Sudden Death outcome, No Contest, winner text and all applicable labels/numbers/arrows/stocks. Source glyphs can be anisotropically scaled. Colors use source prim/env meaning rather than field-name assumptions. Source fill rectangles have no SObj and still need native owners; ignoring them because they are not sprites is missing content.

## Package: actual state and asset inventory

**Outcome:** Map every source display callback, SObj, fill, 3D owner and transition to its real DS native consumer and resource lifetime.

Reuse `nds_results_oam.c`/its generator and shared native helpers if present. Trace whether battle/UI tenants truly retire at Results entry; do not allocate their bank merely because their callbacks are idle. Identify source provenance (asset and offset) instead of ambiguous width/bitmap matches. Distinguish static baked images, color-dependent palettes and runtime digits/width changes.

Compile-time, loaded and actual displayed inventory are separate witnesses. Every required sprite/fill has a natural Results state that reaches it. An owner that claims the screen but draws no tags/tints cannot pass this inventory.

## Correct OBJ budget

The libnds `SpriteSize` enum permits squares 8×8, 16×16, 32×32, 64×64; wide 16×8, 32×8, 32×16, 64×32; tall 8×16, 8×32, 16×32, 32×64. **There is no single 64×16 cell.** Source: [libnds sprite.h](https://libnds.devkitpro.org/sprite_8h.html), SpriteSize.

Worked *planning* example, before allocator alignment/palette overhead: a 53×11 image could occupy two 32×16 4bpp cells (512 bytes, two OBJ IDs), or one 64×32 4bpp cell (1,024 bytes, one OBJ ID). These are alternative layouts, not simultaneously charged. Verify the configured mode, legal tiling, transparent padding and mapping granularity; 4bpp cell bytes are not valid estimates for a direct-color bitmap.

Compute the actual peak across all four-player/long-name/ranking/No Contest states: image storage, palettes, legal OBJ cells, affine matrices, 3D layers and active scanline load. Shared images can reuse graphics but still require each visible OBJ instance. Do not promise a fixed cell count until the real inventory is packed. Use prefiltered source art at the chosen DS size where appropriate; nearest-neighbor runtime rescaling must not silently destroy small digits.

## Package: coordinate, material and depth consistency

Choose the coordinate mapping that matches the accepted source-derived wallpaper/foreground treatment. The earlier document proposed both full-bleed anisotropic mapping and source viewport scale; neither alternative is an owner acceptance merely because it was written down. Use the existing approved treatment, or record the visible delta for owner review before changing it.

Preserve prim/env palette ramps, source alpha/blend meaning, glyph scaling and the source timing of row creation/bar growth. A convenient alpha threshold, dropped tint or unfilled rule is not accepted without measured reason and approval. Use native BG/OAM/GX composition appropriate to the hardware; no software SObj compositor in a diagnostic or published ROM.

**Proof:** Capture before/through/after each tint/table phase with known foreground/background positive controls. Check tags, long winner names, number alignment, plate/rule/bar, all ranking slots and No Contest. Match source output dimensions rather than declaring successful OAM allocation a visual pass.

## Package: lifecycle and natural result flow

**Outcome:** Real Time/Stock/FFA/team/tie/No Contest results lead back correctly, with no inherited battle resources or stale OAM entries.

Prepare source-derived static assets before visible use; updates touch only genuinely changing text/state/palettes. Account for prewarming or overlap at the battle→Results boundary. Clear retired objects and source-owned callbacks on exit; cancel input only where the source does. Re-entry with a different fighter count/winner must not retain old tags, tint state or texture handles.

**Proof:** Actual battle end→Results→START/CSS; alternate counts/team outcomes, Sudden Death-selected participants and No Contest. Native draw/asset/coverage counters plus output; memory/bank ownership and 30 Hz cadence over animation and settled table. Keep the existing regression arm and relevant widest verifier for the candidate.

## Exit checklist

- [ ] Every required sprite, fill, 3D layer and transition has correct native output.
- [ ] Legal packed OBJ/VRAM/palette/affine/scanline budget covers all Result states.
- [ ] Source depth/tint, color/alpha, coordinates and timing are preserved or approved deltas documented.
- [ ] Natural battle→Results→CSS/re-entry and memory/resource handback pass.
- [ ] Exact candidate captures, cadence and any owner review are banked; old design arithmetic is not runtime proof.

## Source and retained evidence

Repository/source baseline: `907c46daffbec55477459cc56e83dfc9a417dabb` (September 10, 2026). This revision defines work and acceptance; it does not claim a new build or runtime pass. Current state belongs to `docs/P2_EXECUTION_BOARD.md`; owner symptoms belong to `docs/BUGS.md`.

- `decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c`.
- `src/nds/nds_results_oam.c`.
- `src/nds/nds_ifcommon_oam.c`.
- `src/port/sprite_preview_backend.c`.
- `docs/p2/P2-1c-vram-map.md`.

[Pre-revision document and its source pins](https://github.com/rockenrooster/Smash64DS_Port/blob/907c46daffbec55477459cc56e83dfc9a417dabb/docs/p2/RESULTS_OAM_DESIGN.md). The bundle installer preserves that document verbatim under `docs/archive/P2_PLAN_BASELINE_2026-09-10/p2/RESULTS_OAM_DESIGN.md`. Use retained investigations only when relevant; superseded diagnoses are not new implementation instructions.
