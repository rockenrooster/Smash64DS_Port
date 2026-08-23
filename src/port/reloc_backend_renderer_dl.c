#include <sys/matrix.h>

#include <nds/nds_task37_itcm.h>
#include <nds/nds_effects.h>
#include <nds/nds_fighter_matrix_index.h>
#include <nds/nds_r2_camera_fixed.h>
#include <nds/nds_fcmp.h>
#if NDS_R2_FOX_GUN_OVERLAY
#include <nds/nds_fox_gun.h>
#endif
#include <nds/nds_ifcommon_oam.h>
#include <ft/ftdata_file_slots.h>

#ifndef NDS_RENDERER_HW_TRIANGLES
#define NDS_RENDERER_HW_TRIANGLES 0
#endif

#if NDS_RENDERER_HW_TRIANGLES
#include <nds/timers.h>
#endif

/* Task 86: see ndsRendererMatrixCopy20p12 in include/nds/nds_renderer.h. Gated
 * so the A/B is one flag, per the Task 79 rule. The definition sits outside the
 * NDS_RENDERER_HW_TRIANGLES block because ndsRendererAdapterPrepareInitialMatrices
 * uses MTXCOPY on both sides of that switch; with it inside, the canonical
 * software target did not compile at all. Folding the triangle flag into the
 * condition keeps the hardware ROM's expansion exactly as it was. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_TASK86_MATRIX_COPY
#define MTXCOPY(d, s) ndsRendererMatrixCopy20p12((d), (s))
#else
#define MTXCOPY(d, s) (*(d) = *(s))
#endif

#define NDS_RENDERER_ADAPTER_MTX_FRAC_BITS 12
#define NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX 32u
#define NDS_RENDERER_ADAPTER_MATERIAL_MOBJ_MAX 64u
/* Mario/Fox top out at four MObjs on one selected model root. Luigi root 4
 * carries six: the source-derived AOT program reaches material_slot 5, and the
 * live BattleShip MObj chain independently counts six nodes. This is renderer
 * capacity, not a Mario/Fox semantic, so size the generic native-owner
 * workspace for the largest admitted source root rather than forcing a valid
 * fighter through the generic display-list interpreter. */
#define NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX 6u
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
#define NDS_RENDERER_ADAPTER_G_TX_LOADTILE 7u
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
#define NDS_RENDERER_ADAPTER_G_MWO_POINT_ST 0x14u
#define NDS_RENDERER_ADAPTER_G_TX_LDBLK_MAX_TXL 2047u
/* dLBCommonFuncMatrixList is consumed as syMtxProcess pairs; kind 0x4B maps
 * to lbCommonFighterPartsFuncMatrix in BattleShip. */
#define NDS_RENDERER_ADAPTER_FIGHTER_PARTS_MTX_KIND 0x4Bu
/* wpmariofireball.c specifies hexadecimal 0x47 (decimal 71), which maps to
 * func_ovl0_800CA5C8 rather than enum nGCMatrixKind47 (decimal 47). */
#define NDS_RENDERER_ADAPTER_MVP_RECALC_RPY_0X47_KIND 0x47u
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
/* dLBCommonFuncMatrixList kind 0x4F maps to func_ovl0_800C994C: the DObj's local
 * matrix IS the world matrix of the joint it is bound to. See
 * ndsRendererAdapterBuildJointAttachMtx. */
#define NDS_RENDERER_ADAPTER_JOINT_ATTACH_MTX_KIND 0x4Fu

#if NDS_R2_IMPACT_WAVE_NATIVE
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
#endif
#if NDS_R2_REBIRTH_HALO_NATIVE
static sb32 sNdsRendererAdapterRebirthHaloNativeActive;
#if NDS_R2_REBIRTH_HALO_FAST_ADAPTER
static sb32 sNdsRendererAdapterRebirthHaloSkipSecondChildList;
#endif
#endif
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

#if NDS_R2_SECOND_ENTRY_DIAG
/* Native stage owner admission split. Declared up here, ahead of every use:
 * this file is #included into scene_backend.c, so a definition placed next to
 * the function it serves lands AFTER the earlier consumers and does not
 * compile. A second scene entry MUST show the rebuild count advance; a run
 * where only the steady-admit count moves across the entry is one where last
 * match's binding_dobjs[] were re-admitted wholesale. */
volatile u32 gNdsR2StageSteadyAdmitCount;
volatile u32 gNdsR2StageTopologyRebuildCount;
/* First stage-material rejection of the run, latched. RejectIndex stays at
 * 0xFFFFFFFF while nothing has been rejected, so "no rejection" is
 * distinguishable from "rejected binding 0". */
volatile u32 gNdsR2StageMaterialRejectCount;
volatile u32 gNdsR2StageMaterialRejectIndex = 0xFFFFFFFFu;
volatile u32 gNdsR2StageMaterialRejectBinding;
volatile u32 gNdsR2StageMaterialRejectDObj;
volatile u32 gNdsR2StageMaterialRejectMObj;
volatile u32 gNdsR2StageMaterialRejectFlagsWant;
volatile u32 gNdsR2StageMaterialRejectFlagsGot;
volatile u32 gNdsR2StageMaterialRejectHeapGen;
#endif

static u32 ndsRendererAdapterFighterColorModulate(const FTStruct *fp)
{
#if NDS_TASK39_FX_FLASH
    if ((fp != NULL) && (fp->colanim.is_use_color1 != FALSE))
    {
        const GMColKeys *color = &fp->colanim.color1;

        gNdsTask39FxFlashDrawCount++;
        ndsTask39EffectsEngage(NDS_TASK39_FX_ENGAGED_FLASH);
        return ((u32)color->r << 24) | ((u32)color->g << 16) |
               ((u32)color->b << 8) | color->a;
    }
#else
    (void)fp;
#endif
    return 0u;
}

#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL < 2
static NDSRendererNativeMaterial
    sNdsRendererAdapterNativeOwnerMaterials[
        NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED]
        [NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX];
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
void ndsFighterRendererInvalidateMaterialCaches(void)
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
static NDSRendererMatrix20p12
    sNdsRendererAdapterNativeOwnerProjection;
static NDSRendererMatrix20p12
    sNdsRendererAdapterNativeOwnerModelviews[
        NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
#else
static NDSRendererNativeMaterial
    sNdsRendererAdapterNativeMaterials[
        NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX];
#endif

typedef struct NDSRendererAdapterNativeOwnerWorkspace
{
    NDSRelocLoadedFile *loaded[
        NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
#if NDS_RENDERER_PROFILE_LEVEL < 2
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
    NDSRendererMatrix20p12 hierarchy_locals[
        NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
#if NDS_R2_FIGHTER_GX_COMPOSE
    /* Slice 43. Per-binding descriptors for the GX compose, plus the chains
     * themselves. This does NOT reuse `hierarchy_locals`: that array is sized at
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
#endif
    NDSRendererMatrix20p12 hierarchy_projection;
    NDSRendererMatrix20p12 hierarchy_camera_modelview;
    NDSRendererConfig hierarchy_config;
    NDSRendererNativeFighterHierarchy hierarchy;
#endif
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

#if NDS_RENDERER_PROFILE_LEVEL < 2
#define NDS_RENDERER_ADAPTER_STAGE_SEGMENT_COUNT 8u
#define NDS_RENDERER_ADAPTER_STAGE_DOBJ_COUNT 57u
#define NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT 42u
#define NDS_RENDERER_ADAPTER_STAGE_ASSET_COUNT 4u
#define NDS_RENDERER_ADAPTER_STAGE_MATERIAL_COUNT 4u

typedef struct NDSRendererAdapterNativeStageWorkspace
{
    NDSRelocLoadedFile *loaded[NDS_RENDERER_ADAPTER_STAGE_ASSET_COUNT];
    GObj *segments[NDS_RENDERER_ADAPTER_STAGE_SEGMENT_COUNT];
    DObj *dobjs[NDS_RENDERER_ADAPTER_STAGE_DOBJ_COUNT];
    DObj *binding_dobjs[NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT];
    NDSRendererNativeStageDObj live_dobjs[
        NDS_RENDERER_ADAPTER_STAGE_DOBJ_COUNT];
    const void *binding_display_lists[
        NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT];
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 camera_modelview;
    NDSRendererMatrix20p12 binding_world[
        NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT];
    NDSRendererMatrix20p12 binding_composed[
        NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT];
#if NDS_TASK36_HW_COMPOSE
    NDSRendererAdapterStageWorldSourceKey task36_rigid_source_keys[
        NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT];
    u64 task36_runtime_rigid_mask;
#if NDS_TASK44_STAGE_STEADY
    /* Task 44 item 4: the rigid/dynamic partition of the 42 bindings is fixed
     * for one topology, so build it once at stage capture. Steady-state matrix
     * preparation and rigid validation then walk their own dense list instead
     * of scanning all 42 and re-testing the 64-bit mask per entry. */
    u8 task44_rigid_bindings[NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT];
    u8 task44_dynamic_bindings[NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT];
    u8 task44_rigid_binding_count;
    u8 task44_dynamic_binding_count;
    u8 task44_binding_lists_valid;
#if NDS_R2_STAGE_VALIDATE_STRIDE
    /* Slice 44. Which residue class of the stride is revalidated this frame.
     * Advanced once per PrepareNativeStageOwner, which is once per frame --
     * the c120 profile pins that at 26 rigid checks per frame against a
     * 26-entry list. Both the rigid sweep and the dynamic chain walk read it,
     * so a binding's rigid proof and its world rebuild land on the same frame
     * and the per-frame cost stays flat instead of spiking every Nth frame. */
    u8 slice44_validate_cursor;
#endif
#endif
#endif
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
#if NDS_TASK44_STAGE_STEADY
    /* Task 44 item 3: the stage-asset mutation generation this workspace was
     * last fully validated against. Zero means "never admitted". */
    u32 task44_admission_generation;
    /* Per-segment root DObj recorded at collect time, so steady admission can
     * catch a display graph rebuilt underneath an unchanged segment GObj. */
    DObj *task44_segment_roots[NDS_RENDERER_ADAPTER_STAGE_SEGMENT_COUNT];
#endif
} NDSRendererAdapterNativeStageWorkspace;

/* The stage owner remains armed across fighter links 9, so it cannot share
 * the complete-fighter workspace. */
static NDSRendererAdapterNativeStageWorkspace
    sNdsRendererAdapterNativeStageWorkspace;
#endif

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
static sb32 ndsRendererAdapterCollectFighterTopology(
    DObj *dobj,
    u32 parent_index,
    DObj **joints,
    u8 *joint_parents,
    u32 *joint_count);
#endif

#if NDS_RENDERER_PROFILE_LEVEL < 2
typedef struct NDSRendererAdapterNativeOwnerValidationCache
{
    const void *data;
    u32 asset_id;
    u32 owner_generation;
    u32 data_size;
    u32 root_count;
    u32 use_low_detail;
    u32 root_offsets[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    u32 material_counts[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    u32 valid;
} NDSRendererAdapterNativeOwnerValidationCache;

/* The generated tables and relocated owner payload are immutable for one
 * reloc generation.  Prove every selected root/material identity each draw,
 * but do the full generated-array/span walk only when that identity changes. */
static NDSRendererAdapterNativeOwnerValidationCache
    sNdsRendererAdapterNativeOwnerValidationCache[
        NDS_RENDERER_NATIVE_FIGHTER_OWNER_COUNT];
#endif

typedef struct NDSRendererAdapterCameraCacheEntry
{
    const CObj *cobj;
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    u32 projection_valid;
    u32 modelview_valid;
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
#if NDS_R2_STAGE_VALIDATE_STRIDE
#if !NDS_TASK36_HW_COMPOSE || !NDS_TASK44_STAGE_STEADY
#error "NDS_R2_STAGE_VALIDATE_STRIDE needs NDS_TASK36_HW_COMPOSE and NDS_TASK44_STAGE_STEADY: the cursor lives in the Task 44 dense-list workspace and the sweep it strides is the Task 36 rigid guard."
#endif
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
#endif
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
static u32 sNdsRendererAdapterStageWorldNextGeneration;
static u8 sNdsRendererAdapterStageWorldIndex[
    NDS_RENDERER_ADAPTER_STAGE_WORLD_INDEX_COUNT];
#endif

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

static void ndsRendererAdapterMtxIdentity20p12(
    NDSRendererMatrix20p12 *out)
{
    if (out == NULL)
    {
        return;
    }

    /* R2-03 E69. Was `memset(out, 0, 64)` plus a four-iteration diagonal loop --
     * a library call and a loop to write twelve zeros. */
    ndsRendererMatrixIdentity20p12(
        out, 1 << NDS_RENDERER_ADAPTER_MTX_FRAC_BITS);
}

static void ndsRendererAdapterMulInto(NDSRendererMatrix20p12 *target,
                                      const NDSRendererMatrix20p12 *incoming,
                                      u32 *valid)
{
    if ((target == NULL) || (incoming == NULL) || (valid == NULL))
    {
        return;
    }

    if (*valid != 0u)
    {
        ndsRendererMtxMul20p12(target, incoming, target);
    }
    else
    {
        /* R2-03 E69. `*target = *incoming` is 64 bytes and GCC answers it with
         * `bl memcpy`; this function is inlined into
         * ndsRendererAdapterBuildDObjLocalMatrix, which E68b measured as 18.6%
         * of the whole memset/memcpy class. Task 86's helper already exists. */
        ndsRendererMatrixCopy20p12(target, incoming);
        *valid = TRUE;
    }
}

static void ndsRendererAdapterMulBefore(NDSRendererMatrix20p12 *target,
                                        const NDSRendererMatrix20p12 *incoming,
                                        u32 *valid)
{
    if ((target == NULL) || (incoming == NULL) || (valid == NULL))
    {
        return;
    }

    if (*valid != 0u)
    {
        ndsRendererMtxMul20p12(incoming, target, target);
    }
    else
    {
        ndsRendererMatrixCopy20p12(target, incoming);
        *valid = TRUE;
    }
}

static void ndsRendererAdapterMtxFromN64(
    const Mtx *src, NDSRendererMatrix20p12 *dst)
{
    if ((src == NULL) || (dst == NULL))
    {
        return;
    }
    ndsRendererMtxLoadN64ToDS20p12(src, dst);
}

/* The renderer's camera producers, routed.  Every site below ran
 *
 *     syMatrixLookAtReflect(&mtx, &look_at, ...)   float chain + syMatrixF2L
 *     ndsRendererAdapterMtxFromN64(&mtx, out)      s15.16 -> Q20.12
 *
 * which is float -> 16.16 -> 12.12 for a consumer that only ever sees twelve
 * fractional bits.  The Q20.12 arm produces the SAME representable value with
 * no float and no intermediate Mtx, so syMatrixF2L disappears for 4.138 of its
 * 6.138 entries a frame rather than being converted.
 *
 * The route word is read, not compiled: gNdsR2CameraFixedEnabled lives in
 * `.data` so one binary carries both arms with byte-identical placement.  See
 * include/nds/nds_r2_camera_fixed.h. */
static void ndsRendererAdapterCameraLookAtReflect(
    NDSRendererMatrix20p12 *out, LookAt *look_at,
    f32 eye_x, f32 eye_y, f32 eye_z,
    f32 at_x, f32 at_y, f32 at_z,
    f32 up_x, f32 up_y, f32 up_z)
{
    Mtx mtx;

    if (gNdsR2CameraFixedEnabled != 0u)
    {
        /* NULL, not `look_at`: every caller here passes a stack local that
         * nothing reads, so the six reflectance conversions are dead work the
         * float arm still pays and this arm skips. */
        (void)look_at;
        ndsR2CameraLookAtReflect20p12(out, NULL, eye_x, eye_y, eye_z,
                                      at_x, at_y, at_z, up_x, up_y, up_z);
        return;
    }
    gNdsR2CameraFixedFloatLookAtCalls++;
    syMatrixLookAtReflect(&mtx, look_at, eye_x, eye_y, eye_z,
                          at_x, at_y, at_z, up_x, up_y, up_z);
    ndsRendererAdapterMtxFromN64(&mtx, out);
}

static void ndsRendererAdapterCameraPerspFast(
    NDSRendererMatrix20p12 *out, u16 *persp_norm,
    f32 fovy, f32 aspect, f32 near, f32 far, f32 scale)
{
    Mtx mtx;

    if (gNdsR2CameraFixedEnabled != 0u)
    {
        ndsR2CameraPerspFast20p12(out, persp_norm, fovy, aspect, near, far,
                                  scale);
        return;
    }
    gNdsR2CameraFixedFloatPerspCalls++;
    syMatrixPerspFast(&mtx, persp_norm, fovy, aspect, near, far, scale);
    ndsRendererAdapterMtxFromN64(&mtx, out);
}

static inline sb32 ndsRendererAdapterFloatPow2ToS32(
    f32 value, u32 scale_bits, s32 *out)
{
    union
    {
        f32 f;
        u32 u;
    } bits = { value };
    u32 exponent = (bits.u >> 23) & 0xffu;
    u32 magnitude;
    s32 shift;

    if ((out == NULL) || (exponent == 0xffu))
    {
        return FALSE;
    }
    if (exponent == 0u)
    {
        *out = 0;
        return TRUE;
    }

    magnitude = (bits.u & 0x7fffffu) | 0x800000u;
    shift = (s32)exponent - 127 - 23 + (s32)scale_bits;
    if (shift < 0)
    {
        magnitude = (shift <= -24) ? 0u : magnitude >> (u32)-shift;
    }
    else
    {
        if (shift > 8)
        {
            return FALSE;
        }
        magnitude <<= (u32)shift;
        if (((bits.u & 0x80000000u) == 0u) &&
            (magnitude > 0x7fffffffu))
        {
            return FALSE;
        }
    }

    *out = ((bits.u & 0x80000000u) != 0u) ?
        (s32)(0u - magnitude) : (s32)magnitude;
    return TRUE;
}

#if NDS_R2_FIGHTER_MTX_DIRECT
/* R2-03 E9. The fighter-parts path converts float -> N64 16.16 -> DS 20.12, and
 * E8 established that 97.5% of local-matrix builds take it. The intermediate is
 * a pure round trip: F2LFixedWExact packs each value into COMBINE_INTEGRAL /
 * COMBINE_FRACTIONAL halves, and MtxCellS16p16 recombines exactly those halves,
 * so recombine(split(x)) == x. The whole two-step therefore equals, per cell,
 * RoundShiftS32(FloatPow2ToS32(v, 16), 4) -- and the cell mapping works out to
 * the identity, with the w column zero except (3,3) = 1.0.
 *
 * Computing that directly is bit-exact by construction rather than by tolerance,
 * so unlike E6's fixed-point angle lever this needs no fidelity budget. Level 2
 * runs both and compares anyway, because "by construction" is exactly the kind
 * of claim E8 proved I get wrong by reading. */
#if NDS_R2_FIGHTER_MTX_DIRECT >= 2
/* E10 counts the TraRotRpy branch separately: the two paths reach 20.12 by
 * different arithmetic and a shared counter would hide one refuting while the
 * other passed.
 *
 * These six live only at the verify level now. At the shipped level 1 they were
 * six `volatile u32` increments -- a forced load-modify-store the compiler may
 * not sink or fold -- on the hottest local-matrix path in the fighter draw.
 * gNdsR2MtxDirectRpyCalls alone measured 481 ticks/frame over 84,308 executions
 * in the c106 profile. Both experiments have long since published: the direct
 * path is bit-exact by construction, and the fallback ran 0 times in 101,569
 * builder calls. Nothing in scripts/ or docs/ reads any of them. */
volatile u32 gNdsR2MtxDirectCalls;
volatile u32 gNdsR2MtxDirectFallback;
volatile u32 gNdsR2MtxDirectVerifyFail;
volatile u32 gNdsR2MtxDirectRpyCalls;
volatile u32 gNdsR2MtxDirectRpyFallback;
volatile u32 gNdsR2MtxDirectRpyVerifyFail;
#endif

static s32 ndsRendererAdapterRoundShift20p12(s32 value)
{
    /* Mirrors nds_renderer.c's ndsRendererRoundShiftS32(value, 4), which is
     * file-static there: round half away from zero, not an arithmetic shift. */
    s64 wide = value;

    if (wide < 0)
    {
        return (s32)(-(((-wide) + 8) >> 4));
    }
    return (s32)((wide + 8) >> 4);
}

static sb32 ndsRendererAdapterF2LDirect20p12(
    const Mtx44f *src, NDSRendererMatrix20p12 *dst)
{
    u32 row;
    u32 col;
    s32 e;

    for (row = 0u; row < 4u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            if (ndsRendererAdapterFloatPow2ToS32(
                    (*src)[row][col], 16u, &e) == FALSE)
            {
                return FALSE;
            }
            dst->m[row][col] = ndsRendererAdapterRoundShift20p12(e);
        }
        dst->m[row][3] = 0;
    }
    /* F2LFixedWExact pairs src[3][2] with a literal 0x00010000, i.e. 1.0 in
     * 16.16, which lands in cell (3,3). */
    dst->m[3][3] = ndsRendererAdapterRoundShift20p12(0x00010000);
    return TRUE;
}
#endif

static sb32 ndsRendererAdapterF2LFixedWExact(
    const Mtx44f *src, Mtx *dst)
{
    s32 e1;
    s32 e2;

#define NDS_F2L_FIXED(value, output) \
    do \
    { \
        if (ndsRendererAdapterFloatPow2ToS32((value), 16u, &(output)) == FALSE) \
        { \
            return FALSE; \
        } \
    } while (0)

    NDS_F2L_FIXED((*src)[0][0], e1);
    NDS_F2L_FIXED((*src)[0][1], e2);
    dst->m[0][0] = COMBINE_INTEGRAL((u32)e1, (u32)e2);
    dst->m[2][0] = COMBINE_FRACTIONAL((u32)e1, (u32)e2);
    NDS_F2L_FIXED((*src)[0][2], e1);
    dst->m[0][1] = COMBINE_INTEGRAL((u32)e1, 0u);
    dst->m[2][1] = COMBINE_FRACTIONAL((u32)e1, 0u);
    NDS_F2L_FIXED((*src)[1][0], e1);
    NDS_F2L_FIXED((*src)[1][1], e2);
    dst->m[0][2] = COMBINE_INTEGRAL((u32)e1, (u32)e2);
    dst->m[2][2] = COMBINE_FRACTIONAL((u32)e1, (u32)e2);
    NDS_F2L_FIXED((*src)[1][2], e1);
    dst->m[0][3] = COMBINE_INTEGRAL((u32)e1, 0u);
    dst->m[2][3] = COMBINE_FRACTIONAL((u32)e1, 0u);
    NDS_F2L_FIXED((*src)[2][0], e1);
    NDS_F2L_FIXED((*src)[2][1], e2);
    dst->m[1][0] = COMBINE_INTEGRAL((u32)e1, (u32)e2);
    dst->m[3][0] = COMBINE_FRACTIONAL((u32)e1, (u32)e2);
    NDS_F2L_FIXED((*src)[2][2], e1);
    dst->m[1][1] = COMBINE_INTEGRAL((u32)e1, 0u);
    dst->m[3][1] = COMBINE_FRACTIONAL((u32)e1, 0u);
    NDS_F2L_FIXED((*src)[3][0], e1);
    NDS_F2L_FIXED((*src)[3][1], e2);
    dst->m[1][2] = COMBINE_INTEGRAL((u32)e1, (u32)e2);
    dst->m[3][2] = COMBINE_FRACTIONAL((u32)e1, (u32)e2);
    NDS_F2L_FIXED((*src)[3][2], e1);
    dst->m[1][3] = COMBINE_INTEGRAL((u32)e1, 0x00010000u);
    dst->m[3][3] = COMBINE_FRACTIONAL((u32)e1, 0x00010000u);

#undef NDS_F2L_FIXED
    return TRUE;
}

static inline s32 ndsRendererAdapterFighterSinFromIndex(s32 index)
{
    u32 id = (u32)index & 0xfffu;
    s32 value = (s32)gSYSinTable[id & 0x7ffu];

    return ((id & 0x800u) != 0u) ? -value : value;
}

static inline s32 ndsRendererAdapterFighterCosFromIndex(s32 index)
{
    return ndsRendererAdapterFighterSinFromIndex(index + 0x400);
}

static NDS_RENDERER_ADAPTER_FIGHTER_MATRIX_CODE sb32
ndsRendererAdapterBuildFighterTraRotRpyExact(
    Mtx *mtx,
    f32 tx,
    f32 ty,
    f32 tz,
    f32 r,
    f32 p,
    f32 y)
{
    s32 indexr;
    s32 indexp;
    s32 indexy;
    s32 sinr;
    s32 sinp;
    s32 siny;
    s32 cosr;
    s32 cosp;
    s32 cosy;
    s32 fixed_x;
    s32 fixed_y;
    s32 fixed_z;
    u32 e1;
    u32 e2;

    if ((mtx == NULL) ||
        (ndsFighterMatrixAngleToIndexExact(r, &indexr) == 0) ||
        (ndsFighterMatrixAngleToIndexExact(p, &indexp) == 0) ||
        (ndsFighterMatrixAngleToIndexExact(y, &indexy) == 0) ||
        (ndsRendererAdapterFloatPow2ToS32(tx, 16u, &fixed_x) == FALSE) ||
        (ndsRendererAdapterFloatPow2ToS32(ty, 16u, &fixed_y) == FALSE) ||
        (ndsRendererAdapterFloatPow2ToS32(tz, 16u, &fixed_z) == FALSE))
    {
        return FALSE;
    }

    sinr = ndsRendererAdapterFighterSinFromIndex(indexr);
    cosr = ndsRendererAdapterFighterCosFromIndex(indexr);
    sinp = ndsRendererAdapterFighterSinFromIndex(indexp);
    cosp = ndsRendererAdapterFighterCosFromIndex(indexp);
    siny = ndsRendererAdapterFighterSinFromIndex(indexy);
    cosy = ndsRendererAdapterFighterCosFromIndex(indexy);

    e1 = (u32)((cosp * cosy) >> 14);
    e2 = (u32)((cosp * siny) >> 14);
    mtx->m[0][0] = COMBINE_INTEGRAL(e1, e2);
    mtx->m[2][0] = COMBINE_FRACTIONAL(e1, e2);

    e1 = (u32)(-sinp * 2);
    mtx->m[0][1] = COMBINE_INTEGRAL(e1, 0u);
    mtx->m[2][1] = COMBINE_FRACTIONAL(e1, 0u);

    e1 = (u32)((((sinr * sinp) >> 15) * cosy) >> 14) -
         (u32)((cosr * siny) >> 14);
    e2 = (u32)((((sinr * sinp) >> 15) * siny) >> 14) +
         (u32)((cosr * cosy) >> 14);
    mtx->m[0][2] = COMBINE_INTEGRAL(e1, e2);
    mtx->m[2][2] = COMBINE_FRACTIONAL(e1, e2);

    e1 = (u32)((sinr * cosp) >> 14);
    mtx->m[0][3] = COMBINE_INTEGRAL(e1, 0u);
    mtx->m[2][3] = COMBINE_FRACTIONAL(e1, 0u);

    e1 = (u32)((((cosr * sinp) >> 15) * cosy) >> 14) +
         (u32)((sinr * siny) >> 14);
    e2 = (u32)((((cosr * sinp) >> 15) * siny) >> 14) -
         (u32)((sinr * cosy) >> 14);
    mtx->m[1][0] = COMBINE_INTEGRAL(e1, e2);
    mtx->m[3][0] = COMBINE_FRACTIONAL(e1, e2);

    e1 = (u32)((cosr * cosp) >> 14);
    mtx->m[1][1] = COMBINE_INTEGRAL(e1, 0u);
    mtx->m[3][1] = COMBINE_FRACTIONAL(e1, 0u);

    mtx->m[1][2] = COMBINE_INTEGRAL((u32)fixed_x, (u32)fixed_y);
    mtx->m[3][2] = COMBINE_FRACTIONAL((u32)fixed_x, (u32)fixed_y);
    mtx->m[1][3] = COMBINE_INTEGRAL((u32)fixed_z, 0x00010000u);
    mtx->m[3][3] = COMBINE_FRACTIONAL((u32)fixed_z, 0x00010000u);
    return TRUE;
}

#if NDS_R2_FIGHTER_MTX_DIRECT
/* R2-03 E10. Same observation as E9, applied to the branch E9 did not touch.
 * This builder already works in fixed point -- the sin/cos table lookup E6
 * proposed exists here already -- and then packs its s16.16 results into the N64
 * split format purely so MtxFromN64 can unpack them again. The pairing is the
 * same as F2LFixedWExact's, so the cell mapping is again the identity, and
 * writing 20.12 straight out is bit-exact by construction.
 *
 * Kept as a separate function rather than parameterising the original: the two
 * differ only in their store, and a shared version would need a branch per cell
 * in the hottest loop of the phase. */
static NDS_RENDERER_ADAPTER_FIGHTER_MATRIX_CODE NDS_R2_ITCM_PACK2_CODE sb32
ndsRendererAdapterBuildFighterTraRotRpyDirect20p12(
    NDSRendererMatrix20p12 *dst,
    f32 tx,
    f32 ty,
    f32 tz,
    f32 r,
    f32 p,
    f32 y)
{
    s32 indexr;
    s32 indexp;
    s32 indexy;
    s32 sinr;
    s32 sinp;
    s32 siny;
    s32 cosr;
    s32 cosp;
    s32 cosy;
    s32 fixed_x;
    s32 fixed_y;
    s32 fixed_z;

    if ((dst == NULL) ||
        (ndsFighterMatrixAngleToIndexExact(r, &indexr) == 0) ||
        (ndsFighterMatrixAngleToIndexExact(p, &indexp) == 0) ||
        (ndsFighterMatrixAngleToIndexExact(y, &indexy) == 0) ||
        (ndsRendererAdapterFloatPow2ToS32(tx, 16u, &fixed_x) == FALSE) ||
        (ndsRendererAdapterFloatPow2ToS32(ty, 16u, &fixed_y) == FALSE) ||
        (ndsRendererAdapterFloatPow2ToS32(tz, 16u, &fixed_z) == FALSE))
    {
        return FALSE;
    }

    sinr = ndsRendererAdapterFighterSinFromIndex(indexr);
    cosr = ndsRendererAdapterFighterCosFromIndex(indexr);
    sinp = ndsRendererAdapterFighterSinFromIndex(indexp);
    cosp = ndsRendererAdapterFighterCosFromIndex(indexp);
    siny = ndsRendererAdapterFighterSinFromIndex(indexy);
    cosy = ndsRendererAdapterFighterCosFromIndex(indexy);

#define NDS_R2_RS20P12(value) ndsRendererAdapterRoundShift20p12((s32)(value))

    dst->m[0][0] = NDS_R2_RS20P12((cosp * cosy) >> 14);
    dst->m[0][1] = NDS_R2_RS20P12((cosp * siny) >> 14);
    dst->m[0][2] = NDS_R2_RS20P12(-sinp * 2);
    dst->m[0][3] = 0;

    dst->m[1][0] = NDS_R2_RS20P12(
        ((((sinr * sinp) >> 15) * cosy) >> 14) - ((cosr * siny) >> 14));
    dst->m[1][1] = NDS_R2_RS20P12(
        ((((sinr * sinp) >> 15) * siny) >> 14) + ((cosr * cosy) >> 14));
    dst->m[1][2] = NDS_R2_RS20P12((sinr * cosp) >> 14);
    dst->m[1][3] = 0;

    dst->m[2][0] = NDS_R2_RS20P12(
        ((((cosr * sinp) >> 15) * cosy) >> 14) + ((sinr * siny) >> 14));
    dst->m[2][1] = NDS_R2_RS20P12(
        ((((cosr * sinp) >> 15) * siny) >> 14) - ((sinr * cosy) >> 14));
    dst->m[2][2] = NDS_R2_RS20P12((cosr * cosp) >> 14);
    dst->m[2][3] = 0;

    dst->m[3][0] = NDS_R2_RS20P12(fixed_x);
    dst->m[3][1] = NDS_R2_RS20P12(fixed_y);
    dst->m[3][2] = NDS_R2_RS20P12(fixed_z);
    dst->m[3][3] = NDS_R2_RS20P12(0x00010000);

#undef NDS_R2_RS20P12
    return TRUE;
}
#endif

static void ndsRendererAdapterBuildDObjFallbackMtx(DObj *dobj, Mtx *mtx)
{
    if ((dobj == NULL) || (mtx == NULL))
    {
        return;
    }

    syMatrixTraRotRpyRSca(mtx,
                          dobj->translate.vec.f.x,
                          dobj->translate.vec.f.y,
                          dobj->translate.vec.f.z,
                          dobj->rotate.vec.f.x,
                          dobj->rotate.vec.f.y,
                          dobj->rotate.vec.f.z,
                          dobj->scale.vec.f.x,
                          dobj->scale.vec.f.y,
                          dobj->scale.vec.f.z);
}

static f32 ndsRendererAdapterSquareF(f32 value)
{
    return value * value;
}

static void ndsRendererAdapterBuildBillboardMtxF(Mtx44f *mtx_f,
                                                 DObj *dobj,
                                                 u32 kind)
{
    CObj *cobj;
    f32 distx;
    f32 disty;
    f32 distz;
    f32 res;
    u32 is_translate;

    if ((mtx_f == NULL) || (dobj == NULL))
    {
        return;
    }

    memset(mtx_f, 0, sizeof(*mtx_f));
    (*mtx_f)[0][0] = 1.0F;
    (*mtx_f)[1][1] = 1.0F;
    (*mtx_f)[2][2] = 1.0F;
    (*mtx_f)[3][3] = 1.0F;

    cobj = (gGCCurrentCamera != NULL) ? CObjGetStruct(gGCCurrentCamera) : NULL;
    if (cobj == NULL)
    {
        return;
    }

    is_translate = (((kind & 1u) == 0u) ? TRUE : FALSE);
    distx = dobj->translate.vec.f.x - cobj->vec.eye.x;
    disty = dobj->translate.vec.f.y - cobj->vec.eye.y;
    distz = dobj->translate.vec.f.z - cobj->vec.eye.z;

    switch (kind)
    {
    case 33:
    case 34:
        res = sqrtf(ndsRendererAdapterSquareF(distx) +
                    ndsRendererAdapterSquareF(disty));
        (*mtx_f)[2][2] = 1.0F;
        if (res != 0.0F)
        {
            f32 inv = 1.0F / res;

            distx *= inv;
            disty *= inv;
            (*mtx_f)[0][0] = -distx;
            (*mtx_f)[0][1] = -disty;
            (*mtx_f)[1][0] = disty;
            (*mtx_f)[1][1] = -distx;
        }
        break;
    case 35:
    case 36:
        res = 1.0F / sqrtf(ndsRendererAdapterSquareF(distx) +
                           ndsRendererAdapterSquareF(disty) +
                           ndsRendererAdapterSquareF(distz));
        distx *= res;
        disty *= res;
        distz *= res;
        res = sqrtf(ndsRendererAdapterSquareF(distx) +
                    ndsRendererAdapterSquareF(disty));
        if (res != 0.0F)
        {
            f32 inv = 1.0F / res;

            (*mtx_f)[2][2] = res;
            (*mtx_f)[0][0] = -distx;
            (*mtx_f)[1][0] = disty * inv;
            (*mtx_f)[2][0] = -distx * distz * inv;
            (*mtx_f)[0][1] = -disty;
            (*mtx_f)[1][1] = -distx * inv;
            (*mtx_f)[2][1] = -disty * distz * inv;
            (*mtx_f)[0][2] = -distz;
        }
        break;
    case 37:
    case 38:
        res = sqrtf(ndsRendererAdapterSquareF(distx) +
                    ndsRendererAdapterSquareF(distz));
        if (res != 0.0F)
        {
            f32 inv = 1.0F / res;

            distx *= inv;
            distz *= inv;
            (*mtx_f)[0][2] = distx;
            (*mtx_f)[2][0] = -distx;
            (*mtx_f)[0][0] = -distz;
            (*mtx_f)[2][2] = -distz;
        }
        break;
    case 39:
    case 40:
        res = 1.0F / sqrtf(ndsRendererAdapterSquareF(distx) +
                           ndsRendererAdapterSquareF(disty) +
                           ndsRendererAdapterSquareF(distz));
        distx *= res;
        disty *= res;
        distz *= res;
        res = sqrtf(ndsRendererAdapterSquareF(distx) +
                    ndsRendererAdapterSquareF(distz));
        if (res != 0.0F)
        {
            f32 inv = 1.0F / res;

            (*mtx_f)[0][0] = -distz * inv;
            (*mtx_f)[1][0] = -disty * distx * inv;
            (*mtx_f)[2][0] = -distx;
            (*mtx_f)[1][1] = res;
            (*mtx_f)[2][1] = -disty;
            (*mtx_f)[0][2] = distx * inv;
            (*mtx_f)[1][2] = -disty * distz * inv;
            (*mtx_f)[2][2] = -distz;
        }
        break;
    default:
        break;
    }

    if (is_translate != FALSE)
    {
        (*mtx_f)[3][0] = dobj->translate.vec.f.x;
        (*mtx_f)[3][1] = dobj->translate.vec.f.y;
        (*mtx_f)[3][2] = dobj->translate.vec.f.z;
    }
}

static sb32 ndsRendererAdapterBuildBillboardMtx(DObj *dobj, u32 kind,
                                                Mtx *mtx)
{
    Mtx44f mtx_f;

    if ((dobj == NULL) || (mtx == NULL))
    {
        return FALSE;
    }

    ndsRendererAdapterBuildBillboardMtxF(&mtx_f, dobj, kind);
    syMatrixF2LFixedW(&mtx_f, mtx);
    return TRUE;
}

static sb32 ndsRendererAdapterBuildRecalcLocalMtx(DObj *dobj, u32 kind,
                                                  Mtx *mtx)
{
    if ((dobj == NULL) || (mtx == NULL))
    {
        return FALSE;
    }

    switch (kind)
    {
    case nGCMatrixKindRecalcRotPyrR:
        syMatrixRotPyrR(mtx,
                        dobj->rotate.vec.f.x,
                        dobj->rotate.vec.f.y,
                        dobj->rotate.vec.f.z);
        break;
    case nGCMatrixKindRecalcRotRpyR:
        syMatrixRotRpyR(mtx,
                        dobj->rotate.vec.f.x,
                        dobj->rotate.vec.f.y,
                        dobj->rotate.vec.f.z);
        break;
    case nGCMatrixKindRecalcRotPyrRSca:
        syMatrixTraRotPyrRSca(mtx,
                              0.0F, 0.0F, 0.0F,
                              dobj->rotate.vec.f.x,
                              dobj->rotate.vec.f.y,
                              dobj->rotate.vec.f.z,
                              dobj->scale.vec.f.x,
                              dobj->scale.vec.f.y,
                              dobj->scale.vec.f.z);
        break;
    case nGCMatrixKindRecalcRotRpyRSca:
        syMatrixTraRotRpyRSca(mtx,
                              0.0F, 0.0F, 0.0F,
                              dobj->rotate.vec.f.x,
                              dobj->rotate.vec.f.y,
                              dobj->rotate.vec.f.z,
                              dobj->scale.vec.f.x,
                              dobj->scale.vec.f.y,
                              dobj->scale.vec.f.z);
        break;
    case nGCMatrixKind45:
        syMatrixTraRotPyrRSca(mtx,
                              0.0F, 0.0F, 0.0F,
                              dobj->rotate.vec.f.x, 0.0F, 0.0F,
                              dobj->scale.vec.f.x,
                              dobj->scale.vec.f.y,
                              dobj->scale.vec.f.z);
        break;
    case nGCMatrixKind46:
        syMatrixTraRotRpyRSca(mtx,
                              0.0F, 0.0F, 0.0F,
                              0.0F, 0.0F, dobj->rotate.vec.f.z,
                              dobj->scale.vec.f.x,
                              dobj->scale.vec.f.y,
                              dobj->scale.vec.f.z);
        break;
    case nGCMatrixKind47:
    case nGCMatrixKind48:
    case nGCMatrixKind49:
    case nGCMatrixKind50:
        syMatrixSca(mtx,
                    dobj->scale.vec.f.x,
                    dobj->scale.vec.f.y,
                    dobj->scale.vec.f.z);
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

static sb32 ndsRendererAdapterBuildFighterPartsMtx(
    DObj *dobj, NDSRendererMatrix20p12 *out)
{
    FTParts *parts;
    Mtx mtx;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    Mtx oracle;
    sb32 has_oracle = FALSE;
#endif

    if ((dobj == NULL) || (out == NULL))
    {
        return FALSE;
    }

    parts = ftGetParts(dobj);
    if (parts == NULL)
    {
        return FALSE;
    }

    if (parts->transform_update_mode != 0)
    {
#if NDS_R2_FIGHTER_MTX_DIRECT
        {
            NDSRendererMatrix20p12 direct;

            if (ndsRendererAdapterF2LDirect20p12(
                    &parts->unk_dobjtrans_0x10, &direct) != FALSE)
            {
#if NDS_R2_FIGHTER_MTX_DIRECT >= 2
                gNdsR2MtxDirectCalls++;
                if (ndsRendererAdapterF2LFixedWExact(
                        &parts->unk_dobjtrans_0x10, &mtx) == FALSE)
                {
                    syMatrixF2LFixedW(&parts->unk_dobjtrans_0x10, &mtx);
                }
                ndsRendererAdapterMtxFromN64(&mtx, out);
                if (memcmp(out, &direct, sizeof(direct)) != 0)
                {
                    gNdsR2MtxDirectVerifyFail++;
                }
                return TRUE;
#else
                *out = direct;
                return TRUE;
#endif
            }
#if NDS_R2_FIGHTER_MTX_DIRECT >= 2
            gNdsR2MtxDirectFallback++;
#endif
        }
#endif
        /* BattleShip lbCommonFighterPartsFuncMatrix quantizes the complete
         * source float matrix through syMatrixF2LFixedW before the RSP sees
         * it. Preserve that 16.16 boundary, then convert to DS 20.12. */
        if (ndsRendererAdapterF2LFixedWExact(
                &parts->unk_dobjtrans_0x10, &mtx) == FALSE)
        {
            syMatrixF2LFixedW(&parts->unk_dobjtrans_0x10, &mtx);
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        syMatrixF2LFixedW(&parts->unk_dobjtrans_0x10, &oracle);
        has_oracle = TRUE;
#endif
    }
    else if (ndsFcmpNeC(dobj->scale.vec.f.x, 1.0F) ||
             ndsFcmpNeC(dobj->scale.vec.f.y, 1.0F) ||
             ndsFcmpNeC(dobj->scale.vec.f.z, 1.0F))
    {
        /* Bit-exact pattern compares (nds_fcmp.h): three __aeabi_fcmpeq
         * calls per unscaled joint otherwise, 76,616 executions a profile. */
        syMatrixTraRotRpyRSca(&mtx,
                              dobj->translate.vec.f.x,
                              dobj->translate.vec.f.y,
                              dobj->translate.vec.f.z,
                              dobj->rotate.vec.f.x,
                              dobj->rotate.vec.f.y,
                              dobj->rotate.vec.f.z,
                              dobj->scale.vec.f.x,
                              dobj->scale.vec.f.y,
                              dobj->scale.vec.f.z);
    }
    else
    {
#if NDS_R2_FIGHTER_MTX_DIRECT
        {
            NDSRendererMatrix20p12 direct;

            if (ndsRendererAdapterBuildFighterTraRotRpyDirect20p12(
                    &direct,
                    dobj->translate.vec.f.x,
                    dobj->translate.vec.f.y,
                    dobj->translate.vec.f.z,
                    dobj->rotate.vec.f.x,
                    dobj->rotate.vec.f.y,
                    dobj->rotate.vec.f.z) != FALSE)
            {
#if NDS_R2_FIGHTER_MTX_DIRECT >= 2
                gNdsR2MtxDirectRpyCalls++;
                if (ndsRendererAdapterBuildFighterTraRotRpyExact(
                        &mtx,
                        dobj->translate.vec.f.x,
                        dobj->translate.vec.f.y,
                        dobj->translate.vec.f.z,
                        dobj->rotate.vec.f.x,
                        dobj->rotate.vec.f.y,
                        dobj->rotate.vec.f.z) == FALSE)
                {
                    syMatrixTraRotRpyR(&mtx,
                                       dobj->translate.vec.f.x,
                                       dobj->translate.vec.f.y,
                                       dobj->translate.vec.f.z,
                                       dobj->rotate.vec.f.x,
                                       dobj->rotate.vec.f.y,
                                       dobj->rotate.vec.f.z);
                }
                ndsRendererAdapterMtxFromN64(&mtx, out);
                if (memcmp(out, &direct, sizeof(direct)) != 0)
                {
                    gNdsR2MtxDirectRpyVerifyFail++;
                }
                return TRUE;
#else
                *out = direct;
                return TRUE;
#endif
            }
#if NDS_R2_FIGHTER_MTX_DIRECT >= 2
            gNdsR2MtxDirectRpyFallback++;
#endif
        }
#endif
        if (ndsRendererAdapterBuildFighterTraRotRpyExact(
                &mtx,
                dobj->translate.vec.f.x,
                dobj->translate.vec.f.y,
                dobj->translate.vec.f.z,
                dobj->rotate.vec.f.x,
                dobj->rotate.vec.f.y,
                dobj->rotate.vec.f.z) == FALSE)
        {
            syMatrixTraRotRpyR(&mtx,
                               dobj->translate.vec.f.x,
                               dobj->translate.vec.f.y,
                               dobj->translate.vec.f.z,
                               dobj->rotate.vec.f.x,
                               dobj->rotate.vec.f.y,
                               dobj->rotate.vec.f.z);
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        syMatrixTraRotRpyR(&oracle,
                           dobj->translate.vec.f.x,
                           dobj->translate.vec.f.y,
                           dobj->translate.vec.f.z,
                           dobj->rotate.vec.f.x,
                           dobj->rotate.vec.f.y,
                           dobj->rotate.vec.f.z);
        has_oracle = TRUE;
#endif
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if ((has_oracle != FALSE) &&
        (memcmp(&mtx, &oracle, sizeof(mtx)) != 0))
    {
        gNdsRendererProfileOracleMismatches++;
    }
#endif
    ndsRendererAdapterMtxFromN64(&mtx, out);
    return TRUE;
}

/* Matrix kind 0x4F, the joint-attach transform. It is not an XObjTransformKind
 * enumerator: source routes every kind >= 66 through
 * sGCMatrixFuncList[kind - 66] (objdisplay.c:1161), and the battle task installs
 * dLBCommonFuncMatrixList, whose pair 13 is func_ovl0_800C994C
 * (lbcommon.c:1445). That callback ignores the DObj's own translate/rotate/scale
 * entirely and loads the matrix from the world matrix of the joint stored in
 * dobj->user_data.p:
 *
 *     attach = dobj->user_data.p;  parts = attach->user_data.p;
 *     func_ovl2_800EDBA4(attach);
 *     gmCollisionCopyMatrix(f, parts->mtx_translate);
 *     syMatrixF2LFixedW(&f, mtx);
 *
 * dEFManagerShieldEffectDesc's root DObj uses it (efmanager.c:462) and
 * efManagerShieldMakeEffect stores fp->joints[nFTPartsJointYRotN] there
 * (efmanager.c:4139). That is the whole attachment mechanism -- the shield has no
 * update proc that moves it, so without this case the root fell to
 * ndsRendererAdapterBuildDObjFallbackMtx, which for its identity vectors builds
 * the identity and parks the bubble at the world origin. Measured 2026-08-04 on
 * builds/build-c50-flag1 inside the tic 1994-1982 guard: k0=0x4f, user_data.p
 * non-NULL, root translate/rotate 0 and scale 1.
 *
 * func_ovl2_800EDBA4 FIRST, exactly as source does, and not as a formality:
 * mtx_translate is a per-frame cache that ndsFTParamsInvalidateFighterParts
 * clears, and the fighter's own draw (kind 0x4B) reads unk_dobjtrans_0x10 rather
 * than filling it, so the same probe read mtx_translate as all zeros. Copying it
 * without the rebuild would collapse the bubble to a point rather than move it.
 *
 * gmCollisionCopyMatrix's copy into a local is skipped: it writes columns 0-2
 * only, which is exactly what F2LFixedW reads. */
/* SOURCE'S gGCScaleX, AND IT IS NOT BOOKKEEPING. func_ovl0_800C994C's middle
 * line is `gGCScaleX = sqrtf(SQUARE(f[0][0]) + SQUARE(f[0][1]) +
 * SQUARE(f[0][2]))` -- the length of row 0 of the joint's world matrix -- and
 * the kind-44 billboard below multiplies the perspective by it. For the shield
 * that length is the SHIELD SIZE: ftCommonGuardUpdateShieldCollision writes
 * `((0.65 * shield_health/55) + 0.35) * attr->shield_size / 30` into
 * fp->joints[nFTPartsJointYRotN]->scale (ftcommonguard1.c:125), which for Fox
 * at full health is 280/30 = 9.3333 -- exactly the row-0 length this probe
 * measures. Dropping it makes the bubble a five-pixel dot, because the billboard
 * then scales the model by 1.0 instead of by the guard's own size.
 *
 * gcDrawDObjTreeDLLinksForGObj resets it to 1.0 per tree, which
 * ndsRendererAdapterPrepareInitialMatrices mirrors: the world build that calls
 * this runs first, the recalc that consumes it runs after, both inside one
 * call. */
static f32 sNdsRendererAdapterMvpRecalcScaleX = 1.0F;

static sb32 ndsRendererAdapterBuildJointAttachMtx(
    DObj *dobj, NDSRendererMatrix20p12 *out)
{
    DObj *attach;
    FTParts *parts;
    Mtx mtx;

    if ((dobj == NULL) || (out == NULL))
    {
        return FALSE;
    }
    attach = (DObj *)dobj->user_data.p;
    if ((attach == NULL) || (attach == DOBJ_PARENT_NULL) ||
        (attach->parent_gobj == NULL))
    {
        return FALSE;
    }
    parts = ftGetParts(attach);
    if (parts == NULL)
    {
        return FALSE;
    }
    /* func_ovl2_800EDBA4 walks to the joint root and dereferences ftGetStruct and
     * ftGetParts without a guard, so both are checked above before the call. */
    func_ovl2_800EDBA4(attach);
    sNdsRendererAdapterMvpRecalcScaleX =
        sqrtf((parts->mtx_translate[0][0] * parts->mtx_translate[0][0]) +
              (parts->mtx_translate[0][1] * parts->mtx_translate[0][1]) +
              (parts->mtx_translate[0][2] * parts->mtx_translate[0][2]));
    if (ndsRendererAdapterF2LFixedWExact(&parts->mtx_translate, &mtx) == FALSE)
    {
        syMatrixF2LFixedW(&parts->mtx_translate, &mtx);
    }
    ndsRendererAdapterMtxFromN64(&mtx, out);
    return TRUE;
}

/* Matrix kind 0x50, func_ovl0_800C99CC. Same attachment DATA as 0x4F -- the
 * joint DObj in user_data.p -- and deliberately different maths: source takes
 * only the joint's world POSITION and builds a pure translation, so the halo
 * neither inherits the fighter's yaw nor scales with him. Its size comes from
 * the child's own scale, which efManagerRebirthHaloMakeEffect writes from the
 * maker's argument (efmanager.c:6017).
 *
 * gmCollisionGetFighterPartsWorldPosition dereferences
 * ftGetStruct(main_dobj->parent_gobj) before testing anything, so the same two
 * guards the 0x4F builder uses are required here. It also refreshes the
 * per-frame transform cache itself (gmCollisionTransformMatrixAll when
 * transform_update_mode is 0), which is why this path needs no separate
 * func_ovl2_800EDBA4 call.
 *
 * Fills the caller's Mtx rather than the 20.12 output, so it shares the common
 * ndsRendererAdapterMtxFromN64 tail with every other syMatrix* case. */
static sb32 ndsRendererAdapterBuildJointAttachTraMtx(DObj *dobj, Mtx *out)
{
    DObj *attach;
    Vec3f translate_base = { 0.0F, 0.0F, 0.0F };

    if ((dobj == NULL) || (out == NULL))
    {
        return FALSE;
    }
    attach = (DObj *)dobj->user_data.p;
    if ((attach == NULL) || (attach == DOBJ_PARENT_NULL) ||
        (attach->parent_gobj == NULL) || (ftGetParts(attach) == NULL))
    {
        return FALSE;
    }
    gmCollisionGetFighterPartsWorldPosition(attach, &translate_base);
    syMatrixTra(out, translate_base.x, translate_base.y, translate_base.z);
    return TRUE;
}

static void ndsRendererAdapterGetDObjVectorTracks(
    DObj *dobj,
    GCTranslate **translate,
    GCRotate **rotate,
    GCScale **scale)
{
    uintptr_t cursor;
    u32 i;

    if ((dobj == NULL) || (translate == NULL) || (rotate == NULL) ||
        (scale == NULL))
    {
        return;
    }

    *translate = &dobj->translate;
    *rotate = &dobj->rotate;
    *scale = &dobj->scale;

    if (dobj->vec == NULL)
    {
        return;
    }

    cursor = (uintptr_t)dobj->vec->data;
    for (i = 0u; i < 3u; i++)
    {
        switch (dobj->vec->kinds[i])
        {
        case nGCDrawVectorKindTranslate:
            *translate = (GCTranslate *)cursor;
            cursor += sizeof(GCTranslate);
            break;
        case nGCDrawVectorKindRotate:
            *rotate = (GCRotate *)cursor;
            cursor += sizeof(GCRotate);
            break;
        case nGCDrawVectorKindScale:
            *scale = (GCScale *)cursor;
            cursor += sizeof(GCScale);
            break;
        default:
            break;
        }
    }
}

static sb32 ndsRendererAdapterBuildDObjXObjMatrix(
    DObj *dobj, XObj *xobj, NDSRendererMatrix20p12 *out)
{
    Mtx mtx;
    GCTranslate *translate = NULL;
    GCRotate *rotate = NULL;
    GCScale *scale = NULL;

    if ((dobj == NULL) || (xobj == NULL) || (out == NULL))
    {
        return FALSE;
    }

    /* Only the nGCMatrixKindVec* family reads these tracks; every other kind
     * takes its values straight off the DObj. The gather was unconditional and
     * walked the DObj's GCDrawVector list on every builder call -- 996
     * ticks/frame over 340,237 executions in the c106 profile. The family is
     * contiguous (nGCMatrixKindVecTra = 56 .. nGCMatrixKindVecTraRotRpyRSca =
     * 63), so the gate is a subtract and a compare. */
    if (((u32)xobj->kind - (u32)nGCMatrixKindVecTra) <=
        ((u32)nGCMatrixKindVecTraRotRpyRSca - (u32)nGCMatrixKindVecTra))
    {
        ndsRendererAdapterGetDObjVectorTracks(
            dobj, &translate, &rotate, &scale);
    }

    switch (xobj->kind)
    {
    case 1:
        /* Convert in place: the 64-byte struct copy into `mtx` was the
         * builder's only memcpy (13,810 calls a profile, 2026-08-23 census)
         * and the converter reads its source exactly once. */
        ndsRendererAdapterMtxFromN64(&xobj->mtx, out);
        return TRUE;
    case 2:
        return FALSE;
    case NDS_RENDERER_ADAPTER_MVP_RECALC_RPY_0X47_KIND:
        /* This source callback rewrites the current MVP rotation rows after
         * the ordinary translation matrix. It is applied after world*camera
         * composition, not multiplied as another affine local transform. */
        return FALSE;
    case nGCMatrixKindTra:
        syMatrixTra(&mtx, dobj->translate.vec.f.x,
                    dobj->translate.vec.f.y,
                    dobj->translate.vec.f.z);
        break;
    case nGCMatrixKindRotD:
        syMatrixRotD(&mtx, dobj->rotate.a, dobj->rotate.vec.f.x,
                     dobj->rotate.vec.f.y, dobj->rotate.vec.f.z);
        break;
    case nGCMatrixKindTraRotD:
        syMatrixTraRotD(&mtx, dobj->translate.vec.f.x,
                        dobj->translate.vec.f.y,
                        dobj->translate.vec.f.z,
                        dobj->rotate.a,
                        dobj->rotate.vec.f.x,
                        dobj->rotate.vec.f.y,
                        dobj->rotate.vec.f.z);
        break;
    case nGCMatrixKindRotRpyD:
        syMatrixRotRpyD(&mtx, dobj->rotate.vec.f.x,
                        dobj->rotate.vec.f.y,
                        dobj->rotate.vec.f.z);
        break;
    case nGCMatrixKindTraRotRpyD:
        syMatrixTraRotRpyD(&mtx, dobj->translate.vec.f.x,
                           dobj->translate.vec.f.y,
                           dobj->translate.vec.f.z,
                           dobj->rotate.vec.f.x,
                           dobj->rotate.vec.f.y,
                           dobj->rotate.vec.f.z);
        break;
    case nGCMatrixKindRotR:
        syMatrixRotR(&mtx, dobj->rotate.a, dobj->rotate.vec.f.x,
                     dobj->rotate.vec.f.y, dobj->rotate.vec.f.z);
        break;
    case nGCMatrixKindTraRotR:
        syMatrixTraRotR(&mtx, dobj->translate.vec.f.x,
                        dobj->translate.vec.f.y,
                        dobj->translate.vec.f.z,
                        dobj->rotate.a,
                        dobj->rotate.vec.f.x,
                        dobj->rotate.vec.f.y,
                        dobj->rotate.vec.f.z);
        break;
    case nGCMatrixKindTraRotRSca:
        syMatrixTraRotRSca(&mtx, dobj->translate.vec.f.x,
                           dobj->translate.vec.f.y,
                           dobj->translate.vec.f.z,
                           dobj->rotate.a,
                           dobj->rotate.vec.f.x,
                           dobj->rotate.vec.f.y,
                           dobj->rotate.vec.f.z,
                           dobj->scale.vec.f.x,
                           dobj->scale.vec.f.y,
                           dobj->scale.vec.f.z);
        break;
    case nGCMatrixKindRotRpyR:
        syMatrixRotRpyR(&mtx, dobj->rotate.vec.f.x,
                        dobj->rotate.vec.f.y,
                        dobj->rotate.vec.f.z);
        break;
    case nGCMatrixKindTraRotRpyR:
        syMatrixTraRotRpyR(&mtx, dobj->translate.vec.f.x,
                           dobj->translate.vec.f.y,
                           dobj->translate.vec.f.z,
                           dobj->rotate.vec.f.x,
                           dobj->rotate.vec.f.y,
                           dobj->rotate.vec.f.z);
        break;
    case nGCMatrixKindTraRotRpyRSca:
        syMatrixTraRotRpyRSca(&mtx, dobj->translate.vec.f.x,
                              dobj->translate.vec.f.y,
                              dobj->translate.vec.f.z,
                              dobj->rotate.vec.f.x,
                              dobj->rotate.vec.f.y,
                              dobj->rotate.vec.f.z,
                              dobj->scale.vec.f.x,
                              dobj->scale.vec.f.y,
                              dobj->scale.vec.f.z);
        break;
    case nGCMatrixKindRotPyrR:
        syMatrixRotPyrR(&mtx, dobj->rotate.vec.f.x,
                        dobj->rotate.vec.f.y,
                        dobj->rotate.vec.f.z);
        break;
    case nGCMatrixKindTraRotPyrR:
        syMatrixTraRotPyrR(&mtx, dobj->translate.vec.f.x,
                           dobj->translate.vec.f.y,
                           dobj->translate.vec.f.z,
                           dobj->rotate.vec.f.x,
                           dobj->rotate.vec.f.y,
                           dobj->rotate.vec.f.z);
        break;
    case nGCMatrixKindTraRotPyrRSca:
        syMatrixTraRotPyrRSca(&mtx, dobj->translate.vec.f.x,
                              dobj->translate.vec.f.y,
                              dobj->translate.vec.f.z,
                              dobj->rotate.vec.f.x,
                              dobj->rotate.vec.f.y,
                              dobj->rotate.vec.f.z,
                              dobj->scale.vec.f.x,
                              dobj->scale.vec.f.y,
                              dobj->scale.vec.f.z);
        break;
    case nGCMatrixKindSca:
        syMatrixSca(&mtx, dobj->scale.vec.f.x,
                    dobj->scale.vec.f.y,
                    dobj->scale.vec.f.z);
        break;
    case 33:
    case 34:
    case 35:
    case 36:
    case 37:
    case 38:
    case 39:
    case 40:
        ndsRendererAdapterBuildBillboardMtx(dobj, xobj->kind, &mtx);
        break;
    case nGCMatrixKindRecalcRotPyrR:
    case nGCMatrixKindRecalcRotRpyR:
    case nGCMatrixKindRecalcRotPyrRSca:
    case nGCMatrixKindRecalcRotRpyRSca:
    case nGCMatrixKind45:
    case nGCMatrixKind46:
    case nGCMatrixKind47:
    case nGCMatrixKind48:
    case nGCMatrixKind49:
    case nGCMatrixKind50:
        ndsRendererAdapterBuildRecalcLocalMtx(dobj, xobj->kind, &mtx);
        break;
    case NDS_RENDERER_ADAPTER_FIGHTER_PARTS_MTX_KIND:
        if (ndsRendererAdapterBuildFighterPartsMtx(dobj, out) != FALSE)
        {
            return TRUE;
        }
        ndsRendererAdapterBuildDObjFallbackMtx(dobj, &mtx);
        break;
    case NDS_RENDERER_ADAPTER_JOINT_ATTACH_MTX_KIND:
        if (ndsRendererAdapterBuildJointAttachMtx(dobj, out) != FALSE)
        {
            return TRUE;
        }
        ndsRendererAdapterBuildDObjFallbackMtx(dobj, &mtx);
        break;
    case NDS_RENDERER_ADAPTER_JOINT_ATTACH_TRA_MTX_KIND:
        if (ndsRendererAdapterBuildJointAttachTraMtx(dobj, &mtx) == FALSE)
        {
            ndsRendererAdapterBuildDObjFallbackMtx(dobj, &mtx);
        }
        break;
    case nGCMatrixKindVecTra:
        syMatrixTra(&mtx, translate->vec.f.x,
                    translate->vec.f.y,
                    translate->vec.f.z);
        break;
    case nGCMatrixKindVecRotR:
        syMatrixRotR(&mtx, rotate->a,
                     rotate->vec.f.x,
                     rotate->vec.f.y,
                     rotate->vec.f.z);
        break;
    case nGCMatrixKindVecRotRpyR:
        syMatrixRotRpyR(&mtx,
                        rotate->vec.f.x,
                        rotate->vec.f.y,
                        rotate->vec.f.z);
        break;
    case nGCMatrixKindVecSca:
        syMatrixSca(&mtx, scale->vec.f.x,
                    scale->vec.f.y,
                    scale->vec.f.z);
        break;
    case nGCMatrixKindVecTraRotR:
        syMatrixTraRotR(&mtx,
                        translate->vec.f.x,
                        translate->vec.f.y,
                        translate->vec.f.z,
                        rotate->a,
                        rotate->vec.f.x,
                        rotate->vec.f.y,
                        rotate->vec.f.z);
        break;
    case nGCMatrixKindVecTraRotRSca:
        syMatrixTraRotRSca(&mtx,
                           translate->vec.f.x,
                           translate->vec.f.y,
                           translate->vec.f.z,
                           rotate->a,
                           rotate->vec.f.x,
                           rotate->vec.f.y,
                           rotate->vec.f.z,
                           scale->vec.f.x,
                           scale->vec.f.y,
                           scale->vec.f.z);
        break;
    case nGCMatrixKindVecTraRotRpyR:
        syMatrixTraRotRpyR(&mtx,
                           translate->vec.f.x,
                           translate->vec.f.y,
                           translate->vec.f.z,
                           rotate->vec.f.x,
                           rotate->vec.f.y,
                           rotate->vec.f.z);
        break;
    case nGCMatrixKindVecTraRotRpyRSca:
        syMatrixTraRotRpyRSca(&mtx,
                              translate->vec.f.x,
                              translate->vec.f.y,
                              translate->vec.f.z,
                              rotate->vec.f.x,
                              rotate->vec.f.y,
                              rotate->vec.f.z,
                              scale->vec.f.x,
                              scale->vec.f.y,
                              scale->vec.f.z);
        break;
    default:
        ndsRendererAdapterBuildDObjFallbackMtx(dobj, &mtx);
        break;
    }

    ndsRendererAdapterMtxFromN64(&mtx, out);
    return TRUE;
}

#if NDS_TASK91_DRAW_PHASE_CENSUS
/* E7. Before arguing about whether a fixed-point angle index can reproduce the
 * float one bit-for-bit -- a claim about poses, and an expensive one to get
 * wrong -- ask the cheaper question E5 asked: is this work redundant at all?
 * 46.5 local matrices are built per frame, but a fighter animation does not
 * necessarily move every joint every frame. Any joint whose local matrix comes
 * out identical to last frame's was rebuilt for nothing, and a memo on that
 * needs no numerical equivalence argument whatsoever.
 *
 * Direct-mapped on the DObj pointer, 256 entries. A collision evicts and
 * reports a miss, so the hit rate is a lower bound on the real redundancy. */
#define NDS_TASK91_LOCAL_MEMO_COUNT 512u
#define NDS_TASK91_LOCAL_MEMO_MASK (NDS_TASK91_LOCAL_MEMO_COUNT - 1u)
u32 gNdsTask91LocalMemoHit;
u32 gNdsTask91LocalMemoMiss;
u32 gNdsTask91LocalMemoFill;
u32 gNdsTask91LocalMemoEvict;
static const DObj *sNdsTask91LocalMemoKey[NDS_TASK91_LOCAL_MEMO_COUNT];
static u32 sNdsTask91LocalMemoHash[NDS_TASK91_LOCAL_MEMO_COUNT];

static void ndsRendererAdapterTask91LocalMemoProbe(
    const DObj *dobj, const NDSRendererMatrix20p12 *out)
{
    u32 hash = 2166136261u;
    u32 slot;
    u32 row;
    u32 col;

    /* All sixteen, not the affine twelve: a kind that writes the fourth column
     * differently would otherwise be invisible to this probe. */
    for (row = 0u; row < 4u; row++)
    {
        for (col = 0u; col < 4u; col++)
        {
            hash = (hash ^ (u32)out->m[row][col]) * 16777619u;
        }
    }
    /* DObjs come from a pool, so the low address bits are strided and a plain
     * shift aliases badly -- the first run of this probe evicted on 37% of
     * calls, which floors the hit rate it can report. Knuth multiplicative,
     * taking high bits, spreads a strided key. */
    slot = (u32)((((uintptr_t)dobj) * 2654435761u) >> 13) &
        NDS_TASK91_LOCAL_MEMO_MASK;
    if (sNdsTask91LocalMemoKey[slot] == NULL)
    {
        sNdsTask91LocalMemoKey[slot] = dobj;
        sNdsTask91LocalMemoHash[slot] = hash;
        gNdsTask91LocalMemoFill++;
    }
    else if (sNdsTask91LocalMemoKey[slot] != dobj)
    {
        sNdsTask91LocalMemoKey[slot] = dobj;
        sNdsTask91LocalMemoHash[slot] = hash;
        gNdsTask91LocalMemoEvict++;
    }
    else if (sNdsTask91LocalMemoHash[slot] != hash)
    {
        sNdsTask91LocalMemoHash[slot] = hash;
        gNdsTask91LocalMemoMiss++;
    }
    else
    {
        gNdsTask91LocalMemoHit++;
    }
}
#endif

/* THE TWO SOURCE MVP-RECALC CALLBACKS THE PORT IMPLEMENTS. Both `continue` out
 * of gcPrepDObjMatrix without emitting a gSPMatrix, so neither contributes a
 * local matrix to the parent chain: they rewrite the COMPOSED MVP instead, and
 * ndsRendererAdapterApplyMvpRecalc is where that happens. A DObj carrying one
 * must therefore skip it in the local build or the transform is applied twice
 * and in the wrong space.
 *
 * The remaining recalc kinds (41-43, 45-50) still fall through to
 * ndsRendererAdapterBuildRecalcLocalMtx, which composes them as ordinary local
 * matrices. That is wrong by the same argument, but each has a different
 * orientation formula and none has a measured consumer in the P1 scene, so they
 * are left as they are rather than converted on speculation. */
static sb32 ndsRendererAdapterIsMvpRecalcKind(u32 kind)
{
    return ((kind == NDS_RENDERER_ADAPTER_MVP_RECALC_RPY_0X47_KIND) ||
            (kind == NDS_RENDERER_ADAPTER_MVP_RECALC_PERSP_SCA_KIND)) ?
        TRUE : FALSE;
}

static sb32 __attribute__((section(".itcm")))
ndsRendererAdapterBuildDObjLocalMatrix(
    DObj *dobj, NDSRendererMatrix20p12 *out)
{
    NDSRendererMatrix20p12 incoming;
    u32 valid = FALSE;
    u32 has_mvp_recalc_rpy_0x47 = FALSE;
    u32 i;

    if ((dobj == NULL) || (out == NULL))
    {
        return FALSE;
    }

    for (i = 0u; i < dobj->xobjs_num; i++)
    {
        if ((dobj->xobjs[i] != NULL) &&
            (ndsRendererAdapterIsMvpRecalcKind(dobj->xobjs[i]->kind) !=
                FALSE))
        {
            has_mvp_recalc_rpy_0x47 = TRUE;
            continue;
        }
        if (dobj->xobjs[i] == NULL)
        {
            continue;
        }
        if (valid == FALSE)
        {
            /* The first contributing xobj is built straight into `out`. It used
             * to land in `incoming` and then be copied, and with a mean of one
             * xobj per DObj that copy was the whole of MulInto's work: 2,266
             * ticks/frame of ndsRendererMatrixCopy20p12 over 1,060,479
             * executions in the c106 profile, plus 64 bytes of stack traffic
             * per call. A FALSE return may leave `out` partly written, which is
             * harmless -- `valid` stays FALSE and the block below overwrites it
             * with the identity or the fallback matrix. */
            if (ndsRendererAdapterBuildDObjXObjMatrix(
                    dobj, dobj->xobjs[i], out) != FALSE)
            {
                valid = TRUE;
            }
        }
        else if (ndsRendererAdapterBuildDObjXObjMatrix(
                     dobj, dobj->xobjs[i], &incoming) != FALSE)
        {
            ndsRendererAdapterMulInto(out, &incoming, &valid);
        }
    }

    if (valid == FALSE)
    {
        Mtx mtx;

        if (has_mvp_recalc_rpy_0x47 != FALSE)
        {
            /* The source special callback can legally follow the current
             * parent/camera matrix without another local affine transform. */
            ndsRendererAdapterMtxIdentity20p12(out);
        }
        else
        {
            /* Measured cold: 0 of 101,569 builder calls in a 60-second both-CPU
             * match take this arm (101,528 xobj, 41 recalc identity). The board
             * has the run; do not re-instrument it. */
            ndsRendererAdapterBuildDObjFallbackMtx(dobj, &mtx);
            ndsRendererAdapterMtxFromN64(&mtx, out);
        }
    }
#if NDS_TASK91_DRAW_PHASE_CENSUS
    ndsRendererAdapterTask91LocalMemoProbe(dobj, out);
#endif
    return TRUE;
}

static u32 ndsRendererAdapterDirectMvpRecalcKind(DObj *dobj)
{
    u32 i;

    if (dobj == NULL)
    {
        return 0u;
    }
    for (i = 0u; i < dobj->xobjs_num; i++)
    {
        if ((dobj->xobjs[i] != NULL) &&
            (ndsRendererAdapterIsMvpRecalcKind(dobj->xobjs[i]->kind) !=
                FALSE))
        {
            return (u32)dobj->xobjs[i]->kind;
        }
    }
    return 0u;
}

static void ndsRendererAdapterApplyMvpRecalc(
    DObj *dobj,
    u32 kind,
    CObj *cobj,
    NDSRendererMatrix20p12 *projection,
    const NDSRendererMatrix20p12 **projection_ptr,
    NDSRendererMatrix20p12 *modelview,
    const NDSRendererMatrix20p12 **modelview_ptr)
{
    Mtx rotation_mtx;
    NDSRendererMatrix20p12 rotation;
    NDSRendererMatrix20p12 perspective;
    NDSRendererMatrix20p12 source_orientation;
    NDSRendererMatrix20p12 composed;
    s32 translate[4];
    s32 scale_x;
    s32 scale_y;
    u16 perspective_norm;
    u32 has_battle_camera = FALSE;
    u32 i;
    u32 row;
    u32 col;

    if ((dobj == NULL) || (ndsRendererAdapterIsMvpRecalcKind(kind) == FALSE))
    {
        return;
    }
    gNdsRendererAdapterCustom47DetectedCount++;
    gNdsRendererAdapterCustom47LastXObjsNum = dobj->xobjs_num;
    gNdsRendererAdapterCustom47LastKinds =
        ((dobj->xobjs_num > 0u) && (dobj->xobjs[0] != NULL) ?
            (u32)dobj->xobjs[0]->kind : 0u) |
        (((dobj->xobjs_num > 1u) && (dobj->xobjs[1] != NULL) ?
            (u32)dobj->xobjs[1]->kind : 0u) << 8);
    gNdsRendererAdapterCustom47LastRotateXBits =
        ndsFloatBits(dobj->rotate.vec.f.x);
    gNdsRendererAdapterCustom47LastRotateYBits =
        ndsFloatBits(dobj->rotate.vec.f.y);

    if ((cobj != NULL) && (cobj->xobjs_num > 0))
    {
        for (i = 0u; i < (u32)cobj->xobjs_num; i++)
        {
            if ((cobj->xobjs[i] != NULL) &&
                (cobj->xobjs[i]->kind ==
                    NDS_RENDERER_ADAPTER_GM_CAMERA_MTX_KIND))
            {
                has_battle_camera = TRUE;
                break;
            }
        }
    }
    if ((cobj == NULL) || (has_battle_camera == FALSE) ||
        (projection == NULL) || (projection_ptr == NULL) ||
        (modelview == NULL) || (modelview_ptr == NULL) ||
        ((*projection_ptr == NULL) && (*modelview_ptr == NULL)))
    {
        gNdsRendererAdapterCustom47RejectCount++;
        return;
    }

    if ((*modelview_ptr != NULL) && (*projection_ptr != NULL))
    {
        ndsRendererMtxMul20p12(
            *modelview_ptr, *projection_ptr, &composed);
    }
    else if (*modelview_ptr != NULL)
    {
        composed = **modelview_ptr;
    }
    else
    {
        composed = **projection_ptr;
    }
    for (col = 0u; col < 4u; col++)
    {
        translate[col] = composed.m[3][col];
    }

    /* Both callbacks replace all three orientation rows of the current MVP with
     * something built from gGCMatrixPerspF alone.  The translation row from
     * Tra * LookAt * Persp stays live.  The battle camera's adapter projection
     * contains LookAt * Persp, so seed the renderer with the completed MVP to
     * avoid applying LookAt a second time to the rewritten orientation. */
    perspective_norm = cobj->projection.persp.norm;
    ndsRendererAdapterCameraPerspFast(&perspective,
                                      &perspective_norm,
                                      cobj->projection.persp.fovy,
                                      cobj->projection.persp.aspect,
                                      cobj->projection.persp.near,
                                      cobj->projection.persp.far,
                                      cobj->projection.persp.scale);
    if (kind == NDS_RENDERER_ADAPTER_MVP_RECALC_PERSP_SCA_KIND)
    {
        NDSRendererMatrix20p12 scale;

        /* objdisplay.c:876. Only four elements of the perspective block
         * survive, each scaled: [0][0] and [2][2]/[2][3] by gGCScaleX, [1][1]
         * by gGCScaleX * scale.y. gGCScaleX is 1.0 at
         * gcDrawDObjTreeDLLinksForGObj and is multiplied by scale.x HERE, so
         * for a tree with one recalc node -- which is what the shield desc
         * builds -- gGCScaleX is exactly this DObj's scale.x. A chain with two
         * recalc nodes would accumulate; none exists in the P1 scene, and the
         * apply counter below is what would show one appearing.
         *
         * diag(sx, sy, sx) * Persp reproduces those four writes and nothing
         * else, because syMatrixPerspFast leaves every off-diagonal of rows 0-2
         * at zero -- so this reuses the shared fixed-point multiply instead of
         * hand-rolling four 20.12 products.
         *
         * Both factors are formed in FLOAT first, exactly as source does, so
         * the guard-size term (see sNdsRendererAdapterMvpRecalcScaleX) is not
         * quantized twice. */
        if ((ndsRendererAdapterFloatPow2ToS32(
                 sNdsRendererAdapterMvpRecalcScaleX * dobj->scale.vec.f.x,
                 12u, &scale_x) == FALSE) ||
            (ndsRendererAdapterFloatPow2ToS32(
                 sNdsRendererAdapterMvpRecalcScaleX * dobj->scale.vec.f.y,
                 12u, &scale_y) == FALSE))
        {
            gNdsRendererAdapterCustom47RejectCount++;
            return;
        }
        ndsRendererAdapterMtxIdentity20p12(&scale);
        scale.m[0][0] = scale_x;
        scale.m[1][1] = scale_y;
        scale.m[2][2] = scale_x;
        ndsRendererMtxMul20p12(&scale, &perspective, &source_orientation);
        gNdsRendererAdapterMvpRecalcPerspScaCount++;
    }
    else
    {
        /* func_ovl0_800CA5C8: RotRpyR(x, y, 0) * gGCMatrixPerspF. */
        syMatrixRotRpyR(&rotation_mtx,
                        dobj->rotate.vec.f.x,
                        dobj->rotate.vec.f.y,
                        0.0F);
        ndsRendererAdapterMtxFromN64(&rotation_mtx, &rotation);
        ndsRendererMtxMul20p12(
            &rotation, &perspective, &source_orientation);
    }
    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 4u; col++)
        {
            composed.m[row][col] = source_orientation.m[row][col];
        }
    }
    for (col = 0u; col < 4u; col++)
    {
        if (composed.m[3][col] != translate[col])
        {
            gNdsRendererAdapterCustom47TranslationMismatchCount++;
            composed.m[3][col] = translate[col];
        }
    }
    *modelview = composed;
    *modelview_ptr = modelview;
    *projection_ptr = NULL;
    gNdsRendererAdapterCustom47LastTranslateX20p12 =
        (u32)modelview->m[3][0];
    gNdsRendererAdapterCustom47LastTranslateY20p12 =
        (u32)modelview->m[3][1];
    gNdsRendererAdapterCustom47LastTranslateZ20p12 =
        (u32)modelview->m[3][2];
    gNdsRendererAdapterCustom47AppliedCount++;
}

#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
/* Cycle 99, charter section 3.12: the baked fighter draw plan holds DObj and
 * loaded-file pointers, so it is re-derived at every scene entry -- including a
 * START-restart out of Results -- and never trusted across one. Defined with
 * the plan itself, next to the draw. */
static void ndsFighterDrawPlanInvalidate(void);
#endif

static void ndsRendererAdapterResetSceneCaches(void)
{
    ndsRendererHardwareResetSourceCaches();
    ndsRendererResetNativeStageValidationCache();
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    ndsFighterDrawPlanInvalidate();
#endif
    sNdsRendererAdapterCameraCacheFrame = 0u;
    sNdsRendererAdapterCameraCacheCount = 0u;
    sNdsRendererAdapterDObjWorldCache = NULL;
    sNdsRendererAdapterDObjWorldCacheFrame = 0u;
    sNdsRendererAdapterDObjWorldCacheCount = 0u;
    sNdsRendererAdapterDObjWorldCacheDynamicLimit =
        NDS_RENDERER_ADAPTER_DOBJ_WORLD_CACHE_COUNT;
    sNdsRendererAdapterDObjWorldCacheAllocationAttempted = FALSE;
    memset(sNdsRendererAdapterDObjWorldIndex, 0,
           sizeof(sNdsRendererAdapterDObjWorldIndex));
    sNdsRendererAdapterStageWorldCache = NULL;
    sNdsRendererAdapterStageWorldCacheCount = 0u;
    sNdsRendererAdapterStageWorldCacheAllocationAttempted = FALSE;
    sNdsRendererAdapterStageWorldNextGeneration = 1u;
    memset(sNdsRendererAdapterStageWorldIndex, 0,
           sizeof(sNdsRendererAdapterStageWorldIndex));
#if NDS_RENDERER_PROFILE_LEVEL < 2
    memset(&sNdsRendererAdapterNativeStageWorkspace, 0,
           sizeof(sNdsRendererAdapterNativeStageWorkspace));
#endif
}

static u32 ndsRendererAdapterDObjWorldIndexHash(const DObj *dobj)
{
    uintptr_t key = (uintptr_t)dobj >> 2;

    key ^= key >> 8;
    key ^= key >> 16;
    return (u32)key & NDS_RENDERER_ADAPTER_DOBJ_WORLD_INDEX_MASK;
}

static sb32 ndsRendererAdapterEnsureDObjWorldCache(void)
{
    uintptr_t aligned;
    size_t bytes = sizeof(NDSRendererAdapterDObjWorldCacheEntry) *
        NDS_RENDERER_ADAPTER_DOBJ_WORLD_CACHE_COUNT;

    if (sNdsRendererAdapterDObjWorldCache != NULL)
    {
        return TRUE;
    }
    if (sNdsRendererAdapterDObjWorldCacheAllocationAttempted != FALSE)
    {
        return FALSE;
    }
    sNdsRendererAdapterDObjWorldCacheAllocationAttempted = TRUE;
    if ((gSYTaskmanGeneralHeap.ptr == NULL) ||
        (gSYTaskmanGeneralHeap.end == NULL))
    {
        return FALSE;
    }
    aligned = ((uintptr_t)gSYTaskmanGeneralHeap.ptr + 0x0fu) &
        ~(uintptr_t)0x0fu;
    if ((aligned > (uintptr_t)gSYTaskmanGeneralHeap.end) ||
        (bytes > ((uintptr_t)gSYTaskmanGeneralHeap.end - aligned)))
    {
        return FALSE;
    }
    sNdsRendererAdapterDObjWorldCache =
        (NDSRendererAdapterDObjWorldCacheEntry *)syTaskmanMalloc(bytes, 0x10u);
    /* This DS cache now consumes the original scene heap, so keep the P1
     * reserve ledger truthful after its lazy first-frame allocation. */
    ndsRelocUpdateMemoryLedger();
    return (sNdsRendererAdapterDObjWorldCache != NULL) ? TRUE : FALSE;
}

static const NDSRendererMatrix20p12 *
ndsRendererAdapterFindDObjWorldMatrix(const DObj *dobj)
{
    u32 frame = gNdsRendererProfileFrameCount;
    u32 slot;
    u32 probe;

    if (sNdsRendererAdapterDObjWorldCacheFrame != frame)
    {
        sNdsRendererAdapterDObjWorldCacheFrame = frame;
        sNdsRendererAdapterDObjWorldCacheCount = 0u;
        memset(sNdsRendererAdapterDObjWorldIndex, 0,
               sizeof(sNdsRendererAdapterDObjWorldIndex));
    }
    if (ndsRendererAdapterEnsureDObjWorldCache() == FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileDObjWorldCacheMissCount++;
#endif
        return NULL;
    }
    slot = ndsRendererAdapterDObjWorldIndexHash(dobj);
    for (probe = 0u;
         probe < NDS_RENDERER_ADAPTER_DOBJ_WORLD_INDEX_COUNT;
         probe++)
    {
        u32 encoded = sNdsRendererAdapterDObjWorldIndex[slot];
        u32 cache_index;

        if (encoded == 0u)
        {
            break;
        }
        cache_index = encoded - 1u;
        if ((cache_index < sNdsRendererAdapterDObjWorldCacheCount) &&
            (sNdsRendererAdapterDObjWorldCache[cache_index].dobj == dobj))
        {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            gNdsRendererProfileDObjWorldCacheHitCount++;
#endif
            return &sNdsRendererAdapterDObjWorldCache[cache_index].world;
        }
        slot = (slot + 1u) & NDS_RENDERER_ADAPTER_DOBJ_WORLD_INDEX_MASK;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileDObjWorldCacheMissCount++;
#endif
    return NULL;
}

static void ndsRendererAdapterStoreDObjWorldMatrix(
    const DObj *dobj, const NDSRendererMatrix20p12 *world)
{
    NDSRendererAdapterDObjWorldCacheEntry *entry;
    u32 cache_index;
    u32 slot;
    u32 probe;

    if ((dobj == NULL) || (world == NULL))
    {
        return;
    }
    if ((sNdsRendererAdapterDObjWorldCache == NULL) ||
        (sNdsRendererAdapterDObjWorldCacheCount >=
         sNdsRendererAdapterDObjWorldCacheDynamicLimit))
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileDObjWorldCacheOverflowCount++;
#endif
        return;
    }
    cache_index = sNdsRendererAdapterDObjWorldCacheCount++;
    entry = &sNdsRendererAdapterDObjWorldCache[cache_index];
    entry->dobj = dobj;
    /* R2-03 E69. 64-byte struct assignment -> `bl memcpy`; this function is
     * inlined into ndsRendererAdapterBuildDObjWorldMatrix, which E68b measured
     * at 9.4% of the memset/memcpy class. E68's first run misattributed these
     * very samples to the hash below, which copies nothing. */
    ndsRendererMatrixCopy20p12(&entry->world, world);
    slot = ndsRendererAdapterDObjWorldIndexHash(dobj);
    for (probe = 0u;
         probe < NDS_RENDERER_ADAPTER_DOBJ_WORLD_INDEX_COUNT;
         probe++)
    {
        if (sNdsRendererAdapterDObjWorldIndex[slot] == 0u)
        {
            sNdsRendererAdapterDObjWorldIndex[slot] = (u8)(cache_index + 1u);
            return;
        }
        slot = (slot + 1u) & NDS_RENDERER_ADAPTER_DOBJ_WORLD_INDEX_MASK;
    }
}

static u32 ndsRendererAdapterStageWorldIndexHash(const DObj *dobj)
{
    uintptr_t key = (uintptr_t)dobj >> 2;

    key ^= key >> 7;
    key ^= key >> 15;
    return (u32)key & NDS_RENDERER_ADAPTER_STAGE_WORLD_INDEX_MASK;
}

static sb32 ndsRendererAdapterEnsureStageWorldCache(void)
{
    uintptr_t aligned;
    size_t bytes = sizeof(NDSRendererAdapterStageWorldCacheEntry) *
        NDS_RENDERER_ADAPTER_STAGE_WORLD_CACHE_COUNT;

    if (sNdsRendererAdapterStageWorldCache != NULL)
    {
        return TRUE;
    }
    if (sNdsRendererAdapterStageWorldCacheAllocationAttempted != FALSE)
    {
        return FALSE;
    }
    if ((ndsRendererAdapterEnsureDObjWorldCache() == FALSE) ||
        (sNdsRendererAdapterDObjWorldCacheCount >
         NDS_RENDERER_ADAPTER_STAGE_WORLD_SLOT_BASE))
    {
        return FALSE;
    }
    sNdsRendererAdapterStageWorldCacheAllocationAttempted = TRUE;
    if ((gSYTaskmanGeneralHeap.ptr == NULL) ||
        (gSYTaskmanGeneralHeap.end == NULL))
    {
        return FALSE;
    }
    aligned = ((uintptr_t)gSYTaskmanGeneralHeap.ptr + 0x0fu) &
        ~(uintptr_t)0x0fu;
    if ((aligned > (uintptr_t)gSYTaskmanGeneralHeap.end) ||
        (bytes > ((uintptr_t)gSYTaskmanGeneralHeap.end - aligned)))
    {
        return FALSE;
    }
    sNdsRendererAdapterStageWorldCache =
        (NDSRendererAdapterStageWorldCacheEntry *)
            syTaskmanMalloc(bytes, 0x10u);
    if (sNdsRendererAdapterStageWorldCache != NULL)
    {
        memset(sNdsRendererAdapterStageWorldCache, 0, bytes);
        sNdsRendererAdapterDObjWorldCacheDynamicLimit =
            NDS_RENDERER_ADAPTER_STAGE_WORLD_SLOT_BASE;
        ndsRelocUpdateMemoryLedger();
        return TRUE;
    }
    return FALSE;
}

static NDSRendererAdapterStageWorldCacheEntry *
__attribute__((section(".itcm")))
ndsRendererAdapterFindStageWorldEntry(const DObj *dobj)
{
    u32 slot;
    u32 probe;

    if ((dobj == NULL) ||
        (ndsRendererAdapterEnsureStageWorldCache() == FALSE))
    {
        return NULL;
    }
    slot = ndsRendererAdapterStageWorldIndexHash(dobj);
    for (probe = 0u;
         probe < NDS_RENDERER_ADAPTER_STAGE_WORLD_INDEX_COUNT;
         probe++)
    {
        u32 encoded = sNdsRendererAdapterStageWorldIndex[slot];
        u32 cache_index;

        if (encoded == 0u)
        {
            return NULL;
        }
        cache_index = encoded - 1u;
        if ((cache_index < sNdsRendererAdapterStageWorldCacheCount) &&
            (sNdsRendererAdapterStageWorldCache[cache_index].dobj == dobj))
        {
            return &sNdsRendererAdapterStageWorldCache[cache_index];
        }
        slot = (slot + 1u) & NDS_RENDERER_ADAPTER_STAGE_WORLD_INDEX_MASK;
    }
    return NULL;
}

static NDSRendererAdapterStageWorldCacheEntry *
ndsRendererAdapterFindOrAllocateStageWorldEntry(const DObj *dobj)
{
    NDSRendererAdapterStageWorldCacheEntry *entry;
    u32 cache_index;
    u32 slot;
    u32 probe;

    entry = ndsRendererAdapterFindStageWorldEntry(dobj);
    if (entry != NULL)
    {
        return entry;
    }
    if ((sNdsRendererAdapterStageWorldCache == NULL) ||
        (sNdsRendererAdapterStageWorldCacheCount >=
         NDS_RENDERER_ADAPTER_STAGE_WORLD_CACHE_COUNT))
    {
        return NULL;
    }
    cache_index = sNdsRendererAdapterStageWorldCacheCount++;
    entry = &sNdsRendererAdapterStageWorldCache[cache_index];
    memset(entry, 0, sizeof(*entry));
    entry->dobj = dobj;
    /* A zero frame is valid in accelerated harnesses. Ensure a freshly
     * allocated entry cannot take the same-frame shortcut before its first
     * exact world build; no u32 equals its own bitwise complement. */
    entry->validated_frame = ~gNdsRendererProfileFrameCount;
    entry->world_slot = (u8)(NDS_RENDERER_ADAPTER_STAGE_WORLD_SLOT_BASE +
                             cache_index);
    sNdsRendererAdapterDObjWorldCache[entry->world_slot].dobj = dobj;
    slot = ndsRendererAdapterStageWorldIndexHash(dobj);
    for (probe = 0u;
         probe < NDS_RENDERER_ADAPTER_STAGE_WORLD_INDEX_COUNT;
         probe++)
    {
        if (sNdsRendererAdapterStageWorldIndex[slot] == 0u)
        {
            sNdsRendererAdapterStageWorldIndex[slot] =
                (u8)(cache_index + 1u);
            return entry;
        }
        slot = (slot + 1u) & NDS_RENDERER_ADAPTER_STAGE_WORLD_INDEX_MASK;
    }
    return NULL;
}

static NDSRendererMatrix20p12 *
ndsRendererAdapterStageWorldEntryMatrix(
    const NDSRendererAdapterStageWorldCacheEntry *entry)
{
    return &sNdsRendererAdapterDObjWorldCache[entry->world_slot].world;
}

static sb32 ndsRendererAdapterCaptureStageWorldSourceKey(
    DObj *dobj, NDSRendererAdapterStageWorldSourceKey *source_key)
{
    u32 i;

    if ((dobj == NULL) || (source_key == NULL) ||
        (dobj->xobjs_num > 5u) || (dobj->vec != NULL))
    {
        return FALSE;
    }
    memset(source_key, 0, sizeof(*source_key));
    memcpy(source_key->base_translate, &dobj->translate.vec.f,
           sizeof(source_key->base_translate));
    memcpy(&source_key->base_rotate[0], &dobj->rotate.a,
           sizeof(source_key->base_rotate[0]));
    memcpy(&source_key->base_rotate[1], &dobj->rotate.vec.f,
           sizeof(source_key->base_rotate) -
               sizeof(source_key->base_rotate[0]));
    memcpy(source_key->base_scale, &dobj->scale.vec.f,
           sizeof(source_key->base_scale));

    source_key->xobjs_num = dobj->xobjs_num;
    for (i = 0u; i < dobj->xobjs_num; i++)
    {
        XObj *xobj = dobj->xobjs[i];

        if (xobj == NULL)
        {
            continue;
        }
        source_key->xobj_present_mask |= (u8)(1u << i);
        source_key->xobj_kinds[i] = xobj->kind;
        /* Kind 1 consumes an arbitrary 64-byte matrix. Kinds 33-40 consume
         * live camera state, and 0x4B and 0x4F consume live FTParts state. Keep
         * those exact paths frame-local instead of enlarging this first
         * stage-only proof cache.
         *
         * 0x4F was measured missing from this list on 2026-08-04 and it is the
         * reason the shield's attachment fix produced a byte-identical frame:
         * the key hashes the DObj's OWN translate/rotate/scale, which for an
         * attached effect root are the constants 0/0/1, so the entry matched on
         * every later frame and the world matrix was built exactly once per
         * guard -- one LOCAL build against ten submits, on the probe. A cached
         * matrix is only sound when the local build is a pure function of what
         * the key covers, and both fighter-parts kinds read state outside it.
         *
         * 0x50 joined the list on 2026-08-04 for the identical reason, measured
         * the identical way: the rebirth halo's root carries it, its own
         * translate/rotate/scale are the same constants 0/0/1, and the probe saw
         * ndsRendererAdapterBuildDObjLocalMatrix fire ONCE per spawn -- one
         * build for the whole ~90-tic life of the platform. Adding the kind case
         * without adding this line would have moved a frozen matrix instead of
         * tracking the fighter, which is exactly what cycle 52 already paid for
         * once. Any future kind that reads live FTParts state belongs here in
         * the same commit that teaches the builder about it. */
        if ((xobj->kind == 1u) ||
            ((xobj->kind >= 33u) && (xobj->kind <= 40u)) ||
            (xobj->kind == NDS_RENDERER_ADAPTER_FIGHTER_PARTS_MTX_KIND) ||
            (xobj->kind == NDS_RENDERER_ADAPTER_JOINT_ATTACH_MTX_KIND) ||
            (xobj->kind == NDS_RENDERER_ADAPTER_JOINT_ATTACH_TRA_MTX_KIND))
        {
            return FALSE;
        }
    }
    return TRUE;
}

static sb32 NDS_R2_ITCM_PACK2_CODE ndsRendererAdapterStageWorldSourceKeyMatches(
    DObj *dobj, const NDSRendererAdapterStageWorldSourceKey *source_key)
{
    u32 i;

    if ((dobj == NULL) || (source_key == NULL) ||
        (dobj->xobjs_num > 5u) || (dobj->vec != NULL) ||
        (dobj->xobjs_num != source_key->xobjs_num) ||
        (memcmp(source_key->base_translate, &dobj->translate.vec.f,
                sizeof(source_key->base_translate)) != 0) ||
        (memcmp(&source_key->base_rotate[0], &dobj->rotate.a,
                sizeof(source_key->base_rotate[0])) != 0) ||
        (memcmp(&source_key->base_rotate[1], &dobj->rotate.vec.f,
                sizeof(source_key->base_rotate) -
                    sizeof(source_key->base_rotate[0])) != 0) ||
        (memcmp(source_key->base_scale, &dobj->scale.vec.f,
                sizeof(source_key->base_scale)) != 0))
    {
        return FALSE;
    }
    for (i = 0u; i < dobj->xobjs_num; i++)
    {
        XObj *xobj = dobj->xobjs[i];
        u32 expected_present =
            (source_key->xobj_present_mask & (u8)(1u << i));

        if (xobj == NULL)
        {
            if (expected_present != 0u)
            {
                return FALSE;
            }
            continue;
        }
        if ((expected_present == 0u) ||
            (source_key->xobj_kinds[i] != xobj->kind) ||
            (xobj->kind == 1u) ||
            ((xobj->kind >= 33u) && (xobj->kind <= 40u)) ||
            (xobj->kind == NDS_RENDERER_ADAPTER_FIGHTER_PARTS_MTX_KIND))
        {
            return FALSE;
        }
    }
    return TRUE;
}

static u32 ndsRendererAdapterNextStageWorldGeneration(void)
{
    u32 generation = sNdsRendererAdapterStageWorldNextGeneration++;

    if (generation == 0u)
    {
        generation = sNdsRendererAdapterStageWorldNextGeneration++;
    }
    return generation;
}
#endif

#if NDS_RENDERER_HW_TRIANGLES
static sb32 ndsRendererAdapterBuildDObjWorldMatrixUncached(
    DObj *dobj, NDSRendererMatrix20p12 *out)
{
    DObj *chain[NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX];
    DObj *cursor = dobj;
    NDSRendererMatrix20p12 local;
    u32 depth = 0u;
    u32 i;

    if ((dobj == NULL) || (out == NULL))
    {
        return FALSE;
    }
    while ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL) &&
           (depth < NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX))
    {
        chain[depth++] = cursor;
        cursor = cursor->parent;
    }
    if ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL))
    {
        return FALSE;
    }
    ndsRendererAdapterMtxIdentity20p12(out);
    for (i = depth; i != 0u; i--)
    {
        if (ndsRendererAdapterBuildDObjLocalMatrix(chain[i - 1u], &local) !=
            FALSE)
        {
            ndsRendererMtxMulAffine20p12(&local, out, out);
        }
    }
    return TRUE;
}
#endif

#if NDS_TASK91_DRAW_PHASE_CENSUS
/* R2-03 E6. MatrixPrep is 91,338 ticks/frame, four times MaterialPrep, and E5
 * spent a whole cycle proving the smaller bucket's obvious lever banks nothing
 * -- so split this one before touching it rather than after. PrepareNativeOwner
 * Matrices is a per-frame camera fetch followed by, per selected binding, a
 * DObj world-matrix build and one affine multiply. Those three have completely
 * different fixes: a hoist, a cache, or nothing. Bindings and Calls normalise
 * them, because a per-binding cost and a per-frame cost look identical inside a
 * total.
 *
 * The first split put 82% in the world build at 2,799 ticks/binding against 184
 * for the multiply it feeds. The DObj world cache is a linear-probed hash reset
 * once a frame, so a lookup is cheap and the cost has to be in the miss path.
 * The second group separates what a miss actually does -- walk the parent
 * chain, build each local matrix, compose and store the prefix -- because only
 * one of those is worth attacking and they are indistinguishable inside 2,799.
 *
 * Declared here because both consumers are defined later in this file, but the
 * other Task 91 counters live near the draw entry point, which is later still.
 */
u32 gNdsTask91MtxCameraTicks;
u32 gNdsTask91MtxWorldTicks;
u32 gNdsTask91MtxMulTicks;
u32 gNdsTask91MtxBindings;
u32 gNdsTask91MtxCalls;
u32 gNdsTask91MtxWorldEntryHit;
u32 gNdsTask91MtxWorldAncestorHit;
u32 gNdsTask91MtxWorldColdStart;
u32 gNdsTask91MtxWorldLocalCalls;
u32 gNdsTask91MtxWorldLocalTicks;
u32 gNdsTask91MtxWorldComposeTicks;
u32 gNdsTask91MtxWorldChainDepth;
#endif

static sb32 ndsRendererAdapterBuildDObjWorldMatrix(
    DObj *dobj, NDSRendererMatrix20p12 *out)
{
    DObj *chain[NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX];
    DObj *cursor = dobj;
    NDSRendererMatrix20p12 local;
    u32 depth = 0u;
    u32 i;
#if NDS_RENDERER_HW_TRIANGLES
    const NDSRendererMatrix20p12 *cached;
#endif

    if ((dobj == NULL) || (out == NULL))
    {
        return FALSE;
    }
#if NDS_RENDERER_HW_TRIANGLES
    cached = ndsRendererAdapterFindDObjWorldMatrix(dobj);
    if (cached != NULL)
    {
#if NDS_TASK91_DRAW_PHASE_CENSUS
        gNdsTask91MtxWorldEntryHit++;
#endif
        ndsRendererMatrixCopy20p12(out, cached);
        return TRUE;
    }
#endif

    while ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL) &&
           (depth < NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX))
    {
        chain[depth++] = cursor;
        cursor = cursor->parent;
    }
#if NDS_TASK91_DRAW_PHASE_CENSUS
    gNdsTask91MtxWorldChainDepth += depth;
#endif
    if ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL))
    {
        return FALSE;
    }

#if NDS_RENDERER_HW_TRIANGLES
    for (i = 1u; i < depth; i++)
    {
        cached = ndsRendererAdapterFindDObjWorldMatrix(chain[i]);
        if (cached != NULL)
        {
            MTXCOPY(out, cached);
            break;
        }
    }
    if (i == depth)
    {
        ndsRendererAdapterMtxIdentity20p12(out);
#if NDS_TASK91_DRAW_PHASE_CENSUS
        gNdsTask91MtxWorldColdStart++;
#endif
    }
#if NDS_TASK91_DRAW_PHASE_CENSUS
    else
    {
        gNdsTask91MtxWorldAncestorHit++;
    }
#endif
#else
    ndsRendererAdapterMtxIdentity20p12(out);
    i = depth;
#endif
    for (; i != 0u; i--)
    {
#if NDS_TASK91_DRAW_PHASE_CENSUS
        u32 task91_local_mark = cpuGetTiming();
        u32 task91_local_end;

        gNdsTask91MtxWorldLocalCalls++;
#endif
        if (ndsRendererAdapterBuildDObjLocalMatrix(chain[i - 1u], &local) !=
            FALSE)
        {
#if NDS_TASK91_DRAW_PHASE_CENSUS
            task91_local_end = cpuGetTiming();
            gNdsTask91MtxWorldLocalTicks += task91_local_end -
                task91_local_mark;
            task91_local_mark = task91_local_end;
#endif
            /* objdisplay.c:1183-1191 left-multiplies each child local matrix. */
            ndsRendererMtxMulAffine20p12(&local, out, out);
#if NDS_RENDERER_HW_TRIANGLES
            /* DObj transforms are finalized before the camera's gcDrawAll
             * pass. Cache each prefix for sibling/child draws, and reset at
             * the next presented frame so live fighter poses remain live. */
            ndsRendererAdapterStoreDObjWorldMatrix(chain[i - 1u], out);
#endif
#if NDS_TASK91_DRAW_PHASE_CENSUS
            gNdsTask91MtxWorldComposeTicks += cpuGetTiming() -
                task91_local_mark;
#endif
        }
    }
    return TRUE;
}

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
static sb32 ndsRendererAdapterBuildDObjWorldMatrixM2Profile(
    DObj *dobj,
    NDSRendererMatrix20p12 *out,
    volatile NDSRendererOwnerProfile *owner)
{
    DObj *chain[NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX];
    DObj *cursor = dobj;
    NDSRendererMatrix20p12 local;
    const NDSRendererMatrix20p12 *cached;
    u32 depth = 0u;
    u32 phase_start;
    u32 i;

    if ((dobj == NULL) || (out == NULL) || (owner == NULL))
    {
        return FALSE;
    }
    owner->m2_world_matrix_request_count++;
    phase_start = cpuGetTiming();
    cached = ndsRendererAdapterFindDObjWorldMatrix(dobj);
    if (cached != NULL)
    {
        ndsRendererMatrixCopy20p12(out, cached);
        owner->m2_world_matrix_cache_hit_count++;
        owner->m2_hash_parent_lookup_ticks +=
            cpuGetTiming() - phase_start;
        return TRUE;
    }
    while ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL) &&
           (depth < NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX))
    {
        chain[depth++] = cursor;
        cursor = cursor->parent;
    }
    if ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL))
    {
        owner->m2_hash_parent_lookup_ticks +=
            cpuGetTiming() - phase_start;
        return FALSE;
    }
    for (i = 1u; i < depth; i++)
    {
        cached = ndsRendererAdapterFindDObjWorldMatrix(chain[i]);
        if (cached != NULL)
        {
            MTXCOPY(out, cached);
            owner->m2_world_matrix_cache_hit_count++;
            break;
        }
    }
    if (i == depth)
    {
        ndsRendererAdapterMtxIdentity20p12(out);
    }
    owner->m2_hash_parent_lookup_ticks += cpuGetTiming() - phase_start;

    for (; i != 0u; i--)
    {
        sb32 local_valid;

        phase_start = cpuGetTiming();
        local_valid = ndsRendererAdapterBuildDObjLocalMatrix(
            chain[i - 1u], &local);
        owner->m2_local_matrix_ticks += cpuGetTiming() - phase_start;
        owner->m2_local_matrix_build_count++;
        if (local_valid != FALSE)
        {
            phase_start = cpuGetTiming();
            ndsRendererMtxMulAffine20p12(&local, out, out);
            owner->m2_world_affine_ticks +=
                cpuGetTiming() - phase_start;
            owner->m2_world_affine_count++;

            phase_start = cpuGetTiming();
            ndsRendererAdapterStoreDObjWorldMatrix(chain[i - 1u], out);
            owner->m2_hash_parent_lookup_ticks +=
                cpuGetTiming() - phase_start;
        }
    }
    return TRUE;
}
#endif

#if NDS_RENDERER_HW_TRIANGLES
/* allow_stale: slice 44. TRUE means "this DObj's chain is not in this frame's
 * stride class, so a world matrix built on an earlier frame is accepted as-is".
 * Only the stage's own dynamic bindings may pass TRUE. The general caller at
 * the effect/fighter seam must not: kinds 0x4F and 0x50 are on the source-key
 * reject list precisely because their locals read live FTParts state, and a
 * matrix held over even one frame there is the frozen-attachment bug cycle 52
 * already paid for. */
static sb32 ndsRendererAdapterBuildPersistentStageWorldMatrix(
    DObj *dobj, NDSRendererMatrix20p12 *out, sb32 allow_stale)
{
    DObj *chain[NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX];
    DObj *cursor = dobj;
    NDSRendererMatrix20p12 parent_world;
    NDSRendererMatrix20p12 local;
    NDSRendererAdapterStageWorldCacheEntry *entry;
    NDSRendererAdapterStageWorldSourceKey source_key;
    u32 parent_generation = 0u;
    u32 depth = 0u;
    u32 frame = gNdsRendererProfileFrameCount;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    u32 reused_persistent = FALSE;
#endif
    u32 i;

#if !NDS_R2_STAGE_VALIDATE_STRIDE
    (void)allow_stale;
#endif
    if ((dobj == NULL) || (out == NULL))
    {
        return FALSE;
    }
    entry = ndsRendererAdapterFindStageWorldEntry(dobj);
    if ((entry != NULL) &&
        ((entry->validated_frame == frame)
#if NDS_R2_STAGE_VALIDATE_STRIDE
         /* generation, not validated_frame, is the "has a world matrix at all"
          * test: ndsRendererAdapterNextStageWorldGeneration never returns 0,
          * and frame 0 is a real frame. */
         || ((allow_stale != FALSE) && (entry->generation != 0u))
#endif
         ))
    {
        ndsRendererMatrixCopy20p12(
            out, ndsRendererAdapterStageWorldEntryMatrix(entry));
#if NDS_R2_STAGE_VALIDATE_STRIDE
        if (entry->validated_frame != frame)
        {
            gNdsR2Slice44StaleReuse++;
        }
#endif
        return TRUE;
    }
    while ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL) &&
           (depth < NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX))
    {
        chain[depth++] = cursor;
        cursor = cursor->parent;
    }
    if ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL))
    {
        return FALSE;
    }
    if (ndsRendererAdapterEnsureStageWorldCache() == FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileStageWorldPersistentOverflowCount++;
#endif
        return ndsRendererAdapterBuildDObjWorldMatrixUncached(dobj, out);
    }

    ndsRendererAdapterMtxIdentity20p12(&parent_world);
    for (i = depth; i != 0u; i--)
    {
        DObj *node = chain[i - 1u];
        u32 source_key_valid;
        u32 reuse = FALSE;

        entry = ndsRendererAdapterFindOrAllocateStageWorldEntry(node);
        if (entry == NULL)
        {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            gNdsRendererProfileStageWorldPersistentOverflowCount++;
#endif
            return ndsRendererAdapterBuildDObjWorldMatrixUncached(dobj, out);
        }
        if (entry->validated_frame == frame)
        {
            ndsRendererMatrixCopy20p12(
                &parent_world,
                ndsRendererAdapterStageWorldEntryMatrix(entry));
            parent_generation = entry->generation;
            continue;
        }

        source_key_valid = FALSE;
        if ((entry->source_key_valid != FALSE) &&
            (entry->parent == node->parent) &&
            (entry->parent_generation == parent_generation) &&
            (ndsRendererAdapterStageWorldSourceKeyMatches(
                 node, &entry->source_key) != FALSE))
        {
            reuse = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            reused_persistent = TRUE;
            gNdsRendererProfileStageWorldPersistentHitCount++;
#endif
        }
        if (reuse == FALSE)
        {
            source_key_valid = ndsRendererAdapterCaptureStageWorldSourceKey(
                node, &source_key);
            if (ndsRendererAdapterBuildDObjLocalMatrix(node, &local) == FALSE)
            {
                return FALSE;
            }
            ndsRendererMtxMulAffine20p12(
                &local, &parent_world,
                ndsRendererAdapterStageWorldEntryMatrix(entry));
            entry->parent = node->parent;
            entry->parent_generation = parent_generation;
            entry->generation = ndsRendererAdapterNextStageWorldGeneration();
            entry->source_key_valid = source_key_valid;
            if (source_key_valid != FALSE)
            {
                entry->source_key = source_key;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                gNdsRendererProfileStageWorldPersistentMissCount++;
#endif
            }
            else
            {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                gNdsRendererProfileStageWorldPersistentRejectCount++;
#endif
            }
        }
        entry->validated_frame = frame;
        ndsRendererMatrixCopy20p12(
                &parent_world,
                ndsRendererAdapterStageWorldEntryMatrix(entry));
        parent_generation = entry->generation;
    }
    *out = parent_world;

#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if (reused_persistent != FALSE)
    {
        NDSRendererMatrix20p12 oracle_world;

        if (ndsRendererAdapterBuildDObjWorldMatrixUncached(
                dobj, &oracle_world) != FALSE)
        {
            gNdsRendererProfileStageWorldPersistentOracleSampleCount++;
            if (memcmp(out, &oracle_world, sizeof(oracle_world)) != 0)
            {
                NDSRendererAdapterStageWorldCacheEntry *target =
                    ndsRendererAdapterFindStageWorldEntry(dobj);

                gNdsRendererProfileStageWorldPersistentOracleMismatchCount++;
                *out = oracle_world;
                if (target != NULL)
                {
                    target->source_key_valid = FALSE;
                    target->validated_frame = 0u;
                    /* Slice 44 reads `generation != 0` as "this entry holds a
                     * world matrix a later frame may reuse", so the oracle's
                     * rebuild-forcing invalidation has to clear that too or the
                     * stride would hand back the matrix the oracle just
                     * rejected. The two are never compiled together today --
                     * this branch is PROFILE_LEVEL >= 2 -- but a one-word
                     * omission here is exactly how a lab arm reads clean. */
                    target->generation = 0u;
                }
            }
        }
    }
#endif
    return TRUE;
}
#endif

static sb32 ndsRendererAdapterBuildCameraMatrices(
    CObj *cobj,
    NDSRendererMatrix20p12 *projection,
    u32 *projection_valid,
    NDSRendererMatrix20p12 *modelview,
    u32 *modelview_valid)
{
    NDSRendererMatrix20p12 incoming;
    XObj *xobj;
    Mtx mtx;
    u32 i;

    if ((projection == NULL) || (projection_valid == NULL) ||
        (modelview == NULL) || (modelview_valid == NULL))
    {
        return FALSE;
    }

    *projection_valid = FALSE;
    *modelview_valid = FALSE;

    if (cobj == NULL)
    {
        return FALSE;
    }
    if (cobj->xobjs_num <= 0)
    {
        return FALSE;
    }

    for (i = 0u; i < (u32)cobj->xobjs_num; i++)
    {
        xobj = cobj->xobjs[i];
        if (xobj == NULL)
        {
            continue;
        }

        switch (xobj->kind)
        {
        case NDS_RENDERER_ADAPTER_GM_CAMERA_MTX_KIND:
        {
            LookAt look_at;
            NDSRendererMatrix20p12 lookat;
            NDSRendererMatrix20p12 persp;

            ndsRendererAdapterCameraLookAtReflect(
                &lookat, &look_at,
                cobj->vec.eye.x, cobj->vec.eye.y,
                cobj->vec.eye.z, cobj->vec.at.x,
                cobj->vec.at.y, cobj->vec.at.z,
                cobj->vec.up.x, cobj->vec.up.y,
                cobj->vec.up.z);
            ndsRendererAdapterCameraPerspFast(
                &persp, &cobj->projection.persp.norm,
                cobj->projection.persp.fovy,
                cobj->projection.persp.aspect,
                cobj->projection.persp.near,
                cobj->projection.persp.far,
                cobj->projection.persp.scale);
            ndsRendererMtxMul20p12(&lookat, &persp, projection);
            *projection_valid = TRUE;
            break;
        }
        case nGCMatrixKindPerspFastF:
            ndsRendererAdapterCameraPerspFast(
                projection, &cobj->projection.persp.norm,
                cobj->projection.persp.fovy,
                cobj->projection.persp.aspect,
                cobj->projection.persp.near,
                cobj->projection.persp.far,
                cobj->projection.persp.scale);
            *projection_valid = TRUE;
            break;
        case nGCMatrixKindPerspF:
            syMatrixPersp(&mtx, &cobj->projection.persp.norm,
                          cobj->projection.persp.fovy,
                          cobj->projection.persp.aspect,
                          cobj->projection.persp.near,
                          cobj->projection.persp.far,
                          cobj->projection.persp.scale);
            ndsRendererAdapterMtxFromN64(&mtx, projection);
            *projection_valid = TRUE;
            break;
        case nGCMatrixKindOrtho:
            syMatrixOrtho(&mtx,
                          cobj->projection.ortho.l,
                          cobj->projection.ortho.r,
                          cobj->projection.ortho.b,
                          cobj->projection.ortho.t,
                          cobj->projection.ortho.n,
                          cobj->projection.ortho.f,
                          cobj->projection.ortho.scale);
            ndsRendererAdapterMtxFromN64(&mtx, projection);
            *projection_valid = TRUE;
            break;
        case 6:
        case 7:
            syMatrixLookAt(&mtx,
                           cobj->vec.eye.x, cobj->vec.eye.y,
                           cobj->vec.eye.z, cobj->vec.at.x,
                           cobj->vec.at.y, cobj->vec.at.z,
                           cobj->vec.up.x, cobj->vec.up.y,
                           cobj->vec.up.z);
            ndsRendererAdapterMtxFromN64(&mtx, &incoming);
            if (xobj->kind == 6)
            {
                ndsRendererAdapterMulBefore(projection, &incoming,
                                            projection_valid);
            }
            else
            {
                *modelview = incoming;
                *modelview_valid = TRUE;
            }
            break;
        case 8:
        case 9:
            syMatrixModLookAt(&mtx,
                              cobj->vec.eye.x, cobj->vec.eye.y,
                              cobj->vec.eye.z, cobj->vec.at.x,
                              cobj->vec.at.y, cobj->vec.at.z,
                              cobj->vec.up.x, 0.0F, 1.0F, 0.0F);
            ndsRendererAdapterMtxFromN64(&mtx, &incoming);
            if (xobj->kind == 8)
            {
                ndsRendererAdapterMulBefore(projection, &incoming,
                                            projection_valid);
            }
            else
            {
                *modelview = incoming;
                *modelview_valid = TRUE;
            }
            break;
        case 10:
        case 11:
            syMatrixModLookAt(&mtx,
                              cobj->vec.eye.x, cobj->vec.eye.y,
                              cobj->vec.eye.z, cobj->vec.at.x,
                              cobj->vec.at.y, cobj->vec.at.z,
                              cobj->vec.up.x, 0.0F, 0.0F, 1.0F);
            ndsRendererAdapterMtxFromN64(&mtx, &incoming);
            if (xobj->kind == 10)
            {
                ndsRendererAdapterMulBefore(projection, &incoming,
                                            projection_valid);
            }
            else
            {
                *modelview = incoming;
                *modelview_valid = TRUE;
            }
            break;
        default:
            break;
        }
    }

    return ((*projection_valid != 0u) || (*modelview_valid != 0u)) ?
        TRUE : FALSE;
}

#if NDS_RENDERER_HW_TRIANGLES
static void ndsRendererAdapterBuildDefaultBattleCameraMatrices(
    NDSRendererMatrix20p12 *projection,
    u32 *projection_valid,
    NDSRendererMatrix20p12 *modelview,
    u32 *modelview_valid)
{
    u16 norm = 0u;
    Mtx mtx;

    if ((projection == NULL) || (projection_valid == NULL) ||
        (modelview == NULL) || (modelview_valid == NULL))
    {
        return;
    }

    ndsRendererAdapterCameraPerspFast(projection, &norm, 38.0F, 15.0F / 11.0F,
                                      256.0F, 39936.0F, 1.0F);
    *projection_valid = TRUE;

    syMatrixLookAt(&mtx,
                   0.0F, 300.0F, 1600.0F,
                   0.0F, 300.0F, 0.0F,
                   0.0F, 1.0F, 0.0F);
    ndsRendererAdapterMtxFromN64(&mtx, modelview);
    *modelview_valid = TRUE;
}

#if NDS_TASK36_HW_COMPOSE
static sb32 ndsRendererAdapterBuildTask36StageCameraMatrices(
    CObj *cobj,
    NDSRendererMatrix20p12 *projection,
    NDSRendererMatrix20p12 *modelview)
{
    LookAt look_at;
    u32 i;

    if ((cobj == NULL) || (projection == NULL) || (modelview == NULL))
    {
        return FALSE;
    }
    for (i = 0u; i < (u32)cobj->xobjs_num; i++)
    {
        if ((cobj->xobjs[i] == NULL) ||
            (cobj->xobjs[i]->kind != NDS_RENDERER_ADAPTER_GM_CAMERA_MTX_KIND))
        {
            continue;
        }
        ndsRendererAdapterCameraLookAtReflect(
            modelview, &look_at,
            cobj->vec.eye.x, cobj->vec.eye.y,
            cobj->vec.eye.z, cobj->vec.at.x,
            cobj->vec.at.y, cobj->vec.at.z,
            cobj->vec.up.x, cobj->vec.up.y,
            cobj->vec.up.z);
        ndsRendererAdapterCameraPerspFast(
            projection, &cobj->projection.persp.norm,
            cobj->projection.persp.fovy,
            cobj->projection.persp.aspect,
            cobj->projection.persp.near,
            cobj->projection.persp.far,
            cobj->projection.persp.scale);
        return TRUE;
    }
    return FALSE;
}
#endif

static void ndsRendererAdapterGetFrameCameraMatrices(
    CObj *cobj,
    NDSRendererMatrix20p12 *projection,
    u32 *projection_valid,
    NDSRendererMatrix20p12 *modelview,
    u32 *modelview_valid)
{
    u32 frame = gNdsRendererProfileFrameCount;
    u32 i;

    if ((projection == NULL) || (projection_valid == NULL) ||
        (modelview == NULL) || (modelview_valid == NULL))
    {
        return;
    }
    if (sNdsRendererAdapterCameraCacheFrame != frame)
    {
        sNdsRendererAdapterCameraCacheFrame = frame;
        sNdsRendererAdapterCameraCacheCount = 0u;
    }
    for (i = 0u; i < sNdsRendererAdapterCameraCacheCount; i++)
    {
        const NDSRendererAdapterCameraCacheEntry *entry =
            &sNdsRendererAdapterCameraCache[i];

        if (entry->cobj == cobj)
        {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            gNdsRendererProfileCameraMatrixCacheHitCount++;
#endif
            MTXCOPY(projection, &entry->projection);
            MTXCOPY(modelview, &entry->modelview);
            *projection_valid = entry->projection_valid;
            *modelview_valid = entry->modelview_valid;
            return;
        }
    }

#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileCameraMatrixCacheMissCount++;
#endif
    ndsRendererAdapterBuildCameraMatrices(
        cobj, projection, projection_valid, modelview, modelview_valid);
    if ((*projection_valid == FALSE) && (*modelview_valid == FALSE))
    {
        ndsRendererAdapterBuildDefaultBattleCameraMatrices(
            projection, projection_valid, modelview, modelview_valid);
    }
    if (sNdsRendererAdapterCameraCacheCount <
        NDS_RENDERER_ADAPTER_CAMERA_CACHE_COUNT)
    {
        NDSRendererAdapterCameraCacheEntry *entry =
            &sNdsRendererAdapterCameraCache[
                sNdsRendererAdapterCameraCacheCount++];

        /* BattleShip updates CObj state before gcDrawAll; every DObj in that
         * camera's draw pass consumes the same matrices. The frame token
         * prevents fighter poses or camera motion from crossing presents. */
        entry->cobj = cobj;
        MTXCOPY(&entry->projection, projection);
        MTXCOPY(&entry->modelview, modelview);
        entry->projection_valid = *projection_valid;
        entry->modelview_valid = *modelview_valid;
    }
    else
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileCameraMatrixCacheOverflowCount++;
#endif
    }
}

s32 ndsRendererAdapterSetWorldQuadCamera(void *camera_gobj)
{
    GObj *gobj = camera_gobj;
    CObj *cobj = (gobj != NULL) ? CObjGetStruct(gobj) : NULL;
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    u32 projection_valid = FALSE;
    u32 modelview_valid = FALSE;

    if (cobj == NULL)
    {
        return FALSE;
    }
    ndsRendererAdapterGetFrameCameraMatrices(
        cobj, &projection, &projection_valid, &modelview, &modelview_valid);
    if ((projection_valid == FALSE) && (modelview_valid == FALSE))
    {
        return FALSE;
    }
    /* The battle camera's 0x4C XObj stores look-at * perspective in the
     * projection result. Other camera shapes can retain split projection and
     * modelview matrices. Identity-fill only the absent side, preserving both
     * contracts without composing another matrix on ARM9. */
    if (projection_valid == FALSE)
    {
        ndsRendererAdapterMtxIdentity20p12(&projection);
    }
    if (modelview_valid == FALSE)
    {
        ndsRendererAdapterMtxIdentity20p12(&modelview);
    }
    ndsRendererSetParticleCamera(&projection, &modelview);
    return TRUE;
}
#endif

/* Defined with the other stage statics further down; declared here because
 * ndsRendererAdapterPrepareInitialMatrices publishes its verdict only for the
 * effect submit. A tentative file-scope declaration, so this and the definition
 * name the same object. */
static sb32 sNdsRendererAdapterEffectSubmitActive;

static void ndsRendererAdapterPrepareInitialMatrices(
    DObj *dobj,
    CObj *cobj,
    u32 persistent_stage_world,
    NDSRendererMatrix20p12 *projection,
    const NDSRendererMatrix20p12 **projection_ptr,
    NDSRendererMatrix20p12 *modelview,
    const NDSRendererMatrix20p12 **modelview_ptr)
{
    NDSRendererMatrix20p12 camera_projection;
    NDSRendererMatrix20p12 camera_modelview;
    NDSRendererMatrix20p12 dobj_world;
    u32 mvp_recalc_kind;
    u32 camera_projection_valid = FALSE;
    u32 camera_modelview_valid = FALSE;
    u32 dobj_world_valid = FALSE;

    if ((projection_ptr == NULL) || (modelview_ptr == NULL))
    {
        return;
    }
    *projection_ptr = NULL;
    *modelview_ptr = NULL;
    /* gcDrawDObjTreeDLLinksForGObj resets gGCScaleX to 1.0 before every tree;
     * the joint-attach build below is what raises it, and the recalc at the end
     * of this function is what consumes it. Same order, same scope. */
    sNdsRendererAdapterMvpRecalcScaleX = 1.0F;
    mvp_recalc_kind = ndsRendererAdapterDirectMvpRecalcKind(dobj);

    if ((projection == NULL) || (modelview == NULL))
    {
        return;
    }

#if NDS_RENDERER_HW_TRIANGLES
    ndsRendererAdapterGetFrameCameraMatrices(
        cobj, &camera_projection, &camera_projection_valid,
        &camera_modelview, &camera_modelview_valid);
#else
    ndsRendererAdapterBuildCameraMatrices(cobj, &camera_projection,
                                          &camera_projection_valid,
                                          &camera_modelview,
                                          &camera_modelview_valid);
#endif
    if (dobj != NULL)
    {
#if NDS_RENDERER_HW_TRIANGLES
        if (persistent_stage_world != FALSE)
        {
            dobj_world_valid =
                ndsRendererAdapterBuildPersistentStageWorldMatrix(
                    dobj, &dobj_world, FALSE);
        }
        else
#endif
        {
            dobj_world_valid =
                ndsRendererAdapterBuildDObjWorldMatrix(dobj, &dobj_world);
        }
    }

    if (camera_projection_valid != FALSE)
    {
        MTXCOPY(projection, &camera_projection);
        *projection_ptr = projection;
    }

    if ((camera_modelview_valid != FALSE) && (dobj_world_valid != FALSE))
    {
        ndsRendererMtxMul20p12(&dobj_world, &camera_modelview, modelview);
        *modelview_ptr = modelview;
    }
    else if (camera_modelview_valid != FALSE)
    {
        MTXCOPY(modelview, &camera_modelview);
        *modelview_ptr = modelview;
    }
    else if (dobj_world_valid != FALSE)
    {
        MTXCOPY(modelview, &dobj_world);
        *modelview_ptr = modelview;
    }
    /* WHICH OF THE THREE VERDICTS DIED, for the effect submit only. Cycle 53
     * read config.initial_projection AND initial_modelview as NULL at the
     * executor (raw words, max_commands 8192 proving the config), while the
     * camera cache entry for the same CObj read projection_valid 1 -- and the
     * code above cannot produce both of those. One of the two reads is about a
     * different moment, and a latch taken here, between the decision and the
     * config fill, is the only thing that can say which. Effect-only because
     * the stage submits hundreds of lists per frame through this same path. */
    if (sNdsRendererAdapterEffectSubmitActive != FALSE)
    {
        gNdsRendererAdapterEffectPrepCount++;
        gNdsRendererAdapterEffectPrepMask =
            ((camera_projection_valid != FALSE) ? 1u : 0u) |
            ((camera_modelview_valid != FALSE) ? 2u : 0u) |
            ((dobj_world_valid != FALSE) ? 4u : 0u) |
            ((*projection_ptr != NULL) ? 8u : 0u) |
            ((*modelview_ptr != NULL) ? 16u : 0u);
    }
    ndsRendererAdapterApplyMvpRecalc(
        (mvp_recalc_kind != 0u) ? dobj : NULL, mvp_recalc_kind, cobj,
        projection, projection_ptr, modelview, modelview_ptr);
    if (sNdsRendererAdapterEffectSubmitActive != FALSE)
    {
        gNdsRendererAdapterEffectPrepMask |=
            ((*projection_ptr != NULL) ? 32u : 0u) |
            ((*modelview_ptr != NULL) ? 64u : 0u);
    }
}

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
static sb32 ndsRendererAdapterComposeNativeRootMatrix(
    const NDSRendererMatrix20p12 *modelview,
    const NDSRendererMatrix20p12 *projection,
    NDSRendererMatrix20p12 *out)
{
    if (out == NULL)
    {
        return FALSE;
    }
    if ((modelview != NULL) && (projection != NULL))
    {
        ndsRendererMtxMul20p12(modelview, projection, out);
        return TRUE;
    }
    if (modelview != NULL)
    {
        ndsRendererMatrixCopy20p12(out, modelview);
        return TRUE;
    }
    if (projection != NULL)
    {
        ndsRendererMatrixCopy20p12(out, projection);
        return TRUE;
    }
    return FALSE;
}

#if NDS_R2_FOX_GUN_OVERLAY
#if NDS_R2_POSITION_PROBE
/* One-shot fixed-point proof for the post-visibility Fox attachment row. The
 * gameplay probe in battleship_fox_blaster.c records the source helper's world
 * point. This records the exact 20.12 joint-17 world matrix the visible gun
 * consumes on the first gun draw after that source spawn, plus that matrix's
 * local {60,0,0} result. If the two world points disagree, the defect is before
 * the camera; if they agree, the remaining Y error is strictly draw-space.
 * Probe build only; GDB is the sole consumer. */
extern u32 gNdsFoxSpawnProbeCount;
__attribute__((used)) u32 gNdsFoxGunWorldProbeCount;
__attribute__((used)) f32 gNdsFoxGunWorldProbeFloatMtx[16];
__attribute__((used)) s32 gNdsFoxGunWorldProbeMtx[16];
__attribute__((used)) s32 gNdsFoxGunWorldProbeShotQ12[4];
#define NDS_FOX_GUN_POSITION_CHAIN_MAX 18u
__attribute__((used)) u32 gNdsFoxGunChainDepth;
__attribute__((used)) u32 gNdsFoxGunChainDObj[NDS_FOX_GUN_POSITION_CHAIN_MAX];
__attribute__((used)) u32 gNdsFoxGunChainMode[NDS_FOX_GUN_POSITION_CHAIN_MAX];
__attribute__((used)) u32 gNdsFoxGunChainKind0[NDS_FOX_GUN_POSITION_CHAIN_MAX];
__attribute__((used)) f32 gNdsFoxGunChainSourceLocal[
    NDS_FOX_GUN_POSITION_CHAIN_MAX * 16u];
__attribute__((used)) s32 gNdsFoxGunChainRendererLocal[
    NDS_FOX_GUN_POSITION_CHAIN_MAX * 16u];
static u32 sNdsFoxGunWorldProbeSpawnCount;
#endif

/* BUGS.md "Fox's pistol model is missing": joint 17's world matrix, for the
 * blaster overlay.
 *
 * This is the 0x4F builder's arithmetic without its DObj indirection -- the gun
 * has no attachment DObj because it has no GObj, which is the whole reason it
 * costs nothing but a matrix. func_ovl2_800EDBA4 FIRST, for the same reason the
 * shield needs it: mtx_translate is a per-frame cache that
 * ndsFTParamsInvalidateFighterParts clears, and the fighter's own draw does not
 * fill it. The chain probe read it as exactly zero at the setter
 * (artifacts/bugs/2026-08-12_r2-07-cluster/modelpart-chain.log, `MPJW w=0,0,0`),
 * so copying it without the rebuild would park the gun at the world origin.
 *
 * It deliberately does NOT write sNdsRendererAdapterMvpRecalcScaleX. That
 * global is the shield's billboard size, consumed by a recalc later in the same
 * call; the gun is not a billboard and must not move it. */
static sb32 __attribute__((noinline, cold))
ndsRendererAdapterFoxGunGameplayOwner(const FTStruct *fp)
{
    return ((fp != NULL) &&
            ((fp->pkind == nFTPlayerKindMan) ||
             (fp->pkind == nFTPlayerKindCom) ||
             (fp->pkind == nFTPlayerKindGameKey))) ? TRUE : FALSE;
}

static sb32 ndsRendererAdapterBuildFoxGunJointMtx(
    FTStruct *fp, CObj *cobj, NDSRendererMatrix20p12 *out)
{
    DObj *joint;
    FTParts *parts;
    Mtx mtx;
    NDSRendererMatrix20p12 world;
    NDSRendererMatrix20p12 camera_projection;
    NDSRendererMatrix20p12 camera_modelview;
    u32 camera_projection_valid = FALSE;
    u32 camera_modelview_valid = FALSE;

    if ((fp == NULL) || (out == NULL) ||
        (fp->fkind != nFTKindFox) ||
        ((u32)NDS_FOX_GUN_HOLD_JOINT >= ARRAY_COUNT(fp->joints)) ||
        (ndsRendererAdapterFoxGunGameplayOwner(fp) == FALSE))
    {
        return FALSE;
    }
    /* Results constructs Demo fighters and its win/lose statuses do not own the
     * battle-only pistol overlay. modelpart_status can inherit a non-negative
     * value there, which used to draw a solid/stray gun over the Results pose.
     * Gate on fighter ownership rather than scene kind so other real gameplay
     * scenes can still use Fox's source blaster presentation. */
    if (fp->modelpart_status[NDS_FOX_GUN_HOLD_JOINT -
                             nFTPartsJointCommonStart].modelpart_id_curr < 0)
    {
        return FALSE;
    }
    joint = fp->joints[NDS_FOX_GUN_HOLD_JOINT];
    if ((joint == NULL) || (joint == DOBJ_PARENT_NULL) ||
        (joint->parent_gobj == NULL))
    {
        return FALSE;
    }
    parts = ftGetParts(joint);
    if (parts == NULL)
    {
        return FALSE;
    }
    func_ovl2_800EDBA4(joint);
    if (ndsRendererAdapterF2LFixedWExact(&parts->mtx_translate, &mtx) == FALSE)
    {
        syMatrixF2LFixedW(&parts->mtx_translate, &mtx);
    }
    ndsRendererAdapterMtxFromN64(&mtx, &world);
#if NDS_R2_POSITION_PROBE
    if (gNdsFoxSpawnProbeCount != sNdsFoxGunWorldProbeSpawnCount)
    {
        u32 row;
        u32 col;
        DObj *cursor = joint;
        u32 depth = 0u;

        sNdsFoxGunWorldProbeSpawnCount = gNdsFoxSpawnProbeCount;
        while ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL) &&
               (depth < NDS_FOX_GUN_POSITION_CHAIN_MAX))
        {
            FTParts *cursor_parts = ftGetParts(cursor);
            NDSRendererMatrix20p12 renderer_local;

            gNdsFoxGunChainDObj[depth] = (u32)(uintptr_t)cursor;
            gNdsFoxGunChainKind0[depth] =
                ((cursor->xobjs_num != 0u) && (cursor->xobjs[0] != NULL)) ?
                    cursor->xobjs[0]->kind : 0xffffffffu;
            gNdsFoxGunChainMode[depth] = (cursor_parts != NULL) ?
                (u32)cursor_parts->transform_update_mode : 0xffffffffu;
            if (cursor_parts != NULL)
            {
                for (row = 0u; row < 4u; row++)
                {
                    for (col = 0u; col < 4u; col++)
                    {
                        gNdsFoxGunChainSourceLocal[(depth * 16u) +
                            (row * 4u) + col] =
                            cursor_parts->unk_dobjtrans_0x10[row][col];
                    }
                }
            }
            if (ndsRendererAdapterBuildDObjLocalMatrix(
                    cursor, &renderer_local) != FALSE)
            {
                for (row = 0u; row < 4u; row++)
                {
                    for (col = 0u; col < 4u; col++)
                    {
                        gNdsFoxGunChainRendererLocal[(depth * 16u) +
                            (row * 4u) + col] = renderer_local.m[row][col];
                    }
                }
            }
            depth++;
            cursor = cursor->parent;
        }
        gNdsFoxGunChainDepth = depth;
        for (row = 0u; row < 4u; row++)
        {
            for (col = 0u; col < 4u; col++)
            {
                gNdsFoxGunWorldProbeFloatMtx[(row * 4u) + col] =
                    parts->mtx_translate[row][col];
                gNdsFoxGunWorldProbeMtx[(row * 4u) + col] = world.m[row][col];
            }
        }
        for (col = 0u; col < 4u; col++)
        {
            gNdsFoxGunWorldProbeShotQ12[col] =
                (world.m[0][col] * 60) +
                world.m[3][col];
        }
        gNdsFoxGunWorldProbeCount++;
    }
#endif

    /* The same two multiplies a fighter root gets, in the same order. The
     * binding pass seeds its forward compose with the camera modelview and then
     * hands the product to ndsRendererAdapterComposeNativeRootMatrix, so
     * `world x camera x projection` IS a root's matrix; the gun is simply a
     * root the baked plan does not know about. Both terms come from the per-
     * frame cache the fighter's own prepare already filled this frame, so this
     * costs two 4x4s and no camera work. */
    ndsRendererAdapterGetFrameCameraMatrices(
        cobj, &camera_projection, &camera_projection_valid,
        &camera_modelview, &camera_modelview_valid);
    if (camera_modelview_valid != FALSE)
    {
        ndsRendererMtxMulAffine20p12(&world, &camera_modelview, &world);
    }
    if (camera_projection_valid == FALSE)
    {
        return FALSE;
    }
    return ndsRendererAdapterComposeNativeRootMatrix(
        &world, &camera_projection, out);
}
#endif


#if NDS_R2_FIGHTER_SHUFFLE_FOLD
/* R2-03 E32. The hitlag shuffle used to switch the whole native fighter owner
 * off (reloc_backend_renderer_dl.c's AnimLock fallback), dropping the fighter
 * to the generic interpreter for every hit -- E31 measured that at 5 fallbacks
 * over frames 460..500, one per burst frame, and the bursts are 41.9% of the
 * P95 tail's excess.
 *
 * The source does not need any of that. ftdisplaymain.c:1205 is one
 * G_MTX_PUSH plus syMatrixTra(x, y, 0) around the entire fighter draw and one
 * gSPPopMatrix, and lbcommon.c:1627 expresses the identical effect for the
 * attached-DObj path as `f[3][0] += x; f[3][1] += y;` on the part's *world*
 * matrix, before the camera is applied. This is that second form: the offset
 * lands on the world matrix in exactly the same space, so it is mechanically
 * equivalent rather than an approximation.
 *
 * 20.12, because that is what NDSRendererMatrix20p12 holds. */
extern Vec2f dFTDisplayMainShufflePositions[][4];
static s32 sNdsR2ShuffleWorldX;
static s32 sNdsR2ShuffleWorldY;
u32 gNdsR2ShuffleFoldedFrames;

static void ndsRendererAdapterSetShuffleOffset(const FTStruct *fp)
{
    sNdsR2ShuffleWorldX = 0;
    sNdsR2ShuffleWorldY = 0;
    if ((fp == NULL) || (fp->shuffle_tics == 0u))
    {
        return;
    }
    {
        const Vec2f *offset =
            &dFTDisplayMainShufflePositions[fp->is_shuffle_electric]
                                           [fp->shuffle_frame_index];

        sNdsR2ShuffleWorldX = (s32)(offset->x * 4096.0F);
        sNdsR2ShuffleWorldY = (s32)(offset->y * 4096.0F);
        gNdsR2ShuffleFoldedFrames++;
    }
}
#endif

#if NDS_RENDERER_HW_TRIANGLES
/* ---------------------------------------------------------------------------
 * The flat baked world compose.
 *
 * The shipped path asks `ndsRendererAdapterBuildDObjWorldMatrix` for each of the
 * 14/18 selected bindings independently. Because that entry point knows nothing
 * about the order it is being called in, it pays for the ignorance every time: a
 * linear-probed hash lookup on the binding, a walk all the way to the root, one
 * hash probe per ancestor looking for a prefix somebody already built, and a
 * hash store per composed step. The census prices the machinery at
 * `BuildDObjWorldMatrix` self 13,947 + `FindDObjWorldMatrix` 4,385 = 18,332
 * ticks/frame -- against ~50 local builds a frame whose arithmetic this does not
 * touch. That ratio is the whole point: the cost is the traversal and the cache
 * around the arithmetic, not the arithmetic.
 *
 * The order is not unknown, it is baked. `BindingParents` gives each binding's
 * nearest BOUND ancestor and the generator derives it from a preorder joint
 * list, so `binding_parents[i] < i` always -- one forward pass composes every
 * world with no cache at all, each binding starting from its parent binding's
 * finished world and walking only the one to three joints between them.
 *
 * Two things this deliberately does NOT do. It does not use `BindingParents` as
 * if it were the DObj parent -- the table skips unbound joints, so the live
 * chain between two bindings still has to be walked and composed, and treating
 * the table as a direct parent would silently drop those transforms. And it does
 * not stop storing prefixes: the generic display-list path shares this hash for
 * effects parented under fighter joints, so every joint it composes is still
 * published. What is deleted is the probing, not the publishing.
 *
 * Fail-closed: any disagreement between the baked table and the live tree
 * returns FALSE and the caller falls back to the per-binding path.
 *
 * MEASURED and graduated, cycle 110. Priced on ONE binary through a `.data`
 * route (identical `romSha256` in both arms, poke read back at end of run):
 * `FTR` mean 374,332 -> 366,597, P50 387,072 -> 379,328, P95 390,400 ->
 * 382,464, `WORK-H` mean 1,052,509 -> 1,044,687. Every unrelated bucket flat
 * within +/-40 -- `STG` -20, `SRC` -25, `SINT` +6, `SCPU` -8 -- and `FTR` agrees
 * with `WORK-H` to 87 ticks, so the whole delta lands in the bucket that owns
 * the change. Engagement was 3,951 calls and 0 rejects, so the route and its two
 * counters are deleted rather than left behind as proof-only machinery. */
/* `seed` is what the chain starts from, and it is the camera, not the identity.
 * Left-multiplying the whole chain into camera space costs nothing -- the first
 * multiply of a root chain used to be against the identity, which is a full
 * 4x4 that produces its own input -- and it deletes the per-binding
 * `world * camera` that ran once for every one of the ~31 bindings a frame.
 * Reassociating a fixed-point product is not bit-exact; these matrices reach
 * GX and nothing else, so that is a render-side difference by the doctrine in
 * PROJECT_GOAL.md, and the Boundary visual gate is what checks it. */
#if NDS_R2_FIGHTER_GX_COMPOSE
/* Slice 43 engagement. `Captures` counts bindings described, `Locals` the
 * MTX_MULT the backend will issue, `Declines` a fall-back to the CPU compose.
 * A cut with no counter is how cycle 110 read FTR -13,587 off a skip it could
 * not prove fired. */
u32 gNdsR2GxComposeCaptures;
u32 gNdsR2GxComposeLocals;
u32 gNdsR2GxComposeDeclines;

/* One palette slot per binding that is some other binding's baked parent, so the
 * backend can RESTORE it instead of the adapter composing into it. A binding that
 * already owns a cross-run slot reuses it -- the root loop stores there anyway --
 * and the rest are allocated DOWNWARD from 30. Downward on purpose: the cross-run
 * range is 16..23, and MTX_STORE/MTX_RESTORE address ABSOLUTE stack levels while
 * glPushMatrix writes whatever level the stack pointer is at, so the low levels
 * belong to anyone who pushes inside the execute (ndsRendererR2WriteLightVector
 * does, once). Mario needs one new slot and Fox eleven. */
static u8 sNdsR2GxSlotTable[
    NDS_RENDERER_NATIVE_FIGHTER_OWNER_COUNT]
    [NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
static u8 sNdsR2GxSlotTableValid[
    NDS_RENDERER_NATIVE_FIGHTER_OWNER_COUNT];

static sb32 ndsRendererAdapterBuildGxSlotTable(u32 slot, u32 binding_count)
{
    const u8 *parents;
    u32 parent_count = 0u;
    u32 cross_reserved_mask = 0u;
    u32 next_free = NDS_RENDERER_FIGHTER_GX_SLOT_NONE - 1u;
    u8 *table;
    u32 owner_slot;
    u32 i;

    if ((slot >= NDS_RENDERER_NATIVE_FIGHTER_OWNER_COUNT) ||
        (binding_count > NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED))
    {
        return FALSE;
    }
    table = sNdsR2GxSlotTable[slot];
    if (sNdsR2GxSlotTableValid[slot] != 0u)
    {
        return TRUE;
    }
    parents = ndsRendererNativeFighterBindingParents(slot, &parent_count);
    if ((parents == NULL) || (parent_count != binding_count))
    {
        return FALSE;
    }
    /* There is one hardware position/vector palette shared by every native
     * fighter owner. Parent slots are allocated per owner, so reserve the UNION
     * of every generated cross-run slot table. The original two-owner code used
     * `slot ^ 1`; admitting Luigi made that both incomplete and an invalid owner
     * lookup for slot 2. Parent-vs-parent overlap remains safe because every
     * parent slot is stored before a child restores it within one owner execute. */
    for (owner_slot = 0u;
         owner_slot < NDS_RENDERER_NATIVE_FIGHTER_OWNER_COUNT;
         owner_slot++)
    {
        const u8 *cross;
        u32 cross_count = 0u;
        u32 cross_index;

        cross = ndsRendererNativeFighterCrossPaletteSlots(
            owner_slot, &cross_count);
        if (cross == NULL)
        {
            return FALSE;
        }
        if ((owner_slot == slot) && (cross_count != binding_count))
        {
            return FALSE;
        }
        for (cross_index = 0u; cross_index < cross_count; cross_index++)
        {
            u32 cross_slot = cross[cross_index];

            if (cross_slot < NDS_RENDERER_FIGHTER_GX_SLOT_NONE)
            {
                cross_reserved_mask |= 1u << cross_slot;
            }
        }
    }

    /* A parent slot may NOT reuse the binding's cross-run slot. The cross-run
     * emitter restores a foreign binding's slot and then submits model-space
     * vertices, so what it needs there is the world with the world-unit scale
     * already applied -- and what a CHILD needs is the same world without it.
     * Two different matrices, so two different slots; the root loop's existing
     * glStoreMatrix keeps writing the scaled one. */
    for (i = 0u; i < binding_count; i++)
    {
        table[i] = (u8)NDS_RENDERER_FIGHTER_GX_SLOT_NONE;
    }
    for (i = 0u; i < binding_count; i++)
    {
        u32 parent = parents[i];

        if (parent == 0xffu)
        {
            continue;
        }
        /* The generator's contract, and what makes one forward pass correct. */
        if (parent >= i)
        {
            return FALSE;
        }
        if (table[parent] != (u8)NDS_RENDERER_FIGHTER_GX_SLOT_NONE)
        {
            continue;
        }
        while ((cross_reserved_mask & (1u << next_free)) != 0u)
        {
            if (next_free == 0u)
            {
                return FALSE;
            }
            next_free--;
        }
        table[parent] = (u8)next_free;
        if (next_free == 0u)
        {
            return FALSE;
        }
        next_free--;
    }
    sNdsR2GxSlotTableValid[slot] = 1u;
    return TRUE;
}

/* The same forward pass as ndsRendererAdapterComposeOwnerWorldsFlat, with the
 * multiplies removed. Each binding's chain is copied out in the order that pass
 * would have multiplied it -- nearest ancestor first, the binding itself last --
 * because DS MTX_MULT is `current = M x current`, which is exactly the
 * `MtxMulAffine20p12(&local, out, out)` convention. Nothing is reassociated.
 *
 * The locals land in `hierarchy_locals`, which mode 9 owns and mode 10 does not
 * use: every joint sits on exactly one binding's chain, so JOINT_MAX is the
 * exact bound. */
static sb32 ndsRendererAdapterCaptureOwnerChainsGx(
    u32 slot,
    DObj *const *bindings,
    u32 binding_count,
    NDSRendererAdapterNativeOwnerWorkspace *workspace)
{
    const u8 *binding_parents;
    const u8 *slots;
    u32 parent_count = 0u;
    u32 next_local = 0u;
    u32 binding_index;

    binding_parents = ndsRendererNativeFighterBindingParents(slot,
                                                             &parent_count);
    if ((binding_parents == NULL) || (parent_count != binding_count) ||
        (bindings == NULL) || (workspace == NULL) ||
        (ndsRendererAdapterBuildGxSlotTable(slot, binding_count) == FALSE))
    {
        return FALSE;
    }
    slots = sNdsR2GxSlotTable[slot];

    for (binding_index = 0u; binding_index < binding_count; binding_index++)
    {
        DObj *chain[NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX];
        DObj *cursor = bindings[binding_index];
        DObj *stop = DOBJ_PARENT_NULL;
        u32 parent = binding_parents[binding_index];
        u32 depth = 0u;
        u32 count = 0u;
        u32 i;

        if (cursor == NULL)
        {
            return FALSE;
        }
        if (parent != 0xffu)
        {
            if ((parent >= binding_index) || (bindings[parent] == NULL) ||
                (slots[parent] >= NDS_RENDERER_FIGHTER_GX_SLOT_NONE))
            {
                return FALSE;
            }
            stop = bindings[parent];
        }
        while ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL) &&
               (cursor != stop))
        {
            if (depth >= NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX)
            {
                return FALSE;
            }
            chain[depth++] = cursor;
            cursor = cursor->parent;
        }
        if (parent != 0xffu)
        {
            if (cursor != stop)
            {
                return FALSE;
            }
        }
        else if ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL))
        {
            return FALSE;
        }

        workspace->gx_local_first[binding_index] = (u8)next_local;
        for (i = depth; i != 0u; i--)
        {
            if (next_local >= NDS_RENDERER_FIGHTER_GX_LOCAL_MAX)
            {
                return FALSE;
            }
            /* A joint whose local build fails contributes nothing, exactly as
             * the compose pass treats it. */
            if (ndsRendererAdapterBuildDObjLocalMatrix(
                    chain[i - 1u],
                    &workspace->gx_locals[next_local]) != FALSE)
            {
                next_local++;
                count++;
            }
        }
        workspace->gx_local_count[binding_index] = (u8)count;
        workspace->gx_parent_slot[binding_index] = (parent == 0xffu) ?
            (u8)NDS_RENDERER_FIGHTER_GX_SLOT_NONE : slots[parent];
        workspace->gx_store_slot[binding_index] = slots[binding_index];
        gNdsR2GxComposeLocals += count;
    }
    gNdsR2GxComposeCaptures += binding_count;
    return TRUE;
}
#endif

static sb32 ndsRendererAdapterComposeOwnerWorldsFlat(
    u32 slot,
    DObj *const *bindings,
    u32 binding_count,
    NDSRendererMatrix20p12 *worlds,
    const NDSRendererMatrix20p12 *seed,
    sb32 seed_is_identity)
{
    const u8 *binding_parents;
    u32 parent_count = 0u;
    u32 binding_index;

    binding_parents = ndsRendererNativeFighterBindingParents(slot,
                                                             &parent_count);
    if ((binding_parents == NULL) || (parent_count != binding_count) ||
        (bindings == NULL) || (worlds == NULL) || (seed == NULL))
    {
        return FALSE;
    }
    for (binding_index = 0u; binding_index < binding_count; binding_index++)
    {
        DObj *chain[NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX];
        NDSRendererMatrix20p12 *out = &worlds[binding_index];
        const NDSRendererMatrix20p12 *base;
        DObj *cursor = bindings[binding_index];
        DObj *stop = DOBJ_PARENT_NULL;
        u32 parent = binding_parents[binding_index];
        u32 depth = 0u;
        sb32 base_identity;
        sb32 applied;
        u32 i;

        if (cursor == NULL)
        {
            return FALSE;
        }
        if (parent != 0xffu)
        {
            /* Strictly lower index, so its world is already composed. */
            if ((parent >= binding_index) || (bindings[parent] == NULL))
            {
                return FALSE;
            }
            stop = bindings[parent];
        }
        while ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL) &&
               (cursor != stop))
        {
            if (depth >= NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX)
            {
                return FALSE;
            }
            chain[depth++] = cursor;
            cursor = cursor->parent;
        }
        if (parent != 0xffu)
        {
            if (cursor != stop)
            {
                return FALSE;
            }
            base = &worlds[parent];
            base_identity = FALSE;
        }
        else
        {
            /* The root walk ends on either spelling of "no parent", exactly as
             * ndsRendererAdapterBuildDObjWorldMatrix accepts both. */
            if ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL))
            {
                return FALSE;
            }
            base = seed;
            base_identity = seed_is_identity;
        }
        applied = FALSE;
        for (i = depth; i != 0u; i--)
        {
            NDSRendererMatrix20p12 local;

            /* objdisplay.c:1183-1191 left-multiplies each child local matrix,
             * and a joint whose local build fails contributes nothing -- both
             * exactly as the per-binding path does.
             *
             * The per-DObj world cache used to be written here. The c112 census
             * says ndsRendererAdapterFindDObjWorldMatrix and
             * ndsRendererAdapterBuildDObjWorldMatrix both execute ZERO cycles
             * over a whole match, so every one of those stores fed a cache with
             * no reader -- 4,744,740 cycles of them, and ~4 KB a frame of write
             * traffic streamed through a 4 KB D-cache. The cache still exists
             * and the fallback path still fills it for itself; nothing populates
             * it speculatively any more. */
            /* The base is not folded in until the first contributing joint,
             * which turns two operations into one. All 55.5
             * ndsRendererMtxMulAffine20p12 calls a frame come from this loop at
             * 687 cycles each, and one per binding used to be `copy the base
             * in, then multiply the base straight back out`. When the base is
             * the identity the joint's local matrix IS the world, so it is
             * built directly into `out` and neither the multiply nor the
             * 64-byte temporary happens. */
            if ((applied == FALSE) && (base_identity != FALSE))
            {
                if (ndsRendererAdapterBuildDObjLocalMatrix(chain[i - 1u],
                                                           out) != FALSE)
                {
                    applied = TRUE;
                }
                continue;
            }
            if (ndsRendererAdapterBuildDObjLocalMatrix(chain[i - 1u],
                                                       &local) != FALSE)
            {
                if (applied == FALSE)
                {
                    ndsRendererMtxMulAffine20p12(&local, base, out);
                    applied = TRUE;
                }
                else
                {
                    ndsRendererMtxMulAffine20p12(&local, out, out);
                }
            }
        }
        if (applied == FALSE)
        {
            /* No joint contributed, so the binding's world IS its base. */
            ndsRendererMatrixCopy20p12(out, base);
        }
    }
    return TRUE;
}
#endif

/* The per-binding world build, outlined. It is the fail-closed path for when
 * the flat compose declines, and in a whole-match census it declined zero times
 * in 49,422 binding visits -- yet it was inlined into
 * ndsFighterMarioFoxDLAllDrawForSlot and held ~3,150 of that function's 10,708
 * bytes, `ndsRendererAdapterBuildDObjWorldMatrix` and all. The 64-byte `world`
 * matrix moves out of the hot frame with it. Outlined, never deleted: the flat
 * compose is allowed to decline, and when it does this must still be correct. */
static sb32 __attribute__((noinline, cold, optimize("Os")))
ndsRendererAdapterPrepareOwnerMatricesPerBinding(
    DObj *const *bindings,
    u32 binding_count,
    const NDSRendererMatrix20p12 *camera_modelview,
    u32 camera_modelview_valid,
    const NDSRendererMatrix20p12 **modelview_ptrs
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    , volatile NDSRendererOwnerProfile *m2_owner
#endif
    )
{
    NDSRendererMatrix20p12 world;
    u32 binding_index;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    u32 m2_phase_start;
#endif
#if NDS_TASK91_DRAW_PHASE_CENSUS
    u32 task91_mtx_mark = cpuGetTiming();
#endif

    for (binding_index = 0u;
         binding_index < binding_count;
         binding_index++)
    {
        if (bindings[binding_index] != NULL)
        {
#if NDS_TASK91_DRAW_PHASE_CENSUS
            task91_mtx_mark = cpuGetTiming();
            gNdsTask91MtxBindings++;
#endif
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
            if (ndsRendererAdapterBuildDObjWorldMatrixM2Profile(
                    bindings[binding_index], &world,
                    m2_owner) == FALSE)
#else
            if (ndsRendererAdapterBuildDObjWorldMatrix(
                    bindings[binding_index], &world) == FALSE)
#endif
            {
                return FALSE;
            }
#if NDS_R2_FIGHTER_SHUFFLE_FOLD
            /* R2-03 E32, and this is the whole cut: lbcommon.c:1627's
             * `f[3][0] += x; f[3][1] += y;` on the world matrix, before the
             * camera multiply below. Zero when the fighter is not in hitlag. */
            world.m[3][0] += sNdsR2ShuffleWorldX;
            world.m[3][1] += sNdsR2ShuffleWorldY;
#endif
#if NDS_TASK91_DRAW_PHASE_CENSUS
            {
                u32 task91_world_end = cpuGetTiming();

                gNdsTask91MtxWorldTicks += task91_world_end - task91_mtx_mark;
                task91_mtx_mark = task91_world_end;
            }
#endif
            if (camera_modelview_valid != FALSE)
            {
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
                m2_phase_start = cpuGetTiming();
#endif
                ndsRendererMtxMulAffine20p12(
                    &world, camera_modelview,
                    &sNdsRendererAdapterNativeOwnerModelviews[
                        binding_index]);
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
                if (m2_owner != NULL)
                {
                    m2_owner->m2_world_camera_ticks +=
                        cpuGetTiming() - m2_phase_start;
                    m2_owner->m2_world_camera_count++;
                }
#endif
            }
            else
            {
                sNdsRendererAdapterNativeOwnerModelviews[binding_index] =
                    world;
            }
#if NDS_TASK91_DRAW_PHASE_CENSUS
            gNdsTask91MtxMulTicks += cpuGetTiming() - task91_mtx_mark;
#endif
            modelview_ptrs[binding_index] =
                &sNdsRendererAdapterNativeOwnerModelviews[binding_index];
        }
        else if (camera_modelview_valid != FALSE)
        {
            sNdsRendererAdapterNativeOwnerModelviews[binding_index] =
                *camera_modelview;
            modelview_ptrs[binding_index] =
                &sNdsRendererAdapterNativeOwnerModelviews[binding_index];
        }
    }
    return TRUE;
}

/* Do NOT add `noinline` here. It was tried (slice 14) on the theory that shrinking
 * the 10.5 KB driver toward the 8 KB I-cache is good regardless of what moves:
 * the driver did lose 916 bytes and FTR rose 2,192. Outlining code that RUNS is
 * not the same lever as outlining code that never runs -- slice 12 won 5,747 by
 * moving out bytes with zero executions, and this loses by moving out bytes with
 * many. */
static sb32 ndsRendererAdapterPrepareNativeOwnerMatrices(
    u32 slot,
    DObj *root,
    DObj *const *bindings,
    u32 binding_count,
    CObj *cobj,
    const NDSRendererMatrix20p12 **projection_ptr,
    const NDSRendererMatrix20p12 **modelview_ptrs
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    , volatile NDSRendererOwnerProfile *m2_owner
#endif
    )
{
    NDSRendererMatrix20p12 camera_projection;
    NDSRendererMatrix20p12 camera_modelview;
    u32 camera_projection_valid = FALSE;
    u32 camera_modelview_valid = FALSE;
    u32 binding_index;
#if NDS_RENDERER_HW_TRIANGLES
    NDSRendererMatrix20p12 compose_seed;
    sb32 seed_is_identity;
    u32 flat_worlds = FALSE;
#endif
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    u32 m2_phase_start;
#endif
#if NDS_TASK91_DRAW_PHASE_CENSUS
    u32 task91_mtx_mark;
#endif

    if ((bindings == NULL) || (projection_ptr == NULL) ||
        (modelview_ptrs == NULL) ||
        (binding_count > NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED))
    {
        return FALSE;
    }
    *projection_ptr = NULL;
    for (binding_index = 0u;
         binding_index < binding_count;
         binding_index++)
    {
        modelview_ptrs[binding_index] = NULL;
    }

#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    m2_phase_start = cpuGetTiming();
#endif
#if NDS_TASK91_DRAW_PHASE_CENSUS
    task91_mtx_mark = cpuGetTiming();
#endif
    ndsRendererAdapterGetFrameCameraMatrices(
        cobj, &camera_projection, &camera_projection_valid,
        &camera_modelview, &camera_modelview_valid);
#if NDS_TASK91_DRAW_PHASE_CENSUS
    gNdsTask91MtxCameraTicks += cpuGetTiming() - task91_mtx_mark;
    gNdsTask91MtxCalls++;
#endif
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    if (m2_owner != NULL)
    {
        m2_owner->m2_camera_fetch_ticks +=
            cpuGetTiming() - m2_phase_start;
        m2_owner->m2_camera_fetch_count++;
    }
#endif
    if (camera_projection_valid != FALSE)
    {
        sNdsRendererAdapterNativeOwnerProjection = camera_projection;
        *projection_ptr = &sNdsRendererAdapterNativeOwnerProjection;
    }

    (void)root;
#if NDS_RENDERER_HW_TRIANGLES
    /* The seed the whole binding forest hangs off. Camera when there is one, so
     * the compose lands in camera space directly; identity when there is not,
     * which is the old world-space result unchanged. */
    if (camera_modelview_valid != FALSE)
    {
        ndsRendererMatrixCopy20p12(&compose_seed, &camera_modelview);
        seed_is_identity = FALSE;
    }
    else
    {
        /* The measured case. NDS_R2_FIGHTER_HW_MTX hands the camera to the
         * hardware, so this path composes world matrices and the seed is the
         * identity -- which is why the compose can skip folding it in at all. */
        ndsRendererAdapterMtxIdentity20p12(&compose_seed);
        seed_is_identity = TRUE;
    }
#if NDS_R2_FIGHTER_SHUFFLE_FOLD
    /* R2-03 E32's hitlag shuffle used to be added to every binding's world row
     * 3 before the camera multiply. `world * T * camera` reassociates to
     * `world * (T * camera)` and T is the same for every binding, so one 4x4 a
     * frame replaces one row-3 add per binding -- and it keeps the shuffle out
     * of the compose entirely. Zero whenever the fighter is not in hitlag. */
    if ((sNdsR2ShuffleWorldX != 0) || (sNdsR2ShuffleWorldY != 0))
    {
        NDSRendererMatrix20p12 shuffle;

        ndsRendererAdapterMtxIdentity20p12(&shuffle);
        shuffle.m[3][0] = sNdsR2ShuffleWorldX;
        shuffle.m[3][1] = sNdsR2ShuffleWorldY;
        ndsRendererMtxMulAffine20p12(&shuffle, &compose_seed, &compose_seed);
        seed_is_identity = FALSE;
    }
#endif
    /* One forward pass over the baked binding order, composing straight into the
     * modelview array so the worlds need no second home -- and, since the seed
     * carries the camera, no second pass either. On failure nothing has been
     * consumed yet and the per-binding path runs unchanged. */
#if NDS_R2_FIGHTER_GX_COMPOSE
    /* Slice 43. Same forward pass, no multiplies: describe each binding's chain
     * and let the geometry engine compose it in the root loop. Declining leaves
     * nothing consumed, exactly as the compose does, so the CPU pass below is
     * still the fail-closed answer. */
    sNdsRendererAdapterNativeOwnerWorkspace.gx_valid = 0u;
    if (ndsRendererAdapterCaptureOwnerChainsGx(
            slot, bindings, binding_count,
            &sNdsRendererAdapterNativeOwnerWorkspace) != FALSE)
    {
        sNdsRendererAdapterNativeOwnerWorkspace.gx_valid = 1u;
        ndsRendererMatrixCopy20p12(
            &sNdsRendererAdapterNativeOwnerWorkspace.gx_seed, &compose_seed);
        sNdsRendererAdapterNativeOwnerWorkspace.gx_seed_is_identity =
            (u8)((seed_is_identity != FALSE) ? 1u : 0u);
        for (binding_index = 0u;
             binding_index < binding_count;
             binding_index++)
        {
#if NDS_TASK91_DRAW_PHASE_CENSUS
            gNdsTask91MtxBindings++;
#endif
            /* Never loaded under this flag, but every preflight NULL-checks it
             * and the seed is the honest answer for "what did the CPU leave". */
            modelview_ptrs[binding_index] =
                &sNdsRendererAdapterNativeOwnerWorkspace.gx_seed;
        }
        /* `*projection_ptr` was already published above, or deliberately left
         * NULL when the camera had no projection; do not second-guess it. */
        return TRUE;
    }
    gNdsR2GxComposeDeclines++;
#endif
    flat_worlds = ndsRendererAdapterComposeOwnerWorldsFlat(
        slot, bindings, binding_count,
        sNdsRendererAdapterNativeOwnerModelviews, &compose_seed,
        seed_is_identity);
#endif
    /* `flat_worlds` is decided once, above, and never changes inside the loop,
     * so hoisting it out of the loop is a pure transformation -- and it is what
     * lets the fallback leave the hot function entirely. */
#if NDS_RENDERER_HW_TRIANGLES
    if (flat_worlds != FALSE)
    {
        /* Nothing left to do per binding but publish the pointer: the compose
         * already wrote camera-space modelviews in place. A NULL binding makes
         * the compose decline, so inside this arm every binding is non-NULL. */
        for (binding_index = 0u;
             binding_index < binding_count;
             binding_index++)
        {
#if NDS_TASK91_DRAW_PHASE_CENSUS
            gNdsTask91MtxBindings++;
#endif
            modelview_ptrs[binding_index] =
                &sNdsRendererAdapterNativeOwnerModelviews[binding_index];
        }
    }
    else
#endif
    {
        if (ndsRendererAdapterPrepareOwnerMatricesPerBinding(
                bindings, binding_count, &camera_modelview,
                camera_modelview_valid, modelview_ptrs
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
                , m2_owner
#endif
                ) == FALSE)
        {
            return FALSE;
        }
    }
    return ((*projection_ptr != NULL) ||
            (camera_modelview_valid != FALSE) ||
            (binding_count == 0u)) ? TRUE : FALSE;
}

static sb32 ndsRendererAdapterMatrixIsAffine20p12(
    const NDSRendererMatrix20p12 *matrix)
{
    return ((matrix != NULL) &&
            (matrix->m[0][3] == 0) &&
            (matrix->m[1][3] == 0) &&
            (matrix->m[2][3] == 0) &&
            (matrix->m[3][3] == (1 << 12))) ? TRUE : FALSE;
}

/* The ordinary camera adapter preserves BattleShip's historical combined
 * 0x4C look-at/projection matrix.  The hierarchy candidate needs the same
 * product split at its natural affine boundary so source-unit translation can
 * be scaled once in GX without CPU-composing every fighter root. */
/* Cold with the hierarchy mode it serves: only
 * NDS_RENDERER_FAST_RUN_NATIVE_FIGHTERS reaches it, and the live mode is
 * NATIVE_FIGHTER_OWNER_PRODUCTION. */
static sb32 __attribute__((noinline, cold, optimize("Os")))
ndsRendererAdapterGetHierarchyCameraMatrices(
    CObj *cobj,
    NDSRendererMatrix20p12 *projection,
    NDSRendererMatrix20p12 *modelview)
{
    u32 projection_valid = FALSE;
    u32 modelview_valid = FALSE;
    u32 i;

    if ((projection == NULL) || (modelview == NULL))
    {
        return FALSE;
    }
    ndsRendererAdapterMtxIdentity20p12(projection);
    ndsRendererAdapterMtxIdentity20p12(modelview);
    if (cobj == NULL)
    {
        return FALSE;
    }
    for (i = 0u; i < (u32)cobj->xobjs_num; i++)
    {
        XObj *xobj = cobj->xobjs[i];

        if ((xobj != NULL) &&
            (xobj->kind == NDS_RENDERER_ADAPTER_GM_CAMERA_MTX_KIND))
        {
            LookAt look_at;

            ndsRendererAdapterCameraLookAtReflect(
                modelview, &look_at,
                cobj->vec.eye.x, cobj->vec.eye.y,
                cobj->vec.eye.z, cobj->vec.at.x,
                cobj->vec.at.y, cobj->vec.at.z,
                cobj->vec.up.x, cobj->vec.up.y,
                cobj->vec.up.z);
            ndsRendererAdapterCameraPerspFast(
                projection, &cobj->projection.persp.norm,
                cobj->projection.persp.fovy,
                cobj->projection.persp.aspect,
                cobj->projection.persp.near,
                cobj->projection.persp.far,
                cobj->projection.persp.scale);
            return TRUE;
        }
    }
    ndsRendererAdapterGetFrameCameraMatrices(
        cobj, projection, &projection_valid,
        modelview, &modelview_valid);
    if (projection_valid == FALSE)
    {
        ndsRendererAdapterMtxIdentity20p12(projection);
    }
    if (modelview_valid == FALSE)
    {
        ndsRendererAdapterMtxIdentity20p12(modelview);
    }
    return ((projection_valid != FALSE) ||
            (modelview_valid != FALSE)) ? TRUE : FALSE;
}

/* The alternate matrix mode, selected only by
 * NDS_RENDERER_FAST_RUN_NATIVE_FIGHTERS. The c112 cold map found it and its
 * camera helper inlined into ndsFighterMarioFoxDLAllDrawForSlot across two cold
 * runs totalling ~1,100 bytes, none of which executes in a whole match. It is
 * still a live mode and still correct; it just stops renting I-cache lines from
 * the mode that does run. */
static sb32 __attribute__((noinline, cold, optimize("Os")))
ndsRendererAdapterPrepareNativeOwnerHierarchy(
    u32 slot,
    FTStruct *fp,
    DObj *root,
    DObj *const *matrix_bindings,
    u32 binding_count,
    CObj *cobj,
    NDSRendererAdapterNativeOwnerWorkspace *workspace
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    , volatile NDSRendererOwnerProfile *m2_owner
#endif
    )
{
    u32 expected_joint_count = (slot == 0u) ? 25u : 27u;
    u32 expected_binding_count = (slot == 0u) ? 14u : 18u;
    u32 joint_count = 0u;
    u32 joint_index;
    u32 binding_index;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    u32 phase_start;
#endif

    if ((slot > 1u) || (fp == NULL) || (root == NULL) ||
        (matrix_bindings == NULL) || (workspace == NULL) ||
        (binding_count != expected_binding_count) ||
        (fp->is_use_animlocks != FALSE) || (fp->shuffle_tics != 0u))
    {
        return FALSE;
    }
    memset(workspace->hierarchy_joints, 0,
           sizeof(workspace->hierarchy_joints));
    memset(workspace->hierarchy_parents, 31,
           sizeof(workspace->hierarchy_parents));
    memset(workspace->hierarchy_bindings, 31,
           sizeof(workspace->hierarchy_bindings));
    if ((ndsRendererAdapterCollectFighterTopology(
             root, 31u, workspace->hierarchy_joints,
             workspace->hierarchy_parents, &joint_count) == FALSE) ||
        (joint_count != expected_joint_count))
    {
        return FALSE;
    }
    for (binding_index = 0u; binding_index < binding_count; binding_index++)
    {
        u32 found = NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX;

        for (joint_index = 0u; joint_index < joint_count; joint_index++)
        {
            if (workspace->hierarchy_joints[joint_index] ==
                matrix_bindings[binding_index])
            {
                found = joint_index;
                break;
            }
        }
        if ((found >= joint_count) ||
            (workspace->hierarchy_bindings[found] != 31u))
        {
            return FALSE;
        }
        workspace->hierarchy_bindings[found] = (u8)binding_index;
    }
    for (joint_index = 0u; joint_index < joint_count; joint_index++)
    {
        DObj *joint = workspace->hierarchy_joints[joint_index];
        u32 xobj_index;

        if ((joint == NULL) ||
            ((workspace->hierarchy_parents[joint_index] == 31u) ?
                 (joint->parent != DOBJ_PARENT_NULL) :
                 (joint->parent != workspace->hierarchy_joints[
                     workspace->hierarchy_parents[joint_index]])))
        {
            return FALSE;
        }
        for (xobj_index = 0u; xobj_index < joint->xobjs_num; xobj_index++)
        {
            XObj *xobj = joint->xobjs[xobj_index];

            if (xobj == NULL)
            {
                continue;
            }
            if ((xobj->kind != nGCMatrixKindNull) &&
                (xobj->kind !=
                 NDS_RENDERER_ADAPTER_FIGHTER_PARTS_MTX_KIND))
            {
                return FALSE;
            }
            if ((xobj->kind ==
                 NDS_RENDERER_ADAPTER_FIGHTER_PARTS_MTX_KIND) &&
                (ftGetParts(joint) == NULL))
            {
                return FALSE;
            }
        }
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        phase_start = cpuGetTiming();
#endif
        if ((ndsRendererAdapterBuildDObjLocalMatrix(
                 joint, &workspace->hierarchy_locals[joint_index]) == FALSE) ||
            (ndsRendererAdapterMatrixIsAffine20p12(
                 &workspace->hierarchy_locals[joint_index]) == FALSE))
        {
            return FALSE;
        }
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        if (m2_owner != NULL)
        {
            m2_owner->m2_local_matrix_ticks +=
                cpuGetTiming() - phase_start;
            m2_owner->m2_local_matrix_build_count++;
        }
#endif
    }
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    phase_start = cpuGetTiming();
#endif
    if (ndsRendererAdapterGetHierarchyCameraMatrices(
            cobj, &workspace->hierarchy_projection,
            &workspace->hierarchy_camera_modelview) == FALSE)
    {
        return FALSE;
    }
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    if (m2_owner != NULL)
    {
        m2_owner->m2_camera_fetch_ticks += cpuGetTiming() - phase_start;
        m2_owner->m2_camera_fetch_count++;
    }
#endif
    workspace->hierarchy.projection = &workspace->hierarchy_projection;
    workspace->hierarchy.camera_modelview =
        &workspace->hierarchy_camera_modelview;
    workspace->hierarchy.joint_locals = workspace->hierarchy_locals;
    workspace->hierarchy.joint_parents = workspace->hierarchy_parents;
    workspace->hierarchy.joint_bindings = workspace->hierarchy_bindings;
    workspace->hierarchy.joint_count = joint_count;
    return TRUE;
}

static u32 ndsRendererAdapterNormalizeNativeGeometryMode(u32 geometry_mode)
{
    const u32 legacy_cull_front = 0x00001000u;
    const u32 legacy_cull_back = G_CULL_BACK;
    u32 legacy = geometry_mode &
        (legacy_cull_front | legacy_cull_back);

    if (legacy != 0u)
    {
        geometry_mode &= ~(legacy_cull_front | legacy_cull_back);
        if ((legacy & legacy_cull_front) != 0u)
        {
            geometry_mode |= NDS_RENDERER_GEOM_CULL_FRONT;
        }
        if ((legacy & legacy_cull_back) != 0u)
        {
            geometry_mode |= NDS_RENDERER_GEOM_CULL_BACK;
        }
    }
    return geometry_mode;
}
#endif

static s32 ndsFighterDLScanRangeInTaskmanArena(const void *ptr, size_t bytes)
{
    const u8 *arena = ndsTaskmanArenaStart();
    uintptr_t base = (uintptr_t)arena;
    uintptr_t addr = (uintptr_t)ptr;
    size_t arena_size = ndsTaskmanArenaSize();

    if ((ptr == NULL) || (arena == NULL) || (addr < base) ||
        (addr > (base + arena_size)))
    {
        return FALSE;
    }
    return (bytes <= (arena_size - (size_t)(addr - base))) ? TRUE : FALSE;
}

static size_t ndsRendererAdapterImmutableCommandSpan(const Gfx *dl,
                                                      void *user)
{
    NDSRelocLoadedFile *loaded;
    uintptr_t base;
    uintptr_t addr;

    (void)user;
    loaded = ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
    if (loaded == NULL)
    {
        return 0u;
    }
    base = (uintptr_t)loaded->data;
    addr = (uintptr_t)dl;
    return loaded->data_size - (size_t)(addr - base);
}

static s32 ndsFighterDLScanValidateRange(const Gfx *dl, size_t bytes,
                                         void *user)
{
    (void)user;

    if ((((uintptr_t)dl & (sizeof(u32) - 1u)) != 0u) ||
        ((ndsRelocFindLoadedFileContaining(dl, bytes) == NULL) &&
         (ndsFighterDLScanRangeInTaskmanArena(dl, bytes) == FALSE) &&
         (ndsRendererAdapterRangeIsEmptySegmentEDL(dl, bytes) == FALSE)))
    {
        gNdsFighterDLScanRangeRejectCount++;
        return FALSE;
    }
    return TRUE;
}

static const Gfx *ndsFighterDLScanResolveBranch(const Gfx *dl,
                                                 u32 *resolve_kind,
                                                 void *user)
{
    NDSFighterDLScanContext *context = user;
    uintptr_t raw = (uintptr_t)dl;
    uintptr_t offset = raw & 0x00ffffffu;
    u32 i;

    if (resolve_kind != NULL)
    {
        *resolve_kind = NDS_RENDERER_RESOLVE_NONE;
    }

    if (ndsRelocFindLoadedFileContaining(dl, sizeof(Gfx)) != NULL)
    {
        return dl;
    }
    if (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(Gfx)) != FALSE)
    {
        return dl;
    }

    if ((context != NULL) &&
        (context->primary_file != NULL) &&
        (ndsRelocRangeInLoadedFile(context->primary_file,
                                   offset,
                                   sizeof(Gfx)) != FALSE))
    {
        if (resolve_kind != NULL)
        {
            *resolve_kind = NDS_RENDERER_RESOLVE_SEGMENT;
        }
        gNdsFighterDLScanBranchResolveCount++;
        return (const Gfx *)((const u8 *)context->primary_file->data + offset);
    }

    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        if (ndsRelocRangeInLoadedFile(&sNdsRelocLoadedFiles[i],
                                      offset,
                                      sizeof(Gfx)) != FALSE)
        {
            if (resolve_kind != NULL)
            {
                *resolve_kind = NDS_RENDERER_RESOLVE_SEGMENT;
            }
            gNdsFighterDLScanBranchResolveCount++;
            return (const Gfx *)((const u8 *)sNdsRelocLoadedFiles[i].data +
                                 offset);
        }
    }

    return dl;
}

static const void *ndsFighterDLScanResolveDataPointer(const void *ptr,
                                                      size_t bytes,
                                                      void *user)
{
    NDSFighterDLScanContext *context = user;
    uintptr_t raw = (uintptr_t)ptr;
    uintptr_t offset = raw & 0x00ffffffu;
    u32 i;

    if ((ndsRelocFindLoadedFileContaining(ptr, bytes) != NULL) ||
        (ndsFighterDLScanRangeInTaskmanArena(ptr, bytes) != FALSE))
    {
        return ptr;
    }

    if ((context != NULL) &&
        (context->primary_file != NULL) &&
        (ndsRelocRangeInLoadedFile(context->primary_file,
                                   offset,
                                   bytes) != FALSE))
    {
        return (const u8 *)context->primary_file->data + offset;
    }

    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        if (ndsRelocRangeInLoadedFile(&sNdsRelocLoadedFiles[i],
                                      offset,
                                      bytes) != FALSE)
        {
            return (const u8 *)sNdsRelocLoadedFiles[i].data + offset;
        }
    }

    return NULL;
}

static void ndsFighterMarioFoxCopyDLScanStats(u32 slot,
                                               const NDSRendererStats *stats)
{
    if (stats == NULL)
    {
        return;
    }

    if (slot == 0u)
    {
        gNdsFighterDLScanP0Blocker = stats->blocker;
        gNdsFighterDLScanP0CommandCount = stats->command_count;
        gNdsFighterDLScanP0FirstOpcode = stats->first_opcode;
        gNdsFighterDLScanP0UnsupportedOpcode = stats->unsupported_opcode;
        gNdsFighterDLScanP0UnsupportedCommandCount =
            stats->unsupported_command_count;
        gNdsFighterDLScanP0VertexCommandCount =
            stats->vertex_command_count;
        gNdsFighterDLScanP0TriangleCommandCount =
            stats->triangle_command_count;
        gNdsFighterDLScanP0VertexCount = stats->vertex_count;
        gNdsFighterDLScanP0TriangleCount = stats->triangle_count;
        gNdsFighterDLScanP0EndCommandCount = stats->end_command_count;
        gNdsFighterDLScanP0BranchCommandCount = stats->branch_command_count;
        gNdsFighterDLScanP0SegmentResolveCount = stats->segment_resolve_count;
        gNdsFighterDLScanP0TextureMask = stats->texture_mask;
        gNdsFighterDLScanP0OtherModeCommandCount =
            stats->othermode_command_count;
        gNdsFighterDLScanP0CullCommandCount = stats->cull_command_count;
        gNdsFighterDLScanP0StateCommandCount = stats->state_command_count;
        gNdsFighterDLScanP0SkipCommandCount = stats->skip_command_count;
        gNdsFighterDLScanP0RenderCommandCount = stats->render_command_count;
        gNdsFighterDLScanP0MaxDepthSeen = stats->max_depth_seen;
    }
    else if (slot == 1u)
    {
        gNdsFighterDLScanP1Blocker = stats->blocker;
        gNdsFighterDLScanP1CommandCount = stats->command_count;
        gNdsFighterDLScanP1FirstOpcode = stats->first_opcode;
        gNdsFighterDLScanP1UnsupportedOpcode = stats->unsupported_opcode;
        gNdsFighterDLScanP1UnsupportedCommandCount =
            stats->unsupported_command_count;
        gNdsFighterDLScanP1VertexCommandCount =
            stats->vertex_command_count;
        gNdsFighterDLScanP1TriangleCommandCount =
            stats->triangle_command_count;
        gNdsFighterDLScanP1VertexCount = stats->vertex_count;
        gNdsFighterDLScanP1TriangleCount = stats->triangle_count;
        gNdsFighterDLScanP1EndCommandCount = stats->end_command_count;
        gNdsFighterDLScanP1BranchCommandCount = stats->branch_command_count;
        gNdsFighterDLScanP1SegmentResolveCount = stats->segment_resolve_count;
        gNdsFighterDLScanP1TextureMask = stats->texture_mask;
        gNdsFighterDLScanP1OtherModeCommandCount =
            stats->othermode_command_count;
        gNdsFighterDLScanP1CullCommandCount = stats->cull_command_count;
        gNdsFighterDLScanP1StateCommandCount = stats->state_command_count;
        gNdsFighterDLScanP1SkipCommandCount = stats->skip_command_count;
        gNdsFighterDLScanP1RenderCommandCount = stats->render_command_count;
        gNdsFighterDLScanP1MaxDepthSeen = stats->max_depth_seen;
    }
}

static void ndsFighterMarioFoxScanDLForSlot(u32 slot, FTStruct *fp)
{
    DObj *root;
    DObj *selected;
    const Gfx *dl;
    NDSRelocLoadedFile *loaded;
    NDSRendererConfig config = {0};
    NDSRendererStats stats;
    NDSFighterDLScanContext context;
    u32 dobj_index;
    u32 root_x_before;
    u32 root_x_after;

    if ((slot > 1u) || (ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL) ||
        (fp->status_id != nFTCommonStatusWait) ||
        (fp->motion_id != nFTCommonMotionWait) ||
        (fp->ga != nMPKineticsGround))
    {
        return;
    }

    root = fp->joints[nFTPartsJointTopN];
    root_x_before = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) :
        0u;
    selected = ndsFighterFindFirstDObjWithDL(root, &dobj_index);

    if (slot == 0u)
    {
        gNdsFighterDLScanP0RootXBeforeBits = root_x_before;
        gNdsFighterDLScanP0DObjIndex = dobj_index;
    }
    else
    {
        gNdsFighterDLScanP1RootXBeforeBits = root_x_before;
        gNdsFighterDLScanP1DObjIndex = dobj_index;
    }

    if (selected == NULL)
    {
        return;
    }

    dl = selected->dl;
    loaded = ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));

    if (slot == 0u)
    {
        gNdsFighterDLScanP0FirstDL = (u32)(uintptr_t)dl;
        if (loaded != NULL)
        {
            gNdsFighterDLScanP0AssetID = loaded->asset_id;
            gNdsFighterDLScanP0Offset =
                (u32)((uintptr_t)dl - (uintptr_t)loaded->data);
        }
        else if (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) != FALSE)
        {
            gNdsFighterDLScanP0AssetID = NDS_FIGHTER_DL_SCAN_ASSET_ARENA;
            gNdsFighterDLScanP0Offset =
                (u32)((uintptr_t)dl - (uintptr_t)ndsTaskmanArenaStart());
        }
        else
        {
            gNdsFighterDLScanP0AssetID = 0xffffffffu;
            gNdsFighterDLScanP0Offset = 0xffffffffu;
        }
    }
    else
    {
        gNdsFighterDLScanP1FirstDL = (u32)(uintptr_t)dl;
        if (loaded != NULL)
        {
            gNdsFighterDLScanP1AssetID = loaded->asset_id;
            gNdsFighterDLScanP1Offset =
                (u32)((uintptr_t)dl - (uintptr_t)loaded->data);
        }
        else if (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) != FALSE)
        {
            gNdsFighterDLScanP1AssetID = NDS_FIGHTER_DL_SCAN_ASSET_ARENA;
            gNdsFighterDLScanP1Offset =
                (u32)((uintptr_t)dl - (uintptr_t)ndsTaskmanArenaStart());
        }
        else
        {
            gNdsFighterDLScanP1AssetID = 0xffffffffu;
            gNdsFighterDLScanP1Offset = 0xffffffffu;
        }
    }

    if ((loaded == NULL) &&
        (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) == FALSE))
    {
        return;
    }

    context.primary_file = loaded;
    context.slot = slot;
    config.max_depth = 8u;
    config.max_commands = 2048u;
    config.max_list_commands = 512u;
    config.initial_projection = NULL;
    config.initial_modelview = NULL;
    config.initial_geometry_mode = 0u;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    config.validate_range = ndsFighterDLScanValidateRange;
    config.immutable_command_span = ndsRendererAdapterImmutableCommandSpan;
    config.resolve_branch = ndsFighterDLScanResolveBranch;
    config.resolve_data = ndsFighterDLScanResolveDataPointer;
    config.user = &context;

    ndsRendererInitStats(&stats);
    ndsRendererScanDisplayList(dl, &config, &stats);
    ndsFighterMarioFoxCopyDLScanStats(slot, &stats);

    root_x_after = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) :
        0u;

    if (slot == 0u)
    {
        gNdsFighterDLScanP0StatusAfter = (u32)fp->status_id;
        gNdsFighterDLScanP0MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLScanP0GAAfter = (u32)fp->ga;
        gNdsFighterDLScanP0RootXAfterBits = root_x_after;
    }
    else
    {
        gNdsFighterDLScanP1StatusAfter = (u32)fp->status_id;
        gNdsFighterDLScanP1MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLScanP1GAAfter = (u32)fp->ga;
        gNdsFighterDLScanP1RootXAfterBits = root_x_after;
    }

    gNdsFighterMarioFoxDLScanCount++;
}

static void ndsFighterMarioFoxRunDLScanProbe(void)
{
    u32 mask = 0u;
    u32 gobj_before;
    u32 gobj_after;

    if ((ndsFighterMarioFoxDLScanProofEnabled() == FALSE) ||
        (gNdsFighterMarioFoxDLScanResult != 0u))
    {
        return;
    }

    if ((gNdsFighterMarioFoxDisplayResult ==
            NDS_FIGHTER_MARIOFOX_DISPLAY_PASS) &&
        (gNdsFighterMarioFoxDisplaySafeResult ==
            NDS_FIGHTER_MARIOFOX_DISPLAY_SAFE_PASS) &&
        ((gNdsFighterMarioFoxDisplayMask & 0x7ffu) == 0x7ffu) &&
        (gNdsFighterMarioFoxDisplayCallbackCount == 2u))
    {
        mask |= 1u << 0;
    }
    else
    {
        gNdsFighterMarioFoxDLScanMask = mask;
        return;
    }

    gobj_before = (u32)gcGetGObjsActiveNum();

    ndsFighterMarioFoxScanDLForSlot(0u, &sNdsFighterStructPool[0]);
    ndsFighterMarioFoxScanDLForSlot(1u, &sNdsFighterStructPool[1]);

    if ((gNdsFighterDLScanP0FirstDL != 0u) &&
        (gNdsFighterDLScanP1FirstDL != 0u))
    {
        mask |= 1u << 1;
    }
    if ((gNdsFighterDLScanP0AssetID != 0xffffffffu) &&
        (gNdsFighterDLScanP1AssetID != 0xffffffffu))
    {
        mask |= 1u << 2;
    }
    if (gNdsFighterMarioFoxDLScanCount == 2u)
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterDLScanP0CommandCount > 0u) &&
        (gNdsFighterDLScanP1CommandCount > 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterDLScanP0FirstOpcode != 0u) &&
        (gNdsFighterDLScanP1FirstOpcode != 0u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsFighterDLScanP0Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLScanP1Blocker == NDS_RENDERER_BLOCKER_NONE))
    {
        mask |= 1u << 6;
    }
    if ((gNdsFighterDLScanP0UnsupportedOpcode == 0u) &&
        (gNdsFighterDLScanP1UnsupportedOpcode == 0u) &&
        (gNdsFighterDLScanP0UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLScanP1UnsupportedCommandCount == 0u))
    {
        mask |= 1u << 7;
    }

    gobj_after = (u32)gcGetGObjsActiveNum();
    gNdsFighterDLScanGObjDelta =
        (gobj_after >= gobj_before) ? (gobj_after - gobj_before) :
            (gobj_before - gobj_after);

    if ((gNdsFighterDLScanP0StatusAfter == (u32)nFTCommonStatusWait) &&
        (gNdsFighterDLScanP1StatusAfter == (u32)nFTCommonStatusWait) &&
        (gNdsFighterDLScanP0MotionAfter == (u32)nFTCommonMotionWait) &&
        (gNdsFighterDLScanP1MotionAfter == (u32)nFTCommonMotionWait) &&
        (gNdsFighterDLScanP0GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterDLScanP1GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterDLScanP0RootXBeforeBits ==
            gNdsFighterDLScanP0RootXAfterBits) &&
        (gNdsFighterDLScanP1RootXBeforeBits ==
            gNdsFighterDLScanP1RootXAfterBits) &&
        (gNdsFighterDLScanGObjDelta == 0u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsFighterDLScanDrawCallCount == 0u) &&
        (gNdsFighterDLScanMatrixCallCount == 0u) &&
        (gNdsFighterDLScanGameplayUpdateCount == 0u))
    {
        mask |= 1u << 9;
    }
    if ((gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
        (gSCManagerSceneData.scene_prev == nSCKindMaps))
    {
        mask |= 1u << 10;
    }

    gNdsFighterMarioFoxDLScanMask = mask;
    gNdsFighterMarioFoxDLScanDeferredMask = 0xffu;

    if ((mask & 0x7ffu) == 0x7ffu)
    {
        gNdsFighterMarioFoxDLScanResult =
            NDS_FIGHTER_MARIOFOX_DL_SCAN_PASS;
        gNdsFighterMarioFoxDLScanSafeResult =
            NDS_FIGHTER_MARIOFOX_DL_SCAN_SAFE_PASS;
    }
}

static u32 ndsFighterDLExecReadU32(const void *ptr)
{
    const u8 *bytes = ptr;

    return (u32)bytes[0] |
           ((u32)bytes[1] << 8) |
           ((u32)bytes[2] << 16) |
           ((u32)bytes[3] << 24);
}

static const void *ndsFighterDLExecResolveDataPointer(uintptr_t raw,
                                                      size_t bytes,
                                                      NDSFighterDLExecState *state)
{
    const void *ptr = (const void *)raw;
    uintptr_t offset = raw & 0x00ffffffu;
    u32 i;

    if ((ndsRelocFindLoadedFileContaining(ptr, bytes) != NULL) ||
        (ndsFighterDLScanRangeInTaskmanArena(ptr, bytes) != FALSE))
    {
        return ptr;
    }

    if ((state != NULL) &&
        (state->primary_file != NULL) &&
        (ndsRelocRangeInLoadedFile(state->primary_file,
                                   offset,
                                   bytes) != FALSE))
    {
        return (const u8 *)state->primary_file->data + offset;
    }

    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        if (ndsRelocRangeInLoadedFile(&sNdsRelocLoadedFiles[i],
                                      offset,
                                      bytes) != FALSE)
        {
            return (const u8 *)sNdsRelocLoadedFiles[i].data + offset;
        }
    }
    return NULL;
}

static const void *ndsFighterDLExecResolveRendererData(const void *ptr,
                                                       size_t bytes,
                                                       void *user)
{
    return ndsFighterDLExecResolveDataPointer((uintptr_t)ptr, bytes, user);
}

static s32 ndsFighterDLExecValidateRange(const Gfx *dl, size_t bytes,
                                         void *user)
{
    (void)user;

    if ((((uintptr_t)dl & (sizeof(u32) - 1u)) != 0u) ||
        ((ndsRelocFindLoadedFileContaining(dl, bytes) == NULL) &&
         (ndsFighterDLScanRangeInTaskmanArena(dl, bytes) == FALSE) &&
         (ndsRendererAdapterRangeIsEmptySegmentEDL(dl, bytes) == FALSE)))
    {
        gNdsFighterDLExecRangeRejectCount++;
        return FALSE;
    }
    return TRUE;
}

static void ndsFighterDLExecUpdateBounds(NDSFighterDLExecState *state,
                                         const NDSFighterDLExecVtx *vtx)
{
    if (state->bounds_valid == 0u)
    {
        state->min_x = state->max_x = vtx->x;
        state->min_y = state->max_y = vtx->y;
        state->min_z = state->max_z = vtx->z;
        state->bounds_valid = 1u;
    }
    else
    {
        if (vtx->x < state->min_x) { state->min_x = vtx->x; }
        if (vtx->x > state->max_x) { state->max_x = vtx->x; }
        if (vtx->y < state->min_y) { state->min_y = vtx->y; }
        if (vtx->y > state->max_y) { state->max_y = vtx->y; }
        if (vtx->z < state->min_z) { state->min_z = vtx->z; }
        if (vtx->z > state->max_z) { state->max_z = vtx->z; }
    }
}

static void ndsFighterDLExecDecodeVtx(NDSFighterDLExecState *state,
                                      u32 index, const u8 *src)
{
    NDSFighterDLExecVtx *dst;
    u32 xy;
    u32 zf;
    u32 st;
    u32 rgba;

    if ((state == NULL) || (src == NULL) || (index >= 32u))
    {
        return;
    }

    dst = &state->vertices[index];
    xy = ndsFighterDLExecReadU32(src + 0);
    zf = ndsFighterDLExecReadU32(src + 4);
    st = ndsFighterDLExecReadU32(src + 8);
    rgba = ndsFighterDLExecReadU32(src + 12);

    dst->x = (s16)(xy >> 16);
    dst->y = (s16)(xy & 0xffffu);
    dst->z = (s16)(zf >> 16);
    dst->s = (s16)(st >> 16);
    dst->t = (s16)(st & 0xffffu);
    dst->r = rgba >> 24;
    dst->g = rgba >> 16;
    dst->b = rgba >> 8;
    dst->a = rgba;
    dst->valid = TRUE;
    if (dst->a == 0)
    {
        dst->a = 0xffu;
    }

    state->vertex_valid_mask |= 1u << index;
    state->vertex_decoded_count++;
    state->color_checksum =
        (state->color_checksum * 33u) ^
        (u32)((u16)dst->x + ((u16)dst->y << 1) + ((u16)dst->z << 2)) ^
        rgba;
    ndsFighterDLExecUpdateBounds(state, dst);
}

static sb32 ndsFighterDLExecTriangleValid(NDSFighterDLExecState *state,
                                          u32 packed)
{
    u32 i0;
    u32 i1;
    u32 i2;
    u32 mask;

    ndsGBIDecodePackedTriIndices(packed, &i0, &i1, &i2);

    if ((state == NULL) || (i0 >= 32u) || (i1 >= 32u) || (i2 >= 32u))
    {
        return FALSE;
    }

    mask = (1u << i0) | (1u << i1) | (1u << i2);
    return ((state->vertex_valid_mask & mask) == mask) ? TRUE : FALSE;
}

static s32 ndsFighterMarioFoxVisitDLExecuteCommand(
    const NDSRendererCommand *command, void *user)
{
    NDSFighterDLExecState *state = user;
    u32 op;

    if ((command == NULL) || (state == NULL))
    {
        return FALSE;
    }

    op = command->op;
    switch (op)
    {
    case NDS_FIGHTER_DL_OP_NOOP:
        return TRUE;

    case NDS_FIGHTER_DL_OP_MODIFYVTX:
    {
        u32 where = (command->w0 >> 16) & 0xffu;
        u32 packed_index = command->w0 & 0xffffu;
        u32 index = packed_index / 2u;

        if (where != NDS_RENDERER_ADAPTER_G_MWO_POINT_ST)
        {
            return TRUE;
        }
        if (((packed_index & 1u) != 0u) ||
            (index >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
            ((state->vertex_valid_mask & (1u << index)) == 0u))
        {
            state->vertex_range_reject_count++;
            return TRUE;
        }
        state->vertices[index].s = (s16)(command->w1 >> 16);
        state->vertices[index].t = (s16)(command->w1 & 0xffffu);
        return TRUE;
    }

    case NDS_FIGHTER_DL_OP_VTX:
    {
        u32 v0;
        u32 count;
        size_t bytes;
        const u8 *src;
        u32 i;

        state->vertex_command_count++;
        if (ndsGBIDecodeF3DEX2Vtx(command->w0, 32u, &v0, &count) == FALSE)
        {
            state->vertex_range_reject_count++;
            return FALSE;
        }
        bytes = (size_t)count * 16u;
        src = ndsFighterDLExecResolveDataPointer((uintptr_t)command->w1,
                                                 bytes,
                                                 state);
        if (src == NULL)
        {
            state->vertex_range_reject_count++;
            return FALSE;
        }
        for (i = 0; i < count; i++)
        {
            ndsFighterDLExecDecodeVtx(state, v0 + i, src + (i * 16u));
        }
        return TRUE;
    }

    case NDS_FIGHTER_DL_OP_TRI1:
        state->triangle_command_count++;
        state->triangle_count++;
        if (ndsFighterDLExecTriangleValid(state,
                                          ndsGBIDecodeF3DEX2Tri1(command->w0))
            != FALSE)
        {
            state->triangle_valid_count++;
        }
        return TRUE;

    case NDS_FIGHTER_DL_OP_TRI2:
        state->triangle_command_count++;
        state->triangle_count += 2u;
        if (ndsFighterDLExecTriangleValid(state,
                                          ndsGBIDecodeF3DEX2Tri2First(
                                              command->w0)) != FALSE)
        {
            state->triangle_valid_count++;
        }
        if (ndsFighterDLExecTriangleValid(state,
                                          ndsGBIDecodeF3DEX2Tri2Second(
                                              command->w1)) != FALSE)
        {
            state->triangle_valid_count++;
        }
        return TRUE;

    case NDS_FIGHTER_DL_OP_CULLDL:
    case NDS_FIGHTER_DL_OP_TEXTURE:
    case NDS_FIGHTER_DL_OP_POPMTX:
    case NDS_FIGHTER_DL_OP_MTX:
    case NDS_FIGHTER_DL_OP_GEOMETRYMODE:
    case NDS_FIGHTER_DL_OP_MOVEWORD:
    case NDS_FIGHTER_DL_OP_SPECIAL_1:
    case NDS_FIGHTER_DL_OP_DL:
    case NDS_FIGHTER_DL_OP_ENDDL:
    case NDS_FIGHTER_DL_OP_SETOTHERMODE_H:
    case NDS_FIGHTER_DL_OP_SETOTHERMODE_L:
    case NDS_FIGHTER_DL_OP_SETSCISSOR:
    case NDS_FIGHTER_DL_OP_SETCOMBINE:
    case NDS_FIGHTER_DL_OP_SETCIMG:
    case NDS_FIGHTER_DL_OP_SETFOGCOLOR:
    case NDS_FIGHTER_DL_OP_SETBLENDCOLOR:
    case NDS_FIGHTER_DL_OP_SETENVCOLOR:
    case NDS_FIGHTER_DL_OP_SETPRIMCOLOR:
    case NDS_FIGHTER_DL_OP_SETTIMG:
    case NDS_FIGHTER_DL_OP_SETTILE:
    case NDS_FIGHTER_DL_OP_LOADBLOCK:
    case NDS_FIGHTER_DL_OP_LOADTLUT:
    case NDS_FIGHTER_DL_OP_SETTILESIZE:
    case NDS_FIGHTER_DL_OP_RDPSETOTHERMODE:
    case NDS_FIGHTER_DL_OP_RDPPIPESYNC:
    case NDS_FIGHTER_DL_OP_RDPLOADSYNC:
    case NDS_FIGHTER_DL_OP_RDPTILESYNC:
    case NDS_FIGHTER_DL_OP_RDPFULLSYNC:
        return TRUE;

    default:
        if (state->unsupported_opcode == 0u)
        {
            state->unsupported_opcode = op;
        }
        state->unsupported_command_count++;
        return FALSE;
    }
}

static void ndsFighterMarioFoxCopyDLExecStats(
    u32 slot, const NDSFighterDLExecState *state,
    const NDSRendererStats *stats)
{
    if ((state == NULL) || (stats == NULL))
    {
        return;
    }

    if (slot == 0u)
    {
        gNdsFighterDLExecP0Blocker = stats->blocker;
        gNdsFighterDLExecP0CommandCount = stats->command_count;
        gNdsFighterDLExecP0FirstOpcode = stats->first_opcode;
        gNdsFighterDLExecP0UnsupportedOpcode =
            (stats->unsupported_opcode != 0u) ? stats->unsupported_opcode :
                state->unsupported_opcode;
        gNdsFighterDLExecP0UnsupportedCommandCount =
            stats->unsupported_command_count + state->unsupported_command_count;
        gNdsFighterDLExecP0VertexCommandCount = state->vertex_command_count;
        gNdsFighterDLExecP0VertexDecodedCount = state->vertex_decoded_count;
        gNdsFighterDLExecP0VertexValidMask = state->vertex_valid_mask;
        gNdsFighterDLExecP0TriangleCommandCount =
            state->triangle_command_count;
        gNdsFighterDLExecP0TriangleCount = state->triangle_count;
        gNdsFighterDLExecP0TriangleValidCount = state->triangle_valid_count;
        gNdsFighterDLExecP0MinX = state->min_x;
        gNdsFighterDLExecP0MaxX = state->max_x;
        gNdsFighterDLExecP0MinY = state->min_y;
        gNdsFighterDLExecP0MaxY = state->max_y;
        gNdsFighterDLExecP0MinZ = state->min_z;
        gNdsFighterDLExecP0MaxZ = state->max_z;
        gNdsFighterDLExecP0ColorChecksum = state->color_checksum;
        gNdsFighterDLExecP0OtherModeCommandCount =
            stats->othermode_command_count;
        gNdsFighterDLExecP0CullCommandCount = stats->cull_command_count;
        gNdsFighterDLExecP0StateCommandCount = stats->state_command_count;
        gNdsFighterDLExecP0SkipCommandCount = stats->skip_command_count;
        gNdsFighterDLExecP0RenderCommandCount = stats->render_command_count;
        gNdsFighterDLExecP0BranchCommandCount = stats->branch_command_count;
        gNdsFighterDLExecP0SegmentResolveCount =
            stats->segment_resolve_count;
        gNdsFighterDLExecP0TextureMask = stats->texture_mask;
        gNdsFighterDLExecVertexRangeRejectCount +=
            state->vertex_range_reject_count;
    }
    else if (slot == 1u)
    {
        gNdsFighterDLExecP1Blocker = stats->blocker;
        gNdsFighterDLExecP1CommandCount = stats->command_count;
        gNdsFighterDLExecP1FirstOpcode = stats->first_opcode;
        gNdsFighterDLExecP1UnsupportedOpcode =
            (stats->unsupported_opcode != 0u) ? stats->unsupported_opcode :
                state->unsupported_opcode;
        gNdsFighterDLExecP1UnsupportedCommandCount =
            stats->unsupported_command_count + state->unsupported_command_count;
        gNdsFighterDLExecP1VertexCommandCount = state->vertex_command_count;
        gNdsFighterDLExecP1VertexDecodedCount = state->vertex_decoded_count;
        gNdsFighterDLExecP1VertexValidMask = state->vertex_valid_mask;
        gNdsFighterDLExecP1TriangleCommandCount =
            state->triangle_command_count;
        gNdsFighterDLExecP1TriangleCount = state->triangle_count;
        gNdsFighterDLExecP1TriangleValidCount = state->triangle_valid_count;
        gNdsFighterDLExecP1MinX = state->min_x;
        gNdsFighterDLExecP1MaxX = state->max_x;
        gNdsFighterDLExecP1MinY = state->min_y;
        gNdsFighterDLExecP1MaxY = state->max_y;
        gNdsFighterDLExecP1MinZ = state->min_z;
        gNdsFighterDLExecP1MaxZ = state->max_z;
        gNdsFighterDLExecP1ColorChecksum = state->color_checksum;
        gNdsFighterDLExecP1OtherModeCommandCount =
            stats->othermode_command_count;
        gNdsFighterDLExecP1CullCommandCount = stats->cull_command_count;
        gNdsFighterDLExecP1StateCommandCount = stats->state_command_count;
        gNdsFighterDLExecP1SkipCommandCount = stats->skip_command_count;
        gNdsFighterDLExecP1RenderCommandCount = stats->render_command_count;
        gNdsFighterDLExecP1BranchCommandCount = stats->branch_command_count;
        gNdsFighterDLExecP1SegmentResolveCount =
            stats->segment_resolve_count;
        gNdsFighterDLExecP1TextureMask = stats->texture_mask;
        gNdsFighterDLExecVertexRangeRejectCount +=
            state->vertex_range_reject_count;
    }
}

static void ndsFighterMarioFoxExecuteDLForSlot(u32 slot, FTStruct *fp)
{
    DObj *root;
    DObj *selected;
    const Gfx *dl;
    NDSRelocLoadedFile *loaded;
    NDSRendererConfig config = {0};
    NDSRendererStats stats;
    NDSFighterDLExecState state;
    u32 root_x_before;
    u32 root_x_after;
    u32 unused_index;

    if ((slot > 1u) || (ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL) ||
        (fp->status_id != nFTCommonStatusWait) ||
        (fp->motion_id != nFTCommonMotionWait) ||
        (fp->ga != nMPKineticsGround))
    {
        return;
    }

    root = fp->joints[nFTPartsJointTopN];
    root_x_before = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) :
        0u;
    selected = ndsFighterFindFirstDObjWithDL(root, &unused_index);
    if (selected == NULL)
    {
        return;
    }

    dl = selected->dl;
    loaded = ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
    if ((loaded == NULL) &&
        (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) == FALSE))
    {
        return;
    }

    bzero(&state, sizeof(state));
    state.primary_file = loaded;
    state.slot = slot;

    config.max_depth = 8u;
    config.max_commands = 2048u;
    config.max_list_commands = 512u;
    config.initial_projection = NULL;
    config.initial_modelview = NULL;
    config.initial_geometry_mode = 0u;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    config.validate_range = ndsFighterDLExecValidateRange;
    config.immutable_command_span = ndsRendererAdapterImmutableCommandSpan;
    config.resolve_branch = ndsFighterDLScanResolveBranch;
    config.resolve_data = ndsFighterDLExecResolveRendererData;
    config.user = &state;

    ndsRendererInitStats(&stats);
    ndsRendererExecuteDisplayList(dl,
                                  &config,
                                  ndsFighterMarioFoxVisitDLExecuteCommand,
                                  &state,
                                  &stats);
    ndsFighterMarioFoxCopyDLExecStats(slot, &state, &stats);

    root_x_after = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) :
        0u;
    if (slot == 0u)
    {
        gNdsFighterDLExecP0StatusAfter = (u32)fp->status_id;
        gNdsFighterDLExecP0MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLExecP0GAAfter = (u32)fp->ga;
        gNdsFighterDLExecP0RootXBeforeBits = root_x_before;
        gNdsFighterDLExecP0RootXAfterBits = root_x_after;
    }
    else
    {
        gNdsFighterDLExecP1StatusAfter = (u32)fp->status_id;
        gNdsFighterDLExecP1MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLExecP1GAAfter = (u32)fp->ga;
        gNdsFighterDLExecP1RootXBeforeBits = root_x_before;
        gNdsFighterDLExecP1RootXAfterBits = root_x_after;
    }

    gNdsFighterMarioFoxDLExecCount++;
}

static void ndsFighterMarioFoxRunDLExecuteProbe(void)
{
    u32 mask = 0u;
    u32 gobj_before;
    u32 gobj_after;

    if ((ndsFighterMarioFoxDLExecuteProofEnabled() == FALSE) ||
        (gNdsFighterMarioFoxDLExecResult != 0u))
    {
        return;
    }

    if ((gNdsFighterMarioFoxDLScanResult ==
            NDS_FIGHTER_MARIOFOX_DL_SCAN_PASS) &&
        (gNdsFighterMarioFoxDLScanSafeResult ==
            NDS_FIGHTER_MARIOFOX_DL_SCAN_SAFE_PASS) &&
        ((gNdsFighterMarioFoxDLScanMask & 0x7ffu) == 0x7ffu) &&
        (gNdsFighterMarioFoxDLScanCount == 2u) &&
        (gNdsFighterDLScanP0Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLScanP1Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLScanP0UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLScanP1UnsupportedCommandCount == 0u))
    {
        mask |= 1u << 0;
    }
    else
    {
        gNdsFighterMarioFoxDLExecMask = mask;
        return;
    }

    gobj_before = (u32)gcGetGObjsActiveNum();

    ndsFighterMarioFoxExecuteDLForSlot(0u, &sNdsFighterStructPool[0]);
    ndsFighterMarioFoxExecuteDLForSlot(1u, &sNdsFighterStructPool[1]);

    if ((gNdsFighterDLScanP0FirstDL != 0u) &&
        (gNdsFighterDLScanP1FirstDL != 0u))
    {
        mask |= 1u << 1;
    }
    if (gNdsFighterMarioFoxDLExecCount == 2u)
    {
        mask |= 1u << 2;
    }
    if ((gNdsFighterDLExecP0CommandCount > 0u) &&
        (gNdsFighterDLExecP1CommandCount > 0u) &&
        (gNdsFighterDLExecP0FirstOpcode != 0u) &&
        (gNdsFighterDLExecP1FirstOpcode != 0u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterDLExecP0VertexCommandCount > 0u) &&
        (gNdsFighterDLExecP1VertexCommandCount > 0u) &&
        (gNdsFighterDLExecP0VertexDecodedCount > 0u) &&
        (gNdsFighterDLExecP1VertexDecodedCount > 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterDLExecP0TriangleCommandCount > 0u) &&
        (gNdsFighterDLExecP1TriangleCommandCount > 0u) &&
        (gNdsFighterDLExecP0TriangleCount > 0u) &&
        (gNdsFighterDLExecP1TriangleCount > 0u) &&
        (gNdsFighterDLExecP0TriangleValidCount > 0u) &&
        (gNdsFighterDLExecP1TriangleValidCount > 0u))
    {
        mask |= 1u << 5;
    }
    if (((gNdsFighterDLExecP0MinX != gNdsFighterDLExecP0MaxX) ||
         (gNdsFighterDLExecP0MinY != gNdsFighterDLExecP0MaxY) ||
         (gNdsFighterDLExecP0MinZ != gNdsFighterDLExecP0MaxZ)) &&
        ((gNdsFighterDLExecP1MinX != gNdsFighterDLExecP1MaxX) ||
         (gNdsFighterDLExecP1MinY != gNdsFighterDLExecP1MaxY) ||
         (gNdsFighterDLExecP1MinZ != gNdsFighterDLExecP1MaxZ)) &&
        (gNdsFighterDLExecP0ColorChecksum != 0u) &&
        (gNdsFighterDLExecP1ColorChecksum != 0u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsFighterDLExecP0Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLExecP1Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLExecP0UnsupportedOpcode == 0u) &&
        (gNdsFighterDLExecP1UnsupportedOpcode == 0u) &&
        (gNdsFighterDLExecP0UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLExecP1UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLExecRangeRejectCount == 0u) &&
        (gNdsFighterDLExecVertexRangeRejectCount == 0u))
    {
        mask |= 1u << 7;
    }

    gobj_after = (u32)gcGetGObjsActiveNum();
    gNdsFighterDLExecGObjDelta =
        (gobj_after >= gobj_before) ? (gobj_after - gobj_before) :
            (gobj_before - gobj_after);

    if ((gNdsFighterDLExecP0StatusAfter == (u32)nFTCommonStatusWait) &&
        (gNdsFighterDLExecP1StatusAfter == (u32)nFTCommonStatusWait) &&
        (gNdsFighterDLExecP0MotionAfter == (u32)nFTCommonMotionWait) &&
        (gNdsFighterDLExecP1MotionAfter == (u32)nFTCommonMotionWait) &&
        (gNdsFighterDLExecP0GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterDLExecP1GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterDLExecP0RootXBeforeBits ==
            gNdsFighterDLExecP0RootXAfterBits) &&
        (gNdsFighterDLExecP1RootXBeforeBits ==
            gNdsFighterDLExecP1RootXAfterBits) &&
        (gNdsFighterDLExecGObjDelta == 0u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsFighterDLExecDrawCallCount == 0u) &&
        (gNdsFighterDLExecMatrixCallCount == 0u) &&
        (gNdsFighterDLExecGameplayUpdateCount == 0u))
    {
        mask |= 1u << 9;
    }
    if ((gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
        (gSCManagerSceneData.scene_prev == nSCKindMaps))
    {
        mask |= 1u << 10;
    }

    gNdsFighterMarioFoxDLExecMask = mask;
    gNdsFighterMarioFoxDLExecDeferredMask = 0xffu;

    if ((mask & 0x7ffu) == 0x7ffu)
    {
        gNdsFighterMarioFoxDLExecResult =
            NDS_FIGHTER_MARIOFOX_DL_EXEC_PASS;
        gNdsFighterMarioFoxDLExecSafeResult =
            NDS_FIGHTER_MARIOFOX_DL_EXEC_SAFE_PASS;
    }
}

static const Gfx *ndsFighterDLDrawResolveBranch(const Gfx *dl,
                                                 u32 *resolve_kind,
                                                 void *user)
{
    NDSFighterDLDrawState *state = user;
    uintptr_t raw = (uintptr_t)dl;
    uintptr_t offset = raw & 0x00ffffffu;
    u32 i;

    if (resolve_kind != NULL)
    {
        *resolve_kind = NDS_RENDERER_RESOLVE_NONE;
    }
    if ((ndsRelocFindLoadedFileContaining(dl, sizeof(Gfx)) != NULL) ||
        (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(Gfx)) != FALSE))
    {
        return dl;
    }
    if ((state != NULL) && (state->segment_e_base != NULL) &&
        ((raw >> 24) == 0x0eu))
    {
        uintptr_t base = (uintptr_t)state->segment_e_base;
        uintptr_t end = (uintptr_t)state->segment_e_end;

        if ((end > base) && (offset <= (end - base)) &&
            (sizeof(Gfx) <= (size_t)(end - base - offset)))
        {
            if (resolve_kind != NULL)
            {
                *resolve_kind = NDS_RENDERER_RESOLVE_SEGMENT;
            }
            return (const Gfx *)(base + offset);
        }
    }
    if ((raw >> 24) == 0x0eu)
    {
        if (resolve_kind != NULL)
        {
            *resolve_kind = NDS_RENDERER_RESOLVE_SEGMENT;
        }
        return sNdsRendererAdapterEmptySegmentEDL;
    }
    if ((state != NULL) &&
        (state->primary_file != NULL) &&
        (ndsRelocRangeInLoadedFile(state->primary_file,
                                   offset,
                                   sizeof(Gfx)) != FALSE))
    {
        if (resolve_kind != NULL)
        {
            *resolve_kind = NDS_RENDERER_RESOLVE_SEGMENT;
        }
        return (const Gfx *)((const u8 *)state->primary_file->data + offset);
    }
    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        if (ndsRelocRangeInLoadedFile(&sNdsRelocLoadedFiles[i],
                                      offset,
                                      sizeof(Gfx)) != FALSE)
        {
            if (resolve_kind != NULL)
            {
                *resolve_kind = NDS_RENDERER_RESOLVE_SEGMENT;
            }
            return (const Gfx *)((const u8 *)sNdsRelocLoadedFiles[i].data +
                                 offset);
        }
    }
    return dl;
}

static const void *ndsFighterDLDrawResolveDataPointer(uintptr_t raw,
                                                      size_t bytes,
                                                      NDSFighterDLDrawState *state)
{
    const void *ptr = (const void *)raw;
    uintptr_t offset = raw & 0x00ffffffu;
    u32 i;

    if ((ndsRelocFindLoadedFileContaining(ptr, bytes) != NULL) ||
        (ndsFighterDLScanRangeInTaskmanArena(ptr, bytes) != FALSE))
    {
        return ptr;
    }
    if ((state != NULL) && (state->segment_e_base != NULL) &&
        ((raw >> 24) == 0x0eu))
    {
        uintptr_t base = (uintptr_t)state->segment_e_base;
        uintptr_t end = (uintptr_t)state->segment_e_end;

        if ((end > base) && (offset <= (end - base)) &&
            (bytes <= (size_t)(end - base - offset)))
        {
            return (const void *)(base + offset);
        }
    }
    if ((raw >> 24) == 0x0eu)
    {
        return NULL;
    }
    if ((state != NULL) &&
        (state->primary_file != NULL) &&
        (ndsRelocRangeInLoadedFile(state->primary_file,
                                   offset,
                                   bytes) != FALSE))
    {
        return (const u8 *)state->primary_file->data + offset;
    }
    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        if (ndsRelocRangeInLoadedFile(&sNdsRelocLoadedFiles[i],
                                      offset,
                                      bytes) != FALSE)
        {
            return (const u8 *)sNdsRelocLoadedFiles[i].data + offset;
        }
    }
    return NULL;
}

static const void *ndsFighterDLDrawResolveRendererData(const void *ptr,
                                                       size_t bytes,
                                                       void *user)
{
    return ndsFighterDLDrawResolveDataPointer((uintptr_t)ptr, bytes, user);
}

static s32 ndsFighterDLDrawValidateRange(const Gfx *dl, size_t bytes,
                                         void *user)
{
    (void)user;

    if ((((uintptr_t)dl & (sizeof(u32) - 1u)) != 0u) ||
        ((ndsRelocFindLoadedFileContaining(dl, bytes) == NULL) &&
         (ndsFighterDLScanRangeInTaskmanArena(dl, bytes) == FALSE) &&
         (ndsRendererAdapterRangeIsEmptySegmentEDL(dl, bytes) == FALSE)))
    {
        gNdsFighterDLDrawRangeRejectCount++;
        return FALSE;
    }
    return TRUE;
}

static void ndsFighterDLDrawDecodeVtx(NDSFighterDLDrawState *state,
                                      u32 index, const u8 *src)
{
    NDSFighterDLDrawVtx *dst;
    u32 xy;
    u32 zf;
    u32 st;
    u32 rgba;

    if ((state == NULL) || (src == NULL) ||
        (index >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
    {
        return;
    }

    dst = &state->vertices[index];
    xy = ndsFighterDLExecReadU32(src + 0);
    zf = ndsFighterDLExecReadU32(src + 4);
    st = ndsFighterDLExecReadU32(src + 8);
    rgba = ndsFighterDLExecReadU32(src + 12);

    dst->x = (s16)(xy >> 16);
    dst->y = (s16)(xy & 0xffffu);
    dst->z = (s16)(zf >> 16);
    dst->s = (s16)(st >> 16);
    dst->t = (s16)(st & 0xffffu);
    dst->r = rgba >> 24;
    dst->g = rgba >> 16;
    dst->b = rgba >> 8;
    dst->a = rgba;
    if (dst->a == 0)
    {
        dst->a = 0xffu;
    }
    dst->valid = TRUE;

    state->vertex_valid_mask |= 1u << index;
    state->vertex_decoded_count++;
    state->color_checksum =
        (state->color_checksum * 33u) ^
        (u32)((u16)dst->x + ((u16)dst->y << 1) + ((u16)dst->z << 2)) ^
        rgba;
}

static u32 ndsFighterDLDrawCountValidVertices(u32 mask)
{
    u32 count = 0u;

    while (mask != 0u)
    {
        count += mask & 1u;
        mask >>= 1;
    }
    return count;
}

static void ndsFighterDLDrawSeedPersistentState(
    NDSFighterDLDrawState *state, const NDSFighterDLDrawState *persistent)
{
    if ((state == NULL) || (persistent == NULL))
    {
        return;
    }

    state->segment_e_base = persistent->segment_e_base;
    state->segment_e_end = persistent->segment_e_end;
    memcpy(state->vertices, persistent->vertices, sizeof(state->vertices));
    state->vertex_valid_mask = persistent->vertex_valid_mask;
    state->vertex_decoded_count =
        ndsFighterDLDrawCountValidVertices(state->vertex_valid_mask);
}

static void ndsFighterDLDrawCapturePersistentState(
    NDSFighterDLDrawState *persistent, const NDSFighterDLDrawState *state)
{
    if ((persistent == NULL) || (state == NULL))
    {
        return;
    }

    persistent->segment_e_base = state->segment_e_base;
    persistent->segment_e_end = state->segment_e_end;
    memcpy(persistent->vertices, state->vertices,
           sizeof(persistent->vertices));
    persistent->vertex_valid_mask = state->vertex_valid_mask;
}

static void ndsFighterDLDrawCopyPersistentRendererState(
    NDSRendererStats *dst, const NDSRendererStats *src)
{
#define NDS_RENDERER_COPY_STATE(field) dst->field = src->field

    if ((dst == NULL) || (src == NULL))
    {
        return;
    }

    NDS_RENDERER_COPY_STATE(othermode_h);
    NDS_RENDERER_COPY_STATE(othermode_l);
    NDS_RENDERER_COPY_STATE(geometry_mode);
    NDS_RENDERER_COPY_STATE(geometry_clear_mask);
    NDS_RENDERER_COPY_STATE(geometry_set_mask);
    NDS_RENDERER_COPY_STATE(texture_load_kind);
    NDS_RENDERER_COPY_STATE(texture_scale_s);
    NDS_RENDERER_COPY_STATE(texture_scale_t);
    NDS_RENDERER_COPY_STATE(texture_level);
    NDS_RENDERER_COPY_STATE(texture_tile);
    NDS_RENDERER_COPY_STATE(texture_on);
    NDS_RENDERER_COPY_STATE(texture_xparam);
    NDS_RENDERER_COPY_STATE(texture_state_flags);
    NDS_RENDERER_COPY_STATE(texture_image);
    NDS_RENDERER_COPY_STATE(texture_format);
    NDS_RENDERER_COPY_STATE(texture_size);
    NDS_RENDERER_COPY_STATE(texture_image_width);
    NDS_RENDERER_COPY_STATE(texture_tlut_image);
    NDS_RENDERER_COPY_STATE(texture_tlut_count);
    NDS_RENDERER_COPY_STATE(texture_tlut_tile);
    NDS_RENDERER_COPY_STATE(texture_render_tile);
    NDS_RENDERER_COPY_STATE(texture_render_tile_format);
    NDS_RENDERER_COPY_STATE(texture_render_tile_size);
    NDS_RENDERER_COPY_STATE(texture_render_tile_line);
    NDS_RENDERER_COPY_STATE(texture_render_tile_tmem);
    NDS_RENDERER_COPY_STATE(texture_render_tile_palette);
    NDS_RENDERER_COPY_STATE(texture_render_tile_cms);
    NDS_RENDERER_COPY_STATE(texture_render_tile_cmt);
    NDS_RENDERER_COPY_STATE(texture_render_tile_masks);
    NDS_RENDERER_COPY_STATE(texture_render_tile_maskt);
    NDS_RENDERER_COPY_STATE(texture_render_tile_shifts);
    NDS_RENDERER_COPY_STATE(texture_render_tile_shiftt);
    NDS_RENDERER_COPY_STATE(texture_render_tile_flags);
    NDS_RENDERER_COPY_STATE(texture_load_tile);
    NDS_RENDERER_COPY_STATE(texture_load_block_uls);
    NDS_RENDERER_COPY_STATE(texture_load_block_ult);
    NDS_RENDERER_COPY_STATE(texture_load_block_lrs);
    NDS_RENDERER_COPY_STATE(texture_load_block_dxt);
    NDS_RENDERER_COPY_STATE(texture_load_texels);
    NDS_RENDERER_COPY_STATE(texture_tile_size_tile);
    NDS_RENDERER_COPY_STATE(texture_tile_size_uls);
    NDS_RENDERER_COPY_STATE(texture_tile_size_ult);
    NDS_RENDERER_COPY_STATE(texture_tile_size_lrs);
    NDS_RENDERER_COPY_STATE(texture_tile_size_lrt);
    NDS_RENDERER_COPY_STATE(texture_tile_width);
    NDS_RENDERER_COPY_STATE(texture_tile_height);
    /* The tile-sync memo's two words travel with the state they describe. This
     * copy carries texture_tiles[] and every republished texture_render_tile_*
     * field, so carrying the serials keeps dst's memo exact; dropping either
     * one would let dst skip a republish it still owes. */
    NDS_RENDERER_COPY_STATE(texture_tile_write_serial);
    NDS_RENDERER_COPY_STATE(texture_tile_sync_serial);
    memcpy(dst->texture_tiles, src->texture_tiles, sizeof(dst->texture_tiles));
    NDS_RENDERER_COPY_STATE(texture_load_sequence);
    memcpy(dst->texture_loads, src->texture_loads,
           sizeof(dst->texture_loads));
    NDS_RENDERER_COPY_STATE(texture_combine_w0);
    NDS_RENDERER_COPY_STATE(texture_combine_w1);
    NDS_RENDERER_COPY_STATE(texture_combine_count);
    NDS_RENDERER_COPY_STATE(prim_color);
    NDS_RENDERER_COPY_STATE(prim_min_level);
    NDS_RENDERER_COPY_STATE(prim_lod_fraction);
    NDS_RENDERER_COPY_STATE(env_color);
    NDS_RENDERER_COPY_STATE(blend_color);
    NDS_RENDERER_COPY_STATE(light_color_1);
    NDS_RENDERER_COPY_STATE(light_color_2);
    NDS_RENDERER_COPY_STATE(light_color_mask);
    NDS_RENDERER_COPY_STATE(light_dir_x);
    NDS_RENDERER_COPY_STATE(light_dir_y);
    NDS_RENDERER_COPY_STATE(light_dir_z);
    NDS_RENDERER_COPY_STATE(light_dir_mask);
    NDS_RENDERER_COPY_STATE(prim_depth);
    NDS_RENDERER_COPY_STATE(prim_depth_delta);
    NDS_RENDERER_COPY_STATE(fog_color);
    NDS_RENDERER_COPY_STATE(fog_min);
    NDS_RENDERER_COPY_STATE(fog_max);
    NDS_RENDERER_COPY_STATE(fog_status);
    NDS_RENDERER_COPY_STATE(texture_source_hash1);
    NDS_RENDERER_COPY_STATE(texture_source_hash2);

#undef NDS_RENDERER_COPY_STATE
}

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
static void ndsFighterDLDrawResetTransientRendererStats(
    NDSRendererStats *stats)
{
    if (stats == NULL)
    {
        return;
    }
#if NDS_TICK_HUD
    /* Cycle 98. Both call sites of this function sit in the `detailed_output`
     * arm of an if/else whose other arm calls the Runtime reset below, and the
     * native owner production path is itself gated on detailed_output == FALSE
     * (:13862). So this is predicted to read 0 on the gate arm, i.e. the seam
     * the board called "deletion-shaped" is predicted already dead. Runtime is
     * the control that makes that zero readable. */
    gNdsFtrPreResetTransient++;
#endif

    /* The prefix is MOSTLY per-list proof/counter output. The renderer state
     * begins at othermode_h and remains live across BattleShip's stage heads
     * and fighter-part lists. A few diagnostics are interleaved with that state
     * for the profile-2 ABI and are reset explicitly.
     *
     * CORRECTED 2026-07-30 (R2-07 R4f): this comment used to say "exclusively",
     * and that is wrong in a way that invites deleting the bzero. Three fields
     * before othermode_h are read for DECISIONS, not just published --
     * hardware_texture_format, hardware_texture_width, hardware_texture_height.
     * The R2-03 E12 texture memo compares `memo->format !=
     * stats->hardware_texture_format` (nds_renderer.c:18545), and two sites
     * build masks with `1u << stats->hardware_texture_format` (:8597, :11825).
     * Dropping this clear would leak the previous part list's texture format
     * and size into those decisions. Priced before anyone tries: the prefix is
     * ~75 u32, and the clear is ~3.1% of the Results frame (~52,000 ticks/tic),
     * so it is not worth the risk even if it were safe. */
    bzero(stats, offsetof(NDSRendererStats, othermode_h));
    stats->first_cull_w0 = 0u;
    stats->first_cull_w1 = 0u;
    stats->first_branch_dl = NULL;
    stats->first_resolved_branch_dl = NULL;
    stats->geometry_command_count = 0u;
    stats->texture_mask = 0u;
    stats->texture_command_count = 0u;
    stats->texture_set_tile_count = 0u;
    stats->prim_depth_command_count = 0u;
}

static void ndsFighterDLDrawResetRuntimeRendererStats(
    NDSRendererStats *stats)
{
    if (stats == NULL)
    {
        return;
    }
#if NDS_TICK_HUD
    gNdsFtrPreResetRuntime++;
#endif

    /* Profiles 0/1 keep the ordered RDP state below intact. The null-callback
     * path only needs traversal guards and the owner-level hardware totals;
     * historical command/matrix/color ledgers remain profile-2 output. */
    stats->blocker = NDS_RENDERER_BLOCKER_NONE;
    stats->command_count = 0u;
    stats->unsupported_command_count = 0u;
    stats->end_command_count = 0u;
    stats->hardware_triangle_count = 0u;
    stats->hardware_zbuffer_triangle_count = 0u;
    stats->hardware_projected_depth_triangle_count = 0u;
    stats->hardware_decal_depth_triangle_count = 0u;
    stats->hardware_texture_bind_count = 0u;
    stats->hardware_texture_upload_count = 0u;
    stats->hardware_texture_ready_count = 0u;
    stats->hardware_texture_reject_count = 0u;
}
#endif

static sb32 ndsFighterDLDrawAppendTriangle(NDSFighterDLDrawState *state,
                                           u32 packed)
{
    u32 i0;
    u32 i1;
    u32 i2;
    u32 mask;

    ndsGBIDecodePackedTriIndices(packed, &i0, &i1, &i2);

    if ((state == NULL) ||
        (state->triangle_count >= NDS_FIGHTER_DL_DRAW_MAX_TRIS))
    {
        return FALSE;
    }

    state->tris[state->triangle_count].v0 = i0;
    state->tris[state->triangle_count].v1 = i1;
    state->tris[state->triangle_count].v2 = i2;
    state->triangle_count++;

    if ((i0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
        (i1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
        (i2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
    {
        return FALSE;
    }
    mask = (1u << i0) | (1u << i1) | (1u << i2);
    if ((state->vertex_valid_mask & mask) != mask)
    {
        return FALSE;
    }
    state->triangle_valid_count++;
    return TRUE;
}

static s32 ndsFighterMarioFoxVisitDLDrawCommand(
    const NDSRendererCommand *command, void *user)
{
    NDSFighterDLDrawState *state = user;
    u32 op;

    if ((command == NULL) || (state == NULL))
    {
        return FALSE;
    }

    op = command->op;
    switch (op)
    {
    case NDS_FIGHTER_DL_OP_NOOP:
        return TRUE;

    case NDS_FIGHTER_DL_OP_MODIFYVTX:
        return TRUE;

    case NDS_FIGHTER_DL_OP_VTX:
    {
        u32 v0;
        u32 count;
        size_t bytes;
        const u8 *src;
        u32 i;

        if (ndsGBIDecodeF3DEX2Vtx(command->w0, NDS_FIGHTER_DL_DRAW_MAX_VTX,
                                  &v0, &count) == FALSE)
        {
            state->vertex_range_reject_count++;
            return FALSE;
        }
        bytes = (size_t)count * 16u;
        src = ndsFighterDLDrawResolveDataPointer((uintptr_t)command->w1,
                                                 bytes,
                                                 state);
        if (src == NULL)
        {
            state->vertex_range_reject_count++;
            return FALSE;
        }
        for (i = 0; i < count; i++)
        {
            ndsFighterDLDrawDecodeVtx(state, v0 + i, src + (i * 16u));
        }
        return TRUE;
    }

    case NDS_FIGHTER_DL_OP_TRI1:
        ndsFighterDLDrawAppendTriangle(state,
                                       ndsGBIDecodeF3DEX2Tri1(command->w0));
        return TRUE;

    case NDS_FIGHTER_DL_OP_TRI2:
        ndsFighterDLDrawAppendTriangle(state,
                                       ndsGBIDecodeF3DEX2Tri2First(
                                           command->w0));
        ndsFighterDLDrawAppendTriangle(state,
                                       ndsGBIDecodeF3DEX2Tri2Second(
                                           command->w1));
        return TRUE;

    case NDS_FIGHTER_DL_OP_CULLDL:
    case NDS_FIGHTER_DL_OP_TEXTURE:
    case NDS_FIGHTER_DL_OP_POPMTX:
    case NDS_FIGHTER_DL_OP_MTX:
    case NDS_FIGHTER_DL_OP_GEOMETRYMODE:
    case NDS_FIGHTER_DL_OP_MOVEWORD:
    case NDS_FIGHTER_DL_OP_SPECIAL_1:
    case NDS_FIGHTER_DL_OP_DL:
    case NDS_FIGHTER_DL_OP_ENDDL:
    case NDS_FIGHTER_DL_OP_SETOTHERMODE_H:
    case NDS_FIGHTER_DL_OP_SETOTHERMODE_L:
    case NDS_FIGHTER_DL_OP_SETSCISSOR:
    case NDS_FIGHTER_DL_OP_SETCOMBINE:
    case NDS_FIGHTER_DL_OP_SETCIMG:
    case NDS_FIGHTER_DL_OP_SETFOGCOLOR:
    case NDS_FIGHTER_DL_OP_SETBLENDCOLOR:
    case NDS_FIGHTER_DL_OP_SETENVCOLOR:
    case NDS_FIGHTER_DL_OP_SETPRIMCOLOR:
    case NDS_FIGHTER_DL_OP_SETTIMG:
    case NDS_FIGHTER_DL_OP_SETTILE:
    case NDS_FIGHTER_DL_OP_LOADBLOCK:
    case NDS_FIGHTER_DL_OP_LOADTLUT:
    case NDS_FIGHTER_DL_OP_SETTILESIZE:
    case NDS_FIGHTER_DL_OP_RDPSETOTHERMODE:
    case NDS_FIGHTER_DL_OP_RDPPIPESYNC:
    case NDS_FIGHTER_DL_OP_RDPLOADSYNC:
    case NDS_FIGHTER_DL_OP_RDPTILESYNC:
    case NDS_FIGHTER_DL_OP_RDPFULLSYNC:
        return TRUE;

    default:
        if (state->unsupported_opcode == 0u)
        {
            state->unsupported_opcode = op;
        }
        state->unsupported_command_count++;
        return FALSE;
    }
}

/* scVSBattleFuncLights computes this once from gMPCollisionLightAngleX/Y and
 * reaches ndsFighterDisplayContractSetLight through the imported gSPLight. */
static Light sNdsFighterDisplayCurrentLight;
static u32 sNdsFighterDisplayCurrentLightCount;
static u32 sNdsFighterDisplayCurrentLightValid;
/* NDSRendererStats.light_color_mask bits (nds_renderer.c's private
 * NDS_RENDERER_LIGHT_COLOR_*_MASK). Declared up here because the entry-effect
 * owner seeds them long before the fighter display contract is defined. */
#define NDS_FIGHTER_DISPLAY_LIGHT_COLOR_1_MASK (1u << 0)
#define NDS_FIGHTER_DISPLAY_LIGHT_COLOR_2_MASK (1u << 1)

#if NDS_RENDERER_HW_TRIANGLES
#define NDS_RENDERER_STAGE_DL_HEADS 4u

static NDSFighterDLDrawState sNdsRendererAdapterStagePersistentState;
static NDSRendererStats sNdsRendererAdapterStagePersistentStats;
static NDSRendererVertexCache sNdsRendererAdapterStageVertexCache;
static sb32 sNdsRendererAdapterStagePersistentActive;
/* Set only while an EFFECT tree submit is on the stack. The stage, the weapons
 * and the effects all reach the hardware through the same SubmitStageDL, and
 * the stage submits hundreds of lists per frame, so publishing the executor's
 * verdict unconditionally would report the last stage list rather than the
 * effect that is being investigated. */
static sb32 sNdsRendererAdapterEffectSubmitActive;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u32 sNdsRendererAdapterStageOwnerOccurrence;
static u32 sNdsRendererAdapterStageNextOccurrence;
static u32 sNdsRendererAdapterStageListOrdinal;
#endif

#if NDS_RENDERER_PROFILE_LEVEL >= 2
#define NDS_RENDERER_OWNER_HASH_SEED 2166136261u

typedef struct NDSRendererOwnerStatsSnapshot
{
    u32 vertex_command_count;
    u32 source_vertex_count;
    u32 triangle_command_count;
    u32 triangle_count;
    u32 matrix_command_count;
} NDSRendererOwnerStatsSnapshot;

static u32 ndsRendererOwnerHashBytes(u32 hash, const void *data,
                                     size_t bytes)
{
    const u8 *cursor = data;
    size_t i;

    if (hash == 0u)
    {
        hash = NDS_RENDERER_OWNER_HASH_SEED;
    }
    for (i = 0u; i < bytes; i++)
    {
        hash ^= cursor[i];
        hash *= 16777619u;
    }
    /* Zero is the public "not started" sentinel for the compact ledgers.
     * Keep an intermediate hash from ever aliasing that sentinel. */
    if (hash == 0u)
    {
        hash = 1u;
    }
    return hash;
}

static u32 ndsRendererOwnerHashU32(u32 hash, u32 value)
{
    return ndsRendererOwnerHashBytes(hash, &value, sizeof(value));
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u32 ndsRendererOwnerRootBranchPath(
    const NDSRelocLoadedFile *loaded, const Gfx *dl, u32 selected_event)
{
    u32 hash = 0u;

    hash = ndsRendererOwnerHashU32(hash, 0x524f4f54u);
    if ((loaded != NULL) && ((uintptr_t)dl >= (uintptr_t)loaded->data) &&
        ((uintptr_t)dl <
         ((uintptr_t)loaded->data + loaded->data_size)))
    {
        hash = ndsRendererOwnerHashU32(hash, 1u);
        hash = ndsRendererOwnerHashU32(hash, loaded->asset_id);
        hash = ndsRendererOwnerHashU32(hash, loaded->owner_generation);
        hash = ndsRendererOwnerHashU32(
            hash, (u32)((uintptr_t)dl - (uintptr_t)loaded->data));
    }
    else if ((gSYTaskmanGraphicsHeap.start != NULL) &&
             (gSYTaskmanGraphicsHeap.end != NULL) &&
             ((uintptr_t)dl >=
              (uintptr_t)gSYTaskmanGraphicsHeap.start) &&
             ((uintptr_t)dl <
              (uintptr_t)gSYTaskmanGraphicsHeap.end))
    {
        hash = ndsRendererOwnerHashU32(hash, 2u);
        hash = ndsRendererOwnerHashU32(
            hash, (u32)((uintptr_t)dl -
                        (uintptr_t)gSYTaskmanGraphicsHeap.start));
    }
    else
    {
        /* Valid roots are reloc- or taskman-backed. Preserve a stable
         * segmented source token for any future resolver-backed root without
         * hashing its process address. */
        hash = ndsRendererOwnerHashU32(hash, 3u);
        hash = ndsRendererOwnerHashU32(
            hash, (u32)((uintptr_t)dl & 0x00ffffffu));
    }
    hash = ndsRendererOwnerHashU32(hash, selected_event);
    return hash;
}
#endif

#define NDS_RENDERER_OWNER_POINTER_NULL 0u
#define NDS_RENDERER_OWNER_POINTER_EMPTY_SEGMENT 1u
#define NDS_RENDERER_OWNER_POINTER_RELOC 2u
#define NDS_RENDERER_OWNER_POINTER_TASKMAN 3u
#define NDS_RENDERER_OWNER_POINTER_GRAPHICS_HEAP 4u
#define NDS_RENDERER_OWNER_POINTER_SEGMENTED 5u
#define NDS_RENDERER_OWNER_POINTER_RAW 6u

static u32 ndsRendererOwnerHashStablePointer(u32 hash, uintptr_t value)
{
    const NDSRelocLoadedFile *loaded;
    const u8 *arena = ndsTaskmanArenaStart();
    uintptr_t arena_base = (uintptr_t)arena;
    size_t arena_size = ndsTaskmanArenaSize();
    u32 segment = (u32)(value >> 24);

    hash = ndsRendererOwnerHashU32(hash, 0x50545231u);
    if (value == 0u)
    {
        return ndsRendererOwnerHashU32(
            hash, NDS_RENDERER_OWNER_POINTER_NULL);
    }
    if (value == (uintptr_t)sNdsRendererAdapterEmptySegmentEDL)
    {
        return ndsRendererOwnerHashU32(
            hash, NDS_RENDERER_OWNER_POINTER_EMPTY_SEGMENT);
    }

    loaded = ndsRelocFindLoadedFileContaining(
        (const void *)value, 1u);
    if (loaded != NULL)
    {
        hash = ndsRendererOwnerHashU32(
            hash, NDS_RENDERER_OWNER_POINTER_RELOC);
        hash = ndsRendererOwnerHashU32(hash, loaded->asset_id);
        hash = ndsRendererOwnerHashU32(
            hash, loaded->owner_generation);
        return ndsRendererOwnerHashU32(
            hash, (u32)(value - (uintptr_t)loaded->data));
    }
    if ((arena != NULL) && (value >= arena_base) &&
        ((size_t)(value - arena_base) < arena_size))
    {
        hash = ndsRendererOwnerHashU32(
            hash, NDS_RENDERER_OWNER_POINTER_TASKMAN);
        return ndsRendererOwnerHashU32(
            hash, (u32)(value - arena_base));
    }
    if ((gSYTaskmanGraphicsHeap.start != NULL) &&
        (gSYTaskmanGraphicsHeap.end != NULL) &&
        (value >= (uintptr_t)gSYTaskmanGraphicsHeap.start) &&
        (value < (uintptr_t)gSYTaskmanGraphicsHeap.end))
    {
        hash = ndsRendererOwnerHashU32(
            hash, NDS_RENDERER_OWNER_POINTER_GRAPHICS_HEAP);
        return ndsRendererOwnerHashU32(
            hash, (u32)(value -
                        (uintptr_t)gSYTaskmanGraphicsHeap.start));
    }
    if ((segment != 0u) && (segment <= 0x0fu))
    {
        hash = ndsRendererOwnerHashU32(
            hash, NDS_RENDERER_OWNER_POINTER_SEGMENTED);
        hash = ndsRendererOwnerHashU32(hash, segment);
        return ndsRendererOwnerHashU32(
            hash, (u32)(value & 0x00ffffffu));
    }

    /* Valid renderer operands are reloc-, taskman-, or segment-backed. Keep
     * an explicit raw fallback so an unexpected operand mutation is still
     * visible instead of silently aliasing the null provenance. */
    hash = ndsRendererOwnerHashU32(
        hash, NDS_RENDERER_OWNER_POINTER_RAW);
    return ndsRendererOwnerHashU32(hash, (u32)value);
}

static s32 ndsRendererOwnerCommandUsesPointer(u32 op)
{
    return ((op == NDS_FIGHTER_DL_OP_VTX) ||
            (op == NDS_FIGHTER_DL_OP_MTX) ||
            (op == 0xdcu) || /* F3DEX2 G_MOVEMEM */
            (op == NDS_FIGHTER_DL_OP_DL) ||
            (op == NDS_FIGHTER_DL_OP_SETTIMG) ||
            (op == 0xfeu) || /* G_SETZIMG */
            (op == NDS_FIGHTER_DL_OP_SETCIMG)) ? TRUE : FALSE;
}

static u32 ndsRendererOwnerHashDisplayList(
    u32 hash, const Gfx *dl, const NDSRendererConfig *config,
    u32 depth, u32 *remaining_commands)
{
    u32 i;

    hash = ndsRendererOwnerHashU32(hash, 0x4c495354u);
    hash = ndsRendererOwnerHashStablePointer(
        hash, (uintptr_t)dl);
    hash = ndsRendererOwnerHashU32(hash, depth);
    if ((dl == NULL) || (config == NULL) ||
        (remaining_commands == NULL))
    {
        return ndsRendererOwnerHashU32(hash, 0xffffffffu);
    }
    if (depth > config->max_depth)
    {
        return ndsRendererOwnerHashU32(hash, 0xfffffffeu);
    }

    for (i = 0u; i < config->max_list_commands; i++, dl++)
    {
        u32 w0;
        u32 w1;
        u32 op;

        if (*remaining_commands == 0u)
        {
            return ndsRendererOwnerHashU32(hash, 0xfffffffdu);
        }
        if ((config->validate_range != NULL) &&
            (config->validate_range(dl, sizeof(*dl), config->user) == FALSE))
        {
            hash = ndsRendererOwnerHashStablePointer(
                hash, (uintptr_t)dl);
            return ndsRendererOwnerHashU32(hash, 0xfffffffcu);
        }

        w0 = dl->words.w0;
        w1 = dl->words.w1;
        op = w0 >> 24;
        (*remaining_commands)--;
        hash = ndsRendererOwnerHashU32(hash, 0x434d4431u);
        hash = ndsRendererOwnerHashU32(hash, i);
        hash = ndsRendererOwnerHashU32(hash, w0);
        if (ndsRendererOwnerCommandUsesPointer(op) != FALSE)
        {
            hash = ndsRendererOwnerHashStablePointer(
                hash, (uintptr_t)w1);
        }
        else
        {
            hash = ndsRendererOwnerHashU32(hash, w1);
        }

        if (op == NDS_FIGHTER_DL_OP_DL)
        {
            const Gfx *branch = (const Gfx *)(uintptr_t)w1;
            u32 resolve_kind = NDS_RENDERER_RESOLVE_NONE;
            u32 branch_is_jump =
                ((w0 & (1u << 16)) != 0u) ? TRUE : FALSE;

            if (config->resolve_branch != NULL)
            {
                branch = config->resolve_branch(
                    branch, &resolve_kind, config->user);
            }
            hash = ndsRendererOwnerHashU32(hash, 0x4252414eu);
            hash = ndsRendererOwnerHashU32(hash, resolve_kind);
            hash = ndsRendererOwnerHashU32(hash, branch_is_jump);
            hash = ndsRendererOwnerHashDisplayList(
                hash, branch, config, depth + 1u, remaining_commands);
            if (branch_is_jump != FALSE)
            {
                return hash;
            }
        }
        else if (op == NDS_FIGHTER_DL_OP_ENDDL)
        {
            return ndsRendererOwnerHashU32(hash, 0x454e444cu);
        }
    }
    return ndsRendererOwnerHashU32(hash, 0x4e4f454eu);
}

static u32 ndsRendererOwnerHashTileState(
    u32 hash, const NDSRendererTileState *tile)
{
#define NDS_RENDERER_HASH_TILE_FIELD(field) \
    hash = ndsRendererOwnerHashU32(hash, tile->field)

    NDS_RENDERER_HASH_TILE_FIELD(set_seen);
    NDS_RENDERER_HASH_TILE_FIELD(size_seen);
    NDS_RENDERER_HASH_TILE_FIELD(format);
    NDS_RENDERER_HASH_TILE_FIELD(size);
    NDS_RENDERER_HASH_TILE_FIELD(line);
    NDS_RENDERER_HASH_TILE_FIELD(tmem);
    NDS_RENDERER_HASH_TILE_FIELD(palette);
    NDS_RENDERER_HASH_TILE_FIELD(cms);
    NDS_RENDERER_HASH_TILE_FIELD(cmt);
    NDS_RENDERER_HASH_TILE_FIELD(masks);
    NDS_RENDERER_HASH_TILE_FIELD(maskt);
    NDS_RENDERER_HASH_TILE_FIELD(shifts);
    NDS_RENDERER_HASH_TILE_FIELD(shiftt);
    NDS_RENDERER_HASH_TILE_FIELD(uls);
    NDS_RENDERER_HASH_TILE_FIELD(ult);
    NDS_RENDERER_HASH_TILE_FIELD(lrs);
    NDS_RENDERER_HASH_TILE_FIELD(lrt);
    NDS_RENDERER_HASH_TILE_FIELD(width);
    NDS_RENDERER_HASH_TILE_FIELD(height);
    NDS_RENDERER_HASH_TILE_FIELD(flags);

#undef NDS_RENDERER_HASH_TILE_FIELD
    return hash;
}

static u32 ndsRendererOwnerHashTextureLoadState(
    u32 hash, const NDSRendererTextureLoadState *load)
{
    hash = ndsRendererOwnerHashU32(hash, load->image);
    hash = ndsRendererOwnerHashU32(hash, load->sequence);
    hash = ndsRendererOwnerHashU32(hash, load->image_width);
    hash = ndsRendererOwnerHashU32(hash, load->load_uls);
    hash = ndsRendererOwnerHashU32(hash, load->load_ult);
    hash = ndsRendererOwnerHashU32(hash, load->load_lrs);
    hash = ndsRendererOwnerHashU32(hash, load->load_dxt);
    hash = ndsRendererOwnerHashU32(hash, load->load_texels);
    hash = ndsRendererOwnerHashU32(hash, load->load_tmem);
    hash = ndsRendererOwnerHashU32(hash, load->valid);
    hash = ndsRendererOwnerHashU32(hash, load->image_format);
    hash = ndsRendererOwnerHashU32(hash, load->image_size);
    hash = ndsRendererOwnerHashU32(hash, load->load_kind);
    hash = ndsRendererOwnerHashU32(hash, load->load_tile);
    return hash;
}

static u32 ndsRendererOwnerHashRuntimeState(const NDSRendererStats *stats)
{
    u32 hash = 0u;
    u32 i;

    if (stats == NULL)
    {
        return 0u;
    }

    /* Serialize exactly the persistent renderer contract copied by
     * ndsFighterDLDrawCopyPersistentRendererState().  Do not hash the raw
     * tail: it interleaves proof counters and pointer-bearing diagnostics,
     * and struct padding is not semantic state. */
#define NDS_RENDERER_HASH_STATE_FIELD(field) \
    hash = ndsRendererOwnerHashU32(hash, (u32)stats->field)

    NDS_RENDERER_HASH_STATE_FIELD(othermode_h);
    NDS_RENDERER_HASH_STATE_FIELD(othermode_l);
    NDS_RENDERER_HASH_STATE_FIELD(geometry_mode);
    NDS_RENDERER_HASH_STATE_FIELD(geometry_clear_mask);
    NDS_RENDERER_HASH_STATE_FIELD(geometry_set_mask);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_kind);
    NDS_RENDERER_HASH_STATE_FIELD(texture_scale_s);
    NDS_RENDERER_HASH_STATE_FIELD(texture_scale_t);
    NDS_RENDERER_HASH_STATE_FIELD(texture_level);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile);
    NDS_RENDERER_HASH_STATE_FIELD(texture_on);
    NDS_RENDERER_HASH_STATE_FIELD(texture_xparam);
    NDS_RENDERER_HASH_STATE_FIELD(texture_state_flags);
    NDS_RENDERER_HASH_STATE_FIELD(texture_image);
    NDS_RENDERER_HASH_STATE_FIELD(texture_format);
    NDS_RENDERER_HASH_STATE_FIELD(texture_size);
    NDS_RENDERER_HASH_STATE_FIELD(texture_image_width);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tlut_image);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tlut_count);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tlut_tile);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_format);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_size);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_line);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_tmem);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_palette);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_cms);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_cmt);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_masks);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_maskt);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_shifts);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_shiftt);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_flags);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_tile);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_block_uls);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_block_ult);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_block_lrs);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_block_dxt);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_texels);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_size_tile);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_size_uls);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_size_ult);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_size_lrs);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_size_lrt);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_width);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_height);
    for (i = 0u; i < NDS_RENDERER_TILE_COUNT; i++)
    {
        hash = ndsRendererOwnerHashTileState(hash,
                                              &stats->texture_tiles[i]);
    }
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_sequence);
    for (i = 0u; i < NDS_RENDERER_TEXTURE_LOAD_HISTORY_COUNT; i++)
    {
        hash = ndsRendererOwnerHashTextureLoadState(
            hash, &stats->texture_loads[i]);
    }
    NDS_RENDERER_HASH_STATE_FIELD(texture_combine_w0);
    NDS_RENDERER_HASH_STATE_FIELD(texture_combine_w1);
    NDS_RENDERER_HASH_STATE_FIELD(texture_combine_count);
    NDS_RENDERER_HASH_STATE_FIELD(prim_color);
    NDS_RENDERER_HASH_STATE_FIELD(prim_min_level);
    NDS_RENDERER_HASH_STATE_FIELD(prim_lod_fraction);
    NDS_RENDERER_HASH_STATE_FIELD(env_color);
    NDS_RENDERER_HASH_STATE_FIELD(blend_color);
    NDS_RENDERER_HASH_STATE_FIELD(light_color_1);
    NDS_RENDERER_HASH_STATE_FIELD(light_color_2);
    NDS_RENDERER_HASH_STATE_FIELD(light_color_mask);
    NDS_RENDERER_HASH_STATE_FIELD(light_dir_x);
    NDS_RENDERER_HASH_STATE_FIELD(light_dir_y);
    NDS_RENDERER_HASH_STATE_FIELD(light_dir_z);
    NDS_RENDERER_HASH_STATE_FIELD(light_dir_mask);
    NDS_RENDERER_HASH_STATE_FIELD(prim_depth);
    NDS_RENDERER_HASH_STATE_FIELD(prim_depth_delta);
    NDS_RENDERER_HASH_STATE_FIELD(fog_color);
    NDS_RENDERER_HASH_STATE_FIELD(fog_min);
    NDS_RENDERER_HASH_STATE_FIELD(fog_max);
    NDS_RENDERER_HASH_STATE_FIELD(fog_status);
    NDS_RENDERER_HASH_STATE_FIELD(texture_source_hash1);
    NDS_RENDERER_HASH_STATE_FIELD(texture_source_hash2);

#undef NDS_RENDERER_HASH_STATE_FIELD
    return hash;
}

static u32 ndsRendererOwnerHashVertexCache(
    const NDSRendererVertexCache *cache)
{
    u32 hash = 0u;
    u32 i;
    u32 row;
    u32 col;
    u32 snapshot_count;
    u32 input_mask;
    u32 transformed_mask;
    u32 color_mask;

    if (cache == NULL)
    {
        return 0u;
    }
    input_mask = cache->input_valid_mask;
    transformed_mask = cache->transformed_valid_mask & input_mask;
    color_mask = cache->vertex_color_valid_mask & input_mask;
    hash = ndsRendererOwnerHashU32(hash, input_mask);
    hash = ndsRendererOwnerHashU32(
        hash, cache->raw_vertex_fit_mask & input_mask);
    hash = ndsRendererOwnerHashU32(hash, transformed_mask);
    hash = ndsRendererOwnerHashU32(hash, color_mask);
    snapshot_count = cache->matrix_snapshot_count;
    if (snapshot_count > NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY)
    {
        snapshot_count = NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY;
    }
    hash = ndsRendererOwnerHashU32(hash, snapshot_count);
    for (i = 0u; i < NDS_RENDERER_VERTEX_CACHE_SIZE; i++)
    {
        u32 bit = 1u << i;

        if ((input_mask & bit) != 0u)
        {
            const NDSRendererInputVertex *input = &cache->input_vertices[i];

            hash = ndsRendererOwnerHashU32(hash, (u32)(s32)input->x);
            hash = ndsRendererOwnerHashU32(hash, (u32)(s32)input->y);
            hash = ndsRendererOwnerHashU32(hash, (u32)(s32)input->z);
            hash = ndsRendererOwnerHashU32(hash, (u32)(s32)input->s);
            hash = ndsRendererOwnerHashU32(hash, (u32)(s32)input->t);
            hash = ndsRendererOwnerHashU32(hash, input->r);
            hash = ndsRendererOwnerHashU32(hash, input->g);
            hash = ndsRendererOwnerHashU32(hash, input->b);
            hash = ndsRendererOwnerHashU32(hash, input->a);
            hash = ndsRendererOwnerHashU32(
                hash, cache->vertex_matrix_snapshot[i]);
            hash = ndsRendererOwnerHashU32(
                hash, cache->vertex_clip_snapshot[i]);
        }
        if ((transformed_mask & bit) != 0u)
        {
            const NDSRendererClipVertex20p12 *clip =
                &cache->transformed_vertices[i];

            hash = ndsRendererOwnerHashU32(hash, (u32)clip->x);
            hash = ndsRendererOwnerHashU32(hash, (u32)clip->y);
            hash = ndsRendererOwnerHashU32(hash, (u32)clip->z);
            hash = ndsRendererOwnerHashU32(hash, (u32)clip->w);
        }
        if ((color_mask & bit) != 0u)
        {
            hash = ndsRendererOwnerHashU32(hash,
                                           cache->vertex_colors[i]);
        }
    }
    for (i = 0u; i < snapshot_count; i++)
    {
        const NDSRendererMatrixSnapshot *snapshot =
            &cache->matrix_snapshots[i];

        for (row = 0u; row < 4u; row++)
        {
            for (col = 0u; col < 4u; col++)
            {
                hash = ndsRendererOwnerHashU32(
                    hash, (u32)snapshot->matrix.m[row][col]);
            }
        }
        hash = ndsRendererOwnerHashU32(hash, snapshot->generation);
        hash = ndsRendererOwnerHashU32(hash, snapshot->signature);
    }
    return hash;
}

static u32 ndsRendererOwnerHashResolver(
    const NDSFighterDLDrawState *state)
{
    u32 hash = 0u;
    uintptr_t base;
    uintptr_t end;
    size_t bytes;
    size_t i;

    if (state == NULL)
    {
        return 0u;
    }
    if (state->primary_file != NULL)
    {
        hash = ndsRendererOwnerHashU32(hash, 1u);
        hash = ndsRendererOwnerHashU32(
            hash, state->primary_file->asset_id);
        hash = ndsRendererOwnerHashU32(
            hash, state->primary_file->owner_generation);
        hash = ndsRendererOwnerHashU32(
            hash, state->primary_file->data_size);
    }
    else
    {
        hash = ndsRendererOwnerHashU32(hash, 0u);
    }

    base = (uintptr_t)state->segment_e_base;
    end = (uintptr_t)state->segment_e_end;
    if ((base == 0u) || (end <= base))
    {
        return ndsRendererOwnerHashU32(hash, 0u);
    }
    bytes = (size_t)(end - base);
    if (((bytes % sizeof(Gfx)) != 0u) ||
        (ndsFighterDLScanRangeInTaskmanArena(
             state->segment_e_base, bytes) == FALSE))
    {
        hash = ndsRendererOwnerHashU32(hash, 0xffffffffu);
        return ndsRendererOwnerHashU32(hash, (u32)bytes);
    }

    hash = ndsRendererOwnerHashU32(hash, (u32)(bytes / sizeof(Gfx)));
    for (i = 0u; i < (bytes / sizeof(Gfx)); i++)
    {
        const Gfx *command = &state->segment_e_base[i];
        u32 w0 = command->words.w0;
        u32 w1 = command->words.w1;
        u32 op = w0 >> 24;

        hash = ndsRendererOwnerHashU32(
            hash, w0);
        if ((op == NDS_FIGHTER_DL_OP_DL) ||
            (op == NDS_FIGHTER_DL_OP_VTX) ||
            (op == NDS_FIGHTER_DL_OP_MTX) ||
            (op == 0xdcu) || /* F3DEX2 G_MOVEMEM */
            (op == NDS_FIGHTER_DL_OP_SETTIMG))
        {
            const void *pointer = (const void *)(uintptr_t)w1;
            const NDSRelocLoadedFile *loaded = NULL;
            uintptr_t pointer_value = (uintptr_t)pointer;

            if ((pointer_value >= base) && (pointer_value < end))
            {
                hash = ndsRendererOwnerHashU32(hash, 1u);
                hash = ndsRendererOwnerHashU32(
                    hash, (u32)(pointer_value - base));
                continue;
            }
            loaded = ndsRelocFindLoadedFileContaining(pointer, 1u);
            if (loaded != NULL)
            {
                hash = ndsRendererOwnerHashU32(hash, 2u);
                hash = ndsRendererOwnerHashU32(hash, loaded->asset_id);
                hash = ndsRendererOwnerHashU32(
                    hash, loaded->owner_generation);
                hash = ndsRendererOwnerHashU32(
                    hash, (u32)(pointer_value -
                                (uintptr_t)loaded->data));
                continue;
            }
            if ((gSYTaskmanGraphicsHeap.start != NULL) &&
                (gSYTaskmanGraphicsHeap.end != NULL) &&
                (pointer_value >=
                 (uintptr_t)gSYTaskmanGraphicsHeap.start) &&
                (pointer_value <
                 (uintptr_t)gSYTaskmanGraphicsHeap.end))
            {
                hash = ndsRendererOwnerHashU32(hash, 3u);
                hash = ndsRendererOwnerHashU32(
                    hash, (u32)(pointer_value -
                                (uintptr_t)gSYTaskmanGraphicsHeap.start));
                continue;
            }
            hash = ndsRendererOwnerHashU32(hash, 4u);
        }
        hash = ndsRendererOwnerHashU32(hash, w1);
    }
    return hash;
}


static void ndsRendererOwnerSnapshotStats(
    const NDSRendererStats *stats, NDSRendererOwnerStatsSnapshot *snapshot)
{
    snapshot->vertex_command_count = stats->vertex_command_count;
    snapshot->source_vertex_count = stats->source_vertex_count;
    snapshot->triangle_command_count = stats->triangle_command_count;
    snapshot->triangle_count = stats->triangle_count;
    snapshot->matrix_command_count = stats->matrix_command_count;
}

static void ndsRendererOwnerAccumulateList(
    NDSRendererProfileOwner owner_id,
    const NDSRelocLoadedFile *loaded,
    const Gfx *dl,
    u32 selected_event,
    const NDSRendererMatrix20p12 *projection,
    const NDSRendererMatrix20p12 *modelview,
    const NDSRendererConfig *config,
    const NDSRendererOwnerStatsSnapshot *before,
    const NDSRendererStats *after)
{
    volatile NDSRendererOwnerProfile *owner;
    u32 dl_offset;
    u32 remaining_commands;

    if ((u32)owner_id >= (u32)NDS_RENDERER_PROFILE_OWNER_COUNT)
    {
        return;
    }
    owner = &gNdsRendererProfileOwners[(u32)owner_id];
    owner->selected_count++;
    owner->source_command_count += after->command_count;
    owner->vertex_command_count +=
        after->vertex_command_count - before->vertex_command_count;
    owner->source_vertex_count +=
        after->source_vertex_count - before->source_vertex_count;
    owner->triangle_command_count +=
        after->triangle_command_count - before->triangle_command_count;
    owner->triangle_count += after->triangle_count - before->triangle_count;
    /* Every selected list binds its live camera/DObj matrix pair before the
     * source stream runs.  Source MTX commands, when present, are additional
     * changes rather than the whole owner-level matrix census. */
    owner->matrix_change_count += 1u +
        after->matrix_command_count - before->matrix_command_count;

    if ((loaded != NULL) && ((uintptr_t)dl >= (uintptr_t)loaded->data) &&
        ((uintptr_t)dl <
         ((uintptr_t)loaded->data + loaded->data_size)))
    {
        dl_offset = (u32)((uintptr_t)dl - (uintptr_t)loaded->data);
    }
    else if (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) != FALSE)
    {
        dl_offset = (u32)((uintptr_t)dl -
                          (uintptr_t)ndsTaskmanArenaStart());
    }
    else
    {
        dl_offset = (u32)((uintptr_t)dl & 0x00ffffffu);
    }
    owner->topology_signature = ndsRendererOwnerHashU32(
        owner->topology_signature, 0x4f574e31u);
    owner->topology_signature = ndsRendererOwnerHashStablePointer(
        owner->topology_signature, (uintptr_t)dl);
    owner->topology_signature = ndsRendererOwnerHashU32(
        owner->topology_signature, after->command_count);
    remaining_commands = (config != NULL) ? config->max_commands : 0u;
    owner->topology_signature = ndsRendererOwnerHashDisplayList(
        owner->topology_signature, dl, config, 0u,
        &remaining_commands);
    owner->topology_signature = ndsRendererOwnerHashU32(
        owner->topology_signature, remaining_commands);
    owner->selected_event_signature = ndsRendererOwnerHashU32(
        owner->selected_event_signature, selected_event);
    owner->selected_event_signature = ndsRendererOwnerHashU32(
        owner->selected_event_signature, dl_offset);
    if (projection != NULL)
    {
        owner->camera_signature = ndsRendererOwnerHashBytes(
            owner->camera_signature, projection, sizeof(*projection));
    }
    if (modelview != NULL)
    {
        owner->dobj_matrix_signature = ndsRendererOwnerHashBytes(
            owner->dobj_matrix_signature, modelview, sizeof(*modelview));
    }
    owner->material_signature = ndsRendererOwnerHashU32(
        owner->material_signature, after->prim_color);
    owner->material_signature = ndsRendererOwnerHashU32(
        owner->material_signature, after->env_color);
    owner->material_signature = ndsRendererOwnerHashU32(
        owner->material_signature, after->blend_color);
    owner->material_signature = ndsRendererOwnerHashU32(
        owner->material_signature, after->texture_combine_w0);
    owner->material_signature = ndsRendererOwnerHashU32(
        owner->material_signature, after->texture_combine_w1);
    owner->light_signature = ndsRendererOwnerHashU32(
        owner->light_signature, after->light_color_1);
    owner->light_signature = ndsRendererOwnerHashU32(
        owner->light_signature, after->light_color_2);
    owner->light_signature = ndsRendererOwnerHashU32(
        owner->light_signature, (u32)after->light_dir_x);
    owner->light_signature = ndsRendererOwnerHashU32(
        owner->light_signature, (u32)after->light_dir_y);
    owner->light_signature = ndsRendererOwnerHashU32(
        owner->light_signature, (u32)after->light_dir_z);
    owner->texture_signature = ndsRendererOwnerHashU32(
        owner->texture_signature, after->texture_image);
    owner->texture_signature = ndsRendererOwnerHashU32(
        owner->texture_signature, after->texture_tlut_image);
    owner->texture_signature = ndsRendererOwnerHashU32(
        owner->texture_signature, after->texture_scale_s);
    owner->texture_signature = ndsRendererOwnerHashU32(
        owner->texture_signature, after->texture_scale_t);
    owner->texture_signature = ndsRendererOwnerHashU32(
        owner->texture_signature, after->texture_render_tile);
}
#endif

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static void ndsRendererAdapterAccumulateDepth(
    const NDSRendererStats *stats,
    volatile u32 *samples,
    volatile s32 *depth_min,
    volatile s32 *depth_max,
    volatile s32 *w_min,
    volatile s32 *w_max)
{
    if ((stats == NULL) || (samples == NULL) || (depth_min == NULL) ||
        (depth_max == NULL) || (w_min == NULL) || (w_max == NULL) ||
        (stats->hardware_projected_depth_sample_count == 0u))
    {
        return;
    }
    if (*samples == 0u)
    {
        *depth_min = stats->hardware_projected_depth_min;
        *depth_max = stats->hardware_projected_depth_max;
        *w_min = stats->hardware_projected_w_min;
        *w_max = stats->hardware_projected_w_max;
    }
    else
    {
        if (stats->hardware_projected_depth_min < *depth_min)
        {
            *depth_min = stats->hardware_projected_depth_min;
        }
        if (stats->hardware_projected_depth_max > *depth_max)
        {
            *depth_max = stats->hardware_projected_depth_max;
        }
        if (stats->hardware_projected_w_min < *w_min)
        {
            *w_min = stats->hardware_projected_w_min;
        }
        if (stats->hardware_projected_w_max > *w_max)
        {
            *w_max = stats->hardware_projected_w_max;
        }
    }
    *samples += stats->hardware_projected_depth_sample_count;
}
#endif

void ndsRendererAdapterResetDepthDiagnostics(void)
{
    gNdsRendererDepthStageSamples = 0u;
    gNdsRendererDepthFighterP0Samples = 0u;
    gNdsRendererDepthFighterP1Samples = 0u;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererAdapterStageOwnerOccurrence = 0u;
    sNdsRendererAdapterStageNextOccurrence = 0u;
    sNdsRendererAdapterStageListOrdinal = 0u;
#endif
}

static sb32 ndsRendererAdapterStatsHasArmedTexture(
    const NDSRendererStats *stats)
{
    return ((stats != NULL) &&
            ((stats->texture_image != 0u) ||
             (stats->texture_tlut_image != 0u) ||
             (stats->texture_on != 0u))) ? TRUE : FALSE;
}

static sb32 ndsRendererAdapterStatsHasArmedTile(
    const NDSRendererStats *stats)
{
    u32 i;

    if (stats == NULL)
    {
        return FALSE;
    }
    for (i = 0u; i < NDS_RENDERER_TILE_COUNT; i++)
    {
        const NDSRendererTileState *tile = &stats->texture_tiles[i];

        if ((tile->set_seen != 0u) || (tile->size_seen != 0u) ||
            (tile->line != 0u) || (tile->width != 0u) ||
            (tile->height != 0u))
        {
            return TRUE;
        }
    }
    return FALSE;
}

/* THE SOURCE PROC'S OWN prim/env, read back out of the DL head span it wrote.
 *
 * gDPSetPrimColor/gDPSetEnvColor now carry their words (include/PR/gbi.h), and
 * a source effect's proc_display emits exactly those two immediately before it
 * draws its model. Nothing executes the head streams on the DS, so this scans
 * the span the proc appended and hands the values to the effect submit, which
 * seeds them into the renderer's RDP state before the model list runs. Without
 * it every source effect drew in whatever prim/env the previous list left
 * behind -- the stage's, which is what made the shield bubble dark.
 *
 * Bounded and cheap: the shield's span is three commands, and a head whose
 * pointer did not move is skipped outright. */
#define NDS_RENDERER_ADAPTER_DISPLAY_PROC_SCAN_MAX 32u

static const Gfx *sNdsRendererAdapterDisplayProcHeadMark[
    NDS_RENDERER_STAGE_DL_HEADS];
static u32 sNdsRendererAdapterEffectColorMask;
static u32 sNdsRendererAdapterEffectPrimColor;
static u32 sNdsRendererAdapterEffectEnvColor;
/* Colour is per-effect; BLEND STATE IS PER-LAYER AND STICKY. The XLU bracket
 * GObj emits G_RM_AA_ZB_XLU_SURF at display order 0 and the CLD bracket
 * switches to G_RM_CLD_SURF at order 3 (efdisplay.c:15, :5), and every effect
 * drawn in between inherits it -- efManagerShieldProcDisplay emits no render
 * mode of its own at all. So this accumulates across display procs instead of
 * being rebuilt per span, which is what the RDP does. */
static u32 sNdsRendererAdapterEffectOtherModeL;
static u32 sNdsRendererAdapterEffectOtherModeValid;

/* One G_SETOTHERMODE_L packet folded into the running value, using the same
 * shift/length decode as ndsRendererRecordOtherMode (nds_renderer.c:5869) so
 * the two cannot disagree about what a word means. */
static void ndsRendererAdapterFoldDisplayProcOtherModeL(u32 w0, u32 w1)
{
    u32 bits = (w0 & 0xffu) + 1u;
    u32 pos = (w0 >> 8) & 0xffu;
    u32 shift;
    u32 mask;

    if ((bits > 32u) || (pos >= 32u) || ((bits + pos) > 32u))
    {
        return;
    }
    shift = 32u - pos - bits;
    mask = (bits >= 32u) ? 0xffffffffu : (((1u << bits) - 1u) << shift);
    sNdsRendererAdapterEffectOtherModeL =
        (sNdsRendererAdapterEffectOtherModeL & ~mask) | (w1 & mask);
    sNdsRendererAdapterEffectOtherModeValid = 1u;
}

/* Both ends of a display-proc span must be real main-RAM DL pointers before
 * anything walks between them. On the bounded fast target nothing presents,
 * so `gSYTaskmanDLHeads[]` never receives live DL cursors and holds small
 * non-NULL residue (the P2-3r3 abort read cursor=0x230 end=0x240: the NULL
 * guard passed, `ldr [0x230]` took the MPU data abort, and the nested abort
 * was every "pc=0xfffffffc" corpse the proof harness autopsied). Realtime
 * builds always carry >= 0x02000000 pointers here, so this is behavior-free
 * for every shipping configuration. */
static inline sb32 ndsRendererAdapterDisplayProcSpanValid(const Gfx *cursor,
                                                          const Gfx *end)
{
    return (((uintptr_t)cursor >= 0x02000000u) &&
            ((uintptr_t)end >= 0x02000000u) &&
            (cursor < end)) ? TRUE : FALSE;
}

/* The span one display proc emitted. Folding is idempotent (last writer wins
 * per field), so scanning a span twice -- which the effect path does, once at
 * its own draw and once when the next proc re-marks -- costs nothing. */
static void ndsRendererAdapterScanDisplayProcOtherMode(void)
{
    u32 head;

    for (head = 0u; head < NDS_RENDERER_STAGE_DL_HEADS; head++)
    {
        const Gfx *cursor = sNdsRendererAdapterDisplayProcHeadMark[head];
        const Gfx *end = gSYTaskmanDLHeads[head];
        u32 scanned = 0u;

        if (ndsRendererAdapterDisplayProcSpanValid(cursor, end) == FALSE)
        {
            continue;
        }
        while ((cursor < end) &&
               (scanned < NDS_RENDERER_ADAPTER_DISPLAY_PROC_SCAN_MAX))
        {
            if ((cursor->words.w0 >> 24) ==
                NDS_FIGHTER_DL_OP_SETOTHERMODE_L)
            {
                ndsRendererAdapterFoldDisplayProcOtherModeL(
                    cursor->words.w0, cursor->words.w1);
            }
            cursor++;
            scanned++;
        }
    }
}

void __attribute__((section(".itcm")))
ndsRendererAdapterMarkDisplayProcHeads(void)
{
    u32 i;

    /* Close the previous proc's span before opening this one. The XLU and CLD
     * brackets draw NOTHING, so they never reach the effect submit path and
     * their span would otherwise be overwritten unread -- which is precisely
     * the state the shield depends on. */
    ndsRendererAdapterScanDisplayProcOtherMode();
    for (i = 0u; i < NDS_RENDERER_STAGE_DL_HEADS; i++)
    {
        sNdsRendererAdapterDisplayProcHeadMark[i] = gSYTaskmanDLHeads[i];
    }
}

void ndsRendererAdapterCaptureDisplayProcColors(void)
{
    u32 head;

    sNdsRendererAdapterEffectColorMask = 0u;
    for (head = 0u; head < NDS_RENDERER_STAGE_DL_HEADS; head++)
    {
        const Gfx *cursor = sNdsRendererAdapterDisplayProcHeadMark[head];
        const Gfx *end = gSYTaskmanDLHeads[head];
        u32 scanned = 0u;

        if (ndsRendererAdapterDisplayProcSpanValid(cursor, end) == FALSE)
        {
            continue;
        }
        while ((cursor < end) &&
               (scanned < NDS_RENDERER_ADAPTER_DISPLAY_PROC_SCAN_MAX))
        {
            u32 op = cursor->words.w0 >> 24;

            if (op == NDS_FIGHTER_DL_OP_SETPRIMCOLOR)
            {
                sNdsRendererAdapterEffectPrimColor = cursor->words.w1;
                sNdsRendererAdapterEffectColorMask |= 1u;
            }
            else if (op == NDS_FIGHTER_DL_OP_SETENVCOLOR)
            {
                sNdsRendererAdapterEffectEnvColor = cursor->words.w1;
                sNdsRendererAdapterEffectColorMask |= 2u;
            }
            else if (op == NDS_FIGHTER_DL_OP_SETOTHERMODE_L)
            {
                /* The impact wave sets its own mode at the top of its own
                 * proc (efmanager.c:3286), so the current span matters too. */
                ndsRendererAdapterFoldDisplayProcOtherModeL(
                    cursor->words.w0, cursor->words.w1);
            }
            cursor++;
            scanned++;
        }
    }
    gNdsEffectDLColorMask = sNdsRendererAdapterEffectColorMask;
    gNdsEffectDLPrimColor = sNdsRendererAdapterEffectPrimColor;
    gNdsEffectDLEnvColor = sNdsRendererAdapterEffectEnvColor;
    gNdsEffectDLOtherModeL = sNdsRendererAdapterEffectOtherModeL;
    gNdsEffectDLOtherModeValid = sNdsRendererAdapterEffectOtherModeValid;
}

void ndsRendererAdapterBeginStageTraversal(void)
{
    bzero(&sNdsRendererAdapterStagePersistentState,
          sizeof(sNdsRendererAdapterStagePersistentState));
    ndsRendererInitStats(&sNdsRendererAdapterStagePersistentStats);
    if ((sNdsFighterDisplayCurrentLightValid != FALSE) &&
        (sNdsFighterDisplayCurrentLightCount != 0u))
    {
        sNdsRendererAdapterStagePersistentStats.light_dir_x =
            sNdsFighterDisplayCurrentLight.l.dir[0];
        sNdsRendererAdapterStagePersistentStats.light_dir_y =
            sNdsFighterDisplayCurrentLight.l.dir[1];
        sNdsRendererAdapterStagePersistentStats.light_dir_z =
            sNdsFighterDisplayCurrentLight.l.dir[2];
        sNdsRendererAdapterStagePersistentStats.light_dir_mask = 1u;
    }
    ndsRendererInitVertexCache(&sNdsRendererAdapterStageVertexCache);
    sNdsRendererAdapterStagePersistentActive = TRUE;
#if NDS_RENDERER_HW_TRIANGLES
    ndsRendererProfileSetOwner(NDS_RENDERER_PROFILE_OWNER_STAGE);
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererAdapterStageOwnerOccurrence =
        sNdsRendererAdapterStageNextOccurrence++;
    sNdsRendererAdapterStageListOrdinal = 0u;
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].entry_state_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].entry_state_hash,
            ndsRendererOwnerHashRuntimeState(
                &sNdsRendererAdapterStagePersistentStats));
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].entry_vertex_cache_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].entry_vertex_cache_hash,
            ndsRendererOwnerHashVertexCache(
                &sNdsRendererAdapterStageVertexCache));
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].entry_resolver_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].entry_resolver_hash,
            ndsRendererOwnerHashResolver(
                &sNdsRendererAdapterStagePersistentState));
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].entry_global_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].entry_global_hash,
            ndsRendererProfileGlobalStateHash());
#endif
}

void ndsRendererAdapterEndStageTraversal(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].exit_state_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].exit_state_hash,
            ndsRendererOwnerHashRuntimeState(
                &sNdsRendererAdapterStagePersistentStats));
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].exit_vertex_cache_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].exit_vertex_cache_hash,
            ndsRendererOwnerHashVertexCache(
                &sNdsRendererAdapterStageVertexCache));
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].exit_resolver_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].exit_resolver_hash,
            ndsRendererOwnerHashResolver(
                &sNdsRendererAdapterStagePersistentState));
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].exit_global_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].exit_global_hash,
            ndsRendererProfileGlobalStateHash());
#endif
#if NDS_RENDERER_HW_TRIANGLES
    ndsRendererProfileSetOwner(NDS_RENDERER_PROFILE_OWNER_NONE);
#endif
    sNdsRendererAdapterStagePersistentActive = FALSE;
}

static s32 ndsRendererAdapterStageValidateRange(const Gfx *dl, size_t bytes,
                                                void *user)
{
    (void)user;

    if ((((uintptr_t)dl & (sizeof(u32) - 1u)) != 0u) ||
        ((ndsFighterDLScanRangeInTaskmanArena(dl, bytes) == FALSE) &&
         (ndsRelocFindLoadedFileContaining(dl, bytes) == NULL) &&
         (ndsRendererAdapterRangeIsEmptySegmentEDL(dl, bytes) == FALSE)))
    {
        return FALSE;
    }
    return TRUE;
}

static sb32 ndsRendererAdapterStageDObjDrawable(DObj *dobj, u32 kind)
{
    if (dobj == NULL)
    {
        return FALSE;
    }
    if ((dobj->flags & DOBJ_FLAG_HIDDEN) != 0)
    {
        return FALSE;
    }

    switch (kind)
    {
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS:
        return ((dobj->flags & DOBJ_FLAG_NOTEXTURE) == 0) ? TRUE : FALSE;

    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD0:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1:
        return (dobj->flags == DOBJ_FLAG_NONE) ? TRUE : FALSE;

    default:
        return FALSE;
    }
}

static u32 ndsRendererAdapterMaterialFlags(const MObj *mobj)
{
    u32 flags;

    if (mobj == NULL)
    {
        return MOBJ_FLAG_NONE;
    }
    flags = mobj->sub.flags;
    return (flags == MOBJ_FLAG_NONE) ?
        (MOBJ_FLAG_TEXTURE | 0x20u | MOBJ_FLAG_ALPHA) : flags;
}

static u32 ndsRendererAdapterMaterialPositiveOrOne(s32 value)
{
    return (value <= 0) ? 1u : (u32)value;
}

static void ndsRendererAdapterMaterialLoadBlock(const MObj *mobj,
                                                u32 *texels,
                                                u32 *dxt)
{
    s32 load_texels = 0;
    u32 divisor = 1u;

    if ((mobj == NULL) || (texels == NULL) || (dxt == NULL))
    {
        return;
    }

    switch (mobj->sub.block_siz)
    {
    case G_IM_SIZ_4b:
        load_texels =
            ((((s32)mobj->sub.block_dxt * (s32)mobj->sub.unk36) + 3) >> 2) -
            1;
        divisor = ndsRendererAdapterMaterialPositiveOrOne(
            (s32)mobj->sub.block_dxt / 16);
        break;
    case G_IM_SIZ_8b:
        load_texels =
            ((((s32)mobj->sub.block_dxt * (s32)mobj->sub.unk36) + 1) >> 1) -
            1;
        divisor = ndsRendererAdapterMaterialPositiveOrOne(
            (s32)mobj->sub.block_dxt / 8);
        break;
    case G_IM_SIZ_16b:
        load_texels =
            ((s32)mobj->sub.block_dxt * (s32)mobj->sub.unk36) - 1;
        divisor = ndsRendererAdapterMaterialPositiveOrOne(
            ((s32)mobj->sub.block_dxt * 2) / 8);
        break;
    case G_IM_SIZ_32b:
        load_texels =
            ((s32)mobj->sub.block_dxt * (s32)mobj->sub.unk36) - 1;
        divisor = ndsRendererAdapterMaterialPositiveOrOne(
            ((s32)mobj->sub.block_dxt * 4) / 8);
        break;
    default:
        break;
    }

    *texels = (load_texels > 0) ? (u32)load_texels : 0u;
    *dxt = (divisor + 0x7ffu) / divisor;
}

static u32 ndsRendererAdapterMaterialCommandCount(const MObj *mobj, u32 flags)
{
    u32 count = 1u;

    if (((flags & MOBJ_FLAG_PALETTE) == 0u) &&
        (mobj != NULL) &&
        (mobj->sub.palettes != NULL))
    {
        count++;
    }
    if ((flags & MOBJ_FLAG_PALETTE) != 0)
    {
        count++;
        if ((flags & (MOBJ_FLAG_SPLIT | MOBJ_FLAG_ALPHA)) != 0)
        {
            count += 5u;
        }
    }
    if ((flags & MOBJ_FLAG_LIGHT1) != 0)
    {
        count += 2u;
    }
    if ((flags & MOBJ_FLAG_LIGHT2) != 0)
    {
        count += 2u;
    }
    if ((flags & (MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_FRAC | 0x8u)) != 0)
    {
        count++;
    }
    if ((flags & MOBJ_FLAG_ENVCOLOR) != 0)
    {
        count++;
    }
    if ((flags & MOBJ_FLAG_BLENDCOLOR) != 0)
    {
        count++;
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_SPLIT)) != 0)
    {
        count++;
        if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
        {
            count += 3u;
        }
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
    {
        count++;
    }
    if ((flags & 0x20u) != 0)
    {
        count++;
    }
    if ((flags & 0x40u) != 0)
    {
        count++;
    }
    if ((flags & MOBJ_FLAG_TEXTURE) != 0)
    {
        count++;
    }
    return count;
}

static sb32 ndsRendererAdapterCountMaterialCommands(DObj *dobj,
                                                    u32 *mobj_count,
                                                    u32 *branch_commands)
{
    MObj *mobj;
    u32 count = 0u;
    u32 commands = 0u;

    if ((dobj == NULL) || (mobj_count == NULL) ||
        (branch_commands == NULL))
    {
        return FALSE;
    }
    for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
    {
        count++;
        if (count > NDS_RENDERER_ADAPTER_MATERIAL_MOBJ_MAX)
        {
            return FALSE;
        }
        commands += ndsRendererAdapterMaterialCommandCount(
            mobj, ndsRendererAdapterMaterialFlags(mobj));
    }
    *mobj_count = count;
    *branch_commands = commands;
    return TRUE;
}

static void ndsRendererAdapterMaterialTextureState(
    const MObj *mobj,
    u32 flags,
    f32 *scau,
    f32 *scav,
    f32 *trau,
    f32 *trav,
    f32 *scrollu,
    f32 *scrollv)
{
    if ((mobj == NULL) || (scau == NULL) || (scav == NULL) ||
        (trau == NULL) || (trav == NULL) || (scrollu == NULL) ||
        (scrollv == NULL) ||
        ((flags & (MOBJ_FLAG_TEXTURE | 0x40u | 0x20u)) == 0))
    {
        return;
    }

    *scau = mobj->sub.scau;
    *scav = mobj->sub.scav;
    *trau = mobj->sub.trau;
    *trav = mobj->sub.trav;
    *scrollu = mobj->sub.scrollu;
    *scrollv = mobj->sub.scrollv;

    if (mobj->sub.unk10 == 1)
    {
        *scau *= 0.5F;
        *trau =
            ((*trau - mobj->sub.unk24) + 1.0F -
             (mobj->sub.unk28 * 0.5F)) *
            0.5F;
        *scrollu =
            ((*scrollu - mobj->sub.unk44) + 1.0F -
             (mobj->sub.unk28 * 0.5F)) *
            0.5F;
    }
}

static u32 ndsRendererAdapterClampU8S32(s32 value)
{
    if (value < 0)
    {
        return 0u;
    }
    return (value > 0xff) ? 0xffu : (u32)value;
}

static u32 ndsRendererAdapterClampU8F32(f32 value)
{
    return ndsRendererAdapterClampU8S32((s32)value);
}

static u32 ndsRendererAdapterPackColor(const SYColorPack *color)
{
    if (color == NULL)
    {
        return 0u;
    }
    return ((u32)color->s.r << 24) |
           ((u32)color->s.g << 16) |
           ((u32)color->s.b << 8) |
           (u32)color->s.a;
}

static const void *ndsRendererAdapterReadPointerEntry(void **items,
                                                      s32 index)
{
    if ((items == NULL) || (index < 0))
    {
        return NULL;
    }
    return items[index];
}

static void ndsRendererAdapterEmitBranchTableCommand(Gfx *cmd,
                                                     const Gfx *branch)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 = (NDS_FIGHTER_DL_OP_DL << 24) | (1u << 16);
    cmd->words.w1 = (u32)(uintptr_t)branch;
}

static void ndsRendererAdapterEmitEndDL(Gfx *cmd)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 = NDS_FIGHTER_DL_OP_ENDDL << 24;
    cmd->words.w1 = 0u;
}

static void ndsRendererAdapterEmitSync(Gfx *cmd, u32 op)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 = op << 24;
    cmd->words.w1 = 0u;
}

static void ndsRendererAdapterEmitTextureImage(Gfx *cmd,
                                               u32 fmt,
                                               u32 siz,
                                               u32 width,
                                               const void *image)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_SETTIMG << 24) |
        ((fmt & 0x7u) << 21) |
        ((siz & 0x3u) << 19) |
        (((width != 0u) ? (width - 1u) : 0u) & 0x0fffu);
    cmd->words.w1 = (u32)(uintptr_t)image;
}

static void ndsRendererAdapterEmitSetTile(Gfx *cmd,
                                          u32 fmt,
                                          u32 siz,
                                          u32 line,
                                          u32 tmem,
                                          u32 tile,
                                          u32 palette,
                                          u32 cmt,
                                          u32 maskt,
                                          u32 shiftt,
                                          u32 cms,
                                          u32 masks,
                                          u32 shifts)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_SETTILE << 24) |
        ((fmt & 0x7u) << 21) |
        ((siz & 0x3u) << 19) |
        ((line & 0x01ffu) << 9) |
        (tmem & 0x01ffu);
    cmd->words.w1 =
        ((tile & 0x7u) << 24) |
        ((palette & 0x0fu) << 20) |
        ((cmt & 0x3u) << 18) |
        ((maskt & 0x0fu) << 14) |
        ((shiftt & 0x0fu) << 10) |
        ((cms & 0x3u) << 8) |
        ((masks & 0x0fu) << 4) |
        (shifts & 0x0fu);
}

static void ndsRendererAdapterEmitLoadTlut(Gfx *cmd, u32 tile, u32 count)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 = NDS_FIGHTER_DL_OP_LOADTLUT << 24;
    cmd->words.w1 =
        ((tile & 0x7u) << 24) |
        ((count & 0x03ffu) << 14);
}

static void ndsRendererAdapterEmitMoveWord(Gfx *cmd,
                                           u32 index,
                                           u32 offset,
                                           u32 data)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_MOVEWORD << 24) |
        ((index & 0xffu) << 16) |
        (offset & 0xffffu);
    cmd->words.w1 = data;
}

static Gfx *ndsRendererAdapterEmitLightColor(Gfx *branch_dl,
                                             u32 light,
                                             u32 color)
{
    u32 offset_a = NDS_RENDERER_ADAPTER_G_MWO_A_LIGHT_1;
    u32 offset_b = NDS_RENDERER_ADAPTER_G_MWO_B_LIGHT_1;

    if (branch_dl == NULL)
    {
        return branch_dl;
    }
    if (light == 2u)
    {
        offset_a = NDS_RENDERER_ADAPTER_G_MWO_A_LIGHT_2;
        offset_b = NDS_RENDERER_ADAPTER_G_MWO_B_LIGHT_2;
    }
    ndsRendererAdapterEmitMoveWord(branch_dl++,
                                   NDS_RENDERER_ADAPTER_G_MW_LIGHTCOL,
                                   offset_a,
                                   color);
    ndsRendererAdapterEmitMoveWord(branch_dl++,
                                   NDS_RENDERER_ADAPTER_G_MW_LIGHTCOL,
                                   offset_b,
                                   color);
    return branch_dl;
}

static void ndsRendererAdapterEmitPrimColor(Gfx *cmd,
                                            u32 m,
                                            u32 l,
                                            u32 r,
                                            u32 g,
                                            u32 b,
                                            u32 a)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_SETPRIMCOLOR << 24) |
        ((m & 0xffu) << 8) |
        (l & 0xffu);
    cmd->words.w1 =
        ((r & 0xffu) << 24) |
        ((g & 0xffu) << 16) |
        ((b & 0xffu) << 8) |
        (a & 0xffu);
}

static void ndsRendererAdapterEmitColor(Gfx *cmd,
                                        u32 op,
                                        u32 r,
                                        u32 g,
                                        u32 b,
                                        u32 a)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 = op << 24;
    cmd->words.w1 =
        ((r & 0xffu) << 24) |
        ((g & 0xffu) << 16) |
        ((b & 0xffu) << 8) |
        (a & 0xffu);
}

static void ndsRendererAdapterEmitLoadBlock(Gfx *cmd,
                                            u32 tile,
                                            u32 uls,
                                            u32 ult,
                                            u32 lrs,
                                            u32 dxt)
{
    if (cmd == NULL)
    {
        return;
    }
    if (lrs > NDS_RENDERER_ADAPTER_G_TX_LDBLK_MAX_TXL)
    {
        lrs = NDS_RENDERER_ADAPTER_G_TX_LDBLK_MAX_TXL;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_LOADBLOCK << 24) |
        ((uls & 0x0fffu) << 12) |
        (ult & 0x0fffu);
    cmd->words.w1 =
        ((tile & 0x7u) << 24) |
        ((lrs & 0x0fffu) << 12) |
        (dxt & 0x0fffu);
}

static void ndsRendererAdapterEmitTileSize(Gfx *cmd,
                                           u32 tile,
                                           s32 uls,
                                           s32 ult,
                                           s32 lrs,
                                           s32 lrt)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_SETTILESIZE << 24) |
        (((u32)uls & 0x0fffu) << 12) |
        ((u32)ult & 0x0fffu);
    cmd->words.w1 =
        ((tile & 0x7u) << 24) |
        (((u32)lrs & 0x0fffu) << 12) |
        ((u32)lrt & 0x0fffu);
}

static void ndsRendererAdapterEmitTexture(Gfx *cmd,
                                          u32 s,
                                          u32 t,
                                          u32 level,
                                          u32 tile,
                                          u32 on)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_TEXTURE << 24) |
        ((level & 0x7u) << 11) |
        ((tile & 0x7u) << 8) |
        ((on & 0x7fu) << 1);
    cmd->words.w1 =
        ((s & 0xffffu) << 16) |
        (t & 0xffffu);
}

static void ndsRendererAdapterNativeMaterialImage(
    u32 fmt, u32 siz, u32 width, const void *image,
    u32 *out_w0, u32 *out_image)
{
    if ((out_w0 == NULL) || (out_image == NULL))
    {
        return;
    }
    *out_w0 =
        (NDS_FIGHTER_DL_OP_SETTIMG << 24) |
        ((fmt & 0x7u) << 21) |
        ((siz & 0x3u) << 19) |
        (((width != 0u) ? (width - 1u) : 0u) & 0x0fffu);
    *out_image = (u32)(uintptr_t)image;
}

static void ndsRendererAdapterNativeMaterialTile(
    u32 fmt, u32 siz, u32 line, u32 tmem, u32 tile, u32 palette,
    u32 cmt, u32 maskt, u32 shiftt, u32 cms, u32 masks, u32 shifts,
    u32 *out_w0, u32 *out_w1)
{
    if ((out_w0 == NULL) || (out_w1 == NULL))
    {
        return;
    }
    *out_w0 =
        (NDS_FIGHTER_DL_OP_SETTILE << 24) |
        ((fmt & 0x7u) << 21) |
        ((siz & 0x3u) << 19) |
        ((line & 0x01ffu) << 9) |
        (tmem & 0x01ffu);
    *out_w1 =
        ((tile & 0x7u) << 24) |
        ((palette & 0x0fu) << 20) |
        ((cmt & 0x3u) << 18) |
        ((maskt & 0x0fu) << 14) |
        ((shiftt & 0x0fu) << 10) |
        ((cms & 0x3u) << 8) |
        ((masks & 0x0fu) << 4) |
        (shifts & 0x0fu);
}

static void ndsRendererAdapterNativeMaterialTileSize(
    u32 tile, s32 uls, s32 ult, s32 lrs, s32 lrt,
    u32 *out_w0, u32 *out_w1)
{
    if ((out_w0 == NULL) || (out_w1 == NULL))
    {
        return;
    }
    *out_w0 =
        (NDS_FIGHTER_DL_OP_SETTILESIZE << 24) |
        (((u32)uls & 0x0fffu) << 12) |
        ((u32)ult & 0x0fffu);
    *out_w1 =
        ((tile & 0x7u) << 24) |
        (((u32)lrs & 0x0fffu) << 12) |
        ((u32)lrt & 0x0fffu);
}

static sb32 ndsRendererAdapterBuildNativeMaterialSnapshot(
    MObj *mobj, NDSRendererNativeMaterial *out, sb32 advance_texture_ids,
    s32 *out_curr, s32 *out_next)
{
    u32 flags;
    f32 scau = 0.0F;
    f32 scav = 0.0F;
    f32 trau = 0.0F;
    f32 trav = 0.0F;
    f32 scrollu = 0.0F;
    f32 scrollv = 0.0F;
    s32 uls;
    s32 ult;
    s32 s;
    s32 t;
    s32 texture_id_curr;
    s32 texture_id_next;

    if ((mobj == NULL) || (out == NULL))
    {
        return FALSE;
    }
    texture_id_curr = mobj->texture_id_curr;
    texture_id_next = mobj->texture_id_next;
    bzero(out, sizeof(*out));
    flags = ndsRendererAdapterMaterialFlags(mobj);
    out->command_count = 1u; /* ENDDL */
    ndsRendererAdapterMaterialTextureState(
        mobj, flags, &scau, &scav, &trau, &trav, &scrollu, &scrollv);

    if (((flags & MOBJ_FLAG_PALETTE) == 0u) &&
        (mobj->sub.palettes != NULL))
    {
        const void *palette = ndsRendererAdapterReadPointerEntry(
            mobj->sub.palettes, (s32)mobj->palette_id);

        if (palette != NULL)
        {
            out->effects |= NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE;
            ndsRendererAdapterNativeMaterialImage(
                G_IM_FMT_RGBA, G_IM_SIZ_16b, 1u, palette,
                &out->palette_image_w0, &out->palette_image);
            out->command_count++;
        }
    }
    if ((flags & MOBJ_FLAG_PALETTE) != 0u)
    {
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE;
        ndsRendererAdapterNativeMaterialImage(
            G_IM_FMT_RGBA, G_IM_SIZ_16b, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.palettes, (s32)mobj->palette_id),
            &out->palette_image_w0, &out->palette_image);
        out->command_count++;
        if ((flags & (MOBJ_FLAG_SPLIT | MOBJ_FLAG_ALPHA)) != 0u)
        {
            out->effects |= NDS_RENDERER_NATIVE_MATERIAL_PALETTE_TLUT;
            ndsRendererAdapterNativeMaterialTile(
                G_IM_FMT_RGBA, G_IM_SIZ_4b, 0u, 0x0100u, 5u, 0u,
                NDS_RENDERER_ADAPTER_G_TX_WRAP,
                NDS_RENDERER_ADAPTER_G_TX_NOMASK,
                NDS_RENDERER_ADAPTER_G_TX_NOLOD,
                NDS_RENDERER_ADAPTER_G_TX_WRAP,
                NDS_RENDERER_ADAPTER_G_TX_NOMASK,
                NDS_RENDERER_ADAPTER_G_TX_NOLOD,
                &out->palette_tile_w0, &out->palette_tile_w1);
            out->palette_tlut_w1 =
                (5u << 24) |
                (((mobj->sub.siz == G_IM_SIZ_8b) ? 0xffu : 0x0fu) << 14);
            out->sync_count += 3u;
            out->command_count += 5u;
        }
    }
    if ((flags & MOBJ_FLAG_LIGHT1) != 0u)
    {
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_LIGHT1;
        out->light1 = ndsRendererAdapterPackColor(&mobj->sub.light1color);
        out->command_count += 2u;
    }
    if ((flags & MOBJ_FLAG_LIGHT2) != 0u)
    {
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_LIGHT2;
        out->light2 = ndsRendererAdapterPackColor(&mobj->sub.light2color);
        out->command_count += 2u;
    }
    if ((flags & (MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_FRAC | 0x8u)) != 0u)
    {
        u32 level;

        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_PRIM;
        if ((flags & MOBJ_FLAG_FRAC) != 0u)
        {
            s32 trunc = (s32)mobj->lfrac;

            level = ndsRendererAdapterClampU8F32(
                (mobj->lfrac - (f32)trunc) * 256.0F);
            texture_id_curr = trunc;
            texture_id_next = trunc + 1;
        }
        else
        {
            level = ndsRendererAdapterClampU8F32(mobj->lfrac * 255.0F);
        }
        out->prim_w0 =
            (NDS_FIGHTER_DL_OP_SETPRIMCOLOR << 24) |
            (((u32)mobj->sub.prim_m & 0xffu) << 8) |
            (level & 0xffu);
        out->prim_w1 = ndsRendererAdapterPackColor(&mobj->sub.primcolor);
        out->command_count++;
    }
    if ((flags & MOBJ_FLAG_ENVCOLOR) != 0u)
    {
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_ENV;
        out->env_color = ndsRendererAdapterPackColor(&mobj->sub.envcolor);
        out->command_count++;
    }
    if ((flags & MOBJ_FLAG_BLENDCOLOR) != 0u)
    {
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_BLEND;
        out->blend_color =
            ndsRendererAdapterPackColor(&mobj->sub.blendcolor);
        out->command_count++;
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_SPLIT)) != 0u)
    {
        u32 block_siz = (mobj->sub.block_siz == G_IM_SIZ_32b) ?
            G_IM_SIZ_32b : G_IM_SIZ_16b;

        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_BLOCK_IMAGE;
        ndsRendererAdapterNativeMaterialImage(
            mobj->sub.block_fmt, block_siz, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.sprites, texture_id_next),
            &out->block_image_w0, &out->block_image);
        out->command_count++;
        if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0u)
        {
            u32 texels = 0u;
            u32 dxt = 0u;

            ndsRendererAdapterMaterialLoadBlock(mobj, &texels, &dxt);
            if (texels > NDS_RENDERER_ADAPTER_G_TX_LDBLK_MAX_TXL)
            {
                texels = NDS_RENDERER_ADAPTER_G_TX_LDBLK_MAX_TXL;
            }
            out->effects |= NDS_RENDERER_NATIVE_MATERIAL_LOAD_BLOCK;
            out->load_block_w0 = NDS_FIGHTER_DL_OP_LOADBLOCK << 24;
            out->load_block_w1 =
                (6u << 24) | ((texels & 0x0fffu) << 12) |
                (dxt & 0x0fffu);
            out->sync_count += 2u;
            out->command_count += 3u;
        }
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0u)
    {
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE;
        ndsRendererAdapterNativeMaterialImage(
            mobj->sub.fmt, mobj->sub.siz, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.sprites, texture_id_curr),
            &out->current_image_w0, &out->current_image);
        out->command_count++;
    }
    if ((flags & 0x20u) != 0u)
    {
        if (mobj->sub.unk10 == 2)
        {
            uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
                (s32)((((f32)mobj->sub.unk0C * trau) / scau) * 4.0F) : 0;
            ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
                (s32)((((f32)mobj->sub.unk0E * trav) / scav) * 4.0F) : 0;
            if (uls < 0) { uls = 0; }
            if (ult < 0) { ult = 0; }
        }
        else
        {
            uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
                (s32)(((((f32)mobj->sub.unk0C * trau) +
                         (f32)mobj->sub.unk0A) / scau) * 4.0F) : 0;
            ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
                (s32)((((((1.0F - scav) - trav) *
                          (f32)mobj->sub.unk0E) +
                         (f32)mobj->sub.unk0A) / scav) * 4.0F) : 0;
        }
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_RENDER_TILE_SIZE;
        ndsRendererAdapterNativeMaterialTileSize(
            NDS_RENDERER_ADAPTER_G_TX_RENDERTILE, uls, ult,
            (((s32)mobj->sub.unk0C - 1) << 2) + uls,
            (((s32)mobj->sub.unk0E - 1) << 2) + ult,
            &out->render_tile_size_w0, &out->render_tile_size_w1);
        out->command_count++;
    }
    if ((flags & 0x40u) != 0u)
    {
        uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
            (s32)(((((f32)mobj->sub.unk38 * scrollu) +
                     (f32)mobj->sub.unk0A) / scau) * 4.0F) : 0;
        ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
            (s32)((((((1.0F - scav) - scrollv) *
                      (f32)mobj->sub.unk3A) +
                     (f32)mobj->sub.unk0A) / scav) * 4.0F) : 0;
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_SCROLL_TILE_SIZE;
        ndsRendererAdapterNativeMaterialTileSize(
            1u, uls, ult,
            (((s32)mobj->sub.unk38 - 1) << 2) + uls,
            (((s32)mobj->sub.unk3A - 1) << 2) + ult,
            &out->scroll_tile_size_w0, &out->scroll_tile_size_w1);
        out->command_count++;
    }
    if ((flags & MOBJ_FLAG_TEXTURE) != 0u)
    {
        if (mobj->sub.unk10 == 2)
        {
            s = (ABSF(scau) > (1.0F / 65535.0F)) ?
                (s32)(((f32)mobj->sub.unk0C * 64.0F) / scau) : 0;
            t = (ABSF(scav) > (1.0F / 65535.0F)) ?
                (s32)(((f32)mobj->sub.unk0E * 64.0F) / scav) : 0;
        }
        else
        {
            s = ((mobj->sub.unk08 != 0) &&
                 (ABSF(scau) > (1.0F / 65535.0F))) ?
                (s32)((2097152.0F / (f32)mobj->sub.unk08) / scau) : 0;
            t = ((mobj->sub.unk08 != 0) &&
                 (ABSF(scav) > (1.0F / 65535.0F))) ?
                (s32)((2097152.0F / (f32)mobj->sub.unk08) / scav) : 0;
        }
        if (s > 0xffff) { s = 0xffff; }
        if (t > 0xffff) { t = 0xffff; }
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_TEXTURE;
        out->texture_w0 =
            (NDS_FIGHTER_DL_OP_TEXTURE << 24) |
            (NDS_RENDERER_ADAPTER_G_TX_RENDERTILE << 8) |
            (NDS_RENDERER_ADAPTER_G_ON << 1);
        out->texture_w1 =
            (((u32)s & 0xffffu) << 16) | ((u32)t & 0xffffu);
        out->command_count++;
    }
    if (out_curr != NULL)
    {
        *out_curr = texture_id_curr;
    }
    if (out_next != NULL)
    {
        *out_next = texture_id_next;
    }
    if (advance_texture_ids != FALSE)
    {
        mobj->texture_id_curr = texture_id_curr;
        mobj->texture_id_next = texture_id_next;
    }
    return TRUE;
}

#if NDS_TICK_HUD
/* CYCLE 98 -- is the fighter material snapshot re-deriving a constant?
 *
 * BuildNativeMaterialSnapshot has no cache, so unlike the owner validate there
 * is no hit/miss to count. The equivalent question is content invariance: hash
 * the snapshot it just produced and compare it with the last snapshot produced
 * for that same MObj. `Same` is then the number of calls that recomputed a
 * value they had already computed, which is exactly the size of the deletion.
 *
 * Direct-mapped and O(1) on purpose. A linear table would search up to 128
 * entries per call at tens of calls per fighter per frame -- instrument cost
 * inside the very span being priced. Collisions are not hidden: a slot holding
 * a different key is counted as Evict, so a thrashing table reads as thrash
 * rather than as "every material varies".
 *
 * Charter 3.12: keyed on MObj pointers, which are arena addresses valid only
 * within a scene. The census only has to survive the match it measures, and P1
 * boots straight into one. */
#define NDS_FTR_PRE_MAT_TABLE_SIZE 256u

static const MObj *sNdsFtrPreMatKey[NDS_FTR_PRE_MAT_TABLE_SIZE];
static u32 sNdsFtrPreMatHash[NDS_FTR_PRE_MAT_TABLE_SIZE];

static void ndsFtrPreMaterialCensus(const MObj *mobj,
                                    const NDSRendererNativeMaterial *out)
{
    const u32 *words = (const u32 *)(const void *)out;
    u32 hash = 2166136261u;
    u32 i;
    u32 index;

    /* The whole struct is bzero'd at the top of the snapshot builder, so any
     * padding is deterministic and a word-wise hash is stable. */
    for (i = 0u; i < (u32)(sizeof(*out) / sizeof(u32)); i++)
    {
        hash ^= words[i];
        hash *= 16777619u;
    }
    /* Multiplicative rather than a low-bit mask: MObjs come out of the taskman
     * arena at a fixed stride, so their low address bits are the least
     * distinguishing ones they have. */
    index = ((u32)(uintptr_t)mobj * 2654435761u) >> 24;
    gNdsFtrPreMatCalls++;
    if (sNdsFtrPreMatKey[index] == mobj)
    {
        if (sNdsFtrPreMatHash[index] == hash)
        {
            gNdsFtrPreMatSame++;
        }
        else
        {
            gNdsFtrPreMatVariant++;
            sNdsFtrPreMatHash[index] = hash;
        }
        return;
    }
    if (sNdsFtrPreMatKey[index] != NULL)
    {
        gNdsFtrPreMatEvict++;
    }
    else
    {
        gNdsFtrPreMatNew++;
    }
    sNdsFtrPreMatKey[index] = mobj;
    sNdsFtrPreMatHash[index] = hash;
}
#endif

static sb32 ndsRendererAdapterBuildNativeMaterial(
    MObj *mobj, NDSRendererNativeMaterial *out)
{
#if NDS_TICK_HUD
    sb32 built = ndsRendererAdapterBuildNativeMaterialSnapshot(
        mobj, out, TRUE, NULL, NULL);

    if (built != FALSE)
    {
        ndsFtrPreMaterialCensus(mobj, out);
    }
    return built;
#else
    return ndsRendererAdapterBuildNativeMaterialSnapshot(
        mobj, out, TRUE, NULL, NULL);
#endif
}

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
static GObj *ndsRendererAdapterNativeStageSegmentGObj(u32 segment_index)
{
    switch (segment_index)
    {
    case 0u: return gGRCommonLayerGObjs[0];
    case 1u: return gGRCommonStruct.pupupu.map_gobj[0];
    case 2u: return gGRCommonStruct.pupupu.map_gobj[1];
    case 3u: return gGRCommonStruct.pupupu.map_gobj[2];
    case 4u: return gGRCommonLayerGObjs[1];
    case 5u: return gGRCommonLayerGObjs[2];
    case 6u: return gGRCommonStruct.pupupu.map_gobj[3];
    case 7u: return gGRCommonLayerGObjs[3];
    default: return NULL;
    }
}

static u32 ndsRendererAdapterNativeStageSegmentLink(u32 segment_index)
{
    static const u8 links[NDS_RENDERER_ADAPTER_STAGE_SEGMENT_COUNT] = {
        4u, 4u, 4u, 4u, 6u, 13u, 16u, 17u
    };
    return (segment_index < ARRAY_COUNT(links)) ? links[segment_index] : 0xffu;
}

static sb32 ndsRendererAdapterNativeStageProcMatches(
    u32 segment_index, GObj *gobj)
{
    if ((gobj == NULL) || (gobj->proc_display == NULL))
    {
        return FALSE;
    }
    switch (segment_index)
    {
    case 0u:
    case 1u:
    case 2u:
    case 3u:
        return (gobj->proc_display == grDisplayLayer0PriProcDisplay) ?
            TRUE : FALSE;
    case 4u:
        return (gobj->proc_display == grDisplayLayer1PriProcDisplay) ?
            TRUE : FALSE;
    case 5u:
        return (gobj->proc_display == grDisplayLayer2PriProcDisplay) ?
            TRUE : FALSE;
    case 6u:
    case 7u:
        return (gobj->proc_display == grDisplayLayer3PriProcDisplay) ?
            TRUE : FALSE;
    default:
        return FALSE;
    }
}

static sb32 ndsRendererAdapterNativeStageGObjLinked(GObj *target, u32 link)
{
    GObj *gobj;
    u32 guard = 0u;

    if ((target == NULL) || (link >= GC_COMMON_MAX_DLLINKS))
    {
        return FALSE;
    }
    for (gobj = gGCCommonDLLinks[link];
         (gobj != NULL) && (guard < 256u);
         gobj = gobj->dl_link_next, guard++)
    {
        if (gobj == target)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static sb32 ndsRendererAdapterNativeStageLayer0OrderMatches(
    GObj *const *segments)
{
    GObj *gobj;
    u32 next = 0u;
    u32 guard = 0u;

    if (segments == NULL)
    {
        return FALSE;
    }
    for (gobj = gGCCommonDLLinks[4];
         (gobj != NULL) && (guard < 256u) && (next < 4u);
         gobj = gobj->dl_link_next, guard++)
    {
        if (gobj == segments[next])
        {
            next++;
        }
    }
    return (next == 4u) ? TRUE : FALSE;
}

static sb32 ndsRendererAdapterNativeStageTransformFlags(
    const DObj *dobj, u16 *out_flags)
{
    if ((dobj == NULL) || (out_flags == NULL))
    {
        return FALSE;
    }
    if ((dobj->xobjs_num == 1u) && (dobj->xobjs[0] != NULL) &&
        (dobj->xobjs[0]->kind == nGCMatrixKindTraRotRpyRSca))
    {
        *out_flags = 0u;
        return TRUE;
    }
    if ((dobj->xobjs_num == 2u) && (dobj->xobjs[0] != NULL) &&
        (dobj->xobjs[1] != NULL) &&
        (dobj->xobjs[0]->kind == nGCMatrixKindTra))
    {
        if (dobj->xobjs[1]->kind == nGCMatrixKind48)
        {
            *out_flags = 2u;
            return TRUE;
        }
        if (dobj->xobjs[1]->kind == nGCMatrixKind46)
        {
            *out_flags = 4u;
            return TRUE;
        }
    }
    return FALSE;
}

static sb32 ndsRendererAdapterCollectNativeStageDObjs(
    DObj *dobj, u32 owner, u16 parent_index, u8 depth,
    NDSRendererAdapterNativeStageWorkspace *workspace)
{
    for (; dobj != NULL; dobj = dobj->sib_next)
    {
        NDSRendererNativeStageDObj *live;
        u32 index;
        u16 transform_flags;

        if ((workspace->dobj_count >= NDS_RENDERER_ADAPTER_STAGE_DOBJ_COUNT) ||
            (depth > 31u) || ((dobj->flags & DOBJ_FLAG_HIDDEN) != 0u) ||
            (ndsRendererAdapterNativeStageTransformFlags(
                 dobj, &transform_flags) == FALSE))
        {
            return FALSE;
        }
        index = workspace->dobj_count++;
        workspace->dobjs[index] = dobj;
        live = &workspace->live_dobjs[index];
        live->identity = dobj;
        live->parent_index = parent_index;
        live->transform_flags = transform_flags;
        live->owner = (u8)owner;
        live->depth = depth;
        if (dobj->dv != NULL)
        {
            u32 binding = workspace->binding_count++;
            if ((binding >= NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT) ||
                ((dobj->flags & DOBJ_FLAG_NOTEXTURE) != 0u))
            {
                return FALSE;
            }
            live->binding_index = (u16)binding;
            workspace->binding_dobjs[binding] = dobj;
            workspace->binding_display_lists[binding] = dobj->dv;
        }
        else
        {
            live->binding_index = 0xffffu;
        }
        if ((dobj->child != NULL) &&
            (ndsRendererAdapterCollectNativeStageDObjs(
                 dobj->child, owner, (u16)index, (u8)(depth + 1u),
                 workspace) == FALSE))
        {
            return FALSE;
        }
    }
    return TRUE;
}

static u32 ndsRendererAdapterNativeStageStampValue(u32 stamp, uintptr_t value)
{
    stamp ^= (u32)value;
    stamp *= 16777619u;
    stamp ^= stamp >> 16;
    return stamp;
}

static sb32 ndsRendererAdapterBuildNativeStageTopologyStamp(
    NDSRendererAdapterNativeStageWorkspace *workspace,
    u32 generation,
    u32 *out_stamp)
{
    u32 stamp = 2166136261u;
    u32 i;

    if ((workspace == NULL) || (out_stamp == NULL) || (generation == 0u) ||
        (workspace->dobj_count != NDS_RENDERER_ADAPTER_STAGE_DOBJ_COUNT) ||
        (workspace->binding_count !=
         NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT))
    {
        return FALSE;
    }
    stamp = ndsRendererAdapterNativeStageStampValue(stamp, generation);
    for (i = 0u; i < NDS_RENDERER_ADAPTER_STAGE_ASSET_COUNT; i++)
    {
        NDSRelocLoadedFile *loaded = workspace->loaded[i];

        if ((loaded == NULL) || (loaded->data == NULL) ||
            (loaded->owner_generation != generation))
        {
            return FALSE;
        }
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)loaded);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)loaded->data);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, loaded->asset_id);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, loaded->data_size);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, loaded->owner_generation);
    }
    for (i = 0u; i < NDS_RENDERER_ADAPTER_STAGE_SEGMENT_COUNT; i++)
    {
        GObj *gobj = ndsRendererAdapterNativeStageSegmentGObj(i);

        if ((gobj == NULL) || (gobj != workspace->segments[i]) ||
            (DObjGetStruct(gobj) == NULL) ||
            ((gobj->flags & GOBJ_FLAG_HIDDEN) != 0u) ||
            (gobj->dl_link_id != ndsRendererAdapterNativeStageSegmentLink(i)) ||
            (ndsRendererAdapterNativeStageProcMatches(i, gobj) == FALSE) ||
            (ndsRendererAdapterNativeStageGObjLinked(
                 gobj, gobj->dl_link_id) == FALSE))
        {
            return FALSE;
        }
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)gobj);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)DObjGetStruct(gobj));
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, gobj->flags);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, gobj->dl_link_id);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)gobj->proc_display);
    }
    if (ndsRendererAdapterNativeStageLayer0OrderMatches(
            workspace->segments) == FALSE)
    {
        return FALSE;
    }
    for (i = 0u; i < NDS_RENDERER_ADAPTER_STAGE_DOBJ_COUNT; i++)
    {
        DObj *dobj = workspace->dobjs[i];
        NDSRendererNativeStageDObj *live = &workspace->live_dobjs[i];
        u16 transform_flags;
        u32 xobj_index;

        if ((dobj == NULL) || (live->identity != dobj) ||
            (ndsRendererAdapterNativeStageTransformFlags(
                 dobj, &transform_flags) == FALSE) ||
            (transform_flags != live->transform_flags))
        {
            return FALSE;
        }
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->parent_gobj);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->parent);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->child);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->sib_next);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->sib_prev);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->dv);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->mobj);
        stamp = ndsRendererAdapterNativeStageStampValue(stamp, dobj->flags);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, dobj->xobjs_num);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, live->parent_index);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, live->binding_index);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, live->owner);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, live->depth);
        for (xobj_index = 0u; xobj_index < dobj->xobjs_num; xobj_index++)
        {
            XObj *xobj = dobj->xobjs[xobj_index];

            if (xobj == NULL)
            {
                return FALSE;
            }
            stamp = ndsRendererAdapterNativeStageStampValue(
                stamp, (uintptr_t)xobj);
            stamp = ndsRendererAdapterNativeStageStampValue(
                stamp, xobj->kind);
        }
    }
    for (i = 0u; i < NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT; i++)
    {
        if ((workspace->binding_dobjs[i] == NULL) ||
            (workspace->binding_display_lists[i] == NULL) ||
            (workspace->binding_dobjs[i]->dv !=
             workspace->binding_display_lists[i]))
        {
            return FALSE;
        }
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)workspace->binding_dobjs[i]);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)workspace->binding_display_lists[i]);
    }
    *out_stamp = (stamp != 0u) ? stamp : 1u;
    return TRUE;
}

#if NDS_TASK44_STAGE_STEADY
/* Task 44 item 3: the cheap half of stage admission.
 *
 * The asset-mutation generation proves the four reloc payloads have not been
 * replaced or unloaded; it says nothing about the scene graph that hangs off
 * them. These eight checks cover the graph mutations the stage owner must fail
 * closed on — a segment GObj swapped, hidden, relinked, or given a different
 * display proc — and every one of them is a direct global load. What steady
 * state no longer pays for is the O(n) work: the four loaded-file table scans,
 * the eight DL-link list walks, the two layer-0 order walks, and the 57-DObj /
 * 42-binding stamp rebuild with its per-DObj transform-flag derivation. Any
 * failure here drops through to exactly that full validation. */
static sb32 ndsRendererAdapterNativeStageSegmentsUnchanged(
    const NDSRendererAdapterNativeStageWorkspace *workspace)
{
    u32 i;

    for (i = 0u; i < NDS_RENDERER_ADAPTER_STAGE_SEGMENT_COUNT; i++)
    {
        GObj *gobj = ndsRendererAdapterNativeStageSegmentGObj(i);

        if ((gobj == NULL) || (gobj != workspace->segments[i]) ||
            ((gobj->flags & GOBJ_FLAG_HIDDEN) != 0u) ||
            (gobj->dl_link_id !=
             ndsRendererAdapterNativeStageSegmentLink(i)) ||
            (DObjGetStruct(gobj) != workspace->task44_segment_roots[i]) ||
            (ndsRendererAdapterNativeStageProcMatches(i, gobj) == FALSE))
        {
            return FALSE;
        }
    }
    return TRUE;
}
#endif

static sb32 ndsRendererAdapterCollectNativeStageTopology(
    NDSRendererAdapterNativeStageWorkspace *workspace)
{
    static const u8 dobj_counts[NDS_RENDERER_ADAPTER_STAGE_SEGMENT_COUNT] = {
        21u, 3u, 6u, 7u, 2u, 4u, 10u, 4u
    };
    u32 i;

    for (i = 0u; i < NDS_RENDERER_ADAPTER_STAGE_SEGMENT_COUNT; i++)
    {
        u32 first_dobj = workspace->dobj_count;
        GObj *gobj = ndsRendererAdapterNativeStageSegmentGObj(i);

        workspace->segments[i] = gobj;
        if ((gobj == NULL) || ((gobj->flags & GOBJ_FLAG_HIDDEN) != 0u) ||
            (gobj->dl_link_id != ndsRendererAdapterNativeStageSegmentLink(i)) ||
            (ndsRendererAdapterNativeStageProcMatches(i, gobj) == FALSE) ||
            (ndsRendererAdapterNativeStageGObjLinked(
                 gobj, gobj->dl_link_id) == FALSE) ||
            (ndsRendererAdapterCollectNativeStageDObjs(
                 DObjGetStruct(gobj),
                 (i == 0u) ? 0u : (i == 1u) ? 4u :
                 (i == 2u) ? 5u : (i == 3u) ? 6u :
                 (i == 4u) ? 1u : (i == 5u) ? 2u :
                 (i == 6u) ? 7u : 3u,
                 0xffffu, 0u, workspace) == FALSE) ||
            ((workspace->dobj_count - first_dobj) != dobj_counts[i]))
        {
            return FALSE;
        }
#if NDS_TASK44_STAGE_STEADY
        workspace->task44_segment_roots[i] = DObjGetStruct(gobj);
#endif
    }
    return ((workspace->dobj_count ==
             NDS_RENDERER_ADAPTER_STAGE_DOBJ_COUNT) &&
            (workspace->binding_count ==
             NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT) &&
            (ndsRendererAdapterNativeStageLayer0OrderMatches(
                 workspace->segments) != FALSE)) ? TRUE : FALSE;
}

static sb32 ndsRendererAdapterPrepareNativeStageBindingMatrix(
    CObj *cobj, NDSRendererAdapterNativeStageWorkspace *workspace,
    u32 binding_index)
{
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    const NDSRendererMatrix20p12 *projection_ptr;
    const NDSRendererMatrix20p12 *modelview_ptr;

    ndsRendererAdapterPrepareInitialMatrices(
        workspace->binding_dobjs[binding_index], cobj, TRUE,
        &projection, &projection_ptr, &modelview, &modelview_ptr);
    if ((projection_ptr == NULL) || (modelview_ptr == NULL))
    {
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
        gNdsRendererTask36AdapterRejectReason = 52u;
#endif
        return FALSE;
    }
#if !NDS_TASK36_HW_COMPOSE
    if ((binding_index != 0u) &&
        (memcmp(&workspace->projection, projection_ptr,
                sizeof(workspace->projection)) != 0))
    {
        return FALSE;
    }
#endif
    if (ndsRendererAdapterComposeNativeRootMatrix(
            modelview_ptr, projection_ptr,
            &workspace->binding_composed[binding_index]) == FALSE)
    {
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
        gNdsRendererTask36AdapterRejectReason = 54u;
#endif
        return FALSE;
    }
    if (binding_index == 0u)
    {
        ndsRendererMatrixCopy20p12(&workspace->projection, projection_ptr);
    }
    return TRUE;
}

static sb32 ndsRendererAdapterPrepareNativeStageMatrices(
    CObj *cobj, NDSRendererAdapterNativeStageWorkspace *workspace)
{
    u32 binding_index;

#if NDS_TASK36_HW_COMPOSE
    {
        if (ndsRendererAdapterBuildTask36StageCameraMatrices(
                cobj, &workspace->projection,
                &workspace->camera_modelview) == FALSE)
        {
#if NDS_RENDERER_PROFILE_LEVEL == 1
            gNdsRendererTask36AdapterRejectReason = 51u;
#endif
            return FALSE;
        }
    }
#if NDS_RENDERER_M3_PHASE0_PROFILE
    gNdsRendererTask36ObservedDynamicMaskLo = 0u;
    gNdsRendererTask36ObservedDynamicMaskHi = 0u;
    for (binding_index = 0u;
         binding_index < NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT;
         binding_index++)
    {
        NDSRendererMatrix20p12 current_world;

        if ((ndsRendererAdapterBuildDObjWorldMatrixUncached(
                 workspace->binding_dobjs[binding_index],
                 &current_world) == FALSE) ||
            (memcmp(&current_world, &workspace->binding_world[binding_index],
                    sizeof(current_world)) != 0))
        {
            if (binding_index < 32u)
            {
                gNdsRendererTask36ObservedDynamicMaskLo |= 1u << binding_index;
            }
            else
            {
                gNdsRendererTask36ObservedDynamicMaskHi |=
                    1u << (binding_index - 32u);
            }
            if ((workspace->task36_runtime_rigid_mask &
                 ((u64)1u << binding_index)) != 0u)
            {
                gNdsRendererTask36RigidConstancyMismatchCount++;
                return FALSE;
            }
        }
    }
#endif
#endif

#if NDS_TASK36_HW_COMPOSE && NDS_TASK44_STAGE_STEADY
    /* Task 44 item 4: steady state composes exactly the 16 dynamic bindings.
     * The dense list is trusted only while the runtime rigid mask still equals
     * the captured one — a rigid-constancy fallback drops the mask to 0, which
     * makes every binding dynamic and must take the full scan. */
    if ((workspace->task44_binding_lists_valid != FALSE) &&
        (workspace->task36_runtime_rigid_mask ==
         NDS_RENDERER_TASK36_RIGID_BINDING_MASK))
    {
        u32 dynamic_slot;
#if NDS_R2_STAGE_VIEWPROJ
        /* R2-02 E7. Composing the 16 dynamic bindings the long way cost 54,901
         * ticks/frame, 44.6% of the stage preflight, and almost none of it was
         * arithmetic. Per binding the old path ran the camera cache lookup and
         * three 64-byte matrix copies -- and MTXCOPY is a `bl memcpy` here, see
         * the Task 86 note on NDSRendererMatrix20p12 -- to rebuild operands
         * that are identical for all 16.
         *
         * This is exact, not an approximation. For the battle camera
         * ndsRendererAdapterBuildCameraMatrices leaves modelview_valid FALSE
         * and returns projection = MtxMul(lookat, persp), so the old compose
         * was world x (lookat x persp) with the modelview a plain copy of the
         * world -- one multiply, never two.
         * ndsRendererAdapterBuildTask36StageCameraMatrices derives
         * camera_modelview and projection from the same syMatrixLookAtReflect
         * and syMatrixPerspFast calls on the same CObj, so view_projection
         * reproduces that product bit-for-bit rather than reassociating it.
         * Verified against the pre-E7 arm: binding_composed[] identical across
         * all 42 bindings at frames 260/420/500/700/1100/1700, spanning the
         * camera's full range of motion. No fidelity budget is spent. */
        NDSRendererMatrix20p12 view_projection;

        ndsRendererMtxMul20p12(&workspace->camera_modelview,
                               &workspace->projection, &view_projection);
        for (dynamic_slot = 0u;
             dynamic_slot < workspace->task44_dynamic_binding_count;
             dynamic_slot++)
        {
            u32 vp_binding = workspace->task44_dynamic_bindings[dynamic_slot];
            DObj *vp_dobj = workspace->binding_dobjs[vp_binding];
            NDSRendererMatrix20p12 vp_world;

            /* An mvp-recalc (0x47 or the kind-44 billboard) rewrites the pair
             * after composition, so those bindings keep the exact original
             * path. */
            if ((vp_dobj == NULL) ||
                (ndsRendererAdapterDirectMvpRecalcKind(vp_dobj) != 0u) ||
                /* BUGS.md row 1, 2026-08-13. These are the stage's DYNAMIC
                 * bindings, and several of them are animated (Whispy eyes and
                 * mouth plus the flower actors). Slice 44 used to pass
                 * allow_stale=TRUE on seven of eight frames, before the source
                 * key was even checked. The source DObj therefore advanced at
                 * 30 Hz while its cached world matrix could remain frozen for
                 * eight presented frames. FALSE does not force a rebuild: the
                 * persistent cache still reuses an unchanged source key and
                 * parent generation. It only forbids blind stale reuse. Keep
                 * the stride on the separately-proven rigid-binding guard, not
                 * on dynamic world transforms. */
                (ndsRendererAdapterBuildPersistentStageWorldMatrix(
                     vp_dobj, &vp_world, FALSE) == FALSE))
            {
                if (ndsRendererAdapterPrepareNativeStageBindingMatrix(
                        cobj, workspace, vp_binding) == FALSE)
                {
                    return FALSE;
                }
                continue;
            }
            ndsRendererMtxMul20p12(&vp_world, &view_projection,
                                   &workspace->binding_composed[vp_binding]);
        }
        return TRUE;
#else

        for (dynamic_slot = 0u;
             dynamic_slot < workspace->task44_dynamic_binding_count;
             dynamic_slot++)
        {
            if (ndsRendererAdapterPrepareNativeStageBindingMatrix(
                    cobj, workspace,
                    workspace->task44_dynamic_bindings[dynamic_slot]) == FALSE)
            {
                return FALSE;
            }
        }
        return TRUE;
#endif
    }
#endif
    for (binding_index = 0u;
         binding_index < NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT;
         binding_index++)
    {
#if NDS_TASK36_HW_COMPOSE
        if ((workspace->task36_runtime_rigid_mask &
             ((u64)1u << binding_index)) != 0u)
        {
            continue;
        }
#endif
        if (ndsRendererAdapterPrepareNativeStageBindingMatrix(
                cobj, workspace, binding_index) == FALSE)
        {
            return FALSE;
        }
    }
    return TRUE;
}

#if NDS_TASK36_HW_COMPOSE
static sb32 ndsRendererAdapterCaptureTask36StageWorld(
    NDSRendererAdapterNativeStageWorkspace *workspace)
{
    u32 binding_index;

    for (binding_index = 0u;
         binding_index < NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT;
         binding_index++)
    {
        if (ndsRendererAdapterBuildDObjWorldMatrixUncached(
                workspace->binding_dobjs[binding_index],
                &workspace->binding_world[binding_index]) == FALSE)
        {
            return FALSE;
        }
        if (((NDS_RENDERER_TASK36_RIGID_BINDING_MASK &
              ((u64)1u << binding_index)) != 0u) &&
            (ndsRendererAdapterCaptureStageWorldSourceKey(
                 workspace->binding_dobjs[binding_index],
                 &workspace->task36_rigid_source_keys[binding_index]) ==
             FALSE))
        {
            return FALSE;
        }
    }
    workspace->task36_runtime_rigid_mask =
        NDS_RENDERER_TASK36_RIGID_BINDING_MASK;
#if NDS_TASK44_STAGE_STEADY
    workspace->task44_rigid_binding_count = 0u;
    workspace->task44_dynamic_binding_count = 0u;
    for (binding_index = 0u;
         binding_index < NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT;
         binding_index++)
    {
        if ((NDS_RENDERER_TASK36_RIGID_BINDING_MASK &
             ((u64)1u << binding_index)) != 0u)
        {
            workspace->task44_rigid_bindings[
                workspace->task44_rigid_binding_count++] = (u8)binding_index;
        }
        else
        {
            workspace->task44_dynamic_bindings[
                workspace->task44_dynamic_binding_count++] = (u8)binding_index;
        }
    }
    workspace->task44_binding_lists_valid = TRUE;
#endif
    return TRUE;
}

static void ndsRendererAdapterValidateTask36StageWorld(
    NDSRendererAdapterNativeStageWorkspace *workspace)
{
    u32 binding_index;
#if NDS_TASK44_STAGE_STEADY
    u32 rigid_slot;
#endif

#if NDS_R2_STAGE_VALIDATE_STRIDE
    /* Slice 44. Demotion is one-way within a topology: a binding that stopped
     * being rigid does not become rigid again until capture re-arms the mask at
     * ndsRendererAdapterCaptureTask36StageWorld. Before the stride the mask was
     * rebuilt from the constant here every frame and cleared again by the sweep,
     * which was equivalent only because the sweep was complete. With a partial
     * sweep, re-arming would resurrect a binding this frame's slice did not
     * look at. */
    if (workspace->task36_runtime_rigid_mask == 0u)
    {
        return;
    }
#endif
    workspace->task36_runtime_rigid_mask =
        NDS_RENDERER_TASK36_RIGID_BINDING_MASK;
#if NDS_TASK44_STAGE_STEADY
    /* Task 44 item 4: walk the 26 rigid bindings directly. The list is only
     * consulted when capture built it for this topology; otherwise fall back
     * to the mask scan below so a torn workspace can never skip validation. */
    if (workspace->task44_binding_lists_valid != FALSE)
    {
        for (rigid_slot = 0u;
             rigid_slot < workspace->task44_rigid_binding_count;
             rigid_slot++)
        {
#if NDS_R2_STAGE_VALIDATE_STRIDE
            if ((rigid_slot % NDS_R2_STAGE_VALIDATE_STRIDE) !=
                workspace->slice44_validate_cursor)
            {
                gNdsR2Slice44RigidSkips++;
                continue;
            }
            gNdsR2Slice44RigidChecks++;
#endif
            binding_index = workspace->task44_rigid_bindings[rigid_slot];
            if (ndsRendererAdapterStageWorldSourceKeyMatches(
                    workspace->binding_dobjs[binding_index],
                    &workspace->task36_rigid_source_keys[binding_index]) ==
                FALSE)
            {
                workspace->task36_runtime_rigid_mask = 0u;
#if NDS_RENDERER_PROFILE_LEVEL == 1
                gNdsRendererTask36RigidConstancyMismatchCount++;
#endif
                return;
            }
        }
        return;
    }
#endif
    for (binding_index = 0u;
         binding_index < NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT;
         binding_index++)
    {
        if ((NDS_RENDERER_TASK36_RIGID_BINDING_MASK &
             ((u64)1u << binding_index)) == 0u)
        {
            continue;
        }
        if (ndsRendererAdapterStageWorldSourceKeyMatches(
                workspace->binding_dobjs[binding_index],
                &workspace->task36_rigid_source_keys[binding_index]) ==
            FALSE)
        {
            workspace->task36_runtime_rigid_mask = 0u;
#if NDS_RENDERER_PROFILE_LEVEL == 1
            gNdsRendererTask36RigidConstancyMismatchCount++;
#endif
            return;
        }
    }
}
#endif

static sb32 ndsRendererAdapterPrepareNativeStageMaterials(
    NDSRendererAdapterNativeStageWorkspace *workspace)
{
    static const u8 bindings[NDS_RENDERER_ADAPTER_STAGE_MATERIAL_COUNT] = {
        20u, 22u, 31u, 32u
    };
    static const u16 flags[NDS_RENDERER_ADAPTER_STAGE_MATERIAL_COUNT] = {
        0x0001u, 0x0001u, 0x006bu, 0x006bu
    };
    u32 i;

    for (i = 0u; i < NDS_RENDERER_ADAPTER_STAGE_MATERIAL_COUNT; i++)
    {
        MObj *mobj = workspace->binding_dobjs[bindings[i]]->mobj;

        if ((mobj == NULL) ||
            (ndsRendererAdapterMaterialFlags(mobj) != flags[i]) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 mobj, &workspace->materials[i], FALSE,
                 &workspace->material_curr[i],
                 &workspace->material_next[i]) == FALSE))
        {
#if NDS_R2_SECOND_ENTRY_DIAG
            /* THIS is the question, not "are the pointers stale". The four
             * bindings are addressed by the fixed constants {20,22,31,32} and
             * each is checked against a fixed expected material flag word. If
             * the second entry builds the DObj tree in a different ORDER, the
             * indices resolve to other objects, the flag word does not match,
             * and this returns FALSE -- which rejects the whole native stage
             * owner and drops the stage onto the generic path. That would show
             * up as "the stage is drawn wrong" without a single stale pointer
             * anywhere, which is exactly the state four other hypotheses have
             * now failed to explain. Latch the FIRST failure only. */
            gNdsR2StageMaterialRejectCount++;
            if (gNdsR2StageMaterialRejectIndex == 0xFFFFFFFFu)
            {
                gNdsR2StageMaterialRejectIndex = i;
                gNdsR2StageMaterialRejectBinding = bindings[i];
                gNdsR2StageMaterialRejectDObj =
                    (u32)(uintptr_t)workspace->binding_dobjs[bindings[i]];
                gNdsR2StageMaterialRejectMObj = (u32)(uintptr_t)mobj;
                gNdsR2StageMaterialRejectFlagsWant = flags[i];
                gNdsR2StageMaterialRejectFlagsGot =
                    ndsRendererAdapterMaterialFlags(mobj);
                gNdsR2StageMaterialRejectHeapGen = gNdsTaskmanHeapGeneration;
            }
#endif
            return FALSE;
        }
        workspace->material_mobjs[i] = mobj;
    }
    return TRUE;
}

static void ndsRendererAdapterCommitNativeStageMaterials(
    NDSRendererAdapterNativeStageWorkspace *workspace, u32 segment_index)
{
    u32 first = 0u;
    u32 count = 0u;
    u32 i;

    if (segment_index == 1u) { first = 0u; count = 1u; }
    else if (segment_index == 2u) { first = 1u; count = 1u; }
    else if (segment_index == 5u) { first = 2u; count = 2u; }
    for (i = first; i < first + count; i++)
    {
        workspace->material_mobjs[i]->texture_id_curr =
            workspace->material_curr[i];
        workspace->material_mobjs[i]->texture_id_next =
            workspace->material_next[i];
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererM3MaterialCommitCount++;
#endif
    }
}

#if NDS_TASK103_STAGE_RUN_PHASE
/* Task 103 E2/E4. Defined here rather than in nds_renderer.c because the spans
 * they measure live in this file, and ahead of both users because C needs the
 * declaration first.
 *
 * E2 wraps the segment commit and the material setup ahead of it. E3 then found
 * that path is only 40% of the stage bucket: **236,039 ticks/frame -- 60% of
 * STG and 18% of all frame work -- are inside
 * ndsRendererAdapterPrepareNativeStageOwner, at one call per frame**, which no
 * task had ever profiled. E4's six spans split that function's steady-state
 * body into the steps it actually runs, so the number stops being a function
 * name and becomes a lever. Lab only, default off. */
volatile u32 gNdsTask103CommitTicks;
volatile u32 gNdsTask103CommitCount;
volatile u32 gNdsTask103MaterialTicks;
volatile u32 gNdsTask103PrepAdmitTicks;
volatile u32 gNdsTask103PrepValidateTicks;
volatile u32 gNdsTask103PrepMatrixTicks;
volatile u32 gNdsTask103PrepMaterialTicks;
volatile u32 gNdsTask103PrepConfigTicks;
volatile u32 gNdsTask103PrepOwnerTicks;
volatile u32 gNdsTask103PrepCalls;
#endif

s32 ndsRendererAdapterPrepareNativeStageOwner(void *camera_gobj_ptr)
{
    static const u32 asset_ids[NDS_RENDERER_ADAPTER_STAGE_ASSET_COUNT] = {
        0x67u, 0x68u, 0x98u, 0xffu
    };
    static const u32 asset_sizes[NDS_RENDERER_ADAPTER_STAGE_ASSET_COUNT] = {
        0x2fc0u, 0x43f0u, 0x3700u, 0x00c0u
    };
    NDSRendererAdapterNativeStageWorkspace *workspace =
        &sNdsRendererAdapterNativeStageWorkspace;
    NDSRelocLoadedFile *loaded[NDS_RENDERER_ADAPTER_STAGE_ASSET_COUNT];
    GObj *camera_gobj = camera_gobj_ptr;
    CObj *cobj = (camera_gobj != NULL) ? CObjGetStruct(camera_gobj) : NULL;
    u32 topology_generation = 0u;
    u32 topology_stamp = 0u;
    sb32 topology_cached = FALSE;
    u32 i;
#if NDS_TASK44_STAGE_STEADY
    sb32 steady_admitted = FALSE;
#endif
    /* UNCONDITIONAL. The reject label this feeds calls
     * ndsRendererHardwareAbortBattleStaticTextures, which discards the whole
     * hardware texture cache and leaves it unable to re-arm for the rest of the
     * scene. Which branch got there therefore cannot be a profile-only fact --
     * it is the difference between a next cycle that reads one counter and a
     * next cycle that re-derives six branches. See nds_renderer.c's row 6 note. */
    u32 task36_reject_reason = 1u;
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
    gNdsRendererTask36AdapterRejectReason = 0u;
#endif
#if NDS_TASK103_STAGE_RUN_PHASE
    u32 task103_prep_entry = cpuGetTiming();
    u32 task103_prep_mark;
#endif

    workspace->active = FALSE;
    if (gNdsRendererFastRunMode !=
        NDS_RENDERER_FAST_RUN_NATIVE_COMPLETE_STAGE)
    {
        return FALSE;
    }
    if (cobj == NULL)
    {
        task36_reject_reason = 2u;
        goto reject;
    }
#if NDS_TASK44_STAGE_STEADY
    /* Task 44 item 3: steady-state admission. One generation compare plus the
     * cheap segment guard replaces the four asset lookups and the whole
     * topology stamp rebuild. Everything the fast path would have recomputed
     * (loaded[], asset_bases[], topology_generation, topology_stamp) is
     * already recorded in the workspace and provably unchanged, because every
     * seam that can change it bumps sNdsRelocStageAssetMutation. */
    if ((workspace->topology_valid != FALSE) &&
        (workspace->task44_admission_generation != 0u) &&
        (workspace->task44_admission_generation ==
         sNdsRelocStageAssetMutation) &&
        (ndsRendererAdapterNativeStageSegmentsUnchanged(workspace) != FALSE))
    {
        steady_admitted = TRUE;
        topology_generation = workspace->topology_generation;
        topology_stamp = workspace->topology_stamp;
        topology_cached = TRUE;
#if NDS_R2_SECOND_ENTRY_DIAG
        /* The fast path that reuses binding_dobjs[] wholesale. It consults
         * neither owner_generation nor the heap generation -- only
         * sNdsRelocStageAssetMutation -- so if that fails to move on a second
         * scene entry, last match's DObj pointers are re-admitted intact. The
         * existing counters here are PROFILE_LEVEL==1 only, which the tick-HUD
         * build is not, so this bug class was invisible to every run. */
        gNdsR2StageSteadyAdmitCount++;
#endif
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererTask44SteadyAdmitCount++;
#endif
    }
    if (steady_admitted == FALSE)
#endif
    {
#if NDS_TASK44_STAGE_STEADY && (NDS_RENDERER_PROFILE_LEVEL == 1)
    gNdsRendererTask44RevalidateCount++;
#endif
    for (i = 0u; i < NDS_RENDERER_ADAPTER_STAGE_ASSET_COUNT; i++)
    {
        loaded[i] = ndsRelocFindLoadedFileByAsset(asset_ids[i]);
        if ((loaded[i] == NULL) || (loaded[i]->data == NULL) ||
            (loaded[i]->data_size != asset_sizes[i]) ||
            (loaded[i]->owner_generation == 0u) ||
            ((i != 0u) &&
             (loaded[i]->owner_generation != topology_generation)))
        {
            task36_reject_reason = 3u;
            goto reject;
        }
        topology_generation = loaded[i]->owner_generation;
    }
    if ((workspace->topology_valid != FALSE) &&
        (workspace->topology_generation == topology_generation))
    {
        topology_cached = TRUE;
        for (i = 0u; i < NDS_RENDERER_ADAPTER_STAGE_ASSET_COUNT; i++)
        {
            if (workspace->loaded[i] != loaded[i])
            {
                topology_cached = FALSE;
                break;
            }
        }
        if ((topology_cached != FALSE) &&
            ((ndsRendererAdapterBuildNativeStageTopologyStamp(
                  workspace, topology_generation, &topology_stamp) == FALSE) ||
             (topology_stamp != workspace->topology_stamp)))
        {
            topology_cached = FALSE;
        }
    }
    if (topology_cached == FALSE)
    {
#if NDS_R2_SECOND_ENTRY_DIAG
        /* A full rebuild: binding_dobjs[] is re-collected from the live tree.
         * This is what MUST happen on a second scene entry. */
        gNdsR2StageTopologyRebuildCount++;
#endif
        bzero(workspace, sizeof(*workspace));
#if NDS_R2_STAGE_VALIDATE_STRIDE
        /* Slice 44. binding_dobjs[] is about to be re-collected from the live
         * tree, so every stage world entry keyed on an old DObj address is now
         * meaningless -- and a recycled heap address makes one of them *match*.
         * That was harmless while `validated_frame == frame` forced a rebuild
         * every frame; under the stride a collision would hand back the
         * previous topology's matrix. Free at runtime: this branch is the
         * once-per-scene rebuild that already bzeroes the whole workspace. */
        sNdsRendererAdapterStageWorldCacheCount = 0u;
        memset(sNdsRendererAdapterStageWorldIndex, 0,
               sizeof(sNdsRendererAdapterStageWorldIndex));
#endif
        for (i = 0u; i < NDS_RENDERER_ADAPTER_STAGE_ASSET_COUNT; i++)
        {
            workspace->loaded[i] = loaded[i];
            workspace->frame.asset_bases[i] = loaded[i]->data;
        }
        if ((ndsRendererAdapterCollectNativeStageTopology(workspace) == FALSE) ||
#if NDS_TASK36_HW_COMPOSE
            (ndsRendererAdapterCaptureTask36StageWorld(workspace) == FALSE) ||
#endif
            (ndsRendererAdapterBuildNativeStageTopologyStamp(
                 workspace, topology_generation, &topology_stamp) == FALSE))
        {
            task36_reject_reason = 4u;
            goto reject;
        }
        workspace->topology_generation = topology_generation;
        workspace->topology_stamp = topology_stamp;
        workspace->topology_valid = TRUE;
    }
    else
    {
        for (i = 0u; i < NDS_RENDERER_ADAPTER_STAGE_ASSET_COUNT; i++)
        {
            workspace->frame.asset_bases[i] = loaded[i]->data;
        }
    }
#if NDS_TASK44_STAGE_STEADY
    /* Only a completed full validation may arm the fast path. */
    workspace->task44_admission_generation = sNdsRelocStageAssetMutation;
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererTask44AdmissionGeneration = sNdsRelocStageAssetMutation;
#endif
#endif
    }
#if NDS_TASK103_STAGE_RUN_PHASE
    task103_prep_mark = cpuGetTiming();
    gNdsTask103PrepAdmitTicks += task103_prep_mark - task103_prep_entry;
#endif
#if NDS_TASK36_HW_COMPOSE
#if NDS_R2_STAGE_VALIDATE_STRIDE
    /* Slice 44: one advance per frame, here, because this is the only site the
     * c120 profile shows running exactly once -- 26 rigid checks a frame against
     * a 26-entry list. Both the rigid sweep below and the dynamic chain walk in
     * ndsRendererAdapterPrepareNativeStageMatrices read the cursor, so they stay
     * in phase and every frame does the same amount of work. */
    workspace->slice44_validate_cursor =
        (u8)((workspace->slice44_validate_cursor + 1u) %
             NDS_R2_STAGE_VALIDATE_STRIDE);
#endif
    ndsRendererAdapterValidateTask36StageWorld(workspace);
#endif
#if NDS_TASK103_STAGE_RUN_PHASE
    gNdsTask103PrepValidateTicks += cpuGetTiming() - task103_prep_mark;
    task103_prep_mark = cpuGetTiming();
#endif
    if (ndsRendererAdapterPrepareNativeStageMatrices(cobj, workspace) == FALSE)
    {
        task36_reject_reason = 5u;
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
        /* The matrix path publishes a finer sub-reason, but only when
         * profiling is compiled in; 5u is the honest answer without it. */
        if (gNdsRendererTask36AdapterRejectReason != 0u)
        {
            task36_reject_reason = gNdsRendererTask36AdapterRejectReason;
        }
#endif
        goto reject;
    }
#if NDS_TASK103_STAGE_RUN_PHASE
    gNdsTask103PrepMatrixTicks += cpuGetTiming() - task103_prep_mark;
    task103_prep_mark = cpuGetTiming();
#endif
    if (ndsRendererAdapterPrepareNativeStageMaterials(workspace) == FALSE)
    {
        task36_reject_reason = 7u;
        goto reject;
    }
#if NDS_TASK103_STAGE_RUN_PHASE
    gNdsTask103PrepMaterialTicks += cpuGetTiming() - task103_prep_mark;
    task103_prep_mark = cpuGetTiming();
#endif

    bzero(&workspace->resolver, sizeof(workspace->resolver));
    workspace->resolver.primary_file = workspace->loaded[0];
    workspace->config = (NDSRendererConfig){0};
    workspace->config.max_depth = 8u;
    workspace->config.max_commands = 2048u;
    workspace->config.max_list_commands = 512u;
    workspace->config.texture_data_layout =
        NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    workspace->config.validate_range = ndsRendererAdapterStageValidateRange;
    workspace->config.immutable_command_span =
        ndsRendererAdapterImmutableCommandSpan;
    workspace->config.resolve_branch = ndsFighterDLDrawResolveBranch;
    workspace->config.resolve_data = ndsFighterDLDrawResolveRendererData;
    workspace->config.user = &workspace->resolver;
    workspace->frame.dobjs = workspace->live_dobjs;
    workspace->frame.binding_display_lists =
        workspace->binding_display_lists;
    workspace->frame.projection = &workspace->projection;
#if NDS_TASK36_HW_COMPOSE
    workspace->frame.camera_modelview = &workspace->camera_modelview;
    workspace->frame.binding_world = workspace->binding_world;
    workspace->frame.rigid_binding_mask =
        workspace->task36_runtime_rigid_mask;
#else
    workspace->frame.camera_modelview = NULL;
    workspace->frame.binding_world = NULL;
    workspace->frame.rigid_binding_mask = 0u;
#endif
    workspace->frame.binding_composed = workspace->binding_composed;
    workspace->frame.materials = workspace->materials;
    workspace->frame.config = &workspace->config;
    workspace->frame.topology_generation = workspace->topology_generation;
    workspace->frame.topology_stamp = workspace->topology_stamp;
#if NDS_TASK103_STAGE_RUN_PHASE
    gNdsTask103PrepConfigTicks += cpuGetTiming() - task103_prep_mark;
    task103_prep_mark = cpuGetTiming();
#endif
    if (ndsRendererPrepareNativeStageOwner(
            &workspace->frame, &workspace->stats) == FALSE)
    {
        workspace->topology_valid = FALSE;
        task36_reject_reason = 6u;
        goto reject;
    }
#if NDS_TASK103_STAGE_RUN_PHASE
    gNdsTask103PrepOwnerTicks += cpuGetTiming() - task103_prep_mark;
    gNdsTask103PrepCalls++;
#endif
    workspace->next_segment = 0u;
    workspace->active = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererM3DObjCount = workspace->dobj_count;
    gNdsRendererM3BindingCount = workspace->binding_count;
    gNdsRendererM3MaterialShadowCount =
        NDS_RENDERER_ADAPTER_STAGE_MATERIAL_COUNT;
#endif
    return TRUE;

reject:
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
    gNdsRendererTask36AdapterRejectReason = task36_reject_reason;
#endif
    /* BUGS row 6. Published unconditionally, and the FIRST reason is latched
     * rather than only the last: the abort below is irreversible for the scene,
     * so the branch that caused it is the one worth keeping. A later reject is
     * a consequence of the first one, not independent evidence. */
    gNdsRendererStageOwnerRejectCount++;
    gNdsRendererStageOwnerLastRejectReason = task36_reject_reason;
    if (gNdsRendererStageOwnerFirstRejectReason == 0u)
    {
        gNdsRendererStageOwnerFirstRejectReason = task36_reject_reason;
    }
    workspace->active = FALSE;
    /* A FAILED DISPLAY-GRAPH PREPARE SAYS NOTHING ABOUT TEXTURE RESIDENCY, AND
     * THIS IS WHERE IT USED TO CLAIM OTHERWISE.
     *
     * This block was:
     *
     *     if (gNdsRendererBattleStaticTextureArmCount != 0u)
     *         ndsRendererHardwareAbortBattleStaticTextures();
     *
     * which discards the entire hardware texture cache and clears
     * sNdsRendererBattleStaticTexturePrepared. Both Arm call sites
     * (taskman_seam.c:5251 and :7975) fire only on the Wait->Go TRANSITION, so
     * they never run again inside a match -- and Arm refuses to re-arm without
     * Prepared anyway. One transient frame therefore dropped the 24 pinned
     * statics for the remainder of the match with no path back.
     *
     * Measured on 2026-08-04 (build-row6-v1, two-death match): at frame 427,
     * right after a star KO, abort went 0->1, preparednow 1->0, and
     * staticpin froze at 7144 and never moved again through frame 2174. The
     * scene stayed correctly textured -- the dynamic cache absorbed it -- but
     * frame rate fell from 27.9 to 20.0 for the rest of the match, and the
     * pinned corpus was gone for good.
     *
     * Nothing here justified that. The pinned corpus is uploaded at scene
     * prepare and is untouched by whatever made this frame's display graph
     * unusable; workspace->active = FALSE above is already the whole fallback
     * contract, and the generic path does not read the workspace. So the
     * correct behaviour on a post-arm reject is to degrade for THIS FRAME and
     * leave every texture exactly where it is, which also leaves the next
     * frame free to succeed normally.
     *
     * The event stays loud rather than becoming silent: the reject counters
     * above fire unconditionally, and this one isolates the post-arm case that
     * used to be destructive, so a regression here is still one counter away.
     */
    if (gNdsRendererBattleStaticTextureArmCount != 0u)
    {
        gNdsRendererStageOwnerPostArmRejectCount++;
    }
    return FALSE;
}

s32 __attribute__((section(".itcm")))
ndsRendererAdapterCommitNativeStageDisplay(
    void *display_gobj_ptr, s32 link_id)
{
    NDSRendererAdapterNativeStageWorkspace *workspace =
        &sNdsRendererAdapterNativeStageWorkspace;
    GObj *display_gobj = display_gobj_ptr;
    u32 i;

    if (workspace->active == FALSE)
    {
        return FALSE;
    }
    for (i = 0u; i < NDS_RENDERER_ADAPTER_STAGE_SEGMENT_COUNT; i++)
    {
        if (display_gobj == workspace->segments[i])
        {
            if ((i != workspace->next_segment) ||
                ((u32)link_id != ndsRendererAdapterNativeStageSegmentLink(i)))
            {
                (void)ndsRendererCommitNativeStageSegment(0xffffffffu);
                return TRUE;
            }
#if NDS_TASK29_GX_CENSUS
            ndsRendererTask29GXSetOwner(NDS_RENDERER_PROFILE_OWNER_STAGE);
#endif
#if NDS_TASK103_STAGE_RUN_PHASE
            /* Task 103 E2. The run loop accounts for only 136,519 of the
             * ~370,000 stage bucket, so these two spans say whether the
             * remainder is per-segment scaffolding inside the commit or
             * material setup ahead of it. Wrapped at the call site because
             * ndsRendererCommitNativeStageSegment has several early returns
             * and an in-function span would miss them. */
            {
                u32 task103_mat_start = cpuGetTiming();
                u32 task103_commit_start;
                s32 task103_committed;

                ndsRendererAdapterCommitNativeStageMaterials(workspace, i);
                task103_commit_start = cpuGetTiming();
                task103_committed = ndsRendererCommitNativeStageSegment(i);
                gNdsTask103MaterialTicks +=
                    task103_commit_start - task103_mat_start;
                gNdsTask103CommitTicks +=
                    cpuGetTiming() - task103_commit_start;
                gNdsTask103CommitCount++;
                if (task103_committed == FALSE)
                {
                    ndsRendererFinishNativeStageOwner();
                    workspace->active = FALSE;
                    return FALSE;
                }
            }
#else
            ndsRendererAdapterCommitNativeStageMaterials(workspace, i);
            if (ndsRendererCommitNativeStageSegment(i) == FALSE)
            {
                ndsRendererFinishNativeStageOwner();
                workspace->active = FALSE;
                return FALSE;
            }
#endif
            workspace->next_segment++;
            return TRUE;
        }
    }
    return FALSE;
}

void ndsRendererAdapterFinishNativeStageOwner(void)
{
    NDSRendererAdapterNativeStageWorkspace *workspace =
        &sNdsRendererAdapterNativeStageWorkspace;

    if (workspace->active != FALSE)
    {
        ndsRendererFinishNativeStageOwner();
    }
#if NDS_TASK29_GX_CENSUS
    ndsRendererTask29GXSetOwner(NDS_RENDERER_PROFILE_OWNER_NONE);
#endif
    workspace->active = FALSE;
    workspace->next_segment = 0u;
}
#else
s32 ndsRendererAdapterPrepareNativeStageOwner(void *camera_gobj)
{
    (void)camera_gobj;
    return FALSE;
}

s32 __attribute__((section(".itcm")))
ndsRendererAdapterCommitNativeStageDisplay(
    void *display_gobj, s32 link_id)
{
    (void)display_gobj;
    (void)link_id;
    return FALSE;
}

void ndsRendererAdapterFinishNativeStageOwner(void)
{
}
#endif

/* Always compiled, unlike the validator below: this counts how often the
 * material write walk's capacity guard actually fires, and a guard whose hit
 * count nobody can read is indistinguishable from one that never runs. */
volatile u32 gNdsR2MaterialWalkBoundHits;
/* Runtime toggle so ONE binary can run both arms. Clearing this from a debugger
 * restores the pre-guard unbounded walk with identical code layout, identical
 * inlining and an identical ROM -- which is the only way to attribute the
 * Sudden Death freeze to the guard rather than to the placement change adding
 * the guard caused. Two different builds cannot answer that question; this
 * campaign has repeatedly measured layout moving results on its own (E11
 * removed real work and P95 still rose 15,744). Defaults to 1: the guard is
 * live in every build, and only a deliberate debugger write disables it. */
volatile u32 gNdsR2MaterialWalkBoundEnabled = 1u;

#if NDS_R2_SECOND_ENTRY_DIAG
/* Second-entry chain validator. Default OFF (Makefile NDS_R2_SECOND_ENTRY_DIAG).
 *
 * The material write walk was running off the end of a four-entry array, which
 * means the MObj chain it walks stops being what pass one measured. Bounding
 * the walk contained the damage; it did not say WHEN the list goes bad. This
 * answers that directly instead of inferring it from the overflow site: the
 * chain is recorded twice per DObj -- once before the counting pass, once
 * immediately before the writing pass -- so a list that is sound at the first
 * probe and broken at the second localises the corruption to the counting pass
 * itself, and one broken at both puts it upstream of this function entirely. */
#define NDS_R2_CHAIN_PROBE_MAX 64u

enum {
    NDS_R2_CHAIN_OK = 0u,
    NDS_R2_CHAIN_OVERLONG = 1u,   /* more nodes than any real DObj has */
    NDS_R2_CHAIN_CYCLE = 2u,      /* next pointer revisits a seen node */
    NDS_R2_CHAIN_OUT_OF_ARENA = 3u/* node or next outside the taskman arena */
};

typedef struct NDSR2ChainProbe {
    u32 status;
    u32 nodes;        /* nodes walked before terminating or failing */
    u32 first_bad;    /* address of the offending node, 0 when clean */
    u32 dobj;         /* which DObj owned the chain */
    u32 generation;   /* taskman-heap generation at probe time */
} NDSR2ChainProbe;

volatile NDSR2ChainProbe gNdsR2ChainProbePass1;
volatile NDSR2ChainProbe gNdsR2ChainProbePass2;
/* Latched at the FIRST failure of the run and never overwritten, so a later
 * clean frame cannot erase the evidence. */
volatile NDSR2ChainProbe gNdsR2ChainProbeFirstBad;
volatile u32 gNdsR2ChainProbeFirstBadPass;
volatile u32 gNdsR2ChainProbeInvalidCount;
volatile u32 gNdsR2ChainProbeCount;

static void ndsR2ChainProbe(DObj *dobj, volatile NDSR2ChainProbe *out, u32 pass)
{
    const MObj *seen[NDS_R2_CHAIN_PROBE_MAX];
    const MObj *mobj;
    u32 count = 0u;
    u32 status = NDS_R2_CHAIN_OK;
    u32 first_bad = 0u;

    gNdsR2ChainProbeCount++;
    for (mobj = (dobj != NULL) ? dobj->mobj : NULL; mobj != NULL;
         mobj = mobj->next)
    {
        u32 i;

        if (ndsFighterDLScanRangeInTaskmanArena(mobj, sizeof(*mobj)) == FALSE)
        {
            status = NDS_R2_CHAIN_OUT_OF_ARENA;
            first_bad = (u32)(uintptr_t)mobj;
            break;
        }
        /* O(n^2) against a 64 bound is 4,096 compares worst case, and this is a
         * default-off diagnostic -- a hash would be more code for no answer. */
        for (i = 0u; i < count; i++)
        {
            if (seen[i] == mobj)
            {
                status = NDS_R2_CHAIN_CYCLE;
                first_bad = (u32)(uintptr_t)mobj;
                break;
            }
        }
        if (status != NDS_R2_CHAIN_OK)
        {
            break;
        }
        if (count >= NDS_R2_CHAIN_PROBE_MAX)
        {
            status = NDS_R2_CHAIN_OVERLONG;
            first_bad = (u32)(uintptr_t)mobj;
            break;
        }
        seen[count++] = mobj;
    }
    out->status = status;
    out->nodes = count;
    out->first_bad = first_bad;
    out->dobj = (u32)(uintptr_t)dobj;
    out->generation = gNdsTaskmanHeapGeneration;

    if (status != NDS_R2_CHAIN_OK)
    {
        gNdsR2ChainProbeInvalidCount++;
        if (gNdsR2ChainProbeFirstBadPass == 0u)
        {
            gNdsR2ChainProbeFirstBadPass = pass;
            gNdsR2ChainProbeFirstBad = *out;
        }
    }
}
#endif

/* Cycle 110. The fighter material snapshot is a pure function of `mobj->sub`
 * plus texture_id_curr/next, lfrac and palette_id -- and it never varies. The
 * NDS_TICK_HUD census that has sat in this file since cycle 98 answered it on a
 * 60-second match: 20,100 builds, 20,069 of them byte-identical to the previous
 * snapshot for the same MObj, 31 first-sights, and **zero** variants. So ~761
 * cycles a build, about twelve times a frame, reconstruct a constant --
 * 13,176 ticks/frame of it, 2,124 of which is the single `mobj->sub.flags`
 * load missing cache at 139 cycles an execution.
 *
 * The key hashes the COMPLETE input set rather than the fields I believe can
 * animate. That is the difference between a skip that is correct by
 * construction and one that is correct until someone adds a texture-scroll
 * track: the builder reads nothing outside that set, so equal inputs means
 * equal output with only a 2^-32 collision to argue about. The heap generation
 * is in the key because MObj pointers are taskman-arena addresses and a scene
 * rewind reuses them.
 *
 * The build's side effect -- writing texture_id_curr/next back into the MObj --
 * is safe to skip for the same reason: the stored hash is taken AFTER the
 * write-back, so a match means the write would store what is already there.
 *
 * Only the production path passes keys. The hierarchy path's materials array is
 * a caller local that does not survive the frame, so it passes NULL and always
 * builds. After this the census counts REBUILDS, which makes gNdsFtrPreMatCalls
 * engagement proof: it should read tens, not tens of thousands. */
#if NDS_TICK_HUD
/* Engagement proof. gNdsFtrPreMatCalls cannot answer this: it counts calls that
 * reach the census inside the build wrapper, and the hierarchy call site passes
 * no keys, so a skipping production path and a never-skipping one can produce
 * the same census. These two count the decision itself. */
volatile u32 gNdsR2MatKeySkip;
volatile u32 gNdsR2MatKeyBuild;
/* And WHY a build happened, because the two answers point at different fixes.
 * Identity: the row holds a different MObj than last frame, so the block for
 * this one is sitting in some other row -- fix is a stable row assignment.
 * Inputs: same MObj, hash moved -- fix would be a narrower key, which is
 * already refuted. Guessing between them once cost a build. */
volatile u32 gNdsR2MatKeyMissIdentity;
volatile u32 gNdsR2MatKeyMissInputs;
#endif

/* Hashes all 30 words of MObjSub, including the six the builder never reads
 * (sub.unk48, sub.unk4C, sub.unk68..unk74). Narrowing it to the builder's exact
 * read set was tried and is REFUTED: the engagement counters came back
 * bit-identical -- 28,786 skips and 30,606 builds either way -- and FTR rose
 * 1,155. The rebuilds are not caused by those words at all. They are
 * `keys[count].mobj != mobj`: the materials array is indexed by (selected-root
 * slot, chain position), and which DObj lands in slot i rotates between frames,
 * so about half the lookups find the right block under the wrong index.
 *
 * Recovering that half needs a per-MObj store -- 33 live MObjs x 100 bytes plus
 * keys, about 7 KB of bss against an arena whose low-water is already under the
 * GObj-cap threshold -- or a stable slot assignment. Neither is this slice.
 * Since the narrow hash bought nothing, keep the one that needs no field audit
 * to stay correct. */
static u32 ndsRendererAdapterMaterialRow(DObj *dobj, u32 fallback_row)
{
    u32 base;
    u32 probe;

    if (dobj == NULL)
    {
        sNdsRendererAdapterMaterialRowClaimMask |= 1u << fallback_row;
        return fallback_row;
    }
    if (sNdsRendererAdapterMaterialRowGeneration != gNdsTaskmanHeapGeneration)
    {
        u32 j;

        for (j = 0u; j < NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED; j++)
        {
            sNdsRendererAdapterMaterialRowOwner[j] = NULL;
        }
        sNdsRendererAdapterMaterialRowGeneration = gNdsTaskmanHeapGeneration;
        sNdsRendererAdapterMaterialRowClaimMask = 0u;
    }
    /* Multiplicative, not `>> 4`: DObjs are allocated contiguously and a shift
     * hash strided them onto a handful of rows, so the probe loop ran several
     * iterations and the whole lookup measured 106 cycles a call (2,024
     * ticks/frame) on a 128-byte table that is always in cache. */
    base = ((u32)(uintptr_t)dobj * 2654435761u) >>
        (32u - NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED_LOG2);
    for (probe = 0u; probe < NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED; probe++)
    {
        u32 row = (base + probe) & (NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED - 1u);

        if (sNdsRendererAdapterMaterialRowOwner[row] == dobj)
        {
            /* The display contract can reference the same material DObj more
             * than once. Sharing in that case is source-equivalent and safe. */
            sNdsRendererAdapterMaterialRowClaimMask |= 1u << row;
            return row;
        }
        if ((sNdsRendererAdapterMaterialRowOwner[row] == NULL) &&
            ((sNdsRendererAdapterMaterialRowClaimMask & (1u << row)) == 0u))
        {
            sNdsRendererAdapterMaterialRowOwner[row] = dobj;
            sNdsRendererAdapterMaterialRowClaimMask |= 1u << row;
            return row;
        }
    }
    /* Every row is owned by some persistent DObj. Reclaim one that this CURRENT
     * owner has not claimed. The input key catches the changed identity and
     * rebuilds that row. selected_count cannot exceed the row count, so unless
     * duplicate DObjs reduced the number of claims, an unclaimed row is always
     * available here. */
    for (probe = 0u; probe < NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED; probe++)
    {
        u32 row = (base + probe) & (NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED - 1u);

        if ((sNdsRendererAdapterMaterialRowClaimMask & (1u << row)) == 0u)
        {
            sNdsRendererAdapterMaterialRowOwner[row] = dobj;
            sNdsRendererAdapterMaterialRowClaimMask |= 1u << row;
            return row;
        }
    }
    /* Defensive only: the collection bound above makes this unreachable. */
    return fallback_row;
}

/* The nine words that actually move during a match, in the two contiguous runs
 * they occupy. `primcolor` through `light2color` is `MObjSub` 0x50..0x67 -- the
 * five colour tracks gcPlayMObjMatAnim writes plus the prim level/min byte pair
 * that shares their run -- and texture_id_curr through palette_id is the twelve
 * bytes immediately after `sub`. Two cache lines instead of five, nine
 * multiply-accumulates instead of thirty-four. */
static u32 __attribute__((section(".itcm")))
ndsRendererAdapterMaterialAnimHash(const MObj *mobj)
{
    const u32 *colors = (const u32 *)(const void *)&mobj->sub.primcolor;
    u32 hash = 2166136261u;
    u32 i;

    for (i = 0u; i < 6u; i++)
    {
        hash = (hash ^ colors[i]) * 16777619u;
    }
    hash = (hash ^ (((u32)mobj->texture_id_curr << 16) |
                    (u32)mobj->texture_id_next)) * 16777619u;
    hash = (hash ^ *(const u32 *)(const void *)&mobj->lfrac) * 16777619u;
    hash = (hash ^ *(const u32 *)(const void *)&mobj->palette_id) * 16777619u;
    return hash;
}

/* `light2color` is the last field of the colour run, so the six words starting
 * at `primcolor` must land exactly on it. If MObjSub is ever reordered this
 * stops compiling rather than silently hashing the wrong bytes. */
_Static_assert(offsetof(MObjSub, light2color) ==
                   offsetof(MObjSub, primcolor) + 20u,
               "material anim hash assumes primcolor..light2color are six "
               "contiguous words");

/* P2-2 fighter packet: the identity of everything the current fighter's
 * material rows were built from -- every prepared MObj's animation hash and
 * pointer plus the colour modulate -- folded into one word per draw and handed
 * to the production owner as its packet key. The hashes already exist (the
 * material memo computes them per MObj per frame), so this is a few XORs. */
static u32 sNdsFighterPacketMaterialIdentity;

/* The animation-state hash and identity of every material the selected roots
 * carry, from the live MObj chains alone -- no rows, no snapshots -- so the
 * adapter can ask whether the packet will replay before it spends the
 * preparation on a frame whose replay reads none of it. Taken before any
 * build on the record path too, so the key means one thing on both paths. */
static u32 ndsRendererAdapterMaterialIdentity(
    DObj *const *material_dobjs, u32 count)
{
    u32 identity = 2166136261u;
    u32 i;

    for (i = 0u; i < count; i++)
    {
        const DObj *dobj = material_dobjs[i];
        const MObj *mobj;

        for (mobj = (dobj != NULL) ? dobj->mobj : NULL;
             mobj != NULL;
             mobj = mobj->next)
        {
            identity = (identity ^ ndsRendererAdapterMaterialAnimHash(mobj)) *
                       16777619u;
            identity ^= (u32)(uintptr_t)mobj;
        }
    }
    return identity;
}

static sb32 ndsRendererAdapterPrepareNativeMaterials(
    DObj *dobj, NDSRendererNativeMaterial *materials,
    u32 capacity, u32 *out_count,
    NDSRendererAdapterMaterialKey *keys,
    s32 *save_curr, s32 *save_next)
{
    MObj *mobj;
    u32 count = 0u;

    if ((materials == NULL) || (out_count == NULL))
    {
        return FALSE;
    }
    *out_count = 0u;
    if ((dobj == NULL) || (dobj->mobj == NULL))
    {
        return TRUE;
    }
#if NDS_R2_SECOND_ENTRY_DIAG
    ndsR2ChainProbe(dobj, &gNdsR2ChainProbePass1, 1u);
    ndsR2ChainProbe(dobj, &gNdsR2ChainProbePass2, 2u);
#endif
    /* There is no counting pre-pass any more. It walked the whole MObj chain a
     * second time -- a dependent pointer chase, 37 chains a frame, 1,215
     * ticks/frame on its `mobj = mobj->next` alone in the c115 per-PC census --
     * purely so an over-capacity chain could be rejected before anything was
     * written. The write walk below already carries that bound of its own, and
     * since the rollback slice it also reports `*out_count` on rejection, so the
     * caller undoes exactly the entries this walk touched. Two passes proved one
     * fact; one pass proves it at the point of use.
     *
     * The chain validator measured 13,938 chains of ONE node against a capacity
     * of four, so the difference between rejecting before and rejecting during
     * is a path that has never been taken. */
    for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
    {
        /* Bound the WRITE walk too, not just the counting one above. The count
         * pass only constrains this pass if the list is identical across both,
         * and it is not: ndsRendererAdapterBuildNativeMaterial is the
         * advance_texture_ids=TRUE wrapper, so this loop writes
         * mobj->texture_id_curr/next into every node as it walks. A list that
         * turns cyclic or is corrupted mid-walk ran `materials[count]` off the
         * end of a `capacity`-entry array -- capacity is 4 -- with no check at
         * all. Returning FALSE hands the caller its existing generic fallback
         * instead of corrupting whatever follows the array.
         *
         * Found while reproducing the Sudden Death freeze (docs/BUGS.md): the
         * scene presents two frames and then none, with no overflow assert
         * firing anywhere, and an interrupt landing inside this loop's inlined
         * material build. */
        if ((gNdsR2MaterialWalkBoundEnabled != 0u) && (count >= capacity))
        {
            /* Engagement proof, and it is NOT decoration. The Sudden Death
             * freeze stopped when this guard went in, but the default-off chain
             * validator then found 13,938 clean chains of ONE node against a
             * capacity of four -- so on that evidence this branch can never be
             * reached, and "the guard fixed the freeze" and "the chain is fine"
             * cannot both be true. This counter is what tells them apart: a run
             * that is freeze-free with this still at zero proves the guard was
             * not the cure and the real cause is still live. */
            gNdsR2MaterialWalkBoundHits++;
            /* Report what the snapshot holds so the caller rolls back exactly
             * the entries this walk mutated, no more and no fewer. */
            *out_count = count;
            return FALSE;
        }
        if (save_curr != NULL)
        {
            /* The rollback snapshot, taken here rather than in a walk of its
             * own. ndsRendererAdapterSaveNativeMaterialTextureIds was a second
             * pass over the same chain reading the same two fields -- 3,429
             * ticks/frame in the c110 profile -- and the hash below loads them
             * anyway, so this costs two stores into a line the caller owns. */
            save_curr[count] = mobj->texture_id_curr;
            save_next[count] = mobj->texture_id_next;
        }
        if (keys != NULL)
        {
            u32 hash = ndsRendererAdapterMaterialAnimHash(mobj);

            if ((keys[count].mobj == mobj) &&
                (keys[count].heap_generation == gNdsTaskmanHeapGeneration) &&
                (keys[count].hash == hash))
            {
#if NDS_TICK_HUD
                gNdsR2MatKeySkip++;
#endif
                count++;
                continue;
            }
#if NDS_TICK_HUD
            gNdsR2MatKeyBuild++;
            if (keys[count].mobj != mobj)
            {
                gNdsR2MatKeyMissIdentity++;
            }
            else
            {
                gNdsR2MatKeyMissInputs++;
            }
#endif
        }
        if (ndsRendererAdapterBuildNativeMaterial(
                mobj, &materials[count]) == FALSE)
        {
            if (keys != NULL)
            {
                keys[count].mobj = NULL;
            }
            /* count + 1: entry `count` was snapshotted above and the builder is
             * the advance_texture_ids=TRUE wrapper, so it may have written this
             * MObj's ids before failing. Restoring an untouched entry writes
             * back what is already there, so over-reporting by one is safe and
             * under-reporting is not. */
            *out_count = count + 1u;
            return FALSE;
        }
        if (keys != NULL)
        {
            /* After the build, so the stored hashes describe the MObj the build
             * left behind -- it writes texture_id_curr/next back. Both are
             * stored on every build: the full one is only CHECKED periodically,
             * but it has to be current whenever that check lands. */
            keys[count].mobj = mobj;
            keys[count].heap_generation = gNdsTaskmanHeapGeneration;
            keys[count].hash = ndsRendererAdapterMaterialAnimHash(mobj);
        }
        count++;
    }
    *out_count = count;
    return TRUE;
}

static void ndsRendererAdapterRestoreNativeMaterialTextureIds(
    DObj *dobj,
    const s32 *curr,
    const s32 *next,
    u32 count)
{
    MObj *mobj;
    u32 i = 0u;

    if ((dobj == NULL) || (curr == NULL) || (next == NULL))
    {
        return;
    }
    for (mobj = dobj->mobj;
         (mobj != NULL) && (i < count);
         mobj = mobj->next, i++)
    {
        mobj->texture_id_curr = curr[i];
        mobj->texture_id_next = next[i];
    }
}

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
static sb32 ndsRendererAdapterValidateNativeOwnerMaterials(
    const NDSRendererNativeMaterial *materials,
    u32 material_count)
{
    u32 i;

    if ((material_count != 0u) && (materials == NULL))
    {
        return FALSE;
    }
    for (i = 0u; i < material_count; i++)
    {
        const NDSRendererNativeMaterial *material = &materials[i];
        u32 effects = material->effects;

        if (((effects & NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE) != 0u) &&
            (ndsRelocFindLoadedFileContaining(
                 (const void *)(uintptr_t)material->palette_image,
                 1u) == NULL))
        {
            return FALSE;
        }
        if (((effects & NDS_RENDERER_NATIVE_MATERIAL_BLOCK_IMAGE) != 0u) &&
            (ndsRelocFindLoadedFileContaining(
                 (const void *)(uintptr_t)material->block_image,
                 1u) == NULL))
        {
            return FALSE;
        }
        if (((effects & NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE) != 0u) &&
            (ndsRelocFindLoadedFileContaining(
                 (const void *)(uintptr_t)material->current_image,
                 1u) == NULL))
        {
            return FALSE;
        }
    }
    return TRUE;
}

static void ndsRendererAdapterRestoreNativeOwnerMaterialTextureIds(
    DObj *const *material_dobjs,
    u32 root_count)
{
    if (material_dobjs == NULL)
    {
        return;
    }
    /* A contract may select one material DObj more than once. Roll back in
     * reverse event order so each saved pre-event state is restored and the
     * earliest snapshot remains live for the ordinary renderer fallback. */
    while (root_count != 0u)
    {
        u32 root_index = --root_count;

        ndsRendererAdapterRestoreNativeMaterialTextureIds(
            material_dobjs[root_index],
            sNdsRendererAdapterNativeOwnerTextureCurr[root_index],
            sNdsRendererAdapterNativeOwnerTextureNext[root_index],
            sNdsRendererAdapterNativeOwnerTextureCounts[root_index]);
    }
}
#endif

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
/* A plan hit skips this outright (see "THE DELETION" at the plan-hit branch), and
 * the plan hits every frame -- the c112 cold map found its body inside the
 * driver's third-largest never-executed run. */
static sb32 __attribute__((noinline, cold, optimize("Os")))
ndsRendererAdapterValidateNativeOwnerCached(
    u32 slot,
    u32 use_low_detail,
    const NDSRelocLoadedFile *owner_file,
    u32 root_count,
    const u32 *root_offsets,
    const u32 *material_counts)
{
    NDSRendererAdapterNativeOwnerValidationCache *cache;
    u32 i;
    sb32 identity_matches;

    if ((slot >= NDS_RENDERER_NATIVE_FIGHTER_OWNER_COUNT) ||
        (owner_file == NULL) ||
        (root_offsets == NULL) || (material_counts == NULL) ||
        (root_count > NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED))
    {
#if NDS_TICK_HUD
        gNdsFtrPreValidateReject++;
#endif
        return FALSE;
    }
    cache = &sNdsRendererAdapterNativeOwnerValidationCache[slot];
    identity_matches =
        ((cache->valid != 0u) &&
         (cache->data == owner_file->data) &&
         (cache->asset_id == owner_file->asset_id) &&
         (cache->owner_generation == owner_file->owner_generation) &&
         (cache->data_size == owner_file->data_size) &&
         (cache->root_count == root_count) &&
         (cache->use_low_detail == use_low_detail)) ? TRUE : FALSE;
    if (identity_matches != FALSE)
    {
        for (i = 0u; i < root_count; i++)
        {
            if ((cache->root_offsets[i] != root_offsets[i]) ||
                (cache->material_counts[i] != material_counts[i]))
            {
                identity_matches = FALSE;
                break;
            }
        }
    }
    if (identity_matches != FALSE)
    {
#if NDS_TICK_HUD
        /* Cycle 98. THE counter this row existed to add: without it a cache hit
         * and a cache miss are indistinguishable here, and deleting work whose
         * cache already hits is the mistake cycle 93 avoided on the stage. */
        gNdsFtrPreValidateReuse++;
#endif
        return TRUE;
    }

#if NDS_TICK_HUD
    gNdsFtrPreValidateBuild++;
#endif
    cache->valid = FALSE;
    if (ndsRendererValidateNativeFighterOwner(
            slot, use_low_detail, owner_file->data_size, root_count,
            root_offsets, material_counts) == FALSE)
    {
        return FALSE;
    }
    cache->data = owner_file->data;
    cache->asset_id = owner_file->asset_id;
    cache->owner_generation = owner_file->owner_generation;
    cache->data_size = owner_file->data_size;
    cache->root_count = root_count;
    cache->use_low_detail = use_low_detail;
    for (i = 0u; i < root_count; i++)
    {
        cache->root_offsets[i] = root_offsets[i];
        cache->material_counts[i] = material_counts[i];
    }
    cache->valid = TRUE;
    return TRUE;
}
#endif

static Gfx *ndsRendererAdapterEmitMaterialCommands(Gfx *branch_dl, MObj *mobj)
{
    u32 flags = ndsRendererAdapterMaterialFlags(mobj);
    f32 scau = 0.0F;
    f32 scav = 0.0F;
    f32 trau = 0.0F;
    f32 trav = 0.0F;
    f32 scrollu = 0.0F;
    f32 scrollv = 0.0F;
    s32 uls;
    s32 ult;
    s32 s;
    s32 t;

    if ((branch_dl == NULL) || (mobj == NULL))
    {
        return branch_dl;
    }

    ndsRendererAdapterMaterialTextureState(
        mobj, flags, &scau, &scav, &trau, &trav, &scrollu, &scrollv);

    if (((flags & MOBJ_FLAG_PALETTE) == 0u) &&
        (mobj->sub.palettes != NULL))
    {
        const void *palette = ndsRendererAdapterReadPointerEntry(
            mobj->sub.palettes, (s32)mobj->palette_id);

        if (palette != NULL)
        {
            ndsRendererAdapterEmitTextureImage(
                branch_dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1u, palette);
        }
    }
    if ((flags & MOBJ_FLAG_PALETTE) != 0)
    {
        ndsRendererAdapterEmitTextureImage(
            branch_dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.palettes, (s32)mobj->palette_id));
        if ((flags & (MOBJ_FLAG_SPLIT | MOBJ_FLAG_ALPHA)) != 0)
        {
            ndsRendererAdapterEmitSync(branch_dl++,
                                       NDS_FIGHTER_DL_OP_RDPTILESYNC);
            ndsRendererAdapterEmitSetTile(
                branch_dl++, G_IM_FMT_RGBA, G_IM_SIZ_4b, 0u, 0x0100u, 5u,
                0u, NDS_RENDERER_ADAPTER_G_TX_WRAP,
                NDS_RENDERER_ADAPTER_G_TX_NOMASK,
                NDS_RENDERER_ADAPTER_G_TX_NOLOD,
                NDS_RENDERER_ADAPTER_G_TX_WRAP,
                NDS_RENDERER_ADAPTER_G_TX_NOMASK,
                NDS_RENDERER_ADAPTER_G_TX_NOLOD);
            ndsRendererAdapterEmitSync(branch_dl++,
                                       NDS_FIGHTER_DL_OP_RDPLOADSYNC);
            ndsRendererAdapterEmitLoadTlut(
                branch_dl++, 5u,
                (mobj->sub.siz == G_IM_SIZ_8b) ? 0xffu : 0x0fu);
            ndsRendererAdapterEmitSync(branch_dl++,
                                       NDS_FIGHTER_DL_OP_RDPPIPESYNC);
        }
    }
    if ((flags & MOBJ_FLAG_LIGHT1) != 0)
    {
        branch_dl = ndsRendererAdapterEmitLightColor(
            branch_dl, 1u,
            ndsRendererAdapterPackColor(&mobj->sub.light1color));
    }
    if ((flags & MOBJ_FLAG_LIGHT2) != 0)
    {
        branch_dl = ndsRendererAdapterEmitLightColor(
            branch_dl, 2u,
            ndsRendererAdapterPackColor(&mobj->sub.light2color));
    }
    if ((flags & (MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_FRAC | 0x8u)) != 0)
    {
        if ((flags & MOBJ_FLAG_FRAC) != 0)
        {
            s32 trunc = (s32)mobj->lfrac;

            ndsRendererAdapterEmitPrimColor(
                branch_dl++, mobj->sub.prim_m,
                ndsRendererAdapterClampU8F32(
                    (mobj->lfrac - (f32)trunc) * 256.0F),
                mobj->sub.primcolor.s.r,
                mobj->sub.primcolor.s.g,
                mobj->sub.primcolor.s.b,
                mobj->sub.primcolor.s.a);
            mobj->texture_id_curr = trunc;
            mobj->texture_id_next = trunc + 1;
        }
        else
        {
            ndsRendererAdapterEmitPrimColor(
                branch_dl++, mobj->sub.prim_m,
                ndsRendererAdapterClampU8F32(mobj->lfrac * 255.0F),
                mobj->sub.primcolor.s.r,
                mobj->sub.primcolor.s.g,
                mobj->sub.primcolor.s.b,
                mobj->sub.primcolor.s.a);
        }
    }
    if ((flags & MOBJ_FLAG_ENVCOLOR) != 0)
    {
        ndsRendererAdapterEmitColor(
            branch_dl++, NDS_FIGHTER_DL_OP_SETENVCOLOR,
            mobj->sub.envcolor.s.r, mobj->sub.envcolor.s.g,
            mobj->sub.envcolor.s.b, mobj->sub.envcolor.s.a);
    }
    if ((flags & MOBJ_FLAG_BLENDCOLOR) != 0)
    {
        ndsRendererAdapterEmitColor(
            branch_dl++, NDS_FIGHTER_DL_OP_SETBLENDCOLOR,
            mobj->sub.blendcolor.s.r, mobj->sub.blendcolor.s.g,
            mobj->sub.blendcolor.s.b, mobj->sub.blendcolor.s.a);
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_SPLIT)) != 0)
    {
        s32 block_siz = (mobj->sub.block_siz == G_IM_SIZ_32b) ?
            G_IM_SIZ_32b : G_IM_SIZ_16b;

        ndsRendererAdapterEmitTextureImage(
            branch_dl++, mobj->sub.block_fmt, (u32)block_siz, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.sprites, mobj->texture_id_next));
        if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
        {
            u32 texels = 0u;
            u32 dxt = 0u;

            ndsRendererAdapterMaterialLoadBlock(mobj, &texels, &dxt);
            ndsRendererAdapterEmitSync(branch_dl++,
                                       NDS_FIGHTER_DL_OP_RDPLOADSYNC);
            ndsRendererAdapterEmitLoadBlock(branch_dl++, 6u, 0u, 0u,
                                            texels, dxt);
            ndsRendererAdapterEmitSync(branch_dl++,
                                       NDS_FIGHTER_DL_OP_RDPLOADSYNC);
        }
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
    {
        ndsRendererAdapterEmitTextureImage(
            branch_dl++, mobj->sub.fmt, mobj->sub.siz, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.sprites, mobj->texture_id_curr));
    }
    if ((flags & 0x20u) != 0)
    {
        if (mobj->sub.unk10 == 2)
        {
            uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
                (s32)((((f32)mobj->sub.unk0C * trau) / scau) * 4.0F) : 0;
            ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
                (s32)((((f32)mobj->sub.unk0E * trav) / scav) * 4.0F) : 0;
            if (uls < 0)
            {
                uls = 0;
            }
            if (ult < 0)
            {
                ult = 0;
            }
        }
        else
        {
            uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
                (s32)(((((f32)mobj->sub.unk0C * trau) +
                         (f32)mobj->sub.unk0A) / scau) * 4.0F) : 0;
            ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
                (s32)((((((1.0F - scav) - trav) *
                          (f32)mobj->sub.unk0E) +
                         (f32)mobj->sub.unk0A) / scav) * 4.0F) : 0;
        }
        ndsRendererAdapterEmitTileSize(
            branch_dl++, NDS_RENDERER_ADAPTER_G_TX_RENDERTILE, uls, ult,
            (((s32)mobj->sub.unk0C - 1) << 2) + uls,
            (((s32)mobj->sub.unk0E - 1) << 2) + ult);
    }
    if ((flags & 0x40u) != 0)
    {
        uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
            (s32)(((((f32)mobj->sub.unk38 * scrollu) +
                     (f32)mobj->sub.unk0A) / scau) * 4.0F) : 0;
        ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
            (s32)((((((1.0F - scav) - scrollv) *
                      (f32)mobj->sub.unk3A) +
                     (f32)mobj->sub.unk0A) / scav) * 4.0F) : 0;
        ndsRendererAdapterEmitTileSize(
            branch_dl++, 1u, uls, ult,
            (((s32)mobj->sub.unk38 - 1) << 2) + uls,
            (((s32)mobj->sub.unk3A - 1) << 2) + ult);
    }
    if ((flags & MOBJ_FLAG_TEXTURE) != 0)
    {
        if (mobj->sub.unk10 == 2)
        {
            s = (ABSF(scau) > (1.0F / 65535.0F)) ?
                (s32)(((f32)mobj->sub.unk0C * 64.0F) / scau) : 0;
            t = (ABSF(scav) > (1.0F / 65535.0F)) ?
                (s32)(((f32)mobj->sub.unk0E * 64.0F) / scav) : 0;
        }
        else
        {
            s = ((mobj->sub.unk08 != 0) &&
                 (ABSF(scau) > (1.0F / 65535.0F))) ?
                (s32)((2097152.0F / (f32)mobj->sub.unk08) / scau) : 0;
            t = ((mobj->sub.unk08 != 0) &&
                 (ABSF(scav) > (1.0F / 65535.0F))) ?
                (s32)((2097152.0F / (f32)mobj->sub.unk08) / scav) : 0;
        }
        if (s > 0xffff)
        {
            s = 0xffff;
        }
        if (t > 0xffff)
        {
            t = 0xffff;
        }
        ndsRendererAdapterEmitTexture(
            branch_dl++, (u32)s, (u32)t, 0u,
            NDS_RENDERER_ADAPTER_G_TX_RENDERTILE,
            NDS_RENDERER_ADAPTER_G_ON);
    }

    ndsRendererAdapterEmitEndDL(branch_dl++);
    return branch_dl;
}

static sb32 ndsRendererAdapterPrepareMaterialSegment(
    DObj *dobj, NDSFighterDLDrawState *state)
{
    MObj *mobj;
    Gfx *table;
    Gfx *branch_dl;
    uintptr_t heap_start;
    uintptr_t heap_end;
    uintptr_t heap_ptr;
    u32 mobj_count = 0u;
    u32 branch_commands = 0u;
    size_t heap_bytes;
    u32 i = 0u;

    if ((dobj == NULL) || (state == NULL) || (dobj->mobj == NULL))
    {
        return FALSE;
    }
    if (ndsRendererAdapterCountMaterialCommands(
            dobj, &mobj_count, &branch_commands) == FALSE)
    {
        return FALSE;
    }
    if ((mobj_count == 0u) ||
        (gSYTaskmanGraphicsHeap.ptr == NULL) ||
        (gSYTaskmanGraphicsHeap.start == NULL) ||
        (gSYTaskmanGraphicsHeap.end == NULL))
    {
        return FALSE;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererProfileRecordMaterialOperations(mobj_count);
#endif

    heap_start = (uintptr_t)gSYTaskmanGraphicsHeap.start;
    heap_end = (uintptr_t)gSYTaskmanGraphicsHeap.end;
    heap_ptr = (uintptr_t)gSYTaskmanGraphicsHeap.ptr;
    heap_bytes = (size_t)(mobj_count + branch_commands) * sizeof(Gfx);
    if ((heap_ptr < heap_start) || (heap_ptr > heap_end) ||
        (heap_bytes > (size_t)(heap_end - heap_ptr)))
    {
        return FALSE;
    }

    table = (Gfx *)gSYTaskmanGraphicsHeap.ptr;
    branch_dl = table + mobj_count;
    for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next, i++)
    {
        ndsRendererAdapterEmitBranchTableCommand(&table[i], branch_dl);
        branch_dl = ndsRendererAdapterEmitMaterialCommands(branch_dl, mobj);
    }

    gSYTaskmanGraphicsHeap.ptr = branch_dl;
    state->segment_e_base = table;
    state->segment_e_end = branch_dl;
    return TRUE;
}

#if NDS_TICK_HUD
/* G3 STEP 0 -- THE UNIQUE-TEMPLATE CENSUS, and it is the number a packet arena
 * is sized by. Every G3 figure banked so far counts list INSTANCES
 * (gNdsEffectDLSubmitCount: 1,360 Boundary, 527-563 gate arm); an arena sized
 * from an instance count is wrong by whatever the reuse factor is, and that
 * factor has never been measured.
 *
 * The key is the display-list pointer. That is sound WITHIN a match -- dl points
 * into a loaded-file buffer resident for the scene, the same property G1's
 * texture-site memo relies on -- and it is NOT sound across one: charter 3.12,
 * the taskman arena rewinds and hands the next scene the same addresses. A
 * builder sized by this census must re-derive at scene entry. The census only
 * has to survive the window it measures, and P1 boots straight into one match.
 *
 * StateVariants/CommandVariants are the feasibility guards: if one dl is
 * submitted under two different entry blend modes or yields two different
 * command counts, then "one packet per unique dl" is not a complete key and the
 * arena needs more entries than Unique. They must be read before Unique is
 * trusted as the sizing input.
 *
 * Called from the epilogue, OUTSIDE the Exec bracket that closes above it, so
 * ticks/list stays the interpreter's own cost rather than the census's. */
#define NDS_EFFECT_DL_CENSUS_CAPACITY 256u

static const Gfx *sNdsEffectDLCensusKey[NDS_EFFECT_DL_CENSUS_CAPACITY];
static u32 sNdsEffectDLCensusOtherMode[NDS_EFFECT_DL_CENSUS_CAPACITY];
static u32 sNdsEffectDLCensusCommands[NDS_EFFECT_DL_CENSUS_CAPACITY];
/* PER-TEMPLATE GEOMETRY, AND IT MUST BE THE MAX RATHER THAN THE FIRST SIGHTING.
 * hardware_triangle_count is a POST-CULL count, so the same template submits
 * different geometry on different frames as it moves through the frustum. A
 * packet has to encode the template's whole content, so the sizing input is the
 * largest submission ever seen, not a sample of one. GeomVariants says whether
 * culling moves it at all: 0 means the geometry is frame-invariant and the max
 * is exact; non-zero means the max is the honest lower bound on static content
 * and the arena wants margin over it. */
static u32 sNdsEffectDLCensusTrisMax[NDS_EFFECT_DL_CENSUS_CAPACITY];
static u32 sNdsEffectDLCensusVertsMax[NDS_EFFECT_DL_CENSUS_CAPACITY];

static void ndsEffectDLCensusRecord(const Gfx *dl, u32 commands,
                                    u32 othermode_in, u32 tris, u32 verts)
{
    u32 count = gNdsEffectDLCensusUnique;
    u32 i;

    for (i = 0u; i < count; i++)
    {
        if (sNdsEffectDLCensusKey[i] == dl)
        {
            if (sNdsEffectDLCensusOtherMode[i] != othermode_in)
            {
                gNdsEffectDLCensusStateVariants++;
            }
            if (sNdsEffectDLCensusCommands[i] != commands)
            {
                gNdsEffectDLCensusCommandVariants++;
            }
            if (tris != sNdsEffectDLCensusTrisMax[i])
            {
                gNdsEffectDLCensusGeomVariants++;
            }
            /* Totals are maintained incrementally so the report never needs a
             * second pass over the table. */
            if (tris > sNdsEffectDLCensusTrisMax[i])
            {
                gNdsEffectDLCensusTrisMaxTotal +=
                    tris - sNdsEffectDLCensusTrisMax[i];
                sNdsEffectDLCensusTrisMax[i] = tris;
            }
            if (verts > sNdsEffectDLCensusVertsMax[i])
            {
                gNdsEffectDLCensusVertsMaxTotal +=
                    verts - sNdsEffectDLCensusVertsMax[i];
                sNdsEffectDLCensusVertsMax[i] = verts;
            }
            return;
        }
    }
    if (count >= NDS_EFFECT_DL_CENSUS_CAPACITY)
    {
        /* Overflow is reported, never silently truncated: a capped unique count
         * reads exactly like a small one and would size the arena short. */
        gNdsEffectDLCensusOverflow++;
        return;
    }
    sNdsEffectDLCensusKey[count] = dl;
    sNdsEffectDLCensusOtherMode[count] = othermode_in;
    sNdsEffectDLCensusCommands[count] = commands;
    sNdsEffectDLCensusTrisMax[count] = tris;
    sNdsEffectDLCensusVertsMax[count] = verts;
    gNdsEffectDLCensusUniqueCommandTotal += commands;
    gNdsEffectDLCensusTrisMaxTotal += tris;
    gNdsEffectDLCensusVertsMaxTotal += verts;
    if (commands > gNdsEffectDLCensusCommandMax)
    {
        gNdsEffectDLCensusCommandMax = commands;
    }
    gNdsEffectDLCensusUnique = count + 1u;
}

/* G3 STEP 1 -- the per-template verdict on the captured GX stream. The capture
 * itself is in nds_renderer.c, hooked into the GX record funnel; this is the
 * comparison, and it deliberately reuses the census's own key so the two answer
 * for exactly the same template population.
 *
 * 32 entries against a measured 8 uniques is 4x margin, and the overflow is
 * counted rather than wrapped: a table that silently dropped a template would
 * report perfect agreement for the ones it kept. */
#define NDS_EFFECT_PACKET_TEMPLATE_CAPACITY 32u

static const Gfx *sNdsEffectPacketKey[NDS_EFFECT_PACKET_TEMPLATE_CAPACITY];
static u32 sNdsEffectPacketGeomHashSeen[NDS_EFFECT_PACKET_TEMPLATE_CAPACITY];
static u32 sNdsEffectPacketColorHashSeen[NDS_EFFECT_PACKET_TEMPLATE_CAPACITY];
static u32 sNdsEffectPacketMatrixHashSeen[NDS_EFFECT_PACKET_TEMPLATE_CAPACITY];
static u32 sNdsEffectPacketGeomWordsSeen[NDS_EFFECT_PACKET_TEMPLATE_CAPACITY];

static void ndsEffectPacketVerdictRecord(const Gfx *dl)
{
    u32 count = gNdsEffectPacketTemplates;
    u32 i;

    for (i = 0u; i < count; i++)
    {
        if (sNdsEffectPacketKey[i] != dl)
        {
            continue;
        }
        if (sNdsEffectPacketGeomHashSeen[i] == gNdsEffectPacketGeomHash)
        {
            gNdsEffectPacketGeomMatchCount++;
        }
        else
        {
            gNdsEffectPacketGeomVariantCount++;
        }
        if (sNdsEffectPacketGeomWordsSeen[i] != gNdsEffectPacketGeomWords)
        {
            /* Separate from the hash verdict on purpose: a stream that changed
             * LENGTH is a different failure from one that changed VALUES, and
             * only the second is a candidate for a patch table. */
            gNdsEffectPacketGeomWordVariantCount++;
        }
        if (sNdsEffectPacketColorHashSeen[i] == gNdsEffectPacketColorHash)
        {
            gNdsEffectPacketColorMatchCount++;
        }
        else
        {
            gNdsEffectPacketColorVariantCount++;
        }
        if (sNdsEffectPacketMatrixHashSeen[i] == gNdsEffectPacketMatrixHash)
        {
            gNdsEffectPacketMatrixMatchCount++;
        }
        else
        {
            gNdsEffectPacketMatrixVariantCount++;
        }
        return;
    }
    if (count >= NDS_EFFECT_PACKET_TEMPLATE_CAPACITY)
    {
        gNdsEffectPacketTableOverflow++;
        return;
    }
    sNdsEffectPacketKey[count] = dl;
    sNdsEffectPacketGeomHashSeen[count] = gNdsEffectPacketGeomHash;
    sNdsEffectPacketColorHashSeen[count] = gNdsEffectPacketColorHash;
    sNdsEffectPacketMatrixHashSeen[count] = gNdsEffectPacketMatrixHash;
    sNdsEffectPacketGeomWordsSeen[count] = gNdsEffectPacketGeomWords;
    gNdsEffectPacketTemplates = count + 1u;
}
#endif

static sb32 ndsRendererAdapterTryNativeEntryEffect(
    DObj *dobj, const Gfx *dl, GObj *camera_gobj, u32 initial_geometry_mode)
{
#if NDS_RENDERER_HW_TRIANGLES
    const u8 *base = NULL;
    u32 owner_asset_id = 0u;
    u32 root_offset = 0u;
    sb32 candidate = FALSE;
    NDSRendererConfig config = {0};
    NDSRendererStats stats;
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    const NDSRendererMatrix20p12 *projection_ptr;
    const NDSRendererMatrix20p12 *modelview_ptr;

    if ((dobj == NULL) || (dl == NULL))
    {
        return FALSE;
    }

    /* Exact source asset + exact generated root is the whole admission test.
     * Do not classify arbitrary effect lists by shape: this path intentionally
     * owns only Mario's pipe and Fox's Arwing. */
    if ((gFTMarioFileSpecial2 != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTMarioFileSpecial2))
    {
        base = (const u8 *)gFTMarioFileSpecial2;
        root_offset = (u32)((const u8 *)dl - base);
        if ((root_offset == 0x03c0u) || (root_offset == 0x04c0u))
        {
            owner_asset_id = 356u;
            candidate = TRUE;
        }
    }
#if NDS_P2_LUIGI
    /* dFTLuigiData points its Special2 slot at llMarioSpecial2FileID exactly
     * like Mario, but ftManager owns a separate destination pointer for each
     * fighter kind.  In a Luigi-vs-Fox match gFTMarioFileSpecial2 is therefore
     * legitimately NULL while gFTDataLuigiSpecial2 contains the same source
     * asset.  Admit that second live base explicitly; otherwise Luigi's pipe
     * falls back to the N64 interpreter even though the AOT packet is already
     * an exact bake of file 356. */
    if ((candidate == FALSE) && (gFTDataLuigiSpecial2 != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataLuigiSpecial2))
    {
        base = (const u8 *)gFTDataLuigiSpecial2;
        root_offset = (u32)((const u8 *)dl - base);
        if ((root_offset == 0x03c0u) || (root_offset == 0x04c0u))
        {
            owner_asset_id = 356u;
            candidate = TRUE;
        }
    }
#endif
    if ((candidate == FALSE) && (gFTDataFoxSpecial3 != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataFoxSpecial3))
    {
        base = (const u8 *)gFTDataFoxSpecial3;
        root_offset = (u32)((const u8 *)dl - base);
        switch (root_offset)
        {
        case 0x1fa0u:
        case 0x2920u:
        case 0x29d0u:
        case 0x29f0u:
        case 0x2a20u:
        case 0x2868u:
        case 0x2a50u:
        case 0x2b00u:
            owner_asset_id = 161u;
            candidate = TRUE;
            break;
        default:
            break;
        }
    }
    if (candidate == FALSE)
    {
        return FALSE;
    }

    ndsRendererAdapterPrepareInitialMatrices(
        dobj,
        (camera_gobj != NULL) ? CObjGetStruct(camera_gobj) :
            ((gGCCurrentCamera != NULL) ? CObjGetStruct(gGCCurrentCamera) : NULL),
        FALSE, &projection, &projection_ptr, &modelview, &modelview_ptr);
    /* The default battle camera has one legitimate split shape where the
     * complete camera transform lives on only one side of the DS pair (the
     * world-quad bridge handles the same contract above).  The generic DL
     * interpreter tolerates that because its matrix stream starts from
     * identity; this fixed owner has no stream to do the implicit fill for it.
     * Make that identity explicit so Fox's six small Arwing glow lists stay on
     * the native path instead of falling back solely because their capture pass
     * supplies one camera half. */
    if ((projection_ptr == NULL) && (modelview_ptr != NULL))
    {
        ndsRendererAdapterMtxIdentity20p12(&projection);
        projection_ptr = &projection;
    }
    if ((modelview_ptr == NULL) && (projection_ptr != NULL))
    {
        ndsRendererAdapterMtxIdentity20p12(&modelview);
        modelview_ptr = &modelview;
    }
    if ((projection_ptr == NULL) || (modelview_ptr == NULL))
    {
        gNdsEntryEffectNativeFallbackCount++;
        return FALSE;
    }

    ndsRendererInitStats(&stats);
    /* Light state. Both lists inherit G_LIGHTING from the battle display and
     * carry their own gSPLightColor words in the packet; what they do not
     * carry is seeded from what the display left in the RSP: the direction
     * scVSBattleFuncLights aimed, re-aimed by the fighter's own
     * ftDisplayLightsDrawReflect when it uses a light (same seed as
     * ndsRendererAdapterBeginStageTraversal), and for a group before the
     * first colour word the last colours written: the effect's own MObj
     * colours when it carries MOBJ_FLAG_LIGHT1/2, otherwise the fighter
     * material drawn before it. */
    if ((sNdsFighterDisplayCurrentLightValid != FALSE) &&
        (sNdsFighterDisplayCurrentLightCount != 0u))
    {
        stats.light_dir_x = sNdsFighterDisplayCurrentLight.l.dir[0];
        stats.light_dir_y = sNdsFighterDisplayCurrentLight.l.dir[1];
        stats.light_dir_z = sNdsFighterDisplayCurrentLight.l.dir[2];
        stats.light_dir_mask = 1u;
    }
    {
        MObj *mobj;

        for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
        {
            u32 flags = mobj->sub.flags;

            if ((flags & MOBJ_FLAG_LIGHT1) != 0u)
            {
                stats.light_color_1 =
                    ndsRendererAdapterPackColor(&mobj->sub.light1color);
                stats.light_color_mask |=
                    NDS_FIGHTER_DISPLAY_LIGHT_COLOR_1_MASK;
            }
            if ((flags & MOBJ_FLAG_LIGHT2) != 0u)
            {
                stats.light_color_2 =
                    ndsRendererAdapterPackColor(&mobj->sub.light2color);
                stats.light_color_mask |=
                    NDS_FIGHTER_DISPLAY_LIGHT_COLOR_2_MASK;
            }
        }
        if (gNdsFighterDisplayContractMaterialLightSeedCount != 0u)
        {
            if ((stats.light_color_mask &
                 NDS_FIGHTER_DISPLAY_LIGHT_COLOR_1_MASK) == 0u)
            {
                stats.light_color_1 =
                    gNdsFighterDisplayContractMaterialLight1;
                stats.light_color_mask |=
                    NDS_FIGHTER_DISPLAY_LIGHT_COLOR_1_MASK;
            }
            if ((stats.light_color_mask &
                 NDS_FIGHTER_DISPLAY_LIGHT_COLOR_2_MASK) == 0u)
            {
                stats.light_color_2 =
                    gNdsFighterDisplayContractMaterialLight2;
                stats.light_color_mask |=
                    NDS_FIGHTER_DISPLAY_LIGHT_COLOR_2_MASK;
            }
        }
    }
    config.max_depth = 4u;
    config.max_commands = 1u;
    config.max_list_commands = 1u;
    config.initial_projection = projection_ptr;
    config.initial_modelview = modelview_ptr;
    config.initial_geometry_mode = initial_geometry_mode;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;

    if (ndsRendererSubmitNativeEntryEffect(
            owner_asset_id, root_offset, &config, &stats) == FALSE)
    {
        gNdsEntryEffectNativeFallbackCount++;
        return FALSE;
    }

    gNdsStageGCDrawAllLoopHardwareTriangleCount += stats.hardware_triangle_count;
    gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount +=
        stats.hardware_zbuffer_triangle_count;
    gNdsStageGCDrawAllLoopHardwareProjectedDepthTriangleCount +=
        stats.hardware_projected_depth_triangle_count;
    gNdsStageGCDrawAllLoopHardwareDecalDepthTriangleCount +=
        stats.hardware_decal_depth_triangle_count;
    gNdsStageGCDrawAllLoopHardwareTextureBindCount +=
        stats.hardware_texture_bind_count;
    gNdsStageGCDrawAllLoopHardwareTextureUploadCount +=
        stats.hardware_texture_upload_count;
    gNdsStageGCDrawAllLoopHardwareTextureReadyCount +=
        stats.hardware_texture_ready_count;
    gNdsStageGCDrawAllLoopHardwareTextureRejectCount +=
        stats.hardware_texture_reject_count;
    return TRUE;
#else
    (void)dobj;
    (void)dl;
    (void)camera_gobj;
    (void)initial_geometry_mode;
    return FALSE;
#endif
}

static void ndsRendererAdapterSubmitStageDL(DObj *dobj, const Gfx *dl,
                                             GObj *camera_gobj,
                                             u32 initial_geometry_mode)
{
    NDSRelocLoadedFile *loaded;
    NDSRendererConfig config = {0};
    NDSRendererStats stats;
    NDSRendererStats *render_stats;
    NDSFighterDLDrawState state;
    NDSRendererCommandCallback callback;
    void *callback_user;
    NDSRendererMatrix20p12 initial_projection;
    NDSRendererMatrix20p12 initial_modelview;
    const NDSRendererMatrix20p12 *initial_projection_ptr;
    const NDSRendererMatrix20p12 *initial_modelview_ptr;
#if NDS_R2_IMPACT_WAVE_NATIVE
    NDSRendererNativeMaterial impact_wave_material;
    sb32 impact_wave_native_candidate = FALSE;
    sb32 impact_wave_native_handled = FALSE;
#endif
#if NDS_R2_REBIRTH_HALO_NATIVE
    u32 rebirth_halo_root_offset = 0u;
    sb32 rebirth_halo_native_candidate = FALSE;
    sb32 rebirth_halo_native_handled = FALSE;
#endif
    u32 effect_seed_before = 0u;
    u32 effect_matrix_cmd_before = 0u;
    u32 effect_xform_before = 0u;
    u32 effect_hw_vertex_before = 0u;
    u32 effect_hw_triangle_before = 0u;
#if NDS_RENDERER_HW_TRIANGLES
    void *saved_graphics_heap_ptr;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    sb32 detailed_output;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 step_start;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    u32 adapter_start;
    u32 adapter_ticks;
    NDSRendererOwnerStatsSnapshot owner_stats_before;
#endif
    sb32 inherited_texture = FALSE;
    sb32 inherited_tile = FALSE;
    sb32 inherited_segment = FALSE;
#endif
#if NDS_TICK_HUD
    /* R2-08 phase split. Latched once at entry rather than re-read per phase:
     * the flag is cleared by the tree submit's own epilogue, and a phase that
     * started inside the effect layer must be charged to it whatever the flag
     * says by the time the phase ends. */
    sb32 phase_effect = FALSE;
    u32 phase_dl_mark = 0u;
    u32 phase_mark = 0u;
#endif

    if ((dobj == NULL) || (dl == NULL))
    {
        return;
    }

    /* Entry models are closed generated owners. This sits before the generic
     * loaded-file scan and N64 interpreter setup on purpose: an accepted pipe or
     * Arwing leaf executes only live DObj matrix composition plus the DS-native
     * packet. Compatibility fallback remains available if scene texture prepare
     * failed, and is counted so production verification can require zero. */
    if (ndsRendererAdapterTryNativeEntryEffect(
            dobj, dl, camera_gobj, initial_geometry_mode) != FALSE)
    {
        return;
    }

#if NDS_R2_REBIRTH_HALO_NATIVE && NDS_R2_REBIRTH_HALO_FAST_ADAPTER
    /* RebirthHalo is already identified by the effect-tree owner before any
     * child list reaches here. Its six generated groups contain every source
     * state/texture/vertex dependency, so do not pay the generic adapter's
     * loaded-file scan, segment-E material preparation, callback context and
     * command-interpreter setup merely to arrive at the native submitter.
     *
     * Keep this per-DObj for the experiment: it preserves the exact world
     * matrix of the child and rotating grandchild while isolating the cost of
     * generic adapter ceremony. A later all-tree owner can merge the duplicate
     * child matrix/load once this gate has a visual/tick verdict. */
    if ((sNdsRendererAdapterRebirthHaloNativeActive != FALSE) &&
        (gEFManagerFiles[2] != NULL) &&
        ((const u8 *)dl >= (const u8 *)gEFManagerFiles[2]))
    {
        uintptr_t rebirth_offset = (uintptr_t)((const u8 *)dl -
                                               (const u8 *)gEFManagerFiles[2]);

        if ((rebirth_offset == 0x2378u) || (rebirth_offset == 0x2a88u) ||
            (rebirth_offset == 0x27e8u))
        {
            NDSRendererConfig rebirth_config = {0};
            NDSRendererStats rebirth_stats;
            NDSRendererStats *rebirth_render_stats;
            NDSRendererMatrix20p12 rebirth_projection;
            NDSRendererMatrix20p12 rebirth_modelview;
            const NDSRendererMatrix20p12 *rebirth_projection_ptr;
            const NDSRendererMatrix20p12 *rebirth_modelview_ptr;
#if NDS_RENDERER_HW_TRIANGLES
            void *rebirth_saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;
#endif

            /* 0x2378 and 0x2a88 are the two DL links on the SAME child DObj.
             * Once the first one has emitted both native roots with one matrix
             * setup, the tree walker will immediately offer 0x2a88 again. */
            if ((rebirth_offset == 0x2a88u) &&
                (sNdsRendererAdapterRebirthHaloSkipSecondChildList != FALSE))
            {
                sNdsRendererAdapterRebirthHaloSkipSecondChildList = FALSE;
                return;
            }

            ndsRendererAdapterPrepareInitialMatrices(
                dobj,
                (camera_gobj != NULL) ? CObjGetStruct(camera_gobj) :
                    ((gGCCurrentCamera != NULL) ? CObjGetStruct(gGCCurrentCamera) : NULL),
                TRUE,
                &rebirth_projection,
                &rebirth_projection_ptr,
                &rebirth_modelview,
                &rebirth_modelview_ptr);

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
            if (sNdsRendererAdapterStagePersistentActive != FALSE)
            {
                rebirth_render_stats = &sNdsRendererAdapterStagePersistentStats;
                ndsFighterDLDrawResetRuntimeRendererStats(rebirth_render_stats);
            }
            else
#endif
            {
                rebirth_render_stats = &rebirth_stats;
                ndsRendererInitStats(rebirth_render_stats);
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
                if (sNdsRendererAdapterStagePersistentActive != FALSE)
                {
                    ndsFighterDLDrawCopyPersistentRendererState(
                        rebirth_render_stats, &sNdsRendererAdapterStagePersistentStats);
                }
#endif
            }
            if ((sNdsRendererAdapterEffectColorMask & 1u) != 0u)
            {
                rebirth_render_stats->prim_color = sNdsRendererAdapterEffectPrimColor;
            }
            if ((sNdsRendererAdapterEffectColorMask & 2u) != 0u)
            {
                rebirth_render_stats->env_color = sNdsRendererAdapterEffectEnvColor;
            }
            if (sNdsRendererAdapterEffectOtherModeValid != 0u)
            {
                rebirth_render_stats->othermode_l = sNdsRendererAdapterEffectOtherModeL;
            }

            rebirth_config.max_depth = 8u;
            rebirth_config.max_commands = 8192u;
            rebirth_config.max_list_commands = 512u;
            rebirth_config.initial_projection = rebirth_projection_ptr;
            rebirth_config.initial_modelview = rebirth_modelview_ptr;
            rebirth_config.initial_geometry_mode = initial_geometry_mode;
            rebirth_config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;

            if (ndsRendererSubmitNativeRebirthHalo(
                    (u32)rebirth_offset, &rebirth_config,
                    rebirth_render_stats) != FALSE)
            {
                gNdsRebirthHaloNativeDrawCount++;
                if (rebirth_offset == 0x2378u)
                {
                    /* Same DObj, same source matrix, adjacent source order.
                     * Keep the live renderer state produced by 0x2378 and emit
                     * its second linked list without rebuilding the adapter. */
                    if (ndsRendererSubmitNativeRebirthHalo(
                            0x2a88u, &rebirth_config,
                            rebirth_render_stats) != FALSE)
                    {
                        gNdsRebirthHaloNativeDrawCount++;
                        sNdsRendererAdapterRebirthHaloSkipSecondChildList = TRUE;
                    }
                    else
                    {
                        gNdsRebirthHaloNativeFallbackCount++;
                    }
                }
                gNdsStageGCDrawAllLoopHardwareTriangleCount +=
                    rebirth_render_stats->hardware_triangle_count;
                gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount +=
                    rebirth_render_stats->hardware_zbuffer_triangle_count;
                gNdsStageGCDrawAllLoopHardwareProjectedDepthTriangleCount +=
                    rebirth_render_stats->hardware_projected_depth_triangle_count;
                gNdsStageGCDrawAllLoopHardwareDecalDepthTriangleCount +=
                    rebirth_render_stats->hardware_decal_depth_triangle_count;
                gNdsStageGCDrawAllLoopHardwareTextureBindCount +=
                    rebirth_render_stats->hardware_texture_bind_count;
                gNdsStageGCDrawAllLoopHardwareTextureUploadCount +=
                    rebirth_render_stats->hardware_texture_upload_count;
                gNdsStageGCDrawAllLoopHardwareTextureReadyCount +=
                    rebirth_render_stats->hardware_texture_ready_count;
                gNdsStageGCDrawAllLoopHardwareTextureRejectCount +=
                    rebirth_render_stats->hardware_texture_reject_count;
#if NDS_RENDERER_HW_TRIANGLES
                gSYTaskmanGraphicsHeap.ptr = rebirth_saved_graphics_heap_ptr;
#endif
                return;
            }
            gNdsRebirthHaloNativeFallbackCount++;
#if NDS_RENDERER_HW_TRIANGLES
            gSYTaskmanGraphicsHeap.ptr = rebirth_saved_graphics_heap_ptr;
#endif
        }
    }
#endif

#if NDS_TICK_HUD
    phase_effect =
        (sNdsRendererAdapterEffectSubmitActive != FALSE) ? TRUE : FALSE;
    if (phase_effect != FALSE)
    {
        gNdsEffectPhaseDLCount++;
        phase_dl_mark = cpuGetTiming();
        phase_mark = phase_dl_mark;
    }
#endif
    loaded = ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
    if ((loaded == NULL) &&
        (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) == FALSE))
    {
#if NDS_TICK_HUD
        /* The REJECT exit still costs a full loaded-file scan plus an arena
         * scan, so it is charged rather than dropped -- an unmeasured early
         * return is exactly how a phase split acquires a residual. */
        if (phase_effect != FALSE)
        {
            gNdsEffectPhaseFindTicks += cpuGetTiming() - phase_mark;
            gNdsEffectPhaseDLTicks += cpuGetTiming() - phase_dl_mark;
        }
#endif
        return;
    }
#if NDS_R2_REBIRTH_HALO_NATIVE
    if ((sNdsRendererAdapterRebirthHaloNativeActive != FALSE) &&
        (gEFManagerFiles[2] != NULL) &&
        ((const u8 *)dl >= (const u8 *)gEFManagerFiles[2]))
    {
        uintptr_t offset = (uintptr_t)((const u8 *)dl -
                                       (const u8 *)gEFManagerFiles[2]);

        if ((offset == 0x2378u) || (offset == 0x2a88u) ||
            (offset == 0x27e8u))
        {
            rebirth_halo_root_offset = (u32)offset;
            rebirth_halo_native_candidate = TRUE;
        }
    }
#endif
#if NDS_TICK_HUD
    if (phase_effect != FALSE)
    {
        gNdsEffectPhaseFindTicks += cpuGetTiming() - phase_mark;
    }
#endif

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    detailed_output = (ndsRendererHardwareNoOracleEnabled() == FALSE) ?
        TRUE : FALSE;
    if (detailed_output != FALSE)
    {
        bzero(&state, sizeof(state));
    }
    else
    {
        /* Profile 0/1 submit with a null command callback. Only the compact
         * branch/data resolver context is live; software-preview vertices
         * are already retained by the renderer's persistent vertex cache. */
        state.segment_e_base = NULL;
        state.segment_e_end = NULL;
    }
#else
    bzero(&state, sizeof(state));
#endif
    state.primary_file = loaded;
    state.slot = 0u;
#if NDS_RENDERER_HW_TRIANGLES
    if (sNdsRendererAdapterStagePersistentActive != FALSE)
    {
        inherited_texture = ndsRendererAdapterStatsHasArmedTexture(
            &sNdsRendererAdapterStagePersistentStats);
        inherited_tile = ndsRendererAdapterStatsHasArmedTile(
            &sNdsRendererAdapterStagePersistentStats);
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (detailed_output != FALSE)
        {
            ndsFighterDLDrawSeedPersistentState(
                &state, &sNdsRendererAdapterStagePersistentState);
        }
        else
        {
            state.segment_e_base =
                sNdsRendererAdapterStagePersistentState.segment_e_base;
            state.segment_e_end =
                sNdsRendererAdapterStagePersistentState.segment_e_end;
        }
#else
        ndsFighterDLDrawSeedPersistentState(
            &state, &sNdsRendererAdapterStagePersistentState);
#endif
        inherited_segment = (state.segment_e_base != NULL) ? TRUE : FALSE;
        gNdsStageGCDrawAllLoopHardwareCarrySeedCount++;
        if (inherited_texture != FALSE)
        {
            gNdsStageGCDrawAllLoopHardwareCarryTextureSeedCount++;
        }
        if (inherited_tile != FALSE)
        {
            gNdsStageGCDrawAllLoopHardwareCarryTileSeedCount++;
        }
        if (inherited_segment != FALSE)
        {
            gNdsStageGCDrawAllLoopHardwareCarrySegmentSeedCount++;
        }
    }
    saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    adapter_start = cpuGetTiming();
    step_start = adapter_start;
#elif NDS_RENDERER_PROFILE_LEVEL >= 1
    step_start = cpuGetTiming();
#endif
#endif
#if NDS_TICK_HUD
    if (phase_effect != FALSE)
    {
        phase_mark = cpuGetTiming();
    }
#endif
#if NDS_R2_IMPACT_WAVE_NATIVE
    if ((sNdsRendererAdapterImpactWaveNativeActive != FALSE) &&
        (dobj->mobj != NULL) &&
        (ndsRendererAdapterBuildNativeMaterial(
             dobj->mobj, &impact_wave_material) != FALSE))
    {
        impact_wave_native_candidate = TRUE;
    }
    else
#endif
    {
        ndsRendererAdapterPrepareMaterialSegment(dobj, &state);
    }
#if NDS_TICK_HUD
    if (phase_effect != FALSE)
    {
        gNdsEffectPhaseMaterialTicks += cpuGetTiming() - phase_mark;
        phase_mark = cpuGetTiming();
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 1)
    gNdsRendererProfileMaterialTicks += cpuGetTiming() - step_start;
    step_start = cpuGetTiming();
#endif
    ndsRendererAdapterPrepareInitialMatrices(dobj,
                                             (camera_gobj != NULL) ?
                                                 CObjGetStruct(camera_gobj) :
                                                 ((gGCCurrentCamera != NULL) ?
                                                      CObjGetStruct(
                                                          gGCCurrentCamera) :
                                                      NULL),
                                             TRUE,
                                             &initial_projection,
                                             &initial_projection_ptr,
                                             &initial_modelview,
                                             &initial_modelview_ptr);
#if NDS_TICK_HUD
    if (phase_effect != FALSE)
    {
        gNdsEffectPhaseMatrixTicks += cpuGetTiming() - phase_mark;
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 1)
    gNdsRendererProfileMatrixTicks += cpuGetTiming() - step_start;
#endif

    config.max_depth = 8u;
    config.max_commands = 8192u;
    config.max_list_commands = 512u;
    config.initial_projection = initial_projection_ptr;
    config.initial_modelview = initial_modelview_ptr;
    config.initial_geometry_mode = 0u;
    config.initial_geometry_mode = initial_geometry_mode;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    config.validate_range = ndsRendererAdapterStageValidateRange;
    config.immutable_command_span = ndsRendererAdapterImmutableCommandSpan;
    config.resolve_branch = ndsFighterDLDrawResolveBranch;
    config.resolve_data = ndsFighterDLDrawResolveRendererData;
    config.user = &state;
    callback = (ndsRendererHardwareNoOracleEnabled() != FALSE) ?
        NULL : ndsFighterMarioFoxVisitDLDrawCommand;
    callback_user = &state;

    render_stats = &stats;
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    if (sNdsRendererAdapterStagePersistentActive != FALSE)
    {
        render_stats = &sNdsRendererAdapterStagePersistentStats;
        if (detailed_output != FALSE)
        {
            ndsFighterDLDrawResetTransientRendererStats(render_stats);
        }
        else
        {
            ndsFighterDLDrawResetRuntimeRendererStats(render_stats);
        }
    }
    else
    {
        ndsRendererInitStats(render_stats);
    }
#else
    ndsRendererInitStats(render_stats);
#if NDS_RENDERER_HW_TRIANGLES
    if (sNdsRendererAdapterStagePersistentActive != FALSE)
    {
        ndsFighterDLDrawCopyPersistentRendererState(
            render_stats, &sNdsRendererAdapterStagePersistentStats);
    }
#endif
#endif
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererOwnerSnapshotStats(render_stats, &owner_stats_before);
    step_start = cpuGetTiming();
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererProfileSetSourceProvenance(
        sNdsRendererAdapterStageOwnerOccurrence,
        sNdsRendererAdapterStageListOrdinal,
        ndsRendererOwnerRootBranchPath(
            loaded, dl, sNdsRendererAdapterStageListOrdinal));
    sNdsRendererAdapterStageListOrdinal++;
#endif
#endif
    if (sNdsRendererAdapterEffectSubmitActive != FALSE)
    {
        /* Source order: the proc emits prim/env into the head stream and THEN
         * the model list, so these must land before the list executes and must
         * be allowed to be overridden by the list's own colour commands. */
        if ((sNdsRendererAdapterEffectColorMask & 1u) != 0u)
        {
            render_stats->prim_color = sNdsRendererAdapterEffectPrimColor;
        }
        if ((sNdsRendererAdapterEffectColorMask & 2u) != 0u)
        {
            render_stats->env_color = sNdsRendererAdapterEffectEnvColor;
        }
        /* THE ONLY WRITER OF THE CAPTURED BLEND STATE, and it is inside the
         * effect-submit guard. Without this the effect layer inherits whatever
         * othermode_l the previous list left, which is an opaque stage mode,
         * and ndsRendererHardwareAlpha takes its alpha-31 early return for
         * every effect polygon. */
        if (sNdsRendererAdapterEffectOtherModeValid != 0u)
        {
            render_stats->othermode_l = sNdsRendererAdapterEffectOtherModeL;
        }
        /* Latched HERE, not at the display-proc marker, because the marker
         * publishes the layer's sticky value and a probe stopping at one effect
         * would read another effect's mode. This is the mode THIS list starts
         * with; the Out latch below is what it finishes with. */
        gNdsEffectDLSubmitOtherModeIn = render_stats->othermode_l;
        effect_seed_before = render_stats->hardware_matrix_seed_count;
        effect_matrix_cmd_before = render_stats->matrix_command_count;
        effect_xform_before = render_stats->transformed_vertex_count;
        effect_hw_vertex_before = render_stats->hardware_vertex_count;
        effect_hw_triangle_before = render_stats->hardware_triangle_count;
    }
#if NDS_TICK_HUD
    if (phase_effect != FALSE)
    {
        /* Armed OUTSIDE the Exec tick bracket so the capture's own setup is not
         * charged to Exec. The per-word recording inside it necessarily is,
         * which is why this build's Exec ticks are not a performance reading --
         * this run is about the stream's SHAPE, not its cost. */
        ndsEffectPacketCaptureBegin();
        phase_mark = cpuGetTiming();
    }
#endif
#if NDS_R2_REBIRTH_HALO_NATIVE
    if (rebirth_halo_native_candidate != FALSE)
    {
        rebirth_halo_native_handled = ndsRendererSubmitNativeRebirthHalo(
            rebirth_halo_root_offset, &config, render_stats);
        if (rebirth_halo_native_handled != FALSE)
        {
            gNdsRebirthHaloNativeDrawCount++;
        }
        else
        {
            gNdsRebirthHaloNativeFallbackCount++;
        }
    }
#endif
#if NDS_R2_IMPACT_WAVE_NATIVE
    if (
#if NDS_R2_REBIRTH_HALO_NATIVE
        (rebirth_halo_native_handled == FALSE) &&
#endif
        (impact_wave_native_candidate != FALSE))
    {
        impact_wave_native_handled = ndsRendererSubmitNativeImpactWave(
            sNdsImpactWaveVertices,
            (u32)(sizeof(sNdsImpactWaveVertices) /
                  sizeof(sNdsImpactWaveVertices[0])),
            sNdsImpactWaveTriangles,
            (u32)(sizeof(sNdsImpactWaveTriangles) / 3u),
            dl,
            &impact_wave_material,
            sNdsRendererAdapterImpactWaveVariant,
            &config,
            render_stats);
    }
    if (
#if NDS_R2_REBIRTH_HALO_NATIVE
        (rebirth_halo_native_handled == FALSE) &&
#endif
        (impact_wave_native_handled == FALSE))
    {
        if (impact_wave_native_candidate != FALSE)
        {
            /* Native rejection occurs before any mesh GX emit (near-plane or
             * malformed-contract fallback). Rebuild the source segment-E table
             * only on that rare path, then execute the untouched interpreter. */
            ndsRendererAdapterPrepareMaterialSegment(dobj, &state);
            gNdsImpactWaveNativeFallbackCount++;
        }
        ndsRendererExecuteDisplayListWithVertexCache(
            dl,
            &config,
            callback,
            callback_user,
            render_stats,
            (sNdsRendererAdapterStagePersistentActive != FALSE) ?
                &sNdsRendererAdapterStageVertexCache : NULL);
    }
    else
    {
        gNdsImpactWaveNativeDrawCount++;
    }
#else
#if NDS_R2_REBIRTH_HALO_NATIVE
    if (rebirth_halo_native_handled == FALSE)
#endif
    {
        ndsRendererExecuteDisplayListWithVertexCache(
            dl,
            &config,
            callback,
            callback_user,
            render_stats,
            (sNdsRendererAdapterStagePersistentActive != FALSE) ?
                &sNdsRendererAdapterStageVertexCache : NULL);
    }
#endif
#if NDS_TICK_HUD
    if (phase_effect != FALSE)
    {
        gNdsEffectPhaseExecTicks += cpuGetTiming() - phase_mark;
        ndsEffectPacketCaptureEnd();
    }
#endif
    if (sNdsRendererAdapterEffectSubmitActive != FALSE)
    {
        /* The config's matrices read from OUR locals, and the executor's own
         * verdict on them read as a delta. Cycles 53-55 tried to read the same
         * two pointers out of the callee's argument register and got three
         * different answers; nds_effects.h records why that read can never
         * settle it. */
        /* Out - In names an asset list that carries its own render mode: the
         * rebirth halo's DObj entry[2] list-0 leaves set TEX_EDGE and restore
         * OPA_SURF (85.vpk0.bin 0x23a8/0x2490), while its list-1 beam
         * (0x2890) emits no G_SETOTHERMODE_L at all and leaves Out == In. */
        gNdsEffectDLSubmitOtherModeOut = render_stats->othermode_l;
        gNdsEffectDLSubmitCount++;
        gNdsEffectDLCfgMask =
            ((config.initial_projection != NULL) ? 1u : 0u) |
            ((config.initial_modelview != NULL) ? 2u : 0u);
        if (config.initial_modelview != NULL)
        {
            gNdsEffectDLCfgMvT[0] = config.initial_modelview->m[3][0];
            gNdsEffectDLCfgMvT[1] = config.initial_modelview->m[3][1];
            gNdsEffectDLCfgMvT[2] = config.initial_modelview->m[3][2];
        }
        gNdsEffectDLMatrixSeed =
            render_stats->hardware_matrix_seed_count - effect_seed_before;
        gNdsEffectDLMatrixCmd =
            render_stats->matrix_command_count - effect_matrix_cmd_before;
        gNdsEffectDLXformVertexCount =
            render_stats->transformed_vertex_count - effect_xform_before;
        gNdsEffectDLHwVertexCount =
            render_stats->hardware_vertex_count - effect_hw_vertex_before;
        gNdsEffectDLHwTriangleCount =
            render_stats->hardware_triangle_count - effect_hw_triangle_before;
#if NDS_RENDERER_HW_TRIANGLES
        if (sNdsRendererAdapterStagePersistentActive != FALSE)
        {
            gNdsEffectDLVtx0[0] =
                sNdsRendererAdapterStageVertexCache.transformed_vertices[0].x;
            gNdsEffectDLVtx0[1] =
                sNdsRendererAdapterStageVertexCache.transformed_vertices[0].y;
            gNdsEffectDLVtx0[2] =
                sNdsRendererAdapterStageVertexCache.transformed_vertices[0].z;
            gNdsEffectDLVtx0[3] =
                sNdsRendererAdapterStageVertexCache.transformed_vertices[0].w;
        }
#endif
        gNdsEffectDLBlocker = render_stats->blocker;
        gNdsEffectDLCommandCount = render_stats->command_count;
#if NDS_TICK_HUD
        /* R2-08 CAP-VERSUS-END. The two lines above are LAST-VALUE-WINS, so a
         * stop reads one list; these are cumulative, so a stop reads the whole
         * window. The question they settle is whether the interpreter stops at
         * the list's end or runs to config->max_commands (8192): the executor
         * sets blocker = BUDGET on exactly that path (nds_renderer.c:28303),
         * and BLOCKER_NONE means it reached G_ENDDL under its own steam.
         *
         * This exists because an arithmetic COINCIDENCE nearly bought a
         * deferral: 8192 x 12.54 = 102,727 against a measured 102,730 per list
         * looks like proof the loop runs to its cap, but the 12.54 was obtained
         * by dividing 102,730 BY 8192, so the agreement is a tautology and
         * carries no information. Mean commands per list is the honest form of
         * the same question and it is two adds. */
        gNdsEffectDLCommandTotal += render_stats->command_count;
        if (render_stats->blocker == NDS_RENDERER_BLOCKER_BUDGET)
        {
            gNdsEffectDLTermCapCount++;
        }
        else if (render_stats->blocker == NDS_RENDERER_BLOCKER_NONE)
        {
            gNdsEffectDLTermEndCount++;
        }
        else
        {
            gNdsEffectDLTermOtherCount++;
            gNdsEffectDLTermOtherMask |= 1u << (render_stats->blocker & 31u);
        }
#endif
        gNdsEffectDLFirstOpcode = render_stats->first_opcode;
        gNdsEffectDLUnsupportedOpcode = render_stats->unsupported_opcode;
        /* vertex_count/triangle_count, NOT the *_command_count pair: every
         * site that increments those is wrapped in
         * NDS_RENDERER_RECORD_PROOF_ONLY, which is ((void)0) whenever
         * NDS_RENDERER_HW_TRIANGLES is set -- i.e. dead in every build that
         * can draw. Reading them cost this investigation one wrong conclusion. */
        gNdsEffectDLVertexCount = render_stats->vertex_count;
        gNdsEffectDLTriangleCount = render_stats->triangle_count;
        gNdsEffectDLPublishCount++;
#if NDS_TICK_HUD
        /* Cumulative twins of the two last-value-wins deltas above: a stop reads
         * one list from those, and the census needs the whole window. */
        {
            u32 census_tris = render_stats->hardware_triangle_count -
                effect_hw_triangle_before;
            u32 census_verts = render_stats->hardware_vertex_count -
                effect_hw_vertex_before;

            gNdsEffectDLTriangleTotal += census_tris;
            gNdsEffectDLVertexTotal += census_verts;
            /* OtherModeIn was latched for THIS list before the executor ran, so
             * it is the entry state; render_stats->othermode_l is now exit. */
            ndsEffectDLCensusRecord(dl, render_stats->command_count,
                                    (u32)gNdsEffectDLSubmitOtherModeIn,
                                    census_tris, census_verts);
            /* Same key, same instant, same population as the census above. If
             * the capture's own arming condition ever disagreed with this one,
             * gNdsEffectPacketCaptureCount would diverge from
             * gNdsEffectDLSubmitCount -- both are published, so the
             * disagreement would be visible rather than silent. */
            ndsEffectPacketVerdictRecord(dl);
        }
#endif
    }
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererAdapterAccumulateDepth(
        render_stats,
        &gNdsRendererDepthStageSamples,
        &gNdsRendererDepthStageMin,
        &gNdsRendererDepthStageMax,
        &gNdsRendererDepthStageWMin,
        &gNdsRendererDepthStageWMax);
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileDLTicks += cpuGetTiming() - step_start;
    adapter_ticks = cpuGetTiming() - adapter_start;
    gNdsRendererProfileStageAdapterTicks += adapter_ticks;
    ndsRendererOwnerAccumulateList(
        NDS_RENDERER_PROFILE_OWNER_STAGE, loaded, dl,
        gNdsRendererProfileOwners[
            NDS_RENDERER_PROFILE_OWNER_STAGE].selected_count,
        initial_projection_ptr, initial_modelview_ptr,
        &config,
        &owner_stats_before, render_stats);
#endif
    gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
    if (sNdsRendererAdapterStagePersistentActive != FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (detailed_output != FALSE)
        {
            ndsFighterDLDrawCapturePersistentState(
                &sNdsRendererAdapterStagePersistentState, &state);
        }
        else
        {
            sNdsRendererAdapterStagePersistentState.segment_e_base =
                state.segment_e_base;
            sNdsRendererAdapterStagePersistentState.segment_e_end =
                state.segment_e_end;
        }
#else
        ndsFighterDLDrawCapturePersistentState(
            &sNdsRendererAdapterStagePersistentState, &state);
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        ndsFighterDLDrawCopyPersistentRendererState(
            &sNdsRendererAdapterStagePersistentStats, render_stats);
#endif
        gNdsStageGCDrawAllLoopHardwareCarryCaptureCount++;
        if (render_stats->command_count <= 5u)
        {
            if (inherited_texture != FALSE)
            {
                gNdsStageGCDrawAllLoopHardwareCarryShortTextureSeedCount++;
            }
            if (inherited_tile != FALSE)
            {
                gNdsStageGCDrawAllLoopHardwareCarryShortTileSeedCount++;
            }
        }
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if ((gNdsRendererProfileHardwareTriangles > 2048u) ||
        (gNdsRendererProfileHardwareVertices > 6144u))
    {
        gNdsRendererProfileHardwareOverLimit = 1u;
    }
#endif
#endif
    gNdsStageGCDrawAllLoopHardwareTriangleCount +=
        render_stats->hardware_triangle_count;
    gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount +=
        render_stats->hardware_zbuffer_triangle_count;
    gNdsStageGCDrawAllLoopHardwareProjectedDepthTriangleCount +=
        render_stats->hardware_projected_depth_triangle_count;
    gNdsStageGCDrawAllLoopHardwareDecalDepthTriangleCount +=
        render_stats->hardware_decal_depth_triangle_count;
    gNdsStageGCDrawAllLoopHardwareTextureBindCount +=
        render_stats->hardware_texture_bind_count;
    gNdsStageGCDrawAllLoopHardwareTextureUploadCount +=
        render_stats->hardware_texture_upload_count;
    gNdsStageGCDrawAllLoopHardwareTextureReadyCount +=
        render_stats->hardware_texture_ready_count;
    gNdsStageGCDrawAllLoopHardwareTextureRejectCount +=
        render_stats->hardware_texture_reject_count;
    if (render_stats->hardware_texture_ready_count != 0u)
    {
        if (render_stats->hardware_texture_format < 32u)
        {
            gNdsStageGCDrawAllLoopHardwareTextureFormatMask |=
                1u << render_stats->hardware_texture_format;
        }
        if (render_stats->hardware_texture_width >
            gNdsStageGCDrawAllLoopHardwareTextureMaxWidth)
        {
            gNdsStageGCDrawAllLoopHardwareTextureMaxWidth =
                render_stats->hardware_texture_width;
        }
        if (render_stats->hardware_texture_height >
            gNdsStageGCDrawAllLoopHardwareTextureMaxHeight)
        {
            gNdsStageGCDrawAllLoopHardwareTextureMaxHeight =
                render_stats->hardware_texture_height;
        }
    }
#if NDS_TICK_HUD
    if (phase_effect != FALSE)
    {
        gNdsEffectPhaseDLTicks += cpuGetTiming() - phase_dl_mark;
    }
#endif
}

/* ONE NODE IS NOT A TREE, AND THAT IS WHY THE SHIELD WAS A QUARTER CIRCLE.
 *
 * This submitted the DObj it was handed and stopped. The source does not:
 * gcDrawDObjTree (objdisplay.c) recurses into `child` and, from the first
 * sibling, walks the whole `sib_next` chain. Every multi-node effect model
 * therefore drew only its ROOT node here -- the owner's own words on the first
 * build that routed the real asset were "it is using the correct asset but its
 * only like 1/2 or 1/4 of it, like a 1/4 slice of the complete circle".
 *
 * That single omission is what four BUGS.md rows have in common. The shield,
 * Fox's reflector, the rebirth halo and the impact wave are all EFDescs whose
 * geometry is a DObj tree, so all four were being drawn one node deep, and no
 * amount of atlas resolution or palette work could ever have shown the rest of
 * them.
 *
 * The sibling rule is the source's, kept exactly: only a node with no
 * `sib_prev` walks the chain, so a tree is traversed once rather than once per
 * sibling. Recursion depth is the model's own node depth, which these effect
 * descs keep shallow.
 *
 * NOT copied from the source: gcPrepDObjMatrix and the matching gSPPopMatrix.
 * The DS path composes its transform inside ndsRendererAdapterSubmitStageDL
 * per display list rather than pushing an N64 matrix stack here, so adding a
 * push/pop pair around the recursion would double-transform every child. */
static void ndsRendererAdapterSubmitStageDObjNode(DObj *dobj, u32 kind,
                                                  GObj *camera_gobj,
                                                  u32 initial_geometry_mode);

/* BOUNDED, because this walks a tree the PORT builds from resolved offsets and
 * not one the N64 shipped. Those offsets have held raw symbol addresses before
 * -- the note at Makefile:1393 records gcSetupCustomDObjs walking garbage and
 * allocating a DObj per bogus node until the allocator gave up -- so a cycle or
 * a wild pointer here is a real possibility, and unbounded recursion over one
 * is a hung handheld rather than a wrong picture. The first build of this walk
 * timed out a 300-second probe, which is exactly that failure.
 *
 * The limits are far above any real effect model (these descs are a root plus a
 * handful of parts) and both overruns are counted, so hitting one is a
 * diagnosable defect instead of a freeze. */
#define NDS_RENDERER_STAGE_DOBJ_MAX_DEPTH 16u
#define NDS_RENDERER_STAGE_DOBJ_MAX_SIBLINGS 64u

/* The three counters live in diagnostics.c, not here. Defining them inside this
 * `#if NDS_RENDERER_HW_TRIANGLES` block would make them exist only in the
 * configurations that increment them, and a probe naming an absent symbol loses
 * its whole gdb run. nds_effects.h declares them; cliff_ledge.c resets them,
 * which is also what keeps --gc-sections from collecting them. */

static void ndsRendererAdapterSubmitStageDObjTreeDepth(
    DObj *dobj, u32 kind, GObj *camera_gobj, u32 initial_geometry_mode,
    u32 depth)
{
    DObj *sibling;
    u32 seen;

    if (dobj == NULL)
    {
        return;
    }
    if (depth >= NDS_RENDERER_STAGE_DOBJ_MAX_DEPTH)
    {
        gNdsRendererStageDObjDepthOverrunCount++;
        return;
    }
    gNdsRendererStageDObjNodeCount++;
#if NDS_TICK_HUD
    if (sNdsRendererAdapterEffectSubmitActive != FALSE)
    {
        gNdsEffectPhaseNodeCount++;
    }
#endif
    ndsRendererAdapterSubmitStageDObjNode(dobj, kind, camera_gobj,
                                          initial_geometry_mode);
    if (dobj->child != NULL)
    {
        ndsRendererAdapterSubmitStageDObjTreeDepth(
            dobj->child, kind, camera_gobj, initial_geometry_mode, depth + 1u);
    }
    if (dobj->sib_prev == NULL)
    {
        seen = 0u;
        for (sibling = dobj->sib_next; sibling != NULL;
             sibling = sibling->sib_next)
        {
            if (++seen > NDS_RENDERER_STAGE_DOBJ_MAX_SIBLINGS)
            {
                gNdsRendererStageDObjSiblingOverrunCount++;
                break;
            }
            ndsRendererAdapterSubmitStageDObjTreeDepth(
                sibling, kind, camera_gobj, initial_geometry_mode,
                depth + 1u);
        }
    }
}

/* EFFECTS ONLY, AND THE MEASUREMENT IS WHY.
 *
 * The first version of this recursed inside ndsRendererAdapterSubmitStageDObj,
 * which is the STAGE entry point -- the stage, the weapons and the effects all
 * reach the hardware through it. A synchronized tick-HUD A/B on identical
 * frames priced that at one whole VBlank:
 *
 *     frame   control      candidate
 *       441   1,119,936    1,120,000
 *       443   1,119,872    1,120,000
 *       447   1,119,488    1,680,384   <- 2 VBlanks -> 3
 *       449   1,119,872    1,680,256   <- 2 VBlanks -> 3
 *
 * +560,896 ticks is 560,190-per-VBlank almost exactly, on frames that were
 * inside the 1.12M gate. The stage carries 57 DObjs (M3_NATIVE_STAGE_CHECK
 * dobjs=57) and the native stage path already handles their geometry, so
 * re-walking them bought nothing and cost a frame.
 *
 * The effect models are what needed the tree. So the recursion lives on the
 * EFFECT call site now and the shared stage entry is a single-node submit
 * again, exactly as it was. */
void ndsRendererAdapterSubmitEffectDObjTree(void *dobj_ptr, u32 kind,
                                            void *camera_gobj_ptr,
                                            u32 initial_geometry_mode)
{
#if NDS_R2_IMPACT_WAVE_NATIVE || NDS_R2_REBIRTH_HALO_NATIVE
    DObj *root = (DObj *)dobj_ptr;
#endif
#if NDS_R2_IMPACT_WAVE_NATIVE

    sNdsRendererAdapterImpactWaveVariant = 0u;
    sNdsRendererAdapterImpactWaveNativeActive =
        ((root != NULL) && (root->parent_gobj != NULL) &&
         (ndsEFManagerImpactWaveVariant(
              root->parent_gobj,
              &sNdsRendererAdapterImpactWaveVariant) != FALSE)) ?
            TRUE : FALSE;
#endif
#if NDS_R2_REBIRTH_HALO_NATIVE
    sNdsRendererAdapterRebirthHaloNativeActive =
        ((root != NULL) && (root->xobjs_num != 0) &&
         (root->xobjs[0] != NULL) &&
         (root->xobjs[0]->kind == NDS_RENDERER_ADAPTER_JOINT_ATTACH_TRA_MTX_KIND) &&
         (root->child != NULL) && (gEFManagerFiles[2] != NULL) &&
         (root->child->dl_link ==
              (DObjDLLink *)((u8 *)gEFManagerFiles[2] + 0x2a98u))) ?
            TRUE : FALSE;
#if NDS_R2_REBIRTH_HALO_FAST_ADAPTER
    sNdsRendererAdapterRebirthHaloSkipSecondChildList = FALSE;
#endif
#endif
    sNdsRendererAdapterEffectSubmitActive = TRUE;
#if NDS_TICK_HUD
    gNdsEffectPhaseActive = 1u;
#endif
    ndsRendererAdapterSubmitStageDObjTreeDepth(dobj_ptr, kind, camera_gobj_ptr,
                                               initial_geometry_mode, 0u);
#if NDS_TICK_HUD
    gNdsEffectPhaseActive = 0u;
#endif
    sNdsRendererAdapterEffectSubmitActive = FALSE;
#if NDS_R2_IMPACT_WAVE_NATIVE
    sNdsRendererAdapterImpactWaveNativeActive = FALSE;
    sNdsRendererAdapterImpactWaveVariant = 0u;
#endif
#if NDS_R2_REBIRTH_HALO_NATIVE
    sNdsRendererAdapterRebirthHaloNativeActive = FALSE;
#if NDS_R2_REBIRTH_HALO_FAST_ADAPTER
    sNdsRendererAdapterRebirthHaloSkipSecondChildList = FALSE;
#endif
#endif
}

void ndsRendererAdapterSubmitStageDObj(void *dobj_ptr, u32 kind,
                                       void *camera_gobj_ptr,
                                       u32 initial_geometry_mode)
{
    ndsRendererAdapterSubmitStageDObjNode(dobj_ptr, kind, camera_gobj_ptr,
                                          initial_geometry_mode);
}

static void ndsRendererAdapterSubmitStageDObjNode(DObj *dobj, u32 kind,
                                                  GObj *camera_gobj,
                                                  u32 initial_geometry_mode)
{
    DObjDLLink *dl_link;
    u32 i;

    if (ndsRendererAdapterStageDObjDrawable(dobj, kind) == FALSE)
    {
        return;
    }

    switch (kind)
    {
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD0:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1:
        if (dobj->dv != NULL)
        {
            ndsRendererAdapterSubmitStageDL(dobj, dobj->dl, camera_gobj,
                                            initial_geometry_mode);
        }
        break;

    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS:
        dl_link = dobj->dl_link;
        if (dl_link == NULL)
        {
            return;
        }
        for (i = 0u; i < GC_COMMON_MAX_DLLINKS; i++, dl_link++)
        {
            if (dl_link->list_id == (s32)NDS_RENDERER_STAGE_DL_HEADS)
            {
                break;
            }
            if ((dl_link->list_id >= 0) &&
                ((u32)dl_link->list_id < NDS_RENDERER_STAGE_DL_HEADS) &&
                (dl_link->dl != NULL))
            {
                ndsRendererAdapterSubmitStageDL(dobj, dl_link->dl,
                                                camera_gobj,
                                                initial_geometry_mode);
            }
        }
        break;

    default:
        break;
    }
}

#else
void ndsRendererAdapterBeginStageTraversal(void)
{
}

void ndsRendererAdapterEndStageTraversal(void)
{
}

void ndsRendererAdapterSubmitStageDObj(void *dobj, u32 kind,
                                       void *camera_gobj,
                                       u32 initial_geometry_mode)
{
    (void)dobj;
    (void)kind;
    (void)camera_gobj;
    (void)initial_geometry_mode;
}

void ndsRendererAdapterSubmitEffectDObjTree(void *dobj, u32 kind,
                                            void *camera_gobj,
                                            u32 initial_geometry_mode)
{
    (void)dobj;
    (void)kind;
    (void)camera_gobj;
    (void)initial_geometry_mode;
}

void ndsRendererAdapterMarkDisplayProcHeads(void)
{
}

void ndsRendererAdapterCaptureDisplayProcColors(void)
{
}

#endif

static s32 ndsFighterDLDrawAxisCoord(const NDSFighterDLDrawVtx *vtx,
                                     u32 axis, u32 coord)
{
    if (axis == 0u)
    {
        return (coord == 0u) ? vtx->x : vtx->y;
    }
    if (axis == 1u)
    {
        return (coord == 0u) ? vtx->x : vtx->z;
    }
    return (coord == 0u) ? vtx->y : vtx->z;
}

static void ndsFighterDLDrawRecordAxisPoint(
    const NDSFighterDLDrawVtx *vtx, u32 axis, u32 *bounds_valid,
    s32 *min_a, s32 *max_a, s32 *min_b, s32 *max_b)
{
    s32 a;
    s32 b;

    if ((vtx == NULL) || (bounds_valid == NULL) || (min_a == NULL) ||
        (max_a == NULL) || (min_b == NULL) || (max_b == NULL) ||
        (vtx->valid == FALSE))
    {
        return;
    }

    a = ndsFighterDLDrawAxisCoord(vtx, axis, 0u);
    b = ndsFighterDLDrawAxisCoord(vtx, axis, 1u);
    if (*bounds_valid == 0u)
    {
        *min_a = *max_a = a;
        *min_b = *max_b = b;
        *bounds_valid = 1u;
        return;
    }
    if (a < *min_a) { *min_a = a; }
    if (a > *max_a) { *max_a = a; }
    if (b < *min_b) { *min_b = b; }
    if (b > *max_b) { *max_b = b; }
}

static u16 ndsFighterDLDrawRGB15(u8 r, u8 g, u8 b)
{
    return (u16)(0x8000u | ((u16)(r >> 3)) |
                 ((u16)(g >> 3) << 5) | ((u16)(b >> 3) << 10));
}

static s32 ndsFighterDLDrawEdge(s32 ax, s32 ay, s32 bx, s32 by,
                                s32 px, s32 py)
{
    return ((px - ax) * (by - ay)) - ((py - ay) * (bx - ax));
}

static void ndsFighterDLDrawTriangle(u16 *pixels, u32 pitch,
                                     s32 x0, s32 y0,
                                     s32 x1, s32 y1,
                                     s32 x2, s32 y2,
                                     u16 fill, u16 edge,
                                     u32 *pixel_count)
{
    s32 min_x = x0;
    s32 max_x = x0;
    s32 min_y = y0;
    s32 max_y = y0;
    s32 area;
    s32 x;
    s32 y;

    if ((pixels == NULL) || (pixel_count == NULL))
    {
        return;
    }
    if (x1 < min_x) { min_x = x1; }
    if (x2 < min_x) { min_x = x2; }
    if (x1 > max_x) { max_x = x1; }
    if (x2 > max_x) { max_x = x2; }
    if (y1 < min_y) { min_y = y1; }
    if (y2 < min_y) { min_y = y2; }
    if (y1 > max_y) { max_y = y1; }
    if (y2 > max_y) { max_y = y2; }
    if (min_x < 0) { min_x = 0; }
    if (min_y < 0) { min_y = 0; }
    if (max_x >= (s32)NDS_FIGHTER_DL_DRAW_WIDTH)
    {
        max_x = (s32)NDS_FIGHTER_DL_DRAW_WIDTH - 1;
    }
    if (max_y >= (s32)NDS_FIGHTER_DL_DRAW_HEIGHT)
    {
        max_y = (s32)NDS_FIGHTER_DL_DRAW_HEIGHT - 1;
    }

    area = ndsFighterDLDrawEdge(x0, y0, x1, y1, x2, y2);
    if (area == 0)
    {
        return;
    }

    for (y = min_y; y <= max_y; y++)
    {
        for (x = min_x; x <= max_x; x++)
        {
            s32 w0 = ndsFighterDLDrawEdge(x1, y1, x2, y2, x, y);
            s32 w1 = ndsFighterDLDrawEdge(x2, y2, x0, y0, x, y);
            s32 w2 = ndsFighterDLDrawEdge(x0, y0, x1, y1, x, y);

            if (((w0 >= 0) && (w1 >= 0) && (w2 >= 0)) ||
                ((w0 <= 0) && (w1 <= 0) && (w2 <= 0)))
            {
                pixels[(y * (s32)pitch) + x] =
                    ((w0 == 0) || (w1 == 0) || (w2 == 0)) ? edge : fill;
                (*pixel_count)++;
            }
        }
    }
}

static s32 ndsFighterDLDrawMapCoord(s32 value, s32 min_value, s32 max_value,
                                    s32 out_min, s32 out_max)
{
    s32 range = max_value - min_value;
    s32 out_range = out_max - out_min;

    if (range == 0)
    {
        return out_min;
    }
    return out_min + (((value - min_value) * out_range) / range);
}

static u16 ndsFighterDLDrawTriangleColor(
    const NDSFighterDLDrawState *state, const NDSFighterDLDrawTri *tri)
{
    const NDSFighterDLDrawVtx *v0 = &state->vertices[tri->v0];
    const NDSFighterDLDrawVtx *v1 = &state->vertices[tri->v1];
    const NDSFighterDLDrawVtx *v2 = &state->vertices[tri->v2];
    u32 r = ((u32)v0->r + v1->r + v2->r) / 3u;
    u32 g = ((u32)v0->g + v1->g + v2->g) / 3u;
    u32 b = ((u32)v0->b + v1->b + v2->b) / 3u;

    if ((r == 0u) && (g == 0u) && (b == 0u))
    {
        if (state->slot == 0u)
        {
            r = 255u; g = 96u; b = 32u;
        }
        else
        {
            r = 224u; g = 255u; b = 64u;
        }
    }
    return ndsFighterDLDrawRGB15((u8)r, (u8)g, (u8)b);
}

static void ndsFighterDLDrawRasterizeState(NDSFighterDLDrawState *state,
                                           u16 *pixels, u32 pitch)
{
    u32 axis;
    u32 best_axis = 0xffffffffu;
    u32 best_area = 0u;
    u32 best_nondegenerate_count = 0u;
    u32 i;
    s32 min_a = 0;
    s32 max_a = 0;
    s32 min_b = 0;
    s32 max_b = 0;
    u32 bounds_valid = 0u;
    s32 box_min_x = (state->slot == 0u) ? 4 : 52;
    s32 box_max_x = (state->slot == 0u) ? 43 : 91;
    s32 box_min_y = 4;
    s32 box_max_y = 67;
    s32 screen_min_x = 0;
    s32 screen_max_x = 0;
    s32 screen_min_y = 0;
    s32 screen_max_y = 0;
    u32 screen_valid = 0u;
    u32 pixel_count = 0u;
    u32 drawn_count = 0u;
    u32 real_drawn_count = 0u;
    u32 marker_drawn_count = 0u;

    if ((state == NULL) || (pixels == NULL))
    {
        return;
    }

    for (axis = 0u; axis < 3u; axis++)
    {
        s32 axis_min_a = 0;
        s32 axis_max_a = 0;
        s32 axis_min_b = 0;
        s32 axis_max_b = 0;
        u32 axis_bounds_valid = 0u;
        u32 area_sum = 0u;
        u32 nondegenerate_count = 0u;

        for (i = 0u; i < state->triangle_count; i++)
        {
            const NDSFighterDLDrawTri *tri = &state->tris[i];
            const NDSFighterDLDrawVtx *v0;
            const NDSFighterDLDrawVtx *v1;
            const NDSFighterDLDrawVtx *v2;
            if ((tri->v0 < NDS_FIGHTER_DL_DRAW_MAX_VTX) &&
                (tri->v1 < NDS_FIGHTER_DL_DRAW_MAX_VTX) &&
                (tri->v2 < NDS_FIGHTER_DL_DRAW_MAX_VTX) &&
                (state->vertices[tri->v0].valid != FALSE) &&
                (state->vertices[tri->v1].valid != FALSE) &&
                (state->vertices[tri->v2].valid != FALSE))
            {
                v0 = &state->vertices[tri->v0];
                v1 = &state->vertices[tri->v1];
                v2 = &state->vertices[tri->v2];
                ndsFighterDLDrawRecordAxisPoint(
                    v0, axis, &axis_bounds_valid, &axis_min_a, &axis_max_a,
                    &axis_min_b, &axis_max_b);
                ndsFighterDLDrawRecordAxisPoint(
                    v1, axis, &axis_bounds_valid, &axis_min_a, &axis_max_a,
                    &axis_min_b, &axis_max_b);
                ndsFighterDLDrawRecordAxisPoint(
                    v2, axis, &axis_bounds_valid, &axis_min_a, &axis_max_a,
                    &axis_min_b, &axis_max_b);
            }
        }
        if ((axis_bounds_valid == 0u) ||
            ((axis_min_a == axis_max_a) && (axis_min_b == axis_max_b)))
        {
            continue;
        }

        for (i = 0u; i < state->triangle_count; i++)
        {
            const NDSFighterDLDrawTri *tri = &state->tris[i];
            const NDSFighterDLDrawVtx *v0;
            const NDSFighterDLDrawVtx *v1;
            const NDSFighterDLDrawVtx *v2;
            s32 x0;
            s32 y0;
            s32 x1;
            s32 y1;
            s32 x2;
            s32 y2;
            s32 area;

            if ((tri->v0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                (tri->v1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                (tri->v2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
            {
                continue;
            }
            v0 = &state->vertices[tri->v0];
            v1 = &state->vertices[tri->v1];
            v2 = &state->vertices[tri->v2];
            if ((v0->valid == FALSE) || (v1->valid == FALSE) ||
                (v2->valid == FALSE))
            {
                continue;
            }
            x0 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v0, axis, 0u),
                axis_min_a, axis_max_a, box_min_x, box_max_x);
            y0 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v0, axis, 1u),
                axis_min_b, axis_max_b, box_max_y, box_min_y);
            x1 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v1, axis, 0u),
                axis_min_a, axis_max_a, box_min_x, box_max_x);
            y1 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v1, axis, 1u),
                axis_min_b, axis_max_b, box_max_y, box_min_y);
            x2 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v2, axis, 0u),
                axis_min_a, axis_max_a, box_min_x, box_max_x);
            y2 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v2, axis, 1u),
                axis_min_b, axis_max_b, box_max_y, box_min_y);

            area = ndsFighterDLDrawEdge(x0, y0, x1, y1, x2, y2);
            if (area == 0)
            {
                continue;
            }
            nondegenerate_count++;
            area_sum += (area < 0) ? (u32)-area : (u32)area;
        }

        if ((best_axis > 2u) ||
            (nondegenerate_count > best_nondegenerate_count) ||
            ((nondegenerate_count == best_nondegenerate_count) &&
             (area_sum > best_area)))
        {
            best_area = area_sum;
            best_nondegenerate_count = nondegenerate_count;
            best_axis = axis;
            min_a = axis_min_a;
            max_a = axis_max_a;
            min_b = axis_min_b;
            max_b = axis_max_b;
            bounds_valid = 1u;
        }
    }

    if (best_axis > 2u)
    {
        return;
    }
    if ((bounds_valid == 0u) || ((min_a == max_a) && (min_b == max_b)))
    {
        return;
    }

    for (i = 0u; i < state->triangle_count; i++)
    {
        const NDSFighterDLDrawTri *tri = &state->tris[i];
        const NDSFighterDLDrawVtx *v0;
        const NDSFighterDLDrawVtx *v1;
        const NDSFighterDLDrawVtx *v2;
        s32 x0;
        s32 y0;
        s32 x1;
        s32 y1;
        s32 x2;
        s32 y2;
        u32 before;
        u32 marker_drawn = 0u;
        u16 fill;
        u16 edge;

        if ((tri->v0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
            (tri->v1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
            (tri->v2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
        {
            continue;
        }
        v0 = &state->vertices[tri->v0];
        v1 = &state->vertices[tri->v1];
        v2 = &state->vertices[tri->v2];
        if ((v0->valid == FALSE) || (v1->valid == FALSE) ||
            (v2->valid == FALSE))
        {
            continue;
        }

        x0 = ndsFighterDLDrawMapCoord(
            ndsFighterDLDrawAxisCoord(v0, best_axis, 0u),
            min_a, max_a, box_min_x, box_max_x);
        y0 = ndsFighterDLDrawMapCoord(
            ndsFighterDLDrawAxisCoord(v0, best_axis, 1u),
            min_b, max_b, box_max_y, box_min_y);
        x1 = ndsFighterDLDrawMapCoord(
            ndsFighterDLDrawAxisCoord(v1, best_axis, 0u),
            min_a, max_a, box_min_x, box_max_x);
        y1 = ndsFighterDLDrawMapCoord(
            ndsFighterDLDrawAxisCoord(v1, best_axis, 1u),
            min_b, max_b, box_max_y, box_min_y);
        x2 = ndsFighterDLDrawMapCoord(
            ndsFighterDLDrawAxisCoord(v2, best_axis, 0u),
            min_a, max_a, box_min_x, box_max_x);
        y2 = ndsFighterDLDrawMapCoord(
            ndsFighterDLDrawAxisCoord(v2, best_axis, 1u),
            min_b, max_b, box_max_y, box_min_y);
        fill = ndsFighterDLDrawTriangleColor(state, tri);
        edge = ndsFighterDLDrawRGB15(255, 255, 255);
        before = pixel_count;
        if (ndsFighterDLDrawEdge(x0, y0, x1, y1, x2, y2) == 0)
        {
            s32 cx = (x0 + x1 + x2) / 3;
            s32 cy = (y0 + y1 + y2) / 3;

            ndsFighterDLDrawTriangle(pixels, pitch,
                                     cx - 4, cy - 3,
                                     cx + 4, cy - 3,
                                     cx, cy + 4,
                                     fill, edge, &pixel_count);
            marker_drawn = 1u;
            x0 = cx - 4;
            y0 = cy - 3;
            x1 = cx + 4;
            y1 = cy - 3;
            x2 = cx;
            y2 = cy + 4;
        }
        else
        {
            ndsFighterDLDrawTriangle(pixels, pitch, x0, y0, x1, y1, x2, y2,
                                     fill, edge, &pixel_count);
        }
        if (pixel_count != before)
        {
            drawn_count++;
            if (marker_drawn != 0u)
            {
                marker_drawn_count++;
            }
            else
            {
                real_drawn_count++;
            }
            if (screen_valid == 0u)
            {
                screen_min_x = screen_max_x = x0;
                screen_min_y = screen_max_y = y0;
                screen_valid = 1u;
            }
            if (x0 < screen_min_x) { screen_min_x = x0; }
            if (x1 < screen_min_x) { screen_min_x = x1; }
            if (x2 < screen_min_x) { screen_min_x = x2; }
            if (x0 > screen_max_x) { screen_max_x = x0; }
            if (x1 > screen_max_x) { screen_max_x = x1; }
            if (x2 > screen_max_x) { screen_max_x = x2; }
            if (y0 < screen_min_y) { screen_min_y = y0; }
            if (y1 < screen_min_y) { screen_min_y = y1; }
            if (y2 < screen_min_y) { screen_min_y = y2; }
            if (y0 > screen_max_y) { screen_max_y = y0; }
            if (y1 > screen_max_y) { screen_max_y = y1; }
            if (y2 > screen_max_y) { screen_max_y = y2; }
        }
    }

    if (state->slot == 0u)
    {
        gNdsFighterDLDrawP0Axis = best_axis;
        gNdsFighterDLDrawP0Area = best_area;
        gNdsFighterDLDrawP0MinA = min_a;
        gNdsFighterDLDrawP0MaxA = max_a;
        gNdsFighterDLDrawP0MinB = min_b;
        gNdsFighterDLDrawP0MaxB = max_b;
        gNdsFighterDLDrawP0ScreenMinX = screen_min_x;
        gNdsFighterDLDrawP0ScreenMaxX = screen_max_x;
        gNdsFighterDLDrawP0ScreenMinY = screen_min_y;
        gNdsFighterDLDrawP0ScreenMaxY = screen_max_y;
        gNdsFighterDLDrawP0PixelCount = pixel_count;
        gNdsFighterDLDrawP0TriangleDrawnCount = drawn_count;
        gNdsFighterDLDrawP0RealTriangleDrawnCount = real_drawn_count;
        gNdsFighterDLDrawP0MarkerTriangleDrawnCount = marker_drawn_count;
    }
    else
    {
        gNdsFighterDLDrawP1Axis = best_axis;
        gNdsFighterDLDrawP1Area = best_area;
        gNdsFighterDLDrawP1MinA = min_a;
        gNdsFighterDLDrawP1MaxA = max_a;
        gNdsFighterDLDrawP1MinB = min_b;
        gNdsFighterDLDrawP1MaxB = max_b;
        gNdsFighterDLDrawP1ScreenMinX = screen_min_x;
        gNdsFighterDLDrawP1ScreenMaxX = screen_max_x;
        gNdsFighterDLDrawP1ScreenMinY = screen_min_y;
        gNdsFighterDLDrawP1ScreenMaxY = screen_max_y;
        gNdsFighterDLDrawP1PixelCount = pixel_count;
        gNdsFighterDLDrawP1TriangleDrawnCount = drawn_count;
        gNdsFighterDLDrawP1RealTriangleDrawnCount = real_drawn_count;
        gNdsFighterDLDrawP1MarkerTriangleDrawnCount = marker_drawn_count;
    }
}

static void ndsFighterMarioFoxCopyDLDrawStats(
    u32 slot, const NDSFighterDLDrawState *state,
    const NDSRendererStats *stats)
{
    if ((state == NULL) || (stats == NULL))
    {
        return;
    }

    if (slot == 0u)
    {
        gNdsFighterDLDrawP0Blocker = stats->blocker;
        gNdsFighterDLDrawP0CommandCount = stats->command_count;
        gNdsFighterDLDrawP0FirstOpcode = stats->first_opcode;
        gNdsFighterDLDrawP0UnsupportedOpcode =
            (stats->unsupported_opcode != 0u) ? stats->unsupported_opcode :
                state->unsupported_opcode;
        gNdsFighterDLDrawP0UnsupportedCommandCount =
            stats->unsupported_command_count + state->unsupported_command_count;
        gNdsFighterDLDrawP0VertexDecodedCount = state->vertex_decoded_count;
        gNdsFighterDLDrawP0TriangleCount = state->triangle_count;
        gNdsFighterDLDrawP0TriangleValidCount = state->triangle_valid_count;
        gNdsFighterDLDrawP0ColorChecksum = state->color_checksum;
    }
    else if (slot == 1u)
    {
        gNdsFighterDLDrawP1Blocker = stats->blocker;
        gNdsFighterDLDrawP1CommandCount = stats->command_count;
        gNdsFighterDLDrawP1FirstOpcode = stats->first_opcode;
        gNdsFighterDLDrawP1UnsupportedOpcode =
            (stats->unsupported_opcode != 0u) ? stats->unsupported_opcode :
                state->unsupported_opcode;
        gNdsFighterDLDrawP1UnsupportedCommandCount =
            stats->unsupported_command_count + state->unsupported_command_count;
        gNdsFighterDLDrawP1VertexDecodedCount = state->vertex_decoded_count;
        gNdsFighterDLDrawP1TriangleCount = state->triangle_count;
        gNdsFighterDLDrawP1TriangleValidCount = state->triangle_valid_count;
        gNdsFighterDLDrawP1ColorChecksum = state->color_checksum;
    }
    gNdsFighterDLDrawVertexRangeRejectCount +=
        state->vertex_range_reject_count;
}

static void ndsFighterMarioFoxDrawDLForSlot(u32 slot, FTStruct *fp,
                                            u16 *pixels, u32 pitch)
{
    DObj *root;
    DObj *selected;
    const Gfx *dl;
    NDSRelocLoadedFile *loaded;
    NDSRendererConfig config = {0};
    NDSRendererStats stats;
    NDSFighterDLDrawState state;
    NDSRendererMatrix20p12 initial_projection;
    NDSRendererMatrix20p12 initial_modelview;
    const NDSRendererMatrix20p12 *initial_projection_ptr;
    const NDSRendererMatrix20p12 *initial_modelview_ptr;
#if NDS_RENDERER_HW_TRIANGLES
    void *saved_graphics_heap_ptr;
#endif
    u32 root_x_before;
    u32 root_x_after;
    u32 unused_index;

    if ((slot > 1u) || (pixels == NULL) ||
        (ndsFighterStructIsTrackedPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL) ||
        (fp->status_id != nFTCommonStatusWait) ||
        (fp->motion_id != nFTCommonMotionWait) ||
        (fp->ga != nMPKineticsGround))
    {
        return;
    }

    root = fp->joints[nFTPartsJointTopN];
    root_x_before = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) :
        0u;
    selected = ndsFighterFindFirstDObjWithDL(root, &unused_index);
    if (selected == NULL)
    {
        return;
    }

    dl = selected->dl;
    loaded = ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
    if ((loaded == NULL) &&
        (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) == FALSE))
    {
        return;
    }

    bzero(&state, sizeof(state));
    state.primary_file = loaded;
    state.slot = slot;
#if NDS_RENDERER_HW_TRIANGLES
    saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;
    ndsRendererAdapterPrepareMaterialSegment(selected, &state);
#endif
    ndsRendererAdapterPrepareInitialMatrices(selected,
                                             (gGCCurrentCamera != NULL) ?
                                                 CObjGetStruct(
                                                     gGCCurrentCamera) :
                                                 NULL,
                                             FALSE,
                                             &initial_projection,
                                             &initial_projection_ptr,
                                             &initial_modelview,
                                             &initial_modelview_ptr);

    config.max_depth = 8u;
    config.max_commands = 2048u;
    config.max_list_commands = 512u;
    config.initial_projection = initial_projection_ptr;
    config.initial_modelview = initial_modelview_ptr;
    config.initial_geometry_mode = 0u;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    config.validate_range = ndsFighterDLDrawValidateRange;
    config.immutable_command_span = ndsRendererAdapterImmutableCommandSpan;
    config.resolve_branch = ndsFighterDLDrawResolveBranch;
    config.resolve_data = ndsFighterDLDrawResolveRendererData;
    config.user = &state;

    ndsRendererInitStats(&stats);
    ndsRendererExecuteDisplayList(dl,
                                  &config,
                                  ndsFighterMarioFoxVisitDLDrawCommand,
                                  &state,
                                  &stats);
#if NDS_RENDERER_HW_TRIANGLES
    gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
#endif
    ndsFighterMarioFoxCopyDLDrawStats(slot, &state, &stats);
    if ((stats.blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (state.unsupported_command_count == 0u) &&
        (state.vertex_range_reject_count == 0u))
    {
        ndsFighterDLDrawRasterizeState(&state, pixels, pitch);
    }

    root_x_after = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) :
        0u;
    if (slot == 0u)
    {
        gNdsFighterDLDrawP0StatusAfter = (u32)fp->status_id;
        gNdsFighterDLDrawP0MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLDrawP0GAAfter = (u32)fp->ga;
        gNdsFighterDLDrawP0RootXBeforeBits = root_x_before;
        gNdsFighterDLDrawP0RootXAfterBits = root_x_after;
    }
    else
    {
        gNdsFighterDLDrawP1StatusAfter = (u32)fp->status_id;
        gNdsFighterDLDrawP1MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLDrawP1GAAfter = (u32)fp->ga;
        gNdsFighterDLDrawP1RootXBeforeBits = root_x_before;
        gNdsFighterDLDrawP1RootXAfterBits = root_x_after;
    }

    gNdsFighterMarioFoxDLDrawCount++;
}

static void ndsFighterMarioFoxRunDLDrawProbe(void)
{
    u32 mask = 0u;
    u32 gobj_before;
    u32 gobj_after;
    u32 pitch = 0u;
    u16 *pixels;

    if ((ndsFighterMarioFoxDLDrawProofEnabled() == FALSE) ||
        (gNdsFighterMarioFoxDLDrawResult != 0u))
    {
        return;
    }

    if ((gNdsFighterMarioFoxDLExecResult ==
            NDS_FIGHTER_MARIOFOX_DL_EXEC_PASS) &&
        (gNdsFighterMarioFoxDLExecSafeResult ==
            NDS_FIGHTER_MARIOFOX_DL_EXEC_SAFE_PASS) &&
        ((gNdsFighterMarioFoxDLExecMask & 0x7ffu) == 0x7ffu) &&
        (gNdsFighterMarioFoxDLExecCount == 2u) &&
        (gNdsFighterDLExecP0Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLExecP1Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLExecP0UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLExecP1UnsupportedCommandCount == 0u))
    {
        mask |= 1u << 0;
    }
    else
    {
        gNdsFighterMarioFoxDLDrawMask = mask;
        return;
    }

    gNdsFighterDLDrawPreviewCommitBefore = gNdsOriginalDLPreviewCommitCount;
    pixels = ndsPlatformBeginOriginalDLPreview(NDS_FIGHTER_DL_DRAW_WIDTH,
                                               NDS_FIGHTER_DL_DRAW_HEIGHT,
                                               &pitch);
    if (pixels != NULL)
    {
        gNdsFighterDLDrawPreviewWidth = NDS_FIGHTER_DL_DRAW_WIDTH;
        gNdsFighterDLDrawPreviewHeight = NDS_FIGHTER_DL_DRAW_HEIGHT;
        gNdsFighterDLDrawPreviewPitch = pitch;
        mask |= 1u << 1;
    }
    else
    {
        gNdsFighterMarioFoxDLDrawMask = mask;
        return;
    }

    gobj_before = (u32)gcGetGObjsActiveNum();
    ndsFighterMarioFoxDrawDLForSlot(0u, &sNdsFighterStructPool[0],
                                    pixels, pitch);
    ndsFighterMarioFoxDrawDLForSlot(1u, &sNdsFighterStructPool[1],
                                    pixels, pitch);
    if (gNdsFighterMarioFoxDLDrawCount == 2u)
    {
        mask |= 1u << 2;
    }
    if ((gNdsFighterDLDrawP0VertexDecodedCount > 0u) &&
        (gNdsFighterDLDrawP1VertexDecodedCount > 0u) &&
        (gNdsFighterDLDrawP0TriangleCount > 0u) &&
        (gNdsFighterDLDrawP1TriangleCount > 0u) &&
        (gNdsFighterDLDrawP0TriangleValidCount > 0u) &&
        (gNdsFighterDLDrawP1TriangleValidCount > 0u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterDLDrawP0Axis <= 2u) &&
        (gNdsFighterDLDrawP1Axis <= 2u) &&
        (gNdsFighterDLDrawP0Area > 0u) &&
        (gNdsFighterDLDrawP1Area > 0u) &&
        ((gNdsFighterDLDrawP0MaxA != gNdsFighterDLDrawP0MinA) ||
         (gNdsFighterDLDrawP0MaxB != gNdsFighterDLDrawP0MinB)) &&
        ((gNdsFighterDLDrawP1MaxA != gNdsFighterDLDrawP1MinA) ||
         (gNdsFighterDLDrawP1MaxB != gNdsFighterDLDrawP1MinB)))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterDLDrawP0RealTriangleDrawnCount > 0u) &&
        (gNdsFighterDLDrawP1RealTriangleDrawnCount > 0u))
    {
        mask |= 1u << 5;
    }

    gNdsFighterDLDrawTotalPixelCount =
        gNdsFighterDLDrawP0PixelCount + gNdsFighterDLDrawP1PixelCount;
    if ((gNdsFighterDLDrawP0PixelCount > 0u) &&
        (gNdsFighterDLDrawP1PixelCount > 0u) &&
        (gNdsFighterDLDrawTotalPixelCount > 0u) &&
        (gNdsFighterDLDrawP0ColorChecksum != 0u) &&
        (gNdsFighterDLDrawP1ColorChecksum != 0u))
    {
        mask |= 1u << 6;
    }
    if (gNdsFighterDLDrawTotalPixelCount > 0u)
    {
        ndsPlatformCommitOriginalDLPreview();
        gNdsFighterDLDrawPreviewCommitAfter =
            gNdsOriginalDLPreviewCommitCount;
        gNdsFighterDLDrawPreviewCommitDelta =
            gNdsFighterDLDrawPreviewCommitAfter -
            gNdsFighterDLDrawPreviewCommitBefore;
        gNdsFighterDLDrawPreviewReady = gNdsOriginalDLPreviewReady;
    }
    if ((gNdsFighterDLDrawPreviewReady != 0u) &&
        (gNdsFighterDLDrawPreviewCommitDelta == 1u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterDLDrawP0Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLDrawP1Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLDrawP0UnsupportedOpcode == 0u) &&
        (gNdsFighterDLDrawP1UnsupportedOpcode == 0u) &&
        (gNdsFighterDLDrawP0UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLDrawP1UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLDrawRangeRejectCount == 0u) &&
        (gNdsFighterDLDrawVertexRangeRejectCount == 0u))
    {
        mask |= 1u << 8;
    }

    gobj_after = (u32)gcGetGObjsActiveNum();
    gNdsFighterDLDrawGObjDelta =
        (gobj_after >= gobj_before) ? (gobj_after - gobj_before) :
            (gobj_before - gobj_after);
    if ((gNdsFighterDLDrawP0StatusAfter == (u32)nFTCommonStatusWait) &&
        (gNdsFighterDLDrawP1StatusAfter == (u32)nFTCommonStatusWait) &&
        (gNdsFighterDLDrawP0MotionAfter == (u32)nFTCommonMotionWait) &&
        (gNdsFighterDLDrawP1MotionAfter == (u32)nFTCommonMotionWait) &&
        (gNdsFighterDLDrawP0GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterDLDrawP1GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterDLDrawP0RootXBeforeBits ==
            gNdsFighterDLDrawP0RootXAfterBits) &&
        (gNdsFighterDLDrawP1RootXBeforeBits ==
            gNdsFighterDLDrawP1RootXAfterBits) &&
        (gNdsFighterDLDrawGObjDelta == 0u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsFighterDLDrawDrawCallCount == 0u) &&
        (gNdsFighterDLDrawMatrixCallCount == 0u) &&
        (gNdsFighterDLDrawGameplayUpdateCount == 0u) &&
        (gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
        (gSCManagerSceneData.scene_prev == nSCKindMaps))
    {
        mask |= 1u << 10;
    }

    gNdsFighterMarioFoxDLDrawMask = mask;
    gNdsFighterMarioFoxDLDrawDeferredMask = 0xffu;
    if ((mask & 0x7ffu) == 0x7ffu)
    {
        gNdsFighterMarioFoxDLDrawResult =
            NDS_FIGHTER_MARIOFOX_DL_DRAW_PASS;
        gNdsFighterMarioFoxDLDrawSafeResult =
            NDS_FIGHTER_MARIOFOX_DL_DRAW_SAFE_PASS;
    }
}

typedef struct NDSFighterDLMultiDrawCollection {
    DObj *dobjs[NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED];
    u32 indices[NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED];
    u32 total_count;
    u32 selected_count;
    u32 selected_index_mask;
} NDSFighterDLMultiDrawCollection;

static void ndsFighterCollectDObjsWithDLRecursive(
    DObj *dobj, NDSFighterDLMultiDrawCollection *collection,
    u32 *traversal_index)
{
    while (dobj != NULL)
    {
        u32 current_index = (traversal_index != NULL) ? *traversal_index : 0u;

        if (traversal_index != NULL)
        {
            (*traversal_index)++;
        }

        if ((collection != NULL) && (dobj->dl != NULL))
        {
            if (collection->selected_count <
                NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED)
            {
                u32 selected = collection->selected_count;
                collection->dobjs[selected] = dobj;
                collection->indices[selected] = current_index;
                collection->selected_count++;
                if (current_index < 32u)
                {
                    collection->selected_index_mask |= 1u << current_index;
                }
            }
            collection->total_count++;
        }

        ndsFighterCollectDObjsWithDLRecursive(dobj->child,
                                              collection,
                                              traversal_index);
        dobj = dobj->sib_next;
    }
}

static void ndsFighterCollectDObjsWithDL(
    DObj *root, NDSFighterDLMultiDrawCollection *collection)
{
    u32 traversal_index = 0u;

    if (collection == NULL)
    {
        return;
    }
    bzero(collection, sizeof(*collection));
    ndsFighterCollectDObjsWithDLRecursive(root, collection,
                                          &traversal_index);
}

static s32 ndsFighterDLMultiDrawValidateRange(const Gfx *dl, size_t bytes,
                                              void *user)
{
    (void)user;

    if ((((uintptr_t)dl & (sizeof(u32) - 1u)) != 0u) ||
        ((ndsRelocFindLoadedFileContaining(dl, bytes) == NULL) &&
         (ndsFighterDLScanRangeInTaskmanArena(dl, bytes) == FALSE) &&
         (ndsRendererAdapterRangeIsEmptySegmentEDL(dl, bytes) == FALSE)))
    {
        gNdsFighterDLMultiDrawRangeRejectCount++;
        return FALSE;
    }
    return TRUE;
}

static const Gfx *ndsFighterDLMultiDrawResolveBranch(const Gfx *dl,
                                                     u32 *resolve_kind,
                                                     void *user)
{
    return ndsFighterDLDrawResolveBranch(dl, resolve_kind, user);
}

static void ndsFighterDLMultiDrawRecordScreenPoint(
    u32 slot, s32 x, s32 y, u32 *screen_valid,
    s32 *screen_min_x, s32 *screen_max_x,
    s32 *screen_min_y, s32 *screen_max_y)
{
    if ((screen_valid == NULL) || (screen_min_x == NULL) ||
        (screen_max_x == NULL) || (screen_min_y == NULL) ||
        (screen_max_y == NULL))
    {
        return;
    }

    (void)slot;
    if (x < 0) { x = 0; }
    if (x >= (s32)NDS_FIGHTER_DL_DRAW_WIDTH)
    {
        x = (s32)NDS_FIGHTER_DL_DRAW_WIDTH - 1;
    }
    if (y < 0) { y = 0; }
    if (y >= (s32)NDS_FIGHTER_DL_DRAW_HEIGHT)
    {
        y = (s32)NDS_FIGHTER_DL_DRAW_HEIGHT - 1;
    }

    if (*screen_valid == 0u)
    {
        *screen_min_x = *screen_max_x = x;
        *screen_min_y = *screen_max_y = y;
        *screen_valid = 1u;
        return;
    }
    if (x < *screen_min_x) { *screen_min_x = x; }
    if (x > *screen_max_x) { *screen_max_x = x; }
    if (y < *screen_min_y) { *screen_min_y = y; }
    if (y > *screen_max_y) { *screen_max_y = y; }
}

static void ndsFighterDLMultiDrawRasterizeStates(
    u32 slot, NDSFighterDLDrawState *states, const u8 *clean,
    u32 selected_count, u16 *pixels, u32 pitch)
{
    u32 axis;
    u32 best_axis = 0xffffffffu;
    u32 best_area = 0u;
    u32 best_nondegenerate_count = 0u;
    u32 i;
    s32 min_a = 0;
    s32 max_a = 0;
    s32 min_b = 0;
    s32 max_b = 0;
    u32 bounds_valid = 0u;
    s32 box_min_x = (slot == 0u) ? 4 : 52;
    s32 box_max_x = (slot == 0u) ? 43 : 91;
    s32 box_min_y = 4;
    s32 box_max_y = 67;
    s32 screen_min_x = 0;
    s32 screen_max_x = 0;
    s32 screen_min_y = 0;
    s32 screen_max_y = 0;
    u32 screen_valid = 0u;
    u32 pixel_count = 0u;
    u32 drawn_count = 0u;
    u32 real_drawn_count = 0u;
    u32 marker_drawn_count = 0u;
    u32 drawn_dobj_count = 0u;

    if ((states == NULL) || (clean == NULL) || (pixels == NULL))
    {
        return;
    }

    for (axis = 0u; axis < 3u; axis++)
    {
        s32 axis_min_a = 0;
        s32 axis_max_a = 0;
        s32 axis_min_b = 0;
        s32 axis_max_b = 0;
        u32 axis_bounds_valid = 0u;
        u32 area_sum = 0u;
        u32 nondegenerate_count = 0u;

        for (i = 0u; i < selected_count; i++)
        {
            u32 tri_index;

            if (clean[i] == FALSE)
            {
                continue;
            }
            for (tri_index = 0u; tri_index < states[i].triangle_count;
                 tri_index++)
            {
                const NDSFighterDLDrawTri *tri = &states[i].tris[tri_index];
                const NDSFighterDLDrawVtx *v0;
                const NDSFighterDLDrawVtx *v1;
                const NDSFighterDLDrawVtx *v2;

                if ((tri->v0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                    (tri->v1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                    (tri->v2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
                {
                    continue;
                }
                v0 = &states[i].vertices[tri->v0];
                v1 = &states[i].vertices[tri->v1];
                v2 = &states[i].vertices[tri->v2];
                if ((v0->valid == FALSE) || (v1->valid == FALSE) ||
                    (v2->valid == FALSE))
                {
                    continue;
                }
                ndsFighterDLDrawRecordAxisPoint(
                    v0, axis, &axis_bounds_valid, &axis_min_a, &axis_max_a,
                    &axis_min_b, &axis_max_b);
                ndsFighterDLDrawRecordAxisPoint(
                    v1, axis, &axis_bounds_valid, &axis_min_a, &axis_max_a,
                    &axis_min_b, &axis_max_b);
                ndsFighterDLDrawRecordAxisPoint(
                    v2, axis, &axis_bounds_valid, &axis_min_a, &axis_max_a,
                    &axis_min_b, &axis_max_b);
            }
        }
        if ((axis_bounds_valid == 0u) ||
            ((axis_min_a == axis_max_a) && (axis_min_b == axis_max_b)))
        {
            continue;
        }

        for (i = 0u; i < selected_count; i++)
        {
            u32 tri_index;

            if (clean[i] == FALSE)
            {
                continue;
            }
            for (tri_index = 0u; tri_index < states[i].triangle_count;
                 tri_index++)
            {
                const NDSFighterDLDrawTri *tri = &states[i].tris[tri_index];
                const NDSFighterDLDrawVtx *v0;
                const NDSFighterDLDrawVtx *v1;
                const NDSFighterDLDrawVtx *v2;
                s32 x0;
                s32 y0;
                s32 x1;
                s32 y1;
                s32 x2;
                s32 y2;
                s32 area;

                if ((tri->v0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                    (tri->v1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                    (tri->v2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
                {
                    continue;
                }
                v0 = &states[i].vertices[tri->v0];
                v1 = &states[i].vertices[tri->v1];
                v2 = &states[i].vertices[tri->v2];
                if ((v0->valid == FALSE) || (v1->valid == FALSE) ||
                    (v2->valid == FALSE))
                {
                    continue;
                }
                x0 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v0, axis, 0u),
                    axis_min_a, axis_max_a, box_min_x, box_max_x);
                y0 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v0, axis, 1u),
                    axis_min_b, axis_max_b, box_max_y, box_min_y);
                x1 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v1, axis, 0u),
                    axis_min_a, axis_max_a, box_min_x, box_max_x);
                y1 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v1, axis, 1u),
                    axis_min_b, axis_max_b, box_max_y, box_min_y);
                x2 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v2, axis, 0u),
                    axis_min_a, axis_max_a, box_min_x, box_max_x);
                y2 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v2, axis, 1u),
                    axis_min_b, axis_max_b, box_max_y, box_min_y);

                area = ndsFighterDLDrawEdge(x0, y0, x1, y1, x2, y2);
                if (area == 0)
                {
                    continue;
                }
                nondegenerate_count++;
                area_sum += (area < 0) ? (u32)-area : (u32)area;
            }
        }

        if ((best_axis > 2u) ||
            (nondegenerate_count > best_nondegenerate_count) ||
            ((nondegenerate_count == best_nondegenerate_count) &&
             (area_sum > best_area)))
        {
            best_area = area_sum;
            best_nondegenerate_count = nondegenerate_count;
            best_axis = axis;
            min_a = axis_min_a;
            max_a = axis_max_a;
            min_b = axis_min_b;
            max_b = axis_max_b;
            bounds_valid = 1u;
        }
    }

    if (best_axis > 2u)
    {
        return;
    }
    if ((bounds_valid == 0u) || ((min_a == max_a) && (min_b == max_b)))
    {
        return;
    }

    for (i = 0u; i < selected_count; i++)
    {
        u32 tri_index;
        u32 state_drawn = 0u;

        if (clean[i] == FALSE)
        {
            continue;
        }
        for (tri_index = 0u; tri_index < states[i].triangle_count;
             tri_index++)
        {
            const NDSFighterDLDrawTri *tri = &states[i].tris[tri_index];
            const NDSFighterDLDrawVtx *v0;
            const NDSFighterDLDrawVtx *v1;
            const NDSFighterDLDrawVtx *v2;
            s32 x0;
            s32 y0;
            s32 x1;
            s32 y1;
            s32 x2;
            s32 y2;
            u32 before;
            u32 marker_drawn = 0u;
            u16 fill;
            u16 edge;

            if ((tri->v0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                (tri->v1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                (tri->v2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
            {
                continue;
            }
            v0 = &states[i].vertices[tri->v0];
            v1 = &states[i].vertices[tri->v1];
            v2 = &states[i].vertices[tri->v2];
            if ((v0->valid == FALSE) || (v1->valid == FALSE) ||
                (v2->valid == FALSE))
            {
                continue;
            }

            x0 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v0, best_axis, 0u),
                min_a, max_a, box_min_x, box_max_x);
            y0 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v0, best_axis, 1u),
                min_b, max_b, box_max_y, box_min_y);
            x1 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v1, best_axis, 0u),
                min_a, max_a, box_min_x, box_max_x);
            y1 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v1, best_axis, 1u),
                min_b, max_b, box_max_y, box_min_y);
            x2 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v2, best_axis, 0u),
                min_a, max_a, box_min_x, box_max_x);
            y2 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v2, best_axis, 1u),
                min_b, max_b, box_max_y, box_min_y);
            fill = ndsFighterDLDrawTriangleColor(&states[i], tri);
            edge = ndsFighterDLDrawRGB15(255, 255, 255);
            before = pixel_count;
            if (ndsFighterDLDrawEdge(x0, y0, x1, y1, x2, y2) == 0)
            {
                s32 cx = (x0 + x1 + x2) / 3;
                s32 cy = (y0 + y1 + y2) / 3;

                ndsFighterDLDrawTriangle(pixels, pitch,
                                         cx - 4, cy - 3,
                                         cx + 4, cy - 3,
                                         cx, cy + 4,
                                         fill, edge, &pixel_count);
                marker_drawn = 1u;
                x0 = cx - 4;
                y0 = cy - 3;
                x1 = cx + 4;
                y1 = cy - 3;
                x2 = cx;
                y2 = cy + 4;
            }
            else
            {
                ndsFighterDLDrawTriangle(pixels, pitch,
                                         x0, y0, x1, y1, x2, y2,
                                         fill, edge, &pixel_count);
            }
            if (pixel_count != before)
            {
                drawn_count++;
                if (marker_drawn != 0u)
                {
                    marker_drawn_count++;
                }
                else
                {
                    real_drawn_count++;
                }
                state_drawn = 1u;
                ndsFighterDLMultiDrawRecordScreenPoint(
                    slot, x0, y0, &screen_valid, &screen_min_x,
                    &screen_max_x, &screen_min_y, &screen_max_y);
                ndsFighterDLMultiDrawRecordScreenPoint(
                    slot, x1, y1, &screen_valid, &screen_min_x,
                    &screen_max_x, &screen_min_y, &screen_max_y);
                ndsFighterDLMultiDrawRecordScreenPoint(
                    slot, x2, y2, &screen_valid, &screen_min_x,
                    &screen_max_x, &screen_min_y, &screen_max_y);
            }
        }
        if (state_drawn != 0u)
        {
            drawn_dobj_count++;
        }
    }

    if (slot == 0u)
    {
        gNdsFighterDLMultiDrawP0Axis = best_axis;
        gNdsFighterDLMultiDrawP0Area = best_area;
        gNdsFighterDLMultiDrawP0MinA = min_a;
        gNdsFighterDLMultiDrawP0MaxA = max_a;
        gNdsFighterDLMultiDrawP0MinB = min_b;
        gNdsFighterDLMultiDrawP0MaxB = max_b;
        gNdsFighterDLMultiDrawP0ScreenMinX = screen_min_x;
        gNdsFighterDLMultiDrawP0ScreenMaxX = screen_max_x;
        gNdsFighterDLMultiDrawP0ScreenMinY = screen_min_y;
        gNdsFighterDLMultiDrawP0ScreenMaxY = screen_max_y;
        gNdsFighterDLMultiDrawP0PixelCount = pixel_count;
        gNdsFighterDLMultiDrawP0TriangleDrawnCount = drawn_count;
        gNdsFighterDLMultiDrawP0RealTriangleDrawnCount = real_drawn_count;
        gNdsFighterDLMultiDrawP0MarkerTriangleDrawnCount = marker_drawn_count;
        gNdsFighterDLMultiDrawP0DrawnDObjCount = drawn_dobj_count;
    }
    else
    {
        gNdsFighterDLMultiDrawP1Axis = best_axis;
        gNdsFighterDLMultiDrawP1Area = best_area;
        gNdsFighterDLMultiDrawP1MinA = min_a;
        gNdsFighterDLMultiDrawP1MaxA = max_a;
        gNdsFighterDLMultiDrawP1MinB = min_b;
        gNdsFighterDLMultiDrawP1MaxB = max_b;
        gNdsFighterDLMultiDrawP1ScreenMinX = screen_min_x;
        gNdsFighterDLMultiDrawP1ScreenMaxX = screen_max_x;
        gNdsFighterDLMultiDrawP1ScreenMinY = screen_min_y;
        gNdsFighterDLMultiDrawP1ScreenMaxY = screen_max_y;
        gNdsFighterDLMultiDrawP1PixelCount = pixel_count;
        gNdsFighterDLMultiDrawP1TriangleDrawnCount = drawn_count;
        gNdsFighterDLMultiDrawP1RealTriangleDrawnCount = real_drawn_count;
        gNdsFighterDLMultiDrawP1MarkerTriangleDrawnCount = marker_drawn_count;
        gNdsFighterDLMultiDrawP1DrawnDObjCount = drawn_dobj_count;
    }
}

static void ndsFighterDLMultiDrawAccumulateStats(
    u32 slot, u32 selected_index, const NDSFighterDLDrawState *state,
    const NDSRendererStats *stats, u8 *clean)
{
    u32 blocker = (stats != NULL) ? stats->blocker : 0xffffffffu;
    u32 unsupported_opcode = 0u;
    u32 unsupported_count = 0u;
    u32 clean_selected;

    if ((state == NULL) || (stats == NULL) || (clean == NULL))
    {
        return;
    }

    unsupported_opcode = (stats->unsupported_opcode != 0u) ?
        stats->unsupported_opcode : state->unsupported_opcode;
    unsupported_count = stats->unsupported_command_count +
        state->unsupported_command_count;
    clean_selected =
        (blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (unsupported_opcode == 0u) &&
        (unsupported_count == 0u) &&
        (state->vertex_range_reject_count == 0u) &&
        (state->vertex_decoded_count != 0u) &&
        (state->triangle_valid_count != 0u);
    clean[selected_index] = (u8)clean_selected;

    if (slot == 0u)
    {
        gNdsFighterDLMultiDrawP0AttemptCount++;
        if (clean_selected != FALSE)
        {
            gNdsFighterDLMultiDrawP0CleanCount++;
        }
        else
        {
            gNdsFighterDLMultiDrawP0FailedCount++;
        }
        if ((blocker != NDS_RENDERER_BLOCKER_NONE) &&
            (gNdsFighterDLMultiDrawP0FirstBlocker == 0u))
        {
            gNdsFighterDLMultiDrawP0FirstBlocker = blocker;
        }
        if (blocker != NDS_RENDERER_BLOCKER_NONE)
        {
            gNdsFighterDLMultiDrawP0BlockerMask |=
                1u << (selected_index & 31u);
        }
        gNdsFighterDLMultiDrawP0CommandCount += stats->command_count;
        if (gNdsFighterDLMultiDrawP0FirstOpcode == 0u)
        {
            gNdsFighterDLMultiDrawP0FirstOpcode = stats->first_opcode;
        }
        if ((unsupported_opcode != 0u) &&
            (gNdsFighterDLMultiDrawP0UnsupportedOpcode == 0u))
        {
            gNdsFighterDLMultiDrawP0UnsupportedOpcode = unsupported_opcode;
        }
        gNdsFighterDLMultiDrawP0UnsupportedCommandCount +=
            unsupported_count;
        gNdsFighterDLMultiDrawP0VertexDecodedCount +=
            state->vertex_decoded_count;
        gNdsFighterDLMultiDrawP0TriangleCount += state->triangle_count;
        gNdsFighterDLMultiDrawP0TriangleValidCount +=
            state->triangle_valid_count;
        gNdsFighterDLMultiDrawP0ColorChecksum =
            (gNdsFighterDLMultiDrawP0ColorChecksum * 33u) ^
            state->color_checksum;
    }
    else if (slot == 1u)
    {
        gNdsFighterDLMultiDrawP1AttemptCount++;
        if (clean_selected != FALSE)
        {
            gNdsFighterDLMultiDrawP1CleanCount++;
        }
        else
        {
            gNdsFighterDLMultiDrawP1FailedCount++;
        }
        if ((blocker != NDS_RENDERER_BLOCKER_NONE) &&
            (gNdsFighterDLMultiDrawP1FirstBlocker == 0u))
        {
            gNdsFighterDLMultiDrawP1FirstBlocker = blocker;
        }
        if (blocker != NDS_RENDERER_BLOCKER_NONE)
        {
            gNdsFighterDLMultiDrawP1BlockerMask |=
                1u << (selected_index & 31u);
        }
        gNdsFighterDLMultiDrawP1CommandCount += stats->command_count;
        if (gNdsFighterDLMultiDrawP1FirstOpcode == 0u)
        {
            gNdsFighterDLMultiDrawP1FirstOpcode = stats->first_opcode;
        }
        if ((unsupported_opcode != 0u) &&
            (gNdsFighterDLMultiDrawP1UnsupportedOpcode == 0u))
        {
            gNdsFighterDLMultiDrawP1UnsupportedOpcode = unsupported_opcode;
        }
        gNdsFighterDLMultiDrawP1UnsupportedCommandCount +=
            unsupported_count;
        gNdsFighterDLMultiDrawP1VertexDecodedCount +=
            state->vertex_decoded_count;
        gNdsFighterDLMultiDrawP1TriangleCount += state->triangle_count;
        gNdsFighterDLMultiDrawP1TriangleValidCount +=
            state->triangle_valid_count;
        gNdsFighterDLMultiDrawP1ColorChecksum =
            (gNdsFighterDLMultiDrawP1ColorChecksum * 33u) ^
            state->color_checksum;
    }

    gNdsFighterDLMultiDrawVertexRangeRejectCount +=
        state->vertex_range_reject_count;
}

static void ndsFighterMarioFoxDLMultiDrawForSlot(u32 slot, FTStruct *fp,
                                                 u16 *pixels, u32 pitch)
{
    DObj *root;
    NDSFighterDLMultiDrawCollection collection;
    NDSFighterDLDrawState states[
        NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED];
    NDSFighterDLDrawState persistent_state;
    NDSRendererStats stats;
    NDSRendererStats persistent_stats;
    u8 clean[NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED];
    u32 root_x_before;
    u32 root_x_after;
    u32 i;

    if ((slot > 1u) || (pixels == NULL) ||
        (ndsFighterStructIsTrackedPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL) ||
        (fp->status_id != nFTCommonStatusWait) ||
        (fp->motion_id != nFTCommonMotionWait) ||
        (fp->ga != nMPKineticsGround))
    {
        return;
    }

    root = fp->joints[nFTPartsJointTopN];
    root_x_before = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) :
        0u;

    ndsFighterCollectDObjsWithDL(root, &collection);
    if (slot == 0u)
    {
        gNdsFighterDLMultiDrawP0CandidateCount = collection.total_count;
        gNdsFighterDLMultiDrawP0SelectedCount = collection.selected_count;
        gNdsFighterDLMultiDrawP0SelectedIndexMask =
            collection.selected_index_mask;
    }
    else
    {
        gNdsFighterDLMultiDrawP1CandidateCount = collection.total_count;
        gNdsFighterDLMultiDrawP1SelectedCount = collection.selected_count;
        gNdsFighterDLMultiDrawP1SelectedIndexMask =
            collection.selected_index_mask;
    }

    bzero(states, sizeof(states));
    bzero(&persistent_state, sizeof(persistent_state));
    bzero(&stats, sizeof(stats));
    ndsRendererInitStats(&persistent_stats);
    bzero(clean, sizeof(clean));

    for (i = 0u; i < collection.selected_count; i++)
    {
        const Gfx *dl = collection.dobjs[i]->dl;
        NDSRelocLoadedFile *loaded =
            ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
        NDSRendererConfig config = {0};
        NDSRendererMatrix20p12 initial_projection;
        NDSRendererMatrix20p12 initial_modelview;
        const NDSRendererMatrix20p12 *initial_projection_ptr;
        const NDSRendererMatrix20p12 *initial_modelview_ptr;
#if NDS_RENDERER_HW_TRIANGLES
        void *saved_graphics_heap_ptr;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        u32 step_start;
#endif
#endif

        if ((loaded == NULL) &&
            (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) == FALSE))
        {
            continue;
        }

        states[i].primary_file = loaded;
        states[i].slot = slot;
        ndsFighterDLDrawSeedPersistentState(&states[i],
                                            &persistent_state);
#if NDS_RENDERER_HW_TRIANGLES
        saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        step_start = cpuGetTiming();
#endif
        ndsRendererAdapterPrepareMaterialSegment(collection.dobjs[i],
                                                 &states[i]);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileMaterialTicks += cpuGetTiming() - step_start;
        step_start = cpuGetTiming();
#endif
#endif
        ndsRendererAdapterPrepareInitialMatrices(collection.dobjs[i],
                                                 (gGCCurrentCamera != NULL) ?
                                                     CObjGetStruct(
                                                         gGCCurrentCamera) :
                                                     NULL,
                                                 FALSE,
                                                 &initial_projection,
                                                 &initial_projection_ptr,
                                                 &initial_modelview,
                                                 &initial_modelview_ptr);
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
        gNdsRendererProfileMatrixTicks += cpuGetTiming() - step_start;
#endif
        config.max_depth = 8u;
        config.max_commands = 2048u;
        config.max_list_commands = 512u;
        config.initial_projection = initial_projection_ptr;
        config.initial_modelview = initial_modelview_ptr;
        config.initial_geometry_mode = 0u;
        config.texture_data_layout =
            NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
        config.validate_range = ndsFighterDLMultiDrawValidateRange;
        config.immutable_command_span =
            ndsRendererAdapterImmutableCommandSpan;
        config.resolve_branch = ndsFighterDLMultiDrawResolveBranch;
        config.resolve_data = ndsFighterDLDrawResolveRendererData;
        config.user = &states[i];

        ndsRendererInitStats(&stats);
        ndsFighterDLDrawCopyPersistentRendererState(&stats,
                                                    &persistent_stats);
        ndsRendererExecuteDisplayList(dl,
                                      &config,
                                      ndsFighterMarioFoxVisitDLDrawCommand,
                                      &states[i],
                                      &stats);
#if NDS_RENDERER_HW_TRIANGLES
        gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
#endif
        ndsFighterDLDrawCapturePersistentState(&persistent_state,
                                               &states[i]);
        ndsFighterDLDrawCopyPersistentRendererState(&persistent_stats,
                                                    &stats);
        ndsFighterDLMultiDrawAccumulateStats(slot, i, &states[i],
                                             &stats, clean);
    }

    ndsFighterDLMultiDrawRasterizeStates(slot, states, clean,
                                         collection.selected_count,
                                         pixels,
                                         pitch);

    root_x_after = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) :
        0u;
    if (slot == 0u)
    {
        gNdsFighterDLMultiDrawP0StatusAfter = (u32)fp->status_id;
        gNdsFighterDLMultiDrawP0MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLMultiDrawP0GAAfter = (u32)fp->ga;
        gNdsFighterDLMultiDrawP0RootXBeforeBits = root_x_before;
        gNdsFighterDLMultiDrawP0RootXAfterBits = root_x_after;
    }
    else
    {
        gNdsFighterDLMultiDrawP1StatusAfter = (u32)fp->status_id;
        gNdsFighterDLMultiDrawP1MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLMultiDrawP1GAAfter = (u32)fp->ga;
        gNdsFighterDLMultiDrawP1RootXBeforeBits = root_x_before;
        gNdsFighterDLMultiDrawP1RootXAfterBits = root_x_after;
    }

    gNdsFighterMarioFoxDLMultiDrawCount++;
}

static void ndsFighterMarioFoxRunDLMultiDrawProbe(void)
{
    u32 mask = 0u;
    u32 gobj_before;
    u32 gobj_after;
    u32 pitch = 0u;
    u16 *pixels;

    if ((ndsFighterMarioFoxDLMultiDrawProofEnabled() == FALSE) ||
        (gNdsFighterMarioFoxDLMultiDrawResult != 0u))
    {
        return;
    }

    if (
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
        (gNdsFighterManagerResult == NDS_FIGHTER_MANAGER_PASS) &&
        ((gNdsFighterManagerWaitMask & 0x3u) == 0x3u)
#else
        (gNdsFighterMarioFoxDLDrawResult ==
            NDS_FIGHTER_MARIOFOX_DL_DRAW_PASS) &&
        (gNdsFighterMarioFoxDLDrawSafeResult ==
            NDS_FIGHTER_MARIOFOX_DL_DRAW_SAFE_PASS) &&
        ((gNdsFighterMarioFoxDLDrawMask & 0x7ffu) == 0x7ffu) &&
        (gNdsFighterMarioFoxDLDrawCount == 2u) &&
        (gNdsFighterDLDrawP0Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLDrawP1Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLDrawP0UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLDrawP1UnsupportedCommandCount == 0u)
#endif
        )
    {
        mask |= 1u << 0;
    }
    else
    {
        gNdsFighterMarioFoxDLMultiDrawMask = mask;
        return;
    }

    gNdsFighterDLMultiDrawPreviewCommitBefore =
        gNdsOriginalDLPreviewCommitCount;
    pixels = ndsPlatformBeginOriginalDLPreview(
        NDS_FIGHTER_DL_MULTI_DRAW_WIDTH,
        NDS_FIGHTER_DL_MULTI_DRAW_HEIGHT,
        &pitch);
    if (pixels != NULL)
    {
        gNdsFighterDLMultiDrawPreviewWidth =
            NDS_FIGHTER_DL_MULTI_DRAW_WIDTH;
        gNdsFighterDLMultiDrawPreviewHeight =
            NDS_FIGHTER_DL_MULTI_DRAW_HEIGHT;
        gNdsFighterDLMultiDrawPreviewPitch = pitch;
        mask |= 1u << 1;
    }
    else
    {
        gNdsFighterMarioFoxDLMultiDrawMask = mask;
        return;
    }

    gobj_before = (u32)gcGetGObjsActiveNum();
    ndsFighterMarioFoxDLMultiDrawForSlot(
        0u, ndsFighterMarioFoxProofStructForSlot(0u), pixels, pitch);
    ndsFighterMarioFoxDLMultiDrawForSlot(
        1u, ndsFighterMarioFoxProofStructForSlot(1u), pixels, pitch);
    if (gNdsFighterMarioFoxDLMultiDrawCount == 2u)
    {
        mask |= 1u << 2;
    }
    if ((gNdsFighterDLMultiDrawP0CandidateCount == 14u) &&
        (gNdsFighterDLMultiDrawP1CandidateCount == 18u) &&
        (gNdsFighterDLMultiDrawP0SelectedCount ==
            NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED) &&
        (gNdsFighterDLMultiDrawP1SelectedCount ==
            NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED) &&
        (gNdsFighterDLMultiDrawP0SelectedIndexMask != 0u) &&
        (gNdsFighterDLMultiDrawP1SelectedIndexMask != 0u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterDLMultiDrawP0AttemptCount == 4u) &&
        (gNdsFighterDLMultiDrawP1AttemptCount == 4u) &&
        (gNdsFighterDLMultiDrawP0CleanCount == 4u) &&
        (gNdsFighterDLMultiDrawP1CleanCount == 4u) &&
        (gNdsFighterDLMultiDrawP0FailedCount == 0u) &&
        (gNdsFighterDLMultiDrawP1FailedCount == 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterDLMultiDrawP0VertexDecodedCount >
            gNdsFighterDLDrawP0VertexDecodedCount) &&
        (gNdsFighterDLMultiDrawP1VertexDecodedCount >
            gNdsFighterDLDrawP1VertexDecodedCount) &&
        (gNdsFighterDLMultiDrawP0TriangleCount >
            gNdsFighterDLDrawP0TriangleCount) &&
        (gNdsFighterDLMultiDrawP1TriangleCount >
            gNdsFighterDLDrawP1TriangleCount) &&
        (gNdsFighterDLMultiDrawP0TriangleValidCount >
            gNdsFighterDLDrawP0TriangleValidCount) &&
        (gNdsFighterDLMultiDrawP1TriangleValidCount >
            gNdsFighterDLDrawP1TriangleValidCount) &&
        (gNdsFighterDLMultiDrawP0RealTriangleDrawnCount >
            gNdsFighterDLDrawP0RealTriangleDrawnCount) &&
        (gNdsFighterDLMultiDrawP1RealTriangleDrawnCount >
            gNdsFighterDLDrawP1RealTriangleDrawnCount))
    {
        mask |= 1u << 5;
    }

    gNdsFighterDLMultiDrawTotalPixelCount =
        gNdsFighterDLMultiDrawP0PixelCount +
        gNdsFighterDLMultiDrawP1PixelCount;
    if ((gNdsFighterDLMultiDrawP0PixelCount >=
            gNdsFighterDLDrawP0PixelCount) &&
        (gNdsFighterDLMultiDrawP1PixelCount >=
            gNdsFighterDLDrawP1PixelCount) &&
        (gNdsFighterDLMultiDrawTotalPixelCount >=
            gNdsFighterDLDrawTotalPixelCount) &&
        (gNdsFighterDLMultiDrawP0ColorChecksum != 0u) &&
        (gNdsFighterDLMultiDrawP1ColorChecksum != 0u))
    {
        mask |= 1u << 6;
    }
    if (gNdsFighterDLMultiDrawTotalPixelCount > 0u)
    {
        ndsPlatformCommitOriginalDLPreview();
        gNdsFighterDLMultiDrawPreviewCommitAfter =
            gNdsOriginalDLPreviewCommitCount;
        gNdsFighterDLMultiDrawPreviewCommitDelta =
            gNdsFighterDLMultiDrawPreviewCommitAfter -
            gNdsFighterDLMultiDrawPreviewCommitBefore;
        gNdsFighterDLMultiDrawPreviewReady = gNdsOriginalDLPreviewReady;
    }
    if ((gNdsFighterDLMultiDrawPreviewReady != 0u) &&
        (gNdsFighterDLMultiDrawPreviewCommitDelta == 1u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterDLMultiDrawP0FirstBlocker ==
            NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLMultiDrawP1FirstBlocker ==
            NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLMultiDrawP0BlockerMask == 0u) &&
        (gNdsFighterDLMultiDrawP1BlockerMask == 0u) &&
        (gNdsFighterDLMultiDrawP0UnsupportedOpcode == 0u) &&
        (gNdsFighterDLMultiDrawP1UnsupportedOpcode == 0u) &&
        (gNdsFighterDLMultiDrawP0UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLMultiDrawP1UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLMultiDrawRangeRejectCount == 0u) &&
        (gNdsFighterDLMultiDrawVertexRangeRejectCount == 0u))
    {
        mask |= 1u << 8;
    }

    gobj_after = (u32)gcGetGObjsActiveNum();
    gNdsFighterDLMultiDrawGObjDelta =
        (gobj_after >= gobj_before) ? (gobj_after - gobj_before) :
            (gobj_before - gobj_after);
    if ((gNdsFighterDLMultiDrawP0StatusAfter ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterDLMultiDrawP1StatusAfter ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterDLMultiDrawP0MotionAfter ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterDLMultiDrawP1MotionAfter ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterDLMultiDrawP0GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterDLMultiDrawP1GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterDLMultiDrawP0RootXBeforeBits ==
            gNdsFighterDLMultiDrawP0RootXAfterBits) &&
        (gNdsFighterDLMultiDrawP1RootXBeforeBits ==
            gNdsFighterDLMultiDrawP1RootXAfterBits) &&
        (gNdsFighterDLMultiDrawGObjDelta == 0u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsFighterDLMultiDrawDrawCallCount == 0u) &&
        (gNdsFighterDLMultiDrawMatrixCallCount == 0u) &&
        (gNdsFighterDLMultiDrawGameplayUpdateCount == 0u) &&
        (gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
        (gSCManagerSceneData.scene_prev == nSCKindMaps))
    {
        mask |= 1u << 10;
    }

    gNdsFighterMarioFoxDLMultiDrawMask = mask;
    gNdsFighterMarioFoxDLMultiDrawDeferredMask = 0xffu;
    if ((mask & 0x7ffu) == 0x7ffu)
    {
        gNdsFighterMarioFoxDLMultiDrawResult =
            NDS_FIGHTER_MARIOFOX_DL_MULTI_DRAW_PASS;
        gNdsFighterMarioFoxDLMultiDrawSafeResult =
            NDS_FIGHTER_MARIOFOX_DL_MULTI_DRAW_SAFE_PASS;
    }
}

typedef struct NDSFighterDLAllDrawCollection {
    DObj *dobjs[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    u32 indices[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    u32 total_count;
    u32 selected_count;
    u32 selected_index_mask;
} NDSFighterDLAllDrawCollection;

#if NDS_TICK_HUD
/* CYCLE 98 -- does the DObj walk rebuild the same collection every frame?
 *
 * The board's seam table calls the walk "match-load constant for Mario/Fox --
 * poses move, topology does not", and flags that it needs proof no status or
 * motion alters the DObj set. This is that proof, and it costs one u32 of state
 * per slot: hash the collection's identity and compare it with the previous
 * frame's hash for the same slot.
 *
 * The dl pointer is in the hash deliberately. A collection whose DObj set is
 * unchanged but whose display lists have been re-pointed is NOT a constant for
 * any purpose a baked collection order would serve, and hashing only the DObj
 * pointers would report it as one. */
static u32 sNdsFtrPreWalkHash[GMCOMMON_PLAYERS_MAX];
static u32 sNdsFtrPreWalkSeen[GMCOMMON_PLAYERS_MAX];

static void ndsFtrPreWalkCensus(
    u32 slot, const NDSFighterDLAllDrawCollection *collection)
{
    u32 hash = 2166136261u;
    u32 i;

    if ((slot >= GMCOMMON_PLAYERS_MAX) || (collection == NULL))
    {
        return;
    }
#define NDS_FTR_PRE_MIX(v) \
    do { hash ^= (u32)(v); hash *= 16777619u; } while (0)
    NDS_FTR_PRE_MIX(collection->total_count);
    NDS_FTR_PRE_MIX(collection->selected_count);
    NDS_FTR_PRE_MIX(collection->selected_index_mask);
    for (i = 0u; (i < collection->selected_count) &&
                 (i < NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED); i++)
    {
        const DObj *dobj = collection->dobjs[i];

        NDS_FTR_PRE_MIX(collection->indices[i]);
        NDS_FTR_PRE_MIX((uintptr_t)dobj);
        NDS_FTR_PRE_MIX((dobj != NULL) ? (uintptr_t)dobj->dl : 0u);
    }
#undef NDS_FTR_PRE_MIX
    if (sNdsFtrPreWalkSeen[slot] == 0u)
    {
        gNdsFtrPreWalkFirst++;
    }
    else if (sNdsFtrPreWalkHash[slot] == hash)
    {
        gNdsFtrPreWalkSame++;
    }
    else
    {
        gNdsFtrPreWalkVariant++;
    }
    sNdsFtrPreWalkHash[slot] = hash;
    sNdsFtrPreWalkSeen[slot] = 1u;
}
#endif

#define NDS_FIGHTER_DL_ALL_FAIL_BLOCKER 0x1u
#define NDS_FIGHTER_DL_ALL_FAIL_UNSUPPORTED_OPCODE 0x2u
#define NDS_FIGHTER_DL_ALL_FAIL_UNSUPPORTED_COUNT 0x4u
#define NDS_FIGHTER_DL_ALL_FAIL_VERTEX_RANGE 0x8u
#define NDS_FIGHTER_DL_ALL_FAIL_NO_VERTS 0x10u
#define NDS_FIGHTER_DL_ALL_FAIL_NO_VALID_TRIS 0x20u
#define NDS_FIGHTER_DL_ALL_FAIL_UNKNOWN 0x80000000u

static void ndsFighterCollectAllDObjsWithDLRecursive(
    DObj *dobj, NDSFighterDLAllDrawCollection *collection,
    u32 *traversal_index)
{
    while (dobj != NULL)
    {
        u32 current_index = (traversal_index != NULL) ? *traversal_index : 0u;

        if (traversal_index != NULL)
        {
            (*traversal_index)++;
        }

        if ((collection != NULL) && (dobj->dl != NULL))
        {
            if (collection->selected_count <
                NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED)
            {
                u32 selected = collection->selected_count;
                collection->dobjs[selected] = dobj;
                collection->indices[selected] = current_index;
                collection->selected_count++;
                if (current_index < 32u)
                {
                    collection->selected_index_mask |= 1u << current_index;
                }
            }
            collection->total_count++;
        }

        ndsFighterCollectAllDObjsWithDLRecursive(dobj->child,
                                                 collection,
                                                 traversal_index);
        dobj = dobj->sib_next;
    }
}

/* Four pointers, 16 bytes, and nothing else.
 *
 * This used to carry the render preamble too -- geometry mode, cycle type,
 * render mode, prim and env colour, the Light and its valid flag -- which made
 * it 56 bytes and put its two halves on different cache lines. The halves have
 * opposite access patterns: the pointers are read by three tight per-root loops
 * in the same pass that walks the collection, while the preamble is read once
 * per root by ndsRendererAdapterBuildNativeProductionInputs, a pass later,
 * after the matrix and material work has evicted it. c106 priced that eviction
 * at 2,966 ticks/frame on `root->preamble.geometry_mode = event->geometry_mode`
 * and 1,759 on `if (event->light_valid)`: ~110 cycles an event of pure miss.
 *
 * So the preamble moved to its own array, in the consumer's own struct layout,
 * written by the producer. 32 events is 512 bytes of pointers plus 768 of
 * preamble instead of 1,792 bytes interleaved, and the consumer's read is one
 * 24-byte struct copy out of a dense array rather than a field-by-field gather
 * across two cold lines. */
typedef struct NDSFighterDisplayContractEvent {
    DObj *dobj;
    DObj *matrix_dobj;
    DObj *material_dobj;
    const Gfx *dl;
} NDSFighterDisplayContractEvent;

#define NDS_FIGHTER_DISPLAY_CYCLETYPE_MASK (3u << 20)

typedef struct NDSFighterDisplayContract {
    NDSFighterDisplayContractEvent events[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    Gfx scratch[4][128];
    Gfx *saved_heads[4];
    void *saved_graphics_heap_ptr;
    DObj *current_dobj;
    DObj *material_dobj;
    s32 pending_event;
    u32 event_count;
    u32 geometry_mode;
    u32 cycle_type;
    u32 render_mode;
    u32 prim_color;
    u32 env_color;
    u32 light_count;
    Light light;
    u32 light_valid;
    u32 active;
    u32 matrix_ready;
    u32 material_ready;
} NDSFighterDisplayContract;

static NDSFighterDisplayContract sNdsFighterDisplayContract;
/* Parallel to sNdsFighterDisplayContract.events, in the consumer's layout. */
static NDSRendererNativeFighterPreamble sNdsFighterDisplayContractPreambles[
    NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
/* Playback normally consumes the freshly captured arrays above. A draw-memo
 * hit instead points these two views straight at the immutable per-slot memo.
 * That removes the old hit-side event+preamble memcpy (up to 1,280 B per
 * fighter draw) without changing the source capture, key, invalidation, or
 * event order. Capture writers never use these views. */
static const NDSFighterDisplayContractEvent *sNdsFighterDisplayReplayEvents =
    sNdsFighterDisplayContract.events;
static const NDSRendererNativeFighterPreamble *
    sNdsFighterDisplayReplayPreambles = sNdsFighterDisplayContractPreambles;
/* What a root points at when there is no contract event behind it. The roots
 * hold the preamble by reference, so the no-event case needs somewhere real to
 * point rather than a zeroed inline copy; its cleared VALID bit is what the
 * backend's preflight rejects, exactly as before. */
static const NDSRendererNativeFighterPreamble sNdsRendererAdapterZeroPreamble;
static sb32 sNdsFighterDisplayContractPlayback;
static u32 sNdsFighterDisplayContractLastFrame[GMCOMMON_PLAYERS_MAX] = {
    0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu
};

#if NDS_R2_FTR_CONTRACT_CENSUS
/* 2026-08-16, the discriminating measurement for FTR_LANE.md section 5.
 *
 * The capture pass re-derives this contract from BattleShip's own display code
 * every frame for both fighters and costs 34,307 tk/fr. That figure is a
 * CEILING: a memo only pays for the frames on which the contract is actually
 * unchanged, and nobody had measured that rate. These hashes measure it,
 * against each slot's own previous capture:
 *
 *   CNT   event_count
 *   DOBJ  every event's dobj / matrix_dobj / material_dobj  (tree structure)
 *   DL    every event's dl                                  (model part swaps)
 *   PRE   every preamble                                    (material state)
 *   KEY   a CANDIDATE memo key, accumulated by the tree walk that already runs
 *         in ndsFighterDisplayContractCountFlags: per DObj, its flags, dl, dv,
 *         dls and dls[0..1], plus its FTParts flags. Those are exactly what
 *         ftdisplaymain.c:753-841 branches on, so a key-guarded memo would use
 *         this. It deliberately does NOT cover the fp-level state that decides
 *         the preamble, so KEY-same/contract-different is a real possibility
 *         and is counted -- that column is the soundness answer, not a
 *         formality. */
#define NDS_FTR_CONTRACT_H_CNT 0
#define NDS_FTR_CONTRACT_H_DOBJ 1
#define NDS_FTR_CONTRACT_H_DL 2
#define NDS_FTR_CONTRACT_H_PRE 3
#define NDS_FTR_CONTRACT_H_KEY 4
#define NDS_FTR_CONTRACT_H_COUNT 5
#define NDS_FTR_CONTRACT_HASH_SEED 2166136261u

static u32 sNdsFtrContractCensusPrev[GMCOMMON_PLAYERS_MAX]
                                     [NDS_FTR_CONTRACT_H_COUNT];
static u32 sNdsFtrContractCensusSeen[GMCOMMON_PLAYERS_MAX];
static u32 sNdsFtrContractCensusRun[GMCOMMON_PLAYERS_MAX];
static u32 sNdsFtrContractCensusKey = NDS_FTR_CONTRACT_HASH_SEED;

static u32 ndsFtrContractCensusMix(u32 hash, u32 word)
{
    return (hash ^ word) * 16777619u;
}

static void ndsFtrContractCensusRecord(u32 slot)
{
    u32 h[NDS_FTR_CONTRACT_H_COUNT];
    u32 count = sNdsFighterDisplayContract.event_count;
    u32 i;
    u32 same_all;

    if (slot >= GMCOMMON_PLAYERS_MAX)
    {
        return;
    }
    h[NDS_FTR_CONTRACT_H_CNT] = count;
    h[NDS_FTR_CONTRACT_H_DOBJ] = NDS_FTR_CONTRACT_HASH_SEED;
    h[NDS_FTR_CONTRACT_H_DL] = NDS_FTR_CONTRACT_HASH_SEED;
    h[NDS_FTR_CONTRACT_H_PRE] = NDS_FTR_CONTRACT_HASH_SEED;
    h[NDS_FTR_CONTRACT_H_KEY] = sNdsFtrContractCensusKey;
    for (i = 0u; i < count; i++)
    {
        const NDSFighterDisplayContractEvent *ev =
            &sNdsFighterDisplayContract.events[i];
        const u32 *pre = (const u32 *)(const void *)
            &sNdsFighterDisplayContractPreambles[i];
        u32 w;

        h[NDS_FTR_CONTRACT_H_DOBJ] = ndsFtrContractCensusMix(
            h[NDS_FTR_CONTRACT_H_DOBJ], (u32)(size_t)ev->dobj);
        h[NDS_FTR_CONTRACT_H_DOBJ] = ndsFtrContractCensusMix(
            h[NDS_FTR_CONTRACT_H_DOBJ], (u32)(size_t)ev->matrix_dobj);
        h[NDS_FTR_CONTRACT_H_DOBJ] = ndsFtrContractCensusMix(
            h[NDS_FTR_CONTRACT_H_DOBJ], (u32)(size_t)ev->material_dobj);
        h[NDS_FTR_CONTRACT_H_DL] = ndsFtrContractCensusMix(
            h[NDS_FTR_CONTRACT_H_DL], (u32)(size_t)ev->dl);
        for (w = 0u;
             w < (sizeof(NDSRendererNativeFighterPreamble) / sizeof(u32));
             w++)
        {
            h[NDS_FTR_CONTRACT_H_PRE] = ndsFtrContractCensusMix(
                h[NDS_FTR_CONTRACT_H_PRE], pre[w]);
        }
    }
    if (sNdsFtrContractCensusSeen[slot] != 0u)
    {
        u32 key_same =
            (h[NDS_FTR_CONTRACT_H_KEY] ==
             sNdsFtrContractCensusPrev[slot][NDS_FTR_CONTRACT_H_KEY]) ? 1u : 0u;

        same_all =
            ((h[NDS_FTR_CONTRACT_H_CNT] ==
              sNdsFtrContractCensusPrev[slot][NDS_FTR_CONTRACT_H_CNT]) &&
             (h[NDS_FTR_CONTRACT_H_DOBJ] ==
              sNdsFtrContractCensusPrev[slot][NDS_FTR_CONTRACT_H_DOBJ]) &&
             (h[NDS_FTR_CONTRACT_H_DL] ==
              sNdsFtrContractCensusPrev[slot][NDS_FTR_CONTRACT_H_DL]) &&
             (h[NDS_FTR_CONTRACT_H_PRE] ==
              sNdsFtrContractCensusPrev[slot][NDS_FTR_CONTRACT_H_PRE])) ?
            1u : 0u;
        gNdsFtrContractCaptures++;
        gNdsFtrContractEventTotal += count;
        if (count == 0u)
        {
            gNdsFtrContractZeroEvents++;
        }
        if (h[NDS_FTR_CONTRACT_H_CNT] ==
            sNdsFtrContractCensusPrev[slot][NDS_FTR_CONTRACT_H_CNT])
        {
            gNdsFtrContractCountSame++;
        }
        if (h[NDS_FTR_CONTRACT_H_DOBJ] ==
            sNdsFtrContractCensusPrev[slot][NDS_FTR_CONTRACT_H_DOBJ])
        {
            gNdsFtrContractDObjSame++;
        }
        if (h[NDS_FTR_CONTRACT_H_DL] ==
            sNdsFtrContractCensusPrev[slot][NDS_FTR_CONTRACT_H_DL])
        {
            gNdsFtrContractDLSame++;
        }
        if (h[NDS_FTR_CONTRACT_H_PRE] ==
            sNdsFtrContractCensusPrev[slot][NDS_FTR_CONTRACT_H_PRE])
        {
            gNdsFtrContractPreSame++;
        }
        if (key_same != 0u)
        {
            gNdsFtrContractKeySame++;
            if (same_all == 0u)
            {
                gNdsFtrContractKeySameContractDiff++;
            }
        }
        else if (same_all != 0u)
        {
            gNdsFtrContractKeyDiffContractSame++;
        }
        if (same_all != 0u)
        {
            gNdsFtrContractSame++;
            sNdsFtrContractCensusRun[slot]++;
            if (sNdsFtrContractCensusRun[slot] > gNdsFtrContractMaxRun)
            {
                gNdsFtrContractMaxRun = sNdsFtrContractCensusRun[slot];
            }
        }
        else
        {
            gNdsFtrContractChangeTotal++;
            sNdsFtrContractCensusRun[slot] = 0u;
        }
    }
    for (i = 0u; i < (u32)NDS_FTR_CONTRACT_H_COUNT; i++)
    {
        sNdsFtrContractCensusPrev[slot][i] = h[i];
    }
    sNdsFtrContractCensusSeen[slot] = 1u;
}
#endif

static u32 ndsFighterDisplayContractPackColor(u8 r, u8 g, u8 b, u8 a)
{
    return ((u32)r << 24) | ((u32)g << 16) | ((u32)b << 8) | (u32)a;
}

#if NDS_R2_FTR_CONTRACT_CENSUS
/* A recursive walk of both fighters' whole DObj trees, every frame, inside the
 * run's largest lane -- 4,117 tk/fr on build-c220-camship -- whose only outputs
 * are gNdsFighterDisplayContractHiddenCount and ...NoTextureCount. Nothing in
 * the runtime reads either; the only readers are two harness printf lines
 * (verify-battle-mariofox-gcrunall-loop-harness.ps1:2065, probe-ko-vfx.ps1),
 * neither in Boundary. It is now census-only: the census needs the walk for its
 * candidate tree key, and the shipping binary does not run it at all. */
static void ndsFighterDisplayContractCountFlags(DObj *dobj)
{
    while (dobj != NULL)
    {
        {
            const FTParts *parts = ftGetParts(dobj);
            u32 key = sNdsFtrContractCensusKey;

            key = ndsFtrContractCensusMix(key, (u32)(size_t)dobj);
            key = ndsFtrContractCensusMix(key, (u32)dobj->flags);
            key = ndsFtrContractCensusMix(key, (u32)(size_t)dobj->dl);
            key = ndsFtrContractCensusMix(key, (u32)(size_t)dobj->dv);
            key = ndsFtrContractCensusMix(key, (u32)(size_t)dobj->dls);
            if (dobj->dls != NULL)
            {
                key = ndsFtrContractCensusMix(key,
                                              (u32)(size_t)dobj->dls[0]);
                key = ndsFtrContractCensusMix(key,
                                              (u32)(size_t)dobj->dls[1]);
            }
            key = ndsFtrContractCensusMix(
                key, (parts != NULL) ? (u32)parts->flags : 0xffffffffu);
            sNdsFtrContractCensusKey = key;
        }
        if ((dobj->flags & DOBJ_FLAG_HIDDEN) != 0)
        {
            gNdsFighterDisplayContractHiddenCount++;
        }
        if ((dobj->flags & DOBJ_FLAG_NOTEXTURE) != 0)
        {
            gNdsFighterDisplayContractNoTextureCount++;
        }
        ndsFighterDisplayContractCountFlags(dobj->child);
        dobj = dobj->sib_next;
    }
}
#endif

void ndsFighterDisplayContractSetGeometryMode(u32 clear_mask, u32 set_mask)
{
    if (sNdsFighterDisplayContract.active == 0u)
    {
        return;
    }
    sNdsFighterDisplayContract.geometry_mode &= ~clear_mask;
    sNdsFighterDisplayContract.geometry_mode |= set_mask;
    gNdsFighterDisplayContractGeometryMode =
        sNdsFighterDisplayContract.geometry_mode;
}

void ndsFighterDisplayContractSetCycleType(u32 cycle_type)
{
    if (sNdsFighterDisplayContract.active != 0u)
    {
        sNdsFighterDisplayContract.cycle_type = cycle_type;
    }
}

void ndsFighterDisplayContractSetRenderMode(u32 mode1, u32 mode2)
{
    if (sNdsFighterDisplayContract.active != 0u)
    {
        sNdsFighterDisplayContract.render_mode = mode1 | mode2;
    }
}

void ndsFighterDisplayContractSetEnvColor(u8 r, u8 g, u8 b, u8 a)
{
    if (sNdsFighterDisplayContract.active != 0u)
    {
        sNdsFighterDisplayContract.env_color =
            ndsFighterDisplayContractPackColor(r, g, b, a);
    }
}

void ndsFighterDisplayContractSetPrimColor(u8 r, u8 g, u8 b, u8 a)
{
    if (sNdsFighterDisplayContract.active != 0u)
    {
        sNdsFighterDisplayContract.prim_color =
            ndsFighterDisplayContractPackColor(r, g, b, a);
    }
}

void ndsFighterDisplayContractSetLightCount(u32 count)
{
    sNdsFighterDisplayCurrentLightCount = count;
    if (sNdsFighterDisplayContract.active != 0u)
    {
        sNdsFighterDisplayContract.light_count = count;
        gNdsFighterDisplayContractLightCount += count;
    }
}

void ndsFighterDisplayContractSetLight(const Light *light, u32 slot)
{
    if ((light == NULL) || (slot != 1u))
    {
        return;
    }
    sNdsFighterDisplayCurrentLight = *light;
    sNdsFighterDisplayCurrentLightValid = TRUE;
    if (sNdsFighterDisplayContract.active != 0u)
    {
        sNdsFighterDisplayContract.light = *light;
        sNdsFighterDisplayContract.light_valid = TRUE;
        gNdsFighterDisplayContractLightDirectionCount++;
    }
}

void ndsFighterDisplayContractResetSceneLight(void)
{
    sNdsFighterDisplayCurrentLightCount = 0u;
    sNdsFighterDisplayCurrentLightValid = FALSE;
}

u8 ndsFighterDisplayContractSetStageEnvColor(Gfx **dls)
{
    (void)dls;
    /* mpCollisionInitGroundData sets this source color to opaque white. */
    ndsFighterDisplayContractSetEnvColor(0xffu, 0xffu, 0xffu, 0xffu);
    return 0xffu;
}

sb32 ndsFighterDisplayContractCheckTargetInBounds(f32 pos_x, f32 pos_y)
{
    extern sb32 gmCameraCheckTargetInBounds(f32 pos_x, f32 pos_y);
    sb32 is_in_bounds;

    gNdsFighterDisplayContractBoundsXBits = ndsFloatBits(pos_x);
    gNdsFighterDisplayContractBoundsYBits = ndsFloatBits(pos_y);
    is_in_bounds = gmCameraCheckTargetInBounds(pos_x, pos_y);
    if (is_in_bounds != FALSE)
    {
        gNdsFighterDisplayContractBoundsPassCount++;
    }
    else
    {
        gNdsFighterDisplayContractBoundsFailCount++;
    }
    return is_in_bounds;
}

void ndsFighterDisplayContractProjectTarget(CObj *cobj,
                                            Mtx44f matrix,
                                            Vec3f *pos,
                                            f32 *dist_x,
                                            f32 *dist_y)
{
    f32 x;
    f32 y;
    f32 z;
    f32 projected_x;
    f32 projected_y;
    f32 scale;

    if ((cobj == NULL) || (pos == NULL) || (dist_x == NULL) ||
        (dist_y == NULL))
    {
        return;
    }
    /* BattleShip ftparam.c:2421-2439, used by fighter magnify culling. */
    x = pos->x;
    y = pos->y;
    z = pos->z;
    projected_x = ((matrix[0][0] * x) + (matrix[1][0] * y) +
                   (matrix[2][0] * z)) + matrix[3][0];
    projected_y = ((matrix[0][1] * x) + (matrix[1][1] * y) +
                   (matrix[2][1] * z)) + matrix[3][1];
    scale = ((matrix[0][3] * x) + (matrix[1][3] * y) +
             (matrix[2][3] * z)) + matrix[3][3];
    if (ABSF(scale) < 0.1F)
    {
        scale = (scale < 0.0F) ? -0.1F : 0.1F;
    }
    scale = 1.0F / scale;
    *dist_x = (cobj->viewport.vp.vscale[0] / 4) *
              (projected_x * scale);
    *dist_y = (cobj->viewport.vp.vscale[1] / 4) *
              (projected_y * scale);
}

void ndsFighterDisplayContractSelectDL(const Gfx *dl)
{
    NDSFighterDisplayContractEvent *event;

    if ((sNdsFighterDisplayContract.active == 0u) || (dl == NULL) ||
        (sNdsFighterDisplayContract.event_count >=
            NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED))
    {
        return;
    }
    event = &sNdsFighterDisplayContract.events[
        sNdsFighterDisplayContract.event_count++];
    event->dl = dl;
    event->dobj = (sNdsFighterDisplayContract.matrix_ready != 0u) ?
        sNdsFighterDisplayContract.current_dobj : NULL;
    event->matrix_dobj = event->dobj;
    event->material_dobj =
        (sNdsFighterDisplayContract.material_ready != 0u) ?
            sNdsFighterDisplayContract.material_dobj : NULL;
    {
        /* Built here in the consumer's layout, including the flags word the
         * two build sites used to derive. Identical arithmetic, one pass
         * earlier, into a line that is hot because we just wrote the event. */
        NDSRendererNativeFighterPreamble *preamble =
            &sNdsFighterDisplayContractPreambles[
                sNdsFighterDisplayContract.event_count - 1u];
        u32 flags = NDS_RENDERER_NATIVE_PREAMBLE_VALID;

        preamble->geometry_mode = sNdsFighterDisplayContract.geometry_mode;
        preamble->cycle_type = sNdsFighterDisplayContract.cycle_type;
        preamble->render_mode = sNdsFighterDisplayContract.render_mode;
        preamble->prim_color = sNdsFighterDisplayContract.prim_color;
        preamble->env_color = sNdsFighterDisplayContract.env_color;
        if (sNdsFighterDisplayContract.light_valid != 0u)
        {
            preamble->light_dir_x =
                sNdsFighterDisplayContract.light.l.dir[0];
            preamble->light_dir_y =
                sNdsFighterDisplayContract.light.l.dir[1];
            preamble->light_dir_z =
                sNdsFighterDisplayContract.light.l.dir[2];
            flags |= NDS_RENDERER_NATIVE_PREAMBLE_LIGHT_VALID;
        }
        else
        {
            /* An invalid light has to clear these itself or a previous
             * frame's direction survives behind a cleared valid bit. */
            preamble->light_dir_x = 0;
            preamble->light_dir_y = 0;
            preamble->light_dir_z = 0;
        }
        preamble->flags = (u8)flags;
        if (gNdsFighterDisplayContractSelectedCount == 0u)
        {
            /* ftdisplaymain.c:1176-1178 establishes the normal preamble. */
            gNdsFighterDisplayContractCycleType = preamble->cycle_type;
            gNdsFighterDisplayContractRenderMode = preamble->render_mode;
        }
    }
    if (event->dobj == NULL)
    {
        sNdsFighterDisplayContract.pending_event =
            (s32)sNdsFighterDisplayContract.event_count - 1;
    }
    sNdsFighterDisplayContract.matrix_ready = FALSE;
    sNdsFighterDisplayContract.material_ready = FALSE;
    gNdsFighterDisplayContractSelectedCount++;
}

s32 gcPrepDObjMatrix(Gfx **dls, DObj *dobj)
{
    (void)dls;
    if (sNdsFighterDisplayContract.active == 0u)
    {
        return FALSE;
    }
    if ((sNdsFighterDisplayContract.pending_event >= 0) &&
        ((u32)sNdsFighterDisplayContract.pending_event <
            sNdsFighterDisplayContract.event_count))
    {
        NDSFighterDisplayContractEvent *event =
            &sNdsFighterDisplayContract.events[
                sNdsFighterDisplayContract.pending_event];

        event->dobj = dobj;
        event->matrix_dobj =
            (dobj->parent != DOBJ_PARENT_NULL) ? dobj->parent : NULL;
        sNdsFighterDisplayContract.pending_event = -1;
    }
    sNdsFighterDisplayContract.current_dobj = dobj;
    sNdsFighterDisplayContract.matrix_ready = TRUE;
    return FALSE;
}

void gcDrawMObjForDObj(DObj *dobj, Gfx **dls)
{
    (void)dls;
    if (sNdsFighterDisplayContract.active != 0u)
    {
        sNdsFighterDisplayContract.material_dobj = dobj;
        sNdsFighterDisplayContract.material_ready = TRUE;
    }
}

/* ===========================================================================
 * The fighter draw-contract memo (2026-08-16).
 *
 * artifacts/performance/2026-08-16_ftr-capture-memo/CAPTURE_MEMO.md measured the
 * only number that decides whether this pass is memoisable: over a whole
 * canonical match the contract is UNCHANGED on 4,025 of 4,076 captures
 * (98.75%), MaxRun 848, and all 51 changes are decided in the HEAD of
 * ftDisplayMainProcDisplay -- the magnify/invisible early returns and the fp
 * material state -- never in the DObj tree. The same cycle REFUTED the obvious
 * DObj-tree key: it reads "unchanged" on 49 of those 51 frames.
 *
 * So the memo skips the WALK (ftDisplayMainDrawAll -> ftDisplayMainDrawDefault,
 * 19,300 tk/fr inclusive) and keeps the head, which is not read-only: it writes
 * the off-screen player arrow HUD, gLBCommonScale, the fog statics and the
 * scene light twice, and BoundsFailCount=142 proves the magnify branch fires in
 * the canonical match.
 *
 * MECHANISM, and why it is where it is. The decision cannot be taken before the
 * head runs (the key IS the head's output) and it must be taken before the
 * walk, but head and walk are welded inside one decomp function. The
 * preprocessor cannot split a definition from its uses, so a shim rename cannot
 * intercept ftDisplayMainDrawAll's two same-TU call sites, and the Makefile
 * forbids new battleship import-overlay inputs ("New adaptations belong
 * directly in src/import/src/port"). The seam that IS available is the head's
 * last contract-visible action: ftDisplayMainProcDisplay emits exactly one
 * fog colour (ftdisplaymain.c:1211-1213 -> :668 / :676 / :682, the only three
 * gDPSetFogColor sites in the file) immediately before the walk, and
 * gDPSetFogColor is a macro the import shim owns. The shim calls
 * ndsFighterDisplayContractHeadBoundary from it, one-shot per capture.
 *
 * On a hit the walk is collapsed rather than skipped: gNdsFtrDrawMemoSkipRoot
 * is pointed at this fighter's live root DObj, and the shim's DObjGetStruct
 * hands ftDisplayMainDrawAll an empty HIDDEN stub instead. Nothing in the live
 * tree is written, and ftDisplayMainDrawDefault(stub) is a flag test and a
 * NULL sibling test. The cached event list and preambles are then copied back
 * over the (empty) contract before ndsFighterDisplayContractSubmit reads it.
 *
 * SOUNDNESS. The key covers the ordinary per-frame inputs the walk branches on:
 *  - the head's own contract output (geometry mode, cycle type, render mode,
 *    prim/env colour, light + validity + count) -- 51 of 51 measured changes;
 *  - sFTDisplayMainSkyFogAlpha / sFTDisplayMainIsShadeFog, the two head statics
 *    ftDisplayMainDecideFogDraw reads inside the walk;
 *  - the fp fields the walk and ftDisplayMainDrawAll read directly
 *    (colanim.is_use_color1, colanim.skeleton_id, fkind, shade, costume,
 *    detail_curr, lr, display_mode, attr) and the tree root pointer, so a
 *    rebuilt fighter tree invalidates by construction.
 * The per-DObj state the walk also reads -- dobj->dl / dls / dv / flags and
 * FTParts flags -- is deliberately not re-hashed every frame. One source path
 * DOES mutate fighter topology, however, and the original memo missed it:
 * ftMainSetStatus interprets FTAnimDesc's enabled-joint bits by calling
 * ftMainUpdateHiddenPartID / AddHiddenPartID / EjectHiddenPartID. Those calls
 * allocate, detach and re-parent live DObjs while the fighter GObj/root pointer
 * can stay unchanged. Mario/Fox Catch, CatchPull and both Throws all carry
 * 0x10000000, so this is not theoretical -- it is the grab/throw path.
 *
 * Hashing the tree here would put the pointer walk we are deleting back on the
 * hot path. Instead the imported ftMainSetStatus wrapper bumps one per-slot
 * renderer status generation and invalidates this memo immediately after the
 * authoritative source status change. Hidden-part helpers have no other callers
 * in BattleShip, so the cache cannot survive a topology rewrite; the next draw
 * performs the real walk and refills from the new DObjs. This also covers
 * same-status/same-motion re-entry, which a key made only from status_id /
 * motion_id / anim_desc.word would miss.
 * Two states are refused outright rather than keyed: a display_mode other than
 * Master (the MapCollision and hit-outline blocks re-read DObjGetStruct AFTER
 * the walk) and a pending afterimage draw (ftDisplayMainDrawAfterImage runs
 * inside ftDisplayMainDrawAll and builds fresh scratch geometry every frame).
 * ========================================================================= */

#define NDS_FTR_DRAW_MEMO_KEY_WORDS 16u

typedef struct NDSFtrDrawMemoSlot {
    u32 key[NDS_FTR_DRAW_MEMO_KEY_WORDS];
    NDSFighterDisplayContractEvent
        events[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    NDSRendererNativeFighterPreamble
        preambles[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    u32 event_count;
    u32 valid;
} NDSFtrDrawMemoSlot;

static NDSFtrDrawMemoSlot sNdsFtrDrawMemo[GMCOMMON_PLAYERS_MAX];
/* The empty tree the walk gets on a hit. Zeroed: no child, no siblings, no
 * FTParts. Only flags and parent_gobj are ever written. */
static DObj sNdsFtrDrawMemoStub;
/* Published as void* so the import shim can compare without needing DObj to be
 * complete in the header. Disarmed, Skip points at the stub itself, so the
 * shim's compare can never match a live root and needs no NULL case. */
void *gNdsFtrDrawMemoStubRoot = &sNdsFtrDrawMemoStub;
void *gNdsFtrDrawMemoSkipRoot = &sNdsFtrDrawMemoStub;

static FTStruct *sNdsFtrDrawMemoFp;
static GObj *sNdsFtrDrawMemoGObj;
static u32 sNdsFtrDrawMemoSlotIndex;
/* Monotonic renderer-coherency generation. The imported ftMainSetStatus wrapper
 * bumps this after every authoritative source status change. Besides the
 * display-contract memo below, the later Cycle-99 draw plan stores DObj pointers
 * too, so both caches share the same writer-side invalidation generation. */
static u32 sNdsFighterStatusGeneration[GMCOMMON_PLAYERS_MAX];
/* 0 = not a candidate, 1 = armed and awaiting the head boundary, 2 = the
 * boundary fired and the key is built, so the result is cacheable. */
static u32 sNdsFtrDrawMemoState;
static u32 sNdsFtrDrawMemoHit;
static u32 sNdsFtrDrawMemoKey[NDS_FTR_DRAW_MEMO_KEY_WORDS];

/* ftMainSetStatus is the sole owner of BattleShip's fighter hidden-part DObj
 * topology changes. Invalidate at that write seam rather than rediscovering the
 * mutation by walking/hash-reading the whole tree every draw. The function is
 * intentionally public to the import wrapper; both renderer caches remain
 * private here and the later draw plan keys the generation bumped below. */
void ndsFighterRendererInvalidateStatusCachesOnSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp;
    u32 slot;

    if (fighter_gobj == NULL)
    {
        return;
    }
    fp = ftGetStruct(fighter_gobj);
    if ((fp == NULL) || ((u32)fp->nds_slot >= GMCOMMON_PLAYERS_MAX))
    {
        return;
    }
    slot = (u32)fp->nds_slot;
    sNdsFighterStatusGeneration[slot]++;
    sNdsFtrDrawMemo[slot].valid = 0u;
}

static u32 ndsFtrDrawMemoMixBytes(const void *data, u32 bytes)
{
    const u8 *p = data;
    u32 h = 2166136261u;
    u32 i;

    for (i = 0u; i < bytes; i++)
    {
        h = (h ^ p[i]) * 16777619u;
    }
    return h;
}

static void ndsFtrDrawMemoBuildKey(u32 *k, const FTStruct *fp,
                                   const void *root,
                                   u32 sky_fog_alpha, u32 is_shade_fog)
{
    k[0] = sNdsFighterDisplayContract.geometry_mode;
    k[1] = sNdsFighterDisplayContract.cycle_type;
    k[2] = sNdsFighterDisplayContract.render_mode;
    k[3] = sNdsFighterDisplayContract.prim_color;
    k[4] = sNdsFighterDisplayContract.env_color;
    k[5] = sNdsFighterDisplayContract.light_count;
    k[6] = sNdsFighterDisplayContract.light_valid;
    k[7] = ndsFtrDrawMemoMixBytes(&sNdsFighterDisplayContract.light,
                                  (u32)sizeof(sNdsFighterDisplayContract.light));
    k[8] = (sky_fog_alpha & 0xffffu) | (is_shade_fog << 16);
    k[9] = (u32)(uintptr_t)root;
    k[10] = (u32)(uintptr_t)fp->attr;
    k[11] = ((u32)(u8)fp->colanim.is_use_color1) |
            (((u32)(u8)fp->colanim.skeleton_id) << 8) |
            (((u32)(u8)fp->shade) << 16) |
            (((u32)(u8)fp->costume) << 24);
    k[12] = ((u32)(u8)fp->detail_curr) |
            (((u32)(u8)fp->is_modelpart_modify) << 8) |
            (((u32)(u8)fp->afterimage.drawstatus) << 16) |
            (((u32)(u8)fp->afterimage.is_itemswing) << 24);
    k[13] = (u32)fp->fkind;
    k[14] = (u32)fp->lr;
    k[15] = (u32)fp->display_mode;
}

/* Called from src/import/battleship_ftdisplaymain.c's gDPSetFogColor, i.e. from
 * ftDisplayMainProcDisplay's last contract-visible head action. One-shot: the
 * walk's own ftDisplayMainDecideFogDraw reaches the same macro and must not
 * re-arm. display_mode_master is passed in because the enum lives in a header
 * this translation unit does not include and fp is in scope at the macro. */
void ndsFighterDisplayContractHeadBoundary(u32 sky_fog_alpha, u32 is_shade_fog,
                                           u32 display_mode_master)
{
    const FTStruct *fp = sNdsFtrDrawMemoFp;
    const NDSFtrDrawMemoSlot *slot;
    u32 i;

    if (sNdsFtrDrawMemoState != 1u)
    {
        return;
    }
    sNdsFtrDrawMemoState = 3u;      /* boundary seen; not cacheable yet */
    gNdsFtrDrawMemoBoundary++;
    if ((display_mode_master == 0u) || (fp == NULL) ||
        (fp->afterimage.drawstatus >= 2))
    {
        return;
    }
    ndsFtrDrawMemoBuildKey(sNdsFtrDrawMemoKey, fp,
                           (const void *)sNdsFtrDrawMemoGObj->obj,
                           sky_fog_alpha, is_shade_fog);
    sNdsFtrDrawMemoState = 2u;
    slot = &sNdsFtrDrawMemo[sNdsFtrDrawMemoSlotIndex];
    if (slot->valid == 0u)
    {
        return;
    }
    for (i = 0u; i < NDS_FTR_DRAW_MEMO_KEY_WORDS; i++)
    {
        if (slot->key[i] != sNdsFtrDrawMemoKey[i])
        {
            gNdsFtrDrawMemoInvalidations++;
            return;
        }
    }
    sNdsFtrDrawMemoHit = 1u;
    sNdsFtrDrawMemoStub.flags = DOBJ_FLAG_HIDDEN;
    sNdsFtrDrawMemoStub.parent_gobj = sNdsFtrDrawMemoGObj;
    gNdsFtrDrawMemoSkipRoot = (void *)sNdsFtrDrawMemoGObj->obj;
}

static void ndsFtrDrawMemoFinish(void)
{
    NDSFtrDrawMemoSlot *slot;
    u32 n;
    u32 i;

    gNdsFtrDrawMemoSkipRoot = gNdsFtrDrawMemoStubRoot;
    if (sNdsFtrDrawMemoState != 2u)
    {
        gNdsFtrDrawMemoBypass++;
        sNdsFtrDrawMemoState = 0u;
        return;
    }
    slot = &sNdsFtrDrawMemo[sNdsFtrDrawMemoSlotIndex];
    if (sNdsFtrDrawMemoHit != 0u)
    {
        n = slot->event_count;
        sNdsFighterDisplayReplayEvents = slot->events;
        sNdsFighterDisplayReplayPreambles = slot->preambles;
        sNdsFighterDisplayContract.event_count = n;
        /* The contract still selected these display lists; only the derivation
         * was replayed. Keeping the counter's meaning is what makes Boundary's
         * ftrContract smoke an equality control rather than a known diff. */
        gNdsFighterDisplayContractSelectedCount += n;
        gNdsFtrDrawMemoHits++;
        gNdsFtrDrawMemoReplayEvents += n;
        sNdsFtrDrawMemoState = 0u;
        return;
    }
    n = sNdsFighterDisplayContract.event_count;
    if (n > NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED)
    {
        n = NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED;
    }
    if (n != 0u)
    {
        memcpy(slot->events, sNdsFighterDisplayContract.events,
               n * sizeof(slot->events[0]));
        memcpy(slot->preambles, sNdsFighterDisplayContractPreambles,
               n * sizeof(slot->preambles[0]));
    }
    slot->event_count = n;
    for (i = 0u; i < NDS_FTR_DRAW_MEMO_KEY_WORDS; i++)
    {
        slot->key[i] = sNdsFtrDrawMemoKey[i];
    }
    slot->valid = 1u;
    gNdsFtrDrawMemoFills++;
    sNdsFtrDrawMemoState = 0u;
}

static void ndsFighterDisplayContractCapture(GObj *fighter_gobj)
{
    extern void ndsBaseFTDisplayMainProcDisplay(GObj *fighter_gobj);
    extern sb32 gmCameraLookAtFuncMatrix(Mtx *mtx, CObj *cobj, Gfx **dls);
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 i;

    /* A miss/bypass consumes this frame's fresh source capture. MemoFinish
     * changes the views only after proving a hit. */
    sNdsFighterDisplayReplayEvents = sNdsFighterDisplayContract.events;
    sNdsFighterDisplayReplayPreambles = sNdsFighterDisplayContractPreambles;

    /* Every consumed event field and scratch command is overwritten before
     * use. Reset only the live capture state instead of clearing the 6,240-byte
     * event/scratch arena for each fighter every frame. */
    sNdsFighterDisplayContract.current_dobj = NULL;
    sNdsFighterDisplayContract.material_dobj = NULL;
    sNdsFighterDisplayContract.event_count = 0u;
    sNdsFighterDisplayContract.pending_event = -1;
    sNdsFighterDisplayContract.geometry_mode = 0u;
    sNdsFighterDisplayContract.cycle_type = 0u;
    sNdsFighterDisplayContract.render_mode = 0u;
    sNdsFighterDisplayContract.prim_color = 0xffffffffu;
    sNdsFighterDisplayContract.env_color = 0xffffffffu;
    sNdsFighterDisplayContract.light_count =
        sNdsFighterDisplayCurrentLightCount;
    sNdsFighterDisplayContract.light = (Light){ 0 };
    sNdsFighterDisplayContract.light_valid = FALSE;
    sNdsFighterDisplayContract.matrix_ready = FALSE;
    sNdsFighterDisplayContract.material_ready = FALSE;
    if (sNdsFighterDisplayCurrentLightValid != 0u)
    {
        sNdsFighterDisplayContract.light = sNdsFighterDisplayCurrentLight;
        sNdsFighterDisplayContract.light_valid = TRUE;
    }
    sNdsFighterDisplayContract.saved_graphics_heap_ptr =
        gSYTaskmanGraphicsHeap.ptr;
    for (i = 0u; i < 4u; i++)
    {
        sNdsFighterDisplayContract.saved_heads[i] = gSYTaskmanDLHeads[i];
        gSYTaskmanDLHeads[i] = sNdsFighterDisplayContract.scratch[i];
    }
    sNdsFighterDisplayContract.active = TRUE;
    /* ftdisplaymain.c:1093-1129 only needs the battle visibility matrix for
     * player/CPU/game-key fighters. Results fighters are Demo fighters. */
    if ((fp != NULL) &&
        ((fp->pkind == nFTPlayerKindMan) ||
         (fp->pkind == nFTPlayerKindCom) ||
         (fp->pkind == nFTPlayerKindGameKey)) &&
        (gGMCameraGObj != NULL) &&
        (CObjGetStruct(gGMCameraGObj) != NULL))
    {
        /* BattleShip gmcamera.c:985-1015 prepares the visibility matrix.
         * NULL, not a local Mtx: this call site wants only the side effects on
         * gGCMatrixPerspF / sGCMatrixProjectL / gGMCameraMatrix that
         * ftdisplaymain.c:1093-1129 reads. It passed `&camera_mtx` until
         * 2026-08-09 and never read it back, so the function's closing
         * syMatrixF2L -- a 4x4 float-to-fixed conversion, once per fighter per
         * frame -- wrote into a dead local. The port wrapper in
         * battleship_gmcamera.c skips that conversion for a NULL out-pointer
         * and still gives decomp's kind-0x4C caller its matrix. */
        gmCameraLookAtFuncMatrix(NULL,
                                 CObjGetStruct(gGMCameraGObj),
                                 gSYTaskmanDLHeads);
    }
#if NDS_R2_FTR_CONTRACT_CENSUS
    sNdsFtrContractCensusKey = NDS_FTR_CONTRACT_HASH_SEED;
    ndsFighterDisplayContractCountFlags(DObjGetStruct(fighter_gobj));
#endif
    sNdsFtrDrawMemoFp = fp;
    sNdsFtrDrawMemoGObj = fighter_gobj;
    sNdsFtrDrawMemoHit = 0u;
    /* Do not memoise the source entry-camera walk.  Mario/Fox Appear motions
     * deliberately mutate DOBJ_FLAG_HIDDEN inside the same status as their
     * entry animation advances.  The old memo saw the first source-hidden pose
     * (zero selected DLs), cached that empty contract, then kept substituting
     * the hidden stub after the animation exposed the fighter -- exactly the
     * "pipe/Arwing plays, fighter pops in at Wait" regression.
     *
     * This is the narrow ownership boundary: the short Entry window keeps the
     * source DObj visibility walk live, while the resulting fighter display
     * lists still execute through the DS-native production owner below.  Normal
     * gameplay retains the memo and its measured hot-path saving.  A live GDB
     * A/B on 2026-08-21 proved the discriminator: with the memo route enabled
     * Appear stayed at 0 events; disabling only the memo produced 12 Mario
     * events before Wait without changing is_invisible or camera bounds. */
    sNdsFtrDrawMemoState =
        ((gNdsFtrDrawMemoRoute.route != 0u) && (fp != NULL) &&
         ((u32)fp->nds_slot < GMCOMMON_PLAYERS_MAX) &&
         (fp->camera_mode != nFTCameraModeEntry)) ? 1u : 0u;
    sNdsFtrDrawMemoSlotIndex = (fp != NULL) ? (u32)fp->nds_slot : 0u;
    ndsBaseFTDisplayMainProcDisplay(fighter_gobj);
    ndsFtrDrawMemoFinish();
    sNdsFighterDisplayContract.active = FALSE;
    for (i = 0u; i < 4u; i++)
    {
        gSYTaskmanDLHeads[i] = sNdsFighterDisplayContract.saved_heads[i];
    }
    gSYTaskmanGraphicsHeap.ptr =
        sNdsFighterDisplayContract.saved_graphics_heap_ptr;
}

static void ndsFighterCollectAllDObjsWithDL(
    DObj *root, NDSFighterDLAllDrawCollection *collection)
{
    u32 traversal_index = 0u;
    u32 i;

    if (collection == NULL)
    {
        return;
    }
    bzero(collection, sizeof(*collection));
    if (sNdsFighterDisplayContractPlayback != FALSE)
    {
        for (i = 0u; i < sNdsFighterDisplayContract.event_count; i++)
        {
            if (sNdsFighterDisplayReplayEvents[i].dobj == NULL)
            {
                continue;
            }
            collection->dobjs[collection->selected_count] =
                sNdsFighterDisplayReplayEvents[i].dobj;
            collection->indices[collection->selected_count] = i;
            collection->selected_count++;
            collection->total_count++;
        }
        return;
    }
    ndsFighterCollectAllDObjsWithDLRecursive(root, collection,
                                              &traversal_index);
}

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
static sb32 ndsRendererAdapterCollectFighterTopology(
    DObj *dobj,
    u32 parent_index,
    DObj **joints,
    u8 *joint_parents,
    u32 *joint_count)
{
    while (dobj != NULL)
    {
        if (*joint_count >= NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX)
        {
            return FALSE;
        }
        u32 joint_index = (*joint_count)++;

        joints[joint_index] = dobj;
        joint_parents[joint_index] = (u8)parent_index;
        if (ndsRendererAdapterCollectFighterTopology(
                dobj->child, joint_index,
                joints, joint_parents, joint_count) == FALSE)
        {
            return FALSE;
        }
        dobj = dobj->sib_next;
    }
    return TRUE;
}

#endif

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER

static void ndsRendererAdapterM2CensusFighter(
    u32 slot,
    FTStruct *fp,
    DObj *root,
    const NDSFighterDLAllDrawCollection *collection,
    volatile NDSRendererOwnerProfile *owner)
{
    DObj *joints[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    u8 joint_parents[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    u8 joint_bindings[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    u32 joint_count = 0u;
    u32 joint_index;
    u32 binding_index;
    u32 schedule_matches = 0u;
    u32 binding_matches = 0u;

    if ((slot > 1u) || (fp == NULL) || (root == NULL) ||
        (collection == NULL) || (owner == NULL))
    {
        return;
    }
    memset(joints, 0, sizeof(joints));
    memset(joint_parents, 31, sizeof(joint_parents));
    memset(joint_bindings, 31, sizeof(joint_bindings));
    ndsRendererAdapterCollectFighterTopology(
        root, 31u, joints, joint_parents, &joint_count);

    for (binding_index = 0u;
         binding_index < collection->selected_count;
         binding_index++)
    {
        for (joint_index = 0u;
             joint_index < joint_count;
             joint_index++)
        {
            if (joints[joint_index] == collection->dobjs[binding_index])
            {
                if (joint_bindings[joint_index] == 31u)
                {
                    joint_bindings[joint_index] = (u8)binding_index;
                }
                break;
            }
        }
    }
    for (joint_index = 0u; joint_index < joint_count; joint_index++)
    {
        DObj *joint = joints[joint_index];
        FTParts *parts = ftGetParts(joint);
        u32 xobj_index;

        for (xobj_index = 0u;
             xobj_index < joint->xobjs_num;
             xobj_index++)
        {
            XObj *xobj = joint->xobjs[xobj_index];

            if (xobj == NULL)
            {
                owner->m2_xobj_null_count++;
                continue;
            }
            owner->m2_xobj_count++;
            if (xobj->kind ==
                NDS_RENDERER_ADAPTER_FIGHTER_PARTS_MTX_KIND)
            {
                owner->m2_xobj_kind_4b_count++;
            }
            else if (xobj->kind == nGCMatrixKindNull)
            {
                owner->m2_xobj_kind_2_count++;
            }
            else
            {
                owner->m2_xobj_other_count++;
            }
        }
        if (parts != NULL)
        {
            owner->m2_parts_count++;
            switch (parts->transform_update_mode)
            {
            case 0:
                owner->m2_parts_matrix_mode0_count++;
                break;
            case 1:
                owner->m2_parts_matrix_mode1_count++;
                break;
            case 3:
                owner->m2_parts_matrix_mode3_count++;
                break;
            default:
                owner->m2_parts_matrix_other_count++;
                break;
            }
        }
    }
    ndsRendererProfileCensusNativeFighterSchedule(
        slot, joint_parents, joint_bindings,
        joint_count, collection->selected_count,
        &schedule_matches, &binding_matches);
    owner->m2_schedule_joint_count = joint_count;
    owner->m2_schedule_match_count = schedule_matches;
    owner->m2_binding_count = collection->selected_count;
    owner->m2_binding_match_count = binding_matches;
    owner->m2_animlock_active = (fp->is_use_animlocks != FALSE) ? 1u : 0u;
}
#endif

static void ndsFighterDisplayContractSeedMaterialLights(
    NDSRendererStats *stats)
{
    u32 i;

    if ((stats == NULL) ||
        ((stats->light_color_mask &
          (NDS_FIGHTER_DISPLAY_LIGHT_COLOR_1_MASK |
           NDS_FIGHTER_DISPLAY_LIGHT_COLOR_2_MASK)) ==
         (NDS_FIGHTER_DISPLAY_LIGHT_COLOR_1_MASK |
          NDS_FIGHTER_DISPLAY_LIGHT_COLOR_2_MASK)))
    {
        return;
    }
    for (i = 0u; i < sNdsFighterDisplayContract.event_count; i++)
    {
        DObj *dobj = sNdsFighterDisplayReplayEvents[i].material_dobj;
        MObj *mobj = (dobj != NULL) ? dobj->mobj : NULL;

        if (mobj == NULL)
        {
            continue;
        }
        /* gcAddMObjForDObj copies the source MObjSub verbatim
         * (objman.c:1302-1335). Use its first selected material as the
         * initial RSP light state; later objdisplay.c:1289-1295 commands
         * remain authoritative and carry in original event order. */
        stats->light_color_1 = ndsFighterDisplayContractPackColor(
            mobj->sub.light1color.s.r, mobj->sub.light1color.s.g,
            mobj->sub.light1color.s.b, mobj->sub.light1color.s.a);
        stats->light_color_2 = ndsFighterDisplayContractPackColor(
            mobj->sub.light2color.s.r, mobj->sub.light2color.s.g,
            mobj->sub.light2color.s.b, mobj->sub.light2color.s.a);
        stats->light_color_mask |=
            NDS_FIGHTER_DISPLAY_LIGHT_COLOR_1_MASK |
            NDS_FIGHTER_DISPLAY_LIGHT_COLOR_2_MASK;
        gNdsFighterDisplayContractMaterialLightSeedCount++;
        gNdsFighterDisplayContractMaterialLight1 = stats->light_color_1;
        gNdsFighterDisplayContractMaterialLight2 = stats->light_color_2;
        return;
    }
}

static s32 ndsFighterDLAllDrawValidateRange(const Gfx *dl, size_t bytes,
                                            void *user)
{
    (void)user;

    if ((((uintptr_t)dl & (sizeof(u32) - 1u)) != 0u) ||
        ((ndsFighterDLScanRangeInTaskmanArena(dl, bytes) == FALSE) &&
         (ndsRelocFindLoadedFileContaining(dl, bytes) == NULL) &&
         (ndsRendererAdapterRangeIsEmptySegmentEDL(dl, bytes) == FALSE)))
    {
        gNdsFighterDLAllDrawRangeRejectCount++;
        return FALSE;
    }
    return TRUE;
}

static const Gfx *ndsFighterDLAllDrawResolveBranch(const Gfx *dl,
                                                   u32 *resolve_kind,
                                                   void *user)
{
    return ndsFighterDLDrawResolveBranch(dl, resolve_kind, user);
}

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
/* ---------------------------------------------------------------------------
 * The invariant half of the production inputs, primed once.
 *
 * `BuildNativeProductionInputs` rebuilt both structs from scratch for every
 * selected root on every frame -- **13,175 ticks/frame, 44% of the whole adapter
 * driver** by the c106 line census -- and almost none of what it wrote had
 * changed. Nine of the config's thirteen fields are compile-time constants or
 * function pointers; three of the root's are the addresses of fixed workspace
 * slots. On top of that it zeroed both structs first, which is ~30 word stores a
 * root before a single useful field is written.
 *
 * None of it depends on WHICH fighter is drawing, because the workspace is a
 * single shared static and slot 0 and slot 1 write the same addresses into the
 * same slots. So this primes all `NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED` entries
 * once and is never keyed on anything -- no stamp, no generation, no compare.
 *
 * What genuinely changes per frame, and is all the loop below now writes: the
 * two camera pointers, the geometry mode, the damage/hurt colour modulate, the
 * resolver (a stack local in the caller, so its address is not guaranteed
 * stable), the plan's root offsets and material counts, and the display
 * contract's preamble. The preamble is assigned rather than OR-ed now, because
 * the per-frame zeroing that used to clear it is gone. */
static u32 sNdsRendererAdapterProductionInputsPrimed;

/* Runs ONCE per match and was inlined into ndsFighterMarioFoxDLAllDrawForSlot,
 * where the census found it occupying ~1,640 of that function's 10,708 bytes
 * without ever executing. The ARM946E-S I-cache is 8 KB: a 10.7 KB hot driver
 * evicts itself every frame, which is why the whole function measures 4.21
 * cycles per instruction. Cold code inlined into a hot path is not free even
 * when it never runs -- it lands on the same 32-byte lines. */
static void __attribute__((noinline, cold, optimize("Os")))
ndsRendererAdapterPrimeProductionInputs(
    NDSRendererAdapterNativeOwnerWorkspace *workspace)
{
    u32 i;

    for (i = 0u; i < NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED; i++)
    {
        NDSRendererConfig *config = &workspace->production_configs[i];
        NDSRendererNativeFighterRoot *root =
            &workspace->production_roots[i];

        *config = (NDSRendererConfig){0};
        *root = (NDSRendererNativeFighterRoot){0};

        config->max_depth = 8u;
        config->max_commands = 2048u;
        config->max_list_commands = 512u;
        config->texture_data_layout =
            NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
        config->validate_range = ndsFighterDLAllDrawValidateRange;
        config->immutable_command_span =
            ndsRendererAdapterImmutableCommandSpan;
        config->resolve_branch = ndsFighterDLAllDrawResolveBranch;
        config->resolve_data = ndsFighterDLDrawResolveRendererData;

        root->composed_matrix = &workspace->composed_matrices[i];
        root->preamble = &sNdsRendererAdapterZeroPreamble;
        /* `materials` is NOT primed here any more: the materials row is now
         * owned by the material DObj rather than by this slot index, so which
         * row root i points at is a per-frame fact. BuildNativeProductionInputs
         * sets it. */
        root->config = config;
    }
    sNdsRendererAdapterProductionInputsPrimed = TRUE;
}

/* No `noinline` here either, and for the same measured reason as
 * ndsRendererAdapterPrepareNativeOwnerMatrices above. */
static sb32 ndsRendererAdapterBuildNativeProductionInputs(
    u32 slot,
    u32 color_modulate,
    NDSRelocLoadedFile *owner_file,
    const NDSFighterDLAllDrawCollection *collection,
    const NDSRendererMatrix20p12 *projection,
    const NDSRendererMatrix20p12 *const *modelviews,
    NDSFighterDLDrawState *resolver,
    NDSRendererAdapterNativeOwnerWorkspace *workspace
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    , volatile NDSRendererOwnerProfile *m2_owner
#endif
    )
{
    u32 i;

    if ((slot >= NDS_RENDERER_NATIVE_FIGHTER_OWNER_COUNT) ||
        (owner_file == NULL) ||
        (owner_file->data == NULL) || (collection == NULL) ||
        (modelviews == NULL) || (resolver == NULL) ||
        (workspace == NULL) || (collection->selected_count == 0u) ||
        (collection->selected_count >
         NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED))
    {
        return FALSE;
    }

    resolver->primary_file = owner_file;
    resolver->slot = slot;
    resolver->segment_e_base = NULL;
    resolver->segment_e_end = NULL;

    if (sNdsRendererAdapterProductionInputsPrimed == FALSE)
    {
        ndsRendererAdapterPrimeProductionInputs(workspace);
    }

    for (i = 0u; i < collection->selected_count; i++)
    {
        NDSRendererConfig *config = &workspace->production_configs[i];
        NDSRendererNativeFighterRoot *root =
            &workspace->production_roots[i];
        const NDSFighterDisplayContractEvent *event =
            (sNdsFighterDisplayContractPlayback != FALSE) ?
                &sNdsFighterDisplayReplayEvents[
                    collection->indices[i]] : NULL;

        config->initial_projection = projection;
        config->initial_modelview = modelviews[i];
        config->initial_geometry_mode = (event != NULL) ?
            sNdsFighterDisplayReplayPreambles[
                collection->indices[i]].geometry_mode : 0u;
        config->color_modulate = color_modulate;
        config->user = resolver;

#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        {
            u32 m2_compose_start = cpuGetTiming();
            sb32 m2_compose_valid =
                ((modelviews[i] != NULL) &&
                 (ndsRendererAdapterComposeNativeRootMatrix(
                      modelviews[i], projection,
                      &workspace->composed_matrices[i]) != FALSE)) ?
                    TRUE : FALSE;

            if (m2_owner != NULL)
            {
                m2_owner->m2_final_compose_ticks +=
                    cpuGetTiming() - m2_compose_start;
                m2_owner->m2_final_compose_count++;
            }
            if (m2_compose_valid == FALSE)
            {
                return FALSE;
            }
        }
#elif NDS_R2_FIGHTER_HW_MTX
        /* R2-03 E16b. The hardware performs this multiply now, and E16b traced
         * the composed matrix to no other production consumer, so the compose
         * is skipped outright -- one 4x4 per root out of MatrixPrep. */
        if (modelviews[i] == NULL)
        {
            return FALSE;
        }
#else
        if ((modelviews[i] == NULL) ||
            (ndsRendererAdapterComposeNativeRootMatrix(
                 modelviews[i], projection,
                 &workspace->composed_matrices[i]) == FALSE))
        {
            return FALSE;
        }
#endif

        root->root_offset = workspace->root_offsets[i];
        root->material_count = workspace->material_counts[i];
        root->materials = sNdsRendererAdapterNativeOwnerMaterials[
            sNdsRendererAdapterNativeOwnerMaterialRows[i]];
        root->modelview_matrix = modelviews[i];
#if NDS_R2_FIGHTER_HW_MTX
        root->projection_matrix = projection;
#endif
#if NDS_R2_FIGHTER_GX_COMPOSE
        root->gx_valid = workspace->gx_valid;
        if (workspace->gx_valid != 0u)
        {
            root->gx_locals = &workspace->gx_locals[
                workspace->gx_local_first[i]];
            root->gx_seed = &workspace->gx_seed;
            root->gx_local_count = workspace->gx_local_count[i];
            root->gx_parent_slot = workspace->gx_parent_slot[i];
            root->gx_store_slot = workspace->gx_store_slot[i];
            root->gx_seed_is_identity = workspace->gx_seed_is_identity;
        }
        else
        {
            /* Leave nothing stale behind: a declining owner that inherited the
             * previous owner's chains would draw the wrong fighter's skeleton
             * and still report full engagement. */
            root->gx_locals = NULL;
            root->gx_seed = NULL;
            root->gx_local_count = 0u;
            root->gx_parent_slot = (u8)NDS_RENDERER_FIGHTER_GX_SLOT_NONE;
            root->gx_store_slot = (u8)NDS_RENDERER_FIGHTER_GX_SLOT_NONE;
            root->gx_seed_is_identity = 0u;
        }
#endif
#if NDS_RENDERER_M2_DETAILED_LEDGER
        root->owner_generation = owner_file->owner_generation;
#endif

        root->preamble = (event != NULL) ?
            &sNdsFighterDisplayReplayPreambles[collection->indices[i]] :
            &sNdsRendererAdapterZeroPreamble;
    }
    return TRUE;
}

static sb32 ndsRendererAdapterBuildNativeHierarchyInputs(
    u32 slot,
    u32 color_modulate,
    NDSRelocLoadedFile *owner_file,
    const NDSFighterDLAllDrawCollection *collection,
    NDSFighterDLDrawState *resolver,
    NDSRendererAdapterNativeOwnerWorkspace *workspace)
{
    NDSRendererConfig *config;
    u32 i;

    if ((slot >= NDS_RENDERER_NATIVE_FIGHTER_OWNER_COUNT) ||
        (owner_file == NULL) ||
        (owner_file->data == NULL) || (collection == NULL) ||
        (resolver == NULL) || (workspace == NULL) ||
        (collection->selected_count == 0u) ||
        (collection->selected_count >
         NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED))
    {
        return FALSE;
    }
    resolver->primary_file = owner_file;
    resolver->slot = slot;
    resolver->segment_e_base = NULL;
    resolver->segment_e_end = NULL;

    /* This path shares `production_roots` and re-points every `root->config` at
     * the hierarchy config, so the production primer's invariants are gone once
     * it has run. Mode 7 and mode 9 are different builds' runtime modes rather
     * than alternating states, but a stale invariant is a silent wrong pointer,
     * so make the production path re-prime rather than assume. */
    sNdsRendererAdapterProductionInputsPrimed = FALSE;

    config = &workspace->hierarchy_config;
    *config = (NDSRendererConfig){0};
    config->max_depth = 8u;
    config->max_commands = 2048u;
    config->max_list_commands = 512u;
    config->initial_geometry_mode = 0u;
    config->color_modulate = color_modulate;
    config->texture_data_layout =
        NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    config->validate_range = ndsFighterDLAllDrawValidateRange;
    config->immutable_command_span =
        ndsRendererAdapterImmutableCommandSpan;
    config->resolve_branch = ndsFighterDLAllDrawResolveBranch;
    config->resolve_data = ndsFighterDLDrawResolveRendererData;
    config->user = resolver;

    for (i = 0u; i < collection->selected_count; i++)
    {
        NDSRendererNativeFighterRoot *root =
            &workspace->production_roots[i];
        const NDSFighterDisplayContractEvent *event =
            (sNdsFighterDisplayContractPlayback != FALSE) ?
                &sNdsFighterDisplayReplayEvents[
                    collection->indices[i]] : NULL;

        *root = (NDSRendererNativeFighterRoot){0};
        root->root_offset = workspace->root_offsets[i];
        root->material_count = workspace->material_counts[i];
        root->materials = sNdsRendererAdapterNativeOwnerMaterials[
            sNdsRendererAdapterNativeOwnerMaterialRows[i]];
        root->config = config;
        root->preamble = (event != NULL) ?
            &sNdsFighterDisplayReplayPreambles[collection->indices[i]] :
            &sNdsRendererAdapterZeroPreamble;
    }
    workspace->hierarchy.roots = workspace->production_roots;
    workspace->hierarchy.config = config;
    workspace->hierarchy.root_count = collection->selected_count;
    return TRUE;
}
#endif

static void ndsFighterDLAllDrawRecordScreenPoint(
    s32 x, s32 y, u32 *screen_valid,
    s32 *screen_min_x, s32 *screen_max_x,
    s32 *screen_min_y, s32 *screen_max_y)
{
    if ((screen_valid == NULL) || (screen_min_x == NULL) ||
        (screen_max_x == NULL) || (screen_min_y == NULL) ||
        (screen_max_y == NULL))
    {
        return;
    }

    if (x < 0) { x = 0; }
    if (x >= (s32)NDS_FIGHTER_DL_DRAW_WIDTH)
    {
        x = (s32)NDS_FIGHTER_DL_DRAW_WIDTH - 1;
    }
    if (y < 0) { y = 0; }
    if (y >= (s32)NDS_FIGHTER_DL_DRAW_HEIGHT)
    {
        y = (s32)NDS_FIGHTER_DL_DRAW_HEIGHT - 1;
    }

    if (*screen_valid == 0u)
    {
        *screen_min_x = *screen_max_x = x;
        *screen_min_y = *screen_max_y = y;
        *screen_valid = 1u;
        return;
    }
    if (x < *screen_min_x) { *screen_min_x = x; }
    if (x > *screen_max_x) { *screen_max_x = x; }
    if (y < *screen_min_y) { *screen_min_y = y; }
    if (y > *screen_max_y) { *screen_max_y = y; }
}

static void ndsFighterDLAllDrawRasterizeStates(
    u32 slot, NDSFighterDLDrawState *states, const u8 *clean,
    u32 selected_count, u16 *pixels, u32 pitch)
{
    u32 axis;
    u32 best_axis = 0xffffffffu;
    u32 best_area = 0u;
    u32 best_nondegenerate_count = 0u;
    u32 i;
    s32 min_a = 0;
    s32 max_a = 0;
    s32 min_b = 0;
    s32 max_b = 0;
    u32 bounds_valid = 0u;
    s32 box_min_x = (slot == 0u) ? 4 : 52;
    s32 box_max_x = (slot == 0u) ? 43 : 91;
    s32 box_min_y = 4;
    s32 box_max_y = 67;
    s32 screen_min_x = 0;
    s32 screen_max_x = 0;
    s32 screen_min_y = 0;
    s32 screen_max_y = 0;
    u32 screen_valid = 0u;
    u32 pixel_count = 0u;
    u32 drawn_count = 0u;
    u32 real_drawn_count = 0u;
    u32 marker_drawn_count = 0u;
    u32 drawn_dobj_count = 0u;

    if ((states == NULL) || (clean == NULL) || (pixels == NULL))
    {
        return;
    }

    for (axis = 0u; axis < 3u; axis++)
    {
        s32 axis_min_a = 0;
        s32 axis_max_a = 0;
        s32 axis_min_b = 0;
        s32 axis_max_b = 0;
        u32 axis_bounds_valid = 0u;
        u32 area_sum = 0u;
        u32 nondegenerate_count = 0u;

        for (i = 0u; i < selected_count; i++)
        {
            u32 tri_index;

            if (clean[i] == FALSE)
            {
                continue;
            }
            for (tri_index = 0u; tri_index < states[i].triangle_count;
                 tri_index++)
            {
                const NDSFighterDLDrawTri *tri = &states[i].tris[tri_index];
                const NDSFighterDLDrawVtx *v0;
                const NDSFighterDLDrawVtx *v1;
                const NDSFighterDLDrawVtx *v2;

                if ((tri->v0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                    (tri->v1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                    (tri->v2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
                {
                    continue;
                }
                v0 = &states[i].vertices[tri->v0];
                v1 = &states[i].vertices[tri->v1];
                v2 = &states[i].vertices[tri->v2];
                if ((v0->valid == FALSE) || (v1->valid == FALSE) ||
                    (v2->valid == FALSE))
                {
                    continue;
                }
                ndsFighterDLDrawRecordAxisPoint(
                    v0, axis, &axis_bounds_valid, &axis_min_a, &axis_max_a,
                    &axis_min_b, &axis_max_b);
                ndsFighterDLDrawRecordAxisPoint(
                    v1, axis, &axis_bounds_valid, &axis_min_a, &axis_max_a,
                    &axis_min_b, &axis_max_b);
                ndsFighterDLDrawRecordAxisPoint(
                    v2, axis, &axis_bounds_valid, &axis_min_a, &axis_max_a,
                    &axis_min_b, &axis_max_b);
            }
        }
        if ((axis_bounds_valid == 0u) ||
            ((axis_min_a == axis_max_a) && (axis_min_b == axis_max_b)))
        {
            continue;
        }

        for (i = 0u; i < selected_count; i++)
        {
            u32 tri_index;

            if (clean[i] == FALSE)
            {
                continue;
            }
            for (tri_index = 0u; tri_index < states[i].triangle_count;
                 tri_index++)
            {
                const NDSFighterDLDrawTri *tri = &states[i].tris[tri_index];
                const NDSFighterDLDrawVtx *v0;
                const NDSFighterDLDrawVtx *v1;
                const NDSFighterDLDrawVtx *v2;
                s32 x0;
                s32 y0;
                s32 x1;
                s32 y1;
                s32 x2;
                s32 y2;
                s32 area;

                if ((tri->v0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                    (tri->v1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                    (tri->v2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
                {
                    continue;
                }
                v0 = &states[i].vertices[tri->v0];
                v1 = &states[i].vertices[tri->v1];
                v2 = &states[i].vertices[tri->v2];
                if ((v0->valid == FALSE) || (v1->valid == FALSE) ||
                    (v2->valid == FALSE))
                {
                    continue;
                }
                x0 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v0, axis, 0u),
                    axis_min_a, axis_max_a, box_min_x, box_max_x);
                y0 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v0, axis, 1u),
                    axis_min_b, axis_max_b, box_max_y, box_min_y);
                x1 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v1, axis, 0u),
                    axis_min_a, axis_max_a, box_min_x, box_max_x);
                y1 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v1, axis, 1u),
                    axis_min_b, axis_max_b, box_max_y, box_min_y);
                x2 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v2, axis, 0u),
                    axis_min_a, axis_max_a, box_min_x, box_max_x);
                y2 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v2, axis, 1u),
                    axis_min_b, axis_max_b, box_max_y, box_min_y);

                area = ndsFighterDLDrawEdge(x0, y0, x1, y1, x2, y2);
                if (area == 0)
                {
                    continue;
                }
                nondegenerate_count++;
                area_sum += (area < 0) ? (u32)-area : (u32)area;
            }
        }

        if ((best_axis > 2u) ||
            (nondegenerate_count > best_nondegenerate_count) ||
            ((nondegenerate_count == best_nondegenerate_count) &&
             (area_sum > best_area)))
        {
            best_area = area_sum;
            best_nondegenerate_count = nondegenerate_count;
            best_axis = axis;
            min_a = axis_min_a;
            max_a = axis_max_a;
            min_b = axis_min_b;
            max_b = axis_max_b;
            bounds_valid = 1u;
        }
    }

    if (best_axis > 2u)
    {
        return;
    }
    if ((bounds_valid == 0u) || ((min_a == max_a) && (min_b == max_b)))
    {
        return;
    }

    for (i = 0u; i < selected_count; i++)
    {
        u32 tri_index;
        u32 state_drawn = 0u;

        if (clean[i] == FALSE)
        {
            continue;
        }
        for (tri_index = 0u; tri_index < states[i].triangle_count;
             tri_index++)
        {
            const NDSFighterDLDrawTri *tri = &states[i].tris[tri_index];
            const NDSFighterDLDrawVtx *v0;
            const NDSFighterDLDrawVtx *v1;
            const NDSFighterDLDrawVtx *v2;
            s32 x0;
            s32 y0;
            s32 x1;
            s32 y1;
            s32 x2;
            s32 y2;
            u32 before;
            u32 marker_drawn = 0u;
            u16 fill;
            u16 edge;

            if ((tri->v0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                (tri->v1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                (tri->v2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
            {
                continue;
            }
            v0 = &states[i].vertices[tri->v0];
            v1 = &states[i].vertices[tri->v1];
            v2 = &states[i].vertices[tri->v2];
            if ((v0->valid == FALSE) || (v1->valid == FALSE) ||
                (v2->valid == FALSE))
            {
                continue;
            }

            x0 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v0, best_axis, 0u),
                min_a, max_a, box_min_x, box_max_x);
            y0 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v0, best_axis, 1u),
                min_b, max_b, box_max_y, box_min_y);
            x1 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v1, best_axis, 0u),
                min_a, max_a, box_min_x, box_max_x);
            y1 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v1, best_axis, 1u),
                min_b, max_b, box_max_y, box_min_y);
            x2 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v2, best_axis, 0u),
                min_a, max_a, box_min_x, box_max_x);
            y2 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v2, best_axis, 1u),
                min_b, max_b, box_max_y, box_min_y);
            fill = ndsFighterDLDrawTriangleColor(&states[i], tri);
            edge = ndsFighterDLDrawRGB15(255, 255, 255);
            before = pixel_count;
            if (ndsFighterDLDrawEdge(x0, y0, x1, y1, x2, y2) == 0)
            {
                s32 cx = (x0 + x1 + x2) / 3;
                s32 cy = (y0 + y1 + y2) / 3;

                ndsFighterDLDrawTriangle(pixels, pitch,
                                         cx - 5, cy - 3,
                                         cx + 5, cy - 3,
                                         cx, cy + 5,
                                         fill, edge, &pixel_count);
                marker_drawn = 1u;
                x0 = cx - 5;
                y0 = cy - 3;
                x1 = cx + 5;
                y1 = cy - 3;
                x2 = cx;
                y2 = cy + 5;
            }
            else
            {
                ndsFighterDLDrawTriangle(pixels, pitch,
                                         x0, y0, x1, y1, x2, y2,
                                         fill, edge, &pixel_count);
            }
            if (pixel_count != before)
            {
                drawn_count++;
                if (marker_drawn != 0u)
                {
                    marker_drawn_count++;
                }
                else
                {
                    real_drawn_count++;
                }
                state_drawn = 1u;
                ndsFighterDLAllDrawRecordScreenPoint(
                    x0, y0, &screen_valid, &screen_min_x, &screen_max_x,
                    &screen_min_y, &screen_max_y);
                ndsFighterDLAllDrawRecordScreenPoint(
                    x1, y1, &screen_valid, &screen_min_x, &screen_max_x,
                    &screen_min_y, &screen_max_y);
                ndsFighterDLAllDrawRecordScreenPoint(
                    x2, y2, &screen_valid, &screen_min_x, &screen_max_x,
                    &screen_min_y, &screen_max_y);
            }
        }
        if (state_drawn != 0u)
        {
            drawn_dobj_count++;
        }
    }

    if (slot == 0u)
    {
        gNdsFighterDLAllDrawP0Axis = best_axis;
        gNdsFighterDLAllDrawP0Area = best_area;
        gNdsFighterDLAllDrawP0MinA = min_a;
        gNdsFighterDLAllDrawP0MaxA = max_a;
        gNdsFighterDLAllDrawP0MinB = min_b;
        gNdsFighterDLAllDrawP0MaxB = max_b;
        gNdsFighterDLAllDrawP0ScreenMinX = screen_min_x;
        gNdsFighterDLAllDrawP0ScreenMaxX = screen_max_x;
        gNdsFighterDLAllDrawP0ScreenMinY = screen_min_y;
        gNdsFighterDLAllDrawP0ScreenMaxY = screen_max_y;
        gNdsFighterDLAllDrawP0PixelCount = pixel_count;
        gNdsFighterDLAllDrawP0TriangleDrawnCount = drawn_count;
        gNdsFighterDLAllDrawP0RealTriangleDrawnCount = real_drawn_count;
        gNdsFighterDLAllDrawP0MarkerTriangleDrawnCount = marker_drawn_count;
        gNdsFighterDLAllDrawP0DrawnDObjCount = drawn_dobj_count;
    }
    else
    {
        gNdsFighterDLAllDrawP1Axis = best_axis;
        gNdsFighterDLAllDrawP1Area = best_area;
        gNdsFighterDLAllDrawP1MinA = min_a;
        gNdsFighterDLAllDrawP1MaxA = max_a;
        gNdsFighterDLAllDrawP1MinB = min_b;
        gNdsFighterDLAllDrawP1MaxB = max_b;
        gNdsFighterDLAllDrawP1ScreenMinX = screen_min_x;
        gNdsFighterDLAllDrawP1ScreenMaxX = screen_max_x;
        gNdsFighterDLAllDrawP1ScreenMinY = screen_min_y;
        gNdsFighterDLAllDrawP1ScreenMaxY = screen_max_y;
        gNdsFighterDLAllDrawP1PixelCount = pixel_count;
        gNdsFighterDLAllDrawP1TriangleDrawnCount = drawn_count;
        gNdsFighterDLAllDrawP1RealTriangleDrawnCount = real_drawn_count;
        gNdsFighterDLAllDrawP1MarkerTriangleDrawnCount = marker_drawn_count;
        gNdsFighterDLAllDrawP1DrawnDObjCount = drawn_dobj_count;
    }
}

static void ndsFighterDLAllDrawResetFailureDiagnostics(void)
{
    gNdsFighterDLAllDrawP0FirstFailedSelectedIndex = 0xffffffffu;
    gNdsFighterDLAllDrawP1FirstFailedSelectedIndex = 0xffffffffu;
    gNdsFighterDLAllDrawP0FirstFailedTreeIndex = 0xffffffffu;
    gNdsFighterDLAllDrawP1FirstFailedTreeIndex = 0xffffffffu;
    gNdsFighterDLAllDrawP0FirstFailedReason = 0u;
    gNdsFighterDLAllDrawP1FirstFailedReason = 0u;
    gNdsFighterDLAllDrawP0FirstFailedDObj = 0u;
    gNdsFighterDLAllDrawP1FirstFailedDObj = 0u;
    gNdsFighterDLAllDrawP0FirstFailedDL = 0u;
    gNdsFighterDLAllDrawP1FirstFailedDL = 0u;
    gNdsFighterDLAllDrawP0FirstFailedCommandCount = 0u;
    gNdsFighterDLAllDrawP1FirstFailedCommandCount = 0u;
    gNdsFighterDLAllDrawP0FirstFailedFirstOpcode = 0u;
    gNdsFighterDLAllDrawP1FirstFailedFirstOpcode = 0u;
    gNdsFighterDLAllDrawP0FirstFailedBlocker = 0u;
    gNdsFighterDLAllDrawP1FirstFailedBlocker = 0u;
    gNdsFighterDLAllDrawP0FirstFailedUnsupportedOpcode = 0u;
    gNdsFighterDLAllDrawP1FirstFailedUnsupportedOpcode = 0u;
    gNdsFighterDLAllDrawP0FirstFailedUnsupportedCommandCount = 0u;
    gNdsFighterDLAllDrawP1FirstFailedUnsupportedCommandCount = 0u;
    gNdsFighterDLAllDrawP0FirstFailedVertexRangeRejectCount = 0u;
    gNdsFighterDLAllDrawP1FirstFailedVertexRangeRejectCount = 0u;
    gNdsFighterDLAllDrawP0FirstFailedVertexDecodedCount = 0u;
    gNdsFighterDLAllDrawP1FirstFailedVertexDecodedCount = 0u;
    gNdsFighterDLAllDrawP0FirstFailedTriangleCount = 0u;
    gNdsFighterDLAllDrawP1FirstFailedTriangleCount = 0u;
    gNdsFighterDLAllDrawP0FirstFailedTriangleValidCount = 0u;
    gNdsFighterDLAllDrawP1FirstFailedTriangleValidCount = 0u;
}

static u32 ndsFighterDLAllDrawFailureReason(
    u32 blocker, u32 unsupported_opcode, u32 unsupported_count,
    const NDSFighterDLDrawState *state)
{
    u32 reason = 0u;

    if (blocker != NDS_RENDERER_BLOCKER_NONE)
    {
        reason |= NDS_FIGHTER_DL_ALL_FAIL_BLOCKER;
    }
    if (unsupported_opcode != 0u)
    {
        reason |= NDS_FIGHTER_DL_ALL_FAIL_UNSUPPORTED_OPCODE;
    }
    if (unsupported_count != 0u)
    {
        reason |= NDS_FIGHTER_DL_ALL_FAIL_UNSUPPORTED_COUNT;
    }
    if (state != NULL)
    {
        if (state->vertex_range_reject_count != 0u)
        {
            reason |= NDS_FIGHTER_DL_ALL_FAIL_VERTEX_RANGE;
        }
        if (state->vertex_decoded_count == 0u)
        {
            reason |= NDS_FIGHTER_DL_ALL_FAIL_NO_VERTS;
        }
        if (state->triangle_valid_count == 0u)
        {
            reason |= NDS_FIGHTER_DL_ALL_FAIL_NO_VALID_TRIS;
        }
    }
    if (reason == 0u)
    {
        reason = NDS_FIGHTER_DL_ALL_FAIL_UNKNOWN;
    }
    return reason;
}

static void ndsFighterDLAllDrawRecordFirstFailure(
    u32 slot, u32 selected_index, u32 tree_index, const DObj *dobj,
    const Gfx *dl, u32 reason, u32 blocker, u32 unsupported_opcode,
    u32 unsupported_count, const NDSFighterDLDrawState *state,
    const NDSRendererStats *stats)
{
    if ((reason == 0u) || (state == NULL) || (stats == NULL))
    {
        return;
    }

    if ((slot == 0u) &&
        (gNdsFighterDLAllDrawP0FirstFailedReason == 0u))
    {
        gNdsFighterDLAllDrawP0FirstFailedSelectedIndex = selected_index;
        gNdsFighterDLAllDrawP0FirstFailedTreeIndex = tree_index;
        gNdsFighterDLAllDrawP0FirstFailedReason = reason;
        gNdsFighterDLAllDrawP0FirstFailedDObj = (u32)(uintptr_t)dobj;
        gNdsFighterDLAllDrawP0FirstFailedDL = (u32)(uintptr_t)dl;
        gNdsFighterDLAllDrawP0FirstFailedCommandCount =
            stats->command_count;
        gNdsFighterDLAllDrawP0FirstFailedFirstOpcode =
            stats->first_opcode;
        gNdsFighterDLAllDrawP0FirstFailedBlocker = blocker;
        gNdsFighterDLAllDrawP0FirstFailedUnsupportedOpcode =
            unsupported_opcode;
        gNdsFighterDLAllDrawP0FirstFailedUnsupportedCommandCount =
            unsupported_count;
        gNdsFighterDLAllDrawP0FirstFailedVertexRangeRejectCount =
            state->vertex_range_reject_count;
        gNdsFighterDLAllDrawP0FirstFailedVertexDecodedCount =
            state->vertex_decoded_count;
        gNdsFighterDLAllDrawP0FirstFailedTriangleCount =
            state->triangle_count;
        gNdsFighterDLAllDrawP0FirstFailedTriangleValidCount =
            state->triangle_valid_count;
    }
    else if ((slot == 1u) &&
             (gNdsFighterDLAllDrawP1FirstFailedReason == 0u))
    {
        gNdsFighterDLAllDrawP1FirstFailedSelectedIndex = selected_index;
        gNdsFighterDLAllDrawP1FirstFailedTreeIndex = tree_index;
        gNdsFighterDLAllDrawP1FirstFailedReason = reason;
        gNdsFighterDLAllDrawP1FirstFailedDObj = (u32)(uintptr_t)dobj;
        gNdsFighterDLAllDrawP1FirstFailedDL = (u32)(uintptr_t)dl;
        gNdsFighterDLAllDrawP1FirstFailedCommandCount =
            stats->command_count;
        gNdsFighterDLAllDrawP1FirstFailedFirstOpcode =
            stats->first_opcode;
        gNdsFighterDLAllDrawP1FirstFailedBlocker = blocker;
        gNdsFighterDLAllDrawP1FirstFailedUnsupportedOpcode =
            unsupported_opcode;
        gNdsFighterDLAllDrawP1FirstFailedUnsupportedCommandCount =
            unsupported_count;
        gNdsFighterDLAllDrawP1FirstFailedVertexRangeRejectCount =
            state->vertex_range_reject_count;
        gNdsFighterDLAllDrawP1FirstFailedVertexDecodedCount =
            state->vertex_decoded_count;
        gNdsFighterDLAllDrawP1FirstFailedTriangleCount =
            state->triangle_count;
        gNdsFighterDLAllDrawP1FirstFailedTriangleValidCount =
            state->triangle_valid_count;
    }
}

/* Runs only under `detailed_output`, which the shipped configuration never sets:
 * the c112 cold map found its body spread across two of the driver's largest
 * never-executed runs. Cold, so it stops interleaving with the code that does
 * run. The `#else` arm below still calls it unconditionally, but that arm is the
 * forensic and no-HW builds, which are not performance configurations. */
static void __attribute__((noinline, cold, optimize("Os")))
ndsFighterDLAllDrawAccumulateStats(
    u32 slot, u32 selected_index, u32 tree_index, const DObj *dobj,
    const Gfx *dl, const NDSFighterDLDrawState *state,
    const NDSRendererStats *stats, u8 *clean)
{
    u32 blocker = (stats != NULL) ? stats->blocker : 0xffffffffu;
    u32 unsupported_opcode = 0u;
    u32 unsupported_count = 0u;
    u32 clean_selected;
    u32 failure_reason = 0u;

    if ((state == NULL) || (stats == NULL) || (clean == NULL))
    {
        return;
    }

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
    if (slot == 0u)
    {
        ndsRendererAdapterAccumulateDepth(
            stats,
            &gNdsRendererDepthFighterP0Samples,
            &gNdsRendererDepthFighterP0Min,
            &gNdsRendererDepthFighterP0Max,
            &gNdsRendererDepthFighterP0WMin,
            &gNdsRendererDepthFighterP0WMax);
    }
    else if (slot == 1u)
    {
        ndsRendererAdapterAccumulateDepth(
            stats,
            &gNdsRendererDepthFighterP1Samples,
            &gNdsRendererDepthFighterP1Min,
            &gNdsRendererDepthFighterP1Max,
            &gNdsRendererDepthFighterP1WMin,
            &gNdsRendererDepthFighterP1WMax);
    }
#endif

    gNdsFighterDLAllDrawHardwareTextureBindCount +=
        stats->hardware_texture_bind_count;
    gNdsFighterDLAllDrawHardwareTextureUploadCount +=
        stats->hardware_texture_upload_count;
    gNdsFighterDLAllDrawHardwareTextureReadyCount +=
        stats->hardware_texture_ready_count;
    gNdsFighterDLAllDrawHardwareTextureRejectCount +=
        stats->hardware_texture_reject_count;
    if (stats->hardware_texture_ready_count != 0u)
    {
        if (stats->hardware_texture_format < 32u)
        {
            gNdsFighterDLAllDrawHardwareTextureFormatMask |=
                1u << stats->hardware_texture_format;
        }
        if (stats->hardware_texture_width >
            gNdsFighterDLAllDrawHardwareTextureMaxWidth)
        {
            gNdsFighterDLAllDrawHardwareTextureMaxWidth =
                stats->hardware_texture_width;
        }
        if (stats->hardware_texture_height >
            gNdsFighterDLAllDrawHardwareTextureMaxHeight)
        {
            gNdsFighterDLAllDrawHardwareTextureMaxHeight =
                stats->hardware_texture_height;
        }
    }

    unsupported_opcode = (stats->unsupported_opcode != 0u) ?
        stats->unsupported_opcode : state->unsupported_opcode;
    unsupported_count = stats->unsupported_command_count +
        state->unsupported_command_count;
    clean_selected =
        (blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (unsupported_opcode == 0u) &&
        (unsupported_count == 0u) &&
        (state->vertex_range_reject_count == 0u) &&
        (state->vertex_decoded_count != 0u) &&
        (state->triangle_valid_count != 0u);
    clean[selected_index] = (u8)clean_selected;
    if (clean_selected == FALSE)
    {
        failure_reason = ndsFighterDLAllDrawFailureReason(
            blocker, unsupported_opcode, unsupported_count, state);
        ndsFighterDLAllDrawRecordFirstFailure(
            slot, selected_index, tree_index, dobj, dl, failure_reason,
            blocker, unsupported_opcode, unsupported_count, state, stats);
    }

    if (slot == 0u)
    {
        gNdsFighterDLAllDrawP0AttemptCount++;
        if (clean_selected != FALSE)
        {
            gNdsFighterDLAllDrawP0CleanCount++;
        }
        else
        {
            gNdsFighterDLAllDrawP0FailedCount++;
        }
        if ((blocker != NDS_RENDERER_BLOCKER_NONE) &&
            (gNdsFighterDLAllDrawP0FirstBlocker == 0u))
        {
            gNdsFighterDLAllDrawP0FirstBlocker = blocker;
        }
        if (blocker != NDS_RENDERER_BLOCKER_NONE)
        {
            gNdsFighterDLAllDrawP0BlockerMask |=
                1u << (selected_index & 31u);
        }
        gNdsFighterDLAllDrawP0CommandCount += stats->command_count;
        if (gNdsFighterDLAllDrawP0FirstOpcode == 0u)
        {
            gNdsFighterDLAllDrawP0FirstOpcode = stats->first_opcode;
        }
        if ((unsupported_opcode != 0u) &&
            (gNdsFighterDLAllDrawP0UnsupportedOpcode == 0u))
        {
            gNdsFighterDLAllDrawP0UnsupportedOpcode = unsupported_opcode;
        }
        gNdsFighterDLAllDrawP0UnsupportedCommandCount += unsupported_count;
        gNdsFighterDLAllDrawP0VertexDecodedCount +=
            state->vertex_decoded_count;
        gNdsFighterDLAllDrawP0MatrixMvpRecalcCount +=
            stats->matrix_mvp_recalc_count;
        gNdsFighterDLAllDrawP0MatrixMoveWordCount +=
            stats->matrix_move_word_count;
        gNdsFighterDLAllDrawP0HardwareTriangleCount +=
            stats->hardware_triangle_count;
        gNdsFighterDLAllDrawP0HardwareZBufferTriangleCount +=
            stats->hardware_zbuffer_triangle_count;
        gNdsFighterDLAllDrawP0HardwareProjectedDepthTriangleCount +=
            stats->hardware_projected_depth_triangle_count;
        gNdsFighterDLAllDrawP0HardwareDecalDepthTriangleCount +=
            stats->hardware_decal_depth_triangle_count;
        gNdsFighterDLAllDrawP0HardwareOracleTriangleCount +=
            stats->hardware_oracle_triangle_count;
        gNdsFighterDLAllDrawP0HardwareOracleRejectCount +=
            stats->hardware_oracle_reject_count;
        gNdsFighterDLAllDrawP0HardwareMatrixSeedCount +=
            stats->hardware_matrix_seed_count;
        gNdsFighterDLAllDrawP0TriangleCount += state->triangle_count;
        gNdsFighterDLAllDrawP0TriangleValidCount +=
            state->triangle_valid_count;
        gNdsFighterDLAllDrawP0ColorChecksum =
            (gNdsFighterDLAllDrawP0ColorChecksum * 33u) ^
            state->color_checksum;
    }
    else if (slot == 1u)
    {
        gNdsFighterDLAllDrawP1AttemptCount++;
        if (clean_selected != FALSE)
        {
            gNdsFighterDLAllDrawP1CleanCount++;
        }
        else
        {
            gNdsFighterDLAllDrawP1FailedCount++;
        }
        if ((blocker != NDS_RENDERER_BLOCKER_NONE) &&
            (gNdsFighterDLAllDrawP1FirstBlocker == 0u))
        {
            gNdsFighterDLAllDrawP1FirstBlocker = blocker;
        }
        if (blocker != NDS_RENDERER_BLOCKER_NONE)
        {
            gNdsFighterDLAllDrawP1BlockerMask |=
                1u << (selected_index & 31u);
        }
        gNdsFighterDLAllDrawP1CommandCount += stats->command_count;
        if (gNdsFighterDLAllDrawP1FirstOpcode == 0u)
        {
            gNdsFighterDLAllDrawP1FirstOpcode = stats->first_opcode;
        }
        if ((unsupported_opcode != 0u) &&
            (gNdsFighterDLAllDrawP1UnsupportedOpcode == 0u))
        {
            gNdsFighterDLAllDrawP1UnsupportedOpcode = unsupported_opcode;
        }
        gNdsFighterDLAllDrawP1UnsupportedCommandCount += unsupported_count;
        gNdsFighterDLAllDrawP1VertexDecodedCount +=
            state->vertex_decoded_count;
        gNdsFighterDLAllDrawP1MatrixMvpRecalcCount +=
            stats->matrix_mvp_recalc_count;
        gNdsFighterDLAllDrawP1MatrixMoveWordCount +=
            stats->matrix_move_word_count;
        gNdsFighterDLAllDrawP1HardwareTriangleCount +=
            stats->hardware_triangle_count;
        gNdsFighterDLAllDrawP1HardwareZBufferTriangleCount +=
            stats->hardware_zbuffer_triangle_count;
        gNdsFighterDLAllDrawP1HardwareProjectedDepthTriangleCount +=
            stats->hardware_projected_depth_triangle_count;
        gNdsFighterDLAllDrawP1HardwareDecalDepthTriangleCount +=
            stats->hardware_decal_depth_triangle_count;
        gNdsFighterDLAllDrawP1HardwareOracleTriangleCount +=
            stats->hardware_oracle_triangle_count;
        gNdsFighterDLAllDrawP1HardwareOracleRejectCount +=
            stats->hardware_oracle_reject_count;
        gNdsFighterDLAllDrawP1HardwareMatrixSeedCount +=
            stats->hardware_matrix_seed_count;
        gNdsFighterDLAllDrawP1TriangleCount += state->triangle_count;
        gNdsFighterDLAllDrawP1TriangleValidCount +=
            state->triangle_valid_count;
        gNdsFighterDLAllDrawP1ColorChecksum =
            (gNdsFighterDLAllDrawP1ColorChecksum * 33u) ^
            state->color_checksum;
    }

    gNdsFighterDLAllDrawVertexRangeRejectCount +=
        state->vertex_range_reject_count;
}

#if NDS_TASK91_DRAW_PHASE_CENSUS
/* Task 91 E1. The M2 phase ledger already measures this split but is restricted
 * to profile level 1 (nds_renderer.h:39) and its one target overrides
 * FAST_RUN_DEFAULT, so it cannot referee the Boundary configuration. These
 * three counters are the minimum that can: ticks spent rediscovering the DObj
 * tree, ticks spent revalidating that it still matches the generated program,
 * and the call count to normalise both. Lab only, default off. */
u32 gNdsTask91WalkTicks;
u32 gNdsTask91ValidateTicks;
u32 gNdsTask91DrawCalls;
u32 gNdsTask91NativeEligible;
/* R2-03 E32. Splits the shared AnimLock fallback reason into its two causes. */
u32 gNdsR2FallbackShuffleTics;
u32 gNdsR2FallbackAnimLocks;
/* R2-03 E3. E2 measured the walk and the revalidation at 3,289 + 10,381
 * ticks/frame against the 37,206 the symbol census charges to this function,
 * and the walk is a separate symbol -- so more than half of the function's own
 * body was unmeasured, and R2-02 E3's lesson is that the unmeasured half is
 * where the answer lives. Total brackets the whole call so the residual is
 * arithmetic rather than assumption; Reset covers the three bzeros plus the
 * vertex-cache and stats initialisation; OwnerPrep covers the matrix and
 * material preparation between the revalidation and the submit. */
u32 gNdsTask91TotalTicks;
u32 gNdsTask91ResetTicks;
u32 gNdsTask91OwnerPrepTicks;
/* R2-03 E4. OwnerPrep's two halves, mirroring the profile-level-1 split the
 * tick-HUD target cannot compile. */
u32 gNdsTask91MatrixPrepTicks;
u32 gNdsTask91MaterialPrepTicks;
/* R2-03 E6's MatrixPrep sub-counters are declared above
 * ndsRendererAdapterPrepareNativeOwnerMatrices, which is defined earlier in
 * this file than this block. */
/* R2-03 E14. E13 measured the fighter draw at 500,833 ticks/frame and its named
 * phases at 164,803 of that, leaving 336,030 -- 67% -- charged to a residual
 * that has never been bracketed, because E3's split stopped at the point the
 * owner inputs are built. These two counters cover everything past it: building
 * the production inputs, and executing them against GX. Whatever those two do
 * not account for is the trailing bookkeeping, by arithmetic. */
u32 gNdsTask91InputsTicks;
u32 gNdsTask91ExecuteTicks;
/* R2-03 E14b. Execute is 279,617 ticks/frame for ~626 triangles, and the cut
 * that follows depends entirely on which side of the FIFO is slow. GXSTAT bits
 * 16-24 are the command FIFO's 40-bit entry count (0..256) and bit 27 is the
 * geometry engine busy flag, sampled either side of the submission:
 *   near-full at the end  -> the CPU is outrunning the geometry engine and is
 *                            being throttled; only submitting less geometry
 *                            helps, which is a visual-fidelity trade.
 *   near-empty at the end -> the geometry engine is starved waiting for us and
 *                            the cost is our own ARM9 work, which a faster
 *                            emitter (R2-02 E2's GXFIFO DMA) addresses.
 * Two register reads per fighter per frame, so this cannot itself perturb the
 * thing it measures. */
/* This translation unit is source-compatibility code and does not pull in
 * <nds.h>, so GXSTAT is named by address rather than by adding a libnds include
 * to it for a lab counter. */
#define NDS_TASK91_GXSTAT (*(volatile u32 *)0x04000600u)
u32 gNdsTask91GxFifoSamples;
u32 gNdsTask91GxFifoEntriesEnd;
u32 gNdsTask91GxFifoEntriesStart;
u32 gNdsTask91GxFifoMaxEnd;
u32 gNdsTask91GxBusyEnd;
/* Positive control, because "every FIFO field reads zero" is also exactly what
 * a probe pointed at nothing produces. GXSTAT bit 26 is "command FIFO empty",
 * so a live register over a genuinely drained FIFO must OR to at least
 * 0x04000000; an OR of 0 means the register was never read. */
u32 gNdsTask91GxStatOr;
#endif

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
/* ---------------------------------------------------------------------------
 * Cycle 99 -- the baked fighter draw plan.
 *
 * Cycle 98 measured the ordinary DObj collection as stable over its both-CPU
 * match. That remains the dominant case, but it was not a lifetime proof:
 * ftMainSetStatus's enabled-joint path can replace/re-parent DObjs during a live
 * fighter status without changing the reloc file identity. The plan therefore
 * treats the collection as stable only inside one renderer status generation.
 * Everything the eligibility pass then derives from that collection is a
 * function of it plus the owner asset file -- the resolved NDSRelocLoadedFile,
 * the root offsets, the material counts, the matrix bindings and the material
 * DObjs. Between status changes the pass is still re-proving a constant once per
 * selected root per fighter per frame, with a loaded-file search at its centre.
 * Charter R2-03 names exactly this ("no PrepareProductionRun policy re-checks,
 * no per-frame texture identity proof").
 *
 * This bakes that derivation once per (slot, owner-asset identity) and replays
 * it. It deletes, per fighter per frame: the collection walk, the whole
 * eligibility loop, and ndsRendererAdapterValidateNativeOwnerCached.
 *
 * Why dropping the validate is equivalence and not a shortcut: the plan key IS
 * that validator's identity key (data / asset_id / owner_generation /
 * data_size), and the offsets and material counts it compares are the very
 * arrays the plan carries. A plan hit therefore implies the validator's
 * identity path would have hit and returned TRUE without doing anything else.
 *
 * Section 3.12: the plan is keyed on the loaded file's identity and is
 * additionally cleared from ndsRendererAdapterResetSceneCaches, so a scene
 * entry -- including a START-restart out of Results -- re-derives it. It also
 * carries sNdsFighterStatusGeneration: ftMainSetStatus can replace/re-parent
 * hidden-part DObjs without changing the loaded fighter file, and the plan
 * stores both matrix_bindings and material_dobjs. A source status change must
 * therefore miss this cache even when every reloc identity word still matches.
 *
 * gNdsFtrPlanRoute remains runtime-selectable for same-binary A/B; the shipping
 * default is route 1, so the lifetime key above is part of the correctness
 * contract rather than lab-only instrumentation.
 * ------------------------------------------------------------------------- */

typedef enum NDSFighterDrawPlanResult
{
    nNDSFighterDrawPlanOk = 0,
    nNDSFighterDrawPlanSelected,
    nNDSFighterDrawPlanDisplayList,
    nNDSFighterDrawPlanMaterialCount
} NDSFighterDrawPlanResult;

/* Derived data only. The memcmp verification below compares this whole struct,
 * so it must contain nothing that is legitimately per-frame. */
typedef struct NDSFighterDrawPlanData
{
    NDSFighterDLAllDrawCollection collection;
    NDSRelocLoadedFile *owner_file;
    NDSRelocLoadedFile *loaded[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    u32 root_offsets[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    u32 material_counts[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    DObj *matrix_bindings[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    DObj *material_dobjs[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
} NDSFighterDrawPlanData;

typedef struct NDSFighterDrawPlan
{
    NDSFighterDrawPlanData data;
    const void *key_data;
    u32 key_asset_id;
    u32 key_owner_generation;
    u32 key_data_size;
    u32 key_status_generation;
    u32 key_use_low_detail;
    u32 valid;
} NDSFighterDrawPlan;

static NDSFighterDrawPlan sNdsFighterDrawPlan[GMCOMMON_PLAYERS_MAX];

static void ndsFighterDrawPlanInvalidate(void)
{
    u32 slot;

    for (slot = 0u; slot < GMCOMMON_PLAYERS_MAX; slot++)
    {
        sNdsFighterDrawPlan[slot].valid = 0u;
    }
}

static sb32 ndsFighterDrawPlanHit(u32 slot, u32 use_low_detail)
{
    const NDSFighterDrawPlan *plan;
    const NDSRelocLoadedFile *file;

    if ((gNdsFtrPlanRoute == 0u) || (slot >= GMCOMMON_PLAYERS_MAX))
    {
        return FALSE;
    }
    plan = &sNdsFighterDrawPlan[slot];
    file = plan->data.owner_file;
    if ((plan->valid == 0u) || (file == NULL))
    {
        return FALSE;
    }
    return ((file->data == plan->key_data) &&
            (file->asset_id == plan->key_asset_id) &&
            (file->owner_generation == plan->key_owner_generation) &&
            (file->data_size == plan->key_data_size) &&
            (plan->key_use_low_detail == use_low_detail) &&
            (plan->key_status_generation ==
             sNdsFighterStatusGeneration[slot])) ? TRUE : FALSE;
}

/* The eligibility pass, lifted out of the draw so the live path, the capture
 * and the verification all run one copy of it. It writes the same workspace
 * fields, in the same order, that the inline loop wrote, so route 0 performs
 * byte-identical work to the build this replaces. Read-only apart from the
 * workspace, which is what makes it safe to run twice in the verify arm. */
static NDSFighterDrawPlanResult ndsFighterDrawPlanResolve(
    u32 expected_asset_id,
    const NDSFighterDLAllDrawCollection *collection,
    NDSRendererAdapterNativeOwnerWorkspace *workspace,
    NDSRelocLoadedFile **out_owner_file)
{
    NDSRelocLoadedFile *owner_file = NULL;
    u32 i;
    *out_owner_file = NULL;
    if ((collection->selected_count == 0u) ||
        (collection->selected_count > NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED))
    {
        return nNDSFighterDrawPlanSelected;
    }
    for (i = 0u; i < collection->selected_count; i++)
    {
        const NDSFighterDisplayContractEvent *event =
            (sNdsFighterDisplayContractPlayback != FALSE) ?
                &sNdsFighterDisplayReplayEvents[
                    collection->indices[i]] : NULL;
        const Gfx *native_dl =
            (event != NULL) ? event->dl : collection->dobjs[i]->dl;
        DObj *material_dobj =
            (event != NULL) ? event->material_dobj : collection->dobjs[i];
        NDSRelocLoadedFile *loaded =
            ndsRelocFindLoadedFileContaining(
                native_dl, sizeof(*native_dl));
        MObj *mobj;
        u32 material_count = 0u;

        if ((native_dl == NULL) || (loaded == NULL) ||
            (loaded->data == NULL) ||
            (loaded->asset_id != expected_asset_id) ||
            ((owner_file != NULL) && (loaded != owner_file)) ||
            (loaded->data_size < sizeof(*native_dl)) ||
            ((uintptr_t)native_dl < (uintptr_t)loaded->data) ||
            (((uintptr_t)native_dl - (uintptr_t)loaded->data) >
             (loaded->data_size - sizeof(*native_dl))))
        {
            *out_owner_file = owner_file;
            return nNDSFighterDrawPlanDisplayList;
        }
        owner_file = loaded;
        workspace->loaded[i] = loaded;
        workspace->root_offsets[i] =
            (u32)((uintptr_t)native_dl - (uintptr_t)loaded->data);
        workspace->matrix_bindings[i] =
            (event != NULL) ? event->matrix_dobj : collection->dobjs[i];
        workspace->material_dobjs[i] = material_dobj;
        for (mobj = (material_dobj != NULL) ? material_dobj->mobj : NULL;
             mobj != NULL;
             mobj = mobj->next)
        {
            material_count++;
        }
        if (material_count > NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX)
        {
            workspace->material_counts[i] = material_count;
            *out_owner_file = owner_file;
            return nNDSFighterDrawPlanMaterialCount;
        }
        workspace->material_counts[i] = material_count;
    }
    *out_owner_file = owner_file;
    return nNDSFighterDrawPlanOk;
}

/* Gather the resolved prefix out of the workspace. Only the used prefix is
 * touched; the tail beyond selected_count is never read by anything, and is
 * left zeroed so a whole-struct memcmp is meaningful. */
static void ndsFighterDrawPlanGather(
    const NDSFighterDLAllDrawCollection *collection,
    NDSRelocLoadedFile *owner_file,
    const NDSRendererAdapterNativeOwnerWorkspace *workspace,
    NDSFighterDrawPlanData *out)
{
    u32 i;

    bzero(out, sizeof(*out));
    out->collection = *collection;
    out->owner_file = owner_file;
    for (i = 0u; i < collection->selected_count; i++)
    {
        out->loaded[i] = workspace->loaded[i];
        out->root_offsets[i] = workspace->root_offsets[i];
        out->material_counts[i] = workspace->material_counts[i];
        out->matrix_bindings[i] = workspace->matrix_bindings[i];
        out->material_dobjs[i] = workspace->material_dobjs[i];
    }
}

/* Replay the baked plan into the shared owner workspace the rest of the draw
 * reads. This is the whole per-frame cost of the baked route. */
static void ndsFighterDrawPlanApply(
    const NDSFighterDrawPlanData *plan,
    NDSRendererAdapterNativeOwnerWorkspace *workspace)
{
    u32 i;

    for (i = 0u; i < plan->collection.selected_count; i++)
    {
        workspace->loaded[i] = plan->loaded[i];
        workspace->root_offsets[i] = plan->root_offsets[i];
        workspace->material_counts[i] = plan->material_counts[i];
        workspace->matrix_bindings[i] = plan->matrix_bindings[i];
        workspace->material_dobjs[i] = plan->material_dobjs[i];
    }
}

#if NDS_TICK_HUD
/* Equivalence by construction, and it supersedes any variance rate: on a draw
 * that took the baked path, derive the plan live and memcmp it against the
 * baked one. Zero mismatches over a whole match is the claim, and it covers
 * fields no cycle-98 counter covered (material_dobj, matrix_dobj, the resolved
 * file and the root offsets). Armed only by gNdsFtrPlanVerify, because
 * computing both paths is exactly what a tick measurement must not do. The
 * caller applies the baked plan AFTER this returns, so the workspace this
 * scribbles on is overwritten before it is read. */
static NDSFighterDrawPlanData sNdsFighterDrawPlanVerifyScratch;

static void ndsFighterDrawPlanVerify(
    u32 slot, DObj *root, u32 expected_asset_id,
    NDSRendererAdapterNativeOwnerWorkspace *workspace)
{
    NDSFighterDrawPlanData *scratch = &sNdsFighterDrawPlanVerifyScratch;
    NDSFighterDLAllDrawCollection live;
    NDSRelocLoadedFile *owner_file = NULL;

    ndsFighterCollectAllDObjsWithDL(root, &live);
    (void)ndsFighterDrawPlanResolve(
        expected_asset_id, &live, workspace, &owner_file);
    ndsFighterDrawPlanGather(&live, owner_file, workspace, scratch);
    gNdsFtrPlanVerifyRuns++;
    if (memcmp(scratch, &sNdsFighterDrawPlan[slot].data,
               sizeof(*scratch)) != 0)
    {
        gNdsFtrPlanVerifyMismatch++;
    }
}
#endif
#endif

/* P2-2 separates two values that were accidentally identical in the old
 * two-fighter match: the battle PLAYER slot (0..3) and the generated native
 * OWNER slot (0 Mario, 1 Fox, then P2-3 roster owners). Instance caches use
 * the former; generated topology/material tables and native renderer entry
 * points use the latter. Keep fighter-kind admission here so adding a source
 * inventory cannot silently make a half-integrated owner drawable. */
static sb32 ndsFighterGetNativeOwnerSlot(const FTStruct *fp, u32 *owner_slot)
{
    if ((fp == NULL) || (owner_slot == NULL))
    {
        return FALSE;
    }
    if (fp->fkind == nFTKindMario)
    {
        *owner_slot = 0u;
        return TRUE;
    }
    if (fp->fkind == nFTKindFox)
    {
        *owner_slot = 1u;
        return TRUE;
    }
#if NDS_P2_LUIGI
    if (fp->fkind == nFTKindLuigi)
    {
        *owner_slot = 2u;
        return TRUE;
    }
#endif
#if NDS_P2_DONKEY
    if (fp->fkind == nFTKindDonkey)
    {
        *owner_slot = 3u;
        return TRUE;
    }
#endif
    return FALSE;
}

static u32 ndsFighterNativeOwnerModelAssetId(u32 owner_slot)
{
    if (owner_slot == 0u)
    {
        return 0x128u; /* llMarioModelFileID */
    }
    if (owner_slot == 1u)
    {
        return 0x139u; /* llFoxModelFileID */
    }
#if NDS_P2_LUIGI
    if (owner_slot == 2u)
    {
        return 0x143u; /* llLuigiModelFileID, BattleShip dFTLuigiData */
    }
#endif
#if NDS_P2_DONKEY
    if (owner_slot == 3u)
    {
        return 0x13du; /* llDonkeyModelFileID, BattleShip dFTDonkeyData */
    }
#endif
    return 0u;
}

static NDSRendererProfileOwner ndsFighterNativeOwnerProfileId(u32 owner_slot)
{
    if (owner_slot == 0u)
    {
        return NDS_RENDERER_PROFILE_OWNER_MARIO;
    }
    if (owner_slot == 1u)
    {
        return NDS_RENDERER_PROFILE_OWNER_FOX;
    }
#if NDS_P2_LUIGI
    if (owner_slot == 2u)
    {
        return NDS_RENDERER_PROFILE_OWNER_LUIGI;
    }
#endif
#if NDS_P2_DONKEY
    if (owner_slot == 3u)
    {
        return NDS_RENDERER_PROFILE_OWNER_DONKEY;
    }
#endif
    return NDS_RENDERER_PROFILE_OWNER_NONE;
}

static void ndsFighterMarioFoxDLAllDrawForSlot(u32 slot, FTStruct *fp,
                                               u16 *pixels, u32 pitch)
{
#if NDS_TASK91_DRAW_PHASE_CENSUS
    u32 task91_phase_start;
    u32 task91_total_start;
    u32 task91_mark;
#endif
    DObj *root;
    NDSFighterDLAllDrawCollection collection;
    NDSFighterDLDrawState *states;
    NDSFighterDLDrawState persistent_state;
    /* The snapshot table is traversal-owned but too large for BattleShip's
     * nested task stack. Draw callbacks are serialized, so one reset fixed
     * cache preserves the same per-fighter lifetime without caching output. */
    static NDSRendererVertexCache persistent_renderer_vertices;
#if !NDS_RENDERER_HW_TRIANGLES || (NDS_RENDERER_PROFILE_LEVEL >= 2)
    NDSRendererStats *stats;
#endif
    NDSRendererStats persistent_stats;
    u8 *clean;
    sb32 no_oracle;
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    sb32 detailed_output;
    u32 runtime_hardware_triangle_count = 0u;
#endif
    u32 root_x_before;
    u32 root_x_after;
    u32 color_modulate;
    u32 owner_slot;
    u32 use_low_detail;
    u32 i;
#if NDS_RENDERER_HW_TRIANGLES
    NDSRendererProfileOwner owner_id;
    u32 expected_asset_id;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    volatile NDSRendererOwnerProfile *m2_owner;
    u32 m2_phase_start;
#endif
    NDSRelocLoadedFile *native_owner_file = NULL;
    NDSRelocLoadedFile **native_owner_loaded =
        sNdsRendererAdapterNativeOwnerWorkspace.loaded;
    u32 *native_owner_root_offsets =
        sNdsRendererAdapterNativeOwnerWorkspace.root_offsets;
    u32 *native_owner_material_counts =
        sNdsRendererAdapterNativeOwnerWorkspace.material_counts;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    NDSRendererNativeMaterial *native_materials =
        &sNdsRendererAdapterNativeOwnerMaterials[0u][0u];
    DObj **native_owner_matrix_bindings =
        sNdsRendererAdapterNativeOwnerWorkspace.matrix_bindings;
    DObj **native_owner_material_dobjs =
        sNdsRendererAdapterNativeOwnerWorkspace.material_dobjs;
    const NDSRendererMatrix20p12 **native_owner_modelviews =
        sNdsRendererAdapterNativeOwnerWorkspace.modelviews;
    const NDSRendererMatrix20p12 *native_owner_projection = NULL;
    u32 native_owner_material_saved_root_count = 0u;
    sb32 native_owner_started = FALSE;
    sb32 native_owner_failed = FALSE;
    sb32 native_owner_production_attempted = FALSE;
    sb32 native_owner_production_mode;
    sb32 native_owner_hierarchy_mode;
    /* Cycle 99. TRUE == this draw replayed the baked plan instead of walking
     * the DObj tree and re-resolving every selected root. */
    sb32 native_owner_plan_hit = FALSE;
    /* P2-2p4. TRUE == the renderer predicted a packet replay for this draw,
     * so the material rows/snapshots were skipped and the production inputs
     * are already built. */
    sb32 native_owner_packet_predicted = FALSE;
    u32 native_owner_texture_key = 0u;
#else
    NDSRendererNativeMaterial *native_materials =
        sNdsRendererAdapterNativeMaterials;
#endif
    u32 native_material_count = 0u;
    sb32 native_owner_enabled;
#endif
#if NDS_R2_DRAW_SUPPRESS_MASK
    /* R2-03 E13. Prices a whole fighter: the phase census says how the draw
     * divides internally, this says what the frame costs without it at all.
     * Engagement is self-proving -- the slot's hardware triangle count and its
     * half of gNdsTask91DrawCalls both go to zero. */
    if ((slot < GMCOMMON_PLAYERS_MAX) &&
        (((NDS_R2_DRAW_SUPPRESS_MASK >> slot) & 1u) != 0u))
    {
        return;
    }
#endif

    if ((slot >= GMCOMMON_PLAYERS_MAX) ||
        (ndsFighterStructIsTrackedPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL) ||
        (ndsFighterGetNativeOwnerSlot(fp, &owner_slot) == FALSE) ||
        ((pixels != NULL) &&
         ((fp->status_id != nFTCommonStatusWait) ||
          (fp->motion_id != nFTCommonMotionWait) ||
          (fp->ga != nMPKineticsGround))))
    {
        return;
    }

    /* BattleShip selects the Low JointTree for every 3+ fighter VSBattle
     * descriptor (scvsbattle.c:188/:460).  Carry that source decision into the
     * AOT renderer rather than inferring it from battle slot/count here. */
    use_low_detail = (fp->detail_curr == nFTPartsDetailLow) ? TRUE : FALSE;
    root = fp->joints[nFTPartsJointTopN];
    color_modulate = ndsRendererAdapterFighterColorModulate(fp);
    root_x_before = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) :
        0u;

#if NDS_RENDERER_HW_TRIANGLES
    owner_id = ndsFighterNativeOwnerProfileId(owner_slot);
    expected_asset_id = ndsFighterNativeOwnerModelAssetId(owner_slot);
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    m2_owner = &gNdsRendererProfileOwners[(u32)owner_id];
    m2_phase_start = cpuGetTiming();
#endif
#endif
#if NDS_TASK91_DRAW_PHASE_CENSUS
    gNdsTask91DrawCalls++;
    task91_total_start = cpuGetTiming();
    task91_phase_start = task91_total_start;
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    native_owner_plan_hit = ndsFighterDrawPlanHit(slot, use_low_detail);
    if (native_owner_plan_hit != FALSE)
    {
        /* The walk is deleted here, not memoised: the collection is a
         * match-load constant (cycle 98, 3,961 same / 0 variant) and the plan
         * carries the one it produced. */
        collection = sNdsFighterDrawPlan[slot].data.collection;
    }
    else
    {
        ndsFighterCollectAllDObjsWithDL(root, &collection);
    }
#else
    ndsFighterCollectAllDObjsWithDL(root, &collection);
#endif
#if NDS_TASK91_DRAW_PHASE_CENSUS
    gNdsTask91WalkTicks += cpuGetTiming() - task91_phase_start;
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    m2_owner->m2_collection_ticks += cpuGetTiming() - m2_phase_start;
    m2_phase_start = cpuGetTiming();
    ndsRendererAdapterM2CensusFighter(
        owner_slot, fp, root, &collection, m2_owner);
    m2_owner->m2_census_ticks += cpuGetTiming() - m2_phase_start;
#endif
    if (slot == 0u)
    {
        gNdsFighterDLAllDrawP0CandidateCount = collection.total_count;
        gNdsFighterDLAllDrawP0SelectedCount = collection.selected_count;
        gNdsFighterDLAllDrawP0SelectedIndexMask =
            collection.selected_index_mask;
    }
    else if (slot == 1u)
    {
        gNdsFighterDLAllDrawP1CandidateCount = collection.total_count;
        gNdsFighterDLAllDrawP1SelectedCount = collection.selected_count;
        gNdsFighterDLAllDrawP1SelectedIndexMask =
            collection.selected_index_mask;
    }
#if NDS_TICK_HUD
    /* Cycle 98. Placed after the publish so it sees the same collection the
     * rest of the draw does, and before any early-out below, so
     * Same+Variant+First is exactly the number of draws that reached here --
     * which is what makes it checkable against
     * gNdsFighterMarioFoxDLAllDrawCount rather than merely plausible. */
    ndsFtrPreWalkCensus(slot, &collection);
#endif

#if NDS_TASK91_DRAW_PHASE_CENSUS
    /* Reset opens here. The bzeros a few lines down land in the memset symbol,
     * not this function's, so the E0 census cannot see them from the top-45
     * table -- memset is 38,393 ticks/frame across the whole program and
     * nothing says how much of it is this.
     *
     * ANSWERED 2026-07-30 (R2-07 R4c) by sampling $lr at a memset breakpoint on
     * the Results lab, 80 dynamic hits: HALF of every memset call in the scene
     * comes from this function, and another 18.8% from
     * ndsRendererAdapterBuildNativeMaterialSnapshot -- so memset's 8.80% of the
     * Results frame is ~69% fighter-draw work and is NOT a separate subsystem
     * to attack. Within this function the three bzeros below are only 15% of
     * its memset traffic; 70% is two call sites in
     * ndsFighterDLDrawResetTransientRendererStats (:4748), which clears the
     * per-list proof/counter prefix once per part list per fighter per frame.
     * That reset runs only when `detailed_output` is set, i.e. when the oracle
     * is on -- and the stage draw already turns the oracle OFF around its own
     * draw (reloc_backend_movement.c:13559) while the fighter draw leaves it
     * on. Static `bl memset` counts cannot show any of this: 96 functions call
     * memset and the hot ones are not the ones with the most call sites. */
    task91_phase_start = cpuGetTiming();
#endif
    states = sNdsFighterDLAllDrawStates;
#if !NDS_RENDERER_HW_TRIANGLES || (NDS_RENDERER_PROFILE_LEVEL >= 2)
    stats = sNdsFighterDLAllDrawStats[slot];
#endif
    clean = sNdsFighterDLAllDrawClean[slot];
    no_oracle = (ndsRendererHardwareNoOracleEnabled() != FALSE) ? TRUE :
                                                                    FALSE;
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    detailed_output = ((pixels != NULL) || (no_oracle == FALSE)) ? TRUE :
                                                                       FALSE;
    if (detailed_output != FALSE)
    {
        bzero(states, sizeof(sNdsFighterDLAllDrawStates));
        bzero(&persistent_state, sizeof(persistent_state));
        bzero(clean, sizeof(sNdsFighterDLAllDrawClean[slot]));
    }
    else
    {
        /* The shipping callback is null, so only the resolver's segment
         * ownership crosses source part lists. Do not clear/copy the
         * software-preview vertices or update its historical proof ledger. */
        persistent_state.segment_e_base = NULL;
        persistent_state.segment_e_end = NULL;
    }
#else
    bzero(states, sizeof(sNdsFighterDLAllDrawStates));
    bzero(&persistent_state, sizeof(persistent_state));
    bzero(clean, sizeof(sNdsFighterDLAllDrawClean[slot]));
#endif
    ndsRendererInitVertexCache(&persistent_renderer_vertices);
    ndsRendererInitStats(&persistent_stats);
    if (sNdsFighterDisplayContractPlayback != FALSE)
    {
        persistent_stats.geometry_mode =
            sNdsFighterDisplayContract.geometry_mode;
        persistent_stats.prim_color = sNdsFighterDisplayContract.prim_color;
        persistent_stats.env_color = sNdsFighterDisplayContract.env_color;
        if (sNdsFighterDisplayContract.light_valid != 0u)
        {
            persistent_stats.light_dir_x =
                sNdsFighterDisplayContract.light.l.dir[0];
            persistent_stats.light_dir_y =
                sNdsFighterDisplayContract.light.l.dir[1];
            persistent_stats.light_dir_z =
                sNdsFighterDisplayContract.light.l.dir[2];
            persistent_stats.light_dir_mask = 1u;
        }
        ndsFighterDisplayContractSeedMaterialLights(&persistent_stats);
    }
#if NDS_RENDERER_HW_TRIANGLES
    native_owner_enabled =
        (((gNdsRendererFastRunMode ==
           NDS_RENDERER_FAST_RUN_NATIVE_MARIO) && (owner_slot == 0u)) ||
         ((gNdsRendererFastRunMode ==
           NDS_RENDERER_FAST_RUN_NATIVE_FOX) && (owner_slot == 1u)) ||
          (gNdsRendererFastRunMode ==
           NDS_RENDERER_FAST_RUN_NATIVE_FIGHTERS) ||
          (gNdsRendererFastRunMode ==
           NDS_RENDERER_FAST_RUN_NATIVE_FIGHTER_OWNER_PRODUCTION) ||
          (gNdsRendererFastRunMode ==
           NDS_RENDERER_FAST_RUN_NATIVE_COMPLETE_STAGE)) ? TRUE :
                                                                    FALSE;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    native_owner_production_mode =
        ((gNdsRendererFastRunMode ==
          NDS_RENDERER_FAST_RUN_NATIVE_FIGHTER_OWNER_PRODUCTION) ||
         (gNdsRendererFastRunMode ==
          NDS_RENDERER_FAST_RUN_NATIVE_COMPLETE_STAGE)) ? TRUE :
                                                                   FALSE;
    native_owner_hierarchy_mode =
        (gNdsRendererFastRunMode ==
         NDS_RENDERER_FAST_RUN_NATIVE_FIGHTERS) ? TRUE : FALSE;
    if ((native_owner_hierarchy_mode != FALSE) && (use_low_detail != 0u))
    {
        /* Mode 7's hierarchy schedule is generated only from the high-detail
         * JointTree.  A 3+ fighter source match is Low, so fail closed to the
         * ordinary renderer instead of combining mismatched source geometry and
         * a high-detail hierarchy experiment.  Shipping mode 9 has a Low AOT
         * program and does not take this branch. */
        native_owner_enabled = FALSE;
    }
#if NDS_TICK_HUD
#if NDS_TASK91_DRAW_PHASE_CENSUS
    /* Closing Reset also re-arms the mark, so OwnerPrep is well defined even on
     * the ~1.7% of calls where the native-owner block is skipped entirely. */
    task91_mark = cpuGetTiming();
    gNdsTask91ResetTicks += task91_mark - task91_phase_start;
    task91_phase_start = task91_mark;
#endif
    NDS_TICK_HUD_NATIVE_OWNER_MARK(nNDSTickHudNativeOwnerFallbackCalls);
    if (native_owner_enabled != FALSE)
    {
        NDS_TICK_HUD_NATIVE_OWNER_MARK(
            nNDSTickHudNativeOwnerFallbackEligible);
    }
#endif
    if ((native_owner_enabled != FALSE) &&
        ((native_owner_production_mode != FALSE) ||
         (native_owner_hierarchy_mode != FALSE)) &&
#if NDS_R2_FIGHTER_SHUFFLE_FOLD
        /* R2-03 E32. The shuffle is folded into the world matrix in
         * ndsRendererAdapterPrepareNativeOwnerMatrices, so it no longer costs
         * the fighter its native path. Animation locks still do -- E32's census
         * measured 5 shuffle fallbacks and 0 animlock fallbacks over frames
         * 460..500, so the remaining half of this condition is unexercised in
         * the Boundary scene and stays conservative. */
        (fp->is_use_animlocks != FALSE))
#else
        ((fp->is_use_animlocks != FALSE) || (fp->shuffle_tics != 0u)))
#endif
    {
        native_owner_enabled = FALSE;
#if NDS_TICK_HUD
        NDS_TICK_HUD_NATIVE_OWNER_FALLBACK(
            nNDSTickHudNativeOwnerFallbackAnimLock);
#endif
#if NDS_TASK91_DRAW_PHASE_CENSUS
        /* R2-03 E32. The reason code above is shared by both halves of the
         * disjunction, and they need different fixes: the hitlag shuffle is one
         * whole-model translate the owner can absorb, animation locks are not.
         * E31 measured 5 AnimLock fallbacks over frames 460..500 without being
         * able to say which. */
        if (fp->shuffle_tics != 0u) { gNdsR2FallbackShuffleTics++; }
        if (fp->is_use_animlocks != FALSE) { gNdsR2FallbackAnimLocks++; }
#endif
    }
#endif
#if NDS_R2_FIGHTER_SHUFFLE_FOLD
    /* R2-03 E32. Latch this fighter's hitlag offset for the matrix prep below.
     * Set unconditionally so it is cleared to zero on the ordinary frames. */
    ndsRendererAdapterSetShuffleOffset(fp);
#endif
    ndsRendererProfileSetOwner(owner_id);
    if (native_owner_enabled != FALSE)
    {
#if NDS_TASK91_DRAW_PHASE_CENSUS
        task91_phase_start = cpuGetTiming();
#endif
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        m2_phase_start = cpuGetTiming();
#endif

#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (native_owner_plan_hit != FALSE)
        {
            /* THE DELETION. The eligibility pass and
             * ndsRendererAdapterValidateNativeOwnerCached are both skipped
             * outright, not memoised. The plan key is that validator's own
             * identity key and the plan carries the offsets and material
             * counts it compares, so a plan hit implies its identity path
             * would have hit and returned TRUE without doing anything else. */
#if NDS_TICK_HUD
            gNdsFtrPlanHit++;
            if (gNdsFtrPlanVerify != 0u)
            {
                ndsFighterDrawPlanVerify(
                    slot, root, expected_asset_id,
                    &sNdsRendererAdapterNativeOwnerWorkspace);
            }
#endif
            native_owner_file = sNdsFighterDrawPlan[slot].data.owner_file;
            ndsFighterDrawPlanApply(
                &sNdsFighterDrawPlan[slot].data,
                &sNdsRendererAdapterNativeOwnerWorkspace);
        }
        else
        {
            NDSFighterDrawPlanResult plan_result =
                ndsFighterDrawPlanResolve(
                    expected_asset_id, &collection,
                    &sNdsRendererAdapterNativeOwnerWorkspace,
                    &native_owner_file);

            if (plan_result != nNDSFighterDrawPlanOk)
            {
                native_owner_enabled = FALSE;
#if NDS_TICK_HUD
                if (plan_result == nNDSFighterDrawPlanSelected)
                {
                    NDS_TICK_HUD_NATIVE_OWNER_FALLBACK(
                        nNDSTickHudNativeOwnerFallbackSelected);
                }
                else if (plan_result == nNDSFighterDrawPlanMaterialCount)
                {
                    NDS_TICK_HUD_NATIVE_OWNER_FALLBACK(
                        nNDSTickHudNativeOwnerFallbackMaterialCount);
                }
                else
                {
                    NDS_TICK_HUD_NATIVE_OWNER_FALLBACK(
                        nNDSTickHudNativeOwnerFallbackDisplayList);
                }
#endif
            }
            if ((native_owner_enabled != FALSE) &&
                ((native_owner_file == NULL) ||
                 (ndsRendererAdapterValidateNativeOwnerCached(
                       owner_slot, use_low_detail, native_owner_file,
                       collection.selected_count,
                       native_owner_root_offsets,
                       native_owner_material_counts) == FALSE)))
            {
                native_owner_enabled = FALSE;
#if NDS_TICK_HUD
                NDS_TICK_HUD_NATIVE_OWNER_FALLBACK(
                    nNDSTickHudNativeOwnerFallbackValidate);
#endif
            }
            else if ((native_owner_enabled != FALSE) &&
                     (gNdsFtrPlanRoute != 0u) &&
                     (slot < GMCOMMON_PLAYERS_MAX))
            {
                /* Bake. The derivation and the validator have both just
                 * succeeded, so key the result on the same loaded-file
                 * identity the validator caches. */
                NDSFighterDrawPlan *plan = &sNdsFighterDrawPlan[slot];

                ndsFighterDrawPlanGather(
                    &collection, native_owner_file,
                    &sNdsRendererAdapterNativeOwnerWorkspace, &plan->data);
                plan->key_data = native_owner_file->data;
                plan->key_asset_id = native_owner_file->asset_id;
                plan->key_owner_generation =
                    native_owner_file->owner_generation;
                plan->key_data_size = native_owner_file->data_size;
                plan->key_status_generation =
                    sNdsFighterStatusGeneration[slot];
                plan->key_use_low_detail = use_low_detail;
                plan->valid = 1u;
#if NDS_TICK_HUD
                gNdsFtrPlanBuild++;
#endif
            }
        }
#else
        if ((collection.selected_count == 0u) ||
            (collection.selected_count >
             NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED))
        {
            native_owner_enabled = FALSE;
#if NDS_TICK_HUD
            NDS_TICK_HUD_NATIVE_OWNER_FALLBACK(
                nNDSTickHudNativeOwnerFallbackSelected);
#endif
        }
        for (i = 0u;
             (native_owner_enabled != FALSE) &&
             (i < collection.selected_count);
             i++)
        {
            const NDSFighterDisplayContractEvent *event =
                (sNdsFighterDisplayContractPlayback != FALSE) ?
                    &sNdsFighterDisplayReplayEvents[
                        collection.indices[i]] : NULL;
            const Gfx *native_dl =
                (event != NULL) ? event->dl : collection.dobjs[i]->dl;
            DObj *material_dobj =
                (event != NULL) ? event->material_dobj :
                                  collection.dobjs[i];
            NDSRelocLoadedFile *loaded =
                ndsRelocFindLoadedFileContaining(
                    native_dl, sizeof(*native_dl));
            MObj *mobj;
            u32 material_count = 0u;

            if ((native_dl == NULL) || (loaded == NULL) ||
                (loaded->data == NULL) ||
                (loaded->asset_id != expected_asset_id) ||
                ((native_owner_file != NULL) &&
                 (loaded != native_owner_file)) ||
                (loaded->data_size < sizeof(*native_dl)) ||
                ((uintptr_t)native_dl < (uintptr_t)loaded->data) ||
                (((uintptr_t)native_dl - (uintptr_t)loaded->data) >
                 (loaded->data_size - sizeof(*native_dl))))
            {
                native_owner_enabled = FALSE;
#if NDS_TICK_HUD
                NDS_TICK_HUD_NATIVE_OWNER_FALLBACK(
                    nNDSTickHudNativeOwnerFallbackDisplayList);
#endif
                break;
            }
            native_owner_file = loaded;
            native_owner_loaded[i] = loaded;
            native_owner_root_offsets[i] =
                (u32)((uintptr_t)native_dl - (uintptr_t)loaded->data);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            native_owner_matrix_bindings[i] =
                (event != NULL) ? event->matrix_dobj :
                                  collection.dobjs[i];
            native_owner_material_dobjs[i] = material_dobj;
#endif
            for (mobj = (material_dobj != NULL) ? material_dobj->mobj :
                                                  NULL;
                 mobj != NULL;
                 mobj = mobj->next)
            {
                material_count++;
                if (material_count >
                    NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX)
                {
                    native_owner_enabled = FALSE;
#if NDS_TICK_HUD
                    NDS_TICK_HUD_NATIVE_OWNER_FALLBACK(
                        nNDSTickHudNativeOwnerFallbackMaterialCount);
#endif
                    break;
                }
            }
            native_owner_material_counts[i] = material_count;
        }
        if ((native_owner_enabled != FALSE) &&
            ((native_owner_file == NULL) ||
            (ndsRendererValidateNativeFighterOwner(
                 owner_slot, use_low_detail, native_owner_file->data_size,
                 collection.selected_count,
                 native_owner_root_offsets,
                 native_owner_material_counts) == FALSE)))
        {
            native_owner_enabled = FALSE;
#if NDS_TICK_HUD
            NDS_TICK_HUD_NATIVE_OWNER_FALLBACK(
                nNDSTickHudNativeOwnerFallbackValidate);
#endif
        }
#endif
#if NDS_TASK91_DRAW_PHASE_CENSUS
        task91_mark = cpuGetTiming();
        gNdsTask91ValidateTicks += task91_mark - task91_phase_start;
        task91_phase_start = task91_mark;   /* OwnerPrep opens here */
        if (native_owner_enabled != FALSE) { gNdsTask91NativeEligible++; }
#endif
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        m2_owner->m2_owner_validation_ticks +=
            cpuGetTiming() - m2_phase_start;
#endif
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (native_owner_enabled != FALSE)
        {
#if NDS_RENDERER_PROFILE_LEVEL >= 1
            u32 owner_matrix_start = cpuGetTiming();
#endif
#if NDS_TASK91_DRAW_PHASE_CENSUS
            /* R2-03 E4. The matrix/material split already existed at profile
             * level 1, and the tick-HUD target overrides that to 0 -- the same
             * reason Task 91 exists at all. Mirror it here so the 113,199-tick
             * owner-preparation span E3 found can be split in the Boundary
             * configuration. */
            task91_mark = cpuGetTiming();
#endif
            if (((native_owner_hierarchy_mode != FALSE) &&
                 (ndsRendererAdapterPrepareNativeOwnerHierarchy(
                    owner_slot, fp, root, native_owner_matrix_bindings,
                    collection.selected_count,
                    (gGCCurrentCamera != NULL) ?
                        CObjGetStruct(gGCCurrentCamera) : NULL,
                    &sNdsRendererAdapterNativeOwnerWorkspace
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
                    , m2_owner
#endif
                    ) == FALSE)) ||
                ((native_owner_hierarchy_mode == FALSE) &&
                 (ndsRendererAdapterPrepareNativeOwnerMatrices(
                    owner_slot, root, native_owner_matrix_bindings,
                    collection.selected_count,
                    (gGCCurrentCamera != NULL) ?
                        CObjGetStruct(gGCCurrentCamera) : NULL,
                    &native_owner_projection,
                    native_owner_modelviews
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
                    , m2_owner
#endif
                    ) == FALSE)))
            {
                native_owner_enabled = FALSE;
#if NDS_TICK_HUD
                NDS_TICK_HUD_NATIVE_OWNER_FALLBACK(
                    nNDSTickHudNativeOwnerFallbackMatrices);
#endif
            }
#if NDS_TASK91_DRAW_PHASE_CENSUS
            gNdsTask91MatrixPrepTicks += cpuGetTiming() - task91_mark;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 1
            {
                u32 owner_matrix_ticks =
                    cpuGetTiming() - owner_matrix_start;

                gNdsRendererProfileMatrixTicks += owner_matrix_ticks;
            }
#endif
        }
#endif
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (native_owner_enabled != FALSE)
        {
#if NDS_RENDERER_PROFILE_LEVEL >= 1
            u32 owner_material_start = cpuGetTiming();
#endif
#if NDS_TASK91_DRAW_PHASE_CENSUS
            task91_mark = cpuGetTiming();
#endif
            native_owner_material_saved_root_count = 0u;
            /* One claim set per fighter submission. Persistent row ownership is
             * retained for cache hits, but no stale fighter may make two roots
             * of this fighter alias the same mutable material row. */
            sNdsRendererAdapterMaterialRowClaimMask = 0u;
            /* The packet key's material half comes from the live chains alone,
             * before any row or snapshot work, so the record path and the
             * replay pre-check below see the same word. The colour modulate is
             * a replay-time tint, not a key input. */
            sNdsFighterPacketMaterialIdentity =
                ndsRendererAdapterMaterialIdentity(
                    native_owner_material_dobjs, collection.selected_count);
            /* The domains are disjoint: owner[7:0], detail[8], player[10:9],
             * appearance[26:11]. This is source costume + shade identity, not a
             * render approximation. */
            native_owner_texture_key =
                (owner_slot & 0xffu) |
                ((use_low_detail & 1u) << 8) |
                ((slot & 3u) << 9) |
                ((((u32)fp->costume) |
                  ((u32)fp->shade << 8)) & 0xffffu) << 11;
#if NDS_R2_FIGHTER_PACKET
            /* P2-2p4. Ask before preparing: on a replay the production owner
             * reads none of the material rows, snapshots or validation, which
             * were 48K ticks a frame across four fighters. The inputs built
             * here are the ones the execute consumes, so a predicted hit is
             * exact; a predicted miss simply falls through to the preparation
             * below and rebuilds the inputs once the rows exist. */
            if ((native_owner_hierarchy_mode == FALSE) &&
                (detailed_output == FALSE) && (no_oracle != FALSE) &&
                (ndsRendererAdapterBuildNativeProductionInputs(
                    owner_slot, color_modulate, native_owner_file, &collection,
                    native_owner_projection, native_owner_modelviews,
                    &persistent_state,
                    &sNdsRendererAdapterNativeOwnerWorkspace
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
                    , m2_owner
#endif
                    ) != FALSE) &&
                (ndsRendererFighterPacketPrecheck(
                    owner_slot, use_low_detail, native_owner_texture_key,
                    sNdsFighterPacketMaterialIdentity,
                    sNdsRendererAdapterNativeOwnerWorkspace.production_roots,
                    collection.selected_count) != FALSE))
            {
                native_owner_packet_predicted = TRUE;
            }
            if (native_owner_packet_predicted == FALSE)
#endif
            for (i = 0u; i < collection.selected_count; i++)
            {
                u32 prepared_material_count = 0u;
                u32 material_row = ndsRendererAdapterMaterialRow(
                    native_owner_material_dobjs[i], i);

                sNdsRendererAdapterNativeOwnerMaterialRows[i] =
                    (u8)material_row;
                /* The snapshot now rides the prepare walk, which fills the
                 * arrays in chain order exactly as the separate save pass did
                 * and reports how many it filled even when it fails, so a
                 * partial walk still rolls back exactly what it mutated. */
                sb32 prepared = ndsRendererAdapterPrepareNativeMaterials(
                    native_owner_material_dobjs[i],
                    sNdsRendererAdapterNativeOwnerMaterials[material_row],
                    NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX,
                    &prepared_material_count,
                    sNdsRendererAdapterNativeOwnerMaterialKeys[material_row],
                    sNdsRendererAdapterNativeOwnerTextureCurr[i],
                    sNdsRendererAdapterNativeOwnerTextureNext[i]);

                sNdsRendererAdapterNativeOwnerTextureCounts[i] =
                    prepared_material_count;
                native_owner_material_saved_root_count = i + 1u;
                if ((prepared == FALSE) ||
                    (prepared_material_count !=
                     native_owner_material_counts[i]) ||
                    (ndsRendererAdapterValidateNativeOwnerMaterials(
                         sNdsRendererAdapterNativeOwnerMaterials[material_row],
                         prepared_material_count) == FALSE))
                {
                    native_owner_enabled = FALSE;
#if NDS_TICK_HUD
                    NDS_TICK_HUD_NATIVE_OWNER_FALLBACK(
                        nNDSTickHudNativeOwnerFallbackMaterialPrep);
#endif
                    break;
                }
            }
            if (native_owner_enabled == FALSE)
            {
                ndsRendererAdapterRestoreNativeOwnerMaterialTextureIds(
                    native_owner_material_dobjs,
                    native_owner_material_saved_root_count);
                native_owner_material_saved_root_count = 0u;
            }
#if NDS_TASK91_DRAW_PHASE_CENSUS
            gNdsTask91MaterialPrepTicks += cpuGetTiming() - task91_mark;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 1
            {
                u32 owner_material_ticks =
                    cpuGetTiming() - owner_material_start;

                gNdsRendererProfileMaterialTicks += owner_material_ticks;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
                m2_owner->m2_material_ticks += owner_material_ticks;
#endif
            }
#endif
        }
#endif
    }
#endif
#if NDS_TASK91_DRAW_PHASE_CENSUS
    /* OwnerPrep closes: everything between the revalidation and the point the
     * owner inputs are built and submitted -- the matrix and material
     * preparation. What follows is the submit itself, and the census already
     * charges most of that to its own symbols. */
    gNdsTask91OwnerPrepTicks += cpuGetTiming() - task91_phase_start;
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    if ((native_owner_enabled != FALSE) &&
        ((native_owner_production_mode != FALSE) ||
         (native_owner_hierarchy_mode != FALSE)) &&
        (detailed_output == FALSE) && (no_oracle != FALSE))
    {
#if NDS_TASK91_DRAW_PHASE_CENSUS
        task91_mark = cpuGetTiming();
#endif
        if (((native_owner_hierarchy_mode != FALSE) &&
             (ndsRendererAdapterBuildNativeHierarchyInputs(
                owner_slot, color_modulate, native_owner_file, &collection,
                &persistent_state,
                &sNdsRendererAdapterNativeOwnerWorkspace) == FALSE)) ||
            ((native_owner_hierarchy_mode == FALSE) &&
             (native_owner_packet_predicted == FALSE) &&
             (ndsRendererAdapterBuildNativeProductionInputs(
                owner_slot, color_modulate, native_owner_file, &collection,
                native_owner_projection, native_owner_modelviews,
                &persistent_state,
                &sNdsRendererAdapterNativeOwnerWorkspace
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
                , m2_owner
#endif
                ) == FALSE)))
        {
            ndsRendererAdapterRestoreNativeOwnerMaterialTextureIds(
                native_owner_material_dobjs,
                native_owner_material_saved_root_count);
            native_owner_material_saved_root_count = 0u;
            native_owner_enabled = FALSE;
#if NDS_TICK_HUD
            NDS_TICK_HUD_NATIVE_OWNER_FALLBACK(
                nNDSTickHudNativeOwnerFallbackInputs);
#endif
        }
#if NDS_TASK91_DRAW_PHASE_CENSUS
        gNdsTask91InputsTicks += cpuGetTiming() - task91_mark;
#endif
        if (native_owner_enabled != FALSE)
        {
            s32 production_result;
            u32 production_hardware_started = FALSE;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
            u32 owner_dl_start = cpuGetTiming();
#endif
#if NDS_TASK91_DRAW_PHASE_CENSUS
            task91_mark = cpuGetTiming();
            gNdsTask91GxFifoEntriesStart +=
                (NDS_TASK91_GXSTAT >> 16) & 0x1FFu;
#endif
            ndsFighterDLDrawResetRuntimeRendererStats(&persistent_stats);
            native_owner_production_attempted = TRUE;
            production_result = (native_owner_hierarchy_mode != FALSE) ?
                ndsRendererExecuteNativeFighterOwnerHierarchy(
                    owner_slot, native_owner_file->data,
                    &sNdsRendererAdapterNativeOwnerWorkspace.hierarchy,
                    NULL, NULL, &persistent_stats,
                    &production_hardware_started) :
                ndsRendererExecuteNativeFighterOwnerProduction(
                    owner_slot, use_low_detail,
                    /* Packed once in the material phase above, in main RAM so
                     * the 4-CPU build does not spend another 32 B of its
                     * already-full ITCM on once-per-owner bookkeeping. */
                    native_owner_texture_key,
                    sNdsFighterPacketMaterialIdentity,
                    native_owner_file->data,
                    sNdsRendererAdapterNativeOwnerWorkspace.production_roots,
                    collection.selected_count,
                    NULL, NULL, &persistent_stats,
                    &production_hardware_started);
            if (production_result != FALSE)
            {
                runtime_hardware_triangle_count =
                    persistent_stats.hardware_triangle_count;
                /* Native material preparation advances every live MObj once,
                 * matching the generic path. A successful owner consumes that
                 * advancement and must not restore it. */
                native_owner_material_saved_root_count = 0u;
#if NDS_R2_FOX_GUN_OVERLAY
                /* BUGS.md "Fox's pistol model is missing". HERE, and only on a
                 * successful production run, because the overlay depends on the
                 * camera and projection that run just established. On the
                 * fallback path the generic renderer owns its own matrices and
                 * would have to be joined differently; that arm has never been
                 * the shipped one, so it stays without a gun rather than with a
                 * gun in the wrong place. */
                {
                    NDSRendererMatrix20p12 gun_world;

                    if (ndsRendererAdapterBuildFoxGunJointMtx(
                            fp,
                            (gGCCurrentCamera != NULL) ?
                                CObjGetStruct(gGCCurrentCamera) : NULL,
                            &gun_world) != FALSE)
                    {
                        (void)ndsRendererSubmitFoxGun(&gun_world);
                    }
                }
#endif
            }
            else if (production_hardware_started == FALSE)
            {
                /* The production owner rejected its complete input contract
                 * before touching GX. Restore the FRAC fields serialized by
                 * native material preparation and let the ordinary path
                 * advance and draw them exactly once. */
                ndsRendererAdapterRestoreNativeOwnerMaterialTextureIds(
                    native_owner_material_dobjs,
                    native_owner_material_saved_root_count);
                native_owner_material_saved_root_count = 0u;
                native_owner_enabled = FALSE;
                native_owner_production_attempted = FALSE;
#if NDS_TICK_HUD
                NDS_TICK_HUD_NATIVE_OWNER_FALLBACK(
                    nNDSTickHudNativeOwnerFallbackContract);
#endif
            }
            else
            {
#if NDS_TICK_HUD
                NDS_TICK_HUD_NATIVE_OWNER_FALLBACK(
                    nNDSTickHudNativeOwnerFallbackPostGx);
#endif
                native_owner_failed = TRUE;
                if (persistent_stats.blocker == NDS_RENDERER_BLOCKER_NONE)
                {
                    persistent_stats.blocker =
                        NDS_RENDERER_BLOCKER_UNSUPPORTED;
                }
            }
#if NDS_TASK91_DRAW_PHASE_CENSUS
            {
                u32 task91_gxstat = NDS_TASK91_GXSTAT;
                u32 task91_entries = (task91_gxstat >> 16) & 0x1FFu;

                gNdsTask91ExecuteTicks += cpuGetTiming() - task91_mark;
                gNdsTask91GxFifoSamples++;
                gNdsTask91GxStatOr |= task91_gxstat;
                gNdsTask91GxFifoEntriesEnd += task91_entries;
                if (task91_entries > gNdsTask91GxFifoMaxEnd)
                {
                    gNdsTask91GxFifoMaxEnd = task91_entries;
                }
                if ((task91_gxstat & (1u << 27)) != 0u)
                {
                    gNdsTask91GxBusyEnd++;
                }
            }
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 1
            gNdsRendererProfileDLTicks += cpuGetTiming() - owner_dl_start;
#endif
        }
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
    gNdsRendererProfileOwners[(u32)owner_id].entry_state_hash =
        ndsRendererOwnerHashRuntimeState(&persistent_stats);
    gNdsRendererProfileOwners[(u32)owner_id].entry_vertex_cache_hash =
        ndsRendererOwnerHashVertexCache(&persistent_renderer_vertices);
    gNdsRendererProfileOwners[(u32)owner_id].entry_resolver_hash =
        ndsRendererOwnerHashResolver(&persistent_state);
    gNdsRendererProfileOwners[(u32)owner_id].entry_global_hash =
        ndsRendererProfileGlobalStateHash();
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    if ((native_owner_enabled != FALSE) &&
        (native_owner_production_attempted == FALSE))
    {
        if (ndsRendererBeginNativeFighterOwner(
                owner_slot, &persistent_stats,
                &persistent_renderer_vertices) != FALSE)
        {
            native_owner_started = TRUE;
        }
        else
        {
            ndsRendererAdapterRestoreNativeOwnerMaterialTextureIds(
                native_owner_material_dobjs,
                native_owner_material_saved_root_count);
            native_owner_material_saved_root_count = 0u;
            native_owner_enabled = FALSE;
#if NDS_TICK_HUD
            NDS_TICK_HUD_NATIVE_OWNER_FALLBACK(
                nNDSTickHudNativeOwnerFallbackBegin);
#endif
        }
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    if (native_owner_production_attempted == FALSE)
#endif
    {
    for (i = 0u; i < collection.selected_count; i++)
    {
        const NDSFighterDisplayContractEvent *contract_event =
            (sNdsFighterDisplayContractPlayback != FALSE) ?
                &sNdsFighterDisplayReplayEvents[collection.indices[i]] :
                NULL;
        const Gfx *dl = (sNdsFighterDisplayContractPlayback != FALSE) ?
            contract_event->dl :
            collection.dobjs[i]->dl;
        NDSRelocLoadedFile *loaded;
        NDSFighterDLDrawState *current_state;
        NDSRendererConfig config = {0};
        NDSRendererStats *current_stats;
        NDSRendererMatrix20p12 initial_projection;
        NDSRendererMatrix20p12 initial_modelview;
        const NDSRendererMatrix20p12 *initial_projection_ptr;
        const NDSRendererMatrix20p12 *initial_modelview_ptr;
#if NDS_RENDERER_HW_TRIANGLES
        void *saved_graphics_heap_ptr;
        DObj *material_dobj;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        s32 native_saved_texture_curr[
            NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX];
        s32 native_saved_texture_next[
            NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX];
        u32 native_saved_texture_count = 0u;
        u32 native_prepared_material_count = 0u;
        sb32 native_material_built = FALSE;
#endif
        u32 native_root_offset = 0u;
        sb32 native_root_enabled = FALSE;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        u32 step_start;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        NDSRendererOwnerStatsSnapshot owner_stats_before;
#endif
#endif

#if NDS_RENDERER_HW_TRIANGLES
        loaded = (native_owner_enabled != FALSE) ?
            native_owner_loaded[i] :
            ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
#else
        loaded = ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
#endif

        if ((loaded == NULL) &&
            (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) == FALSE))
        {
            continue;
        }

#if NDS_RENDERER_HW_TRIANGLES
        if ((native_owner_enabled != FALSE) && (loaded != NULL) &&
            (loaded->data != NULL) &&
            (loaded->asset_id == expected_asset_id) &&
            (loaded->data_size >= sizeof(*dl)) &&
            ((uintptr_t)dl >= (uintptr_t)loaded->data) &&
            (((uintptr_t)dl - (uintptr_t)loaded->data) <=
             (loaded->data_size - sizeof(*dl))))
        {
            native_root_offset =
                (u32)((uintptr_t)dl - (uintptr_t)loaded->data);
            native_root_enabled =
                (native_root_offset == native_owner_root_offsets[i]) ?
                    TRUE : FALSE;
        }
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (native_root_enabled != FALSE)
        {
            native_materials =
                sNdsRendererAdapterNativeOwnerMaterials[
                    sNdsRendererAdapterNativeOwnerMaterialRows[i]];
            native_material_count = native_owner_material_counts[i];
        }
#endif
#endif

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
        current_state = (detailed_output != FALSE) ? &states[i] :
                                                    &persistent_state;
#else
        current_state = &states[i];
#endif
        current_state->primary_file = loaded;
        current_state->slot = owner_slot;
        if (contract_event != NULL)
        {
            const NDSRendererNativeFighterPreamble *contract_preamble =
                &sNdsFighterDisplayReplayPreambles[collection.indices[i]];

            persistent_stats.geometry_mode =
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
                (native_root_enabled != FALSE) ?
                    ndsRendererAdapterNormalizeNativeGeometryMode(
                        contract_preamble->geometry_mode) :
#endif
                    contract_preamble->geometry_mode;
            persistent_stats.othermode_h =
                (persistent_stats.othermode_h &
                 ~NDS_FIGHTER_DISPLAY_CYCLETYPE_MASK) |
                contract_preamble->cycle_type;
            persistent_stats.othermode_l = contract_preamble->render_mode;
            persistent_stats.prim_color = contract_preamble->prim_color;
            persistent_stats.env_color = contract_preamble->env_color;
            if ((contract_preamble->flags &
                 NDS_RENDERER_NATIVE_PREAMBLE_LIGHT_VALID) != 0u)
            {
                persistent_stats.light_dir_x = contract_preamble->light_dir_x;
                persistent_stats.light_dir_y = contract_preamble->light_dir_y;
                persistent_stats.light_dir_z = contract_preamble->light_dir_z;
                persistent_stats.light_dir_mask = 1u;
            }
        }
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
        if (detailed_output != FALSE)
        {
            ndsFighterDLDrawSeedPersistentState(current_state,
                                                &persistent_state);
        }
#else
        ndsFighterDLDrawSeedPersistentState(current_state,
                                            &persistent_state);
#endif
#if NDS_RENDERER_HW_TRIANGLES
        saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        step_start = cpuGetTiming();
#endif
        material_dobj =
            (sNdsFighterDisplayContractPlayback != FALSE) ?
                contract_event->material_dobj : collection.dobjs[i];
        if ((sNdsFighterDisplayContractPlayback == FALSE) ||
            (contract_event->material_dobj != NULL))
        {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            if ((native_root_enabled != FALSE) &&
                (material_dobj != NULL) &&
                (material_dobj->mobj != NULL))
            {
                /* The snapshot rides the prepare walk: it fills these two in
                 * chain order exactly as the separate save pass did, and
                 * reports how many entries it touched even when it fails. */
                sb32 native_prepared =
                    ndsRendererAdapterPrepareNativeMaterials(
                        material_dobj, native_materials,
                        NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX,
                        &native_prepared_material_count,
                        /* Caller local, gone by next frame -- always build. */
                        NULL,
                        native_saved_texture_curr,
                        native_saved_texture_next);

                native_saved_texture_count = native_prepared_material_count;
                if (native_prepared != FALSE)
                {
                    native_material_count =
                        native_prepared_material_count;
                    native_material_built = TRUE;
                }
                else
                {
                    ndsRendererAdapterRestoreNativeMaterialTextureIds(
                        material_dobj,
                        native_saved_texture_curr,
                        native_saved_texture_next,
                        native_saved_texture_count);
                    native_root_enabled = FALSE;
                }
            }
            if (native_material_built != FALSE)
            {
                /* Forensic builds retain the exact resolver-boundary oracle.
                 * Restore the two FRAC fields before its ordinary material
                 * builder so the live MObj advances exactly once. */
                ndsRendererAdapterRestoreNativeMaterialTextureIds(
                    material_dobj,
                    native_saved_texture_curr,
                    native_saved_texture_next,
                    native_saved_texture_count);
            }
            ndsRendererAdapterPrepareMaterialSegment(
                material_dobj, current_state);
#else
            /* The owner preflight already advanced every live MObj once and
             * serialized this root's descriptors. Only the ordinary fallback
             * prepares the material segment in the selected-list loop. */
            if (native_root_enabled == FALSE)
            {
                ndsRendererAdapterPrepareMaterialSegment(
                    material_dobj, current_state);
            }
#endif
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        gNdsRendererProfileMaterialTicks += cpuGetTiming() - step_start;
        step_start = cpuGetTiming();
#endif
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
        if (native_root_enabled != FALSE)
        {
            initial_projection_ptr = native_owner_projection;
            initial_modelview_ptr = native_owner_modelviews[i];
        }
        else
#endif
        {
            ndsRendererAdapterPrepareInitialMatrices(
                (sNdsFighterDisplayContractPlayback != FALSE) ?
                    contract_event->matrix_dobj : collection.dobjs[i],
                (gGCCurrentCamera != NULL) ?
                    CObjGetStruct(gGCCurrentCamera) : NULL,
                FALSE,
                &initial_projection,
                &initial_projection_ptr,
                &initial_modelview,
                &initial_modelview_ptr);
        }
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 1)
        gNdsRendererProfileMatrixTicks += cpuGetTiming() - step_start;
#endif
        config.max_depth = 8u;
        config.max_commands = 2048u;
        config.max_list_commands = 512u;
        config.initial_projection = initial_projection_ptr;
        config.initial_modelview = initial_modelview_ptr;
        config.initial_geometry_mode =
            (sNdsFighterDisplayContractPlayback != FALSE) ?
                sNdsFighterDisplayReplayPreambles[
                    collection.indices[i]].geometry_mode : 0u;
        config.color_modulate = color_modulate;
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
        if (native_root_enabled != FALSE)
        {
            config.initial_geometry_mode =
                ndsRendererAdapterNormalizeNativeGeometryMode(
                    config.initial_geometry_mode);
        }
#endif
        config.texture_data_layout =
            NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
        config.validate_range = ndsFighterDLAllDrawValidateRange;
        config.immutable_command_span =
            ndsRendererAdapterImmutableCommandSpan;
        config.resolve_branch = ndsFighterDLAllDrawResolveBranch;
        config.resolve_data = ndsFighterDLDrawResolveRendererData;
        config.user = current_state;

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
        if (detailed_output != FALSE)
        {
            ndsFighterDLDrawResetTransientRendererStats(&persistent_stats);
        }
        else
        {
            ndsFighterDLDrawResetRuntimeRendererStats(&persistent_stats);
        }
        current_stats = &persistent_stats;
#else
        ndsRendererInitStats(&stats[i]);
        ndsFighterDLDrawCopyPersistentRendererState(&stats[i],
                                                    &persistent_stats);
        current_stats = &stats[i];
#endif
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        ndsRendererOwnerSnapshotStats(current_stats, &owner_stats_before);
        step_start = cpuGetTiming();
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        ndsRendererProfileSetSourceProvenance(
            0u, i,
            ndsRendererOwnerRootBranchPath(
                loaded, dl, collection.indices[i]));
#endif
#endif
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (native_owner_failed != FALSE)
        {
            current_stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
        }
        else if (native_owner_started != FALSE)
        {
            if ((native_root_enabled == FALSE) ||
                (ndsRendererExecuteNativeFighterRoot(
                     owner_slot, i, loaded->data, native_root_offset,
                     native_materials, native_material_count,
                     &config,
                     (no_oracle != FALSE) ?
                         NULL : ndsFighterMarioFoxVisitDLDrawCommand,
                     current_state,
                     current_stats,
                     &persistent_renderer_vertices) == FALSE))
            {
                /* The compact owner may have submitted earlier runs or
                 * updated its dense slot map. Never replay this root through
                 * the generic cache on top of that partial state. */
                if (current_stats->blocker == NDS_RENDERER_BLOCKER_NONE)
                {
                    current_stats->blocker =
                        NDS_RENDERER_BLOCKER_UNSUPPORTED;
                }
                ndsRendererAbortNativeFighterOwner();
                native_owner_started = FALSE;
                native_owner_failed = TRUE;
            }
        }
        else
        {
            ndsRendererExecuteDisplayListWithVertexCache(
                dl, &config,
                (no_oracle != FALSE) ?
                    NULL : ndsFighterMarioFoxVisitDLDrawCommand,
                current_state, current_stats,
                &persistent_renderer_vertices);
        }
#else
        if ((native_root_enabled == FALSE) ||
            (ndsRendererExecuteNativeFighterRoot(
                 owner_slot, i, loaded->data, native_root_offset,
                 native_materials, native_material_count,
                 &config,
                 (no_oracle != FALSE) ?
                     NULL : ndsFighterMarioFoxVisitDLDrawCommand,
                 current_state,
                 current_stats,
                 &persistent_renderer_vertices) == FALSE))
        {
            ndsRendererExecuteDisplayListWithVertexCache(
                dl, &config,
                (no_oracle != FALSE) ?
                    NULL : ndsFighterMarioFoxVisitDLDrawCommand,
                current_state, current_stats,
                &persistent_renderer_vertices);
        }
#endif
#else
        ndsRendererExecuteDisplayListWithVertexCache(
            dl, &config,
            (no_oracle != FALSE) ?
                NULL : ndsFighterMarioFoxVisitDLDrawCommand,
            current_state, current_stats,
            &persistent_renderer_vertices);
#endif
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileDLTicks += cpuGetTiming() - step_start;
        ndsRendererOwnerAccumulateList(
            owner_id, loaded, dl, collection.indices[i],
            initial_projection_ptr, initial_modelview_ptr,
            &config,
            &owner_stats_before, current_stats);
#endif
        gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
        if (detailed_output != FALSE)
        {
            ndsFighterDLDrawCapturePersistentState(&persistent_state,
                                                   current_state);
        }
#else
        ndsFighterDLDrawCapturePersistentState(&persistent_state,
                                               current_state);
#endif
#if !NDS_RENDERER_HW_TRIANGLES || (NDS_RENDERER_PROFILE_LEVEL >= 2)
        ndsFighterDLDrawCopyPersistentRendererState(&persistent_stats,
                                                    current_stats);
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
        if (detailed_output != FALSE)
        {
            ndsFighterDLAllDrawAccumulateStats(
                slot, i, collection.indices[i], collection.dobjs[i], dl,
                current_state, current_stats, clean);
        }
        else
        {
            runtime_hardware_triangle_count +=
                current_stats->hardware_triangle_count;
        }
#else
        ndsFighterDLAllDrawAccumulateStats(slot, i, collection.indices[i],
                                           collection.dobjs[i], dl,
                                           current_state, current_stats, clean);
#endif
    }
    }

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    if (native_owner_started != FALSE)
    {
        if (ndsRendererEndNativeFighterOwner(
                owner_slot, &persistent_stats,
                &persistent_renderer_vertices) == FALSE)
        {
            persistent_stats.blocker =
                NDS_RENDERER_BLOCKER_UNSUPPORTED;
            native_owner_failed = TRUE;
            ndsRendererAbortNativeFighterOwner();
        }
        native_owner_started = FALSE;
    }
    if (native_owner_failed != FALSE)
    {
        u32 failure_index = (collection.selected_count != 0u) ?
            collection.selected_count - 1u : 0u;

        /* Make an unreachable integrity failure visible to both the detailed
         * proof ledger and the null-callback performance verifier. */
        runtime_hardware_triangle_count = 0u;
        if (slot == 0u)
        {
            if (gNdsFighterDLAllDrawP0FirstBlocker == 0u)
            {
                gNdsFighterDLAllDrawP0FirstBlocker =
                    NDS_RENDERER_BLOCKER_UNSUPPORTED;
            }
            gNdsFighterDLAllDrawP0BlockerMask |=
                1u << (failure_index & 31u);
        }
        else if (slot == 1u)
        {
            if (gNdsFighterDLAllDrawP1FirstBlocker == 0u)
            {
                gNdsFighterDLAllDrawP1FirstBlocker =
                    NDS_RENDERER_BLOCKER_UNSUPPORTED;
            }
            gNdsFighterDLAllDrawP1BlockerMask |=
                1u << (failure_index & 31u);
        }
    }
#endif

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
    gNdsRendererProfileOwners[(u32)owner_id].exit_state_hash =
        ndsRendererOwnerHashRuntimeState(&persistent_stats);
    gNdsRendererProfileOwners[(u32)owner_id].exit_vertex_cache_hash =
        ndsRendererOwnerHashVertexCache(&persistent_renderer_vertices);
    gNdsRendererProfileOwners[(u32)owner_id].exit_resolver_hash =
        ndsRendererOwnerHashResolver(&persistent_state);
    gNdsRendererProfileOwners[(u32)owner_id].exit_global_hash =
        ndsRendererProfileGlobalStateHash();
#endif
#if NDS_RENDERER_HW_TRIANGLES
    ndsRendererProfileSetOwner(NDS_RENDERER_PROFILE_OWNER_NONE);
#endif

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    if (detailed_output == FALSE)
    {
        if (slot == 0u)
        {
            gNdsFighterDLAllDrawP0HardwareTriangleCount +=
                runtime_hardware_triangle_count;
        }
        else if (slot == 1u)
        {
            gNdsFighterDLAllDrawP1HardwareTriangleCount +=
                runtime_hardware_triangle_count;
        }
    }
#endif
    if (pixels != NULL)
    {
        ndsFighterDLAllDrawRasterizeStates(slot, states, clean,
                                           collection.selected_count,
                                           pixels,
                                           pitch);
    }

    root_x_after = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) :
        0u;
    if (slot == 0u)
    {
        gNdsFighterDLAllDrawP0StatusAfter = (u32)fp->status_id;
        gNdsFighterDLAllDrawP0MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLAllDrawP0GAAfter = (u32)fp->ga;
        gNdsFighterDLAllDrawP0RootXBeforeBits = root_x_before;
        gNdsFighterDLAllDrawP0RootXAfterBits = root_x_after;
    }
    else if (slot == 1u)
    {
        gNdsFighterDLAllDrawP1StatusAfter = (u32)fp->status_id;
        gNdsFighterDLAllDrawP1MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLAllDrawP1GAAfter = (u32)fp->ga;
        gNdsFighterDLAllDrawP1RootXBeforeBits = root_x_before;
        gNdsFighterDLAllDrawP1RootXAfterBits = root_x_after;
    }

#if NDS_TASK91_DRAW_PHASE_CENSUS
    gNdsTask91TotalTicks += cpuGetTiming() - task91_total_start;
#endif
    gNdsFighterMarioFoxDLAllDrawCount++;
}

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
static void ndsRendererAdapterM2FinishOwner(
    volatile NDSRendererOwnerProfile *owner,
    u32 owner_start)
{
    u32 owner_ticks;
    u32 measured_ticks;

    if (owner == NULL)
    {
        return;
    }
    owner_ticks = cpuGetTiming() - owner_start;
    measured_ticks =
        owner->m2_contract_capture_ticks +
        owner->m2_collection_ticks +
        owner->m2_owner_validation_ticks +
        owner->m2_census_ticks +
        owner->m2_camera_fetch_ticks +
        owner->m2_hash_parent_lookup_ticks +
        owner->m2_local_matrix_ticks +
        owner->m2_world_affine_ticks +
        owner->m2_world_camera_ticks +
        owner->m2_final_compose_ticks +
        owner->m2_material_ticks +
        owner->m2_production_total_ticks;
    owner->exclusive_ticks += owner_ticks;
    if (owner_ticks >= measured_ticks)
    {
        owner->m2_owner_residual_ticks += owner_ticks - measured_ticks;
    }
    else
    {
        owner->m2_owner_phase_overlap_count++;
    }
}
#endif

void ndsFighterDisplayContractSubmit(GObj *fighter_gobj)
{
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_IMPORT_BATTLESHIP_VS_RESULTS
    extern volatile u32 gNdsVSResultsFighterSubmitCount;
#endif
    FTStruct *fp;
    u32 owner_slot;
    u32 submitted_before;
    u32 triangles_before;
    u32 triangles_after;
#if NDS_R2_FIGHTER_NO_ORACLE && (NDS_RENDERER_PROFILE_LEVEL < 2)
    u32 saved_no_oracle;
#endif
#if NDS_TICK_HUD || (NDS_RENDERER_PROFILE_LEVEL >= 1)
    u32 owner_start;
    NDSRendererProfileOwner owner_id;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    u32 m2_capture_start;
#endif
#endif

    if (fighter_gobj == NULL)
    {
        return;
    }
#if NDS_IMPORT_BATTLESHIP_VS_RESULTS
    if (gSCManagerSceneData.scene_curr == nSCKindVSResults)
    {
        gNdsVSResultsFighterSubmitCount++;
    }
#endif
    fp = ftGetStruct(fighter_gobj);
    if ((ndsFighterStructIsTrackedPointer(fp) == FALSE) ||
        ((u32)fp->nds_slot >= GMCOMMON_PLAYERS_MAX) ||
        (ndsFighterGetNativeOwnerSlot(fp, &owner_slot) == FALSE))
    {
        return;
    }
    if (sNdsFighterDisplayContractLastFrame[(u32)fp->nds_slot] ==
        gNdsRendererProfileFrameCount)
    {
        return;
    }
    sNdsFighterDisplayContractLastFrame[(u32)fp->nds_slot] =
        gNdsRendererProfileFrameCount;
#if NDS_TICK_HUD || (NDS_RENDERER_PROFILE_LEVEL >= 1)
    owner_id = ndsFighterNativeOwnerProfileId(owner_slot);
    owner_start = cpuGetTiming();
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    m2_capture_start = cpuGetTiming();
#endif
#endif
    ndsFighterDisplayContractCapture(fighter_gobj);
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    gNdsRendererProfileOwners[(u32)owner_id].m2_contract_capture_ticks +=
        cpuGetTiming() - m2_capture_start;
#endif
#if NDS_R2_FTR_CONTRACT_CENSUS
    /* Before the event_count == 0 early-out, so a magnified/off-screen fighter
     * (ftdisplaymain.c:1140-1152 returns before drawing anything) is counted as
     * the zero-event contract it is rather than dropped from the census. */
    ndsFtrContractCensusRecord((u32)fp->nds_slot);
#endif
    if (sNdsFighterDisplayContract.event_count == 0u)
    {
#if NDS_TICK_HUD
        gNdsTickHudFighterTicks += cpuGetTiming() - owner_start;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 1
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
        ndsRendererAdapterM2FinishOwner(
            &gNdsRendererProfileOwners[(u32)owner_id], owner_start);
#else
        gNdsRendererProfileOwners[(u32)owner_id].exclusive_ticks +=
            cpuGetTiming() - owner_start;
#endif
#endif
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
        ndsRendererBenchmarkSinkEndOwner(owner_id);
#endif
        return;
    }
    submitted_before = gNdsFighterMarioFoxDLAllDrawCount;
    triangles_before =
        gNdsFighterDLAllDrawP0HardwareTriangleCount +
        gNdsFighterDLAllDrawP1HardwareTriangleCount;
    sNdsFighterDisplayContractPlayback = TRUE;
    /* R2-07 R4c. Select the native fighter owner here as well. `no_oracle` is
     * not a proof switch: reloc_backend_renderer_dl.c:12603 enters the native
     * owner only when it is set, so this chooses the renderer (R4g). The battle
     * present already sets it around its whole draw
     * (reloc_backend_movement.c:13724/:13810), which is why the match gets the
     * native owner and VS Results -- reaching this function through the scene
     * draw with no bracket on the path -- gets the generic DL interpreter.
     *
     * SAVE AND RESTORE, do not set FALSE. This function is also called from
     * ndsFighterDisplayContractSubmitStageFighters INSIDE the battle bracket,
     * so ending with FALSE would clear the stage's own setting for everything
     * drawn after the first fighter. Restoring makes the bracket a no-op
     * wherever it is already set, which is exactly the battle path. */
#if NDS_R2_FIGHTER_NO_ORACLE && (NDS_RENDERER_PROFILE_LEVEL < 2)
    saved_no_oracle = ndsRendererHardwareNoOracleEnabled();
    ndsRendererHardwareSetNoOracle(TRUE);
#endif
    ndsFighterMarioFoxDLAllDrawForSlot((u32)fp->nds_slot, fp, NULL, 0u);
#if NDS_R2_FIGHTER_NO_ORACLE && (NDS_RENDERER_PROFILE_LEVEL < 2)
    ndsRendererHardwareSetNoOracle(saved_no_oracle);
#endif
    sNdsFighterDisplayContractPlayback = FALSE;
    if (gNdsFighterMarioFoxDLAllDrawCount != submitted_before)
    {
        gNdsFighterDisplayContractSubmittedCount +=
            sNdsFighterDisplayContract.event_count;
        gNdsStageGCDrawAllLoopHardwareFighterSubmitCount++;
        triangles_after =
            gNdsFighterDLAllDrawP0HardwareTriangleCount +
            gNdsFighterDLAllDrawP1HardwareTriangleCount;
        if (triangles_after > triangles_before)
        {
            gNdsStageGCDrawAllLoopHardwareFighterTriangleCount +=
                triangles_after - triangles_before;
        }
    }
#if NDS_TICK_HUD
    gNdsTickHudFighterTicks += cpuGetTiming() - owner_start;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 1
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    ndsRendererAdapterM2FinishOwner(
        &gNdsRendererProfileOwners[(u32)owner_id], owner_start);
#else
    gNdsRendererProfileOwners[(u32)owner_id].exclusive_ticks +=
        cpuGetTiming() - owner_start;
#endif
#endif
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    ndsRendererBenchmarkSinkEndOwner(owner_id);
#endif
#else
    (void)fighter_gobj;
#endif
}

#if NDS_RENDERER_HW_TRIANGLES
static void ndsFighterDisplayContractSubmitStageFighters(void)
{
    GObj *saved_camera = gGCCurrentCamera;
    u32 slot;
#if NDS_RENDERER_M3_PHASE0_PROFILE
    u32 phase05_start = NDS_RENDERER_PHASE05_TICK();
#endif

    gGCCurrentCamera = ndsBattleCompatMainCameraGObj();
    for (slot = 0u; slot < GMCOMMON_PLAYERS_MAX; slot++)
    {
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
        GObj *fighter_gobj = ndsFighterManagerLiveGObj(slot);
#else
        GObj *fighter_gobj = ndsFighterMarioFoxProofGObjForSlot(slot);
#endif

        if (fighter_gobj == NULL)
        {
            continue;
        }
        /* P2-2: submit the source-created LIVE fighter, not the historical
         * sNdsFighterStructPool snapshot. The pool fallback remains only for
         * non-import proof builds; production must never resurrect an empty
         * player slot from old pool state after a rematch / Sudden Death arena
         * rewind. */
        ndsFighterDisplayContractSubmit(fighter_gobj);
    }
    gGCCurrentCamera = saved_camera;
#if NDS_RENDERER_M3_PHASE0_PROFILE
    NDS_RENDERER_PHASE05_FINISH(
        gNdsRendererPhase05FighterWrapperTicks, phase05_start);
#endif
}
#endif

static void ndsFighterMarioFoxRecordDLAllDrawFromDisplayCallback(
    GObj *fighter_gobj)
{
    FTStruct *fp;
    u32 slot;

    if ((fighter_gobj == NULL) || (sNdsFighterDLAllDrawPixels == NULL))
    {
        return;
    }

    fp = ftGetStruct(fighter_gobj);
    if (ndsFighterStructIsTrackedPointer(fp) == FALSE)
    {
        return;
    }

    slot = (u32)fp->nds_slot;
    if (slot > 1u)
    {
        return;
    }

    gNdsFighterDLAllDrawDisplayCallbackCount++;
    if (slot == 0u)
    {
        gNdsFighterDLAllDrawP0DisplayCallbackCount++;
    }
    else
    {
        gNdsFighterDLAllDrawP1DisplayCallbackCount++;
    }

    ndsFighterMarioFoxDLAllDrawForSlot(slot, fp,
                                       sNdsFighterDLAllDrawPixels,
                                       sNdsFighterDLAllDrawPitch);
}

static void ndsFighterMarioFoxRunDLAllDrawProbe(void)
{
    u32 mask = 0u;
    u32 gobj_before;
    u32 gobj_after;
    u32 pitch = 0u;
    u16 *pixels;
    GObj *saved_camera;

    if ((ndsFighterMarioFoxDLAllDrawProofEnabled() == FALSE) ||
        (gNdsFighterMarioFoxDLAllDrawResult != 0u))
    {
        return;
    }

    if (
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
        (gNdsFighterMarioFoxDLMultiDrawCount == 2u) &&
        (gNdsFighterDLMultiDrawP0CandidateCount > 0u) &&
        (gNdsFighterDLMultiDrawP1CandidateCount > 0u)
#else
        (gNdsFighterMarioFoxDLMultiDrawResult ==
            NDS_FIGHTER_MARIOFOX_DL_MULTI_DRAW_PASS) &&
        (gNdsFighterMarioFoxDLMultiDrawSafeResult ==
            NDS_FIGHTER_MARIOFOX_DL_MULTI_DRAW_SAFE_PASS) &&
        ((gNdsFighterMarioFoxDLMultiDrawMask & 0x7ffu) == 0x7ffu) &&
        (gNdsFighterMarioFoxDLMultiDrawCount == 2u) &&
        (gNdsFighterDLMultiDrawP0CandidateCount == 14u) &&
        (gNdsFighterDLMultiDrawP1CandidateCount == 18u) &&
        (gNdsFighterDLMultiDrawP0SelectedCount == 4u) &&
        (gNdsFighterDLMultiDrawP1SelectedCount == 4u) &&
        (gNdsFighterDLMultiDrawP0CleanCount == 4u) &&
        (gNdsFighterDLMultiDrawP1CleanCount == 4u) &&
        (gNdsFighterDLMultiDrawP0FailedCount == 0u) &&
        (gNdsFighterDLMultiDrawP1FailedCount == 0u)
#endif
        )
    {
        mask |= 1u << 0;
    }
    else
    {
        gNdsFighterMarioFoxDLAllDrawMask = mask;
        return;
    }

    gNdsFighterDLAllDrawPreviewCommitBefore =
        gNdsOriginalDLPreviewCommitCount;
    ndsFighterDLAllDrawResetFailureDiagnostics();
    pixels = ndsPlatformBeginOriginalDLPreview(
        NDS_FIGHTER_DL_ALL_DRAW_WIDTH,
        NDS_FIGHTER_DL_ALL_DRAW_HEIGHT,
        &pitch);
    if (pixels != NULL)
    {
        gNdsFighterDLAllDrawPreviewWidth = NDS_FIGHTER_DL_ALL_DRAW_WIDTH;
        gNdsFighterDLAllDrawPreviewHeight = NDS_FIGHTER_DL_ALL_DRAW_HEIGHT;
        gNdsFighterDLAllDrawPreviewPitch = pitch;
        mask |= 1u << 1;
    }
    else
    {
        gNdsFighterMarioFoxDLAllDrawMask = mask;
        return;
    }

    gobj_before = (u32)gcGetGObjsActiveNum();
    sNdsFighterDLAllDrawProbeActive = TRUE;
    sNdsFighterDLAllDrawPixels = pixels;
    sNdsFighterDLAllDrawPitch = pitch;
    saved_camera = gGCCurrentCamera;
    gGCCurrentCamera = ndsBattleCompatMainCameraGObj();
    ftDisplayMainProcDisplay(ndsFighterMarioFoxProofGObjForSlot(0u));
    ftDisplayMainProcDisplay(ndsFighterMarioFoxProofGObjForSlot(1u));
    gGCCurrentCamera = saved_camera;
    sNdsFighterDLAllDrawProbeActive = FALSE;
    sNdsFighterDLAllDrawPixels = NULL;
    sNdsFighterDLAllDrawPitch = 0u;

    if ((gNdsFighterDLAllDrawDisplayCallbackCount == 2u) &&
        (gNdsFighterDLAllDrawP0DisplayCallbackCount == 1u) &&
        (gNdsFighterDLAllDrawP1DisplayCallbackCount == 1u) &&
        (gNdsFighterMarioFoxDLAllDrawCount == 2u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsFighterDLAllDrawP0CandidateCount == 14u) &&
        (gNdsFighterDLAllDrawP1CandidateCount == 18u) &&
        (gNdsFighterDLAllDrawP0SelectedCount == 14u) &&
        (gNdsFighterDLAllDrawP1SelectedCount == 18u) &&
        (gNdsFighterDLAllDrawP0SelectedIndexMask != 0u) &&
        (gNdsFighterDLAllDrawP1SelectedIndexMask != 0u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterDLAllDrawP0AttemptCount == 14u) &&
        (gNdsFighterDLAllDrawP1AttemptCount == 18u) &&
        (gNdsFighterDLAllDrawP0CleanCount == 14u) &&
        (gNdsFighterDLAllDrawP1CleanCount == 18u) &&
        (gNdsFighterDLAllDrawP0DrawnDObjCount ==
            gNdsFighterDLAllDrawP0CleanCount) &&
        (gNdsFighterDLAllDrawP1DrawnDObjCount ==
            gNdsFighterDLAllDrawP1CleanCount) &&
        (gNdsFighterDLAllDrawP0FailedCount == 0u) &&
        (gNdsFighterDLAllDrawP1FailedCount == 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterDLAllDrawP0VertexDecodedCount >
            gNdsFighterDLMultiDrawP0VertexDecodedCount) &&
        (gNdsFighterDLAllDrawP1VertexDecodedCount >
            gNdsFighterDLMultiDrawP1VertexDecodedCount) &&
        (gNdsFighterDLAllDrawP0TriangleCount >
            gNdsFighterDLMultiDrawP0TriangleCount) &&
        (gNdsFighterDLAllDrawP1TriangleCount >
            gNdsFighterDLMultiDrawP1TriangleCount) &&
        (gNdsFighterDLAllDrawP0TriangleValidCount >
            gNdsFighterDLMultiDrawP0TriangleValidCount) &&
        (gNdsFighterDLAllDrawP1TriangleValidCount >
            gNdsFighterDLMultiDrawP1TriangleValidCount) &&
        (gNdsFighterDLAllDrawP0RealTriangleDrawnCount >
            gNdsFighterDLMultiDrawP0RealTriangleDrawnCount) &&
        (gNdsFighterDLAllDrawP1RealTriangleDrawnCount >
            gNdsFighterDLMultiDrawP1RealTriangleDrawnCount))
    {
        mask |= 1u << 5;
    }

    gNdsFighterDLAllDrawTotalPixelCount =
        gNdsFighterDLAllDrawP0PixelCount +
        gNdsFighterDLAllDrawP1PixelCount;
    if ((gNdsFighterDLAllDrawP0PixelCount >=
            gNdsFighterDLMultiDrawP0PixelCount) &&
        (gNdsFighterDLAllDrawP1PixelCount >=
            gNdsFighterDLMultiDrawP1PixelCount) &&
        (gNdsFighterDLAllDrawTotalPixelCount >=
            gNdsFighterDLMultiDrawTotalPixelCount) &&
        (gNdsFighterDLAllDrawP0ColorChecksum != 0u) &&
        (gNdsFighterDLAllDrawP1ColorChecksum != 0u))
    {
        mask |= 1u << 6;
    }
    if (gNdsFighterDLAllDrawTotalPixelCount > 0u)
    {
        ndsPlatformCommitOriginalDLPreview();
        gNdsFighterDLAllDrawPreviewCommitAfter =
            gNdsOriginalDLPreviewCommitCount;
        gNdsFighterDLAllDrawPreviewCommitDelta =
            gNdsFighterDLAllDrawPreviewCommitAfter -
            gNdsFighterDLAllDrawPreviewCommitBefore;
        gNdsFighterDLAllDrawPreviewReady = gNdsOriginalDLPreviewReady;
    }
    if ((gNdsFighterDLAllDrawPreviewReady != 0u) &&
        (gNdsFighterDLAllDrawPreviewCommitDelta == 1u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterDLAllDrawP0FirstBlocker ==
            NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLAllDrawP1FirstBlocker ==
            NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLAllDrawP0BlockerMask == 0u) &&
        (gNdsFighterDLAllDrawP1BlockerMask == 0u) &&
        (gNdsFighterDLAllDrawP0UnsupportedOpcode == 0u) &&
        (gNdsFighterDLAllDrawP1UnsupportedOpcode == 0u) &&
        (gNdsFighterDLAllDrawP0UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLAllDrawP1UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLAllDrawRangeRejectCount == 0u) &&
        (gNdsFighterDLAllDrawVertexRangeRejectCount == 0u))
    {
        mask |= 1u << 8;
    }

    gobj_after = (u32)gcGetGObjsActiveNum();
    gNdsFighterDLAllDrawGObjDelta =
        (gobj_after >= gobj_before) ? (gobj_after - gobj_before) :
            (gobj_before - gobj_after);
    if ((gNdsFighterDLAllDrawP0StatusAfter ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterDLAllDrawP1StatusAfter ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterDLAllDrawP0MotionAfter ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterDLAllDrawP1MotionAfter ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterDLAllDrawP0GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterDLAllDrawP1GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterDLAllDrawP0RootXBeforeBits ==
            gNdsFighterDLAllDrawP0RootXAfterBits) &&
        (gNdsFighterDLAllDrawP1RootXBeforeBits ==
            gNdsFighterDLAllDrawP1RootXAfterBits) &&
        (gNdsFighterDLAllDrawGObjDelta == 0u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsFighterDLAllDrawDrawCallCount == 0u) &&
        (gNdsFighterDLAllDrawMatrixCallCount == 0u) &&
        (gNdsFighterDLAllDrawGameplayUpdateCount == 0u) &&
        (gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
        (gSCManagerSceneData.scene_prev == nSCKindMaps))
    {
        mask |= 1u << 10;
    }

    gNdsFighterMarioFoxDLAllDrawMask = mask;
    gNdsFighterMarioFoxDLAllDrawDeferredMask = 0xffu;
    if ((mask & 0x7ffu) == 0x7ffu)
    {
        gNdsFighterMarioFoxDLAllDrawResult =
            NDS_FIGHTER_MARIOFOX_DL_ALL_DRAW_PASS;
        gNdsFighterMarioFoxDLAllDrawSafeResult =
            NDS_FIGHTER_MARIOFOX_DL_ALL_DRAW_SAFE_PASS;
    }
}
