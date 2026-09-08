/* Dream Land Bronto Burt efground native actor rows (new, review slice).
 *
 * Two Bronto variants (ef/efground.c:673-749) share one template: 3 live
 * joints from efGroundMakeEffect flags 0x4|USERDATA|0x1 plus
 * efGroundSetupEffectDObjs over DObjDesc_0x33B8
 * (104_StagePupupuFile2.c:1036-1040): A (tk1 TraRotRpyRSca root from
 * gcAddDObjForGObj, the live efGroundUpdatePhysics spawn pose) -> e0
 * (desc entry 0, NULL dl, carries the single MObj from mobjlink_0x31F4) ->
 * e1 (entry 1, DL_0x32C8 over Vtx_0x3288[4], no MObj). e0 is a live
 * NULL-dl intermediate (static offset -2113, 32.8, 0 plus joint anim), so
 * it stays a live joint (3, not 2).
 * e1 carries XObjs TraRotRpyR(27) + the billboard (kind72 0x48,
 * func_ovl0_800CAB48, lbcommon.c:1852, for lr +/-1; kind46 0x2E,
 * objdisplay.c:960, for lr +/-3) on the 0x4000 id; one shared DL_0x32C8[30] over
 * Vtx_0x3288[4] (2 tris).
 * The word-16 material hook resolves at runtime to e0's MObj sprite frame
 * mobj->sub.sprites[mobj->texture_id_curr] (objdisplay.c:1421-1430, the
 * spritelink_0x31F8 = {0x306C, 0x2EE4, 0x2D5C});
 * the palette (TLUT @ 0x2D38) is static. The adapter slot reads the live
 * frame index from e0; the executor emits the frame's exact image words.
 * The source matanim table has NULL at e0, so this index stays at frame 0.
 *
 * Texture lifetimes (no fallback): the palette + 3 frame images live in
 * ExternDataBank104, uploaded once by the shared texture cache at Dream
 * Land prepare and persistent for the stage; per submit the executor binds
 * the live frame's epoch through the same record path the fighter
 * production path uses. A short asset range or a frame index >= 3 fails
 * closed (FALSE) -- never a stale frame, never a solid color.
 *
 * Counts pinned by scripts/stages/generate_nds_native_ef_lakitu_bronto.py
 * --check against the bank payload and typed source. Do not hand-edit
 * counts: re-run it.
 */
#ifndef NDS_NATIVE_ACTOR_EF_BRONTO_H
#define NDS_NATIVE_ACTOR_EF_BRONTO_H

#include <nds/nds_renderer.h>

#define NDS_NATIVE_ACTOR_EF_BRONTO_JOINT_COUNT 3u
#define NDS_NATIVE_ACTOR_EF_BRONTO_DRAWABLE_COUNT 1u
#define NDS_NATIVE_ACTOR_EF_BRONTO_RUN_COUNT 1u
#define NDS_NATIVE_ACTOR_EF_BRONTO_TRIANGLE_COUNT 2u
#define NDS_NATIVE_ACTOR_EF_BRONTO_VERT_COUNT 4u
#define NDS_NATIVE_ACTOR_EF_BRONTO_CORNER_COUNT 6u
#define NDS_NATIVE_ACTOR_EF_BRONTO_FRAME_COUNT 3u
#define NDS_NATIVE_ACTOR_EF_BRONTO_STATE_SETUP_A 12u
#define NDS_NATIVE_ACTOR_EF_BRONTO_STATE_SETUP_B 2u
#define NDS_NATIVE_ACTOR_EF_BRONTO_STATE_FINISH 4u
/* Child XObj allowlist from efground.c:1326-1342 (tk2 = TraRotRpyR plus
 * the billboard on the 0x4000 id; root tk1 = TraRotRpyRSca). The billboard
 * is kind72 (0x48, func_ovl0_800CAB48) for lr +/-1 and kind46 (0x2E,
 * objdisplay.c:960) for lr +/-3 (efground.c:1332-1336, lr_bool set); same
 * DL either way, so the adapter slot admits both and dispatches live. */
#define NDS_NATIVE_ACTOR_EF_BRONTO_XOBJ_KIND_TRA_ROTRPYRSCA 28u
#define NDS_NATIVE_ACTOR_EF_BRONTO_XOBJ_KIND_TRA_ROTRPYR 27u
#define NDS_NATIVE_ACTOR_EF_BRONTO_XOBJ_KIND_BILLBOARD 72u
#define NDS_NATIVE_ACTOR_EF_BRONTO_XOBJ_KIND_BILLBOARD_46 46u
/* Material frame range: mobj->texture_id_curr indexes the 3-entry
 * spritelink_0x31F8; anything else is a corrupt topology, not a frame. */
#define NDS_NATIVE_ACTOR_EF_BRONTO_FRAME_MAX 2u

/* Hierarchy slot entry (renderer_adapter_matrix.c, patch-only) and the
 * route's commit wrapper (reloc_backend_movement.c, patch-only). Dream
 * Land is the base stage, so this entry needs no stage flag beyond the
 * hardware-triangles gate. See
 * builds/resume-20260906/ef-lakitu-bronto.patch.
 */
sb32 ndsRendererAdapterSubmitNativeEfBronto(void *root, void *cobj,
    u32 initial_geometry_mode, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeEfBronto(const void *packet_base,
    u32 packet_bytes, const NDSRendererNativeFighterHierarchy *hierarchy,
    const NDSRendererMatrix20p12 *drawable_mvp, u32 frame,
    u32 initial_geometry_mode, NDSRendererStats *stats);

extern volatile u32 gNdsStageGCDrawAllLoopEfBrontoDisplayCallbackCount;
extern volatile u32 gNdsStageGCDrawAllLoopEfBrontoTriangleCount;

#endif /* NDS_NATIVE_ACTOR_EF_BRONTO_H */
