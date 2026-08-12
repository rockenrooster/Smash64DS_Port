#ifndef SSB64_NDS_FOX_GUN_H
#define SSB64_NDS_FOX_GUN_H

#include <ssb_types.h>

/* Fox's blaster model part, as DS-ready data.
 *
 * BUGS.md "Fox's pistol model is missing". This header exposes only the baked
 * mesh; the submit lives in nds_renderer.c beside the beam quad, because that
 * is where the camera matrices, the texture cache and the GX batch state are.
 * The split mirrors nds_firegrind.h: this module owns source-derived data and
 * nothing else.
 *
 * The mesh never changes and is never animated. It rides joint 17's world
 * matrix, and the model-part state that gates it is already recorded by
 * ftParamSetModelPartID.
 */

/* ftfox.h:6. The port's own copy lives in battleship_fox_blaster.c behind an
 * #ifndef; it is repeated here rather than reached for because that TU is an
 * imported-source wrapper and including it for one constant would drag the
 * whole weapon in. Both are the source's 17 and check-decomp-header-mirror.py
 * covers the header this mirrors. */
#define NDS_FOX_GUN_HOLD_JOINT 17

typedef struct NDSFoxGunVertex {
    s16 pos[3];
    s16 st[2];
    s8 normal[3];
} NDSFoxGunVertex;

/* Source Vtx texcoords are S10.5 over the texel grid; the DS wants 12.4 over
 * the same grid, so a source unit is a DS unit shifted right by one. */
#define NDS_FOX_GUN_TEXCOORD_SHIFT 1

/* Source model units to DS v16 (4.12): one unit is 16. This is not a choice --
 * it is `1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT)`, exactly what
 * ndsRendererHardwareCoordToV16 computes for every other vertex in this
 * renderer, and the gun rides the same joint space. The mesh spans x -72..60,
 * y -42..54, z -12..12, so scaled it peaks at 1,152 and needs none of the
 * overflow down-shift that path carries.
 *
 * IT IS HALF OF A PAIR. The other half is dividing the composed matrix's
 * homogeneous row by the same 256 (ndsRendererBuildRawHardwareMatrix), and the
 * submit MUST go through ndsRendererLoadHardwareRawComposedMatrix to get it.
 * Applying only this half drew the gun at 0.036 px -- right place, right
 * orientation, invisible, every counter green. */
#define NDS_FOX_GUN_VERTEX_SCALE 16

/* 16 wide x 32 tall, from the source display list and nothing else. CI4 makes
 * 16x32 and 32x16 both exactly 256 bytes, so the byte count cannot tell them
 * apart and an earlier bake guessed the transpose. MiscData315's list settles
 * it three independent ways: G_SETTILE (command 10) carries maskS=4 -> 16 and
 * maskT=5 -> 32 with line=1, which is one 64-bit word per row and therefore 16
 * CI4 texels; G_SETTILESIZE (command 16) carries lrs=60 and lrt=124 in 10.2,
 * i.e. 15+1 by 31+1; and the baked Vtx texcoords run s 0..512 and t 0..1024 in
 * S10.5, which is 16 texels by 32. scripts/fox_gun_bake.py now parses those
 * commands and fails closed rather than asserting a constant. */
#define NDS_FOX_GUN_TEXTURE_WIDTH 16u
#define NDS_FOX_GUN_TEXTURE_HEIGHT 32u

const NDSFoxGunVertex *ndsFoxGunVertices(u32 *count_out);
const u8 (*ndsFoxGunTriangles(u32 *count_out))[3];
const u16 *ndsFoxGunPalette(u32 *count_out);
const u8 *ndsFoxGunTexels(u32 *bytes_out);

#endif /* SSB64_NDS_FOX_GUN_H */
