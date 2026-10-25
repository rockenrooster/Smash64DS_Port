/* Native renderer, shared pools and stage binding: the Polygon Team's image
 * pool, texgen owners, the native stage's dense records and its bind
 * workspace. A slice of nds_renderer.c. */

#if NDS_1P_GAME
/* THE POLYGON TEAM SHARES FIVE IMAGE BUFFERS, NOT TWELVE.
 *
 * sc1PGame stage 12 loads every polygon kind at FuncStart and replaces a
 * defeated polygon mid-battle with the next variation; at most three are
 * alive, plus the one being replaced, whose GObj is still linked when its
 * successor is made. Twelve resident images (the old wave preload) were ~170 KB
 * of an arena that reached 5,248 B free. The images now live in a pool of
 * NDS_NATIVE_POLYGON_POOL_ENTRIES buffers, each the largest polygon image:
 * fighter construction -- load time for the first wave, wave replacement
 * afterwards, the same mid-battle stream a Kirby copy hat makes -- takes a free
 * buffer, or the least recently loaded one whose kind no linked fighter uses
 * after unbinding that kind's tables. A reload binds through NDS_IMG_BIND,
 * which forgets the table's UV memo, so fresh prepared_dense is rebuilt.
 *
 * ftmanager arms the pool for the scene's arena generation, so a later scene
 * that ensures a polygon image (a CSS preview, Results) allocates as before. */
#define NDS_NATIVE_POLYGON_POOL_ENTRIES 5u
#define NDS_NATIVE_POLYGON_POOL_FREE 0xffu

static struct
{
    u8 *base;
    u32 stride;
    u32 heap_generation;
    u32 armed_generation;
    u32 clock;
    u8 owner[NDS_NATIVE_POLYGON_POOL_ENTRIES];
    u32 loaded_at[NDS_NATIVE_POLYGON_POOL_ENTRIES];
} sNdsNativePolygonPool = { NULL, 0u, 0u, 0xffffffffu, 0u, { 0u }, { 0u } };
__attribute__((used)) volatile u32 gNdsNativePolygonPoolBytes;
__attribute__((used)) volatile u32 gNdsNativePolygonPoolEvictions;
__attribute__((used)) volatile u32 gNdsNativePolygonPoolFull;

NDS_RENDERER_COLD void ndsRendererNativeArmPolygonImagePool(void)
{
    sNdsNativePolygonPool.armed_generation = gNdsTaskmanHeapGeneration;
}

static s32 ndsRendererNativePolygonPoolOwns(u32 owner_slot)
{
    return ((owner_slot >= NDS_NATIVE_IMAGE_SLOT_NMARIO) &&
            (owner_slot <= NDS_NATIVE_IMAGE_SLOT_NNESS) &&
            (sNdsNativePolygonPool.armed_generation ==
             gNdsTaskmanHeapGeneration)) ? TRUE : FALSE;
}

static void ndsRendererNativePolygonPoolUnbind(u32 owner_slot)
{
    u32 detail;

    for (detail = 0u; detail < NDS_NATIVE_IMAGE_DETAILS; detail++)
    {
        NDSNativeOwnerImageSlot *slot =
            &sNdsNativeOwnerImage[owner_slot][detail];

        if (slot->base != NULL)
        {
            ndsRendererNativeBindOwnerImage(owner_slot, detail, NULL);
            slot->base = NULL;
            slot->heap_generation = 0u;
            slot->bytes = 0u;
        }
    }
}

NDS_RENDERER_COLD static void *ndsRendererNativePolygonPoolTake(u32 owner_slot, u32 bytes)
{
    u32 entry = NDS_NATIVE_POLYGON_POOL_ENTRIES;
    u32 i;

    if ((sNdsNativePolygonPool.base == NULL) ||
        (sNdsNativePolygonPool.heap_generation != gNdsTaskmanHeapGeneration))
    {
        u32 stride = 0u;
        u32 slot;
        u32 detail;

        for (slot = NDS_NATIVE_IMAGE_SLOT_NMARIO;
             slot <= NDS_NATIVE_IMAGE_SLOT_NNESS; slot++)
        {
            for (detail = 0u; detail < NDS_NATIVE_IMAGE_DETAILS; detail++)
            {
                u32 image_bytes =
                    ndsRendererNativeOwnerImageBytes(slot, detail);

                if (image_bytes > stride)
                {
                    stride = image_bytes;
                }
            }
        }
        stride = (stride + 0xfu) & ~0xfu;
        sNdsNativePolygonPool.base = (stride != 0u) ?
            (u8 *)ndsSceneAssetAlloc(
                stride * NDS_NATIVE_POLYGON_POOL_ENTRIES, 0x10u) : NULL;
        if (sNdsNativePolygonPool.base == NULL)
        {
            return NULL;
        }
        sNdsNativePolygonPool.stride = stride;
        sNdsNativePolygonPool.heap_generation = gNdsTaskmanHeapGeneration;
        sNdsNativePolygonPool.clock = 0u;
        for (i = 0u; i < NDS_NATIVE_POLYGON_POOL_ENTRIES; i++)
        {
            sNdsNativePolygonPool.owner[i] = NDS_NATIVE_POLYGON_POOL_FREE;
            sNdsNativePolygonPool.loaded_at[i] = 0u;
        }
        gNdsNativePolygonPoolBytes = stride * NDS_NATIVE_POLYGON_POOL_ENTRIES;
    }
    if (bytes > sNdsNativePolygonPool.stride)
    {
        return NULL;
    }
    for (i = 0u; i < NDS_NATIVE_POLYGON_POOL_ENTRIES; i++)
    {
        if (sNdsNativePolygonPool.owner[i] == NDS_NATIVE_POLYGON_POOL_FREE)
        {
            entry = i;
            break;
        }
    }
    if (entry == NDS_NATIVE_POLYGON_POOL_ENTRIES)
    {
        u32 oldest = 0xffffffffu;

        for (i = 0u; i < NDS_NATIVE_POLYGON_POOL_ENTRIES; i++)
        {
            if ((sNdsNativePolygonPool.loaded_at[i] < oldest) &&
                (ndsFTManagerOwnerImageSlotLive(
                     sNdsNativePolygonPool.owner[i]) == FALSE))
            {
                oldest = sNdsNativePolygonPool.loaded_at[i];
                entry = i;
            }
        }
        if (entry == NDS_NATIVE_POLYGON_POOL_ENTRIES)
        {
            NDS_DIAG(gNdsNativePolygonPoolFull++);
            return NULL;
        }
        ndsRendererNativePolygonPoolUnbind(sNdsNativePolygonPool.owner[entry]);
        NDS_DIAG(gNdsNativePolygonPoolEvictions++);
    }
    sNdsNativePolygonPool.owner[entry] = (u8)owner_slot;
    sNdsNativePolygonPool.loaded_at[entry] = ++sNdsNativePolygonPool.clock;
    return sNdsNativePolygonPool.base + (entry * sNdsNativePolygonPool.stride);
}
#endif

