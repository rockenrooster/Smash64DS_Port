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
    u32 i;

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

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    for (i = 0u; i < (u32)r->op_count; i++)
    {
        const NDSNativeBakedOp *op = &op_table[r->first_op + i];

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

            sNdsRendererHardwareIntensityCoverage =
                ((g->flags & NDS_NATIVE_BAKED_GROUP_I_COVERAGE) != 0u) ?
                    1u : 0u;
            emitted = ndsNativeItemWave1Emit(
                g->verts, g->colors, g->indices,
                (u32)g->vertex_count, (u32)g->triangle_count,
                (u32)g->triangle_count * 3u,
                config, stats, &state,
                &gNdsItemBakedSubmitStep, &gNdsItemBakedAlpha);
            sNdsRendererHardwareIntensityCoverage = 0u;
            if (emitted == FALSE)
            {
                return FALSE;
            }
            break;
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
    }
    gNdsItemBakedSubmitStep = 9u;
    return TRUE;
}

#endif
