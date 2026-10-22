typedef struct NDSFighterDisplayContractEvent {
    DObj *dobj;
    DObj *matrix_dobj;
    DObj *material_dobj;
    const Gfx *dl;
} NDSFighterDisplayContractEvent;

_Static_assert(NDS_NATIVE_KIRBY_HAT_BATTLE_SLOTS == GMCOMMON_PLAYERS_MAX,
               "Kirby hat residency must cover every battle player slot");

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
    u32 selected_overflow_count;
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
    u32 skip_empty_dl;          /* P4 donor fighter: see SelectDL */
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
/* 1P Intro transient-submit flag. Tentative definition here so the memo-arm
 * gate in the capture path (far above the transient block) can read it;
 * the transient block below owns every write. TRUE only while a transient
 * Intro submit is inside capture/production. */
static sb32 sNdsIntroTransientActive;

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


static u32 ndsFighterDisplayContractPackColor(u8 r, u8 g, u8 b, u8 a)
{
    return ((u32)r << 24) | ((u32)g << 16) | ((u32)b << 8) | (u32)a;
}


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
        NDS_DIAG(gNdsFighterDisplayContractLightCount += count);
    }
}

#if NDS_1P_INTRO_BAKE
/* Set by the 1P intro's still bake around its own draw only
 * (battleship_sc1pintro.c, in the frontend overlay, which the resident
 * renderer never names): the light under a camera whose view the bake moved
 * into the projection, from view into world space. */
void (*gNdsFighterDisplayLightHook)(Light *light);
#endif

void ndsFighterDisplayContractSetLight(const Light *light, u32 slot)
{
    if ((light == NULL) || (slot != 1u))
    {
        return;
    }
    sNdsFighterDisplayCurrentLight = *light;
#if NDS_1P_INTRO_BAKE
    if (gNdsFighterDisplayLightHook != NULL)
    {
        gNdsFighterDisplayLightHook(&sNdsFighterDisplayCurrentLight);
        light = &sNdsFighterDisplayCurrentLight;
    }
#endif
    sNdsFighterDisplayCurrentLightValid = TRUE;
    if (sNdsFighterDisplayContract.active != 0u)
    {
        sNdsFighterDisplayContract.light = *light;
        sNdsFighterDisplayContract.light_valid = TRUE;
        NDS_DIAG(gNdsFighterDisplayContractLightDirectionCount++);
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
        NDS_DIAG(gNdsFighterDisplayContractBoundsPassCount++);
    }
    else
    {
        gNdsFighterDisplayContractBoundsFailCount++;
    }
    return is_in_bounds;
}

/* BattleShip ftparam.c:2421-2439 in fixed point (P2-2p8, 2026-10-05; owner:
 * "Software floating point should not exist, fixed point only"; ruling D13).
 * The camera matrix's rotation rows are below 8 in magnitude (look-at times
 * the perspective's ~1-4 scales) and its translation row below 2^19, so they
 * take Q28 and Q12; positions take Q12 (saturated at 2^16 units, past every
 * blast zone); the three dot products accumulate at Q40 in 64 bits, the
 * source's |w| >= 0.1 clamp is applied there, and x / w and y / w come from
 * the hardware divider (both scaled under 2^40 first, so the Q22 quotient
 * fits). The integer `vscale / 4` multiplies the quotient as in the source. A
 * matrix element outside those ranges never occurs with the game camera; it
 * would saturate rather than wrap. The converted matrix is kept while its
 * twelve source words are unchanged (once a frame for the game camera, which
 * every caller passes). */
#include <stdint.h>
#include <nds/nds_p4.h>
#include <nds/nds_r2_collision_mtx.h>
#include <nds/nds_r2_hwmath_unit.h>

static const u32 sNdsProjectCols[3] = { 0u, 1u, 3u };
static u32 sNdsProjectSrc[4][3];
static int32_t sNdsProjectQ[4][3];
static u32 sNdsProjectValid;

static __attribute__((noinline, target("arm"))) int32_t
ndsProjectToFixed(f32 value, u32 bits_frac, int32_t limit)
{
    int32_t q = ndsR2CollisionF32ToFixed(value, bits_frac);
    u32 bits;

    __builtin_memcpy(&bits, &value, sizeof(bits));
    if ((q == NDS_R2_COLLISION_F32_OVERFLOW) || (q >= limit) || (q <= -limit))
    {
        return ((bits & 0x80000000u) != 0u) ? -limit : limit;
    }
    return q;
}

/* A rotation-row cell at Q28. ndsR2CollisionF32ToFixed declines a left shift
 * of 7, which is every |v| in [4, 8) at Q28, although it fits; the old code
 * saturated those to 8. That is the perspective scale itself once the field of
 * view narrows under ~28 degrees (cot 12.5 = 4.52 in Master Hand's intro, cot
 * 14 = 4.01 in the close-up entries): the projected point doubled, the bounds
 * test failed and ftDisplayMainProcDisplay culled the fighter as magnified
 * (owner, 2026-10-06: "MH has brief invisible frames"; close-up entry
 * fighters "become invisible"). A float in [4, 8) is a multiple of 2^-21,
 * exact at Q27, so it converts there and doubles; below 4 nothing changes. */
static int32_t __attribute__((target("arm")))
ndsProjectRowToQ28(f32 value)
{
    int32_t q = ndsR2CollisionF32ToFixed(value, 28u);

    if (q == NDS_R2_COLLISION_F32_OVERFLOW)
    {
        q = ndsProjectToFixed(value, 27u, INT32_C(1) << 30);
        if ((q == (INT32_C(1) << 30)) || (q == -(INT32_C(1) << 30)))
        {
            return (q < 0) ? -INT32_MAX : INT32_MAX;
        }
        return q * 2;
    }
    return q;
}

static int64_t __attribute__((target("arm")))
ndsProjectRatioQ22(int64_t num, int64_t den)
{
    const uint64_t an = (num < 0) ? (uint64_t)-num : (uint64_t)num;
    const uint64_t ad = (den < 0) ? (uint64_t)-den : (uint64_t)den;
    const uint64_t m = (an > ad) ? an : ad;
    u32 shift = 0u;

    if (m >= (UINT64_C(1) << 40))
    {
        shift = (u32)(64 - __builtin_clzll(m)) - 40u;
    }
    num >>= shift;
    den >>= shift;
    if (den == 0)
    {
        return 0;
    }
    return ndsR2HwMathDivideFast(num * (INT64_C(1) << 22), den);
}

/* The projection's two results before they leave as floats: Q22 units of
 * the source's dist_x / dist_y, from a Q12 point (saturated like the float
 * entry's). For integer consumers (the HUD's OAM anchors). */
void __attribute__((target("arm")))
ndsProjectToViewportQ22(CObj *cobj, Mtx44f matrix, const int32_t p_q12[3],
                        int64_t *dist_x_q22, int64_t *dist_y_q22)
{
    const int64_t clamp = INT64_C(109951162778); /* 0.1 at Q40 */
    int64_t acc[3];
    int32_t p[3];
    u32 row;
    u32 col;
    u32 same = sNdsProjectValid;

    for (row = 0u; row < 4u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            /* A union read: the 4-byte __builtin_memcpy here, at a variable
             * column, compiled to a real memcpy call (12 a projection). */
            union
            {
                f32 f;
                u32 u;
            } cell = { matrix[row][sNdsProjectCols[col]] };
            const u32 bits = cell.u;

            if (bits != sNdsProjectSrc[row][col])
            {
                same = 0u;
                sNdsProjectSrc[row][col] = bits;
            }
        }
    }
    if (same == 0u)
    {
        for (row = 0u; row < 4u; row++)
        {
            for (col = 0u; col < 3u; col++)
            {
                const f32 v = matrix[row][sNdsProjectCols[col]];

                sNdsProjectQ[row][col] = (row < 3u) ?
                    ndsProjectRowToQ28(v) :
                    ndsProjectToFixed(v, 12u, INT32_MAX);
            }
        }
        sNdsProjectValid = 1u;
    }
    for (col = 0u; col < 3u; col++)
    {
        const int32_t limit = INT32_C(1) << 28;

        p[col] = (p_q12[col] >= limit) ? limit :
                 ((p_q12[col] <= -limit) ? -limit : p_q12[col]);
    }
    for (col = 0u; col < 3u; col++)
    {
        acc[col] = ((int64_t)sNdsProjectQ[0][col] * p[0]) +
                   ((int64_t)sNdsProjectQ[1][col] * p[1]) +
                   ((int64_t)sNdsProjectQ[2][col] * p[2]) +
                   ((int64_t)sNdsProjectQ[3][col] << 28);
    }
    /* acc[2] is w: |w| < 0.1 becomes +-0.1, a zero w +0.1. */
    if ((acc[2] < clamp) && (acc[2] > -clamp))
    {
        acc[2] = (acc[2] < 0) ? -clamp : clamp;
    }
    *dist_x_q22 = (int64_t)(cobj->viewport.vp.vscale[0] / 4) *
        ndsProjectRatioQ22(acc[0], acc[2]);
    *dist_y_q22 = (int64_t)(cobj->viewport.vp.vscale[1] / 4) *
        ndsProjectRatioQ22(acc[1], acc[2]);
}

void __attribute__((target("arm")))
ndsProjectToViewport(CObj *cobj, Mtx44f matrix, Vec3f *pos, f32 *dist_x,
                     f32 *dist_y)
{
    int32_t p[3];
    int64_t dx;
    int64_t dy;

    p[0] = ndsProjectToFixed(pos->x, 12u, INT32_C(1) << 28);
    p[1] = ndsProjectToFixed(pos->y, 12u, INT32_C(1) << 28);
    p[2] = ndsProjectToFixed(pos->z, 12u, INT32_C(1) << 28);
    ndsProjectToViewportQ22(cobj, matrix, p, &dx, &dy);
    *dist_x = ndsR2CollisionFixedToF32(dx, 22u);
    *dist_y = ndsR2CollisionFixedToF32(dy, 22u);
}

void ndsFighterDisplayContractProjectTarget(CObj *cobj,
                                            Mtx44f matrix,
                                            Vec3f *pos,
                                            f32 *dist_x,
                                            f32 *dist_y)
{
    if ((cobj == NULL) || (pos == NULL) || (dist_x == NULL) ||
        (dist_y == NULL))
    {
        return;
    }
    /* BattleShip ftparam.c:2421-2439, used by fighter magnify culling: the
     * fixed-point projection func_ovl2_800EB924 uses (2026-10-05,
     * battle_playable_compat_stubs.c). */
    ndsProjectToViewport(cobj, matrix, pos, dist_x, dist_y);
}

/* TRUE when the list ends before any vertex, triangle, rectangle or branch
 * command: it sets state and draws nothing. */
static sb32 ndsFighterDisplayListDrawsNothing(const Gfx *dl)
{
    const u32 *w = (const u32 *)dl;
    u32 i;

    for (i = 0u; i < 32u; i++, w += 2)
    {
        switch (w[0] >> 24)
        {
        case 0xdfu: /* G_ENDDL */
            return TRUE;
        case 0x01u: /* G_VTX */
        case 0x05u: /* G_TRI1 */
        case 0x06u: /* G_TRI2 */
        case 0x07u: /* G_QUAD */
        case 0xdeu: /* G_DL */
        case 0xe4u: /* G_TEXRECT */
        case 0xf6u: /* G_FILLRECT */
            return FALSE;
        default:
            break;
        }
    }
    return FALSE;
}

