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

#if NDS_P2_NESS
/* Temporary P2-3f47 admission witness.  A 14-root successful Ness production
 * owner is source-derived as exactly 318 High-detail triangles.  Keep the
 * first positive mismatch in RAM so the debugger does not have to recover
 * optimized locals at the success assignment.  This block is removed once
 * the admission failure is assigned. */
volatile u32 gNdsP2NessOddDrawCount;
volatile u32 gNdsP2NessOddFrame;
volatile u32 gNdsP2NessOddTriangles;
volatile u32 gNdsP2NessOddSelected;
volatile u32 gNdsP2NessOddStatus;
volatile u32 gNdsP2NessOddMotion;
volatile u32 gNdsP2NessOddCamera;
volatile u32 gNdsP2NessOddDetail;
volatile u32 gNdsP2NessOddPacketPredicted;
volatile u32 gNdsP2NessOddProductionAttempted;
volatile u32 gNdsP2NessPositiveDrawCount;
volatile u32 gNdsP2NessProductionPositiveDrawCount;
volatile u32 gNdsP2NessNonProductionPositiveDrawCount;
volatile u32 gNdsP2NessNonProductionTriangles;
volatile u32 gNdsP2NessZeroProductionDrawCount;
volatile u32 gNdsP2NessLastNativeOwnerEnabled;
volatile u32 gNdsP2NessLastProductionMode;
volatile u32 gNdsP2NessLastHierarchyMode;
volatile u32 gNdsP2NessLastNoOracle;
volatile u32 gNdsP2NessLastDetailedOutput;
volatile u32 gNdsP2NessLastPlanHit;
volatile u32 gNdsP2NessFirstFallbackReason;
volatile u32 gNdsP2NessLastFallbackReason;
volatile u32 gNdsP2NessLastPlanResult;
#define NDS_P2_NESS_FALLBACK(reason_) do { \
    if (owner_slot == 9u) { \
        u32 nds_p2_ness_reason = (u32)(reason_); \
        gNdsP2NessLastFallbackReason = nds_p2_ness_reason; \
        if (gNdsP2NessFirstFallbackReason == 0u) \
            gNdsP2NessFirstFallbackReason = nds_p2_ness_reason; \
    } \
} while (0)
#else
#define NDS_P2_NESS_FALLBACK(reason_) ((void)0)
#endif

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

/* Writer-side coherency for source mutations of DObj display state/topology.
 * ftMainSetStatus is one writer (hidden-part replacement/re-parenting), but
 * BattleShip's ftParamSet/Reset/HideModelPart* family is another: it changes
 * joint->dl and FTParts.flags without changing the fighter root.  Both the
 * display-contract memo and Cycle-99 draw plan cache those values/pointers, so
 * either source mutation must advance the same generation rather than forcing
 * a hot-path tree hash every draw. */
void ndsFighterRendererInvalidateStatusCachesOnSetStatus(GObj *fighter_gobj);

void ndsFighterRendererInvalidateDObjStateCaches(GObj *fighter_gobj)
{
    ndsFighterRendererInvalidateStatusCachesOnSetStatus(fighter_gobj);
}

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

#if NDS_R2_FOX_GUN_OVERLAY
/* Fox Neutral-B does exactly what BattleShip asks: SetModelPartID(17, 0)
 * replaces joint 17's live DL with dFoxUnknown_DL from reloc file 315.  That
 * file is deliberately NOT part of Fox's model owner (0x139), so feeding this
 * one root to ndsFighterDrawPlanResolve makes the entire fighter fail the
 * native-owner asset gate and sends Fox through the generic renderer while the
 * pistol is out.
 *
 * The DS already owns this source model part as NDS_R2_FOX_GUN_OVERLAY: the
 * exact 44-vertex / 22-triangle file-315 mesh is baked separately and follows
 * the same joint-17 matrix.  Preserve the source DObj mutation for gameplay and
 * effect attachment, but remove that ONE foreign render root from the body
 * owner's collection.  The sidecar submit below draws it after either body
 * path, so a fail-closed body fallback still retains the source pistol.
 *
 * Pin the source identity here rather than suppressing arbitrary foreign roots:
 * file 315 is asset 0x13b and Fox's source hold joint is 17.  If either changes,
 * this helper declines and the existing asset gate falls back rather than
 * silently hiding new content. */
