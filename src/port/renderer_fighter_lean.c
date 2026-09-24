/* P2-2p8 Phase 1 slices 1-4 -- the lean fighter path, adapter side.
 *
 * #included at the end of renderer_adapter_fighter.c so it shares that
 * translation unit (reloc_backend_renderer_dl.c) with the draw plan, the
 * display contract and the matrix helpers it reads. Not in CFILES.
 *
 * Slice 4: Donkey, Samus, Link and Kirby, every program and detail, one
 * instance per battle slot, drawn from lists MATERIALIZED from the generated
 * owner tables (nds_renderer_native_common.c, ndsFtrLeanMaterialize) -- the
 * old path is never asked to draw or record first. Per draw:
 *   head (unchanged, already ran) -> the proofs of the active entry (tuple,
 *   status re-tuple, material identity, preambles once per memo fill, the
 *   list's guard) -> on any moved input, the event path: the plan the old
 *   path would resolve, the entry key, and the entry that holds its list or a
 *   new materialization -> ndsFtrLeanKernelCompose (12-word LOAD4x3 straight
 *   into the list) -> the replay's patches (P', tint tiles, shade, light,
 *   Link's texgen) -> the replay's submit tail.
 * Routes 2/3 stop after the patch and arm the TryReplay oracle instead of
 * drawing. See include/nds/renderer_fighter_lean.h. */

#include <nds/renderer_fighter_lean.h>
#include <nds/generated/nds_fighter_admission.generated.h>
#include <nds/nds_reloc_assets.h>

/* Slice 2b BSS diet: the per-root arrays hold the most DL-bearing joints any
 * kind x detail has (generator max, 24) instead of the draw collection's 32;
 * a plan with more selected roots is never adopted. */