NDS_RENDERER_COLD s32 ndsRendererNativeEnsureOwnerImage(u32 owner_slot, u32 use_low_detail)
{
    NdsRelocAssetStream stream;
    NDSNativeOwnerImageSlot *slot;
    const char *path;
    u32 bytes;
    void *buffer;

    if ((owner_slot >= NDS_NATIVE_IMAGE_OWNER_SLOTS_ALL) ||
        (use_low_detail >= NDS_NATIVE_IMAGE_DETAILS))
    {
        return FALSE;
    }
    slot = &sNdsNativeOwnerImage[owner_slot][use_low_detail];
    if ((slot->base != NULL) &&
        (slot->heap_generation == gNdsTaskmanHeapGeneration))
    {
        return TRUE;
    }
    path = ndsRendererNativeOwnerImagePath(owner_slot, use_low_detail);
    bytes = ndsRendererNativeOwnerImageBytes(owner_slot, use_low_detail);
    if ((path == NULL) || (bytes == 0u))
    {
        return FALSE;
    }
    /* An owner whose two details the generator proved to be one image binds
     * the second detail to the bytes the first already loaded. The Polygon
     * Team preloads both details of all twelve polygons, so reading them twice
     * was 165 KB the stage could not spare. Both slots then point into one
     * buffer, and ndsRendererNativeReleaseOwnerImagesInRange unbinds both
     * when it goes. The shared prepared_dense scratch is written whole by
     * each draw from the same baked inputs, exactly as two fighters of one
     * owner and detail already share it. */
#define NDS_NATIVE_OWNER_IMAGE_SHARED_ROW(slot_)                               \
    if (owner_slot == (slot_))                                                 \
    {                                                                          \
        const NDSNativeOwnerImageSlot *other_ =                                \
            &sNdsNativeOwnerImage[owner_slot][use_low_detail ^ 1u];            \
        if ((other_->base != NULL) &&                                          \
            (other_->heap_generation == gNdsTaskmanHeapGeneration) &&          \
            (other_->bytes == bytes))                                          \
        {                                                                      \
            slot->base = other_->base;                                         \
            slot->heap_generation = other_->heap_generation;                   \
            slot->bytes = other_->bytes;                                       \
            gNdsNativeOwnerImageSharedCount++;                                 \
            ndsRendererNativeBindOwnerImage(owner_slot, use_low_detail,        \
                                            slot->base);                       \
            return TRUE;                                                       \
        }                                                                      \
    }
    NDS_NATIVE_OWNER_IMAGE_SHARED_DETAIL_ROWS(NDS_NATIVE_OWNER_IMAGE_SHARED_ROW)
#undef NDS_NATIVE_OWNER_IMAGE_SHARED_ROW
    /* Battle-lifetime, like the figatree heaps: in a two-player VS match the
     * idle fighter-packet ports carry a scratch block the taskman arena never
     * sees (gcSetupObjman), so an image that fits there leaves the arena its
     * bytes. Kirby's high image alone is 30,160 of them, which is what froze
     * Kongo Jungle at load. The scratch is empty in every other scene and
     * re-seeded with the arena at each battle entry, and the slot's
     * heap_generation retires the image on exactly that boundary. */
#if NDS_1P_GAME
    if (ndsRendererNativePolygonPoolOwns(owner_slot) != FALSE)
    {
        buffer = ndsRendererNativePolygonPoolTake(owner_slot, bytes);
    }
    else
#endif
    {
        buffer = ndsBattleIdleScratchAlloc(bytes, 0x10u);
        if (buffer != NULL)
        {
            gNdsNativeOwnerImageScratchBytes += bytes;
        }
        else
        {
            buffer = ndsSceneAssetAlloc(bytes, 0x10u);
        }
    }
    if (buffer == NULL)
    {
        gNdsNativeOwnerImageFailCount++;
        return FALSE;
    }
    if (ndsRelocAssetStreamOpen(&stream, path) == FALSE)
    {
        gNdsNativeOwnerImageFailCount++;
        return FALSE;
    }
    if (ndsRelocAssetStreamRead(&stream, 0u, buffer, bytes) == FALSE)
    {
        ndsRelocAssetStreamClose(&stream);
        gNdsNativeOwnerImageFailCount++;
        return FALSE;
    }
    ndsRelocAssetStreamClose(&stream);
    /* Same-size stale images would decode wrong: reject anything that does
     * not carry this ABI's tag before binding a single member. The buffer is
     * 16-aligned, so the word load is safe. */
    if (*(const u32 *)buffer != (u32)NDS_NATIVE_OWNER_IMAGE_ABI_TAG)
    {
        gNdsNativeOwnerImageFailCount++;
        return FALSE;
    }
    slot->base = buffer;
    slot->heap_generation = gNdsTaskmanHeapGeneration;
    slot->bytes = bytes;
    gNdsNativeOwnerImageLoadCount++;
    gNdsNativeOwnerImageBytes += bytes;
    ndsRendererNativeBindOwnerImage(owner_slot, use_low_detail, buffer);
    return TRUE;
}

static void ndsRendererNativeBuildDenseShadeWords(
    const NDSNativeDenseVertex *vertices, u32 vertex_count,
    const NDSNativeRun *runs, u32 run_count,
    const u16 *run_first_unique, const u8 *run_unique_count,
    const u16 *run_unique_dense, u32 *dense_words);



NDS_RENDERER_COLD void ndsRendererNativeReleaseOwnerImagesInRange(const void *base, size_t size)
{
    uintptr_t start;
    uintptr_t end;
    u32 owner_slot;
    u32 detail;

    if ((base == NULL) || (size == 0u))
    {
        return;
    }
    start = (uintptr_t)base;
    end = start + (uintptr_t)size;
    if (end < start)
    {
        return;
    }
    for (owner_slot = 0u; owner_slot < NDS_NATIVE_IMAGE_OWNER_SLOTS_ALL;
         owner_slot++)
    {
        for (detail = 0u; detail < NDS_NATIVE_IMAGE_DETAILS; detail++)
        {
            NDSNativeOwnerImageSlot *slot =
                &sNdsNativeOwnerImage[owner_slot][detail];
            uintptr_t addr = (uintptr_t)slot->base;

            if ((slot->base != NULL) && (addr >= start) && (addr < end))
            {
                /* The runtime tables point inside slot->base. Clear those
                 * aliases before the resettable arena can reuse the bytes, so
                 * any stale draw fails closed instead of following freed RAM. */
                ndsRendererNativeBindOwnerImage(owner_slot, detail, NULL);
                slot->base = NULL;
                slot->heap_generation = 0u;
                slot->bytes = 0u;
            }
        }
    }
#if NDS_1P_GAME
    if ((sNdsNativePolygonPool.base != NULL) &&
        ((uintptr_t)sNdsNativePolygonPool.base >= start) &&
        ((uintptr_t)sNdsNativePolygonPool.base < end))
    {
        sNdsNativePolygonPool.base = NULL;
    }
#endif
}

static const NDSNativeFighterOwnerRuntime *
ndsRendererNativeFighterCanonicalOwnerForDetail(u32 slot, u32 use_low_detail)
{
    if (slot == 0u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeMarioLowOwner : &sNdsNativeMarioHighOwner;
    }
    if (slot == 1u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeFoxLowOwner : &sNdsNativeFoxHighOwner;
    }
#if NDS_LUIGI
    if (slot == 2u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeLuigiLowOwner : &sNdsNativeLuigiHighOwner;
    }
#endif
#if NDS_DONKEY
    if (slot == 3u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeDonkeyLowOwner : &sNdsNativeDonkeyHighOwner;
    }
#endif
#if NDS_CAPTAIN
    if (slot == 4u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeCaptainLowOwner : &sNdsNativeCaptainHighOwner;
    }
#endif
    if (slot == 5u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeSamusLowOwner : &sNdsNativeSamusHighOwner;
    }
#if NDS_LINK
    if (slot == 6u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeLinkLowOwner : &sNdsNativeLinkHighOwner;
    }
#endif
    if (slot == 7u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativePikachuLowOwner : &sNdsNativePikachuHighOwner;
    }
    if (slot == 8u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeYoshiLowOwner : &sNdsNativeYoshiHighOwner;
    }
    if (slot == 9u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeNessLowOwner : &sNdsNativeNessHighOwner;
    }
    if (slot == 10u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativePurinLowOwner : &sNdsNativePurinHighOwner;
    }
    if (slot == 11u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeKirbyLowOwner : &sNdsNativeKirbyHighOwner;
    }
#if NDS_MMARIO
    if (slot == 12u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeMMarioLowOwner : &sNdsNativeMMarioHighOwner;
    }
#endif
#if NDS_NMARIO
    if (slot == 13u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeNMarioLowOwner : &sNdsNativeNMarioHighOwner;
    }
#endif
#if NDS_NFOX
    if (slot == 14u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeNFoxLowOwner : &sNdsNativeNFoxHighOwner;
    }
#endif
#define NDS_P4_OWNER_LOOKUP(id_, T_, N_, n_, parent_, model_, main_)           \
    if (slot == NDS_P4_RENDERER_OWNER_SLOT(id_))                                \
    {                                                                           \
        return (use_low_detail != 0u) ?                                         \
            &sNdsNative##T_##LowOwner : &sNdsNative##T_##HighOwner;             \
    }
    NDS_P4_CONTENT_ROWS(NDS_P4_OWNER_LOOKUP)
#undef NDS_P4_OWNER_LOOKUP
#if NDS_NDONKEY
    if (slot == 15u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeNDonkeyLowOwner : &sNdsNativeNDonkeyHighOwner;
    }
#endif
#if NDS_NSAMUS
    if (slot == 16u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeNSamusLowOwner : &sNdsNativeNSamusHighOwner;
    }
#endif
#if NDS_NLINK
    if (slot == 17u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeNLinkLowOwner : &sNdsNativeNLinkHighOwner;
    }
#endif
#if NDS_NYOSHI
    if (slot == 18u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeNYoshiLowOwner : &sNdsNativeNYoshiHighOwner;
    }
#endif
#if NDS_NCAPTAIN
    if (slot == 19u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeNCaptainLowOwner : &sNdsNativeNCaptainHighOwner;
    }
#endif
#if NDS_NKIRBY
    if (slot == 20u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeNKirbyLowOwner : &sNdsNativeNKirbyHighOwner;
    }
#endif
#if NDS_NPIKACHU
    if (slot == 21u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeNPikachuLowOwner : &sNdsNativeNPikachuHighOwner;
    }
#endif
#if NDS_NPURIN
    if (slot == 22u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeNPurinLowOwner : &sNdsNativeNPurinHighOwner;
    }
#endif
#if NDS_NNESS
    if (slot == 23u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeNNessLowOwner : &sNdsNativeNNessHighOwner;
    }
#endif
#if NDS_1P_GAME
    if (slot == 24u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeBossLowOwner : &sNdsNativeBossHighOwner;
    }
#endif
    return NULL;
}

