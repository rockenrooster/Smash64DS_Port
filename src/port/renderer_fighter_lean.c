/* P2-2p8 Phase 1 slices 1-3 -- the lean fighter path, adapter side.
 *
 * #included at the end of renderer_adapter_fighter.c so it shares that
 * translation unit (reloc_backend_renderer_dl.c) with the draw plan, the
 * display contract and the matrix helpers it reads. Not in CFILES.
 *
 * Slice 3: Donkey, Samus, Link and Kirby, every program and detail the old
 * path records, one instance per battle slot. Per draw once a list is adopted:
 *   head (unchanged, already ran) -> tuple compare (status/heap generation,
 *   root, detail, program, event count) -> the per-draw inputs the list was
 *   keyed on that no patch covers yet (material identity; preambles, proven
 *   once per memo fill) -> ndsFtrLeanKernelCompose -> the replay's patches on
 *   the lean copy (tint tiles, shade, matrices, light, Link's texgen) -> the
 *   replay's submit tail on the lean copy.
 * Route 2/3 stop after the patch and arm the TryReplay oracle instead of
 * drawing. See include/nds/renderer_fighter_lean.h. */

#include <nds/renderer_fighter_lean.h>
#include <nds/generated/nds_fighter_admission.generated.h>
#include <nds/nds_reloc_assets.h>

/* Slice 2b BSS diet: the per-root arrays hold the most DL-bearing joints any
 * kind x detail has (generator max, 24) instead of the draw collection's 32;
 * a plan with more selected roots is never adopted. */
#define NDS_FTR_LEAN_ROOT_MAX NDS_FIGHTER_ADMISSION_ROOT_MAX

#if defined(__arm__)
volatile u32 gNdsFtrLeanRoute
    __attribute__((used, section(".dtcm.bss"), aligned(4)));
volatile u32 gNdsFtrLeanAdmit
    __attribute__((used, section(".dtcm.bss"), aligned(4)));
#else
volatile u32 gNdsFtrLeanRoute;
volatile u32 gNdsFtrLeanAdmit;
#endif
#if NDS_FTR_LEAN_LAB
NDSFtrLeanCounters gNdsFtrLean __attribute__((used, aligned(32)));
#endif
volatile u32 gNdsFtrLeanOracleSourceOk __attribute__((used));
#if NDS_VRAM_CENSUS_LIVE
/* Slice 2a (lab): 1 = walk the texture/palette VRAM at every frame end.
 * DTCM like the route words, so a gdb poke is never hidden by the cache. */
#if defined(__arm__)
volatile u32 gNdsVramCensusEnable
    __attribute__((used, section(".dtcm.bss"), aligned(4)));
#else
volatile u32 gNdsVramCensusEnable;
#endif
#endif

/* ---- P2-2p8 Phase 1 slice 2b: fighter texture admission, adapter side ----
 * The creation seam (ndsFTManagerEnsureOwnerImages' caller) notes every
 * fighter of a battle; the admission itself (ndsFtrLeanAdmitRun, TU A) runs
 * once per battle: when gNdsFtrLeanAdmit is already set, at the end of the
 * battle scene's own texture preparation (ndsFtrLeanAdmitSceneTexturesReady,
 * slice 2c), else at the first frame end that sees it set. */
static u32 sNdsFtrAdmitGen;      /* gNdsTaskmanHeapGeneration + 1 noted */
static u32 sNdsFtrAdmitCount;
static u32 sNdsFtrAdmitKind[NDS_FTR_LEAN_ADMIT_FIGHTERS];
static u32 sNdsFtrAdmitCostume[NDS_FTR_LEAN_ADMIT_FIGHTERS];
static u32 sNdsFtrAdmitDetail[NDS_FTR_LEAN_ADMIT_FIGHTERS];
static u32 sNdsFtrAdmitPlayer[NDS_FTR_LEAN_ADMIT_FIGHTERS];
static u32 sNdsFtrAdmitDone;     /* generation + 1 the admission ran for */

/* "Is this a fight" is the scene manager's battle flag (VS, every 1P fight,
 * Training, the demo), never a scene-kind literal (nds_scene_manager.h). */
extern volatile u32 gNdsSceneManagerCurrIsBattle;

static u32 ndsFtrLeanAdmitBattleScene(void)
{
    return ((gSCManagerBattleState != NULL) &&
            (gNdsSceneManagerCurrIsBattle != 0u)) ? TRUE : FALSE;
}

static void ndsFtrLeanAdmitSync(void)
{
    u32 gen = gNdsTaskmanHeapGeneration + 1u;

    if (sNdsFtrAdmitGen != gen)
    {
        sNdsFtrAdmitGen = gen;
        sNdsFtrAdmitCount = 0u;
        sNdsFtrAdmitDone = 0u;
    }
}

#if NDS_FTR_LEAN_LIVE
static NDSRendererStats sNdsFtrLeanStats;

/* The loaded files of the fighters present, as asset id -> loaded data: each
 * kind's FTData file pointers, named by the reloc backend's own provenance
 * (the id the admission table's records carry). */
#define NDS_FTR_ADMIT_BASE_MAX (NDS_FTR_LEAN_ADMIT_FIGHTERS * 9u)
static u32 sNdsFtrAdmitBaseAsset[NDS_FTR_ADMIT_BASE_MAX];
static const void *sNdsFtrAdmitBaseData[NDS_FTR_ADMIT_BASE_MAX];

static u32 ndsFtrLeanAdmitCollectBases(void)
{
    u32 n = 0u;
    u32 i;

    for (i = 0u; i < sNdsFtrAdmitCount; i++)
    {
        const FTData *data = dFTManagerDataFiles[sNdsFtrAdmitKind[i]];
        void **files[9];
        u32 f;

        if (data == NULL)
        {
            continue;
        }
        files[0] = data->p_file_main;
        files[1] = data->p_file_mainmotion;
        files[2] = data->p_file_submotion;
        files[3] = data->p_file_model;
        files[4] = data->p_file_shieldpose;
        files[5] = data->p_file_special1;
        files[6] = data->p_file_special2;
        files[7] = data->p_file_special3;
        files[8] = data->p_file_special4;
        for (f = 0u; f < 9u; f++)
        {
            u32 asset = 0u;
            u32 offset = 0u;
            u32 k;

            if ((files[f] == NULL) || (*files[f] == NULL) ||
                (ndsRelocGetLoadedPointerProvenance(*files[f], &asset,
                                                    &offset) == FALSE) ||
                (offset != 0u))
            {
                continue;
            }
            for (k = 0u; k < n; k++)
            {
                if (sNdsFtrAdmitBaseAsset[k] == asset)
                {
                    break;
                }
            }
            if ((k == n) && (n < NDS_FTR_ADMIT_BASE_MAX))
            {
                sNdsFtrAdmitBaseAsset[n] = asset;
                sNdsFtrAdmitBaseData[n] = *files[f];
                n++;
            }
        }
    }
    return n;
}
#endif

static void ndsFtrLeanAdmitMaybeRun(u32 word)
{
#if NDS_FTR_LEAN_LIVE
    u32 bases;

    if ((word == 0u) || (sNdsFtrAdmitCount == 0u) ||
        (sNdsFtrAdmitDone == sNdsFtrAdmitGen) ||
        (ndsFtrLeanAdmitBattleScene() == FALSE))
    {
        return;
    }
    sNdsFtrAdmitDone = sNdsFtrAdmitGen;
    bases = ndsFtrLeanAdmitCollectBases();
    (void)ndsFtrLeanAdmitRun(&sNdsFtrLeanStats, sNdsFtrAdmitKind,
                             sNdsFtrAdmitCostume, sNdsFtrAdmitDetail,
                             sNdsFtrAdmitPlayer, sNdsFtrAdmitCount,
                             sNdsFtrAdmitBaseAsset, sNdsFtrAdmitBaseData,
                             bases, word);
#else
    (void)word;
#endif
}

