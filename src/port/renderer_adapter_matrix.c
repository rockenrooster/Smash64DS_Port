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
/* wpsamusbomb.c specifies hexadecimal 0x46 (decimal 70), which maps to
 * func_ovl0_800CA194.  Like the 0x47 callback it emits gSPMvpRecalc and rewrites
 * the composed orientation while preserving the translation row. */
#define NDS_RENDERER_ADAPTER_MVP_RECALC_Z_0X46_KIND 0x46u
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
/* dLBCommonFuncMatrixList pair 16 (kind 0x52) is func_ovl0_800C9F70,
 * the source held-item attachment used by itMainSetFighterHold.  Do not treat
 * it as enum value decimal 52: custom matrix kinds are dispatched from 66. */
#define NDS_RENDERER_ADAPTER_ITEM_ATTACH_MTX_KIND 0x52u

extern void func_ovl2_800ED490(Mtx44f dst, Mtx44f lhs, Mtx44f rhs);
extern Vec2f dFTDisplayMainShufflePositions[][4];
#if NDS_LAB_NO_CULL
extern volatile u32 gNdsLabSeamArm;
#if NDS_R2_STRIP_ROUTE && (NDS_TASK56_FIGHTER_PRIMITIVES >= 1) && \
    (NDS_RENDERER_PROFILE_LEVEL < 2) && NDS_RENDERER_HW_TRIANGLES
#define NDS_RENDERER_ADAPTER_LAB_SOURCE_WORLD_ARM 4u
#else
#define NDS_RENDERER_ADAPTER_LAB_SOURCE_WORLD_ARM 3u
#endif
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

void ndsFighterRendererInvalidateMaterialCaches(void)
{
    /* Unknown/global lifetime seam: preserve the conservative contract. */
    ndsRendererFighterPacketInvalidateAll();
    ndsFighterRendererInvalidateMaterialRows();
}

