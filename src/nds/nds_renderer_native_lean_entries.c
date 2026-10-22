/* Native renderer, the lean entries: the two per-fighter entries, the parked-list
 * pool behind the spare, variant switches, wide lists and their learn, and the
 * per-draw packet patch. A slice of nds_renderer.c. */

/* ---- slice 4: the two entries -------------------------------------------- */

static inline u32 ndsFtrLeanKeyEqual(const u32 *a, const u32 *b)
{
    u32 i;

    for (i = 0u; i < NDS_FTR_LEAN_KEY_WORDS; i++)
    {
        if (a[i] != b[i])
        {
            return FALSE;
        }
    }
    return TRUE;
}

static inline u32 ndsFtrLeanFenceNow(void)
{
    return sNdsRendererHardwareTextureKeyGeneration ^
        (sNdsRendererRuntimeTextureCacheEvictCount << 16);
}

/* A valid entry whose list no longer matches the live state for its own
 * key: a fold under a moved tint-tile set (the fold may bind now), or a
 * texture fence that moved under a list that needs it. A fold-free list
 * under a moved set is current -- the patch re-points its binds. */
static u32 ndsFtrLeanEntryStale(const NDSFtrLeanEntryState *es, u32 fence)
{
    return (((es->tint_set_generation != gNdsR2FighterTintSetGeneration) &&
             (es->tint_folds != 0u)) ||
            ((es->fence_needed != 0u) && (es->fence != fence))) ?
        TRUE : FALSE;
}

/* Entry `e`'s code for `key` (its words, or one of its records), or NONE. The
 * spare (e == NDS_FTR_LEAN_SPARE) is matched by the same rules. */
static u32 ndsFtrLeanEntryMatch(u32 battle_slot, u32 e, const u32 *key,
                                u32 fence)
{
    const NDSFtrLeanEntryState *es = &sNdsFtrLeanSlots[battle_slot].entry[e];
    u32 r;

    if ((es->valid == 0u) ||
        ((es->tint_set_generation != gNdsR2FighterTintSetGeneration) &&
         (es->tint_folds != 0u)))
    {
        return NDS_FTR_LEAN_ENTRY_NONE;
    }
    if (((es->fence_needed == 0u) || (es->fence == fence)) &&
        (ndsFtrLeanKeyEqual(es->key, key) != FALSE))
    {
        return e;
    }
    for (r = 0u; r < es->variant_count; r++)
    {
        const NDSFtrLeanVariant *v =
            ndsFtrLeanVariantRecord(battle_slot, e, r);

        if ((r != es->variant_cur) &&
            ((v->fence_needed == 0u) || (v->fence == fence)) &&
            (ndsFtrLeanKeyEqual(v->key, key) != FALSE))
        {
            return e | ((r + 1u) << 4);
        }
    }
    return NDS_FTR_LEAN_ENTRY_NONE;
}

u32 ndsFtrLeanEntryFind(u32 battle_slot, const u32 *key)
{
    NDSFtrLeanSlotState *s;
    u32 fence;
    u32 pass;

    if ((battle_slot >= NDS_FIGHTER_PACKET_SLOTS) || (key == NULL))
    {
        return NDS_FTR_LEAN_ENTRY_NONE;
    }
    s = &sNdsFtrLeanSlots[battle_slot];
    fence = ndsFtrLeanFenceNow();
    /* The active entry first: the state its words hold, then its records. */
    for (pass = 0u; pass < NDS_FTR_LEAN_ENTRIES; pass++)
    {
        u32 e = (s->active < NDS_FTR_LEAN_ENTRIES) ?
            ((pass == 0u) ? s->active : (s->active ^ 1u)) : pass;
        u32 code;

        if (ndsFtrLeanEntryUsable(e) == FALSE)
        {
            continue;
        }
        code = ndsFtrLeanEntryMatch(battle_slot, e, key, fence);
        if (code != NDS_FTR_LEAN_ENTRY_NONE)
        {
            return code;
        }
    }
    return NDS_FTR_LEAN_ENTRY_NONE;
}

/* Swap entry `e`'s list, state and buffer with the spare's. */
static void ndsFtrLeanSpareSwap(NDSFtrLeanSlotState *s, u32 e)
{
    NDSFtrLeanEntryState state = s->entry[e];
    u8 buffer;

    if (s->phys_on == 0u)
    {
        s->phys[0] = 0u;
        s->phys[1] = 1u;
        s->phys[NDS_FTR_LEAN_SPARE] = (u8)NDS_FTR_LEAN_SPARE;
        s->phys_on = 1u;
    }
    buffer = s->phys[e];
    s->entry[e] = s->entry[NDS_FTR_LEAN_SPARE];
    s->entry[NDS_FTR_LEAN_SPARE] = state;
    s->phys[e] = s->phys[NDS_FTR_LEAN_SPARE];
    s->phys[NDS_FTR_LEAN_SPARE] = buffer;
    if (s->active == e)
    {
        s->active = NDS_FTR_LEAN_ENTRY_NONE;
    }
}

/* A wide list is entry 0 over both halves: put the identity map back. The
 * list the spare buffer holds (whichever entry maps to it) stays as the spare;
 * the two halves' lists are about to be overwritten anyway. */
static void ndsFtrLeanSpareIdentity(NDSFtrLeanSlotState *s)
{
    u32 e;

    if (s->phys_on == 0u)
    {
        return;
    }
    for (e = 0u; e < NDS_FTR_LEAN_ENTRIES; e++)
    {
        if (s->phys[e] == (u8)NDS_FTR_LEAN_SPARE)
        {
            s->entry[NDS_FTR_LEAN_SPARE] = s->entry[e];
        }
        s->entry[e].valid = 0u;
        if (s->active == e)
        {
            s->active = NDS_FTR_LEAN_ENTRY_NONE;
        }
    }
    s->phys_on = 0u;
}

/* ---- the parked-list pool behind the spare (P2-2p8, 2026-10-06) -----------
 * A slot's states outnumber its two entries and spare: on the four-CPU lab
 * match 91% of the lean key events name a key the slot has drawn before, yet
 * 4 x Yoshi materializes 75 lists for 36 distinct keys and Peach's Castle 45
 * for 26 -- every one of the others re-derives a list the slot held earlier
 * and lost (~300-500K ticks each, the over-gate frames' largest premium).
 * The spare keeps one evicted list; the pool keeps the lists the spare is
 * about to lose: the entry state, the packet, its words (and Link's texgen
 * map after them) and its variant records, copied compactly into one of a
 * few fixed slots from the battle's general heap. A miss whose key a parked
 * copy holds (by the entry rules: tint set, texture fence, key, records)
 * copies it back into the victim and re-bases its words -- ~10K cycles of
 * copying instead of a materialization. The lab pool of 2026-10-05
 * (`artifacts/performance/2026-10-05_lean-park`) replaced the spare where the
 * heap refused it; this one stands behind it. Sized once per heap generation
 * at its first use, leaving the spare floor and every slot's pending spare.
 * 2026-10-12: up to sixteen (was eight) where the heap has them: four Meta
 * Knights still re-derived 43 of 93 lists with the pool full at eight. */
#define NDS_FTR_LEAN_POOL_MAX 16u
#define NDS_FTR_LEAN_POOL_FREE 0xffu
typedef struct NDSFtrLeanPoolHead
{
    u32 slot;                   /* battle slot, or NDS_FTR_LEAN_POOL_FREE */
    u32 stamp;                  /* LRU: the park serial */
    u32 list_words;             /* words copied after the packet (the floor) */
    u32 pad;
    NDSFtrLeanEntryState state;
} NDSFtrLeanPoolHead;
#define NDS_FTR_LEAN_POOL_HEAD_WORDS \
    ((((u32)sizeof(NDSFtrLeanPoolHead)) + 31u) / 32u * 8u)
#define NDS_FTR_LEAN_POOL_SLOT_WORDS \
    (NDS_FTR_LEAN_POOL_HEAD_WORDS + NDS_FTR_LEAN_HALF_WORDS)

static u32 *sNdsFtrLeanPoolBase;
static u32 sNdsFtrLeanPoolCount;
static u32 sNdsFtrLeanPoolGen;  /* gNdsTaskmanHeapGeneration + 1 sized */
static u32 sNdsFtrLeanPoolStamp;

static inline NDSFtrLeanPoolHead *ndsFtrLeanPoolAt(u32 i)
{
    return (NDSFtrLeanPoolHead *)(void *)(sNdsFtrLeanPoolBase +
                                          (i * NDS_FTR_LEAN_POOL_SLOT_WORDS));
}

static u32 __attribute__((noinline, cold)) ndsFtrLeanPoolSize(void)
{
    const u32 slot_bytes = NDS_FTR_LEAN_POOL_SLOT_WORDS * (u32)sizeof(u32);
    const u32 spare_bytes = ndsFtrLeanHalfWords() * (u32)sizeof(u32);
    u32 reserve = NDS_FTR_LEAN_SPARE_KEEP_FREE;
    u32 free_bytes;
    u32 count;
    u32 i;

    sNdsFtrLeanPoolGen = gNdsTaskmanHeapGeneration + 1u;
    sNdsFtrLeanPoolBase = NULL;
    sNdsFtrLeanPoolCount = 0u;
    for (i = 0u; i < NDS_FIGHTER_PACKET_SLOTS; i++)
    {
        if ((sNdsFtrLeanSlots[i].spare_buf == NULL) ||
            (sNdsFtrLeanSlots[i].spare_gen != gNdsTaskmanHeapGeneration))
        {
            reserve += spare_bytes;     /* that slot's spare may still come */
        }
    }
    free_bytes = ((uintptr_t)gSYTaskmanGeneralHeap.end >
                  (uintptr_t)gSYTaskmanGeneralHeap.ptr) ?
        (u32)((uintptr_t)gSYTaskmanGeneralHeap.end -
              (uintptr_t)gSYTaskmanGeneralHeap.ptr) : 0u;
    count = (free_bytes > reserve) ? ((free_bytes - reserve) / slot_bytes) :
        0u;
    if (count > NDS_FTR_LEAN_POOL_MAX)
    {
        count = NDS_FTR_LEAN_POOL_MAX;
    }
    if (count != 0u)
    {
        u32 *base = syTaskmanMalloc(count * slot_bytes, 32u);

        if (base != NULL)
        {
            sNdsFtrLeanPoolBase = base;
            sNdsFtrLeanPoolCount = count;
            for (i = 0u; i < count; i++)
            {
                ndsFtrLeanPoolAt(i)->slot = NDS_FTR_LEAN_POOL_FREE;
            }
        }
    }
    return sNdsFtrLeanPoolCount;
}

static inline u32 ndsFtrLeanPoolReady(void)
{
    if (sNdsFtrLeanPoolGen != gNdsTaskmanHeapGeneration + 1u)
    {
        (void)ndsFtrLeanPoolSize();
    }
    return (sNdsFtrLeanPoolCount != 0u) ? TRUE : FALSE;
}

/* ndsFtrLeanPacketDrop's rule for a slot's parked copies. */
static void ndsFtrLeanPoolDrop(u32 battle_slot)
{
    u32 i;

    if (sNdsFtrLeanPoolGen != gNdsTaskmanHeapGeneration + 1u)
    {
        return;
    }
    for (i = 0u; i < sNdsFtrLeanPoolCount; i++)
    {
        NDSFtrLeanPoolHead *h = ndsFtrLeanPoolAt(i);

        if (h->slot == battle_slot)
        {
            h->slot = NDS_FTR_LEAN_POOL_FREE;
        }
    }
}

/* The words after the packet a list owns: its words, and Link's texgen site
 * map right after them (the variant floor, ndsFtrLeanMaterialize). */
static u32 ndsFtrLeanPoolFloor(const NDSFtrLeanEntryState *es,
                               const NDSFighterPacket *packet)
{
    u32 floor = packet->word_count;

#if NDS_P2_LINK
    if (es->texgen_map != NDS_FTR_LEAN_TEXGEN_MAP_NONE)
    {
        floor = ((floor + 7u) & ~7u) + NDS_FTR_LEAN_TEXGEN_MAP_WORDS;
    }
#else
    (void)es;
#endif
    return floor;
}

/* ndsFtrLeanEntryMatch over a parked copy (its records keep their top-down
 * order after the words): 0 or a record's bits, or NONE. */
static u32 ndsFtrLeanPoolMatch(const NDSFtrLeanPoolHead *h, const u32 *key,
                               u32 fence)
{
    const NDSFtrLeanEntryState *es = &h->state;
    const u32 *tail;
    u32 r;

    if ((es->valid == 0u) ||
        ((es->tint_set_generation != gNdsR2FighterTintSetGeneration) &&
         (es->tint_folds != 0u)))
    {
        return NDS_FTR_LEAN_ENTRY_NONE;
    }
    if (((es->fence_needed == 0u) || (es->fence == fence)) &&
        (ndsFtrLeanKeyEqual(es->key, key) != FALSE))
    {
        return 0u;
    }
    tail = (const u32 *)(const void *)h + NDS_FTR_LEAN_POOL_HEAD_WORDS +
        NDS_FTR_LEAN_STRUCT_WORDS + h->list_words +
        (es->variant_count * NDS_FTR_LEAN_VARIANT_WORDS);
    for (r = 0u; r < es->variant_count; r++)
    {
        const NDSFtrLeanVariant *v = (const NDSFtrLeanVariant *)(const void *)
            (tail - ((r + 1u) * NDS_FTR_LEAN_VARIANT_WORDS));

        if ((r != es->variant_cur) &&
            ((v->fence_needed == 0u) || (v->fence == fence)) &&
            (ndsFtrLeanKeyEqual(v->key, key) != FALSE))
        {
            return (r + 1u) << 4;
        }
    }
    return NDS_FTR_LEAN_ENTRY_NONE;
}