#if defined(NDS_FTR_LEAN_ADMIT_DEFAULT) && NDS_FTR_LEAN_ADMIT_DEFAULT && \
    ((defined(NDS_TICK_HUD) && NDS_TICK_HUD) || \
     (defined(NDS_FTR_LEAN_ADMIT_LAB) && NDS_FTR_LEAN_ADMIT_LAB))
/* P2-2p8 Phase 1 slice 2c (lab builds only, Makefile
 * NDS_FTR_LEAN_ADMIT_DEFAULT; slice 3 lets a non-tick-HUD lab target -- the
 * 1P campaign walk ROM -- through with NDS_FTR_LEAN_ADMIT_LAB=1): boot with
 * the word set, so the admission runs
 * at the end of the battle scene's texture preparation -- the shipping path --
 * instead of at the first frame the sampler pokes. */
static u32 sNdsFtrAdmitDefaultApplied;
#endif

void ndsFtrLeanAdmitNoteFighter(u32 player, u32 fkind, u32 costume,
                                u32 detail)
{
#if defined(NDS_FTR_LEAN_ADMIT_DEFAULT) && NDS_FTR_LEAN_ADMIT_DEFAULT && \
    ((defined(NDS_TICK_HUD) && NDS_TICK_HUD) || \
     (defined(NDS_FTR_LEAN_ADMIT_LAB) && NDS_FTR_LEAN_ADMIT_LAB))
    if (sNdsFtrAdmitDefaultApplied == 0u)
    {
        sNdsFtrAdmitDefaultApplied = 1u;
        gNdsFtrLeanAdmit = NDS_FTR_LEAN_ADMIT_DEFAULT;
    }
#endif
    ndsFtrLeanAdmitSync();
    if ((ndsFtrLeanAdmitBattleScene() == FALSE) ||
        (sNdsFtrAdmitCount >= NDS_FTR_LEAN_ADMIT_FIGHTERS) ||
        (fkind >= 12u) || (sNdsFtrAdmitDone == sNdsFtrAdmitGen))
    {
        return;
    }
    sNdsFtrAdmitKind[sNdsFtrAdmitCount] = fkind;
    sNdsFtrAdmitCostume[sNdsFtrAdmitCount] = costume;
    sNdsFtrAdmitDetail[sNdsFtrAdmitCount] = detail;
    sNdsFtrAdmitPlayer[sNdsFtrAdmitCount] = player;
    sNdsFtrAdmitCount++;
}

/* Slice 2c: the creation-time admission. It used to run here, at the creation
 * of the battle's last fighter -- but every battle entry creates its fighters
 * BEFORE ndsBattlePrepareSceneTextures resets the texture VRAM
 * (glResetTextures) and places the scene's own static set, clouds and atlases,
 * so that reset discarded the whole admission (lab: exit_safety 1, and 458
 * fighter uploads after GO, exactly word 0's). The scene preparation calls
 * this last instead: the fighters are made, the scene's own textures are
 * placed, and the first frame has not been drawn. */
void ndsFtrLeanAdmitSceneTexturesReady(void)
{
    ndsFtrLeanAdmitSync();
    if ((gSCManagerBattleState != NULL) &&
        (sNdsFtrAdmitCount ==
         ((u32)gSCManagerBattleState->pl_count +
          (u32)gSCManagerBattleState->cp_count)))
    {
        ndsFtrLeanAdmitMaybeRun(gNdsFtrLeanAdmit);
    }
}


#if defined(__arm__)
volatile u32 gNdsFtrLeanSlow
    __attribute__((used, section(".dtcm.bss"), aligned(4)));
#else
volatile u32 gNdsFtrLeanSlow;
#endif

/* Slice 3: the texture-part writes of each battle slot (ftParamSetTexturePart-
 * ID / ResetTexturePartAll, reloc_backend_compat_shims.c) -- the one in-status
 * writer of a fighter MObj's texture state that is not a material animation. */
static u32 sNdsFtrLeanTexPartSerial[GMCOMMON_PLAYERS_MAX];

void ndsFtrLeanNoteTexturePart(u32 slot)
{
    if (slot < GMCOMMON_PLAYERS_MAX)
    {
        sNdsFtrLeanTexPartSerial[slot]++;
    }
}

#if NDS_FTR_LEAN_LIVE

/* Material identity (key[0]) without hashing every MObj every draw. Within
 * one status generation a fighter MObj's animated words (colours, texture
 * ids, lfrac, palette) have exactly three writers: its material animation,
 * which the per-frame player runs only while anim_wait != AOBJ_ANIM_NULL and
 * which only MObj creation attaches (lbCommonAddMObjForFighterPartsDObj --
 * creation, costume and model-part changes, all of which bump the status
 * generation and the rebind); the texture-part shims (counted per slot); and
 * the old path's own material preparation. So a draw proves: the texture-part
 * count is unchanged, each MObj with a live animation still hashes as at
 * adoption, and -- after any old-path draw of this fighter, and on every draw
 * of the oracle routes -- the whole identity is still the adoption's. */
#define NDS_FTR_LEAN_WATCH_MAX 6u
#define NDS_FTR_LEAN_WATCH_ALL 0xffu

/* One adopted list per battle slot (slice 3). Only what the per-draw path
 * reads lives here: the draw plan, keyed on the same status generation the
 * tuple compares, still holds the root events and material DObjs the list
 * was adopted from. */