#define NDS_FOX_GUN_SOURCE_ASSET_ID 0x13bu
static void ndsFighterCollectStripFoxGunSidecar(
    const FTStruct *fp, NDSFighterDLAllDrawCollection *collection)
{
    DObj *gun_joint;
    u32 read_index;
    u32 write_index = 0u;

    if ((fp == NULL) || (collection == NULL) ||
        (fp->fkind != nFTKindFox) ||
        ((u32)NDS_FOX_GUN_HOLD_JOINT >= ARRAY_COUNT(fp->joints)) ||
        (fp->modelpart_status[NDS_FOX_GUN_HOLD_JOINT -
                              nFTPartsJointCommonStart].modelpart_id_curr < 0))
    {
        return;
    }
    gun_joint = fp->joints[NDS_FOX_GUN_HOLD_JOINT];
    if ((gun_joint == NULL) || (gun_joint == DOBJ_PARENT_NULL))
    {
        return;
    }

    for (read_index = 0u; read_index < collection->selected_count; read_index++)
    {
        DObj *dobj = collection->dobjs[read_index];
        u32 event_index = collection->indices[read_index];
        const Gfx *dl = (sNdsFighterDisplayContractPlayback != FALSE) ?
            sNdsFighterDisplayReplayEvents[event_index].dl :
            ((dobj != NULL) ? dobj->dl : NULL);
        NDSRelocLoadedFile *loaded = (dl != NULL) ?
            ndsRelocFindLoadedFileContaining(dl, sizeof(*dl)) : NULL;
        sb32 is_source_gun =
            (dobj == gun_joint) && (loaded != NULL) &&
            (loaded->asset_id == NDS_FOX_GUN_SOURCE_ASSET_ID);

        if (is_source_gun != FALSE)
        {
            if (event_index < 32u)
            {
                collection->selected_index_mask &= ~(1u << event_index);
            }
            if (collection->total_count != 0u)
            {
                collection->total_count--;
            }
            continue;
        }
        if (write_index != read_index)
        {
            collection->dobjs[write_index] = dobj;
            collection->indices[write_index] = event_index;
        }
        write_index++;
    }
    collection->selected_count = write_index;
}
#endif

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
        root->gx_modelview_mirror_valid =
            workspace->gx_modelview_mirror_valid;
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
            root->gx_modelview_mirror_valid = 0u;
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
        workspace->root_offsets[i] = ndsRelocNativeRootOffset(loaded, native_dl);
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
    u32 slot, FTStruct *fp, DObj *root, u32 expected_asset_id,
    NDSRendererAdapterNativeOwnerWorkspace *workspace)
{
    NDSFighterDrawPlanData *scratch = &sNdsFighterDrawPlanVerifyScratch;
    NDSFighterDLAllDrawCollection live;
    NDSRelocLoadedFile *owner_file = NULL;

    ndsFighterCollectAllDObjsWithDL(root, &live);
#if NDS_R2_FOX_GUN_OVERLAY
    ndsFighterCollectStripFoxGunSidecar(fp, &live);
#else
    (void)fp;
#endif
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
#if NDS_P2_CAPTAIN
    if (fp->fkind == nFTKindCaptain)
    {
        *owner_slot = 4u;
        return TRUE;
    }
#endif
#if NDS_P2_SAMUS
    if (fp->fkind == nFTKindSamus)
    {
        *owner_slot = 5u;
        return TRUE;
    }
#endif
#if NDS_P2_LINK
    if (fp->fkind == nFTKindLink)
    {
        *owner_slot = 6u;
        return TRUE;
    }
#endif
#if NDS_P2_PIKACHU
    if (fp->fkind == nFTKindPikachu)
    {
        *owner_slot = 7u;
        return TRUE;
    }
#endif
#if NDS_P2_YOSHI
    if (fp->fkind == nFTKindYoshi)
    {
        *owner_slot = 8u;
        return TRUE;
    }
#endif
#if NDS_P2_NESS
    if (fp->fkind == nFTKindNess)
    {
        *owner_slot = 9u;
        return TRUE;
    }
#endif
#if NDS_P2_PURIN
    if (fp->fkind == nFTKindPurin)
    {
        *owner_slot = 10u;
        return TRUE;
    }
#endif
#if NDS_P2_KIRBY
    if (fp->fkind == nFTKindKirby)
    {
        *owner_slot = 11u;
        return TRUE;
    }
#endif
#if NDS_P2_GDONKEY
    if (fp->fkind == nFTKindGDonkey)
    {
        /* P2-6 variant: GDonkey reuses the Donkey owner packet verbatim
         * (BattleShip DonkeyModel 0x13d, admit_fighter.py). */
        *owner_slot = 3u;
        return TRUE;
    }
#endif
#if NDS_P2_MMARIO
    if (fp->fkind == nFTKindMMario)
    {
        *owner_slot = 12u;
        return TRUE;
    }
#endif
#if NDS_P2_NMARIO
    if (fp->fkind == nFTKindNMario)
    {
        *owner_slot = 13u;
        return TRUE;
    }
#endif
#if NDS_P2_NFOX
    if (fp->fkind == nFTKindNFox)
    {
        *owner_slot = 14u;
        return TRUE;
    }
#endif
#if NDS_P2_NDONKEY
    if (fp->fkind == nFTKindNDonkey)
    {
        *owner_slot = 15u;
        return TRUE;
    }
#endif
#if NDS_P2_NSAMUS
    if (fp->fkind == nFTKindNSamus)
    {
        *owner_slot = 16u;
        return TRUE;
    }
#endif
#if NDS_P2_NLUIGI
    if (fp->fkind == nFTKindNLuigi)
    {
        /* P2-6 variant: NLuigi reuses the NMario owner packet verbatim
         * (BattleShip NMarioModel 0x12d, admit_fighter.py). */
        *owner_slot = 13u;
        return TRUE;
    }
#endif
#if NDS_P2_NLINK
    if (fp->fkind == nFTKindNLink)
    {
        *owner_slot = 17u;
        return TRUE;
    }
#endif
#if NDS_P2_NYOSHI
    if (fp->fkind == nFTKindNYoshi)
    {
        *owner_slot = 18u;
        return TRUE;
    }
#endif
#if NDS_P2_NCAPTAIN
    if (fp->fkind == nFTKindNCaptain)
    {
        *owner_slot = 19u;
        return TRUE;
    }
#endif
#if NDS_P2_NKIRBY
    if (fp->fkind == nFTKindNKirby)
    {
        *owner_slot = 20u;
        return TRUE;
    }
#endif
#if NDS_P2_NPIKACHU
    if (fp->fkind == nFTKindNPikachu)
    {
        *owner_slot = 21u;
        return TRUE;
    }
#endif
#if NDS_P2_NPURIN
    if (fp->fkind == nFTKindNPurin)
    {
        *owner_slot = 22u;
        return TRUE;
    }
#endif
#if NDS_P2_NNESS
    if (fp->fkind == nFTKindNNess)
    {
        *owner_slot = 23u;
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
#if NDS_P2_CAPTAIN
    if (owner_slot == 4u)
    {
        return 0x14cu; /* llCaptainModelFileID, BattleShip dFTCaptainData */
    }
#endif
#if NDS_P2_SAMUS
    if (owner_slot == 5u)
    {
        return 0x140u; /* llSamusModelFileID, BattleShip dFTSamusData */
    }
#endif
#if NDS_P2_LINK
    if (owner_slot == 6u)
    {
        return 0x144u; /* llLinkModelFileID, BattleShip dFTLinkData */
    }
#endif
#if NDS_P2_PIKACHU
    if (owner_slot == 7u)
    {
        return 0x155u; /* llPikachuModelFileID, BattleShip dFTPikachuData */
    }
#endif
#if NDS_P2_YOSHI
    if (owner_slot == 8u)
    {
        return 0x152u; /* llYoshiModelFileID, BattleShip dFTYoshiData */
    }
#endif
#if NDS_P2_NESS
    if (owner_slot == 9u)
    {
        return 0x14fu; /* llNessModelFileID, BattleShip dFTNessData */
    }
#endif
#if NDS_P2_PURIN
    if (owner_slot == 10u)
    {
        return 0x14au; /* llPurinModelFileID, BattleShip dFTPurinData */
    }
#endif
#if NDS_P2_KIRBY
    if (owner_slot == 11u)
    {
        return 0x148u; /* llKirbyModelFileID, BattleShip dFTKirbyData */
    }
#endif
#if NDS_P2_MMARIO
    if (owner_slot == 12u)
    {
        return 0x12cu; /* llMMarioModelFileID, BattleShip dFTMMarioData */
    }
#endif
#if NDS_P2_NMARIO
    if (owner_slot == 13u)
    {
        return 0x12du; /* llNMarioModelFileID, BattleShip dFTNMarioData */
    }
#endif
#if NDS_P2_NFOX
    if (owner_slot == 14u)
    {
        return 0x12fu; /* llNFoxModelFileID, BattleShip dFTNFoxData */
    }
#endif
#if NDS_P2_NDONKEY
    if (owner_slot == 15u)
    {
        return 0x134u; /* llNDonkeyModelFileID, BattleShip dFTNDonkeyData */
    }
#endif
#if NDS_P2_NSAMUS
    if (owner_slot == 16u)
    {
        return 0x135u; /* llNSamusModelFileID, BattleShip dFTNSamusData */
    }
#endif
#if NDS_P2_NLINK
    if (owner_slot == 17u)
    {
        return 0x136u; /* llNLinkModelFileID, BattleShip dFTNLinkData */
    }
#endif
#if NDS_P2_NYOSHI
    if (owner_slot == 18u)
    {
        return 0x130u; /* llNYoshiModelFileID, BattleShip dFTNYoshiData */
    }
#endif
#if NDS_P2_NCAPTAIN
    if (owner_slot == 19u)
    {
        return 0x137u; /* llNCaptainModelFileID, BattleShip dFTNCaptainData */
    }
#endif
#if NDS_P2_NKIRBY
    if (owner_slot == 20u)
    {
        return 0x131u; /* llNKirbyModelFileID, BattleShip dFTNKirbyData */
    }
#endif
#if NDS_P2_NPIKACHU
    if (owner_slot == 21u)
    {
        return 0x133u; /* llNPikachuModelFileID, BattleShip dFTNPikachuData */
    }
#endif
#if NDS_P2_NPURIN
    if (owner_slot == 22u)
    {
        return 0x132u; /* llNPurinModelFileID, BattleShip dFTNPurinData */
    }
#endif
#if NDS_P2_NNESS
    if (owner_slot == 23u)
    {
        return 0x138u; /* llNNessModelFileID, BattleShip dFTNNessData */
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
#if NDS_P2_CAPTAIN
    if (owner_slot == 4u)
    {
        return NDS_RENDERER_PROFILE_OWNER_CAPTAIN;
    }
#endif
#if NDS_P2_SAMUS
    if (owner_slot == 5u)
    {
        return NDS_RENDERER_PROFILE_OWNER_SAMUS;
    }
#endif
#if NDS_P2_LINK
    if (owner_slot == 6u)
    {
        return NDS_RENDERER_PROFILE_OWNER_LINK;
    }
#endif
#if NDS_P2_PIKACHU
    if (owner_slot == 7u)
    {
        return NDS_RENDERER_PROFILE_OWNER_PIKACHU;
    }
#endif
#if NDS_P2_YOSHI
    if (owner_slot == 8u)
    {
        return NDS_RENDERER_PROFILE_OWNER_YOSHI;
    }
#endif
#if NDS_P2_NESS
    if (owner_slot == 9u)
    {
        return NDS_RENDERER_PROFILE_OWNER_NESS;
    }
#endif
#if NDS_P2_PURIN
    if (owner_slot == 10u)
    {
        return NDS_RENDERER_PROFILE_OWNER_PURIN;
    }
#endif
#if NDS_P2_KIRBY
    if (owner_slot == 11u)
    {
        return NDS_RENDERER_PROFILE_OWNER_KIRBY;
    }
#endif
#if NDS_P2_MMARIO
    if (owner_slot == 12u)
    {
        return NDS_RENDERER_PROFILE_OWNER_MMARIO;
    }
#endif
#if NDS_P2_NMARIO
    if (owner_slot == 13u)
    {
        return NDS_RENDERER_PROFILE_OWNER_NMARIO;
    }
#endif
#if NDS_P2_NFOX
    if (owner_slot == 14u)
    {
        return NDS_RENDERER_PROFILE_OWNER_NFOX;
    }
#endif
#if NDS_P2_NDONKEY
    if (owner_slot == 15u)
    {
        return NDS_RENDERER_PROFILE_OWNER_NDONKEY;
    }
#endif
#if NDS_P2_NSAMUS
    if (owner_slot == 16u)
    {
        return NDS_RENDERER_PROFILE_OWNER_NSAMUS;
    }
#endif
#if NDS_P2_NLINK
    if (owner_slot == 17u)
    {
        return NDS_RENDERER_PROFILE_OWNER_NLINK;
    }
#endif
#if NDS_P2_NYOSHI
    if (owner_slot == 18u)
    {
        return NDS_RENDERER_PROFILE_OWNER_NYOSHI;
    }
#endif
#if NDS_P2_NCAPTAIN
    if (owner_slot == 19u)
    {
        return NDS_RENDERER_PROFILE_OWNER_NCAPTAIN;
    }
#endif
#if NDS_P2_NKIRBY
    if (owner_slot == 20u)
    {
        return NDS_RENDERER_PROFILE_OWNER_NKIRBY;
    }
#endif
#if NDS_P2_NPIKACHU
    if (owner_slot == 21u)
    {
        return NDS_RENDERER_PROFILE_OWNER_NPIKACHU;
    }
#endif
#if NDS_P2_NPURIN
    if (owner_slot == 22u)
    {
        return NDS_RENDERER_PROFILE_OWNER_NPURIN;
    }
#endif
#if NDS_P2_NNESS
    if (owner_slot == 23u)
    {
        return NDS_RENDERER_PROFILE_OWNER_NNESS;
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
#if NDS_R2_FOX_GUN_OVERLAY
    /* File 315 is rendered by the dedicated DS sidecar below.  Remove that
     * one live source root before native-owner admission so Neutral-B does not
     * demote Fox's otherwise unchanged body to the generic interpreter. */
    ndsFighterCollectStripFoxGunSidecar(fp, &collection);
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
        NDS_P2_NESS_FALLBACK(1u);
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
                    slot, fp, root, expected_asset_id,
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
#if NDS_P2_NESS
            if (owner_slot == 9u)
                gNdsP2NessLastPlanResult = (u32)plan_result;
#endif

            if (plan_result != nNDSFighterDrawPlanOk)
            {
                native_owner_enabled = FALSE;
                NDS_P2_NESS_FALLBACK(2u + (u32)plan_result);
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
                NDS_P2_NESS_FALLBACK(6u);
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
            native_owner_root_offsets[i] = ndsRelocNativeRootOffset(loaded, native_dl);
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
                    NDS_P2_NESS_FALLBACK(8u);
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
                 owner_slot, use_low_detail, ndsRelocNativeSourceSize(native_owner_file),
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
                NDS_P2_NESS_FALLBACK(7u);
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
            NDS_P2_NESS_FALLBACK(9u);
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
                NDS_P2_NESS_FALLBACK(10u);
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
            NDS_P2_NESS_FALLBACK(11u);
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
#if NDS_P2_1P_GAME && NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
        /* Packed previews contain native root identities, not an interpreter
         * fallback program. A native admission failure is a visible failure
         * to fix at its owning seam, never an empty replacement display list. */
        if ((loaded != NULL) && (loaded->reserved[0] != 0u) &&
            (native_owner_enabled == FALSE))
        {
            ndsPreviewPackLoadHalt(20u, loaded->reserved[0] - 1u);
        }
#endif

#if NDS_RENDERER_HW_TRIANGLES
        if ((native_owner_enabled != FALSE) && (loaded != NULL) &&
            (loaded->data != NULL) &&
            (loaded->asset_id == expected_asset_id) &&
            (loaded->data_size >= sizeof(*dl)) &&
            ((uintptr_t)dl >= (uintptr_t)loaded->data) &&
            (((uintptr_t)dl - (uintptr_t)loaded->data) <=
             (loaded->data_size - sizeof(*dl))))
        {
            native_root_offset = ndsRelocNativeRootOffset(loaded, dl);
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

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2) && \
    NDS_R2_FOX_GUN_OVERLAY
    /* File 315 is sidecar-owned on DS, independent of whether the fighter body
     * used native production or the fail-closed generic path. The builder
     * itself gates on Fox + gameplay pkind + the live source model-part state,
     * so this is a no-op unless Neutral-B has actually exposed joint 17. */
    if (native_owner_failed == FALSE)
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
#if NDS_P2_NESS
        if ((slot == 0u) && (owner_slot == 9u))
        {
            gNdsP2NessLastNativeOwnerEnabled = (u32)native_owner_enabled;
            gNdsP2NessLastProductionMode = (u32)native_owner_production_mode;
            gNdsP2NessLastHierarchyMode = (u32)native_owner_hierarchy_mode;
            gNdsP2NessLastNoOracle = (u32)no_oracle;
            gNdsP2NessLastDetailedOutput = (u32)detailed_output;
            gNdsP2NessLastPlanHit = (u32)native_owner_plan_hit;
            if (runtime_hardware_triangle_count != 0u)
            {
                gNdsP2NessPositiveDrawCount++;
                if (native_owner_production_attempted != FALSE)
                {
                    gNdsP2NessProductionPositiveDrawCount++;
                }
                else
                {
                    gNdsP2NessNonProductionPositiveDrawCount++;
                    gNdsP2NessNonProductionTriangles +=
                        runtime_hardware_triangle_count;
                }
                if (runtime_hardware_triangle_count != 318u)
                {
                    gNdsP2NessOddDrawCount++;
                    if (gNdsP2NessOddFrame == 0u)
                    {
                        gNdsP2NessOddFrame = gNdsRendererProfileFrameCount;
                        gNdsP2NessOddTriangles = runtime_hardware_triangle_count;
                        gNdsP2NessOddSelected = collection.selected_count;
                        gNdsP2NessOddStatus = (u32)fp->status_id;
                        gNdsP2NessOddMotion = (u32)fp->motion_id;
                        gNdsP2NessOddCamera = (u32)fp->camera_mode;
                        gNdsP2NessOddDetail = (u32)fp->detail_curr;
                        gNdsP2NessOddPacketPredicted =
                            (u32)native_owner_packet_predicted;
                        gNdsP2NessOddProductionAttempted =
                            (u32)native_owner_production_attempted;
                    }
                }
            }
            else if (native_owner_production_attempted != FALSE)
            {
                gNdsP2NessZeroProductionDrawCount++;
            }
        }
#endif
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
        /* P2-3r15: the two totals above are slots 0 and 1, so on a four-
         * distinct-kind roster they cannot say whether Luigi and Donkey Kong
         * drew. One bit per slot that emitted triangles can. */
        if (runtime_hardware_triangle_count != 0u)
        {
            gNdsFighterDLAllDrawSlotTriangleMask |= 1u << (slot & 3u);
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
     * not a proof switch: the consumer gate at :3544-3547 enters the native
     * owner only when it is set, so this chooses the renderer (R4g). The battle
     * present already sets it around its whole draw
     * (reloc_backend_movement.c:13234/:13266), which is why the match got the
     * native owner and VS Results -- reaching this function through the scene
     * draw with no bracket on the path -- got the generic DL interpreter.
     *
     * READ THAT LAST SENTENCE AS PAST TENSE: it is what R4c FIXED, not an open
     * defect. Both paths converge on this one function --
     * ndsFighterDisplayContractSubmitStageFighters at :4509 for the battle and
     * ftDisplayMainProcDisplay via reloc_backend_fighter_display_seam.c:86 for
     * Results -- so the bracket below covers both. R4h then made it
     * save/restore, and the landing measured the Results fighter draw dropping
     * 1,449,776 -> 364,784. A 2026-09-04 law-8 survey read the sentence as
     * present tense and filed a phantom work item, which cost an
     * implementation pass; the three file:line citations above were all stale
     * after ad6caa9d829 split this adapter, so there was no cheap way to check
     * it. They are corrected here for that reason.
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