static NDSFtrLeanPoolHead *ndsFtrLeanPoolFind(u32 battle_slot, const u32 *key,
                                              u32 fence, u32 *bits)
{
    u32 i;

    for (i = 0u; i < sNdsFtrLeanPoolCount; i++)
    {
        NDSFtrLeanPoolHead *h = ndsFtrLeanPoolAt(i);

        if (h->slot == battle_slot)
        {
            const u32 b = ndsFtrLeanPoolMatch(h, key, fence);

            if (b != NDS_FTR_LEAN_ENTRY_NONE)
            {
                *bits = b;
                return h;
            }
        }
    }
    return NULL;
}

/* Copy entry `e`'s valid list (any buffer the map gives it) into a free pool
 * slot, else the least recently parked one -- never `keep`, the copy about to
 * be revived. */
static void __attribute__((noinline)) ndsFtrLeanPoolStore(
    u32 battle_slot, u32 e, const NDSFtrLeanPoolHead *keep)
{
    const NDSFtrLeanEntryState *es = &sNdsFtrLeanSlots[battle_slot].entry[e];
    const u32 *src;
    NDSFtrLeanPoolHead *park = NULL;
    u32 floor;
    u32 records;
    u32 i;

    if ((es->valid == 0u) || ((es->valid & NDS_FTR_LEAN_VALID_WIDE) != 0u))
    {
        return;
    }
    src = ndsFtrLeanEntryBase(battle_slot, e);
    floor = ndsFtrLeanPoolFloor(es, (const NDSFighterPacket *)(const void *)src);
    records = es->variant_count * NDS_FTR_LEAN_VARIANT_WORDS;
    if ((floor + records) > ndsFtrLeanWordCapacity())
    {
        return;
    }
    for (i = 0u; i < sNdsFtrLeanPoolCount; i++)
    {
        NDSFtrLeanPoolHead *h = ndsFtrLeanPoolAt(i);

        if (h == keep)
        {
            continue;
        }
        if (h->slot == NDS_FTR_LEAN_POOL_FREE)
        {
            park = h;
            break;
        }
        if ((park == NULL) || ((s32)(h->stamp - park->stamp) < 0))
        {
            park = h;
        }
    }
    if (park == NULL)
    {
        return;
    }
    {
        u32 *dst = (u32 *)(void *)park + NDS_FTR_LEAN_POOL_HEAD_WORDS;

        memcpy(dst, src, (NDS_FTR_LEAN_STRUCT_WORDS + floor) * sizeof(u32));
        if (records != 0u)
        {
            memcpy(dst + NDS_FTR_LEAN_STRUCT_WORDS + floor,
                   src + NDS_FTR_LEAN_STRUCT_WORDS +
                       ndsFtrLeanWordCapacity() - records,
                   records * sizeof(u32));
        }
        NDS_FTR_LEAN_CTR(gNdsFtrLean.pool_store_words +=
                             NDS_FTR_LEAN_STRUCT_WORDS + floor + records);
    }
    park->state = *es;
    park->list_words = floor;
    park->slot = battle_slot;
    park->stamp = ++sNdsFtrLeanPoolStamp;
    NDS_FTR_LEAN_CTR(gNdsFtrLean.spare_parks++);
}

/* Copy a parked list back as entry `victim` and free its pool slot; returns
 * the entry code (the victim, with the record's bits). */
static u32 __attribute__((noinline)) ndsFtrLeanPoolRevive(
    u32 battle_slot, u32 victim, NDSFtrLeanPoolHead *found, u32 bits)
{
    NDSFtrLeanEntryState *ves = &sNdsFtrLeanSlots[battle_slot].entry[victim];
    u32 *dst = ndsFtrLeanEntryBase(battle_slot, victim);
    const u32 *src = (const u32 *)(const void *)found +
        NDS_FTR_LEAN_POOL_HEAD_WORDS;
    const u32 floor = found->list_words;
    const u32 records = found->state.variant_count * NDS_FTR_LEAN_VARIANT_WORDS;
    NDSFighterPacket *packet;

    /* Nothing may still be streaming the victim's old words. */
    ndsFighterPacketDmaWait();
    NDS_FTR_LEAN_CTR(gNdsFtrLean.pool_revive_words +=
                         NDS_FTR_LEAN_STRUCT_WORDS + floor + records);
    memcpy(dst, src, (NDS_FTR_LEAN_STRUCT_WORDS + floor) * sizeof(u32));
    if (records != 0u)
    {
        memcpy(dst + NDS_FTR_LEAN_STRUCT_WORDS + ndsFtrLeanWordCapacity() -
                   records,
               src + NDS_FTR_LEAN_STRUCT_WORDS + floor,
               records * sizeof(u32));
    }
    *ves = found->state;
    memset(ves->dirty, 0, sizeof(ves->dirty));
    packet = (NDSFighterPacket *)(void *)dst;
    packet->words = dst + NDS_FTR_LEAN_STRUCT_WORDS;
    /* The DMA reads the words from memory: clean them, as a fresh
     * materialization does. */
    DC_FlushRange(packet->words, packet->word_count * sizeof(u32));
    found->slot = NDS_FTR_LEAN_POOL_FREE;
    NDS_FTR_LEAN_CTR(gNdsFtrLean.spare_hits++);
    return victim | bits;
}

/* The miss path, with `victim` chosen: the code of the spare's list for `key`,
 * swapped in as `victim` (NONE when the spare does not hold it), else the
 * victim's valid list parked in the spare before it is overwritten -- the
 * spare's own list going to the pool first -- and `key`'s parked copy, if the
 * pool holds one, copied back as the victim. */
u32 ndsFtrLeanSpareTake(u32 battle_slot, u32 victim, const u32 *key)
{
    NDSFtrLeanSlotState *s;
    NDSFtrLeanPoolHead *pooled = NULL;
    u32 pooled_bits = 0u;
    u32 pool;
    u32 code;

    if ((battle_slot >= NDS_FIGHTER_PACKET_SLOTS) || (key == NULL) ||
        (victim >= NDS_FTR_LEAN_ENTRIES))
    {
        return NDS_FTR_LEAN_ENTRY_NONE;
    }
    s = &sNdsFtrLeanSlots[battle_slot];
    if ((s->spare_buf != NULL) && (s->spare_gen != gNdsTaskmanHeapGeneration))
    {
        /* The battle's heap was rewound under the map: nothing mapped to the
         * old buffer is a list any more. */
        ndsFtrLeanSpareIdentity(s);
        s->entry[NDS_FTR_LEAN_SPARE].valid = 0u;
        s->spare_buf = NULL;
    }
    if ((s->lower_owned == 0u) || (s->active == victim) ||
        ((s->entry[0].valid & NDS_FTR_LEAN_VALID_WIDE) != 0u) ||
        ((s->entry[1].valid & NDS_FTR_LEAN_VALID_WIDE) != 0u))
    {
        return NDS_FTR_LEAN_ENTRY_NONE;
    }
    if (s->spare_buf != NULL)
    {
        code = ndsFtrLeanEntryMatch(battle_slot, NDS_FTR_LEAN_SPARE, key,
                                    ndsFtrLeanFenceNow());
        if (code != NDS_FTR_LEAN_ENTRY_NONE)
        {
            ndsFtrLeanSpareSwap(s, victim);
            NDS_FTR_LEAN_CTR(gNdsFtrLean.spare_hits++);
            return victim | (code & ~0xfu);
        }
    }
    pool = ndsFtrLeanPoolReady();
    if (pool != FALSE)
    {
        pooled = ndsFtrLeanPoolFind(battle_slot, key, ndsFtrLeanFenceNow(),
                                    &pooled_bits);
    }
    if (s->entry[victim].valid == 0u)
    {
        return (pooled != NULL) ?
            ndsFtrLeanPoolRevive(battle_slot, victim, pooled, pooled_bits) :
            NDS_FTR_LEAN_ENTRY_NONE;
    }
    if (s->spare_buf == NULL)
    {
        u32 bytes = ndsFtrLeanHalfWords() * (u32)sizeof(u32);

        if (((uintptr_t)gSYTaskmanGeneralHeap.end <
             (uintptr_t)gSYTaskmanGeneralHeap.ptr) ||
            ((uintptr_t)gSYTaskmanGeneralHeap.end -
             (uintptr_t)gSYTaskmanGeneralHeap.ptr <
             bytes + NDS_FTR_LEAN_SPARE_KEEP_FREE) ||
            ((s->spare_buf = syTaskmanMalloc(bytes, 32u)) == NULL))
        {
            /* No spare: the pool alone keeps the victim's list. */
            NDS_FTR_LEAN_CTR(gNdsFtrLean.spare_alloc_refused++);
            if (pool == FALSE)
            {
                return NDS_FTR_LEAN_ENTRY_NONE;
            }
            ndsFtrLeanPoolStore(battle_slot, victim, pooled);
            s->entry[victim].valid = 0u;
            return (pooled != NULL) ?
                ndsFtrLeanPoolRevive(battle_slot, victim, pooled,
                                     pooled_bits) :
                NDS_FTR_LEAN_ENTRY_NONE;
        }
        s->spare_gen = gNdsTaskmanHeapGeneration;
        s->entry[NDS_FTR_LEAN_SPARE].valid = 0u;
        NDS_FTR_LEAN_CTR(gNdsFtrLean.spare_allocs++);
    }
    /* The swap overwrites the spare's list: keep it in the pool first. */
    if (pool != FALSE)
    {
        ndsFtrLeanPoolStore(battle_slot, NDS_FTR_LEAN_SPARE, pooled);
    }
    ndsFtrLeanSpareSwap(s, victim);
    /* The victim now maps to the old spare buffer, about to be rewritten. */
    s->entry[victim].valid = 0u;
    NDS_FTR_LEAN_CTR(gNdsFtrLean.spare_parks++);
    return (pooled != NULL) ?
        ndsFtrLeanPoolRevive(battle_slot, victim, pooled, pooled_bits) :
        NDS_FTR_LEAN_ENTRY_NONE;
}

void ndsFtrLeanNoteKeyMiss(u32 battle_slot, const u32 *key)
{
#if NDS_FTR_LEAN_LAB
    u32 kind;
    u32 e;
    u32 i;

    if ((battle_slot >= NDS_FIGHTER_PACKET_SLOTS) || (key == NULL))
    {
        return;
    }
    kind = sNdsFtrLeanSlots[battle_slot].kind;
    for (e = 0u; e < NDS_FTR_LEAN_ENTRIES; e++)
    {
        const NDSFtrLeanEntryState *es = &sNdsFtrLeanSlots[battle_slot].entry[e];
        u32 differs = 0u;
        u32 why;

        if (ndsFtrLeanEntryUsable(e) == FALSE)
        {
            continue;
        }
        if (es->valid == 0u)
        {
            why = 0u;
        }
        else
        {
            for (i = 0u; i < NDS_FTR_LEAN_KEY_WORDS; i++)
            {
                if (es->key[i] != key[i])
                {
                    differs = 1u;
                    gNdsFtrLean.materialize_key_miss[i]++;
                    if (kind < NDS_FTR_LEAN_KINDS)
                    {
                        gNdsFtrLean.k_key_miss[kind][i]++;
                    }
                }
            }
            why = (differs != 0u) ? 3u :
                (es->tint_set_generation != gNdsR2FighterTintSetGeneration) ?
                1u : 2u;
        }
        gNdsFtrLean.materialize_why[why]++;
        if (kind < NDS_FTR_LEAN_KINDS)
        {
            gNdsFtrLean.k_why[kind][why]++;
        }
    }
#else
    (void)battle_slot;
    (void)key;
#endif
}

u32 ndsFtrLeanEntryVictim(u32 battle_slot)
{
    NDSFtrLeanSlotState *s;
    u32 fence;
    u32 e;

    if (battle_slot >= NDS_FIGHTER_PACKET_SLOTS)
    {
        return NDS_FTR_LEAN_ENTRY_NONE;
    }
    s = &sNdsFtrLeanSlots[battle_slot];
    fence = ndsFtrLeanFenceNow();
    /* An empty or stale entry first -- the upper half before the lower (it
     * holds the first list, which the recorder never touches) -- then the
     * entry that is not active. */
    for (e = NDS_FTR_LEAN_ENTRIES; e-- > 0u;)
    {
        if ((s->entry[e].valid == 0u) ||
            (ndsFtrLeanEntryStale(&s->entry[e], fence) != FALSE))
        {
            return e;
        }
    }
    return (s->active == 1u) ? 0u : 1u;
}

u32 ndsFtrLeanEntryActiveIndex(u32 battle_slot)
{
    return ((battle_slot < NDS_FIGHTER_PACKET_SLOTS) &&
            (sNdsFtrLeanSlots[battle_slot].active < NDS_FTR_LEAN_ENTRIES)) ?
        sNdsFtrLeanSlots[battle_slot].active : NDS_FTR_LEAN_ENTRY_NONE;
}

void ndsFtrLeanEntryDropActive(u32 battle_slot)
{
    NDSFtrLeanSlotState *s;

    if (battle_slot >= NDS_FIGHTER_PACKET_SLOTS)
    {
        return;
    }
    s = &sNdsFtrLeanSlots[battle_slot];
    if (s->active < NDS_FTR_LEAN_ENTRIES)
    {
        s->entry[s->active].valid = 0u;
    }
    s->active = NDS_FTR_LEAN_ENTRY_NONE;
}

