/* Native renderer, entry effects: the P4 roots and corner blocks, ramp palettes,
 * live materials, packet replay and texture preparation. A slice of nds_renderer.c. */

#ifdef NDS_ENTRY_EFFECT_P4_ROOT_COUNT
/* The battle's players' contents: the VS select's or a lab descriptor's, and
 * in a campaign, bonus or training battle its own seats' (P7 C1: Palutena's
 * and Snake's article roots failed closed in the 1P game, their corner
 * blocks never loaded). nds_p4.c. */
extern u32 ndsP4MatchContent(s32 player);

static sb32 ndsRendererEntryEffectP4ContentInMatch(u32 content)
{
    u32 player;

#ifdef NDS_ENTRY_EFFECT_P5_CONTENT_BASE
    /* P5: a Remix stage's hazard item roots (scripts/p5/p5_articles.py) are
     * in the match on their stage (src/port/nds_p5_stages.c). */
    if (content >= NDS_ENTRY_EFFECT_P5_CONTENT_BASE)
    {
        extern u32 ndsP5BattleGkind(void);
        u32 k = content - NDS_ENTRY_EFFECT_P5_CONTENT_BASE;

        return ((k < NDS_ENTRY_EFFECT_P5_STAGE_COUNT) &&
                (ndsP5BattleGkind() == (u32)sNdsEntryEffectP5StageGkind[k])) ?
            TRUE : FALSE;
    }
#endif

    for (player = 0u; player < 4u; player++)
    {
        if (ndsP4MatchContent((s32)player) == content)
        {
            return TRUE;
        }
    }
    return FALSE;
}

/* Renderer adapter: a P4 article root is the display list at its content's
 * special file plus the root's offset (the file moves per load, the offset
 * never does). Fills the root's asset, offset and TEXID frame count (0: no
 * live material), with NDS_P4_VARIANT_PALETTE_FOLLOWS set when frame i was
 * baked with palette i (Sonic's spring) and NDS_P4_VARIANT_MATERIAL when a
 * list with no TEXID frames still draws through the MObj's segment-E branch
 * (the eggs, the grape, the needle). renderer_adapter_stage.c defines the
 * same bits. */
#define NDS_P4_VARIANT_PALETTE_FOLLOWS 0x80u
#define NDS_P4_VARIANT_MATERIAL 0x40u
/* An entry root whose entry is over (ndsRendererEntryP4RetireEntries): the
 * caller draws nothing. */
#define NDS_P4_VARIANT_RETIRED 0x20u
/* The rows' answers by display list (2026-10-13): every effect, weapon and
 * item DObj asked on every draw, and the walk reads each row's file pointer
 * -- four Sonics' entry planes paid 122K cycles a frame here. A list's
 * answer holds for the heap generation: the contents' special files load
 * once a scene and its bump heap never reuses an address within one (the
 * dead bytes a VS battle lends out are reused, but never as a display list
 * a DObj names). A list probes up to NDS_ENTRY_P4_ADMIT_PROBES ways from its
 * home way and takes the first empty one, or its home way when none is
 * (P8, 2026-10-23: direct-mapped, two lists drawn in one frame could share a
 * way and rescan the rows on every draw). */
#define NDS_ENTRY_P4_ADMIT_MEMO 256u
#define NDS_ENTRY_P4_ADMIT_PROBES 4u
#define NDS_ENTRY_P4_ADMIT_NONE 0xffu
_Static_assert(NDS_ENTRY_EFFECT_P4_ROOT_COUNT < NDS_ENTRY_P4_ADMIT_NONE,
               "the admission memo keeps a P4 row in a byte");
static const void *sNdsEntryP4AdmitMemoDl[NDS_ENTRY_P4_ADMIT_MEMO];
static u8 sNdsEntryP4AdmitMemoRow[NDS_ENTRY_P4_ADMIT_MEMO];
static u32 sNdsEntryP4AdmitMemoGen;

/* ENTRY ROOTS RETIRE (P7, 2026-10-21). A content's entry vehicles, gates and
 * waves (articles.json "entry") draw only before GO in a VS battle, and a
 * VSBattle never asks for them again after it; their corners are then dead
 * bytes of this scene's heap. ndsRendererEntryP4RetireEntries marks a
 * content's entry roots retired and lends their corner runs to the battle
 * (scvsbattle.c once GO has come and no entry root drew for two frames; and
 * at once for an entry-only special file that did not fit, nds_p4.c, whose
 * roots cannot draw). A retired root's draw is nothing. */
#if defined(NDS_ENTRY_EFFECT_P4_CORNER_BLOCKS) && \
    defined(NDS_ENTRY_EFFECT_P4_ENTRY_RUN_COUNT)
#if NDS_ENTRY_EFFECT_P4_ENTRY_RUN_COUNT > 0
#define NDS_ENTRY_P4_RETIRE 1
#endif
#endif
#ifndef NDS_ENTRY_P4_RETIRE
#define NDS_ENTRY_P4_RETIRE 0
#endif
#if NDS_ENTRY_P4_RETIRE
static u32 sNdsEntryP4Retired[(NDS_ENTRY_EFFECT_P4_ROOT_COUNT + 31u) / 32u];
static u32 sNdsEntryP4RetiredScene = 0xffffffffu;
/* The presented frame that last drew an entry root, as
 * sNdsEntryEffectStartupDrawSerial is for the startup-only textures. */
static u32 sNdsEntryP4EntryDrawSerial;
/* Contents retired so far: the P4 state cache starts over at each, so the
 * entry roots' slots go to the roots the match draws after GO. */
static u32 sNdsEntryP4RetireCount;

static sb32 ndsRendererEntryP4IsEntry(u32 p)
{
    return ((sNdsEntryEffectP4EntryRoots[p >> 5] >> (p & 31u)) & 1u) != 0u;
}

u32 ndsRendererEntryP4EntryIdleFrames(void)
{
    return (sNdsRendererHardwareFrameSerial + 1u) - sNdsEntryP4EntryDrawSerial;
}
#endif

static void ndsRendererEntryEffectP4AdmitRow(u32 p, u32 *owner_asset_id,
                                             u32 *root_offset,
                                             u32 *variant_count)
{
    const NDSEntryEffectP4Root *row = &sNdsEntryEffectP4Roots[p];

    *owner_asset_id = row->asset_id;
    *root_offset =
        sNdsEntryEffectRoots[NDS_ENTRY_EFFECT_P4_ROOT_FIRST + p].source_offset;
    *variant_count = row->variant_count |
        (((row->material_flags & 1u) != 0u) ? NDS_P4_VARIANT_PALETTE_FOLLOWS : 0u) |
        (((row->material_flags & 2u) != 0u) ? NDS_P4_VARIANT_MATERIAL : 0u);
#if NDS_ENTRY_P4_RETIRE
    if (ndsRendererEntryP4IsEntry(p) != FALSE)
    {
        if ((sNdsEntryP4RetiredScene == gNdsRendererSceneTextureVramResetCount) &&
            (((sNdsEntryP4Retired[p >> 5] >> (p & 31u)) & 1u) != 0u))
        {
            *variant_count |= NDS_P4_VARIANT_RETIRED;
        }
        else
        {
            sNdsEntryP4EntryDrawSerial = sNdsRendererHardwareFrameSerial + 1u;
        }
    }
#endif
}

sb32 ndsRendererEntryEffectP4Admit(const void *dl, u32 *owner_asset_id,
                                   u32 *root_offset, u32 *variant_count)
{
    const u32 home = ((u32)(uintptr_t)dl * 2654435761u) >> 24;
    u32 way = home;
    u32 probe;
    u32 p;

    if (sNdsEntryP4AdmitMemoGen != gNdsTaskmanHeapGeneration + 1u)
    {
        memset(sNdsEntryP4AdmitMemoDl, 0, sizeof(sNdsEntryP4AdmitMemoDl));
        sNdsEntryP4AdmitMemoGen = gNdsTaskmanHeapGeneration + 1u;
    }
    for (probe = 0u; (dl != NULL) && (probe < NDS_ENTRY_P4_ADMIT_PROBES);
         probe++)
    {
        const void *memo_dl = sNdsEntryP4AdmitMemoDl[way];

        if (memo_dl == dl)
        {
            p = sNdsEntryP4AdmitMemoRow[way];
            if (p == NDS_ENTRY_P4_ADMIT_NONE)
            {
                return FALSE;
            }
            ndsRendererEntryEffectP4AdmitRow(p, owner_asset_id, root_offset,
                                             variant_count);
            return TRUE;
        }
        if (memo_dl == NULL)
        {
            break;
        }
        way = (way + 1u) & (NDS_ENTRY_P4_ADMIT_MEMO - 1u);
    }
    if (probe == NDS_ENTRY_P4_ADMIT_PROBES)
    {
        way = home;
    }
    if (dl != NULL)
    {
        sNdsEntryP4AdmitMemoDl[way] = dl;
        sNdsEntryP4AdmitMemoRow[way] = NDS_ENTRY_P4_ADMIT_NONE;
    }
    for (p = 0u; p < NDS_ENTRY_EFFECT_P4_ROOT_COUNT; p++)
    {
        const NDSEntryEffectP4Root *row = &sNdsEntryEffectP4Roots[p];
        const u8 *file = (row->storage != NULL) ?
            (const u8 *)*row->storage : NULL;
        u32 offset =
            sNdsEntryEffectRoots[NDS_ENTRY_EFFECT_P4_ROOT_FIRST + p].source_offset;
        const u8 *root;

        if (file == NULL)
        {
            continue;
        }
        /* A root in a file the special file depends on is the list the
         * special file's pointer names (Sheik's needle graphic), or, with
         * material bit 2, the place that pointer names less via_bias plus the
         * offset (Crash's dig effect file base, Peach's turnip tree). */
        if (row->via != 0u)
        {
            root = *(const u8 *const *)(file + row->via - 1u);
            if ((row->material_flags & 4u) != 0u)
            {
                if (root == NULL)
                {
                    continue;
                }
                root += offset - (u32)row->via_bias;
            }
        }
        else root = file + offset;

        if ((const u8 *)dl == root)
        {
            if (dl != NULL)
            {
                sNdsEntryP4AdmitMemoRow[way] = (u8)p;
            }
            ndsRendererEntryEffectP4AdmitRow(p, owner_asset_id, root_offset,
                                             variant_count);
            return TRUE;
        }
    }
    return FALSE;
}

/* Fighter renderer: a content's model part drawn beside the body (Wolf's
 * gun, joint 17), whose list lives outside the content's model file. A
 * Fox-parent content asks for every draw of every fighter; the answer is a
 * property of the generated table, so it is memoized per content and joint
 * (perf, 2026-10-18: four Palutenas, who have none, scanned every root at
 * 46K cycles a frame). Index 0 of the memo: not looked up yet. */
#define NDS_ENTRY_P4_SIDECAR_MEMO_CONTENTS 64u
static u16 sNdsEntryP4SidecarMemo[2][NDS_ENTRY_P4_SIDECAR_MEMO_CONTENTS];

static u32 ndsRendererEntryEffectP4SidecarScan(u32 content, u32 joint)
{
    u32 p;

    for (p = 0u; p < NDS_ENTRY_EFFECT_P4_ROOT_COUNT; p++)
    {
        const NDSEntryEffectP4Root *row = &sNdsEntryEffectP4Roots[p];

        if (((u32)row->content == content) && ((u32)row->sidecar_joint == joint))
        {
            return p;
        }
    }
    return NDS_ENTRY_EFFECT_P4_ROOT_COUNT;
}

sb32 ndsRendererEntryEffectP4Sidecar(u32 content, u32 joint,
                                     u32 *owner_asset_id, u32 *root_offset)
{
    u32 way = ((joint & 0x80u) != 0u) ? 1u : 0u;
    u32 p;

    if ((content < NDS_ENTRY_P4_SIDECAR_MEMO_CONTENTS) && ((joint & 0x7Fu) == 17u))
    {
        u16 *memo = &sNdsEntryP4SidecarMemo[way][content];

        if (*memo == 0u)
        {
            *memo = (u16)(ndsRendererEntryEffectP4SidecarScan(content, joint) + 1u);
        }
        p = (u32)*memo - 1u;
    }
    else
    {
        p = ndsRendererEntryEffectP4SidecarScan(content, joint);
    }
    if (p >= NDS_ENTRY_EFFECT_P4_ROOT_COUNT)
    {
        return FALSE;
    }
    *owner_asset_id = sNdsEntryEffectP4Roots[p].asset_id;
    *root_offset = sNdsEntryEffectRoots[NDS_ENTRY_EFFECT_P4_ROOT_FIRST + p].source_offset;
    return TRUE;
}

/* ndsRendererEntryEffectRoot's memo of P4 roots by (asset, offset). */
#define NDS_ENTRY_ROOT_MEMO_BITS 7u
#define NDS_ENTRY_ROOT_MEMO_WAYS (1u << NDS_ENTRY_ROOT_MEMO_BITS)
#define NDS_ENTRY_ROOT_MEMO_PROBES 8u
_Static_assert(NDS_ENTRY_EFFECT_P4_ROOT_COUNT <= 256u,
               "the root memo keeps a P4 row in a byte");
#endif

static const NDSEntryEffectRoot *ndsRendererEntryEffectRoot(
    u32 owner_asset_id, u32 root_offset)
{
    /* CaptainSpecial2 has two disjoint generated lifetimes: the startup Flyer
     * roots in the main Captain range and the gameplay Falcon Kick appended at
     * the tail so every existing generated root ordinal stays stable. */
    if ((owner_asset_id == 350u) && (root_offset == 0x0a30u))
    {
        const NDSEntryEffectRoot *root =
            &sNdsEntryEffectRoots[NDS_ENTRY_EFFECT_FALCON_KICK_ROOT_FIRST];
        return (root->source_offset == root_offset) ? root : NULL;
    }
    if ((owner_asset_id == 333u) && (root_offset == 0x0760u))
    {
        const NDSEntryEffectRoot *root =
            &sNdsEntryEffectRoots[NDS_ENTRY_EFFECT_FALCON_PUNCH_ROOT_FIRST];
        return (root->source_offset == root_offset) ? root : NULL;
    }
    /* SamusSpecial2 has two disjoint generated lifetimes: the early entry-point
     * pair and Catch's grapple glow appended at the tail so every previously
     * accepted root ordinal stays stable. */
    if ((owner_asset_id == 349u) && (root_offset == 0x02e0u))
    {
        const NDSEntryEffectRoot *root =
            &sNdsEntryEffectRoots[NDS_ENTRY_EFFECT_SAMUS_GRAPPLE_ROOT_FIRST];
        return (root->source_offset == root_offset) ? root : NULL;
    }
    /* YoshiModel's egg, and one root serves two states. Yoshi's shield
     * (efmanager.c:490) and his egg-hatching intro (:1375) name the SAME
     * source DObj setup field, differing only in flags and a matrix row. Both
     * hide his whole body on purpose -- ftcommonguard1.c:391 /
     * ftcommonguard2.c:23 and efmanager.c:5441 each call
     * ftParamHideModelPartAll behind `fkind == nFTKindYoshi` -- so with no
     * owner here nothing drew at all and he simply disappeared. The ordinary
     * shield (asset 163) cannot cover him because those call sites are an
     * if/else on fkind. Appended at the tail like the three above so no
     * previously accepted generated ordinal moves. */
    if ((owner_asset_id == 338u) && (root_offset == 0xa860u))
    {
        const NDSEntryEffectRoot *root =
            &sNdsEntryEffectRoots[NDS_ENTRY_EFFECT_YOSHI_EGG_ROOT_FIRST];
        return (root->source_offset == root_offset) ? root : NULL;
    }
#ifdef NDS_ENTRY_EFFECT_P4_ROOT_COUNT
    /* P4 content article roots, appended after every original root; their
     * assets are Remix's own files, past every original id. Two contents
     * may name one file (Marth's and Roy's entry, 0x0DA7): the root of a
     * content in the match, whose corner block is loaded, else the first. */
    if (owner_asset_id >= NDS_ENTRY_EFFECT_P4_ASSET_MIN)
    {
        /* The answer by (asset, offset) for the heap generation, whose match
         * fixes the contents the scan prefers (2026-10-13: four Sonics'
         * entry planes paid 79K cycles a frame scanning the rows). Open
         * addressing that never evicts (P8, 2026-10-23: on 64 direct-mapped
         * ways Palutena's wings and light roots, drawn in one frame, shared
         * ways and rescanned the rows every draw): a key probes from its
         * home way to the first empty one, and a key that finds neither
         * within the probe bound is scanned unmemoized. */
        static u32 memo_key[NDS_ENTRY_ROOT_MEMO_WAYS];
        static u8 memo_row[NDS_ENTRY_ROOT_MEMO_WAYS];
        static u32 memo_gen;
        const u32 key = (owner_asset_id << 16) ^ root_offset;
        u32 way = (key * 2654435761u) >> (32u - NDS_ENTRY_ROOT_MEMO_BITS);
        u32 empty = NDS_ENTRY_ROOT_MEMO_WAYS;
        const NDSEntryEffectRoot *first = NULL;
        u32 probe;
        u32 p;

        if (memo_gen != gNdsTaskmanHeapGeneration + 1u)
        {
            memset(memo_key, 0, sizeof(memo_key));
            memo_gen = gNdsTaskmanHeapGeneration + 1u;
        }
        for (probe = 0u; probe < NDS_ENTRY_ROOT_MEMO_PROBES; probe++)
        {
            if (memo_key[way] == key + 1u)
            {
                return &sNdsEntryEffectRoots[NDS_ENTRY_EFFECT_P4_ROOT_FIRST +
                                             (u32)memo_row[way]];
            }
            if (memo_key[way] == 0u)
            {
                empty = way;
                break;
            }
            way = (way + 1u) & (NDS_ENTRY_ROOT_MEMO_WAYS - 1u);
        }
        for (p = 0u; p < NDS_ENTRY_EFFECT_P4_ROOT_COUNT; p++)
        {
            const NDSEntryEffectRoot *root =
                &sNdsEntryEffectRoots[NDS_ENTRY_EFFECT_P4_ROOT_FIRST + p];

            if ((sNdsEntryEffectP4Roots[p].asset_id == owner_asset_id) &&
                (root->source_offset == root_offset))
            {
#ifdef NDS_ENTRY_EFFECT_P4_CORNER_BLOCKS
                if (ndsRendererEntryEffectP4ContentInMatch(
                        sNdsEntryEffectP4Roots[p].content) == FALSE)
                {
                    if (first == NULL)
                    {
                        first = root;
                    }
                    continue;
                }
#endif
                first = root;
                break;
            }
        }
        if ((first != NULL) && (empty < NDS_ENTRY_ROOT_MEMO_WAYS))
        {
            memo_key[empty] = key + 1u;
            memo_row[empty] = (u8)(first - &sNdsEntryEffectRoots[
                                               NDS_ENTRY_EFFECT_P4_ROOT_FIRST]);
        }
        return first;
    }
#endif
    u32 first = (owner_asset_id == 356u) ? 0u :
                (owner_asset_id == 161u) ? NDS_ENTRY_EFFECT_FOX_ROOT_FIRST :
                (owner_asset_id == 355u) ? NDS_ENTRY_EFFECT_DONKEY_ROOT_FIRST :
                (owner_asset_id == 349u) ? NDS_ENTRY_EFFECT_SAMUS_ROOT_FIRST :
                (owner_asset_id == 350u) ? NDS_ENTRY_EFFECT_CAPTAIN_ROOT_FIRST :
                (owner_asset_id == 353u) ? NDS_ENTRY_EFFECT_LINK_ROOT_FIRST :
                (owner_asset_id == 324u) ?
                    NDS_ENTRY_EFFECT_LINK_SPIN_WEAPON_ROOT_FIRST :
                (owner_asset_id == 325u) ?
                    NDS_ENTRY_EFFECT_LINK_BOOMERANG_ROOT_FIRST :
                (owner_asset_id == 163u) ? NDS_ENTRY_EFFECT_SHIELD_ROOT_FIRST :
                (owner_asset_id == 346u) ? NDS_ENTRY_EFFECT_REFLECTOR_ROOT_FIRST :
                (owner_asset_id == 84u) ? NDS_ENTRY_EFFECT_CATCH_ROOT_FIRST :
                (owner_asset_id == 85u) ?
                    NDS_ENTRY_EFFECT_MBALLRAYS_ROOT_FIRST :
                (owner_asset_id == 348u) ?
                    NDS_ENTRY_EFFECT_KIRBY_CUTTER_ROOT_FIRST :
                (owner_asset_id == 328u) ?
                    NDS_ENTRY_EFFECT_KIRBY_CUTTER_WEAPON_ROOT_FIRST :
                                           NDS_ENTRY_EFFECT_ROOT_COUNT;
    u32 last = (owner_asset_id == 356u) ? NDS_ENTRY_EFFECT_MARIO_ROOT_COUNT :
               (owner_asset_id == 161u) ? NDS_ENTRY_EFFECT_DONKEY_ROOT_FIRST :
               (owner_asset_id == 355u) ? NDS_ENTRY_EFFECT_SAMUS_ROOT_FIRST :
               (owner_asset_id == 349u) ? NDS_ENTRY_EFFECT_CAPTAIN_ROOT_FIRST :
               (owner_asset_id == 350u) ? NDS_ENTRY_EFFECT_LINK_ROOT_FIRST :
               (owner_asset_id == 353u) ?
                   NDS_ENTRY_EFFECT_LINK_SPIN_WEAPON_ROOT_FIRST :
               (owner_asset_id == 324u) ?
                   NDS_ENTRY_EFFECT_LINK_BOOMERANG_ROOT_FIRST :
               (owner_asset_id == 325u) ? NDS_ENTRY_EFFECT_SHIELD_ROOT_FIRST :
               (owner_asset_id == 163u) ? NDS_ENTRY_EFFECT_REFLECTOR_ROOT_FIRST :
               (owner_asset_id == 346u) ? NDS_ENTRY_EFFECT_CATCH_ROOT_FIRST :
               (owner_asset_id == 84u) ?
                   NDS_ENTRY_EFFECT_MBALLRAYS_ROOT_FIRST :
               (owner_asset_id == 85u) ?
                   NDS_ENTRY_EFFECT_KIRBY_CUTTER_ROOT_FIRST :
               (owner_asset_id == 348u) ?
                   NDS_ENTRY_EFFECT_KIRBY_CUTTER_WEAPON_ROOT_FIRST :
               /* The tail after KirbyModel's range is the explicit roots
                * above (and P4's): never scanned by offset. */
               (owner_asset_id == 328u) ?
                   NDS_ENTRY_EFFECT_SAMUS_GRAPPLE_ROOT_FIRST : first;
    u32 i;

    for (i = first; i < last; i++)
    {
        if (sNdsEntryEffectRoots[i].source_offset == root_offset)
        {
            return &sNdsEntryEffectRoots[i];
        }
    }
    return NULL;
}


