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

typedef struct NDSNativeBakedOp
{
    u32 w0;
    u32 w1;
    u16 arg;    /* EMIT: group; MATERIAL: segment-E slot */
    u8 op;      /* an NDS_NATIVE_STATE_* effect, or NDS_NATIVE_BAKED_OP_* */
    u8 pad;
} NDSNativeBakedOp;

typedef struct NDSNativeBakedGroup
{
    const s16 *verts;
    const u32 *colors;
    const u16 *indices;
    u8 vertex_count;
    u8 triangle_count;
    u16 flags;          /* NDS_NATIVE_BAKED_GROUP_I_COVERAGE */
} NDSNativeBakedGroup;

/* The group draws an I4/I8 tile under a combine that reads TEXEL0 alpha
 * (Koffing's smog): its texture bakes the intensity as coverage
 * (sNdsRendererHardwareIntensityCoverage). */
#define NDS_NATIVE_BAKED_GROUP_I_COVERAGE 1u

typedef struct NDSNativeBakedRoot
{
    u32 root;
    u32 file_bytes;     /* the source payload the ops' offsets live in */
    u32 enddl_w0;       /* the root list's last word, a layout fingerprint */
    u16 asset_id;
    u16 gobj_kind;      /* GObj id the root is admitted for */
    u16 first_op;
    u16 op_count;
    u16 top_words;      /* root list length, ENDDL included */
    u8 material_slots;
    u8 pad;
} NDSNativeBakedRoot;

#include "generated/nds_native_item_baked.generated.inc"

__attribute__((used)) volatile u32 gNdsItemBakedDrawCount;
__attribute__((used)) volatile u32 gNdsItemBakedSubmitFailCount;
__attribute__((used)) volatile u32 gNdsItemBakedSubmitStep;
__attribute__((used)) volatile u32 gNdsItemBakedAlpha;
__attribute__((used)) volatile u32 gNdsItemBakedLastRoot;

/* The root `dl` names, if one is baked for this GObj kind; NULL otherwise.
 * `dl` must be the root's first word: its ENDDL word is the fingerprint. */
const void *ndsNativeBakedItemFind(u32 asset_id, u32 root, u32 gobj_kind,
                                   const Gfx *dl, u32 *material_slots)
{
    u32 i;

    if (!NDS_NATIVE_BAKED_ASSET_MATCH(asset_id))
    {
        return NULL;
    }
    for (i = 0u; i < NDS_NATIVE_BAKED_ROOT_COUNT; i++)
    {
        const NDSNativeBakedRoot *r = &sNdsNativeBakedRoots[i];

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
    NDSRendererTraversalState state;
    u32 i;

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

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    for (i = 0u; i < (u32)r->op_count; i++)
    {
        const NDSNativeBakedOp *op = &sNdsNativeBakedOps[r->first_op + i];

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
            const NDSNativeBakedGroup *g = &sNdsNativeBakedGroups[op->arg];
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
