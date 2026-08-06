# Ground Geometry PORT_RESOLVE + Collision Byte-Swap (2026-04-08) — FIXED

**Symptoms:** Scene 30 (nSCKindOpeningMario) segfaults on first game-loop frame in `gcSetupCustomDObjs` (R11=0xCDCDCDCDCDCDCDCD). After fixing, subsequent crashes in `mpCollisionGetMapObjPositionID` (Castle bumper lookup), `mpCollisionInitYakumonoAll` (line info iteration), and `ifCommonPlayerStockMakeStockSnap` (fighter death due to wrong death boundary).

**Root cause — PORT_RESOLVE:** `grDisplayMakeGeometryLayer()` passed raw `u32` token values from `MPGroundDesc` fields directly to API functions (`gcSetupCustomDObjs`, `gcAddMObjAll`, `gcAddAnimAll`, `grDisplayDObjSetNoAnimXObj`) that expect resolved pointers.

**Root cause — byte-swap:** Multiple all-u16 data structures in map/collision files are corrupted by the blanket `u32` byte-swap. After swap, u16 pairs within each u32 word are position-swapped. Affected: `MPGeometryData` (yakumono_count, mapobj_count), `MPLineInfo`, `MPMapObjData`, `MPVertexLinks` arrays, and `MPGroundData` camera/map bounds (s16 fields). The bounds corruption caused fighters to die immediately (wrong death boundary), crashing the uninitialized HUD stock display. Hard-coded byte offsets were wrong due to MSVC struct padding differences — using runtime pointer arithmetic (`&field - base`) to compute offsets is required.

**Fix:**
1. `grdisplay.c`: Wrapped all `MPGroundDesc` token field accesses with `PORT_RESOLVE()` + appropriate casts
2. `mpcollision.c`: Added `portFixupStructU16` calls in `mpCollisionInitGroundData` for: `MPGeometryData` u16 fields, `MPMapObjData` array, `MPLineInfo` array, `MPVertexLinks` array, and `MPGroundData` camera/map/team bounds (using runtime offsetof via pointer arithmetic). `MPVertexData` fixup deferred (needs safe vertex count).

**Files:** `src/gr/grdisplay.c`, `src/mp/mpcollision.c`
