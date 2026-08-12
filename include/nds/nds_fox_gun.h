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
 * it is the same scale the native stage/fighter emit uses
 * (ndsRendererNativeStageEmitNoZVertex writes `x * 16`), and the gun rides the
 * same joint space, so any other scale would put it at the right place in the
 * wrong size. The mesh spans x -72..60, y -42..54, z -12..12, so scaled it
 * peaks at 1,152 and needs none of the overflow down-shift that path carries. */
#define NDS_FOX_GUN_VERTEX_SCALE 16

#define NDS_FOX_GUN_TEXTURE_WIDTH 32u
#define NDS_FOX_GUN_TEXTURE_HEIGHT 16u

const NDSFoxGunVertex *ndsFoxGunVertices(u32 *count_out);
const u8 (*ndsFoxGunTriangles(u32 *count_out))[3];
const u16 *ndsFoxGunPalette(u32 *count_out);
const u8 *ndsFoxGunTexels(u32 *bytes_out);

#endif /* SSB64_NDS_FOX_GUN_H */