typedef struct NDSFtrLeanInstance
{
    u8 valid;
    u8 kind;                    /* NDS_FTR_LEAN_OWNER_KIND */
    u8 detail;                  /* use_low_detail */
    u8 program;                 /* the plan's root program */
    u8 root_count;
    u8 joint_count;
    u8 rebind;
    u8 last_refuse;             /* last adoption refusal (0 = none) */
    u32 status_gen;
    u32 heap_gen;
    DObj *root;
    u32 event_count;
    u32 head_key;               /* the Kirby trio head key the draw publishes */
    u32 ident_adopt;            /* full material identity at adoption */
    u32 texpart_serial;         /* texture-part writes seen at adoption */
    u32 pre_hash;               /* the roots' key[3] preamble fields */
    u32 pre_serial;             /* memo fill serial + 1 they were proven at */
    u32 patch_serial;           /* memo fill serial + 1 of the last patch */
    u32 topo_hash;              /* the kept joints' links (oracle routes) */
    u8 watch_count;             /* animated MObjs, or WATCH_ALL */
    u8 reprove;                 /* the old path drew since the last proof */
    u8 learn_pending;           /* identity moved to an unlearned state */
    u8 pad2;
    const MObj *watch[NDS_FTR_LEAN_WATCH_MAX];
    u32 watch_hash[NDS_FTR_LEAN_WATCH_MAX];
    const Gfx *root_dl[NDS_FTR_LEAN_ROOT_MAX]; /* each root event's DL */
    NDSFtrLeanJoint joints[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
} NDSFtrLeanInstance;

static NDSFtrLeanInstance sNdsFtrLeanInstances[GMCOMMON_PLAYERS_MAX];
static u32 sNdsFtrLeanMatchHeapGen;
static u32 sNdsFtrLeanCamFrame;     /* profile frame + 1 of the fetch */
static const CObj *sNdsFtrLeanCamCobj;
static u32 sNdsFtrLeanCamOk;

/* Slice 3 RAM: a lean draw owns no per-draw BSS. The kernel's Q43.20 worlds
 * are a per-depth stack in its own frame; the modelviews go straight into the
 * list (ndsFtrLeanPacketModelviewSites); the rest is borrowed from the old
 * path, which a draw of either kind rewrites before it reads (the old path
 * never runs inside a lean draw, and routes 2/3 finish the lean patch before
 * the old path starts):
 *   binding worlds  sNdsRendererAdapterNativeOwnerModelviews (the old path's
 *                   own binding modelviews; written only for a texgen list)
 *   projection      sNdsRendererAdapterNativeOwnerProjection (same camera)
 *   root inputs     the workspace's production roots/configs: the lean draw
 *                   writes exactly the fields ndsRendererAdapterRefresh-
 *                   NativePacketInputs refreshes before every old-path read
 *                   (config modulate and initial geometry mode, the root's
 *                   preamble, matrices and gx_valid), never the primed ones. */
_Static_assert(NDS_FTR_LEAN_ROOT_MAX <= NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED,
               "lean roots must fit the borrowed binding arrays");
#define sNdsFtrLeanWorlds sNdsRendererAdapterNativeOwnerModelviews
#define sNdsFtrLeanProjection sNdsRendererAdapterNativeOwnerProjection

#if NDS_FTR_LEAN_LAB
#define NDS_FTR_LEAN_GUARD_PART(index, mark)                               \
    do                                                                     \
    {                                                                      \
        u32 part_now_ = cpuGetTiming();                                    \
        gNdsFtrLean.guard_part_ticks[index] += part_now_ - (mark);         \
        (mark) = part_now_;                                                \
    } while (0)
#else
#define NDS_FTR_LEAN_GUARD_PART(index, mark) ((void)0)
#endif

static void ndsFtrLeanDecline(u32 kind, u32 reason)
{
#if NDS_FTR_LEAN_LAB
    if (reason < nNDSFtrLeanDeclineCount)
    {
        gNdsFtrLean.decline[reason]++;
        if (kind < NDS_FTR_LEAN_KINDS)
        {
            gNdsFtrLean.k_decline[kind][reason]++;
        }
    }
#else
    (void)kind;
    (void)reason;
#endif
}

static void ndsFtrLeanRefuse(NDSFtrLeanInstance *inst, u32 kind, u32 reason)
{
    inst->last_refuse = (u8)reason;
#if NDS_FTR_LEAN_LAB
    if (reason < nNDSFtrLeanAdoptRefuseCount)
    {
        gNdsFtrLean.adopt_refuse[reason]++;
        if (kind < NDS_FTR_LEAN_KINDS)
        {
            gNdsFtrLean.k_adopt_refuse[kind][reason]++;
        }
    }
#else
    (void)kind;
#endif
}

static void ndsFtrLeanInvalidate(u32 slot)
{
    NDSFtrLeanInstance *inst = &sNdsFtrLeanInstances[slot];

    if (inst->valid != 0u)
    {
        ndsFtrLeanPacketDrop(slot);
    }
    inst->valid = 0u;
    inst->rebind = 0u;
    inst->learn_pending = 0u;
}

void ndsFtrLeanNoteRebind(u32 player_slot)
{
    if ((player_slot < GMCOMMON_PLAYERS_MAX) &&
        (sNdsFtrLeanInstances[player_slot].valid != 0u))
    {
        sNdsFtrLeanInstances[player_slot].rebind = 1u;
    }
}

/* Every joint the kernel's integer forms do not take: the adapter's own
 * source builder with the joint's lock accumulator passed in and out exactly
 * as ComposeOwnerWorldsSource passes lock_accum[j] (one Vec3f, aliased), then
 * the same N64-cell decode that compose performs. */
static s32 ndsFtrLeanSlowLocal(DObj *dobj, f32 *accum, s32 *cells,
                               u32 *has_local)
{
    Vec3f *scale = (Vec3f *)(void *)accum;
    Mtx mtx;
    sb32 local = FALSE;
    u32 row;
    u32 col;

    if (ndsRendererAdapterBuildSourceFighterLocalMtx(
            dobj, scale, scale, &mtx, &local) == FALSE)
    {
        return FALSE;
    }
    *has_local = (local != FALSE) ? TRUE : FALSE;
    if (local != FALSE)
    {
        for (row = 0u; row < 4u; row++)
        {
            for (col = 0u; col < 3u; col++)
            {
                cells[(row * 3u) + col] =
                    ndsRendererMtxCellS16p16(&mtx, row, col);
            }
        }
    }
    return TRUE;
}

/* The joint's XObj class, in ndsRendererAdapterBuildSourceFighterLocalMtx's
 * own order; fixed for the DObj's life, so classified once per adoption. */
static u32 ndsFtrLeanLocalKind(DObj *dobj, const FTParts **parts_out)
{
    u32 xobj_index;
    u32 parts_count = 0u;

    *parts_out = NULL;
    if ((dobj == NULL) || (dobj == DOBJ_PARENT_NULL) ||
        (dobj->parent_gobj == NULL))
    {
        return NDS_FTR_LEAN_LOCAL_NO_GOBJ;
    }
    for (xobj_index = 0u; xobj_index < dobj->xobjs_num; xobj_index++)
    {
        XObj *xobj = dobj->xobjs[xobj_index];

        if ((xobj == NULL) || (xobj->kind == nGCMatrixKindNull))
        {
            continue;
        }
        if (xobj->kind != NDS_RENDERER_ADAPTER_FIGHTER_PARTS_MTX_KIND)
        {
            return NDS_FTR_LEAN_LOCAL_XOBJ;
        }
        parts_count++;
    }
    if (parts_count == 0u)
    {
        return NDS_FTR_LEAN_LOCAL_NONE;
    }
    if ((parts_count != 1u) || (ftGetStruct(dobj->parent_gobj) == NULL) ||
        (ftGetParts(dobj) == NULL))
    {
        return NDS_FTR_LEAN_LOCAL_XOBJ;
    }
    *parts_out = ftGetParts(dobj);
    return NDS_FTR_LEAN_LOCAL_PARTS;
}

/* The live links of every kept joint (the oracle routes' topology proof, and
 * the cost A/B's slice 1 guard). */
static u32 ndsFtrLeanTopologyHash(const NDSFtrLeanInstance *inst)
{
    u32 h = 2166136261u;
    u32 j;

    for (j = 0u; j < inst->joint_count; j++)
    {
        const DObj *dobj = inst->joints[j].dobj;

        h = (h ^ (u32)(uintptr_t)dobj->child) * 16777619u;
        h = (h ^ (u32)(uintptr_t)dobj->sib_next) * 16777619u;
        h = (h ^ (u32)(uintptr_t)dobj->parent) * 16777619u;
    }
    return h;
}

/* The live topology ComposeOwnerWorldsSource collects every draw, captured
 * once per adoption with the same link checks, then cut to the joints a drawn
 * binding hangs from: a binding's world (lock accumulator included) depends
 * on its ancestor chain alone, so the rest are never composed. Preorder is
 * kept, so every parent still precedes its children. */
static s32 ndsFtrLeanBuildJoints(NDSFtrLeanInstance *inst, DObj *root,
                                 DObj *const *bindings, u32 binding_count)
{
    DObj *joints[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    u8 parents[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    u8 binding_of[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    u8 needed[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    u8 remap[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    u8 depth_of[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    u32 count = 0u;
    u32 kept = 0u;
    u32 j;
    u32 b;

    if ((root == NULL) || (root == DOBJ_PARENT_NULL) ||
        (binding_count == 0u) || (binding_count > NDS_FTR_LEAN_ROOT_MAX))
    {
        return FALSE;
    }
    memset(joints, 0, sizeof(joints));
    memset(parents, 0xff, sizeof(parents));
    if ((ndsRendererAdapterCollectFighterTopology(
             root, 0xffu, joints, parents, &count) == FALSE) ||
        (count == 0u) || (count > NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX))
    {
        return FALSE;
    }
    memset(binding_of, 0xff, sizeof(binding_of));
    memset(needed, 0, sizeof(needed));
    memset(depth_of, 0, sizeof(depth_of));
    for (j = 0u; j < count; j++)
    {
        DObj *want_parent = (parents[j] == 0xffu) ?
            DOBJ_PARENT_NULL : joints[parents[j]];

        if ((joints[j] == NULL) ||
            ((parents[j] != 0xffu) && ((u32)parents[j] >= j)) ||
            (joints[j]->parent != want_parent))
        {
            return FALSE;
        }
    }
    for (b = 0u; b < binding_count; b++)
    {
        for (j = 0u; j < count; j++)
        {
            if (joints[j] == bindings[b])
            {
                break;
            }
        }
        if ((j >= count) || (binding_of[j] != 0xffu))
        {
            return FALSE;
        }
        binding_of[j] = (u8)b;
    }
    for (j = count; j-- > 0u;)
    {
        if (binding_of[j] != 0xffu)
        {
            needed[j] = 1u;
        }
        if ((needed[j] != 0u) && (parents[j] != 0xffu))
        {
            needed[parents[j]] = 1u;
        }
    }
    for (j = 0u; j < count; j++)
    {
        NDSFtrLeanJoint *out;
        const FTParts *parts;

        if (needed[j] == 0u)
        {
            continue;
        }
        /* Depth in the kept tree (a kept joint's parent is kept). */
        depth_of[j] = (parents[j] == 0xffu) ? 0u :
            (u8)(depth_of[parents[j]] + 1u);
        if (depth_of[j] >= NDS_FTR_LEAN_DEPTH_MAX)
        {
            return FALSE;
        }
        remap[j] = (u8)kept;
        out = &inst->joints[kept++];
        out->dobj = joints[j];
        out->parent = (parents[j] == 0xffu) ? 0xffu : remap[parents[j]];
        out->binding = binding_of[j];
        out->local_kind = (u8)ndsFtrLeanLocalKind(joints[j], &parts);
        out->parts = parts;
        out->depth = depth_of[j];
    }
    inst->joint_count = (u8)kept;
    return (kept != 0u) ? TRUE : FALSE;
}

/* The per-root preamble fields key[3] folds in (prim and the light direction
 * are patched, not keyed). */
static u32 ndsFtrLeanPreambleHash(const NDSFighterDrawPlanData *plan,
                                  u32 count)
{
    u32 h = 2166136261u;
    u32 i;

    for (i = 0u; i < count; i++)
    {
        const NDSRendererNativeFighterPreamble *pre =
            &sNdsFighterDisplayReplayPreambles[plan->collection.indices[i]];

        h = (h ^ (u32)pre->geometry_mode) * 16777619u;
        h = (h ^ (u32)pre->cycle_type) * 16777619u;
        h = (h ^ (u32)pre->render_mode) * 16777619u;
        h = (h ^ (u32)pre->env_color) * 16777619u;
        h = (h ^ (u32)pre->flags) * 16777619u;
    }
    return h;
}

/* The list now holds the words of material identity `ident` (its base or a
 * learned variant): hold that identity from here. */
static void ndsFtrLeanRebaseIdentity(u32 slot, NDSFtrLeanInstance *inst,
                                     const NDSFighterDrawPlanData *plan,
                                     u32 ident)
{
    u32 w;

    (void)plan;
    inst->ident_adopt = ident;
    inst->texpart_serial = sNdsFtrLeanTexPartSerial[slot];
    if (inst->watch_count != NDS_FTR_LEAN_WATCH_ALL)
    {
        for (w = 0u; w < inst->watch_count; w++)
        {
            inst->watch_hash[w] =
                ndsRendererAdapterMaterialAnimHash(inst->watch[w]);
        }
    }
}

/* Slice 3: a status change that leaves the drawn roots exactly as they were.
 * Every status change bumps the generation the draw plan is keyed on, and the
 * old path then re-resolves the plan from this frame's display contract --
 * a pure function of the contract's events (the collection is derived from
 * them, and each root resolves from its event's DL, matrix and material
 * DObjs), the owner's loaded file and the detail. When all of those are the
 * ones the adopted plan was resolved from, that resolve returns this plan, so
 * re-keying it to the new generation is the old path's own answer without the
 * old path's draw: the list stays adopted and the status change draws lean.
 * The identity and the preambles are then re-proven in full on this draw
 * (the status change may have moved them); the kernel's link check covers
 * any re-parenting. The oracle routes also re-derive the plan outright
 * (ndsFighterDrawPlanVerify: gNdsFtrPlanVerifyRuns / Mismatch). */
static s32 ndsFtrLeanRetupleBody(u32 slot, FTStruct *fp,
                                 NDSFtrLeanInstance *inst,
                                 u32 use_low_detail, u32 owner_slot,
                                 u32 route)
{
    NDSFighterDrawPlan *plan = &sNdsFighterDrawPlan[slot];
    const NDSRelocLoadedFile *file = plan->data.owner_file;
    NDSFighterDLAllDrawCollection live;
    u32 i;

    if ((gNdsFtrPlanRoute == 0u) || (plan->valid == 0u) || (file == NULL) ||
        (file->data != plan->key_data) ||
        (file->asset_id != plan->key_asset_id) ||
        (file->owner_generation != plan->key_owner_generation) ||
        (file->data_size != plan->key_data_size) ||
        (plan->key_use_low_detail != use_low_detail) ||
        (plan->key_status_generation != inst->status_gen))
    {
        return FALSE;
    }
    ndsFighterCollectAllDObjsWithDL(fp->joints[nFTPartsJointTopN], &live);
#if NDS_R2_FOX_GUN_OVERLAY
    ndsFighterCollectStripFoxGunSidecar(fp, &live);
#endif
    if (memcmp(&live, &plan->data.collection, sizeof(live)) != 0)
    {
        return FALSE;
    }
    for (i = 0u; i < inst->root_count; i++)
    {
        const NDSFighterDisplayContractEvent *event =
            &sNdsFighterDisplayReplayEvents[live.indices[i]];

        if ((event->dl != inst->root_dl[i]) ||
            (event->matrix_dobj != plan->data.matrix_bindings[i]) ||
            (event->material_dobj != plan->data.material_dobjs[i]))
        {
            return FALSE;
        }
    }
    plan->key_status_generation = sNdsFighterStatusGeneration[slot];
    inst->status_gen = sNdsFighterStatusGeneration[slot];
    inst->reprove = 1u;
    inst->pre_serial = 0u;
    inst->patch_serial = 0u;
    /* A status change may add or drop DObjs that draw nothing (the drawn
     * collection above is unchanged), which moves the kept joints' child and
     * sibling links but no drawn world; the oracle routes' link hash restarts
     * from here. Re-parenting a kept joint is the kernel's to refuse. */
    inst->topo_hash = ndsFtrLeanTopologyHash(inst);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.retuples++);
#if NDS_TICK_HUD
    if (route != NDS_FTR_LEAN_ROUTE_DRAW)
    {
        ndsFighterDrawPlanVerify(
            slot, owner_slot, use_low_detail, fp,
            fp->joints[nFTPartsJointTopN],
            ndsFighterNativeOwnerModelAssetId(owner_slot),
            &sNdsRendererAdapterNativeOwnerWorkspace);
    }
#else
    (void)owner_slot;
    (void)route;
#endif
    return TRUE;
}

static s32 ndsFtrLeanRetuple(u32 slot, FTStruct *fp,
                             NDSFtrLeanInstance *inst, u32 use_low_detail,
                             u32 owner_slot, u32 route)
{
#if NDS_FTR_LEAN_LAB
    u32 t0 = cpuGetTiming();
    s32 kept = ndsFtrLeanRetupleBody(slot, fp, inst, use_low_detail,
                                     owner_slot, route);

    gNdsFtrLean.retuple_ticks += cpuGetTiming() - t0;
    return kept;
#else
    return ndsFtrLeanRetupleBody(slot, fp, inst, use_low_detail, owner_slot,
                                 route);
#endif
}

/* Slice 3's admission: the four stress kinds, any detail, native production
 * mode, never the 1P intro's transient actors (they never plan). */
static u32 ndsFtrLeanEligible(FTStruct *fp, u32 *owner_slot)
{
    u32 kind;

    if ((sNdsIntroTransientActive != FALSE) ||
        (ndsFighterGetNativeOwnerSlot(fp, owner_slot) == FALSE))
    {
        return NDS_FTR_LEAN_KIND_NONE;
    }
    kind = NDS_FTR_LEAN_OWNER_KIND(*owner_slot);
    if ((kind == NDS_FTR_LEAN_KIND_NONE) ||
        ((gNdsRendererFastRunMode !=
          NDS_RENDERER_FAST_RUN_NATIVE_FIGHTER_OWNER_PRODUCTION) &&
         (gNdsRendererFastRunMode !=
          NDS_RENDERER_FAST_RUN_NATIVE_COMPLETE_STAGE)))
    {
        return NDS_FTR_LEAN_KIND_NONE;
    }
    return kind;
}

/* Returns TRUE when route 1 drew the fighter (the old path must not run). */
static sb32 ndsFtrLeanRun(u32 slot, FTStruct *fp, u32 route)
{
    NDSFtrLeanInstance *inst = &sNdsFtrLeanInstances[slot];
    const NDSFighterDrawPlanData *plan;
    u32 owner_slot = 0u;
    u32 kind;
    u32 use_low_detail;
    u32 slow_mode = gNdsFtrLeanSlow;
    u32 reason;
    u32 t0;
    u32 t1;
    u32 mark;
    u32 kernel_flags;
    u32 color_modulate;
    u32 sites;
    u32 *mv_sites[NDS_FTR_LEAN_ROOT_MAX];
    u32 *class_counts = NULL;
    u32 *part_ticks = NULL;
    s32 shuffle_x = 0;
    s32 shuffle_y = 0;
    NDSRendererMatrix20p12 camera_modelview;
    u32 camera_projection_valid = FALSE;
    u32 camera_modelview_valid = FALSE;
    u32 i;

    kind = ndsFtrLeanEligible(fp, &owner_slot);
    if (kind == NDS_FTR_LEAN_KIND_NONE)
    {
        return FALSE;   /* not a lean kind: not an attempt */
    }
    NDS_FTR_LEAN_CTR(gNdsFtrLean.attempts++);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.k_attempts[kind]++);
    t0 = cpuGetTiming();
    mark = t0;
    if (fp->colanim.skeleton_id != 0)
    {
        /* R7: no native electric skeleton exists for these kinds. */
        ndsFtrLeanDecline(kind, nNDSFtrLeanDeclineSkeleton);
        return FALSE;
    }
#if !NDS_R2_FIGHTER_SHUFFLE_FOLD
    if (fp->shuffle_tics != 0u)
    {
        ndsFtrLeanDecline(kind, nNDSFtrLeanDeclineKind);
        return FALSE;
    }
#endif
    if (inst->valid == 0u)
    {
        ndsFtrLeanDecline(kind,
            (inst->last_refuse == nNDSFtrLeanAdoptPlan) ?
                nNDSFtrLeanDeclineUncacheable :
                nNDSFtrLeanDeclineAdoptPending);
        return FALSE;
    }
    if (inst->rebind != 0u)
    {
        ndsFtrLeanInvalidate(slot);
        ndsFtrLeanDecline(kind, nNDSFtrLeanDeclineRebind);
        return FALSE;
    }
    /* The tuple: kind, detail, status and heap generation, root, the plan
     * (program and root vector) and the display contract's event count. */
    use_low_detail = (fp->detail_curr == nFTPartsDetailLow) ? TRUE : FALSE;
    if ((inst->kind != kind) || (inst->detail != use_low_detail) ||
        (inst->heap_gen != gNdsTaskmanHeapGeneration) ||
        (inst->root != fp->joints[nFTPartsJointTopN]) ||
        ((inst->status_gen != sNdsFighterStatusGeneration[slot]) &&
         (((slow_mode & NDS_FTR_LEAN_SLOW_GUARD) != 0u) ||
          (ndsFtrLeanRetuple(slot, fp, inst, use_low_detail, owner_slot,
                             route) == FALSE))) ||
        (ndsFighterDrawPlanHit(slot, use_low_detail) == FALSE))
    {
        ndsFtrLeanInvalidate(slot);
        ndsFtrLeanDecline(kind, nNDSFtrLeanDeclineTuple);
        return FALSE;
    }
    plan = &sNdsFighterDrawPlan[slot].data;
    if ((plan->root_program != inst->program) ||
        (plan->collection.selected_count != inst->root_count))
    {
        ndsFtrLeanInvalidate(slot);
        ndsFtrLeanDecline(kind, nNDSFtrLeanDeclineTuple);
        return FALSE;
    }
    /* A changed display contract under an unchanged plan (a hidden flag
     * flipped inside a status) is what the old path draws from too: its plan
     * hit reads the roots' preambles at the plan's event indices, exactly as
     * the lean inputs do, and the preamble proof below re-checks those
     * (an event-count change is always a memo fill or bypass). */
    NDS_FTR_LEAN_GUARD_PART(0u, mark);
    /* Topology: the kernel proves every kept joint's parent link per draw
     * (the source compose's own check). The oracle routes and the cost A/B's
     * slice 1 form also hash the kept joints' child/sibling links. */
    if (((route != NDS_FTR_LEAN_ROUTE_DRAW) ||
         ((slow_mode & NDS_FTR_LEAN_SLOW_GUARD) != 0u)) &&
        (ndsFtrLeanTopologyHash(inst) != inst->topo_hash))
    {
        ndsFtrLeanInvalidate(slot);
        ndsFtrLeanDecline(kind, nNDSFtrLeanDeclineTopology);
        return FALSE;
    }
    NDS_FTR_LEAN_GUARD_PART(1u, mark);
    {
        /* The camera matrices are a per-frame, per-camera constant (the
         * adapter's own frame cache); fetch them once per frame. The storage
         * is the old path's projection, which any draw of this frame
         * rewrites with the same value. */
        CObj *cobj = (gGCCurrentCamera != NULL) ?
            CObjGetStruct(gGCCurrentCamera) : NULL;

        if ((sNdsFtrLeanCamFrame != gNdsRendererProfileFrameCount + 1u) ||
            (sNdsFtrLeanCamCobj != cobj))
        {
            ndsRendererAdapterGetFrameCameraMatrices(
                cobj, &sNdsFtrLeanProjection, &camera_projection_valid,
                &camera_modelview, &camera_modelview_valid, NULL, NULL,
                NULL);
            sNdsFtrLeanCamFrame = gNdsRendererProfileFrameCount + 1u;
            sNdsFtrLeanCamCobj = cobj;
            sNdsFtrLeanCamOk = ((camera_projection_valid != FALSE) &&
                                (camera_modelview_valid == FALSE)) ?
                TRUE : FALSE;
        }
        if (sNdsFtrLeanCamOk == FALSE)
        {
            ndsFtrLeanDecline(kind, nNDSFtrLeanDeclineCamera);
            return FALSE;
        }
    }
    NDS_FTR_LEAN_GUARD_PART(2u, mark);
    /* key[0]: the live MObj chains' identity (texture ids, matanim words),
     * proven through its writers (see NDS_FTR_LEAN_WATCH_MAX). */
    {
        u32 watch_ok = (inst->texpart_serial ==
                        sNdsFtrLeanTexPartSerial[slot]) ? TRUE : FALSE;
        u32 full = ((route != NDS_FTR_LEAN_ROUTE_DRAW) ||
                    ((slow_mode & NDS_FTR_LEAN_SLOW_GUARD) != 0u) ||
                    (inst->reprove != 0u) ||
                    (inst->watch_count == NDS_FTR_LEAN_WATCH_ALL)) ?
            TRUE : FALSE;

        if ((watch_ok != FALSE) &&
            (inst->watch_count != NDS_FTR_LEAN_WATCH_ALL))
        {
            u32 w;

            for (w = 0u; w < inst->watch_count; w++)
            {
                if (ndsRendererAdapterMaterialAnimHash(inst->watch[w]) !=
                    inst->watch_hash[w])
                {
                    watch_ok = FALSE;
                    break;
                }
            }
        }
        u32 ident_now = 0u;

        if ((watch_ok != FALSE) && (full != FALSE))
        {
            ident_now = ndsRendererAdapterMaterialIdentity(
                plan->material_dobjs, inst->root_count);
            if (ident_now != inst->ident_adopt)
            {
                /* A writer the watch did not see: after an old-path draw
                 * (its material preparation) this is expected and fine; on
                 * a draw that did not follow one it is a hole in the watch. */
                if (inst->reprove == 0u)
                {
                    NDS_FTR_LEAN_CTR(gNdsFtrLean.ident_watch_miss++);
                }
                watch_ok = FALSE;
            }
        }
        else if (watch_ok == FALSE)
        {
            ident_now = ndsRendererAdapterMaterialIdentity(
                plan->material_dobjs, inst->root_count);
        }
        if (watch_ok == FALSE)
        {
            /* The identity moved. A state this list already learned (its
             * base or a variant) is a few word writes; anything else is the
             * old path's to draw once, and AfterOldPath learns it. */
            if ((ident_now != inst->ident_adopt) &&
                (ndsFtrLeanPacketSelectVariant(slot, ident_now) != FALSE))
            {
                ndsFtrLeanRebaseIdentity(slot, inst, plan, ident_now);
                NDS_FTR_LEAN_CTR(gNdsFtrLean.variant_switches++);
            }
            else if (ident_now == inst->ident_adopt)
            {
                /* Only the texture-part count moved (a write of the same id
                 * or to an undrawn MObj): nothing to switch. */
                ndsFtrLeanRebaseIdentity(slot, inst, plan, ident_now);
            }
            else
            {
                inst->learn_pending = 1u;
                ndsFtrLeanDecline(kind, nNDSFtrLeanDeclineMaterial);
                return FALSE;
            }
        }
        inst->reprove = 0u;
    }
    NDS_FTR_LEAN_GUARD_PART(3u, mark);
    /* key[3]'s per-root preamble fields. A memo hit replays the slot's stored
     * preambles byte for byte, so they are proven once per memo fill. */
    if ((sNdsFtrLeanMemoFromSlot == 0u) ||
        (inst->pre_serial != sNdsFtrLeanMemoFill[slot] + 1u) ||
        ((slow_mode & NDS_FTR_LEAN_SLOW_GUARD) != 0u))
    {
        NDS_FTR_LEAN_CTR(gNdsFtrLean.pre_checks++);
        if (ndsFtrLeanPreambleHash(plan, inst->root_count) != inst->pre_hash)
        {
            ndsFtrLeanInvalidate(slot);
            ndsFtrLeanDecline(kind, nNDSFtrLeanDeclinePreamble);
            return FALSE;
        }
        inst->pre_serial = (sNdsFtrLeanMemoFromSlot != 0u) ?
            (sNdsFtrLeanMemoFill[slot] + 1u) : 0u;
    }
    else
    {
        NDS_FTR_LEAN_CTR(gNdsFtrLean.pre_skips++);
    }
    NDS_FTR_LEAN_GUARD_PART(4u, mark);
    reason = ndsFtrLeanPacketGuard(slot,
        (route == NDS_FTR_LEAN_ROUTE_DRAW) ? 1u : 0u);
    if (reason != 0u)
    {
        ndsFtrLeanInvalidate(slot);
        ndsFtrLeanDecline(kind, reason);
        return FALSE;
    }
    NDS_FTR_LEAN_GUARD_PART(5u, mark);
#if NDS_R2_FIGHTER_SHUFFLE_FOLD
    /* The old path latches this every draw; so does the lean one, so the
     * statics read after this draw are the same in both routes. */
    ndsRendererAdapterSetShuffleOffset(fp);
    shuffle_x = sNdsR2ShuffleWorldX;
    shuffle_y = sNdsR2ShuffleWorldY;
#endif
    NDS_FTR_LEAN_GUARD_PART(6u, mark);
    t1 = cpuGetTiming();
    NDS_FTR_LEAN_CTR(gNdsFtrLean.guard_ticks += t1 - t0);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.k_guard_ticks[kind] += t1 - t0);
#if NDS_FTR_LEAN_LAB
    class_counts = gNdsFtrLean.k_kernel_class[kind];
#if NDS_FTR_LEAN_KTIME
    if ((slow_mode & NDS_FTR_LEAN_SLOW_KTIME) != 0u)
    {
        part_ticks = gNdsFtrLean.kernel_part_ticks;
        gNdsFtrLean.kernel_part_joints += inst->joint_count;
    }
#endif
#endif
    kernel_flags = ((fp->is_use_animlocks != FALSE) ?
                        NDS_FTR_LEAN_KERNEL_LOCKS : 0u) |
        (((slow_mode & NDS_FTR_LEAN_SLOW_KERNEL) != 0u) ?
             NDS_FTR_LEAN_KERNEL_NO_FAST : 0u);
    /* The modelviews go straight into the list; only a texgen group's root
     * also needs its Q20.12 world (PatchTexgen reads it). */
    sites = ndsFtrLeanPacketModelviewSites(slot, mv_sites, inst->root_count);
    if ((sites == NDS_FTR_LEAN_SITES_NONE) ||
        (ndsFtrLeanKernelCompose(
             inst->joints, inst->joint_count, mv_sites, sNdsFtrLeanWorlds,
             sites, inst->root_count, shuffle_x, shuffle_y, kernel_flags,
             ndsFtrLeanSlowLocal, class_counts, part_ticks) == FALSE))
    {
        /* A kept joint was re-parented (a status change that moves no drawn
         * root: the re-tuple proved the roots). The world is a function of
         * the live tree, which the source compose re-collects every draw, so
         * re-collect it once and compose again; a second refusal is the
         * source compose's own. */
        NDS_FTR_LEAN_CTR(gNdsFtrLean.kernel_fail++);
        if ((sites == NDS_FTR_LEAN_SITES_NONE) ||
            (ndsFtrLeanBuildJoints(inst, fp->joints[nFTPartsJointTopN],
                                   plan->matrix_bindings,
                                   inst->root_count) == FALSE) ||
            (ndsFtrLeanKernelCompose(
                 inst->joints, inst->joint_count, mv_sites, sNdsFtrLeanWorlds,
                 sites, inst->root_count, shuffle_x, shuffle_y, kernel_flags,
                 ndsFtrLeanSlowLocal, class_counts, part_ticks) == FALSE))
        {
            ndsFtrLeanInvalidate(slot);
            ndsFtrLeanDecline(kind, nNDSFtrLeanDeclineKernel);
            return FALSE;
        }
        inst->topo_hash = ndsFtrLeanTopologyHash(inst);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.kernel_rebuilds++);
    }
    t0 = cpuGetTiming();
    NDS_FTR_LEAN_CTR(gNdsFtrLean.kernel_ticks += t0 - t1);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.k_kernel_ticks[kind] += t0 - t1);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.kernel_joints += inst->joint_count);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.k_kernel_joints[kind] += inst->joint_count);

    if (sNdsRendererAdapterProductionInputsPrimed == FALSE)
    {
        ndsRendererAdapterPrimeProductionInputs(
            &sNdsRendererAdapterNativeOwnerWorkspace);
    }
    color_modulate = ndsRendererAdapterFighterColorModulate(fp);
    for (i = 0u; i < inst->root_count; i++)
    {
        NDSRendererNativeFighterRoot *input =
            &sNdsRendererAdapterNativeOwnerWorkspace.production_roots[i];
        NDSRendererConfig *config =
            &sNdsRendererAdapterNativeOwnerWorkspace.production_configs[i];
        const NDSRendererNativeFighterPreamble *preamble =
            &sNdsFighterDisplayReplayPreambles[plan->collection.indices[i]];

        /* The fields RefreshNativePacketInputs writes, and only those (the
         * primed root->config already points at this config). */
        config->initial_geometry_mode = preamble->geometry_mode;
        config->color_modulate = color_modulate;
        input->preamble = preamble;
        input->modelview_matrix = &sNdsFtrLeanWorlds[i];
        input->projection_matrix = &sNdsFtrLeanProjection;
        input->gx_valid = 0u;
    }
    /* The global state the old path's draw leaves behind: the owner's root
     * program (Link's texgen tables are selected from it) and the Kirby trio
     * head key it publishes for every fighter. */
    ndsRendererNativeFighterSetRootProgram(owner_slot, inst->program);
#if NDS_P2_KIRBY
    ndsRendererNativeKirbyTrioSetHeadKey(inst->head_key);
#endif
    /* pre_same: this draw's preambles are the same memo fill the last patch
     * read, so their prim and light words cannot have moved. */
    reason = ndsFtrLeanPacketPatch(
        slot, sNdsRendererAdapterNativeOwnerWorkspace.production_roots,
        inst->root_count, owner_slot, use_low_detail,
        ((sNdsFtrLeanMemoFromSlot != 0u) &&
         (inst->patch_serial == sNdsFtrLeanMemoFill[slot] + 1u)) ?
            TRUE : FALSE);
    if (reason != 0u)
    {
        ndsFtrLeanInvalidate(slot);
        ndsFtrLeanDecline(kind, reason);
        return FALSE;
    }
    inst->patch_serial = (sNdsFtrLeanMemoFromSlot != 0u) ?
        (sNdsFtrLeanMemoFill[slot] + 1u) : 0u;
    t1 = cpuGetTiming();
    NDS_FTR_LEAN_CTR(gNdsFtrLean.patch_ticks += t1 - t0);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.k_patch_ticks[kind] += t1 - t0);
    if (route != NDS_FTR_LEAN_ROUTE_DRAW)
    {
        ndsFtrLeanShadowArm(slot, 1u);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.shadow_runs++);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.k_draws[kind]++);
        return FALSE;
    }

    /* Route 1: submit, then the bookkeeping DrawForSlot does after a
     * successful native draw (RAF tail), so every verifier count agrees. */
    gNdsFtrDeclineStage = 0u;
    gNdsFtrDeclineDisplayListClause = 0u;
    gNdsFtrRootProgramsTried = 0u;