u32 ndsFtrLeanEntryWide(u32 battle_slot, u32 entry)
{
    return ((battle_slot < NDS_FIGHTER_PACKET_SLOTS) &&
            (entry < NDS_FTR_LEAN_ENTRIES) &&
            ((sNdsFtrLeanSlots[battle_slot].entry[entry].valid &
              NDS_FTR_LEAN_VALID_WIDE) != 0u)) ? TRUE : FALSE;
}

/* Put an entry's words, texture table, fences and key in record `to`'s
 * state: the current record's words back to the base, then `to`'s. */
static u32 ndsFtrLeanVariantSwitch(u32 battle_slot, u32 entry, u32 to)
{
    NDSFtrLeanEntryState *es = &sNdsFtrLeanSlots[battle_slot].entry[entry];
    NDSFighterPacket *packet = ndsFtrLeanEntryPacket(battle_slot, entry);
    const NDSFtrLeanVariant *from;
    const NDSFtrLeanVariant *v;
    u32 i;

    if ((to >= es->variant_count) || (to == es->variant_cur))
    {
        return FALSE;
    }
    from = ndsFtrLeanVariantRecord(battle_slot, entry, es->variant_cur);
    v = ndsFtrLeanVariantRecord(battle_slot, entry, to);
    /* Nothing may still be streaming this list into the FIFO. */
    ndsFighterPacketDmaWait();
    for (i = 0u; i < (u32)from->diff_count; i++)
    {
        packet->words[from->index[i]] = from->base[i];
        ndsFtrLeanMarkDirty(es, packet->words, from->index[i], 1u);
    }
    for (i = 0u; i < (u32)v->diff_count; i++)
    {
        packet->words[v->index[i]] = v->value[i];
        ndsFtrLeanMarkDirty(es, packet->words, v->index[i], 1u);
    }
    memcpy(packet->textures, v->textures,
           (u32)packet->texture_count * sizeof(packet->textures[0]));
    es->fence_needed = v->fence_needed;
    es->fence = v->fence;
    es->all_pinned = v->all_pinned;
    for (i = 0u; i < NDS_FTR_LEAN_KEY_WORDS; i++)
    {
        es->key[i] = v->key[i];
    }
    es->variant_cur = to;
#if NDS_FTR_LEAN_LAB
    gNdsFtrLean.variant_switches++;
    if (sNdsFtrLeanSlots[battle_slot].kind < NDS_FTR_LEAN_KINDS)
    {
        gNdsFtrLean.k_variant_switches[sNdsFtrLeanSlots[battle_slot].kind]++;
    }
#endif
    return TRUE;
}

u32 ndsFtrLeanEntryActivate(u32 battle_slot, u32 code)
{
    u32 entry = code & 0xfu;
    u32 record = (code >> 4) & 0xfu;
    u32 switched = FALSE;
    NDSFtrLeanSlotState *s;

    if ((battle_slot >= NDS_FIGHTER_PACKET_SLOTS) ||
        (ndsFtrLeanEntryUsable(entry) == FALSE))
    {
        return FALSE;
    }
    s = &sNdsFtrLeanSlots[battle_slot];
    if (s->entry[entry].valid == 0u)
    {
        return FALSE;
    }
    if ((record != 0u) &&
        (ndsFtrLeanVariantSwitch(battle_slot, entry, record - 1u) != FALSE))
    {
        switched = TRUE;
    }
    if (s->active != entry)
    {
        switched = TRUE;
    }
    s->active = entry;
    return switched;
}

/* Two lists of the same shape: every word a patch rewrites sits at the same
 * place with the same inputs (roots, P', light, shade sites and their
 * colours, texgen, tint binds), the frame accounting is the same, and only
 * words outside those sites -- and the textures they bind -- may differ. */
