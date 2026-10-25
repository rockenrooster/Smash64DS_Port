/* Renderer adapter, the matrix front: the stage preview camera, fighter
 * material caches and the native stage's assets. A slice of
 * reloc_backend_renderer_dl.c; its slices follow it there: matrix_math,
 * matrix_world, matrix_owner. */
#include <sys/matrix.h>
#if NDS_P2_STAGE_SECTOR
#include <sys/interp.h>
#include <sys/objanim.h>
#include <sys/vector.h>
#endif

#include <nds/nds_itcm_placement.h>
#include <nds/nds_effects.h>
#include <nds/nds_fighter_matrix_index.h>
#include <nds/renderer_fighter_lean.h>
#include <nds/nds_r2_camera_fixed.h>
#include <nds/nds_fcmp.h>
#include <nds/nds_r2_collision_mtx.h>
#include <nds/nds_r2_hwmath_unit.h>
#include <nds/nds_fixed_convert.h>
#include <nds/nds_fox_gun.h>
#include <nds/nds_ifcommon_oam.h>
#if NDS_P2_STAGE_JUNGLE
#include <nds/generated/nds_native_actor_tarucann.generated.h>
#endif
/* YOSTER-CLOUDS: adapter slot beside SubmitNativeTaruCann. */
#if NDS_P2_STAGE_YOSTER
#include <nds/nds_native_actor_yoster_cloud.h>
#endif
#include <ft/ftdata_file_slots.h>
#include <nds/nds_frontend_overlay.h>


#include <nds/timers.h>

/* Task 86: see ndsRendererMatrixCopy20p12 in include/nds/nds_renderer.h. Gated
 * so the A/B is one flag, per the Task 79 rule. The definition sits outside the
 * NDS_RENDERER_HW_TRIANGLES block because ndsRendererAdapterPrepareInitialMatrices
 * uses MTXCOPY on both sides of that switch; with it inside, the canonical
 * software target did not compile at all. Folding the triangle flag into the
 * condition keeps the hardware ROM's expansion exactly as it was. */
#define MTXCOPY(d, s) ndsRendererMatrixCopy20p12((d), (s))

#define NDS_RENDERER_ADAPTER_MTX_FRAC_BITS 12
#define NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX 32u
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
/* LAB ONLY: the stage-DL census's split of PrepareInitialMatrices (camera, world,
 * compose, recalc) and the persistent world build's uncached fallbacks. */
extern u32 cpuGetTiming(void);
u32 gNdsLabPimAcc[5];
/* LAB ONLY (item draw split): [0] item draw total, [1] display-proc capture,
 * [2] tree submit, [3] traversal begin/end, [4] fast-lane PIM, [5] fast-lane
 * prep, [6] executor, [7] fast-lane tail, [8] fast-lane item roots, [9] 0x52
 * attach ticks, [10] 0x52 attach builds, [11] offscreen test, [15] draws. */
__attribute__((used)) volatile u32 gNdsLabItemAcc[16];
#define NDS_LAB_PIM_MARK(v) ((v) = cpuGetTiming())
#define NDS_LAB_PIM_ADD(i, v) (gNdsLabPimAcc[(i)] += cpuGetTiming() - (v))
#else
#define NDS_LAB_PIM_MARK(v) ((void)0)
#define NDS_LAB_PIM_ADD(i, v) ((void)0)
#endif

#define NDS_RENDERER_ADAPTER_MATERIAL_MOBJ_MAX 64u
/* Mario/Fox top out at four MObjs on one selected model root. Luigi root 4
 * carried six when this bound was introduced. Purin's Win2/CSS body root in
 * asset 0x14a carries nine live MObjs, so the complete
 * shipping roster needs nine. This is renderer capacity, not a fighter
 * semantic: size the native-owner workspace for the largest admitted source
 * root rather than rejecting valid native geometry. */
#define NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX 9u
#define NDS_RENDERER_ADAPTER_CAMERA_CACHE_COUNT 4u
#define NDS_RENDERER_ADAPTER_DOBJ_WORLD_CACHE_COUNT 128u
#define NDS_RENDERER_ADAPTER_DOBJ_WORLD_INDEX_COUNT 256u
#define NDS_RENDERER_ADAPTER_DOBJ_WORLD_INDEX_MASK \
    (NDS_RENDERER_ADAPTER_DOBJ_WORLD_INDEX_COUNT - 1u)
#define NDS_RENDERER_ADAPTER_STAGE_WORLD_CACHE_COUNT 64u
#define NDS_RENDERER_ADAPTER_STAGE_WORLD_SLOT_BASE \
    (NDS_RENDERER_ADAPTER_DOBJ_WORLD_CACHE_COUNT - \
     NDS_RENDERER_ADAPTER_STAGE_WORLD_CACHE_COUNT)
#define NDS_RENDERER_ADAPTER_STAGE_WORLD_INDEX_COUNT 128u
#define NDS_RENDERER_ADAPTER_STAGE_WORLD_INDEX_MASK \
    (NDS_RENDERER_ADAPTER_STAGE_WORLD_INDEX_COUNT - 1u)
#define NDS_RENDERER_ADAPTER_G_TX_RENDERTILE 0u
#define NDS_RENDERER_ADAPTER_G_TX_WRAP 0u
#define NDS_RENDERER_ADAPTER_G_TX_NOMASK 0u
#define NDS_RENDERER_ADAPTER_G_TX_NOLOD 0u
#define NDS_RENDERER_ADAPTER_G_ON 1u
#define NDS_RENDERER_ADAPTER_G_MW_LIGHTCOL 0x0au
#define NDS_RENDERER_ADAPTER_G_MWO_A_LIGHT_1 0x00u
#define NDS_RENDERER_ADAPTER_G_MWO_B_LIGHT_1 0x04u
#define NDS_RENDERER_ADAPTER_G_MWO_A_LIGHT_2 0x18u
#define NDS_RENDERER_ADAPTER_G_MWO_B_LIGHT_2 0x1cu
#define NDS_RENDERER_ADAPTER_G_TX_LDBLK_MAX_TXL 2047u
/* dLBCommonFuncMatrixList is consumed as syMtxProcess pairs; kind 0x4B maps
 * to lbCommonFighterPartsFuncMatrix in BattleShip. */
#define NDS_RENDERER_ADAPTER_FIGHTER_PARTS_MTX_KIND 0x4Bu
/* wpmariofireball.c specifies hexadecimal 0x47 (decimal 71), which maps to
 * func_ovl0_800CA5C8 rather than enum nGCMatrixKind47 (decimal 47). */
#define NDS_RENDERER_ADAPTER_MVP_RECALC_RPY_0X47_KIND 0x47u
/* wpsamusbomb.c specifies hexadecimal 0x46 (decimal 70), which maps to
 * func_ovl0_800CA194.  Like the 0x47 callback it emits gSPMvpRecalc and rewrites
 * the composed orientation while preserving the translation row. */
#define NDS_RENDERER_ADAPTER_MVP_RECALC_Z_0X46_KIND 0x46u
/* ef/efground.c:1336 specifies hexadecimal 0x48 (decimal 72), which maps to
 * func_ovl0_800CAB48 (dLBCommonFuncMatrixList index 12, lbcommon.c:1852).
 * Every efground actor's billboard drawable (lr +/-1) carries it beside
 * its TraRotRpyR XObj. Like the 0x46/0x47 callbacks it rewrites the composed
 * orientation while preserving the translation row, with the source's
 * gGCScaleX accumulation threaded per tree. Listed here (not just in the
 * actor slot) so ndsRendererAdapterBuildDObjLocalMatrix skips it exactly
 * like the other recalc kinds, leaving the drawable's plain TraRotRpyR
 * local for the actor billboard to complete. */
#define NDS_RENDERER_ADAPTER_EF_GROUND_BILLBOARD_KIND 72u
/* nGCMatrixKindRecalcRotRpyRSca, decimal 44 -- dEFManagerShieldEffectDesc's
 * SECOND transform struct, i.e. the transform every non-root node of the shield
 * tree carries (efmanager.c:472). Despite the enum name it applies NO rotation:
 * gcPrepDObjMatrix case 44 (objdisplay.c:876) writes a pure scaled PERSPECTIVE
 * block into sGCMatrixMvpF, emits gSPMvpRecalc plus the gMoveWd pairs for rows
 * 0-2 ONLY, and `continue`s WITHOUT a gSPMatrix -- so the accumulated MVP's
 * orientation is REPLACED by a screen-aligned block while its translation row,
 * which is where the parent 0x4F joint attachment lives, survives untouched.
 * That is a billboard, and it is the whole reason the shield is a circle facing
 * the player rather than a quad wearing the fighter's yaw. */
#define NDS_RENDERER_ADAPTER_MVP_RECALC_PERSP_SCA_KIND 44u
/* dLBCommonFuncMatrixList kind 0x4C maps to gmCameraLookAtFuncMatrix. */
#define NDS_RENDERER_ADAPTER_GM_CAMERA_MTX_KIND 0x4Cu

/* Set while the stage select's 3D preview owns the stage packet
 * (ndsRendererAdapterSetStagePreviewLayers, renderer_adapter_stage.c). Its
 * camera is the default one (gcAddCameraMatrixSets: PerspFastF, then
 * LookAt): the same perspective and look-at the battle's 0x4C camera
 * carries, so the stage paths that ask for that camera accept it then. */
static u32 sNdsRendererAdapterStagePreviewMode;

static sb32 ndsRendererAdapterIsStagePreviewCamera(const CObj *cobj)
{
    u32 seen = 0u;
    u32 i;

    if ((sNdsRendererAdapterStagePreviewMode == 0u) || (cobj == NULL))
    {
        return FALSE;
    }
    for (i = 0u; i < (u32)cobj->xobjs_num; i++)
    {
        if (cobj->xobjs[i] == NULL)
        {
            continue;
        }
        if (cobj->xobjs[i]->kind == nGCMatrixKindPerspFastF)
        {
            seen |= 1u;
        }
        else if (cobj->xobjs[i]->kind == nGCMatrixKindLookAt)
        {
            seen |= 2u;
        }
    }
    return (seen == 3u) ? TRUE : FALSE;
}

/* objdisplay.c:2950-2983, camera xobj kinds 12-17: the view of 6/7, 8/9 or
 * 10/11 together with the LookAt the RSP's texgen reads. */
static void ndsRendererAdapterReflectCameraView(const CObj *cobj, u32 kind,
                                                Mtx *mtx, LookAt *look_at)
{
    if (kind <= 13u)
    {
        syMatrixLookAtReflect(mtx, look_at,
                              cobj->vec.eye.x, cobj->vec.eye.y,
                              cobj->vec.eye.z, cobj->vec.at.x,
                              cobj->vec.at.y, cobj->vec.at.z,
                              cobj->vec.up.x, cobj->vec.up.y,
                              cobj->vec.up.z);
    }
    else
    {
        syMatrixModLookAtReflect(mtx, look_at,
                                 cobj->vec.eye.x, cobj->vec.eye.y,
                                 cobj->vec.eye.z, cobj->vec.at.x,
                                 cobj->vec.at.y, cobj->vec.at.z,
                                 cobj->vec.up.x, 0.0F,
                                 (kind <= 15u) ? 1.0F : 0.0F,
                                 (kind <= 15u) ? 0.0F : 1.0F);
    }
}

