/* Native renderer, the lean list materializer: root blocks, root keys, material
 * corners and the walk's texture memo, compiled into an entry's packet words.
 * A slice of nds_renderer.c. */

/* ---- slice 4: the list materializer --------------------------------------
 * ndsRendererExecuteNativeFighterOwnerProduction's walk, its run prepare
 * (ndsRendererNativePrepareProductionRunCore, packet mode 0, no hierarchy
 * run), its shade (ndsRendererNativeShadeProductionActions) and its two
 * emitters, into a lean entry: no geometry reaches GX; the words and patch
 * tables are the ones the packet recorder (deleted 2026-10-05) captured from
 * a live draw, and the matrices take the lean layout. The run texture memo is never read: a
 * donor-table root binds its own texture (production's memo keys on the run
 * index and the player, so it replays the owner root's texture there -- the
 * oracle names that class instead of counting it). */
typedef struct NDSFtrLeanPacker
{
    u32 *words;
    u32 count;
    u32 capacity;
    u32 cmd_word;
    u32 cmd_slot;
    u32 header_params;
    u32 header_valid;
    u32 fault;
} NDSFtrLeanPacker;

/* The packed GX command: a header word carries up to four opcodes and
 * each opcode's parameters follow in order; a header whose commands take no
 * parameters at all gets one dummy word before the next header. Returns the
 * index of the command's first parameter word. */
static u32 ndsFtrLeanPack(NDSFtrLeanPacker *pk, u32 opcode, u32 param_count)
{
    u32 first;

    if (pk->fault != 0u)
    {
        return 0u;
    }
    if (pk->cmd_slot >= 4u)
    {
        if ((pk->header_valid != 0u) && (pk->header_params == 0u))
        {
            if (pk->count >= pk->capacity)
            {
                pk->fault = 1u;
                return 0u;
            }
            pk->words[pk->count++] = 0u;
        }
        if (pk->count >= pk->capacity)
        {
            pk->fault = 1u;
            return 0u;
        }
        pk->cmd_word = pk->count++;
        pk->words[pk->cmd_word] = 0u;
        pk->cmd_slot = 0u;
        pk->header_params = 0u;
        pk->header_valid = 1u;
    }
    if (pk->count + param_count > pk->capacity)
    {
        pk->fault = 1u;
        return 0u;
    }
    pk->words[pk->cmd_word] |= opcode << (pk->cmd_slot * 8u);
    pk->cmd_slot++;
    pk->header_params += param_count;
    first = pk->count;
    pk->count += param_count;
    return first;
}

static inline void ndsFtrLeanPack1(NDSFtrLeanPacker *pk, u32 opcode, u32 word)
{
    u32 i = ndsFtrLeanPack(pk, opcode, 1u);

    if (pk->fault == 0u)
    {
        pk->words[i] = word;
    }
}

typedef struct NDSFtrLeanMat
{
    NDSFtrLeanPacker pk;
    NDSFighterPacket *packet;
    u32 root;                   /* current root (rec->current_root) */
    u32 prim_overridden;        /* rec->prim_overridden */
    u32 texgen_group;           /* current run's group (rec->texgen_group) */
    u32 light_written;
    u32 light_cmd_word;         /* the head's LIGHT_VECTOR header word */
    u32 light_cmd_slot;         /* ... and its byte in it */
    u32 run_count;
    u32 triangle_count;
    u32 raw_triangles;
    u32 raw_reuse;
    u32 cross_triangles;
    u32 cross_reuse;
    u32 vertex_loads;
    u32 prepare_begin;
    u32 prepare_reuse;
    u32 texture_binds;
    u32 tint_binds;
    u32 tint_pending;
    u32 mark;                   /* lab: the materializer's part timer */
    u32 vtx10;                  /* corners as VTX_10: 1 at 4x, 2 at 2x
                                   (NDS_FTR_LEAN_ENTRY_VTX16/VTX10H) */
    u32 vtx10_fail;             /* NDS_FTR_LEAN_VTX_FAIL_*: walk again */
    u32 hw_texgen_ok;           /* the owner takes GX texgen (1P owners) */
    u32 hw_texgen;              /* the current run's texture matrix is loaded */
    u32 root_tex_mask;          /* packet->textures[] the current root bound */
    u32 root_lit;               /* the current root shaded a lit epoch */
    u32 root_fence;             /* ... bound a texture with no cache entry */
} NDSFtrLeanMat;

/* P2-2p8 (2026-10-05): route-1 lists take VTX_10 corners where every corner
 * fits (NDS_FTR_LEAN_ENTRY_VTX16 in renderer_fighter_lean.h). The GXFIFO DMA
 * of the four lean lists cost ~14.8K ticks a frame of bus stalls on the gate
 * (same ROM, DMA skipped vs kept); VTX_16's second parameter word was a third
 * of those lists' words. */

/* ~300 clock reads per materialization, and materializations are tail
 * frames: attribution, so only in a phase-ticks build (NDS_FTR_LEAN_TIMED). */
#if NDS_FTR_LEAN_TIMED
#define NDS_FTR_LEAN_MAT_PART(mat, index)                                  \
    do                                                                     \
    {                                                                      \
        u32 part_now_ = cpuGetTiming();                                    \
        gNdsFtrLean.materialize_part_ticks[index] +=                       \
            part_now_ - (mat)->mark;                                       \
        (mat)->mark = part_now_;                                           \
    } while (0)
#else
#define NDS_FTR_LEAN_MAT_PART(mat, index) ((void)0)
#endif

/* A span that writes the prim colour sets the recorder's prim_overridden
 * (the PRIM case of ndsRendererNativeApplyStateDelta). */
static u32 ndsFtrLeanSpanSetsPrim(u16 first, u32 count)
{
    const NDSNativeFighterRuntimeTables *tables = sNdsNativeFighterActiveTables;
    u32 i;

    if ((count == 0u) || (first == NDS_NATIVE_STATE_NONE))
    {
        return FALSE;
    }
    for (i = 0u; i < count; i++)
    {
        u32 delta_index = tables->state_sequence[first + i];

        if (tables->state_deltas[delta_index].effect == NDS_NATIVE_STATE_PRIM)
        {
            return TRUE;
        }
    }
    return FALSE;
}

/* ---- 2026-10-13: ROOT BLOCKS ---------------------------------------------
 * A lean list is the head and then one block per root: the root's seed
 * LOAD4x3, its epochs' state, shades and runs. Four Crashes materialized 71
 * lists in 1,200 frames, 53 of them for keys the slot had never drawn -- a
 * blink, a mouth, a hand -- and each was a whole walk (~550K ticks) of twenty
 * roots of which one or two had changed. A root whose runs read only RDP state
 * the root sets first (ndsFtrLeanRootSelfContained: every root of every P4
 * owner image but a handful) walks to the same words whatever the roots before
 * it left: its block is a function of the root's own inputs -- tables,
 * preamble, light preamble, material rows, GX slots, colour modulate and corner
 * format (ndsFtrLeanRootKey). So a list keeps, per root, where its block sits
 * and what it added to the packet's tables (sites, tint binds, textures,
 * counters) under that key, and a later list of the slot whose root keys the
 * same copies the block from the other entry or the spare instead of walking
 * it. Blocks start and end on a packed-command header boundary, so a block is
 * position independent but for the word indices the packet keeps, which the
 * copy re-bases. A list copies only when every root of its plan is
 * self-contained; a donor only while it holds its base words under the live
 * tint-tile set, and a block only while every texture it binds is still the
 * cache entry it bound. The records live in the packet's texgen site table,
 * which a list without texgen sites never uses (a list with any keeps none).
 * Lab: gNdsFtrLeanRootCopyOff = 1 walks every root. */
typedef struct NDSFtrLeanRootRec
{
    u32 key;                    /* ndsFtrLeanRootKey */
    u16 start;                  /* block words [start, end) */
    u16 end;
    u8 site_first;              /* packet->sites the block appended */
    u8 site_count;
    u8 tint_first;              /* packet->tint_binds it appended */
    u8 tint_count;
    u32 tex_mask;               /* packet->textures[] it bound */
    u16 triangles;
    u16 vertex_loads;
    u16 raw_triangles;
    u16 cross_triangles;
    u8 raw_runs;
    u8 cross_runs;
    u8 prepare_begin;
    u8 prepare_reuse;
    u8 texture_binds;
    u8 tint_binds;
    u8 tint_seen;
    u8 flags;                   /* NDS_FTR_LEAN_ROOT_* */
    /* A run's POLYGON_ATTR carries the walk's combine count (mod 64) as its
     * polygon ID, a count that runs on across roots: the count at the
     * block's start and the combines the block applied, so a copy re-bases
     * the IDs (ndsFtrLeanRebasePolyIds) and advances the count. */
    u8 combine_base;
    u8 combine_count;
} NDSFtrLeanRootRec;
#define NDS_FTR_LEAN_ROOT_COPY 1u   /* the block may be copied */
#define NDS_FTR_LEAN_ROOT_LIT 2u    /* it shades a lit epoch */
#define NDS_FTR_LEAN_ROOT_FENCE 4u  /* it set needs_fence (a tint bind) */
/* The records a packet's texgen site table holds. */
#define NDS_FTR_LEAN_ROOT_RECS 28u
_Static_assert(sizeof(NDSFtrLeanRootRec) == 36u, "root record: 36 bytes");
_Static_assert(sizeof(((NDSFighterPacket *)0)->texgen_sites) >=
                   (NDS_FTR_LEAN_ROOT_RECS * sizeof(NDSFtrLeanRootRec)),
               "the root records fit the texgen site table");
_Static_assert((offsetof(NDSFighterPacket, texgen_sites) & 3u) == 0u,
               "the root records are word aligned");

#if defined(__arm__)
volatile u32 gNdsFtrLeanRootCopyOff __attribute__((used, section(".data"))) =
    0u;
#else
volatile u32 gNdsFtrLeanRootCopyOff;
#endif

static inline NDSFtrLeanRootRec *ndsFtrLeanRootRecs(NDSFighterPacket *packet)
{
    return (NDSFtrLeanRootRec *)(void *)packet->texgen_sites;
}

/* The othermode_h bits a G_SETOTHERMODE_H / G_RDPSETOTHERMODE writes
 * (ndsRendererRecordOtherMode's decode). */
static u32 ndsFtrLeanOtherModeHBits(u32 w0)
{
    const u32 op = w0 >> 24;
    u32 bits;
    u32 pos;

    if (op == NDS_RENDERER_OP_RDPSETOTHERMODE)
    {
        return 0x00ffffffu;
    }
    if (op != NDS_RENDERER_OP_SETOTHERMODE_H)
    {
        return 0u;
    }
    bits = (w0 & 0xffu) + 1u;
    pos = (w0 >> 8) & 0xffu;
    if ((bits > 32u) || (pos >= 32u) || ((bits + pos) > 32u))
    {
        return 0u;
    }
    if (bits >= 32u)
    {
        return 0xffffffffu;
    }
    return ((1u << bits) - 1u) << (32u - pos - bits);
}

/* What any root of a table set writes: state no root writes is the walk's
 * own seed for every root, so reading it carries nothing between roots. */
#define NDS_FTR_LEAN_ANY_TLUT 1u
#define NDS_FTR_LEAN_ANY_BLEND 2u
#define NDS_FTR_LEAN_ANY_LIGHT 4u
#define NDS_FTR_LEAN_ANY_LIGHT_DIR 8u
#define NDS_FTR_LEAN_SELF_IMAGES 8u
typedef struct NDSFtrLeanSelfImage
{
    const NDSNativeFighterRuntimeTables *tables;
    u32 oh_bits;
    u32 any;
} NDSFtrLeanSelfImage;
static NDSFtrLeanSelfImage sNdsFtrLeanSelfImages[NDS_FTR_LEAN_SELF_IMAGES];
static u32 sNdsFtrLeanSelfImageGen;
static u32 sNdsFtrLeanSelfImageNext;

static const NDSFtrLeanSelfImage *NDS_FIGHTER_PACKET_COLD_CODE
ndsFtrLeanSelfImageOf(const NDSNativeFighterRuntimeTables *tables)
{
    NDSFtrLeanSelfImage *img;
    u32 i;

    if (sNdsFtrLeanSelfImageGen != gNdsTaskmanHeapGeneration + 1u)
    {
        memset(sNdsFtrLeanSelfImages, 0, sizeof(sNdsFtrLeanSelfImages));
        sNdsFtrLeanSelfImageGen = gNdsTaskmanHeapGeneration + 1u;
        sNdsFtrLeanSelfImageNext = 0u;
    }
    for (i = 0u; i < NDS_FTR_LEAN_SELF_IMAGES; i++)
    {
        if (sNdsFtrLeanSelfImages[i].tables == tables)
        {
            return &sNdsFtrLeanSelfImages[i];
        }
    }
    img = &sNdsFtrLeanSelfImages[sNdsFtrLeanSelfImageNext];
    sNdsFtrLeanSelfImageNext =
        (sNdsFtrLeanSelfImageNext + 1u) % NDS_FTR_LEAN_SELF_IMAGES;
    img->tables = tables;
    img->oh_bits = 0u;
    img->any = 0u;
    for (i = 0u; i < tables->state_delta_count; i++)
    {
        const NDSNativeStateDelta *d = &tables->state_deltas[i];

        if (d->effect == NDS_NATIVE_STATE_OTHERMODE)
        {
            img->oh_bits |= ndsFtrLeanOtherModeHBits(d->w0);
        }
        else if (d->effect == NDS_NATIVE_STATE_LOAD_TLUT)
        {
            img->any |= NDS_FTR_LEAN_ANY_TLUT;
        }
        else if (d->effect == NDS_NATIVE_STATE_BLEND)
        {
            img->any |= NDS_FTR_LEAN_ANY_BLEND;
        }
        else if (d->effect == NDS_NATIVE_STATE_LIGHT_COLOR)
        {
            img->any |= NDS_FTR_LEAN_ANY_LIGHT;
        }
    }
    return img;
}

/* One root's state writes in walk order, against what its runs read. A
 * material is not counted as a write (its effects vary by frame), so state
 * only a material sets reads as carried. Returns FALSE at the first read of
 * carried state, else NDS_FTR_LEAN_SELF_OK, with NDS_FTR_LEAN_SELF_BLEND when a
 * run reads the blend colour the roots before it left (where anything sets
 * one). The blend colour is in no list word: a list reads it only for the
 * G_AC_THRESHOLD decline (ndsFtrLeanMatRun), so such a root keys the carried
 * colour's decline bit (ndsFtrLeanRootKeyCarriedBlend) and a copy of it leaves
 * the colour as its walk would (ndsFtrLeanRootBlendEnd) -- 2026-10-18: Captain
 * Falcon's tables set one, and two thirds of the cast's materializations
 * walked every root for it (~300K ticks each, the P95 tail).
 *   every run      the combine
 *   textured run   G_TEXTURE, an image, a block or tile load, the render
 *                  tile's SETTILE and SETTILESIZE, a TLUT load where anything
 *                  loads one, and every othermode_h bit any root writes
 *   shaded epoch   both light colours where anything moves them (the root's
 *                  light preamble counts), and the preamble's light direction
 *                  where any root's preamble carries one
 *   a load         its tile's SETTILE first. */
#define NDS_FTR_LEAN_HAVE_COMBINE 1u
#define NDS_FTR_LEAN_HAVE_TEXTURE 2u
#define NDS_FTR_LEAN_HAVE_IMAGE 4u
#define NDS_FTR_LEAN_HAVE_LOAD 8u
#define NDS_FTR_LEAN_HAVE_TLUT 16u
#define NDS_FTR_LEAN_HAVE_BLEND 32u
#define NDS_FTR_LEAN_HAVE_LIGHT1 64u
#define NDS_FTR_LEAN_HAVE_LIGHT2 128u
#define NDS_FTR_LEAN_SELF_OK 1u
#define NDS_FTR_LEAN_SELF_BLEND 2u
typedef struct NDSFtrLeanSelfWalk
{
    u32 have;
    u32 oh;
    u32 tiles;
    u32 sizes;
    u32 render_tile;
    u32 bad;
    u8 tile_fmt[8];             /* each tile's SETTILE format */
} NDSFtrLeanSelfWalk;