#define NDS_FTR_LEAN_ROOT_MAX NDS_FIGHTER_ADMISSION_ROOT_MAX
_Static_assert(NDS_FTR_LEAN_ROOT_MAX < 31u,
               "bit 31 of a modelview-sites mask is NDS_FTR_LEAN_SITES_INPUTS");

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
#if NDS_FTR_LEAN_ATTR_LIVE
NDSFtrLeanAttr gNdsFtrLeanAttr __attribute__((used, aligned(32)));
#endif
#if NDS_FTR_LEAN_ORACLE_ROUTES
volatile u32 gNdsFtrLeanOracleSourceOk __attribute__((used));
#endif
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
    /* Slice 5: the admission's renderer-state scratch lives on the stack for
     * the call (each record re-initializes it), not in a static. */
    NDSRendererStats scratch;
    u32 bases;

    if ((word == 0u) || (sNdsFtrAdmitCount == 0u) ||
        (sNdsFtrAdmitDone == sNdsFtrAdmitGen) ||
        (ndsFtrLeanAdmitBattleScene() == FALSE))
    {
        return;
    }
    sNdsFtrAdmitDone = sNdsFtrAdmitGen;
    bases = ndsFtrLeanAdmitCollectBases();
    (void)ndsFtrLeanAdmitRun(&scratch, sNdsFtrAdmitKind,
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

/* Material identity without hashing every MObj every draw. Within one status
 * generation a fighter MObj's animated words (colours, texture ids, lfrac,
 * palette) have exactly three writers: its material animation, which the
 * per-frame player runs only while anim_wait != AOBJ_ANIM_NULL and which only
 * MObj creation attaches (lbCommonAddMObjForFighterPartsDObj -- creation,
 * costume and model-part changes, all of which bump the status generation and
 * the rebind); the texture-part shims (counted per slot); and the old path's
 * own material preparation. So a draw proves: the texture-part count is
 * unchanged, each MObj with a live animation still hashes as when the entry
 * was selected, and -- after any old-path draw of this fighter, and on every
 * draw of the oracle routes -- the whole identity is still that one. */
#define NDS_FTR_LEAN_WATCH_MAX 6u
#define NDS_FTR_LEAN_WATCH_ALL 0xffu

/* One instance per battle slot (slice 4). The lists themselves live in the
 * slot's region (ndsFtrLeanMaterialize, two entries); the instance holds the
 * plan the active entry was selected for -- the drawn roots' display-contract
 * events, matrix and material DObjs -- and the kernel's joint table. */
typedef struct NDSFtrLeanInstance
{
    u8 valid;                   /* an entry is active for the fields below */
    u8 kind;                    /* NDS_FTR_LEAN_OWNER_KIND */
    u8 detail;                  /* use_low_detail */
    u8 program;                 /* the plan's root program */
    u8 root_count;
    u8 joint_count;
    u8 rebind;
    u8 reprove;                 /* the old path drew since the last proof */
    u8 rerecord;                /* the old path's packet was invalidated */
    u8 skeleton;                /* slice 6: colanim.skeleton_id the plan was
                                   resolved under (Mario/Fox program 0xFE) */
    u8 wide_high;               /* slice 6: this slot's high-detail lists
                                   outgrow an entry (Donkey, Link): go wide
                                   at once instead of walking twice */
    u8 pad0[1];
    u32 rr_key;                 /* its packet key's shadow at the last
                                   re-record (ndsFtrLeanRerecordKey) */
    u32 status_gen;
    u32 heap_gen;
    DObj *root;
    const NDSRelocLoadedFile *owner_file;   /* the file the plan resolved */
    const void *file_data;      /* ... and its identity when it did */
    u32 file_asset;
    u32 file_generation;
    u32 file_size;
    u32 head_key;               /* the Kirby trio head key the draw publishes */
    u32 ident_live;             /* material identity (MObj pointers too) */
    u32 texpart_serial;         /* texture-part writes seen at selection */
    u32 pre_hash;               /* the roots' key[3] preamble fields */
    u32 pre_serial;             /* memo fill serial + 1 they were proven at */
    u32 patch_serial;           /* memo fill serial + 1 of the last patch */
    u32 topo_hash;              /* the kept joints' links (oracle routes) */
    u8 watch_count;             /* animated MObjs, or WATCH_ALL */
    u8 pad[3];
    const MObj *watch[NDS_FTR_LEAN_WATCH_MAX];
    u32 watch_hash[NDS_FTR_LEAN_WATCH_MAX];
    u32 root_offsets[NDS_FTR_LEAN_ROOT_MAX];
    u8 material_counts[NDS_FTR_LEAN_ROOT_MAX];
    u8 event_index[NDS_FTR_LEAN_ROOT_MAX];  /* each root's contract event */
    const Gfx *root_dl[NDS_FTR_LEAN_ROOT_MAX];
    DObj *matrix_dobjs[NDS_FTR_LEAN_ROOT_MAX];
    DObj *material_dobjs[NDS_FTR_LEAN_ROOT_MAX];
    NDSFtrLeanJoint joints[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
} NDSFtrLeanInstance;

static NDSFtrLeanInstance sNdsFtrLeanInstances[GMCOMMON_PLAYERS_MAX];
static u32 sNdsFtrLeanMatchHeapGen;
static u32 sNdsFtrLeanCamFrame;     /* profile frame + 1 of the fetch */
static const CObj *sNdsFtrLeanCamCobj;
static u32 sNdsFtrLeanCamOk;

/* A lean draw owns no per-draw BSS. The kernel's Q43.20 worlds are a
 * per-depth stack in its own frame; the LOAD4x3 parameters go straight into
 * the list (ndsFtrLeanPacketModelviewSites); the rest is borrowed from the old
 * path, which a draw of either kind rewrites before it reads (the old path
 * never runs inside a lean draw, and routes 2/3 finish the lean patch before
 * the old path starts):
 *   binding worlds  sNdsRendererAdapterNativeOwnerModelviews (the old path's
 *                   own binding modelviews; written only for a texgen list)
 *   projection      sNdsRendererAdapterNativeOwnerProjection (same camera)
 *   root inputs     the workspace's production roots/configs and material
 *                   rows: an event writes them as the old path's full input
 *                   producer does (which rebuilds every one of them before its
 *                   own next read); a lean draw refreshes exactly the fields
 *                   ndsRendererAdapterRefreshNativePacketInputs refreshes */
_Static_assert(NDS_FTR_LEAN_ROOT_MAX <= NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED,
               "lean roots must fit the borrowed binding arrays");
#define sNdsFtrLeanWorlds sNdsRendererAdapterNativeOwnerModelviews
#define sNdsFtrLeanProjection sNdsRendererAdapterNativeOwnerProjection

#if NDS_FTR_LEAN_LAB
/* `row` is the caller's own counter row, the battle slot (the per-row split
 * exists only in the slice 5 attribution ROM). */
#define NDS_FTR_LEAN_GUARD_PART(index, mark)                               \
    do                                                                     \
    {                                                                      \
        u32 part_now_ = cpuGetTiming();                                    \
        gNdsFtrLean.guard_part_ticks[index] += part_now_ - (mark);         \
        NDS_FTR_LEAN_ATTR(if (row < NDS_FTR_LEAN_KINDS)                    \
            gNdsFtrLeanAttr.guard_part_ticks[row][index] +=                \
                part_now_ - (mark));                                       \
        (mark) = part_now_;                                                \
    } while (0)
#else
#define NDS_FTR_LEAN_GUARD_PART(index, mark) ((void)0)
#endif
#if NDS_FTR_LEAN_ATTR_LIVE
#define NDS_FTR_LEAN_EVENT_PART(index, mark)                               \
    do                                                                     \
    {                                                                      \
        u32 part_now_ = cpuGetTiming();                                    \
        gNdsFtrLeanAttr.event_part_ticks[index] += part_now_ - (mark);     \
        gNdsFtrLeanAttr.event_part_count[index]++;                         \
        (mark) = part_now_;                                                \
    } while (0)
#else
#define NDS_FTR_LEAN_EVENT_PART(index, mark) ((void)0)
#endif

/* `row`: the counter row, the battle slot (slice 6; NDS_FTR_LEAN_KINDS). */
static void ndsFtrLeanDecline(u32 row, u32 reason)
{
#if NDS_FTR_LEAN_LAB
    if (reason < nNDSFtrLeanDeclineCount)
    {
        gNdsFtrLean.decline[reason]++;
        if (row < NDS_FTR_LEAN_KINDS)
        {
            gNdsFtrLean.k_decline[row][reason]++;
        }
    }
#else
    (void)row;
    (void)reason;
#endif
}

static void ndsFtrLeanCountEvent(u32 row, u32 event)
{
#if NDS_FTR_LEAN_LAB
    if (event < nNDSFtrLeanEventCount)
    {
        gNdsFtrLean.event[event]++;
        if (row < NDS_FTR_LEAN_KINDS)
        {
            gNdsFtrLean.k_event[row][event]++;
        }
    }
#else
    (void)row;
    (void)event;
#endif
}

/* Forget the instance's plan; `lists` also drops the slot's materialized
 * entries (a new battle, a lost joint table). */
static void ndsFtrLeanInvalidate(u32 slot, u32 lists)
{
    NDSFtrLeanInstance *inst = &sNdsFtrLeanInstances[slot];

    if (lists != FALSE)
    {
        ndsFtrLeanPacketDrop(slot);
        inst->wide_high = 0u;
    }
    inst->valid = 0u;
    inst->rebind = 0u;
}

void ndsFtrLeanNoteRebind(u32 player_slot)
{
    if (player_slot < GMCOMMON_PLAYERS_MAX)
    {
        /* The old path's packet for this slot is gone: its next draw
         * re-records whatever the lean path's instance state. */
        sNdsFtrLeanInstances[player_slot].rerecord = 1u;
        if (sNdsFtrLeanInstances[player_slot].valid != 0u)
        {
            sNdsFtrLeanInstances[player_slot].rebind = 1u;
        }
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
 * own order; fixed for the DObj's life, so classified once per joint table. */
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
 * once per event with the same link checks, then cut to the joints a drawn
 * binding hangs from: a binding's world (lock accumulator included) depends
 * on its ancestor chain alone, so the rest are never composed. Preorder is
 * kept, so every parent still precedes its children. */
static s32 __attribute__((noinline, cold, optimize("Os")))
ndsFtrLeanBuildJoints(NDSFtrLeanInstance *inst, DObj *root,
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

/* The per-root fields the recorder's key[3] folds in -- everything a list's
 * words depend on in the roots' inputs that no patch covers (prim, the colour
 * modulate and the light direction are patched): the root offset, the
 * display preamble's geometry / cycle / render mode, env colour and flags,
 * the config's initial geometry mode (the preamble's) and the material
 * count. */
static u32 ndsFtrLeanPreambleHash(const u8 *event_index,
                                  const u32 *root_offsets,
                                  const u8 *material_counts, u32 count)
{
    u32 h = 2166136261u;
    u32 i;

    for (i = 0u; i < count; i++)
    {
        const NDSRendererNativeFighterPreamble *pre =
            &sNdsFighterDisplayReplayPreambles[event_index[i]];

        h = (h ^ root_offsets[i]) * 16777619u;
        h = (h ^ (u32)pre->geometry_mode) * 16777619u;
        h = (h ^ (u32)pre->cycle_type) * 16777619u;
        h = (h ^ (u32)pre->render_mode) * 16777619u;
        h = (h ^ (u32)pre->env_color) * 16777619u;
        h = (h ^ (u32)pre->flags) * 16777619u;
        h = (h ^ (u32)material_counts[i]) * 16777619u;
    }
    return h;
}

#if NDS_FTR_LEAN_LAB
/* Lab census: the last 16 distinct entry keys each slot selected, so a run
 * says how many events re-selected a state it had already drawn (what more
 * entries or variants could keep) against how many were new. */
#define NDS_FTR_LEAN_KEY_CENSUS 16u
static u32 sNdsFtrLeanKeySeen[GMCOMMON_PLAYERS_MAX][NDS_FTR_LEAN_KEY_CENSUS];
static u32 sNdsFtrLeanKeySeenCount[GMCOMMON_PLAYERS_MAX];

static void ndsFtrLeanKeyCensus(u32 slot, u32 row, const u32 *key)
{
    u32 h = 2166136261u;
    u32 i;

    for (i = 0u; i < NDS_FTR_LEAN_KEY_WORDS; i++)
    {
        h = (h ^ key[i]) * 16777619u;
    }
    gNdsFtrLean.key_events++;
    if (row < NDS_FTR_LEAN_KINDS)
    {
        gNdsFtrLean.k_key_events[row]++;
    }
    for (i = 0u; i < sNdsFtrLeanKeySeenCount[slot]; i++)
    {
        if (sNdsFtrLeanKeySeen[slot][i] == h)
        {
            gNdsFtrLean.key_seen_before++;
            if (row < NDS_FTR_LEAN_KINDS)
            {
                gNdsFtrLean.k_key_seen_before[row]++;
            }
            return;
        }
    }
    gNdsFtrLean.key_distinct++;
    if (sNdsFtrLeanKeySeenCount[slot] < NDS_FTR_LEAN_KEY_CENSUS)
    {
        sNdsFtrLeanKeySeen[slot][sNdsFtrLeanKeySeenCount[slot]++] = h;
    }
}
#endif

/* The watch list: every MObj of the drawn roots whose material animation is
 * live, with its animation hash now. More than the list holds proves the whole
 * identity on every draw instead. */
static void ndsFtrLeanBuildWatch(NDSFtrLeanInstance *inst, u32 count)
{
    u32 n = 0u;
    u32 i;

    for (i = 0u; i < count; i++)
    {
        const DObj *dobj = inst->material_dobjs[i];
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

/* A status change that leaves the drawn roots exactly as they were: every
 * root's display-contract event, DL, matrix and material DObj are the ones
 * the active entry was selected for, so the plan the event path would resolve
 * is this one and the entry stays. The identity and the preambles are then
 * re-proven in full on this draw (the status change may have moved them); the
 * kernel's link check covers any re-parenting. */
static s32 ndsFtrLeanRetuple(u32 slot, FTStruct *fp, NDSFtrLeanInstance *inst)
{
    NDSFighterDLAllDrawCollection live;
    u32 i;

    ndsFighterCollectAllDObjsWithDL(fp->joints[nFTPartsJointTopN], &live);
#if NDS_R2_FOX_GUN_OVERLAY
    ndsFighterCollectStripFoxGunSidecar(fp, &live);
#endif
    if (live.selected_count != inst->root_count)
    {
        return FALSE;
    }
    for (i = 0u; i < inst->root_count; i++)
    {
        const NDSFighterDisplayContractEvent *event;

        if (live.indices[i] != (u32)inst->event_index[i])
        {
            return FALSE;
        }
        event = &sNdsFighterDisplayReplayEvents[live.indices[i]];
        if ((event->dl != inst->root_dl[i]) ||
            (event->matrix_dobj != inst->matrix_dobjs[i]) ||
            (event->material_dobj != inst->material_dobjs[i]))
        {
            return FALSE;
        }
    }
    inst->status_gen = sNdsFighterStatusGeneration[slot];
    inst->reprove = 1u;
    inst->pre_serial = 0u;
    inst->patch_serial = 0u;
    /* A status change may add or drop DObjs that draw nothing (the drawn
     * collection above is unchanged), which moves the kept joints' child and
     * sibling links but no drawn world; the oracle routes' link hash restarts
     * from here. Re-parenting a kept joint is the kernel's to refuse. */
#if NDS_FTR_LEAN_ORACLE_ROUTES
    inst->topo_hash = ndsFtrLeanTopologyHash(inst);
#endif
    NDS_FTR_LEAN_CTR(gNdsFtrLean.retuples++);
    return TRUE;
}

/* Every VS kind (slice 6; slices 3-5: the four stress kinds), any detail,
 * native production mode, never the 1P intro's transient actors (they never
 * plan). */
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

/* The draw's per-frame production inputs: exactly the fields
 * ndsRendererAdapterRefreshNativePacketInputs refreshes (the primed
 * root->config already points at its config). */
static void ndsFtrLeanRefreshInputs(const NDSFtrLeanInstance *inst,
                                    u32 color_modulate)
{
    u32 i;

    if (sNdsRendererAdapterProductionInputsPrimed == FALSE)
    {
        ndsRendererAdapterPrimeProductionInputs(
            &sNdsRendererAdapterNativeOwnerWorkspace);
    }
    for (i = 0u; i < inst->root_count; i++)
    {
        NDSRendererNativeFighterRoot *input =
            &sNdsRendererAdapterNativeOwnerWorkspace.production_roots[i];
        NDSRendererConfig *config =
            &sNdsRendererAdapterNativeOwnerWorkspace.production_configs[i];
        const NDSRendererNativeFighterPreamble *preamble =
            &sNdsFighterDisplayReplayPreambles[inst->event_index[i]];

        config->initial_geometry_mode = preamble->geometry_mode;
        config->color_modulate = color_modulate;
        input->root_offset = inst->root_offsets[i];
        input->material_count = inst->material_counts[i];
        input->preamble = preamble;
        input->modelview_matrix = &sNdsFtrLeanWorlds[i];
        input->projection_matrix = &sNdsFtrLeanProjection;
        input->gx_valid = 0u;
        input->gx_locals = NULL;
        input->gx_seed = NULL;
        input->gx_local_count = 0u;
        input->gx_parent_slot = (u8)NDS_RENDERER_FIGHTER_GX_SLOT_NONE;
        input->gx_store_slot = (u8)NDS_RENDERER_FIGHTER_GX_SLOT_NONE;
        input->gx_seed_is_identity = 0u;
    }
}

/* Slice 5: the per-draw patch view (NDSFtrLeanPatchView) -- the draw's replay
 * preambles through the instance's event indices, the colour modulate, P's
 * source projection and the binding worlds; `inputs` only when the caller
 * refreshed the shared production inputs. */
static void ndsFtrLeanView(NDSFtrLeanPatchView *view,
                           const NDSFtrLeanInstance *inst, u32 color_modulate,
                           const NDSRendererNativeFighterRoot *inputs)
{
    view->preambles = sNdsFighterDisplayReplayPreambles;
    view->event_index = inst->event_index;
    view->root_count = inst->root_count;
    view->modulate = color_modulate;
    view->projection = &sNdsFtrLeanProjection;
    view->worlds = sNdsFtrLeanWorlds;
    view->inputs = inputs;
}

/* Materialize `key`'s list into `*entry` from the plan the event path just
 * resolved: the production inputs' remaining fields and the draw's initial
 * renderer state, seeded as the old path seeds its own
 * (ndsFighterMarioFoxDLAllDrawForSlot). Returns 0 or a decline reason.
 *
 * Slice 6: a list that outgrows an entry (Capacity) is materialized again
 * WIDE -- entry 0 over the slot's whole region, *entry set to 0 -- when
 * `allow_wide` (route 1's event path; never the lab verify, whose held list
 * the wide list would overwrite). A slot whose high-detail list went wide
 * once goes wide directly (wide_high). The walk mutates the renderer state,
 * so each walk starts from a fresh seed. */
static u32 __attribute__((noinline, cold, optimize("Os")))
ndsFtrLeanMaterializeFor(u32 slot, u32 *entry, const u32 *key, u32 count,
                         u32 owner_slot, u32 use_low_detail,
                         const NDSRelocLoadedFile *owner_file, u32 allow_wide)
{
    NDSRendererAdapterNativeOwnerWorkspace *ws =
        &sNdsRendererAdapterNativeOwnerWorkspace;
    /* Slice 5: the materializer's renderer state is this call's own (it was
     * a 1,300 B static); the old path's DrawForSlot keeps the same block on
     * its stack at the same depth. */
    NDSRendererStats stats;
    u32 reason;
    u32 wide;
    u32 t1;
    u32 i;

    for (i = 0u; i < count; i++)
    {
        NDSRendererNativeFighterRoot *input = &ws->production_roots[i];
        NDSRendererConfig *config = &ws->production_configs[i];

        /* ndsRendererAdapterBuildNativeProductionInputs' remaining fields;
         * texture data resolves through loaded-file pointers, so no DL
         * resolver is needed. */
        config->initial_projection = &sNdsFtrLeanProjection;
        config->initial_modelview = &sNdsFtrLeanWorlds[i];
        config->user = NULL;
        input->asset_base = (ws->loaded[i] != NULL) ?
            ws->loaded[i]->data : NULL;
        input->materials = sNdsRendererAdapterNativeOwnerMaterials[
            sNdsRendererAdapterNativeOwnerMaterialRows[i]];
        input->gx_modelview_mirror_valid = 0u;
    }
#if NDS_FTR_LEAN_ORACLE_ROUTES && !NDS_FTR_LEAN_ORACLE_WIDE
    if (gNdsFtrLeanRoute != NDS_FTR_LEAN_ROUTE_DRAW)
    {
        allow_wide = FALSE;         /* wide lists are route 1's */
    }
#endif
    wide = ((allow_wide != FALSE) && (use_low_detail == FALSE) &&
            (sNdsFtrLeanInstances[slot].wide_high != 0u)) ? TRUE : FALSE;
    ndsRendererProfileSetOwner(ndsFighterNativeOwnerProfileId(owner_slot));
    t1 = cpuGetTiming();
    for (;;)
    {
        /* The draw's initial renderer state, seeded exactly as the old path
         * seeds its own (ndsFighterMarioFoxDLAllDrawForSlot). */
        ndsRendererInitStats(&stats);
        if (sNdsFighterDisplayContractPlayback != FALSE)
        {
            stats.geometry_mode = sNdsFighterDisplayContract.geometry_mode;
            stats.prim_color = sNdsFighterDisplayContract.prim_color;
            stats.env_color = sNdsFighterDisplayContract.env_color;
            if (sNdsFighterDisplayContract.light_valid != 0u)
            {
                stats.light_dir_x = sNdsFighterDisplayContract.light.l.dir[0];
                stats.light_dir_y = sNdsFighterDisplayContract.light.l.dir[1];
                stats.light_dir_z = sNdsFighterDisplayContract.light.l.dir[2];
                stats.light_dir_mask = 1u;
            }
            ndsFighterDisplayContractSeedMaterialLights(&stats);
        }
        if (wide != FALSE)
        {
            *entry = 0u;
        }
        reason = ndsFtrLeanMaterialize(
            slot, (wide != FALSE) ? NDS_FTR_LEAN_ENTRY_WIDE : *entry, key,
            ws->production_roots, count, owner_slot, use_low_detail,
            owner_file->data, &stats);
        if ((wide != FALSE) || (allow_wide == FALSE) ||
            (reason != nNDSFtrLeanDeclineCapacity))
        {
            break;
        }
        wide = TRUE;                /* Capacity: again, wide */
    }
    if ((wide != FALSE) && (reason == 0u) && (use_low_detail == FALSE))
    {
        sNdsFtrLeanInstances[slot].wide_high = 1u;
    }
    NDS_FTR_LEAN_CTR(gNdsFtrLean.materialize_list_ticks +=
                         cpuGetTiming() - t1);
    ndsRendererProfileSetOwner(NDS_RENDERER_PROFILE_OWNER_NONE);
    (void)t1;
    return reason;
}

#if NDS_FTR_LEAN_LAB
/* Lab verify (slice 5, gNdsFtrLeanSlow bit 16): the plan a material event
 * keeps -- the instance's -- against the plan the resolve just returned for
 * the same draw. TRUE when both name the same roots (contract events, root
 * offsets, display lists, material counts), the same matrix and material
 * DObjs, the same file and the same program. */
static s32 __attribute__((noinline, cold, optimize("Os")))
ndsFtrLeanVerifyPlan(const NDSFtrLeanInstance *inst,
                     const NDSFighterDLAllDrawCollection *collection,
                     const NDSRendererAdapterNativeOwnerWorkspace *ws,
                     u32 count, u32 program,
                     const NDSRelocLoadedFile *owner_file)
{
    u32 bad[3] = { 0u, 0u, 0u };
    u32 i;

    if ((count != inst->root_count) || (program != inst->program) ||
        (owner_file != inst->owner_file))
    {
        bad[0] = 1u;
    }
    for (i = 0u; (bad[0] == 0u) && (i < count); i++)
    {
        const NDSFighterDisplayContractEvent *event =
            &sNdsFighterDisplayReplayEvents[collection->indices[i]];

        if (((u32)inst->event_index[i] != (u32)collection->indices[i]) ||
            (inst->root_offsets[i] != ws->root_offsets[i]) ||
            ((u32)inst->material_counts[i] != ws->material_counts[i]) ||
            (inst->root_dl[i] != event->dl))
        {
            bad[1] = 1u;
        }
        if ((inst->matrix_dobjs[i] != ws->matrix_bindings[i]) ||
            (inst->material_dobjs[i] != ws->material_dobjs[i]))
        {
            bad[2] = 1u;
        }
    }
    gNdsFtrLean.verify_plan_runs++;
    for (i = 0u; i < 3u; i++)
    {
        gNdsFtrLean.verify_plan_mismatch[i] += bad[i];
    }
    return ((bad[0] | bad[1] | bad[2]) == 0u) ? TRUE : FALSE;
}
#endif

/* The event path: the plan the old path would resolve for this draw's
 * display contract (the same resolver, program selection and validator),
 * the entry key from it, then the entry that holds that key's list or a new
 * list materialized from the templates -- no old-path draw, no recording.
 * Returns 0 or a decline reason.
 *
 * Slice 5, `reuse_plan`: a material event (the identity moved, and nothing
 * else did: the tuple, the status generation or a proven re-tuple, and no
 * rebind all held) keeps the instance's plan -- the same roots, display lists,
 * DObjs, file and program, which is what the resolve would return -- and
 * rebuilds only the material rows and the key. A held entry then takes the
 * draw; a key no entry holds, or a root whose MObj count moved, restarts as a
 * full event (the materializer needs the resolve's workspace). The slice 5
 * attribution priced a held-entry event at ~57K ticks, ~28K of it the
 * resolve, program + validate and joint-table steps this skips. The joint
 * table is kept as every event-free draw keeps it: the kernel proves each
 * kept joint's parent link per draw and rebuilds the table on a refusal.
 * The lab verify arm (gNdsFtrLeanSlow bit 16) resolves a kept plan anyway,
 * counts any difference (ndsFtrLeanVerifyPlan), then goes on exactly as the
 * kept plan would. */
static u32 __attribute__((noinline, cold, optimize("Os")))
ndsFtrLeanEvent(u32 slot, FTStruct *fp, NDSFtrLeanInstance *inst, u32 kind,
                u32 owner_slot, u32 use_low_detail, u32 color_modulate,
                u32 force_new, u32 reuse_plan)
{
    NDSRendererAdapterNativeOwnerWorkspace *ws =
        &sNdsRendererAdapterNativeOwnerWorkspace;
    NDSFighterDLAllDrawCollection collection;
    NDSRelocLoadedFile *owner_file = NULL;
    NDSFtrLeanPatchView view;
    u32 expected_asset_id = ndsFighterNativeOwnerModelAssetId(owner_slot);
    u32 key[NDS_FTR_LEAN_KEY_WORDS];
    u32 head_key = 0u;
    u32 tried = 0u;
    u32 program;
    u32 count;
    u32 entry;
    u32 code;
    u32 ident;
    u32 rr_key;
    u32 i;
    u32 row = slot & 3u;        /* counter row (NDS_FTR_LEAN_KINDS) */
    u32 t0 = cpuGetTiming();
#if NDS_FTR_LEAN_ATTR_LIVE
    u32 ev_mark = t0;
#endif
#if NDS_FTR_LEAN_LAB
    u32 reuse_check = ((reuse_plan != FALSE) &&
                       ((NDS_FTR_LEAN_SLOW_WORD() &
                         NDS_FTR_LEAN_SLOW_VERIFY) != 0u)) ? TRUE : FALSE;
#endif

restart:
#if NDS_P2_KIRBY
    {
        sb32 body = ((owner_slot == 11u) &&
                     (ndsFighterKirbyTrioBodyActive(fp) != FALSE)) ?
            TRUE : FALSE;
        sb32 known = (ndsFighterKirbyTrioHeadKey(fp, &head_key) != FALSE) ?
            TRUE : FALSE;

        if ((body != FALSE) && (known == FALSE))
        {
            return nNDSFtrLeanDeclineKirbyHead;
        }
        if (known == FALSE)
        {
            head_key = 0u;
        }
        if ((reuse_plan != FALSE) && (head_key != inst->head_key))
        {
            reuse_plan = FALSE;
            NDS_FTR_LEAN_CTR(gNdsFtrLean.plan_reuse_restarts++);
        }
        ndsRendererNativeKirbyTrioSetHeadKey(head_key);
    }
#endif
#if NDS_FTR_LEAN_LAB
    if ((reuse_plan != FALSE) && (reuse_check == FALSE))
#else
    if (reuse_plan != FALSE)
#endif
    {
        count = inst->root_count;
        program = inst->program;
        owner_file = (NDSRelocLoadedFile *)inst->owner_file;
        goto rows;
    }
    ndsFighterCollectAllDObjsWithDL(fp->joints[nFTPartsJointTopN],
                                    &collection);
#if NDS_R2_FOX_GUN_OVERLAY
    ndsFighterCollectStripFoxGunSidecar(fp, &collection);
#endif
    if ((ndsFighterDrawPlanResolve(owner_slot, expected_asset_id,
                                   &collection, ws, &owner_file) !=
         nNDSFighterDrawPlanOk) ||
        (owner_file == NULL) || (owner_file->data == NULL))
    {
        return nNDSFtrLeanDeclinePlan;
    }
    count = collection.selected_count;
    if ((count == 0u) || (count > NDS_FTR_LEAN_ROOT_MAX))
    {
        return nNDSFtrLeanDeclineRoots;
    }
    for (i = 0u; i < count; i++)
    {
        if ((collection.indices[i] > 0xffu) ||
            (ws->material_counts[i] > 0xffu))
        {
            return nNDSFtrLeanDeclineRoots;
        }
    }
    NDS_FTR_LEAN_EVENT_PART(0u, ev_mark);
    program = ndsRendererNativeFighterSelectRootProgram(
        owner_slot, use_low_detail, ws->root_offsets, count, &tried);
    if (program == 0xffu)
    {
        program = 0u;
    }
    ndsRendererNativeFighterSetRootProgram(owner_slot, program);
    if (ndsRendererAdapterValidateNativeOwnerCached(
            owner_slot, slot, use_low_detail, owner_file, count,
            ws->root_offsets, ws->material_counts) == FALSE)
    {
        return nNDSFtrLeanDeclineValidate;
    }
    NDS_FTR_LEAN_EVENT_PART(1u, ev_mark);
#if NDS_FTR_LEAN_LAB
    if ((reuse_plan != FALSE) && (reuse_check != FALSE) &&
        (ndsFtrLeanVerifyPlan(inst, &collection, ws, count, program,
                              owner_file) == FALSE))
    {
        reuse_plan = FALSE;
    }
#endif
    /* The instance now describes this plan (the kernel, the patches and the
     * per-frame proofs read it); the entry below is selected for it. */
    inst->valid = 0u;
    inst->root_count = (u8)count;
    for (i = 0u; i < count; i++)
    {
        const NDSFighterDisplayContractEvent *event =
            &sNdsFighterDisplayReplayEvents[collection.indices[i]];

        inst->event_index[i] = (u8)collection.indices[i];
        inst->root_offsets[i] = ws->root_offsets[i];
        inst->material_counts[i] = (u8)ws->material_counts[i];
        inst->root_dl[i] = event->dl;
        inst->matrix_dobjs[i] = ws->matrix_bindings[i];
        inst->material_dobjs[i] = ws->material_dobjs[i];
    }
    /* The material rows (the old path's own storage and row mapping, built
     * from the live MObjs without advancing their texture ids): every word a
     * list takes from its materials comes from these, so their content is
     * the entry key's word 0 -- a rebuilt MObj graph, or an animation that
     * moved nothing the rows carry, keeps its list. The instance holds the
     * plan's material DObjs and counts (just filled from the resolve, or
     * reused for a material event). */
rows:
    sNdsRendererAdapterMaterialRowClaimMask = 0u;
    key[0] = 2166136261u;
    for (i = 0u; i < count; i++)
    {
        u32 row = ndsRendererAdapterMaterialRow(inst->material_dobjs[i], i);
        NDSRendererNativeMaterial *rows =
            sNdsRendererAdapterNativeOwnerMaterials[row];
        const u32 *row_words = (const u32 *)(const void *)rows;
        const MObj *mobj;
        u32 n = 0u;
        u32 k;

        sNdsRendererAdapterNativeOwnerMaterialRows[i] = (u8)row;
        for (mobj = (inst->material_dobjs[i] != NULL) ?
                 inst->material_dobjs[i]->mobj : NULL;
             mobj != NULL; mobj = mobj->next)
        {
            if ((n >= NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX) ||
                (ndsRendererAdapterBuildNativeMaterialSnapshot(
                     (MObj *)mobj, &rows[n], FALSE, NULL, NULL) == FALSE))
            {
                return nNDSFtrLeanDeclineMaterial;
            }
            n++;
        }
        if ((n != inst->material_counts[i]) && (reuse_plan != FALSE))
        {
            /* The MObj chain moved under the reused plan: resolve it. */
            reuse_plan = FALSE;
            NDS_FTR_LEAN_CTR(gNdsFtrLean.plan_reuse_restarts++);
            goto restart;
        }
        if ((n != inst->material_counts[i]) ||
            (ndsRendererAdapterValidateNativeOwnerMaterials(rows, n) == FALSE))
        {
            return nNDSFtrLeanDeclineMaterial;
        }
        for (k = 0u;
             k < (n * (u32)sizeof(NDSRendererNativeMaterial)) / sizeof(u32);
             k++)
        {
            key[0] = (key[0] ^ row_words[k]) * 16777619u;
        }
        key[0] = (key[0] ^ n) * 16777619u;
    }
    key[1] = (owner_slot & 0xffu) | ((use_low_detail & 1u) << 8) |
        ((slot & 3u) << 9) |
        (((((u32)fp->costume) | ((u32)fp->shade << 8)) & 0xffffu) << 11);
    key[2] = (u32)(uintptr_t)owner_file->data ^
        (owner_file->owner_generation * 0x9E3779B1u) ^
        (gNdsTaskmanHeapGeneration * 0x85EBCA77u);
    key[3] = ndsFtrLeanPreambleHash(inst->event_index, inst->root_offsets,
                                    inst->material_counts, count);
    key[4] = program | (count << 8);
    key[5] = head_key;
    ndsFtrLeanPacketNoteKind(slot, row);
#if NDS_FTR_LEAN_LAB
    ndsFtrLeanKeyCensus(slot, row, key);
#endif
    NDS_FTR_LEAN_EVENT_PART(2u, ev_mark);
    ndsFtrLeanRefreshInputs(inst, color_modulate);
    ndsFtrLeanView(&view, inst, color_modulate, ws->production_roots);
    ident = ndsRendererAdapterMaterialIdentity(inst->material_dobjs, count);
    rr_key = ndsFtrLeanRerecordKey(ident, key, ws->production_roots, count);
    NDS_FTR_LEAN_EVENT_PART(3u, ev_mark);
    code = (force_new != FALSE) ? NDS_FTR_LEAN_ENTRY_NONE :
        ndsFtrLeanEntryFind(slot, key);
    if (code != NDS_FTR_LEAN_ENTRY_NONE)
    {
        NDS_FTR_LEAN_CTR(gNdsFtrLean.entry_hits++);
        if (ndsFtrLeanEntryActivate(slot, code) != FALSE)
        {
            /* A switch stands for the old path's re-record (its packet's
             * key moved): put the shade words where that record puts them. */
            ndsFtrLeanEntryResetShade(slot, &view);
            NDS_FTR_LEAN_CTR(gNdsFtrLean.entry_switches++);
        }
        else if ((inst->rerecord != 0u) || (inst->rr_key != rr_key))
        {
            /* The same list, but the old path re-records here (its packet
             * was invalidated -- a model part, a rebuilt MObj -- or its key
             * moved without moving this list's): the same derivation. */
            ndsFtrLeanEntryResetShade(slot, &view);
            NDS_FTR_LEAN_CTR(gNdsFtrLean.rerecord_resets++);
        }
#if NDS_FTR_LEAN_LAB
        if ((NDS_FTR_LEAN_SLOW_WORD() & NDS_FTR_LEAN_SLOW_VERIFY) != 0u)
        {
            /* Lab: the re-selected list against a fresh materialization of
             * the same key in the other entry. */
            u32 held = ndsFtrLeanEntryActiveIndex(slot);

            entry = ndsFtrLeanEntryVictim(slot);
            /* A wide list owns both halves: nothing to compare it in. */
            if ((held != NDS_FTR_LEAN_ENTRY_NONE) && (entry != held) &&
                (ndsFtrLeanEntryWide(slot, held) == FALSE) &&
                (ndsFtrLeanMaterializeFor(slot, &entry, key, count,
                                          owner_slot, use_low_detail,
                                          owner_file, FALSE) == 0u))
            {
                ndsFtrLeanVerifyEntries(slot, held, entry);
            }
        }
#endif
        NDS_FTR_LEAN_EVENT_PART(4u, ev_mark);
    }
    else
    {
        u32 reason;

        if (reuse_plan != FALSE)
        {
            /* A new list: the materializer reads the resolve's workspace. */
            reuse_plan = FALSE;
            NDS_FTR_LEAN_CTR(gNdsFtrLean.plan_reuse_restarts++);
            goto restart;
        }
        entry = ndsFtrLeanEntryVictim(slot);
        ndsFtrLeanNoteKeyMiss(slot, key);
        reason = ndsFtrLeanMaterializeFor(slot, &entry, key, count,
                                          owner_slot, use_low_detail,
                                          owner_file, TRUE);
        if (reason != 0u)
        {
            return reason;
        }
        code = ndsFtrLeanLearnVariant(slot, entry);
        if (code != NDS_FTR_LEAN_ENTRY_NONE)
        {
            /* The held list now draws this state: its words moved as the
             * old path's re-record would move them. */
            (void)ndsFtrLeanEntryActivate(slot, code);
            ndsFtrLeanEntryResetShade(slot, &view);
        }
        else
        {
            (void)ndsFtrLeanEntryActivate(slot, entry);
        }
#if NDS_FTR_LEAN_LAB
        {
            u32 ticks = cpuGetTiming() - t0;

            gNdsFtrLean.materialize_ticks += ticks;
            gNdsFtrLean.k_materialize_ticks[row] += ticks;
        }
#endif
        NDS_FTR_LEAN_EVENT_PART(5u, ev_mark);
    }
    inst->rr_key = rr_key;
    inst->rerecord = 0u;
    if ((reuse_plan == FALSE) &&
        (ndsFtrLeanBuildJoints(inst, fp->joints[nFTPartsJointTopN],
                               inst->matrix_dobjs, count) == FALSE))
    {
        return nNDSFtrLeanDeclineTopology;
    }
    NDS_FTR_LEAN_CTR(gNdsFtrLean.plan_reuses += (reuse_plan != FALSE) ? 1u : 0u);
    NDS_FTR_LEAN_EVENT_PART(6u, ev_mark);
    inst->kind = (u8)kind;
    inst->detail = (u8)use_low_detail;
    inst->skeleton = (u8)fp->colanim.skeleton_id;
    inst->program = (u8)program;
    inst->status_gen = sNdsFighterStatusGeneration[slot];
    inst->heap_gen = gNdsTaskmanHeapGeneration;
    inst->root = fp->joints[nFTPartsJointTopN];
    inst->owner_file = owner_file;
    inst->file_data = owner_file->data;
    inst->file_asset = owner_file->asset_id;
    inst->file_generation = owner_file->owner_generation;
    inst->file_size = owner_file->data_size;
    inst->head_key = head_key;
    inst->ident_live = ident;
    inst->texpart_serial = sNdsFtrLeanTexPartSerial[slot];
    ndsFtrLeanBuildWatch(inst, count);
    inst->pre_hash = key[3];
    inst->pre_serial = (sNdsFtrLeanMemoFromSlot != 0u) ?
        (sNdsFtrLeanMemoFill[slot] + 1u) : 0u;
    inst->patch_serial = 0u;
#if NDS_FTR_LEAN_ORACLE_ROUTES
    inst->topo_hash = ndsFtrLeanTopologyHash(inst);
#endif
    inst->rebind = 0u;
    inst->reprove = 0u;
    inst->valid = 1u;
    NDS_FTR_LEAN_EVENT_PART(7u, ev_mark);
#if NDS_FTR_LEAN_LAB
    {
        u32 ticks = cpuGetTiming() - t0;

        gNdsFtrLean.event_ticks += ticks;
        gNdsFtrLean.k_event_ticks[row] += ticks;
    }
#endif
    (void)t0;
    (void)row;
    return 0u;
}

/* The per-frame proofs of the active entry: the tuple (kind, detail, heap
 * generation, root, owner file, status generation -- a status change keeps
 * the entry when the drawn roots are unchanged), the material identity
 * through its writers, the preamble fields once per memo fill, and the
 * list's own guard. Returns 0 or the event that moves the draw to the event
 * path. */
static u32 ndsFtrLeanProve(u32 slot, FTStruct *fp, NDSFtrLeanInstance *inst,
                           u32 kind, u32 use_low_detail, u32 route,
                           u32 slow_mode, u32 *mark)
{
    const NDSRelocLoadedFile *file = inst->owner_file;
    u32 reason;
    u32 row = slot & 3u;        /* counter row (attribution ROM only) */

#if !NDS_FTR_LEAN_ORACLE_ROUTES
    route = NDS_FTR_LEAN_ROUTE_DRAW;    /* the shipping image's only route */
#endif
    (void)row;
    if (inst->valid == 0u)
    {
        return nNDSFtrLeanEventFirst;
    }
    if (inst->rebind != 0u)
    {
        return nNDSFtrLeanEventRebind;
    }
    /* Slice 6: the electric skeleton (Mario/Fox, program 0xFE) is a
     * colour-animation state with no status change and no writer to hook:
     * it toggles the drawn display lists every few frames, so the tuple
     * carries it and a toggle re-selects the list by key (both lists stay
     * held, one per entry). */
    if ((inst->kind != kind) || (inst->detail != use_low_detail) ||
        (inst->skeleton != (u8)fp->colanim.skeleton_id) ||
        (inst->heap_gen != gNdsTaskmanHeapGeneration) ||
        (inst->root != fp->joints[nFTPartsJointTopN]) ||
        (file == NULL) || (file->data != inst->file_data) ||
        (file->asset_id != inst->file_asset) ||
        (file->owner_generation != inst->file_generation) ||
        (file->data_size != inst->file_size))
    {
        return nNDSFtrLeanEventTuple;
    }
    NDS_FTR_LEAN_GUARD_PART(0u, *mark);
    if (inst->status_gen != sNdsFighterStatusGeneration[slot])
    {
        if (((slow_mode & NDS_FTR_LEAN_SLOW_GUARD) != 0u) ||
            (ndsFtrLeanRetuple(slot, fp, inst) == FALSE))
        {
            return nNDSFtrLeanEventStatus;
        }
        /* The status re-tuple is an event-time proof: its ticks go with
         * the event path's (part 7), not the tuple's. */
        NDS_FTR_LEAN_GUARD_PART(7u, *mark);
    }
    /* Topology: the kernel proves every kept joint's parent link per draw
     * (the source compose's own check). The oracle routes and the cost A/B's
     * slice 1 form also hash the kept joints' child/sibling links. */
#if NDS_FTR_LEAN_ORACLE_ROUTES
    if (((route != NDS_FTR_LEAN_ROUTE_DRAW) ||
         ((slow_mode & NDS_FTR_LEAN_SLOW_GUARD) != 0u)) &&
        (ndsFtrLeanTopologyHash(inst) != inst->topo_hash))
    {
        return nNDSFtrLeanEventKernel;
    }
#endif
    NDS_FTR_LEAN_GUARD_PART(1u, *mark);
    /* The material identity, proven through its writers (see
     * NDS_FTR_LEAN_WATCH_MAX); a moved identity is an event: the entry for
     * the new state, or a new list. */
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
        if ((watch_ok != FALSE) && (full != FALSE) &&
            (ndsRendererAdapterMaterialIdentity(
                 inst->material_dobjs, inst->root_count) != inst->ident_live))
        {
            /* A writer the watch did not see: after an old-path draw (its
             * material preparation) this is expected; on a draw that did not
             * follow one it is a hole in the watch. */
            if (inst->reprove == 0u)
            {
                NDS_FTR_LEAN_CTR(gNdsFtrLean.ident_watch_miss++);
            }
            watch_ok = FALSE;
        }
        if (watch_ok == FALSE)
        {
            if (ndsRendererAdapterMaterialIdentity(
                    inst->material_dobjs, inst->root_count) ==
                inst->ident_live)
            {
                /* Only the texture-part count moved (a write of the same id
                 * or to an undrawn MObj): nothing to select. */
                inst->texpart_serial = sNdsFtrLeanTexPartSerial[slot];
                ndsFtrLeanBuildWatch(inst, inst->root_count);
            }
            else
            {
                return nNDSFtrLeanEventMaterial;
            }
        }
        inst->reprove = 0u;
    }
    NDS_FTR_LEAN_GUARD_PART(3u, *mark);
    /* key[3]'s per-root fields. A memo hit replays the slot's stored
     * preambles byte for byte, so they are proven once per memo fill. */
    if ((sNdsFtrLeanMemoFromSlot == 0u) ||
        (inst->pre_serial != sNdsFtrLeanMemoFill[slot] + 1u) ||
        ((slow_mode & NDS_FTR_LEAN_SLOW_GUARD) != 0u))
    {
        NDS_FTR_LEAN_CTR(gNdsFtrLean.pre_checks++);
        if (ndsFtrLeanPreambleHash(inst->event_index, inst->root_offsets,
                                   inst->material_counts, inst->root_count) !=
            inst->pre_hash)
        {
            return nNDSFtrLeanEventPreamble;
        }
        inst->pre_serial = (sNdsFtrLeanMemoFromSlot != 0u) ?
            (sNdsFtrLeanMemoFill[slot] + 1u) : 0u;
    }
    else
    {
        NDS_FTR_LEAN_CTR(gNdsFtrLean.pre_skips++);
    }
    NDS_FTR_LEAN_GUARD_PART(4u, *mark);
    reason = ndsFtrLeanPacketGuard(slot,
        (route == NDS_FTR_LEAN_ROUTE_DRAW) ? 1u : 0u);
    NDS_FTR_LEAN_GUARD_PART(5u, *mark);
    return reason;
}

/* Returns TRUE when route 1 drew the fighter (the old path must not run). */
static NDS_FTR_LEAN_RUN_INLINE sb32
ndsFtrLeanRun(u32 slot, FTStruct *fp, u32 route)
{
    NDSFtrLeanInstance *inst = &sNdsFtrLeanInstances[slot];
    u32 owner_slot = 0u;
    u32 kind;
    u32 use_low_detail;
    u32 slow_mode = NDS_FTR_LEAN_SLOW_WORD();
    u32 reason;
    u32 event;
    u32 pass;
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
    NDSFtrLeanPatchView view;
    u32 hardware_triangles;
    u32 camera_projection_valid = FALSE;
    u32 camera_modelview_valid = FALSE;
    u32 row = slot & 3u;        /* counter row (NDS_FTR_LEAN_KINDS) */
#if NDS_FTR_LEAN_ATTR_LIVE
    u32 retried = FALSE;
#endif

#if !NDS_FTR_LEAN_ORACLE_ROUTES
    route = NDS_FTR_LEAN_ROUTE_DRAW;    /* the shipping image's only route */
#endif
    kind = ndsFtrLeanEligible(fp, &owner_slot);
    if (kind == NDS_FTR_LEAN_KIND_NONE)
    {
        return FALSE;   /* not a lean kind: not an attempt */
    }
    NDS_FTR_LEAN_CTR(gNdsFtrLean.attempts++);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.k_attempts[row]++);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.k_owner[row] = owner_slot + 1u);
    t0 = cpuGetTiming();
    mark = t0;
    /* Slice 6: Mario and Fox own a native electric body (program 0xFE, the
     * generated skeleton owner); the lean path resolves it like any program
     * (the tuple re-selects on a toggle). The port gives no other kind the
     * skeleton colour family (ftParamCheckSetSkeletonColAnimID selects the
     * source's no-skeleton flash for them), so a skeleton frame of any other
     * kind is not reachable -- named, never drawn wrong, if it ever is. */
    if ((fp->colanim.skeleton_id != 0) &&
        (owner_slot != NDS_RENDERER_NATIVE_FIGHTER_OWNER_MARIO) &&
        (owner_slot != NDS_RENDERER_NATIVE_FIGHTER_OWNER_FOX))
    {
        ndsFtrLeanDecline(row, nNDSFtrLeanDeclineSkeleton);
        return FALSE;
    }
#if !NDS_R2_FIGHTER_SHUFFLE_FOLD
    if (fp->shuffle_tics != 0u)
    {
        ndsFtrLeanDecline(row, nNDSFtrLeanDeclineKind);
        return FALSE;
    }
#endif
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
            ndsFtrLeanDecline(row, nNDSFtrLeanDeclineCamera);
            return FALSE;
        }
    }
    NDS_FTR_LEAN_GUARD_PART(2u, mark);
    /* Slice 6: Captain HIGH is a lean draw. Its G_AC_THRESHOLD cutout (root 6)
     * compares against G_SETBLENDCOLOR alpha 0, DS reference 0 -- a test that
     * decides no pixel, since the DS never draws an alpha-0 fragment with the
     * test on or off -- so the list carries no alpha register state, and the
     * materializer declines any threshold run whose reference is not 0
     * (nNDSFtrLeanDeclineAlphaTest). */
    use_low_detail = (fp->detail_curr == nFTPartsDetailLow) ? TRUE : FALSE;
    color_modulate = ndsRendererAdapterFighterColorModulate(fp);
    ndsFtrLeanPacketNoteKind(slot, row);
    event = ndsFtrLeanProve(slot, fp, inst, kind, use_low_detail, route,
                            slow_mode, &mark);
    for (pass = 0u;; pass++)
    {
        if (event != 0u)
        {
            u32 t_event = cpuGetTiming();

            ndsFtrLeanCountEvent(row, event);
            if ((event == nNDSFtrLeanEventTintSet) ||
                (event == nNDSFtrLeanEventFence))
            {
                /* The active list is stale against the live tile set or
                 * texture fence (the recorder re-records on the same key
                 * words); the other entry keeps its own. */
                ndsFtrLeanEntryDropActive(slot);
            }
            if (event == nNDSFtrLeanEventKernel)
            {
                inst->valid = 0u;
            }
            reason = ndsFtrLeanEvent(
                slot, fp, inst, kind, owner_slot, use_low_detail,
                color_modulate,
                (event == nNDSFtrLeanEventTintTile) ? TRUE : FALSE,
                ((event == nNDSFtrLeanEventMaterial) && (inst->valid != 0u)) ?
                    TRUE : FALSE);
#if NDS_FTR_LEAN_LAB
            /* The event path is its own part (7); the guard parts after it
             * restart from here. */
            mark = t_event;
            NDS_FTR_LEAN_GUARD_PART(7u, mark);
#endif
            (void)t_event;
            if (reason != 0u)
            {
                ndsFtrLeanInvalidate(slot, FALSE);
                ndsFtrLeanDecline(row, reason);
                return FALSE;
            }
            event = ndsFtrLeanPacketGuard(slot,
                (route == NDS_FTR_LEAN_ROUTE_DRAW) ? 1u : 0u);
            if (event != 0u)
            {
                ndsFtrLeanInvalidate(slot, TRUE);
                ndsFtrLeanDecline(row, nNDSFtrLeanDeclineStale);
                return FALSE;
            }
            NDS_FTR_LEAN_GUARD_PART(5u, mark);
        }
#if NDS_R2_FIGHTER_SHUFFLE_FOLD
        /* The old path latches this every draw; so does the lean one, so the
         * statics read after this draw are the same in both routes. */
        ndsRendererAdapterSetShuffleOffset(fp);
        shuffle_x = sNdsR2ShuffleWorldX;
        shuffle_y = sNdsR2ShuffleWorldY;
#endif
        NDS_FTR_LEAN_GUARD_PART(6u, mark);
#if NDS_FTR_LEAN_ATTR_LIVE
        if ((slow_mode & NDS_FTR_LEAN_SLOW_KERNEL_QUIET) != 0u)
        {
            /* Attribution arm: the previous fighter's list has finished
             * streaming before this kernel reads a DObj. */
            u32 q0 = cpuGetTiming();

            while ((DMA_CR(0) & DMA_BUSY) != 0u) { }
            gNdsFtrLeanAttr.quiet_wait_ticks[row] += cpuGetTiming() - q0;
            gNdsFtrLeanAttr.quiet_draws[row]++;
        }
#endif
        t1 = cpuGetTiming();
        NDS_FTR_LEAN_CTR(gNdsFtrLean.guard_ticks += t1 - t0);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.k_guard_ticks[row] += t1 - t0);
#if NDS_FTR_LEAN_LAB
        class_counts = gNdsFtrLean.k_kernel_class[row];
#if NDS_FTR_LEAN_KTIME
        if ((slow_mode & NDS_FTR_LEAN_SLOW_KTIME) != 0u)
        {
#if NDS_FTR_LEAN_ATTR_LIVE
            part_ticks = gNdsFtrLeanAttr.kernel_part_ticks[row];
            gNdsFtrLeanAttr.kernel_part_joints[row] += inst->joint_count;
#else
            part_ticks = gNdsFtrLean.kernel_part_ticks;
#endif
            gNdsFtrLean.kernel_part_joints += inst->joint_count;
        }
#endif
#endif
        kernel_flags = ((fp->is_use_animlocks != FALSE) ?
                            NDS_FTR_LEAN_KERNEL_LOCKS : 0u) |
            (((slow_mode & NDS_FTR_LEAN_SLOW_KERNEL) != 0u) ?
                 NDS_FTR_LEAN_KERNEL_NO_FAST : 0u);
        /* The LOAD4x3 parameters go straight into the list; only a texgen
         * group's root also needs its Q20.12 world (PatchTexgen reads it). */
#if NDS_FTR_LEAN_ATTR_LIVE
        retried = FALSE;
#endif
        sites = ndsFtrLeanPacketModelviewSites(slot, mv_sites,
                                               inst->root_count);
        if ((sites == NDS_FTR_LEAN_SITES_NONE) ||
            (ndsFtrLeanKernelCompose(
                 inst->joints, inst->joint_count, mv_sites, sNdsFtrLeanWorlds,
                 sites, inst->root_count, shuffle_x, shuffle_y, kernel_flags,
                 ndsFtrLeanSlowLocal, class_counts, part_ticks) == FALSE))
        {
            /* A kept joint was re-parented (a status change that moves no
             * drawn root: the re-tuple proved the roots). The world is a
             * function of the live tree, which the source compose re-collects
             * every draw, so re-collect it once and compose again; a second
             * refusal is the source compose's own. */
            NDS_FTR_LEAN_CTR(gNdsFtrLean.kernel_fail++);
            if ((sites == NDS_FTR_LEAN_SITES_NONE) ||
                (ndsFtrLeanBuildJoints(inst, fp->joints[nFTPartsJointTopN],
                                       inst->matrix_dobjs,
                                       inst->root_count) == FALSE) ||
                (ndsFtrLeanKernelCompose(
                     inst->joints, inst->joint_count, mv_sites,
                     sNdsFtrLeanWorlds, sites, inst->root_count, shuffle_x,
                     shuffle_y, kernel_flags, ndsFtrLeanSlowLocal,
                     class_counts, part_ticks) == FALSE))
            {
                ndsFtrLeanInvalidate(slot, FALSE);
                ndsFtrLeanDecline(row, nNDSFtrLeanDeclineKernel);
                return FALSE;
            }
#if NDS_FTR_LEAN_ORACLE_ROUTES
            inst->topo_hash = ndsFtrLeanTopologyHash(inst);
#endif
            NDS_FTR_LEAN_CTR(gNdsFtrLean.kernel_rebuilds++);
#if NDS_FTR_LEAN_ATTR_LIVE
            retried = TRUE;
#endif
        }
        t0 = cpuGetTiming();
        NDS_FTR_LEAN_CTR(gNdsFtrLean.kernel_ticks += t0 - t1);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.k_kernel_ticks[row] += t0 - t1);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.kernel_joints += inst->joint_count);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.k_kernel_joints[row] +=
                             inst->joint_count);