#if NDS_1P_INTRO_BAKE
/* Set by the 1P intro's still bake around its own draw only
 * (battleship_sc1pintro.c, in the frontend overlay): the reset LookAt under a
 * camera whose view the bake moved into the projection, into world space. */
void (*gNdsAdapterLookAtHook)(LookAt *look_at);
#endif

const LookAt *ndsRendererAdapterCurrentLookAt(void)
{
    /* BattleShip Interpreter::SpReset initializes these two coefficient
     * directions. Ordinary cameras (including VS CSS's kinds 3/6) do not
     * emit gSPLookAt, so they retain that state, not GMCamera's last frame.
     * Reading the zero-initialized battle vectors here used to abort Link's
     * shield texgen and every remaining native body root on first CSS entry. */
    static const LookAt reset_look_at = {
        .l = { { .l = { .dir = { 0, 127, 0 } } },
               { .l = { .dir = { 127, 0, 0 } } } }
    };
    static LookAt reflect_look_at;
    CObj *cobj = (gGCCurrentCamera != NULL) ?
        CObjGetStruct(gGCCurrentCamera) : NULL;
    u32 i;

    if (cobj != NULL)
    {
        for (i = 0u; i < (u32)cobj->xobjs_num; i++)
        {
            if ((cobj->xobjs[i] != NULL) &&
                (cobj->xobjs[i]->kind == NDS_RENDERER_ADAPTER_GM_CAMERA_MTX_KIND))
            {
                return ndsR2CameraCurrentLookAt();
            }
            /* A Reflect camera emits its own gSPLookAtX/Y. */
            if ((cobj->xobjs[i] != NULL) && (cobj->xobjs[i]->kind >= 12) &&
                (cobj->xobjs[i]->kind <= 17))
            {
                Mtx mtx;

                ndsRendererAdapterReflectCameraView(
                    cobj, (u32)cobj->xobjs[i]->kind, &mtx, &reflect_look_at);
                return &reflect_look_at;
            }
        }
    }
#if NDS_1P_INTRO_BAKE
    if (gNdsAdapterLookAtHook != NULL)
    {
        static LookAt baked_look_at;

        baked_look_at = reset_look_at;
        gNdsAdapterLookAtHook(&baked_look_at);
        return &baked_look_at;
    }
#endif
    return &reset_look_at;
}

/* dLBCommonFuncMatrixList kind 0x4F maps to func_ovl0_800C994C: the DObj's local
 * matrix IS the world matrix of the joint it is bound to. See
 * ndsRendererAdapterBuildJointAttachMtx. */
#define NDS_RENDERER_ADAPTER_JOINT_ATTACH_MTX_KIND 0x4Fu

/* EFCommonEffects1 file 83, ImpactWave Vtx block @ 0x7B08 and TRI2 commands
 * 20..27 of the display list @ 0x7C28. These are immutable source geometry,
 * compiled AOT instead of decoded from N64 VTX/TRI2 commands every draw. Source
 * Vtx alpha bytes are zero; ndsRendererDecodeInputVertex maps that legacy zero
 * to 0xFF, so the typed native copy stores the post-decode value directly. */
static const NDSRendererInputVertex sNdsImpactWaveVertices[18] = {
    {  125, 62, -125, 14976, 1024, 255, 255, 255, 255 },
    {   97,  0,  -97, 14976,    0, 255, 255, 255, 255 },
    {  177, 62,    0, 13824, 1024, 255, 255, 255, 255 },
    {  137,  0,    0, 13824,    0, 255, 255, 255, 255 },
    {  125, 62,  125, 12672, 1024, 255, 255, 255, 255 },
    {   97,  0,   97, 12672,    0, 255, 255, 255, 255 },
    {    0,  0,  137, 11520,    0, 255, 255, 255, 255 },
    {    0, 62,  177, 11520, 1024, 255, 255, 255, 255 },
    { -125, 62,  125, 10368, 1024, 255, 255, 255, 255 },
    {  125, 62, -125,  5760, 1024, 255, 255, 255, 255 },
    {  -97,  0,   97, 10368,    0, 255, 255, 255, 255 },
    {   97,  0,  -97,  5760,    0, 255, 255, 255, 255 },
    { -177, 62,    0,  9216, 1024, 255, 255, 255, 255 },
    {    0,  0, -137,  6912,    0, 255, 255, 255, 255 },
    {    0, 62, -177,  6912, 1024, 255, 255, 255, 255 },
    { -137,  0,    0,  9216,    0, 255, 255, 255, 255 },
    {  -97,  0,  -97,  8064,    0, 255, 255, 255, 255 },
    { -125, 62, -125,  8064, 1024, 255, 255, 255, 255 }
};

static const u8 sNdsImpactWaveTriangles[16 * 3] = {
    17,16,15, 16,17,14,
    13,16,14, 12,17,15,
    11,13,14, 12,15,10,
     9,11,14,  8,12,10,
     7, 8,10,  6, 7,10,
     7, 6, 5,  4, 7, 5,
     4, 5, 3,  2, 4, 3,
     2, 3, 1,  0, 2, 1
};

static sb32 sNdsRendererAdapterImpactWaveNativeActive;
static u32 sNdsRendererAdapterImpactWaveVariant;
/* The procedural visual templates. Unflagged and unconditional: there is no
 * second renderer behind this owner, so a selector would only choose between
 * drawing and recording a failure. */
static sb32 sNdsRendererAdapterVisualEffectNativeActive;
static u32 sNdsRendererAdapterVisualEffectTemplate;
static sb32 sNdsRendererAdapterRebirthHaloNativeActive;
static sb32 sNdsRendererAdapterRebirthHaloSkipSecondChildList;
/* Kind 0x50 is 0x4F's NEIGHBOUR IN THE TABLE AND A DIFFERENT CALLBACK.
 * sGCMatrixFuncList entries are {proc_diff, proc_same} PAIRS (objdisplay.h:20),
 * so kind - 66 selects pair 14 of dLBCommonFuncMatrixList = func_ovl0_800C99CC
 * (lbcommon.c:1461), not pair 13's func_ovl0_800C994C. It is a PURE TRANSLATION
 * to the bound joint's world position and writes no gGCScaleX at all:
 *
 *     attach = dobj->user_data.p;
 *     Vec3f p = { 0, 0, 0 };
 *     gmCollisionGetFighterPartsWorldPosition(attach, &p);
 *     syMatrixTra(mtx, p.x, p.y, p.z);
 *
 * dEFManagerRebirthHaloEffectDesc's root uses it (efmanager.c:1655) and
 * efManagerRebirthHaloMakeEffect stores fp->joints[nFTPartsJointTopN] there
 * (efmanager.c:6014). Do NOT confuse it with enum nGCMatrixKind50, which is
 * DECIMAL 50; the 0x47 constant above records the same trap. Without this case
 * 0x50 fell through to `default:` and the fallback identity, which is why the
 * respawn platform was nowhere near the respawning fighter. */
#define NDS_RENDERER_ADAPTER_JOINT_ATTACH_TRA_MTX_KIND 0x50u
/* dLBCommonFuncMatrixList pair 16 (kind 0x52) is func_ovl0_800C9F70,
 * the source held-item attachment used by itMainSetFighterHold.  Do not treat
 * it as enum value decimal 52: custom matrix kinds are dispatched from 66. */
#define NDS_RENDERER_ADAPTER_ITEM_ATTACH_MTX_KIND 0x52u
/* dLBCommonFuncMatrixList pair 17 (kind 0x53) is Sector's US-region Arwing
 * matrix callback, grSectorArwingLaser3DFuncMatrix. */
#define NDS_RENDERER_ADAPTER_SECTOR_ARWING_MTX_KIND 0x53u
/* dLBCommonFuncMatrixList pair 3 (kind 0x45, decimal 69) is
 * lbCommonRotScaFuncMatrix (lbcommon.c:1686), which calls lbCommonMatrixRotSca
 * with the DObj's rotate/scale and writes a ZERO translation row. It is an
 * ordinary local affine -- it returns 0, emits no DL and rewrites no MVP -- so
 * it must NOT join ndsRendererAdapterIsMvpRecalcKind.
 *
 * Its element arithmetic is syMatrixTraRotRpyRSca's (matrix.c:1086-1141) term
 * for term, on the same gSYSinTable index and the same 256x row scales, with
 * tx/ty/tz forced to 0: lbcommon.c:678-713 writes COMBINE_INTEGRAL(0, 0) into
 * m[1][2] and COMBINE_INTEGRAL(0, 0x10000) into m[1][3], which is exactly what
 * matrix.c:1133-1140 produces for tx=ty=tz=0 (FTOFIX32(1.0F) == 0x10000).
 *
 * Do NOT confuse this with enum nGCMatrixKind45, which is DECIMAL 45 and is
 * handled by ndsRendererAdapterBuildRecalcLocalMtx; the 0x47/0x50 comments
 * above record the same trap.
 *
 * Without this case the kind fell through to `default:` and
 * ndsRendererAdapterBuildDObjFallbackMtx, which is TraRotRpyRSca WITH the
 * DObj's translation. dEFManagerDamageSlashEffectDesc's root carries
 * { 0x28, 0x45 } (efmanager.c:89-91) and efManagerDamageSlashMakeEffect writes
 * the WORLD-SPACE collision contact into that same root's translate
 * (efmanager.c:2500), so the contact was applied twice: once by the kind-0x28
 * camera billboard's translation row and again by the fallback. The displacement
 * is the billboard basis times the contact vector, so it grows with distance
 * from the world origin and swings with the camera -- Link's slash damage
 * effect drawing away from the hit.
 *
 * The other source users of 0x45 split into two shapes, both repaired by the
 * same case: ShockSmall and StarRodSpark repeat DamageSlash's { 0x28, 0x45 }
 * ROOT (efmanager.c:119-120, 239-240), so they double-translated identically;
 * StarRodSpark, DamageFlySparks and FireSpark additionally give 0x45 to every
 * NON-root node on its own (efmanager.c:246, 276, 396), where the fallback was
 * adding each child's DObjDesc translate to a subtree the source leaves
 * untranslated. Custom kinds 0x44, 0x49 and 0x51 are still unhandled and
 * still take that fallback; they carry other rows' effects (MBallThrown,
 * MBallRays, DamageFlyMDust, FireSpark's root, itmain's hold) and are
 * deliberately not touched here. 0x4A is handled below. */
#define NDS_RENDERER_ADAPTER_ROT_SCA_MTX_KIND 0x45u
/* dLBCommonFuncMatrixList pair 8 (kind 0x4A, decimal 74) is func_ovl0_800CB2F0
 * (lbcommon.c:2011-2022), and it BUILDS NO MATRIX. It returns 1, and
 * gcPrepDObjMatrix `continue`s past the gSPMatrix of a custom proc that does
 * (objdisplay.c:1167-1170). Its whole effect is one write to the DObj's OWN
 * rotate.z, which the kind-46 billboard after it in the XObj list reads:
 *
 *     attach = dobj->user_data.p;
 *     func_ovl2_800EDBA4(attach);
 *     dobj->rotate.z = (ftGetParts(attach)->mtx_translate[0][2] > 0.0F) ?
 *                      attach->rotate.x : -attach->rotate.x;
 *
 * Its one source user is dEFManagerYoshiEggEscapeEffectDesc, { 0x50, 0x4A,
 * 0x2E } (efmanager.c:1375-1402): 0x50 puts the egg on fp->joints[5]
 * (efManagerYoshiEggEscapeMakeEffect, efmanager.c:5453), 0x4A hands that
 * joint's pitch to the egg as screen-plane roll, and kind 46
 * (objdisplay.c:960-1004) turns the camera-facing quad by it. joints[5] is
 * Yoshi's body, the first drawn YoshiModel JointTree entry, and RollF/RollB
 * (FTYoshiAnim059/060) sweep its rotate.x by +4pi/-4pi, so the source egg
 * turns twice per roll. [0][2] is the Z of the joint's world X axis, i.e. his
 * facing, so the turn reads as rolling forward on either side.
 *
 * Without this case the kind fell to `default:`, whose TRS fallback added a
 * spare 1.5 scale that kind 46 then overwrote and never wrote rotate.z: the
 * escape egg drew in the right place at the right size and never turned
 * (owner, docs/BUGS.md, Yoshi). */