#if NDS_TICK_HUD
    ndsFtrPreWalkCensus(slot, &plan->collection);
    NDS_TICK_HUD_NATIVE_OWNER_MARK(nNDSTickHudNativeOwnerFallbackCalls);
    NDS_TICK_HUD_NATIVE_OWNER_MARK(nNDSTickHudNativeOwnerFallbackEligible);
#endif
    /* The submit accounts into exactly these five fields and the draw reads
     * back one of them; the stats block is this path's own (1,300 B), so
     * zeroing the rest every draw (ndsRendererInitStats) bought nothing. */
    sNdsFtrLeanStats.first_opcode = 0u;
    sNdsFtrLeanStats.triangle_count = 0u;
    sNdsFtrLeanStats.hardware_triangle_count = 0u;
    sNdsFtrLeanStats.hardware_vertex_count = 0u;
    sNdsFtrLeanStats.hardware_zbuffer_triangle_count = 0u;
    ndsRendererProfileSetOwner(ndsFighterNativeOwnerProfileId(owner_slot));
    t0 = cpuGetTiming();
    NDS_FTR_LEAN_CTR(gNdsFtrLean.book_ticks += t0 - t1);
    ndsFtrLeanPacketSubmit(slot, &sNdsFtrLeanStats);
    t1 = cpuGetTiming();
    NDS_FTR_LEAN_CTR(gNdsFtrLean.submit_ticks += t1 - t0);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.k_submit_ticks[kind] += t1 - t0);
    ndsRendererProfileSetOwner(NDS_RENDERER_PROFILE_OWNER_NONE);
    if (slot == 0u)
    {
        gNdsFighterDLAllDrawP0HardwareTriangleCount +=
            sNdsFtrLeanStats.hardware_triangle_count;
        gNdsFighterDLAllDrawP0StatusAfter = (u32)fp->status_id;
        gNdsFighterDLAllDrawP0MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLAllDrawP0GAAfter = (u32)fp->ga;
    }
    else if (slot == 1u)
    {
        gNdsFighterDLAllDrawP1HardwareTriangleCount +=
            sNdsFtrLeanStats.hardware_triangle_count;
        gNdsFighterDLAllDrawP1StatusAfter = (u32)fp->status_id;
        gNdsFighterDLAllDrawP1MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLAllDrawP1GAAfter = (u32)fp->ga;
    }
    if (sNdsFtrLeanStats.hardware_triangle_count != 0u)
    {
        gNdsFighterDLAllDrawSlotTriangleMask |= 1u << (slot & 3u);
    }
    gNdsFighterMarioFoxDLAllDrawCount++;
    NDS_FTR_LEAN_CTR(gNdsFtrLean.book_ticks += cpuGetTiming() - t1);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.draws++);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.k_draws[kind]++);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.k_program_draws[kind][
        (inst->program < 15u) ? inst->program : 15u]++);
    if (use_low_detail == FALSE)
    {
        NDS_FTR_LEAN_CTR(gNdsFtrLean.k_high_draws[kind]++);
    }
    (void)t0;
    (void)t1;
    (void)mark;
    return TRUE;
}

