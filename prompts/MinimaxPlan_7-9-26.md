/task Canonical HW battle playable fidelity session. Today's snapshot (Tyler):
3 platforms textured CORRECT (single-tile material path works), fence
texture visible but in the WRONG position, stage floor green/white, tree /
bushes / flowers / stage-bottom all white, background dark blue
(wallpaper/SObj path - DEFERRED, do not block), Mario/Fox visible broken
into PIECES with rainbow textures, input does nothing (held buttons a
while at ~3fps). Frame stays below 60fps; do NOT start cache/perf work.
Renderer-debt remains: RENDER_ORACLE = CPU-oracle projected-submit
fallback, self-referential (oracle vs itself, mismatches 0 is a
tautology). Cite pixel gates and independent checks, never cite the
oracle as DS-matrix correctness evidence.

Corrections to plan_7-8-26_night.md after reading today's source:
- nds_renderer.c:845-880 -> the suspect lines today are the SHARED
  texture_tile_size_* reads in ndsRendererSyncTextureTile (~:520-555)
  populated from per-tile state, and NDSRendererHardwareTextureKey
  (.tile_uls/.tile_ult filled from those shared fields). Same shape bug,
  different exact lines because the file has moved.
- input bridge "maps ONLY LEFT/RIGHT/UP/DOWN/A/START" claim is STALE.
  ndsPlatformReadInput at src/nds/nds_platform.c:147-167 already maps
  B/X/Y/L/R. The held-keys-do-nothing symptom is therefore in the
  DOWNSTREAM chain (gNdsPlatformHeldKeys -> osContGetReadData ->
  gSYControllerDevices[0] -> syControllerUpdateGlobalData ->
  FTStruct.controller), not the read itself. Frame this session's Task
  5 as "trace downstream bridge per-hop markers", not "complete the
  input bridge."

Hard constraints (AGENTS.md / STATUS.md): decomp/ read-only;
pixel-gates one-way ratchets (failing must keep failing); renderer
behavior derives from game-submitted data with GBI citations, not
invented constants; per-hypothesis timebox ~10 emulator runs / ~1h
then checkpoint; debug probes compile-time default OFF and never
shipped in any ROM; one writer for the tree; per-pixel symlink not
allowed; at session end run docs/check-harness-registry /
check-gbi-decode-fixtures / check-docs / commit / Lean snapshot,
then Tyler launches scripts/start-overnight-regression.ps1. Per
the BattleShip-pacing docs: pixel gates must move UP, never DOWN.

---

### Task 0 - Preflight + Gate Ratchet (REQUIRED before any other task)

Where:
  - scripts/verify-battle-playable-harness.ps1 (existing canonical
    HW realtime gate) -- add one detail-pixel assertion.
  - scripts/capture-melonds.ps1 -- already writes captures under
    artifacts/visibility/.

Shape: Add ONE additional pixel-gate assertion: top-screen
detail-pixel fraction MUST be greater than zero and bounded by the
existing non-clear / green / fighter-region ratios. Define
"detail-pixel" = non-clear AND non-white AND non-dominant-green (use
existing ndsRendererTextureColorNonWhite /
ndsRendererTextureColorDominantGreen against captured top-screen
pixels). Baseline-cap the assertion against today's snapshot so the
existing capture keeps passing during this session, then ratchet UP
at each task that should move the metric. Save every milestone
capture under artifacts/visibility/2026-07-09_* for review.

Success: Existing captures (green/non-clear/delta/fighter-region)
still pass with at least one numeric baseline; the new
detail-pixel assertion passes the baseline and fails the lower bound
if a future regressing commit drops it to zero.

Timebox: 20 min. If the existing pixel sampler does not yet expose
per-texel class, do NOT widen the capture -- instead ASSERT
detail > previous-capture-detail and call it a one-way ratchet.

Checkpoint: capture artifacts/visibility/2026-07-09_task0_baseline.png
and the new metric string in Handoff/STATUS.