static u32 ndsFtrLeanSameShape(const NDSFighterPacket *a,
                               const NDSFtrLeanEntryState *as,
                               const NDSFighterPacket *b,
                               const NDSFtrLeanEntryState *bs)
{
    u32 i;

    if ((a->word_count != b->word_count) ||
        (a->root_count != b->root_count) ||
        (a->projection_index != b->projection_index) ||
        (a->light_index != b->light_index) ||
        (a->light_root != b->light_root) ||
        (a->light_valid != b->light_valid) ||
        (a->needs_fence != b->needs_fence) ||
        (a->texture_count != b->texture_count) ||
        (a->texgen_group_count != b->texgen_group_count) ||
        (a->texgen_site_count != b->texgen_site_count) ||
        (a->triangle_count != b->triangle_count) ||
        (a->run_count != b->run_count) ||
        (a->raw_triangles != b->raw_triangles) ||
        (a->raw_reuse != b->raw_reuse) ||
        (a->cross_triangles != b->cross_triangles) ||
        (a->cross_reuse != b->cross_reuse) ||
        (a->batch_begin != b->batch_begin) ||
        (a->batch_reuse != b->batch_reuse) ||
        (a->batch_end != b->batch_end) ||
        (a->prepare_begin != b->prepare_begin) ||
        (a->prepare_reuse != b->prepare_reuse) ||
        (a->matrix_loads != b->matrix_loads) ||
        (a->texture_binds != b->texture_binds) ||
        (a->vertex_loads != b->vertex_loads) ||
        (a->site_count != b->site_count) ||
        (a->tint_bind_count != b->tint_bind_count) ||
        (a->tint_bind_overflow != b->tint_bind_overflow) ||
        (a->fence_other != b->fence_other) ||
        (a->tint_bind_seen != b->tint_bind_seen) ||
        (as->donor_mask != bs->donor_mask) ||
        (as->texgen_map != bs->texgen_map) ||
        (as->tint_folds != bs->tint_folds))
    {
        return FALSE;
    }
    for (i = 0u; i < a->root_count; i++)
    {
        const NDSFighterPacketRoot *x = &a->roots[i];
        const NDSFighterPacketRoot *y = &b->roots[i];
        u32 j;

        if ((x->seed_index != y->seed_index) ||
            (x->local_count != y->local_count) ||
            (x->parent_slot != y->parent_slot) ||
            (x->store_slot != y->store_slot) ||
            (x->seed_is_identity != y->seed_is_identity))
        {
            return FALSE;
        }
        for (j = 0u; j < NDS_FIGHTER_PACKET_LOCAL_MAX; j++)
        {
            if (x->local_index[j] != y->local_index[j])
            {
                return FALSE;
            }
        }
    }
    for (i = 0u; i < a->site_count; i++)
    {
        const NDSFighterPacketShadeSite *x = &a->sites[i];
        const NDSFighterPacketShadeSite *y = &b->sites[i];

        if ((x->index != y->index) || (x->root != y->root) ||
            (x->use_material != y->use_material) ||
            (x->prim_from_root != y->prim_from_root) ||
            (x->light_color_1 != y->light_color_1) ||
            (x->light_color_2 != y->light_color_2) ||
            (x->material_color != y->material_color))
        {
            return FALSE;
        }
    }
    for (i = 0u; i < (u32)a->texgen_group_count; i++)
    {
        const NDSFighterPacketTexgenGroup *x = &a->texgen_groups[i];
        const NDSFighterPacketTexgenGroup *y = &b->texgen_groups[i];

        if ((x->scale_s != y->scale_s) || (x->scale_t != y->scale_t) ||
            (x->origin_s != y->origin_s) || (x->origin_t != y->origin_t) ||
            (x->offset != y->offset) || (x->first_site != y->first_site) ||
            (x->site_count != y->site_count) || (x->root != y->root))
        {
            return FALSE;
        }
    }
    for (i = 0u; i < (u32)a->texgen_site_count; i++)
    {
        if ((a->texgen_sites[i].index != b->texgen_sites[i].index) ||
            (a->texgen_sites[i].dense_id != b->texgen_sites[i].dense_id))
        {
            return FALSE;
        }
    }
    for (i = 0u; i < (u32)a->tint_bind_count; i++)
    {
        const NDSFighterPacketTintBind *x = &a->tint_binds[i];
        const NDSFighterPacketTintBind *y = &b->tint_binds[i];

        if ((x->tex_index != y->tex_index) ||
            (x->pal_index != y->pal_index) || (x->root != y->root) ||
            (x->prim_from_root != y->prim_from_root))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/* The words a per-frame patch rewrites, as a bit mask over the list's
 * first `word_capacity` words. */
static void ndsFtrLeanPatchMaskN(const NDSFighterPacket *packet, u32 *mask,
                                 u32 word_capacity)
{
    u32 i;
    u32 j;
    u32 mask_words = (word_capacity + 31u) / 32u;

    memset(mask, 0, mask_words * sizeof(u32));
#define NDS_FTR_LEAN_MASK_SET(index_)                                      \
    do                                                                     \
    {                                                                      \
        u32 w_ = (u32)(index_);                                            \
        if (w_ < word_capacity)                                             \
        {                                                                  \
            mask[w_ >> 5] |= 1u << (w_ & 31u);                             \
        }                                                                  \
    } while (0)
    for (j = 0u; j < 16u; j++)
    {
        NDS_FTR_LEAN_MASK_SET(packet->projection_index + j);
    }
    if (packet->light_index != NDS_FIGHTER_PACKET_INDEX_NONE)
    {
        NDS_FTR_LEAN_MASK_SET(packet->light_index);
    }
    for (i = 0u; i < packet->root_count; i++)
    {
        for (j = 0u; j < 12u; j++)
        {
            NDS_FTR_LEAN_MASK_SET(packet->roots[i].seed_index + j);
        }
    }
    for (i = 0u; i < packet->site_count; i++)
    {
        NDS_FTR_LEAN_MASK_SET(packet->sites[i].index);
    }
    for (i = 0u; i < (u32)packet->texgen_site_count; i++)
    {
        NDS_FTR_LEAN_MASK_SET(packet->texgen_sites[i].index);
    }
    for (i = 0u; i < (u32)packet->tint_bind_count; i++)
    {
        NDS_FTR_LEAN_MASK_SET(packet->tint_binds[i].tex_index);
        if (packet->tint_binds[i].pal_index != NDS_FIGHTER_PACKET_INDEX_NONE)
        {
            NDS_FTR_LEAN_MASK_SET(packet->tint_binds[i].pal_index);
        }
    }
#undef NDS_FTR_LEAN_MASK_SET
}

/* ... over an entry's word capacity. */
static void ndsFtrLeanPatchMask(const NDSFighterPacket *packet, u32 *mask)
{
    ndsFtrLeanPatchMaskN(packet, mask, ndsFtrLeanWordCapacity());
}

static void ndsFtrLeanVariantReject(u32 reason)
{
#if NDS_FTR_LEAN_LAB
    gNdsFtrLean.variant_reject[reason]++;
#else
    (void)reason;
#endif
}

#if defined(NDS_LAB_LEAN_ROOT_CENSUS) && NDS_LAB_LEAN_ROOT_CENSUS
/* Lab (2026-10-05): the reuse an incremental materialization could find. For
 * each root, is the fresh list's block (its LOAD4x3 seed to the next root's)
 * the held list's block word for word, patch sites masked out? [0] lists
 * compared, [1] roots, [2] roots of equal length, [3] identical roots, [4]
 * words, [5] words in identical roots, [6] leading identical roots, [7] lists
 * with at most two differing roots. */
__attribute__((used, aligned(32))) volatile u32 gNdsLabLeanRootCensus[8];
static u32 sNdsLabLeanRootMask[2][NDS_FTR_LEAN_MASK_WORDS];

void ndsFtrLeanRootCensus(u32 battle_slot, u32 fresh)
{
    NDSFtrLeanSlotState *s;
    const NDSFighterPacket *a;
    const NDSFighterPacket *b;
    u32 held;
    u32 roots;
    u32 r;
    u32 same = 0u;
    u32 leading = 0u;
    sb32 lead_open = TRUE;

    if ((battle_slot >= NDS_FIGHTER_PACKET_SLOTS) ||
        (fresh >= NDS_FTR_LEAN_ENTRIES))
    {
        return;
    }
    s = &sNdsFtrLeanSlots[battle_slot];
    held = fresh ^ 1u;
    if ((ndsFtrLeanEntryUsable(held) == FALSE) || (s->entry[held].valid == 0u) ||
        (s->entry[fresh].valid == 0u) ||
        (((s->entry[held].valid | s->entry[fresh].valid) &
          NDS_FTR_LEAN_VALID_WIDE) != 0u))
    {
        return;
    }
    a = ndsFtrLeanEntryPacket(battle_slot, fresh);
    b = ndsFtrLeanEntryPacket(battle_slot, held);
    if ((a->root_count != b->root_count) || (a->root_count == 0u))
    {
        return;
    }
    ndsFtrLeanPatchMask(a, sNdsLabLeanRootMask[0]);
    ndsFtrLeanPatchMask(b, sNdsLabLeanRootMask[1]);
    roots = a->root_count;
    gNdsLabLeanRootCensus[0]++;
    for (r = 0u; r < roots; r++)
    {
        const u32 a0 = a->roots[r].seed_index;
        const u32 b0 = b->roots[r].seed_index;
        const u32 a1 = (r + 1u < roots) ? a->roots[r + 1u].seed_index :
            a->word_count;
        const u32 b1 = (r + 1u < roots) ? b->roots[r + 1u].seed_index :
            b->word_count;
        sb32 identical = TRUE;
        u32 i;

        gNdsLabLeanRootCensus[1]++;
        if ((a0 == NDS_FIGHTER_PACKET_INDEX_NONE) ||
            (b0 == NDS_FIGHTER_PACKET_INDEX_NONE) || (a1 < a0) || (b1 < b0) ||
            (a1 > a->word_count) || (b1 > b->word_count))
        {
            lead_open = FALSE;
            continue;
        }
        gNdsLabLeanRootCensus[4] += a1 - a0;
        if ((a1 - a0) != (b1 - b0))
        {
            lead_open = FALSE;
            continue;
        }
        gNdsLabLeanRootCensus[2]++;
        for (i = 0u; i < a1 - a0; i++)
        {
            const u32 ia = a0 + i;
            const u32 ib = b0 + i;
            const u32 ma = (sNdsLabLeanRootMask[0][ia >> 5] >> (ia & 31u)) & 1u;
            const u32 mb = (sNdsLabLeanRootMask[1][ib >> 5] >> (ib & 31u)) & 1u;

            if ((ma != mb) || ((ma == 0u) && (a->words[ia] != b->words[ib])))
            {
                identical = FALSE;
                break;
            }
        }
        if (identical != FALSE)
        {
            same++;
            gNdsLabLeanRootCensus[3]++;
            gNdsLabLeanRootCensus[5] += a1 - a0;
            if (lead_open != FALSE)
            {
                leading++;
            }
        }
        else
        {
            lead_open = FALSE;
        }
    }
    gNdsLabLeanRootCensus[6] += leading;
    if (same + 2u >= roots)
    {
        gNdsLabLeanRootCensus[7]++;
    }
    DC_FlushRange((const void *)gNdsLabLeanRootCensus,
                  sizeof(gNdsLabLeanRootCensus));
}
#endif

u32 NDS_FIGHTER_PACKET_COLD_CODE
ndsFtrLeanLearnVariant(u32 battle_slot, u32 fresh)
{
    NDSFtrLeanSlotState *s;
    NDSFtrLeanEntryState *fs;
    NDSFtrLeanEntryState *hs;
    NDSFighterPacket *a;
    NDSFighterPacket *b;
    NDSFtrLeanVariant *v;
    u32 *mask;
    u32 held;
    u32 record;
    u32 n = 0u;
    u32 i;
    u32 word_capacity = ndsFtrLeanWordCapacity();
    u32 mask_words = ndsFtrLeanMaskWords();
    /* The differing words, in index order, until a seventeenth rejects. */
    u16 diff_index[NDS_FTR_LEAN_VARIANT_DIFF_MAX];

    if ((battle_slot >= NDS_FIGHTER_PACKET_SLOTS) ||
        (fresh >= NDS_FTR_LEAN_ENTRIES))
    {
        return NDS_FTR_LEAN_ENTRY_NONE;
    }
    s = &sNdsFtrLeanSlots[battle_slot];
    held = fresh ^ 1u;
    fs = &s->entry[fresh];
    hs = &s->entry[held];
    if ((ndsFtrLeanEntryUsable(held) == FALSE) || (hs->valid == 0u) ||
        (fs->valid == 0u) ||
        (ndsFtrLeanEntryStale(hs, ndsFtrLeanFenceNow()) != FALSE))
    {
        ndsFtrLeanVariantReject(0u);
        return NDS_FTR_LEAN_ENTRY_NONE;
    }
    a = ndsFtrLeanEntryPacket(battle_slot, fresh);
    b = ndsFtrLeanEntryPacket(battle_slot, held);
    if (ndsFtrLeanSameShape(a, fs, b, hs) == FALSE)
    {
        ndsFtrLeanVariantReject(1u);
        return NDS_FTR_LEAN_ENTRY_NONE;
    }
    if (hs->variant_max < 2u)
    {
        ndsFtrLeanVariantReject(2u);
        return NDS_FTR_LEAN_ENTRY_NONE;
    }
    if (a->word_count + mask_words > word_capacity)
    {
        ndsFtrLeanVariantReject(2u);
        return NDS_FTR_LEAN_ENTRY_NONE;
    }
    /* The held list back at its base; the top of the fresh entry's capacity
     * (it has no records yet) holds the mask. */
    if (hs->variant_count != 0u)
    {
        (void)ndsFtrLeanVariantSwitch(battle_slot, held, 0u);
    }
    mask = ndsFtrLeanEntryWords(battle_slot, fresh) +
        word_capacity - mask_words;
    ndsFtrLeanPatchMask(a, mask);
    /* One pass (2026-10-06): the differences are recorded as they are found
     * and the scan stops at the seventeenth, where it used to count every
     * difference in the list and then walk it again to record them -- two
     * reads of two ~16 KB lists from main RAM after every materialization,
     * the learn's whole cost (~46K cycles a marked frame, 4 x Yoshi). Same
     * records, same rejects. 2026-10-13: the words first, the mask only
     * where they differ -- nearly every word of two same-shape lists is
     * equal (25.7K cycles a marked frame, four Crashes). */
    {
        const u32 *aw = a->words;
        const u32 *bw = b->words;
        const u32 count = a->word_count;
        /* 2026-10-18: a self-contained root's block is a function of its
         * key (the premise ndsFtrLeanRootCopy copies on), so a block whose
         * key, place and polygon-ID base are the same in both lists, and
         * whose textures the held list still has resident, holds the same
         * words in both: only the head and the blocks that differ are read.
         * Both ~12 KB lists word by word were ~77K cycles a learn (four
         * Bowsers: 12 of a census's 26 over-gate frames learned). */
        const NDSFtrLeanRootRec *ra = ndsFtrLeanRootRecs(a);
        const NDSFtrLeanRootRec *rb = ndsFtrLeanRootRecs(b);
        const u32 recs = ((a->root_rec_count != 0u) &&
                          (a->root_rec_count == b->root_rec_count) &&
                          (a->texgen_site_count == 0u) &&
                          (b->texgen_site_count == 0u)) ?
            (u32)a->root_rec_count : 0u;
        u32 pos = 0u;
        u32 r;

        for (r = 0u; r <= recs; r++)
        {
            u32 stop = count;
            u32 next = count;

            if (r < recs)
            {
                const NDSFtrLeanRootRec *x = &ra[r];
                const NDSFtrLeanRootRec *y = &rb[r];

                if ((x->start < pos) || (x->end < x->start) ||
                    ((u32)x->end > count))
                {
                    /* Not in order: compare the rest whole. */
                    r = recs;
                }
                else if ((x->key == y->key) && (x->start == y->start) &&
                         (x->end == y->end) &&
                         (x->combine_base == y->combine_base) &&
                         ((x->flags & y->flags & NDS_FTR_LEAN_ROOT_COPY) != 0u) &&
                         (ndsFtrLeanRootRecResident(b, y) != FALSE))
                {
                    stop = x->start;
                    next = x->end;
                }
                else
                {
                    continue;
                }
            }
            for (i = pos; i < stop; i++)
            {
                if ((aw[i] != bw[i]) &&
                    (((mask[i >> 5] >> (i & 31u)) & 1u) == 0u))
                {
                    if (n == NDS_FTR_LEAN_VARIANT_DIFF_MAX)
                    {
                        ndsFtrLeanVariantReject(3u);
                        return NDS_FTR_LEAN_ENTRY_NONE;
                    }
                    diff_index[n++] = (u16)i;
                }
            }
            pos = next;
        }
    }
    if (hs->variant_count == 0u)
    {
        /* Record 0: the base the words hold now. */
        v = ndsFtrLeanVariantRecord(battle_slot, held, 0u);
        memset(v, 0, sizeof(*v));
        for (i = 0u; i < NDS_FTR_LEAN_KEY_WORDS; i++)
        {
            v->key[i] = hs->key[i];
        }
        v->fence = hs->fence;
        v->fence_needed = (u8)hs->fence_needed;
        v->all_pinned = (u8)hs->all_pinned;
        memcpy(v->textures, b->textures,
               (u32)b->texture_count * sizeof(b->textures[0]));
        hs->variant_count = 1u;
        hs->variant_cur = 0u;
        hs->variant_next = 1u;
    }
    if (hs->variant_count < hs->variant_max)
    {
        record = hs->variant_count++;
    }
    else
    {
        record = hs->variant_next;
        hs->variant_next = (record + 1u < hs->variant_max) ? (record + 1u) : 1u;
        NDS_FTR_LEAN_CTR(gNdsFtrLean.variant_evictions++);
    }
    v = ndsFtrLeanVariantRecord(battle_slot, held, record);
    memset(v, 0, sizeof(*v));
    for (i = 0u; i < NDS_FTR_LEAN_KEY_WORDS; i++)
    {
        v->key[i] = fs->key[i];
    }
    for (i = 0u; i < n; i++)
    {
        const u32 word = diff_index[i];

        v->index[i] = (u16)word;
        v->base[i] = b->words[word];
        v->value[i] = a->words[word];
    }
    v->diff_count = (u8)n;
    v->fence = fs->fence;
    v->fence_needed = (u8)fs->fence_needed;
    v->all_pinned = (u8)fs->all_pinned;
    memcpy(v->textures, a->textures,
           (u32)a->texture_count * sizeof(a->textures[0]));
    (void)ndsFtrLeanVariantSwitch(battle_slot, held, record);
#if NDS_FTR_LEAN_LAB
    gNdsFtrLean.variants_learned++;
    gNdsFtrLean.variant_diff_words += n;
    if (n > gNdsFtrLean.variant_diff_max)
    {
        gNdsFtrLean.variant_diff_max = n;
    }
    if (s->kind < NDS_FTR_LEAN_KINDS)
    {
        gNdsFtrLean.k_variants_learned[s->kind]++;
    }
#endif
    return held;
}

/* ndsFtrLeanSameShape's fields, folded (NDSFtrLeanWideHold). */
static u32 NDS_FIGHTER_PACKET_COLD_CODE
ndsFtrLeanShapeHash(const NDSFighterPacket *p, const NDSFtrLeanEntryState *es)
{
    u32 h = 2166136261u;
    u32 i;
    u32 j;

#define NDS_FTR_LEAN_SHAPE(v_) (h = (h ^ (u32)(v_)) * 16777619u)
    NDS_FTR_LEAN_SHAPE(p->word_count);
    NDS_FTR_LEAN_SHAPE(p->root_count);
    NDS_FTR_LEAN_SHAPE(p->projection_index);
    NDS_FTR_LEAN_SHAPE(p->light_index);
    NDS_FTR_LEAN_SHAPE(p->light_root);
    NDS_FTR_LEAN_SHAPE(p->light_valid);
    NDS_FTR_LEAN_SHAPE(p->needs_fence);
    NDS_FTR_LEAN_SHAPE(p->texture_count);
    NDS_FTR_LEAN_SHAPE(p->texgen_group_count);
    NDS_FTR_LEAN_SHAPE(p->texgen_site_count);
    NDS_FTR_LEAN_SHAPE(p->triangle_count);
    NDS_FTR_LEAN_SHAPE(p->run_count);
    NDS_FTR_LEAN_SHAPE(p->raw_triangles);
    NDS_FTR_LEAN_SHAPE(p->raw_reuse);
    NDS_FTR_LEAN_SHAPE(p->cross_triangles);
    NDS_FTR_LEAN_SHAPE(p->cross_reuse);
    NDS_FTR_LEAN_SHAPE(p->batch_begin);
    NDS_FTR_LEAN_SHAPE(p->batch_reuse);
    NDS_FTR_LEAN_SHAPE(p->batch_end);
    NDS_FTR_LEAN_SHAPE(p->prepare_begin);
    NDS_FTR_LEAN_SHAPE(p->prepare_reuse);
    NDS_FTR_LEAN_SHAPE(p->matrix_loads);
    NDS_FTR_LEAN_SHAPE(p->texture_binds);
    NDS_FTR_LEAN_SHAPE(p->vertex_loads);
    NDS_FTR_LEAN_SHAPE(p->site_count);
    NDS_FTR_LEAN_SHAPE(p->tint_bind_count);
    NDS_FTR_LEAN_SHAPE(p->tint_bind_overflow);
    NDS_FTR_LEAN_SHAPE(p->fence_other);
    NDS_FTR_LEAN_SHAPE(p->tint_bind_seen);
    NDS_FTR_LEAN_SHAPE(es->donor_mask);
    NDS_FTR_LEAN_SHAPE(es->texgen_map);
    NDS_FTR_LEAN_SHAPE(es->tint_folds);
    for (i = 0u; (i < p->root_count) && (i < NDS_FIGHTER_PACKET_ROOT_MAX); i++)
    {
        const NDSFighterPacketRoot *x = &p->roots[i];

        NDS_FTR_LEAN_SHAPE(x->seed_index);
        NDS_FTR_LEAN_SHAPE(x->local_count);
        NDS_FTR_LEAN_SHAPE(x->parent_slot);
        NDS_FTR_LEAN_SHAPE(x->store_slot);
        NDS_FTR_LEAN_SHAPE(x->seed_is_identity);
        for (j = 0u; j < NDS_FIGHTER_PACKET_LOCAL_MAX; j++)
        {
            NDS_FTR_LEAN_SHAPE(x->local_index[j]);
        }
    }
    for (i = 0u; (i < p->site_count) && (i < NDS_FIGHTER_PACKET_SITE_MAX); i++)
    {
        const NDSFighterPacketShadeSite *x = &p->sites[i];

        NDS_FTR_LEAN_SHAPE(x->index);
        NDS_FTR_LEAN_SHAPE(x->root);
        NDS_FTR_LEAN_SHAPE(x->use_material);
        NDS_FTR_LEAN_SHAPE(x->prim_from_root);
        NDS_FTR_LEAN_SHAPE(x->light_color_1);
        NDS_FTR_LEAN_SHAPE(x->light_color_2);
        NDS_FTR_LEAN_SHAPE(x->material_color);
    }
    for (i = 0u; (i < (u32)p->texgen_group_count) &&
         (i < NDS_FIGHTER_PACKET_TEXGEN_GROUP_MAX); i++)
    {
        const NDSFighterPacketTexgenGroup *x = &p->texgen_groups[i];

        NDS_FTR_LEAN_SHAPE(x->scale_s);
        NDS_FTR_LEAN_SHAPE(x->scale_t);
        NDS_FTR_LEAN_SHAPE(x->origin_s);
        NDS_FTR_LEAN_SHAPE(x->origin_t);
        NDS_FTR_LEAN_SHAPE(x->offset);
        NDS_FTR_LEAN_SHAPE(x->first_site);
        NDS_FTR_LEAN_SHAPE(x->site_count);
        NDS_FTR_LEAN_SHAPE(x->root);
    }
    for (i = 0u; (i < (u32)p->texgen_site_count) &&
         (i < NDS_FIGHTER_PACKET_TEXGEN_SITE_MAX); i++)
    {
        NDS_FTR_LEAN_SHAPE(p->texgen_sites[i].index);
        NDS_FTR_LEAN_SHAPE(p->texgen_sites[i].dense_id);
    }
    for (i = 0u; (i < (u32)p->tint_bind_count) &&
         (i < NDS_FIGHTER_PACKET_TINT_BIND_MAX); i++)
    {
        const NDSFighterPacketTintBind *x = &p->tint_binds[i];

        NDS_FTR_LEAN_SHAPE(x->tex_index);
        NDS_FTR_LEAN_SHAPE(x->pal_index);
        NDS_FTR_LEAN_SHAPE(x->root);
        NDS_FTR_LEAN_SHAPE(x->prim_from_root);
    }
#undef NDS_FTR_LEAN_SHAPE
    return h;
}

/* A wide entry's hold block for a list of `count` words and `records`
 * records -- in the learn's scratch (its mask, the block, then the copy)
 * when there is one, else in the region above the list -- or NULL when
 * neither has room. */
static NDSFtrLeanWideHold *ndsFtrLeanWideHoldAt(u32 battle_slot, u32 count,
                                                u32 records)
{
    const u32 top = records * NDS_FTR_LEAN_VARIANT_WORDS;
    const u32 capacity = ndsFtrLeanWideCapacity();
    u32 *scratch;

    if (records < 2u)
    {
        return NULL;
    }
    scratch = ndsFtrLeanWideScratch(count, FALSE);
    if ((scratch != NULL) && ((top + count) <= capacity))
    {
        return (NDSFtrLeanWideHold *)(void *)(scratch + ((count + 31u) / 32u));
    }
    if ((top + count + NDS_FTR_LEAN_WIDE_RELEARN_WORDS(count)) > capacity)
    {
        return NULL;
    }
    return (NDSFtrLeanWideHold *)(void *)(ndsFtrLeanEntryWords(battle_slot, 0u) +
        capacity - top - count - NDS_FTR_LEAN_WIDE_HOLD_WORDS);
}

u32 NDS_FIGHTER_PACKET_COLD_CODE ndsFtrLeanWideHoldBegin(u32 battle_slot)
{
    NDSFtrLeanSlotState *s;
    NDSFtrLeanEntryState *es;
    NDSFighterPacket *packet;
    NDSFtrLeanWideHold *hold;
    u32 i;

    if (battle_slot >= NDS_FIGHTER_PACKET_SLOTS)
    {
        return FALSE;
    }
    s = &sNdsFtrLeanSlots[battle_slot];
    es = &s->entry[0];
    if (((es->valid & NDS_FTR_LEAN_VALID_WIDE) == 0u) || (s->phys_on != 0u) ||
        (ndsFtrLeanEntryStale(es, ndsFtrLeanFenceNow()) != FALSE))
    {
        return FALSE;
    }
    packet = ndsFtrLeanEntryPacket(battle_slot, 0u);
    hold = ndsFtrLeanWideHoldAt(battle_slot, packet->word_count,
                                es->variant_max);
    if (hold == NULL)
    {
        return FALSE;
    }
    /* The records are differences from the base: the words at it. */
    if ((es->variant_count != 0u) && (es->variant_cur != 0u))
    {
        (void)ndsFtrLeanVariantSwitch(battle_slot, 0u, 0u);
    }
    memcpy((u32 *)(void *)hold + NDS_FTR_LEAN_WIDE_HOLD_WORDS, packet->words,
           packet->word_count * sizeof(u32));
    hold->magic = NDS_FTR_LEAN_WIDE_HOLD_MAGIC;
    hold->word_count = packet->word_count;
    hold->shape = ndsFtrLeanShapeHash(packet, es);
    hold->variant_count = es->variant_count;
    hold->variant_next = es->variant_next;
    hold->variant_max = es->variant_max;
    for (i = 0u; i < NDS_FTR_LEAN_KEY_WORDS; i++)
    {
        hold->key[i] = es->key[i];
    }
    hold->fence = es->fence;
    hold->fence_needed = es->fence_needed;
    hold->all_pinned = es->all_pinned;
    hold->texture_count = packet->texture_count;
    memcpy(hold->textures, packet->textures,
           (u32)packet->texture_count * sizeof(packet->textures[0]));
    return TRUE;
}

void NDS_FIGHTER_PACKET_COLD_CODE ndsFtrLeanWideLearn(u32 battle_slot)
{
    NDSFtrLeanSlotState *s;
    NDSFtrLeanEntryState *es;
    NDSFighterPacket *packet;
    NDSFtrLeanWideHold *hold;
    NDSFtrLeanVariant *v;
    const u32 *copy;
    u32 *mask;
    u32 record;
    u32 count;
    u32 n = 0u;
    u32 i;
    u16 diff_index[NDS_FTR_LEAN_VARIANT_DIFF_MAX];

    if (battle_slot >= NDS_FIGHTER_PACKET_SLOTS)
    {
        return;
    }
    s = &sNdsFtrLeanSlots[battle_slot];
    es = &s->entry[0];
    if (((es->valid & NDS_FTR_LEAN_VALID_WIDE) == 0u) || (s->phys_on != 0u))
    {
        return;
    }
    packet = ndsFtrLeanEntryPacket(battle_slot, 0u);
    count = packet->word_count;
    /* Where the held list's block sat if the fresh list has its shape. */
    hold = ndsFtrLeanWideHoldAt(battle_slot, count, es->variant_max);
    if ((hold == NULL) || (hold->magic != NDS_FTR_LEAN_WIDE_HOLD_MAGIC) ||
        (hold->word_count != count) ||
        (hold->variant_max != es->variant_max) ||
        (hold->texture_count != packet->texture_count))
    {
        return;
    }
    hold->magic = 0u;
    if (ndsFtrLeanShapeHash(packet, es) != hold->shape)
    {
        ndsFtrLeanVariantReject(1u);
        return;
    }
    copy = (const u32 *)(const void *)hold + NDS_FTR_LEAN_WIDE_HOLD_WORDS;
    mask = (u32 *)(void *)hold - ((count + 31u) / 32u);
    ndsFtrLeanPatchMaskN(packet, mask, count);
    for (i = 0u; i < count; i++)
    {
        if ((packet->words[i] != copy[i]) &&
            (((mask[i >> 5] >> (i & 31u)) & 1u) == 0u))
        {
            if (n == NDS_FTR_LEAN_VARIANT_DIFF_MAX)
            {
                ndsFtrLeanVariantReject(3u);
                return;
            }
            diff_index[n++] = (u16)i;
        }
    }
    es->variant_count = hold->variant_count;
    es->variant_next = hold->variant_next;
    if (es->variant_count == 0u)
    {
        /* Record 0: the held state, the base the copy holds. */
        v = ndsFtrLeanVariantRecord(battle_slot, 0u, 0u);
        memset(v, 0, sizeof(*v));
        for (i = 0u; i < NDS_FTR_LEAN_KEY_WORDS; i++)
        {
            v->key[i] = hold->key[i];
        }
        v->fence = hold->fence;
        v->fence_needed = (u8)hold->fence_needed;
        v->all_pinned = (u8)hold->all_pinned;
        memcpy(v->textures, hold->textures,
               hold->texture_count * sizeof(hold->textures[0]));
        es->variant_count = 1u;
        es->variant_next = 1u;
    }
    if (es->variant_count < es->variant_max)
    {
        record = es->variant_count++;
    }
    else
    {
        record = es->variant_next;
        es->variant_next = (record + 1u < es->variant_max) ? (record + 1u) : 1u;
        NDS_FTR_LEAN_CTR(gNdsFtrLean.variant_evictions++);
    }
    /* The fresh state, which the words in place already hold. */
    v = ndsFtrLeanVariantRecord(battle_slot, 0u, record);
    memset(v, 0, sizeof(*v));
    for (i = 0u; i < NDS_FTR_LEAN_KEY_WORDS; i++)
    {
        v->key[i] = es->key[i];
    }
    for (i = 0u; i < n; i++)
    {
        const u32 word = diff_index[i];

        v->index[i] = (u16)word;
        v->base[i] = copy[word];
        v->value[i] = packet->words[word];
    }
    v->diff_count = (u8)n;
    v->fence = es->fence;
    v->fence_needed = (u8)es->fence_needed;
    v->all_pinned = (u8)es->all_pinned;
    memcpy(v->textures, packet->textures,
           (u32)packet->texture_count * sizeof(packet->textures[0]));
    es->variant_cur = record;
#if NDS_FTR_LEAN_LAB
    gNdsFtrLean.variants_learned++;
    gNdsFtrLean.variant_diff_words += n;
    if (n > gNdsFtrLean.variant_diff_max)
    {
        gNdsFtrLean.variant_diff_max = n;
    }
    if (s->kind < NDS_FTR_LEAN_KINDS)
    {
        gNdsFtrLean.k_variants_learned[s->kind]++;
    }
#endif
}

/* Lab: the list an event re-selected (an entry or a record of one) against a
 * fresh materialization of the same key in the other entry. */
void ndsFtrLeanVerifyEntries(u32 battle_slot, u32 held, u32 fresh)
{
#if NDS_FTR_LEAN_LAB
    NDSFtrLeanSlotState *s;
    NDSFtrLeanEntryState *hs;
    NDSFtrLeanEntryState *fs;
    NDSFighterPacket *a;
    NDSFighterPacket *b;
    u32 *mask;
    u32 i;
    u32 word_capacity = ndsFtrLeanWordCapacity();
    u32 mask_words = ndsFtrLeanMaskWords();

    if ((battle_slot >= NDS_FIGHTER_PACKET_SLOTS) ||
        (held >= NDS_FTR_LEAN_ENTRIES) || (fresh >= NDS_FTR_LEAN_ENTRIES) ||
        (held == fresh))
    {
        return;
    }
    s = &sNdsFtrLeanSlots[battle_slot];
    hs = &s->entry[held];
    fs = &s->entry[fresh];
    if ((hs->valid == 0u) || (fs->valid == 0u))
    {
        return;
    }
    a = ndsFtrLeanEntryPacket(battle_slot, fresh);
    b = ndsFtrLeanEntryPacket(battle_slot, held);
    gNdsFtrLean.verify_runs++;
    if (hs->variant_count != 0u)
    {
        gNdsFtrLean.verify_variant_runs++;
    }
    if (ndsFtrLeanSameShape(a, fs, b, hs) == FALSE)
    {
        gNdsFtrLean.verify_mismatch[0]++;
        return;
    }
    if (a->word_count + mask_words > word_capacity)
    {
        return;
    }
    mask = ndsFtrLeanEntryWords(battle_slot, fresh) +
        word_capacity - mask_words;
    ndsFtrLeanPatchMask(a, mask);
    for (i = 0u; i < a->word_count; i++)
    {
        if ((((mask[i >> 5] >> (i & 31u)) & 1u) == 0u) &&
            (a->words[i] != b->words[i]))
        {
            if (gNdsFtrLean.verify_mismatch[1] == 0u)
            {
                gNdsFtrLean.verify_first[0] =
                    battle_slot | (held << 4) | (hs->variant_cur << 8) |
                    (hs->variant_count << 16);
                gNdsFtrLean.verify_first[1] = i;
                gNdsFtrLean.verify_first[2] = b->words[i];
                gNdsFtrLean.verify_first[3] = a->words[i];
            }
            gNdsFtrLean.verify_mismatch[1]++;
        }
    }
    for (i = 0u; i < (u32)a->texture_count; i++)
    {
        if ((a->textures[i].slot_plus1 != b->textures[i].slot_plus1) ||
            (a->textures[i].name != b->textures[i].name) ||
            (a->textures[i].key_generation != b->textures[i].key_generation))
        {
            gNdsFtrLean.verify_mismatch[2]++;
            break;
        }
    }
    /* The patched words too (ROOT BLOCKS re-derives a copy's shade words and
     * takes its tint binds' words from the donor): the held list's are at
     * its last patch, the fresh list's at this draw's modulate and tiles. */
    for (i = 0u; i < (u32)a->site_count; i++)
    {
        if (a->words[a->sites[i].index] != b->words[b->sites[i].index])
        {
            gNdsFtrLean.verify_site_mismatch++;
        }
    }
    for (i = 0u; i < (u32)a->tint_bind_count; i++)
    {
        if ((a->words[a->tint_binds[i].tex_index] !=
             b->words[b->tint_binds[i].tex_index]) ||
            ((a->tint_binds[i].pal_index != NDS_FIGHTER_PACKET_INDEX_NONE) &&
             (a->words[a->tint_binds[i].pal_index] !=
              b->words[b->tint_binds[i].pal_index])))
        {
            gNdsFtrLean.verify_tint_mismatch++;
        }
    }
    if ((fs->fence_needed != hs->fence_needed) ||
        (fs->all_pinned != hs->all_pinned))
    {
        gNdsFtrLean.verify_mismatch[3]++;
    }
#else
    (void)battle_slot;
    (void)held;
    (void)fresh;
#endif
}

/* The packet recorder (deleted 2026-10-05) re-recorded whenever its key's
 * words moved (or the packet was invalidated); the lean path re-derives its
 * shade words at those moments (ndsFtrLeanEntryResetShade). This is that
 * key's shadow over the lean inputs: the material identity (the live
 * MObj keys, pointers included), the roots' display fields and chain shapes,
 * and the lean key's owner / file / program / head words for the tables and
 * heap generation. The tint-tile set generation is left out: a moved set is
 * handled where it is seen (the fold-free repatch below, a TintSet event). */
u32 ndsFtrLeanRerecordKey(u32 ident, const u32 *key,
                          const NDSRendererNativeFighterRoot *inputs,
                          u32 input_count)
{
    u32 h = 2166136261u;
    u32 i;

    if ((key == NULL) || (inputs == NULL))
    {
        return 0u;
    }
    for (i = 0u; i < input_count; i++)
    {
        const NDSRendererNativeFighterRoot *input = &inputs[i];
        const NDSRendererNativeFighterPreamble *preamble = input->preamble;

        h = ndsFighterPacketMix(h, input->root_offset);
        if (preamble != NULL)
        {
            h = ndsFighterPacketMix(h, preamble->geometry_mode);
            h = ndsFighterPacketMix(h, preamble->cycle_type);
            h = ndsFighterPacketMix(h, preamble->render_mode);
            h = ndsFighterPacketMix(h, preamble->env_color);
            h = ndsFighterPacketMix(h, preamble->flags);
        }
        if (input->config != NULL)
        {
            h = ndsFighterPacketMix(h, input->config->initial_geometry_mode);
        }
        h = ndsFighterPacketMix(h, input->material_count);
        h = ndsFighterPacketMix(h,
            (u32)input->gx_parent_slot |
                ((u32)input->gx_store_slot << 8) |
                ((u32)input->gx_local_count << 16) |
                ((u32)input->gx_seed_is_identity << 24) |
                ((u32)input->gx_valid << 25));
    }
    h = ndsFighterPacketMix(h, ident);
    for (i = 1u; i < NDS_FTR_LEAN_KEY_WORDS; i++)
    {
        h = ndsFighterPacketMix(h, key[i]);
    }
    return ndsFighterPacketMix(h, input_count);
}

/* An entry switch stands for the re-record the old path made when its
 * packet's key moved: every shade word derived again from this draw's prims,
 * at the live colour modulate (ApplyTint's derivation). The old recorder
 * derived them at modulate 0 while naming the live one, so a steady modulate
 * never reached them after a re-record -- a Kirby holding MRGAW's power
 * (colour 1 a full blend to black, held) drew pink after the first texture
 * fence move (BUGS.md, 2026-10-14), as would a team's shaded fighter. */
void NDS_FIGHTER_PACKET_COLD_CODE
ndsFtrLeanEntryResetShade(u32 battle_slot, const NDSFtrLeanPatchView *view)
{
    NDSFtrLeanEntryState *es;
    NDSFighterPacket *packet = ndsFtrLeanActive(battle_slot, &es);
    u32 prim_hash = 2166136261u;
    u32 i;

    if ((packet == NULL) || (view == NULL) ||
        (packet->root_count != view->root_count))
    {
        return;
    }
    for (i = 0u; i < packet->root_count; i++)
    {
        prim_hash = ndsFighterPacketMix(prim_hash,
                                        NDS_FTR_LEAN_VIEW_PRE(view, i)->prim_color);
    }
    for (i = 0u; i < packet->site_count; i++)
    {
        const NDSFighterPacketShadeSite *site = &packet->sites[i];
        u32 material_color = site->material_color;
        u32 diffuse;
        u32 ambient;
        u32 word;

        if ((site->prim_from_root != 0u) &&
            ((u32)site->root < packet->root_count))
        {
            material_color =
                NDS_FTR_LEAN_VIEW_PRE(view, site->root)->prim_color;
        }
        diffuse = ndsRendererR2MaterialColor15(
            site->light_color_1, material_color, site->use_material,
            view->modulate);
        ambient = ndsRendererR2MaterialColor15(
            site->light_color_2, material_color, site->use_material,
            view->modulate);
        diffuse = ndsRendererR2ClampDiffuseToMaterial(
            diffuse, ambient, material_color, site->use_material,
            view->modulate);
        word = diffuse | (ambient << 16);
        if (packet->words[site->index] != word)
        {
            packet->words[site->index] = word;
            ndsFtrLeanMarkDirty(es, packet->words, site->index, 1u);
        }
    }
    packet->tint_modulate = view->modulate;
    packet->tint_prim_hash = prim_hash;
    /* Slice 6: this stands for an old-path record, which keys the texture
     * fence of now (ndsFtrLeanPacketPatch's fence mirror). */
    packet->key[5] = ndsFtrLeanFenceNow();
}

u32 NDS_FTR_LEAN_DRAW_CODE
ndsFtrLeanPacketModelviewSites(u32 battle_slot, u32 **sites,
                                   u32 root_count)
{
    NDSFtrLeanEntryState *state;
    NDSFighterPacket *packet;
    u32 mask = 0u;
    u32 i;

    if (sites == NULL)
    {
        return NDS_FTR_LEAN_SITES_NONE;
    }
    packet = ndsFtrLeanActive(battle_slot, &state);
    if ((packet == NULL) || (packet->root_count != root_count))
    {
        return NDS_FTR_LEAN_SITES_NONE;
    }
    for (i = 0u; i < root_count; i++)
    {
        u32 index = packet->roots[i].seed_index;

        sites[i] = &packet->words[index];
        ndsFtrLeanMarkDirty(state, packet->words, index, 12u);
    }
    for (i = 0u; i < (u32)packet->texgen_group_count; i++)
    {
        if ((u32)packet->texgen_groups[i].root < 31u)
        {
            mask |= 1u << packet->texgen_groups[i].root;
        }
    }
    if ((packet->texgen_group_count != 0u) &&
        (state->texgen_map == NDS_FTR_LEAN_TEXGEN_MAP_NONE))
    {
        /* No site map: the generic texgen patch reads the refreshed
         * production inputs (the caller refreshes them for this draw). */
        mask |= NDS_FTR_LEAN_SITES_INPUTS;
    }
    if (state->vtx10 == 1u)
    {
        mask |= NDS_FTR_LEAN_SITES_VTX10;
    }
    else if (state->vtx10 == 2u)
    {
        mask |= NDS_FTR_LEAN_SITES_VTX10H;
    }
    return mask;
}

/* The active list's validity half. Every list is materialized against the
 * tint-tile set of its draw (the tiles it bound, and the folds it drew where
 * a colour had none) -- the set generation is part of the recorder's key[3]
 * too, so a moved set re-records there and re-materializes here; a list that
 * could not name a texture keeps the global fence; residency is proved every
 * draw except for an all-admitted list in route 1. Returns 0 or the event
 * reason (nNDSFtrLeanEvent*). */
/* P2-2p8 2026-09-27 ITCM swap: out (noinline, so no ITCM caller pulls it
 * back in) for ndsStageGxDraw; census rent under ~1,300 cycles a byte. */
u32 __attribute__((noinline))
ndsFtrLeanPacketGuard(u32 battle_slot, u32 touch)
{
    NDSFtrLeanEntryState *state;
    NDSFighterPacket *packet = ndsFtrLeanActive(battle_slot, &state);

    if (packet == NULL)
    {
        return nNDSFtrLeanEventFirst;
    }
    if (state->tint_set_generation != gNdsR2FighterTintSetGeneration)
    {
        if (state->tint_folds != 0u)
        {
            return nNDSFtrLeanEventTintSet;
        }
        /* A fold-free list bound a resident tile for every tinted epoch, so
         * a moved set (a tile built or evicted anywhere) changes at most
         * those binds' words: the patch re-points every one of them, and a
         * colour whose tile left the set re-materializes there. */
        state->tint_repatch = 1u;
    }
    if ((state->fence_needed != 0u) &&
        (state->fence != (sNdsRendererHardwareTextureKeyGeneration ^
                          (sNdsRendererRuntimeTextureCacheEvictCount << 16))))
    {
        return nNDSFtrLeanEventFence;
    }
    if ((state->all_pinned != 0u) &&
        ((NDS_FTR_LEAN_SLOW_WORD() & NDS_FTR_LEAN_SLOW_GUARD) == 0u))
    {
        return 0u;
    }
    if (ndsFighterPacketTexturesResident(packet) == FALSE)
    {
        if (state->all_pinned != 0u)
        {
            NDS_FTR_LEAN_CTR(gNdsFtrLean.pinned_residency_miss++);
        }
        return nNDSFtrLeanEventFence;
    }
    if (touch != 0u)
    {
        ndsFighterPacketTouchTextures(packet);
    }
    return 0u;
}

/* 2.6 tile-word patch: each tint bind's TEXIMAGE_PARAM/PAL_FORMAT words for
 * the live colour, read from the resident tile (the words the live draw's
 * bind would write). The colour is the root's prim unless the display list
 * overrode it, exactly as ndsRendererR2ResolveEpochShade sees it. FALSE when
 * the live draw would bind differently: a white prim draws untinted, a
 * colour with no resident tile folds (and queues the tile), or the palette
 * word's presence differs -- the list is re-materialized then. */
static s32 ndsFtrLeanPatchTintTiles(NDSFighterPacket *packet,
                                    NDSFtrLeanEntryState *state,
                                    const NDSFtrLeanPatchView *view,
                                    u32 force)
{
    const u32 touch = 1u;
    u32 i;

    for (i = 0u; i < (u32)packet->tint_bind_count; i++)
    {
        /* The list is private: its bind rgb is the colour the words were
         * last patched for (at materialization, the live one). The guard
         * holds the tint-tile set generation to the list's, so an unmoved
         * colour still names the same resident tile and its words are
         * already right; only a moved colour pays the tile lookup. */
        NDSFighterPacketTintBind *bind = &packet->tint_binds[i];
        u32 rgb = bind->rgb;
        u32 teximage = 0u;
        u32 pltt = 0xffffffffu;

        if ((bind->prim_from_root != 0u) &&
            ((u32)bind->root < view->root_count))
        {
            rgb = (NDS_FTR_LEAN_VIEW_PRE(view, bind->root)->prim_color >> 8) &
                0x00ffffffu;
        }
        if ((rgb == bind->rgb) && (force == 0u) &&
            ((NDS_FTR_LEAN_SLOW_WORD() & NDS_FTR_LEAN_SLOW_SUBMIT) == 0u))
        {
            continue;
        }
        if (rgb == 0x00ffffffu)
        {
            NDS_FTR_LEAN_CTR(gNdsFtrLean.tint_patch_white++);
            return FALSE;
        }
        if (ndsFtrLeanTintTileWords(rgb, touch, &teximage, &pltt) == FALSE)
        {
            NDS_FTR_LEAN_CTR(gNdsFtrLean.tint_patch_miss++);
            return FALSE;
        }
        if ((((u32)bind->pal_index == NDS_FIGHTER_PACKET_INDEX_NONE) ?
                 1u : 0u) != ((pltt == 0xffffffffu) ? 1u : 0u))
        {
            NDS_FTR_LEAN_CTR(gNdsFtrLean.tint_patch_shape++);
            return FALSE;
        }
        if (packet->words[bind->tex_index] != teximage)
        {
            packet->words[bind->tex_index] = teximage;
            ndsFtrLeanMarkDirty(state, packet->words, bind->tex_index, 1u);
        }
        if (((u32)bind->pal_index != NDS_FIGHTER_PACKET_INDEX_NONE) &&
            (packet->words[bind->pal_index] != pltt))
        {
            packet->words[bind->pal_index] = pltt;
            ndsFtrLeanMarkDirty(state, packet->words, bind->pal_index, 1u);
        }
        NDS_FTR_LEAN_CTR(gNdsFtrLean.tint_patch_binds++);
        if (rgb != bind->rgb)
        {
            NDS_FTR_LEAN_CTR(gNdsFtrLean.tint_patch_moved++);
        }
        bind->rgb = rgb;
    }
    return TRUE;
}

/* TryReplay's patch block (tint, matrices, light, texgen) on the active
 * list, each through the replay's own helper so the words are formatted by
 * exactly the code TryReplay uses; the projection is P' -- this frame's
 * projection with row 3 across the world-unit seam -- written once at the
 * head. Returns 0, NDS_FTR_LEAN_PATCH_REMATERIALIZE, or a decline reason. */
#if NDS_FTR_LEAN_TIMED
/* `part_kind_` (the function's own local) names the kind for the slice 5
 * per-kind split, which only the attribution ROM carries. */
#define NDS_FTR_LEAN_PART_KIND(array, index, delta) ((void)0)
#define NDS_FTR_LEAN_PART(array, index, mark)                              \
    do                                                                     \
    {                                                                      \
        u32 part_now_ = NDS_FTR_LEAN_CLOCK();                              \
        gNdsFtrLean.array[index] += part_now_ - (mark);                    \
        NDS_FTR_LEAN_PART_KIND(array, index, part_now_ - (mark));          \
        (mark) = part_now_;                                                \
    } while (0)
#else
#define NDS_FTR_LEAN_PART(array, index, mark) ((void)0)
#endif

u32 ndsFtrLeanPacketPatch(u32 battle_slot, const NDSFtrLeanPatchView *view,
                          u32 owner_slot, u32 use_low_detail, u32 pre_same)
{
    NDSFtrLeanEntryState *state;
    NDSFighterPacket *packet;
    u32 *words;
    u32 tint_modulate;
    u32 tint_prim_hash;
    u32 i;
#if NDS_FTR_LEAN_TIMED
    u32 mark = NDS_FTR_LEAN_CLOCK();
#endif

    packet = ndsFtrLeanActive(battle_slot, &state);
    if ((packet == NULL) || (view == NULL))
    {
        return nNDSFtrLeanDeclineStale;
    }
    if (packet->root_count != view->root_count)
    {
        return nNDSFtrLeanDeclineStale;
    }
    words = packet->words;
    if ((NDS_FTR_LEAN_SLOW_WORD() & NDS_FTR_LEAN_SLOW_SUBMIT) != 0u)
    {
        pre_same = FALSE;
    }
    if (((pre_same == FALSE) || (state->tint_repatch != 0u)) &&
        (ndsFtrLeanPatchTintTiles(packet, state, view,
                                  state->tint_repatch) == FALSE))
    {
        /* The live draw binds differently now: this list is done. */
        state->valid = 0u;
        return NDS_FTR_LEAN_PATCH_REMATERIALIZE;
    }
    if (state->tint_repatch != 0u)
    {
        /* The moved set is in the old path's packet key: it re-records this
         * draw, so the shade words take that record's derivation too. */
        state->tint_repatch = 0u;
        state->tint_set_generation = gNdsR2FighterTintSetGeneration;
        ndsFtrLeanEntryResetShade(battle_slot, view);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.tint_repatches++);
    }
    NDS_FTR_LEAN_PART(patch_part_ticks, 0u, mark);
    tint_modulate = packet->tint_modulate;
    tint_prim_hash = packet->tint_prim_hash;
    if ((packet->needs_fence != 0u) && (state->fence_needed == 0u) &&
        (packet->key[5] != ndsFtrLeanFenceNow()))
    {
        /* Slice 6: a list whose only unvalidated binds are tint tiles stays
         * current when the global texture fence moves (the tiles are proved
         * per tile) -- but the old path's packet kept that fence in its key
         * (needs_fence, key[5]) and re-recorded on the move. The list takes
         * that record's derivation here (at the live modulate); key[5] of a
         * lean list holds the fence of its last (virtual) record. */
        ndsFtrLeanEntryResetShade(battle_slot, view);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.fence_rerecords_mirrored++);
    }
    /* With the prims unchanged since the last patch, ApplyTint's own early
     * out reduces to the modulate compare; skip its prim hash walk. */
    else if ((pre_same == FALSE) || (tint_modulate != view->modulate))
    {
        u32 prims[NDS_FIGHTER_PACKET_ROOT_MAX];

        for (i = 0u; i < packet->root_count; i++)
        {
            prims[i] = NDS_FTR_LEAN_VIEW_PRE(view, i)->prim_color;
        }
        if (ndsFighterPacketApplyTintPrims(packet, prims,
                                           view->modulate) == FALSE)
        {
            /* Slice 6: ApplyTint failed closed on a tinted site (reserved[0])
             * because a root's prim moved -- where the old path's replay
             * re-recorded this draw. The list's words take that record's
             * derivation (at the live modulate); the tile words were
             * re-pointed above. */
            ndsFtrLeanEntryResetShade(battle_slot, view);
            NDS_FTR_LEAN_CTR(gNdsFtrLean.tint_rerecords_mirrored++);
        }
    }
    if ((packet->tint_modulate != tint_modulate) ||
        (packet->tint_prim_hash != tint_prim_hash))
    {
        for (i = 0u; i < packet->site_count; i++)
        {
            ndsFtrLeanMarkDirty(state, words, packet->sites[i].index, 1u);
        }
    }
    NDS_FTR_LEAN_PART(patch_part_ticks, 1u, mark);
    /* P': the projection one list loads once. Rows 0-2 as they are, row 3
     * RoundShiftS32Signed(., 8) (ndsRendererBuildRawHardwareMatrix's seam),
     * written only when this frame's camera moved it. */
    {
        const s32 *projection = &view->projection->m[0][0];
        u32 *site = &words[packet->projection_index];
        u32 pprime[16];
        u32 same = ((NDS_FTR_LEAN_SLOW_WORD() & NDS_FTR_LEAN_SLOW_SUBMIT) == 0u) ?
            TRUE : FALSE;
        /* A VTX_10 list's eye coordinates are 4x the VTX_16 list's but its
         * homogeneous w is still 1: row 3 takes the 4x as well (the uniform
         * clip scale, NDS_FTR_LEAN_ENTRY_VTX16). */
        const u32 row3_shift = NDS_RENDERER_HW_WORLD_UNIT_SHIFT -
            ((state->vtx10 == 1u) ? 2u : ((state->vtx10 == 2u) ? 1u : 0u));

        for (i = 0u; i < 12u; i++)
        {
            pprime[i] = (u32)projection[i];
        }
        for (i = 12u; i < 16u; i++)
        {
            pprime[i] = (u32)ndsRendererRoundShiftS32Signed(
                projection[i], row3_shift);
        }
        for (i = 0u; (same != FALSE) && (i < 16u); i++)
        {
            if (site[i] != pprime[i])
            {
                same = FALSE;
            }
        }
        if (same == FALSE)
        {
            for (i = 0u; i < 16u; i++)
            {
                site[i] = pprime[i];
            }
            ndsFtrLeanMarkDirty(state, words, packet->projection_index, 16u);
            NDS_FTR_LEAN_CTR(gNdsFtrLean.projection_patches++);
        }
    }
    NDS_FTR_LEAN_PART(patch_part_ticks, 2u, mark);
    if ((packet->light_index != NDS_FIGHTER_PACKET_INDEX_NONE) &&
        (packet->light_valid != 0u) &&
        ((u32)packet->light_root < view->root_count) &&
        ((pre_same == FALSE) || (state->light_valid == 0u)))
    {
        const NDSRendererNativeFighterPreamble *preamble =
            NDS_FTR_LEAN_VIEW_PRE(view, packet->light_root);

        if (((preamble->flags &
              NDS_RENDERER_NATIVE_PREAMBLE_LIGHT_VALID) != 0u) &&
            ((state->light_valid == 0u) ||
             ((NDS_FTR_LEAN_SLOW_WORD() & NDS_FTR_LEAN_SLOW_SUBMIT) != 0u) ||
             (state->light_dir[0] != (s32)preamble->light_dir_x) ||
             (state->light_dir[1] != (s32)preamble->light_dir_y) ||
             (state->light_dir[2] != (s32)preamble->light_dir_z)))
        {
            words[packet->light_index] = ndsFighterPacketLightWord(
                preamble->light_dir_x, preamble->light_dir_y,
                preamble->light_dir_z);
            ndsFtrLeanMarkDirty(state, words, packet->light_index, 1u);
            state->light_valid = 1u;
            state->light_dir[0] = (s32)preamble->light_dir_x;
            state->light_dir[1] = (s32)preamble->light_dir_y;
            state->light_dir[2] = (s32)preamble->light_dir_z;
            NDS_FTR_LEAN_CTR(gNdsFtrLean.light_patches++);
        }
    }
    NDS_FTR_LEAN_PART(patch_part_ticks, 3u, mark);
    if (packet->texgen_group_count != 0u)
    {
        /* The texgen words derive from the owner's runtime tables (dense
         * normals) for the root program the caller already set. */
        s32 texgen_ok;

        if (ndsRendererNativeSelectFighterRuntimeTables(
                owner_slot, use_low_detail) == FALSE)
        {
            return nNDSFtrLeanDeclineTexgen;
        }
#if NDS_P2_LINK
        if (state->texgen_map == NDS_FTR_LEAN_TEXGEN_MAP_READY)
        {
            texgen_ok = ndsFtrLeanPatchTexgenMapped(
                packet, ndsFtrLeanTexgenMapOf(packet), view->worlds,
                view->root_count);
        }
        else if (state->texgen_map == NDS_FTR_LEAN_TEXGEN_MAP_REFUSE)
        {
            texgen_ok = FALSE;
        }
        else
#endif
        {
            /* No site map: the generic patch reads the refreshed production
             * inputs (the caller refreshed them: NDS_FTR_LEAN_SITES_INPUTS). */
            texgen_ok = (view->inputs != NULL) ?
                ndsFighterPacketPatchTexgen(packet, view->inputs,
                                            view->root_count) : FALSE;
        }
        if (texgen_ok == FALSE)
        {
            return nNDSFtrLeanDeclineTexgen;
        }
        if (state->texgen_first <= state->texgen_last)
        {
            ndsFtrLeanMarkDirty(state, words, state->texgen_first,
                                state->texgen_last - state->texgen_first +
                                    1u);
        }
        NDS_FTR_LEAN_CTR(gNdsFtrLean.texgen_patches++);
    }
    NDS_FTR_LEAN_PART(patch_part_ticks, 4u, mark);
    (void)owner_slot;
    (void)use_low_detail;
    return 0u;
}