static void ndsFtrLeanSelfSpan(const NDSNativeFighterRuntimeTables *tables,
                               u16 first, u32 count, NDSFtrLeanSelfWalk *w)
{
    u32 i;

    if ((count == 0u) || (first == NDS_NATIVE_STATE_NONE))
    {
        return;
    }
    for (i = 0u; i < count; i++)
    {
        const NDSNativeStateDelta *d =
            &tables->state_deltas[tables->state_sequence[first + i]];
        const u32 tile = (d->w1 >> 24) & 7u;

        switch (d->effect)
        {
        case NDS_NATIVE_STATE_OTHERMODE:
            w->oh |= ndsFtrLeanOtherModeHBits(d->w0);
            break;
        case NDS_NATIVE_STATE_COMBINE:
            w->have |= NDS_FTR_LEAN_HAVE_COMBINE;
            break;
        case NDS_NATIVE_STATE_TEXTURE:
            w->have |= NDS_FTR_LEAN_HAVE_TEXTURE;
            w->render_tile = (d->w0 >> 8) & 7u;
            break;
        case NDS_NATIVE_STATE_IMAGE:
            w->have |= NDS_FTR_LEAN_HAVE_IMAGE;
            break;
        case NDS_NATIVE_STATE_TILE:
            w->tiles |= 1u << tile;
            w->tile_fmt[tile] = (u8)((d->w0 >> 21) & 7u);
            break;
        case NDS_NATIVE_STATE_TILE_SIZE:
            w->sizes |= 1u << tile;
            break;
        case NDS_NATIVE_STATE_LOAD_BLOCK:
        case NDS_NATIVE_STATE_LOAD_TILE:
            if ((w->tiles & (1u << tile)) == 0u)
            {
                w->bad = 1u;
            }
            w->have |= NDS_FTR_LEAN_HAVE_LOAD;
            break;
        case NDS_NATIVE_STATE_LOAD_TLUT:
            if ((w->tiles & (1u << tile)) == 0u)
            {
                w->bad = 1u;
            }
            w->have |= NDS_FTR_LEAN_HAVE_TLUT;
            break;
        case NDS_NATIVE_STATE_BLEND:
            w->have |= NDS_FTR_LEAN_HAVE_BLEND;
            break;
        case NDS_NATIVE_STATE_LIGHT_COLOR:
            /* G_MW_LIGHTCOL: offset 0x00/0x04 light 1, 0x18/0x1c light 2. */
            w->have |= (((d->w0 & 0xffffu) >= 0x18u) ?
                        NDS_FTR_LEAN_HAVE_LIGHT2 : NDS_FTR_LEAN_HAVE_LIGHT1);
            break;
        default:
            break;
        }
    }
}

/* A material's writes (ndsRendererNativeApplyMaterial): the root key carries
 * its rows, so state a material sets is the root's own input. */
static void ndsFtrLeanSelfMaterial(const NDSRendererNativeMaterial *material,
                                   NDSFtrLeanSelfWalk *w)
{
    const u32 fx = material->effects;

    if ((fx & (NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE |
               NDS_RENDERER_NATIVE_MATERIAL_BLOCK_IMAGE |
               NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE)) != 0u)
    {
        w->have |= NDS_FTR_LEAN_HAVE_IMAGE;
    }
    if ((fx & NDS_RENDERER_NATIVE_MATERIAL_PALETTE_TLUT) != 0u)
    {
        const u32 tile = (material->palette_tile_w1 >> 24) & 7u;

        w->tiles |= 1u << tile;
        w->tile_fmt[tile] = (u8)((material->palette_tile_w0 >> 21) & 7u);
        w->have |= NDS_FTR_LEAN_HAVE_TLUT;
    }
    if ((fx & NDS_RENDERER_NATIVE_MATERIAL_LIGHT1) != 0u)
    {
        w->have |= NDS_FTR_LEAN_HAVE_LIGHT1;
    }
    if ((fx & NDS_RENDERER_NATIVE_MATERIAL_LIGHT2) != 0u)
    {
        w->have |= NDS_FTR_LEAN_HAVE_LIGHT2;
    }
    if ((fx & NDS_RENDERER_NATIVE_MATERIAL_BLEND) != 0u)
    {
        w->have |= NDS_FTR_LEAN_HAVE_BLEND;
    }
    if ((fx & NDS_RENDERER_NATIVE_MATERIAL_LOAD_BLOCK) != 0u)
    {
        if ((w->tiles & (1u << ((material->load_block_w1 >> 24) & 7u))) == 0u)
        {
            w->bad = 1u;
        }
        w->have |= NDS_FTR_LEAN_HAVE_LOAD;
    }
    if ((fx & NDS_RENDERER_NATIVE_MATERIAL_RENDER_TILE_SIZE) != 0u)
    {
        w->sizes |= 1u << ((material->render_tile_size_w1 >> 24) & 7u);
    }
    if ((fx & NDS_RENDERER_NATIVE_MATERIAL_SCROLL_TILE_SIZE) != 0u)
    {
        w->sizes |= 1u << ((material->scroll_tile_size_w1 >> 24) & 7u);
    }
    if ((fx & NDS_RENDERER_NATIVE_MATERIAL_TEXTURE) != 0u)
    {
        w->have |= NDS_FTR_LEAN_HAVE_TEXTURE;
        w->render_tile = (material->texture_w0 >> 8) & 7u;
    }
}

static u32 NDS_FIGHTER_PACKET_COLD_CODE
ndsFtrLeanRootSelfContained(const NDSNativeFighterRuntimeTables *tables,
                            const NDSNativeRoot *root, u32 light_ok,
                            const NDSRendererNativeFighterRoot *input,
                            const NDSFtrLeanSelfImage *img, u32 any)
{
    const NDSRendererNativeFighterPreamble *preamble = input->preamble;
    NDSFtrLeanSelfWalk w;
    u32 carried_blend = 0u;
    u32 e;

    memset(&w, 0, sizeof(w));
    if (light_ok != FALSE)
    {
        w.have |= NDS_FTR_LEAN_HAVE_LIGHT1 | NDS_FTR_LEAN_HAVE_LIGHT2;
    }
    any |= img->any;
    for (e = 0u; e < (u32)root->epoch_count; e++)
    {
        const u32 epoch_index = (u32)root->first_epoch + e;
        const NDSNativeEpoch *epoch = &tables->epochs[epoch_index];
        const u32 family = tables->epoch_direct_policy[epoch_index] &
            NDS_NATIVE_DIRECT_POLICY_FAMILY_MASK;

        ndsFtrLeanSelfSpan(tables, epoch->before_state_first,
                           epoch->before_state_count, &w);
        if ((epoch->material_slot != NDS_NATIVE_MATERIAL_NONE) &&
            ((u32)epoch->material_slot < input->material_count) &&
            (input->materials != NULL))
        {
            ndsFtrLeanSelfMaterial(&input->materials[epoch->material_slot],
                                   &w);
        }
        ndsFtrLeanSelfSpan(tables, epoch->after_state_first,
                           epoch->after_state_count, &w);
        if (w.bad != 0u)
        {
            NDS_FTR_LEAN_CTR(gNdsFtrLean.root_self_fail[0]++);
            return FALSE;
        }
        if (epoch->action_count != 0u)
        {
            if (((any & NDS_FTR_LEAN_ANY_LIGHT) != 0u) &&
                ((w.have & (NDS_FTR_LEAN_HAVE_LIGHT1 |
                            NDS_FTR_LEAN_HAVE_LIGHT2)) !=
                 (NDS_FTR_LEAN_HAVE_LIGHT1 | NDS_FTR_LEAN_HAVE_LIGHT2)))
            {
                NDS_FTR_LEAN_CTR(gNdsFtrLean.root_self_fail[1]++);
                return FALSE;
            }
            if (((any & NDS_FTR_LEAN_ANY_LIGHT_DIR) != 0u) &&
                ((preamble == NULL) ||
                 ((preamble->flags &
                   NDS_RENDERER_NATIVE_PREAMBLE_LIGHT_VALID) == 0u)))
            {
                NDS_FTR_LEAN_CTR(gNdsFtrLean.root_self_fail[2]++);
                return FALSE;
            }
        }
        if (epoch->run_count == 0u)
        {
            continue;
        }
        if ((w.have & NDS_FTR_LEAN_HAVE_COMBINE) == 0u)
        {
            NDS_FTR_LEAN_CTR(gNdsFtrLean.root_self_fail[3]++);
            return FALSE;
        }
        if (((any & NDS_FTR_LEAN_ANY_BLEND) != 0u) &&
            ((w.have & NDS_FTR_LEAN_HAVE_BLEND) == 0u))
        {
            carried_blend = NDS_FTR_LEAN_SELF_BLEND;
        }
        if ((family == NDS_NATIVE_DIRECT_POLICY_TEXTURED_LIT) ||
            (family == NDS_NATIVE_DIRECT_POLICY_TEXTURED_PASS2))
        {
            /* A TLUT only where the render tile is colour-indexed. */
            const u32 need = NDS_FTR_LEAN_HAVE_TEXTURE |
                NDS_FTR_LEAN_HAVE_IMAGE | NDS_FTR_LEAN_HAVE_LOAD |
                ((((any & NDS_FTR_LEAN_ANY_TLUT) != 0u) &&
                  (w.tile_fmt[w.render_tile] == 2u)) ?
                     NDS_FTR_LEAN_HAVE_TLUT : 0u);

            if ((w.have & need) != need)
            {
                NDS_FTR_LEAN_CTR(gNdsFtrLean.root_self_fail[5]++);
                return FALSE;
            }
            if (((w.tiles & (1u << w.render_tile)) == 0u) ||
                ((w.sizes & (1u << w.render_tile)) == 0u))
            {
                NDS_FTR_LEAN_CTR(gNdsFtrLean.root_self_fail[6]++);
                return FALSE;
            }
            if ((img->oh_bits & ~w.oh) != 0u)
            {
                NDS_FTR_LEAN_CTR(gNdsFtrLean.root_self_fail[7]++);
                return FALSE;
            }
        }
    }
    if (carried_blend != 0u)
    {
        NDS_FTR_LEAN_CTR(gNdsFtrLean.root_self_fail[4]++);
    }
    return NDS_FTR_LEAN_SELF_OK | carried_blend;
}

/* The blend colour a walk of `root` leaves, from `carried`: the last
 * G_SETBLENDCOLOR of its state spans and materials, in walk order (before
 * span, material, after span per epoch, then the tail). */
static u32 NDS_FIGHTER_PACKET_COLD_CODE
ndsFtrLeanRootBlendEnd(const NDSNativeFighterRuntimeTables *tables,
                       const NDSNativeRoot *root,
                       const NDSRendererNativeFighterRoot *input, u32 carried)
{
    u32 blend = carried;
    u32 e;
    u32 pass;

    for (e = 0u; e <= (u32)root->epoch_count; e++)
    {
        const NDSNativeEpoch *epoch = (e < (u32)root->epoch_count) ?
            &tables->epochs[(u32)root->first_epoch + e] : NULL;

        for (pass = 0u; pass < 2u; pass++)
        {
            u16 first;
            u32 count;
            u32 i;

            if (epoch == NULL)
            {
                if (pass != 0u)
                {
                    break;
                }
                first = root->tail_state_first;
                count = root->tail_state_count;
            }
            else
            {
                first = (pass == 0u) ? epoch->before_state_first :
                    epoch->after_state_first;
                count = (pass == 0u) ? epoch->before_state_count :
                    epoch->after_state_count;
            }
            if ((count != 0u) && (first != NDS_NATIVE_STATE_NONE))
            {
                for (i = 0u; i < count; i++)
                {
                    const NDSNativeStateDelta *d = &tables->state_deltas[
                        tables->state_sequence[first + i]];

                    if (d->effect == NDS_NATIVE_STATE_BLEND)
                    {
                        blend = d->w1;
                    }
                }
            }
            if ((epoch != NULL) && (pass == 0u) &&
                (epoch->material_slot != NDS_NATIVE_MATERIAL_NONE) &&
                ((u32)epoch->material_slot < input->material_count) &&
                (input->materials != NULL) &&
                ((input->materials[epoch->material_slot].effects &
                  NDS_RENDERER_NATIVE_MATERIAL_BLEND) != 0u))
            {
                blend = input->materials[epoch->material_slot].blend_color;
            }
        }
    }
    return blend;
}

/* A root reading the carried blend colour keys what its walk reads of it:
 * whether a G_AC_THRESHOLD run would decline (ndsFtrLeanMatRun). */
static inline u32 ndsFtrLeanRootKeyCarriedBlend(u32 key, u32 blend_color)
{
    u32 h = (key ^ (((blend_color & 0xf0u) != 0u) ? 0x5bd1e995u :
                                                   0x27d4eb2fu)) * 16777619u;

    h ^= h >> 15;
    return (h != 0u) ? h : 1u;
}

/* ndsFtrLeanRootSelfContained's answers, by root and the inputs it reads
 * beyond the root's own tables (light preamble, preamble light flag, the
 * plan's writers, the materials' state words): most lists repeat them. */
#define NDS_FTR_LEAN_SELF_CACHE 64u
typedef struct NDSFtrLeanSelfCache
{
    const NDSNativeRoot *root;
    u32 sig;
    u32 self;
} NDSFtrLeanSelfCache;
static NDSFtrLeanSelfCache sNdsFtrLeanSelfCache[NDS_FTR_LEAN_SELF_CACHE];
static u32 sNdsFtrLeanSelfCacheGen;

static u32 NDS_FIGHTER_PACKET_COLD_CODE
ndsFtrLeanRootSelfContainedCached(const NDSNativeFighterRuntimeTables *tables,
                                  const NDSNativeRoot *root, u32 light_ok,
                                  const NDSRendererNativeFighterRoot *input,
                                  u32 any)
{
    NDSFtrLeanSelfCache *c;
    u32 sig = 2166136261u;
    u32 k;

    if (sNdsFtrLeanSelfCacheGen != gNdsTaskmanHeapGeneration + 1u)
    {
        memset(sNdsFtrLeanSelfCache, 0, sizeof(sNdsFtrLeanSelfCache));
        sNdsFtrLeanSelfCacheGen = gNdsTaskmanHeapGeneration + 1u;
    }
#define NDS_FTR_LEAN_SELF_MIX(v) sig = (sig ^ (u32)(v)) * 16777619u
    NDS_FTR_LEAN_SELF_MIX((uintptr_t)tables);
    NDS_FTR_LEAN_SELF_MIX(light_ok | (any << 1) |
                          (((input->preamble != NULL) &&
                            ((input->preamble->flags &
                              NDS_RENDERER_NATIVE_PREAMBLE_LIGHT_VALID) !=
                             0u)) ? 0x100u : 0u));
    NDS_FTR_LEAN_SELF_MIX(input->material_count);
    for (k = 0u; (input->materials != NULL) && (k < input->material_count);
         k++)
    {
        const NDSRendererNativeMaterial *mt = &input->materials[k];

        NDS_FTR_LEAN_SELF_MIX(mt->effects);
        NDS_FTR_LEAN_SELF_MIX(mt->palette_tile_w0);
        NDS_FTR_LEAN_SELF_MIX(mt->palette_tile_w1);
        NDS_FTR_LEAN_SELF_MIX(mt->load_block_w1);
        NDS_FTR_LEAN_SELF_MIX(mt->render_tile_size_w1);
        NDS_FTR_LEAN_SELF_MIX(mt->scroll_tile_size_w1);
        NDS_FTR_LEAN_SELF_MIX(mt->texture_w0);
    }
#undef NDS_FTR_LEAN_SELF_MIX
    c = &sNdsFtrLeanSelfCache[(((u32)(uintptr_t)root >> 2) ^ sig) &
                              (NDS_FTR_LEAN_SELF_CACHE - 1u)];
    if ((c->root != root) || (c->sig != sig))
    {
        c->root = root;
        c->sig = sig;
        c->self = ndsFtrLeanRootSelfContained(tables, root, light_ok, input,
                                              ndsFtrLeanSelfImageOf(tables),
                                              any);
    }
    return c->self;
}

/* The inputs a self-contained root's block is a function of: its tables,
 * root, asset base, light preamble, palette slot, corner format, GX slots,
 * preamble and material rows -- the rows by the material words they are
 * built from (`material_key`, the event path's per-root fold). Not the
 * colour modulate: it moves only the shade words (ndsRendererR2Resolve-
 * EpochShade's diffuse/ambient), which a copy re-derives
 * (ndsFtrLeanRederiveSites).
 *
 * The root by its row, not its address: a content's root programs (Meta
 * Knight's Live1..Live6) repeat the base program's rows, and a walk reads
 * only the row. Keyed by address, a block recorded under one program never
 * served another (Meta Knight copied 10% of his roots). Not the preamble's
 * light direction either: the walk writes it only into the head's vector
 * word, which a copy writes from the live preamble (light_word). */