/* The watch list: every MObj of the drawn roots whose material animation is
 * live, with its animation hash now. More than the list holds proves the whole
 * identity on every draw instead. */
static void ndsFtrLeanBuildWatch(NDSFtrLeanInstance *inst,
                                 const NDSFighterDrawPlanData *plan,
                                 u32 count)
{
    u32 n = 0u;
    u32 i;

    for (i = 0u; i < count; i++)
    {
        const DObj *dobj = plan->material_dobjs[i];
        const MObj *mobj;

        for (mobj = (dobj != NULL) ? dobj->mobj : NULL; mobj != NULL;
             mobj = mobj->next)
        {
            if (mobj->anim_wait == AOBJ_ANIM_NULL)
            {
                continue;
            }
            if (n >= NDS_FTR_LEAN_WATCH_MAX)
            {
                inst->watch_count = NDS_FTR_LEAN_WATCH_ALL;
                return;
            }
            inst->watch[n] = mobj;
            inst->watch_hash[n] = ndsRendererAdapterMaterialAnimHash(mobj);
            n++;
        }
    }
    inst->watch_count = (u8)n;
}

/* After the old path drew this fighter: retire an unconsumed oracle arm and
 * adopt the packet the old path just used, when it describes this draw's
 * tuple exactly (the plan of this status generation hit, and the packet was
 * replayed or recorded by this very draw). `hits_before` is the replay-hit
 * count before the old path ran: a draw that was not a replay hit ran the
 * material preparation, which may have written this fighter's MObjs. */