/* TryReplay's submit tail on the active list: only the lines the patches
 * wrote are cleaned (the list was cleaned whole when it was materialized).
 * Returns the hardware triangles the list presents.
 *
 * Slice 5: the frame-summary half of TryReplay's accounting
 * (ndsRendererFastAccountRawTriangles / ndsRendererNativeAccountGXCross-
 * Triangles, field for field) is written here directly, before the DMA
 * starts. The draw's own NDSRendererStats (a 1,300 B static) was per-draw
 * scratch nobody read but the hardware triangle count, which is returned; the
 * materializer and the admission now take a stack block for the calls that
 * need a whole one. */
u32 NDS_FTR_LEAN_DRAW_CODE
ndsFtrLeanPacketSubmit(u32 battle_slot)
{
    NDSFtrLeanEntryState *state;
    NDSFighterPacket *packet;
    u32 *words;
    u32 flushed;
    u32 raw;
    u32 cross;
#if NDS_FTR_LEAN_TIMED
    u32 mark = NDS_FTR_LEAN_CLOCK();
#endif

    packet = ndsFtrLeanActive(battle_slot, &state);
    if (packet == NULL)
    {
        return 0u;
    }
    words = packet->words;
    if ((NDS_FTR_LEAN_SLOW_WORD() & NDS_FTR_LEAN_SLOW_SUBMIT) != 0u)
    {
        /* Slice 1's form: clean the whole list. */
        memset(state->dirty, 0, sizeof(state->dirty));
        flushed = packet->word_count * sizeof(u32);
        DC_FlushRange(words, flushed);
    }
    else
    {
        flushed = ndsFtrLeanFlushDirty(state, words);
        if ((state->valid & NDS_FTR_LEAN_VALID_WIDE) != 0u)
        {
            /* Slice 6: a wide list's patch sites run past the entry-sized
             * dirty marks, so the lines past them are cleaned every draw
             * and the marked ones below as any list's (2026-10-18: four
             * Palutenas' wide lists, ~400 lines each, were cleaned whole
             * every frame). */
            const uintptr_t marked_end = ((uintptr_t)words & ~(uintptr_t)31u) +
                (NDS_FTR_LEAN_DIRTY_LINES * 32u);
            const uintptr_t end = (uintptr_t)(words + packet->word_count);

            if (end > marked_end)
            {
                DC_FlushRange((const void *)marked_end, (u32)(end - marked_end));
                flushed += (u32)(end - marked_end);
            }
        }
    }
    NDS_FTR_LEAN_CTR(gNdsFtrLean.flush_bytes += flushed);
    (void)flushed;
    NDS_FTR_LEAN_PART(submit_part_ticks, 0u, mark);

    ndsRendererHardwareEndBatch();
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_FOG);
    raw = packet->raw_triangles;
    cross = packet->cross_triangles;
    if (raw != 0u)
    {
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX] += raw;
        sNdsRendererRuntimeFrameSummary.raw_current_candidate_count += raw;
        sNdsRendererRuntimeFrameSummary.hardware_batch_reuse_count +=
            packet->raw_reuse;
        sNdsRendererRuntimeFrameSummary.texture_prepare_reuse_count +=
            packet->raw_reuse;
        sNdsRendererRuntimeFrameSummary.hardware_triangles += raw;
        sNdsRendererRuntimeFrameSummary.hardware_vertices += raw * 3u;
        sNdsRendererHardwareSubmitted = TRUE;
    }
    if (cross != 0u)
    {
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_PROJECTED_CROSS_MATRIX] += cross;
        sNdsRendererRuntimeFrameSummary.raw_cross_matrix_count += cross;
        sNdsRendererRuntimeFrameSummary.hardware_batch_reuse_count +=
            packet->cross_reuse;
        sNdsRendererRuntimeFrameSummary.texture_prepare_reuse_count +=
            packet->cross_reuse;
        sNdsRendererRuntimeFrameSummary.hardware_triangles += cross;
        sNdsRendererRuntimeFrameSummary.hardware_vertices += cross * 3u;
        if ((sNdsRendererRuntimeFrameSummary.hardware_triangles > 2048u) ||
            (sNdsRendererRuntimeFrameSummary.hardware_vertices > 6144u))
        {
            sNdsRendererRuntimeFrameSummary.hardware_over_limit = 1u;
        }
        sNdsRendererHardwareSubmitted = TRUE;
    }
    sNdsRendererRuntimeFrameSummary.hardware_batch_begin_count +=
        packet->batch_begin;
    sNdsRendererRuntimeFrameSummary.hardware_batch_reuse_count +=
        packet->batch_reuse;
    sNdsRendererRuntimeFrameSummary.hardware_batch_end_count +=
        packet->batch_end;
    sNdsRendererRuntimeFrameSummary.texture_prepare_count +=
        packet->prepare_begin;
    sNdsRendererRuntimeFrameSummary.texture_prepare_reuse_count +=
        packet->prepare_reuse;
    sNdsRendererRuntimeFrameSummary.matrix_load_count +=
        packet->matrix_loads;
    sNdsRendererRuntimeFrameSummary.texture_binds +=
        packet->texture_binds;
    sNdsRendererHardwareSourceVertexLoadCount += packet->vertex_loads;
    sNdsRendererFastRunCount += packet->run_count;
    sNdsRendererFastTriangleCount += packet->triangle_count;
    if ((u32)sNdsRendererRuntimeOwner <
        (u32)NDS_RENDERER_PROFILE_OWNER_COUNT)
    {
        sNdsRendererFastOwnerTriangleCount[
            (u32)sNdsRendererRuntimeOwner] += packet->triangle_count;
    }
    NDS_FTR_LEAN_PART(submit_part_ticks, 3u, mark);
    if ((DMA_CR(0) & DMA_BUSY) != 0u)
    {
        u32 wait_start = cpuGetTiming();
        u32 waited;

        ndsGxDma0WaitSlow(6u);
        waited = cpuGetTiming() - wait_start;
        NDS_FTR_LEAN_CTR(gNdsFtrLean.dma_wait_ticks += waited);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.dma_wait_spins++);