static u32 ndsFtrLeanRootKey(const NDSNativeFighterRuntimeTables *tables,
                             const NDSNativeRoot *root,
                             const NDSRendererNativeFighterRoot *input,
                             const void *root_asset_base,
                             const void *light_preambles, u32 light_count,
                             u32 palette_slot, u32 vtx10, u32 material_key)
{
    const NDSRendererNativeFighterPreamble *pre = input->preamble;
    u32 h = 2166136261u;

#define NDS_FTR_LEAN_ROOT_MIX(v) h = (h ^ (u32)(v)) * 16777619u
    NDS_FTR_LEAN_ROOT_MIX((uintptr_t)tables);
    NDS_FTR_LEAN_ROOT_MIX(root->root_offset);
    NDS_FTR_LEAN_ROOT_MIX((u32)root->first_epoch |
                          ((u32)root->tail_state_first << 16));
    NDS_FTR_LEAN_ROOT_MIX((u32)root->source_command_count |
                          ((u32)root->epoch_count << 16) |
                          ((u32)root->tail_state_count << 24));
    NDS_FTR_LEAN_ROOT_MIX((u32)root->tail_sync_count |
                          ((u32)root->light_preamble << 8));
    NDS_FTR_LEAN_ROOT_MIX((uintptr_t)root_asset_base);
    NDS_FTR_LEAN_ROOT_MIX((uintptr_t)light_preambles);
    NDS_FTR_LEAN_ROOT_MIX(light_count);
    NDS_FTR_LEAN_ROOT_MIX(palette_slot | (vtx10 << 8));
    NDS_FTR_LEAN_ROOT_MIX((u32)input->gx_parent_slot |
                          ((u32)input->gx_store_slot << 8) |
                          ((u32)input->gx_seed_is_identity << 16));
    if (pre != NULL)
    {
        NDS_FTR_LEAN_ROOT_MIX(pre->geometry_mode);
        NDS_FTR_LEAN_ROOT_MIX(pre->cycle_type);
        NDS_FTR_LEAN_ROOT_MIX(pre->render_mode);
        NDS_FTR_LEAN_ROOT_MIX(pre->prim_color);
        NDS_FTR_LEAN_ROOT_MIX(pre->env_color);
        NDS_FTR_LEAN_ROOT_MIX(pre->flags);
    }
    NDS_FTR_LEAN_ROOT_MIX(input->material_count);
    NDS_FTR_LEAN_ROOT_MIX(material_key);
#undef NDS_FTR_LEAN_ROOT_MIX
    h ^= h >> 15;
    return (h != 0u) ? h : 1u;
}

/* End the open packed header, so the next command starts a block. */
static void ndsFtrLeanPackClose(NDSFtrLeanPacker *pk)
{
    if (pk->fault != 0u)
    {
        return;
    }
    if ((pk->header_valid != 0u) && (pk->header_params == 0u))
    {
        if (pk->count >= pk->capacity)
        {
            pk->fault = 1u;
            return;
        }
        pk->words[pk->count++] = 0u;
    }
    pk->cmd_slot = 4u;
    pk->header_valid = 0u;
    pk->header_params = 0u;
}

/* A donor's block is still what a walk would write: its textures are the
 * cache entries it bound. */