void ndsFighterRendererInvalidateMaterialCachesForSlot(u32 slot)
{
    /* Material rows remain global because source free-list reuse can move an
     * MObj address between fighters.  The recorded GX stream cannot: packets
     * are keyed by source player slot, so only the fighter whose DObj/MObj graph
     * was replaced needs to re-record. Invalid slots fail safe in the renderer. */
    ndsRendererFighterPacketInvalidateSlot(slot);
    ndsFighterRendererInvalidateMaterialRows();
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
#if NDS_R2_FIGHTER_GX_COMPOSE
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

/* BattleShip lbcommon.c:1477-1604 (func_ovl0_800C9A38) + :1619-1634
 * (func_ovl0_800C9F70), specialized only at the final output boundary for the
 * DS renderer.  This is the 0x52 matrix installed on a held item's root by
 * itMainSetFighterHold.  The source deliberately removes joint/parent scale,
 * preserves world orientation + translation, then folds hitlag shuffle before
 * converting to fixed matrix form.  LinkBomb is the first live item client. */
static sb32 ndsRendererAdapterBuildItemAttachMtx(DObj *dobj, Mtx *out)
{
    DObj *attach_dobj;
    DObj *parent_dobj;
    FTParts *parts;
    FTStruct *fp;
    Mtx44f *p;
    Mtx44f f;
    Mtx44f item_world;
    f32 scale;

    if ((dobj == NULL) || (out == NULL))
    {
        return FALSE;
    }
    attach_dobj = (DObj *)dobj->user_data.p;
    if ((attach_dobj == NULL) || (attach_dobj == DOBJ_PARENT_NULL) ||
        (attach_dobj->parent_gobj == NULL))
    {
        return FALSE;
    }
    parts = ftGetParts(attach_dobj);
    fp = ftGetStruct(attach_dobj->parent_gobj);
    if ((parts == NULL) || (fp == NULL))
    {
        return FALSE;
    }

#define NDS_ITEM_ATTACH_NORMALIZE_ROW(dst, src, row) do { \
    scale = sqrtf(SQUARE((src)[row][0]) + SQUARE((src)[row][1]) + \
                  SQUARE((src)[row][2])); \
    if (scale != 0.0F) { scale = 1.0F / scale; } \
    (dst)[row][0] = (src)[row][0] * scale; \
    (dst)[row][1] = (src)[row][1] * scale; \
    (dst)[row][2] = (src)[row][2] * scale; \
} while (0)

    if ((fp->is_use_animlocks != FALSE) ||
        (attach_dobj->parent == DOBJ_PARENT_NULL))
    {
        func_ovl2_800EDBA4(attach_dobj);
        p = &parts->mtx_translate;
        NDS_ITEM_ATTACH_NORMALIZE_ROW(item_world, (*p), 0);
        NDS_ITEM_ATTACH_NORMALIZE_ROW(item_world, (*p), 1);
        NDS_ITEM_ATTACH_NORMALIZE_ROW(item_world, (*p), 2);
        item_world[3][0] = (*p)[3][0];
        item_world[3][1] = (*p)[3][1];
        item_world[3][2] = (*p)[3][2];
    }
    else
    {
        parent_dobj = attach_dobj->parent;
        if ((parent_dobj == NULL) || (parent_dobj == DOBJ_PARENT_NULL) ||
            (ftGetParts(parent_dobj) == NULL))
        {
            return FALSE;
        }
        gmCollisionTransformMatrixAll(attach_dobj, parts,
                                      parts->unk_dobjtrans_0x10);
        p = &parts->unk_dobjtrans_0x10;
        NDS_ITEM_ATTACH_NORMALIZE_ROW(f, (*p), 0);
        NDS_ITEM_ATTACH_NORMALIZE_ROW(f, (*p), 1);
        NDS_ITEM_ATTACH_NORMALIZE_ROW(f, (*p), 2);
        f[3][0] = (*p)[3][0];
        f[3][1] = (*p)[3][1];
        f[3][2] = (*p)[3][2];

        func_ovl2_800EDBA4(parent_dobj);
        p = &ftGetParts(parent_dobj)->mtx_translate;
        scale = sqrtf(SQUARE((*p)[0][0]) + SQUARE((*p)[0][1]) +
                      SQUARE((*p)[0][2]));
        if (scale != 0.0F) { scale = 1.0F / scale; }
        f[0][0] *= scale; f[1][0] *= scale; f[2][0] *= scale;
        scale = sqrtf(SQUARE((*p)[1][0]) + SQUARE((*p)[1][1]) +
                      SQUARE((*p)[1][2]));
        if (scale != 0.0F) { scale = 1.0F / scale; }
        f[0][1] *= scale; f[1][1] *= scale; f[2][1] *= scale;
        scale = sqrtf(SQUARE((*p)[2][0]) + SQUARE((*p)[2][1]) +
                      SQUARE((*p)[2][2]));
        if (scale != 0.0F) { scale = 1.0F / scale; }
        f[0][2] *= scale; f[1][2] *= scale; f[2][2] *= scale;
        func_ovl2_800ED490(item_world, *p, f);
    }

#undef NDS_ITEM_ATTACH_NORMALIZE_ROW

    if (fp->shuffle_tics != 0u)
    {
        const Vec2f *offset =
            &dFTDisplayMainShufflePositions[fp->is_shuffle_electric]
                                           [fp->shuffle_frame_index];
        item_world[3][0] += offset->x;
        item_world[3][1] += offset->y;
    }
    syMatrixF2LFixedW(&item_world, out);
    gNdsItemRendererAttach52BuildCount++;
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
    case NDS_RENDERER_ADAPTER_ITEM_ATTACH_MTX_KIND:
        if (ndsRendererAdapterBuildItemAttachMtx(dobj, &mtx) == FALSE)
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

/* THE SOURCE MVP-RECALC CALLBACKS THE PORT IMPLEMENTS. They `continue` out
 * of gcPrepDObjMatrix without emitting a gSPMatrix, so neither contributes a
 * local matrix to the parent chain: they rewrite the COMPOSED MVP instead, and
 * ndsRendererAdapterApplyMvpRecalc is where that happens. A DObj carrying one
 * must therefore skip it in the local build or the transform is applied twice
 * and in the wrong space.
 *
 * Samus made two previously-unmeasured cases observable in P2: built-in kind 46
 * (Charge Shot) and custom kind 0x46 / func_ovl0_800CA194 (Bomb).  Both preserve
 * the already-composed translation row in source; treating either as a normal
 * local rotation rotates the weapon's world translation around the origin.
 *
 * The remaining recalc kinds (41-43, 45, 47-50) still fall through to
 * ndsRendererAdapterBuildRecalcLocalMtx. Each has a different source formula
 * and still has no measured consumer, so leave those untouched rather than
 * converting them on speculation. */
static sb32 ndsRendererAdapterIsMvpRecalcKind(u32 kind)
{
    return ((kind == NDS_RENDERER_ADAPTER_MVP_RECALC_RPY_0X47_KIND) ||
            (kind == NDS_RENDERER_ADAPTER_MVP_RECALC_Z_0X46_KIND) ||
            (kind == NDS_RENDERER_ADAPTER_MVP_RECALC_PERSP_SCA_KIND) ||
            (kind == nGCMatrixKind46)) ?
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
    Mtx44f perspective_f;
    Mtx44f zrot_f;
    Mtx44f source_orientation_f;
    NDSRendererMatrix20p12 rotation;
    NDSRendererMatrix20p12 perspective;
    NDSRendererMatrix20p12 source_orientation;
    NDSRendererMatrix20p12 composed;
    s32 translate[4];
    s32 scale_x;
    s32 scale_y;
    f32 sinz;
    f32 cosz;
    f32 recalc_scale_x;
    f32 recalc_scale_y;
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
    else if ((kind == nGCMatrixKind46) ||
             (kind == NDS_RENDERER_ADAPTER_MVP_RECALC_Z_0X46_KIND))
    {
        /* objdisplay.c:960-1004 (kind 46) and lbcommon.c:1702-1771
         * (custom 0x46).  Both callbacks rewrite rows 0-2 of the CURRENT MVP
         * from gGCMatrixPerspF and leave row 3 alone.  Build those source rows
         * in float, convert once like the source's FTOFIX32 writes, and let the
         * common tail below restore the already-composed translation row.
         *
         * The built-in kind also carries the source's odd gGCScaleX contract:
         * X and Z use prior_scale_x * scale.x, Y uses prior_scale_x * scale.y.
         * Custom 0x46 is the unscaled Z-only version used by Samus Bomb. */
        syMatrixPerspFastF(perspective_f, &perspective_norm,
                           cobj->projection.persp.fovy,
                           cobj->projection.persp.aspect,
                           cobj->projection.persp.near,
                           cobj->projection.persp.far,
                           cobj->projection.persp.scale);
        syMatrixRotRpyRF(&zrot_f, 0.0F, 0.0F, dobj->rotate.vec.f.z);
        cosz = zrot_f[0][0];
        sinz = zrot_f[0][1];
        memset(&source_orientation_f, 0, sizeof(source_orientation_f));
        source_orientation_f[3][3] = 1.0F;

        if (kind == nGCMatrixKind46)
        {
            recalc_scale_x = sNdsRendererAdapterMvpRecalcScaleX *
                dobj->scale.vec.f.x;
            recalc_scale_y = sNdsRendererAdapterMvpRecalcScaleX *
                dobj->scale.vec.f.y;
            source_orientation_f[0][0] =
                perspective_f[0][0] * recalc_scale_x * cosz;
            source_orientation_f[1][0] =
                perspective_f[0][0] * recalc_scale_x * -sinz;
            source_orientation_f[0][1] =
                perspective_f[1][1] * recalc_scale_y * sinz;
            source_orientation_f[1][1] =
                perspective_f[1][1] * recalc_scale_y * cosz;
            source_orientation_f[2][2] =
                perspective_f[2][2] * recalc_scale_x;
            source_orientation_f[2][3] =
                perspective_f[2][3] * recalc_scale_x;
            sNdsRendererAdapterMvpRecalcScaleX = recalc_scale_x;
        }
        else
        {
            for (col = 0u; col < 4u; col++)
            {
                source_orientation_f[0][col] =
                    (perspective_f[0][col] * cosz) +
                    (perspective_f[1][col] * sinz);
                source_orientation_f[1][col] =
                    (perspective_f[0][col] * -sinz) +
                    (perspective_f[1][col] * cosz);
                source_orientation_f[2][col] = perspective_f[2][col];
            }
        }
        syMatrixF2L(&source_orientation_f, &rotation_mtx);
        ndsRendererAdapterMtxFromN64(&rotation_mtx, &source_orientation);
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
    if (kind == nGCMatrixKind46)
    {
        gNdsRendererAdapterKind46AppliedCount++;
    }
    else if (kind == NDS_RENDERER_ADAPTER_MVP_RECALC_Z_0X46_KIND)
    {
        gNdsRendererAdapterCustom46AppliedCount++;
    }
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
            (xobj->kind == NDS_RENDERER_ADAPTER_JOINT_ATTACH_TRA_MTX_KIND) ||
            (xobj->kind == NDS_RENDERER_ADAPTER_ITEM_ATTACH_MTX_KIND))
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
 * and the rest are allocated DOWNWARD from 30. Downward on purpose:
 * MTX_STORE/MTX_RESTORE address ABSOLUTE stack levels while glPushMatrix writes
 * whatever level the stack pointer is at, so the low levels belong to anyone who
 * pushes inside the execute (ndsRendererR2WriteLightVector does, once). Mario
 * needs one new slot and Fox eleven.
 *
 * THE CROSS-RUN RANGE IS 16..25, not the 16..23 this comment claimed until
 * 2026-08-25: Donkey's generated table reaches slot 25 (P2-3r17 read it off
 * `decode_joint_topology`, and the loop below takes the real union rather than
 * a constant, so the code was right while the comment was two owners stale).
 * That leaves exactly FIVE free levels above the reserved band -- 26..30 -- and
 * this loop then skips down past the band to 15 and below. Donkey alone needs
 * ten parent slots, so the next owner is already allocating underneath the
 * cross-run range; keep an eye on how far down that reaches against the levels
 * a push can occupy. */
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
 * The locals land in `hierarchy_storage.hierarchy_locals`, which mode 9 owns
 * and mode 10 does not
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

/* BUGS.md fighter seams -- source renderer precision, not a mesh weld.
 *
 * BattleShip's actual render contract is visible on both sides of gSPMatrix:
 * lbCommonFighterPartsFuncMatrix (lbcommon.c:1369-1441) produces one N64 Mtx
 * LOCAL per fighter joint, and Fast3D's GfxSpMatrix (interpreter.cpp:2511-2556)
 * decodes each 16.16 cell to float and concatenates those decoded locals in
 * float.  There is NO fixed-point re-quantisation between parent and child.
 *
 * The DS GX hierarchy had been feeding those same locals to MTX_MULT4x3 after
 * reducing them to Q20.12.  The geometry engine truncates every multiply at
 * 12 fractional bits, so a deep chain can drift by far more than one matrix
 * LSB.  That is the precision surface shared by the old software path and the
 * native GX path.  Do not weld joint-local vertices: DK has no exact-local
 * cross-binding aliases at all, so the BUGS.md weld-table hypothesis would
 * literally move source vertices that are not duplicates.
 *
 * Retain the source N64 local's 16 fractional bits and compose into a Q43.20
 * affine world.  The basis remains s32; translation is s64 so stage position
 * cannot consume the fixed-point integer range.  A 32x32->64 SMULL/SMLAL
 * kernel is radically cheaper on ARM9 than software IEEE-754, and postpones
 * the one DS Q20.12 boundary until the selected draw binding.  The host oracle
 * proves every drawn Mario/DK bind-pose vertex remains within one source 1/16
 * coordinate step and one final DS matrix LSB of Fast3D.  This is a measured
 * representation adaptation of the source matrix stack, not collision-world
 * substitution (FTParts::mtx_translate was tested and is observably different
 * because it skips the per-local 16.16 boundary). */
static s64 ndsRendererAdapterSourceRoundShiftS64(s64 value, u32 shift)
{
    s64 bias;

    if (shift == 0u)
    {
        return value;
    }
    bias = (s64)1 << (shift - 1u);
    if (value < 0)
    {
        return -(((-value) + bias) >> shift);
    }
    return (value + bias) >> shift;
}

static void ndsRendererAdapterSourceWorldIdentity(
    NDSRendererAdapterSourceWorld *out)
{
    u32 row;
    u32 col;

    memset(out, 0, sizeof(*out));
    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            out->basis[row][col] = (row == col) ?
                (1 << NDS_RENDERER_ADAPTER_SOURCE_WORLD_FRAC_BITS) : 0;
        }
    }
}

