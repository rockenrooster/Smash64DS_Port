/* Baked item/weapon roots. TEXTUAL INCLUDE ONLY, after
 * nds_native_item_wave1_emit.exec.inc.
 *
 * The hand-written item owners bake one root each as helper calls and
 * arrays. The roots in generate_nds_native_item_baked.py have the same shape
 * -- fixed state words, fixed geometry, at most live segment-E materials --
 * so the generator writes them as data instead: native state deltas (run
 * through the shared ndsRendererNativeApplyStateDelta), ENV colours,
 * material hooks and geometry groups for ndsNativeItemWave1Emit, in source
 * order. Nothing here reads a source display list; the adapter only checks
 * the root's ENDDL word as a layout fingerprint.
 *
 * 2026-09-30: the roots a full-match lab census found drawn with no native
 * owner -- PK Fire's pillar, Bob-omb walking left, the placed Bumper, the Ray
 * Gun's shot, Goldeen, Koffing and its smog. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

#include <nds/nds_native_baked_types.h>

#include "generated/nds_native_item_baked.generated.inc"

__attribute__((used)) volatile u32 gNdsItemBakedDrawCount;
__attribute__((used)) volatile u32 gNdsItemBakedSubmitFailCount;
__attribute__((used)) volatile u32 gNdsItemBakedSubmitStep;
__attribute__((used)) volatile u32 gNdsItemBakedAlpha;
__attribute__((used)) volatile u32 gNdsItemBakedLastRoot;
const NDSNativeBakedRoot *gNdsNativeBakedRoomRoots;
const NDSNativeBakedOp *gNdsNativeBakedRoomOps;
const NDSNativeBakedGroup *gNdsNativeBakedRoomGroups;
u32 gNdsNativeBakedRoomRootCount;

/* The root `dl` names, if one is baked for this GObj kind; NULL otherwise.
 * `dl` must be the root's first word: its ENDDL word is the fingerprint. */
const void *ndsNativeBakedItemFind(u32 asset_id, u32 root, u32 gobj_kind,
                                   const Gfx *dl, u32 *material_slots)
{
    u32 i;

    const NDSNativeBakedRoot *table = sNdsNativeBakedRoots;
    u32 count = NDS_NATIVE_BAKED_ROOT_COUNT;

    if (!NDS_NATIVE_BAKED_ASSET_MATCH(asset_id))
    {
#if NDS_P2_1P_GAME
        /* The room table exists only while the ending runs (it loads
         * it into its own heap). */
        if (NDS_NATIVE_BAKED_ROOM_ASSET_MATCH(asset_id) &&
            (gNdsNativeBakedRoomRoots != NULL))
        {
            table = gNdsNativeBakedRoomRoots;
            count = gNdsNativeBakedRoomRootCount;
        }
        else
#endif
        {
            return NULL;
        }
    }
    for (i = 0u; i < count; i++)
    {
        const NDSNativeBakedRoot *r = &table[i];

        if ((r->root == root) && ((u32)r->asset_id == asset_id) &&
            ((u32)r->gobj_kind == gobj_kind))
        {
            if ((dl == NULL) ||
                (dl[r->top_words - 1u].words.w0 != r->enddl_w0))
            {
                return NULL;
            }
            if (material_slots != NULL)
            {
                *material_slots = r->material_slots;
            }
            return r;
        }
    }
    return NULL;
}

/* P2-6 (2026-10-01): A ROOT WHOLLY OUTSIDE THE VIEW EMITS NOTHING. Board the
 * Platforms submits every platform (body and lights, ~30 roots) each frame
 * while the camera shows a few; each emit binds its texture, loads both
 * matrices and encodes every corner on the CPU (Kirby's board: 2.87M busy
 * cycles a frame, the baked emit 208K of it, 1p-pf09b-st7). A root's
 * object-space bounds are taken once from its groups' vertices (cached by
 * root and heap generation); when every corner lies outside one side plane
 * of its own clip matrix (modelview x projection, the same test as the stage
 * GX runs) its EMIT ops are skipped. Its state ops still run, so what the
 * next list inherits is unchanged. A/B word gNdsBakedCull (0 = emit all). */