#if NDS_FTR_LEAN_LAB
        if (sNdsFtrLeanSlots[battle_slot].kind < NDS_FTR_LEAN_KINDS)
        {
            gNdsFtrLean.k_dma_wait_ticks[sNdsFtrLeanSlots[battle_slot].kind] +=
                waited;
        }
#endif
        (void)waited;
    }
    NDS_FTR_LEAN_PART(submit_part_ticks, 1u, mark);
    DMA_SRC(0) = (u32)(uintptr_t)words;
    DMA_DEST(0) = (u32)(uintptr_t)&GFX_FIFO;
    DMA_CR(0) = DMA_FIFO | packet->word_count;
    sNdsFighterPacketDmaPending = 1u;

    ndsRendererHardwareInvalidateGXState(NDS_RENDERER_GX_STATE_ALL);
    sNdsRendererHardwareBoundTextureName = 0u;
    ndsRendererForgetLibndsTexture();
    sNdsRendererHardwareActiveTextureEntry = NULL;
    sNdsR2GxLastProjection = NULL;
    sNdsRendererHardwareMatrixMode =
        NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED;
    sNdsRendererHardwareMatrixGeneration = ndsRendererNextMatrixGeneration();
    sNdsRendererHardwareMatrixLoaded = FALSE;
    NDS_FTR_LEAN_PART(submit_part_ticks, 2u, mark);
    return raw + cross;
}