---

### Task 1 - Texture-Only Classification Probe

Where: src/nds/nds_renderer.c around ndsRendererHardwareColorVertex
(~:1800-1900, immediately before the glColor3b call). Wrap
NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY macro check (already defined to 0
at top of file).

Shape: When NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY == 1, in the
textured-poly branch force use_material_color = FALSE and
use_vertex_color = TRUE so the underlying geometry shows raw
texture texels with no material lighting/color override. Build a
local debug ROM (NOT shipped) with NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY=1
and run the focused visual loop (fixtures, canonical -SkipScreenshot,
capture). Build is diagnostic-only; do not land any change to
src/nds/nds_renderer.c outside the macro guard and never set the flag
to 1 in the configured/shipped ROM.

Success: New capture artifacts/visibility/2026-07-09_task1_texonly.png
with classification per object class recorded in HANDOFF:
  - platforms: textured with recognizable texels -> CONFIRMED
    combine/material/light bug, NOT a tile/TMEM bug.
  - fence: texels appear -> tile source rectangle bug.
  - tree / bushes / flowers / stage-bottom: if still flat white ->
    texture/tile bug; if texels appear -> material/combine bug.
  - fighters: rainbow breakup -> NOT a texture bug; matrix bug.

Timebox: 10 emulator runs / 1h. After capturing, REVERT the flag to 0
in the build config and never ship with the flag on.

---

### Task 2 - Hardware Submission / Truncation Diagnostic

Where: src/nds/nds_renderer.c in ndsRendererHardwareBeginBatch / the
triangle-submit loop (~:3500 plus, beyond current read). First read
lines 1500+ before editing.

Shape: Add four per-class counters wrapped in
NDS_RENDERER_HW_DEBUG = 0:
  - gNdsRendererProfilePolysByKind (split: stage / fighter /
    platform / other).
  - gNdsRendererProfileVertsByKind.
  - gNdsRendererProfileHardwareOverLimit (incremented when batch
    open hits 2048 polys / 6144 verts -- the silently-dropped DS
    hardware budget).
  - gNdsRendererProfileStagePolysBeforeFighter (yes/no marker so we
    can tell if the stage triangles get submitted before fighters
    push the budget out).

Build and capture one canonical HW frame. The counter increment
decides the next task:
  - HardwareOverLimit nonzero -> fighters hit the DS HW budget and
    are silently dropped. STOP and address budget slicing FIRST
    (no fighter fix is demonstrable until the bug class is gone).
  - StagePolysBeforeFighter == 0 but fighters never reach submit ->
    routing bug upstream (gcDrawDObjLLinksForGObj etc), investigate
    before any per-part fix.
  - All zero + counters split one per class ->
    budget is fine; proceed.

Success: New markers in HUD; classification recorded with the
capture; null hypothesis (poly budget) is either confirmed or ruled
out before any per-tile / per-material fix.

Timebox: 1-2 emulator runs / 30 min. Cheap diagnostic.

---

### Task 3 - Per-Tile Texture State + Cache Key (fixes fence+rainbow)

Where: src/nds/nds_renderer.c
  - ndsRendererRecordSetTileSize (~:628-668 today, already writes
    per-tile state correctly).
  - ndsRendererSyncTextureTile (~:520-555 today) populates the
    SHARED stats->texture_tile_size_uls/ult/lrs/lrt/width/height
    from per-tile state.
  - NDSRendererHardwareTextureKey (~:155-180 today, in
    include/nds/nds_renderer.h's NDSRendererStats fields are the
    source-of-truth for what gets hashed) -- key currently reads
    key.tile_uls from stats->texture_tile_size_uls which is the
    CLOBBERED shared field.
  - ndsRendererHardwareFindTexture / TextureKeyEqual
    (~:1300-1380 today).