static const NDSNativeFighterOwnerRuntime *
ndsRendererNativeFighterOwnerForProgramDetail(
    u32 slot, u32 use_low_detail, u32 program)
{
    if (program == NDS_NATIVE_SKELETON_PROGRAM)
    {
        return ndsNativeSkeletonOwner(slot, 1u);
    }
    if (program == 0u)
    {
        return ndsRendererNativeFighterCanonicalOwnerForDetail(
            slot, use_low_detail);
    }
#if defined(NDS_NATIVE_SAMUS_ROOT_PROGRAMS_PRESENT)
    if ((slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_SAMUS) && (program == 1u))
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeSamusCatchLowOwner : &sNdsNativeSamusCatchHighOwner;
    }
    if ((slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_SAMUS) && (program == 2u))
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeSamusMorphUnfoldLowOwner : &sNdsNativeSamusMorphUnfoldHighOwner;
    }
    if ((slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_SAMUS) && (program == 3u))
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeSamusMorphBallLowOwner : &sNdsNativeSamusMorphBallHighOwner;
    }
    if ((slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_SAMUS) && (program == 4u))
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeSamusFSmashLowOwner : &sNdsNativeSamusFSmashHighOwner;
    }
#endif
#if NDS_LINK && defined(NDS_NATIVE_LINK_ROOT_PROGRAMS_PRESENT)
    if (slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_LINK)
    {
        if (program == 1u)
        {
            return (use_low_detail != 0u) ?
                &sNdsNativeLinkEntryLowOwner : &sNdsNativeLinkEntryHighOwner;
        }
        if (program == 2u)
        {
            return (use_low_detail != 0u) ?
                &sNdsNativeLinkCatchLowOwner : &sNdsNativeLinkCatchHighOwner;
        }
        if (program == 3u)
        {
            return (use_low_detail != 0u) ?
                &sNdsNativeLinkSpecialNLowOwner :
                &sNdsNativeLinkSpecialNHighOwner;
        }
        if (program == 4u)
        {
            return (use_low_detail != 0u) ?
                &sNdsNativeLinkClapsLowOwner : &sNdsNativeLinkClapsHighOwner;
        }
        if (program == 5u)
        {
            return (use_low_detail != 0u) ?
                &sNdsNativeLinkSpecialNItemLowOwner :
                &sNdsNativeLinkSpecialNItemHighOwner;
        }
#if defined(NDS_NATIVE_LINK_INTRO_PROGRAM_PRESENT)
        if (program == 6u)
        {
            return (use_low_detail != 0u) ?
                &sNdsNativeLinkIntroLowOwner : &sNdsNativeLinkIntroHighOwner;
        }
#endif
    }
#endif
#if defined(NDS_NATIVE_KIRBY_ROOT_PROGRAMS_PRESENT)
    if (slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_KIRBY)
    {
        /* 1..NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT are the trio head contexts in
         * generated order, then Stone, CopyLink, and the seven-root copy
         * transition. Program numbers are this file's own; the generator names
         * its programs. */
        if ((program >= 1u) &&
            (program <= NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT))
        {
            return (use_low_detail != 0u) ?
                sNdsKirbyTrioLowOwners[program - 1u] :
                sNdsKirbyTrioHighOwners[program - 1u];
        }
        if (program == NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT + 1u)
        {
            return (use_low_detail != 0u) ?
                &sNdsNativeKirbyStoneLowOwner :
                &sNdsNativeKirbyStoneHighOwner;
        }
        if (program == NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT + 2u)
        {
            return (use_low_detail != 0u) ?
                &sNdsNativeKirbyCopyLinkLowOwner :
                &sNdsNativeKirbyCopyLinkHighOwner;
        }
        if (program == NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT + 3u)
        {
            return (use_low_detail != 0u) ?
                &sNdsNativeKirbyCopyTransitionLowOwner :
                &sNdsNativeKirbyCopyTransitionHighOwner;
        }
    }
#endif
#if defined(NDS_NATIVE_YOSHI_ROOT_PROGRAMS_PRESENT)
    if (slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_YOSHI)
    {
        if (program == 1u)
        {
            return (use_low_detail != 0u) ?
                &sNdsNativeYoshiCatchLowOwner : &sNdsNativeYoshiCatchHighOwner;
        }
        if (program == 2u)
        {
            return (use_low_detail != 0u) ?
                &sNdsNativeYoshiThrowLowOwner : &sNdsNativeYoshiThrowHighOwner;
        }
    }
#endif
#if defined(NDS_NATIVE_NESS_ROOT_PROGRAMS_PRESENT)
    if (slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_NESS)
    {
        if (program == 1u)
        {
            return (use_low_detail != 0u) ?
                &sNdsNativeNessWin3LowOwner : &sNdsNativeNessWin3HighOwner;
        }
        if (program == 2u)
        {
            return (use_low_detail != 0u) ?
                &sNdsNativeNessYoYoLowOwner : &sNdsNativeNessYoYoHighOwner;
        }
        if (program == 3u)
        {
            return (use_low_detail != 0u) ?
                &sNdsNativeNessFSmashLowOwner : &sNdsNativeNessFSmashHighOwner;
        }
    }
#endif
#if defined(NDS_NATIVE_PIKACHU_ROOT_PROGRAMS_PRESENT)
    if ((slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_PIKACHU) && (program == 1u))
    {
        return (use_low_detail != 0u) ?
            &sNdsNativePikachuAccessoryLowOwner :
            &sNdsNativePikachuAccessoryHighOwner;
    }
#endif
#if defined(NDS_NATIVE_PURIN_ROOT_PROGRAMS_PRESENT)
    if ((slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_PURIN) && (program == 1u))
    {
        return (use_low_detail != 0u) ?
            &sNdsNativePurinAccessoryLowOwner :
            &sNdsNativePurinAccessoryHighOwner;
    }
#endif
    {
        u32 count;
        const NDSP4RootProgram *programs = ndsP4RootPrograms(slot, &count);

        if ((programs != NULL) && (program >= 1u) && (program <= count))
        {
            return programs[program - 1u].owners[(use_low_detail != 0u) ? 1u : 0u];
        }
    }
#if (!defined(NDS_NATIVE_SAMUS_ROOT_PROGRAMS_PRESENT)) && (!(NDS_LINK && defined(NDS_NATIVE_LINK_ROOT_PROGRAMS_PRESENT))) && (!defined(NDS_NATIVE_KIRBY_ROOT_PROGRAMS_PRESENT)) && (!defined(NDS_NATIVE_YOSHI_ROOT_PROGRAMS_PRESENT)) && (!defined(NDS_NATIVE_NESS_ROOT_PROGRAMS_PRESENT)) && (!defined(NDS_NATIVE_PIKACHU_ROOT_PROGRAMS_PRESENT)) && (!defined(NDS_NATIVE_PURIN_ROOT_PROGRAMS_PRESENT))
    (void)slot;
    (void)use_low_detail;
#endif
    return NULL;
}

static const NDSNativeFighterOwnerRuntime *
ndsRendererNativeFighterOwnerForDetail(u32 slot, u32 use_low_detail)
{
    const NDSNativeFighterOwnerRuntime *owner;
    u32 program = (slot < NDS_NATIVE_FIGHTER_OWNER_COUNT) ?
        (u32)sNdsNativeFighterRootPrograms[slot] : 0u;

    owner = ndsRendererNativeFighterOwnerForProgramDetail(
        slot, use_low_detail, program);
    return (owner != NULL) ? owner :
        ndsRendererNativeFighterCanonicalOwnerForDetail(slot, use_low_detail);
}

#if NDS_LINK && defined(NDS_NATIVE_LINK_ROOT_PROGRAMS_PRESENT)
#if defined(NDS_NATIVE_LINK_INTRO_PROGRAM_PRESENT)
#define NDS_NATIVE_LINK_LAST_PROGRAM 6u
#else
#define NDS_NATIVE_LINK_LAST_PROGRAM 5u
#endif
#endif

u32 ndsRendererNativeFighterRootProgram(u32 slot)
{
    return (slot < NDS_NATIVE_FIGHTER_OWNER_COUNT) ?
        (u32)sNdsNativeFighterRootPrograms[slot] : 0u;
}

/* Source G_TEXTURE_GEN (Fast3D's LookAt texgen) consumes the finished current
 * modelview, which a GX-compose owner otherwise leaves only on the geometry
 * engine. Link's shield was the first such run; every polygon body and Metal
 * Mario are environment-mapped whole, and without the CPU mirror each of their
 * texgen runs rejected the owner -- the fighter drew nothing. Whether an owner
 * needs the mirror is a property of its generated state deltas, so it is
 * answered once per bound delta table, not per frame. */