#define NDS_RENDERER_ADAPTER_JOINT_PITCH_TO_ROLL_KIND 0x4Au

volatile u32 gNdsRendererAdapterSectorArwingMtxCount __attribute__((used));
/* Engagement proof for the kind-0x45 case above: a repair that leaves this at
 * zero did not run. */
volatile u32 gNdsRendererAdapterCustom45AppliedCount __attribute__((used));
/* Kind 0x4A witness, one cache line; only Yoshi's escape egg carries 0x4A.
 *   [0] engagement: the joint's pitch was copied into a DObj's roll.
 *   [1] the proof: a kind-46 MVP rewrite consumed a NON-ZERO roll that 0x4A
 *       wrote in the same prepare, i.e. an escape egg drawn turned.
 *   [2] f32 bits of the last roll consumed; it must sweep during a roll.
 * Cleaned to RAM on every write: the cache-accurate melonDS fork's gdb stub
 * reads RAM, not a dirty dcache line, and a hot counter read through it can
 * sit at 0 while the CPU's copy counts. Egg draws only, so the clean is free. */
volatile u32 gNdsRendererAdapterCustom4AWitness[3]
    __attribute__((used, aligned(32)));
static const DObj *sNdsRendererAdapterCustom4ARollDObj;
/* 0x45 as the SOLE transform on a DObj: kept on the pre-L01 fallback on
 * purpose. Non-zero means the three spark-family children are still taking
 * dobj->translate, i.e. this repair did not touch them. */
volatile u32 gNdsRendererAdapterCustom45SoloCount __attribute__((used));
volatile u32 gNdsSectorArwingBasisDecline __attribute__((used));

#if NDS_P2_STAGE_SECTOR
extern void lbCommonCross3D(Vec3f *a, Vec3f *b, Vec3f *out);
extern void guMtxF2L(f32 mf[4][4], Mtx *m);
extern f32 ndsGRSectorArwingTargetX(void);
extern s32 ndsGRSectorArwingLaserCount(void);
extern DObj *ndsGRSectorArwingMapDObj11(void);
sb32 ndsTraIDescUsable(DObj *dobj, const AObj *aobj, u32 site);
#endif

extern void func_ovl2_800ED490(Mtx44f dst, Mtx44f lhs, Mtx44f rhs);
extern Vec2f dFTDisplayMainShufflePositions[][4];
#if NDS_P2_JOINT_RESIDENT
/* src/port/nds_p2_hurtbox_reject.c: fighter joint worlds resident in fixed
 * point for the latch epoch (owner ruling D13). */
int ndsP2JointItemAttach(DObj *attach, const Vec2f *shuffle, s32 m[4][4]);
#endif

#if defined(__arm__)
#define NDS_RENDERER_ADAPTER_FIGHTER_MATRIX_CODE \
    __attribute__((noinline, optimize("O3"), target("arm")))
#else
#define NDS_RENDERER_ADAPTER_FIGHTER_MATRIX_CODE \
    __attribute__((noinline, optimize("O3")))
#endif

static const Gfx sNdsRendererAdapterEmptySegmentEDL[1] = {
    { { NDS_FIGHTER_DL_OP_ENDDL << 24, 0u } }
};


/* ftDisplayMainCalcFogColor: a shaded fighter's colour animation blends the
 * animation's fog colour with the team shade (same integer steps). */
static u32 ndsRendererAdapterFighterShadeFlash(const FTStruct *fp)
{
    const SYColorRGBA *shade = &fp->attr->shade_color[fp->shade - 1];
    const GMColKeys *ck = &fp->colanim.color1;
    s32 shade_base = ((0xFF - shade->a) * (0xFF - ck->a)) / 0xFF;
    s32 temp_color, red, green, blue, alpha;

    if (shade_base == 0xFF)
    {
        red = ck->r;
        green = ck->g;
        blue = ck->b;
        alpha = ck->a;
    }
    else
    {
        shade_base = 0xFF - shade_base;
        temp_color = (((ck->r - fp->shade_color.r) * ck->a) / 0xFF) + fp->shade_color.r;
        red = (temp_color * 0xFF) / shade_base;
        alpha = (red != 0) ? ((temp_color * 0xFF) / red) :
                             (((shade_base - temp_color) * 0xFF) / 0xFF);
        temp_color = (((ck->g - fp->shade_color.g) * ck->a) / 0xFF) + fp->shade_color.g;
        green = (temp_color * 0xFF) / shade_base;
        temp_color = (((ck->b - fp->shade_color.b) * ck->a) / 0xFF) + fp->shade_color.b;
        blue = (temp_color * 0xFF) / shade_base;
    }
    if (fp->attr->fog_color.r != 0xFF)
    {
        alpha = (fp->attr->fog_color.r * alpha) / 0xFF;
    }
    return ((u32)(u8)red << 24) | ((u32)(u8)green << 16) |
           ((u32)(u8)blue << 8) | (u32)(u8)alpha;
}

static u32 ndsRendererAdapterFighterColorModulate(const FTStruct *fp)
{
    if ((fp != NULL) && (fp->colanim.is_use_color1 != FALSE))
    {
        const GMColKeys *color = &fp->colanim.color1;

        NDS_DIAG(gNdsTask39FxFlashDrawCount++);
        ndsTask39EffectsEngage(NDS_TASK39_FX_ENGAGED_FLASH);
        if ((fp->shade != 0) && (fp->attr != NULL))
        {
            return ndsRendererAdapterFighterShadeFlash(fp);
        }
        return ((u32)color->r << 24) | ((u32)color->g << 16) |
               ((u32)color->b << 8) | color->a;
    }
    /* A team's second and third fighter of one kind (ftManagerMakeFighter's
     * shade) draw through the source's G_RM_FOG_PRIM_A with the shade colour
     * as fog (ftDisplayMainDecideFogColor): the same blend toward it by its
     * alpha. */
    if ((fp != NULL) && (fp->shade != 0) && (fp->shade <= 3) && (fp->attr != NULL))
    {
        const SYColorRGBA *shade = &fp->attr->shade_color[fp->shade - 1];

        return ((u32)shade->r << 24) | ((u32)shade->g << 16) |
               ((u32)shade->b << 8) | shade->a;
    }
    return 0u;
}

static NDSRendererNativeMaterial
    sNdsRendererAdapterNativeOwnerMaterials[
        NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED]
        [NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX];
#if NDS_TICK_HUD
volatile u32 gNdsRendererAdapterOwnerSelectedRootsHighWater;
volatile u32 gNdsRendererAdapterOwnerMaterialsPerRootHighWater;
#endif
static s32 sNdsRendererAdapterNativeOwnerTextureCurr[
    NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED]
    [NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX];
static s32 sNdsRendererAdapterNativeOwnerTextureNext[
    NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED]
    [NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX];