#if NDS_FTR_LEAN_ATTR_LIVE
        if (((slow_mode & NDS_FTR_LEAN_SLOW_KERNEL_RERUN) != 0u) &&
            (retried == FALSE) &&
            ((kernel_flags & NDS_FTR_LEAN_KERNEL_LOCKS) == 0u))
        {
            /* Attribution arm: the draw's own pass was cold (t0 - t1). The
             * first eight joints (a preorder prefix, so every parent
             * precedes its child; their data fits the 4 KB data cache) run
             * twice more at once -- the second of those is code and data
             * warm -- and once after DC_FlushAll: data cold, code warm.
             * Every pass writes the same words into the same sites. */
            u32 n = (inst->joint_count < 8u) ? inst->joint_count : 8u;
            u32 a0;
            u32 a1;
            u32 a2;
            u32 a3;

            (void)ndsFtrLeanKernelCompose(
                inst->joints, n, mv_sites, sNdsFtrLeanWorlds, sites,
                inst->root_count, shuffle_x, shuffle_y, kernel_flags,
                ndsFtrLeanSlowLocal, NULL, NULL);
            a0 = cpuGetTiming();
            (void)ndsFtrLeanKernelCompose(
                inst->joints, n, mv_sites, sNdsFtrLeanWorlds, sites,
                inst->root_count, shuffle_x, shuffle_y, kernel_flags,
                ndsFtrLeanSlowLocal, NULL, NULL);
            a1 = cpuGetTiming();
            DC_FlushAll();
            a2 = cpuGetTiming();
            (void)ndsFtrLeanKernelCompose(
                inst->joints, n, mv_sites, sNdsFtrLeanWorlds, sites,
                inst->root_count, shuffle_x, shuffle_y, kernel_flags,
                ndsFtrLeanSlowLocal, NULL, NULL);
            a3 = cpuGetTiming();
            gNdsFtrLeanAttr.kernel_cold_ticks[row] += t0 - t1;
            gNdsFtrLeanAttr.kernel_warm_ticks[row] += a1 - a0;
            gNdsFtrLeanAttr.kernel_dcold_ticks[row] += a3 - a2;
            gNdsFtrLeanAttr.kernel_attr_draws[row]++;
            gNdsFtrLeanAttr.kernel_attr_joints[row] += n;
            t0 = cpuGetTiming();
        }