u32 ndsRendererNativeFighterOwnerUsesTexgen(u32 slot, u32 use_low_detail)
{
    static const NDSNativeStateDelta *sDeltas[NDS_NATIVE_FIGHTER_OWNER_COUNT][2];
    static u8 sTexgen[NDS_NATIVE_FIGHTER_OWNER_COUNT][2];
    const NDSNativeFighterOwnerRuntime *owner;
    const NDSNativeFighterRuntimeTables *tables;
    u32 detail = (use_low_detail != 0u) ? 1u : 0u;
    u32 texgen = FALSE;
    u32 i;

    if (slot >= NDS_NATIVE_FIGHTER_OWNER_COUNT)
    {
        return FALSE;
    }
    owner = ndsRendererNativeFighterOwnerForDetail(slot, use_low_detail);
    tables = (owner != NULL) ? owner->tables : NULL;
    if ((tables == NULL) || (tables->state_deltas == NULL))
    {
        return FALSE;
    }
    if (sDeltas[slot][detail] == tables->state_deltas)
    {
        return sTexgen[slot][detail];
    }
    for (i = 0u; i < tables->state_delta_count; i++)
    {
        if ((tables->state_deltas[i].effect == NDS_NATIVE_STATE_GEOMETRY) &&
            ((tables->state_deltas[i].w1 & NDS_RENDERER_GEOM_TEXTURE_GEN) !=
             0u))
        {
            texgen = TRUE;
            break;
        }
    }
    sDeltas[slot][detail] = tables->state_deltas;
    sTexgen[slot][detail] = (u8)texgen;
    return texgen;
}

void ndsRendererNativeFighterSetRootProgram(u32 slot, u32 program)
{
    if (slot >= NDS_NATIVE_FIGHTER_OWNER_COUNT)
    {
        return;
    }
    if (program == NDS_NATIVE_SKELETON_PROGRAM && ndsNativeSkeletonOwner(slot, 1u) != NULL)
    {
        sNdsNativeFighterRootPrograms[slot] = (u8)program;
        return;
    }
#if defined(NDS_NATIVE_SAMUS_ROOT_PROGRAMS_PRESENT)
    /* Catch, MorphUnfold, MorphBall, FSmash. Every number this bound rejects
     * falls through to the reset below and silently becomes canonical, which is
     * how Kirby's Stone and CopyLink were lost for a month -- so this literal
     * moves whenever a Samus program is added. */
    if ((slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_SAMUS) && (program <= 4u))
    {
        sNdsNativeFighterRootPrograms[slot] = (u8)program;
        return;
    }
#endif
#if NDS_LINK && defined(NDS_NATIVE_LINK_ROOT_PROGRAMS_PRESENT)
    if ((slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_LINK) &&
        (program <= NDS_NATIVE_LINK_LAST_PROGRAM))
    {
        sNdsNativeFighterRootPrograms[slot] = (u8)program;
        return;
    }
#endif
#if defined(NDS_NATIVE_KIRBY_ROOT_PROGRAMS_PRESENT)
    /* Derived, not a literal. This was `program <= 4u` while Kirby had four
     * programs; admitting the ten copy hats made Stone program 13 and CopyLink
     * 14, and every number past the stale bound fell through to the reset
     * below and silently became program 0 -- canonical. Stone's one-root vector
     * then matched nothing and declined 151 times a match, and CopyLink, which
     * had always worked, would have gone with it. The selection loop, the
     * owner map and the head guard all key off the generated head list; this
     * bound is the fourth site and must too. */
    if ((slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_KIRBY) &&
        (program <= (NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT + 3u)))
    {
        sNdsNativeFighterRootPrograms[slot] = (u8)program;
        return;
    }
#endif
#if defined(NDS_NATIVE_YOSHI_ROOT_PROGRAMS_PRESENT)
    if ((slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_YOSHI) && (program <= 2u))
    {
        sNdsNativeFighterRootPrograms[slot] = (u8)program;
        return;
    }
#endif
#if defined(NDS_NATIVE_NESS_ROOT_PROGRAMS_PRESENT)
    /* canonical + Win3 + YoYo + FSmash. Every number this bound rejects falls
     * through to the reset below and silently becomes canonical, which is how
     * Kirby's Stone was lost for a month -- so this literal moves whenever a
     * Ness program is added. */
    if ((slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_NESS) && (program <= 3u))
    {
        sNdsNativeFighterRootPrograms[slot] = (u8)program;
        return;
    }
#endif
#if defined(NDS_NATIVE_PIKACHU_ROOT_PROGRAMS_PRESENT)
    /* canonical + Accessory. */
    if ((slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_PIKACHU) && (program <= 1u))
    {
        sNdsNativeFighterRootPrograms[slot] = (u8)program;
        return;
    }
#endif
#if defined(NDS_NATIVE_PURIN_ROOT_PROGRAMS_PRESENT)
    /* canonical + Accessory. */
    if ((slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_PURIN) && (program <= 1u))
    {
        sNdsNativeFighterRootPrograms[slot] = (u8)program;
        return;
    }
#endif
    {
        u32 count;

        if ((ndsP4RootPrograms(slot, &count) != NULL) && (program <= count))
        {
            sNdsNativeFighterRootPrograms[slot] = (u8)program;
            return;
        }
    }
#if (!defined(NDS_NATIVE_SAMUS_ROOT_PROGRAMS_PRESENT)) && (!(NDS_LINK && defined(NDS_NATIVE_LINK_ROOT_PROGRAMS_PRESENT))) && (!defined(NDS_NATIVE_KIRBY_ROOT_PROGRAMS_PRESENT)) && (!defined(NDS_NATIVE_YOSHI_ROOT_PROGRAMS_PRESENT)) && (!defined(NDS_NATIVE_NESS_ROOT_PROGRAMS_PRESENT)) && (!defined(NDS_NATIVE_PIKACHU_ROOT_PROGRAMS_PRESENT)) && (!defined(NDS_NATIVE_PURIN_ROOT_PROGRAMS_PRESENT))
    (void)program;
#endif
    sNdsNativeFighterRootPrograms[slot] = 0u;
}

/* A P4 owner's variant rows (donor_variants), keyed by canonical binding. */
static sb32 ndsP4RootVariantFits(u32 slot, u32 use_low_detail, u32 binding,
                                 u32 root_offset)
{
    const NDSNativeRootVariant *variants = NULL;
    u32 variant_count = 0u;
    u32 i;

#define NDS_P4_PROGRAM_VARIANTS(id_, T_, N_, n_, parent_, model_, main_)      \
    if (slot == NDS_P4_RENDERER_OWNER_SLOT(id_))                                \
    {                                                                           \
        variants = (use_low_detail != 0u) ?                                     \
            NDS_P4_NATIVE_##N_##_ROOT_VARIANTS_LOW :                            \
            NDS_P4_NATIVE_##N_##_ROOT_VARIANTS;                                 \
        variant_count = (use_low_detail != 0u) ?                                \
            NDS_P4_NATIVE_##N_##_ROOT_VARIANTS_LOW_COUNT :                      \
            NDS_P4_NATIVE_##N_##_ROOT_VARIANTS_COUNT;                           \
    }
    NDS_P4_CONTENT_ROWS(NDS_P4_PROGRAM_VARIANTS)
#undef NDS_P4_PROGRAM_VARIANTS
    for (i = 0u; i < variant_count; i++)
    {
        if ((variants[i].binding == binding) &&
            (variants[i].root.root_offset == root_offset))
        {
            return TRUE;
        }
    }
    return FALSE;
}

/* A P4 owner's live vector may carry model-part variants on any program's
 * roots, so a program matches when every root is its own list or one of the
 * variants its canonical binding owns -- the check ResolveRoot then makes. */
static u32 ndsP4SelectRootProgram(
    u32 slot, u32 use_low_detail, const u32 *root_offsets, u32 root_count,
    const NDSP4RootProgram *programs, u32 count, u32 *programs_tried)
{
    u32 detail = (use_low_detail != 0u) ? 1u : 0u;
    u32 program;

    for (program = 0u; program <= count; program++)
    {
        const NDSNativeFighterOwnerRuntime *owner = (program == 0u) ?
            ndsRendererNativeFighterCanonicalOwnerForDetail(slot, use_low_detail) :
            programs[program - 1u].owners[detail];
        const u8 *bindings = (program == 0u) ? NULL :
            programs[program - 1u].bindings[detail];
        u32 i;

        if (owner == NULL)
        {
            continue;
        }
        if (programs_tried != NULL)
        {
            (*programs_tried)++;
        }
        if (owner->root_count != root_count)
        {
            continue;
        }
        for (i = 0u; i < root_count; i++)
        {
            if ((owner->roots[i].root_offset != root_offsets[i]) &&
                (ndsP4RootVariantFits(slot, use_low_detail,
                                      (bindings != NULL) ? bindings[i] : i,
                                      root_offsets[i]) == FALSE))
            {
                break;
            }
        }
        if (i == root_count)
        {
            return program;
        }
    }
    return 0xffu;
}