static u32 sNdsRendererAdapterNativeOwnerTextureCounts[
    NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
/* One 16-byte identity+input key per entry of the materials array above, so a
 * material whose inputs have not moved keeps the block already sitting there.
 * See ndsRendererAdapterPrepareNativeMaterials for the census that says this is
 * every material every frame. 2,048 bytes; the array it guards is 12,800.
 *
 * TWO hashes, because the full one costs 520 cycles a call and runs 37 times a
 * frame -- 10,829 ticks a frame to confirm nothing changed 59,362 times and
 * catch 0 real changes. `MObjSub` is 120 bytes of material descriptor and the
 * builder reads most of it, so narrowing the hash to the builder's READ set was
 * measured flat: the read set is the struct. Narrowing it to the ANIMATABLE set
 * is a different cut. Exactly nine words move during a match, and they are
 * contiguous in two runs:
 *
 *   MObj+0x58..0x6F  primcolor, prim_l/prim_m, envcolor, blendcolor,
 *                    light1color, light2color -- the five tracks
 *                    gcPlayMObjMatAnim writes, plus the prim level byte
 *   MObj+0x80..0x8B  texture_id_curr, texture_id_next, lfrac, palette_id
 *
 * Everything else in `MObjSub` -- fmt, siz, sprites, palettes, the UV and block
 * fields -- is asset data decoded at load, and `(mobj, heap_generation)` already
 * keys that: a taskman rewind bumps the generation and refuses every skip.
 *
 * That premise was not asserted, it was MEASURED. Slice 17 shipped a fail-closed
 * half -- a second key holding the whole 34-word hash, re-checked on every
 * fourth frame, with `gNdsR2MatKeyMissStatic` counting any disagreement. Over a
 * 60-second match that check ran ~14,848 times and disagreed **0** times, on top
 * of the cycle-98 census's zero material variants. The scaffold itself measured
 * +3,342 FTR -- a volatile frame-counter load and a branch per entry, plus a
 * 16-byte key -- so it is gone and its result is written down here instead. */
typedef struct NDSRendererAdapterMaterialKey
{
    const MObj *mobj;
    u32 heap_generation;
    u32 hash;
} NDSRendererAdapterMaterialKey;

static NDSRendererAdapterMaterialKey sNdsRendererAdapterNativeOwnerMaterialKeys[
    NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED]
    [NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX];

/* ...and which ROW of those two arrays a given material DObj owns.
 *
 * The row used to be the selected-root index `i`, which rotates between frames.
 * With the input key in place the engagement counters said so exactly: of
 * 30,606 rebuilds, **30,606 were `keys[count].mobj != mobj` and not one was an
 * input change**. The block was always correct and always filed under the wrong
 * row. A material DObj is stable for the fighter's life, so hash it to a row
 * and keep it there; distinct DObjs get distinct rows by linear probing, and
 * two roots that genuinely share a material DObj share its row and its block,
 * which is right. Cleared on a taskman-heap rewind because these are arena
 * pointers -- the key's own heap_generation would already refuse a stale skip,
 * so this is about not leaking rows to dead DObjs. 36 bytes. */
#define NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED_LOG2 5u
_Static_assert((1u << NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED_LOG2) ==
                   NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED,
               "material row hash assumes a power-of-two row count");
_Static_assert(NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED <= 32u,
               "material row claim mask must cover every material row");

static DObj *sNdsRendererAdapterMaterialRowOwner[
    NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
static u32 sNdsRendererAdapterMaterialRowGeneration;
/* Rows are a persistent per-DObj cache across fighter draws, but one native
 * owner submission must never alias two DIFFERENT live material DObjs onto the
 * same row.  Once Mario/Fox mirrors made more than one instance of a kind live,
 * the persistent table could become completely occupied by older fighters; the
 * old full-table fallback then returned the hashed row even when another root
 * in the CURRENT fighter had already claimed it.  Material preparation happens
 * for every root before submission, so the later root overwrote the earlier
 * root's material block and produced the mixed-costume colours seen on mirrors.
 *
 * Keep the cache (and its measured gameplay win), but track the rows claimed by
 * this one owner preparation.  A full table may evict an OLD fighter's row; it
 * may not evict a row the current fighter still needs for this submission. */
static u32 sNdsRendererAdapterMaterialRowClaimMask;
static u8 sNdsRendererAdapterNativeOwnerMaterialRows[
    NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];

/* Costume / parts replacement is a materially different lifetime from the
 * ordinary per-frame material animation the key below was designed for.
 *
 * BattleShip's ftParamInitAllParts removes every MObj and immediately creates
 * replacement MObjs from the newly selected costume. objman.c returns removed
 * MObjs to a free list, so the replacement is allowed to receive the SAME MObj
 * address in the SAME taskman heap generation. The hot material key deliberately
 * hashes only the nine fields that animate during a match; if those happen to
 * match too, `(mobj, heap_generation, anim_hash)` cannot distinguish the old
 * costume from the new one and would replay the old immutable texture/material
 * block. Fighter destruction/recreation on the CSS has the same address-reuse
 * lifetime.
 *
 * These events are menu/setup-time rare. Clear the tiny stable-row table and
 * the key identities at that authoritative lifetime seam instead of adding a
 * generation compare to every material on every gameplay frame. This also
 * reclaims rows owned by DObjs from a fighter that the CSS just destroyed. */
static void ndsFighterRendererInvalidateMaterialRows(void)
{
    u32 row;

    for (row = 0u; row < NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED; row++)
    {
        u32 material;

        sNdsRendererAdapterMaterialRowOwner[row] = NULL;
        for (material = 0u;
             material < NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX;
             material++)
        {
            sNdsRendererAdapterNativeOwnerMaterialKeys[row][material].mobj = NULL;
        }
    }
    sNdsRendererAdapterMaterialRowGeneration = gNdsTaskmanHeapGeneration;
    sNdsRendererAdapterMaterialRowClaimMask = 0u;
}

void ndsFighterRendererInvalidateMaterialCachesForSlot(u32 slot)
{
    /* Material rows remain global because source free-list reuse can move an
     * MObj address between fighters.  The recorded GX stream cannot: packets
     * are keyed by source player slot, so only the fighter whose DObj/MObj graph
     * was replaced needs to re-record. Invalid slots fail safe in the renderer. */
    ndsRendererFighterPacketInvalidateSlot(slot);
    ndsFighterRendererInvalidateMaterialRows();
    /* P2-2p8 Phase 1 slice 1 (H6): the lean list's MObj bindings are stale. */
    ndsFtrLeanNoteRebind(slot);
}
static NDSRendererMatrix20p12
    sNdsRendererAdapterNativeOwnerProjection;
static NDSRendererMatrix20p12
    sNdsRendererAdapterNativeOwnerModelviews[
        NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];

/* The seam-correct source hierarchy retains more precision than GX Q20.12.
 * Translation uses s64 so stage position cannot consume the integer range. */
#define NDS_RENDERER_ADAPTER_SOURCE_WORLD_FRAC_BITS 20u
#define NDS_RENDERER_ADAPTER_SOURCE_LOCAL_FRAC_BITS 16u
#define NDS_RENDERER_ADAPTER_SOURCE_TO_DS_SHIFT \
    (NDS_RENDERER_ADAPTER_SOURCE_WORLD_FRAC_BITS - \
     NDS_RENDERER_ADAPTER_MTX_FRAC_BITS)

typedef struct NDSRendererAdapterSourceWorld
{
    s32 basis[3][3];
    s64 translation[3];
} NDSRendererAdapterSourceWorld;

typedef struct NDSRendererAdapterNativeOwnerWorkspace
{
    NDSRelocLoadedFile *loaded[
        NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    DObj *matrix_bindings[
        NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    DObj *material_dobjs[
        NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    const NDSRendererMatrix20p12 *modelviews[
        NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    NDSRendererMatrix20p12 composed_matrices[
        NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    NDSRendererConfig production_configs[
        NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    NDSRendererNativeFighterRoot production_roots[
        NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    DObj *hierarchy_joints[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    u8 hierarchy_parents[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    u8 hierarchy_bindings[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    /* Source-world compose and the alternate hierarchy route are mutually
     * exclusive owner-preparation paths. Overlay their per-joint matrices so
     * seam precision does not shrink the already-tight battle arena. */
    union
    {
        NDSRendererMatrix20p12 hierarchy_locals[
            NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
        NDSRendererAdapterSourceWorld source_worlds[
            NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    } hierarchy_storage;
    /* Slice 43. Per-binding descriptors for the GX compose, plus the chains
     * themselves. This does NOT reuse `hierarchy_storage.hierarchy_locals`: that
     * array is sized at
     * the joint count, and a binding whose baked parent is 0xff walks to the
     * DObj root, so Mario's three root-parented bindings re-capture their shared
     * ancestors and overrun it. */
    u8 gx_local_first[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    u8 gx_local_count[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    u8 gx_parent_slot[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    u8 gx_store_slot[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    NDSRendererMatrix20p12 gx_locals[NDS_RENDERER_FIGHTER_GX_LOCAL_MAX];
    NDSRendererMatrix20p12 gx_seed;
    u8 gx_seed_is_identity;
    u8 gx_valid;
    u8 gx_modelview_mirror_valid;
    NDSRendererMatrix20p12 hierarchy_projection;
    NDSRendererMatrix20p12 hierarchy_camera_modelview;
    NDSRendererConfig hierarchy_config;
    NDSRendererNativeFighterHierarchy hierarchy;
    u32 root_offsets[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    u32 material_counts[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
} NDSRendererAdapterNativeOwnerWorkspace;

/* Fighter display callbacks are serialized by taskman. Keep their whole-owner
 * preflight tables off the already-tight nested task stack. */
static NDSRendererAdapterNativeOwnerWorkspace
    sNdsRendererAdapterNativeOwnerWorkspace;

typedef struct NDSRendererAdapterStageWorldSourceKey
{
    u32 base_translate[3];
    u32 base_rotate[4];
    u32 base_scale[3];
    u8 xobj_kinds[5];
    u8 xobj_present_mask;
    u8 xobjs_num;
    u8 reserved;
} NDSRendererAdapterStageWorldSourceKey;

#define NDS_RENDERER_ADAPTER_STAGE_SEGMENT_COUNT 8u
#define NDS_RENDERER_ADAPTER_STAGE_DOBJ_COUNT 57u
#define NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT 42u
#define NDS_RENDERER_ADAPTER_STAGE_ASSET_COUNT 4u
/* 40: the blob maxima's material event count is 18 (Planet Zebes) among the
 * VS stages and 39 for the Final Destination bonus packet; matches
 * NDS_RENDERER_NATIVE_STAGE_MATERIAL_COUNT (2026-09-07). */
#define NDS_RENDERER_ADAPTER_STAGE_MATERIAL_COUNT 40u
/* P2-4n1 step 2: the five counts above are workspace MAXIMA over stages --
 * every array below is sized from them so a second stage cannot overflow a
 * buffer sized for the first. The ACTIVE counts for the current stage live
 * in the per-stage descriptor below (Dream Land only for now); callers that
 * validate or admit one topology must read the descriptor, never the maxima.
 * Numeric values are unchanged: with only Dream Land present MAX == ACTIVE. */
extern volatile u32 gNdsSCVSBattleStageGKind;

/* P2-4n1 step 6: the stage CAPTURE side. One row per native segment says
 * which live GObj the segment is, what it must look like to be admitted, and
 * which generator owner its DObjs are tagged with. Until this table existed
 * renderer_adapter_stage.c carried Dream Land's eight rows as a switch, two
 * static arrays and a ternary, so a second stage's packet could link but
 * never capture a single DObj. Values are transcribed from each descriptor's
 * segment_partition / callback_partition and its per-layer live DObj counts
 * (scripts/stages/native_stage_descriptors/{dreamland,yoster}.py). */
#define NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER 0u      /* gGRCommonLayerGObjs[index] */
#define NDS_RENDERER_ADAPTER_STAGE_CAPTURE_PUPUPU_MAP 1u /* gGRCommonStruct.pupupu.map_gobj[index] */
#define NDS_RENDERER_ADAPTER_STAGE_CAPTURE_ZEBES_ACID 2u /* grZebesMakeAcid's live GObj */
#define NDS_RENDERER_ADAPTER_STAGE_CAPTURE_INISHIE_SCALE_TREE 3u
#define NDS_RENDERER_ADAPTER_STAGE_CAPTURE_INISHIE_SCALE_PLATFORM 4u
#define NDS_RENDERER_ADAPTER_STAGE_CAPTURE_YAMABUKI_GATE 5u

typedef struct NDSRendererAdapterNativeStageCaptureSegment
{
    u8 source;      /* NDS_RENDERER_ADAPTER_STAGE_CAPTURE_* */
    u8 index;       /* index into that source's GObj array */
    u8 link;        /* dl_link_id the GObj must sit on */
    u8 layer;       /* N of the grDisplayLayerNPriProcDisplay it must run */
    u8 dobj_count;  /* live DObjs the collector must find under it */
    u8 owner;       /* generator OWNER_* id the collected DObjs carry */
    u8 dl_links;    /* 1: the layer's Sec callback draws DObjDLLink arrays, one
                     * binding per link with its display head; 0: Pri callback,
                     * one dv display list per DObj. Rows that omit it are 0. */
} NDSRendererAdapterNativeStageCaptureSegment;

typedef struct NDSRendererAdapterNativeStageDescriptor
{
    u32 segment_count;
    u32 dobj_count;
    u32 binding_count;
    u32 asset_count;
    u32 material_count;
    u32 asset_ids[NDS_RENDERER_ADAPTER_STAGE_ASSET_COUNT];
    u32 asset_sizes[NDS_RENDERER_ADAPTER_STAGE_ASSET_COUNT];
    const NDSRendererAdapterNativeStageCaptureSegment *capture;
    u32 capture_count;   /* == segment_count; the rows the loops walk */
    u32 layer0_count;    /* leading rows on link 4 whose DL order is asserted */
} NDSRendererAdapterNativeStageDescriptor;

/* Dream Land, verbatim from the switch/arrays/ternary this table replaced:
 * segment GObj {L0, M0, M1, M2, L1, L2, M3, L3}, links {4,4,4,4,6,13,16,17},
 * layer procs {0,0,0,0,1,2,3,3}, DObj counts {21,3,6,7,2,4,10,4}, owners
 * {0,4,5,6,1,2,7,3}. */
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureDreamLand[8] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER,      0u,  4u, 0u, 21u, 0u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_PUPUPU_MAP, 0u,  4u, 0u,  3u, 4u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_PUPUPU_MAP, 1u,  4u, 0u,  6u, 5u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_PUPUPU_MAP, 2u,  4u, 0u,  7u, 6u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER,      1u,  6u, 1u,  2u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER,      2u, 13u, 2u,  4u, 2u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_PUPUPU_MAP, 3u, 16u, 3u, 10u, 7u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER,      3u, 17u, 3u,  4u, 3u }
    };

#if defined(NDS_P2_STAGE_YOSTER) && (NDS_P2_STAGE_YOSTER == 1)
/* Yoshi's Island: only the four shared display layers (gryoster.c:260-268
 * makes just the ground GObj; grdisplay.c:10-43 draws layer N under
 * grDisplayLayerNPriProcDisplay at link 4/6/13/17). Live DObj counts 18/5/2/3
 * are the descriptor entry counts 19/6/3/4 minus their sentinels
 * (yoster.py:30-37); owners are OWNER_LAYER0..3 = 0..3. */
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureYoster[4] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u, 18u, 0u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,  5u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 2u, 13u, 2u,  2u, 2u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 3u, 17u, 3u,  3u, 3u }
    };
#endif

static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageDreamLand = {
        8u,
        57u,
        42u,
        4u,
        4u,
        { 0x67u, 0x68u, 0x98u, 0xffu },
        { 0x2fc0u, 0x43f0u, 0x3700u, 0x00c0u },
        sNdsRendererAdapterNativeStageCaptureDreamLand,
        8u,
        4u
    };

/* P2-4n1 step 5: Yoshi's Island, gkind 5. Counts are the live packet counts
 * measured by the generator; asset ids are the O2R file ids in the
 * descriptor's asset_order and the sizes are the pinned payload byte counts.
 * All seven values are copied from
 * scripts/stages/native_stage_descriptors/yoster.py's adapter_* fields, which
 * check_nds_native_stage.py asserts against the generated packet. */
#if defined(NDS_P2_STAGE_YOSTER) && (NDS_P2_STAGE_YOSTER == 1)
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageYoster = {
        4u,
        28u,
        19u,
        4u,
        2u,
        { 0x6eu, 0x6fu, 0x9au, 0x107u },
        { 0x5230u, 0xb930u, 0x06b0u, 0x00c0u },
        sNdsRendererAdapterNativeStageCaptureYoster,
        4u,
        1u
    };
#endif

#if defined(NDS_P2_STAGE_JUNGLE) && (NDS_P2_STAGE_JUNGLE == 1)
/* Binary DObj sentinels give live layer counts 18/4/3/9. Source map 261
 * selects the four primary callbacks; only layer1 has joint animation. */
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureJungle[4] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u, 18u, 0u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,  4u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 2u, 13u, 2u,  3u, 2u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 3u, 17u, 3u,  9u, 3u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageJungle = {
        4u, 34u, 30u, 4u, 0u,
        { 107u, 108u, 158u, 261u },
        { 27792u, 62944u, 3296u, 224u },
        sNdsRendererAdapterNativeStageCaptureJungle,
        4u,
        1u
    };
#endif

#if defined(NDS_P2_STAGE_CASTLE) && (NDS_P2_STAGE_CASTLE == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureCastle[4] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u, 6u, 0u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u, 5u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 2u, 13u, 2u, 3u, 2u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 3u, 17u, 3u, 2u, 3u }
    };
/* Castle's textures live in geometry file 106; there is no image-only bank. */
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageCastle = {
        4u, 16u, 12u, 3u, 0u,
        { 106u, 156u, 259u, 0u },
        { 17696u, 64u, 192u, 0u },
        sNdsRendererAdapterNativeStageCaptureCastle,
        4u,
        1u
    };
#endif

/* VS starter kinds Castle(0)..Yamabuki(7) in gr/grdef.h:11-18 order; Yoshi's
 * Island is 5 and Dream Land is 6. A kind with no baked native packet is NULL
 * and MUST stay NULL. Until P2-4n1 step 5 every slot pointed at the frozen
 * Dream Land descriptor, so a stage the player could actually select resolved
 * to Dream Land's DObj/binding counts and asset ids: the topology stamp then
 * either rejected (silently demoting a shipped stage to the generic renderer)
 * or -- worse -- admitted another stage's identity. Resolving to NULL makes an
 * unbaked kind decline at the seam that knows it cannot draw, which is what
 * the acceleration review's "mandatory fallback is absent" asks for. */
#if defined(NDS_P2_STAGE_SECTOR) && (NDS_P2_STAGE_SECTOR == 1)
/* Sector Z: two DLLink layers on the shared display GObjs 0 and 1, links
   4/6, Sec callbacks; live DObj counts 12/11 (sector.py). */
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureSector[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u, 4u, 0u, 12u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u, 6u, 1u, 11u, 1u, 1u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageSector = {
        2u, 23u, 19u, 2u, 0u,
        { 109u, 262u, 0u, 0u },
        { 47120u, 304u, 0u, 0u },
        sNdsRendererAdapterNativeStageCaptureSector,
        2u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_HYRULE) && (NDS_P2_STAGE_HYRULE == 1)
/* Hyrule Castle: layers 0, 1 and 3 on display GObjs 0, 1 and 3, links 4/6/17;
   only layer 0 is a DLLink (Sec) layer, layers 1 and 3 draw single dv lists
   through the Pri callbacks (hyrule.py callback_partition); live DObj counts
   4/4/10. The emitter's --check caught the first transcription of this. */
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureHyrule[3] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,  4u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,  4u, 1u, 0u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 3u, 17u, 3u, 10u, 3u, 0u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageHyrule = {
        3u, 18u, 15u, 2u, 0u,
        { 113u, 265u, 0u, 0u },
        { 26768u, 224u, 0u, 0u },
        sNdsRendererAdapterNativeStageCaptureHyrule,
        3u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_INISHIE) && (NDS_P2_STAGE_INISHIE == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureInishie[7] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,  10u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,   6u, 1u, 0u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_INISHIE_SCALE_TREE, 0u,
          6u, 0u, 5u, 4u, 0u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_INISHIE_SCALE_PLATFORM, 0u,
          6u, 0u, 1u, 5u, 0u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_INISHIE_SCALE_PLATFORM, 1u,
          6u, 0u, 1u, 6u, 0u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 2u, 13u, 2u,   2u, 2u, 0u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 3u, 17u, 3u,   2u, 3u, 0u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageInishie = {
        7u, 27u, 24u, 3u, 4u,
        { 107u, 155u, 260u, 0u },
        { 27792u, 5136u, 368u, 0u },
        sNdsRendererAdapterNativeStageCaptureInishie,
        7u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_ZEBES) && (NDS_P2_STAGE_ZEBES == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureZebes[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,  28u, 1u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_ZEBES_ACID, 0u, 12u, 0u,   2u, 2u, 1u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageZebes = {
        2u, 30u, 26u, 3u, 19u,
        { 105u, 157u, 257u, 0u },
        { 57184u, 3536u, 224u, 0u },
        sNdsRendererAdapterNativeStageCaptureZebes,
        2u,
        0u
    };
#endif
#if defined(NDS_P2_STAGE_YAMABUKI) && (NDS_P2_STAGE_YAMABUKI == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureYamabuki[4] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,   7u, 0u, 0u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,   9u, 1u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 3u, 17u, 3u,   3u, 3u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_YAMABUKI_GATE, 0u, 6u, 0u, 5u, 4u, 1u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageYamabuki = {
        4u, 24u, 21u, 3u, 2u,
        { 112u, 160u, 264u, 0u },
        { 66160u, 2704u, 832u, 0u },
        sNdsRendererAdapterNativeStageCaptureYamabuki,
        4u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_PUPUPUSMALL) && (NDS_P2_STAGE_PUPUPUSMALL == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCapturePupupuSmall[3] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,   6u, 0u, 0u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,   2u, 1u, 0u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 3u, 17u, 3u,   2u, 3u, 0u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStagePupupuSmall = {
        3u, 10u, 7u, 3u, 0u,
        { 100u, 101u, 256u, 0u },
        { 10176u, 6560u, 192u, 0u },
        sNdsRendererAdapterNativeStageCapturePupupuSmall,
        3u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_YOSTERSMALL) && (NDS_P2_STAGE_YOSTERSMALL == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureYosterSmall[4] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,  14u, 0u, 0u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,   2u, 1u, 0u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 2u, 13u, 2u,   2u, 2u, 0u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 3u, 17u, 3u,   3u, 3u, 0u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageYosterSmall = {
        4u, 21u, 17u, 3u, 0u,
        { 110u, 118u, 270u, 0u },
        { 21040u, 17744u, 192u, 0u },
        sNdsRendererAdapterNativeStageCaptureYosterSmall,
        4u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_METAL) && (NDS_P2_STAGE_METAL == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureMetal[4] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,   2u, 0u, 0u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,   2u, 1u, 0u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 2u, 13u, 2u,   2u, 2u, 0u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 3u, 17u, 3u,   2u, 3u, 0u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageMetal = {
        4u, 8u, 4u, 2u, 13u,
        { 117u, 269u, 0u, 0u },
        { 16320u, 192u, 0u, 0u },
        sNdsRendererAdapterNativeStageCaptureMetal,
        4u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_ZAKO) && (NDS_P2_STAGE_ZAKO == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureZako[1] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,   2u, 1u, 0u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageZako = {
        1u, 2u, 1u, 2u, 0u,
        { 116u, 268u, 0u, 0u },
        { 17328u, 192u, 0u, 0u },
        sNdsRendererAdapterNativeStageCaptureZako,
        1u,
        0u
    };
#endif
#if defined(NDS_P2_STAGE_LAST) && (NDS_P2_STAGE_LAST == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureLast[1] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,   5u, 1u, 0u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageLast = {
        1u, 5u, 5u, 2u, 13u,
        { 114u, 266u, 0u, 0u },
        { 76128u, 176u, 0u, 0u },
        sNdsRendererAdapterNativeStageCaptureLast,
        1u,
        0u
    };
#endif
#if defined(NDS_P2_STAGE_BONUS3) && (NDS_P2_STAGE_BONUS3 == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureBonus3[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,  11u, 0u, 0u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,  12u, 1u, 1u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageBonus3 = {
        2u, 23u, 24u, 2u, 3u,
        { 149u, 295u, 0u, 0u },
        { 26768u, 272u, 0u, 0u },
        sNdsRendererAdapterNativeStageCaptureBonus3,
        2u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_BONUS1_MARIO) && (NDS_P2_STAGE_BONUS1_MARIO == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureBonus1Mario[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,   6u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,   3u, 1u, 0u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageBonus1Mario = {
        2u, 9u, 7u, 3u, 0u,
        { 120u, 124u, 271u, 0u },
        { 2672u, 9152u, 176u, 0u },
        sNdsRendererAdapterNativeStageCaptureBonus1Mario,
        2u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_BONUS1_FOX) && (NDS_P2_STAGE_BONUS1_FOX == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureBonus1Fox[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,   6u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,   5u, 1u, 0u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageBonus1Fox = {
        2u, 11u, 9u, 3u, 0u,
        { 121u, 125u, 272u, 0u },
        { 2032u, 10080u, 176u, 0u },
        sNdsRendererAdapterNativeStageCaptureBonus1Fox,
        2u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_BONUS1_DONKEY) && (NDS_P2_STAGE_BONUS1_DONKEY == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureBonus1Donkey[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,   4u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,   4u, 1u, 0u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageBonus1Donkey = {
        2u, 8u, 6u, 3u, 0u,
        { 120u, 126u, 273u, 0u },
        { 2672u, 9392u, 176u, 0u },
        sNdsRendererAdapterNativeStageCaptureBonus1Donkey,
        2u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_BONUS1_SAMUS) && (NDS_P2_STAGE_BONUS1_SAMUS == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureBonus1Samus[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,   5u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,   3u, 1u, 0u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageBonus1Samus = {
        2u, 8u, 6u, 3u, 0u,
        { 121u, 127u, 274u, 0u },
        { 2032u, 7584u, 176u, 0u },
        sNdsRendererAdapterNativeStageCaptureBonus1Samus,
        2u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_BONUS1_LUIGI) && (NDS_P2_STAGE_BONUS1_LUIGI == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureBonus1Luigi[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,   5u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,   5u, 1u, 0u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageBonus1Luigi = {
        2u, 10u, 8u, 3u, 0u,
        { 120u, 128u, 275u, 0u },
        { 2672u, 8832u, 176u, 0u },
        sNdsRendererAdapterNativeStageCaptureBonus1Luigi,
        2u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_BONUS1_LINK) && (NDS_P2_STAGE_BONUS1_LINK == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureBonus1Link[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,   8u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,   4u, 1u, 0u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageBonus1Link = {
        2u, 12u, 10u, 3u, 0u,
        { 123u, 129u, 276u, 0u },
        { 3024u, 10720u, 176u, 0u },
        sNdsRendererAdapterNativeStageCaptureBonus1Link,
        2u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_BONUS1_YOSHI) && (NDS_P2_STAGE_BONUS1_YOSHI == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureBonus1Yoshi[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,   8u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,   4u, 1u, 0u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageBonus1Yoshi = {
        2u, 12u, 10u, 3u, 0u,
        { 122u, 130u, 277u, 0u },
        { 976u, 13536u, 176u, 0u },
        sNdsRendererAdapterNativeStageCaptureBonus1Yoshi,
        2u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_BONUS1_CAPTAIN) && (NDS_P2_STAGE_BONUS1_CAPTAIN == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureBonus1Captain[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,   4u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,   3u, 1u, 0u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageBonus1Captain = {
        2u, 7u, 5u, 3u, 0u,
        { 121u, 131u, 278u, 0u },
        { 2032u, 7648u, 176u, 0u },
        sNdsRendererAdapterNativeStageCaptureBonus1Captain,
        2u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_BONUS1_KIRBY) && (NDS_P2_STAGE_BONUS1_KIRBY == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureBonus1Kirby[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,   6u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,   3u, 1u, 0u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageBonus1Kirby = {
        2u, 9u, 7u, 3u, 0u,
        { 123u, 132u, 279u, 0u },
        { 3024u, 10112u, 176u, 0u },
        sNdsRendererAdapterNativeStageCaptureBonus1Kirby,
        2u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_BONUS1_PIKACHU) && (NDS_P2_STAGE_BONUS1_PIKACHU == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureBonus1Pikachu[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,   6u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,   3u, 1u, 0u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageBonus1Pikachu = {
        2u, 9u, 7u, 3u, 0u,
        { 122u, 133u, 280u, 0u },
        { 976u, 11488u, 176u, 0u },
        sNdsRendererAdapterNativeStageCaptureBonus1Pikachu,
        2u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_BONUS1_PURIN) && (NDS_P2_STAGE_BONUS1_PURIN == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureBonus1Purin[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,   5u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,   4u, 1u, 0u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageBonus1Purin = {
        2u, 9u, 7u, 3u, 0u,
        { 122u, 134u, 281u, 0u },
        { 976u, 9808u, 176u, 0u },
        sNdsRendererAdapterNativeStageCaptureBonus1Purin,
        2u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_BONUS1_NESS) && (NDS_P2_STAGE_BONUS1_NESS == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureBonus1Ness[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,   5u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,   3u, 1u, 0u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageBonus1Ness = {
        2u, 8u, 6u, 3u, 0u,
        { 123u, 135u, 282u, 0u },
        { 3024u, 12496u, 176u, 0u },
        sNdsRendererAdapterNativeStageCaptureBonus1Ness,
        2u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_BONUS2_MARIO) && (NDS_P2_STAGE_BONUS2_MARIO == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureBonus2Mario[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,   5u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,  15u, 1u, 1u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageBonus2Mario = {
        2u, 20u, 9u, 3u, 0u,
        { 120u, 137u, 283u, 0u },
        { 2672u, 15648u, 176u, 0u },
        sNdsRendererAdapterNativeStageCaptureBonus2Mario,
        2u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_BONUS2_FOX) && (NDS_P2_STAGE_BONUS2_FOX == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureBonus2Fox[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,   5u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,  20u, 1u, 1u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageBonus2Fox = {
        2u, 25u, 14u, 3u, 4u,
        { 121u, 138u, 284u, 0u },
        { 2032u, 68624u, 176u, 0u },
        sNdsRendererAdapterNativeStageCaptureBonus2Fox,
        2u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_BONUS2_DONKEY) && (NDS_P2_STAGE_BONUS2_DONKEY == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureBonus2Donkey[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,  10u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,  14u, 1u, 1u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageBonus2Donkey = {
        2u, 24u, 12u, 3u, 0u,
        { 120u, 139u, 285u, 0u },
        { 2672u, 13760u, 176u, 0u },
        sNdsRendererAdapterNativeStageCaptureBonus2Donkey,
        2u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_BONUS2_SAMUS) && (NDS_P2_STAGE_BONUS2_SAMUS == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureBonus2Samus[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,   3u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,  20u, 1u, 1u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageBonus2Samus = {
        2u, 23u, 11u, 3u, 0u,
        { 121u, 140u, 286u, 0u },
        { 2032u, 10736u, 176u, 0u },
        sNdsRendererAdapterNativeStageCaptureBonus2Samus,
        2u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_BONUS2_LUIGI) && (NDS_P2_STAGE_BONUS2_LUIGI == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureBonus2Luigi[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,   5u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,  20u, 1u, 1u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageBonus2Luigi = {
        2u, 25u, 13u, 3u, 0u,
        { 120u, 141u, 287u, 0u },
        { 2672u, 16032u, 176u, 0u },
        sNdsRendererAdapterNativeStageCaptureBonus2Luigi,
        2u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_BONUS2_LINK) && (NDS_P2_STAGE_BONUS2_LINK == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureBonus2Link[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,  12u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,  18u, 1u, 1u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageBonus2Link = {
        2u, 30u, 18u, 3u, 0u,
        { 123u, 142u, 288u, 0u },
        { 3024u, 18768u, 176u, 0u },
        sNdsRendererAdapterNativeStageCaptureBonus2Link,
        2u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_BONUS2_YOSHI) && (NDS_P2_STAGE_BONUS2_YOSHI == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureBonus2Yoshi[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,   8u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,  21u, 1u, 1u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageBonus2Yoshi = {
        2u, 29u, 17u, 3u, 0u,
        { 122u, 143u, 289u, 0u },
        { 976u, 14352u, 176u, 0u },
        sNdsRendererAdapterNativeStageCaptureBonus2Yoshi,
        2u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_BONUS2_CAPTAIN) && (NDS_P2_STAGE_BONUS2_CAPTAIN == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureBonus2Captain[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,  10u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,  14u, 1u, 1u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageBonus2Captain = {
        2u, 24u, 13u, 3u, 0u,
        { 121u, 144u, 290u, 0u },
        { 2032u, 20368u, 176u, 0u },
        sNdsRendererAdapterNativeStageCaptureBonus2Captain,
        2u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_BONUS2_KIRBY) && (NDS_P2_STAGE_BONUS2_KIRBY == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureBonus2Kirby[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,   6u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,  17u, 1u, 1u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageBonus2Kirby = {
        2u, 23u, 11u, 3u, 0u,
        { 123u, 145u, 291u, 0u },
        { 3024u, 15568u, 176u, 0u },
        sNdsRendererAdapterNativeStageCaptureBonus2Kirby,
        2u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_BONUS2_PIKACHU) && (NDS_P2_STAGE_BONUS2_PIKACHU == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureBonus2Pikachu[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,   8u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,  24u, 1u, 1u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageBonus2Pikachu = {
        2u, 32u, 20u, 3u, 0u,
        { 122u, 146u, 292u, 0u },
        { 976u, 18672u, 176u, 0u },
        sNdsRendererAdapterNativeStageCaptureBonus2Pikachu,
        2u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_BONUS2_PURIN) && (NDS_P2_STAGE_BONUS2_PURIN == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureBonus2Purin[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,   4u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,  32u, 1u, 1u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageBonus2Purin = {
        2u, 36u, 25u, 3u, 0u,
        { 122u, 147u, 293u, 0u },
        { 976u, 21728u, 176u, 0u },
        sNdsRendererAdapterNativeStageCaptureBonus2Purin,
        2u,
        1u
    };
#endif
#if defined(NDS_P2_STAGE_BONUS2_NESS) && (NDS_P2_STAGE_BONUS2_NESS == 1)
static const NDSRendererAdapterNativeStageCaptureSegment
    sNdsRendererAdapterNativeStageCaptureBonus2Ness[2] = {
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 0u,  4u, 0u,   7u, 0u, 1u },
        { NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER, 1u,  6u, 1u,  18u, 1u, 1u }
    };
static const NDSRendererAdapterNativeStageDescriptor
    sNdsRendererAdapterNativeStageBonus2Ness = {
        2u, 25u, 13u, 3u, 0u,
        { 123u, 148u, 294u, 0u },
        { 3024u, 16576u, 176u, 0u },
        sNdsRendererAdapterNativeStageCaptureBonus2Ness,
        2u,
        1u
    };
#endif
#if NDS_P5
/* P5: the Remix stages' capture and descriptor rows, generated from their
 * packets (scripts/p5/stage_generate.py), at kinds past the source's last
 * ground. */
#include <nds/nds_p5.h>
#include "nds_p5_adapter.generated.inc"
#define NDS_RENDERER_ADAPTER_NATIVE_STAGE_KIND_COUNT (NDS_P5_GKIND_MAX + 1u)
#else
#define NDS_RENDERER_ADAPTER_NATIVE_STAGE_KIND_COUNT 41u
#endif

static const NDSRendererAdapterNativeStageDescriptor *const
    sNdsRendererAdapterNativeStageTable[
        NDS_RENDERER_ADAPTER_NATIVE_STAGE_KIND_COUNT] = {
#if defined(NDS_P2_STAGE_CASTLE) && (NDS_P2_STAGE_CASTLE == 1)
        &sNdsRendererAdapterNativeStageCastle,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_SECTOR) && (NDS_P2_STAGE_SECTOR == 1)
        &sNdsRendererAdapterNativeStageSector,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_JUNGLE) && (NDS_P2_STAGE_JUNGLE == 1)
        &sNdsRendererAdapterNativeStageJungle,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_ZEBES) && (NDS_P2_STAGE_ZEBES == 1)
        &sNdsRendererAdapterNativeStageZebes,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_HYRULE) && (NDS_P2_STAGE_HYRULE == 1)
        &sNdsRendererAdapterNativeStageHyrule,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_YOSTER) && (NDS_P2_STAGE_YOSTER == 1)
        &sNdsRendererAdapterNativeStageYoster,
#else
        NULL,
#endif
        &sNdsRendererAdapterNativeStageDreamLand,
#if defined(NDS_P2_STAGE_YAMABUKI) && (NDS_P2_STAGE_YAMABUKI == 1)
        &sNdsRendererAdapterNativeStageYamabuki,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_INISHIE) && (NDS_P2_STAGE_INISHIE == 1)
        &sNdsRendererAdapterNativeStageInishie,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_PUPUPUSMALL) && (NDS_P2_STAGE_PUPUPUSMALL == 1)
        &sNdsRendererAdapterNativeStagePupupuSmall,
#else
        NULL,
#endif
        NULL,
        NULL,
#if defined(NDS_P2_STAGE_YOSTERSMALL) && (NDS_P2_STAGE_YOSTERSMALL == 1)
        &sNdsRendererAdapterNativeStageYosterSmall,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_METAL) && (NDS_P2_STAGE_METAL == 1)
        &sNdsRendererAdapterNativeStageMetal,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_ZAKO) && (NDS_P2_STAGE_ZAKO == 1)
        &sNdsRendererAdapterNativeStageZako,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_BONUS3) && (NDS_P2_STAGE_BONUS3 == 1)
        &sNdsRendererAdapterNativeStageBonus3,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_LAST) && (NDS_P2_STAGE_LAST == 1)
        &sNdsRendererAdapterNativeStageLast,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_BONUS1_MARIO) && (NDS_P2_STAGE_BONUS1_MARIO == 1)
        &sNdsRendererAdapterNativeStageBonus1Mario,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_BONUS1_FOX) && (NDS_P2_STAGE_BONUS1_FOX == 1)
        &sNdsRendererAdapterNativeStageBonus1Fox,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_BONUS1_DONKEY) && (NDS_P2_STAGE_BONUS1_DONKEY == 1)
        &sNdsRendererAdapterNativeStageBonus1Donkey,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_BONUS1_SAMUS) && (NDS_P2_STAGE_BONUS1_SAMUS == 1)
        &sNdsRendererAdapterNativeStageBonus1Samus,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_BONUS1_LUIGI) && (NDS_P2_STAGE_BONUS1_LUIGI == 1)
        &sNdsRendererAdapterNativeStageBonus1Luigi,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_BONUS1_LINK) && (NDS_P2_STAGE_BONUS1_LINK == 1)
        &sNdsRendererAdapterNativeStageBonus1Link,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_BONUS1_YOSHI) && (NDS_P2_STAGE_BONUS1_YOSHI == 1)
        &sNdsRendererAdapterNativeStageBonus1Yoshi,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_BONUS1_CAPTAIN) && (NDS_P2_STAGE_BONUS1_CAPTAIN == 1)
        &sNdsRendererAdapterNativeStageBonus1Captain,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_BONUS1_KIRBY) && (NDS_P2_STAGE_BONUS1_KIRBY == 1)
        &sNdsRendererAdapterNativeStageBonus1Kirby,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_BONUS1_PIKACHU) && (NDS_P2_STAGE_BONUS1_PIKACHU == 1)
        &sNdsRendererAdapterNativeStageBonus1Pikachu,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_BONUS1_PURIN) && (NDS_P2_STAGE_BONUS1_PURIN == 1)
        &sNdsRendererAdapterNativeStageBonus1Purin,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_BONUS1_NESS) && (NDS_P2_STAGE_BONUS1_NESS == 1)
        &sNdsRendererAdapterNativeStageBonus1Ness,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_BONUS2_MARIO) && (NDS_P2_STAGE_BONUS2_MARIO == 1)
        &sNdsRendererAdapterNativeStageBonus2Mario,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_BONUS2_FOX) && (NDS_P2_STAGE_BONUS2_FOX == 1)
        &sNdsRendererAdapterNativeStageBonus2Fox,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_BONUS2_DONKEY) && (NDS_P2_STAGE_BONUS2_DONKEY == 1)
        &sNdsRendererAdapterNativeStageBonus2Donkey,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_BONUS2_SAMUS) && (NDS_P2_STAGE_BONUS2_SAMUS == 1)
        &sNdsRendererAdapterNativeStageBonus2Samus,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_BONUS2_LUIGI) && (NDS_P2_STAGE_BONUS2_LUIGI == 1)
        &sNdsRendererAdapterNativeStageBonus2Luigi,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_BONUS2_LINK) && (NDS_P2_STAGE_BONUS2_LINK == 1)
        &sNdsRendererAdapterNativeStageBonus2Link,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_BONUS2_YOSHI) && (NDS_P2_STAGE_BONUS2_YOSHI == 1)
        &sNdsRendererAdapterNativeStageBonus2Yoshi,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_BONUS2_CAPTAIN) && (NDS_P2_STAGE_BONUS2_CAPTAIN == 1)
        &sNdsRendererAdapterNativeStageBonus2Captain,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_BONUS2_KIRBY) && (NDS_P2_STAGE_BONUS2_KIRBY == 1)
        &sNdsRendererAdapterNativeStageBonus2Kirby,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_BONUS2_PIKACHU) && (NDS_P2_STAGE_BONUS2_PIKACHU == 1)
        &sNdsRendererAdapterNativeStageBonus2Pikachu,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_BONUS2_PURIN) && (NDS_P2_STAGE_BONUS2_PURIN == 1)
        &sNdsRendererAdapterNativeStageBonus2Purin,
#else
        NULL,
#endif
#if defined(NDS_P2_STAGE_BONUS2_NESS) && (NDS_P2_STAGE_BONUS2_NESS == 1)
        &sNdsRendererAdapterNativeStageBonus2Ness,
#else
        NULL,
#endif
#if NDS_P5
#define NDS_P5_ADAPTER_SLOT(g, cname) [g] = &sNdsRendererAdapterNativeStage##cname,
        NDS_P5_ADAPTER_ROWS(NDS_P5_ADAPTER_SLOT)
#undef NDS_P5_ADAPTER_SLOT
#endif
    };

/* NULL when the loaded kind has no baked packet. Every caller treats that as
 * "no native stage", never as Dream Land. */
static const NDSRendererAdapterNativeStageDescriptor *
ndsRendererAdapterNativeStageDescriptor(void)
{
    u32 kind = gNdsSCVSBattleStageGKind;

    if (kind < NDS_RENDERER_ADAPTER_NATIVE_STAGE_KIND_COUNT)
    {
        return sNdsRendererAdapterNativeStageTable[kind];
    }
    return NULL;
}
/* Scalar ACTIVE readers for the tracked stage closures. They keep the only
 * arrow reads on the descriptor inside these untracked helpers, so the
 * M3 consumed-fields certificate sees no new pointer base in the closures
 * that admit one topology. */
/* All four return 0 when the loaded kind has no descriptor. 0 is fail-closed
 * at every call site: a workspace count can never legitimately be 0, and an
 * asset id/size of 0 fails its compare, so an unresolvable kind rejects
 * instead of inheriting Dream Land's identity. */
static u32 ndsRendererAdapterNativeStageActiveDObjCount(void)
{
    const NDSRendererAdapterNativeStageDescriptor *desc =
        ndsRendererAdapterNativeStageDescriptor();

    return (desc != NULL) ? desc->dobj_count : 0u;
}
static u32 ndsRendererAdapterNativeStageActiveBindingCount(void)
{
    const NDSRendererAdapterNativeStageDescriptor *desc =
        ndsRendererAdapterNativeStageDescriptor();

    return (desc != NULL) ? desc->binding_count : 0u;
}
/* P2-4n1 step 6: the capture side moved into the descriptor, so the four
 * segment loops in renderer_adapter_stage.c walk the active count and read
 * their per-segment facts from one row each. 0 / NULL when the loaded kind
 * has no descriptor, which every caller treats as "no native stage". */
static u32 ndsRendererAdapterNativeStageActiveSegmentCount(void)
{
    const NDSRendererAdapterNativeStageDescriptor *desc =
        ndsRendererAdapterNativeStageDescriptor();

    return (desc != NULL) ? desc->capture_count : 0u;
}
static u32 ndsRendererAdapterNativeStageActiveMaterialCount(void)
{
    const NDSRendererAdapterNativeStageDescriptor *desc =
        ndsRendererAdapterNativeStageDescriptor();

    return (desc != NULL) ? desc->material_count : 0u;
}
static u32 ndsRendererAdapterNativeStageActiveAssetCount(void)
{
    const NDSRendererAdapterNativeStageDescriptor *desc =
        ndsRendererAdapterNativeStageDescriptor();

    return (desc != NULL) ? desc->asset_count : 0u;
}
static u32 ndsRendererAdapterNativeStageLayer0Count(void)
{
    const NDSRendererAdapterNativeStageDescriptor *desc =
        ndsRendererAdapterNativeStageDescriptor();

    return (desc != NULL) ? desc->layer0_count : 0u;
}
static const NDSRendererAdapterNativeStageCaptureSegment *
ndsRendererAdapterNativeStageCaptureRow(u32 segment_index)
{
    const NDSRendererAdapterNativeStageDescriptor *desc =
        ndsRendererAdapterNativeStageDescriptor();

    return ((desc != NULL) && (segment_index < desc->capture_count)) ?
        &desc->capture[segment_index] : NULL;
}
static u32 ndsRendererAdapterNativeStageAssetId(u32 index)
{
    const NDSRendererAdapterNativeStageDescriptor *desc =
        ndsRendererAdapterNativeStageDescriptor();

    return ((desc != NULL) && (index < desc->asset_count)) ?
        desc->asset_ids[index] : 0u;
}
/* Loads every asset the active stage's native packet references, at stage
 * staging where load time is cheap. The admission arm only LOOKS the files up
 * (renderer_adapter_stage.c, reject reason 3) and the map's extern tree does
 * not pull all of them in: Congo Jungle's descriptor names bank 107 (27,792 B
 * of geometry its lists reach through segment pointers), which nothing loaded,
 * so every battle rejected the packet and the whole stage drew generically at
 * 8-12 FPS (admission-jungle12, 2026-09-06). */
volatile u32 gNdsNativeStagePreloadFailCount;
void ndsRendererAdapterNativeStagePreloadAssets(void)
{
    const NDSRendererAdapterNativeStageDescriptor *desc =
        ndsRendererAdapterNativeStageDescriptor();
    u32 i;

    if (desc == NULL)
    {
        return;
    }
    for (i = 0u; i < desc->asset_count; i++)
    {
        if (ndsRelocEnsureLoadedAsset(desc->asset_ids[i]) == NULL)
        {
            NDS_DIAG(gNdsNativeStagePreloadFailCount++);
        }
    }
}

/* One of the loaded stage's packet assets (the stage select's preview loads
 * them a tic apiece); FALSE once index is past the last. */
s32 ndsRendererAdapterNativeStagePreloadAsset(u32 index)
{
    const NDSRendererAdapterNativeStageDescriptor *desc =
        ndsRendererAdapterNativeStageDescriptor();

    if ((desc == NULL) || (index >= desc->asset_count))
    {
        return FALSE;
    }
    if (ndsRelocLoadAssetTree(desc->asset_ids[index]) == FALSE)
    {
        NDS_DIAG(gNdsNativeStagePreloadFailCount++);
    }
    return TRUE;
}

#if defined(NDS_P2_COMPACT_GROUND_MAPS) && NDS_P2_COMPACT_GROUND_MAPS
/* Out of line: the flag must not grow the hot stage-prep function whose
 * revalidation loop this is inlined into. */
static __attribute__((noinline))
#else
static
#endif
u32 ndsRendererAdapterNativeStageAssetSize(u32 index)
{
    const NDSRendererAdapterNativeStageDescriptor *desc =
        ndsRendererAdapterNativeStageDescriptor();

#if defined(NDS_P2_COMPACT_GROUND_MAPS) && NDS_P2_COMPACT_GROUND_MAPS
    /* The pinned size of a ground map is its source size; a compact map (T1,
     * reloc_backend_assets.c) is that plus its wallpaper stub. The rows and the
     * P1 golden pin stay the source sizes. */
    if ((desc != NULL) && (index < desc->asset_count))
    {
        return ndsRelocCompactGroundMapExpectedSize(desc->asset_ids[index],
                                                    desc->asset_sizes[index]);
    }
#endif
    return ((desc != NULL) && (index < desc->asset_count)) ?
        desc->asset_sizes[index] : 0u;
}

typedef struct NDSRendererAdapterNativeStageWorkspace
{
    NDSRelocLoadedFile *loaded[NDS_RENDERER_ADAPTER_STAGE_ASSET_COUNT];
    GObj *segments[NDS_RENDERER_ADAPTER_STAGE_SEGMENT_COUNT];
    /* One bit per non-NULL segments[] entry (NDS_STAGE_SEGMENT_BLOOM_BIT),
     * rebuilt whenever segments[] is written: the display commit's
     * no-false-negative pre-test for the camera loop's other GObjs. */
    u32 segment_bloom;
    DObj *dobjs[NDS_RENDERER_ADAPTER_STAGE_DOBJ_COUNT];
    DObj *binding_dobjs[NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT];
    NDSRendererNativeStageDObj live_dobjs[
        NDS_RENDERER_ADAPTER_STAGE_DOBJ_COUNT];
    const void *binding_display_lists[
        NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT];
    /* P2-4n1 step 7: which display head each binding draws into and which
     * DLLink slot it came from (0xff = the DObj's single dv list), so a
     * two-head DObj can own two bindings and re-validation can find each
     * binding's list again. */
    u8 binding_heads[NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT];
    u8 binding_link_index[NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT];
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 camera_modelview;
    NDSRendererMatrix20p12 binding_world[
        NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT];
    NDSRendererMatrix20p12 binding_composed[
        NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT];
    NDSRendererAdapterStageWorldSourceKey task36_rigid_source_keys[
        NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT];
    u64 task36_runtime_rigid_mask;
    /* Static bindings need only the source-key guard; frame matrix preparation
     * consumes the current mask directly. */
    u8 steady_rigid_bindings[NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT];
    u8 steady_rigid_binding_count;
    u8 steady_binding_lists_valid;
    /* Slice 44. Which residue class of the stride is revalidated this frame.
     * Advanced once per PrepareNativeStageOwner, which is once per frame --
     * the c120 profile pins that at 26 rigid checks per frame against a
     * 26-entry list. Both the rigid sweep and the dynamic chain walk read it,
     * so a binding's rigid proof and its world rebuild land on the same frame
     * and the per-frame cost stays flat instead of spiking every Nth frame. */
    u8 slice44_validate_cursor;
    /* P2-2p8 (2026-10-04): per dynamic binding, the frame validations in a
     * row whose chain walk rebuilt nothing (saturating). A binding quiet for
     * NDS_STAGE_DYN_QUIET_MIN of them is revalidated one frame in
     * gNdsStageDynStride, the others reuse its world (ndsRendererAdapter
     * PrepareNativeStageBindingMatrix). */
    u8 dyn_quiet[NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT];
    /* Bindings whose DObj chain holds a yakumono (moving map collision):
     * validated every frame, never strided. Set at topology capture. */
    u64 dyn_pinned_mask;
    NDSRendererNativeMaterial materials[
        NDS_RENDERER_ADAPTER_STAGE_MATERIAL_COUNT];
    MObj *material_mobjs[NDS_RENDERER_ADAPTER_STAGE_MATERIAL_COUNT];
    s32 material_curr[NDS_RENDERER_ADAPTER_STAGE_MATERIAL_COUNT];
    s32 material_next[NDS_RENDERER_ADAPTER_STAGE_MATERIAL_COUNT];
    NDSFighterDLDrawState resolver;
    NDSRendererConfig config;
    NDSRendererStats stats;
    NDSRendererNativeStageFrame frame;
    u32 dobj_count;
    u32 binding_count;
    u32 next_segment;
    u32 active;
    u32 topology_generation;
    u32 topology_stamp;
    u32 topology_valid;
    /* Task 44 item 3: the stage-asset mutation generation this workspace was
     * last fully validated against. Zero means "never admitted". */
    u32 steady_admission_generation;
    /* Per-segment root DObj recorded at collect time, so steady admission can
     * catch a display graph rebuilt underneath an unchanged segment GObj. */
    DObj *steady_segment_roots[NDS_RENDERER_ADAPTER_STAGE_SEGMENT_COUNT];
} NDSRendererAdapterNativeStageWorkspace;

/* The stage owner remains armed across fighter links 9, so it cannot share
 * the complete-fighter workspace. */
static NDSRendererAdapterNativeStageWorkspace
    sNdsRendererAdapterNativeStageWorkspace;

static sb32 ndsRendererAdapterCollectFighterTopology(
    DObj *dobj,
    u32 parent_index,
    DObj **joints,
    u8 *joint_parents,
    u32 *joint_count);

typedef struct NDSRendererAdapterNativeOwnerValidationCache
{
    /* The identity words first: a lookup rejects on them from one line. */
    u32 valid;
    u32 slot;
    const void *data;
    u32 asset_id;
    u32 owner_generation;
    u32 data_size;
    u32 root_count;
    u32 battle_slot;
    u32 use_low_detail;
    u32 stamp;                  /* LRU: the lookup serial of its last hit */
    u32 root_offsets[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    u32 material_counts[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
} NDSRendererAdapterNativeOwnerValidationCache;

/* The generated tables and relocated owner payload are immutable for one
 * reloc generation.  Prove every selected root/material identity each draw,
 * but do the full generated-array/span walk only when that identity changes.
 *
 * P2-2p8 (2026-10-04): a pool rather than one entry per owner slot. A fighter
 * whose model parts alternate (a hand or a face swap) moves between root sets
 * on every lean event, and the single entry re-ran the walk each time (~15K
 * ticks a switch, ~30K under a materialization in the four-CPU gate profile).
 * Twelve entries keyed by slot as well hold three root sets for each of four
 * fighters in less memory than the 25 per-slot entries did. */
#define NDS_RENDERER_ADAPTER_OWNER_VALIDATION_POOL 12u
static NDSRendererAdapterNativeOwnerValidationCache
    sNdsRendererAdapterNativeOwnerValidationCache[
        NDS_RENDERER_ADAPTER_OWNER_VALIDATION_POOL];
static u32 sNdsRendererAdapterNativeOwnerValidationStamp;

typedef struct NDSRendererAdapterParticleCamera
{
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    u32 valid;
} NDSRendererAdapterParticleCamera;

typedef struct NDSRendererAdapterCameraCacheEntry
{
    const CObj *cobj;
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    u32 projection_valid;
    u32 modelview_valid;
    s32 billboard_right_q12[3];
    s32 billboard_up_q12[3];
    u32 billboard_basis_valid;
    /* The 0x4C producer already has both factors before composing them. Keep
     * them in this frame-scoped context for the particle pass, avoiding a
     * second look-at/perspective build for each particle camera callback. */
    NDSRendererAdapterParticleCamera particle;
} NDSRendererAdapterCameraCacheEntry;

typedef struct NDSRendererAdapterDObjWorldCacheEntry
{
    const DObj *dobj;
    NDSRendererMatrix20p12 world;
} NDSRendererAdapterDObjWorldCacheEntry;

typedef struct NDSRendererAdapterStageWorldCacheEntry
{
    const DObj *dobj;
    const DObj *parent;
    NDSRendererAdapterStageWorldSourceKey source_key;
    u32 parent_generation;
    u32 generation;
    u32 validated_frame;
    u8 source_key_valid;
    u8 world_slot;
    u8 reserved[2];
} NDSRendererAdapterStageWorldCacheEntry;

static NDSRendererAdapterCameraCacheEntry
    sNdsRendererAdapterCameraCache[NDS_RENDERER_ADAPTER_CAMERA_CACHE_COUNT];
static u32 sNdsRendererAdapterCameraCacheFrame;
static u32 sNdsRendererAdapterCameraCacheCount;
/* Slice 44 engagement, both sides. `StaleReuse` counts world matrices returned
 * from an earlier frame, `RigidChecks` the rigid source keys actually compared,
 * `RigidSkips` the ones the stride deferred. Producer and consumer both, because
 * slice 43 read 32.06 roots a frame as full engagement while the producer was
 * declining every owner -- one counter would have said the same thing there.
 * __attribute__((used)): these are write-only from C, so -ffunction-sections
 * plus --gc-sections drops them and Boundary fails on a missing ELF symbol. */
__attribute__((used)) u32 gNdsR2Slice44StaleReuse;
__attribute__((used)) u32 gNdsR2Slice44RigidChecks;
__attribute__((used)) u32 gNdsR2Slice44RigidSkips;
static NDSRendererAdapterDObjWorldCacheEntry
    *sNdsRendererAdapterDObjWorldCache;
static u32 sNdsRendererAdapterDObjWorldCacheFrame;
static u32 sNdsRendererAdapterDObjWorldCacheCount;
static u32 sNdsRendererAdapterDObjWorldCacheDynamicLimit =
    NDS_RENDERER_ADAPTER_DOBJ_WORLD_CACHE_COUNT;
static u32 sNdsRendererAdapterDObjWorldCacheAllocationAttempted;
static u8 sNdsRendererAdapterDObjWorldIndex[
    NDS_RENDERER_ADAPTER_DOBJ_WORLD_INDEX_COUNT];
static NDSRendererAdapterStageWorldCacheEntry
    *sNdsRendererAdapterStageWorldCache;
static u32 sNdsRendererAdapterStageWorldCacheCount;
static u32 sNdsRendererAdapterStageWorldCacheAllocationAttempted;
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
/* Lab builds only: the free heap each optional world cache saw at its last
 * allocation attempt and whether it took it; against the match low-water
 * this sizes the reserve the attempt keeps. */
__attribute__((used)) volatile u32 gNdsDObjWorldCacheFreeAtAttempt;
__attribute__((used)) volatile u32 gNdsStageWorldCacheFreeAtAttempt;
__attribute__((used)) volatile u32 gNdsWorldCacheAllocatedMask;
#endif
static u32 sNdsRendererAdapterStageWorldNextGeneration;
/* Persistent world entries rebuilt since boot: a caller that reads it around
 * ndsRendererAdapterPersistentStageWorldPtr learns whether its chain moved. */
static u32 sNdsRendererAdapterStageWorldRebuilds;
static u8 sNdsRendererAdapterStageWorldIndex[
    NDS_RENDERER_ADAPTER_STAGE_WORLD_INDEX_COUNT];

static sb32 ndsRendererAdapterRangeIsEmptySegmentEDL(const Gfx *dl,
                                                     size_t bytes)
{
    uintptr_t base = (uintptr_t)sNdsRendererAdapterEmptySegmentEDL;
    uintptr_t addr = (uintptr_t)dl;
    size_t size = sizeof(sNdsRendererAdapterEmptySegmentEDL);

    if ((dl == NULL) || (addr < base) || (addr > (base + size)))
    {
        return FALSE;
    }
    return (bytes <= (size - (size_t)(addr - base))) ? TRUE : FALSE;
}