#define NDS_NATIVE_BAKED_BOUNDS_CACHE 64u
typedef struct NDSNativeBakedBounds
{
    const NDSNativeBakedRoot *root;
    u32 generation;
    s16 minimum[3];
    s16 maximum[3];
    u32 empty;
} NDSNativeBakedBounds;
static NDSNativeBakedBounds sNdsNativeBakedBounds[NDS_NATIVE_BAKED_BOUNDS_CACHE];
volatile u32 gNdsBakedCull __attribute__((used, section(".data"))) = 1u;
__attribute__((used)) volatile u32 gNdsBakedCulledRoots;

static sb32 ndsNativeBakedRootOutsideView(
    const NDSNativeBakedRoot *r, const NDSNativeBakedOp *op_table,
    const NDSNativeBakedGroup *group_table, const NDSRendererConfig *config)
{
    NDSNativeBakedBounds *b = &sNdsNativeBakedBounds[
        ((u32)(uintptr_t)r >> 2) & (NDS_NATIVE_BAKED_BOUNDS_CACHE - 1u)];
    NDSRendererMatrix20p12 m;
    u32 plane;
    u32 axis;

    if ((b->root != r) || (b->generation != gNdsTaskmanHeapGeneration))
    {
        s32 lo[3] = { 32767, 32767, 32767 };
        s32 hi[3] = { -32768, -32768, -32768 };
        u32 any = FALSE;
        u32 i;

        for (i = 0u; i < (u32)r->op_count; i++)
        {
            const NDSNativeBakedOp *op = &op_table[r->first_op + i];
            const NDSNativeBakedGroup *g;
            u32 v;

            if (op->op != NDS_NATIVE_BAKED_OP_EMIT)
            {
                continue;
            }
            g = &group_table[op->arg];
            for (v = 0u; v < (u32)g->vertex_count; v++)
            {
                for (axis = 0u; axis < 3u; axis++)
                {
                    s32 c = (s32)g->verts[(v * 5u) + axis];

                    lo[axis] = (c < lo[axis]) ? c : lo[axis];
                    hi[axis] = (c > hi[axis]) ? c : hi[axis];
                }
                any = TRUE;
            }
        }
        b->root = r;
        b->generation = gNdsTaskmanHeapGeneration;
        b->empty = (any == FALSE) ? TRUE : FALSE;
        for (axis = 0u; axis < 3u; axis++)
        {
            b->minimum[axis] = (s16)lo[axis];
            b->maximum[axis] = (s16)hi[axis];
        }
    }
    if (b->empty != FALSE)
    {
        return FALSE;
    }
    ndsRendererMtxMul20p12(config->initial_modelview,
                           config->initial_projection, &m);
    for (plane = 0u; plane < 4u; plane++)
    {
        u32 col = plane >> 1;
        s64 sign = ((plane & 1u) != 0u) ? 1 : -1;
        s64 d = (s64)m.m[3][3] + (sign * (s64)m.m[3][col]);

        for (axis = 0u; axis < 3u; axis++)
        {
            s64 c = (s64)m.m[axis][3] + (sign * (s64)m.m[axis][col]);

            d += c * (s64)((c >= 0) ? b->maximum[axis] : b->minimum[axis]);
        }
        if (d < 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}

/* TRUE while `handle` is a root of the ending's loaded room table; the table
 * leaves with the ending's heap, after which no handle is one. */
sb32 ndsNativeBakedRootIsRoom(const void *handle)
{
    const NDSNativeBakedRoot *r = (const NDSNativeBakedRoot *)handle;

    return ((gNdsNativeBakedRoomRoots != NULL) &&
            (r >= gNdsNativeBakedRoomRoots) &&
            (r < gNdsNativeBakedRoomRoots + gNdsNativeBakedRoomRootCount)) ?
        TRUE : FALSE;
}

/* The fast lane's early exit (renderer_adapter_stage.c): TRUE when the
 * root `handle` under `config`'s matrices is wholly outside the view, so the
 * lane can skip its stats reset, material snapshots and submit. The lane
 * resets its stats per draw, so nothing a skipped root would have set is
 * inherited by the next. */
sb32 ndsNativeBakedRootOffscreen(const void *handle,
                                 const NDSRendererConfig *config)
{
    const NDSNativeBakedRoot *r = (const NDSNativeBakedRoot *)handle;
    const NDSNativeBakedOp *op_table = sNdsNativeBakedOps;
    const NDSNativeBakedGroup *group_table = sNdsNativeBakedGroups;
    NDSRendererConfig shifted_config;
    NDSRendererMatrix20p12 shifted_modelview;

    if ((gNdsBakedCull == 0u) || (r == NULL) || (config == NULL) ||
        (config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        return FALSE;
    }
#if NDS_P2_1P_GAME
    if ((gNdsNativeBakedRoomRoots != NULL) &&
        (r >= gNdsNativeBakedRoomRoots) &&
        (r < gNdsNativeBakedRoomRoots + gNdsNativeBakedRoomRootCount))
    {
        op_table = gNdsNativeBakedRoomOps;
        group_table = gNdsNativeBakedRoomGroups;
    }
#endif
    if (r->vertex_shift != 0u)
    {
        const s32 scale = (s32)1 << r->vertex_shift;
        u32 row;
        u32 col;

        shifted_modelview = *config->initial_modelview;
        for (row = 0u; row < 3u; row++)
        {
            for (col = 0u; col < 4u; col++)
            {
                shifted_modelview.m[row][col] *= scale;
            }
        }
        shifted_config = *config;
        shifted_config.initial_modelview = &shifted_modelview;
        config = &shifted_config;
    }
    if (ndsNativeBakedRootOutsideView(r, op_table, group_table, config) ==
        FALSE)
    {
        return FALSE;
    }
    NDS_DIAG(gNdsBakedCulledRoots++);
    return TRUE;
}

sb32 ndsRendererSubmitNativeBaked(
    const void *handle, const void *file_base_ptr, u32 file_bytes,
    const NDSRendererNativeMaterial *materials, u32 material_count,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const NDSNativeBakedRoot *r = (const NDSNativeBakedRoot *)handle;
    const u8 *base = (const u8 *)file_base_ptr;
    const NDSNativeBakedOp *op_table = sNdsNativeBakedOps;
    const NDSNativeBakedGroup *group_table = sNdsNativeBakedGroups;
    NDSRendererTraversalState state;
    NDSRendererConfig shifted_config;
    NDSRendererMatrix20p12 shifted_modelview;
    sb32 skip_emit;
    u32 i;
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    u32 lab_total = cpuGetTiming();
    u32 lab_mark = 0u;
#endif

#if NDS_P2_1P_GAME
    /* A room root (the ending's loaded table) indexes the room's own ops
     * and groups. */
    if ((gNdsNativeBakedRoomRoots != NULL) &&
        (r >= gNdsNativeBakedRoomRoots) &&
        (r < gNdsNativeBakedRoomRoots + gNdsNativeBakedRoomRootCount))
    {
        op_table = gNdsNativeBakedRoomOps;
        group_table = gNdsNativeBakedRoomGroups;
    }
#endif

    if ((r == NULL) || (base == NULL) || (config == NULL) || (stats == NULL))
    {
        gNdsItemBakedSubmitStep = 11u;
        return FALSE;
    }
    gNdsItemBakedLastRoot = ((u32)r->asset_id << 20) | r->root;
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsItemBakedSubmitStep = 12u;
        return FALSE;
    }
    if ((file_bytes < r->file_bytes) ||
        (material_count < (u32)r->material_slots) ||
        ((r->material_slots != 0u) && (materials == NULL)))
    {
        gNdsItemBakedSubmitStep = 2u;
        return FALSE;
    }
    if (r->vertex_shift != 0u)
    {
        /* Stored vertices are source >> vertex_shift: scale the object's
         * axes (rows 0-2) back, translation untouched. */
        const s32 scale = (s32)1 << r->vertex_shift;
        u32 row;
        u32 col;

        shifted_modelview = *config->initial_modelview;
        for (row = 0u; row < 3u; row++)
        {
            for (col = 0u; col < 4u; col++)
            {
                shifted_modelview.m[row][col] *= scale;
            }
        }
        shifted_config = *config;
        shifted_config.initial_modelview = &shifted_modelview;
        config = &shifted_config;
    }

    skip_emit = ((gNdsBakedCull != 0u) &&
                 (ndsNativeBakedRootOutsideView(
                      r, op_table, group_table, config) != FALSE)) ?
        TRUE : FALSE;
    if (skip_emit != FALSE)
    {
        NDS_DIAG(gNdsBakedCulledRoots++);
    }
    NDS_LAB_BAKED_INC(12, (skip_emit == FALSE) ? 1u : 0u);
    NDS_LAB_BAKED_MARK(lab_mark);
    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    NDS_LAB_BAKED_ADD(3, lab_mark);
    /* One matrix load for the root's groups (ndsNativeItemWave1Emit). */
    sNdsNativeItemWave1MatrixGeneration = ndsRendererNextMatrixGeneration();
    for (i = 0u; i < (u32)r->op_count; i++)
    {
        const NDSNativeBakedOp *op = &op_table[r->first_op + i];

        NDS_LAB_BAKED_MARK(lab_mark);
        switch (op->op)
        {
        case NDS_NATIVE_BAKED_OP_ENV:
            /* G_SETENVCOLOR: the hand owners' stats->env_color write. */
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(&state);
            stats->env_color = op->w1;
            stats->color_command_count++;
            break;
        case NDS_NATIVE_BAKED_OP_MATERIAL:
            ndsRendererNativeApplyMaterial(&materials[op->arg], stats, &state);
            break;
        case NDS_NATIVE_BAKED_OP_EMIT:
        {
            const NDSNativeBakedGroup *g = &group_table[op->arg];
            sb32 emitted;

            if (skip_emit != FALSE)
            {
                break;
            }
            NDS_LAB_BAKED_INC(13, 1u);
            NDS_LAB_BAKED_INC(14, (u32)g->triangle_count * 3u);
            sNdsRendererHardwareIntensityCoverage =
                ((g->flags & NDS_NATIVE_BAKED_GROUP_I_COVERAGE) != 0u) ?
                    1u : 0u;
            sNdsNativeItemWave1TexMemoSet = (u32)op->arg;
            emitted = ndsNativeItemWave1Emit(
                g->verts, g->colors, g->indices,
                (u32)g->vertex_count, (u32)g->triangle_count,
                (u32)g->triangle_count * 3u,
                config, stats, &state,
                &gNdsItemBakedSubmitStep, &gNdsItemBakedAlpha);
            sNdsRendererHardwareIntensityCoverage = 0u;
            sNdsNativeItemWave1TexMemoSet =
                NDS_NATIVE_ITEM_WAVE1_TEXMEMO_SET_NONE;
            NDS_LAB_BAKED_ADD(5, lab_mark);
            if (emitted == FALSE)
            {
                sNdsNativeItemWave1MatrixGeneration = 0u;
                return FALSE;
            }
            continue;
        }
        default:
        {
            /* A native state delta; IMAGE's w1 is the file offset. */
            const NDSNativeStateDelta delta = {
                op->w0, op->w1, op->op, { 0u, 0u, 0u }
            };

            ndsRendererNativeApplyStateDelta(&delta, base, stats, &state);
            break;
        }
        }
        NDS_LAB_BAKED_ADD(4, lab_mark);
    }
    sNdsNativeItemWave1MatrixGeneration = 0u;
    gNdsItemBakedSubmitStep = 9u;
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    NDS_DIAG(gNdsLabBakedAcc[2] += cpuGetTiming() - lab_total);
#endif
    return TRUE;
}

#endif