static void ndsRendererAdapterSourceWorldFromLocal(
    NDSRendererAdapterSourceWorld *out, const Mtx *local)
{
    u32 row;
    u32 col;

    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            out->basis[row][col] =
                ndsRendererMtxCellS16p16(local, row, col) * 16;
        }
    }
    for (col = 0u; col < 3u; col++)
    {
        out->translation[col] =
            (s64)ndsRendererMtxCellS16p16(local, 3u, col) * 16;
    }
}

static __attribute__((noinline, optimize("Os"), target("arm"))) void
ndsRendererAdapterSourceWorldMulLocal(
    NDSRendererAdapterSourceWorld *out,
    const Mtx *local,
    const NDSRendererAdapterSourceWorld *parent)
{
    NDSRendererAdapterSourceWorld temp;
    u32 row;
    u32 col;

    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            s64 sum =
                (s64)ndsRendererMtxCellS16p16(local, row, 0u) *
                    parent->basis[0][col] +
                (s64)ndsRendererMtxCellS16p16(local, row, 1u) *
                    parent->basis[1][col] +
                (s64)ndsRendererMtxCellS16p16(local, row, 2u) *
                    parent->basis[2][col];

            temp.basis[row][col] = (s32)
                ndsRendererAdapterSourceRoundShiftS64(
                    sum, NDS_RENDERER_ADAPTER_SOURCE_LOCAL_FRAC_BITS);
        }
    }
    for (col = 0u; col < 3u; col++)
    {
        s64 sum =
            (s64)ndsRendererMtxCellS16p16(local, 3u, 0u) *
                parent->basis[0][col] +
            (s64)ndsRendererMtxCellS16p16(local, 3u, 1u) *
                parent->basis[1][col] +
            (s64)ndsRendererMtxCellS16p16(local, 3u, 2u) *
                parent->basis[2][col];

        temp.translation[col] =
            ndsRendererAdapterSourceRoundShiftS64(
                sum, NDS_RENDERER_ADAPTER_SOURCE_LOCAL_FRAC_BITS) +
            parent->translation[col];
    }
    memcpy(out, &temp, sizeof(temp));
}

