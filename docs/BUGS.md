**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
These bugs should be fixed for P1 delivery:

## Hit-effect presentation (owner, 2026-08-05, with N64/RetroArch reference shots)

- Some VFX textures are rendering half or 1/4 of the full texture. (like ledge grab effect and fox laser muzzle flash)

- during close up shots in pause orbit camera mode, stage floor geometry around the main middle path increases world Y height from some reason. Y height should stay same as main middle path, fixed y height.

  Owner 2026-08-09: the middle slab holds still, the front and rear slabs move.
  Established from `src/nds/nds_native_stage_owner.generated.inc` and
  `NDS_RENDERER_TASK36_RIGID_BINDING_MASK`, the moving set is exactly the rigid
  set: bindings 1 (front floor top, y `0..0`, z `-1756..-249`), 19 (front
  skirt), 30 (rear floor top, y `0..0`, z `418..1541`) and 41 (rear skirt), all
  `PROJECTED_NO_Z`. Binding 29 -- the middle slab, y `-1073..1543`,
  z `-252..444` -- is the one piece NOT in the mask, and it is the piece that
  does not move. So "moves" and "on the Task 36 rigid path" are the same set.
  The rigid branch of `ndsRendererNativeStageEmitNoZTriangle`
  (`nds_renderer.c:29818`) hands GX the projection and the view separately and
  lets the hardware compose, and it never consults `near_inside` nor calls
  `EmitNearClippedTriangle`; the route binding 29 takes does both. A camera that
  zooms in is when a 1500-unit-long slab starts crossing the near plane, and the
  middle slab spans 700.

  NOT the cause, checked and dropped so it is not chased again: BattleShip's
  `max > 32000` camera clamp (`gmcamera.c:1005`) is a uniform rescale of the
  perspective, so it scales all four clip components equally and cancels in the
  perspective divide -- it exists to fit the N64's +-32768 s15.16 `Mtx`, and the
  DS path carries +-524288 in 20.12 with P and V loaded separately, so it cannot
  overflow the same way. The half-coordinate machinery is also not the split:
  the 10 `PROJECTED_RANGE_OR_MATRIX` triangles are asserted binding-29-only at
  `nds_renderer.c:27984`.

  ROOT CAUSE, owner-tested 2026-08-09: the Task 36 replay bakes the projection
  matrix. The A/B pair `NDS_BUG9_FLOOR_TARGETS` (`smash64ds-bug9-rigidon-hwtri`
  / `smash64ds-bug9-rigidoff-hwtri`, differing only in
  `NDS_TASK36_RIGID_BINDING_MASK`) came back clean in BOTH arms, which refutes
  the rigid-mask hypothesis above and moves the variable to the thing both arms
  share and the published ROM does not: they build at
  `NDS_TASK36_HW_COMPOSE=1`, replay off, and the published ROM is mode 2.

  The capture bracket is per RUN
  (`ndsRendererTask36ReplayCaptureBeginRun`/`EndRun`, `nds_renderer.c:31201`
  and `:31298`), and every rigid `PROJECTED_NO_Z` run calls
  `ndsRendererNativeStageTask36LoadNoZProjection` inside it -- so the capture
  frame's PROJECTION lands in the word stream. Only the camera modelview is
  reloaded live, by `Task36BeginSegment`, which sits outside the bracket. During
  a match `fovy` is pinned at 38.0 so nothing moves; the paused player-zoom
  camera calls `gmCameraAdjustFOV(pzoom_fov)` (decomp `gm/gmcamera.c:713` ->
  `:614`), which changes `cobj->projection.persp.fovy`. The replayed rigid slabs
  then render through the stale projection while binding 29, which is not in a
  replayed segment, renders through the live one -- and the front and rear slabs
  read as sitting at a different height from the middle path.

  Fix: the replay staleness guard at `nds_renderer.c:5762` covered materials,
  textures and topology but not the projection. Added it, so replay is declined
  for any frame whose projection differs from the one the stream was baked
  against. `gNdsRendererTask36ReplayProjectionRejectCount` should read 0 for a
  whole match and start counting the moment the pause camera moves the FOV.
  Gameplay cost is nil (constant fovy means the compare never fires); the pause
  screen falls back to the live path, which is what both A/B arms ran.

  Neither A/B arm is publishable: both drop to replay mode 1 and
  `NDS_TASK32_DRAW_HOT_TEXT=0`, so both are slower than the shipping ROM. The
  pre-fix reference ROM is kept at `builds/bug9-reference/`.