#endif

#if NDS_FTR_LEAN_ATTR_LIVE
        {
            u32 r0 = cpuGetTiming();
#endif
        /* Slice 5: the patch reads the view, not the shared production
         * inputs, so they are refreshed per draw only for a texgen list
         * without a site map (the generic texgen patch reads them:
         * NDS_FTR_LEAN_SITES_INPUTS) and in the slice 4 cost form
         * (gNdsFtrLeanSlow bit 256). */
        if (((slow_mode & NDS_FTR_LEAN_SLOW_REFRESH) != 0u) ||
            ((sites & NDS_FTR_LEAN_SITES_INPUTS) != 0u))
        {
            ndsFtrLeanRefreshInputs(inst, color_modulate);
            ndsFtrLeanView(&view, inst, color_modulate,
                sNdsRendererAdapterNativeOwnerWorkspace.production_roots);
        }
        else
        {
            ndsFtrLeanView(&view, inst, color_modulate, NULL);
        }
#if NDS_FTR_LEAN_ATTR_LIVE
            u32 r1 = cpuGetTiming();
#endif
        /* The global state the old path's draw leaves behind: the owner's
         * root program (Link's texgen tables are selected from it) and the
         * Kirby trio head key it publishes for every fighter. */
        ndsRendererNativeFighterSetRootProgram(owner_slot, inst->program);
#if NDS_P2_KIRBY
        ndsRendererNativeKirbyTrioSetHeadKey(inst->head_key);
#endif
#if NDS_FTR_LEAN_ATTR_LIVE
            gNdsFtrLeanAttr.patch_part_ticks[row][5] += r1 - r0;
            gNdsFtrLeanAttr.patch_part_ticks[row][6] += cpuGetTiming() - r1;
        }
