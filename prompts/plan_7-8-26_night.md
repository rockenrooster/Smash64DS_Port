/task Overnight fidelity session: per-tile texture state, fighter assembly,
input bridge completion — then run the FULL Regression sweep to completion
(Tyler is asleep; do not stop at the sweep launch, monitor it until done).

Context from Tyler's playtest of b75f6acc7: platforms textured CORRECTLY (the
single-tile material path works end-to-end); fence texture visible but in the
WRONG position; stage floor green/white, tree/bushes/flowers/stage-bottom
white; background dark blue (wallpaper path, separate); Mario/Fox visible but
BROKEN INTO PIECES with rainbow textures; input has no visible effect.

Two planner notes that override anything else you assume:
- The visible path is now the CPU-oracle projected-submit fallback, so the
  RENDER_ORACLE gate is SELF-REFERENTIAL (oracle vs itself, mismatches 0 is a
  tautology). Do not cite it as correctness evidence this session; the pixel
  gates and independent checks below are the evidence. Record the raw DS
  matrix/z path as explicit renderer debt in KNOWN_ISSUES - the 60fps slice
  must either repair hardware T&L or cache projected command lists.
- Reference snippets from an external ChatGPT review are attached. Planner
  verified claim 1 in code; claim 4 is UNVERIFIED. Apply ideas by hand, never
  patch/paste blindly; all debug probes are compile-time, default OFF, and
  must not ship in any ROM.

1. Tile-size clobber (planner-CONFIRMED at src/nds/nds_renderer.c:845-880:
   ndsRendererRecordSetTileSize extracts the tile index but overwrites the ONE
   shared texture_tile_size_*/width/height regardless of tile). A later
   SetTileSize for tile 1 (scroll/secondary material) clobbers tile 0's source
   rectangle - this matches the misplaced fence directly and plausibly the
   rainbow fighter texels. Fix in two steps:
   a. Quick probe: ignore SetTileSize for non-render tiles; run the focused
      visual loop (fixtures, canonical -SkipScreenshot, capture, assert).
      Expect platforms unchanged, fence/floor/fighter texels improved.
   b. Land the real model: per-tile descriptor array (8 tiles: set_seen/
      size_seen, fmt/siz/line/tmem/palette, cms/cmt/masks/maskt/shifts/shiftt,
      uls/ult/lrs/lrt/width/height), render-tile selection from the recorded
      gSPTexture tile, and per-tile values driving BindTexture dimensions/
      origin/wrap/palette, TextureParams, and the texture cache key (a cache
      keyed without tile origin will alias different source rects of one TMEM
      load). Validate field extraction against the GBI spec, not the sketch.
2. Classify the remaining white (tree, floor patches, stage bottom) with a
   texture-only probe: compile-time NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY that
   forces full-white vertex color on textured polys in one local build.
   Textures become recognizable -> remaining bug is combine/material/lighting;
   still white/misplaced -> continue tile/TMEM/palette state (fighter
   materials: tile-6 load-block, CI palette base per part, MOBJ FRAC/SPLIT
   flags). Report the classification per object class before fixing further.
3. Fighters broken into pieces: this is NOT a texture bug - it is per-part
   matrix composition in the projected-submit path for DObj hierarchies.
   Verify push/pop pairing, G_MTX MUL-vs-LOAD semantics, and billboard flags
   per fighter part in that path. Independent check (do not use RENDER_ORACLE):
   compare a few projected part screen positions against the fighter's
   gameplay joint/root positions - parts must cluster around the root, not
   scatter. Fix until Mario/Fox read as assembled bodies in the capture.
4. Light-state persistent copy (UNVERIFIED external claim): locate where
   persistent fighter DObj renderer stats are copied/reset between parts or
   frames. If it is a field-list copy that omits light_color_1/light_color_2/
   light_color_mask/light_dir_x/y/z/light_dir_mask (fields exist at
   include/nds/nds_renderer.h:251-257), add them; if it is a whole-struct
   assignment, record the claim as a no-op and move on.
5. Background (dark blue): the Dream Land backdrop is the wallpaper/SObj path,
   not HW triangles. Add markers only (wallpaper SObj seen/draw/texture
   counts) and document composition (DS 2D BG layer vs textured quads) as a
   deferred slice. Do not block this session on it.
6. Input bridge (demo-critical, provable without eyes):
   a. Planner-verified gap: ndsPlatformReadInput (src/nds/nds_platform.c:149-
      167) maps ONLY LEFT/RIGHT/UP/DOWN/A/START. There is no B (specials) and
      no shield trigger, so even a perfect bridge cannot special or shield.
      Complete the mapping (B, X/Y, L/R -> the pad bits the original controller
      layer expects; cite the port's pad bridge and original controller
      semantics for every bit).
   b. Statically trace heldKeys -> live pad0 -> the controller port P0 reads in
      canonical mode; fix any break and add a marker for each hop.
   c. Build Tyler a DEBUG variant of the shipped ROM (canonical config,
      NDS_DEBUG_HUD=1) whose HUD shows heldKeys, pad0 buttons/stick, P0
      status, and P0 root-x live - so tomorrow's playtest proves input
      numerically on-screen even at 3fps. Ship BOTH ROMs and name the debug
      one clearly (e.g. smash64ds-battle-playable-hwtri-inputhud.nds).
7. Gate ratchet (one-way): keep non-clear/green/fighter-region/delta gates;
   add a non-white-non-green detail-pixel fraction so blown-out white scenes
   fail. Save every milestone capture under artifacts/visibility/ for Tyler
   and the planner to review.
8. Session end - FULL SWEEP TO COMPLETION (explicit Tyler instruction for
   tonight, supersedes the launch-and-leave policy): finish docs (PORTING,
   KNOWN_ISSUES renderer-debt note), check-docs, check-harness-registry,
   check-gbi-decode-fixtures, COMMIT, snapshot -Mode Lean; THEN launch
   scripts/start-overnight-regression.ps1 and MONITOR to completion (poll the
   prebuild stamp, then the 4 shards; one sequential rerun per transport-noise
   failure per policy). If the sweep exposes regressions, fix on a WIP branch
   (codex/wip-*) - do not rewrite master overnight - and report. Final report
   must include prebuild seconds, per-shard results, and any reruns.

Do NOT start cache/perf work. Constraints: decomp/ read-only; pixel gates are
one-way ratchets; renderer behavior derives from game-submitted data with GBI/
decomp citations; debug probes compile-time default-off, never shipped;
per-hypothesis timebox ~10 emulator runs / ~1h then checkpoint; one writer.