void ndsFighterDisplayContractSelectDL(const Gfx *dl)
{
    NDSFighterDisplayContractEvent *event;
    const FTParts *parts;
    sb32 is_accessory;

    if ((sNdsFighterDisplayContract.active == 0u) || (dl == NULL))
    {
        return;
    }
    /* A Remix donor model can give a joint a list that is only G_ENDDL (its
     * geometry rides in a neighbouring joint's list). It draws nothing, and
     * the donor owner (generate_nds_native_owners.py, P4 donor mode) has no
     * root for it, so it is no event either. The joint's matrix and material
     * are consumed as an ordinary list would consume them. The list is
     * ENDDL plus 0; a character-select preview pack's root cell is ENDDL plus
     * the root's nonzero original offset (scripts/p4/p4_preview_pack.py), and
     * must stay an event. */
    if ((sNdsFighterDisplayContract.skip_empty_dl != 0u) &&
        ((((const u32 *)dl)[0] >> 24) == 0xdfu) &&
        (((const u32 *)dl)[1] == 0u))
    {
        sNdsFighterDisplayContract.matrix_ready = FALSE;
        sNdsFighterDisplayContract.material_ready = FALSE;
        return;
    }
    /* EXTRA's Meta Knight wears, in costumes 1-3, a costume accessory
     * (Jigglypuff's bow mechanism, ftDisplayMainDrawAccessory) whose list
     * only re-sets the lights his joint 6 list sets again: it draws nothing
     * and is no event. It leaves the joint's matrix for that list, as an
     * accessory does (below). */
    if ((sNdsFighterDisplayContract.skip_empty_dl != 0u) &&
        (sNdsFighterDisplayContract.current_dobj != NULL) &&
        ((parts = ftGetParts(sNdsFighterDisplayContract.current_dobj)) != NULL) &&
        (parts->gobj != NULL) && (DObjGetStruct(parts->gobj) != NULL) &&
        (DObjGetStruct(parts->gobj)->dl == dl) &&
        (ndsFighterDisplayListDrawsNothing(dl) != FALSE))
    {
        return;
    }
    if (sNdsFighterDisplayContract.event_count >=
        NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED)
    {
        /* Only post-matrix events become selected fighter roots. Count those
         * separately so an exhausted capture table remains visible to the
         * 4a high-water witness instead of silently looking exactly full. */
        if ((sNdsFighterDisplayContract.matrix_ready != 0u) &&
            (sNdsFighterDisplayContract.current_dobj != NULL))
        {
            sNdsFighterDisplayContract.selected_overflow_count++;
        }
        return;
    }
    /* A costume accessory (ftDisplayMainDrawAccessory, ftdisplaymain.c:723)
     * is the second list drawn under ONE prepared joint matrix: Purin's comes
     * before the joint's own list (:776), Pikachu's after it (:819). Bound
     * like any other list, the second of the pair found the matrix consumed
     * and became the next joint's pre-matrix list: Purin's body was dropped
     * (her bow took its root and the owner declined), Pikachu's hat was never
     * drawn. The accessory belongs to the joint whose parts own it, and it
     * leaves the matrix for the joint's own list. */
    is_accessory =
        ((sNdsFighterDisplayContract.current_dobj != NULL) &&
         ((parts = ftGetParts(sNdsFighterDisplayContract.current_dobj)) !=
          NULL) &&
         (parts->gobj != NULL) && (DObjGetStruct(parts->gobj) != NULL) &&
         (DObjGetStruct(parts->gobj)->dl == dl)) ? TRUE : FALSE;
    event = &sNdsFighterDisplayContract.events[
        sNdsFighterDisplayContract.event_count++];
    event->dl = dl;
    event->dobj = ((sNdsFighterDisplayContract.matrix_ready != 0u) ||
                   (is_accessory != FALSE)) ?
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
    if (is_accessory == FALSE)
    {
        sNdsFighterDisplayContract.matrix_ready = FALSE;
    }
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
    if ((sNdsFighterDisplayContract.pending_event < 0) &&
        (sNdsFighterDisplayContract.event_count > 0u))
    {
        /* FTParts flags case 1 issues dls[0] BEFORE this prep
         * (ftdisplaymain.c:789-798). When the previously prepared joint
         * drew nothing, matrix_ready was still set and SelectDL credited
         * that pre-matrix list to the previous joint as an ordinary root.
         * Identity settles it: the last event's list is this joint's own
         * dls[0], so it is this joint's pre list bound to the parent. */
        const FTParts *parts = ftGetParts(dobj);
        const NDSFighterDisplayContractEvent *last =
            &sNdsFighterDisplayContract.events[
                sNdsFighterDisplayContract.event_count - 1u];

        if ((parts != NULL) && ((parts->flags & 0xFu) == 1u) &&
            (dobj->dls != NULL) && (last->dl == dobj->dls[0]))
        {
            sNdsFighterDisplayContract.pending_event =
                (s32)sNdsFighterDisplayContract.event_count - 1;
        }
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
/* P2-2p8 Phase 1 slice 3: a per-slot count of memo fills, and whether this
 * draw's replay preambles are the memo slot's content (a hit, or the fill that
 * just copied them). The lean path proves a slot's preambles once per fill
 * instead of once per draw (renderer_fighter_lean.c). */
static u32 sNdsFtrLeanMemoFill[GMCOMMON_PLAYERS_MAX];
static u32 sNdsFtrLeanMemoFromSlot;

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

/* A fighter was made in this slot, or the slot's display registration
 * changed. The memo and the draw plan hold the previous fighter's DObjs, and
 * the replacement can take the very same FTStruct, GObj and root DObj (the
 * free lists are LIFO), so the memo key cannot tell the two apart. A status
 * change no longer invalidates them unless it moved the topology
 * (battleship_ftmain.c), which also covered a new fighter's first status, so
 * the lifetime boundary does it here. Owner r70: the 4P VS CSS froze -- a
 * rebuilt Mario preview replayed the old one's freed DObjs (no MObjs,
 * validate reject 7) into ndsPreviewPackLoadHalt(20). */
void ndsFighterRendererInvalidateSlotDrawCaches(u32 slot)
{
    if (slot >= GMCOMMON_PLAYERS_MAX)
    {
        return;
    }
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
    NDS_DIAG(gNdsFtrDrawMemoBoundary++);
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
            NDS_DIAG(gNdsFtrDrawMemoInvalidations++);
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
    sNdsFtrLeanMemoFromSlot = 0u;
    if (sNdsFtrDrawMemoState != 2u)
    {
        NDS_DIAG(gNdsFtrDrawMemoBypass++);
        sNdsFtrDrawMemoState = 0u;
        return;
    }
    slot = &sNdsFtrDrawMemo[sNdsFtrDrawMemoSlotIndex];
    sNdsFtrLeanMemoFromSlot = 1u;
    if ((sNdsFtrDrawMemoHit == 0u) &&
        (sNdsFtrDrawMemoSlotIndex < GMCOMMON_PLAYERS_MAX))
    {
        sNdsFtrLeanMemoFill[sNdsFtrDrawMemoSlotIndex]++;
    }
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
        NDS_DIAG(gNdsFtrDrawMemoHits++);
        NDS_DIAG(gNdsFtrDrawMemoReplayEvents += n);
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
    NDS_DIAG(gNdsFtrDrawMemoFills++);
    sNdsFtrDrawMemoState = 0u;
}

/* K02/P04/J01 facing witness (r25, 2026-09-21).
 *
 * The owner's report is facing-dependent: Kirby's face/body seam is gone facing
 * LEFT, present facing RIGHT, and gone facing RIGHT inside the ledge-balance
 * pose. No material, palette or clamp state can do that, so every shade sample
 * the renderer records has to carry the facing that produced it or two samples
 * cannot be compared at all.
 *
 * `fp->lr` is the source's own facing field, -1 left / 0 centre / +1 right.
 * NOTE for whoever reads this next: it is NOT a mirror. ftmain.c:4477 turns it
 * into `joints[nFTPartsJointTopN]->rotate.vec.f.y = fp->lr * 90 degrees`, a
 * pure rotation, and no lr-driven negative scale exists in decomp/ or
 * src/import/. A facing change is a 180 degree Y rotation of the whole model
 * and nothing else.
 *
 * Deliberately EVENT-LOCAL: both values are overwritten by every captured
 * fighter and never accumulated, so a fighter that stopped drawing cannot leave
 * a stale facing behind for the next reader. `Writes` is a delta counter, not a
 * flag. Defined here, in the only translation unit that has an FTStruct, and
 * unconditionally so the renderer-side externs resolve in every configuration
 * including NDS_R2_FIGHTER_HW_LIGHT=0 builds. */

volatile s32 gNdsR2FighterFacingLr;
volatile u32 gNdsR2FighterFacingSlot;
volatile u32 gNdsR2FighterFacingWrites;

/* P2-2p8 Phase 1 slice 5 (phase1-spec.md 2.1 stage 1): the head's camera
 * LookAt below is a camera-only input. The slice 5 attribution ROM priced
 * "capture setup + LookAt" at 6.7-7.5K ticks per fighter, every fighter,
 * every frame, for the same camera. It is now called only when it could
 * produce something new: the CObj differs from the last call's, its inputs
 * (eye, at, up, perspective, the two camera-path words) hash differently, or
 * its outputs (gGMCameraMatrix, the look-at, the perspective norm) no longer
 * hash as that call left them -- otherwise the call would rewrite the very
 * values already there, and the reuse is exact by construction. The reuse
 * keeps the call's graphics-heap footprint (one Mtx below camera path level
 * 3, which the capture's own heap restore releases), so every later head
 * allocation lands where it did; it does not rewrite that Mtx, which only
 * sGCMatrixProjectL names and nothing on this port reads (battleship_gmcamera.c,
 * W2b). The slice 5 draw memo showed why the footprint matters: the head's
 * scene Light is copied whole -- uninitialised colour bytes included -- into
 * the memo key, so moving it 64 B cost 124 extra memo fills a match.
 * gNdsFtrLeanSlow bit 128 calls it every time (the slice 4 form, same ROM). */
extern Mtx44f gGMCameraMatrix;
extern Mtx *sGCMatrixProjectL;
const LookAt *ndsR2CameraCurrentLookAt(void);
static const CObj *sNdsFtrLookAtCObj;
static u32 sNdsFtrLookAtIn;
static u32 sNdsFtrLookAtOut;

/* Out of line on purpose: every call site passes a constant count, and the
 * inlined, unrolled copies cost ~500 B of main RAM (shipping static RAM must
 * not grow in slice 5). */
static u32 __attribute__((noinline))
ndsFtrLookAtMixWords(u32 h, const void *data, u32 words)
{
    const u32 *w = (const u32 *)data;
    u32 i;

    for (i = 0u; i < words; i++)
    {
        h = (h ^ w[i]) * 16777619u;
    }
    return h;
}

static u32 __attribute__((noinline)) ndsFtrLookAtInputs(const CObj *cobj)
{
    /* eye, at, up: CObjVec's nine consecutive floats; fovy, aspect, near,
     * far, scale: GCPersp's five (the same words, in the same order, as one
     * field at a time). */
    u32 h = ndsFtrLookAtMixWords(2166136261u, &cobj->vec.eye, 9u);

    _Static_assert(__builtin_offsetof(CObjVec, up) ==
                       __builtin_offsetof(CObjVec, eye) + 6u * sizeof(f32),
                   "LookAt inputs: eye, at, up must be consecutive");
    _Static_assert(__builtin_offsetof(GCPersp, scale) ==
                       __builtin_offsetof(GCPersp, fovy) + 4u * sizeof(f32),
                   "LookAt inputs: fovy .. scale must be consecutive");
    return ndsFtrLookAtMixWords(h, &cobj->projection.persp.fovy, 5u);
}

static u32 __attribute__((noinline)) ndsFtrLookAtOutputs(const CObj *cobj)
{
    u32 h = 2166136261u;

    h = ndsFtrLookAtMixWords(h, gGMCameraMatrix,
                             (u32)(sizeof(gGMCameraMatrix) / sizeof(u32)));
    h = ndsFtrLookAtMixWords(h, ndsR2CameraCurrentLookAt(),
                             (u32)(sizeof(LookAt) / sizeof(u32)));
    return (h ^ (u32)cobj->projection.persp.norm) * 16777619u;
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
    sNdsFighterDisplayContract.selected_overflow_count = 0u;
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
    sNdsFighterDisplayContract.skip_empty_dl =
        ((fp != NULL) && (fp->nds_p4_content != 0u)) ? TRUE : FALSE;
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
        /* Slice 5: only when it could write something new (see
         * ndsFtrLookAtInputs above). */
        CObj *look_cobj = CObjGetStruct(gGMCameraGObj);
        u32 look_in = ndsFtrLookAtInputs(look_cobj);

        /* gmCameraLookAtFuncMatrix is a pure matrix writer (no GBI), so an
         * unchanged camera may skip it. */
        if ((look_cobj != sNdsFtrLookAtCObj) || (look_in != sNdsFtrLookAtIn) ||
            ((NDS_FTR_LEAN_SLOW_WORD() & NDS_FTR_LEAN_SLOW_HEAD_LOOKAT) != 0u) ||
            (ndsFtrLookAtOutputs(look_cobj) != sNdsFtrLookAtOut))
        {
            gmCameraLookAtFuncMatrix(NULL, look_cobj, gSYTaskmanDLHeads);
            sNdsFtrLookAtCObj = look_cobj;
            sNdsFtrLookAtIn = look_in;
            sNdsFtrLookAtOut = ndsFtrLookAtOutputs(look_cobj);
        }
        else
        {
            /* The call's graphics-heap footprint, kept: it takes one Mtx for
             * its projection before anything the head allocates, so the
             * head's own allocations (the scene Light the display contract
             * copies whole, uninitialised colour bytes included, into the
             * draw memo's key) land where they always did. The Mtx itself is
             * not rewritten: nothing on this port reads sGCMatrixProjectL
             * (battleship_gmcamera.c, W2b). */
            sGCMatrixProjectL = (Mtx *)gSYTaskmanGraphicsHeap.ptr;
            gSYTaskmanGraphicsHeap.ptr =
                (void *)((Mtx *)gSYTaskmanGraphicsHeap.ptr + 1);
        }
    }
    sNdsFtrDrawMemoFp = fp;
    sNdsFtrDrawMemoGObj = fighter_gobj;
    sNdsFtrDrawMemoHit = 0u;
    /* See the definitions above: the renderer's shade and light-matrix
     * witnesses are unreadable without the facing that produced them. */
    gNdsR2FighterFacingLr = (fp != NULL) ? fp->lr : 0;
    gNdsR2FighterFacingSlot =
        (fp != NULL) ? (u32)fp->nds_slot : 0xffffffffu;
    NDS_DIAG(gNdsR2FighterFacingWrites++);
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
         (sNdsIntroTransientActive == FALSE) &&
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
            /* A pre-matrix list (FTParts flags case 1, dls[0], recorded
             * before gcPrepDObjMatrix and bound to the parent) is not a
             * root: the native bake welds its parent-bound vertex loads
             * into the joint's post-list root (OWNER_DL_PAIR_MODE), so the
             * collection carries one root per joint, as the bake does. */
            if (sNdsFighterDisplayReplayEvents[i].matrix_dobj !=
                sNdsFighterDisplayReplayEvents[i].dobj)
            {
                continue;
            }
            collection->dobjs[collection->selected_count] =
                sNdsFighterDisplayReplayEvents[i].dobj;
            collection->indices[collection->selected_count] = i;
            collection->selected_count++;
            collection->total_count++;
        }
        collection->total_count +=
            sNdsFighterDisplayContract.selected_overflow_count;
        return;
    }
    ndsFighterCollectAllDObjsWithDLRecursive(root, collection,
                                              &traversal_index);
}

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