/* Rehydrate one exact decoded source corner from the compact immutable packet.
 * No numeric conversion happens here: every dictionary entry is the same s16
 * or u8 value the old flat NDSRendererInputVertex table stored.  Keeping this
 * reconstruction at the native owner preserves all downstream lighting,
 * projection and GX submission code while avoiding ~10 KiB of repeated entry
 * data in ARM9 RAM. */
/* The corner rows the root being drawn reads (ndsRendererEntryEffectCornersSelect):
 * the packet's own for an original root, and for a P4 root its content's
 * block, loaded per match (ndsRendererEntryP4CornersLoad) and based so the
 * group's global corner index lands in it. The P4 rows left .main: 31 KB for
 * the eighteen contents, which every scene's heap paid for. */
static const u16 *sNdsEntryCornerPosition = sNdsEntryEffectCornerPosition;
static const NDS_ENTRY_EFFECT_UV_INDEX_T *sNdsEntryCornerS = sNdsEntryEffectCornerS;
static const NDS_ENTRY_EFFECT_UV_INDEX_T *sNdsEntryCornerT = sNdsEntryEffectCornerT;
static const u16 *sNdsEntryCornerColor = sNdsEntryEffectCornerColor;

#ifdef NDS_ENTRY_EFFECT_P4_CORNER_BLOCKS
static const u16 *sNdsEntryP4CornerData[NDS_ENTRY_EFFECT_P4_CONTENT_SLOTS];
static u32 sNdsEntryP4CornerScene = 0xffffffffu;
/* A P4 root whose content's block is not loaded this scene: counted. */
__attribute__((used)) volatile u32 gNdsEntryP4CornerMisses;
static sb32 ndsRendererEntryEffectP4ContentInMatch(u32 content);
#endif

static s32 ndsRendererEntryEffectCornersSelect(u32 root_index)
{
#ifdef NDS_ENTRY_EFFECT_P4_CORNER_BLOCKS
    if (root_index >= NDS_ENTRY_EFFECT_P4_ROOT_FIRST)
    {
        u32 content = sNdsEntryEffectP4Roots[root_index - NDS_ENTRY_EFFECT_P4_ROOT_FIRST].content;
        const NDSEntryEffectP4CornerBlock *block;
        const u16 *data;

        if (content >= NDS_ENTRY_EFFECT_P4_CONTENT_SLOTS)
        {
            return FALSE;
        }
        block = &sNdsEntryEffectP4CornerBlocks[content];
        data = sNdsEntryP4CornerData[content];
        if (block->count != 0u)
        {
            if ((data == NULL) ||
                (sNdsEntryP4CornerScene != gNdsRendererSceneTextureVramResetCount))
            {
                gNdsEntryP4CornerMisses++;
                return FALSE;
            }
            sNdsEntryCornerPosition = data - block->first;
            sNdsEntryCornerS = data + block->count - block->first;
            sNdsEntryCornerT = data + 2u * block->count - block->first;
            sNdsEntryCornerColor = data + 3u * block->count - block->first;
            return TRUE;
        }
    }
#endif
    (void)root_index;
    sNdsEntryCornerPosition = sNdsEntryEffectCornerPosition;
    sNdsEntryCornerS = sNdsEntryEffectCornerS;
    sNdsEntryCornerT = sNdsEntryEffectCornerT;
    sNdsEntryCornerColor = sNdsEntryEffectCornerColor;
    return TRUE;
}

#ifdef NDS_ENTRY_EFFECT_P4_CORNER_BLOCKS
/* The heap a corner block must leave for the setup's allocations after it
 * (the fighters' thread stacks, 4,216 B each). */
#define NDS_ENTRY_P4_CORNERS_KEEP_FREE (8u * 1024u)
/* At battle setup, before any entry draws: the blocks of the contents in the
 * match from nitro:/p4/entry_corners.bin into this scene's heap. A block that
 * does not fit or read stays out; its roots then fail closed (counted). */
NDS_RENDERER_COLD static void __attribute__((noinline, cold, optimize("Os")))
NDS_RENDERER_COLD ndsRendererEntryP4CornersLoad(void)
{
    FILE *file = NULL;
    u32 content;

    for (content = 0u; content < NDS_ENTRY_EFFECT_P4_CONTENT_SLOTS; content++)
    {
        const NDSEntryEffectP4CornerBlock *block = &sNdsEntryEffectP4CornerBlocks[content];
        u32 bytes = (u32)block->count * 8u;
        u16 *data;

        sNdsEntryP4CornerData[content] = NULL;
        if ((block->count == 0u) ||
            (ndsRendererEntryEffectP4ContentInMatch(content) == FALSE))
        {
            continue;
        }
        /* A VS battle's idle overlay tail first, then the heap if it fits with
         * room left for the setup's last allocations (the fighters' thread
         * stacks): four heavy contents on Congo Jungle took the heap's last
         * bytes here and the next thread stack halted (2026-10-18). A block
         * that would take that room stays out: its roots draw nothing. */
        data = ndsVSBattleOverlayTryAlloc(bytes, 4u);
        if ((data == NULL) &&
            (ndsSyMallocWouldFit(&gSYTaskmanGeneralHeap,
                                 bytes + NDS_ENTRY_P4_CORNERS_KEEP_FREE, 4u) == FALSE))
        {
            continue;
        }
        if ((file == NULL) &&
            ((file = ndsRendererHardwareFencedTextureFopen(
                  "nitro:/p4/entry_corners.bin", "rb")) == NULL))
        {
            break;
        }
        if (data == NULL)
        {
            data = syTaskmanMalloc(bytes, 4u);
        }
        if ((data != NULL) &&
            (ndsRendererHardwareFencedTextureFseek(file, (long)block->offset, SEEK_SET) == 0) &&
            (ndsRendererHardwareFencedTextureFread(data, 1u, bytes, file) == bytes))
        {
            sNdsEntryP4CornerData[content] = data;
        }
    }
    if (file != NULL)
    {
        (void)ndsRendererHardwareFencedTextureFclose(file);
    }
    sNdsEntryP4CornerScene = gNdsRendererSceneTextureVramResetCount;
}
#endif

#if NDS_ENTRY_P4_RETIRE
void ndsBattleDeadSpanAdd(void *base, u32 bytes);
u32 ndsSceneIsVSBattle(void);
__attribute__((used)) volatile u32 gNdsEntryP4RetiredCornerBytes;

/* Retire the entry roots of every content in the match whose corners are
 * loaded -- with `storage`, only a content all of whose entry roots live in
 * that special file -- and lend their corner runs (each run's span in the
 * position, s, t and colour rows). Each content once a scene. */
NDS_RENDERER_COLD void ndsRendererEntryP4RetireEntries(void **storage)
{
    static u32 retired_contents[(NDS_ENTRY_EFFECT_P4_CONTENT_SLOTS + 31u) / 32u];
    u32 content;

    if ((ndsSceneIsVSBattle() == FALSE) ||
        (sNdsEntryP4CornerScene != gNdsRendererSceneTextureVramResetCount))
    {
        return;
    }
    if (sNdsEntryP4RetiredScene != gNdsRendererSceneTextureVramResetCount)
    {
        memset(sNdsEntryP4Retired, 0, sizeof(sNdsEntryP4Retired));
        memset(retired_contents, 0, sizeof(retired_contents));
        sNdsEntryP4RetiredScene = gNdsRendererSceneTextureVramResetCount;
    }
    for (content = 0u; content < NDS_ENTRY_EFFECT_P4_CONTENT_SLOTS; content++)
    {
        const NDSEntryEffectP4CornerBlock *block = &sNdsEntryEffectP4CornerBlocks[content];
        u16 *data = (u16 *)(uintptr_t)sNdsEntryP4CornerData[content];
        sb32 any = FALSE;
        sb32 all = TRUE;
        u32 p;
        u32 r;

        if ((data == NULL) || (block->count == 0u) ||
            (((retired_contents[content >> 5] >> (content & 31u)) & 1u) != 0u))
        {
            continue;
        }
        for (p = 0u; p < NDS_ENTRY_EFFECT_P4_ROOT_COUNT; p++)
        {
            if (((u32)sNdsEntryEffectP4Roots[p].content == content) &&
                (ndsRendererEntryP4IsEntry(p) != FALSE))
            {
                any = TRUE;
                if ((storage != NULL) && (sNdsEntryEffectP4Roots[p].storage != storage))
                {
                    all = FALSE;
                }
            }
        }
        if ((any == FALSE) || (all == FALSE))
        {
            continue;
        }
        for (p = 0u; p < NDS_ENTRY_EFFECT_P4_ROOT_COUNT; p++)
        {
            if (((u32)sNdsEntryEffectP4Roots[p].content == content) &&
                (ndsRendererEntryP4IsEntry(p) != FALSE))
            {
                sNdsEntryP4Retired[p >> 5] |= 1u << (p & 31u);
            }
        }
        retired_contents[content >> 5] |= 1u << (content & 31u);
        sNdsEntryP4RetireCount++;
        for (r = 0u; r < NDS_ENTRY_EFFECT_P4_ENTRY_RUN_COUNT; r++)
        {
            const NDSEntryEffectP4EntryRun *run = &sNdsEntryEffectP4EntryRuns[r];
            u32 k;

            if ((u32)run->content != content)
            {
                continue;
            }
            for (k = 0u; k < 4u; k++)
            {
                ndsBattleDeadSpanAdd(data + k * (u32)block->count + run->first,
                                     (u32)run->count * 2u);
                gNdsEntryP4RetiredCornerBytes += (u32)run->count * 2u;
            }
        }
    }
}
#endif

static s32 ndsRendererEntryEffectVertex(u32 corner,
                                        NDSRendererInputVertex *out)
{
    u32 position_index;
    u32 s_index;
    u32 t_index;
    u32 color_index;
    const NDSEntryEffectPosition *position;
    const NDSEntryEffectColor *color;

    if ((out == NULL) || (corner >= NDS_ENTRY_EFFECT_VERTEX_COUNT))
    {
        return FALSE;
    }
    position_index = sNdsEntryCornerPosition[corner];
    s_index = sNdsEntryCornerS[corner];
    t_index = sNdsEntryCornerT[corner];
    color_index = sNdsEntryCornerColor[corner];
    if ((position_index >= NDS_ENTRY_EFFECT_POSITION_COUNT) ||
        (s_index >= NDS_ENTRY_EFFECT_S_COUNT) ||
        (t_index >= NDS_ENTRY_EFFECT_T_COUNT) ||
        (color_index >= NDS_ENTRY_EFFECT_COLOR_COUNT))
    {
        return FALSE;
    }
    position = &sNdsEntryEffectPositions[position_index];
    color = &sNdsEntryEffectColors[color_index];
    out->x = position->x;
    out->y = position->y;
    out->z = position->z;
    out->s = sNdsEntryEffectS[s_index];
    out->t = sNdsEntryEffectT[t_index];
    out->r = color->r;
    out->g = color->g;
    out->b = color->b;
    out->a = color->a;
    return TRUE;
}

/* P2-2p8 (2026-09-29): the generated tables are const, so the per-corner
 * index checks the submit below runs before its first GX write prove the same
 * thing on every draw. Sector Z's own Arwing draws eight FoxSpecial3 roots
 * (~330 corners) on about half the match's frames, and that fence walked
 * every corner of every root each time. A root's static half is now proven
 * once and remembered here; the checks that depend on live state (materials,
 * texture names, the traversal's modelview mask) still run on every draw. */
static u32 sNdsEntryEffectRootStaticOk[
    (NDS_ENTRY_EFFECT_ROOT_COUNT + 31u) / 32u];
/* Set with the root's static bit when every corner's position also converts
 * without ndsRendererEntryEffectCoord's clamp (the fast corners' condition). */
static u32 sNdsEntryEffectRootCoordsFit[
    (NDS_ENTRY_EFFECT_ROOT_COUNT + 31u) / 32u];

static inline __attribute__((always_inline)) u32
ndsRendererEntryEffectPositionFits(const NDSEntryEffectPosition *position)
{
    const s32 limit = 32767 >> (12u - NDS_RENDERER_HW_WORLD_UNIT_SHIFT);

    return ((position->x <= limit) && (position->x >= -limit - 1) &&
            (position->y <= limit) && (position->y >= -limit - 1) &&
            (position->z <= limit) && (position->z >= -limit - 1)) ? 1u : 0u;
}

/* ndsRendererEntryEffectVertex for a corner of a proven root. */
static inline __attribute__((always_inline)) void
ndsRendererEntryEffectVertexProven(u32 corner, NDSRendererInputVertex *out)
{
    const NDSEntryEffectPosition *position =
        &sNdsEntryEffectPositions[sNdsEntryCornerPosition[corner]];
    const NDSEntryEffectColor *color =
        &sNdsEntryEffectColors[sNdsEntryCornerColor[corner]];

    out->x = position->x;
    out->y = position->y;
    out->z = position->z;
    out->s = sNdsEntryEffectS[sNdsEntryCornerS[corner]];
    out->t = sNdsEntryEffectT[sNdsEntryCornerT[corner]];
    out->r = color->r;
    out->g = color->g;
    out->b = color->b;
    out->a = color->a;
}

/* ndsRendererHardwareVertexCoord(value, TRUE), inlined into the corner loop
 * (the out-of-line copy was a call per coordinate, three per corner). */
static inline __attribute__((always_inline)) v16
ndsRendererEntryEffectCoord(s16 value)
{
    const u32 shift = 12u - NDS_RENDERER_HW_WORLD_UNIT_SHIFT;
    s32 scaled = (s32)value << shift;

    if (scaled > 32767)
    {
        return (v16)32767;
    }
    if (scaled < -32768)
    {
        return (v16)-32768;
    }
    return (v16)scaled;
}