static void ndsFtrLeanAfterOldPath(u32 slot, FTStruct *fp, u32 serial_before,
                                   u32 hits_before)
{
    NDSFtrLeanInstance *inst = &sNdsFtrLeanInstances[slot];
    const NDSFighterDrawPlanData *plan;
    NDSFtrLeanAdoptInfo info;
    u32 owner_slot = 0u;
    u32 kind;
    u32 use_low_detail;
    u32 result;
    u32 count;
    u32 i;
    u32 t0;
    u32 t1;
#if NDS_P2_KIRBY
    u32 head = 0u;
#endif

    if (ndsFtrLeanShadowArmed(slot) != 0u)
    {
        ndsFtrLeanShadowArm(slot, 0u);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.oracle_unconsumed++);
    }
    if (gNdsFighterPacketHits == hits_before)
    {
        inst->reprove = 1u;
    }
    kind = ndsFtrLeanEligible(fp, &owner_slot);
    if ((kind == NDS_FTR_LEAN_KIND_NONE) || (fp->colanim.skeleton_id != 0))
    {
        return;
    }
    use_low_detail = (fp->detail_curr == nFTPartsDetailLow) ? TRUE : FALSE;
    if ((inst->valid != 0u) && (inst->kind == kind) &&
        (inst->detail == use_low_detail) &&
        (inst->status_gen == sNdsFighterStatusGeneration[slot]) &&
        (inst->heap_gen == gNdsTaskmanHeapGeneration))
    {
        u32 ident;

        if (inst->learn_pending == 0u)
        {
            return;   /* the list for this tuple is already adopted */
        }
        /* The lean draw declined a material move to a state the list has
         * not learned; the old path just drew it. Learn it as a variant
         * (the list's second entry), or adopt the recording outright. */
        inst->learn_pending = 0u;
        t0 = cpuGetTiming();
        ident = ndsRendererAdapterMaterialIdentity(
            sNdsFighterDrawPlan[slot].data.material_dobjs, inst->root_count);
        if ((ndsFighterDrawPlanHit(slot, use_low_detail) != FALSE) &&
            (ndsFtrLeanPacketUseSerial(slot) != serial_before) &&
            (ndsFtrLeanPacketLearnVariant(slot, ident) != FALSE))
        {
            ndsFtrLeanRebaseIdentity(slot, inst,
                                     &sNdsFighterDrawPlan[slot].data, ident);
            inst->reprove = 0u;
            NDS_FTR_LEAN_CTR(gNdsFtrLean.variant_learn_ticks +=
                                 cpuGetTiming() - t0);
            return;
        }
        NDS_FTR_LEAN_CTR(gNdsFtrLean.variant_learn_ticks +=
                             cpuGetTiming() - t0);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.variant_learn_fail++);
        ndsFtrLeanInvalidate(slot);
    }
    if (ndsFtrLeanPacketUseSerial(slot) == serial_before)
    {
        return;   /* the old path did not replay or record a packet */
    }
    if (ndsFighterDrawPlanHit(slot, use_low_detail) == FALSE)
    {
        /* No baked plan: a mixed-file program (Kirby CopyLink, Link's
         * boomerang donor) re-resolves every draw and is never baked. */
        ndsFtrLeanRefuse(inst, kind, nNDSFtrLeanAdoptPlan);
        return;
    }
    plan = &sNdsFighterDrawPlan[slot].data;
    count = plan->collection.selected_count;
    if ((count == 0u) || (count > NDS_FTR_LEAN_ROOT_MAX))
    {
        ndsFtrLeanRefuse(inst, kind, nNDSFtrLeanAdoptRoots);
        return;
    }
    t0 = cpuGetTiming();
    ndsFtrLeanInvalidate(slot);
    if (ndsFtrLeanBuildJoints(inst, fp->joints[nFTPartsJointTopN],
                              plan->matrix_bindings, count) == FALSE)
    {
        ndsFtrLeanRefuse(inst, kind, nNDSFtrLeanAdoptTopology);
        return;
    }
    ndsFtrLeanPacketNoteKind(slot, kind);
    /* The MObj state the copied words were recorded (or last replayed) with
     * is the state now: a record frame's material preparation ran before its
     * words were taken, so the identity to hold is today's, not key[0]
     * (which the record path takes before the preparation). */
    inst->ident_adopt = ndsRendererAdapterMaterialIdentity(
        plan->material_dobjs, count);
    t1 = cpuGetTiming();
    result = ndsFtrLeanPacketAdopt(slot, count, inst->ident_adopt, &info);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.adopt_copy_ticks += cpuGetTiming() - t1);
    if (result != nNDSFtrLeanAdoptOk)
    {
        ndsFtrLeanRefuse(inst, kind, result);
        return;
    }
    inst->kind = (u8)kind;
    inst->detail = (u8)use_low_detail;
    inst->program = plan->root_program;
    inst->root_count = (u8)count;
    inst->status_gen = sNdsFighterStatusGeneration[slot];
    inst->heap_gen = gNdsTaskmanHeapGeneration;
    inst->root = fp->joints[nFTPartsJointTopN];
    inst->event_count = sNdsFighterDisplayContract.event_count;
    inst->head_key = 0u;