/* Fox is not the only fighter that holds this source part.
 *
 * Kirby's Fox copy selects the SAME descriptor on the SAME joint:
 * dKirbyMain_modelparts_container[13] is dKirbyMain_modelparts_desc_0x3C4,
 * whose two rows are both dFoxUnknown_DL, and container index 13 is joint
 * 13 + nFTPartsJointCommonStart = 17 -- Fox's hold joint exactly. (The same
 * arithmetic puts Kirby's CopyLink boomerang descriptor, container index 8,
 * on the hidden joint 12 its hidden-part row inserts.)
 *
 * Until 2026-09-21 the strip below was pinned to nFTKindFox, so Kirby's
 * copied blaster reached ndsFighterDrawPlanResolve as a foreign root, failed
 * the asset gate at stage 2 clause 4 -- measured: owner 11, expected 0x148,
 * offending 0x13b root 0x3F0, index 5 of 8 -- and a packed fighter that
 * declines is ndsPreviewPackLoadHalt(20). That halt on the firing frame is
 * what "firing the pistol crashes" was. With the root stripped the live
 * vector is the seven roots the existing CopyTransition program already
 * covers, and the sidecar draws the pistol from the same baked file-315 mesh
 * Fox uses.
 *
 * Kirby must actually be holding Fox's copy: passive_vars.kirby.copy_id is
 * the victim kind ftKirbySpecialN stored, and the source reads the same field
 * to pick the copy hat. A Kirby carrying anybody else's copy cannot reach
 * this descriptor, so the modelpart check alone would be a weaker pin.
 *
 * Keeping the strip is not optional -- it is the halt fix, not the
 * presentation. The DRAW was briefly withdrawn for Kirby on 2026-09-21 and is
 * restored: the owner's "get rid of the pistol shot VFX" meant the muzzle
 * flash landing on Kirby's body, which is suppressed in
 * battleship_fox_blaster.c, not the weapon model, which is source-correct. */
static sb32 ndsFighterHoldsFoxGunSource(const FTStruct *fp)
{
    if (fp == NULL)
    {
        return FALSE;
    }
    if (fp->fkind == nFTKindFox)
    {
        return TRUE;
    }
    if ((fp->fkind == nFTKindKirby) &&
        (fp->passive_vars.kirby.copy_id == (s32)nFTKindFox))
    {
        return TRUE;
    }
    /* P4 S8: a power's weapon on Kirby's joint 17 (Remix's extended special
     * part: Wolf's gun, Marth's and Roy's swords, Lanky's gun) is selected
     * through the same source pistol part, so it leaves the body the same
     * way and draws beside it (ndsFighterDrawFoxGunSidecar). */
    if ((fp->fkind == nFTKindKirby) && (fp->nds_p4_kirby_copy != 0u))
    {
        return TRUE;
    }
    return FALSE;
}

/* nds_renderer_native_common.c: a content's joint-17 model part that lives
 * outside its model file (Wolf's gun in WOLF_WEAPON), drawn as a P4 root. */
sb32 ndsRendererEntryEffectP4Sidecar(u32 content, u32 joint,
                                     u32 *owner_asset_id, u32 *root_offset);

/* The file the held pistol's list comes from: Fox's file 315, or a P4
 * content's own gun file (Wolf's runs Fox's code with his own gun). */
static u32 ndsFighterFoxGunSourceAsset(const FTStruct *fp)
{
    u32 asset;
    u32 offset;

    if ((fp->nds_p4_content != 0u) &&
        (ndsRendererEntryEffectP4Sidecar(fp->nds_p4_content,
                                         NDS_FOX_GUN_HOLD_JOINT,
                                         &asset, &offset) != FALSE))
    {
        return asset;
    }
    (void)fp;
    return NDS_FOX_GUN_SOURCE_ASSET_ID;
}