/* P4 S4, Remix SET ENV COLOR (0xD9): a content whose env-colour alpha is
 * under 0xFF draws translucent, as Remix's alpha render modes draw it. The
 * active list's POLY_FORMAT alphas are scaled in place for its one DMA and
 * put back before the next lean draw or frame end
 * (ndsFtrLeanPacketAlphaRestore): the DMA reads them asynchronously, and a
 * rematerialization must find the list as it was built. */
#define NDS_FTR_LEAN_ALPHA_SITES 192u
static u32 *sNdsFtrLeanAlphaWords;
static u32 sNdsFtrLeanAlphaBytes;
static u32 sNdsFtrLeanAlphaCount;
static u16 sNdsFtrLeanAlphaSite[NDS_FTR_LEAN_ALPHA_SITES];
static u8 sNdsFtrLeanAlphaOrig[NDS_FTR_LEAN_ALPHA_SITES];
u32 gNdsFtrLeanAlphaPending;

/* Parameter words of a packed geometry command (GBATEK, 0x10-0x72). */
static u32 ndsFtrLeanGxParams(u32 op)
{
    switch (op)
    {
    case 0x00u: case 0x11u: case 0x15u: case 0x41u:
        return 0u;
    case 0x16u: case 0x18u:
        return 16u;
    case 0x17u: case 0x19u:
        return 12u;
    case 0x1Au:
        return 9u;
    case 0x1Bu: case 0x1Cu: case 0x70u:
        return 3u;
    case 0x23u: case 0x71u:
        return 2u;
    case 0x34u:
        return 32u;
    default:
        return 1u;
    }
}

