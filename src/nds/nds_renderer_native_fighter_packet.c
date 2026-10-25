/* Native renderer, the fighter packet: the arena in the framebuffer's tail, the
 * lean entries' keys, light and texgen patching, and Link's texgen map.
 * A slice of nds_renderer.c. */

extern u16 gSYFramebufferSets[1][231][320];

/* Slice 5: the list code every lean draw runs (the active entry, its guard,
 * the kernel's modelview sites, the dirty-line flush, the submit, and the
 * shade re-derive the replay shares) lives in ITCM beside the joint kernel,
 * in the production execute's former ITCM (a lean draw never runs it).
 * Placement only. The slice 5 attribution priced this code's instruction
 * fetch at roughly one tick per byte per draw: each draw runs it once, cold,
 * after the head. */
#if defined(__arm__)
#define NDS_FTR_LEAN_DRAW_CODE __attribute__((section(".itcm.ftr_lean_draw")))
#else
#define NDS_FTR_LEAN_DRAW_CODE
#endif

static u32 ndsFighterPacketMix(u32 hash, u32 value)
{
    return (hash ^ value) * 16777619u;
}

static s32 ndsFighterPacketTexturesResident(const NDSFighterPacket *packet)
{
    u32 i;

    for (i = 0u; i < (u32)packet->texture_count; i++)
    {
        const NDSFighterPacketTexture *texture = &packet->textures[i];
        const NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[texture->slot_plus1 - 1u];

        if ((entry->ready == FALSE) ||
            ((u32)(u16)entry->name != (u32)texture->name) ||
            (entry->key_generation != texture->key_generation))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/* A packet replay binds these textures directly through its recorded FIFO
 * words, so it bypasses ndsRendererHardwareBindTextureName and the normal
 * texture-prepare path that refresh `last_used_frame`. Without an explicit
 * touch, a texture used by a fighter on every packet-hit frame looks cold to
 * the shared dynamic-cache LRU and is eventually evicted by stage/effect work.
 * That invalidates the packet on the next fighter draw and forces a complete
 * production re-record -- exactly the alternating four-CPU FTR tail.
 *
 * The residency predicate has already proved slot/name/generation identity;
 * this changes cache bookkeeping only. Pinned entries accept the same touch
 * harmlessly, while dynamic entries now reflect the GX use that actually
 * occurred on this frame. */
static void ndsFighterPacketTouchTextures(const NDSFighterPacket *packet)
{
    u32 i;

    for (i = 0u; i < (u32)packet->texture_count; i++)
    {
        const NDSFighterPacketTexture *texture = &packet->textures[i];
        NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[texture->slot_plus1 - 1u];

        entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
    }
}

/* Re-derive every shade site's DIF_AMB word for the live prim/modulate.
 * Skipped when neither moved since the words were last derived.
 *
 * Slice 5: one core for both callers -- the replay's (the roots' prims read
 * from its production inputs) and the lean path's (read from its patch view,
 * NDSFtrLeanPatchView) -- taking the roots' prim colours as an array. A site
 * whose inputs (both light colours, the material colour, use_material) equal
 * the previous site's takes that site's word: the word is a pure function of
 * those inputs and the modulate, and under a flashing modulate each
 * MaterialColor15 costs three divides a channel. */
static s32 NDS_FTR_LEAN_DRAW_CODE
ndsFighterPacketApplyTintPrims(NDSFighterPacket *packet, const u32 *prims,
                               u32 modulate)
{
    u32 prim_hash = 2166136261u;
    u32 memo_light_1 = 0u;
    u32 memo_light_2 = 0u;
    u32 memo_material = 0u;
    u32 memo_use = 0xffffffffu;
    u32 memo_word = 0u;
    u32 i;

    for (i = 0u; i < packet->root_count; i++)
    {
        prim_hash = ndsFighterPacketMix(prim_hash, prims[i]);
    }
    if ((packet->tint_modulate == modulate) &&
        (packet->tint_prim_hash == prim_hash))
    {
        return TRUE;
    }
    /* A TINT TILE IS PART OF THE RECORDED BIND, SO A NEW PRIM CANNOT BE
     * RE-DERIVED INTO IT. A tinted site's colour lives in the tile the packet
     * binds, not in its shade word; when the roots' prim changes, the live draw
     * would look up a different tile, and re-deriving the words here would
     * replay the old one. Fail closed and let the owner re-record. A modulate
     * change alone is fine: it acts on the raw-light word, which is exactly
     * what the live draw re-derives for a tinted epoch. */
    if (packet->tint_prim_hash != prim_hash)
    {
        for (i = 0u; i < packet->site_count; i++)
        {
            if (packet->sites[i].reserved[0] != 0u)
            {
                return FALSE;
            }
        }
    }
    for (i = 0u; i < packet->site_count; i++)
    {
        const NDSFighterPacketShadeSite *site = &packet->sites[i];
        u32 material_color = site->material_color;

        if ((site->prim_from_root != 0u) &&
            ((u32)site->root < packet->root_count))
        {
            material_color = prims[site->root];
        }
        if ((site->light_color_1 != memo_light_1) ||
            (site->light_color_2 != memo_light_2) ||
            (material_color != memo_material) ||
            ((u32)site->use_material != memo_use))
        {
            u32 diffuse = ndsRendererR2MaterialColor15(
                site->light_color_1, material_color, site->use_material,
                modulate);
            u32 ambient = ndsRendererR2MaterialColor15(
                site->light_color_2, material_color, site->use_material,
                modulate);

            /* THE SAME CLAMP THE PRODUCTION PATH APPLIES (:7347), AND ITS ABSENCE
             * HERE WAS P04/K02/J01.
             *
             * The word this function overwrites was written CLAMPED -- the
             * shade site holds the post-clamp value. This
             * re-derive rebuilt it from the frozen inputs and stopped one call
             * short, so the first tint move (damage flash, invincibility blink,
             * team colour) replaced a clamped word with an unclamped one and then
             * latched it: the early-out above keeps the new prim hash, so nothing
             * re-derives it again until the packet is re-recorded.
             *
             * That is the shape of the owner's report exactly. The clamp only bites
             * a TINTED prim -- it returns `diffuse` untouched for use_material == 0
             * and for a white prim -- so the only fighters that can show it are the
             * three it was written for: Pikachu 0xFFD933, Kirby 0x00FF5A and Purin
             * 0xFFCDD8, which are the three face/body rows. A part whose run is
             * untinted kept its value while the tinted run beside it jumped, which
             * reads as two colours on one model. And holding neutral B cured it
             * because a status change re-records the packet, restoring the clamped
             * word -- a static arithmetic error could not be cured by a button, but
             * a latched cache entry is cured by exactly that.
             *
             * Nothing else needs to change: a rigid clamp of the recorded inputs is
             * what the live draw would have written for this prim, so prepared and
             * replayed frames now derive the identical word instead of two. */
            diffuse = ndsRendererR2ClampDiffuseToMaterial(
                diffuse, ambient, material_color, site->use_material,
                modulate);
            memo_light_1 = site->light_color_1;
            memo_light_2 = site->light_color_2;
            memo_material = material_color;
            memo_use = site->use_material;
            memo_word = diffuse | (ambient << 16);
        }
        packet->words[site->index] = memo_word;
    }
    packet->tint_modulate = modulate;
    packet->tint_prim_hash = prim_hash;
    return TRUE;
}

/* ndsRendererR2WriteLightVector's normalisation, same hardware units.
 *
 * TWIN, AND IT IS A CHECK OWED. A replayed frame submits this word instead of
 * the live one, so the two formulas must agree or the same fighter shades
 * differently depending on whether its draw was prepared or replayed --
 * precisely the fresh-versus-replay comparison the P04/K02/J01 audit asks for.
 *
 * NDS_LIGHT_VECTOR_STRETCH_FIX divides the LIVE word by |M L| instead of
 * |L| when the vector matrix stretches past a 1/32 deadband. This function
 * cannot: it patches words into a recorded packet outside the draw, with no
 * vector matrix to read and no un-stale value to read it from -- and a cached
 * last-frame stretch would be exactly the sticky latch this task was told not
 * to add. So it deliberately stays on the r25 arithmetic.
 *
 * That is SAFE ONLY WHILE THE REPAIR NEVER FIRES, which on a rigid chain it
 * provably never does (scripts/check-light-stretch.py CLAIM 2: the repaired
 * word is bit-identical there). The condition is therefore countable rather
 * than assumed: if a run shows gNdsLightMatrixStretchApplied advancing at
 * all, the mechanism is real, this twin is wrong for that fighter, and it must
 * be reconciled before acceptance -- do not read a green screenshot as proof
 * while that counter is non-zero. */
static u32 ndsFighterPacketLightWord(s32 x, s32 y, s32 z)
{
    s64 length_squared = ((s64)x * x) + ((s64)y * y) + ((s64)z * z);
    s32 nx = 0;
    s32 ny = 0;
    s32 nz = 0;

    if (length_squared > 0)
    {
        u32 length = ndsHwMathSqrt64((u64)length_squared);

        if (length != 0u)
        {
            nx = (s32)ndsHwMathDiv64(-(s64)x * 511, (s32)length);
            ny = (s32)ndsHwMathDiv64(-(s64)y * 511, (s32)length);
            nz = (s32)ndsHwMathDiv64(-(s64)z * 511, (s32)length);
        }
    }
    return NDS_NORMAL_PACK((int)nx, (int)ny, (int)nz);
}

#if NDS_LINK
/* P2-2p8. Patch only the FIFO_TEX_COORD words whose source run used
 * G_TEXTURE_GEN.  This is the exact math from
 * ndsRendererNativeRebuildProductionRunUv, evaluated from the packet's frozen
 * texture-preparation inputs plus this draw's live root modelview and LookAt.
 *
 * Sites from one run are contiguous; a site repeating the previous site's
 * normal copies its word. */
_Static_assert(((((u64)0xffffu) * NDS_NATIVE_TEXGEN_SCALE_RECIP_Q48) >> 8) <=
                   (u64)0xffffffffu,
               "the texgen scale factor must fit a 32x32 multiply");

/* GX texgen (gNdsFighterPacketHwTexgen, nds_renderer_preamble.c). The engine
 * forms S = TEXCOORD + (N0*m0 + N1*m4 + N2*m8) >> 21 per GFX_NORMAL, the
 * normal 1.0.9 and the matrix 20.12. The corner's normal word is the source
 * s8 normal n scaled by 511/127 (ndsRendererR2NormalComponent), so the
 * source's (n.Lq) * scale / (8 * 127 * 32767) is N.M >> 21 with
 * M = Lq * scale * 2^18 / (511 * 32767): a 2^48 reciprocal and a shift. */
#define NDS_FIGHTER_PACKET_HW_TEXGEN_RECIP \
    (((u64)1u << 48) / ((u64)511u * 32767u))

static inline u32 ndsFighterPacketHwTexgenEntry(s32 direction, u32 scale)
{
    return (u32)(s32)(((s64)direction * (s64)(scale & 0xffffu) *
                       (s64)NDS_FIGHTER_PACKET_HW_TEXGEN_RECIP) >> 30);
}

/* ndsRendererNativeTexgenCoord's value at a zero dot plus the group's origin
 * and offset terms: the constant every corner of the group shares. */
static u32 ndsFighterPacketTexgenBaseWord(u32 scale_s, u32 scale_t,
                                          u32 origin_s, u32 origin_t,
                                          s32 offset)
{
    const u64 unit = (u64)NDS_NATIVE_TEXGEN_UNIT;
    const u32 factor_s = (u32)((((u64)(scale_s & 0xffffu)) *
                                NDS_NATIVE_TEXGEN_SCALE_RECIP_Q48) >> 8);
    const u32 factor_t = (u32)((((u64)(scale_t & 0xffffu)) *
                                NDS_NATIVE_TEXGEN_SCALE_RECIP_Q48) >> 8);
    const s32 s = (s32)((unit * factor_s) >> 40) + offset -
        ((s32)origin_s << 2);
    const s32 t = (s32)((unit * factor_t) >> 40) + offset -
        ((s32)origin_t << 2);

    return (u32)(u16)(s16)s | ((u32)(u16)(s16)t << 16);
}

/* ndsRendererNativeTexgenCoord's non-LINEAR value with the scale's factor
 * made by the caller: a 32-bit dot (see ndsFighterPacketPatchTexgen) and one
 * 32x32->64 multiply. */
static inline s32 ndsFighterPacketTexgenScaled(
    s32 nx, s32 ny, s32 nz, const NDSNativeTexgenDirectionQ15 *direction,
    u32 factor)
{
    const s32 unit = (s32)NDS_NATIVE_TEXGEN_UNIT;
    s32 dot = nx * direction->x + ny * direction->y + nz * direction->z;

    if (dot > unit)
    {
        dot = unit;
    }
    else if (dot < -unit)
    {
        dot = -unit;
    }
    return (s32)(((u64)(u32)(dot + unit) * (u64)factor) >> 40);
}

/* A texgen site's normal as the source rgba carried it (s8 x, y, z in the
 * top three bytes). A LEAN_TRIM image has no dense_vertices: the normal is
 * recovered from the vertex's baked NORMAL word, each s8 n scaled by 511/127
 * truncating and clamped to [-512, 511] (_pack_ds_normal_from_rgba), which is
 * one-to-one: |n| = ceil(|w| * 127 / 511). Texgen sites are lit vertices, and
 * the generator trims no owner whose texgen could meet an unlit run's colour
 * word (p4_native_owner.py). */
static inline u32 ndsFighterPacketTexgenRgba(
    const NDSNativeFighterRuntimeTables *tables, u32 dense_id)
{
    u32 word;
    u32 rgba = 0u;
    u32 i;

    if (tables->dense_vertices != NULL)
    {
        return tables->dense_vertices[dense_id].rgba;
    }
    word = tables->dense_normals[dense_id];
    for (i = 0u; i < 3u; i++)
    {
        const s32 v = ((s32)(word << (22u - 10u * i))) >> 22;
        s32 n = (((v < 0) ? -v : v) * 127 + 510) / 511;

        if (v < 0)
        {
            n = -n;
        }
        rgba |= (u32)(u8)(s8)n << (24u - 8u * i);
    }
    return rgba;
}

static s32 ndsFighterPacketPatchTexgen(
    NDSFighterPacket *packet,
    const NDSRendererNativeFighterRoot *inputs,
    u32 input_count)
{
    const LookAt *look_at;
    const NDSRendererMatrix20p12 *directions_for = NULL;
    NDSNativeTexgenDirectionQ15 lookat_x = { 0, 0, 0 };
    NDSNativeTexgenDirectionQ15 lookat_y = { 0, 0, 0 };
    u32 group_index;

    if (packet->texgen_group_count == 0u)
    {
        return TRUE;
    }
    if ((packet->texgen_group_count > NDS_FIGHTER_PACKET_TEXGEN_GROUP_MAX) ||
        (packet->texgen_site_count > NDS_FIGHTER_PACKET_TEXGEN_SITE_MAX) ||
        (sNdsNativeFighterActiveTables == NULL))
    {
        return FALSE;
    }
    look_at = ndsRendererAdapterCurrentLookAt();
    if (look_at == NULL)
    {
        return FALSE;
    }
    /* P2-6 (2026-10-01): the lookat directions are reused while consecutive
     * groups share a root's modelview, and the unique-normal cache is searched
     * newest first -- a strip's next corner is almost always one of the last
     * two seen (Race: this patch was the largest single cost of its over-gate
     * frames). */
    for (group_index = 0u;
         group_index < (u32)packet->texgen_group_count;
         group_index++)
    {
        const NDSFighterPacketTexgenGroup *group =
            &packet->texgen_groups[group_index];
        u32 first_site;
        u32 site_end;
        u32 site_index;

        first_site = group->first_site;
        site_end = first_site + group->site_count;
        if (((u32)group->root >= input_count) ||
            (inputs[group->root].modelview_matrix == NULL) ||
            (site_end > (u32)packet->texgen_site_count))
        {
            return FALSE;
        }
        if (inputs[group->root].modelview_matrix != directions_for)
        {
            if ((ndsRendererNativePrepareTexgenDirectionQ15(
                     &look_at->l[0], inputs[group->root].modelview_matrix,
                     &lookat_x) == FALSE) ||
                (ndsRendererNativePrepareTexgenDirectionQ15(
                     &look_at->l[1], inputs[group->root].modelview_matrix,
                     &lookat_y) == FALSE))
            {
                return FALSE;
            }
            directions_for = inputs[group->root].modelview_matrix;
        }
        if (group->reserved[1] != 0u)
        {
            /* GX texgen: the group's texture matrix, rows 0-2 (N.x, N.y,
             * N.z), columns S then T (MTX_LOAD_4x3 parameters 0-1, 3-4,
             * 6-7; the rest stay zero). */
            const u32 at = group->origin_s;
            u32 *m;

            if ((at + 12u) > packet->word_count)
            {
                return FALSE;
            }
            m = &packet->words[at];
            m[0] = ndsFighterPacketHwTexgenEntry(lookat_x.x, group->scale_s);
            m[1] = ndsFighterPacketHwTexgenEntry(lookat_y.x, group->scale_t);
            m[3] = ndsFighterPacketHwTexgenEntry(lookat_x.y, group->scale_s);
            m[4] = ndsFighterPacketHwTexgenEntry(lookat_y.y, group->scale_t);
            m[6] = ndsFighterPacketHwTexgenEntry(lookat_x.z, group->scale_s);
            m[7] = ndsFighterPacketHwTexgenEntry(lookat_y.z, group->scale_t);
            continue;
        }
        /* P2-6 (2026-10-02): a polygon's texgen sites are 93% distinct
         * normals (Race, 1p-pf10r-st11: 364 sites a frame, 337 computed), so
         * the unique-normal cache's search was a full scan per site and 36% of
         * this function, and each coordinate was a call doing 64-bit products.
         * The memo is now the previous site's normal only (strip repeats are
         * adjacent), the per-group scale reciprocal is made once, and the dot
         * product is 32-bit: |n| <= 128 and |direction| <= 32767 bound it by
         * 3 * 2^22, so every value is the one the 64-bit form computed. The
         * LINEAR form keeps ndsRendererNativeTexgenCoord. */
        {
            const NDSNativeFighterRuntimeTables *tables =
                sNdsNativeFighterActiveTables;
            const u32 dense_count = sNdsNativeFighterActiveTables->dense_count;
            const u32 word_count = packet->word_count;
            u32 *words = packet->words;
            const u32 linear = group->reserved[0];
            const u32 factor_s = (u32)((((u64)(group->scale_s & 0xffffu)) *
                                        NDS_NATIVE_TEXGEN_SCALE_RECIP_Q48) >>
                                       8);
            const u32 factor_t = (u32)((((u64)(group->scale_t & 0xffffu)) *
                                        NDS_NATIVE_TEXGEN_SCALE_RECIP_Q48) >>
                                       8);
            const s32 base_s = group->offset - ((s32)group->origin_s << 2);
            const s32 base_t = group->offset - ((s32)group->origin_t << 2);
            u32 last_dense = 0xffffffffu;
            u32 st = 0u;

            for (site_index = first_site; site_index < site_end; site_index++)
            {
                const NDSFighterPacketTexgenSite *site =
                    &packet->texgen_sites[site_index];
                u32 dense_id;

                dense_id = site->dense_id;
                if (dense_id != last_dense)
                {
                    u32 rgba;
                    s32 nx;
                    s32 ny;
                    s32 nz;
                    s32 scaled_s;
                    s32 scaled_t;

                    if (dense_id >= dense_count)
                    {
                        return FALSE;
                    }
                    rgba = ndsFighterPacketTexgenRgba(tables, dense_id);
                    nx = (s32)(s8)(rgba >> 24);
                    ny = (s32)(s8)(rgba >> 16);
                    nz = (s32)(s8)(rgba >> 8);
                    if (linear == 0u)
                    {
                        scaled_s = ndsFighterPacketTexgenScaled(
                            nx, ny, nz, &lookat_x, factor_s);
                        scaled_t = ndsFighterPacketTexgenScaled(
                            nx, ny, nz, &lookat_y, factor_t);
                    }
                    else
                    {
                        scaled_s = ndsRendererNativeTexgenCoord(
                            nx, ny, nz, &lookat_x, group->scale_s, linear);
                        scaled_t = ndsRendererNativeTexgenCoord(
                            nx, ny, nz, &lookat_y, group->scale_t, linear);
                    }
                    st = (u32)(u16)(s16)(scaled_s + base_s) |
                         ((u32)(u16)(s16)(scaled_t + base_t) << 16);
                    last_dense = dense_id;
                }
                if (site->index >= word_count)
                {
                    return FALSE;
                }
                words[site->index] = st;
            }
        }
    }
    return TRUE;
}
#else
static s32 ndsFighterPacketPatchTexgen(
    NDSFighterPacket *packet,
    const NDSRendererNativeFighterRoot *inputs,
    u32 input_count)
{
    (void)packet;
    (void)inputs;
    (void)input_count;
    return TRUE;
}

static u32 ndsFighterPacketTexgenBaseWord(u32 scale_s, u32 scale_t,
                                          u32 origin_s, u32 origin_t,
                                          s32 offset)
{
    (void)scale_s;
    (void)scale_t;
    (void)origin_s;
    (void)origin_t;
    (void)offset;
    return 0u;
}
#endif

/* A fighter packet sets TEXIMAGE_PARAM / PLTT_BASE behind libnds. Forget
 * libnds's active names too, or a later glBindTexture of the name it still
 * holds returns without writing the registers and draws with the packet's
 * texture (Yoshi's guard egg, invisible in the owner's r55 playtest). Out of
 * line because the lean submit runs from ITCM. */
static void __attribute__((noinline)) ndsRendererForgetLibndsTexture(void)
{
    glGlobalData.activeTexture = 0;
    glGlobalData.activePalette = 0;
}

void ndsRendererFighterPacketDmaWait(void)
{
    ndsFighterPacketDmaWait();
}

void ndsRendererFighterPacketInvalidateAll(void)
{
    u32 i;

    /* A preview/costume rebuild happens on the update side, normally between
     * draws, but wait anyway: the borrowed arena must not be considered free
     * for a new list while DMA0 can still be consuming an old one. */
    ndsFighterPacketDmaWait();
    for (i = 0u; i < NDS_FIGHTER_PACKET_SLOTS; i++)
    {
        ndsFtrLeanPacketDrop(i);
    }
}

void ndsRendererFighterPacketInvalidateSlot(u32 slot)
{
    if (slot >= NDS_FIGHTER_PACKET_SLOTS)
    {
        ndsRendererFighterPacketInvalidateAll();
        return;
    }
    /* Player-select rebuilds happen on the update side, between draws.  Still
     * honor the packet arena's DMA lifetime before making one region reusable;
     * a future caller reaching this seam earlier must not race DMA0. */
    ndsFighterPacketDmaWait();
    /* A lean list is keyed by content (the material rows, the roots' display
     * fields, the owner file and the heap generation), so the fighter's lists
     * stay valid and the next draw's event path re-selects one by key (P2-2p8
     * Phase 1 slice 4: DK's and Samus's model-part swaps re-materialized every
     * list they held). */
}

/* THE FRAMEBUFFER'S TAIL. The packet arena stops 6,400 B short of the
 * buffer's end, where sys/video.h's z-buffer start pointer is aimed and never
 * dereferenced (battleship_sys_zbuffer.c), and no draw path reads it in a
 * battle. The shield pose memo keeps its rows there (nds_shield_pose.c); the
 * release below restores the clear over it with the arena's. */
u8 *ndsFighterPacketArenaTail(u32 *out_bytes)
{
    if (out_bytes != NULL)
    {
        *out_bytes = (u32)sizeof(gSYFramebufferSets) -
            (NDS_FIGHTER_PACKET_ARENA_WORDS * (u32)sizeof(u32));
    }
    return (u8 *)(void *)&gSYFramebufferSets[0][0][0] +
        (NDS_FIGHTER_PACKET_ARENA_WORDS * sizeof(u32));
}

NDS_RENDERER_COLD void ndsRendererFighterPacketRelease(void)
{
    u16 *arena = &gSYFramebufferSets[0][0][0];
    u32 i;

    ndsRendererFighterPacketInvalidateAll();
    /* scmanager.c's boot clear, GPACK_RGBA5551(0, 0, 0, 1), over the words
     * the packets borrowed and the tail the shield memo used, so the Results
     * photo wipe reads what it always read. */
    for (i = 0u; i < (u32)(sizeof(gSYFramebufferSets) / sizeof(u16)); i++)
    {
        arena[i] = 0x0001u;
    }
}

/* ---------------------------------------------------------------------------
 * P2-2p8 Phase 1 slices 1-4: the lean fighter path, list side
 * (include/nds/renderer_fighter_lean.h).
 *
 * Slice 4: a lean list is MATERIALIZED -- never recorded, never adopted --
 * from the generated owner tables and the live material state, in the LOAD4x3
 * + P' layout, into one of two entries of the battle slot's own region of the
 * packet arena (entry 0 the lower half, entry 1 the upper). Route 1 uses both
 * and the recorder never arms for a slot whose lower half a lean list holds;
 * routes 2/3 keep the recorder in the lower half (the oracle's reference) and
 * the lean list in the upper. The materializer walks the production execute's
 * own state machine -- root bind, root light preamble, state spans, materials,
 * the shade's ResolveEpochShade, the run prepare's validation and texture bind
 * -- and packs the words the recorder's hooks would record, except that the
 * matrices take the lean layout. The per-frame patches are the replay's own
 * helpers applied to the active entry.
 * ------------------------------------------------------------------------- */
#define NDS_FTR_LEAN_REGION_WORDS NDS_FIGHTER_PACKET_REGION_WORDS
#define NDS_FTR_LEAN_HALF_WORDS (NDS_FTR_LEAN_REGION_WORDS / 2u)
#define NDS_FTR_LEAN_STRUCT_WORDS \
    ((((u32)sizeof(NDSFighterPacket)) + 31u) / 32u * 8u)
#define NDS_FTR_LEAN_WORD_CAPACITY \
    (NDS_FTR_LEAN_HALF_WORDS - NDS_FTR_LEAN_STRUCT_WORDS)
/* Slice 6: a wide list -- entry 0 over the whole region (route 1). Marked in
 * its entry state's `valid` word (bit 1), so the state costs no field. */
#define NDS_FTR_LEAN_WIDE_CAPACITY \
    (NDS_FTR_LEAN_REGION_WORDS - NDS_FTR_LEAN_STRUCT_WORDS)
#define NDS_FTR_LEAN_VALID_WIDE 2u
#define NDS_FTR_LEAN_ENTRIES 2u
_Static_assert((((sizeof(NDSFighterPacket) + 31u) / 32u) * 8u) <
                   (NDS_FIGHTER_PACKET_ARENA_WORDS /
                    NDS_FIGHTER_PACKET_SLOTS / 2u),
               "a lean entry must fit half of a player region");
_Static_assert(NDS_FTR_LEAN_KEY_WORDS == 6u, "entry key width");

/* The submit cleans only the cache lines a patch wrote. A list is cleaned
 * whole once, when it is materialized; after that only the patch helpers
 * write it, and each marks the 32-byte lines it touched. */
#define NDS_FTR_LEAN_DIRTY_LINES ((NDS_FTR_LEAN_WORD_CAPACITY + 7u) / 8u + 1u)
#define NDS_FTR_LEAN_DIRTY_WORDS ((NDS_FTR_LEAN_DIRTY_LINES + 31u) / 32u)


typedef struct NDSFtrLeanEntryState
{
    u32 valid;
    u32 key[NDS_FTR_LEAN_KEY_WORDS];
    u32 tint_set_generation;    /* the tile set its binds (or folds) saw */
    u32 fence_needed;           /* a textured bind named no cache entry */
    u32 fence;                  /* ... the texture fence it was bound at */
    u32 all_pinned;             /* every bound texture admitted / pinned */
    u32 donor_mask;             /* roots drawn from a donor table set */
    u32 light_valid;            /* light word holds light_dir's word */
    s32 light_dir[3];
    u32 texgen_first;           /* word range the texgen sites span */
    u32 texgen_last;
    u32 texgen_map;             /* NDS_FTR_LEAN_TEXGEN_MAP_* */
    u32 tint_folds;             /* tinted epochs that folded (no resident
                                   tile): the list's shape follows the set */
    u32 tint_repatch;           /* the set moved under a fold-free list:
                                   the patch re-points every tint bind */
    u32 variant_count;          /* records in the entry's tail: 0, or the
                                   base (record 0) and its variants */
    u32 variant_cur;            /* the record the words hold */
    u32 variant_max;            /* records the tail has room for */
    u32 variant_next;           /* round-robin replacement (1..max-1) */
    u32 vtx10;                  /* corners VTX_10 (NDS_FTR_LEAN_ENTRY_VTX16) */
    u32 dirty[NDS_FTR_LEAN_DIRTY_WORDS];
} NDSFtrLeanEntryState;

/* Variants. A state whose list has the same shape as an entry's -- the same
 * words at the same places but for a few material words (a blink texture's
 * TEXIMAGE_PARAM, an expression's palette) -- is a record in the entry's word
 * tail rather than a second list: its key, the words it changes (with the
 * base's words there, to switch back), and the texture table and fences it
 * binds under. A record is learned from a full materialization of its state
 * (in the other entry) against the held list, so its words are that
 * materialization's own. */
#define NDS_FTR_LEAN_VARIANT_DIFF_MAX 16u
#define NDS_FTR_LEAN_VARIANT_MAX 8u
typedef struct NDSFtrLeanVariant
{
    u32 key[NDS_FTR_LEAN_KEY_WORDS];
    u32 fence;
    u8 diff_count;
    u8 fence_needed;
    u8 all_pinned;
    u8 pad;
    u16 index[NDS_FTR_LEAN_VARIANT_DIFF_MAX];
    u32 base[NDS_FTR_LEAN_VARIANT_DIFF_MAX];
    u32 value[NDS_FTR_LEAN_VARIANT_DIFF_MAX];
    NDSFighterPacketTexture textures[NDS_FIGHTER_PACKET_TEXTURE_MAX];
} NDSFtrLeanVariant;
#define NDS_FTR_LEAN_VARIANT_WORDS \
    ((u32)((sizeof(NDSFtrLeanVariant) + 3u) / 4u))

/* WIDE LISTS LEARN TOO (2026-10-18). A list past an entry's half (Palutena's
 * 3,215 words against 3,084) takes the whole region, so no other entry can
 * hold a fresh materialization to learn a variant from, and every flip of
 * her blink texture materialized her list again: four Palutenas, 82 a match
 * at ~600K cycles each, all of them in the P95 tail. The region keeps room
 * for one learn instead. Before a wide list is materialized again in place
 * for a key its records lack, its words (at the base record) are copied to
 * the region's tail with the state a record needs (ndsFtrLeanWideHoldBegin);
 * after, the fresh words in place are diffed against that copy
 * (ndsFtrLeanWideLearn) under the same patch-site mask and the same limits
 * as ndsFtrLeanLearnVariant's two entries, and the held state and the fresh
 * one become records of the one list. A wide entry's capacity, top down:
 * its records, the copy, this block, the diff mask; its words from the
 * bottom. The shape test is ndsFtrLeanSameShape's fields as a hash, since
 * the held header is gone by the time the fresh one exists. */
typedef struct NDSFtrLeanWideHold
{
    u32 magic;
    u32 word_count;
    u32 shape;
    u32 variant_count;          /* the held entry's records ... */
    u32 variant_next;
    u32 variant_max;
    u32 key[NDS_FTR_LEAN_KEY_WORDS];    /* ... and its base state */
    u32 fence;
    u32 fence_needed;
    u32 all_pinned;
    u32 texture_count;
    NDSFighterPacketTexture textures[NDS_FIGHTER_PACKET_TEXTURE_MAX];
} NDSFtrLeanWideHold;
#define NDS_FTR_LEAN_WIDE_HOLD_WORDS \
    ((u32)((sizeof(NDSFtrLeanWideHold) + 31u) / 32u * 8u))
#define NDS_FTR_LEAN_WIDE_HOLD_MAGIC 0x57484c44u
/* The words a wide list of `count` words keeps for a learn beyond itself:
 * the copy, the hold block and the mask. */
#define NDS_FTR_LEAN_WIDE_RELEARN_WORDS(count) \
    ((count) + NDS_FTR_LEAN_WIDE_HOLD_WORDS + (((count) + 31u) / 32u))
/* A learn (or the lab verify) masks the patch sites of the fresh list in the
 * top of that entry's own capacity, which holds no record yet. */
#define NDS_FTR_LEAN_MASK_WORDS ((NDS_FTR_LEAN_WORD_CAPACITY + 31u) / 32u)

#define ndsFtrLeanRegionWords() (NDS_FTR_LEAN_REGION_WORDS)
#define ndsFtrLeanHalfWords() (NDS_FTR_LEAN_HALF_WORDS)
#define ndsFtrLeanWordCapacity() (NDS_FTR_LEAN_WORD_CAPACITY)
#define ndsFtrLeanWideCapacity() (NDS_FTR_LEAN_WIDE_CAPACITY)
#define ndsFtrLeanMaskWords() (NDS_FTR_LEAN_MASK_WORDS)

/* The spare (2026-09-27). A slot whose states outnumber its two entries
 * re-materializes a list it held a moment ago (Donkey: 26 of the four-CPU
 * match's 62 materializations; a 3-key LRU model of the key stream says 15).
 * Route 1 gives each such slot one more half-sized buffer from the battle's
 * general heap, and an entry -> buffer map: evicting a valid list parks it in
 * the spare (a map swap, no copy -- a packet's `words` pointer is into its own
 * buffer, so a list never moves), and a miss that finds its key there swaps it
 * back instead of materializing. Entry index 2 of the slot state is the spare;
 * every loop over NDS_FTR_LEAN_ENTRIES leaves it alone. A wide list needs the
 * two halves as entries 0 and 1, so it restores the identity map first. */
#define NDS_FTR_LEAN_SPARE NDS_FTR_LEAN_ENTRIES
/* The GObj cap latches the moment the general heap drops under 25,600 B; the
 * spare and the pool keep that and a margin for what the match takes after
 * them. Until 2026-10-18 the margin was 36,420 B, a match's growth after the
 * stage body's allocation -- which was the optional caches with floors of
 * their own, this spare among them -- so Dinosaur Land's cast (90,964 B free
 * at the start, 79,700 needed) drew the whole match with no spare and no
 * pool: ~92 lists materialized, each one a frame in the P95 tail, against
 * Dream Land's ~46 with three spares; it became the stage body's 12 KB
 * (NDS_STAGE_GX_HEAP_KEEP_FREE).
 * 2026-10-20 (stability audit): 12 KB is a one-minute match's growth, not a
 * long one's. What the source's own pools (GObjs, DObjs, XObjs, MObjs, SObjs,
 * processes, particles) take from the first frame on: 14,780 B in five
 * minutes of four heavy contents on Yoshi's Island with items at the default
 * rate, 19,800 B at Very High, 13,784 B on the campaign's Yoshi Team after
 * Sonic's four spares (its low 24,584). So 24 KB. */
#define NDS_FTR_LEAN_SPARE_KEEP_FREE (25600u + 24576u)
/* THE SPENDS ORDER (P8, 2026-10-24): the motion cache's elastic block sits
 * above the cursor, so `end - ptr` counts it as free and these buffers take
 * its bytes first. Measured the other order (the cache keeping its first
 * 48 KB, gate `s93` against `s92`): the heavy roster's caches grew to
 * 91-106 KB and hit more, but the lean pool lost its bytes and WORK P95 rose
 * 12K-105K on all five stages; the cast flat. The lists win. */

/* THE LEARN'S COPY OFF THE REGION (2026-10-20). A wide list keeping its own
 * copy for a learn needs twice its words below its records: the region's
 * 7,504 hold two records only up to ~3,560 list words. Palutena's grew to
 * 3,698 (her hair shut at the nape, the low's hair whole), her variants went
 * to 0, and every blink flip materialized her list again: four Palutenas 52
 * a 900-frame match against 20 (WORK P95 1,275,456 -> 1,412,608 on Dream
 * Land); her high list (4,588, a duel's or a human's) never had room. The
 * copy, its hold block and its mask only live from ndsFtrLeanWideHoldBegin
 * to ndsFtrLeanWideLearn in one draw, so one buffer a heap generation holds
 * them for every slot (sized to the longest list asked for, in 1,024-word
 * steps, taken under the spare's floor), and the region keeps its words for
 * records. Without it -- no room on the heap -- the copy sits in the region
 * as before. */
#define NDS_FTR_LEAN_WIDE_SCRATCH_STEP 1024u
static u32 *sNdsFtrLeanWideScratch;
static u32 sNdsFtrLeanWideScratchWords;
static u32 sNdsFtrLeanWideScratchGen;

/* The learn's buffer for a list of `count` words, or NULL. `allocate` takes
 * one (a larger one when the held buffer is too small: the heap is a bump
 * arena, so the old one waits for the generation's end) when the heap has
 * its room above the floor. */
static NDS_FIGHTER_PACKET_COLD_CODE u32 *ndsFtrLeanWideScratch(u32 count,
                                                               u32 allocate)
{
    const u32 need = NDS_FTR_LEAN_WIDE_RELEARN_WORDS(count);
    u32 words;
    u32 bytes;
    u32 *buffer;

    if ((sNdsFtrLeanWideScratch != NULL) &&
        (sNdsFtrLeanWideScratchGen != gNdsTaskmanHeapGeneration))
    {
        sNdsFtrLeanWideScratch = NULL;
        sNdsFtrLeanWideScratchWords = 0u;
    }
    if ((sNdsFtrLeanWideScratch != NULL) && (sNdsFtrLeanWideScratchWords >= need))
    {
        return sNdsFtrLeanWideScratch;
    }
    if ((allocate == FALSE) || (count > NDS_FTR_LEAN_WIDE_CAPACITY))
    {
        return NULL;
    }
    words = (need + NDS_FTR_LEAN_WIDE_SCRATCH_STEP - 1u) &
            ~(NDS_FTR_LEAN_WIDE_SCRATCH_STEP - 1u);
    bytes = words * (u32)sizeof(u32);
    if (((uintptr_t)gSYTaskmanGeneralHeap.end <
         (uintptr_t)gSYTaskmanGeneralHeap.ptr) ||
        (((uintptr_t)gSYTaskmanGeneralHeap.end -
          (uintptr_t)gSYTaskmanGeneralHeap.ptr) <
         bytes + NDS_FTR_LEAN_SPARE_KEEP_FREE) ||
        ((buffer = syTaskmanMalloc(bytes, 32u)) == NULL))
    {
        return NULL;
    }
    sNdsFtrLeanWideScratch = buffer;
    sNdsFtrLeanWideScratchWords = words;
    sNdsFtrLeanWideScratchGen = gNdsTaskmanHeapGeneration;
    return buffer;
}

typedef struct NDSFtrLeanSlotState
{
    NDSFtrLeanEntryState entry[NDS_FTR_LEAN_ENTRIES + 1u]; /* [2] the spare */
    u32 active;                 /* entry index, NDS_FTR_LEAN_ENTRY_NONE */
    u32 kind;                   /* counters' kind index, NONE = unknown */
    u32 lower_owned;            /* route 1: entry 0 holds a lean list */
    u32 *spare_buf;             /* general heap, this heap generation */
    u32 spare_gen;              /* gNdsTaskmanHeapGeneration it came from */
    u8 phys[NDS_FTR_LEAN_ENTRIES + 1u]; /* entry -> buffer (2 = spare_buf) */
    u8 phys_on;                 /* 0: the identity map */
    /* The spare grew this struct by 168 B a slot; 88 more make the array's
     * growth exactly 1,024 B, one D-cache way (4 KB, 4-way, 32 B lines),
     * so every BSS object behind it keeps the cache set it was measured
     * at. Unpadded, the same code read P50 +8K with SRC +4K from phase
     * alone; padded, +3K. (76 since the three entries' vtx10 words.) */
    u8 layout_pad[76];
#if NDS_FTR_LEAN_LAB
    /* census: every cache slot this fighter's lists or packets bound */
    u32 union_mask[(NDS_RENDERER_HW_TEXTURE_CACHE_COUNT + 31u) / 32u];
#endif
} NDSFtrLeanSlotState;

static NDSFtrLeanSlotState sNdsFtrLeanSlots[NDS_FIGHTER_PACKET_SLOTS];
static void ndsFtrLeanSpareIdentity(NDSFtrLeanSlotState *s);
static void ndsFtrLeanPoolDrop(u32 battle_slot);

static inline void ndsFtrLeanMarkDirty(NDSFtrLeanEntryState *state,
                                       const u32 *words, u32 first,
                                       u32 count)
{
    uintptr_t base = (uintptr_t)words & ~(uintptr_t)31u;
    u32 line = (u32)(((uintptr_t)&words[first] - base) >> 5);
    u32 last = (u32)(((uintptr_t)&words[first + count - 1u] - base) >> 5);

    for (; (line <= last) && (line < NDS_FTR_LEAN_DIRTY_LINES); line++)
    {
        state->dirty[line >> 5] |= 1u << (line & 31u);
    }
}

/* Clean every marked line (runs of consecutive lines in one call) and clear
 * the marks. Returns the bytes cleaned. */
static u32 NDS_FTR_LEAN_DRAW_CODE
ndsFtrLeanFlushDirty(NDSFtrLeanEntryState *state, const u32 *words)
{
    uintptr_t base = (uintptr_t)words & ~(uintptr_t)31u;
    u32 bytes = 0u;
    u32 w;

    for (w = 0u; w < NDS_FTR_LEAN_DIRTY_WORDS; w++)
    {
        u32 bits = state->dirty[w];

        state->dirty[w] = 0u;
        while (bits != 0u)
        {
            u32 first = (u32)__builtin_ctz(bits);
            u32 shifted = bits >> first;
            u32 run = (shifted == 0xffffffffu) ? 32u :
                (u32)__builtin_ctz(~shifted);
            u32 len;

            len = run << 5;
            DC_FlushRange((const void *)(base + (((w << 5) + first) << 5)),
                          len);
            bytes += len;
            bits = (run + first >= 32u) ? 0u :
                (bits & ~(((1u << run) - 1u) << first));
        }
    }
    return bytes;
}

static u32 *ndsFtrLeanEntryBase(u32 slot, u32 entry)
{
    {
        const NDSFtrLeanSlotState *s = &sNdsFtrLeanSlots[slot];
        u32 buffer = (s->phys_on != 0u) ? (u32)s->phys[entry] : entry;

        if (buffer >= NDS_FTR_LEAN_SPARE)
        {
            return s->spare_buf;
        }
        return (u32 *)(void *)&gSYFramebufferSets[0][0][0] +
            (slot * ndsFtrLeanRegionWords()) + (buffer * ndsFtrLeanHalfWords());
    }
}

static NDSFighterPacket *ndsFtrLeanEntryPacket(u32 slot, u32 entry)
{
    return (NDSFighterPacket *)(void *)ndsFtrLeanEntryBase(slot, entry);
}

static u32 *ndsFtrLeanEntryWords(u32 slot, u32 entry)
{
    return ndsFtrLeanEntryBase(slot, entry) + NDS_FTR_LEAN_STRUCT_WORDS;
}

/* Record `record` of an entry: the tail of its word capacity, top down (a
 * wide entry's, the whole region's). */
static NDSFtrLeanVariant *ndsFtrLeanVariantRecord(u32 slot, u32 entry,
                                                  u32 record)
{
    const u32 capacity = ((entry < NDS_FTR_LEAN_ENTRIES) &&
                          ((sNdsFtrLeanSlots[slot].entry[entry].valid &
                            NDS_FTR_LEAN_VALID_WIDE) != 0u)) ?
        ndsFtrLeanWideCapacity() : ndsFtrLeanWordCapacity();

    return (NDSFtrLeanVariant *)(void *)(ndsFtrLeanEntryWords(slot, entry) +
        capacity - ((record + 1u) * NDS_FTR_LEAN_VARIANT_WORDS));
}

/* The active entry's list and state, or NULL. */
/* 2026-09-27: out of ITCM (hot callers inline it; the out-of-line copy
 * ran ~2 ticks a frame in the four-CPU census). */
static NDSFighterPacket *ndsFtrLeanActive(u32 slot,
                                          NDSFtrLeanEntryState **state)
{
    NDSFtrLeanSlotState *s;

    if (slot >= NDS_FIGHTER_PACKET_SLOTS)
    {
        return NULL;
    }
    s = &sNdsFtrLeanSlots[slot];
    if ((s->active >= NDS_FTR_LEAN_ENTRIES) ||
        (s->entry[s->active].valid == 0u))
    {
        return NULL;
    }
    *state = &s->entry[s->active];
    return ndsFtrLeanEntryPacket(slot, s->active);
}

static inline u32 ndsFtrLeanEntryUsable(u32 entry)
{
    return (entry < NDS_FTR_LEAN_ENTRIES) ? TRUE : FALSE;
}

#if NDS_FTR_LEAN_LAB
static u32 ndsFtrLeanOwnerIsFighter(void)
{
    u32 owner = (u32)sNdsRendererRuntimeOwner;

    return ((owner != (u32)NDS_RENDERER_PROFILE_OWNER_STAGE) &&
            (owner < (u32)NDS_RENDERER_PROFILE_OWNER_COUNT)) ? TRUE : FALSE;
}

/* TEXIMAGE_PARAM (entry->params is glGetTexParameter()): texel bytes only. */
static u32 ndsFtrLeanTextureBytes(u32 params)
{
    static const u8 bits_per_texel[8] = { 0u, 8u, 2u, 4u, 8u, 2u, 8u, 16u };
    u32 width = 8u << ((params >> 20) & 7u);
    u32 height = 8u << ((params >> 23) & 7u);

    return (width * height * (u32)bits_per_texel[(params >> 26) & 7u]) / 8u;
}
#endif

void ndsFtrLeanPacketDrop(u32 battle_slot)
{
    NDSFtrLeanSlotState *s;
    u32 e;

    if (battle_slot >= NDS_FIGHTER_PACKET_SLOTS)
    {
        return;
    }
    s = &sNdsFtrLeanSlots[battle_slot];
    for (e = 0u; e < NDS_FTR_LEAN_ENTRIES; e++)
    {
        s->entry[e].valid = 0u;
    }
    s->active = NDS_FTR_LEAN_ENTRY_NONE;
    /* The identity map, no spare list, and no buffer from an older heap
     * generation. */
    s->lower_owned = 0u;
    s->entry[NDS_FTR_LEAN_SPARE].valid = 0u;
    s->phys_on = 0u;
    if (s->spare_gen != gNdsTaskmanHeapGeneration)
    {
        s->spare_buf = NULL;
    }
    ndsFtrLeanPoolDrop(battle_slot);
}

void ndsFtrLeanPacketNoteKind(u32 battle_slot, u32 kind)
{
    if (battle_slot < NDS_FIGHTER_PACKET_SLOTS)
    {
        sNdsFtrLeanSlots[battle_slot].kind = kind;
    }
}

/* ---- Link's texgen words with the unique-normal slots resolved ----------
 * ndsFighterPacketPatchTexgen derives each group's S/T word once per unique
 * dense normal, finding a site's normal by a linear search of the group's
 * cache at EVERY site on EVERY draw. The site -> slot map is a property of the
 * list, so it is resolved once, when the list is materialized, into the
 * entry's tail after its words; the per-draw pass is then the same arithmetic
 * on the same helpers and a table copy. */
#if NDS_LINK
/* Link's lists hold at most this many groups; the map stays this size when
 * a list's group array grows (32 since 2026-10-05, for the 1P owners' GX
 * texgen, which the map never covers). */
#define NDS_FTR_LEAN_TEXGEN_MAP_GROUPS 8u
typedef struct NDSFtrLeanTexgenMap
{
    u8 slot_count[NDS_FTR_LEAN_TEXGEN_MAP_GROUPS];
    u16 dense[NDS_FTR_LEAN_TEXGEN_MAP_GROUPS]
             [NDS_FIGHTER_PACKET_TEXGEN_DENSE_MAX];
    u8 site_slot[NDS_FIGHTER_PACKET_TEXGEN_SITE_MAX];
} NDSFtrLeanTexgenMap;
#define NDS_FTR_LEAN_TEXGEN_MAP_WORDS \
    ((u32)((sizeof(NDSFtrLeanTexgenMap) + 3u) / 4u))
#endif
#define NDS_FTR_LEAN_TEXGEN_MAP_NONE 0u      /* not built: generic patch */
#define NDS_FTR_LEAN_TEXGEN_MAP_READY 1u
#define NDS_FTR_LEAN_TEXGEN_MAP_REFUSE 2u    /* the generic patch would fail */

#if NDS_LINK
static NDSFtrLeanTexgenMap *ndsFtrLeanTexgenMapOf(const NDSFighterPacket *lean)
{
    u32 offset = ((u32)lean->word_count + 7u) & ~7u;

    /* Slice 6: the list's own capacity (an entry's, or a wide list's). */
    if ((offset + NDS_FTR_LEAN_TEXGEN_MAP_WORDS) > (u32)lean->word_capacity)
    {
        return NULL;
    }
    return (NDSFtrLeanTexgenMap *)(void *)(lean->words + offset);
}

/* Resolve every texgen site to its group's unique-normal slot, in the order
 * ndsFighterPacketPatchTexgen's cache fills. NONE leaves the generic patch in
 * charge (it fails on anything this refuses to map); REFUSE is its own
 * cache-overflow failure, known in advance. */
static u32 ndsFtrLeanBuildTexgenMap(const NDSFighterPacket *lean)
{
    NDSFtrLeanTexgenMap *map = ndsFtrLeanTexgenMapOf(lean);
    u32 g;

    if ((lean->texgen_group_count == 0u) || (map == NULL) ||
        (lean->texgen_group_count > NDS_FTR_LEAN_TEXGEN_MAP_GROUPS) ||
        (lean->texgen_site_count > NDS_FIGHTER_PACKET_TEXGEN_SITE_MAX))
    {
        return NDS_FTR_LEAN_TEXGEN_MAP_NONE;
    }
    for (g = 0u; g < (u32)lean->texgen_group_count; g++)
    {
        const NDSFighterPacketTexgenGroup *group = &lean->texgen_groups[g];
        u32 first = group->first_site;
        u32 end = first + group->site_count;
        u32 count = 0u;
        u32 s;

        if (group->reserved[1] != 0u)
        {
            /* A GX texgen group: the generic patch fills its matrix. */
            return NDS_FTR_LEAN_TEXGEN_MAP_NONE;
        }

        if ((end > (u32)lean->texgen_site_count) ||
            (end > NDS_FIGHTER_PACKET_TEXGEN_SITE_MAX))
        {
            return NDS_FTR_LEAN_TEXGEN_MAP_NONE;
        }
        for (s = first; s < end; s++)
        {
            u32 dense_id = lean->texgen_sites[s].dense_id;
            u32 k;

            for (k = 0u; k < count; k++)
            {
                if ((u32)map->dense[g][k] == dense_id)
                {
                    break;
                }
            }
            if (k == count)
            {
                if (count >= NDS_FIGHTER_PACKET_TEXGEN_DENSE_MAX)
                {
                    return NDS_FTR_LEAN_TEXGEN_MAP_REFUSE;
                }
                map->dense[g][count++] = (u16)dense_id;
            }
            map->site_slot[s] = (u8)k;
        }
        map->slot_count[g] = (u8)count;
    }
    return NDS_FTR_LEAN_TEXGEN_MAP_READY;
}

/* ndsFighterPacketPatchTexgen with the slots pre-resolved: the same checks
 * (a list the generic patch would refuse, this refuses), the same helpers
 * and casts per unique normal, then one table copy per site. */
static s32 ndsFtrLeanPatchTexgenMapped(
    NDSFighterPacket *packet, const NDSFtrLeanTexgenMap *map,
    const NDSRendererMatrix20p12 *worlds, u32 input_count)
{
    const LookAt *look_at;
    u32 group_index;

    if (packet->texgen_group_count == 0u)
    {
        return TRUE;
    }
    if ((packet->texgen_group_count > NDS_FTR_LEAN_TEXGEN_MAP_GROUPS) ||
        (packet->texgen_site_count > NDS_FIGHTER_PACKET_TEXGEN_SITE_MAX) ||
        (sNdsNativeFighterActiveTables == NULL) || (worlds == NULL))
    {
        return FALSE;
    }
    look_at = ndsRendererAdapterCurrentLookAt();
    if (look_at == NULL)
    {
        return FALSE;
    }
    for (group_index = 0u;
         group_index < (u32)packet->texgen_group_count;
         group_index++)
    {
        const NDSFighterPacketTexgenGroup *group =
            &packet->texgen_groups[group_index];
        NDSNativeTexgenDirectionQ15 lookat_x;
        NDSNativeTexgenDirectionQ15 lookat_y;
        u32 st[NDS_FIGHTER_PACKET_TEXGEN_DENSE_MAX];
        u32 first_site = group->first_site;
        u32 site_end = first_site + group->site_count;
        u32 slot;
        u32 site_index;

        if (((u32)group->root >= input_count) ||
            (site_end > (u32)packet->texgen_site_count) ||
            (site_end > NDS_FIGHTER_PACKET_TEXGEN_SITE_MAX) ||
            (ndsRendererNativePrepareTexgenDirectionQ15(
                 &look_at->l[0], &worlds[group->root],
                 &lookat_x) == FALSE) ||
            (ndsRendererNativePrepareTexgenDirectionQ15(
                 &look_at->l[1], &worlds[group->root],
                 &lookat_y) == FALSE))
        {
            return FALSE;
        }
        for (slot = 0u; slot < (u32)map->slot_count[group_index]; slot++)
        {
            u32 dense_id = map->dense[group_index][slot];
            u32 rgba;
            s32 nx;
            s32 ny;
            s32 nz;
            s32 scaled_s;
            s32 scaled_t;
            s16 s;
            s16 t;

            if (dense_id >= sNdsNativeFighterActiveTables->dense_count)
            {
                return FALSE;
            }
            rgba = ndsFighterPacketTexgenRgba(sNdsNativeFighterActiveTables,
                                              dense_id);
            nx = (s32)(s8)(rgba >> 24);
            ny = (s32)(s8)(rgba >> 16);
            nz = (s32)(s8)(rgba >> 8);
            scaled_s = ndsRendererNativeTexgenCoord(
                nx, ny, nz, &lookat_x, group->scale_s, group->reserved[0]);
            scaled_t = ndsRendererNativeTexgenCoord(
                nx, ny, nz, &lookat_y, group->scale_t, group->reserved[0]);
            s = (s16)(scaled_s - ((s32)group->origin_s << 2) +
                      group->offset);
            t = (s16)(scaled_t - ((s32)group->origin_t << 2) +
                      group->offset);
            st[slot] = (u32)(u16)s | ((u32)(u16)t << 16);
        }
        for (site_index = first_site; site_index < site_end; site_index++)
        {
            const NDSFighterPacketTexgenSite *site =
                &packet->texgen_sites[site_index];

            if (site->index >= packet->word_count)
            {
                return FALSE;
            }
            packet->words[site->index] = st[map->site_slot[site_index]];
        }
    }
    return TRUE;
}
#endif