static void ndsFighterCollectStripFoxGunSidecar(
    const FTStruct *fp, NDSFighterDLAllDrawCollection *collection)
{
    DObj *gun_joint;
    u32 read_index;
    u32 write_index = 0u;
    u32 gun_asset;

    if ((fp == NULL) || (collection == NULL) ||
        (ndsFighterHoldsFoxGunSource(fp) == FALSE) ||
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
    gun_asset = ndsFighterFoxGunSourceAsset(fp);

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
            (loaded->asset_id == gun_asset);

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

/* Joint 17 shows the held gun's list: the strip's own test above, on the live
 * joint. A Remix content runs its parent's code, so a Fox-parent content
 * passes the parent's gate (Fox, part >= 0) with a hand of its own on that
 * joint at part 0 for the whole match: Peach held Fox's pistol standing and
 * walking (owner, 2026-10-12, "Peaches tennis racket ... i can see it during
 * idle and walk/running"). */
static sb32 ndsFighterJoint17ShowsGun(const FTStruct *fp)
{
    const DObj *joint = fp->joints[NDS_FOX_GUN_HOLD_JOINT];
    NDSRelocLoadedFile *loaded;

    if ((joint == NULL) || (joint == DOBJ_PARENT_NULL) || (joint->dl == NULL))
    {
        return FALSE;
    }
    loaded = ndsRelocFindLoadedFileContaining(joint->dl, sizeof(*joint->dl));
    return ((loaded != NULL) &&
            (loaded->asset_id == ndsFighterFoxGunSourceAsset(fp))) ? TRUE : FALSE;
}

/* The held pistol beside the body, after either body path: Fox's baked
 * file-315 mesh at joint 17, or, for a P4 content with its own gun (Wolf),
 * that gun's generated root at the same joint. Each builder gates on the
 * gameplay owner and the live joint-17 model part. */
static void ndsFighterDrawFoxGunSidecar(FTStruct *fp)
{
    CObj *cobj = (gGCCurrentCamera != NULL) ?
        CObjGetStruct(gGCCurrentCamera) : NULL;
    NDSRendererMatrix20p12 sidecar_world;
    u32 asset;
    u32 offset;

    /* A Kirby holding a content's power shows the weapon Remix gives him for
     * it (KirbyHats' special parts: Marth's and Roy's swords, Lanky's gun,
     * their roots marked 0x80 on the joint) or the content's own (Wolf's
     * gun); a Fox clone's power with neither (Falco's) keeps Fox's pistol. */
    u32 content = fp->nds_p4_content;
    sb32 found = FALSE;

    if ((content != 0u) && (ndsFighterJoint17ShowsGun(fp) == FALSE))
    {
        return;
    }
    if ((content == 0u) && (fp->fkind == nFTKindKirby))
    {
        content = fp->nds_p4_kirby_copy;
        found = ((content != 0u) &&
                 (ndsRendererEntryEffectP4Sidecar(content, NDS_FOX_GUN_HOLD_JOINT | 0x80u,
                                                  &asset, &offset) != FALSE)) ? TRUE : FALSE;
    }
    if ((found == FALSE) && (content != 0u))
    {
        found = ndsRendererEntryEffectP4Sidecar(content, NDS_FOX_GUN_HOLD_JOINT,
                                                &asset, &offset);
    }
    if (found != FALSE)
    {
        NDSRendererMatrix20p12 projection;

        if (ndsRendererAdapterBuildFoxGunJointMatrices(
                fp, cobj, &projection, &sidecar_world) != FALSE)
        {
            (void)ndsRendererAdapterSubmitP4SidecarRoot(
                fp->joints[NDS_FOX_GUN_HOLD_JOINT], asset, offset,
                &projection, &sidecar_world);
        }
        return;
    }
    if ((fp->fkind == nFTKindKirby) && (fp->nds_p4_kirby_copy != 0u) &&
        (fp->passive_vars.kirby.copy_id != (s32)nFTKindFox))
    {
        return;
    }
    if (ndsRendererAdapterBuildFoxGunJointMtx(fp, cobj, &sidecar_world) != FALSE)
    {
        (void)ndsRendererSubmitFoxGun(&sidecar_world);
    }
}

/* A Kirby holding a custom power (scripts/p4/p4_kirby.py CUSTOM_POWERS,
 * owner 2026-10-12): its hat, a list cut from the content's model or made on
 * its material (p4_kirby_custom_hats.py), drawn at joint 6 over his plain
 * head as the powers' weapons are at joint 17 -- and only over it: Stone and
 * the inhale faces put another part on joint 6. */
#define NDS_KIRBY_HAT_SIDECAR_JOINT 6u
static void ndsFighterDrawKirbyHatSidecar(FTStruct *fp)
{
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 world;
    CObj *cobj;
    u32 asset;
    u32 offset;

    if ((fp == NULL) || (fp->fkind != nFTKindKirby) || (fp->nds_p4_kirby_copy == 0u) ||
        (fp->modelpart_status[NDS_KIRBY_HAT_SIDECAR_JOINT -
                              nFTPartsJointCommonStart].modelpart_id_curr != 0) ||
        (ndsRendererEntryEffectP4Sidecar(fp->nds_p4_kirby_copy,
                                         NDS_KIRBY_HAT_SIDECAR_JOINT | 0x80u,
                                         &asset, &offset) == FALSE))
    {
        return;
    }
    cobj = (gGCCurrentCamera != NULL) ? CObjGetStruct(gGCCurrentCamera) : NULL;
    if (ndsRendererAdapterBuildKirbyHatJointMatrices(
            fp, NDS_KIRBY_HAT_SIDECAR_JOINT, cobj, &projection, &world) != FALSE)
    {
        (void)ndsRendererAdapterSubmitP4SidecarRoot(
            fp->joints[NDS_KIRBY_HAT_SIDECAR_JOINT], asset, offset, &projection, &world);
    }
}

/* Meta Knight's tornado around a Kirby spinning with his power (owner
 * BUGS.md 2026-10-12): the custom hat file's second list, a sidecar root on
 * Kirby's root joint, which turns with the spin (nds_p4_metaknight.c). */
#define NDS_KIRBY_TORNADO_SIDECAR_JOINT 0u
static void ndsFighterDrawKirbyTornadoSidecar(FTStruct *fp)
{
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 world;
    CObj *cobj;
    u32 asset;
    u32 offset;

    if ((fp == NULL) || (fp->fkind != nFTKindKirby) || (fp->nds_p4_kirby_copy == 0u) ||
        (ndsP4MetaknightKirbyTornado(fp) == FALSE) ||
        (ndsRendererEntryEffectP4Sidecar(fp->nds_p4_kirby_copy,
                                         NDS_KIRBY_TORNADO_SIDECAR_JOINT | 0x80u,
                                         &asset, &offset) == FALSE))
    {
        return;
    }
    cobj = (gGCCurrentCamera != NULL) ? CObjGetStruct(gGCCurrentCamera) : NULL;
    if (ndsRendererAdapterBuildKirbyHatJointMatrices(fp, NDS_KIRBY_TORNADO_SIDECAR_JOINT, cobj,
                                                    &projection, &world) != FALSE)
    {
        (void)ndsRendererAdapterSubmitP4SidecarRoot(
            fp->joints[NDS_KIRBY_TORNADO_SIDECAR_JOINT], asset, offset, &projection, &world);
    }
}

/* Kirby hidden-part body (BattleShip 229_KirbyMain.c desc_0x324: joint 7).
 * FTModelPartDesc is modelparts[modelpart][detail], so the two source rows are
 * modelpart 0 HIGH=0x40A0 and LOW=0x4860 -- not mp0/mp1. SpecialN enables
 * this joint together with joint 19 (and Link-copy also joint 18), growing the
 * live draw from canonical 7 roots to a source-order 9/10-root program. The
 * body consumes the selected joint-6 head's vertex cache, so publish that live
 * head key before root-program admission; unknown heads remain a hard decline.
 * Foreign boomerang/Fox-gun models stay donor-file owned and never reach here. */
#define NDS_KIRBY_TRIO_HEAD_JOINT 6u
#define NDS_KIRBY_TRIO_BODY_JOINT 7u
#define NDS_KIRBY_TRIO_BODY_OFFSET_HIGH 0x40A0u
#define NDS_KIRBY_TRIO_BODY_OFFSET_LOW  0x4860u
/* Published by src/nds/nds_renderer_assets.c: the live joint-6 key the
 * context-keyed trio resolve reads. Declared locally so no shared header
 * changes for this Kirby-only seam. */
extern void ndsRendererNativeKirbyTrioSetHeadKey(u32 head_mp);
/* The admitted joint-6 heads, from the baked contexts themselves. */
extern s32 ndsRendererNativeKirbyTrioHeadSupported(u32 head_mp);
static sb32 ndsFighterKirbyTrioHeadKey(const FTStruct *fp, u32 *head_mp)
{
    s32 slot;

    if ((fp == NULL) || (head_mp == NULL))
    {
        return FALSE;
    }
    slot = (s32)NDS_KIRBY_TRIO_HEAD_JOINT - (s32)nFTPartsJointCommonStart;
    if ((slot < 0) ||
        ((u32)slot >= ARRAY_COUNT(fp->modelpart_status)))
    {
        return FALSE;
    }
    if (fp->modelpart_status[slot].modelpart_id_curr <= 0)
    {
        return FALSE;
    }
    /* P4 S8: a Remix hat is its own trio head (its image carries the body
     * baked against it, p4_kirby_hats.py), whatever source hat it stands on. */
    if (fp->nds_p4_kirby_hat != 0u)
    {
        if (ndsRendererNativeKirbyTrioHeadSupported((u32)fp->nds_p4_kirby_hat) != FALSE)
        {
            *head_mp = (u32)fp->nds_p4_kirby_hat;
            return TRUE;
        }
        return FALSE;
    }
    if (ndsRendererNativeKirbyTrioHeadSupported(
            (u32)fp->modelpart_status[slot].modelpart_id_curr) != FALSE)
    {
        *head_mp = (u32)fp->modelpart_status[slot].modelpart_id_curr;
        return TRUE;
    }
    return FALSE;
}
static sb32 ndsFighterKirbyTrioBodyActive(const FTStruct *fp)
{
    DObj *body_joint;
    NDSRelocLoadedFile *loaded;
    u32 expected_offset;

    if ((fp == NULL) || (fp->fkind != nFTKindKirby) ||
        ((u32)NDS_KIRBY_TRIO_BODY_JOINT >= ARRAY_COUNT(fp->joints)))
    {
        return FALSE;
    }
    body_joint = fp->joints[NDS_KIRBY_TRIO_BODY_JOINT];
    if ((body_joint == NULL) || (body_joint == DOBJ_PARENT_NULL) ||
        (body_joint->dl == NULL))
    {
        return FALSE;
    }
    loaded = ndsRelocFindLoadedFileContaining(body_joint->dl, sizeof(Gfx));
    if ((loaded == NULL) || (loaded->data == NULL) ||
        (loaded->asset_id != 0x148u))
    {
        return FALSE;
    }
    expected_offset = (fp->detail_curr == nFTPartsDetailLow) ?
        NDS_KIRBY_TRIO_BODY_OFFSET_LOW : NDS_KIRBY_TRIO_BODY_OFFSET_HIGH;
    return ((ndsRelocNativeRootOffset(loaded, body_joint->dl) ==
             expected_offset) ? TRUE : FALSE);
}

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
        NDS_DIAG(gNdsFighterDLAllDrawRangeRejectCount++);
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

/* Packet hits do not execute the production owner. Refresh only the fields
 * consumed by ndsRendererFighterPacketPrecheck/ndsFighterPacketTryReplay and
 * leave resolver/material/asset setup to the miss path.  The full producer
 * below still rebuilds every production field before a record/direct draw, so
 * this cannot leak a packet-only partial input into ordinary execution. */
static sb32 ndsRendererAdapterRefreshNativePacketInputs(
    u32 color_modulate,
    const NDSFighterDLAllDrawCollection *collection,
    const NDSRendererMatrix20p12 *projection,
    const NDSRendererMatrix20p12 *const *modelviews,
    NDSRendererAdapterNativeOwnerWorkspace *workspace)
{
    u32 i;

    if ((collection == NULL) || (modelviews == NULL) || (workspace == NULL) ||
        (collection->selected_count == 0u) ||
        (collection->selected_count > NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED))
    {
        return FALSE;
    }
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
                &sNdsFighterDisplayReplayEvents[collection->indices[i]] : NULL;

        if (modelviews[i] == NULL)
        {
            return FALSE;
        }
        config->initial_geometry_mode = (event != NULL) ?
            sNdsFighterDisplayReplayPreambles[
                collection->indices[i]].geometry_mode : 0u;
        config->color_modulate = color_modulate;

        root->root_offset = workspace->root_offsets[i];
        root->material_count = workspace->material_counts[i];
        root->modelview_matrix = modelviews[i];
        root->projection_matrix = projection;
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
            root->gx_locals = NULL;
            root->gx_seed = NULL;
            root->gx_local_count = 0u;
            root->gx_parent_slot = (u8)NDS_RENDERER_FIGHTER_GX_SLOT_NONE;
            root->gx_store_slot = (u8)NDS_RENDERER_FIGHTER_GX_SLOT_NONE;
            root->gx_seed_is_identity = 0u;
        }
        root->preamble = (event != NULL) ?
            &sNdsFighterDisplayReplayPreambles[collection->indices[i]] :
            &sNdsRendererAdapterZeroPreamble;
    }
    return TRUE;
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

        /* R2-03 E16b. The hardware performs this multiply now, and E16b traced
         * the composed matrix to no other production consumer, so the compose
         * is skipped outright -- one 4x4 per root out of MatrixPrep. */
        if (modelviews[i] == NULL)
        {
            return FALSE;
        }

        root->root_offset = workspace->root_offsets[i];
        root->material_count = workspace->material_counts[i];
        root->asset_base = (workspace->loaded[i] != NULL) ?
            workspace->loaded[i]->data : NULL;
        root->materials = sNdsRendererAdapterNativeOwnerMaterials[
            sNdsRendererAdapterNativeOwnerMaterialRows[i]];
        root->modelview_matrix = modelviews[i];
        root->projection_matrix = projection;
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
        root->asset_base = (workspace->loaded[i] != NULL) ?
            workspace->loaded[i]->data : NULL;
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
        NDS_DIAG(gNdsFighterDLAllDrawP0AttemptCount++);
        if (clean_selected != FALSE)
        {
            NDS_DIAG(gNdsFighterDLAllDrawP0CleanCount++);
        }
        else
        {
            NDS_DIAG(gNdsFighterDLAllDrawP0FailedCount++);
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
        NDS_DIAG(gNdsFighterDLAllDrawP0CommandCount += stats->command_count);
        if (gNdsFighterDLAllDrawP0FirstOpcode == 0u)
        {
            gNdsFighterDLAllDrawP0FirstOpcode = stats->first_opcode;
        }
        if ((unsupported_opcode != 0u) &&
            (gNdsFighterDLAllDrawP0UnsupportedOpcode == 0u))
        {
            gNdsFighterDLAllDrawP0UnsupportedOpcode = unsupported_opcode;
        }
        NDS_DIAG(gNdsFighterDLAllDrawP0UnsupportedCommandCount += unsupported_count);
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
        NDS_DIAG(gNdsFighterDLAllDrawP0TriangleCount += state->triangle_count);
        gNdsFighterDLAllDrawP0TriangleValidCount +=
            state->triangle_valid_count;
        gNdsFighterDLAllDrawP0ColorChecksum =
            (gNdsFighterDLAllDrawP0ColorChecksum * 33u) ^
            state->color_checksum;
    }
    else if (slot == 1u)
    {
        NDS_DIAG(gNdsFighterDLAllDrawP1AttemptCount++);
        if (clean_selected != FALSE)
        {
            NDS_DIAG(gNdsFighterDLAllDrawP1CleanCount++);
        }
        else
        {
            NDS_DIAG(gNdsFighterDLAllDrawP1FailedCount++);
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
        NDS_DIAG(gNdsFighterDLAllDrawP1CommandCount += stats->command_count);
        if (gNdsFighterDLAllDrawP1FirstOpcode == 0u)
        {
            gNdsFighterDLAllDrawP1FirstOpcode = stats->first_opcode;
        }
        if ((unsupported_opcode != 0u) &&
            (gNdsFighterDLAllDrawP1UnsupportedOpcode == 0u))
        {
            gNdsFighterDLAllDrawP1UnsupportedOpcode = unsupported_opcode;
        }
        NDS_DIAG(gNdsFighterDLAllDrawP1UnsupportedCommandCount += unsupported_count);
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
        NDS_DIAG(gNdsFighterDLAllDrawP1TriangleCount += state->triangle_count);
        gNdsFighterDLAllDrawP1TriangleValidCount +=
            state->triangle_valid_count;
        gNdsFighterDLAllDrawP1ColorChecksum =
            (gNdsFighterDLAllDrawP1ColorChecksum * 33u) ^
            state->color_checksum;
    }

    gNdsFighterDLAllDrawVertexRangeRejectCount +=
        state->vertex_range_reject_count;
}


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
 * The plan is unconditional, so the lifetime key above is part of the
 * correctness contract rather than lab-only instrumentation.
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
    u8 root_program;
    u8 root_program_pad[3];
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
    /* use_low_detail | colanim.skeleton_id << 8 (NDS_FTR_PLAN_DETAIL_KEY). */
    u32 key_use_low_detail;
    u32 valid;
} NDSFighterDrawPlan;