u32 ndsRendererNativeFighterSelectRootProgram(
    u32 slot, u32 use_low_detail, const u32 *root_offsets, u32 root_count,
    u32 *programs_tried)
{
    u32 program_count = 1u;
    u32 program;

    if (programs_tried != NULL)
    {
        *programs_tried = 0u;
    }
    if ((slot >= NDS_NATIVE_FIGHTER_OWNER_COUNT) || (root_offsets == NULL))
    {
        return 0xffu;
    }
    {
        const NDSNativeFighterOwnerRuntime *skeleton = ndsNativeSkeletonOwner(slot, 1u);
        if (skeleton != NULL && root_count == skeleton->root_count)
        {
            u32 i;
            if (programs_tried != NULL) { (*programs_tried)++; }
            for (i = 0u; i < root_count; i++)
            {
                if (root_offsets[i] != skeleton->roots[i].root_offset) { break; }
            }
            if (i == root_count) { return NDS_NATIVE_SKELETON_PROGRAM; }
        }
    }
    {
        u32 count;
        const NDSP4RootProgram *programs = ndsP4RootPrograms(slot, &count);

        if (programs != NULL)
        {
            return ndsP4SelectRootProgram(slot, use_low_detail, root_offsets,
                                          root_count, programs, count,
                                          programs_tried);
        }
    }
#if defined(NDS_NATIVE_SAMUS_ROOT_PROGRAMS_PRESENT)
    if (slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_SAMUS)
    {
        /* canonical + Catch + MorphUnfold + MorphBall + FSmash. */
        program_count = 5u;
    }
#endif
#if NDS_LINK && defined(NDS_NATIVE_LINK_ROOT_PROGRAMS_PRESENT)
    if (slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_LINK)
    {
        /* canonical + Entry + Catch + SpecialN + Claps + SpecialNItem
         * (+ Intro in the intro-still bake). */
        program_count = NDS_NATIVE_LINK_LAST_PROGRAM + 1u;
    }
#endif
#if defined(NDS_NATIVE_KIRBY_ROOT_PROGRAMS_PRESENT)
    if (slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_KIRBY)
    {
        /* canonical + every trio head + Stone + CopyLink + CopyTransition. */
        program_count = NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT + 4u;
    }
#endif
#if defined(NDS_NATIVE_YOSHI_ROOT_PROGRAMS_PRESENT)
    if (slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_YOSHI)
    {
        /* canonical + Catch + Throw. */
        program_count = 3u;
    }
#endif
#if defined(NDS_NATIVE_NESS_ROOT_PROGRAMS_PRESENT)
    if (slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_NESS)
    {
        /* canonical + Win3 + YoYo + FSmash. */
        program_count = 4u;
    }
#endif
#if defined(NDS_NATIVE_PIKACHU_ROOT_PROGRAMS_PRESENT)
    if (slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_PIKACHU)
    {
        /* canonical + Accessory. */
        program_count = 2u;
    }
#endif
#if defined(NDS_NATIVE_PURIN_ROOT_PROGRAMS_PRESENT)
    if (slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_PURIN)
    {
        /* canonical + Accessory. */
        program_count = 2u;
    }
#endif
    for (program = 0u; program < program_count; program++)
    {
        const NDSNativeFighterOwnerRuntime *owner =
            ndsRendererNativeFighterOwnerForProgramDetail(
                slot, use_low_detail, program);
        u32 root_index;

        if (owner == NULL)
        {
            continue;
        }
#if defined(NDS_NATIVE_KIRBY_ROOT_PROGRAMS_PRESENT)
        if (slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_KIRBY)
        {
            /* Programs 1..N are the trio head contexts, then Stone, CopyLink,
             * and the head-10 copy transition. The trio body bake
             * inherits the preceding head's vertex cache, so an
             * identical-looking vector under the wrong live head must never
             * select its sibling bake -- and with twelve heads sharing one
             * root cardinality that is no longer a theoretical collision. */
#if defined(NDS_NATIVE_KIRBY_TRIO_BODY_PRESENT)
            if ((program >= 1u) &&
                (program <= NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT) &&
                (sNdsKirbyTrioHeadMp !=
                 (u32)sNdsKirbyTrioProgramHead[program - 1u]))
            {
                continue;
            }
#endif
            if (((program == NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT + 2u) ||
                 (program == NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT + 3u)) &&
                (sNdsKirbyTrioHeadMp != 10u))
            {
                continue;
            }
        }
#endif
        if (programs_tried != NULL)
        {
            (*programs_tried)++;
        }
        if (owner->root_count != root_count)
        {
            continue;
        }
        for (root_index = 0u; root_index < root_count; root_index++)
        {
            if (owner->roots[root_index].root_offset != root_offsets[root_index])
            {
                break;
            }
        }
        if (root_index == root_count)
        {
            return program;
        }
    }
    return 0xffu;
}

/* Resolve the exact executable native root for one logical JointTree binding.
 * Canonical roots are the common case.  BattleShip passive model parts keep
 * the same DObj/joint matrix but replace its DL, so only an explicitly
 * generated `(binding, root_offset)` pair may select a variant.  Unknown
 * offsets remain a hard decline to the caller; never reinterpret arbitrary
 * fighter DLs as one of these source-qualified programs. */