static u32 ndsFtrLeanRootRecResident(const NDSFighterPacket *donor,
                                     const NDSFtrLeanRootRec *rec)
{
    u32 mask = rec->tex_mask;

    while (mask != 0u)
    {
        const u32 i = (u32)__builtin_ctz(mask);
        const NDSFighterPacketTexture *t = &donor->textures[i];
        const NDSRendererHardwareTextureCacheEntry *entry;

        mask &= mask - 1u;
        if ((i >= (u32)donor->texture_count) || (t->slot_plus1 == 0u) ||
            ((u32)t->slot_plus1 > NDS_RENDERER_HW_TEXTURE_CACHE_COUNT))
        {
            return FALSE;
        }
        entry = &sNdsRendererHardwareTextureCache[t->slot_plus1 - 1u];
        if ((entry->ready == FALSE) || ((u32)(u16)entry->name != t->name) ||
            (entry->key_generation != t->key_generation))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/* A copied block's shade words, re-derived as ndsFighterPacketApplyTintPrims
 * derives them for the list's modulate and the roots' live prims (the donor
 * may hold them at another modulate). */
static void ndsFtrLeanRederiveSites(NDSFighterPacket *packet, u32 first,
                                    u32 count,
                                    const NDSRendererNativeFighterRoot *inputs,
                                    u32 modulate)
{
    u32 i;

    for (i = first; i < first + count; i++)
    {
        const NDSFighterPacketShadeSite *site = &packet->sites[i];
        u32 material_color = site->material_color;
        u32 diffuse;
        u32 ambient;

        if ((site->prim_from_root != 0u) &&
            ((u32)site->root < packet->root_count) &&
            (inputs[site->root].preamble != NULL))
        {
            material_color = inputs[site->root].preamble->prim_color;
        }
        diffuse = ndsRendererR2MaterialColor15(
            site->light_color_1, material_color, site->use_material,
            modulate);
        ambient = ndsRendererR2MaterialColor15(
            site->light_color_2, material_color, site->use_material,
            modulate);
        diffuse = ndsRendererR2ClampDiffuseToMaterial(
            diffuse, ambient, material_color, site->use_material, modulate);
        packet->words[site->index] = diffuse | (ambient << 16);
    }
}

/* The parameter words of each packed GX command a lean list carries (GBATEK
 * 0x10-0x41); 0xff for an opcode it never carries, which the block walk below
 * refuses. */
static const u8 sNdsFtrLeanGxParams[0x42] = {
    [0x00] = 0u,
    [0x01 ... 0x0f] = 0xffu,
    [0x10] = 1u,  [0x11] = 0u,  [0x12] = 1u,  [0x13] = 1u,
    [0x14] = 1u,  [0x15] = 0u,  [0x16] = 16u, [0x17] = 12u,
    [0x18] = 16u, [0x19] = 12u, [0x1a] = 9u,  [0x1b] = 3u,
    [0x1c] = 3u,  [0x1d ... 0x1f] = 0xffu,
    [0x20] = 1u,  [0x21] = 1u,  [0x22] = 1u,  [0x23] = 2u,
    [0x24] = 1u,  [0x25] = 1u,  [0x26] = 1u,  [0x27] = 1u,
    [0x28] = 1u,  [0x29] = 1u,  [0x2a] = 1u,  [0x2b] = 1u,
    [0x2c ... 0x2f] = 0xffu,
    [0x30] = 1u,  [0x31] = 1u,  [0x32] = 1u,  [0x33] = 1u,
    [0x34 ... 0x3f] = 0xffu,
    [0x40] = 1u,  [0x41] = 0u,
};

/* Add `delta` (mod 64) to the polygon ID of every POLYGON_ATTR in a block of
 * packed commands (the packer's layout: a header of four opcodes, their
 * parameters, a dummy word after a header whose opcodes take none). FALSE
 * when the block does not parse. */
static u32 ndsFtrLeanRebasePolyIds(u32 *words, u32 count, u32 delta)
{
    u32 i = 0u;

    while (i < count)
    {
        const u32 header = words[i++];
        u32 params = 0u;
        u32 slot;

        for (slot = 0u; slot < 4u; slot++)
        {
            const u32 op = (header >> (slot * 8u)) & 0xffu;
            const u32 n = (op < sizeof(sNdsFtrLeanGxParams)) ?
                (u32)sNdsFtrLeanGxParams[op] : 0xffu;

            if (n == 0xffu)
            {
                return FALSE;
            }
            if ((op == 0x29u) && ((i + params) < count))
            {
                u32 *w = &words[i + params];

                *w = (*w & ~((u32)NDS_RENDERER_POLY_ID_MASK << 24)) |
                    ((((*w >> 24) + delta) & NDS_RENDERER_POLY_ID_MASK) << 24);
            }
            params += n;
        }
        i += (params != 0u) ? params : 1u;
    }
    return (i == count) ? TRUE : FALSE;
}

/* Copy `donor`'s root `from` block (record `src`) as this list's root
 * `root_index`: the words at a fresh header, the root entry, sites, tint
 * binds and textures re-based, the counters added. FALSE (nothing written)
 * when a table would overflow. */
static u32 NDS_FIGHTER_PACKET_COLD_CODE
ndsFtrLeanRootCopy(NDSFtrLeanMat *m, u32 root_index,
                   const NDSFighterPacket *donor, u32 from,
                   const NDSFtrLeanRootRec *src,
                   const NDSRendererNativeFighterRoot *inputs,
                   u32 light_word, NDSRendererStats *stats,
                   NDSFtrLeanRootRec *out)
{
    NDSFighterPacket *packet = m->packet;
    NDSFtrLeanPacker *pk = &m->pk;
    const u32 len = (u32)src->end - (u32)src->start;
    const NDSFighterPacketRoot *from_root = &donor->roots[from];
    NDSFighterPacketRoot *to_root = &packet->roots[root_index];
    u32 new_textures = 0u;
    u32 tex_mask = 0u;
    u32 mask;
    u32 site_first;
    u32 tint_first;
    s32 delta;
    u32 i;

    if ((src->end <= src->start) || ((u32)src->end > donor->word_count) ||
        (((u32)src->site_first + src->site_count) >
         (u32)donor->site_count) ||
        (((u32)src->tint_first + src->tint_count) >
         (u32)donor->tint_bind_count) ||
        (from >= donor->root_count) ||
        (((u32)packet->site_count + src->site_count) >
         NDS_FIGHTER_PACKET_SITE_MAX) ||
        (((u32)packet->tint_bind_count + src->tint_count) >
         NDS_FIGHTER_PACKET_TINT_BIND_MAX))
    {
        return FALSE;
    }
    mask = src->tex_mask;
    while (mask != 0u)
    {
        const u32 b = (u32)__builtin_ctz(mask);
        u32 k;

        mask &= mask - 1u;
        for (k = 0u; k < (u32)packet->texture_count; k++)
        {
            if (packet->textures[k].slot_plus1 ==
                donor->textures[b].slot_plus1)
            {
                break;
            }
        }
        if (k == (u32)packet->texture_count)
        {
            new_textures++;
        }
    }
    if (((u32)packet->texture_count + new_textures) >
        NDS_FIGHTER_PACKET_TEXTURE_MAX)
    {
        return FALSE;
    }
    if (m->hw_texgen != 0u)
    {
        /* The previous run's GX texgen matrix stops here, as the block's
         * first run would stop it (MatRun): the block was recorded behind a
         * root that left none. */
        ndsFtrLeanPack1(pk, REG2ID(MATRIX_CONTROL), (u32)GL_TEXTURE);
        (void)ndsFtrLeanPack(pk, REG2ID(MATRIX_IDENTITY), 0u);
        ndsFtrLeanPack1(pk, REG2ID(MATRIX_CONTROL), (u32)GL_MODELVIEW);
        m->hw_texgen = 0u;
    }
    ndsFtrLeanPackClose(pk);
    if ((pk->fault != 0u) || ((pk->count + len) > pk->capacity) ||
        (pk->count + len > 0xffffu))
    {
        return FALSE;
    }
    delta = (s32)pk->count - (s32)src->start;
    memcpy(&pk->words[pk->count], &donor->words[src->start],
           len * sizeof(u32));
    NDS_FTR_LEAN_CTR(gNdsFtrLean.root_copy_words += len);
    i = (stats->texture_combine_count - (u32)src->combine_base) &
        NDS_RENDERER_POLY_ID_MASK;
    if ((i != 0u) &&
        (ndsFtrLeanRebasePolyIds(&pk->words[pk->count], len, i) == FALSE))
    {
        return FALSE;
    }
    out->combine_base =
        (u8)(stats->texture_combine_count & NDS_RENDERER_POLY_ID_MASK);
    out->combine_count = src->combine_count;
    stats->texture_combine_count += src->combine_count;
    out->start = (u16)pk->count;
    pk->count += len;
    out->end = (u16)pk->count;
    *to_root = *from_root;
    if (to_root->seed_index != NDS_FIGHTER_PACKET_INDEX_NONE)
    {
        to_root->seed_index = (u16)((s32)to_root->seed_index + delta);
    }
    for (i = 0u; i < (u32)to_root->local_count; i++)
    {
        if (to_root->local_index[i] != NDS_FIGHTER_PACKET_INDEX_NONE)
        {
            to_root->local_index[i] =
                (u16)((s32)to_root->local_index[i] + delta);
        }
    }
    to_root->parent_slot = inputs[root_index].gx_parent_slot;
    to_root->store_slot = inputs[root_index].gx_store_slot;
    to_root->seed_is_identity = inputs[root_index].gx_seed_is_identity;
    site_first = packet->site_count;
    for (i = 0u; i < src->site_count; i++)
    {
        NDSFighterPacketShadeSite *site = &packet->sites[packet->site_count++];

        *site = donor->sites[(u32)src->site_first + i];
        site->index = (u16)((s32)site->index + delta);
        site->root = (u8)root_index;
    }
    tint_first = packet->tint_bind_count;
    for (i = 0u; i < src->tint_count; i++)
    {
        NDSFighterPacketTintBind *bind =
            &packet->tint_binds[packet->tint_bind_count++];

        *bind = donor->tint_binds[(u32)src->tint_first + i];
        bind->tex_index = (u16)((s32)bind->tex_index + delta);
        if (bind->pal_index != NDS_FIGHTER_PACKET_INDEX_NONE)
        {
            bind->pal_index = (u16)((s32)bind->pal_index + delta);
        }
        bind->root = (u8)root_index;
    }
    i = (u32)packet->tint_bind_seen + src->tint_seen;
    packet->tint_bind_seen = (u8)((i > 0xffu) ? 0xffu : i);
    mask = src->tex_mask;
    while (mask != 0u)
    {
        const u32 b = (u32)__builtin_ctz(mask);
        u32 k;

        mask &= mask - 1u;
        for (k = 0u; k < (u32)packet->texture_count; k++)
        {
            if (packet->textures[k].slot_plus1 ==
                donor->textures[b].slot_plus1)
            {
                break;
            }
        }
        if (k == (u32)packet->texture_count)
        {
            packet->textures[packet->texture_count++] = donor->textures[b];
        }
        tex_mask |= 1u << k;
    }
    if ((src->flags & NDS_FTR_LEAN_ROOT_FENCE) != 0u)
    {
        packet->needs_fence = 1u;
    }
    m->run_count += (u32)src->raw_runs + (u32)src->cross_runs;
    m->triangle_count += src->triangles;
    m->raw_triangles += src->raw_triangles;
    m->raw_reuse += (u32)src->raw_triangles - (u32)src->raw_runs;
    m->cross_triangles += src->cross_triangles;
    m->cross_reuse += (u32)src->cross_triangles - (u32)src->cross_runs;
    m->vertex_loads += src->vertex_loads;
    m->prepare_begin += src->prepare_begin;
    m->prepare_reuse += src->prepare_reuse;
    m->texture_binds += src->texture_binds;
    m->tint_binds += src->tint_binds;
    if (((src->flags & NDS_FTR_LEAN_ROOT_LIT) != 0u) &&
        (m->light_written == FALSE))
    {
        /* MatShade's first lit epoch: the head's vector (patched per draw
         * where the root's preamble carries one). */
        packet->light_root = (u8)root_index;
        packet->light_valid = 1u;
        pk->words[packet->light_index] = light_word;
        m->light_written = TRUE;
    }
    ndsFtrLeanRederiveSites(packet, site_first, src->site_count, inputs,
                            packet->tint_modulate);
    out->key = src->key;
    out->site_first = (u8)site_first;
    out->site_count = src->site_count;
    out->tint_first = (u8)tint_first;
    out->tint_count = src->tint_count;
    out->tex_mask = tex_mask;
    out->triangles = src->triangles;
    out->vertex_loads = src->vertex_loads;
    out->raw_triangles = src->raw_triangles;
    out->cross_triangles = src->cross_triangles;
    out->raw_runs = src->raw_runs;
    out->cross_runs = src->cross_runs;
    out->prepare_begin = src->prepare_begin;
    out->prepare_reuse = src->prepare_reuse;
    out->texture_binds = src->texture_binds;
    out->tint_binds = src->tint_binds;
    out->tint_seen = src->tint_seen;
    out->flags = src->flags;
    return TRUE;
}

/* The bind the prepare just made: its cache entry, validated per draw, or
 * the global texture fence when there is none. */
static void NDS_FIGHTER_PACKET_COLD_CODE
ndsFtrLeanMatNoteTexture(NDSFtrLeanMat *m, u32 tint)
{
    NDSFighterPacket *packet = m->packet;
    const NDSRendererHardwareTextureCacheEntry *entry =
        sNdsRendererHardwareActiveTextureEntry;
    u32 slot;
    u32 i;

    if (entry == NULL)
    {
        packet->needs_fence = 1u;
        m->root_fence = 1u;
        if ((tint == 0u) && (packet->fence_other < 0xffu))
        {
            packet->fence_other++;
        }
        return;
    }
    slot = (u32)(entry - sNdsRendererHardwareTextureCache);
    if (slot >= NDS_RENDERER_HW_TEXTURE_CACHE_COUNT)
    {
        packet->needs_fence = 1u;
        if (packet->fence_other < 0xffu)
        {
            packet->fence_other++;
        }
        return;
    }
    for (i = 0u; i < (u32)packet->texture_count; i++)
    {
        if (packet->textures[i].slot_plus1 == slot + 1u)
        {
            m->root_tex_mask |= 1u << i;
            return;
        }
    }
    if ((u32)packet->texture_count >= NDS_FIGHTER_PACKET_TEXTURE_MAX)
    {
        packet->needs_fence = 1u;
        if (packet->fence_other < 0xffu)
        {
            packet->fence_other++;
        }
        return;
    }
    m->root_tex_mask |= 1u << packet->texture_count;
    packet->textures[packet->texture_count].slot_plus1 = (u16)(slot + 1u);
    packet->textures[packet->texture_count].name = (u16)entry->name;
    packet->textures[packet->texture_count].key_generation =
        entry->key_generation;
    packet->texture_count++;
}

/* The shade of one epoch (ndsRendererNativeShadeProductionActions with the
 * hardware light and packet mode 0): the tint route, the DIF_AMB word and
 * its shade site, the first lit epoch's light (the head carries the vector
 * command), and the source vertex loads its actions account. */
static void
ndsFtrLeanMatShade(NDSFtrLeanMat *m, const NDSNativeEpoch *epoch,
                   u32 epoch_policy, const NDSRendererStats *stats,
                   const NDSRendererTraversalState *state)
{
    const NDSNativeDirectPolicy *policy =
        &sNdsNativeFighterDirectPolicies[
            epoch_policy & NDS_NATIVE_DIRECT_POLICY_FAMILY_MASK];
    u32 use_material =
        policy->vertex_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL;
    u32 material_color = (use_material != 0u) ? stats->prim_color : 0u;
    u32 k;

    sNdsEpochTintTexture = 0u;
    if (epoch->action_count == 0u)
    {
        return;
    }
    if (((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) != 0u) &&
        ((stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK) != 0u))
    {
        NDSFighterPacket *packet = m->packet;
        u32 shade;
        u32 tinted;
        u32 i;

        m->root_lit = 1u;
        if (m->light_written == FALSE)
        {
            /* The light vector's table entry; the vector
             * command itself sits in the head, under the identity. */
            packet->light_root = (u8)m->root;
            packet->light_valid = 1u;
            if (m->pk.fault == 0u)
            {
                m->pk.words[packet->light_index] = ndsFighterPacketLightWord(
                    stats->light_dir_x, stats->light_dir_y,
                    stats->light_dir_z);
            }
            m->light_written = TRUE;
        }
        shade = ndsRendererR2ResolveEpochShade(
            stats->light_color_1, stats->light_color_2, material_color,
            use_material, state->color_modulate, policy->textured);
        tinted = (sNdsEpochTintTexture != 0u) ? 1u : 0u;
        if ((tinted == 0u) && (use_material != 0u) &&
            (policy->textured == 0u) &&
            (((material_color >> 8) & 0x00ffffffu) != 0x00ffffffu))
        {
            /* The colour had no resident tile: the fold was drawn and the
             * tile queued; the set generation moves when it is created. */
            m->tint_pending++;
        }
        i = ndsFtrLeanPack(&m->pk, REG2ID(GFX_DIFFUSE_AMBIENT), 1u);
        if (m->pk.fault == 0u)
        {
            NDSFighterPacketShadeSite *site;
            u32 site_use = (tinted != 0u) ? 0u : use_material;

            m->pk.words[i] = shade;
            if (packet->site_count >= NDS_FIGHTER_PACKET_SITE_MAX)
            {
                m->pk.fault = 1u;
                return;
            }
            site = &packet->sites[packet->site_count++];
            site->index = (u16)i;
            site->root = (u8)m->root;
            site->use_material = (site_use != 0u) ? 1u : 0u;
            site->prim_from_root =
                ((site_use != 0u) && (m->prim_overridden == 0u)) ? 1u : 0u;
            /* Lean lists carry a tinted site's colour in its tile bind, which
             * ndsFtrLeanPatchTintTiles re-points: the site re-derives as the
             * no-material raw light it is (prim-independent), like any other
             * site. Slice 6: reserved[0] names it tinted, as the recorder's
             * site does, so ApplyTint fails closed where the old path's
             * replay does (any root's prim moved) and the lean patch mirrors
             * the re-record that follows (see ndsFtrLeanPacketPatch). */
            site->reserved[0] = (tinted != 0u) ? 1u : 0u;
            site->reserved[1] = 0u;
            site->reserved[2] = 0u;
            site->light_color_1 = stats->light_color_1;
            site->light_color_2 = stats->light_color_2;
            site->material_color = (tinted != 0u) ? 0u : material_color;
        }
    }
    for (k = 0u; k < (u32)epoch->action_count; k++)
    {
        const NDSNativeVertexAction *action =
            &sNdsNativeFighterActiveTables->vertex_actions[
                (u32)epoch->first_action + k];

        if (action->kind == NDS_NATIVE_VERTEX_BLOCK)
        {
            m->vertex_loads += action->count;
        }
    }
}

/* A run's corners in either emitter's packet twin (the cross run's RESTOREs
 * included): per corner the shade word (NORMAL, or COLOR for an unlit run),
 * the texture coordinate (ordinary UV from the run's prepared scale / origin /
 * offset -- ndsRendererNativeRebuildProductionRunUv's arithmetic; every
 * corner's dense vertex is in its run's unique list, host-proved over every
 * table set, so the prepared storage the emitter reads holds exactly this --
 * or a texgen site the per-draw patch fills) and the packed VTX16 position.
 * ndsFtrLeanPack's packing, with the packer held in locals and one
 * capacity check per corner. */
#define NDS_FTR_LEAN_OP(opcode, params)                                     \
    do                                                                     \
    {                                                                      \
        if (slot >= 4u)                                                    \
        {                                                                  \
            if ((hv != 0u) && (hp == 0u))                                  \
            {                                                              \
                w[n++] = 0u;                                               \
            }                                                              \
            cw = n++;                                                      \
            w[cw] = 0u;                                                    \
            slot = 0u;                                                     \
            hp = 0u;                                                       \
            hv = 1u;                                                       \
        }                                                                  \
        w[cw] |= (u32)(opcode) << (slot * 8u);                             \
        slot++;                                                            \
        hp += (params);                                                    \
    } while (0)

static void ndsFtrLeanMatCorners(NDSFtrLeanMat *m, const u16 *refs, u32 count,
                                 u32 cross, u32 palette_slot, u32 run_unlit,
                                 u32 textured,
                                 const NDSRendererTraversalState *state)
{
    NDSFtrLeanPacker *pk = &m->pk;
    NDSFighterPacket *packet = m->packet;
    const NDSNativeFighterRuntimeTables *tables = sNdsNativeFighterActiveTables;
    const NDSNativeDenseVertex *dense = tables->dense_vertices;
    const NDSNativePreparedDenseVertex *prepared = tables->prepared_dense;
    const u32 *normals = sNdsNativeFighterActiveDenseNormals;
    u32 *w = pk->words;
    u32 n = pk->count;
    u32 cap = pk->capacity;
    u32 cw = pk->cmd_word;
    u32 slot = pk->cmd_slot;
    u32 hp = pk->header_params;
    u32 hv = pk->header_valid;
    u32 shade_op = (run_unlit != FALSE) ? FIFO_COLOR : FIFO_NORMAL;
    u32 texgen = (m->texgen_group != NDS_FIGHTER_PACKET_TEXGEN_GROUP_NONE) ?
        TRUE : FALSE;
    s32 scale_s = (s32)state->texture_prepare_scale_s;
    s32 scale_t = (s32)state->texture_prepare_scale_t;
    s32 base_s = state->texture_prepare_offset -
        ((s32)state->texture_prepare_origin_s << 2);
    s32 base_t = state->texture_prepare_offset -
        ((s32)state->texture_prepare_origin_t << 2);
    u32 active = palette_slot;

    if (pk->fault != 0u)
    {
        return;
    }
    while (count-- != 0u)
    {
        u32 ref = *refs++;
        u32 d = (cross != FALSE) ? (ref & NDS_NATIVE_DENSE_ID_MASK) : ref;

        /* A corner writes at most two headers, a dummy and six params. */
        if (n + 12u > cap)
        {
            pk->fault = 1u;
            return;
        }
        if (cross != FALSE)
        {
            u32 corner_slot = ref >> NDS_NATIVE_PACKED_CORNER_MATRIX_SHIFT;

            if (corner_slot == NDS_NATIVE_GX_MATRIX_CURRENT)
            {
                corner_slot = palette_slot;
            }
            if (corner_slot != active)
            {
                NDS_FTR_LEAN_OP(REG2ID(MATRIX_RESTORE), 1u);
                w[n++] = corner_slot;
                active = corner_slot;
            }
        }
        NDS_FTR_LEAN_OP(shade_op, 1u);
        w[n++] = normals[d];
        /* Under GX texgen the engine forms the coordinate from the NORMAL;
         * a TEXCOORD here would replace it (RecordTexCoord). */
        if ((textured != 0u) && (m->hw_texgen == 0u))
        {
            NDS_FTR_LEAN_OP(FIFO_TEX_COORD, 1u);
            if (texgen == FALSE)
            {
                /* A LEAN_TRIM image has no dense_vertices: its source s/t
                 * ride in PreparedDense, which nothing else writes in a
                 * lean build. */
                const s32 vs = (dense != NULL) ? (s32)dense[d].s :
                    (s32)prepared[d].s;
                const s32 vt = (dense != NULL) ? (s32)dense[d].t :
                    (s32)prepared[d].t;
                s16 s = (s16)(((vs * scale_s) >> 17) + base_s);
                s16 t = (s16)(((vt * scale_t) >> 17) + base_t);

                w[n] = (u32)(u16)s | ((u32)(u16)t << 16);
            }
            else
            {
                if ((packet->texgen_site_count >=
                     NDS_FIGHTER_PACKET_TEXGEN_SITE_MAX) ||
                    (n > 0xffffu) || (d > 0xffffu))
                {
                    pk->fault = 1u;
                    return;
                }
                w[n] = 0u;
                packet->texgen_sites[packet->texgen_site_count].index = (u16)n;
                packet->texgen_sites[packet->texgen_site_count].dense_id =
                    (u16)d;
                packet->texgen_groups[m->texgen_group].site_count++;
                packet->texgen_site_count++;
            }
            n++;
        }
        if (m->vtx10 != 0u)
        {
            /* The VTX_16 words are whole source units at 4.12 (unit << 4);
             * VTX_10 at 4.6 holds the unit itself, 4x the VTX_16 value, or
             * at half scale the unit halved (rounded up), 2x. */
            const u32 xy = prepared[d].gx_xy;
            const u32 zw = prepared[d].gx_z;
            s32 x = (s32)(s16)xy >> 4;
            s32 y = (s32)(s16)(xy >> 16) >> 4;
            s32 z = (s32)(s16)zw >> 4;

            if (((xy & 0x000f000fu) | (zw & 0xfu)) != 0u)
            {
                m->vtx10_fail |= NDS_FTR_LEAN_VTX_FAIL_BITS;
            }
            if (m->vtx10 == 2u)
            {
                x = (x + 1) >> 1;
                y = (y + 1) >> 1;
                z = (z + 1) >> 1;
            }
            if (((u32)(x + 512) >= 1024u) || ((u32)(y + 512) >= 1024u) ||
                ((u32)(z + 512) >= 1024u))
            {
                m->vtx10_fail |= NDS_FTR_LEAN_VTX_FAIL_RANGE;
            }
            NDS_FTR_LEAN_OP(FIFO_VERTEX10, 1u);
            w[n++] = ((u32)x & 0x3ffu) | (((u32)y & 0x3ffu) << 10) |
                (((u32)z & 0x3ffu) << 20);
        }
        else
        {
            NDS_FTR_LEAN_OP(FIFO_VERTEX16, 2u);
            w[n++] = prepared[d].gx_xy;
            w[n++] = prepared[d].gx_z;
        }
    }
    if ((cross != FALSE) && (active != palette_slot))
    {
        if (n + 4u > cap)
        {
            pk->fault = 1u;
            return;
        }
        NDS_FTR_LEAN_OP(REG2ID(MATRIX_RESTORE), 1u);
        w[n++] = palette_slot;
    }
    pk->count = n;
    pk->cmd_word = cw;
    pk->cmd_slot = slot;
    pk->header_params = hp;
    pk->header_valid = hv;
}

/* 2026-10-12: THE WALK'S TEXTURE BINDS THROUGH A MEMO. Each textured run of
 * a list binds through the full resolver (tile sync, TMEM record search, the
 * ~30-field cache key, its hash, the lookup): a third of a P4
 * materialization (phase-tick lab, 1,200 frames: Banjo 294K ticks a walk,
 * Crash 189K, Meta Knight 126K), and nearly all of it re-resolves a texture
 * the walk or the slot's last walk already bound -- a texture is re-set at
 * every epoch that uses it, and a re-materialized key binds the same runs
 * again. A row answers a bind from a hash of the content's tables, the live
 * inputs the owner memo keys (texel and TLUT images, load size, prim/env
 * without alpha, both othermode words, the combine, the geometry mode), the
 * render tile's words and the TMEM load it samples -- not the run or the
 * battle slot: the resolver reads neither, and keyed by them Banjo's 161
 * runs a walk across four slots missed 46% of 256 rows -- and replays the
 * resolver's cache-hit tail exactly as the owner memo does
 * (tile sync, LRU stamp, name and parameter binds, active entry, pinned hit,
 * stats fields). The entry is revalidated by readiness, name and key
 * generation; anything else takes the full resolver and refills. Rows come
 * from a battle's general heap once per heap generation (5 KB), before the
 * list pool sizes; elsewhere there is no memo. Lab: gNdsFtrLeanTexMemoVerify
 * runs the full resolver on every hit and counts disagreements. */
#define NDS_FTR_LEAN_TEXMEMO_ROWS 256u
typedef struct NDSFtrLeanTexMemo
{
    u32 key;                    /* 0 = empty */
    u32 entry_generation;
    u16 slot_plus1;
    u16 name;
    u16 width;
    u16 height;
    u32 format;
} NDSFtrLeanTexMemo;
static NDSFtrLeanTexMemo *sNdsFtrLeanTexMemo;
static u32 sNdsFtrLeanTexMemoGen;   /* gNdsTaskmanHeapGeneration + 1 tried */
extern volatile u32 gNdsSceneManagerCurrIsBattle;
#if NDS_FTR_LEAN_LAB
volatile u32 gNdsFtrLeanTexMemoHits;
volatile u32 gNdsFtrLeanTexMemoFills;
volatile u32 gNdsFtrLeanTexMemoMisses;
volatile u32 gNdsFtrLeanTexMemoVerify __attribute__((used, section(".data"))) =
    0u;
volatile u32 gNdsFtrLeanTexMemoVerifyRuns;
volatile u32 gNdsFtrLeanTexMemoVerifyFail;
/* Lab: the hit path's parts (key, tile sync, binds) and the misses' full
 * resolves, in ticks. */
volatile u32 gNdsFtrLeanTexMemoKeyTicks;
volatile u32 gNdsFtrLeanTexMemoSyncTicks;
volatile u32 gNdsFtrLeanTexMemoBindTicks;
volatile u32 gNdsFtrLeanTexMemoMissTicks;
#endif

static NDSFtrLeanTexMemo *ndsFtrLeanTexMemoTable(void)
{
    if (sNdsFtrLeanTexMemoGen != gNdsTaskmanHeapGeneration + 1u)
    {
        const u32 bytes =
            NDS_FTR_LEAN_TEXMEMO_ROWS * (u32)sizeof(NDSFtrLeanTexMemo);
        const u32 free_bytes = ((uintptr_t)gSYTaskmanGeneralHeap.end >
                                (uintptr_t)gSYTaskmanGeneralHeap.ptr) ?
            (u32)((uintptr_t)gSYTaskmanGeneralHeap.end -
                  (uintptr_t)gSYTaskmanGeneralHeap.ptr) : 0u;

        sNdsFtrLeanTexMemoGen = gNdsTaskmanHeapGeneration + 1u;
        sNdsFtrLeanTexMemo = NULL;
        if ((gNdsSceneManagerCurrIsBattle != 0u) &&
            (free_bytes > bytes + NDS_FTR_LEAN_SPARE_KEEP_FREE))
        {
            sNdsFtrLeanTexMemo = syTaskmanMalloc(bytes, 32u);
            if (sNdsFtrLeanTexMemo != NULL)
            {
                memset(sNdsFtrLeanTexMemo, 0, bytes);
            }
        }
    }
    return sNdsFtrLeanTexMemo;
}

static u32 ndsFtrLeanTexMemoKey(const NDSRendererStats *stats)
{
    const u32 tile_index =
        ((stats->texture_state_flags & NDS_RENDERER_TEXTURE_STATE_SEEN) !=
         0u) ? (stats->texture_tile & 0x7u) : NDS_RENDERER_RENDER_TILE;
    const NDSRendererTileState *tile = &stats->texture_tiles[tile_index];
    const NDSRendererTextureLoadState *load =
        ndsRendererHardwareFindTextureLoadForTmem(stats, tile->tmem);
    u32 h = 0x811c9dc5u;

#define NDS_FTR_LEAN_TEXMEMO_MIX(value) \
    h = (h ^ (u32)(value)) * 0x01000193u
    NDS_FTR_LEAN_TEXMEMO_MIX((uintptr_t)sNdsNativeFighterActiveTables);
    NDS_FTR_LEAN_TEXMEMO_MIX(tile_index);
    NDS_FTR_LEAN_TEXMEMO_MIX(stats->texture_image);
    NDS_FTR_LEAN_TEXMEMO_MIX(stats->texture_format |
                             (stats->texture_size << 4) |
                             (stats->texture_image_width << 8));
    NDS_FTR_LEAN_TEXMEMO_MIX(stats->texture_tlut_image);
    NDS_FTR_LEAN_TEXMEMO_MIX(stats->texture_load_texels);
    NDS_FTR_LEAN_TEXMEMO_MIX(stats->prim_color & 0xffffff00u);
    NDS_FTR_LEAN_TEXMEMO_MIX(stats->env_color & 0xffffff00u);
    NDS_FTR_LEAN_TEXMEMO_MIX(stats->othermode_h);
    NDS_FTR_LEAN_TEXMEMO_MIX(stats->othermode_l);
    NDS_FTR_LEAN_TEXMEMO_MIX(stats->texture_combine_w0);
    NDS_FTR_LEAN_TEXMEMO_MIX(stats->texture_combine_w1);
    NDS_FTR_LEAN_TEXMEMO_MIX(stats->geometry_mode);
    NDS_FTR_LEAN_TEXMEMO_MIX(stats->texture_state_flags);
    NDS_FTR_LEAN_TEXMEMO_MIX(tile->format | (tile->size << 4) |
                             (tile->palette << 8) | (tile->cms << 12) |
                             (tile->cmt << 14) | (tile->masks << 16) |
                             (tile->maskt << 20) | (tile->shifts << 24) |
                             (tile->shiftt << 28));
    NDS_FTR_LEAN_TEXMEMO_MIX(tile->line | (tile->tmem << 16));
    NDS_FTR_LEAN_TEXMEMO_MIX(tile->uls | (tile->ult << 16));
    NDS_FTR_LEAN_TEXMEMO_MIX(tile->lrs | (tile->lrt << 16));
    NDS_FTR_LEAN_TEXMEMO_MIX(tile->set_seen | (tile->size_seen << 1) |
                             (tile->flags << 2));
    if (load != NULL)
    {
        NDS_FTR_LEAN_TEXMEMO_MIX(load->image);
        NDS_FTR_LEAN_TEXMEMO_MIX(load->image_width |
                                 ((u32)load->load_texels << 16));
        NDS_FTR_LEAN_TEXMEMO_MIX(load->load_uls | ((u32)load->load_ult << 16));
        NDS_FTR_LEAN_TEXMEMO_MIX(load->load_lrs | ((u32)load->load_dxt << 16));
        NDS_FTR_LEAN_TEXMEMO_MIX(load->image_format |
                                 ((u32)load->image_size << 8) |
                                 ((u32)load->load_kind << 16) |
                                 ((u32)load->load_tile << 24));
    }
#undef NDS_FTR_LEAN_TEXMEMO_MIX
    /* The row is the key's low byte, and a multiply carries bits only upward:
     * without this the row saw only each input's low bits, and aligned image
     * addresses put most textures in a few rows (Meta Knight's misses rose
     * 314 -> 463 when the run index left the key). */
    h ^= h >> 16;
    h *= 0x85ebca6bu;
    h ^= h >> 13;
    return (h != 0u) ? h : 1u;
}

static s32 __attribute__((noinline))
ndsFtrLeanBindTexture(NDSRendererStats *stats,
                      const NDSRendererConfig *config,
                      NDSRendererTraversalState *state)
{
    NDSFtrLeanTexMemo *table = ndsFtrLeanTexMemoTable();
    NDSFtrLeanTexMemo *row;
    NDSRendererHardwareTextureCacheEntry *entry;
    u32 key;
#if NDS_FTR_LEAN_LAB
    u32 lab_mark = cpuGetTiming();
#define NDS_FTR_LEAN_TEXMEMO_LAB_PART(counter)                              \
    do                                                                      \
    {                                                                       \
        const u32 lab_now_ = cpuGetTiming();                                \
        (counter) += lab_now_ - lab_mark;                                   \
        lab_mark = lab_now_;                                                \
    } while (0)
#else
#define NDS_FTR_LEAN_TEXMEMO_LAB_PART(counter) ((void)0)
#endif

    if ((table == NULL) || (ndsRendererHardwareUsesTexel01Lerp(stats) != FALSE))
    {
        return ndsRendererHardwareBindTexture(stats, config, state);
    }
    key = ndsFtrLeanTexMemoKey(stats);
    row = &table[key & (NDS_FTR_LEAN_TEXMEMO_ROWS - 1u)];
    NDS_FTR_LEAN_TEXMEMO_LAB_PART(gNdsFtrLeanTexMemoKeyTicks);
    if ((row->key == key) && (row->slot_plus1 != 0u))
    {
        entry = &sNdsRendererHardwareTextureCache[row->slot_plus1 - 1u];
        if ((entry->ready != FALSE) && ((u32)entry->name == row->name) &&
            (entry->key_generation == row->entry_generation))
        {
#if NDS_FTR_LEAN_LAB
            if (gNdsFtrLeanTexMemoVerify != 0u)
            {
                s32 ok = ndsRendererHardwareBindTexture(stats, config, state);

                gNdsFtrLeanTexMemoVerifyRuns++;
                if ((ok == FALSE) ||
                    (sNdsRendererHardwareActiveTextureEntry != entry) ||
                    (sNdsRendererHardwareBoundTextureName != row->name) ||
                    (stats->hardware_texture_format != row->format) ||
                    (stats->hardware_texture_width != row->width) ||
                    (stats->hardware_texture_height != row->height))
                {
                    gNdsFtrLeanTexMemoVerifyFail++;
                }
                return ok;
            }
#endif
            ndsRendererSyncTextureTile(stats);
            NDS_FTR_LEAN_TEXMEMO_LAB_PART(gNdsFtrLeanTexMemoSyncTicks);
            entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
            if (entry->pinned != 0u)
            {
                ndsRendererHardwareBindTextureName(stats, row->name);
                ndsRendererHardwareApplyTextureParams(entry->params);
                sNdsRendererHardwareActiveTextureEntry = entry;
                ndsRendererHardwareRecordBattleStaticTextureHit(entry);
            }
            else if ((sNdsRendererHardwareActiveTextureEntry != entry) ||
                     (sNdsRendererHardwareBoundTextureName != row->name))
            {
                ndsRendererHardwareBindTextureName(stats, row->name);
                ndsRendererHardwareApplyTextureParams(entry->params);
                sNdsRendererHardwareActiveTextureEntry = entry;
            }
            NDS_FTR_LEAN_TEXMEMO_LAB_PART(gNdsFtrLeanTexMemoBindTicks);
            stats->hardware_texture_ready_count++;
            stats->hardware_texture_format = row->format;
            stats->hardware_texture_width = row->width;
            stats->hardware_texture_height = row->height;
            NDS_FTR_LEAN_CTR(gNdsFtrLeanTexMemoHits++);
            return TRUE;
        }
        row->key = 0u;
    }
    NDS_FTR_LEAN_CTR(gNdsFtrLeanTexMemoMisses++);
    if (ndsRendererHardwareBindTexture(stats, config, state) == FALSE)
    {
        return FALSE;
    }
    NDS_FTR_LEAN_TEXMEMO_LAB_PART(gNdsFtrLeanTexMemoMissTicks);
    entry = sNdsRendererHardwareActiveTextureEntry;
    if ((entry != NULL) && (entry->ready != FALSE) && (entry->name > 0) &&
        ((u32)entry->name == sNdsRendererHardwareBoundTextureName) &&
        ((u32)entry->name <= 0xffffu) &&
        (stats->hardware_texture_width <= 0xffffu) &&
        (stats->hardware_texture_height <= 0xffffu))
    {
        const u32 slot = (u32)(entry - sNdsRendererHardwareTextureCache);

        if (slot < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT)
        {
            row->key = key;
            row->entry_generation = entry->key_generation;
            row->slot_plus1 = (u16)(slot + 1u);
            row->name = (u16)entry->name;
            row->width = (u16)stats->hardware_texture_width;
            row->height = (u16)stats->hardware_texture_height;
            row->format = stats->hardware_texture_format;
            NDS_FTR_LEAN_CTR(gNdsFtrLeanTexMemoFills++);
        }
    }
    return TRUE;
}

#undef NDS_FTR_LEAN_TEXMEMO_LAB_PART

/* One run: SubmitProductionRun -> PrepareProductionRunCore (packet mode 0) ->
 * the prepare hook -> the emitter's packet twin -> the run's accounting.
 * Returns 0 or the decline reason where production would reject the run. */
static u32 __attribute__((noinline))
ndsFtrLeanMatRun(NDSFtrLeanMat *m, u32 run_index, u32 epoch_policy,
                 u32 palette_slot, const NDSRendererConfig *config,
                 NDSRendererStats *stats, NDSRendererTraversalState *state)
{
    const NDSNativeFighterRuntimeTables *tables = sNdsNativeFighterActiveTables;
    const NDSNativeDirectPolicy *policy;
    const NDSNativeRun *run;
    NDSFtrLeanPacker *pk = &m->pk;
    NDSFighterPacket *packet = m->packet;
    u32 family = epoch_policy & NDS_NATIVE_DIRECT_POLICY_FAMILY_MASK;
    u32 expected_geometry_cull =
        ((epoch_policy & NDS_NATIVE_DIRECT_POLICY_CULL_NONE) != 0u) ?
            0u : NDS_RENDERER_GEOM_CULL_BACK;
    u32 expected_poly_cull =
        ((epoch_policy & NDS_NATIVE_DIRECT_POLICY_CULL_NONE) != 0u) ?
            POLY_CULL_NONE : POLY_CULL_BACK;
    u32 geometry_cull;
    u32 run_unlit;
    u32 run_poly_alpha;
    u32 run_poly_light = 0u;
    u32 use_texture;
    u32 tint;
    u32 submit_class;
    u32 textured;
    u32 triangles;
    NDSFighterPacketTexgenGroup *hw_group;

    if ((run_index >= tables->run_count) ||
        (family >= (sizeof(sNdsNativeFighterDirectPolicies) /
                    sizeof(sNdsNativeFighterDirectPolicies[0]))))
    {
        return nNDSFtrLeanDeclinePolicy;
    }
    run = &tables->runs[run_index];
    triangles = run->triangle_count;
    if (triangles == 0u)
    {
        return nNDSFtrLeanDeclinePolicy;
    }
    submit_class = ndsRendererNativeRunSubmitClass(run);
    run_unlit = ndsRendererNativeRunUsesVertexColor(run);
    run_poly_alpha = (run_unlit != FALSE) ?
        ndsRendererNativeRunPolyAlpha(run) : 31u;
    if (run_unlit == FALSE)
    {
        run_poly_light = POLY_FORMAT_LIGHT0;
    }
    policy = &sNdsNativeFighterDirectPolicies[family];
    geometry_cull = stats->geometry_mode &
        (NDS_RENDERER_GEOM_CULL_FRONT | NDS_RENDERER_GEOM_CULL_BACK);
    if (((stats->geometry_mode & NDS_RENDERER_GEOM_ZBUFFER) == 0u) ||
        ((run_unlit == FALSE) &&
         ((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) == 0u)) ||
        ((run_unlit != FALSE) &&
         ((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) != 0u)) ||
#if NDS_LINK
        /* G_TEXTURE_GEN_LINEAR only selects the texgen mapping (the polygon
         * team's Link, Captain and Ness parts); without G_TEXTURE_GEN the RSP
         * ignores it. */
        ((stats->geometry_mode & NDS_RENDERER_GEOM_FOG) != 0u) ||
        ((((stats->geometry_mode & NDS_RENDERER_GEOM_TEXTURE_GEN) != 0u) &&
          ((policy->textured == 0u) || (state->modelview_valid == 0u)))) ||
#else
        ((stats->geometry_mode &
          (NDS_RENDERER_GEOM_FOG |
           NDS_RENDERER_GEOM_TEXTURE_GEN_LINEAR)) != 0u) ||
        ((stats->geometry_mode & NDS_RENDERER_GEOM_TEXTURE_GEN) != 0u) ||
#endif
        (geometry_cull != expected_geometry_cull) ||
        ((((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) != 0u) &&
          ((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) !=
           NDS_RENDERER_ALPHA_COMPARE_THRESHOLD))) ||
        ((stats->othermode_l & NDS_RENDERER_ZMODE_MASK) ==
         NDS_RENDERER_ZMODE_DEC) ||
        ((stats->othermode_l & NDS_RENDERER_ZSOURCE_MASK) != 0u) ||
        (stats->texture_combine_w0 != policy->combine_w0) ||
        (stats->texture_combine_w1 != policy->combine_w1) ||
        ((family != NDS_NATIVE_DIRECT_POLICY_LIT_ONLY) &&
         (family != NDS_NATIVE_DIRECT_POLICY_TEXTURED_PASS2) &&
         (family != NDS_NATIVE_DIRECT_POLICY_LIT_PASS2) &&
         (stats->env_color != 0xffffffffu)) ||
        (state->matrix_valid == 0u) ||
        (state->matrix_generation == 0u))
    {
        return nNDSFtrLeanDeclinePolicy;
    }
    /* Slice 6: a G_AC_THRESHOLD run is exact in a list only at DS reference 0
     * (blend alpha below 16; the old path's glAlphaFunc(alpha >> 4)). The DS
     * never draws an alpha-0 fragment, so that test decides no pixel (melonDS:
     * `alpha <= RenderAlphaRef`, 0 with the test off), and the frame-global
     * alpha register -- which no FIFO list carries -- is left as the old
     * path's draw leaves it after its next batch: disabled (the lean submit
     * disables it too). Captain HIGH's cutout compares against
     * G_SETBLENDCOLOR (0,0,0,0). */
    if (((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) ==
         NDS_RENDERER_ALPHA_COMPARE_THRESHOLD) &&
        ((stats->blend_color & 0xf0u) != 0u))
    {
        return nNDSFtrLeanDeclineAlphaTest;
    }
    state->texture_prepare_source_zbuffered = TRUE;
    state->texture_prepare_decal_depth = FALSE;
    state->texture_prepare_prim_depth = FALSE;
    state->texture_prepare_material_color =
        ((policy->vertex_flags &
          NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL) != 0u) ?
            stats->prim_color : 0u;
    state->texture_prepare_vertex_flags = policy->vertex_flags;
    if (state->texture_prepare_valid == 0u)
    {
        u32 texture_name = 0u;
        u32 texture_scale_s = 0u;
        u32 texture_scale_t = 0u;
        u32 texture_origin_s = 0u;
        u32 texture_origin_t = 0u;
        s32 texture_offset = 0;

        use_texture = FALSE;
        if (policy->textured != 0u)
        {
            u32 render_tile_index =
                ((stats->texture_state_flags &
                  NDS_RENDERER_TEXTURE_STATE_SEEN) != 0u) ?
                    (stats->texture_tile & 0x7u) :
                    NDS_RENDERER_RENDER_TILE;
            const NDSRendererTileState *render_tile =
                &stats->texture_tiles[render_tile_index];
            u32 implicit_texture_on = FALSE;
            u32 required_texture_mask =
                NDS_RENDERER_TEXTURE_SETTIMG |
                NDS_RENDERER_TEXTURE_SETTILE |
                NDS_RENDERER_TEXTURE_SETTILESIZE;

            if ((stats->texture_state_flags &
                 NDS_RENDERER_TEXTURE_STATE_ON) == 0u)
            {
                implicit_texture_on =
                    (((stats->texture_mask & required_texture_mask) ==
                      required_texture_mask) &&
                     ((stats->texture_mask &
                       (NDS_RENDERER_TEXTURE_LOADBLOCK |
                        NDS_RENDERER_TEXTURE_LOADTILE)) != 0u) &&
                     (stats->texture_image != 0u) &&
                     (stats->texture_load_texels != 0u) &&
                     (render_tile->set_seen != 0u) &&
                     (render_tile->size_seen != 0u) &&
                     (render_tile->line != 0u) &&
                     (render_tile->width != 0u) &&
                     (render_tile->height != 0u)) ? TRUE : FALSE;
            }
            /* The production bind path itself (resolve, convert on a miss,
             * bind, texture parameters), so the words read back below are
             * the ones a record of this run would read; a repeat answers
             * from the memo with the same binds. */
            NDS_FTR_LEAN_MAT_PART(m, 4u);
            use_texture = ndsFtrLeanBindTexture(stats, config, state);
            NDS_FTR_LEAN_MAT_PART(m, 6u);
            if (use_texture == FALSE)
            {
                return nNDSFtrLeanDeclineTexture;
            }
            texture_name = sNdsRendererHardwareBoundTextureName;
            texture_scale_s = stats->texture_scale_s;
            texture_scale_t = stats->texture_scale_t;
            if (implicit_texture_on != FALSE)
            {
                if ((stats->texture_state_flags &
                     NDS_RENDERER_TEXTURE_STATE_SCALE_S) == 0u)
                {
                    texture_scale_s = NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE;
                }
                if ((stats->texture_state_flags &
                     NDS_RENDERER_TEXTURE_STATE_SCALE_T) == 0u)
                {
                    texture_scale_t = NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE;
                }
            }
            texture_origin_s = render_tile->uls;
            texture_origin_t = render_tile->ult;
            texture_offset =
                ((stats->othermode_h & NDS_RENDERER_TEXTFILT_MASK) !=
                 NDS_RENDERER_TF_POINT) ?
                    NDS_RENDERER_TEXCOORD_FILTER_OFFSET : 0;
        }
        state->texture_prepare_valid = TRUE;
        state->texture_prepare_enabled = use_texture;
        state->texture_prepare_name = texture_name;
        state->texture_prepare_alpha_constant = TRUE;
        state->texture_prepare_poly_alpha = run_poly_alpha;
        state->texture_prepare_poly_fmt =
            expected_poly_cull | POLY_ALPHA(run_poly_alpha) | run_poly_light |
            POLY_ID(stats->texture_combine_count &
                    NDS_RENDERER_POLY_ID_MASK);
        state->texture_prepare_scale_s = texture_scale_s;
        state->texture_prepare_scale_t = texture_scale_t;
        state->texture_prepare_origin_s = texture_origin_s;
        state->texture_prepare_origin_t = texture_origin_t;
        state->texture_prepare_offset = texture_offset;
        m->prepare_begin++;
    }
    else
    {
        use_texture = state->texture_prepare_enabled;
        if ((use_texture != FALSE) != (policy->textured != 0u))
        {
            return nNDSFtrLeanDeclinePolicy;
        }
        state->texture_prepare_poly_alpha = run_poly_alpha;
        state->texture_prepare_poly_fmt =
            (state->texture_prepare_poly_fmt &
             ~((u32)POLY_CULL_NONE | (u32)POLY_ALPHA(31u) |
               (u32)POLY_FORMAT_LIGHT0)) |
            expected_poly_cull | POLY_ALPHA(run_poly_alpha) | run_poly_light;
        m->prepare_reuse++;
    }
    tint = (policy->textured == 0u) ? sNdsEpochTintTexture : 0u;
    NDS_FTR_LEAN_MAT_PART(m, 4u);
    if (tint != 0u)
    {
        ndsRendererR2BindTintTile(stats, tint);
    }
    /* The run's prepare: texture matrix, bind, POLYGON_ATTR, BEGIN. */
    if (m->hw_texgen != 0u)
    {
        /* The previous run's GX texgen
         * matrix stops here; the mode-1 binds everywhere else expect the
         * identity. */
        ndsFtrLeanPack1(pk, REG2ID(MATRIX_CONTROL), (u32)GL_TEXTURE);
        (void)ndsFtrLeanPack(pk, REG2ID(MATRIX_IDENTITY), 0u);
        ndsFtrLeanPack1(pk, REG2ID(MATRIX_CONTROL), (u32)GL_MODELVIEW);
        m->hw_texgen = 0u;
    }
    m->texgen_group = NDS_FIGHTER_PACKET_TEXGEN_GROUP_NONE;
    hw_group = NULL;
    if (((use_texture != FALSE) || (tint != 0u)) &&
        ((stats->geometry_mode & NDS_RENDERER_GEOM_TEXTURE_GEN) != 0u))
    {
        NDSFighterPacketTexgenGroup *group;

        if (((u32)packet->texgen_group_count >=
             NDS_FIGHTER_PACKET_TEXGEN_GROUP_MAX) ||
            (m->root >= NDS_FIGHTER_PACKET_ROOT_MAX))
        {
            pk->fault = 1u;
            return nNDSFtrLeanDeclineCapacity;
        }
        group = &packet->texgen_groups[packet->texgen_group_count];
        group->scale_s = state->texture_prepare_scale_s;
        group->scale_t = state->texture_prepare_scale_t;
        group->origin_s = state->texture_prepare_origin_s;
        group->origin_t = state->texture_prepare_origin_t;
        group->offset = state->texture_prepare_offset;
        group->first_site = packet->texgen_site_count;
        group->site_count = 0u;
        group->root = (u8)m->root;
        group->reserved[0] = (u8)(((stats->geometry_mode &
            NDS_RENDERER_GEOM_TEXTURE_GEN_LINEAR) != 0u) ? 1u : 0u);
        group->reserved[1] = 0u;
        group->reserved[2] = 0u;
        m->texgen_group = packet->texgen_group_count;
        packet->texgen_group_count++;
        /* 2026-10-05: the recorder's GX texgen (gNdsFighterPacketHwTexgen)
         * for the 1P owners, whose spherical texgen covers the whole model
         * (a Polygon's CPU sites outgrew the list): a lit, textured,
         * spherical run takes TEXIMAGE_PARAM texgen mode 2 and a texture
         * matrix the per-draw patch fills (ndsFighterPacketPatchTexgen's
         * reserved[1] branch), and its corners carry no TEXCOORD. */
        if ((m->hw_texgen_ok != 0u) && (use_texture != FALSE) &&
            (tint == 0u) && (run_poly_light != 0u) &&
            (group->reserved[0] == 0u) && (gNdsFighterPacketHwTexgen != 0u))
        {
            hw_group = group;
        }
    }
    if ((use_texture != FALSE) || (tint != 0u))
    {
        /* The words libnds holds for the
         * bind just made (the prepare's or the tint tile's). */
        int palette_format = -1;
        u32 tex_index = ndsFtrLeanPack(pk, REG2ID(GFX_TEX_FORMAT), 1u);
        u32 pal_index = NDS_FIGHTER_PACKET_INDEX_NONE;

        if (pk->fault == 0u)
        {
            pk->words[tex_index] = (u32)glGetTexParameter();
            if (hw_group != NULL)
            {
                pk->words[tex_index] =
                    (pk->words[tex_index] & ~((u32)3u << 30)) |
                    ((u32)2u << 30);
            }
        }
        glGetColorTableParameterEXT(
            GL_TEXTURE_2D, GL_COLOR_TABLE_FORMAT_EXT, &palette_format);
        if (palette_format >= 0)
        {
            pal_index = ndsFtrLeanPack(pk, REG2ID(GFX_PAL_FORMAT), 1u);
            if (pk->fault == 0u)
            {
                pk->words[pal_index] = (u32)palette_format;
            }
        }
        if ((tint != 0u) && (pk->fault == 0u))
        {
            if (packet->tint_bind_seen < 0xffu)
            {
                packet->tint_bind_seen++;
            }
            if (((u32)packet->tint_bind_count >=
                 NDS_FIGHTER_PACKET_TINT_BIND_MAX) ||
                (tex_index >= NDS_FIGHTER_PACKET_INDEX_NONE) ||
                (m->root >= NDS_FIGHTER_PACKET_ROOT_MAX))
            {
                packet->tint_bind_overflow = 1u;
            }
            else
            {
                NDSFighterPacketTintBind *bind =
                    &packet->tint_binds[packet->tint_bind_count++];

                bind->tex_index = (u16)tex_index;
                bind->pal_index = (u16)pal_index;
                bind->root = (u8)m->root;
                bind->prim_from_root = (m->prim_overridden == 0u) ? 1u : 0u;
                bind->reserved[0] = 0u;
                bind->reserved[1] = 0u;
                bind->rgb = sNdsEpochTintRgb;
            }
            m->tint_binds++;
        }
        ndsFtrLeanMatNoteTexture(m, tint);
        m->texture_binds++;
        NDS_FTR_LEAN_MAT_PART(m, 7u);
    }
    else
    {
        ndsFtrLeanPack1(pk, REG2ID(GFX_TEX_FORMAT), 0u);
    }
    if (hw_group != NULL)
    {
        /* RecordPrepare's GX texgen tail: the matrix words (zero until the
         * patch), then the source texcoord's constant part. */
        const u32 base = ndsFighterPacketTexgenBaseWord(
            hw_group->scale_s, hw_group->scale_t, hw_group->origin_s,
            hw_group->origin_t, hw_group->offset);
        u32 at;
        u32 k;

        ndsFtrLeanPack1(pk, REG2ID(MATRIX_CONTROL), (u32)GL_TEXTURE);
        at = ndsFtrLeanPack(pk, REG2ID(MATRIX_LOAD4x3), 12u);
        if (pk->fault == 0u)
        {
            for (k = 0u; k < 12u; k++)
            {
                pk->words[at + k] = 0u;
            }
            hw_group->origin_s = at;
            hw_group->reserved[1] = 1u;
        }
        ndsFtrLeanPack1(pk, REG2ID(MATRIX_CONTROL), (u32)GL_MODELVIEW);
        ndsFtrLeanPack1(pk, FIFO_TEX_COORD, base);
        m->hw_texgen = 1u;
    }
    ndsFtrLeanPack1(pk, REG2ID(GFX_POLY_FORMAT),
                    state->texture_prepare_poly_fmt);
    ndsFtrLeanPack1(pk, FIFO_BEGIN, (u32)GL_TRIANGLE);
    if (tint != 0u)
    {
        /* ndsRendererR2OpenTintBatch: the tile-centre coordinate every vertex
         * of the run keeps, then the prepare is invalidated so the next run
         * re-binds its own texture. */
        ndsFtrLeanPack1(pk, FIFO_TEX_COORD,
                        (u32)TEXTURE_PACK((NDS_FIGHTER_TINT_DIM / 2u) << 4,
                                          (NDS_FIGHTER_TINT_DIM / 2u) << 4));
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
    }
    /* ndsRendererNativeSubmitProductionRun's emit and accounting. */
    textured = state->texture_prepare_enabled;
    NDS_FTR_LEAN_MAT_PART(m, 4u);
    if (submit_class == NDS_NATIVE_RUN_CROSS_MATRIX)
    {
        u32 group = tables->primitive_group_first[run_index];

        if (palette_slot > NDS_NATIVE_GX_MATRIX_SLOT_MAX)
        {
            return nNDSFtrLeanDeclinePolicy;
        }
        ndsFtrLeanMatCorners(
            m, &tables->primitive_vertices[
                tables->primitive_group_first_vertex[group]],
            triangles * 3u, TRUE, palette_slot, run_unlit, textured, state);
        m->cross_triangles += triangles;
        m->cross_reuse += triangles - 1u;
    }
    else if (submit_class == NDS_NATIVE_RUN_RAW_CURRENT)
    {
        u32 g = tables->primitive_group_first[run_index];
        u32 remaining_groups = tables->primitive_group_count[run_index];
        u32 current_type = (u32)GL_TRIANGLE;

        while (remaining_groups-- != 0u)
        {
            u32 gtype = tables->primitive_group_type[g];
            const u16 *vref = &tables->primitive_vertices[
                tables->primitive_group_first_vertex[g]];
            u32 remaining = tables->primitive_group_vertex_count[g];

            g++;
            if ((gtype != current_type) || (gtype != (u32)GL_TRIANGLE))
            {
                ndsFtrLeanPack1(pk, FIFO_BEGIN, gtype);
                current_type = gtype;
            }
            ndsFtrLeanMatCorners(m, vref, remaining, FALSE, palette_slot,
                                 run_unlit, textured, state);
        }
        if (current_type != (u32)GL_TRIANGLE)
        {
            ndsFtrLeanPack1(pk, FIFO_BEGIN, (u32)GL_TRIANGLE);
        }
        m->raw_triangles += triangles;
        m->raw_reuse += triangles - 1u;
    }
    else
    {
        return nNDSFtrLeanDeclinePolicy;
    }
    m->run_count++;
    m->triangle_count += triangles;
    NDS_FTR_LEAN_MAT_PART(m, 5u);
    return (pk->fault != 0u) ? nNDSFtrLeanDeclineCapacity : 0u;
}

/* Which corners failed the last VTX_10 walk (NDS_FTR_LEAN_VTX_FAIL_*). */
static u32 sNdsFtrLeanLastVtxFail;

u32 ndsFtrLeanLastVtxFail(void)
{
    return sNdsFtrLeanLastVtxFail;
}

u32 NDS_FIGHTER_PACKET_COLD_CODE
ndsFtrLeanMaterialize(u32 battle_slot, u32 entry, const u32 *key,
                      const NDSRendererNativeFighterRoot *inputs,
                      u32 input_count, u32 owner_slot,
                      u32 use_low_detail, const void *asset_base,
                      NDSRendererStats *stats)
{
    NDSFtrLeanSlotState *slot_state;
    NDSFtrLeanEntryState *es;
    NDSFighterPacket *packet;
    NDSRendererTraversalState *state =
        &sNdsNativeFighterOwnerExecution.traversal;
    const NDSNativeFighterRuntimeTables *owner_tables;
    const u8 *palette_slots;
    NDSFtrLeanMat m;
    NDSFighterPacket *donors[NDS_FTR_LEAN_ENTRIES + 1u];
    u32 donor_count = 0u;
    u32 plan_any = 0u;
    u32 blend_roots = 0u;
    u32 record;
    u32 root_count;
    u32 root_index;
    u32 reason = 0u;
    u32 i;
    /* Slice 6: NDS_FTR_LEAN_ENTRY_WIDE asks for entry 0 over the whole
     * region (route 1 only); the other entry is given up while it holds. */
    u32 wide = ((entry & NDS_FTR_LEAN_ENTRY_WIDE) != 0u) ? TRUE : FALSE;
    u32 vtx10 = ((entry & NDS_FTR_LEAN_ENTRY_VTX16) != 0u) ? 0u :
        (((entry & NDS_FTR_LEAN_ENTRY_VTX10H) != 0u) ? 2u : 1u);
    u32 capacity = (wide != FALSE) ? ndsFtrLeanWideCapacity() :
        ndsFtrLeanWordCapacity();
    u32 no_copy = ((entry & NDS_FTR_LEAN_ENTRY_NOCOPY) != 0u) ? TRUE : FALSE;
    /* Slice 6: the fence the old path's key would carry for this record --
     * built before its execute, so before any bind below uploads. */
    u32 record_fence = sNdsRendererHardwareTextureKeyGeneration ^
        (sNdsRendererRuntimeTextureCacheEvictCount << 16);

    entry &= ~(NDS_FTR_LEAN_ENTRY_WIDE | NDS_FTR_LEAN_ENTRY_VTX16 |
               NDS_FTR_LEAN_ENTRY_VTX10H | NDS_FTR_LEAN_ENTRY_NOCOPY);
    sNdsFtrLeanLastVtxFail = 0u;
    if ((battle_slot >= NDS_FIGHTER_PACKET_SLOTS) || (key == NULL) ||
        (ndsFtrLeanEntryUsable(entry) == FALSE) || (inputs == NULL) ||
        (stats == NULL) || (asset_base == NULL) || (input_count == 0u) ||
        (input_count > NDS_FIGHTER_PACKET_ROOT_MAX) ||
        (input_count > 32u) ||
        ((wide != FALSE) && (entry != 0u)))
    {
        return nNDSFtrLeanDeclineInputs;
    }
    slot_state = &sNdsFtrLeanSlots[battle_slot];
    if (wide != FALSE)
    {
        ndsFtrLeanSpareIdentity(slot_state);
    }
    es = &slot_state->entry[entry];
    es->valid = 0u;
    if (slot_state->active == entry)
    {
        slot_state->active = NDS_FTR_LEAN_ENTRY_NONE;
    }
    /* A wide list's words run through the other entry's half: whichever of
     * the two is written now, the other stops holding a list. */
    if ((wide != FALSE) ||
        ((slot_state->entry[entry ^ 1u].valid & NDS_FTR_LEAN_VALID_WIDE) !=
         0u))
    {
        slot_state->entry[entry ^ 1u].valid = 0u;
        if (slot_state->active == (entry ^ 1u))
        {
            slot_state->active = NDS_FTR_LEAN_ENTRY_NONE;
        }
    }
    /* The texture binds below write GX registers: nothing may still be
     * streaming the previous fighter's list into the FIFO, and no batch of
     * the previous draw is left open. */
    ndsFighterPacketDmaWait();
    ndsRendererHardwareEndBatch();
    if ((ndsRendererNativeSelectFighterRuntimeTables(
             owner_slot, use_low_detail) == FALSE) ||
        (ndsRendererNativePreflightProductionOwner(
             owner_slot, battle_slot, use_low_detail, asset_base, inputs,
             input_count, NULL, stats) == FALSE))
    {
        return nNDSFtrLeanDeclineTables;
    }
    root_count = sNdsNativeFighterActiveOwner->root_count;
    palette_slots = sNdsNativeFighterActiveOwner->cross_palette_slots;
    owner_tables = sNdsNativeFighterActiveTables;
    if (entry == 0u)
    {
        /* Entry 0 holds a list from here on: the spare may swap with it. */
        if (slot_state->lower_owned == 0u)
        {
            NDS_FTR_LEAN_CTR(gNdsFtrLean.region_takes++);
        }
        slot_state->lower_owned = 1u;
    }
    packet = ndsFtrLeanEntryPacket(battle_slot, entry);
    memset(packet, 0, offsetof(NDSFighterPacket, roots));
    packet->words = ndsFtrLeanEntryWords(battle_slot, entry);
    packet->word_capacity = capacity;
    packet->root_count = root_count;
    packet->projection_index = NDS_FIGHTER_PACKET_INDEX_NONE;
    packet->light_index = NDS_FIGHTER_PACKET_INDEX_NONE;
    packet->tint_bind_count = 0u;
    packet->tint_bind_overflow = 0u;
    packet->fence_other = 0u;
    packet->tint_bind_seen = 0u;
    /* TryReplay's arming: the packet names the live modulate, and the shade
     * words below are derived at it (the traversal takes it after its init
     * below), exactly as a record of this draw would; the per-frame ApplyTint
     * then evolves them as the replay evolves its packet's. They used to be
     * derived at the traversal's own 0 under the live name, so a fighter
     * recorded inside a colour animation -- the challenger's black, a hurt
     * flash -- drew and replayed in its plain colours. */
    packet->tint_modulate = inputs[0].config->color_modulate;
    packet->tint_prim_hash = 2166136261u;
    for (i = 0u; i < input_count; i++)
    {
        packet->tint_prim_hash = ndsFighterPacketMix(
            packet->tint_prim_hash, inputs[i].preamble->prim_color);
    }
    memset(&m, 0, sizeof(m));
#if NDS_FTR_LEAN_TIMED
    m.mark = cpuGetTiming();
#endif
    m.packet = packet;
    m.pk.words = packet->words;
    m.pk.capacity = capacity;
    m.pk.cmd_slot = 4u;
    m.texgen_group = NDS_FIGHTER_PACKET_TEXGEN_GROUP_NONE;
    m.vtx10 = vtx10;
    m.hw_texgen_ok = (owner_slot >= NDS_RENDERER_NATIVE_FIGHTER_OWNER_MMARIO) ?
        1u : 0u;
    ndsRendererInitTraversalState(state, NULL, stats, NULL, NULL, 0u);
    state->color_modulate = packet->tint_modulate;
    if (*sNdsNativeFighterActiveDenseNormalsBuilt == 0u)
    {
        ndsRendererR2BuildDenseNormals();
    }
    /* The head: the execute's light colour, P' once, and the light vector
     * under an identity vector matrix (its word is patched per draw). */
    ndsFtrLeanPack1(&m.pk, REG2ID(GFX_LIGHT_COLOR),
                    (u32)RGB15(31, 31, 31));
    ndsFtrLeanPack1(&m.pk, REG2ID(MATRIX_CONTROL), (u32)GL_PROJECTION);
    i = ndsFtrLeanPack(&m.pk, REG2ID(MATRIX_LOAD4x4), 16u);
    if (m.pk.fault == 0u)
    {
        memset(&m.pk.words[i], 0, 16u * sizeof(u32));
        packet->projection_index = (u16)i;
    }
    ndsFtrLeanPack1(&m.pk, REG2ID(MATRIX_CONTROL), (u32)GL_MODELVIEW);
    (void)ndsFtrLeanPack(&m.pk, REG2ID(MATRIX_IDENTITY), 0u);
    i = ndsFtrLeanPack(&m.pk, REG2ID(GFX_LIGHT_VECTOR), 1u);
    if (m.pk.fault == 0u)
    {
        m.pk.words[i] = 0u;
        packet->light_index = (u16)i;
        m.light_cmd_word = m.pk.cmd_word;
        m.light_cmd_slot = m.pk.cmd_slot - 1u;
    }
    es->donor_mask = 0u;
    /* ROOT BLOCKS: a plan whose roots are all self-contained records its
     * blocks, and copies those a donor (the other entry, the spare) holds. */
    record = (root_count <= NDS_FTR_LEAN_ROOT_RECS) ? TRUE : FALSE;
    for (root_index = 0u; (record != FALSE) && (root_index < root_count);
         root_index++)
    {
        const NDSRendererNativeFighterRoot *input = &inputs[root_index];
        u32 k;

        if (sNdsNativeProductionResolvedRoots[root_index]->light_preamble !=
            0u)
        {
            plan_any |= NDS_FTR_LEAN_ANY_LIGHT;
        }
        if ((input->preamble != NULL) &&
            ((input->preamble->flags &
              NDS_RENDERER_NATIVE_PREAMBLE_LIGHT_VALID) != 0u))
        {
            plan_any |= NDS_FTR_LEAN_ANY_LIGHT_DIR;
        }
        for (k = 0u; k < input->material_count; k++)
        {
            const u32 fx = input->materials[k].effects;

            if ((fx & NDS_RENDERER_NATIVE_MATERIAL_PALETTE_TLUT) != 0u)
            {
                plan_any |= NDS_FTR_LEAN_ANY_TLUT;
            }
            if ((fx & NDS_RENDERER_NATIVE_MATERIAL_BLEND) != 0u)
            {
                plan_any |= NDS_FTR_LEAN_ANY_BLEND;
            }
            if ((fx & (NDS_RENDERER_NATIVE_MATERIAL_LIGHT1 |
                       NDS_RENDERER_NATIVE_MATERIAL_LIGHT2)) != 0u)
            {
                plan_any |= NDS_FTR_LEAN_ANY_LIGHT;
            }
        }
    }
    for (root_index = 0u; (record != FALSE) && (root_index < root_count);
         root_index++)
    {
        const NDSNativeFighterRuntimeTables *t =
            sNdsNativeProductionResolvedTables[root_index];
        const NDSNativeRoot *r = sNdsNativeProductionResolvedRoots[root_index];
        const u32 light_ok =
            ((r->light_preamble != 0u) &&
             ((u32)r->light_preamble <
              sNdsNativeProductionResolvedLightPreambleCounts[root_index])) ?
                TRUE : FALSE;

        const u32 self = ndsFtrLeanRootSelfContainedCached(
            t, r, light_ok, &inputs[root_index], plan_any);

        if (self == FALSE)
        {
            record = FALSE;
        }
        else if ((self & NDS_FTR_LEAN_SELF_BLEND) != 0u)
        {
            blend_roots |= 1u << root_index;
        }
    }
    NDS_FTR_LEAN_CTR(gNdsFtrLean.root_plans += (record != FALSE) ? 1u : 0u);
    if ((record != FALSE) && (no_copy == FALSE) &&
        (gNdsFtrLeanRootCopyOff == 0u))
    {
        u32 e;

        for (e = 0u; e <= NDS_FTR_LEAN_SPARE; e++)
        {
            const NDSFtrLeanEntryState *des = &slot_state->entry[e];
            NDSFighterPacket *dp;

            if ((e == entry) || (des->valid == 0u) ||
                ((e < NDS_FTR_LEAN_ENTRIES) &&
                 (ndsFtrLeanEntryUsable(e) == FALSE)) ||
                ((e == NDS_FTR_LEAN_SPARE) &&
                 ((slot_state->spare_buf == NULL) ||
                  (slot_state->spare_gen != gNdsTaskmanHeapGeneration))) ||
                ((des->variant_count != 0u) && (des->variant_cur != 0u)) ||
                (des->tint_set_generation != gNdsFighterTintSetGeneration) ||
                (des->tint_repatch != 0u) || (des->vtx10 != vtx10))
            {
                continue;
            }
            dp = ndsFtrLeanEntryPacket(battle_slot, e);
            if ((dp == packet) || (dp->valid == 0u) ||
                (dp->root_rec_count == 0u) ||
                (dp->texgen_site_count != 0u))
            {
                continue;
            }
            donors[donor_count++] = dp;
        }
    }
    NDS_FTR_LEAN_CTR(gNdsFtrLean.root_donor_plans +=
                         (donor_count != 0u) ? 1u : 0u);
    NDS_FTR_LEAN_MAT_PART(&m, 0u);
    for (root_index = 0u; (root_index < root_count) && (reason == 0u);
         root_index++)
    {
        const NDSRendererNativeFighterRoot *input = &inputs[root_index];
        const u8 *root_asset_base = (input->asset_base != NULL) ?
            (const u8 *)input->asset_base : (const u8 *)asset_base;
        const NDSNativeRoot *root =
            sNdsNativeProductionResolvedRoots[root_index];
        NDSFighterPacketRoot *root_entry = &packet->roots[root_index];
        u32 palette_slot = palette_slots[root_index];
        u32 epoch_offset;
        u32 j;
        u32 root_key = 0u;
        u32 block_start;
        u32 snap_sites;
        u32 snap_tints;
        u32 snap_tint_seen;
        u32 snap_fence_other;
        u32 snap_combine;
        u32 snap_groups;
        NDSFtrLeanMat snap;

        sNdsNativeFighterActiveTables =
            sNdsNativeProductionResolvedTables[root_index];
        sNdsNativeFighterActiveRootLightPreambles =
            sNdsNativeProductionResolvedLightPreambles[root_index];
        sNdsNativeFighterActiveRootLightPreambleCount =
            sNdsNativeProductionResolvedLightPreambleCounts[root_index];
        if (sNdsNativeFighterActiveTables->dense_normals != NULL)
        {
            sNdsNativeFighterActiveDenseNormals =
                (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        }
        if (sNdsNativeFighterActiveTables != owner_tables)
        {
            es->donor_mask |= 1u << root_index;
        }
        if (record != FALSE)
        {
            root_key = ndsFtrLeanRootKey(
                sNdsNativeFighterActiveTables, root, input, root_asset_base,
                (const void *)sNdsNativeFighterActiveRootLightPreambles,
                sNdsNativeFighterActiveRootLightPreambleCount, palette_slot,
                m.vtx10, gNdsFtrLeanRootMaterialKey[root_index]);
            if ((blend_roots & (1u << root_index)) != 0u)
            {
                root_key = ndsFtrLeanRootKeyCarriedBlend(root_key,
                                                         stats->blend_color);
            }
        }
        if (donor_count != 0u)
        {
            u32 copied = FALSE;
            u32 d;

            for (d = 0u; (d < donor_count) && (copied == FALSE); d++)
            {
                NDSFighterPacket *dp = donors[d];
                const NDSFtrLeanRootRec *drecs = ndsFtrLeanRootRecs(dp);
                u32 k;

                for (k = 0u; k < (u32)dp->root_rec_count; k++)
                {
                    if ((drecs[k].key != root_key) ||
                        ((drecs[k].flags & NDS_FTR_LEAN_ROOT_COPY) == 0u))
                    {
                        continue;
                    }
                    NDS_FTR_LEAN_CTR(gNdsFtrLean.root_key_hits++);
                    if (ndsFtrLeanRootRecResident(dp, &drecs[k]) != FALSE)
                    {
                        const NDSRendererNativeFighterPreamble *pre =
                            input->preamble;
                        NDSFtrLeanRootRec copy_rec;

                        copied = ndsFtrLeanRootCopy(
                            &m, root_index, dp, k, &drecs[k], inputs,
                            ((pre != NULL) &&
                             ((pre->flags &
                               NDS_RENDERER_NATIVE_PREAMBLE_LIGHT_VALID) !=
                              0u)) ?
                                ndsFighterPacketLightWord(pre->light_dir_x,
                                                          pre->light_dir_y,
                                                          pre->light_dir_z) :
                                ndsFighterPacketLightWord(stats->light_dir_x,
                                                          stats->light_dir_y,
                                                          stats->light_dir_z),
                            stats, &copy_rec);
                        if ((copied != FALSE) &&
                            (packet->texgen_site_count == 0u))
                        {
                            ndsFtrLeanRootRecs(packet)[root_index] = copy_rec;
                        }
                    }
                    break;
                }
            }
            if (copied != FALSE)
            {
                NDS_FTR_LEAN_CTR(gNdsFtrLean.root_copies++);
                /* The roots after it read the colour its walk leaves. */
                stats->blend_color = ndsFtrLeanRootBlendEnd(
                    sNdsNativeFighterActiveTables, root, input,
                    stats->blend_color);
                continue;
            }
        }
        NDS_FTR_LEAN_CTR(gNdsFtrLean.root_walks++);
        /* The block starts at a fresh header (ROOT BLOCKS). */
        if (record != FALSE)
        {
            ndsFtrLeanPackClose(&m.pk);
        }
        block_start = m.pk.count;
        snap = m;
        snap_sites = packet->site_count;
        snap_tints = packet->tint_bind_count;
        snap_tint_seen = packet->tint_bind_seen;
        snap_fence_other = packet->fence_other;
        snap_combine = stats->texture_combine_count;
        snap_groups = packet->texgen_group_count;
        m.root_tex_mask = 0u;
        m.root_lit = 0u;
        m.root_fence = 0u;
        ndsRendererNativeBindProductionRoot(state, input, stats);
        /* ndsFighterPacketBeginRoot, then the root's one LOAD4x3. */
        m.root = root_index;
        m.prim_overridden = 0u;
        root_entry->seed_index = NDS_FIGHTER_PACKET_INDEX_NONE;
        for (j = 0u; j < NDS_FIGHTER_PACKET_LOCAL_MAX; j++)
        {
            root_entry->local_index[j] = NDS_FIGHTER_PACKET_INDEX_NONE;
        }
        root_entry->local_count = 0u;
        root_entry->parent_slot = input->gx_parent_slot;
        root_entry->store_slot = input->gx_store_slot;
        root_entry->seed_is_identity = input->gx_seed_is_identity;
        i = ndsFtrLeanPack(&m.pk, REG2ID(MATRIX_LOAD4x3), 12u);
        if (m.pk.fault == 0u)
        {
            memset(&m.pk.words[i], 0, 12u * sizeof(u32));
            root_entry->seed_index = (u16)i;
        }
        if (palette_slot <= NDS_NATIVE_GX_MATRIX_SLOT_MAX)
        {
            ndsFtrLeanPack1(&m.pk, REG2ID(MATRIX_STORE), palette_slot);
        }
        ndsRendererNativeApplyRootLightPreamble(root, stats);
        NDS_FTR_LEAN_MAT_PART(&m, 1u);
        for (epoch_offset = 0u;
             (epoch_offset < root->epoch_count) && (reason == 0u);
             epoch_offset++)
        {
            u32 epoch_index = root->first_epoch + epoch_offset;
            const NDSNativeEpoch *epoch =
                &sNdsNativeFighterActiveTables->epochs[epoch_index];
            u32 epoch_policy =
                sNdsNativeFighterActiveTables->epoch_direct_policy[epoch_index];
            u32 run_offset;

            if (ndsFtrLeanSpanSetsPrim(epoch->before_state_first,
                                       epoch->before_state_count) != FALSE)
            {
                m.prim_overridden = 1u;
            }
            ndsRendererNativeApplyStateSpanPreflight(
                epoch->before_state_first, epoch->before_state_count,
                epoch->before_sync_count, root_asset_base, stats, state);
            if (epoch->material_slot != NDS_NATIVE_MATERIAL_NONE)
            {
                const NDSRendererNativeMaterial *material =
                    &input->materials[epoch->material_slot];

                if ((material->effects &
                     NDS_RENDERER_NATIVE_MATERIAL_PRIM) != 0u)
                {
                    m.prim_overridden = 1u;
                }
                ndsRendererNativeApplyMaterialPreflight(material, stats,
                                                        state);
            }
            if (ndsFtrLeanSpanSetsPrim(epoch->after_state_first,
                                       epoch->after_state_count) != FALSE)
            {
                m.prim_overridden = 1u;
            }
            ndsRendererNativeApplyStateSpanPreflight(
                epoch->after_state_first, epoch->after_state_count,
                epoch->after_sync_count, root_asset_base, stats, state);
            if (stats->blocker != NDS_RENDERER_BLOCKER_NONE)
            {
                reason = nNDSFtrLeanDeclineTables;
                break;
            }
            NDS_FTR_LEAN_MAT_PART(&m, 2u);
            ndsFtrLeanMatShade(&m, epoch, epoch_policy, stats, state);
            NDS_FTR_LEAN_MAT_PART(&m, 3u);
            for (run_offset = 0u;
                 (run_offset < epoch->run_count) && (reason == 0u);
                 run_offset++)
            {
                reason = ndsFtrLeanMatRun(
                    &m, (u32)epoch->first_run + run_offset, epoch_policy,
                    palette_slot, input->config, stats, state);
            }
            if ((m.pk.fault | m.vtx10_fail) != 0u)
            {
                /* 2026-10-12: this walk's list is already lost (an entry
                 * outgrown, a corner VTX_10 cannot hold) and the walk's
                 * rest would be wasted: stop here (the caller walks again
                 * wide or at another corner format, or declines). */
                break;
            }
        }
        if ((m.pk.fault | m.vtx10_fail) != 0u)
        {
            break;
        }
        ndsRendererNativeApplyStateSpanPreflight(
            root->tail_state_first, root->tail_state_count,
            root->tail_sync_count, root_asset_base, stats, state);
        if (record != FALSE)
        {
            /* The block ends at a closed header; its record (ROOT BLOCKS). */
            ndsFtrLeanPackClose(&m.pk);
            if ((m.pk.fault == 0u) && (packet->texgen_site_count == 0u))
            {
                NDSFtrLeanRootRec *rec = &ndsFtrLeanRootRecs(packet)[root_index];
                const u32 raw_tri = m.raw_triangles - snap.raw_triangles;
                const u32 raw_runs = raw_tri - (m.raw_reuse - snap.raw_reuse);
                const u32 cross_tri = m.cross_triangles - snap.cross_triangles;
                const u32 cross_runs =
                    cross_tri - (m.cross_reuse - snap.cross_reuse);
                const u32 triangles = m.triangle_count - snap.triangle_count;
                const u32 vertex_loads = m.vertex_loads - snap.vertex_loads;
                const u32 prepare_begin = m.prepare_begin - snap.prepare_begin;
                const u32 prepare_reuse = m.prepare_reuse - snap.prepare_reuse;
                const u32 texture_binds = m.texture_binds - snap.texture_binds;
                const u32 tint_binds = m.tint_binds - snap.tint_binds;
                const u32 sites = packet->site_count - snap_sites;
                const u32 tints = packet->tint_bind_count - snap_tints;
                const u32 tint_seen = packet->tint_bind_seen - snap_tint_seen;
                const u32 combines =
                    stats->texture_combine_count - snap_combine;
                const u32 copyable =
                    ((combines <= 0xffu) &&
                     (packet->texgen_group_count == snap_groups) &&
                     (snap.hw_texgen == 0u) && (m.hw_texgen == 0u) &&
                     (m.tint_pending == snap.tint_pending) &&
                     (packet->fence_other == snap_fence_other) &&
                     (packet->tint_bind_overflow == 0u) &&
                     (m.pk.count <= 0xffffu) && (raw_runs <= 0xffu) &&
                     (cross_runs <= 0xffu) && (raw_tri <= 0xffffu) &&
                     (cross_tri <= 0xffffu) && (triangles <= 0xffffu) &&
                     (vertex_loads <= 0xffffu) && (prepare_begin <= 0xffu) &&
                     (prepare_reuse <= 0xffu) && (texture_binds <= 0xffu) &&
                     (tint_binds <= 0xffu) && (tint_seen <= 0xffu) &&
                     (snap_sites <= 0xffu) && (sites <= 0xffu) &&
                     (snap_tints <= 0xffu) && (tints <= 0xffu)) ?
                        TRUE : FALSE;

                rec->key = root_key;
                rec->start = (u16)block_start;
                rec->end = (u16)m.pk.count;
                rec->site_first = (u8)snap_sites;
                rec->site_count = (u8)sites;
                rec->tint_first = (u8)snap_tints;
                rec->tint_count = (u8)tints;
                rec->tex_mask = m.root_tex_mask;
                rec->triangles = (u16)triangles;
                rec->vertex_loads = (u16)vertex_loads;
                rec->raw_triangles = (u16)raw_tri;
                rec->cross_triangles = (u16)cross_tri;
                rec->raw_runs = (u8)raw_runs;
                rec->cross_runs = (u8)cross_runs;
                rec->prepare_begin = (u8)prepare_begin;
                rec->prepare_reuse = (u8)prepare_reuse;
                rec->texture_binds = (u8)texture_binds;
                rec->tint_binds = (u8)tint_binds;
                rec->tint_seen = (u8)tint_seen;
                rec->combine_base =
                    (u8)(snap_combine & NDS_RENDERER_POLY_ID_MASK);
                rec->combine_count = (u8)combines;
                rec->flags = (u8)(((copyable != FALSE) ?
                                   NDS_FTR_LEAN_ROOT_COPY : 0u) |
                                  ((m.root_lit != 0u) ?
                                   NDS_FTR_LEAN_ROOT_LIT : 0u) |
                                  ((m.root_fence != 0u) ?
                                   NDS_FTR_LEAN_ROOT_FENCE : 0u));
            }
        }
    }
    /* Every GX tracker is re-derived by the next writer (the lean submit
     * resets the same ones after its DMA). */
    ndsRendererHardwareInvalidateGXState(NDS_RENDERER_GX_STATE_ALL);
    sNdsRendererHardwareBoundTextureName = 0u;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    if ((reason == 0u) && (m.hw_texgen != 0u))
    {
        /* The record's end restores the identity texture matrix too. */
        ndsFtrLeanPack1(&m.pk, REG2ID(MATRIX_CONTROL), (u32)GL_TEXTURE);
        (void)ndsFtrLeanPack(&m.pk, REG2ID(MATRIX_IDENTITY), 0u);
        ndsFtrLeanPack1(&m.pk, REG2ID(MATRIX_CONTROL), (u32)GL_MODELVIEW);
        m.hw_texgen = 0u;
    }
    if (reason == 0u)
    {
        if ((m.pk.header_valid != 0u) && (m.pk.header_params == 0u))
        {
            if (m.pk.count < m.pk.capacity)
            {
                m.pk.words[m.pk.count++] = 0u;
            }
            else
            {
                m.pk.fault = 1u;
            }
        }
        if ((m.pk.fault != 0u) ||
            (packet->projection_index == NDS_FIGHTER_PACKET_INDEX_NONE) ||
            (packet->light_index == NDS_FIGHTER_PACKET_INDEX_NONE))
        {
            reason = nNDSFtrLeanDeclineCapacity;
        }
    }
    if ((reason == 0u) && (m.vtx10_fail != 0u))
    {
        sNdsFtrLeanLastVtxFail = m.vtx10_fail;
        reason = nNDSFtrLeanDeclineVtx10;
    }
    if (reason != 0u)
    {
        return reason;
    }
    if (m.light_written == FALSE)
    {
        /* No lit epoch: the execute writes no light vector, so the head's
         * vector command becomes a MTX_MODE(MODELVIEW) -- the mode it is
         * already in -- and the list leaves the light untouched too. */
        m.pk.words[m.light_cmd_word] =
            (m.pk.words[m.light_cmd_word] &
             ~(0xffu << (m.light_cmd_slot * 8u))) |
            ((u32)REG2ID(MATRIX_CONTROL) << (m.light_cmd_slot * 8u));
        m.pk.words[packet->light_index] = (u32)GL_MODELVIEW;
        packet->light_index = NDS_FIGHTER_PACKET_INDEX_NONE;
        packet->light_valid = 0u;
    }
    packet->word_count = m.pk.count;
    packet->valid = 1u;
    /* ROOT BLOCKS: every root's record was written (a list with texgen sites
     * keeps none: the records share their table). */
    packet->root_rec_count =
        ((record != FALSE) && (packet->texgen_site_count == 0u)) ?
            (u8)root_count : 0u;
    packet->run_count = m.run_count;
    packet->triangle_count = m.triangle_count;
    packet->raw_triangles = m.raw_triangles;
    packet->raw_reuse = m.raw_reuse;
    packet->cross_triangles = m.cross_triangles;
    packet->cross_reuse = m.cross_reuse;
    /* The frame-summary counts this list presents to the hardware (the
     * lean submit credits them as the replay credits a packet's): one
     * prepare per fresh or reused run prepare, a batch per fresh prepare or
     * tint bind, the head's P' plus one LOAD4x3 per root, one bind per
     * textured prepare. */
    packet->prepare_begin = m.prepare_begin;
    packet->prepare_reuse = m.prepare_reuse;
    packet->batch_begin = m.prepare_begin + m.tint_binds;
    packet->batch_end = packet->batch_begin;
    packet->batch_reuse = (m.run_count > packet->batch_begin) ?
        (m.run_count - packet->batch_begin) : 0u;
    packet->matrix_loads = 1u + root_count;
    packet->texture_binds = m.texture_binds;
    packet->vertex_loads = m.vertex_loads;
    DC_FlushRange(packet->words, packet->word_count * sizeof(u32));
    memset(es->dirty, 0, sizeof(es->dirty));
    for (i = 0u; i < NDS_FTR_LEAN_KEY_WORDS; i++)
    {
        es->key[i] = key[i];
    }
    es->tint_set_generation = gNdsFighterTintSetGeneration;
    es->tint_folds = m.tint_pending;
    es->tint_repatch = 0u;
    es->variant_count = 0u;
    es->variant_cur = 0u;
    es->variant_next = 1u;
    es->vtx10 = m.vtx10;
    es->fence_needed = ((packet->needs_fence != 0u) &&
                        ((packet->fence_other != 0u) ||
                         (packet->tint_bind_overflow != 0u))) ? 1u : 0u;
    es->fence = sNdsRendererHardwareTextureKeyGeneration ^
        (sNdsRendererRuntimeTextureCacheEvictCount << 16);
    /* Slice 6: the lean list's own key words are unused (the entry state
     * holds its key); key[5] is the fence of its last (virtual) record, as the
     * old path's packet keys it -- see ndsFtrLeanPacketPatch. */
    packet->key[5] = record_fence;
    es->all_pinned = 1u;
    for (i = 0u; i < (u32)packet->texture_count; i++)
    {
        u32 cache_slot = (u32)packet->textures[i].slot_plus1 - 1u;

        if ((cache_slot >= NDS_RENDERER_HW_TEXTURE_CACHE_COUNT) ||
            ((sNdsRendererHardwareTextureCache[cache_slot].pinned == 0u) &&
             (sNdsRendererHardwareTextureCache[cache_slot].admitted == 0u)))
        {
            es->all_pinned = 0u;
        }
#if NDS_FTR_LEAN_LAB
        if (cache_slot < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT)
        {
            slot_state->union_mask[cache_slot >> 5] |=
                1u << (cache_slot & 31u);
        }
#endif
    }
    es->light_valid = 0u;
    es->texgen_first = 0xffffffffu;
    es->texgen_last = 0u;
    for (i = 0u; i < (u32)packet->texgen_site_count; i++)
    {
        u32 index = packet->texgen_sites[i].index;

        if (index < es->texgen_first)
        {
            es->texgen_first = index;
        }
        if (index > es->texgen_last)
        {
            es->texgen_last = index;
        }
    }
    for (i = 0u; i < (u32)packet->texgen_group_count; i++)
    {
        /* A GX texgen group's twelve matrix words (origin_s names the first)
         * are patched every draw like the sites. */
        const NDSFighterPacketTexgenGroup *group = &packet->texgen_groups[i];

        if (group->reserved[1] != 0u)
        {
            if (group->origin_s < es->texgen_first)
            {
                es->texgen_first = group->origin_s;
            }
            if ((group->origin_s + 11u) > es->texgen_last)
            {
                es->texgen_last = group->origin_s + 11u;
            }
        }
    }
#if NDS_LINK
    es->texgen_map = ndsFtrLeanBuildTexgenMap(packet);
#else
    es->texgen_map = NDS_FTR_LEAN_TEXGEN_MAP_NONE;
#endif
    /* Variant records fill the entry's tail top down (ndsFtrLeanVariant-
     * Record). Slice 6: above the texgen site map when the list has one --
     * the map sits right after the words, and Link's lists (28 sites) put
     * records 6-7 over it: the dense slots and site_slot[] would be
     * overwritten by the seventh learned record. A wide list keeps them
     * above the room a learn in place takes (NDSFtrLeanWideHold), and at
     * least two (its base and one more) or none. */
    {
        u32 floor = packet->word_count;
        u32 word_capacity = ndsFtrLeanWordCapacity();

#if NDS_LINK
        if (es->texgen_map != NDS_FTR_LEAN_TEXGEN_MAP_NONE)
        {
            floor = ((floor + 7u) & ~7u) + NDS_FTR_LEAN_TEXGEN_MAP_WORDS;
        }
#endif
        if (wide != FALSE)
        {
            /* The learn's copy off the region when the scratch takes it. */
            if (ndsFtrLeanWideScratch(packet->word_count, TRUE) == NULL)
            {
                floor += NDS_FTR_LEAN_WIDE_RELEARN_WORDS(packet->word_count);
            }
            word_capacity = ndsFtrLeanWideCapacity();
        }
        es->variant_max = (floor >= word_capacity) ? 0u :
            ((word_capacity - floor) /
             NDS_FTR_LEAN_VARIANT_WORDS);
        if ((wide != FALSE) && (es->variant_max < 2u))
        {
            es->variant_max = 0u;
        }
        if (es->variant_max > NDS_FTR_LEAN_VARIANT_MAX)
        {
            es->variant_max = NDS_FTR_LEAN_VARIANT_MAX;
        }
    }
    es->valid = (wide != FALSE) ? (1u | NDS_FTR_LEAN_VALID_WIDE) : 1u;
#if NDS_FTR_LEAN_LAB
    gNdsFtrLean.materializations++;
    gNdsFtrLean.materialize_words = packet->word_count;
    if (packet->word_count > gNdsFtrLean.materialize_words_max)
    {
        gNdsFtrLean.materialize_words_max = packet->word_count;
    }
    gNdsFtrLean.materialize_roots = root_count;
    gNdsFtrLean.materialize_textures = packet->texture_count;
    gNdsFtrLean.materialize_tint_binds = m.tint_binds;
    gNdsFtrLean.materialize_tint_pending = m.tint_pending;
    if (slot_state->kind < NDS_FTR_LEAN_KINDS)
    {
        gNdsFtrLean.k_materializations[slot_state->kind]++;
        if (packet->word_count > gNdsFtrLean.k_words_max[slot_state->kind])
        {
            gNdsFtrLean.k_words_max[slot_state->kind] = packet->word_count;
        }
    }
#endif
    return 0u;
}