/* P2-2p8 Phase 1 slice 6: the plan's detail word also carries the electric
 * skeleton. ftDisplayMainDrawAll draws Mario's and Fox's skeleton display
 * lists instead of the body while colanim.skeleton_id is set, and the colour
 * animation toggles it every few frames INSIDE one status -- no status
 * generation moves -- so a plan keyed without it replayed the body's roots
 * (program 0) on skeleton frames, or kept program 0xFE after the skeleton
 * ended, until the next status change. Found by the lean route-2 oracle on
 * Mario/Fox with an electric attacker in the match (structure mismatches). */
#define NDS_FTR_PLAN_DETAIL_KEY(use_low_detail, fp) \
    ((u32)(use_low_detail) | ((u32)(u8)(fp)->colanim.skeleton_id << 8))

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

    if (slot >= GMCOMMON_PLAYERS_MAX)
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
/* Pre-validate decline witness. A plan-resolve decline sets
 * native_owner_enabled FALSE, so the validator -- and its reject words -- is
 * short-circuited away and the ROM has nothing that says why. Yoshi declines
 * exactly there: 8,360 FIGHTER/REJECTED_PROGRAM records in 1,200 presents with
 * all six validate words reading zero. Stage: 1 selected, 2 display list,
 * 3 material count, 4 validate, 5 animlock, 13 Link foreign model-part DL,
 * 14 alternate owner program fenced from hierarchy mode. */
