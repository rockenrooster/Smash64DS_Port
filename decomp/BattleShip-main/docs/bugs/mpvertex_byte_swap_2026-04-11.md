# MPVertexData / MPVertexIDs Byte-Swap Deferral (2026-04-11) — FIXED

**Symptoms:** In the DK-vs-Samus intro scene (`nSCKindOpeningJungle`, scene 45, Kongo Jungle stage) both fighters spawned with broken ground collision:
- DK at spawn position `(-1740, 2, 0)` got `floor_line=4 floor_dist=-3758.9` — no snap applied (the "floor 3758 units below" is impossible on a flat platform), fighter stayed at spawn Y, dropped into the stage geometry.
- Samus at `(470, -207, 0)` (post +1100 X offset) got `floor_line=-1 floor_dist=0.0` — "no floor", went into air kinetics and fell off the map.

Both were the same data-access regression class as the 2026-04-08 MPGroundData bounds bug: all-u16 packed N64 map structures read as garbage after pass1's blanket `bswap32`.

**Root cause:** The 2026-04-08 ground-data byte-swap fix applied `portFixupStructU16` to `MPGeometryData`, `MPMapObjData[]`, `MPLineInfo[]`, `MPVertexLinks[]`, and `MPGroundData` camera/map/team bounds — but the fixup for **`MPVertexIDs` (`u16 vertex_id[]`) and `MPVertexData` (`{Vec2h pos; u16 vertex_flags} = 6 bytes each`)** was left with a `TODO: fixup deferred — need safe vertex count` comment. Those two arrays drive every line-collision computation via `gMPCollisionVertexData->vpos[gMPCollisionVertexIDs->vertex_id[...]].pos.x/.y`. With them corrupt, `func_ovl2_800FB554` initialized each `MPVertexInfo::coll_pos_next/prev` from garbage, and `mpCollisionCheckProjectFloor` returned wildly wrong line IDs / floor distances.

Most intro scenes didn't surface the bug because they place their single-character poses at a hard-coded `desc.pos.y` (e.g. `-600.0F` in `mvopeningdonkey.c` for the posed pose-fighter) and don't care about floor snap. Jungle is the only intro scene that actually runs a two-fighter battle simulation with AI key events, so it was the only one that exercised vertex-driven floor collision. Battle mode proper also never hit it because the DK-vs-Samus intro is in a separate scene from `scVSBattle`, and the earlier bounds fix had made battle-mode vertex corruption non-fatal (fighters would just report slightly wrong floor lines, and the bounds clamp rescued them).

**Fix:** Extend `mpCollisionInitGroundData` (runs right after the existing `MPVertexLinks` fixup) with two additional `portFixupStructU16` calls:
1. Walk `MPVertexLinks[0..gMPCollisionLinesNum)` and compute `max_vid_index = max(vertex1 + vertex2)`. That upper bound is the number of `u16` slots actually referenced in `vertex_id[]`. Rotate `(max_vid_index*2 + 3) / 4` u32 words of `gMPCollisionVertexIDs`.
2. Walk the now-correct `vertex_id[0..max_vid_index)` and compute `max_vpos_index = max(vertex_id[j])`. The vpos array length is `max_vpos_index + 1`. Rotate `((max_vpos_index+1)*6 + 3) / 4` u32 words of `gMPCollisionVertexData`.

Both fixups run before `mpCollisionInitLineIDsAll` / `func_ovl2_800FB554`, so downstream line-initialization sees correct vertex positions. `portFixupStructU16`'s `sStructU16Fixups` set keeps the calls idempotent across scene reloads.

**Verification:** After the fix, `mvOpeningJungleMakeFighters` post-spawn diagnostic prints (since removed) showed:
- DK desired=(-1740, 2, 0) → post-coll=(-1740, **0**, 0) `floor_line=4 floor_dist=0 ga=0` (ground state)
- Samus desired=(470, -207, 0) → post-coll=(470, **-258.6**, 0) `floor_line=4 floor_dist=0 ga=0`

Samus snaps down to Y=-258.6, which matches Kongo Jungle's right-side lower platform. Both fighters enter `nMPKineticsGround` instead of `nMPKineticsAir`.

**Files:** `src/mp/mpcollision.c` — `mpCollisionInitGroundData`.

**Caveat:** The rotate16 formula `(byte_count + 3) / 4` rounds up, so for odd struct counts it overruns the last struct by 2 bytes into whatever data follows the array in the file. Since `portFixupStructU16` is idempotent-per-(base, offset) but not idempotent-by-math, if the 2-byte overrun touches data that another fixup also rotates, the adjacent data could end up double-rotated (= corrupt). No regression observed across scenes 27-45 in a 180-second run, but this remains a latent risk class for any stage whose vertex data is immediately followed by another byte-swap-sensitive struct.

**Class-of-bug lesson:** When adding `portFixupStructU16` calls for a new all-u16 file struct, ALSO check whether any sibling structs (accessed via indirection from the same geometry container) need the same treatment. The `TODO: deferred` marker from the 2026-04-08 fix identified the gap but didn't block on it because the specific crash path was unreachable in the scenes tested at the time. Scene-specific coverage gaps like this surface late.