Shape (smallest correct diff): In ndsRendererHardwareBindTexture,
populate key.tile_uls / key.tile_ult / key.tile_lrs / key.tile_lrt
DIRECTLY from stats->texture_tiles[stats->texture_render_tile].uls /
.ult / .lrs / .lrt -- NOT from the shared
texture_tile_size_uls/etc. The shared fields remain for monitoring
but are removed from cache-key equivalence. Add a "render-tile width
/height came from this tile" assertion the first time per frame.
Add a second per-tile class of cache key entries so two source rects
of one TMEM load no longer alias.

Do NOT in this task:
  - Change the renderer-wide per-tile state struct order.
  - Rewrite texture_tiles allocation.
  - Add new optional tiles beyond 8 (existing NDS_RENDERER_TILE_COUNT
    = 8 is correct).

Success: fence aligns over its platform geometry. Rainbow fighter
texels resolve to one consistent color per body part. Capture
artifacts/visibility/2026-07-09_task3_per_tile.png. The new
gNdsRendererProfileTextureCacheTileAliasCount must show distinct
hits when the source rect changes between two same-image binds (this
proves the cache is no longer aliasing).

Timebox: 10 emulator runs / 1h. If the fence shifts but rainbow
fighters still scatter, the per-tile fix exposed a deeper G_MTX
matrix-assembly bug (proceed to Task 4 without further changes here).

---

### Task 4 - Fighter Per-Part Matrix Assembly (fixes broken fighters)

Where: src/nds/nds_renderer.c
  - ndsRendererApplyMatrixCommand (~:880-940 today, MTX_G / MTX_LOAD /
    MTX_MUL / push).
  - ndsRendererApplyPopMatrixCommand (~:1090-1135 today).
  - ndsRendererApplyMvpRecalcCommand (~:835-870 today).
  - The submitted HW triangle path around ndsRendererHardwareClipVertex
    (~:2400+).

Shape: With NDS_RENDERER_HW_TRIANGLES = 1, log per-frame each
push/pop pair counter, each gSPMatrix (MTX_LOAD vs MTX_MUL), each
gSPPopMatrix, and verify after the DObj tree walk that
projection * modelview = identity-up-to-camera for one part vs.
identity-down-to-root for sibling parts. Specifically:
  - Confirm G_MTX_MUL runs AFTER push/on modelview target, not on
    projection (ensure flags masking treats PROJECTION bit
    correctly).
  - Confirm G_MTX_LOAD on a projection target does NOT push the
    stack (current code already filters this).
  - Confirm that a fighter part DObj whose parent matrix has not yet
    been pushed does not reach the HW submitter with stale zeros;
    either refuse submit or push-and-load identity first.

Independent check (NOT RENDER_ORACLE): For one rendered frame, log
the projected screen positions of every Mario/Fox vertex that
attaches to gNdsRendererProfileORMP and assert the cluster center
for body parts is within +/- 64 DS pixels of the projected fighter
root screen position. Currently fighters scatter widely, so this
will FAIL until the fix lands.

Success: A new capture artifacts/visibility/2026-07-09_task4_fighter_asm.png
showing Mario/Fox as assembled bodies; gNdsRendererProfileFighterPartClusterPx
within +-64 of gNdsRendererProfileFighterRootPx. Re-assert by
re-capturing with the same canonical config.

Timebox: 10 emulator runs / 1h. If parts still scatter after two
fix attempts, STOP, file the partial fix in WIP, and surface the
specific DObj / part it still affects in HANDOFF/STATUS. Do not
spend the next milestone on the assembly problem.

---

### Task 5 - Input Bridge: Trace Downstream Chain

Where: src/nds/nds_platform.c (read chain starts at ~620 in
ndsPlatformDebugTextFingerprint, which already mixes
sHeldKeys & 0xfffu), the controller backend that fills
gSYControllerDevices[0], and include/sys/controller.h /
include/nds/nds_controller.h controller shape.