static const NDSNativeRoot *ndsRendererNativeFighterResolveRoot(
    const NDSNativeFighterOwnerRuntime *owner,
    u32 slot,
    u32 battle_slot,
    u32 use_low_detail,
    u32 binding,
    u32 root_offset)
{
    const NDSNativeRootVariant *variants = NULL;
    u32 variant_count = 0u;
    NDSNativeKirbyHatImageSlot *kirby_hat_slot = NULL;
    NDSNativeRoot *kirby_hat_root = NULL;
    u32 i;

    if ((owner == NULL) || (binding >= owner->root_count))
    {
        return NULL;
    }
    if ((use_low_detail < NDS_NATIVE_IMAGE_DETAILS) &&
        (battle_slot < NDS_NATIVE_KIRBY_HAT_BATTLE_SLOTS))
    {
        kirby_hat_slot = ndsKirbyHatSlot(battle_slot, use_low_detail);
        kirby_hat_root = &kirby_hat_slot->root;
    }
    /* CopyLink and the copy-transition program both take binding 0 from the
     * deferred copy-hat mini image. Prefer the scene-resident modelpart-10 root
     * before the program owner's same-offset certificate so its local table
     * indices and light-preamble indices stay paired with that image. */
#if defined(NDS_NATIVE_KIRBY_ROOT_PROGRAMS_PRESENT)
    if ((slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_KIRBY) &&
        (((ndsRendererNativeFighterRootProgram(slot) ==
           NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT + 2u)) ||
         ((ndsRendererNativeFighterRootProgram(slot) ==
           NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT + 3u))) &&
        (binding == 0u) &&
        (kirby_hat_slot != NULL) && (kirby_hat_root != NULL) &&
        (kirby_hat_slot->valid != 0u) &&
        (kirby_hat_slot->heap_generation == gNdsTaskmanHeapGeneration) &&
        ((u32)kirby_hat_slot->use_low_detail == use_low_detail) &&
        (kirby_hat_slot->copy_modelpart_id == 10u) &&
        (kirby_hat_root->root_offset == root_offset))
    {
        return kirby_hat_root;
    }
#endif
    if (owner->roots[binding].root_offset == root_offset)
    {
        return &owner->roots[binding];
    }
    /* Copy hats are the only Kirby joint-6 variants excluded from the resident
     * owner table.  The generated binding is source-derived from the JointTree
     * + setup_parts walk (joint 6 is the first selected drawable root, binding
     * 0); keep the runtime on that generated answer rather than duplicating a
     * joint-number/binding assumption here. An absent/wrong-detail/wrong-hat
     * image is a hard native decline. */
    if ((slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_KIRBY) &&
        (binding == NDS_NATIVE_KIRBY_HAT_BINDING) &&
        (kirby_hat_slot != NULL) && (kirby_hat_root != NULL) &&
        (kirby_hat_slot->valid != 0u) &&
        (kirby_hat_slot->heap_generation == gNdsTaskmanHeapGeneration) &&
        ((u32)kirby_hat_slot->use_low_detail == use_low_detail) &&
        (kirby_hat_root->root_offset == root_offset))
    {
        return kirby_hat_root;
    }
    /* Mario's two hand joints (10 and 16) each carry a second model part in
     * 203_MarioMain.c, and the VS Results pose selects it: without these rows
     * the owner declined at validate code 4 and every root of the Lose pose
     * was rejected (2026-09-08). Same contract as Fox below; unknown Mario
     * offsets still fail closed. */
    if (slot == 0u)
    {
        variants = (use_low_detail != 0u) ?
            sNdsNativeMarioRootVariantsLow : sNdsNativeMarioRootVariants;
        variant_count = (use_low_detail != 0u) ?
            NDS_FTR_COUNT(sNdsNativeMarioRootVariantsLow) :
            NDS_FTR_COUNT(sNdsNativeMarioRootVariants);
    }
    /* BattleShip's Fox Results Lose motion (scsubsysdatafox.c) switches model
     * part 1 onto joints 10 and 16.  The generated rows below are the complete
     * source DL programs for those replacements at their original logical
     * bindings; unknown Fox offsets still fail closed. */
    if (slot == 1u)
    {
        variants = (use_low_detail != 0u) ?
            sNdsNativeFoxRootVariantsLow : sNdsNativeFoxRootVariants;
        variant_count = (use_low_detail != 0u) ?
            NDS_FTR_COUNT(sNdsNativeFoxRootVariantsLow) :
            NDS_FTR_COUNT(sNdsNativeFoxRootVariants);
    }
#if NDS_LUIGI
    /* Luigi's Results Win2 pose sets model part 1 on both hand joints, exactly
     * as Mario's Lose pose does above (scsubsysdataluigi.c D_ovl1_803917A0). */
    if (slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_LUIGI)
    {
        variants = (use_low_detail != 0u) ?
            sNdsNativeLuigiRootVariantsLow : sNdsNativeLuigiRootVariants;
        variant_count = (use_low_detail != 0u) ?
            NDS_FTR_COUNT(sNdsNativeLuigiRootVariantsLow) :
            NDS_FTR_COUNT(sNdsNativeLuigiRootVariants);
    }
#endif
#if NDS_DONKEY
    if (slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_DONKEY)
    {
        variants = (use_low_detail != 0u) ?
            sNdsNativeDonkeyRootVariantsLow : sNdsNativeDonkeyRootVariants;
        variant_count = (use_low_detail != 0u) ?
            NDS_FTR_COUNT(sNdsNativeDonkeyRootVariantsLow) :
            NDS_FTR_COUNT(sNdsNativeDonkeyRootVariants);
    }
#endif
#if NDS_CAPTAIN
    if (slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_CAPTAIN)
    {
        variants = (use_low_detail != 0u) ?
            sNdsNativeCaptainRootVariantsLow : sNdsNativeCaptainRootVariants;
        variant_count = (use_low_detail != 0u) ?
            NDS_FTR_COUNT(sNdsNativeCaptainRootVariantsLow) :
            NDS_FTR_COUNT(sNdsNativeCaptainRootVariants);
    }
#endif
    if (slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_SAMUS)
    {
        variants = (use_low_detail != 0u) ?
            sNdsNativeSamusRootVariantsLow : sNdsNativeSamusRootVariants;
        variant_count = (use_low_detail != 0u) ?
            NDS_FTR_COUNT(sNdsNativeSamusRootVariantsLow) :
            NDS_FTR_COUNT(sNdsNativeSamusRootVariants);
    }
    if (slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_NESS)
    {
        variants = (use_low_detail != 0u) ?
            sNdsNativeNessRootVariantsLow : sNdsNativeNessRootVariants;
        variant_count = (use_low_detail != 0u) ?
            NDS_FTR_COUNT(sNdsNativeNessRootVariantsLow) :
            NDS_FTR_COUNT(sNdsNativeNessRootVariants);
    }
    /* Yoshi's grab throws. dYoshiMainMotion_ThrowF/_ThrowB set joint 7 to
     * model part 1 and restore 0 on release, and they are Yoshi's only
     * model-part mutations. Without the row this resolves to nothing, the
     * owner declines, and at NDS_RENDERER_PROFILE_LEVEL 0 the whole fighter
     * stops drawing for the length of the throw -- the owner's "Grab attacks
     * turn yoshi invisible". Unknown Yoshi offsets still fail closed. */
    if (slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_YOSHI)
    {
        variants = (use_low_detail != 0u) ?
            sNdsNativeYoshiRootVariantsLow : sNdsNativeYoshiRootVariants;
        variant_count = (use_low_detail != 0u) ?
            NDS_FTR_COUNT(sNdsNativeYoshiRootVariantsLow) :
            NDS_FTR_COUNT(sNdsNativeYoshiRootVariants);
    }
#if defined(NDS_NATIVE_KIRBY_ROOT_VARIANTS_PRESENT)
    if (slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_KIRBY)
    {
        variants = (use_low_detail != 0u) ?
            sNdsNativeKirbyRootVariantsLow : sNdsNativeKirbyRootVariants;
        variant_count = (use_low_detail != 0u) ?
            NDS_FTR_COUNT(sNdsNativeKirbyRootVariantsLow) :
            NDS_FTR_COUNT(sNdsNativeKirbyRootVariants);
    }
#endif
    /* A P4 owner carries every alternate model part its donor's joints have
     * (scripts/p4/p4_native_owner.py donor_variants): its own scripts and
     * its parent's code both set them. The rows name canonical bindings, so a
     * program's root is looked up by the canonical binding it draws. */
    {
        const NDSP4RootProgram *program = ndsP4CurrentRootProgram(slot);

        if (program != NULL)
        {
            binding = program->bindings[(use_low_detail != 0u) ? 1u : 0u][binding];
        }
    }
#define NDS_P4_OWNER_VARIANTS(id_, T_, N_, n_, parent_, model_, main_)         \
    if (slot == NDS_P4_RENDERER_OWNER_SLOT(id_))                                \
    {                                                                           \
        variants = (use_low_detail != 0u) ?                                     \
            NDS_P4_NATIVE_##N_##_ROOT_VARIANTS_LOW :                            \
            NDS_P4_NATIVE_##N_##_ROOT_VARIANTS;                                 \
        variant_count = (use_low_detail != 0u) ?                                \
            NDS_P4_NATIVE_##N_##_ROOT_VARIANTS_LOW_COUNT :                      \
            NDS_P4_NATIVE_##N_##_ROOT_VARIANTS_COUNT;                           \
    }
    NDS_P4_CONTENT_ROWS(NDS_P4_OWNER_VARIANTS)
#undef NDS_P4_OWNER_VARIANTS
    for (i = 0u; i < variant_count; i++)
    {
        if ((variants[i].binding == binding) &&
            (variants[i].root.root_offset == root_offset))
        {
            return &variants[i].root;
        }
    }
    return NULL;
}

#undef NDS_FTR_COUNT




#define NDS_NATIVE_FIGHTER_HIERARCHY_JOINT_MAX 27u
#define NDS_NATIVE_FIGHTER_HIERARCHY_BINDING_MAX 18u
#define NDS_NATIVE_FIGHTER_HIERARCHY_EPOCH_COUNT 49u

typedef struct NDSNativeMatrix3x3
{
    s32 m[3][3];
} NDSNativeMatrix3x3;

typedef struct NDSNativeHierarchyPreparedEpoch
{
    NDSRendererHardwareLightDirection light_direction;
    u32 light_direction_valid;
} NDSNativeHierarchyPreparedEpoch;

typedef struct NDSNativeHierarchyPreparedRun
{
    NDSRendererHardwareTextureCacheEntry *texture_entry;
    u32 texture_name;
    u32 texture_params;
    u32 texture_format;
    u32 texture_width;
    u32 texture_height;
    u32 poly_fmt;
    u32 scale_s;
    u32 scale_t;
    u32 origin_s;
    u32 origin_t;
    s32 texture_offset;
    u32 vertex_flags;
    u32 textured;
} NDSNativeHierarchyPreparedRun;

typedef struct NDSNativeFighterOwnerExecution
{
    NDSRendererTraversalState traversal;
    NDSRendererStats preflight_stats;
    NDSNativeMatrix3x3 hierarchy_world[
        NDS_NATIVE_FIGHTER_HIERARCHY_JOINT_MAX];
    NDSNativeHierarchyPreparedEpoch hierarchy_epochs[
        NDS_NATIVE_FIGHTER_HIERARCHY_EPOCH_COUNT];
    NDSNativeHierarchyPreparedRun hierarchy_runs[
        NDS_NATIVE_FIGHTER_HIERARCHY_EPOCH_COUNT];
    NDSRendererStats *stats;
    NDSRendererVertexCache *vertex_cache;
    u32 slot;
    /* Battle player slot (0..3), distinct from the generated owner slot above.
     * The production run texture memo needs this identity because two live
     * instances of the same fighter execute the same generated run indices but
     * can carry different source materials/costumes.  `appearance_id` is the
     * source costume/shade pair for the same reason: CSS reuses one live player
     * slot while changing costumes, so instance identity alone is not enough.
     * Owner execution is serialized, just like sNdsNativeFighterActiveTables,
     * so current values keep the hot run helper ABI unchanged. */
    /* One hot-path identity word for the run-texture memo.  The resolver's
     * result can vary by generated owner, high/low detail program, live fighter
     * instance, costume and shade.  Packing those facts once at owner entry is
     * materially cheaper than re-reading four independent fences for every run
     * (four fighters execute this path thousands of times per gate match). */
    u32 texture_memo_owner_key;
    u32 active;
} NDSNativeFighterOwnerExecution;


typedef struct NDSNativeStagePreparedDense
{
    u16 packed_color;
    s16 s;
    s16 t;
    s16 near_inside;
} NDSNativeStagePreparedDense;

