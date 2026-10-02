#ifndef NDS_NATIVE_BAKED_TYPES_H
#define NDS_NATIVE_BAKED_TYPES_H

/* Baked native roots (scripts/stages/generate_nds_native_item_baked.py):
 * the resident item/weapon/effect/ground table, compiled into the renderer by
 * nds_native_item_baked.exec.inc, and the 1P ending's room table, compiled by
 * nds_native_room_baked.c into the front-end overlay tail. Both are executed
 * by ndsRendererSubmitNativeBaked. */

#include <PR/ultratypes.h>
#include <nds/generated/nds_native_item_baked.generated.h>

typedef struct NDSNativeBakedOp
{
    u32 w0;
    u32 w1;
    u16 arg;    /* EMIT: group; MATERIAL: segment-E slot */
    u8 op;      /* an NDS_NATIVE_STATE_* effect, or NDS_NATIVE_BAKED_OP_* */
    u8 pad;
} NDSNativeBakedOp;

typedef struct NDSNativeBakedGroup
{
    const s16 *verts;
    const u32 *colors;
    const u16 *indices;
    u8 vertex_count;
    u8 triangle_count;
    u16 flags;          /* NDS_NATIVE_BAKED_GROUP_I_COVERAGE */
} NDSNativeBakedGroup;

/* The group draws an I4/I8 tile under a combine that reads TEXEL0 alpha
 * (Koffing's smog): its texture bakes the intensity as coverage
 * (sNdsRendererHardwareIntensityCoverage). */
#define NDS_NATIVE_BAKED_GROUP_I_COVERAGE 1u

typedef struct NDSNativeBakedRoot
{
    u32 root;
    u32 file_bytes;     /* the source payload the ops' offsets live in */
    u32 enddl_w0;       /* the root list's last word, a layout fingerprint */
    u16 asset_id;
    u16 gobj_kind;      /* GObj id the root is admitted for */
    u16 first_op;
    u16 op_count;
    u16 top_words;      /* root list length, ENDDL included */
    u8 material_slots;
    /* The vertices are stored >> vertex_shift (geometry past the v16 range
     * of +-2047 source units: the ending room's walls); the executor scales
     * the modelview's axes back by 1 << vertex_shift. */
    u8 vertex_shift;
} NDSNativeBakedRoot;

/* The ending's room table (nitro:/movies/room_baked.bin, header
 * NDS_NATIVE_BAKED_ROOM_MAGIC/VERSION): loaded into the ending's own heap at
 * its start and cleared when it ends (battleship_mvending.c), so no static
 * image carries it. NULL and 0 in every other scene. */
extern const NDSNativeBakedRoot *gNdsNativeBakedRoomRoots;
extern const NDSNativeBakedOp *gNdsNativeBakedRoomOps;
extern const NDSNativeBakedGroup *gNdsNativeBakedRoomGroups;
extern u32 gNdsNativeBakedRoomRootCount;

#endif