#endif
        /* pre_same: this draw's preambles are the same memo fill the last
         * patch read, so their prim and light words cannot have moved. */
        reason = ndsFtrLeanPacketPatch(
            slot, &view, owner_slot, use_low_detail,
            ((sNdsFtrLeanMemoFromSlot != 0u) &&
             (inst->patch_serial == sNdsFtrLeanMemoFill[slot] + 1u)) ?
                TRUE : FALSE);
        if ((reason == NDS_FTR_LEAN_PATCH_REMATERIALIZE) && (pass == 0u))
        {
            /* A tinted prim went white or lost its tile: the live draw
             * binds differently now (the recorder re-records). */
            event = nNDSFtrLeanEventTintTile;
            continue;
        }
        if (reason != 0u)
        {
            ndsFtrLeanInvalidate(slot, FALSE);
            ndsFtrLeanDecline(row,
                (reason == NDS_FTR_LEAN_PATCH_REMATERIALIZE) ?
                    nNDSFtrLeanDeclineStale : reason);
            return FALSE;
        }
        break;
    }
    inst->patch_serial = (sNdsFtrLeanMemoFromSlot != 0u) ?
        (sNdsFtrLeanMemoFill[slot] + 1u) : 0u;
    t1 = cpuGetTiming();
    NDS_FTR_LEAN_CTR(gNdsFtrLean.patch_ticks += t1 - t0);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.k_patch_ticks[row] += t1 - t0);
