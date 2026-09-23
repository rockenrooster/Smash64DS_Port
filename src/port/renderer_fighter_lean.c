/* P2-2p8 Phase 1 slice 1 -- the lean fighter path, adapter side.
 *
 * #included at the end of renderer_adapter_fighter.c so it shares that
 * translation unit (reloc_backend_renderer_dl.c) with the draw plan, the
 * display contract and the matrix helpers it reads. Not in CFILES.
 *
 * Per Samus draw (LOW, program 0, no locks) once a list is adopted:
 *   head (unchanged, already ran) -> tuple compare -> guards for the inputs
 *   slice 1 cannot patch yet -> ndsFtrLeanKernelCompose -> the replay's patch
 *   block on the lean copy -> the replay's submit tail on the lean copy.
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
    defined(NDS_TICK_HUD) && NDS_TICK_HUD
/* P2-2p8 Phase 1 slice 2c (lab builds only, Makefile
 * NDS_FTR_LEAN_ADMIT_DEFAULT): boot with the word set, so the admission runs
 * at the end of the battle scene's texture preparation -- the shipping path --
 * instead of at the first frame the sampler pokes. */
static u32 sNdsFtrAdmitDefaultApplied;
#endif

void ndsFtrLeanAdmitNoteFighter(u32 player, u32 fkind, u32 costume,
                                u32 detail)
{
#if defined(NDS_FTR_LEAN_ADMIT_DEFAULT) && NDS_FTR_LEAN_ADMIT_DEFAULT && \
    defined(NDS_TICK_HUD) && NDS_TICK_HUD
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

#if NDS_FTR_LEAN_LIVE

typedef struct NDSFtrLeanInstance
{
    u32 valid;
    u32 slot;
    u32 status_gen;
    u32 heap_gen;
    u32 rebind;
    DObj *root;
    u32 root_count;
    u32 joint_count;
    u32 key0;
    NDSFtrLeanJoint joints[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    DObj *expect_child[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    DObj *expect_sib[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    DObj *expect_parent[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    u8 root_event[NDS_FTR_LEAN_ROOT_MAX];
    DObj *material_dobjs[NDS_FTR_LEAN_ROOT_MAX];
    NDSRendererNativeFighterPreamble pre[NDS_FTR_LEAN_ROOT_MAX];
} NDSFtrLeanInstance;

static NDSFtrLeanInstance sNdsFtrLeanInstance;
static NDSRendererMatrix20p12
    sNdsFtrLeanWorlds[NDS_FTR_LEAN_ROOT_MAX];
static NDSRendererNativeFighterRoot
    sNdsFtrLeanInputs[NDS_FTR_LEAN_ROOT_MAX];
static NDSRendererMatrix20p12 sNdsFtrLeanProjection;
static NDSRendererConfig sNdsFtrLeanConfig;
static u32 sNdsFtrLeanMatchHeapGen;

static void ndsFtrLeanDecline(u32 reason)
{
#if NDS_FTR_LEAN_LAB
    if (reason < nNDSFtrLeanDeclineCount)
    {
        NDS_FTR_LEAN_CTR(gNdsFtrLean.decline[reason]++);
    }
#else
    (void)reason;
#endif
}

static void ndsFtrLeanInvalidate(NDSFtrLeanInstance *inst)
{
    if (inst->valid != 0u)
    {
        ndsFtrLeanPacketDrop(inst->slot);
    }
    inst->valid = 0u;
    inst->rebind = 0u;
}

void ndsFtrLeanNoteRebind(u32 player_slot)
{
    if ((sNdsFtrLeanInstance.valid != 0u) &&
        (sNdsFtrLeanInstance.slot == player_slot))
    {
        sNdsFtrLeanInstance.rebind = 1u;
    }
}

/* Every joint the fast path does not take: the adapter's own source builder,
 * then the same N64-cell decode ComposeOwnerWorldsSource performs. */
static s32 ndsFtrLeanSlowLocal(DObj *dobj, s32 *cells, u32 *has_local)
{
    Vec3f accum = { 1.0F, 1.0F, 1.0F };
    Vec3f accum_out;
    Mtx mtx;
    sb32 local = FALSE;
    u32 row;
    u32 col;

    if (ndsRendererAdapterBuildSourceFighterLocalMtx(
            dobj, &accum, &accum_out, &mtx, &local) == FALSE)
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

/* The live topology ComposeOwnerWorldsSource collects every frame, captured
 * once per tuple, with every link it checks recorded so the per-frame guard
 * can prove it unchanged. */
static s32 ndsFtrLeanBuildTopology(NDSFtrLeanInstance *inst, DObj *root,
                                   DObj *const *bindings, u32 binding_count)
{
    DObj *joints[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    u8 parents[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    u32 count = 0u;
    u32 j;
    u32 b;

    if ((root == NULL) || (root == DOBJ_PARENT_NULL) ||
        (binding_count > NDS_FTR_LEAN_ROOT_MAX))
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
        inst->joints[j].dobj = joints[j];
        inst->joints[j].parent = parents[j];
        inst->joints[j].binding = 0xffu;
        inst->expect_child[j] = joints[j]->child;
        inst->expect_sib[j] = joints[j]->sib_next;
        inst->expect_parent[j] = want_parent;
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
        if ((j >= count) || (inst->joints[j].binding != 0xffu))
        {
            return FALSE;
        }
        inst->joints[j].binding = (u8)b;
    }
    inst->joint_count = count;
    return TRUE;
}

static s32 ndsFtrLeanTopologyUnchanged(const NDSFtrLeanInstance *inst)
{
    u32 j;

    for (j = 0u; j < inst->joint_count; j++)
    {
        const DObj *dobj = inst->joints[j].dobj;

        if ((dobj->child != inst->expect_child[j]) ||
            (dobj->sib_next != inst->expect_sib[j]) ||
            (dobj->parent != inst->expect_parent[j]))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/* The slice's admission: Samus, LOW, program 0, native production mode. */
static s32 ndsFtrLeanEligible(FTStruct *fp, u32 *owner_slot)
{
    if ((ndsFighterGetNativeOwnerSlot(fp, owner_slot) == FALSE) ||
        (*owner_slot != NDS_RENDERER_NATIVE_FIGHTER_OWNER_SAMUS) ||
        (fp->detail_curr != nFTPartsDetailLow) ||
        ((gNdsRendererFastRunMode !=
          NDS_RENDERER_FAST_RUN_NATIVE_FIGHTER_OWNER_PRODUCTION) &&
         (gNdsRendererFastRunMode !=
          NDS_RENDERER_FAST_RUN_NATIVE_COMPLETE_STAGE)))
    {
        return FALSE;
    }
    return TRUE;
}

/* Returns TRUE when route 1 drew the fighter (the old path must not run). */
static sb32 ndsFtrLeanRun(u32 slot, FTStruct *fp, u32 route)
{
    NDSFtrLeanInstance *inst = &sNdsFtrLeanInstance;
    const NDSFighterDrawPlanData *plan;
    u32 owner_slot = 0u;
    u32 guard;
    u32 identity;
    u32 t0;
    u32 t1;
    s32 shuffle_x = 0;
    s32 shuffle_y = 0;
    NDSRendererMatrix20p12 camera_projection;
    NDSRendererMatrix20p12 camera_modelview;
    u32 camera_projection_valid = FALSE;
    u32 camera_modelview_valid = FALSE;
    u32 i;

    if (ndsFtrLeanEligible(fp, &owner_slot) == FALSE)
    {
        return FALSE;   /* not in the slice: not an attempt */
    }
    NDS_FTR_LEAN_CTR(gNdsFtrLean.attempts++);
    t0 = cpuGetTiming();
    if ((inst->valid != 0u) && (inst->slot != slot))
    {
        ndsFtrLeanDecline(nNDSFtrLeanDeclineKind);   /* a second Samus */
        return FALSE;
    }
    if (fp->is_use_animlocks != FALSE)
    {
        ndsFtrLeanDecline(nNDSFtrLeanDeclineAnimLock);
        return FALSE;
    }
    if (fp->colanim.skeleton_id != 0)
    {
        ndsFtrLeanDecline(nNDSFtrLeanDeclineSkeleton);
        return FALSE;
    }
#if !NDS_R2_FIGHTER_SHUFFLE_FOLD
    if (fp->shuffle_tics != 0u)
    {
        ndsFtrLeanDecline(nNDSFtrLeanDeclineKind);
        return FALSE;
    }
#endif
    if (inst->valid == 0u)
    {
        ndsFtrLeanDecline(nNDSFtrLeanDeclineAdoptPending);
        return FALSE;
    }
    if (inst->rebind != 0u)
    {
        ndsFtrLeanInvalidate(inst);
        ndsFtrLeanDecline(nNDSFtrLeanDeclineRebind);
        return FALSE;
    }
    if ((inst->status_gen != sNdsFighterStatusGeneration[slot]) ||
        (inst->heap_gen != gNdsTaskmanHeapGeneration) ||
        (inst->root != fp->joints[nFTPartsJointTopN]) ||
        (ndsFighterDrawPlanHit(slot, TRUE) == FALSE))
    {
        ndsFtrLeanInvalidate(inst);
        ndsFtrLeanDecline(nNDSFtrLeanDeclineTuple);
        return FALSE;
    }
    plan = &sNdsFighterDrawPlan[slot].data;
    if ((plan->root_program != 0u) ||
        (plan->collection.selected_count != inst->root_count))
    {
        ndsFtrLeanInvalidate(inst);
        ndsFtrLeanDecline(nNDSFtrLeanDeclineTuple);
        return FALSE;
    }
    if (ndsFtrLeanTopologyUnchanged(inst) == FALSE)
    {
        ndsFtrLeanInvalidate(inst);
        ndsFtrLeanDecline(nNDSFtrLeanDeclineTopology);
        return FALSE;
    }
    ndsRendererAdapterGetFrameCameraMatrices(
        (gGCCurrentCamera != NULL) ? CObjGetStruct(gGCCurrentCamera) : NULL,
        &camera_projection, &camera_projection_valid,
        &camera_modelview, &camera_modelview_valid, NULL, NULL, NULL);
    if ((camera_projection_valid == FALSE) ||
        (camera_modelview_valid != FALSE))
    {
        ndsFtrLeanDecline(nNDSFtrLeanDeclineCamera);
        return FALSE;
    }
    /* key[0]: the live MObj chains' identity (texture ids, matanim words). */
    identity = ndsRendererAdapterMaterialIdentity(
        inst->material_dobjs, inst->root_count);
    if (identity != inst->key0)
    {
        ndsFtrLeanInvalidate(inst);
        ndsFtrLeanDecline(nNDSFtrLeanDeclineMaterial);
        return FALSE;
    }
    /* key[3]'s per-root preamble inputs (prim is patched, not keyed). */
    for (i = 0u; i < inst->root_count; i++)
    {
        const NDSRendererNativeFighterPreamble *pre =
            &sNdsFighterDisplayReplayPreambles[inst->root_event[i]];
        const NDSRendererNativeFighterPreamble *snap = &inst->pre[i];

        if ((pre->geometry_mode != snap->geometry_mode) ||
            (pre->cycle_type != snap->cycle_type) ||
            (pre->render_mode != snap->render_mode) ||
            (pre->env_color != snap->env_color) ||
            (pre->flags != snap->flags))
        {
            ndsFtrLeanInvalidate(inst);
            ndsFtrLeanDecline(nNDSFtrLeanDeclinePreamble);
            return FALSE;
        }
    }
    guard = ndsFtrLeanPacketGuard(slot,
        (route == NDS_FTR_LEAN_ROUTE_DRAW) ? 1u : 0u);
    if (guard != 0u)
    {
        ndsFtrLeanInvalidate(inst);
        ndsFtrLeanDecline(guard);
        return FALSE;
    }
#if NDS_R2_FIGHTER_SHUFFLE_FOLD
    /* The old path latches this every draw; so does the lean one, so the
     * statics read after this draw are the same in both routes. */
    ndsRendererAdapterSetShuffleOffset(fp);
    shuffle_x = sNdsR2ShuffleWorldX;
    shuffle_y = sNdsR2ShuffleWorldY;
#endif
    t1 = cpuGetTiming();
    NDS_FTR_LEAN_CTR(gNdsFtrLean.guard_ticks += t1 - t0);
    if (ndsFtrLeanKernelCompose(inst->joints, inst->joint_count,
                                sNdsFtrLeanWorlds, inst->root_count,
                                shuffle_x, shuffle_y,
                                ndsFtrLeanSlowLocal) == FALSE)
    {
        NDS_FTR_LEAN_CTR(gNdsFtrLean.kernel_fail++);
        ndsFtrLeanDecline(nNDSFtrLeanDeclineKernel);
        return FALSE;
    }
    t0 = cpuGetTiming();
    NDS_FTR_LEAN_CTR(gNdsFtrLean.kernel_ticks += t0 - t1);

    sNdsFtrLeanProjection = camera_projection;
    sNdsFtrLeanConfig.color_modulate =
        ndsRendererAdapterFighterColorModulate(fp);
    for (i = 0u; i < inst->root_count; i++)
    {
        NDSRendererNativeFighterRoot *input = &sNdsFtrLeanInputs[i];

        input->preamble =
            &sNdsFighterDisplayReplayPreambles[inst->root_event[i]];
        input->config = &sNdsFtrLeanConfig;
        input->modelview_matrix = &sNdsFtrLeanWorlds[i];
        input->projection_matrix = &sNdsFtrLeanProjection;
        input->gx_valid = 0u;
    }
    if (ndsFtrLeanPacketPatch(slot, sNdsFtrLeanInputs,
                              inst->root_count) == FALSE)
    {
        ndsFtrLeanInvalidate(inst);
        ndsFtrLeanDecline(nNDSFtrLeanDeclineTint);
        return FALSE;
    }
    t1 = cpuGetTiming();
    NDS_FTR_LEAN_CTR(gNdsFtrLean.patch_ticks += t1 - t0);
    if (route != NDS_FTR_LEAN_ROUTE_DRAW)
    {
        ndsFtrLeanShadowArm(slot, 1u);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.shadow_runs++);
        return FALSE;
    }

    /* Route 1: submit, then the bookkeeping DrawForSlot does after a
     * successful native draw (RAF tail), so every verifier count agrees. */
    ndsRendererNativeFighterSetRootProgram(owner_slot, 0u);
#if NDS_P2_KIRBY
    ndsRendererNativeKirbyTrioSetHeadKey(0u);
#endif
    gNdsFtrDeclineStage = 0u;
    gNdsFtrDeclineDisplayListClause = 0u;
#if NDS_TICK_HUD
    ndsFtrPreWalkCensus(slot, &plan->collection);
    NDS_TICK_HUD_NATIVE_OWNER_MARK(nNDSTickHudNativeOwnerFallbackCalls);
    NDS_TICK_HUD_NATIVE_OWNER_MARK(nNDSTickHudNativeOwnerFallbackEligible);
#endif
    ndsRendererInitStats(&sNdsFtrLeanStats);
    ndsRendererProfileSetOwner(ndsFighterNativeOwnerProfileId(owner_slot));
    ndsFtrLeanPacketSubmit(slot, &sNdsFtrLeanStats);
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
    NDS_FTR_LEAN_CTR(gNdsFtrLean.submit_ticks += cpuGetTiming() - t1);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.draws++);
    (void)t0;
    (void)t1;
    return TRUE;
}

/* After the old path drew this fighter: retire an unconsumed oracle arm and
 * adopt the packet the old path just used, when it describes the slice's
 * tuple exactly. */
static void ndsFtrLeanAfterOldPath(u32 slot, FTStruct *fp, u32 serial_before)
{
    NDSFtrLeanInstance *inst = &sNdsFtrLeanInstance;
    const NDSFighterDrawPlanData *plan;
    NDSFtrLeanAdoptInfo info;
    u32 owner_slot = 0u;
    u32 result;
    u32 i;

    if (ndsFtrLeanShadowArmed(slot) != 0u)
    {
        ndsFtrLeanShadowArm(slot, 0u);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.oracle_unconsumed++);
    }
    if ((ndsFtrLeanEligible(fp, &owner_slot) == FALSE) ||
        (fp->is_use_animlocks != FALSE) ||
        (fp->colanim.skeleton_id != 0) ||
        ((inst->valid != 0u) &&
         ((inst->slot != slot) ||
          ((inst->status_gen == sNdsFighterStatusGeneration[slot]) &&
           (inst->heap_gen == gNdsTaskmanHeapGeneration)))))
    {
        return;
    }
    if (ndsFtrLeanPacketUseSerial(slot) == serial_before)
    {
        return;   /* the old path did not replay or record a packet */
    }
    if (ndsFighterDrawPlanHit(slot, TRUE) == FALSE)
    {
        return;
    }
    plan = &sNdsFighterDrawPlan[slot].data;
    if ((plan->root_program != 0u) ||
        (plan->collection.selected_count == 0u) ||
        (plan->collection.selected_count > NDS_FTR_LEAN_ROOT_MAX))
    {
        return;
    }
    ndsFtrLeanInvalidate(inst);
    if (ndsFtrLeanBuildTopology(inst, fp->joints[nFTPartsJointTopN],
                                plan->matrix_bindings,
                                plan->collection.selected_count) == FALSE)
    {
        NDS_FTR_LEAN_CTR(gNdsFtrLean.adopt_refuse[nNDSFtrLeanAdoptTopology]++);
        return;
    }
    result = ndsFtrLeanPacketAdopt(slot, plan->collection.selected_count,
                                   &info);
    if (result != nNDSFtrLeanAdoptOk)
    {
        if (result < nNDSFtrLeanAdoptRefuseCount)
        {
            NDS_FTR_LEAN_CTR(gNdsFtrLean.adopt_refuse[result]++);
        }
        return;
    }
    inst->slot = slot;
    inst->status_gen = sNdsFighterStatusGeneration[slot];
    inst->heap_gen = gNdsTaskmanHeapGeneration;
    inst->root = fp->joints[nFTPartsJointTopN];
    inst->root_count = plan->collection.selected_count;
    inst->key0 = info.key[0];
    inst->rebind = 0u;
    for (i = 0u; i < inst->root_count; i++)
    {
        inst->root_event[i] = (u8)plan->collection.indices[i];
        inst->material_dobjs[i] = plan->material_dobjs[i];
        inst->pre[i] = sNdsFighterDisplayReplayPreambles[
            plan->collection.indices[i]];
    }
    inst->valid = 1u;
    NDS_FTR_LEAN_CTR(gNdsFtrLean.adopts++);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.adopt_needs_fence = info.needs_fence);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.adopt_words = info.word_count);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.adopt_roots = info.root_count);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.adopt_textures = info.texture_count);
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
        sNdsFtrLeanMatchHeapGen = gNdsTaskmanHeapGeneration;
        ndsFtrLeanInvalidate(&sNdsFtrLeanInstance);
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