/* P2-2p8 (2026-09-29): entry-effect groups lit by the geometry engine.
 *
 * A lit group shaded every corner on the CPU
 * (ndsRendererHardwareLitShadeColorPrepared) against a light direction the
 * submit re-derived with soft float for every group. Sector Z's Arwing body is
 * twenty such groups, ~300 corners, on about half the match's frames. The
 * fighters moved the same formula onto light 0 (R2-03 E16); this does the same
 * for a group the engine transforms itself:
 *
 *  - the direction is prepared once per root: model space, normalised to 127,
 *    the vector the CPU dot used;
 *  - it is stored as light 0 while the vector matrix is identity, and the
 *    root's modelview then goes to the position matrix alone, so the engine
 *    dots the untransformed source normal with that same vector and the
 *    modelview's scale never enters;
 *  - diffuse/ambient are the two source light colours at five bits, the
 *    fighters' no-material mapping (ndsRendererMaterialChannel);
 *  - each corner sends its normal (GFX_NORMAL) instead of a colour.
 *
 * Not exact: the engine's five-bit light arithmetic differs from the CPU's
 * eight-bit clamp by a step or two of 31, the approximation every fighter
 * already draws with. CPU-projected groups, ramp palettes and material-folded
 * (PRIM x SHADE) groups keep the CPU shade. The direction is prepared once
 * per root. (The per-group direction and CPU-shade arms and their A/B word
 * left 2026-10-18.) */
u32 gNdsEntryEffectHwLitGroups;
u32 gNdsEntryEffectCpuLitGroups;

/* P2-2p8 (2026-09-29): the corners of a proven group whose colour is not per
 * corner -- an engine-lit group (a normal per corner) or a ramp-palette group
 * (one constant colour) -- straight from the const tables. The general loop
 * spent ~141 instructions a corner on Sector Z's Arwing (the table walk
 * spilled to the stack, each corner round-tripped an NDSRendererInputVertex
 * through memory, and the normal took three divides); this one reads each
 * table once and writes the four FIFO words. Same words. */
u32 gNdsEntryEffectFastCornerGroups;
/* Defined after NDS_NORMAL_PACK, below. */
static u32 ndsRendererEntryEffectFastCornersReady(void);
static void ndsRendererEntryEffectEmitFastCorners(u32 first, u32 count,
                                                  u32 lit, u32 textured);
static void ndsRendererEntryEffectEmitShadedCorners(u32 first, u32 count,
                                                    u32 material_color,
                                                    s32 use_material_color,
                                                    u32 textured);

/* P2-2p8 (2026-09-29): FoxSpecial3's resolved group state, kept across draws.
 *
 * What a group resolves before its corners -- its alpha, the combine's colour
 * source, the polygon format, the tile's texture parameters, the engine-light
 * decision and its diffuse/ambient word -- is a function of the generated
 * tables and of the state the root inherits at submit: the initial prim, env
 * and othermode, the geometry mode, the combine count (the polygon ID), the
 * light direction's presence and, only where a lit group reads colours no
 * earlier group of the root wrote, the seeded light colours. The same
 * FoxSpecial3 roots draw Fox's entry Arwing and Sector Z's, eight roots and 27
 * groups on about half of a Sector Z match's frames, and that resolution was
 * ~2,500 cycles a group (instruction-cache misses across ~15 helpers).
 *
 * A root whose every drawn group takes the fast corners records those values
 * on one ordinary draw, and replays them while its inherited state is
 * unchanged. What names the live texture system holds is still derived on
 * every draw: the texture name, the bind, the merged parameter word and the
 * ramp palette. Same GX words. */
u32 gNdsEntryEffectStateRecords;
u32 gNdsEntryEffectStateReplays;

#define NDS_ENTRY_STATE_ROOTS 8u
#define NDS_ENTRY_STATE_GROUPS 32u
#define NDS_ENTRY_STATE_SKIP 0x01u
#define NDS_ENTRY_STATE_LIT 0x02u
#define NDS_ENTRY_STATE_HW_LIT 0x04u
#define NDS_ENTRY_STATE_RAMP 0x08u
/* An unlit group that is no ramp: its corners take a colour (P8, 2026-10-23:
 * Palutena's glow, light, gate and the wing roots no cross-matrix corner
 * bends drew through the per-corner CPU loop). Without VCOLOR, dif_amb is the
 * group's one packed colour; with it, every corner's own vertex colour, times
 * dif_amb (the material colour) with MCOLOR. */
#define NDS_ENTRY_STATE_SHADE 0x10u
#define NDS_ENTRY_STATE_VCOLOR 0x20u
#define NDS_ENTRY_STATE_MCOLOR 0x40u

typedef struct NDSEntryEffectGroupState
{
    u32 poly_fmt;
    u32 texture_params; /* before the merge with the bound texture's word */
    u32 dif_amb;        /* or a SHADE group's colour word */
    u32 othermode_l;    /* what the batch's alpha-test state reads */
    u32 flags;
} NDSEntryEffectGroupState;

typedef struct NDSEntryEffectStateRoot
{
    u32 valid;          /* 0 = none, 1 = recording, 2 = replayable */
    u32 light_seed;     /* the seeded light colours are part of the key */
    u32 prim_color;
    u32 env_color;
    u32 othermode_h;
    u32 othermode_l;
    u32 geometry_mode;
    u32 combine_count;
    u32 light_dir_mask;
    u32 light_color_1;
    u32 light_color_2;
    u32 light_color_mask;
    u32 words;          /* ndsRendererEntryEffectStateWords at the record */
} NDSEntryEffectStateRoot;

static NDSEntryEffectStateRoot sNdsEntryEffectStateRoots[NDS_ENTRY_STATE_ROOTS];

/* P2-2p8 (2026-09-29): a root's composed (modelview x projection) CPU matrix
 * is read only by CPU-projected corners -- its own no-Z or cross-matrix
 * groups, or a later root's cross-matrix corners that name it. The roots such
 * corners name are fixed by the generated override table; any other root
 * composes on its first CPU-projected group, and most never have one (Sector
 * Z's Arwing: eight roots a frame). */
static u32 sNdsEntryEffectOverrideSourceMask[
    (NDS_ENTRY_EFFECT_ROOT_COUNT + 31u) / 32u];
static u8 sNdsEntryEffectOverrideSourceReady;
/* Each root's matrix slot: 0 for a root no corner names (its matrices serve
 * its own draw), 1.. for an override source, 0xff for a source past the
 * slots (its draw and every corner naming it reject). */
static u8 sNdsEntryEffectMatrixSlot[NDS_ENTRY_EFFECT_ROOT_COUNT];
#define NDS_ENTRY_EFFECT_MATRIX_SLOT_NONE 0xffu
__attribute__((used)) volatile u32 gNdsEntryEffectMatrixSlotOverflow;

NDS_RENDERER_COLD static void ndsRendererEntryEffectOverrideSourcesInit(void)
{
    u32 next = 1u;
    u32 i;

    for (i = 0u; i < NDS_ENTRY_EFFECT_CROSS_MATRIX_CORNER_COUNT; i++)
    {
        u32 source = sNdsEntryEffectMatrixOverrideRoot[i];

        if ((source < NDS_ENTRY_EFFECT_ROOT_COUNT) &&
            ((sNdsEntryEffectOverrideSourceMask[source >> 5] &
              (1u << (source & 31u))) == 0u))
        {
            sNdsEntryEffectOverrideSourceMask[source >> 5] |=
                1u << (source & 31u);
            if (next < NDS_ENTRY_EFFECT_MATRIX_SLOTS)
            {
                sNdsEntryEffectMatrixSlot[source] = (u8)next++;
            }
            else
            {
                sNdsEntryEffectMatrixSlot[source] =
                    (u8)NDS_ENTRY_EFFECT_MATRIX_SLOT_NONE;
                gNdsEntryEffectMatrixSlotOverflow++;
            }
        }
    }
    sNdsEntryEffectOverrideSourceReady = 1u;
}

static u32 ndsRendererEntryEffectIsOverrideSource(u32 root_index)
{
    if (sNdsEntryEffectOverrideSourceReady == 0u)
    {
        ndsRendererEntryEffectOverrideSourcesInit();
    }
    return ((sNdsEntryEffectOverrideSourceMask[root_index >> 5] &
             (1u << (root_index & 31u))) != 0u) ? 1u : 0u;
}

static u32 ndsRendererEntryEffectMatrixSlot(u32 root_index)
{
    if (sNdsEntryEffectOverrideSourceReady == 0u)
    {
        ndsRendererEntryEffectOverrideSourcesInit();
    }
    return (u32)sNdsEntryEffectMatrixSlot[root_index];
}
static NDSEntryEffectGroupState
    sNdsEntryEffectStateGroups[NDS_ENTRY_STATE_GROUPS];

#ifdef NDS_ENTRY_EFFECT_P4_ROOT_COUNT
/* P4 (2026-10-13): the contents' roots take the state cache too. Four
 * Sonics' entry planes (fifteen roots, 23 groups a plane) re-resolved every
 * group's state on every draw -- the ~2,500-cycle resolution above, ~90 groups
 * a frame for two seconds of entry. A content's root takes a slot and its
 * groups' range on its first cacheable draw, per heap generation and again
 * after an entry retires (the entry roots had kept the slots all match); the
 * packet replay stays the Arwing's (its arena is Sector Z's). */
#define NDS_ENTRY_P4_STATE_ROOTS 40u
#define NDS_ENTRY_P4_STATE_GROUPS 96u
#define NDS_ENTRY_P4_STATE_NONE 0xffu
static NDSEntryEffectStateRoot sNdsEntryP4StateRoots[NDS_ENTRY_P4_STATE_ROOTS];
static NDSEntryEffectGroupState
    sNdsEntryP4StateGroups[NDS_ENTRY_P4_STATE_GROUPS];
static u8 sNdsEntryP4StateSlot[NDS_ENTRY_EFFECT_P4_ROOT_COUNT];
static u8 sNdsEntryP4StateFirst[NDS_ENTRY_P4_STATE_ROOTS];
static u32 sNdsEntryP4StateSlots;
static u32 sNdsEntryP4StateGroupsUsed;
static u32 sNdsEntryP4StateGen;
static u32 sNdsEntryP4StateRetires;

static sb32 ndsRendererEntryEffectP4State(
    u32 root_index, const NDSEntryEffectRoot *root,
    NDSEntryEffectStateRoot **state_root,
    NDSEntryEffectGroupState **state_groups)
{
    const u32 p = root_index - NDS_ENTRY_EFFECT_P4_ROOT_FIRST;
#if NDS_ENTRY_P4_RETIRE
    const u32 retires = sNdsEntryP4RetireCount;
#else
    const u32 retires = 0u;
#endif
    u32 slot;

    if ((sNdsEntryP4StateGen != gNdsTaskmanHeapGeneration + 1u) ||
        (sNdsEntryP4StateRetires != retires))
    {
        memset(sNdsEntryP4StateSlot, NDS_ENTRY_P4_STATE_NONE,
               sizeof(sNdsEntryP4StateSlot));
        sNdsEntryP4StateSlots = 0u;
        sNdsEntryP4StateGroupsUsed = 0u;
        sNdsEntryP4StateGen = gNdsTaskmanHeapGeneration + 1u;
        sNdsEntryP4StateRetires = retires;
    }
    if ((p >= NDS_ENTRY_EFFECT_P4_ROOT_COUNT) ||
        ((u32)root->group_count > NDS_ENTRY_STATE_GROUPS))
    {
        return FALSE;
    }
    slot = sNdsEntryP4StateSlot[p];
    if (slot == NDS_ENTRY_P4_STATE_NONE)
    {
        if ((sNdsEntryP4StateSlots >= NDS_ENTRY_P4_STATE_ROOTS) ||
            (sNdsEntryP4StateGroupsUsed + (u32)root->group_count >
             NDS_ENTRY_P4_STATE_GROUPS))
        {
            return FALSE;
        }
        slot = sNdsEntryP4StateSlots++;
        sNdsEntryP4StateFirst[slot] = (u8)sNdsEntryP4StateGroupsUsed;
        sNdsEntryP4StateGroupsUsed += (u32)root->group_count;
        sNdsEntryP4StateRoots[slot].valid = 0u;
        sNdsEntryP4StateSlot[p] = (u8)slot;
    }
    *state_root = &sNdsEntryP4StateRoots[slot];
    *state_groups = &sNdsEntryP4StateGroups[sNdsEntryP4StateFirst[slot]];
    return TRUE;
}
#endif

static inline __attribute__((always_inline)) u32
ndsRendererEntryEffectStateWords(void)
{
    /* The engine-light and fast-corner arms the recorded state assumes
     * (their A/B words' shipped values, 2 and 1). */
    return 2u | (1u << 8);
}

static u32 ndsRendererEntryEffectStateMatches(
    const NDSEntryEffectStateRoot *state, const NDSRendererStats *stats,
    u32 prim_color, u32 env_color, u32 othermode_h, u32 othermode_l,
    u32 geometry_mode)
{
    return ((state->valid == 2u) &&
            (state->prim_color == prim_color) &&
            (state->env_color == env_color) &&
            (state->othermode_h == othermode_h) &&
            (state->othermode_l == othermode_l) &&
            (state->geometry_mode == geometry_mode) &&
            (state->combine_count == stats->texture_combine_count) &&
            (state->light_dir_mask ==
             (stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK)) &&
            (state->words == ndsRendererEntryEffectStateWords()) &&
            ((state->light_seed == 0u) ||
             ((state->light_color_1 == stats->light_color_1) &&
              (state->light_color_2 == stats->light_color_2) &&
              (state->light_color_mask == stats->light_color_mask)))) ?
        1u : 0u;
}

#define NDS_ENTRY_EFFECT_LIGHT_COLORS \
    (NDS_RENDERER_LIGHT_COLOR_1_MASK | NDS_RENDERER_LIGHT_COLOR_2_MASK)

/* Defined after NDS_NORMAL_PACK, below. */
static inline u32 ndsRendererEntryEffectNormalWord(
    const NDSRendererInputVertex *vtx);
static inline u32 ndsRendererEntryEffectLightWord(
    const NDSRendererHardwareLightDirection *direction);

/* diffuse | ambient << 16 from the two source light colours; bit 15 clear, so
 * the write leaves the vertex colour alone. */
static inline u32 ndsRendererEntryEffectDiffuseAmbient(u32 light_1, u32 light_2)
{
    u32 diffuse = RGB15((light_1 >> 27) & 0x1fu, (light_1 >> 19) & 0x1fu,
                        (light_1 >> 11) & 0x1fu);
    u32 ambient = RGB15((light_2 >> 27) & 0x1fu, (light_2 >> 19) & 0x1fu,
                        (light_2 >> 11) & 0x1fu);

    return diffuse | (ambient << 16);
}

/* After the split load of the same modelview: light 0 is stored while the
 * position and vector matrices are both identity, then the modelview is
 * reloaded into the position matrix alone. The projection and the loader's
 * cache (this submit's generation) still describe what the engine holds. */
static void __attribute__((noinline)) ndsRendererEntryEffectLoadLitMatrices(
    const NDSRendererMatrix20p12 *modelview,
    const NDSRendererHardwareLightDirection *direction)
{
    ndsRendererHardwareEndBatch();
    ndsRendererHardwareFighterSetMatrixMode(GL_MODELVIEW);
    MATRIX_IDENTITY = 0;
    GFX_LIGHT_VECTOR = ndsRendererEntryEffectLightWord(direction);
    GFX_LIGHT_COLOR = (u32)RGB15(31, 31, 31);
    ndsRendererHardwareFighterSetMatrixMode(GL_POSITION);
    ndsRendererHardwareFighterLoadModelviewWorldScaled(modelview);
    ndsRendererHardwareFighterSetMatrixMode(GL_MODELVIEW);
}

/* P2-2p8 (2026-09-30): FoxSpecial3 roots replayed by DMA.
 *
 * A root the state cache replays writes the same GX commands on every draw:
 * per group the bound texture's TEXIMAGE_PARAM and PLTT_BASE, DIF_AMB,
 * POLYGON_ATTR, BEGIN, a COLOR, and the corners' NORMAL / TEX_COORD /
 * VERTEX16 from const tables. On Sector Z that loop is the Arwing's eight
 * roots on ~45% of a match's frames, and it ran at 16-90 cycles an
 * instruction: scattered table reads missing the 4 KB data cache, and GX FIFO
 * writes stalling on the geometry engine. The first replayed draw of a root
 * now also records those commands as a packed list (the fighter packet's
 * format); later draws send it by DMA, which reads memory without the CPU and
 * feeds the FIFO at the engine's own pace. The one per-draw input inside the
 * loop -- the lit-matrix load, whose light vector and position matrix come
 * from this draw's modelview -- stays a CPU write between two DMA segments,
 * at its place in the command order.
 *
 * Texture state is recorded as the state the polygons take, not as the calls
 * that set it: after the batch begins, libnds's bound object holds the merged
 * TEXIMAGE_PARAM the loop wrote and its palette's base (0 without one). What
 * those words bake stays true while the key holds: the state cache replayed
 * this draw, each group binds the same texture and ramp names, and since the
 * record no scene texture reset, entry texture prepare or release, or ramp
 * palette bake has happened. A ramp-eligible texture is drawn only through a
 * ramp (ndsRendererEntryRampSlotEligible), so no group reads a palette another
 * group assigned. The registers outside the FIFO that the loop's batches leave
 * behind -- texturing on, the last threshold group's alpha reference, fog,
 * alpha test off -- are set on the CPU. The arena exists only on Sector Z
 * (ndsGRSectorSetupInitAll); elsewhere Fox's entry Arwing is one fly-by.
 */
__attribute__((used)) volatile u32 gNdsEntryEffectPacketRecords;
__attribute__((used)) volatile u32 gNdsEntryEffectPacketReplays;
__attribute__((used)) volatile u32 gNdsEntryEffectPacketFaults;
__attribute__((used)) volatile u32 gNdsEntryEffectPacketArenaWords;

#define NDS_ENTRY_PACKET_COUNTERS 13u

typedef struct NDSEntryEffectPacket
{
    u32 *words;
    u32 capacity;
    u32 count;
    u32 split;              /* words sent before the lit-matrix load */
    u32 valid;
    u32 disabled;           /* a record faulted: the loop draws it this scene */
    u32 lit;                /* the loop loads the lit matrices */
    u32 batches;            /* groups drawn */
    u32 scene_generation;
    u32 ramp_bakes;
    u32 texture_prepares;
    u32 texture_releases;
    u32 alpha_valid;
    u32 alpha_othermode_l;  /* the last drawn group with a threshold compare */
    u32 last_othermode_l;   /* the last drawn group */
    /* What the loop adds to the stats and the owner's counters
     * (ndsEntryPacketCounters). */
    u32 delta[NDS_ENTRY_PACKET_COUNTERS];
} NDSEntryEffectPacket;

typedef struct NDSEntryEffectPacketRecorder
{
    NDSEntryEffectPacket *packet;
    u32 cmd_word;
    u32 cmd_slot;
    u32 fault;
    u32 before[NDS_ENTRY_PACKET_COUNTERS];
} NDSEntryEffectPacketRecorder;