static sb32 ndsRendererAdapterSourceWorldTo20p12(
    const NDSRendererAdapterSourceWorld *source,
    NDSRendererMatrix20p12 *out)
{
    u32 row;
    u32 col;

    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            out->m[row][col] = (s32)
                ndsRendererAdapterSourceRoundShiftS64(
                    source->basis[row][col],
                    NDS_RENDERER_ADAPTER_SOURCE_TO_DS_SHIFT);
        }
        out->m[row][3] = 0;
    }
    for (col = 0u; col < 3u; col++)
    {
        s64 value = ndsRendererAdapterSourceRoundShiftS64(
            source->translation[col],
            NDS_RENDERER_ADAPTER_SOURCE_TO_DS_SHIFT);

        if ((value < (s64)(-2147483647 - 1)) ||
            (value > (s64)2147483647))
        {
            return FALSE;
        }
        out->m[3][col] = (s32)value;
    }
    out->m[3][3] = 1 << NDS_RENDERER_ADAPTER_MTX_FRAC_BITS;
    return TRUE;
}

static sb32 ndsRendererAdapterBuildSourceFighterLocalMtx(
    DObj *dobj, Mtx *out, sb32 *has_local)
{
    FTParts *parts;
    FTStruct *fp;
    u32 xobj_index;
    u32 fighter_parts_count = 0u;

    if ((dobj == NULL) || (dobj == DOBJ_PARENT_NULL) ||
        (out == NULL) || (has_local == NULL) ||
        (dobj->parent_gobj == NULL))
    {
        return FALSE;
    }
    *has_local = FALSE;
    for (xobj_index = 0u; xobj_index < dobj->xobjs_num; xobj_index++)
    {
        XObj *xobj = dobj->xobjs[xobj_index];

        if (xobj == NULL)
        {
            continue;
        }
        if (xobj->kind == nGCMatrixKindNull)
        {
            /* gcAddDObj3TransformsKind does not create a Null XObj.  Accept an
             * explicit one as the same no-op rather than inventing TRS. */
            continue;
        }
        if (xobj->kind != NDS_RENDERER_ADAPTER_FIGHTER_PARTS_MTX_KIND)
        {
            return FALSE;
        }
        fighter_parts_count++;
    }
    if (fighter_parts_count == 0u)
    {
        return TRUE;
    }
    if (fighter_parts_count != 1u)
    {
        return FALSE;
    }

    fp = ftGetStruct(dobj->parent_gobj);
    parts = ftGetParts(dobj);
    if ((fp == NULL) || (parts == NULL) || (fp->is_use_animlocks != FALSE))
    {
        return FALSE;
    }
    if (parts->transform_update_mode != 0)
    {
        if (ndsRendererAdapterF2LFixedWExact(
                &parts->unk_dobjtrans_0x10, out) == FALSE)
        {
            syMatrixF2LFixedW(&parts->unk_dobjtrans_0x10, out);
        }
    }
    else if (ndsFcmpNeC(dobj->scale.vec.f.x, 1.0F) ||
             ndsFcmpNeC(dobj->scale.vec.f.y, 1.0F) ||
             ndsFcmpNeC(dobj->scale.vec.f.z, 1.0F))
    {
        syMatrixTraRotRpyRSca(
            out,
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
    else if (ndsRendererAdapterBuildFighterTraRotRpyExact(
                 out,
                 dobj->translate.vec.f.x,
                 dobj->translate.vec.f.y,
                 dobj->translate.vec.f.z,
                 dobj->rotate.vec.f.x,
                 dobj->rotate.vec.f.y,
                 dobj->rotate.vec.f.z) == FALSE)
    {
        syMatrixTraRotRpyR(
            out,
            dobj->translate.vec.f.x,
            dobj->translate.vec.f.y,
            dobj->translate.vec.f.z,
            dobj->rotate.vec.f.x,
            dobj->rotate.vec.f.y,
            dobj->rotate.vec.f.z);
    }
    *has_local = TRUE;
    return TRUE;
}

static __attribute__((noinline, optimize("Os"))) sb32
ndsRendererAdapterComposeOwnerWorldsSource(
    DObj *root,
    DObj *const *bindings,
    u32 binding_count,
    NDSRendererMatrix20p12 *worlds,
    const NDSRendererMatrix20p12 *seed,
    sb32 seed_is_identity)
{
    DObj *joints[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    u8 joint_parents[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    u8 joint_bindings[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    NDSRendererAdapterSourceWorld *source_worlds =
        sNdsRendererAdapterNativeOwnerWorkspace.
            hierarchy_storage.source_worlds;
    u32 joint_count = 0u;
    u32 joint_index;
    u32 binding_index;

    if ((root == NULL) || (root == DOBJ_PARENT_NULL) ||
        (bindings == NULL) || (worlds == NULL) || (seed == NULL) ||
        (binding_count > NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED))
    {
        return FALSE;
    }
    memset(joints, 0, sizeof(joints));
    memset(joint_parents, 31, sizeof(joint_parents));
    memset(joint_bindings, 0xff, sizeof(joint_bindings));
    if ((ndsRendererAdapterCollectFighterTopology(
             root, 31u, joints, joint_parents, &joint_count) == FALSE) ||
        (joint_count == 0u) ||
        (joint_count > NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX))
    {
        return FALSE;
    }
    for (binding_index = 0u; binding_index < binding_count; binding_index++)
    {
        u32 found = NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX;

        for (joint_index = 0u; joint_index < joint_count; joint_index++)
        {
            if (joints[joint_index] == bindings[binding_index])
            {
                found = joint_index;
                break;
            }
        }
        if ((found >= joint_count) || (joint_bindings[found] != 0xffu))
        {
            return FALSE;
        }
        joint_bindings[found] = (u8)binding_index;
    }

    for (joint_index = 0u; joint_index < joint_count; joint_index++)
    {
        Mtx source_local;
        NDSRendererAdapterSourceWorld *parent_world;
        sb32 has_local;
        u32 parent = joint_parents[joint_index];

        if ((joints[joint_index] == NULL) ||
            ((parent == 31u) ?
                 (joints[joint_index]->parent != DOBJ_PARENT_NULL) :
                 ((parent >= joint_index) ||
                  (joints[joint_index]->parent != joints[parent]))))
        {
            return FALSE;
        }
        if (parent == 31u)
        {
            ndsRendererAdapterSourceWorldIdentity(
                &source_worlds[joint_index]);
            parent_world = NULL;
        }
        else
        {
            parent_world = &source_worlds[parent];
        }
        if (ndsRendererAdapterBuildSourceFighterLocalMtx(
                joints[joint_index], &source_local, &has_local) == FALSE)
        {
            return FALSE;
        }
        if (has_local != FALSE)
        {
            if (parent_world == NULL)
            {
                ndsRendererAdapterSourceWorldFromLocal(
                    &source_worlds[joint_index],
                    &source_local);
            }
            else
            {
                ndsRendererAdapterSourceWorldMulLocal(
                    &source_worlds[joint_index],
                    &source_local, parent_world);
            }
        }
        else if (parent_world != NULL)
        {
            memcpy(&source_worlds[joint_index],
                   parent_world, sizeof(*parent_world));
        }

        if (joint_bindings[joint_index] != 0xffu)
        {
            NDSRendererMatrix20p12 world;
            u32 out_index = joint_bindings[joint_index];

            /* The selected binding is the one DS representation boundary. */
            if (ndsRendererAdapterSourceWorldTo20p12(
                    &source_worlds[joint_index],
                    &world) == FALSE)
            {
                return FALSE;
            }
            if (seed_is_identity != FALSE)
            {
                ndsRendererMatrixCopy20p12(&worlds[out_index], &world);
            }
            else
            {
                ndsRendererMtxMulAffine20p12(
                    &world, seed, &worlds[out_index]);
            }
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
    sNdsRendererAdapterNativeOwnerWorkspace.gx_modelview_mirror_valid = 0u;
    /* Source-world seam repair.  Keep this deliberately narrow until every
     * owner is visually/source-qualified: Mario is owner slot 0, and profile
     * owner ids are one-based so DK's native slot is DONKEY-1. */
    if (((slot == 0u)
#if NDS_P2_DONKEY
         || (slot == ((u32)NDS_RENDERER_PROFILE_OWNER_DONKEY - 1u))
#endif
         ) &&
#if NDS_LAB_NO_CULL
        (gNdsLabSeamArm == NDS_RENDERER_ADAPTER_LAB_SOURCE_WORLD_ARM) &&
#endif
        (ndsRendererAdapterComposeOwnerWorldsSource(
             root, bindings, binding_count,
             sNdsRendererAdapterNativeOwnerModelviews, &compose_seed,
             seed_is_identity) != FALSE))
    {
        flat_worlds = TRUE;
    }
    else if (ndsRendererAdapterCaptureOwnerChainsGx(
            slot, bindings, binding_count,
            &sNdsRendererAdapterNativeOwnerWorkspace) != FALSE)
    {
        sNdsRendererAdapterNativeOwnerWorkspace.gx_valid = 1u;
        ndsRendererMatrixCopy20p12(
            &sNdsRendererAdapterNativeOwnerWorkspace.gx_seed, &compose_seed);
        sNdsRendererAdapterNativeOwnerWorkspace.gx_seed_is_identity =
            (u8)((seed_is_identity != FALSE) ? 1u : 0u);
#if NDS_P2_LINK
        /* P2-3f31. Link's source model contains a regular G_TEXTURE_GEN epoch.
         * Keep GX as the hierarchy executor, but mirror the exact same forward
         * compose on ARM9 so Fast3D's LookAt transform has the finished current
         * modelview it semantically consumes. This is representation-only: the
         * hardware still loads gx_seed/gx_locals below, and the CPU mirror is
         * never submitted as the fighter matrix. If the mirror cannot be built,
         * decline GX and fall through to the already-correct CPU path. */
        if (slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_LINK)
        {
            if (ndsRendererAdapterComposeOwnerWorldsFlat(
                    slot, bindings, binding_count,
                    sNdsRendererAdapterNativeOwnerModelviews, &compose_seed,
                    seed_is_identity) == FALSE)
            {
                sNdsRendererAdapterNativeOwnerWorkspace.gx_valid = 0u;
            }
            else
            {
                sNdsRendererAdapterNativeOwnerWorkspace.
                    gx_modelview_mirror_valid = 1u;
            }
        }
#endif
        if (sNdsRendererAdapterNativeOwnerWorkspace.gx_valid == 0u)
        {
            goto gx_compose_declined;
        }
        for (binding_index = 0u;
             binding_index < binding_count;
             binding_index++)
        {
#if NDS_TASK91_DRAW_PHASE_CENSUS
            gNdsTask91MtxBindings++;
#endif
            if (sNdsRendererAdapterNativeOwnerWorkspace.
                    gx_modelview_mirror_valid != 0u)
            {
                modelview_ptrs[binding_index] =
                    &sNdsRendererAdapterNativeOwnerModelviews[binding_index];
            }
            else
            {
                /* Never loaded by ordinary GX owners, but every preflight
                 * NULL-checks it and the seed is the honest answer for what the
                 * CPU retained. */
                modelview_ptrs[binding_index] =
                    &sNdsRendererAdapterNativeOwnerWorkspace.gx_seed;
            }
        }
        /* `*projection_ptr` was already published above, or deliberately left
         * NULL when the camera had no projection; do not second-guess it. */
        return TRUE;
    }
gx_compose_declined:
    if (flat_worlds == FALSE)
    {
        gNdsR2GxComposeDeclines++;
    }
#endif
    if (flat_worlds == FALSE)
    {
        flat_worlds = ndsRendererAdapterComposeOwnerWorldsFlat(
            slot, bindings, binding_count,
            sNdsRendererAdapterNativeOwnerModelviews, &compose_seed,
            seed_is_identity);
    }
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
                 joint,
                 &workspace->hierarchy_storage.hierarchy_locals[joint_index]) ==
             FALSE) ||
            (ndsRendererAdapterMatrixIsAffine20p12(
                 &workspace->hierarchy_storage.
                      hierarchy_locals[joint_index]) == FALSE))
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
    workspace->hierarchy.joint_locals =
        workspace->hierarchy_storage.hierarchy_locals;
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