typedef struct NDSRendererProjectedClipVertex
{
    NDSRendererClipVertex20p12 clip;
    s32 s;
    s32 t;
    u16 packed_color;
} NDSRendererProjectedClipVertex;

typedef struct NDSNativeStagePreparedRun
{
    NDSRendererHardwareTextureCacheEntry *texture_entry;
    u32 texture_name;
    u32 texture_params;
    u32 texture_generation;
    u32 poly_fmt;
    u16 texture_width;
    u16 texture_height;
    u8 texture_format;
    u8 textured;
    u8 alpha_test;
    u8 alpha_ref;
    /* PROJECTED_RANGE_OR_MATRIX runs: the coordinate shift the run's vertices
     * and world matrix share, max(1, the largest per-vertex shift the
     * generator packed). Dream Land fits shift 1 everywhere; Sector Z's Great
     * Fox needs more (2026-09-07). 0 for the other classes. */
    u8 coordinate_shift;
} NDSNativeStagePreparedRun;

static s32 ndsRendererNativeStagePreparedTextureValid(
    const NDSNativeStagePreparedRun *prepared)
{
    const NDSRendererHardwareTextureCacheEntry *entry;

    if (prepared == NULL)
    {
        return FALSE;
    }
    entry = prepared->texture_entry;
    if (prepared->textured == FALSE)
    {
        return ((entry == NULL) && (prepared->texture_name == 0u) &&
                (prepared->texture_generation == 0u)) ? TRUE : FALSE;
    }
    return ((entry != NULL) && (entry->ready != FALSE) &&
            ((u32)entry->name == prepared->texture_name) &&
            (entry->key_generation == prepared->texture_generation)) ?
        TRUE : FALSE;
}

/* The stage owner's per-run and per-vertex workspaces (2026-10-13, P5 T5).
 * They were static arrays at the largest packet any stage has (92 runs and
 * 863 dense vertices, 15 KB of .bss that every scene paid), and Hyrule
 * Temple's 177 runs and 1,258 vertices would have grown them for every scene
 * again. The linked packet's (Dream Land's) stay static at its size; a blob
 * packet brings its own beside its slab (nds_native_stage_blob.c), sized by
 * its counts, and ndsRendererNativeStageSelectPacket binds the active one. */
typedef struct NDSNativeStageWorkspace
{
    NDSNativeStagePreparedRun *runs;
    NDSNativeStagePreparedDense *dense;
    /* The validation cache's prepared dense lists: run_capacity + 1 offsets
     * into dense_capacity indices. */
    u16 *dense_offsets;
    u16 *dense_indices;
    /* The per-run emission witness (the scene probe's). */
    volatile u32 *roof_valid;
    volatile u32 *roof_given;
    volatile u32 *roof_emitted;
    u32 run_capacity;
    u32 dense_capacity;
} NDSNativeStageWorkspace;

static NDSNativeStagePreparedRun sNdsNativeStageLinkedRuns[
    NDS_NATIVE_STAGE_LINKED_MAX_RUN_COUNT];
static NDSNativeStagePreparedDense sNdsNativeStageLinkedDense[
    NDS_NATIVE_STAGE_LINKED_MAX_DENSE_VERTEX_COUNT];
static u16 sNdsNativeStageLinkedDenseOffsets[
    NDS_NATIVE_STAGE_LINKED_MAX_RUN_COUNT + 1u];
static u16 sNdsNativeStageLinkedDenseIndices[
    NDS_NATIVE_STAGE_LINKED_MAX_DENSE_VERTEX_COUNT];
static volatile u32 sNdsNativeStageLinkedRoof[3][
    NDS_NATIVE_STAGE_LINKED_MAX_RUN_COUNT];
static const NDSNativeStageWorkspace sNdsNativeStageLinkedWorkspace =
{
    sNdsNativeStageLinkedRuns, sNdsNativeStageLinkedDense,
    sNdsNativeStageLinkedDenseOffsets, sNdsNativeStageLinkedDenseIndices,
    sNdsNativeStageLinkedRoof[0], sNdsNativeStageLinkedRoof[1],
    sNdsNativeStageLinkedRoof[2],
    NDS_NATIVE_STAGE_LINKED_MAX_RUN_COUNT,
    NDS_NATIVE_STAGE_LINKED_MAX_DENSE_VERTEX_COUNT
};
static NDSNativeStageWorkspace sNdsNativeStageWs =
{
    sNdsNativeStageLinkedRuns, sNdsNativeStageLinkedDense,
    sNdsNativeStageLinkedDenseOffsets, sNdsNativeStageLinkedDenseIndices,
    sNdsNativeStageLinkedRoof[0], sNdsNativeStageLinkedRoof[1],
    sNdsNativeStageLinkedRoof[2],
    NDS_NATIVE_STAGE_LINKED_MAX_RUN_COUNT,
    NDS_NATIVE_STAGE_LINKED_MAX_DENSE_VERTEX_COUNT
};
#define sNdsNativeStagePreparedDense (sNdsNativeStageWs.dense)
#define gNdsNativeStageRoofSnapValid (sNdsNativeStageWs.roof_valid)
#define gNdsNativeStageRoofSnapGiven (sNdsNativeStageWs.roof_given)
#define gNdsNativeStageRoofSnapEmitted (sNdsNativeStageWs.roof_emitted)

/* A blob packet's workspace bytes for its counts, in the order
 * ndsRendererNativeStageWorkspaceCarve lays them out (the blob loader
 * allocates them beside the slab). */
u32 ndsRendererNativeStageWorkspaceBytes(u32 runs, u32 dense)
{
    return (runs * (u32)sizeof(NDSNativeStagePreparedRun)) +
           (dense * (u32)sizeof(NDSNativeStagePreparedDense)) +
           (runs * 3u * (u32)sizeof(u32)) +
           ((runs + 1u) * (u32)sizeof(u16)) +
           (dense * (u32)sizeof(u16));
}

static void ndsRendererNativeStageWorkspaceCarve(u8 *base, u32 runs, u32 dense,
                                                NDSNativeStageWorkspace *ws)
{
    ws->runs = (NDSNativeStagePreparedRun *)(void *)base;
    base += runs * sizeof(NDSNativeStagePreparedRun);
    ws->dense = (NDSNativeStagePreparedDense *)(void *)base;
    base += dense * sizeof(NDSNativeStagePreparedDense);
    ws->roof_valid = (volatile u32 *)(void *)base;
    base += runs * sizeof(u32);
    ws->roof_given = (volatile u32 *)(void *)base;
    base += runs * sizeof(u32);
    ws->roof_emitted = (volatile u32 *)(void *)base;
    base += runs * sizeof(u32);
    ws->dense_offsets = (u16 *)(void *)base;
    base += (runs + 1u) * sizeof(u16);
    ws->dense_indices = (u16 *)(void *)base;
    ws->run_capacity = runs;
    ws->dense_capacity = dense;
}
/* ndsRendererNativeStageSelectPacket: the selected packet's workspace. A
 * packet whose counts outrun it declines (none does: the linked one is sized
 * by the linked packets, a blob's by its own counts). */
static s32 ndsRendererNativeStageBindWorkspace(const NDSNativeStagePacket *packet)
{
    sNdsNativeStageWs = sNdsNativeStageLinkedWorkspace;
#if NDS_NATIVE_STAGE_MULTI
    if ((const void *)packet == (const void *)ndsNativeStageBlobPacket())
    {
        u8 *base = (u8 *)ndsNativeStageBlobWorkspace();

        if (base == NULL)
        {
            return FALSE;
        }
        ndsRendererNativeStageWorkspaceCarve(base, packet->run_count,
                                             packet->dense_vertex_count,
                                             &sNdsNativeStageWs);
    }
#endif
    return ((packet->run_count <= sNdsNativeStageWs.run_capacity) &&
            (packet->dense_vertex_count <= sNdsNativeStageWs.dense_capacity)) ?
        TRUE : FALSE;
}
_Static_assert((sizeof(NDSNativeStagePreparedRun) % 4u) == 0u,
               "stage workspace run rows must keep word alignment");
_Static_assert((sizeof(NDSNativeStagePreparedDense) % 4u) == 0u,
               "stage workspace dense rows must keep word alignment");