static NDSEntryEffectPacket sNdsEntryEffectPackets[NDS_ENTRY_STATE_ROOTS];
/* Indexed like sNdsEntryEffectStateGroups: the names each group's words bake. */
static u32 sNdsEntryEffectPacketNames[NDS_ENTRY_STATE_GROUPS];
static u32 sNdsEntryEffectPacketRamps[NDS_ENTRY_STATE_GROUPS];
static u32 sNdsEntryEffectPacketArenaGeneration;

static NDSEntryEffectPacket *ndsRendererEntryEffectPacketFor(u32 slot);
static u32 ndsRendererEntryEffectPacketCurrent(
    const NDSEntryEffectPacket *packet, const NDSEntryEffectRoot *root,
    u32 first, const u32 *ramp_names);
static void ndsRendererEntryEffectPacketBegin(
    NDSEntryEffectPacketRecorder *rec, NDSEntryEffectPacket *packet,
    const NDSRendererStats *stats);
static void ndsEntryPacketCmd(NDSEntryEffectPacketRecorder *rec, u32 opcode,
                              u32 param_count, u32 p0, u32 p1);
static void ndsEntryPacketSplit(NDSEntryEffectPacketRecorder *rec);
static void ndsEntryPacketRecordGroup(NDSEntryEffectPacketRecorder *rec,
                                      const NDSEntryEffectGroup *group,
                                      const NDSEntryEffectGroupState *state,
                                      u32 textured, u32 texture_name,
                                      u32 hw_lit);
static void ndsRendererEntryEffectPacketEnd(
    NDSEntryEffectPacketRecorder *rec, const NDSEntryEffectRoot *root,
    u32 first, const u32 *ramp_names, const NDSRendererStats *stats);
static void __attribute__((noinline)) ndsRendererEntryEffectPacketReplay(
    const NDSEntryEffectPacket *packet, NDSRendererStats *stats,
    const NDSRendererMatrix20p12 *modelview,
    NDSRendererHardwareLightDirection *direction, u32 direction_ready);

/* DS-native landed entry-prop immutable presentation owner.
 *
 * Source ownership deliberately stops at the DObj: BattleShip continues to
 * animate/re-parent/sort the live tree, so the pipe rise and Arwing fly-by use
 * the original timing and transforms.  At each leaf, however, this function
 * consumes only build-generated DS vertices and resident DS textures.  No N64
 * command scan, vertex-cache emulation, texture/TLUT conversion, or software
 * lighting occurs here.  The generator also rejects a packet if any emitted
 * source triangle has G_LIGHTING set; the explicit mask below is a second
 * runtime fence against lighting state leaking from a previous fighter draw. */
__attribute__((used)) volatile u32 gNdsEntryEffectNativeNoZGroupDraws;
__attribute__((used)) volatile u32 gNdsEntryShieldWitnessPolyFmt;


static void ndsRendererEntryEffectApplyLiveMaterial(
    const NDSRendererNativeMaterial *material, NDSRendererStats *stats)
{
    u32 effects;

    if ((material == NULL) || (stats == NULL))
    {
        return;
    }
    effects = material->effects;

    /* The generated root already counts the source segment-E G_DL itself.
     * Replay only the typed branch body plus its generated branch slot here,
     * mirroring gcDrawMObjForDObj without executing a generic mini-DL. */
    stats->command_count += (u32)material->command_count + 1u;
    stats->branch_command_count += 2u;
    stats->branch_call_count++;
    stats->branch_jump_count++;
    stats->segment_resolve_count++;
    stats->end_command_count++;
    stats->sync_command_count += material->sync_count;

    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_LIGHT1) != 0u)
    {
        ndsRendererRecordLightColor(stats, 1u, material->light1);
        ndsRendererRecordLightColor(stats, 1u, material->light1);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_LIGHT2) != 0u)
    {
        ndsRendererRecordLightColor(stats, 2u, material->light2);
        ndsRendererRecordLightColor(stats, 2u, material->light2);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_PRIM) != 0u)
    {
        stats->prim_color = material->prim_w1;
        stats->prim_min_level = (material->prim_w0 >> 8) & 0xffu;
        stats->prim_lod_fraction = material->prim_w0 & 0xffu;
        stats->color_command_count++;
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_ENV) != 0u)
    {
        stats->env_color = material->env_color;
        stats->color_command_count++;
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_RENDER_TILE_SIZE) != 0u)
    {
        ndsRendererRecordSetTileSize(
            stats, material->render_tile_size_w0,
            material->render_tile_size_w1);
    }
    if ((effects & NDS_RENDERER_NATIVE_MATERIAL_TEXTURE) != 0u)
    {
        ndsRendererRecordTextureState(
            stats, material->texture_w0, material->texture_w1);
    }
}

static u32 ndsRendererEntryKoPalette(u32 part, u32 primitive, u32 environment)
{
    u32 player;
    if (part >= 3u) { return 0u; }
    primitive &= 0xffffff00u;
    environment &= 0xffffff00u;
    for (player = 0u; player < 4u; player++)
    {
        if ((primitive == sNdsEntryKoPrimRgb[part][player]) &&
            (environment == sNdsEntryKoEnvRgb[part][player]))
        {
            return sNdsEntryKoPaletteName[part][player];
        }
    }
    return 0u;
}

/* THE RAMP COMBINER, FOR EVERY ENTRY-EFFECT GROUP THAT USES IT.
 *
 * Two combine rows here -- 0xfc30fe61/0x55fef379 and 0xfc309661/0x552eff7f --
 * have the same colour half: (PRIM - ENV) * TEXEL0 + ENV. They differ only in
 * alpha (TEXEL0, or TEXEL0 * PRIM). Their textures are IA planes baked A5I3
 * over one shared eight-step GREY palette, and modulate can only multiply that
 * grey by a single colour, so the ENV end of the ramp was lost everywhere:
 * Link's Spin Attack (ENV 0xd00c03 on the effect, nine oranges 0xff4200..
 * 0xff7500 on the weapon) drew white, and so did Fox's, Captain's and Samus's
 * glows and the Poke Ball rays. KO stars and the shield already solved this
 * for themselves with per-colour palettes; this is the general form.
 *
 * A palette-only GL name holds ENV->PRIM blended through the texture's own
 * grey steps and is attached with glAssignColorTable, exactly like the KO
 * palettes -- no image copy, 16 or 64 bytes of palette RAM per entry. The
 * polygon is then white. Prim is live on several owners (a PRIM-only MObj), so
 * entries are keyed on the colours and replaced round-robin; names die with
 * glResetTextures and are only trusted inside the generation that made them.
 *
 * A slot is eligible only if EVERY group that samples it is a ramp group, so
 * an assigned palette can never leak into a modulate draw of the same image. */
#define NDS_ENTRY_RAMP_PALETTES 12u
#define NDS_ENTRY_RAMP_PALETTE_WIDTH 32u
typedef struct NDSEntryRampPalette
{
    const u16 *source;
    u32 name;
    u32 generation;
    u32 prim;
    u32 env;
    /* Deferred rewrites: the palette's LCD address (NULL: never rewritten in
     * place), the banks it spans, the last epoch that referenced it. */
    u32 *vram;
    u32 banks;
    u32 epoch;
} NDSEntryRampPalette;
static NDSEntryRampPalette sNdsEntryRampPalettes[NDS_ENTRY_RAMP_PALETTES];
static u32 sNdsEntryRampPaletteNext;
static u32 sNdsEntryRampStage[NDS_ENTRY_RAMP_PALETTES]
                             [NDS_ENTRY_RAMP_PALETTE_WIDTH / 2u];
static u32 sNdsEntryRampPending;
static u32 sNdsEntryRampPendingBanks;
/* One per presented frame; entries start at 0, never the current one. */
static u32 sNdsEntryRampEpoch = 1u;
volatile u32 gNdsEntryRampDeferred __attribute__((used, section(".data"))) = 1u;
static u8 sNdsEntryRampSlotState[NDS_ENTRY_EFFECT_TEXTURE_COUNT]; /* 0 unknown, 1 yes, 2 no */
__attribute__((used)) volatile u32 gNdsEntryRampPaletteBakes;
/* One root's resolved material, selectable from gdb: alpha, prim, env,
 * othermode_l as the draw saw them. Stack copies read stale on this emulator. */
__attribute__((used)) volatile u32 gNdsEntryEffectWitnessRoot = 0x0698u;
__attribute__((used)) volatile u32 gNdsEntryEffectWitness[4];
__attribute__((used)) volatile u32 gNdsEntryRampPaletteDraws;

static s32 ndsRendererEntryRampCombine(const NDSEntryEffectPairState *combine)
{
    return (((combine->a == 0xfc30fe61u) && (combine->b == 0x55fef379u)) ||
            ((combine->a == 0xfc309661u) && (combine->b == 0x552eff7fu))) ? TRUE : FALSE;
}

static s32 ndsRendererEntryRampSlotEligible(u32 texture_slot)
{
    u32 i;

    if (texture_slot >= NDS_ENTRY_EFFECT_TEXTURE_COUNT)
    {
        return FALSE;
    }
    if (sNdsEntryRampSlotState[texture_slot] == 0u)
    {
        const NDSEntryEffectTexture *texture = &sNdsEntryEffectTextures[texture_slot];
        u32 state = 1u;

        if ((texture->palette == NULL) ||
            !(((texture->ds_format == NDS_ENTRY_EFFECT_TEXTURE_A5I3) &&
               (texture->palette_entries == 8u)) ||
              ((texture->ds_format == NDS_ENTRY_EFFECT_TEXTURE_A3I5) &&
               (texture->palette_entries == 32u))))
        {
            state = 2u;
        }
        for (i = 0u; (i < NDS_ENTRY_EFFECT_GROUP_COUNT) && (state == 1u); i++)
        {
            const NDSEntryEffectGroup *group = &sNdsEntryEffectGroups[i];

            if ((group->texture_slot == texture_slot) &&
                ((group->combine_state >= NDS_ENTRY_EFFECT_COMBINE_STATE_COUNT) ||
                 (ndsRendererEntryRampCombine(
                      &sNdsEntryEffectCombineStates[group->combine_state]) == FALSE)))
            {
                state = 2u;
            }
        }
        sNdsEntryRampSlotState[texture_slot] = (u8)state;
    }
    return (sNdsEntryRampSlotState[texture_slot] == 1u) ? TRUE : FALSE;
}

/* Returns a palette-only GL name, or 0. May change the bound texture.
 *
 * P2-2p8 (2026-10-05): DEFERRED REWRITES, as for the KO pillar variants
 * (ndsRendererParticleEnvVariantCommit). A miss used to glColorTableEXT a live
 * name mid-frame -- libnds frees and reallocates the palette and maps banks
 * F/G to LCD while the 3D engine renders, and the entry being replaced could
 * still be on screen (Link's Spin Attack keys nine oranges at once). Now each
 * entry allocates a 32-entry palette once, a miss takes the least recently
 * used entry that no draw of the frame being built references, bakes into
 * that entry's staging copy, and ndsRendererEntryRampPaletteCommit copies it
 * in at the VBlank that swaps the frame in. Same-ROM A/B word
 * gNdsEntryRampDeferred (0 = the immediate round robin). */
static u32 ndsRendererEntryRampVictim(u32 generation)
{
    const u32 epoch = sNdsEntryRampEpoch;
    u32 best = NDS_ENTRY_RAMP_PALETTES;
    u32 best_age = 0u;
    u32 i;

    for (i = 0u; i < NDS_ENTRY_RAMP_PALETTES; i++)
    {
        const NDSEntryRampPalette *entry = &sNdsEntryRampPalettes[i];
        u32 age;

        if ((entry->name == 0u) || (entry->generation != generation))
        {
            return i;
        }
        if (entry->vram == NULL)
        {
            continue;
        }
        age = epoch - entry->epoch;
        if (age > best_age)
        {
            best_age = age;
            best = i;
        }
    }
    return best;
}

static u32 ndsRendererEntryRampPalette(const NDSEntryEffectTexture *texture,
                                       u32 primitive, u32 environment)
{
    u32 generation = gNdsRendererSceneTextureVramResetCount + 1u;
    NDSEntryRampPalette *entry;
    u32 deferred = gNdsEntryRampDeferred;
    u32 width = (u32)texture->palette_entries;
    u32 slot;
    u16 *bake;
    int palette_width = 0;
    int name;
    u32 i;

    primitive &= 0xffffff00u;
    environment &= 0xffffff00u;
    for (i = 0u; i < NDS_ENTRY_RAMP_PALETTES; i++)
    {
        entry = &sNdsEntryRampPalettes[i];
        if ((entry->name != 0u) && (entry->generation == generation) &&
            (entry->source == texture->palette) &&
            (entry->prim == primitive) && (entry->env == environment))
        {
            entry->epoch = sNdsEntryRampEpoch;
            return entry->name;
        }
    }
    if (deferred != 0u)
    {
        slot = ndsRendererEntryRampVictim(generation);
        if (slot >= NDS_ENTRY_RAMP_PALETTES)
        {
            return 0u;
        }
        width = NDS_ENTRY_RAMP_PALETTE_WIDTH;
    }
    else
    {
        slot = sNdsEntryRampPaletteNext;
        sNdsEntryRampPaletteNext++;
        if (sNdsEntryRampPaletteNext >= NDS_ENTRY_RAMP_PALETTES)
        {
            sNdsEntryRampPaletteNext = 0u;
        }
    }
    entry = &sNdsEntryRampPalettes[slot];
    if (entry->generation != generation)
    {
        entry->name = 0u; /* the name died with the scene reset */
        entry->vram = NULL;
        sNdsEntryRampPending &= ~(1u << slot);
    }
    bake = (u16 *)sNdsEntryRampStage[slot];
    for (i = 0u; i < texture->palette_entries; i++)
    {
        bake[i] = ndsRendererHardwareBlendPrimEnvTexel0(
            texture->palette[i], primitive | 0xffu, environment | 0xffu);
    }
    for (; i < NDS_ENTRY_RAMP_PALETTE_WIDTH; i++)
    {
        bake[i] = 0u;
    }
    if ((deferred != 0u) && (entry->name != 0u) && (entry->vram != NULL))
    {
        /* Queued: the palette changes at the VBlank that swaps this frame
         * in. No GL call. */
        sNdsEntryRampPending |= 1u << slot;
        sNdsEntryRampPendingBanks |= entry->banks;
        entry->source = texture->palette;
        entry->prim = primitive;
        entry->env = environment;
        entry->epoch = sNdsEntryRampEpoch;
        gNdsEntryRampPaletteBakes++;
        return entry->name;
    }
    name = (int)entry->name;
    ndsRendererHardwareEndBatch();
    if ((name == 0) && (ndsRendererHardwareFencedGlGenTextures(1, &name) == 0))
    {
        return 0u;
    }
    ndsRendererHardwareBindTextureState((u32)name);
    sNdsRendererHardwareBoundTextureName = (u32)name;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    glColorTableEXT(GL_TEXTURE_2D, 0, (int)width, 0, 0, bake);
    glGetColorTableParameterEXT(GL_TEXTURE_2D, GL_COLOR_TABLE_WIDTH_EXT,
                                &palette_width);
    if (palette_width != (int)width)
    {
        ndsRendererHardwareFencedGlDeleteTextures(1, &name);
        sNdsRendererHardwareBoundTextureName = 0u;
        entry->name = 0u;
        entry->vram = NULL;
        return 0u;
    }
    /* Where the palette lives, so later misses rewrite it in place. */
    entry->vram = NULL;
    entry->banks = 0u;
    if (deferred != 0u)
    {
        const gl_palette_data *palette = (const gl_palette_data *)
            DynamicArrayGet(&glGlob->palettePtrs,
                            (unsigned int)glGlob->activePalette);

        if ((palette != NULL) &&
            (((uintptr_t)palette->vramAddr & 3u) == 0u))
        {
            entry->banks = ndsRendererPaletteBanksFor(
                palette->vramAddr, NDS_ENTRY_RAMP_PALETTE_WIDTH * sizeof(u16));
            if (entry->banks != 0u)
            {
                entry->vram = (u32 *)palette->vramAddr;
            }
        }
    }
    entry->name = (u32)name;
    entry->generation = generation;
    entry->source = texture->palette;
    entry->prim = primitive;
    entry->env = environment;
    entry->epoch = sNdsEntryRampEpoch;
    gNdsEntryRampPaletteBakes++;
    return entry->name;
}

/* The swap VBlank (ndsRendererCommitDeferredPalettes): the queued ramp
 * palettes land, unless a scene reset retired the palettes they target. */
static void ndsRendererEntryRampPaletteCommit(void)
{
    u32 pending = sNdsEntryRampPending;
    u32 banks;
    u32 saved;

    sNdsEntryRampEpoch++;
    if (pending == 0u)
    {
        return;
    }
    banks = sNdsEntryRampPendingBanks;
    sNdsEntryRampPending = 0u;
    sNdsEntryRampPendingBanks = 0u;
    saved = ndsRendererPaletteBanksToLcd(banks);
    do
    {
        const u32 slot = (u32)__builtin_ctz(pending);
        const NDSEntryRampPalette *entry = &sNdsEntryRampPalettes[slot];
        const u32 *src = sNdsEntryRampStage[slot];
        u32 *dst = entry->vram;
        u32 i;

        pending &= pending - 1u;
        /* A scene reset since the queue retired this palette. */
        if ((dst == NULL) || (entry->name == 0u) ||
            (entry->generation !=
             gNdsRendererSceneTextureVramResetCount + 1u))
        {
            continue;
        }
        for (i = 0u; i < (NDS_ENTRY_RAMP_PALETTE_WIDTH / 2u); i++)
        {
            dst[i] = src[i];
        }
    } while (pending != 0u);
    ndsRendererPaletteBanksRestore(banks, saved);
}
/* Which refusal the entry-effect owner took (lab builds only: the stores
 * stay out of the gate ROM). A distinct constant store per site keeps GCC
 * from merging the tails. */
#if (defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP) || \
    (defined(NDS_LAB_ENTRY_REJECT_LINE) && NDS_LAB_ENTRY_REJECT_LINE)
__attribute__((used)) volatile u32 gNdsEntryEffectRejectLine;
__attribute__((used)) volatile u32 gNdsEntryEffectRejectRoot;
#define NDS_ENTRY_EFFECT_REJECT() \
    do { gNdsEntryEffectRejectLine = __LINE__; \
         gNdsEntryEffectRejectRoot = (owner_asset_id << 16) ^ root_offset; \
         return FALSE; } while (0)
