/* Castle Lakitu efground native actor rows (new, review slice).
 *
 * Two Lakitu variants (ef/efground.c:27-106) share one template: 6 live
 * joints from efGroundMakeEffect flags 0x4|USERDATA|0x1 plus
 * efGroundSetupEffectDObjs over DObjDesc @ 0x4118
 * (106_StageCastleFile2.c:902-908): A (tk1 TraRotRpyRSca root from
 * gcAddDObjForGObj, the live efGroundUpdatePhysics spawn pose) -> e0
 * (desc entry 0, NULL dl, BB0 flight-sweep anim) -> e1 (entry 1, NULL dl,
 * C14 SCAX/SCAY 1.0<->1.1 flap anim) -> e2 (entry 2, DL_0x3F20 prelude
 * calling the sub quad DL_0x3F80), e0 -> e3 (entry 3, DL_0x3FF8), e0 -> e4
 * (entry 4, gap DL at file 0x4070). The three drawables carry XObjs
 * TraRotRpyR(27) plus the billboard (kind72 0x48, func_ovl0_800CAB48,
 * lbcommon.c:1852, for lr +/-1; kind46 0x2E, objdisplay.c:960, for
 * lr +/-3) on the 0x4000 ids; no MObj (o_mobjsub 0).
 *
 * e1 is a live NULL-dl intermediate: its C14 scale flap feeds e2's world
 * matrix, so it stays a live joint (6, not 5).
 *
 * Counts pinned by scripts/stages/generate_nds_native_ef_lakitu_bronto.py
 * --check against the bank payload and typed source. Do not hand-edit
 * counts: re-run it.
 *
 * Texture lifetimes (no fallback): the three palette+image epochs live in
 * ExternDataBank106, uploaded once into the shared texture cache by the
 * executor's own bounded cold pass at first draw and persistent for the
 * stage; per submit the executor re-records each drawable's epoch, proves
 * all three resident with a pure reverify (a live upload can evict, so
 * three binds alone never imply residency), and binds them by name for
 * emission. A short asset range, a failed upload, or a reverify miss fails
 * closed (FALSE) -- never a stale epoch, never a solid color, never a
 * partial actor.
 */
#ifndef NDS_NATIVE_ACTOR_EF_LAKITU_H
#define NDS_NATIVE_ACTOR_EF_LAKITU_H

#include <nds/nds_renderer.h>

#define NDS_NATIVE_ACTOR_EF_LAKITU_JOINT_COUNT 6u
#define NDS_NATIVE_ACTOR_EF_LAKITU_DRAWABLE_COUNT 3u
#define NDS_NATIVE_ACTOR_EF_LAKITU_RUN_COUNT 3u
#define NDS_NATIVE_ACTOR_EF_LAKITU_TRIANGLE_COUNT 6u
#define NDS_NATIVE_ACTOR_EF_LAKITU_VERT_COUNT 12u
#define NDS_NATIVE_ACTOR_EF_LAKITU_CORNER_COUNT 18u
#define NDS_NATIVE_ACTOR_EF_LAKITU_STATE_PRELUDE 8u
#define NDS_NATIVE_ACTOR_EF_LAKITU_STATE_EPOCH 7u
#define NDS_NATIVE_ACTOR_EF_LAKITU_STATE_FINISH 4u
#define NDS_NATIVE_ACTOR_EF_LAKITU_TEX_EPOCH_COUNT 3u
/* Child XObj allowlist from efground.c:1326-1342 (tk2 = TraRotRpyR plus
 * the billboard on 0x4000 ids; root tk1 = TraRotRpyRSca). The billboard is
 * kind72 (0x48, func_ovl0_800CAB48) for lr +/-1 and kind46 (0x2E,
 * objdisplay.c:960) for lr +/-3 (efground.c:1332-1336, lr_bool set); same
 * DLs either way, so the adapter slot admits both and dispatches live. */
#define NDS_NATIVE_ACTOR_EF_LAKITU_XOBJ_KIND_TRA_ROTRPYRSCA 28u
#define NDS_NATIVE_ACTOR_EF_LAKITU_XOBJ_KIND_TRA_ROTRPYR 27u
#define NDS_NATIVE_ACTOR_EF_LAKITU_XOBJ_KIND_BILLBOARD 72u
#define NDS_NATIVE_ACTOR_EF_LAKITU_XOBJ_KIND_BILLBOARD_46 46u

/* Hierarchy slot entry (renderer_adapter_matrix.c, patch-only) and the
 * route's commit wrapper (reloc_backend_movement.c, patch-only). Both live
 * behind NDS_P2_STAGE_CASTLE. See
 * builds/resume-20260906/ef-lakitu-bronto.patch.
 */
sb32 ndsRendererAdapterSubmitNativeEfLakitu(void *root, void *cobj,
    u32 initial_geometry_mode, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeEfLakitu(const void *packet_base,
    u32 packet_bytes, const NDSRendererNativeFighterHierarchy *hierarchy,
    const NDSRendererMatrix20p12 *drawable_mvps,
    u32 initial_geometry_mode, NDSRendererStats *stats);

extern volatile u32 gNdsStageGCDrawAllLoopEfLakituDisplayCallbackCount;
extern volatile u32 gNdsStageGCDrawAllLoopEfLakituTriangleCount;

#endif /* NDS_NATIVE_ACTOR_EF_LAKITU_H */