#if NDS_P2_KIRBY
    if (ndsFighterKirbyTrioHeadKey(fp, &head) != FALSE)
    {
        inst->head_key = head;
    }
#endif
    inst->texpart_serial = sNdsFtrLeanTexPartSerial[slot];
    inst->learn_pending = 0u;
    ndsFtrLeanBuildWatch(inst, plan, count);
    for (i = 0u; i < count; i++)
    {
        inst->root_dl[i] =
            sNdsFighterDisplayReplayEvents[plan->collection.indices[i]].dl;
    }
    /* Route 1: the recorder's own packet holds these very words, recorded
     * after this draw's material preparation, but keyed on the identity from
     * BEFORE it; route 0 re-records it on its next draw to learn that. The
     * oracle routes keep route 0's behaviour (their record-under-hit compare
     * proves the two recordings agree). */
    if (gNdsFtrLeanRoute == NDS_FTR_LEAN_ROUTE_DRAW)
    {
        ndsFtrLeanPacketRekeyIdentity(slot, inst->ident_adopt);
    }
    inst->reprove = 0u;
    inst->pre_hash = ndsFtrLeanPreambleHash(plan, count);
    inst->pre_serial = (sNdsFtrLeanMemoFromSlot != 0u) ?
        (sNdsFtrLeanMemoFill[slot] + 1u) : 0u;
    inst->patch_serial = 0u;
    inst->topo_hash = ndsFtrLeanTopologyHash(inst);
    inst->rebind = 0u;
    inst->last_refuse = 0u;
    inst->valid = 1u;
    NDS_FTR_LEAN_CTR(gNdsFtrLean.adopt_watch = inst->watch_count);
    ndsFtrLeanPacketNoteKind(slot, kind);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.adopts++);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.k_adopts[kind]++);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.adopt_needs_fence = info.needs_fence);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.adopt_words = info.word_count);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.adopt_roots = info.root_count);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.adopt_textures = info.texture_count);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.adopt_ticks += cpuGetTiming() - t0);
    (void)t0;
    (void)t1;
}