#else
#define NDS_ENTRY_EFFECT_REJECT() return FALSE
#endif
/* The presented frame (renderer serial) that last drew a root with a
 * startup-only entry texture. VSBattle retires those textures at GO, but an
 * entry effect already playing can outlive GO: Samus's drew three more frames
 * on Jungle, Zebes, Hyrule and Dream Land and found its texture name 0 (the
 * refusal at the texture-slot check, 2026-09-30 native-owner census). */
static u32 sNdsEntryEffectStartupDrawSerial;

u32 ndsRendererEntryEffectStartupIdleFrames(void)
{
    return (sNdsRendererHardwareFrameSerial + 1u) -
           sNdsEntryEffectStartupDrawSerial;
}

#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
/* LAB ONLY: what each FoxSpecial3 packet replay hands the GX -- the root, both
 * matrices, the packet's words and the stats it reads -- folded per frame by
 * both the executor's branch and the replay submit, so the two can be compared
 * at the same logic frame without screen captures (gdb resets it). */
__attribute__((used)) volatile u32 gNdsLabFoxGxHash;
__attribute__((used)) volatile u32 gNdsLabFoxGxHashCount;
static void __attribute__((noinline, cold)) ndsLabFoxGxHash(u32 root_index,
    const NDSRendererConfig *config, const NDSEntryEffectPacket *packet,
    const NDSRendererStats *stats)
{
    u32 h = gNdsLabFoxGxHash ^ (root_index * 0x9e3779b1u);
    const u32 *p = (const u32 *)(const void *)config->initial_projection;
    const u32 *m = (const u32 *)(const void *)config->initial_modelview;
    u32 i;

#define NDS_LAB_FOX_MIX(v) (h = (h ^ (u32)(v)) * 0x01000193u)
    for (i = 0u; i < sizeof(NDSRendererMatrix20p12) / 4u; i++)
    {
        NDS_LAB_FOX_MIX(p[i]);
        NDS_LAB_FOX_MIX(m[i]);
    }
    NDS_LAB_FOX_MIX((uintptr_t)packet->words);
    NDS_LAB_FOX_MIX(packet->count);
    NDS_LAB_FOX_MIX(packet->split);
    NDS_LAB_FOX_MIX(packet->lit);
    for (i = 0u; i < packet->count; i++)
    {
        NDS_LAB_FOX_MIX(packet->words[i]);
    }
    NDS_LAB_FOX_MIX(stats->light_dir_x);
    NDS_LAB_FOX_MIX(stats->light_dir_y);
    NDS_LAB_FOX_MIX(stats->light_dir_z);
    NDS_LAB_FOX_MIX(stats->light_dir_mask);
    NDS_LAB_FOX_MIX(stats->othermode_l);
#undef NDS_LAB_FOX_MIX
    gNdsLabFoxGxHash = h;
    NDS_DIAG(gNdsLabFoxGxHashCount++);
}
#define NDS_LAB_FOX_GX_HASH(r, c, p, s) ndsLabFoxGxHash((r), (c), (p), (s))
#else
#define NDS_LAB_FOX_GX_HASH(r, c, p, s) ((void)0)
#endif

/* P2-2p8 (2026-10-05): a no-Z entry group whose corners all load under its
 * root's matrix draws through the GX instead of transforming and dividing
 * every corner on the CPU (ndsRendererHardwareClipVertex). That painter path
 * submits v16 z = depth * 4096 / w -- a constant clip-space z of depth / 4096
 * -- so the GX gets the root's composed matrix with its z column replaced by
 * that constant, at the GX clip scale (CPU clip / 256,
 * ndsRendererBuildRawHardwareMatrix). x and y are the same transform done by
 * the engine; the engine also clips what the CPU path only divided. */

/* Perf (2026-10-18): a Z group with matrix overrides (corners the RSP loaded
 * under an earlier root's matrix: Palutena's wing seams, Mario's pipe body)
 * draws through the GX as well. The engine transforms each vertex with the
 * matrix current when its VTX command arrives, so an override corner's own
 * modelview is loaded between the corners of the open batch (as skinned
 * NSBMD lists restore bone matrices between vertices) and the root's put
 * back after it, where the CPU path transformed and divided every corner of
 * the group (four Palutenas' smash wings: 240 corners each, ~90K cycles a
 * heavy frame). The projection stays; lighting stays on the CPU. No-Z
 * override groups keep the CPU painter. */

