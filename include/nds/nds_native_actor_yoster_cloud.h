/* Yoshi Island cloud-platform native actor rows (new, review slice).
 *
 * Shared template, three live instances. Per cloud GObj the source builds
 * 7 live joints (gryoster.c:199-245): root + 3 template mids from
 * dStageYosterFile3_DObjDesc_0x0100[5] (4 live, terminator id 18) + 3
 * drawable children of llGRYosterMapCloudDisplayList (DL_0x0580[29] over
 * Vtx_0x0540[2]+Vtx_0x0560[2] = 4 source verts, 2 shared triangles).
 * Each drawable carries one MObj from the shared MObjSub_0x04C0 with XObjs
 * Tra + kind48 (gryoster.c:242-244); material alpha runs the Solid
 * (0x0674, 0->255 over 100) / Evaporate (0x0694, 255->0 over 100)
 * PRIMCOLOR scripts; root translate is the yakumono collision pose every
 * tick (gryoster.c:131-134); line ids {1,2,3} (gryoster.c:15).
 *
 * Counts pinned by scripts/stages/generate_nds_native_yoster_clouds.py
 * --check against the bank payload and typed source. Do not hand-edit
 * counts: re-run it.
 */
#ifndef NDS_NATIVE_ACTOR_YOSTER_CLOUD_H
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_H

#include <nds/nds_renderer.h>

#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_JOINT_COUNT 7u
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_TEMPLATE_LIVE 4u
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_DRAWABLE_COUNT 3u
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_RUN_COUNT 3u
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_TRIANGLE_COUNT 6u
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_VERT_COUNT 4u
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_CORNER_COUNT 6u
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_DL_WORDS 29u
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_SRC_VERTS 4u
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_MOBJ_PER_DRAWABLE 1u
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_STATE_SETUP_A 12u
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_STATE_SETUP_B 4u
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_STATE_FINISH 5u
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_MAT_ANIM_COUNT 2u
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_MAT_ANIM_WORDS 5u
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_MAT_FRAME_MAX 100u
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_TEX_IMG_OFFSET 0x2B8u
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_TEX_IMG_SIZE 512u
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_TILE_EXTENT 252u
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_TEX_FMT 4u
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_TEX_SIZ 0u
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_SLAB_BYTES 309u
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_LINE_ID_0 1u
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_LINE_ID_1 2u
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_LINE_ID_2 3u
/* Child XObj allowlist from gryoster.c:242-243. */
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_XOBJ_KIND0 18u
#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_XOBJ_KIND1 48u

/* Hierarchy slot entry (renderer_adapter_matrix.c, patch-only) and the
 * route's commit wrapper (reloc_backend_movement.c, patch-only). The
 * imported Yoster TU owns the per-cloud GObj pointers; both live behind
 * NDS_P2_STAGE_YOSTER. See builds/resume-20260905/yoster-clouds.patch.
 */
sb32 ndsRendererAdapterSubmitNativeYosterCloud(void *root, void *cobj,
    u32 initial_geometry_mode, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeYosterCloud(const void *packet_base,
    u32 packet_bytes, const NDSRendererNativeFighterHierarchy *hierarchy,
    const NDSRendererMatrix20p12 *drawable_mvps,
    u32 prim_alpha_0, u32 prim_alpha_1, u32 prim_alpha_2,
    u32 initial_geometry_mode, NDSRendererStats *stats);
/* Per-cloud GObj accessor, implemented beside the imported Yoster TU
 * (patch-only): index 0..2 selects gGRCommonStruct.yoster.clouds[].gobj.
 */
void *ndsGRYosterCloudGObj(u32 index);
extern volatile u32 gNdsStageGCDrawAllLoopYosterCloudDisplayCallbackCount;
extern volatile u32 gNdsStageGCDrawAllLoopYosterCloudTriangleCount;

#endif /* NDS_NATIVE_ACTOR_YOSTER_CLOUD_H */