Shape: Add compile-time per-hop marker for each link in
gNdsPlatformHeldKeys -> ndsPlatformReadInput() return ->
osContGetReadData() output struct -> gSYControllerDevices[0].button_hold
/ .button_tap / .stick_range -> original syControllerUpdateGlobalData()
-> FTStruct.controller. Confirm every link carries KEY_B through to
FTStruct.controller.button_hold bit. The HUD already prints
inp / sy lines; gate the upstream path on whether pressed B in the
ROM actually toggles the original-controller bits.

Flat rejection of the legacy claim: do NOT add `if KEY_B N64 B_BUTTON`
mapping here -- ndsPlatformReadInput already does it. The trace
must reveal WHERE the bits are dropped AFTER that return.

For CREATE / Throw / Shield triggers: original BattleShip uses
A_BUTTON -> smash attack, Z_TRIG -> shield, and B_BUTTON -> special;
the bridge from native to N64 bitmask is canonical across
N64 controller.h. Confirm the bridge code path with a one-line
per-link marker, then fix the broken link only.

Success: Held B on the ROM now produces nonzero
gSYControllerDevices[0].button_hold bit for B specifically and P0
status shows at least one non-idle value. Held L produces the Z_TRIG
flag in FTStruct.controller. Tyler can prove via HUD-only that input
is wired through. Rebuild a second ROM (canonical config,
NDS_DEBUG_HUD=1) with HUD showing heldKeys / native pad0 button &
stick range / original P0 controller status / P0 root-x live (per
docs/KNOWN_ISSUES shipping guidance), label it
smash64ds-battle-playable-hwtri-inputhud.nds.

Timebox: 30 min trace; 1h max fix. If the failure is in original
syControllerUpdateGlobalData's collision with the DS-owned
gSYControllerDevices[0] bridge, fix is small. If it is in
FTStruct.controller read-back, defer to a follow-up session and
record the bridge marker for downstream work.

---

### Session-end Sequence (mandatory; do not skip)

1. Run docs/static checks: check-docs.ps1, check-harness-registry.ps1,
   check-gbi-decode-fixtures.ps1, check-architecture.ps1.
2. Update HANDOFF.md (under 150 lines) with today's captures, tasks
   attempted, partial-fix residuals, and follow-up rank.
3. Update STATUS.md (under 150 lines) with the new boundary state
   (modes 161/162 + 163 unchanged), today's pixel-gate baseline, and
   the explicit renderer-debt note for raw DS matrix/depth.
4. Update KNOWN_ISSUES.md with a new [coverage-reduced] line if any
   bounded prove-and-revert work simplified the marker stack.
5. Append a row to docs/PORTING.md documenting the per-task result
   with file/line refs and capture paths.
6. Commit progress (one commit per completed task; partial-fix WIP
   branch is `codex/wip-renderer-fidelity-2026-07-09`).
7. Run `scripts/New-Smash64DSSnapshot.ps1 -Mode Lean` LAST. No
   commands after the snapshot; only the final response may follow.
8. Tyler runs scripts/start-overnight-regression.ps1 at end of day.

---

### Do NOT block on / do NOT start

- Background dark blue (wallpaper/SObj) - deferred to a separate
  slice; record marker only.
- Renderer-cache 60fps perf improvement - explicitly blocked until
  raw DS matrix/depth is fixed in a later slice.
- Any opcode-by-opcode rewrite of nds_renderer beyond Tasks 3 / 4.
- Any new harness mode (boundary membership unchanged:
  battle_mariofox_stage_mplivehit_status_loop, menu_chain_*,
  battle_playable).
- Any new BattleShip full TU import (use bounded per-DObj /
  per-tile / per-link fixes only - keep AGENTS.md's runtime-first
  invariant).
- Replacing gNdsPlatformHeldKeys / ndsPlatformReadInput / native
  -> N64 button mapping (already correct; the bug is downstream).