#if NDS_FTR_LEAN_ORACLE_ROUTES
    if (route != NDS_FTR_LEAN_ROUTE_DRAW)
    {
        ndsFtrLeanShadowArm(slot, 1u);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.shadow_runs++);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.k_draws[row]++);
        return FALSE;
    }
#endif

    /* Route 1: submit, then the bookkeeping DrawForSlot does after a
     * successful native draw (RAF tail), so every verifier count agrees. */
    gNdsFtrDeclineStage = 0u;
    gNdsFtrDeclineDisplayListClause = 0u;
    gNdsFtrRootProgramsTried = 0u;
#if NDS_TICK_HUD
    {
        NDSFighterDLAllDrawCollection census;
        u32 i;

        memset(&census, 0, sizeof(census));
        census.selected_count = inst->root_count;
        census.total_count = inst->root_count;
        for (i = 0u; i < inst->root_count; i++)
        {
            census.indices[i] = inst->event_index[i];
            census.dobjs[i] = inst->matrix_dobjs[i];
        }
        ndsFtrPreWalkCensus(slot, &census);
    }
    NDS_TICK_HUD_NATIVE_OWNER_MARK(nNDSTickHudNativeOwnerFallbackCalls);
    NDS_TICK_HUD_NATIVE_OWNER_MARK(nNDSTickHudNativeOwnerFallbackEligible);