__attribute__((used)) volatile u32 gNdsFtrDeclineStage;
/* Stage 2's eight clauses, and the file identities they compared. */
__attribute__((used)) volatile u32 gNdsFtrDeclineDisplayListClause;
__attribute__((used)) volatile u32 gNdsFtrDeclineDisplayListAsset;
__attribute__((used)) volatile u32 gNdsFtrDeclineDisplayListExpected;
__attribute__((used)) volatile u32 gNdsFtrDeclineDisplayListOwnerAsset;
__attribute__((used)) volatile u32 gNdsFtrDeclineOwner;
__attribute__((used)) volatile u32 gNdsFtrDeclineSelected;
__attribute__((used)) volatile u32 gNdsFtrDeclineIndex;
__attribute__((used)) volatile u32 gNdsFtrDeclineAssetId;
__attribute__((used)) volatile u32 gNdsFtrDeclineDetail;
__attribute__((used)) volatile u32 gNdsFtrRootProgramsTried;
/* The live (asset, root offset) vector the plan resolver last examined. */
__attribute__((used)) volatile u32 gNdsFtrPlanVectorCount;
__attribute__((used)) volatile u32 gNdsFtrPlanVectorAsset[
    NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
__attribute__((used)) volatile u32 gNdsFtrPlanVectorOffset[
    NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];

static sb32 ndsFighterNativeLoadedFileAllowed(
    u32 owner_slot, u32 expected_asset_id,
    const NDSRelocLoadedFile *loaded, const Gfx *native_dl)
{
    if ((loaded == NULL) || (native_dl == NULL) || (loaded->data == NULL))
    {
        return FALSE;
    }
    if (loaded->asset_id == expected_asset_id)
    {
        return TRUE;
    }
#if NDS_P2_LINK
    /* Link's Missing/Catching Boomerang motions temporarily select joint 11
     * modelpart 1. LinkMain resolves that model part through extern-data to
     * LinkBoomerangModel asset 0x146 / root 0xF8 while the rest of the fighter
     * remains LinkModel asset 0x144. The exact 20-root generated program still
     * has to match; this only admits that one source-qualified donor file. */
    if ((owner_slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_LINK) &&
        (expected_asset_id == 0x144u) &&
        (loaded->asset_id == 0x146u) &&
        (loaded->data_size == 0x1d0u) &&
        ((uintptr_t)native_dl >= (uintptr_t)loaded->data) &&
        ((uintptr_t)native_dl <=
         ((uintptr_t)loaded->data + loaded->data_size - sizeof(*native_dl))) &&
        (ndsRelocNativeRootOffset(loaded, native_dl) == 0x00f8u))
    {
        return TRUE;
    }
#endif
    /* BattleShip Kirby CopyLink motion 0x122 enables hidden-part ID 6
     * (anim flags 0x02000000): joint 12 is inserted under joint 11 and its
     * modelpart-0 DL is LinkBoomerangModel asset 0x146 / root 0xF8. The exact
     * mixed root vector still has to select generated program 4 below; this
     * admission merely permits that source-owned foreign file to reach it. */
    if ((owner_slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_KIRBY) &&
        (expected_asset_id == 0x148u) &&
        (loaded->asset_id == 0x146u) &&
        (loaded->data_size == 0x1d0u) &&
        ((uintptr_t)native_dl >= (uintptr_t)loaded->data) &&
        ((uintptr_t)native_dl <=
         ((uintptr_t)loaded->data + loaded->data_size - sizeof(*native_dl))) &&
        (ndsRelocNativeRootOffset(loaded, native_dl) == 0x00f8u))
    {
        return TRUE;
    }
    return FALSE;
}

static NDSFighterDrawPlanResult ndsFighterDrawPlanResolve(
    u32 owner_slot, u32 expected_asset_id,
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
        gNdsFtrDeclineStage = 1u;
        gNdsFtrDeclineSelected = collection->selected_count;
        return nNDSFighterDrawPlanSelected;
    }
    /* The whole live vector, not just the entry that refused.
     *
     * A mixed-file program is defined by (asset, root offset) for EVERY
     * selected DObj: the one offending entry names the donor but not the
     * position the donor occupies, and the resolver returns at the first
     * refusal, so the workspace holds nothing past it. Kirby's copied-Fox
     * furniture needs that vector to be baked (2026-09-21). */
    for (i = 0u; i < collection->selected_count; i++)
    {
        const NDSFighterDisplayContractEvent *witness_event =
            (sNdsFighterDisplayContractPlayback != FALSE) ?
                &sNdsFighterDisplayReplayEvents[collection->indices[i]] : NULL;
        const Gfx *witness_dl =
            (witness_event != NULL) ? witness_event->dl :
                                      collection->dobjs[i]->dl;
        NDSRelocLoadedFile *witness_file =
            ndsRelocFindLoadedFileContaining(witness_dl, sizeof(*witness_dl));

        gNdsFtrPlanVectorAsset[i] =
            (witness_file != NULL) ? witness_file->asset_id : 0xffffffffu;
        gNdsFtrPlanVectorOffset[i] =
            ((witness_file != NULL) && (witness_file->data != NULL)) ?
                ndsRelocNativeRootOffset(witness_file, witness_dl) :
                0xffffffffu;
    }
    gNdsFtrPlanVectorCount = collection->selected_count;
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
        sb32 foreign_donor =
            ((loaded != NULL) &&
             (loaded->asset_id != expected_asset_id) &&
             (ndsFighterNativeLoadedFileAllowed(
                  owner_slot, expected_asset_id, loaded, native_dl) != FALSE)) ?
                TRUE : FALSE;
        MObj *mobj;
        u32 material_count = 0u;

        if ((native_dl == NULL) || (loaded == NULL) ||
            (loaded->data == NULL) ||
            (ndsFighterNativeLoadedFileAllowed(
                 owner_slot, expected_asset_id, loaded, native_dl) == FALSE) ||
            ((owner_file != NULL) && (loaded != owner_file) &&
             (foreign_donor == FALSE)) ||
            ((owner_file == NULL) && (foreign_donor != FALSE)) ||
            (loaded->data_size < sizeof(*native_dl)) ||
            ((uintptr_t)native_dl < (uintptr_t)loaded->data) ||
            (((uintptr_t)native_dl - (uintptr_t)loaded->data) >
             (loaded->data_size - sizeof(*native_dl))))
        {
            *out_owner_file = owner_file;
            gNdsFtrDeclineStage =
                ((owner_slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_LINK) &&
                 (loaded != NULL) &&
                 (loaded->asset_id != expected_asset_id)) ? 13u : 2u;
            /* WHICH of the eight clauses, and against what.
             *
             * The identical test below in ndsFighterMarioFoxDLAllDrawForSlot
             * already records this, but THIS resolver is the copy that runs
             * first and returns, so a debugger at the 1P halt read stage 2
             * with clause 0 and no asset ids -- the witness was on the
             * unreachable twin (2026-09-21, Kirby's copy hats). Both copies
             * must publish it or the record names the wrong refusal. */
            gNdsFtrDeclineDisplayListClause =
                (native_dl == NULL) ? 1u :
                (loaded == NULL) ? 2u :
                (loaded->data == NULL) ? 3u :
                (ndsFighterNativeLoadedFileAllowed(
                     owner_slot, expected_asset_id, loaded,
                     native_dl) == FALSE) ? 4u :
                ((owner_file != NULL) && (loaded != owner_file)) ? 5u :
                ((owner_file == NULL) && (foreign_donor != FALSE)) ? 6u :
                (loaded->data_size < sizeof(*native_dl)) ? 7u : 8u;
            gNdsFtrDeclineDisplayListAsset =
                (loaded != NULL) ? loaded->asset_id : 0xffffffffu;
            gNdsFtrDeclineDisplayListExpected = expected_asset_id;
            gNdsFtrDeclineDisplayListOwnerAsset =
                (owner_file != NULL) ? owner_file->asset_id : 0xffffffffu;
            gNdsFtrDeclineSelected = collection->selected_count;
            gNdsFtrDeclineIndex = i;
            gNdsFtrDeclineAssetId =
                (loaded != NULL) ? loaded->asset_id : 0xffffffffu;
            gNdsFtrDeclineDetail =
                ((loaded != NULL) && (loaded->data != NULL)) ?
                    ndsRelocNativeRootOffset(loaded, native_dl) :
                    (u32)(uintptr_t)native_dl;
            return nNDSFighterDrawPlanDisplayList;
        }
        if (owner_file == NULL)
        {
            owner_file = loaded;
        }
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
            gNdsFtrDeclineStage = 3u;
            gNdsFtrDeclineSelected = collection->selected_count;
            gNdsFtrDeclineIndex = i;
            gNdsFtrDeclineAssetId = loaded->asset_id;
            gNdsFtrDeclineDetail = material_count;
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
    u32 root_program,
    const NDSRendererAdapterNativeOwnerWorkspace *workspace,
    NDSFighterDrawPlanData *out)
{
    u32 i;

    bzero(out, sizeof(*out));
    out->collection = *collection;
    out->owner_file = owner_file;
    out->root_program = (u8)root_program;
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
    /* P4: a donor draws its own model; fkind is its setup parent. */
    if ((fp->nds_p4_content != 0u) &&
        ((u32)fp->nds_p4_content <= NDS_P4_CONTENT_MAX_ID))
    {
        *owner_slot = NDS_P4_RENDERER_OWNER_SLOT(fp->nds_p4_content);
        return TRUE;
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
    if (fp->fkind == nFTKindSamus)
    {
        *owner_slot = 5u;
        return TRUE;
    }
#if NDS_P2_LINK
    if (fp->fkind == nFTKindLink)
    {
        *owner_slot = 6u;
        return TRUE;
    }
#endif
    if (fp->fkind == nFTKindPikachu)
    {
        *owner_slot = 7u;
        return TRUE;
    }
    if (fp->fkind == nFTKindYoshi)
    {
        *owner_slot = 8u;
        return TRUE;
    }
    if (fp->fkind == nFTKindNess)
    {
        *owner_slot = 9u;
        return TRUE;
    }
    if (fp->fkind == nFTKindPurin)
    {
        *owner_slot = 10u;
        return TRUE;
    }
    if (fp->fkind == nFTKindKirby)
    {
        *owner_slot = 11u;
        return TRUE;
    }
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
#if NDS_P2_1P_GAME
    if (fp->fkind == nFTKindBoss)
    {
        *owner_slot = 24u;
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
    if (owner_slot == 5u)
    {
        return 0x140u; /* llSamusModelFileID, BattleShip dFTSamusData */
    }
#if NDS_P2_LINK
    if (owner_slot == 6u)
    {
        return 0x144u; /* llLinkModelFileID, BattleShip dFTLinkData */
    }
#endif
    if (owner_slot == 7u)
    {
        return 0x155u; /* llPikachuModelFileID, BattleShip dFTPikachuData */
    }
    if (owner_slot == 8u)
    {
        return 0x152u; /* llYoshiModelFileID, BattleShip dFTYoshiData */
    }
    if (owner_slot == 9u)
    {
        return 0x14fu; /* llNessModelFileID, BattleShip dFTNessData */
    }
    if (owner_slot == 10u)
    {
        return 0x14au; /* llPurinModelFileID, BattleShip dFTPurinData */
    }
    if (owner_slot == 11u)
    {
        return 0x148u; /* llKirbyModelFileID, BattleShip dFTKirbyData */
    }
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
    /* A P4 content's own model file (its export, nds_p4_contents.h). */
#define NDS_P4_OWNER_MODEL_FILE(id_, T_, N_, n_, parent_, model_, main_)       \
    if (owner_slot == NDS_P4_RENDERER_OWNER_SLOT(id_))                          \
    {                                                                           \
        return (model_);                                                        \
    }
    NDS_P4_CONTENT_ROWS(NDS_P4_OWNER_MODEL_FILE)
#undef NDS_P4_OWNER_MODEL_FILE
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
#if NDS_P2_1P_GAME
    if (owner_slot == 24u)
    {
        return 0x158u; /* llBossModelFileID, BattleShip BossModel 0x158 */
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
    if (owner_slot == 5u)
    {
        return NDS_RENDERER_PROFILE_OWNER_SAMUS;
    }
#if NDS_P2_LINK
    if (owner_slot == 6u)
    {
        return NDS_RENDERER_PROFILE_OWNER_LINK;
    }
#endif
    if (owner_slot == 7u)
    {
        return NDS_RENDERER_PROFILE_OWNER_PIKACHU;
    }
    if (owner_slot == 8u)
    {
        return NDS_RENDERER_PROFILE_OWNER_YOSHI;
    }
    if (owner_slot == 9u)
    {
        return NDS_RENDERER_PROFILE_OWNER_NESS;
    }
    if (owner_slot == 10u)
    {
        return NDS_RENDERER_PROFILE_OWNER_PURIN;
    }
    if (owner_slot == 11u)
    {
        return NDS_RENDERER_PROFILE_OWNER_KIRBY;
    }
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
#define NDS_P4_OWNER_PROFILE(id_, T_, N_, n_, parent_, model_, main_)          \
    if (owner_slot == NDS_P4_RENDERER_OWNER_SLOT(id_))                          \
    {                                                                           \
        return NDS_RENDERER_PROFILE_OWNER_##N_;                                 \
    }
    NDS_P4_CONTENT_ROWS(NDS_P4_OWNER_PROFILE)
#undef NDS_P4_OWNER_PROFILE
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
#if NDS_P2_1P_GAME
    if (owner_slot == 24u)
    {
        return NDS_RENDERER_PROFILE_OWNER_BOSS;
    }
#endif
    return NDS_RENDERER_PROFILE_OWNER_NONE;
}

/* Action diagnostics need a fighter-local edge, not the global native-failure
 * latch: gNdsRendererNativeFailure keeps the first failure's identity for the
 * whole run while count continues rising.  Keep these exported so GDB can
 * prove that the sampled player was rejected on the just-finished frame. */
__attribute__((used)) volatile u32
    gNdsFtrRejectCountBySlot[GMCOMMON_PLAYERS_MAX];
__attribute__((used)) volatile u32
    gNdsFtrRejectStatusBySlot[GMCOMMON_PLAYERS_MAX];
__attribute__((used)) volatile u32
    gNdsFtrRejectReasonBySlot[GMCOMMON_PLAYERS_MAX];

static void ndsFighterRejectNativeRender(FTStruct *fp, DObj *dobj,
    const Gfx *dl, u32 reason, NDSRendererStats *stats)
{
    NDSRelocLoadedFile *loaded = (dl != NULL) ?
        ndsRelocFindLoadedFileContaining(dl, sizeof(*dl)) : NULL;
    u32 identity = ((u32)fp->fkind << 16) |
        ((loaded != NULL) ? (loaded->asset_id & 0xffffu) : 0xffffu);
    u32 slot = (u32)fp->nds_slot;

    if (slot < GMCOMMON_PLAYERS_MAX)
    {
        gNdsFtrRejectCountBySlot[slot]++;
        gNdsFtrRejectStatusBySlot[slot] = (u32)fp->status_id;
        gNdsFtrRejectReasonBySlot[slot] = reason;
    }

    ndsRendererRecordNativeFailure(NDS_NATIVE_FAILURE_FIGHTER,
        (u32)gSCManagerSceneData.scene_curr, identity, (u32)fp->status_id,
        (loaded != NULL) ? ndsRelocNativeRootOffset(loaded, dl) :
            (u32)(uintptr_t)dl,
        (dobj != NULL) ? (u32)(uintptr_t)dobj->mobj : 0u, reason);
    if (stats != NULL)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
    }
}

/* 2026-10-05 (owner: delete the old machinery): fighter draws the lean path
 * declined -- skipped, since no other renderer is behind it. */
volatile u32 gNdsFtrLeanSkippedDraws;

#if NDS_P2_1P_GAME
/* 1P Intro transient native context.
 *
 * The source Intro holds up to 21 simultaneous Demo actors
 * (sc1PIntroGetFighterAllocsNum max, Zako loop) while the battle caches
 * stay four entries. One scratch instance slot is bound sequentially at
 * the display boundary: every submit draws through it, and every
 * instance-dependent cache is invalidated when the bound GObj changes.
 * Nothing persists across actors (no plan/memo bake, no per-frame dedup),
 * so a same-kind pair can never replay another actor's DObj pointers,
 * packets, or materials. Per-GObj pose/update state, camera transforms,
 * and source display order are untouched: gcRunAll/gcDrawAll still drive
 * everything, this only submits the current GObj natively.
 *
 * Slot zero is borrowed only during this display scene. Its cached state is
 * discarded before a different actor can use it; no battle slot is added. */
#define NDS_INTRO_TRANSIENT_SCRATCH_SLOT 0u

extern void ndsRendererFighterPacketInvalidateSlot(u32 slot);
extern void ndsFighterRendererInvalidateMaterialCachesForSlot(u32 slot);

volatile u32 gNdsIntroTransientSubmitCount;
volatile u32 gNdsIntroTransientDrawCount;
volatile u32 gNdsIntroTransientBindChanges;
volatile u32 gNdsIntroTransientOwnerRejectCount;

static GObj *sNdsIntroTransientBoundGObj = NULL;

/* Pure ownership test, extracted for the host suite: stale state may be
 * reused only while the exact same source GObj stays bound. */
static sb32 ndsIntroTransientNeedsBindChange(GObj *bound, GObj *incoming)
{
    if (incoming == NULL)
    {
        return FALSE;
    }
    return (bound != incoming) ? TRUE : FALSE;
}

void ndsFighterIntroTransientReset(void)
{
    sNdsIntroTransientBoundGObj = NULL;
    sNdsFighterDrawPlan[NDS_INTRO_TRANSIENT_SCRATCH_SLOT].valid = 0u;
    sNdsFtrDrawMemo[NDS_INTRO_TRANSIENT_SCRATCH_SLOT].valid = 0u;
    ndsRendererFighterPacketInvalidateSlot(NDS_INTRO_TRANSIENT_SCRATCH_SLOT);
}

static void ndsFighterIntroTransientInvalidateScratch(void)
{
    sNdsFighterDrawPlan[NDS_INTRO_TRANSIENT_SCRATCH_SLOT].valid = 0u;
    sNdsFtrDrawMemo[NDS_INTRO_TRANSIENT_SCRATCH_SLOT].valid = 0u;
    /* Material rows are global and MObj-keyed, but a rebuilt fighter tree
     * may reuse MObj addresses at the same heap generation, so the bind
     * change takes the same authoritative seam the CSS rebuild uses. This
     * also invalidates the scratch packet slot. */
    ndsFighterRendererInvalidateMaterialCachesForSlot(
        NDS_INTRO_TRANSIENT_SCRATCH_SLOT);
}

static sb32 ndsFtrLeanTransientDraw(FTStruct *fp, sb32 rebind);

/* Draw one Intro Demo fighter natively through the scratch slot. Returns
 * TRUE when the submit reached native production. Caller-owned: the
 * normal contract gate calls this only after its own tracked/slot checks
 * fail, so battle fighters never enter here. */
static sb32 ndsFighterIntroTransientSubmit(GObj *fighter_gobj)
{
    FTStruct *fp;
    u32 owner_slot;
    u32 submitted_before;
    sb32 saved_playback;
    sb32 rebind = FALSE;
    u32 saved_no_oracle;

    /* The ending's figure (mvEndingMakeFighter: one Demo actor in
     * nFTDemoStatusFigureDropped) is never registered either, nor is the
     * challenger's silhouette (sc1PChallengerMakeFighter: one Demo actor,
     * made with ftManagerMakeFighter alone), nor the continue screen's
     * fallen figure (mnPlayers1PGameContinueMakeFighter, the same Dropped
     * Demo actor; refused here it never drew, 2026-10-04). */
    if ((fighter_gobj == NULL) ||
        ((gNdsSceneManagerCurrKind != nSCKind1PIntro) &&
         (gNdsSceneManagerCurrKind != nSCKindEnding) &&
         (gNdsSceneManagerCurrKind != nSCKind1PChallenger) &&
         (gNdsSceneManagerCurrKind != nSCKind1PContinue)))
    {
        return FALSE;
    }
    fp = ftGetStruct(fighter_gobj);
    if ((fp == NULL) || (fp->fighter_gobj == NULL) ||
        (ndsFighterGetNativeOwnerSlot(fp, &owner_slot) == FALSE))
    {
        if ((fighter_gobj != NULL) && (fp != NULL))
        {
            gNdsIntroTransientOwnerRejectCount++;
        }
        return FALSE;
    }
    if (ndsIntroTransientNeedsBindChange(
            sNdsIntroTransientBoundGObj, fighter_gobj) != FALSE)
    {
        ndsFighterIntroTransientInvalidateScratch();
        sNdsIntroTransientBoundGObj = fighter_gobj;
        gNdsIntroTransientBindChanges++;
        rebind = TRUE;
    }
    gNdsIntroTransientSubmitCount++;
    submitted_before = gNdsFighterMarioFoxDLAllDrawCount;
    sNdsIntroTransientActive = TRUE;
    ndsFighterDisplayContractCapture(fighter_gobj);
    if (sNdsFighterDisplayContract.event_count == 0u)
    {
        sNdsIntroTransientActive = FALSE;
        return FALSE;
    }
    saved_no_oracle = ndsRendererHardwareNoOracleEnabled();
    ndsRendererHardwareSetNoOracle(TRUE);
    saved_playback = sNdsFighterDisplayContractPlayback;
    sNdsFighterDisplayContractPlayback = TRUE;
    if (ndsFtrLeanTransientDraw(fp, rebind) == FALSE)
    {
        gNdsFtrLeanSkippedDraws++;
    }
    (void)rebind;
    sNdsFighterDisplayContractPlayback = saved_playback;
    ndsRendererHardwareSetNoOracle(saved_no_oracle);
    sNdsIntroTransientActive = FALSE;
    if (gNdsFighterMarioFoxDLAllDrawCount != submitted_before)
    {
        gNdsIntroTransientDrawCount +=
            sNdsFighterDisplayContract.event_count;
        return TRUE;
    }
    return FALSE;
}
#endif

/* P2-2p8 Phase 1 slice 1: the lean fighter path, defined in
 * renderer_fighter_lean.c (#included at the end of this file). */
#include <nds/renderer_fighter_lean.h>
#include <port/coroutine.h>
static inline __attribute__((always_inline)) sb32
ndsFtrLeanRun(u32 slot, FTStruct *fp);
static void ndsFtrLeanFrameEnd(void);

/* ndsFtrLeanRun on the DTCM hot stack (port/coroutine.h). */
typedef struct NDSFtrLeanRunCall
{
    u32 slot;
    FTStruct *fp;
} NDSFtrLeanRunCall;

static unsigned int ndsFtrLeanRunOnHotStack(void *arg)
{
    const NDSFtrLeanRunCall *call = arg;

    return (unsigned int)ndsFtrLeanRun(call->slot, call->fp);
}

/* THE AFTERIMAGE TRAIL. ftDisplayMainDrawAll draws it after the fighter
 * (ftDisplayMainDrawAfterImage) into the translucent list, but here the whole
 * source display runs inside the contract capture, whose lists are dropped,
 * so the trail never drew: Link's sword trail, the Beam Sword swing and every
 * Remix sword trail were missing. This is the source routine's vertex build
 * (ftdisplaymain.c:398-588) with its own integer and s16 conversions, and the
 * strip is submitted natively right after the fighter
 * (ndsRendererSubmitAfterImage). The source routine is skipped in the
 * capture (battleship_ftdisplaymain.c). Remix's SwordTrail.asm draw_trail_
 * enters the same loop with its row's start, end and colours. */
extern SYColorRGBA dFTDisplayMainDefaultAfterImageColor1;
extern SYColorRGBA dFTDisplayMainDefaultAfterImageColor2;
extern SYColorRGBA dFTDisplayMainItemAfterImageColor1;
extern SYColorRGBA dFTDisplayMainItemAfterImageColor2;

/* Three edges, and at most six interpolated edges between two of them (a
 * turn of at most 180 degrees in 30 degree steps): 15 edges, 30 vertices. */
#define NDS_AFTERIMAGE_MAX_VERTICES 32u

/* One strip of the trail: the samples' line from `var_f20` to `var_f22`
 * along each sample's direction, `color1` at the first end. */
static void ndsFighterDrawAfterImageStrip(FTStruct *fp, f32 var_f20,
                                          f32 var_f22, const u8 color1[3],
                                          const u8 color2[3])
{
    s16 xyz[NDS_AFTERIMAGE_MAX_VERTICES][3];
    u8 rgba[NDS_AFTERIMAGE_MAX_VERTICES][4];
    f32 rotate;
    s32 add_alpha = 0;
    s32 base_alpha = 0xFF;
    s32 i, j, index, next_index = 0;
    s32 alpha;
    u32 count = 0u;
    u32 v;
    s16 origin[3];
    NDSRendererMatrix20p12 composed;

    rotate = F_CLC_DTOR32(30.0F);

    index = fp->afterimage.desc_id;
    if (index != 0)
    {
        index = index - 1;
    }
    else index = ARRAY_COUNT(fp->afterimage.desc) - 1;

#define NDS_AFTERIMAGE_EMIT(x_, y_, z_, rgb_, a_) do { \
    xyz[count][0] = (x_); xyz[count][1] = (y_); xyz[count][2] = (z_); \
    rgba[count][0] = (rgb_)[0]; rgba[count][1] = (rgb_)[1]; \
    rgba[count][2] = (rgb_)[2]; rgba[count][3] = (u8)(a_); count++; \
} while (0)
    for (i = fp->afterimage.drawstatus - 1; i >= 0; index = next_index, i--)
    {
        FTAfterImage *afterimage = &fp->afterimage.desc[index];

        if ((count + 2u) > NDS_AFTERIMAGE_MAX_VERTICES)
        {
            break;
        }
        alpha = (((base_alpha - add_alpha) / (fp->afterimage.drawstatus - 1)) * i) + add_alpha;
        NDS_AFTERIMAGE_EMIT(
            (s16)(afterimage->translate_x + (afterimage->vec.x * var_f20)),
            (s16)(afterimage->translate_y + (afterimage->vec.y * var_f20)),
            (s16)(afterimage->translate_z + (afterimage->vec.z * var_f20)),
            color1, alpha);
        NDS_AFTERIMAGE_EMIT(
            (s16)(afterimage->translate_x + (afterimage->vec.x * var_f22)),
            (s16)(afterimage->translate_y + (afterimage->vec.y * var_f22)),
            (s16)(afterimage->translate_z + (afterimage->vec.z * var_f22)),
            color2, alpha);

        if (i != 0)
        {
            FTAfterImage *next_afterimage;
            Vec3f spC8;

            if (index != 0)
            {
                next_index = index - 1;
            }
            else next_index = ARRAY_COUNT(fp->afterimage.desc) - 1;

            next_afterimage = &fp->afterimage.desc[next_index];

            if (syVectorNormCross3D(&afterimage->vec, &next_afterimage->vec, &spC8) != NULL)
            {
                f32 f_angle_diff = syVectorAngleDiff3D(&afterimage->vec, &next_afterimage->vec);
                s32 target_angle = f_angle_diff / rotate;

                if (target_angle != 0)
                {
                    s16 n_ai_x, n_ai_y, n_ai_z;
                    s16 vtx_x, vtx_y, vtx_z;
                    s32 alphainc;
                    f32 scale;
                    Vec3f spAC;

                    target_angle++;

                    scale = 1.0F / (target_angle);

                    n_ai_x = afterimage->translate_x;
                    n_ai_y = afterimage->translate_y;
                    n_ai_z = afterimage->translate_z;

                    f_angle_diff *= scale;

                    vtx_x = ((next_afterimage->translate_x - n_ai_x) * scale);
                    vtx_y = ((next_afterimage->translate_y - n_ai_y) * scale);
                    vtx_z = ((next_afterimage->translate_z - n_ai_z) * scale);

                    spAC = afterimage->vec;

                    alphainc = (((((base_alpha - add_alpha) / (fp->afterimage.drawstatus - 1)) * (i - 1)) + add_alpha) - alpha) * scale;

                    for (j = 0; j < target_angle - 1; j++)
                    {
                        if ((count + 2u) > NDS_AFTERIMAGE_MAX_VERTICES)
                        {
                            break;
                        }
                        n_ai_x += vtx_x;
                        n_ai_y += vtx_y;
                        n_ai_z += vtx_z;

                        syVectorRotateAbout3D(&spAC, &spC8, f_angle_diff);

                        alpha += alphainc;

                        NDS_AFTERIMAGE_EMIT(
                            (s16)(n_ai_x + (spAC.x * var_f20)),
                            (s16)(n_ai_y + (spAC.y * var_f20)),
                            (s16)(n_ai_z + (spAC.z * var_f20)),
                            color1, alpha);
                        NDS_AFTERIMAGE_EMIT(
                            (s16)(n_ai_x + (spAC.x * var_f22)),
                            (s16)(n_ai_y + (spAC.y * var_f22)),
                            (s16)(n_ai_z + (spAC.z * var_f22)),
                            color2, alpha);
                    }
                }
            }
        }
    }
#undef NDS_AFTERIMAGE_EMIT
    if (count < 4u)
    {
        return;
    }
    origin[0] = xyz[0][0];
    origin[1] = xyz[0][1];
    origin[2] = xyz[0][2];
    for (v = 0u; v < count; v++)
    {
        u32 axis;

        for (axis = 0u; axis < 3u; axis++)
        {
            s32 d = (s32)xyz[v][axis] - (s32)origin[axis];

            if ((d > NDS_RENDERER_AFTERIMAGE_VERTEX_LIMIT) ||
                (d < -NDS_RENDERER_AFTERIMAGE_VERTEX_LIMIT))
            {
                gNdsRendererAfterImageRejectCount++;
                return;
            }
            xyz[v][axis] = (s16)d;
        }
    }
    if (ndsRendererAdapterBuildWorldOriginMtx(
            (gGCCurrentCamera != NULL) ? CObjGetStruct(gGCCurrentCamera) : NULL,
            origin, &composed) == FALSE)
    {
        return;
    }
    (void)ndsRendererSubmitAfterImage(&composed, (const s16 (*)[3])xyz,
                                      (const u8 (*)[4])rgba, count);
}

static void ndsFighterDrawAfterImage(FTStruct *fp)
{
    u8 color1[3];
    u8 color2[3];
    f32 var_f20;
    f32 var_f22;
    f32 gap_lo = 0.0F;
    f32 gap_hi = 0.0F;

#define NDS_AFTERIMAGE_RGB(dst_, src_) \
    ((dst_)[0] = (src_).r, (dst_)[1] = (src_).g, (dst_)[2] = (src_).b)
    switch (fp->afterimage.is_itemswing)
    {
    case FALSE:
        var_f20 = 50.0F;
        var_f22 = 250.0F;
        NDS_AFTERIMAGE_RGB(color1, dFTDisplayMainDefaultAfterImageColor1);
        NDS_AFTERIMAGE_RGB(color2, dFTDisplayMainDefaultAfterImageColor2);
        break;

    case TRUE:
        var_f20 = 80.0F;
        var_f22 = 580.0F;
        NDS_AFTERIMAGE_RGB(color1, dFTDisplayMainItemAfterImageColor1);
        NDS_AFTERIMAGE_RGB(color2, dFTDisplayMainItemAfterImageColor2);
        break;

    default:
    {
        const NDSP4SwordTrail *row =
            ndsP4SwordTrail(fp, (u32)fp->afterimage.is_itemswing);

        if (row == NULL)
        {
            return;
        }
        var_f20 = row->start;
        var_f22 = row->end;
        gap_lo = row->gap_lo;
        gap_hi = row->gap_hi;
        color1[0] = (u8)(row->colour_1 >> 24);
        color1[1] = (u8)(row->colour_1 >> 16);
        color1[2] = (u8)(row->colour_1 >> 8);
        color2[0] = (u8)(row->colour_2 >> 24);
        color2[1] = (u8)(row->colour_2 >> 16);
        color2[2] = (u8)(row->colour_2 >> 8);
        break;
    }
    }
#undef NDS_AFTERIMAGE_RGB
    /* A row with a gap (P6, owner 2026-10-19: Palutena's staff, its middle third
     * clear) is two strips of the same samples around it. */
    if (gap_lo < gap_hi)
    {
        ndsFighterDrawAfterImageStrip(fp, var_f20, gap_lo, color1, color2);
        ndsFighterDrawAfterImageStrip(fp, gap_hi, var_f22, color1, color2);
        return;
    }
    ndsFighterDrawAfterImageStrip(fp, var_f20, var_f22, color1, color2);
}

void ndsFighterDisplayContractSubmit(GObj *fighter_gobj)
{
    extern volatile u32 gNdsVSResultsFighterSubmitCount;
    FTStruct *fp;
    u32 owner_slot;
    u32 submitted_before;
    u32 triangles_before;
    u32 triangles_after;
    u32 saved_no_oracle;
#if NDS_TICK_HUD
    u32 owner_start;
    NDSRendererProfileOwner owner_id;
#endif

    if (fighter_gobj == NULL)
    {
        return;
    }
    if (gSCManagerSceneData.scene_curr == nSCKindVSResults)
    {
        gNdsVSResultsFighterSubmitCount++;
    }
    fp = ftGetStruct(fighter_gobj);
    if ((ndsFighterStructIsTrackedPointer(fp) == FALSE) ||
        ((u32)fp->nds_slot >= GMCOMMON_PLAYERS_MAX) ||
        (ndsFighterGetNativeOwnerSlot(fp, &owner_slot) == FALSE))
    {
#if NDS_P2_1P_GAME
        /* 1P Intro Demo fighters (up to 21 concurrent actors) and the
         * ending's figure are never registered in the four-entry live
         * registry. Bind each one transiently at this display boundary
         * instead. Battle fighters always pass the gate above and never
         * enter here. */
        (void)ndsFighterIntroTransientSubmit(fighter_gobj);
#endif
        return;
    }
    if (sNdsFighterDisplayContractLastFrame[(u32)fp->nds_slot] ==
        gNdsRendererProfileFrameCount)
    {
        return;
    }
    sNdsFighterDisplayContractLastFrame[(u32)fp->nds_slot] =
        gNdsRendererProfileFrameCount;
#if NDS_TICK_HUD
    owner_id = ndsFighterNativeOwnerProfileId(owner_slot);
    owner_start = NDS_TICK_HUD_SPAN_CLOCK();
#endif
    {
        /* P2-2p8 Phase 1 (Phase 0 leftover): FTR's head sub-phase. */
        u32 lean_head_start = NDS_FTR_LEAN_CLOCK();

        ndsFighterDisplayContractCapture(fighter_gobj);
#if NDS_FTR_LEAN_TIMED
        {
            u32 lean_head = NDS_FTR_LEAN_CLOCK() - lean_head_start;

            gNdsFtrLean.head_ticks += lean_head;
            if (NDS_FTR_LEAN_OWNER_KIND(owner_slot) != NDS_FTR_LEAN_KIND_NONE)
            {
                gNdsFtrLean.k_head_ticks[(u32)fp->nds_slot & 3u] += lean_head;
            }
        }
#endif
        (void)lean_head_start;
    }
    if (sNdsFighterDisplayContract.event_count == 0u)
    {
#if NDS_TICK_HUD
        gNdsTickHudFighterTicks += NDS_TICK_HUD_SPAN_CLOCK() - owner_start;
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
    saved_no_oracle = ndsRendererHardwareNoOracleEnabled();
    ndsRendererHardwareSetNoOracle(TRUE);
    /* A fighter's IA8 masks keep their graded alpha (P4 Meta Knight's Mach
     * Tornado); see sNdsRendererHardwareTexelAlphaGraded. */
    ndsRendererHardwareSetTexelAlphaGraded(TRUE);
    {
        sb32 lean_drew;

#if NDS_VRAM_CENSUS_LIVE
        /* Slice 2a (lab): texture uploads inside this draw belong to it. */
        NDS_DIAG(gNdsVramCensusDrawSlotPlus1 = ((u32)fp->nds_slot & 3u) + 1u);
#endif
        /* 2026-10-05 (owner: delete the old machinery): the lean path is the
         * fighter renderer. A draw it declines is skipped and counted; there
         * is no executor behind it (campaign and VS walks: 0 declines
         * outside one GAME SET frame). */
        {
            NDSFtrLeanRunCall call = { (u32)fp->nds_slot, fp };

            lean_drew = (sb32)ndsDtcmHotStackRun(ndsFtrLeanRunOnHotStack,
                                                 &call);
        }
        if (lean_drew == FALSE)
        {
            gNdsFtrLeanSkippedDraws++;
        }
#if NDS_VRAM_CENSUS_LIVE
        gNdsVramCensusDrawSlotPlus1 = 0u;
#endif
    }
    ndsRendererHardwareSetTexelAlphaGraded(FALSE);
    /* ftDisplayMainDrawAll's tail: the afterimage after the fighter. */
    if (fp->afterimage.drawstatus >= 2)
    {
        ndsFighterDrawAfterImage(fp);
    }
    ndsRendererHardwareSetNoOracle(saved_no_oracle);
    sNdsFighterDisplayContractPlayback = FALSE;
    if (gNdsFighterMarioFoxDLAllDrawCount != submitted_before)
    {
        NDS_DIAG(gNdsFighterDisplayContractSubmittedCount +=
            sNdsFighterDisplayContract.event_count);
        gNdsStageGCDrawAllLoopHardwareFighterSubmitCount++;
        triangles_after =
            gNdsFighterDLAllDrawP0HardwareTriangleCount +
            gNdsFighterDLAllDrawP1HardwareTriangleCount;
        if (triangles_after > triangles_before)
        {
            NDS_DIAG(gNdsStageGCDrawAllLoopHardwareFighterTriangleCount +=
                triangles_after - triangles_before);
        }
    }
#if NDS_TICK_HUD
    gNdsTickHudFighterTicks += NDS_TICK_HUD_SPAN_CLOCK() - owner_start;
#endif
}

static void ndsFighterDisplayContractSubmitStageFighters(void)
{
    GObj *saved_camera = gGCCurrentCamera;
    u32 slot;

    gGCCurrentCamera = ndsBattleCompatMainCameraGObj();
    for (slot = 0u; slot < GMCOMMON_PLAYERS_MAX; slot++)
    {
        GObj *fighter_gobj = ndsFighterManagerLiveGObj(slot);

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
    ndsFtrLeanFrameEnd();
    gGCCurrentCamera = saved_camera;
}

/* P2-2p8 Phase 1 slice 1: the lean fighter path shares this translation
 * unit (it reads the draw plan, contract and matrix statics above). */
#include "renderer_fighter_lean.c"