void __attribute__((noinline, cold)) ndsFtrLeanPacketAlphaRestore(void)
{
    u32 i;

    if (gNdsFtrLeanAlphaPending == 0u)
    {
        return;
    }
    if ((DMA_CR(0) & DMA_BUSY) != 0u)
    {
        ndsGxDma0WaitSlow(6u);
    }
    for (i = 0u; i < sNdsFtrLeanAlphaCount; i++)
    {
        u32 *w = &sNdsFtrLeanAlphaWords[sNdsFtrLeanAlphaSite[i]];

        *w = (*w & ~(31u << 16)) | ((u32)sNdsFtrLeanAlphaOrig[i] << 16);
    }
    DC_FlushRange(sNdsFtrLeanAlphaWords, sNdsFtrLeanAlphaBytes);
    gNdsFtrLeanAlphaPending = 0u;
}

void __attribute__((noinline, cold)) ndsFtrLeanPacketSetAlpha(u32 battle_slot,
                                                             u32 alpha)
{
    NDSFtrLeanEntryState *state;
    NDSFighterPacket *packet;
    u32 *words;
    u32 count;
    u32 alpha5;
    u32 sites = 0u;
    u32 i = 0u;

    ndsFtrLeanPacketAlphaRestore();
    packet = ndsFtrLeanActive(battle_slot, &state);
    alpha5 = (alpha * 31u + 127u) / 255u;
    if ((packet == NULL) || (alpha5 >= 31u))
    {
        return;
    }
    if (alpha5 == 0u)
    {
        alpha5 = 1u;    /* POLY_ALPHA(0) is wireframe */
    }
    if ((DMA_CR(0) & DMA_BUSY) != 0u)
    {
        ndsGxDma0WaitSlow(6u);
    }
    words = packet->words;
    count = packet->word_count;
    while (i < count)
    {
        u32 header = words[i++];
        u32 params = 0u;
        u32 s;

        for (s = 0u; s < 4u; s++)
        {
            u32 op = (header >> (s * 8u)) & 0xFFu;
            u32 n = ndsFtrLeanGxParams(op);

            if ((op == (u32)REG2ID(GFX_POLY_FORMAT)) && (i < count) &&
                (sites < NDS_FTR_LEAN_ALPHA_SITES))
            {
                u32 w = words[i];
                u32 a = (w >> 16) & 31u;

                if (a != 0u)
                {
                    u32 scaled = (a * alpha5 + 15u) / 31u;

                    sNdsFtrLeanAlphaSite[sites] = (u16)i;
                    sNdsFtrLeanAlphaOrig[sites] = (u8)a;
                    sites++;
                    words[i] = (w & ~(31u << 16)) |
                               (((scaled != 0u) ? scaled : 1u) << 16);
                }
            }
            i += n;
            params += n;
        }
        if (params == 0u)
        {
            i++;    /* the packer's pad after a parameterless header */
        }
    }
    sNdsFtrLeanAlphaWords = words;
    sNdsFtrLeanAlphaBytes = count * sizeof(u32);
    sNdsFtrLeanAlphaCount = sites;
    DC_FlushRange(words, sNdsFtrLeanAlphaBytes);
    gNdsFtrLeanAlphaPending = (sites != 0u) ? 1u : 0u;
    (void)state;
}

#if NDS_FTR_LEAN_LAB
/* Census of the texture cache; also the admission's print-first number. */
static void ndsFtrLeanCensusCache(u32 *out)
{
    u32 live = 0u;
    u32 free_slots = 0u;
    u32 pinned = 0u;
    u32 statics = 0u;
    u32 this_frame = 0u;
    u32 evictable = 0u;
    u32 live_bytes = 0u;
    u32 i;

    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        const NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[i];

        if (entry->name == 0)
        {
            free_slots++;
            continue;
        }
        live++;
        live_bytes += ndsFtrLeanTextureBytes(entry->params);
        if (entry->static_record_plus1 != 0u)
        {
            statics++;
        }
        if (entry->pinned != 0u)
        {
            pinned++;
        }
        else if (entry->last_used_frame ==
                 (sNdsRendererHardwareFrameSerial + 1u))
        {
            this_frame++;
        }
        else
        {
            evictable++;
        }
    }
    out[0] = live;
    out[1] = free_slots;
    out[2] = pinned;
    out[3] = statics;
    out[4] = this_frame;
    out[5] = evictable;
    out[6] = live_bytes;
}
#endif

void ndsFtrLeanTextureCensus(void)
{
#if NDS_FTR_LEAN_LAB
    u32 census[7];
    u32 fighter_bytes = 0u;
    u32 slot;

    ndsFtrLeanCensusCache(census);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.census_live = census[0]);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.census_free = census[1]);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.census_pinned = census[2]);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.census_static = census[3]);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.census_live_bytes = census[6]);
    for (slot = 0u; slot < NDS_FIGHTER_PACKET_SLOTS; slot++)
    {
        u32 count = 0u;
        u32 bytes = 0u;
        u32 i;

        for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
        {
            if ((sNdsFtrLeanSlots[slot].union_mask[i >> 5] &
                 (1u << (i & 31u))) != 0u)
            {
                count++;
                if (sNdsRendererHardwareTextureCache[i].name != 0)
                {
                    bytes += ndsFtrLeanTextureBytes(
                        sNdsRendererHardwareTextureCache[i].params);
                }
            }
        }
        NDS_FTR_LEAN_CTR(gNdsFtrLean.union_textures[slot] = count);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.union_bytes[slot] = bytes);
        fighter_bytes += bytes;
    }
    NDS_FTR_LEAN_CTR(gNdsFtrLean.census_fighter_bytes = fighter_bytes);
#endif
}

void ndsFtrLeanNoteGo(u32 frame)
{
#if NDS_FTR_LEAN_LAB
    if (gNdsFtrLean.go_frame == 0u)
    {
        NDS_FTR_LEAN_CTR(gNdsFtrLean.go_frame = (frame != 0u) ? frame : 1u);
        ndsFtrLeanTextureCensus();
    }
#else
    (void)frame;
#endif
}

/* Texture reject witness: the first fighter-owned reject since boot, with
 * the cache census at that instant (the P0 entry failure's root cause). */
void ndsFtrLeanNoteTextureReject(u32 reason, u32 format, u32 size)
{
#if NDS_FTR_LEAN_LAB
    if (ndsFtrLeanOwnerIsFighter() == FALSE)
    {
        return;
    }
    NDS_FTR_LEAN_CTR(gNdsFtrLean.reject_count++);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.reject_mask |= reason);
    if (gNdsFtrLean.reject_first[0] == 0u)
    {
        u32 census[7];

        ndsFtrLeanCensusCache(census);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.reject_first[0] = reason);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.reject_first[1] = format);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.reject_first[2] = size);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.reject_first[3] = (u32)sNdsRendererRuntimeOwner);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.reject_first[4] = gNdsRendererProfileFrameCount);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.reject_first[5] = gNdsFtrLean.go_frame);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.reject_first[6] = census[0]);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.reject_first[7] = census[1]);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.reject_first[8] = census[2]);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.reject_first[9] = census[4]);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.reject_first[10] = census[5]);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.reject_first[11] = census[6]);
#if NDS_VRAM_CENSUS_LIVE
        /* Slice 2a: the whole texture/palette VRAM at the entry burst. */
        ndsVramCensusCaptureBurst();
#endif
    }
#else
    (void)reason;
    (void)format;
    (void)size;
#endif
}

void ndsFtrLeanNoteTextureUpload(u32 bytes)
{
#if NDS_FTR_LEAN_LAB
    if (ndsFtrLeanOwnerIsFighter() == FALSE)
    {
        return;
    }
    NDS_FTR_LEAN_CTR(gNdsFtrLean.fighter_uploads++);
    if (gNdsFtrLean.go_frame != 0u)
    {
        NDS_FTR_LEAN_CTR(gNdsFtrLean.fighter_uploads_after_go++);
        NDS_FTR_LEAN_CTR(gNdsFtrLean.fighter_upload_bytes_after_go += bytes);
    }
#else
    (void)bytes;
#endif
}

#if NDS_VRAM_CENSUS_LIVE
/* Slice 2b lab words outside this TU's structs (nds_platform.c). */
extern volatile u32 gNdsVramBankDLent;
extern volatile u32 gNdsVramBg3RefusedWrites;
extern volatile u32 gNdsVramBankDTakes;
extern volatile u32 gNdsVramBankDReturns;
#endif

void ndsFtrLeanCountersPublish(void)
{
#if NDS_FTR_LEAN_LAB
    DC_FlushRange(&gNdsFtrLean, sizeof(gNdsFtrLean));
#endif
#if NDS_VRAM_CENSUS_LIVE
    /* Slice 2b: the admission lab and the words the sampler reads once at
     * the end -- flushed every frame so a GDB read is never a stale line. */
    DC_FlushRange(&gNdsFtrAdmitLab, sizeof(gNdsFtrAdmitLab));
    DC_FlushRange((const void *)&gNdsFtrAdmitDynReadyHigh, sizeof(u32));
    DC_FlushRange((const void *)&gNdsFtrAdmitDynAdmittedHigh, sizeof(u32));
    DC_FlushRange((const void *)&gNdsFtrAdmitDynTouchedHigh, sizeof(u32));
    DC_FlushRange((const void *)&gNdsFtrAdmitDynFull, sizeof(u32));
    DC_FlushRange((const void *)&gNdsFtrLeanAdmitFail, sizeof(u32));
    DC_FlushRange((const void *)gNdsFtrLeanAdmitFailFirst,
                  sizeof(gNdsFtrLeanAdmitFailFirst));
    DC_FlushRange((const void *)&gNdsVramBankDLent, sizeof(u32));
    DC_FlushRange((const void *)&gNdsVramBg3RefusedWrites, sizeof(u32));
    DC_FlushRange((const void *)&gNdsVramBankDTakes, sizeof(u32));
    DC_FlushRange((const void *)&gNdsVramBankDReturns, sizeof(u32));
#endif
}