#endif
    ndsRendererProfileSetOwner(ndsFighterNativeOwnerProfileId(owner_slot));
    t0 = cpuGetTiming();
    NDS_FTR_LEAN_CTR(gNdsFtrLean.book_ticks += t0 - t1);
    NDS_FTR_LEAN_ATTR(gNdsFtrLeanAttr.book_ticks[row] += t0 - t1);
    hardware_triangles = ndsFtrLeanPacketSubmit(slot);
    t1 = cpuGetTiming();
    NDS_FTR_LEAN_CTR(gNdsFtrLean.submit_ticks += t1 - t0);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.k_submit_ticks[row] += t1 - t0);
#if NDS_R2_FOX_GUN_OVERLAY
    /* Slice 6: Fox's blaster (and Kirby's Fox copy) is the sidecar the old
     * path submits after either body path (DrawForSlot's tail): the same
     * builder -- which gates on the kind, the gameplay owner and the live
     * joint-17 model part, and refreshes that joint's mtx_translate exactly
     * as the old path's call does -- under the fighter's profile owner. The
     * gun waits for this list's DMA itself. */
    if (ndsFighterHoldsFoxGunSource(fp) != FALSE)
    {
        NDSRendererMatrix20p12 sidecar_world;

        if (ndsRendererAdapterBuildFoxGunJointMtx(
                fp,
                (gGCCurrentCamera != NULL) ?
                    CObjGetStruct(gGCCurrentCamera) : NULL,
                &sidecar_world) != FALSE)
        {
            (void)ndsRendererSubmitFoxGun(&sidecar_world);
        }
    }