/* Slice 2b: the lab pokes gNdsFtrLeanAdmit after setup, so the admission of
 * an already-made battle runs at the first frame end that sees the word. Once
 * the admission has run, this frame end is past the battle's setup uploads:
 * lock A+B so D alone allocates from here on (word 2; idempotent). */
static void ndsFtrLeanAdmitFrame(void)
{
    ndsFtrLeanAdmitSync();
    ndsFtrLeanAdmitMaybeRun(gNdsFtrLeanAdmit);
    if ((sNdsFtrAdmitDone != 0u) && (sNdsFtrAdmitDone == sNdsFtrAdmitGen))
    {
        ndsFtrLeanAdmitLockRegions();
    }
}

/* End of the fighter submit loop, once per presented frame: per-match reset,
 * the admission at the first frame after the poke, the GO latch, the GE busy
 * sample, and the counter publish. */
static void ndsFtrLeanFrameEnd(void)
{
    if (sNdsFtrLeanMatchHeapGen != gNdsTaskmanHeapGeneration)
    {
        u32 slot;

        sNdsFtrLeanMatchHeapGen = gNdsTaskmanHeapGeneration;
        for (slot = 0u; slot < GMCOMMON_PLAYERS_MAX; slot++)
        {
            ndsFtrLeanInvalidate(slot);
            sNdsFtrLeanInstances[slot].last_refuse = 0u;
        }
        NDS_FTR_LEAN_CTR(gNdsFtrLean.go_frame = 0u);
    }
    ndsFtrLeanAdmitFrame();
#if NDS_FTR_LEAN_LAB
    if ((gNdsFtrLean.go_frame == 0u) && (gSCManagerBattleState != NULL) &&
        (gSCManagerBattleState->game_status == nSCBattleGameStatusGo))
    {
        ndsFtrLeanNoteGo(gNdsRendererProfileFrameCount);
    }
    NDS_FTR_LEAN_CTR(gNdsFtrLean.ge_busy_samples++);
    if (((*(volatile u32 *)0x04000600u) & (1u << 27)) != 0u)
    {
        NDS_FTR_LEAN_CTR(gNdsFtrLean.ge_busy_hits++);
    }
    ndsFtrLeanCountersPublish();
#endif
#if NDS_VRAM_CENSUS_LIVE && NDS_RENDERER_HW_TRIANGLES && \
    (NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE)
    ndsVramCensusFrame();   /* slice 2a lab census; no-op unless enabled */
#endif
}

#else

void ndsFtrLeanNoteRebind(u32 player_slot)
{
    (void)player_slot;
}

#endif