typedef struct NDSNativeStageOwnerExecution
{
    NDSRendererTraversalState traversal;
    NDSRendererStats preflight_stats;
    const NDSRendererMatrix20p12 *binding_composed;
    u64 hidden_binding_mask;
    const NDSRendererMatrix20p12 *projection;
    const NDSRendererMatrix20p12 *camera_modelview;
    const NDSRendererMatrix20p12 *binding_world;
    u64 rigid_binding_mask;
    u64 hw_seen_binding_mask;
    u32 hw_binding;
    u32 hw_coordinate_shift;
    u32 hw_local_pushed;
    u32 hw_segment_active;
    NDSRendererStats *stats;
    /* Bit i: segment i committed this frame. Segments commit in the order the
     * runtime presents their display GObjs, which is the source's draw order;
     * a segment presented twice is refused, never drawn twice. */
    u32 committed_segments;
    u32 active;
    /* R2-02 E1a. `runs[]` is a pure function of the generated run/epoch/policy
     * tables and the traversal state, and Task 44 already proves the traversal
     * state unchanged every frame. Once built it is reusable, so the whole
     * PrepareRun phase can leave the frame.
     *
     * Safe with respect to texture binding: ndsRendererNativeStageBeginRun
     * binds from this record (`texture_name`, `texture_params`,
     * `texture_entry`) and sets `last_used_frame` itself, so the resolve inside
     * PrepareRun is a producer of the record rather than a side effect the
     * commit path depends on.
     *
     * Keyed on the frame config and asset bases so a scene reload cannot reuse
     * a table built against different assets -- and, since R2-07 E2, on the
     * topology generation and stamp as well.
     *
     * The original key assumed "the topology validation and Task 44's
     * generation compare should both reject first". Neither did. Re-entering
     * the SAME stage keeps the config POINTER and every asset base identical
     * (the bump allocator hands back the same addresses in the same order), so
     * the key could not see a scene boundary at all: E2 measured
     * PrepareBuildCount frozen at 2 across the Sudden Death entry while
     * PrepareReuseCount ran 195 -> 303, i.e. the second scene drew its stage
     * from run data prepared for the first. That is the whole second-entry
     * corruption -- the routing arms agree, replay correct / reuse corrupt /
     * rebuild-from-source correct.
     *
     * topology_generation + topology_stamp are the pair the Task 36 replay
     * owner already keys on, and that owner resets correctly across the entry;
     * reusing them here makes the two caches invalidate together rather than
     * leaving this one keyed on quantities a re-entry cannot move. */
    const NDSRendererConfig *r2_prepared_config;
    const void *r2_prepared_asset_bases[NDS_RENDERER_NATIVE_STAGE_ASSET_COUNT];
    u32 r2_prepared_topology_generation;
    u32 r2_prepared_topology_stamp;
    u64 r2_prepared_epoch_mask;
    u32 r2_prepared_valid;
    /* R2-02 E8. The one member of `preflight_stats` that outlives the segment
     * loop. Memoised with `epoch_mask` so eliding the loop body cannot depend
     * on which segment happens to run last. */
    u32 r2_prepared_sync_count;
} NDSNativeStageOwnerExecution;

typedef struct NDSNativeStageTopologySummary
{
    u32 raw_triangles;
    u32 projected_no_z_triangles;
    u32 projected_range_triangles;
    u32 cross_runs;
    u32 cross_triangles;
    u32 cross_foreign_corners;
} NDSNativeStageTopologySummary;

typedef struct NDSNativeStageValidationCache
{
    NDSNativeStageTopologySummary summary;
    u32 generation;
    u32 stamp;
    u32 valid;
} NDSNativeStageValidationCache;

/* Stage segments straddle the fighter display links, so their accepted
 * preflight must survive the two complete fighter-owner submissions. */
static NDSNativeFighterOwnerExecution sNdsNativeFighterOwnerExecution;
static NDSNativeStageOwnerExecution sNdsNativeStageOwnerExecution;
/* R2-07 leg A. The prepared run table was re-proved against the texture cache
 * ~195 times a frame (54 runs x the reuse key, the commit gate and the replay
 * gate, plus BeginRun's per-run last gate), and the c123 per-line attribution
 * says that cost is not the compares: 5.34 cycles per instruction, i.e. two
 * cold arrays -- runs[] and then the cache entries it names -- dragged through
 * a 4 KB D-cache every frame. Slice 44's shape exactly, whose lever was "stop
 * touching the objects".
 *
 * So prove it where the invariant breaks (slice 30) rather than where it is
 * read. sNdsRendererHardwareTextureKeyGeneration is already stamped by every
 * (re-)key, and ndsRendererHardwareReleaseTexture now stamps it as well -- that
 * function is the ONE seam that destroys an entry's identity (Evict, Alloc's
 * recycle, Discard and the static teardown all route through it, and its memset
 * is what clears `ready` and `name`). Nothing else writes entry->ready,
 * entry->name or entry->key_generation: names are only ever produced by
 * glGenTextures into a slot whose name is already 0, and ready is only ever set
 * TRUE beside a key stamp. The epoch is therefore a COMPLETE invalidation event
 * for the (entry, ready, name, key_generation) quadruple the proof reads, and a
 * table proved at epoch E is still proved for as long as the epoch reads E.
 *
 * Only the POSITIVE answer is cached. A failing table becomes valid again by
 * being rebuilt, which is not an epoch event, so a miss re-sweeps every frame
 * exactly as before; and the rebuild path clears the stamp so a fresh table is
 * never covered by its predecessor's proof.
 *
 * Measured basis for the design, from the same profile: the cache is static
 * across a match -- ndsRendererHardwareAllocTexture executes 3 times in the
 * whole 1,600-frame run and Release/Evict/Discard do not appear at all -- so
 * the epoch is expected to be motionless mid-match and the fast path is the
 * path. A scene boundary is the opposite case and is covered: the battle
 * prepare discards the whole cache, which releases every entry and therefore
 * moves the epoch, and the topology generation/stamp key rebuilds the table
 * independently (R2-07 E2). */
static u32 sNdsNativeStagePreparedTextureProofEpoch;
static u32 sNdsNativeStagePreparedTextureProofValid;
/* Engagement, both sides, read with -ExtraGlobals from the same run that
 * produces the buckets. Predicted on the gate arm: Fast ~= one per consult per
 * frame, Sweep ~= 0 after the first, EpochBump ~= 0 mid-match. A per-frame
 * Sweep count means the epoch is moving and this lever did not engage. */
__attribute__((used)) volatile u32 gNdsR2TexProofFastCount;
__attribute__((used)) volatile u32 gNdsR2TexProofSweepCount;
__attribute__((used)) volatile u32 gNdsR2TexProofSweepFailCount;
__attribute__((used)) volatile u32 gNdsR2TextureEpochBumpCount;

/* Bumped wherever runs[] is (re)written -- this drop is the write seam (see
 * ndsRendererNativeStagePrepareRun) -- so a reader that verified the prepared
 * table once can prove it unchanged by one compare (the stage GX segment fast
 * path, nds_stage_gx.exec.inc). */
static u32 sNdsNativeStagePreparedSerial;

static void ndsRendererNativeStagePreparedTextureProofDrop(void)
{
    sNdsNativeStagePreparedTextureProofValid = FALSE;
    sNdsNativeStagePreparedSerial++;
}

/* Pure reads of the two certificates -- "is the standing proof still current",
 * with no sweep behind them. These are what let the point-of-use gate stop
 * touching the objects: its caller has already proved the table this run
 * belongs to, so re-reading prepared[] and the cache entry can only reach the
 * same answer. When no certificate is current the original per-run proof runs
 * unchanged, so the gate never weakens -- it is skipped only while something
 * else has proved the whole table at the current epoch. */
static s32 ndsRendererNativeStagePreparedTextureProofCurrent(void)
{
    return ((sNdsNativeStagePreparedTextureProofValid != FALSE) &&
            (sNdsNativeStagePreparedTextureProofEpoch ==
             sNdsRendererHardwareTextureKeyGeneration)) ? TRUE : FALSE;
}


static s32 ndsRendererNativeStagePreparedTexturesProven(void)
{
    u32 run_index;

    if ((sNdsNativeStagePreparedTextureProofValid != FALSE) &&
        (sNdsNativeStagePreparedTextureProofEpoch ==
         sNdsRendererHardwareTextureKeyGeneration))
    {
        NDS_DIAG(gNdsR2TexProofFastCount++);
        return TRUE;
    }
    gNdsR2TexProofSweepCount++;
    for (run_index = 0u; run_index < NDS_NATIVE_STAGE_RUN_COUNT; run_index++)
    {
        if (ndsRendererNativeStagePreparedTextureValid(
                &sNdsNativeStageWs.runs[run_index]) == FALSE)
        {
            sNdsNativeStagePreparedTextureProofValid = FALSE;
            gNdsR2TexProofSweepFailCount++;
            return FALSE;
        }
    }
    sNdsNativeStagePreparedTextureProofEpoch =
        sNdsRendererHardwareTextureKeyGeneration;
    sNdsNativeStagePreparedTextureProofValid = TRUE;
    return TRUE;
}
/* R2-02 E1a engagement counters. Non-static so a GDB stop can read them and
 * prove the elision actually engaged -- a flag that silently never fires is
 * indistinguishable from a null result, and this campaign has shipped that
 * mistake (Task 52 found the Task 36 replay structurally disabled). */
volatile u32 gNdsR2StagePrepareReuseCount;
volatile u32 gNdsR2StagePrepareBuildCount;
/* R2-02 E8 engagement, for the same reason: five per frame is the elision
 * working, zero is a flag that compiled but never fired. */
volatile u32 gNdsR2StagePreflightElideCount;
static NDSNativeStageValidationCache sNdsNativeStageValidationCache;