#endif
    ndsRendererProfileSetOwner(NDS_RENDERER_PROFILE_OWNER_NONE);
    if (slot == 0u)
    {
        gNdsFighterDLAllDrawP0HardwareTriangleCount += hardware_triangles;
        gNdsFighterDLAllDrawP0StatusAfter = (u32)fp->status_id;
        gNdsFighterDLAllDrawP0MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLAllDrawP0GAAfter = (u32)fp->ga;
    }
    else if (slot == 1u)
    {
        gNdsFighterDLAllDrawP1HardwareTriangleCount += hardware_triangles;
        gNdsFighterDLAllDrawP1StatusAfter = (u32)fp->status_id;
        gNdsFighterDLAllDrawP1MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLAllDrawP1GAAfter = (u32)fp->ga;
    }
    if (hardware_triangles != 0u)
    {
        gNdsFighterDLAllDrawSlotTriangleMask |= 1u << (slot & 3u);
    }
    gNdsFighterMarioFoxDLAllDrawCount++;
#if NDS_FTR_LEAN_ATTR_LIVE
    gNdsFtrLeanAttr.book_ticks[row] += cpuGetTiming() - t1;
#endif
    NDS_FTR_LEAN_CTR(gNdsFtrLean.book_ticks += cpuGetTiming() - t1);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.draws++);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.k_draws[row]++);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.k_program_draws[row][
        (inst->program < 15u) ? inst->program : 15u]++);
    if (use_low_detail == FALSE)
    {
        NDS_FTR_LEAN_CTR(gNdsFtrLean.k_high_draws[row]++);
    }
    (void)t0;
    (void)t1;
    (void)mark;
    return TRUE;
}

/* After the old path drew this fighter: retire an unconsumed oracle arm,
 * and when the draw was not a replay hit (it prepared materials, which may
 * have written this fighter's MObjs) re-prove the whole identity on the next
 * lean draw. */
static void ndsFtrLeanAfterOldPath(u32 slot, FTStruct *fp, u32 hits_before)
{
    (void)fp;
#if NDS_FTR_LEAN_ORACLE_ROUTES
    if (ndsFtrLeanShadowArmed(slot) != 0u)
    {
        ndsFtrLeanShadowArm(slot, 0u);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.oracle_unconsumed++);
    }
#endif
    if (gNdsFighterPacketHits == hits_before)
    {
        sNdsFtrLeanInstances[slot].reprove = 1u;
    }
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
            ndsFtrLeanInvalidate(slot, TRUE);
#if NDS_FTR_LEAN_LAB
            sNdsFtrLeanKeySeenCount[slot] = 0u;
#endif
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