static void ndsRendererEntryEffectLoadPainter(
    const NDSRendererMatrix20p12 *composed, s32 depth)
{
    NDSRendererMatrix20p12 identity;
    NDSRendererMatrix20p12 painter;

    ndsRendererMtxIdentity20p12(&identity);
    ndsRendererBuildRawHardwareMatrix(composed, &painter);
    painter.m[0][2] = 0;
    painter.m[1][2] = 0;
    painter.m[2][2] = 0;
    painter.m[3][2] = ndsRendererRoundShiftS32Signed(
        depth, NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
    ndsRendererLoadHardwareMatrixPair(
        &identity, &painter, NDS_RENDERER_HW_MATRIX_MODE_NONE, 0u, FALSE);
    /* No later load may take this pair for its own. */
    sNdsRendererHardwareMatrixLoaded = FALSE;
}

s32 ndsRendererSubmitNativeEntryEffect(
    u32 owner_asset_id, u32 root_offset,
    const NDSRendererNativeMaterial *materials, u32 material_count,
    u32 live_texture_variant,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    const NDSEntryEffectRoot *root;
    u32 group_offset;
    u32 matrix_generation;
    u32 root_index;
    NDSRendererHardwareLightDirection
        light_direction_by_slot[NDS_ENTRY_EFFECT_MATRIX_SLOTS];
    u32 light_direction_valid_slots = 0u;
    u32 root_slot;
    u32 initial_prim_color;
    u32 initial_env_color;
    u32 initial_othermode_h;
    u32 initial_othermode_l;
    u32 shield_variant = 0u;
    u32 ko_part = 0xffffffffu;
    sb32 static_proven;
    u32 coords_fit = TRUE;
    u32 hw_light_loaded = FALSE;
    u32 last_dif_amb = 0xffffffffu;
    NDSEntryEffectStateRoot *state_root = NULL;
    NDSEntryEffectGroupState *state_groups = NULL;
    u32 state_recording = FALSE;
    u32 state_ok = TRUE;
    u32 state_replayed = FALSE;
    u32 state_fox = FALSE;
    u32 state_light_written = 0u;
    u32 composed_ready = FALSE;

    if ((config == NULL) || (stats == NULL) ||
        (config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        NDS_ENTRY_EFFECT_REJECT();
    }
    root = ndsRendererEntryEffectRoot(owner_asset_id, root_offset);
#ifdef NDS_ENTRY_EFFECT_P4_ROOT_COUNT
    /* A P4 list that only loads state (the Wolfen's head-0 lists): the
     * generator walked it for the state its head carries on; it draws
     * nothing, as on the N64. */
    if ((root != NULL) && (root->group_count == 0u) &&
        (root >= &sNdsEntryEffectRoots[NDS_ENTRY_EFFECT_P4_ROOT_FIRST]))
    {
        return TRUE;
    }
#endif
    if ((root == NULL) || (root->group_count == 0u) ||
        ((u32)root->first_group + root->group_count >
         NDS_ENTRY_EFFECT_GROUP_COUNT))
    {
        NDS_ENTRY_EFFECT_REJECT();
    }
    root_index = (u32)(root - &sNdsEntryEffectRoots[0]);
    if ((root_index >= NDS_ENTRY_EFFECT_ROOT_COUNT) ||
        (ndsRendererEntryEffectCornersSelect(root_index) == FALSE))
    {
        NDS_ENTRY_EFFECT_REJECT();
    }
    root_slot = ndsRendererEntryEffectMatrixSlot(root_index);
    if (root_slot >= NDS_ENTRY_EFFECT_MATRIX_SLOTS)
    {
        NDS_ENTRY_EFFECT_REJECT();
    }
    initial_prim_color = stats->prim_color;
    initial_env_color = stats->env_color;
    initial_othermode_h = stats->othermode_h;
    initial_othermode_l = stats->othermode_l;
    if (owner_asset_id == 84u)
    {
        u32 part;
        for (part = 0u; part < 3u; part++)
        {
            if (root_offset == sNdsEntryKoRootOffsets[part]) { ko_part = part; break; }
        }
    }
    if (owner_asset_id == 163u)
    {
        for (shield_variant = 0u; shield_variant < 5u; shield_variant++)
        {
            if (initial_env_color == sNdsEntryShieldEnvironment[shield_variant])
            {
                break;
            }
        }
        if ((shield_variant == 5u) ||
            (sNdsEntryShieldTextureName[shield_variant] == 0u))
        {
            NDS_ENTRY_EFFECT_REJECT();
        }
    }
    /* The first root of either source effect begins a new synchronous DObj
     * traversal. Only matrices captured during THIS traversal may satisfy a
     * later vertex-cache provenance read. */
    if ((root_index == 0u) ||
        (root_index == NDS_ENTRY_EFFECT_FOX_ROOT_FIRST) ||
        (root_index == NDS_ENTRY_EFFECT_DONKEY_ROOT_FIRST) ||
        (root_index == NDS_ENTRY_EFFECT_SAMUS_ROOT_FIRST) ||
        (root_index == NDS_ENTRY_EFFECT_CAPTAIN_ROOT_FIRST) ||
        (root_index == NDS_ENTRY_EFFECT_LINK_ROOT_FIRST) ||
        (root_index == NDS_ENTRY_EFFECT_LINK_BOOMERANG_ROOT_FIRST) ||
        (root_index == NDS_ENTRY_EFFECT_SHIELD_ROOT_FIRST) ||
        (root_index == NDS_ENTRY_EFFECT_REFLECTOR_ROOT_FIRST) ||
        (root_index == NDS_ENTRY_EFFECT_CATCH_ROOT_FIRST) ||
        (root_index == NDS_ENTRY_EFFECT_MBALLRAYS_ROOT_FIRST) ||
        (root_index == NDS_ENTRY_EFFECT_KIRBY_CUTTER_ROOT_FIRST) ||
        (root_index == NDS_ENTRY_EFFECT_KIRBY_CUTTER_WEAPON_ROOT_FIRST) ||
        (root_index == NDS_ENTRY_EFFECT_SAMUS_GRAPPLE_ROOT_FIRST) ||
        (root_index == NDS_ENTRY_EFFECT_FALCON_KICK_ROOT_FIRST) ||
        (root_index == NDS_ENTRY_EFFECT_FALCON_PUNCH_ROOT_FIRST) ||
        ((owner_asset_id == 348u) &&
         ((root_offset == 0x0c70u) || (root_offset == 0x11b0u) ||
          (root_offset == 0x2210u))) ||
        ((owner_asset_id == 85u) && (root_offset == 0x2ef0u)) ||
        ((owner_asset_id == 84u) &&
         ((root_offset == 0x5218u) || (root_offset == 0x31d0u))))
    {
        memset(sNdsRendererEntryEffectModelviewValidMask, 0,
               sizeof(sNdsRendererEntryEffectModelviewValidMask));
    }

    /* Fail closed before the first GX write. Besides resident texture names,
     * validate every generated compact-table index and sparse matrix override.
     * The old flat packet could only fail its contiguous corner bound; the
     * dictionary packet has more indices, so all of them are fenced here once
     * and the hot submit loop can remain branch-light. */
    static_proven =
        ((sNdsEntryEffectRootStaticOk[root_index >> 5] &
          (1u << (root_index & 31u))) != 0u) ? TRUE : FALSE;
    for (group_offset = 0u; group_offset < root->group_count; group_offset++)
    {
        const NDSEntryEffectGroup *group =
            &sNdsEntryEffectGroups[(u32)root->first_group + group_offset];
        u32 corner_count = (u32)group->triangle_count * 3u;
        u32 corner;
        u32 override_index;
        u32 previous_override = 0xffffffffu;

        if (static_proven != FALSE)
        {
            /* Only the checks that read live state. */
            if ((group->material_slot != 0xffu) &&
                ((materials == NULL) ||
                 ((u32)group->material_slot >= material_count)))
            {
                NDS_ENTRY_EFFECT_REJECT();
            }
            for (override_index = 0u;
                 override_index < group->matrix_override_count;
                 override_index++)
            {
                u32 source_root = sNdsEntryEffectMatrixOverrideRoot[
                    (u32)group->matrix_override_first + override_index];

                if (((sNdsRendererEntryEffectModelviewValidMask[source_root >> 5] &
                      (1u << (source_root & 31u))) == 0u) ||
                    (ndsRendererEntryEffectMatrixSlot(source_root) >=
                     NDS_ENTRY_EFFECT_MATRIX_SLOTS))
                {
                    NDS_ENTRY_EFFECT_REJECT();
                }
            }
            if ((group->texture_slot != NDS_ENTRY_EFFECT_TEXTURE_NONE) &&
                (owner_asset_id != 163u) &&
                (sNdsRendererEntryEffectTextureName[group->texture_slot] == 0u))
            {
                NDS_ENTRY_EFFECT_REJECT();
            }
            continue;
        }
        if ((group->root_index != root_index) ||
            ((u32)group->first_vertex + corner_count >
             NDS_ENTRY_EFFECT_VERTEX_COUNT) ||
            (group->geometry_state >= NDS_ENTRY_EFFECT_GEOMETRY_STATE_COUNT) ||
            (group->combine_state >= NDS_ENTRY_EFFECT_COMBINE_STATE_COUNT) ||
            (group->othermode_state >= NDS_ENTRY_EFFECT_OTHERMODE_STATE_COUNT) ||
            (group->prim_color_index >= NDS_ENTRY_EFFECT_PRIM_COLOR_COUNT) ||
            (group->env_color_index >= NDS_ENTRY_EFFECT_ENV_COLOR_COUNT) ||
            (group->light_state >= NDS_ENTRY_EFFECT_LIGHT_STATE_COUNT) ||
            ((group->material_slot != 0xffu) &&
             ((materials == NULL) ||
              ((u32)group->material_slot >= material_count))) ||
            ((u32)group->matrix_override_first + group->matrix_override_count >
             NDS_ENTRY_EFFECT_CROSS_MATRIX_CORNER_COUNT))
        {
            NDS_ENTRY_EFFECT_REJECT();
        }
        for (corner = 0u; corner < corner_count; corner++)
        {
            u32 vertex_index = (u32)group->first_vertex + corner;

            if ((sNdsEntryCornerPosition[vertex_index] >=
                 NDS_ENTRY_EFFECT_POSITION_COUNT) ||
                (sNdsEntryCornerS[vertex_index] >=
                 NDS_ENTRY_EFFECT_S_COUNT) ||
                (sNdsEntryCornerT[vertex_index] >=
                 NDS_ENTRY_EFFECT_T_COUNT) ||
                (sNdsEntryCornerColor[vertex_index] >=
                 NDS_ENTRY_EFFECT_COLOR_COUNT))
            {
                NDS_ENTRY_EFFECT_REJECT();
            }
            if (ndsRendererEntryEffectPositionFits(&sNdsEntryEffectPositions[
                    sNdsEntryCornerPosition[vertex_index]]) == 0u)
            {
                coords_fit = FALSE;
            }
        }
        for (override_index = 0u;
             override_index < group->matrix_override_count;
             override_index++)
        {
            u32 table_index =
                (u32)group->matrix_override_first + override_index;
            u32 override_corner =
                sNdsEntryEffectMatrixOverrideCorner[table_index];
            u32 source_root =
                sNdsEntryEffectMatrixOverrideRoot[table_index];

            if ((override_corner >= corner_count) ||
                ((previous_override != 0xffffffffu) &&
                 (override_corner <= previous_override)) ||
                (source_root >= NDS_ENTRY_EFFECT_ROOT_COUNT) ||
                (source_root >= root_index) ||
                ((sNdsRendererEntryEffectModelviewValidMask[source_root >> 5] &
                  (1u << (source_root & 31u))) == 0u) ||
                (ndsRendererEntryEffectMatrixSlot(source_root) >=
                 NDS_ENTRY_EFFECT_MATRIX_SLOTS))
            {
                NDS_ENTRY_EFFECT_REJECT();
            }
            previous_override = override_corner;
        }

        if (group->texture_slot == NDS_ENTRY_EFFECT_TEXTURE_NONE)
        {
            continue;
        }
        if ((group->texture_slot >= NDS_ENTRY_EFFECT_TEXTURE_COUNT) ||
            ((owner_asset_id != 163u) &&
             (sNdsRendererEntryEffectTextureName[group->texture_slot] == 0u)))
        {
            NDS_ENTRY_EFFECT_REJECT();
        }
    }
    if (static_proven == FALSE)
    {
        /* Every group's static half passed (a failure returned above). */
        sNdsEntryEffectRootStaticOk[root_index >> 5] |=
            1u << (root_index & 31u);
        if (coords_fit != FALSE)
        {
            sNdsEntryEffectRootCoordsFit[root_index >> 5] |=
                1u << (root_index & 31u);
        }
    }

    if (owner_asset_id == 84u)
    {
        u32 expected_effects = NDS_RENDERER_NATIVE_MATERIAL_PRIM;
        if ((ko_part == 0u) || (ko_part == 2u))
        {
            expected_effects |= NDS_RENDERER_NATIVE_MATERIAL_ENV;
        }
        if ((root_offset == 0x31d0u) || (root_offset == 0x3258u) || (root_offset == 0x32e0u))
        {
            expected_effects |= NDS_RENDERER_NATIVE_MATERIAL_LIGHT1;
        }
        if ((materials == NULL) || (material_count != 1u) ||
            (materials[0].effects != expected_effects)) { NDS_ENTRY_EFFECT_REJECT(); }
        if (ko_part < 3u)
        {
            for (group_offset = 0u; group_offset < root->group_count; group_offset++)
            {
                u32 index = (u32)root->first_group + group_offset;
                const NDSEntryEffectGroup *group = &sNdsEntryEffectGroups[index];
                u32 environment = ((sNdsEntryEffectColorWriteMasks[index] & 2u) != 0u) ?
                    sNdsEntryEffectEnvColors[group->env_color_index] : initial_env_color;
                if (group->material_slot != 0u) { NDS_ENTRY_EFFECT_REJECT(); }
                if ((expected_effects & NDS_RENDERER_NATIVE_MATERIAL_ENV) != 0u)
                {
                    environment = materials[0].env_color;
                }
                if (ndsRendererEntryKoPalette(ko_part, materials[0].prim_w1, environment) == 0u)
                {
                    NDS_ENTRY_EFFECT_REJECT();
                }
            }
        }
    }
    if (owner_asset_id == 353u)
    {
        u32 expected_effects;

        if ((materials == NULL) || (material_count != 1u))
        {
            NDS_ENTRY_EFFECT_REJECT();
        }
        switch (root_offset)
        {
        case 0x02d8u:
            /* Entry Wave MObjSub flags 0x32a0: LIGHT1 | LIGHT2 | PRIM,
             * source 0x20 render-tile sizing, and G_TEXTURE scaling. */
            expected_effects =
                NDS_RENDERER_NATIVE_MATERIAL_LIGHT1 |
                NDS_RENDERER_NATIVE_MATERIAL_LIGHT2 |
                NDS_RENDERER_NATIVE_MATERIAL_PRIM |
                NDS_RENDERER_NATIVE_MATERIAL_RENDER_TILE_SIZE |
                NDS_RENDERER_NATIVE_MATERIAL_TEXTURE;
            break;
        case 0x0698u:
            /* Entry Beam MObjSub flags 0x3200. */
            expected_effects =
                NDS_RENDERER_NATIVE_MATERIAL_LIGHT1 |
                NDS_RENDERER_NATIVE_MATERIAL_LIGHT2 |
                NDS_RENDERER_NATIVE_MATERIAL_PRIM;
            break;
        case 0x1100u:
            /* Attached Spin effect MObjSub flags 0x0200. */
            expected_effects = NDS_RENDERER_NATIVE_MATERIAL_PRIM;
            break;
        default:
            NDS_ENTRY_EFFECT_REJECT();
        }
        if (materials[0].effects != expected_effects)
        {
            NDS_ENTRY_EFFECT_REJECT();
        }
    }

    if ((owner_asset_id == 349u) && (root_offset == 0x02e0u))
    {
        const NDSEntryEffectGroup *group =
            &sNdsEntryEffectGroups[root->first_group];

        /* Source MObj flags are exactly MOBJ_FLAG_ALPHA: segment-E supplies
         * CURRENT_IMAGE and the root itself owns load/tile state. MatAnim
         * switches only TEXID 0/1, both preconverted by the generator. */
        if ((materials == NULL) || (material_count != 1u) ||
            (materials[0].effects !=
                 NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE) ||
            (live_texture_variant > 1u) ||
            (root->group_count != 1u) || (group->material_slot != 0u) ||
            (group->texture_slot !=
                 NDS_ENTRY_EFFECT_SAMUS_GRAPPLE_TEXTURE0_SLOT) ||
            (sNdsRendererEntryEffectTextureName[
                 NDS_ENTRY_EFFECT_SAMUS_GRAPPLE_TEXTURE0_SLOT] == 0u) ||
            (sNdsRendererEntryEffectTextureName[
                 NDS_ENTRY_EFFECT_SAMUS_GRAPPLE_TEXTURE1_SLOT] == 0u))
        {
            NDS_ENTRY_EFFECT_REJECT();
        }
    }

    if (((owner_asset_id == 350u) && (root_offset == 0x0a30u)) ||
        ((owner_asset_id == 333u) && (root_offset == 0x0760u)))
    {
        const NDSEntryEffectGroup *group =
            &sNdsEntryEffectGroups[root->first_group];
        u32 variant_count = (owner_asset_id == 350u) ? 2u : 3u;
        u32 primary_slot = (owner_asset_id == 350u) ?
            NDS_ENTRY_EFFECT_FALCON_KICK_TEXTURE0_SLOT :
            NDS_ENTRY_EFFECT_FALCON_PUNCH_TEXTURE0_SLOT;
        u32 expected_tile_size_w1 = (owner_asset_id == 350u) ?
            0x000bc0bcu : 0x0007c07cu;
        const u32 expected_effects =
            NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE |
            NDS_RENDERER_NATIVE_MATERIAL_RENDER_TILE_SIZE |
            NDS_RENDERER_NATIVE_MATERIAL_TEXTURE;

        /* Both source MObjs are exactly flags 0x00A1 (ALPHA | 0x20 |
         * TEXTURE). Their root DLs own combine/blend/TLUT/geometry; segment E
         * contributes the current CI4 image, render-tile size and G_TEXTURE.
         * MatAnim changes only TEXID, so every possible image is preconverted
         * and selected by the live texture_id_curr passed here. */
        if ((materials == NULL) || (material_count != 1u) ||
            (materials[0].effects != expected_effects) ||
            (materials[0].render_tile_size_w0 != 0xf2000000u) ||
            (materials[0].render_tile_size_w1 != expected_tile_size_w1) ||
            (materials[0].texture_w0 != 0xd7000002u) ||
            (materials[0].texture_w1 != 0xffffffffu) ||
            (live_texture_variant >= variant_count) ||
            (root->group_count != 1u) || (group->material_slot != 0u) ||
            (group->texture_slot != primary_slot))
        {
            NDS_ENTRY_EFFECT_REJECT();
        }
        if (owner_asset_id == 350u)
        {
            if ((sNdsRendererEntryEffectTextureName[
                     NDS_ENTRY_EFFECT_FALCON_KICK_TEXTURE0_SLOT] == 0u) ||
                (sNdsRendererEntryEffectTextureName[
                     NDS_ENTRY_EFFECT_FALCON_KICK_TEXTURE1_SLOT] == 0u))
            {
                NDS_ENTRY_EFFECT_REJECT();
            }
        }
        else if ((sNdsRendererEntryEffectTextureName[
                      NDS_ENTRY_EFFECT_FALCON_PUNCH_TEXTURE0_SLOT] == 0u) ||
                 (sNdsRendererEntryEffectTextureName[
                      NDS_ENTRY_EFFECT_FALCON_PUNCH_TEXTURE1_SLOT] == 0u) ||
                 (sNdsRendererEntryEffectTextureName[
                      NDS_ENTRY_EFFECT_FALCON_PUNCH_TEXTURE2_SLOT] == 0u))
        {
            NDS_ENTRY_EFFECT_REJECT();
        }
    }

#ifdef NDS_ENTRY_EFFECT_P4_ROOT_COUNT
    /* A P4 article root with a TEXID material (Wolf's slash and blaster):
     * one live MObj whose segment-E branch supplies only the current image
     * and, with MOBJ_FLAG_PALETTE, the palette-0 TLUT the generator baked.
     * MatAnim changes only TEXID, and every frame is preconverted. A MObj
     * with flags 0x20 and MOBJ_FLAG_TEXTURE (Falcon Punch's 0x00A1, which
     * Ganondorf's punch and kick keep) also sets the render tile's size and
     * G_TEXTURE: the generator baked both from the MObjSub (its size at
     * +0x0C, scale 0xFFFF), so they are admitted while the tile's origin is
     * the bake's (no scroll) and the scale is the bake's. */
    if ((root_index >= NDS_ENTRY_EFFECT_P4_ROOT_FIRST) &&
        (sNdsEntryEffectP4Roots[root_index - NDS_ENTRY_EFFECT_P4_ROOT_FIRST]
             .variant_count != 0u))
    {
        const NDSEntryEffectP4Root *p4_root =
            &sNdsEntryEffectP4Roots[root_index - NDS_ENTRY_EFFECT_P4_ROOT_FIRST];
        u32 allowed_effects =
            NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE |
            NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE |
            NDS_RENDERER_NATIVE_MATERIAL_PALETTE_TLUT;

        if ((p4_root->mobj_flags & 0x0020u) != 0u)
        {
            allowed_effects |= NDS_RENDERER_NATIVE_MATERIAL_RENDER_TILE_SIZE;
        }
        if ((p4_root->mobj_flags & 0x0080u) != 0u)
        {
            allowed_effects |= NDS_RENDERER_NATIVE_MATERIAL_TEXTURE;
        }
        if ((materials == NULL) || (material_count != 1u) ||
            ((materials[0].effects & ~allowed_effects) != 0u) ||
            ((materials[0].effects &
              NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE) == 0u) ||
            (((materials[0].effects &
               NDS_RENDERER_NATIVE_MATERIAL_RENDER_TILE_SIZE) != 0u) &&
             (materials[0].render_tile_size_w0 != 0xf2000000u)) ||
            (((materials[0].effects &
               NDS_RENDERER_NATIVE_MATERIAL_TEXTURE) != 0u) &&
             ((materials[0].texture_w0 != 0xd7000002u) ||
              (materials[0].texture_w1 != 0xffffffffu))) ||
            (live_texture_variant >= (u32)p4_root->variant_count) ||
            (sNdsRendererEntryEffectTextureName[
                 p4_root->variant_slots[live_texture_variant]] == 0u))
        {
            NDS_ENTRY_EFFECT_REJECT();
        }
    }
#endif

    /* LinkModel+0x11680 is a deliberately closed dynamic-material owner.
     * BattleShip builds segment 0xE from exactly nine MObjs and every one has
     * MOBJ_FLAG_PRIMCOLOR only. Refuse any broader live material state rather
     * than letting this specialization silently omit source presentation. */
    if (owner_asset_id == 324u)
    {
        u32 material_index;

        if ((root_offset != 0x11680u) || (materials == NULL) ||
            (material_count != 9u))
        {
            NDS_ENTRY_EFFECT_REJECT();
        }
        for (material_index = 0u; material_index < material_count;
             material_index++)
        {
            if (materials[material_index].effects !=
                NDS_RENDERER_NATIVE_MATERIAL_PRIM)
            {
                NDS_ENTRY_EFFECT_REJECT();
            }
        }
    }

    /* Poke Ball entry rays are a closed dynamic-material owner, the same shape
     * as the LinkModel contract above.  BattleShip builds segment 0xE for each
     * ray fan from EXACTLY two MObjs and both carry MOBJ_FLAG_PRIMCOLOR only
     * (EFCommonEffects3 MObjSubs at 0x0118, 0x0190, 0x0208, 0x0280 all read
     * flags 0x0200).  The whole live presentation is that PRIM ramp -- the
     * combiner's alpha mux is TEXEL0_A * PRIM_A -- so refuse any broader
     * material state rather than let this specialization silently drop source
     * presentation. */
    if (owner_asset_id == 85u)
    {
        u32 expected_material_count;
        u32 material_index;

        if ((root_offset == 0x0440u) || (root_offset == 0x0518u))
        {
            expected_material_count = 2u;
        }
        else if ((root_offset == 0x2ef0u) || (root_offset == 0x2f80u) ||
                 (root_offset == 0x3010u) || (root_offset == 0x30a0u))
        {
            expected_material_count = 1u;
        }
        else
        {
            NDS_ENTRY_EFFECT_REJECT();
        }
        if ((materials == NULL) ||
            (material_count != expected_material_count))
        {
            NDS_ENTRY_EFFECT_REJECT();
        }
        for (material_index = 0u; material_index < material_count;
             material_index++)
        {
            if (materials[material_index].effects !=
                NDS_RENDERER_NATIVE_MATERIAL_PRIM)
            {
                NDS_ENTRY_EFFECT_REJECT();
            }
        }
    }

    ndsRendererHardwareEndBatch();
    sNdsRendererEntryEffectModelview[root_slot] = *config->initial_modelview;
    if (ndsRendererEntryEffectIsOverrideSource(root_index) != 0u)
    {
        ndsRendererMtxMul20p12(
            config->initial_modelview, config->initial_projection,
            &sNdsRendererEntryEffectComposed[root_slot]);
        composed_ready = TRUE;
    }
    sNdsRendererEntryEffectModelviewValidMask[root_index >> 5] |=
        1u << (root_index & 31u);
    matrix_generation = ndsRendererNextMatrixGeneration();
    ndsRendererLoadHardwareSplitMatrices(
        config->initial_projection, config->initial_modelview,
        matrix_generation);

    if ((static_proven != FALSE) && (material_count == 0u) &&
        (root_offset != gNdsEntryEffectWitnessRoot))
    {
        if ((owner_asset_id == 161u) &&
            (root_index >= NDS_ENTRY_EFFECT_FOX_ROOT_FIRST) &&
            (root_index <
             NDS_ENTRY_EFFECT_FOX_ROOT_FIRST + NDS_ENTRY_STATE_ROOTS))
        {
            u32 first = (u32)root->first_group -
                (u32)sNdsEntryEffectRoots[
                    NDS_ENTRY_EFFECT_FOX_ROOT_FIRST].first_group;

            if (first + (u32)root->group_count <= NDS_ENTRY_STATE_GROUPS)
            {
                state_root = &sNdsEntryEffectStateRoots[
                    root_index - NDS_ENTRY_EFFECT_FOX_ROOT_FIRST];
                state_groups = &sNdsEntryEffectStateGroups[first];
                state_fox = TRUE;
            }
        }
#ifdef NDS_ENTRY_EFFECT_P4_ROOT_COUNT
        else if (root_index >= NDS_ENTRY_EFFECT_P4_ROOT_FIRST)
        {
            (void)ndsRendererEntryEffectP4State(root_index, root, &state_root,
                                                &state_groups);
        }
#endif
        if (state_root != NULL)
        {
            if (ndsRendererEntryEffectStateMatches(
                    state_root, stats, initial_prim_color, initial_env_color,
                    initial_othermode_h, initial_othermode_l,
                    config->initial_geometry_mode) != 0u)
            {
                state_replayed = TRUE;
            }
            else
            {
                state_recording = TRUE;
                state_root->valid = 1u;
                state_root->light_seed = 0u;
                state_root->prim_color = initial_prim_color;
                state_root->env_color = initial_env_color;
                state_root->othermode_h = initial_othermode_h;
                state_root->othermode_l = initial_othermode_l;
                state_root->geometry_mode = config->initial_geometry_mode;
                state_root->combine_count = stats->texture_combine_count;
                state_root->light_dir_mask =
                    stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK;
                state_root->light_color_1 = stats->light_color_1;
                state_root->light_color_2 = stats->light_color_2;
                state_root->light_color_mask = stats->light_color_mask;
                state_root->words = ndsRendererEntryEffectStateWords();
            }
        }
    }
    if (state_replayed != FALSE)
    {
        u32 ramp_names[NDS_ENTRY_STATE_GROUPS];
        const u32 packet_first = (state_fox != FALSE) ?
            (u32)(state_groups - &sNdsEntryEffectStateGroups[0]) : 0u;
        NDSEntryEffectPacket *packet = NULL;
        NDSEntryEffectPacketRecorder packet_rec;
        u32 packet_recording = FALSE;
        u32 packet_sent = FALSE;

        /* Ramp palettes first: a lookup may upload, and a failed one leaves
         * the ordinary loop a clean start. */
        for (group_offset = 0u; group_offset < root->group_count;
             group_offset++)
        {
            u32 index = (u32)root->first_group + group_offset;
            const NDSEntryEffectGroup *group = &sNdsEntryEffectGroups[index];

            ramp_names[group_offset] = 0u;
            if ((state_groups[group_offset].flags & NDS_ENTRY_STATE_RAMP) != 0u)
            {
                u32 prim = ((sNdsEntryEffectColorWriteMasks[index] & 1u) != 0u) ?
                    sNdsEntryEffectPrimColors[group->prim_color_index] :
                    initial_prim_color;
                u32 env = ((sNdsEntryEffectColorWriteMasks[index] & 2u) != 0u) ?
                    sNdsEntryEffectEnvColors[group->env_color_index] :
                    initial_env_color;

                ramp_names[group_offset] = ndsRendererEntryRampPalette(
                    &sNdsEntryEffectTextures[group->texture_slot], prim, env);
                if (ramp_names[group_offset] == 0u)
                {
                    state_replayed = FALSE;
                    state_root->valid = 0u;
                    break;
                }
            }
        }
        if ((state_replayed != FALSE) && (state_fox != FALSE))
        {
            packet = ndsRendererEntryEffectPacketFor(
                root_index - NDS_ENTRY_EFFECT_FOX_ROOT_FIRST);
        }
        if (packet != NULL)
        {
            if (ndsRendererEntryEffectPacketCurrent(
                    packet, root, packet_first, ramp_names) != 0u)
            {
                u32 bit = 1u << root_slot;

                NDS_LAB_FOX_GX_HASH(root_index, config, packet, stats);
                ndsRendererEntryEffectPacketReplay(
                    packet, stats, config->initial_modelview,
                    &light_direction_by_slot[root_slot],
                    light_direction_valid_slots & bit);
                if (packet->lit != 0u)
                {
                    light_direction_valid_slots |= bit;
                }
                packet_sent = TRUE;
            }
            else
            {
                ndsRendererEntryEffectPacketBegin(&packet_rec, packet, stats);
                packet_recording = TRUE;
            }
        }
        for (group_offset = 0u;
             (state_replayed != FALSE) && (packet_sent == FALSE) &&
             (group_offset < root->group_count);
             group_offset++)
        {
            const NDSEntryEffectGroup *group =
                &sNdsEntryEffectGroups[(u32)root->first_group + group_offset];
            const NDSEntryEffectGroupState *state = &state_groups[group_offset];
            u32 corner_count = (u32)group->triangle_count * 3u;
            u32 textured = (group->texture_slot != NDS_ENTRY_EFFECT_TEXTURE_NONE) ?
                TRUE : FALSE;
            u32 hw_lit = ((state->flags & NDS_ENTRY_STATE_HW_LIT) != 0u) ?
                TRUE : FALSE;
            u32 texture_name = 0u;

            if ((state->flags & NDS_ENTRY_STATE_SKIP) != 0u)
            {
                gNdsEntryEffectNativeAlphaSkipCount++;
                continue;
            }
            if (textured != FALSE)
            {
                texture_name =
                    sNdsRendererEntryEffectTextureName[group->texture_slot];
                ndsRendererHardwareBindTextureName(stats, texture_name);
                if ((state->flags & NDS_ENTRY_STATE_RAMP) != 0u)
                {
                    glAssignColorTable(GL_TEXTURE_2D,
                                       (int)ramp_names[group_offset]);
                    gNdsEntryRampPaletteDraws++;
                }
                ndsRendererHardwareApplyTextureParams(
                    ndsRendererHardwareMergeTextureParams(
                        state->texture_params));
                sNdsRendererHardwareActiveTextureEntry = NULL;
                stats->hardware_texture_ready_count++;
                gNdsEntryEffectNativeTextureBindCount++;
            }
            if (hw_lit != FALSE)
            {
                if (hw_light_loaded == FALSE)
                {
                    if ((light_direction_valid_slots & (1u << root_slot)) == 0u)
                    {
                        ndsRendererHardwarePrepareLitDirection(
                            stats, config->initial_modelview,
                            &light_direction_by_slot[root_slot]);
                        light_direction_valid_slots |= 1u << root_slot;
                    }
                    ndsRendererEntryEffectLoadLitMatrices(
                        config->initial_modelview,
                        &light_direction_by_slot[root_slot]);
                    hw_light_loaded = TRUE;
                    if (packet_recording != FALSE)
                    {
                        ndsEntryPacketSplit(&packet_rec);
                    }
                }
                if (state->dif_amb != last_dif_amb)
                {
                    ndsRendererHardwareWriteDiffuseAmbient(state->dif_amb);
                    last_dif_amb = state->dif_amb;
                    if (packet_recording != FALSE)
                    {
                        ndsEntryPacketCmd(&packet_rec, FIFO_DIFFUSE_AMBIENT, 1u,
                                          state->dif_amb, 0u);
                    }
                }
                gNdsEntryEffectHwLitGroups++;
            }
            else if ((state->flags & NDS_ENTRY_STATE_LIT) != 0u)
            {
                gNdsEntryEffectCpuLitGroups++;
            }
            stats->othermode_l = state->othermode_l;
            ndsRendererHardwareBeginTriangleBatch(
                stats, textured, texture_name, state->poly_fmt,
                sNdsRendererHardwareMatrixMode,
                sNdsRendererHardwareMatrixGeneration);
            if (packet_recording != FALSE)
            {
                ndsEntryPacketRecordGroup(&packet_rec, group, state, textured,
                                          texture_name, hw_lit);
            }
            if ((state->flags & NDS_ENTRY_STATE_VCOLOR) != 0u)
            {
                ndsRendererEntryEffectEmitShadedCorners(
                    (u32)group->first_vertex, corner_count, state->dif_amb,
                    ((state->flags & NDS_ENTRY_STATE_MCOLOR) != 0u) ? TRUE : FALSE,
                    (textured != FALSE) ? 1u : 0u);
            }
            else
            {
                if (hw_lit == FALSE)
                {
                    glColor(((state->flags & NDS_ENTRY_STATE_SHADE) != 0u) ?
                        (rgb)state->dif_amb : 0x7fffu);
                }
                ndsRendererEntryEffectEmitFastCorners(
                    (u32)group->first_vertex, corner_count,
                    (hw_lit != FALSE) ? 1u : 0u, (textured != FALSE) ? 1u : 0u);
            }
            gNdsEntryEffectFastCornerGroups++;
            sNdsRendererHardwareSubmitted = TRUE;
            stats->triangle_count += group->triangle_count;
            stats->transformed_triangle_count += group->triangle_count;
            stats->hardware_triangle_count += group->triangle_count;
            stats->hardware_vertex_count += corner_count;
            stats->hardware_zbuffer_triangle_count += group->triangle_count;
            ndsRendererHardwareEndBatch();
        }
        if (packet_recording != FALSE)
        {
            ndsRendererEntryEffectPacketEnd(&packet_rec, root, packet_first,
                                            ramp_names, stats);
        }
        if (state_replayed != FALSE)
        {
            NDS_DIAG(gNdsEntryEffectStateReplays++);
        }
    }

    for (group_offset = (state_replayed != FALSE) ? (u32)root->group_count : 0u;
         group_offset < root->group_count; group_offset++)
    {
        const NDSEntryEffectGroup *group =
            &sNdsEntryEffectGroups[(u32)root->first_group + group_offset];
        u32 use_texture =
            (group->texture_slot != NDS_ENTRY_EFFECT_TEXTURE_NONE) ? TRUE : FALSE;
        u32 texture_name = 0u;
        u32 ramp_palette = 0u;
        u32 material_color;
        s32 use_material_color;
        s32 use_vertex_color;
        u32 poly_fmt;
        u32 corner_count = (u32)group->triangle_count * 3u;
        u32 polygon_alpha;
        u32 corner;
        u32 projected_group =
            (group->matrix_override_count != 0u) ? TRUE : FALSE;
        u32 no_z_group;
        u32 cpu_projected_group;
        u32 gx_painter_group;
        u32 gx_override_group;
        u32 loaded_slot;
        s32 painter_depth = 0;
        u32 matrix_override_cursor = 0u;
        const NDSEntryEffectPairState *geometry_state =
            &sNdsEntryEffectGeometryStates[group->geometry_state];
        const NDSEntryEffectPairState *combine_state =
            &sNdsEntryEffectCombineStates[group->combine_state];
        const NDSEntryEffectPairState *othermode_state =
            &sNdsEntryEffectOthermodeStates[group->othermode_state];
        const NDSEntryEffectPairState *othermode_writes =
            &sNdsEntryEffectOtherModeWriteMasks[(u32)root->first_group + group_offset];
        u32 color_writes =
            sNdsEntryEffectColorWriteMasks[(u32)root->first_group + group_offset];
        const NDSEntryEffectPairState *light_state =
            &sNdsEntryEffectLightColors[group->light_state];
        u32 light_mask = sNdsEntryEffectLightMasks[group->light_state];
        s32 lit;
        u32 hw_lit_group;
        u32 shade_group;
        u32 shade_word;     /* a SHADE group's colour word (its group state's) */

        /* The RSP transforms a vertex when G_VTX loads its cache slot.  A later
         * display list may then reuse that slot after the modelview changes.
         * Mario's pipe body does exactly that: its first twelve triangles mix
         * vertices loaded under the rim and body matrices.  GX cannot reproduce
         * that by changing its matrix between vertices of one open primitive;
         * the generic renderer handles the same source shape by CPU-projecting
         * the mixed-snapshot triangle.  If a generated state group contains any
         * such corner, project the whole small group from each corner's recorded
         * load-time matrix.  This keeps primitive submission legal and preserves
         * the generic path's load-time transform semantics.
         * Ordinary entry groups remain on the raw split-matrix path.  The
         * generated sparse override count is exactly the old per-corner test. */

        /* A bit the list neither cleared nor set is inherited from the battle
         * display's state, exactly as the RSP had it. Masking G_LIGHTING off
         * here drew the pipe's normals as colours: a black pipe body. */
        stats->geometry_mode =
            (config->initial_geometry_mode & ~geometry_state->b) |
            geometry_state->a;
        no_z_group =
            ((stats->geometry_mode & NDS_RENDERER_GEOM_ZBUFFER) == 0u) ?
                TRUE : FALSE;
        gx_painter_group =
            ((no_z_group != FALSE) && (projected_group == FALSE)) ? TRUE : FALSE;
        gx_override_group =
            ((projected_group != FALSE) && (no_z_group == FALSE)) ? TRUE : FALSE;
        cpu_projected_group =
            (((projected_group != FALSE) && (gx_override_group == FALSE)) ||
             ((no_z_group != FALSE) && (gx_painter_group == FALSE))) ?
                TRUE : FALSE;
        stats->othermode_h = (initial_othermode_h & ~othermode_writes->a) |
                            (othermode_state->a & othermode_writes->a);
        stats->othermode_l = (initial_othermode_l & ~othermode_writes->b) |
                            (othermode_state->b & othermode_writes->b);
        stats->prim_color = ((color_writes & 1u) != 0u) ?
            sNdsEntryEffectPrimColors[group->prim_color_index] : initial_prim_color;
        stats->env_color = ((color_writes & 2u) != 0u) ?
            sNdsEntryEffectEnvColors[group->env_color_index] : initial_env_color;
        ndsRendererRecordSetCombine(stats, combine_state->a, combine_state->b);
        if ((light_mask & 1u) != 0u)
        {
            stats->light_color_1 = light_state->a;
            stats->light_color_mask |= NDS_RENDERER_LIGHT_COLOR_1_MASK;
        }
        if ((light_mask & 2u) != 0u)
        {
            stats->light_color_2 = light_state->b;
            stats->light_color_mask |= NDS_RENDERER_LIGHT_COLOR_2_MASK;
        }
        state_light_written |= light_mask & 3u;
        if (group->material_slot != 0xffu)
        {
            const NDSRendererNativeMaterial *material =
                &materials[group->material_slot];

            /* Source segment 0xE executes immediately before these triangles.
             * The admission contracts above make this a closed typed replay:
             * LinkSpecial2 may carry live light/PRIM/texture-scale state and
             * LinkModel Spin carries PRIM only. */
            ndsRendererEntryEffectApplyLiveMaterial(material, stats);
        }
        polygon_alpha = ndsRendererHardwareAlpha(stats, NULL);
        /* DS alpha zero selects wireframe, not transparent fill. Source
         * fading materials still update state/matrix ownership above, but
         * have no visible geometry once their quantized coverage is zero. */
        if (polygon_alpha == 0u)
        {
            /* A skipped draw must be COUNTED, never read as a success.  The
             * aggregate and per-root draw counters increment once per submitted
             * root whatever its groups resolve to, so without this a fully
             * faded owner is indistinguishable from a drawn one.  MBallRays
             * spends its whole post-ramp tail here by design: its PRIMCOLOR
             * ramp reaches alpha 0 at source tick 50 while its rotation track
             * runs to 130. */
            gNdsEntryEffectNativeAlphaSkipCount++;
            if (state_recording != FALSE)
            {
                state_groups[group_offset].flags = NDS_ENTRY_STATE_SKIP;
            }
            continue;
        }
        lit = ndsRendererHardwareLitShadeCombine(stats);
        if ((state_recording != FALSE) && (lit != FALSE) &&
            ((state_light_written & 3u) != 3u))
        {
            /* A lit group reads a colour this root did not write. */
            state_root->light_seed = 1u;
        }
        if (root_offset == gNdsEntryEffectWitnessRoot)
        {
            gNdsEntryEffectWitness[0] = polygon_alpha;
            gNdsEntryEffectWitness[1] = stats->prim_color;
            gNdsEntryEffectWitness[2] = stats->env_color;
            gNdsEntryEffectWitness[3] = stats->othermode_l;
        }
        /* The direction depends on the battle light and this root's
         * modelview, and no group changes either: once per root. */
        if ((lit != FALSE) &&
            ((light_direction_valid_slots & (1u << root_slot)) == 0u))
        {
            ndsRendererHardwarePrepareLitDirection(
                stats, config->initial_modelview,
                &light_direction_by_slot[root_slot]);
            light_direction_valid_slots |= 1u << root_slot;
        }

        if (use_texture != FALSE)
        {
            const NDSEntryEffectTexture *texture =
                &sNdsEntryEffectTextures[group->texture_slot];
            NDSRendererTileState tile = {0};
            u32 params;

            texture_name =
                sNdsRendererEntryEffectTextureName[group->texture_slot];
            if (owner_asset_id == 163u)
            {
                texture_name = sNdsEntryShieldTextureName[shield_variant];
            }
            else if ((owner_asset_id == 349u) && (root_offset == 0x02e0u))
            {
                texture_name = sNdsRendererEntryEffectTextureName[
                    (live_texture_variant == 0u) ?
                        NDS_ENTRY_EFFECT_SAMUS_GRAPPLE_TEXTURE0_SLOT :
                        NDS_ENTRY_EFFECT_SAMUS_GRAPPLE_TEXTURE1_SLOT];
            }
            else if ((owner_asset_id == 350u) && (root_offset == 0x0a30u))
            {
                texture_name = sNdsRendererEntryEffectTextureName[
                    (live_texture_variant == 0u) ?
                        NDS_ENTRY_EFFECT_FALCON_KICK_TEXTURE0_SLOT :
                        NDS_ENTRY_EFFECT_FALCON_KICK_TEXTURE1_SLOT];
            }
            else if ((owner_asset_id == 333u) && (root_offset == 0x0760u))
            {
                u32 slot = (live_texture_variant == 0u) ?
                    NDS_ENTRY_EFFECT_FALCON_PUNCH_TEXTURE0_SLOT :
                    ((live_texture_variant == 1u) ?
                        NDS_ENTRY_EFFECT_FALCON_PUNCH_TEXTURE1_SLOT :
                        NDS_ENTRY_EFFECT_FALCON_PUNCH_TEXTURE2_SLOT);
                texture_name = sNdsRendererEntryEffectTextureName[slot];
            }
#ifdef NDS_ENTRY_EFFECT_P4_ROOT_COUNT
            else if ((root_index >= NDS_ENTRY_EFFECT_P4_ROOT_FIRST) &&
                     (sNdsEntryEffectP4Roots[root_index -
                          NDS_ENTRY_EFFECT_P4_ROOT_FIRST].variant_count != 0u))
            {
                /* Admitted above: the variant is in range and resident. */
                texture_name = sNdsRendererEntryEffectTextureName[
                    sNdsEntryEffectP4Roots[root_index -
                        NDS_ENTRY_EFFECT_P4_ROOT_FIRST]
                        .variant_slots[live_texture_variant]];
            }
#endif
            tile.set_seen = TRUE;
            tile.width = texture->width;
            tile.height = texture->height;
            tile.cms = group->cms;
            tile.cmt = group->cmt;
            tile.masks = group->masks;
            tile.maskt = group->maskt;
            params = ndsRendererHardwareTextureParams(
                stats, &tile, texture->width, texture->height);
            /* Before the bind: baking a palette binds its own name. The KO
             * stars and the shield keep the palettes they already own. */
            if ((ko_part >= 3u) && (owner_asset_id != 163u) &&
                (ndsRendererEntryRampCombine(combine_state) != FALSE) &&
                (ndsRendererEntryRampSlotEligible(group->texture_slot) != FALSE))
            {
                ramp_palette = ndsRendererEntryRampPalette(
                    texture, stats->prim_color, stats->env_color);
            }
            ndsRendererHardwareBindTextureName(stats, texture_name);
            if (ramp_palette != 0u)
            {
                glAssignColorTable(GL_TEXTURE_2D, (int)ramp_palette);
                gNdsEntryRampPaletteDraws++;
            }
            if (ko_part < 3u)
            {
                /* Palette addresses are immutable throughout queued draws.
                 * Only the selected binding changes; texels stay resident. */
                glAssignColorTable(GL_TEXTURE_2D,
                    (int)ndsRendererEntryKoPalette(ko_part, stats->prim_color, stats->env_color));
            }
            if (state_recording != FALSE)
            {
                state_groups[group_offset].texture_params = params;
            }
            ndsRendererHardwareApplyTextureParams(
                ndsRendererHardwareMergeTextureParams(params));
            sNdsRendererHardwareActiveTextureEntry = NULL;
            stats->hardware_texture_ready_count++;
            gNdsEntryEffectNativeTextureBindCount++;
        }

        material_color = ndsRendererHardwareColorSource(stats);
        use_material_color = ndsRendererHardwareUseMaterialColor(stats);
        use_vertex_color = ndsRendererHardwareUseVertexColor(stats);
        poly_fmt = ndsRendererHardwarePolyFmt(stats, polygon_alpha);
        if ((root->flags & NDS_ENTRY_ROOT_OWN_POLY_ID) != 0u)
        {
            poly_fmt = (poly_fmt & ~(u32)POLY_ID(NDS_RENDERER_POLY_ID_MASK)) |
                       POLY_ID(NDS_ENTRY_ROOT_OWN_POLY_ID_VALUE);
        }
        /* Lit groups are shaded on the CPU below unless the engine lights
         * them (hw_lit_group); otherwise
         * POLY_FORMAT_LIGHT0 must stay absent even if a previous hardware-lit
         * owner left a light vector in GX state. */
        poly_fmt &= ~((u32)POLY_FORMAT_LIGHT0);
        hw_lit_group =
            ((lit != FALSE) &&
             (cpu_projected_group == FALSE) && (gx_painter_group == FALSE) &&
             (projected_group == FALSE) &&
             (ramp_palette == 0u) && (use_material_color == FALSE) &&
             ((stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK) != 0u) &&
             ((stats->light_color_mask & NDS_ENTRY_EFFECT_LIGHT_COLORS) ==
              NDS_ENTRY_EFFECT_LIGHT_COLORS)) ? TRUE : FALSE;
        shade_group = ((lit == FALSE) && (ramp_palette == 0u)) ? TRUE : FALSE;
        shade_word = (shade_group == FALSE) ? 0u :
            (use_vertex_color != FALSE) ? material_color :
            (u32)ndsRendererHardwarePackedValidVertexColor(
                material_color, use_material_color, FALSE, 0u, 0u);
        if (cpu_projected_group != FALSE)
        {
            ndsRendererLoadHardwareMatrices(NULL, FALSE);
            /* The next split load rewrites the vector matrix too. */
            hw_light_loaded = FALSE;
            if (composed_ready == FALSE)
            {
                ndsRendererMtxMul20p12(
                    config->initial_modelview, config->initial_projection,
                    &sNdsRendererEntryEffectComposed[root_slot]);
                composed_ready = TRUE;
            }
        }
        else if (gx_painter_group != FALSE)
        {
            hw_light_loaded = FALSE;
            if (composed_ready == FALSE)
            {
                ndsRendererMtxMul20p12(
                    config->initial_modelview, config->initial_projection,
                    &sNdsRendererEntryEffectComposed[root_slot]);
                composed_ready = TRUE;
            }
            /* The CPU painter takes one depth a triangle; the group draws at
             * its first and consumes the rest. */
            ndsRendererEntryEffectLoadPainter(
                &sNdsRendererEntryEffectComposed[root_slot],
                ndsRendererHardwareNextProjectedDepth());
            if (group->triangle_count > 1u)
            {
                sNdsRendererHardwareProjectedDepth -=
                    NDS_RENDERER_HW_PROJECTED_DEPTH_STEP *
                    ((s32)group->triangle_count - 1);
            }
        }
        else
        {
            ndsRendererLoadHardwareSplitMatrices(
                config->initial_projection, config->initial_modelview,
                matrix_generation);
        }
        if (hw_lit_group != FALSE)
        {
            u32 dif_amb = ndsRendererEntryEffectDiffuseAmbient(
                stats->light_color_1, stats->light_color_2);

            if (hw_light_loaded == FALSE)
            {
                ndsRendererEntryEffectLoadLitMatrices(
                    config->initial_modelview,
                    &light_direction_by_slot[root_slot]);
                hw_light_loaded = TRUE;
            }
            if (dif_amb != last_dif_amb)
            {
                ndsRendererHardwareWriteDiffuseAmbient(dif_amb);
                last_dif_amb = dif_amb;
            }
            poly_fmt |= (u32)POLY_FORMAT_LIGHT0;
            gNdsEntryEffectHwLitGroups++;
        }
        else if (lit != FALSE)
        {
            gNdsEntryEffectCpuLitGroups++;
        }
        if (state_recording != FALSE)
        {
            NDSEntryEffectGroupState *state = &state_groups[group_offset];

            state->poly_fmt = poly_fmt;
            state->dif_amb = (hw_lit_group != FALSE) ?
                ndsRendererEntryEffectDiffuseAmbient(
                    stats->light_color_1, stats->light_color_2) : shade_word;
            state->othermode_l = stats->othermode_l;
            state->flags = ((lit != FALSE) ? NDS_ENTRY_STATE_LIT : 0u) |
                ((hw_lit_group != FALSE) ? NDS_ENTRY_STATE_HW_LIT : 0u) |
                ((ramp_palette != 0u) ? NDS_ENTRY_STATE_RAMP : 0u) |
                ((shade_group != FALSE) ? NDS_ENTRY_STATE_SHADE : 0u) |
                (((shade_group != FALSE) && (use_vertex_color != FALSE)) ?
                    NDS_ENTRY_STATE_VCOLOR : 0u) |
                (((shade_group != FALSE) && (use_material_color != FALSE)) ?
                    NDS_ENTRY_STATE_MCOLOR : 0u);
        }
        ndsRendererHardwareBeginTriangleBatch(
            stats, use_texture, texture_name, poly_fmt,
            sNdsRendererHardwareMatrixMode,
            sNdsRendererHardwareMatrixGeneration);
        if (no_z_group != FALSE)
        {
            NDS_DIAG(gNdsEntryEffectNativeNoZGroupDraws++);
        }

        corner = 0u;
        /* The split load above holds the root's modelview. */
        loaded_slot = root_slot;
        if (gx_override_group != FALSE)
        {
            ndsRendererHardwareFighterSetMatrixMode(GL_MODELVIEW);
        }
        if ((static_proven != FALSE) &&
            (cpu_projected_group == FALSE) && (gx_painter_group == FALSE) &&
            (projected_group == FALSE) &&
            ((hw_lit_group != FALSE) || (ramp_palette != 0u) ||
             (shade_group != FALSE)) &&
            ((sNdsEntryEffectRootCoordsFit[root_index >> 5] &
              (1u << (root_index & 31u))) != 0u) &&
            (ndsRendererEntryEffectFastCornersReady() != 0u))
        {
            if ((shade_group != FALSE) && (use_vertex_color != FALSE))
            {
                ndsRendererEntryEffectEmitShadedCorners(
                    (u32)group->first_vertex, corner_count, material_color,
                    use_material_color, (use_texture != FALSE) ? 1u : 0u);
            }
            else
            {
                if (hw_lit_group == FALSE)
                {
                    /* The ramp palette already holds the whole colour
                     * combiner, or the group has one colour (the corner
                     * loop's packing); the colour register holds for every
                     * corner after it. */
                    glColor((ramp_palette != 0u) ? 0x7fffu : (rgb)shade_word);
                }
                ndsRendererEntryEffectEmitFastCorners(
                    (u32)group->first_vertex, corner_count,
                    (hw_lit_group != FALSE) ? 1u : 0u,
                    (use_texture != FALSE) ? 1u : 0u);
            }
            gNdsEntryEffectFastCornerGroups++;
            corner = corner_count;
            if ((shade_group != FALSE) && (state_fox != FALSE))
            {
                /* The Arwing's packet records white for an unlit group. */
                state_ok = FALSE;
            }
        }
        else
        {
            /* The replay only draws fast corners. */
            state_ok = FALSE;
        }
        for (; corner < corner_count; corner++)
        {
            u32 vertex_index = (u32)group->first_vertex + corner;
            NDSRendererInputVertex vertex;
            const NDSRendererInputVertex *vtx = &vertex;
            u32 source_root = root_index;
            u32 source_slot = root_slot;
            u16 packed_color;

            if (matrix_override_cursor < group->matrix_override_count)
            {
                u32 table_index =
                    (u32)group->matrix_override_first + matrix_override_cursor;

                if (sNdsEntryEffectMatrixOverrideCorner[table_index] == corner)
                {
                    source_root =
                        sNdsEntryEffectMatrixOverrideRoot[table_index];
                    /* Proved to have a slot by the checks above. */
                    source_slot = ndsRendererEntryEffectMatrixSlot(source_root);
                    matrix_override_cursor++;
                }
            }
            (void)source_root;
            if (static_proven != FALSE)
            {
                ndsRendererEntryEffectVertexProven(vertex_index, &vertex);
            }
            else if (ndsRendererEntryEffectVertex(vertex_index, &vertex) == FALSE)
            {
                /* Prevalidation above makes this unreachable unless generated
                 * data is corrupted after the fence. End the open batch before
                 * refusing so the adapter can fall back safely. */
                ndsRendererHardwareEndBatch();
                NDS_ENTRY_EFFECT_REJECT();
            }

            if (hw_lit_group != FALSE)
            {
                /* The engine lights the corner from its source normal. */
                ndsRendererHardwareWriteNormalWord(
                    ndsRendererEntryEffectNormalWord(vtx));
                packed_color = 0u;
            }
            else if (lit != FALSE)
            {
                if ((light_direction_valid_slots & (1u << source_slot)) == 0u)
                {
                    ndsRendererHardwarePrepareLitDirection(
                        stats, &sNdsRendererEntryEffectModelview[source_slot],
                        &light_direction_by_slot[source_slot]);
                    light_direction_valid_slots |= 1u << source_slot;
                }
                /* r,g,b hold the source normal; shade it against the seeded
                 * battle light under the matrix that was live when the RSP
                 * cache slot was loaded, not necessarily this DObj's matrix. */
                packed_color = ndsRendererHardwarePackedResolvedColor(
                    ndsRendererHardwareLitShadeColorPrepared(
                        stats, vtx, &light_direction_by_slot[source_slot]),
                    material_color, use_material_color, 0u);
            }
            else
            {
                u32 vertex_color =
                    ((u32)vtx->r << 24) | ((u32)vtx->g << 16) |
                    ((u32)vtx->b << 8) | (u32)vtx->a;

                packed_color = ndsRendererHardwarePackedVertexColor(
                    stats, vtx, material_color,
                    use_material_color, use_vertex_color,
                    vertex_color, TRUE, 0u);
            }
            if (hw_lit_group == FALSE)
            {
                /* The ramp palette already holds the whole colour combiner. */
                glColor((ramp_palette != 0u) ? 0x7fffu : packed_color);
            }
            if (use_texture != FALSE)
            {
                /* Already converted from N64 s10.5 + tile/scale state to the
                 * DS t16 coordinate in the generator. */
                glTexCoord2t16((t16)vtx->s, (t16)vtx->t);
            }
            if (cpu_projected_group != FALSE)
            {
                NDSRendererClipVertex20p12 clip;

                if ((no_z_group != FALSE) && ((corner % 3u) == 0u))
                {
                    painter_depth = ndsRendererHardwareNextProjectedDepth();
                }

                ndsRendererTransformVertex20p12(
                    &sNdsRendererEntryEffectComposed[source_slot], vtx, &clip);
                stats->matrix_transform_count++;
                stats->transformed_vertex_count++;
                ndsRendererHardwareClipVertex(
                    &clip, (no_z_group != FALSE) ? painter_depth : clip.z
                    );
            }
            else
            {
                if ((gx_override_group != FALSE) && (source_slot != loaded_slot))
                {
                    ndsRendererHardwareFighterLoadModelviewWorldScaled(
                        &sNdsRendererEntryEffectModelview[source_slot]);
                    loaded_slot = source_slot;
                }
                glVertex3v16(
                    ndsRendererEntryEffectCoord(vtx->x),
                    ndsRendererEntryEffectCoord(vtx->y),
                    ndsRendererEntryEffectCoord(vtx->z));
            }
        }
        if (gx_override_group != FALSE)
        {
            /* The engine may hold an override's modelview: the next group
             * loads its own. */
            sNdsRendererHardwareMatrixLoaded = FALSE;
        }
        if ((cpu_projected_group != FALSE) || (gx_painter_group != FALSE) ||
            (gx_override_group != FALSE))
        {
            ndsRendererHardwareEnterProjectedForeground();
        }
        sNdsRendererHardwareSubmitted = TRUE;
        stats->triangle_count += group->triangle_count;
        stats->transformed_triangle_count += group->triangle_count;
        stats->hardware_triangle_count += group->triangle_count;
        stats->hardware_vertex_count += corner_count;
        /* Groups whose merged source state has G_ZBUFFER clear follow the
         * generic interpreter's projected painter path. Mixed-matrix groups
         * also remain CPU-projected, with their source clip depth intact. */
        if ((cpu_projected_group != FALSE) || (gx_painter_group != FALSE))
        {
            stats->hardware_projected_depth_triangle_count +=
                group->triangle_count;
        }
        else
        {
            stats->hardware_zbuffer_triangle_count += group->triangle_count;
        }
        ndsRendererHardwareEndBatch();
    }
    if ((state_recording != FALSE) && (state_ok != FALSE))
    {
        state_root->valid = 2u;
        NDS_DIAG(gNdsEntryEffectStateRecords++);
    }
    NDS_DIAG(gNdsEntryEffectNativeDrawCount++);
    {
        u32 g;

        /* A root drawing a startup-only texture keeps those textures
         * alive (ndsRendererEntryEffectStartupIdleFrames). */
        for (g = 0u; g < (u32)root->group_count; g++)
        {
            u32 slot = sNdsEntryEffectGroups[(u32)root->first_group + g]
                .texture_slot;

            if ((slot != NDS_ENTRY_EFFECT_TEXTURE_NONE) &&
                (sNdsEntryEffectTextureStartupOnly[slot] != 0u))
            {
                sNdsEntryEffectStartupDrawSerial =
                    sNdsRendererHardwareFrameSerial + 1u;
                break;
            }
        }
    }
    if ((owner_asset_id == 350u) && (root_offset == 0x0a30u))
    {
        gNdsFalconKickNativeSubmitCount++;
    }
    else if ((owner_asset_id == 333u) && (root_offset == 0x0760u))
    {
        gNdsFalconPunchNativeSubmitCount++;
    }
    {
        if (root_index < NDS_ENTRY_EFFECT_ROOT_COUNT)
        {
            gNdsEntryEffectNativeRootDraws[root_index]++;
        }
    }
    return TRUE;
}

/* P2-2p8 (2026-10-04, owner r75 Sector Z row): Sector Z's Arwing draws eight
 * FoxSpecial3 roots a frame, and once their packets are recorded every draw is
 * the executor's replay branch above: the static checks, the modelview store,
 * the split matrix load, the state match and one packet DMA. Reaching it
 * through the general executor walked ~15 KB of every other owner's code per
 * root (42.8K cycles a frame at 7.9 cycles an instruction in the lab profile,
 * artifacts/task37-census/sz-szprof02). This is that branch alone, for owner
 * 161 with no materials, in the executor's order and with its stores. It
 * returns FALSE whenever the executor would not take the branch -- before any
 * GX write, except when a ramp palette or the packet turns out stale after
 * the matrix load, which the executor then repeats with the same values.
 */
__attribute__((used)) volatile u32 gNdsEntryEffectFoxReplayDraws;
__attribute__((used)) volatile u32 gNdsEntryEffectFoxReplayDeclines;


s32 ndsRendererReplayNativeEntryEffectFox(u32 root_offset,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const NDSEntryEffectRoot *root;
    NDSEntryEffectStateRoot *state_root;
    const NDSEntryEffectGroupState *state_groups;
    NDSEntryEffectPacket *packet;
    NDSRendererHardwareLightDirection light_direction;
    u32 ramp_names[NDS_ENTRY_STATE_GROUPS];
    u32 root_index;
    u32 root_slot;
    u32 root_bit;
    u32 first;
    u32 group_offset;
    u32 has_ramp = FALSE;
    u32 initial_prim_color;
    u32 initial_env_color;

    NDS_FIGHTER_PACKET_DMA_WAIT();
    if ((config == NULL) ||
        (stats == NULL) || (config->initial_projection == NULL) ||
        (config->initial_modelview == NULL) ||
        (root_offset == gNdsEntryEffectWitnessRoot))
    {
        return FALSE;
    }
    root = ndsRendererEntryEffectRoot(161u, root_offset);
    if ((root == NULL) || (root->group_count == 0u) ||
        ((u32)root->first_group + root->group_count >
         NDS_ENTRY_EFFECT_GROUP_COUNT))
    {
        return FALSE;
    }
    root_index = (u32)(root - &sNdsEntryEffectRoots[0]);
    root_bit = 1u << (root_index & 31u);
    root_slot = (root_index < NDS_ENTRY_EFFECT_ROOT_COUNT) ?
        ndsRendererEntryEffectMatrixSlot(root_index) : NDS_ENTRY_EFFECT_MATRIX_SLOTS;
    if (root_slot >= NDS_ENTRY_EFFECT_MATRIX_SLOTS)
    {
        return FALSE;
    }
    if ((root_index < NDS_ENTRY_EFFECT_FOX_ROOT_FIRST) ||
        (root_index >= NDS_ENTRY_EFFECT_FOX_ROOT_FIRST + NDS_ENTRY_STATE_ROOTS) ||
        ((sNdsEntryEffectRootStaticOk[root_index >> 5] & root_bit) == 0u))
    {
        return FALSE;
    }
    first = (u32)root->first_group -
        (u32)sNdsEntryEffectRoots[NDS_ENTRY_EFFECT_FOX_ROOT_FIRST].first_group;
    if (first + (u32)root->group_count > NDS_ENTRY_STATE_GROUPS)
    {
        return FALSE;
    }
    state_root = &sNdsEntryEffectStateRoots[
        root_index - NDS_ENTRY_EFFECT_FOX_ROOT_FIRST];
    state_groups = &sNdsEntryEffectStateGroups[first];
    initial_prim_color = stats->prim_color;
    initial_env_color = stats->env_color;
    if (ndsRendererEntryEffectStateMatches(
            state_root, stats, initial_prim_color, initial_env_color,
            stats->othermode_h, stats->othermode_l,
            config->initial_geometry_mode) == 0u)
    {
        return FALSE;
    }
    packet = ndsRendererEntryEffectPacketFor(
        root_index - NDS_ENTRY_EFFECT_FOX_ROOT_FIRST);
    if ((packet == NULL) || (packet->valid == 0u))
    {
        return FALSE;
    }
    /* The executor's traversal reset, ahead of its override checks. */
    if (root_index == NDS_ENTRY_EFFECT_FOX_ROOT_FIRST)
    {
        memset(sNdsRendererEntryEffectModelviewValidMask, 0,
               sizeof(sNdsRendererEntryEffectModelviewValidMask));
    }
    /* Its static-proven checks: the ones that read live state. */
    for (group_offset = 0u; group_offset < root->group_count; group_offset++)
    {
        const NDSEntryEffectGroup *group =
            &sNdsEntryEffectGroups[(u32)root->first_group + group_offset];
        u32 override_index;

        if (group->material_slot != 0xffu)
        {
            return FALSE;
        }
        for (override_index = 0u;
             override_index < group->matrix_override_count;
             override_index++)
        {
            u32 source_root = sNdsEntryEffectMatrixOverrideRoot[
                (u32)group->matrix_override_first + override_index];

            if (((sNdsRendererEntryEffectModelviewValidMask[source_root >> 5] &
                  (1u << (source_root & 31u))) == 0u) ||
                (ndsRendererEntryEffectMatrixSlot(source_root) >=
                 NDS_ENTRY_EFFECT_MATRIX_SLOTS))
            {
                return FALSE;
            }
        }
        if ((group->texture_slot != NDS_ENTRY_EFFECT_TEXTURE_NONE) &&
            (sNdsRendererEntryEffectTextureName[group->texture_slot] == 0u))
        {
            return FALSE;
        }
        ramp_names[group_offset] = 0u;
        if ((state_groups[group_offset].flags & NDS_ENTRY_STATE_RAMP) != 0u)
        {
            has_ramp = TRUE;
        }
    }
    if ((has_ramp == FALSE) &&
        (ndsRendererEntryEffectPacketCurrent(packet, root, first,
                                             ramp_names) == 0u))
    {
        return FALSE;
    }

    ndsRendererHardwareEndBatch();
    sNdsRendererEntryEffectModelview[root_slot] = *config->initial_modelview;
    if (ndsRendererEntryEffectIsOverrideSource(root_index) != 0u)
    {
        ndsRendererMtxMul20p12(
            config->initial_modelview, config->initial_projection,
            &sNdsRendererEntryEffectComposed[root_slot]);
    }
    sNdsRendererEntryEffectModelviewValidMask[root_index >> 5] |= root_bit;
    ndsRendererLoadHardwareSplitMatrices(
        config->initial_projection, config->initial_modelview,
        ndsRendererNextMatrixGeneration());
    if (has_ramp != FALSE)
    {
        /* After the matrix load, as the executor looks them up. */
        for (group_offset = 0u; group_offset < root->group_count;
             group_offset++)
        {
            u32 index = (u32)root->first_group + group_offset;
            const NDSEntryEffectGroup *group = &sNdsEntryEffectGroups[index];

            if ((state_groups[group_offset].flags & NDS_ENTRY_STATE_RAMP) == 0u)
            {
                continue;
            }
            ramp_names[group_offset] = ndsRendererEntryRampPalette(
                &sNdsEntryEffectTextures[group->texture_slot],
                ((sNdsEntryEffectColorWriteMasks[index] & 1u) != 0u) ?
                    sNdsEntryEffectPrimColors[group->prim_color_index] :
                    initial_prim_color,
                ((sNdsEntryEffectColorWriteMasks[index] & 2u) != 0u) ?
                    sNdsEntryEffectEnvColors[group->env_color_index] :
                    initial_env_color);
            if (ramp_names[group_offset] == 0u)
            {
                /* The executor invalidates the state and resolves the root. */
                NDS_DIAG(gNdsEntryEffectFoxReplayDeclines++);
                return FALSE;
            }
        }
        if (ndsRendererEntryEffectPacketCurrent(packet, root, first,
                                                ramp_names) == 0u)
        {
            NDS_DIAG(gNdsEntryEffectFoxReplayDeclines++);
            return FALSE;
        }
    }
    NDS_LAB_FOX_GX_HASH(root_index, config, packet, stats);
    ndsRendererEntryEffectPacketReplay(packet, stats, config->initial_modelview,
                                       &light_direction, 0u);
    NDS_DIAG(gNdsEntryEffectStateReplays++);
    NDS_DIAG(gNdsEntryEffectNativeDrawCount++);
    for (group_offset = 0u; group_offset < (u32)root->group_count;
         group_offset++)
    {
        u32 slot = sNdsEntryEffectGroups[(u32)root->first_group + group_offset]
            .texture_slot;

        if ((slot != NDS_ENTRY_EFFECT_TEXTURE_NONE) &&
            (sNdsEntryEffectTextureStartupOnly[slot] != 0u))
        {
            sNdsEntryEffectStartupDrawSerial =
                sNdsRendererHardwareFrameSerial + 1u;
            break;
        }
    }
    gNdsEntryEffectNativeRootDraws[root_index]++;
    NDS_DIAG(gNdsEntryEffectFoxReplayDraws++);
    return TRUE;
}
