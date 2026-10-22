/* Relocation asset loading, the force loads: fighter AObj16 force files and their
 * authoritative records, the status buffer files, the compact IFCommonGameStatus
 * load, lbRelocLoadFilesExtern and file data. A slice of reloc_backend.c. */

void ndsRelocFinishSceneSetup(void)
{
    if ((gNdsSceneManagerCurrIsBattle != 0u) &&
        (sNdsR2AnimCacheSetupGeneration != gNdsTaskmanHeapGeneration))
    {
        /* VS already warms at its source BGM seam. Other battle constructors
         * reach the same cache preparation before their first update/draw. */
        ndsR2AnimCachePreloadMatch();
        (void)ndsR2AnimCachePreloadFinish();
    }
}

static void *ndsRelocForceLoadFighterAObj16File(u32 token, u32 asset_id,
                                                void *heap)
{
    NDSRelocAssetHeader header;
    NDSRelocLoadedFile *loaded;
    size_t asset_size;
    void *packed;

    if ((heap == NULL) ||
        (ndsRelocIsFighterAnimID(asset_id) == FALSE))
    {
        return NULL;
    }
    /* Slice 1 phase 7's denominator. Without it every K0 row could read zero
     * because nobody asked, which is indistinguishable from a deletion. */
    NDS_K0_MARK(gNdsK0AfterGoAcquisitions, asset_id);
    /* The install that follows this fetch replaces the fighter's binding; its
     * old clip is not read in between, so it needs no rescue. */
    ndsR2AnimPinsDropHeap(heap);

    /* Settle arena ownership BEFORE the pack lookup, not only before the raw
     * cache find further down. The pack's bytes live in that arena, and the
     * pack is consulted ahead of every other guard in this function -- so a
     * scene rewind that had not yet been noticed would otherwise return a
     * figatree pointing into memory the new scene already owns. Invalidating at
     * the seam where the invariant breaks, rather than at each reader, is the
     * shape the gMPCollisionGeometry fix settled on. */
    ndsR2AnimCacheValidateGeneration();
    /* The resident battlepack is still the measured Mario/Fox P2-2 feature.
     * P2-3 fighters use the same generic force-loader/cache semantics but are
     * not counted as misses against a pack that cannot contain them. */
    packed = (ndsRelocIsMarioFoxAnimID(asset_id) != FALSE) ?
        ndsBattlePackFindFigatree(asset_id) : NULL;

    /* SLICE 1 PHASE 5 -- the acquisition path, deleted.
     *
     * Everything below this block exists to reconstruct an N64 loaded-file
     * image at `heap` on every action change. For a resident clip none of it
     * is needed: the bytes are already in the parser's own format, at a fixed
     * address. Pristine BattleShip still hardcodes `figatree_heap`; the
     * import/compat seam records this function's return and substitutes it when
     * lbCommonAddFighterPartsFigatree receives that heap pointer.
     *
     * The three status writes stay. They are a table store each, they are what
     * `lbRelocGetStatusBufferFile` answers from, and pointing them at the pack
     * keeps every existing reader correct. What goes is the memcpy, the alias
     * strip, the loaded-file registration and the whole finalize chain --
     * internal fixups, AObj16 normalization, attribute/weapon normalization,
     * external fixups and the sprite pass. */
    if (packed != NULL)
    {
        ndsRelocSetStatusBufferFile(token, packed);
        ndsRelocSetStatusBufferFile(asset_id, packed);
        ndsRelocSetForceStatusBufferFile(token, packed);
        gNdsBattlePackHits++;
        NDS_K0_MARK(gNdsK0AfterGoPackHits, asset_id);
        return packed;
    }
    if (ndsRelocIsMarioFoxAnimID(asset_id) != FALSE)
    {
        gNdsBattlePackMisses++;
    }

    /* Existence check without I/O. This used to be ndsRelocAssetAllocSize,
     * which answers it by opening the file and parsing the header -- a full
     * NitroFS directory walk for a question the asset table already answers,
     * and for a size the load below reports anyway (Task 76).
     * ndsRelocAssetGetPath is a table lookup -- but for a generated fighter
     * clip it FORMATS the path string, on every status change. A clip the raw
     * cache holds was loaded through that path, so it exists; only a clip the
     * cache does not hold asks the table. */
    if (
        (ndsR2AnimCacheFind(asset_id) == NULL) &&
        (ndsRelocAssetGetPath(asset_id) == NULL))
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return NULL;
    }
    asset_size = 0u;
#if NDS_TICK_HUD && defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    /* Lab builds only: the destination scan below walks the whole loaded-file
     * table on every status change, and the gate ROM is not the place for an
     * instrument's own cost.
     *
     * Observation only -- the load below is unchanged. "Force" may well mean
     * the caller wants pristine data restored, and the renderer does mutate
     * loaded fighter data, so the reload is not assumed redundant. This just
     * measures how often it reloads an asset the destination already holds. */
    NDS_TICK_HUD_NATIVE_OWNER_MARK(
        nNDSTickHudNativeOwnerFallbackAnimForceLoad);
    {
        NDSRelocLoadedFile *resident = ndsRelocFindLoadedFileByData(heap);

        if ((resident != NULL) && (resident->asset_id == asset_id))
        {
            NDS_TICK_HUD_NATIVE_OWNER_MARK(
                nNDSTickHudNativeOwnerFallbackAnimForceResident);
        }
    }
    /* R2-04 E0. AnimForceResident reads 0 because it asks whether the
     * DESTINATION heap already holds the asset, and the destination is
     * caller-owned and reused, so it almost never does. That refutes a
     * destination-side residency check, not the whole opportunity: what decides
     * whether an asset-keyed cache is worth building is how often the SAME
     * animation is force-loaded more than once over a match. A 128-frame window
     * is ~4 seconds and cannot show it; Mario returns to Wait/Walk/Jump for the
     * whole minute.
     *
     * Bitmap over the 301 Mario+Fox animation IDs: total loads, distinct assets,
     * repeats. repeats/total is exactly the fraction a cache would remove, and
     * distinct sizes the cache. Lab counters, tick-HUD builds only. */
    if (ndsRelocIsMarioFoxAnimID(asset_id) != FALSE)
    {
        u32 anim_index = asset_id - NDS_RELOC_ASSET_MARIO_ANIM_WAIT;

        NDS_DIAG(gNdsR204AnimForceLoadTotal++);
        if (anim_index < NDS_R204_ANIM_ID_SPAN)
        {
            u32 word = anim_index >> 5;
            u32 mask = 1u << (anim_index & 31u);

            if ((gNdsR204AnimSeen[word] & mask) != 0u)
            {
                NDS_DIAG(gNdsR204AnimForceLoadRepeat++);
            }
            else
            {
                gNdsR204AnimSeen[word] |= mask;
                NDS_DIAG(gNdsR204AnimForceLoadDistinct++);
            }
        }
    }
#endif
    /* R2-04 E1. A hit replaces the NitroFS walk, the cartridge read and the word
     * byte-swap with one copy. The fixups below still run against this heap,
     * because they write absolute pointers into it. */
    {
        const NDSR2AnimCacheEntry *cached = ndsR2AnimCacheFind(asset_id);
        sb32 direct = FALSE;

        if (cached == NULL)
        {
            cached = ndsR2AnimDirectReadEntry(asset_id);
            direct = (cached != NULL) ? TRUE : FALSE;
        }
        if (cached != NULL)
        {
            if (direct == FALSE)
            {
                NDSR2AnimCacheEntry *used =
                    (NDSR2AnimCacheEntry *)(uintptr_t)cached;

                /* A use: the ring steps over this clip on its next pass. */
                used->referenced = 1u;
                sNdsR2AnimCacheRef[used - sNdsR2AnimCache] = 1u;
                if (used->prefetched != 0u)
                {
                    used->prefetched = 0u;
                    NDS_DIAG(gNdsR2AnimPrefetchUsed++);
                }
            }
            if (cached->aobj16_ready == NDS_R2_ANIM_CACHE_READY_STREAM)
            {
                const u8 *payload = ndsR2AnimPinTake(heap, cached, asset_id);

                if (payload != NULL)
                {
                    if (direct != FALSE)
                    {
                        gNdsR2AnimCacheMisses++;
                        NDS_DIAG(gNdsR2AnimDirectReads++);
                    }
                    else
                    {
                        gNdsR2AnimCacheHits++;
                        gNdsR2AnimZeroCopyHits++;
                    }
                    ndsFighterManagerRecordExternToken(token, payload);
                    return (void *)(uintptr_t)payload;
                }
            }
            /* K0 line 6, "raw animation-file cache copies". */
            NDS_K0_MARK(gNdsK0AfterGoCacheCopies, asset_id);
            ndsRelocPrepareFighterAnimHeapOverwrite(asset_id, heap);
            u32 ready;

            memcpy(heap, cached->payload, cached->size);
            asset_size = cached->size;
            header = cached->header;
            /* Read everything the entry holds before anything can allocate:
             * an allocation may take an elastic cache's bytes back and drop
             * this entry (ndsR2AnimCacheElasticYield). */
            ready = cached->aobj16_ready;
            cached = NULL;
            loaded = ndsRelocRegisterLoadedFile(asset_id, 0, heap, &header);
            if (ready == NDS_R2_ANIM_CACHE_READY_STREAM)
            {
                if (loaded == NULL)
                {
                    goto fail;
                }
                ndsRelocMarkLoadedFileRelativeOffsets(loaded);
                if (direct != FALSE)
                {
                    gNdsR2AnimCacheMisses++;
                }
                else
                {
                    gNdsR2AnimCacheHits++;
                }
                ndsFighterManagerRecordExternToken(token, heap);
                ndsRelocSetStatusBufferFile(token, heap);
                ndsRelocSetStatusBufferFile(asset_id, heap);
                ndsRelocSetForceStatusBufferFile(token, heap);
                return heap;
            }
            /* R2-06 E13. The script region of this copy already carries the
             * transform, applied once at warm time against the arena base --
             * position-independent, so byte-identical to what the pass would
             * write here. Claiming it is what removes the pass from the frame.
             * The flag is per entry: anything the prebake declined still runs
             * the pass below, unchanged. */
            if ((loaded != NULL) &&
                (ready == NDS_R2_ANIM_CACHE_READY_PREBAKE))
            {
                loaded->format_fixups_applied = TRUE;
                NDS_DIAG(gNdsR2AObj16PrebakeSkips++);
            }
            if ((loaded == NULL) ||
                (ndsRelocFinalizeLoadedFile(loaded) == FALSE))
            {
                goto fail;
            }
            if (direct != FALSE)
            {
                gNdsR2AnimCacheMisses++;
            }
            else
            {
                gNdsR2AnimCacheHits++;
            }
            ndsFighterManagerRecordExternToken(token, heap);
            ndsRelocSetStatusBufferFile(token, heap);
            ndsRelocSetStatusBufferFile(asset_id, heap);
            ndsRelocSetForceStatusBufferFile(token, heap);
            return heap;
        }
        gNdsR2AnimCacheMisses++;
    }
    {
        u32 stream_size = 0u;

        ndsRelocPrepareFighterAnimHeapOverwrite(asset_id, heap);
        if (ndsRelocAssetLoadFighterStreamClip(asset_id, heap,
                                                &stream_size) != FALSE)
        {
            memset(&header, 0, sizeof(header));
            header.file_id = asset_id;
            header.data_size = stream_size;
            header.reloc_intern_offset = 0xffffu;
            header.reloc_extern_offset = 0xffffu;
            asset_size = stream_size;
            ndsR2AnimCacheStore(asset_id, heap, stream_size, &header,
                                NDS_R2_ANIM_CACHE_READY_STREAM);
            loaded = ndsRelocRegisterLoadedFile(asset_id, 0, heap, &header);
            if (loaded == NULL)
            {
                goto fail;
            }
            ndsRelocMarkLoadedFileRelativeOffsets(loaded);
            ndsFighterManagerRecordExternToken(token, heap);
            ndsRelocSetStatusBufferFile(token, heap);
            ndsRelocSetStatusBufferFile(asset_id, heap);
            ndsRelocSetForceStatusBufferFile(token, heap);
            return heap;
        }
    }
    /* One open for the size, the zero and the payload. Task 72 collapsed the
     * probe-then-load pair here; the sizing call above was the third walk of
     * the same directory for the same header, and this is the on-demand
     * fighter-animation load Task 71 caught running inside the frame that needs
     * the move. The callee zeroes heap over the aligned payload size before
     * reading, which is the memset that used to stand here, and reports that
     * size back for the failure path below. */
    ndsRelocPrepareFighterAnimHeapOverwrite(asset_id, heap);
    gNdsRelocForceFighterAnimFallbackStep = 1u;
    if (ndsRelocAssetLoadIntoZeroedHeap(asset_id, heap, NDS_RELOC_ALIGN_BYTES,
                                        &asset_size, &header) == FALSE)
    {
        goto fail;
    }
    gNdsRelocForceFighterAnimFallbackStep = 2u;
    loaded = ndsRelocRegisterLoadedFile(asset_id, 0, heap, &header);
    if (loaded == NULL)
    {
        goto fail;
    }
    gNdsRelocForceFighterAnimFallbackStep = 3u;
    if (ndsRelocApplyWordByteSwap(loaded) == FALSE)
    {
        goto fail;
    }
    /* Snapshot here: swapped, not yet fixed up. One instruction later the
     * fixups make the image position-dependent and unusable as a template. */
    ndsR2AnimCacheStore(asset_id, heap, (u32)asset_size, &header,
                        NDS_R2_ANIM_CACHE_READY_RAW);
    /* The store can wrap the raw ring over another fighter's pinned
     * zero-copy clip. That clip's rescue re-registers it and compacts
     * sNdsRelocLoadedFiles (ndsRelocPrepareFighterAnimHeapOverwrite), so
     * `loaded` may now name a different record: finalizing that one left this
     * clip's relocation chain unresolved, every joint resolved NULL and the
     * fighter's new status played no animation (a Samus roll that ended the
     * tick it began, 2026-09-30). Whether the ring reaches a pin depends on
     * the arena's size, i.e. on the build's heap, so the simulation followed
     * the code layout. This clip's record is the one that holds its heap. */
    loaded = ndsRelocFindLoadedFileByAsset(asset_id);
    if ((loaded == NULL) || (loaded->data != heap))
    {
        goto fail;
    }
    gNdsRelocForceFighterAnimFallbackStep = 4u;
    if (ndsRelocFinalizeLoadedFile(loaded) == FALSE)
    {
        goto fail;
    }
    gNdsRelocForceFighterAnimFallbackStep = 0u;

    ndsFighterManagerRecordExternToken(token, heap);
    ndsRelocSetStatusBufferFile(token, heap);
    ndsRelocSetStatusBufferFile(asset_id, heap);
    ndsRelocSetForceStatusBufferFile(token, heap);
    return heap;

fail:
    memset(heap, 0, asset_size);
    ndsRelocRecordExternalFixupFail(asset_id);
    return NULL;
}

typedef struct NDSRelocForceResult {
    void *heap;
    void *file;
    u32 heap_generation;
} NDSRelocForceResult;

#define NDS_RELOC_FORCE_RESULT_SLOTS 8u
static NDSRelocForceResult sNdsRelocForceResults[NDS_RELOC_FORCE_RESULT_SLOTS];
static u32 sNdsRelocForceResultNext;

static void ndsRelocRecordAuthoritativeForceFile(void *heap, void *file)
{
    u32 i;

    if (heap == NULL)
    {
        return;
    }
    for (i = 0u; i < NDS_RELOC_FORCE_RESULT_SLOTS; i++)
    {
        if (sNdsRelocForceResults[i].heap == heap)
        {
            sNdsRelocForceResults[i].file = file;
            sNdsRelocForceResults[i].heap_generation = gNdsTaskmanHeapGeneration;
            return;
        }
    }
    i = sNdsRelocForceResultNext++ % NDS_RELOC_FORCE_RESULT_SLOTS;
    sNdsRelocForceResults[i].heap = heap;
    sNdsRelocForceResults[i].file = file;
    sNdsRelocForceResults[i].heap_generation = gNdsTaskmanHeapGeneration;
}

void *ndsRelocResolveAuthoritativeForceFile(void *file)
{
    u32 i;

    if (file == NULL)
    {
        return NULL;
    }
    for (i = 0u; i < NDS_RELOC_FORCE_RESULT_SLOTS; i++)
    {
        if ((sNdsRelocForceResults[i].heap == file) &&
            (sNdsRelocForceResults[i].heap_generation == gNdsTaskmanHeapGeneration))
        {
            return sNdsRelocForceResults[i].file;
        }
    }
    return file;
}

#if ((defined(NDS_LAB_CLIP_TRACE) && NDS_LAB_CLIP_TRACE) && NDS_TICK_HUD) && defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
/* LAB (a lab build with CPPFLAGS=-DNDS_LAB_CLIP_TRACE=1): one word per
 * fighter clip fetch -- clip index (asset - 499, 11 bits), outcome (2: 0
 * zero-copy hit, 1 copy hit, 2 storage read, 3 other), battle slot (2), frame
 * (17). The source of scripts/motion/generate_clip_successors.py's table;
 * read with -ExtraGlobals gNdsLabClipTraceCount,gNdsLabClipTrace[i]... */
#define NDS_LAB_CLIP_TRACE_ON 1
#define NDS_LAB_CLIP_TRACE_MAX 900u
__attribute__((used)) volatile u32 gNdsLabClipTrace[NDS_LAB_CLIP_TRACE_MAX];
__attribute__((used)) volatile u32 gNdsLabClipTraceCount;
extern volatile u32 gNdsFrameCounter;

static void ndsLabClipTraceNote(u32 asset_id, const void *heap, u32 outcome)
{
    u32 slot = 3u;
    GObj *gobj;
    const u32 n = gNdsLabClipTraceCount;

    for (gobj = gGCCommonLinks[nGCCommonLinkIDFighter]; gobj != NULL;
         gobj = gobj->link_next)
    {
        FTStruct *fp = ftGetStruct(gobj);

        if ((fp != NULL) && (fp->figatree_heap == heap))
        {
            slot = (u32)fp->player & 3u;
            break;
        }
    }
    if (n < NDS_LAB_CLIP_TRACE_MAX)
    {
        gNdsLabClipTrace[n] = ((asset_id - 499u) & 0x7ffu) |
            ((outcome & 3u) << 11) | (slot << 13) |
            ((gNdsFrameCounter & 0x1ffffu) << 15);
    }
    gNdsLabClipTraceCount = n + 1u;
}
#else
#define NDS_LAB_CLIP_TRACE_ON 0
#endif

void *lbRelocGetForceExternHeapFile(const void *file_id, void *heap)
{
    u32 token = ndsRelocFileID(file_id);
    void *file;
    u32 asset_id = ndsRelocAssetIDForToken(token);
    NDSRelocLoadedFile *loaded;

    /* BattleShip lbreloc.c:363-369: every force load starts an empty force
     * status buffer. The port accumulated distinct tokens for the whole scene
     * in a 7-slot buffer, so the eighth distinct fighter animation of a match
     * was dropped with an open-fail count (2026-09-07). */
    sNdsRelocForceStatusBufferCount = 0;

    file = ndsBattleShipLoadCSSSelectedFigatree(file_id, heap);
    if (file != NULL)
    {
        /* The heap now holds the compiled Selected table, so neither the
         * zero-copy pin nor the authoritative result of the clip it held
         * before may outlive this load: ftMainSetStatus resolves fp->figatree
         * through that record, and a stale one handed the CSS preview the
         * previous clip (Pikachu and Ness contorted after selection, the
         * selection intro cut short; owner playtest r55, 2026-09-30). */
        ndsR2AnimPinsDropHeap(heap);
        ndsRelocRecordAuthoritativeForceFile(heap, file);
        return file;
    }

    if ((heap != NULL) &&
        (ndsRelocIsFighterAnimID(asset_id) != FALSE))
    {
        /* K0 line 7's other half, and it is the one the pack does NOT delete:
         * the token -> asset-id resolution above runs BEFORE the pack is
         * consulted, so it is still paid on every acquisition. The asset -> path
         * discovery (gNdsK0AfterGoPathLookups) is the half the pack removes. */
        NDS_K0_MARK(gNdsK0AfterGoTokenResolves, asset_id);
#if NDS_TICK_HUD
        /* Census: which fighter clips this match fetched (hit or read). */
        if (asset_id < (NDS_R2_ANIM_STORED_BITS * 32u))
        {
            gNdsAnimUseBits[asset_id >> 5] |= 1u << (asset_id & 31u);
        }
#endif
        /* Finished prefetches become entries first; this clip in flight is
         * waited for rather than read again. */
        ndsR2AnimPrefetchPoll(asset_id);
#if NDS_LAB_CLIP_TRACE_ON
        {
            const u32 lab_zc = gNdsR2AnimZeroCopyHits;
            const u32 lab_hits = gNdsR2AnimCacheHits;
            const u32 lab_reads = gNdsRelocAssetFighterStreamReads;

            file = ndsRelocForceLoadFighterAObj16File(token, asset_id, heap);
            ndsLabClipTraceNote(asset_id, heap,
                (gNdsRelocAssetFighterStreamReads != lab_reads) ? 2u :
                (gNdsR2AnimZeroCopyHits != lab_zc) ? 0u :
                (gNdsR2AnimCacheHits != lab_hits) ? 1u : 3u);
        }
#else
        file = ndsRelocForceLoadFighterAObj16File(token, asset_id, heap);
#endif
        /* P2-3. THE FALLBACK IS SILENT, AND IT COSTS AN ANIMATION.
         * `ndsRelocForceLoadFighterAObj16File` returning NULL means the
         * fighter's animation was not loaded, and handing the raw heap back
         * makes ftMainSetStatus parse whatever the previous motion left there:
         * the status still changes, the figatree pointer still looks valid, and
         * the only symptom is an animation that does not play. Count both arms
         * so the next "the intro does not run" report is one counter read
         * rather than a hunt. */
        if (file != NULL)
        {
            gNdsRelocForceFighterAnimResolveCount++;
            ndsR2AnimLearnNote(heap, asset_id);
            ndsR2AnimPrefetchNote(asset_id);
        }
        else
        {
            gNdsRelocForceFighterAnimFallbackCount++;
            gNdsRelocForceFighterAnimFallbackLastAsset = asset_id;
        }
        file = (file != NULL) ? file : heap;
        ndsRelocRecordAuthoritativeForceFile(heap, file);
        return file;
    }

    file = lbRelocGetExternHeapFile(file_id, heap);
    if ((file != NULL) && (heap != NULL) && (file != heap))
    {
        loaded = ndsRelocFindLoadedFileByData(file);
        /* A compact polygon model is not a contiguous image to copy. */
        if ((loaded != NULL) && (loaded->reserved[2] == 0u))
        {
            ndsRelocCopyLoadedFileToHeap(loaded, heap);
            file = heap;
        }
    }

    ndsRelocSetForceStatusBufferFile(token, file);
    ndsRelocRecordAuthoritativeForceFile(heap, file);
    return file;
}

void *lbRelocGetStatusBufferFile(const void *file_id)
{
    u32 token = ndsRelocFileID(file_id);
    u32 asset_id = ndsRelocAssetIDForToken(token);
    void *file;

    ndsRelocPrepareSceneCache();

    file = ndsRelocFindStatusNode(sNdsRelocStatusBuffer,
                                  sNdsRelocStatusBufferCount,
                                  token);
    if ((file == NULL) && (asset_id != NDS_RELOC_ASSET_INVALID))
    {
        file = ndsRelocFindStatusNode(sNdsRelocStatusBuffer,
                                      sNdsRelocStatusBufferCount,
                                      asset_id);
    }
#if (NDS_P2_1P_GAME || NDS_P2_MENU_SHELL) || NDS_P2_COMPACT_BATTLE_FIGHTERS
    if ((file == NULL) && (asset_id != NDS_RELOC_ASSET_INVALID))
    {
        /* A compact fighter pack makes Main and Model resident WITHOUT a
         * status node. scVSBattleStartBattle then runs the source's
         * ftManagerSetupFilesPlayablesAll, whose ftManagerSetupFilesKind
         * re-queries every Model here (ftmanager.c:312) -- and the miss wiped
         * the slot the pack loader had just published. Every Model-backed
         * EFDesc (Pikachu's Thunder trail, Yoshi's egg escape, Ness's PK
         * Thunder family) then saw a NULL file head and made no effect at
         * all. Still lookup-only: only a record THIS scene already made
         * resident answers, so an unselected fighter stays NULL.
         *
         * Not only pack records. PikachuSpecial3 (asset 342) is a full file
         * that arrives as Special1's external dependency, resident with no
         * node either, so gFTDataPikachuSpecial3 read NULL all match and the
         * grounded Thunder Jolt's effect desc could never be backed. */
        NDSRelocLoadedFile *packed = ndsRelocFindLoadedFileByAsset(asset_id);

        if ((packed != NULL) && (packed->data != NULL) &&
            (packed->owner_generation == sNdsRelocSceneGeneration) &&
            (packed->owner_scene == (u32)gSCManagerSceneData.scene_curr))
        {
            file = packed->data;
        }
    }
#endif
    if (file != NULL)
    {
        ndsFighterManagerRecordStatusToken(token, file);
    }

    /* BattleShip's lbRelocGetStatusBufferFile is a pure residency query:
     * lbreloc.c:63-66 delegates directly to lbRelocFindStatusBufferFile and
     * never allocates or loads on a miss.  Fighter setup relies on the selected
     * fighter's Main-file external closure having populated the status buffer;
     * ftManagerSetupFilesPlayablesAll intentionally leaves absent, unselected
     * fighters as NULL.  The old DS fallback turned those harmless misses into
     * taskman allocations, which both diverged from the source contract and
     * made roster admission consume RAM for fighters that were not in the
     * match.  Keep the DS-only symbol-address -> numeric-id fallback above, but
     * preserve the source's lookup-only behavior after that translation. */
    return file;
}

/* decomp lb/lbreloc.c:85-88. The same residency query as the function above,
 * taking a raw file id instead of a pointer to one: Sector Z looks its own
 * map file up this way (grsector.c:1094). Like its twin it is lookup-only --
 * the source never allocates on a miss and neither does this. */
void *lbRelocGetForceStatusBufferFile(u32 id)
{
    u32 asset_id = ndsRelocAssetIDForToken(id);
    void *file;

    ndsRelocPrepareSceneCache();

    /* lbreloc.c:68-83 scans the force buffer first and only then falls back
     * to the status buffer; the port scanned the status buffer alone, so a
     * file that only a force load had made resident was invisible here. */
    file = ndsRelocFindStatusNode(sNdsRelocForceStatusBuffer,
                                  sNdsRelocForceStatusBufferCount, id);
    if (file == NULL)
    {
        file = ndsRelocFindStatusNode(sNdsRelocStatusBuffer,
                                      sNdsRelocStatusBufferCount, id);
    }
    if ((file == NULL) && (asset_id != NDS_RELOC_ASSET_INVALID))
    {
        file = ndsRelocFindStatusNode(sNdsRelocStatusBuffer,
                                      sNdsRelocStatusBufferCount, asset_id);
    }
    return file;
}

/* P2-2p8 Phase 3 (A7 memory), 2026-09-23: THE COMPACT IFCommonGameStatus.
 *
 * Every battle keeps this 152,368-byte file resident all match, yet after the
 * load the DS reads only its Sprite/Bitmap headers and the lamp, rod and
 * frame pixels: the GO letters are baked into OBJ VRAM while the file
 * finalizes (ndsIFCommonNativeOamPrepareGameStatus), and here both end
 * messages (TIME UP, GAME SET) are baked into run-length streams. So the file
 * is finalized in free arena space at the TOP of the heap -- nothing owns
 * that space, nothing allocates while this runs -- and then copied, minus the
 * twelve letters' pixel payloads (~120 KB), into an ordinary allocation.
 * Every internal pointer is re-seated from the slots the fixup walk recorded;
 * a pointer into a dropped payload becomes NULL (only those letters'
 * Bitmap.buf, which nothing reads after the bake), and ndsRelocGetFileData
 * maps a source offset through the span table. If any step cannot complete,
 * the whole file is moved down instead (the old footprint, never a partial
 * image). */
#define NDS_RELOC_IF_COMPACT_MAX_DROPS 64u
#define NDS_RELOC_IF_COMPACT_MAX_SPANS (NDS_RELOC_IF_COMPACT_MAX_DROPS + 1u)
#define NDS_RELOC_IF_COMPACT_SLOT_CAP 2048u
#define NDS_RELOC_IF_COMPACT_MARGIN (64u * 1024u)

typedef struct NDSRelocIfSpan
{
    u32 source;
    u32 data;
    u32 bytes;
} NDSRelocIfSpan;

static NDSRelocIfSpan sNdsRelocIfSpans[NDS_RELOC_IF_COMPACT_MAX_SPANS];
static u32 sNdsRelocIfSpanCount;
__attribute__((used)) volatile u32 gNdsRelocIfCompactCount;
__attribute__((used)) volatile u32 gNdsRelocIfCompactMoveCount;
__attribute__((used)) volatile u32 gNdsRelocIfCompactFailStage;
__attribute__((used)) volatile u32 gNdsRelocIfCompactSourceBytes;
__attribute__((used)) volatile u32 gNdsRelocIfCompactBytes;
__attribute__((used)) volatile u32 gNdsRelocIfCompactDropped;
__attribute__((used)) volatile u32 gNdsRelocIfCompactSlots;
__attribute__((used)) volatile u32 gNdsRelocIfCompactNulled;

static s32 ndsRelocIfCompactMap(u32 source_offset, u32 bytes,
                                u32 *out_offset)
{
    u32 i;

    for (i = 0u; i < sNdsRelocIfSpanCount; i++)
    {
        const NDSRelocIfSpan *span = &sNdsRelocIfSpans[i];

        if ((source_offset >= span->source) &&
            ((source_offset - span->source) < span->bytes) &&
            (bytes <= (span->bytes - (source_offset - span->source))))
        {
            *out_offset = span->data + (source_offset - span->source);
            return TRUE;
        }
    }
    return FALSE;
}

/* Kept spans = [0, size) minus the sorted, merged drops. Each span's data
 * offset keeps its source offset's alignment modulo 8. Returns the image
 * size, 0 on overflow. */
static u32 ndsRelocIfBuildSpans(u32 size, u32 *drop_at, u32 *drop_bytes,
                                u32 drops)
{
    u32 i;
    u32 source = 0u;
    u32 cursor = 0u;

    for (i = 1u; i < drops; i++)
    {
        u32 at = drop_at[i];
        u32 bytes = drop_bytes[i];
        u32 j = i;

        while ((j > 0u) && (drop_at[j - 1u] > at))
        {
            drop_at[j] = drop_at[j - 1u];
            drop_bytes[j] = drop_bytes[j - 1u];
            j--;
        }
        drop_at[j] = at;
        drop_bytes[j] = bytes;
    }
    sNdsRelocIfSpanCount = 0u;
    for (i = 0u; i <= drops; i++)
    {
        u32 end = (i < drops) ? drop_at[i] : size;

        if (end > source)
        {
            NDSRelocIfSpan *span;

            if (sNdsRelocIfSpanCount >= NDS_RELOC_IF_COMPACT_MAX_SPANS)
            {
                return 0u;
            }
            span = &sNdsRelocIfSpans[sNdsRelocIfSpanCount++];
            cursor += (source - cursor) & 7u;
            span->source = source;
            span->data = cursor;
            span->bytes = end - source;
            cursor += span->bytes;
        }
        if (i < drops)
        {
            u32 drop_end = drop_at[i] + drop_bytes[i];

            if (drop_end > source)
            {
                source = drop_end;
            }
        }
    }
    return cursor;
}

/* Every captured slot must sit inside kept data and point inside the file.
 * Checked before any allocation, so a failure can still fall back cleanly. */
static s32 ndsRelocIfValidateSlots(const u8 *temp, u32 size)
{
    u32 i;

    for (i = 0u; i < sNdsRelocSlotCaptureCount; i++)
    {
        u32 slot = (u32)sNdsRelocSlotCapture[i] * sizeof(u32);
        uintptr_t target;
        u32 mapped;

        if ((slot + sizeof(u32)) > size)
        {
            return FALSE;
        }
        target = (uintptr_t)ndsRelocReadNative32(temp + slot) -
                 (uintptr_t)temp;
        if ((ndsRelocIfCompactMap(slot, sizeof(u32), &mapped) == FALSE) ||
            (target >= size))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/* Copy the finalized file at `temp` into `dst` through the span table and
 * re-seat every captured internal slot (validated first). A slot whose
 * target was dropped becomes NULL; returns how many did. */
static u32 ndsRelocIfCopyImage(const u8 *temp, u8 *dst)
{
    u32 i;
    u32 nulled = 0u;

    for (i = 0u; i < sNdsRelocIfSpanCount; i++)
    {
        memcpy(dst + sNdsRelocIfSpans[i].data,
               temp + sNdsRelocIfSpans[i].source, sNdsRelocIfSpans[i].bytes);
    }
    for (i = 0u; i < sNdsRelocSlotCaptureCount; i++)
    {
        u32 slot = (u32)sNdsRelocSlotCapture[i] * sizeof(u32);
        u32 target =
            (u32)((uintptr_t)ndsRelocReadNative32(temp + slot) -
                  (uintptr_t)temp);
        u32 slot_mapped = 0u;
        u32 target_mapped;

        (void)ndsRelocIfCompactMap(slot, sizeof(u32), &slot_mapped);
        if (ndsRelocIfCompactMap(target, 1u, &target_mapped) != FALSE)
        {
            ndsRelocWriteNativePointer(dst + slot_mapped,
                                       dst + target_mapped);
        }
        else
        {
            ndsRelocWriteNativePointer(dst + slot_mapped, NULL);
            nulled++;
        }
    }
    return nulled;
}

/* The compact load. Returns the resident image -- compact, or the whole file
 * moved down with its pointers re-seated -- or NULL when the top-of-heap
 * window was never used (the caller then loads the file the ordinary way;
 * gNdsRelocIfCompactFailStage <= 4 says so). */
void ndsTaskmanElasticReleaseAll(void);

static void *ndsRelocLoadIfGameStatusCompact(u32 token, u32 asset_id,
                                             u32 bit,
                                             const NDSRelocAssetHeader *in)
{
    NDSRelocAssetHeader header = *in;
    u32 full = (u32)NDS_RELOC_ALIGN(header.data_size);
    u32 slot_cap = (full / sizeof(u32)) + 1u;
    uintptr_t heap_ptr = (uintptr_t)gSYTaskmanGeneralHeap.ptr;
    uintptr_t heap_end = (uintptr_t)gSYTaskmanGeneralHeap.end;
    u32 code_words = ((NDS_IFCOMMON_END_BANK_IMAGE_BYTES / 2u) * 3u) / 2u +
                     16u;
    uintptr_t temp;
    uintptr_t capture;
    uintptr_t scratch;
    uintptr_t code;
    NDSRelocLoadedFile *loaded;
    u32 drop_at[NDS_RELOC_IF_COMPACT_MAX_DROPS];
    u32 drop_bytes[NDS_RELOC_IF_COMPACT_MAX_DROPS];
    u32 drops = 0u;
    u32 image_size = 0u;
    u32 source_size;
    u8 *image;
    u32 compact = FALSE;
    u32 attempt;
    u32 i;

    gNdsRelocIfCompactFailStage = 0u;
    if ((heap_ptr == 0u) || (heap_end <= heap_ptr) || (heap_end < full))
    {
        gNdsRelocIfCompactFailStage = 1u;
        return NULL;
    }
    temp = (heap_end - full) & ~(uintptr_t)31u;
    capture = (temp - (slot_cap * sizeof(u16))) & ~(uintptr_t)31u;
    scratch = (capture - NDS_IFCOMMON_END_BANK_IMAGE_BYTES) &
              ~(uintptr_t)31u;
    code = (scratch - (code_words * sizeof(u16))) & ~(uintptr_t)31u;
    if ((code <= heap_ptr) ||
        ((code - heap_ptr) < ((uintptr_t)full + NDS_RELOC_IF_COMPACT_MARGIN)))
    {
        gNdsRelocIfCompactFailStage = 2u;
        return NULL;
    }
    if (ndsRelocAssetLoadData(asset_id, (void *)temp, full, &header) == FALSE)
    {
        gNdsRelocIfCompactFailStage = 3u;
        return NULL;
    }
    loaded = ndsRelocRegisterLoadedFile(asset_id, bit, (void *)temp, &header);
    if (loaded == NULL)
    {
        gNdsRelocIfCompactFailStage = 4u;
        return NULL;
    }

    /* From here the file lives in space nothing owns: every path must end in
     * an allocation that holds it. */
    sNdsRelocSlotCapture = (u16 *)capture;
    sNdsRelocSlotCaptureCap = slot_cap;
    sNdsRelocSlotCaptureCount = 0u;
    sNdsRelocSlotCaptureOverflow = FALSE;
    sNdsRelocSlotCaptureActive = TRUE;
    if ((ndsRelocApplyWordByteSwap(loaded) != FALSE) &&
        (ndsRelocFinalizeLoadedFile(loaded) != FALSE))
    {
        /* Stop recording, keep the record: the copy below reads it. */
        sNdsRelocSlotCaptureActive = FALSE;
        ndsRelocNormalizeGroundMapAsset(loaded);
        ndsRelocNormalizeStageDreamLandSprite(loaded);
        if (ndsIFCommonNativeOamIsPreparedFile((const void *)temp) != FALSE)
        {
            drops = ndsIFCommonNativeOamLetterPayloads(
                drop_at, drop_bytes, NDS_RELOC_IF_COMPACT_MAX_DROPS);
            if ((drops != 0u) &&
                (ndsIFCommonNativeOamBakeEndVariants(
                     (u16 *)scratch, (u16 *)code, code_words) == FALSE))
            {
                gNdsRelocIfCompactFailStage = 6u;
                drops = 0u;
            }
        }
    }
    else
    {
        gNdsRelocIfCompactFailStage = 5u;
    }
    sNdsRelocSlotCaptureActive = FALSE;
    gNdsRelocIfCompactSlots = sNdsRelocSlotCaptureCount;
    source_size = loaded->data_size;

    /* Attempt 0 drops the letters; attempt 1 is the plain move. */
    image = NULL;
    for (attempt = 0u; (attempt < 2u) && (image == NULL); attempt++)
    {
        u32 use = (attempt == 0u) ? drops : 0u;

        if ((attempt == 0u) && (use == 0u))
        {
            continue;
        }
        image_size = ndsRelocIfBuildSpans(source_size, drop_at, drop_bytes,
                                          use);
        if ((image_size == 0u) || (sNdsRelocSlotCaptureOverflow != FALSE) ||
            (ndsRelocIfValidateSlots((const u8 *)temp, source_size) ==
             FALSE))
        {
            gNdsRelocIfCompactFailStage = 7u;
            continue;
        }
        image = (u8 *)syTaskmanMalloc(NDS_RELOC_ALIGN(image_size), 0x10);
        compact = (use != 0u) ? TRUE : FALSE;
    }
    if ((image == NULL) || (((uintptr_t)image + image_size) > code))
    {
        /* Capture overflow cannot happen (one slot per word); an allocation
         * that reached the window means the heap is exhausted anyway. */
        sNdsRelocSlotCapture = NULL;
        gNdsRelocIfCompactFailStage = 9u;
        return NULL;
    }
    gNdsRelocIfCompactNulled = ndsRelocIfCopyImage((const u8 *)temp, image);
    sNdsRelocSlotCapture = NULL;
    if ((ndsIFCommonNativeOamIsPreparedFile((const void *)temp) != FALSE) &&
        (ndsIFCommonNativeOamRebaseGameStatus(
             (const void *)temp, source_size, image, image_size,
             ndsRelocIfCompactMap, compact) == FALSE))
    {
        gNdsRelocIfCompactFailStage = 8u;
    }
    gNdsRelocIfCompactSourceBytes = source_size;
    loaded->data = image;
    sNdsRelocLoadedFilesEpoch++;
    loaded->data_size = image_size;
    loaded->reserved[0] = (compact != FALSE) ? NDS_RELOC_IF_COMPACT_MARK : 0u;
    ndsRelocAddStatusBufferFile(token, image);
    gNdsRelocIfCompactBytes = image_size;
    gNdsRelocIfCompactDropped = 0u;
    if (compact != FALSE)
    {
        for (i = 0u; i < drops; i++)
        {
            NDS_DIAG(gNdsRelocIfCompactDropped += drop_bytes[i]);
        }
        NDS_DIAG(gNdsRelocIfCompactCount++);
    }
    else
    {
        /* A moved whole file maps as itself; clear the table so no lookup
         * ever consults a stale one. */
        sNdsRelocIfSpanCount = 0u;
        NDS_DIAG(gNdsRelocIfCompactMoveCount++);
    }
    return image;
}

size_t lbRelocGetAllocSize(u32 *ids, u32 len)
{
    size_t total = 0;
    u32 i;

    for (i = 0; i < len; i++)
    {
        u32 asset_id = ndsRelocAssetIDForToken(ids[i]);
        size_t asset_size = ndsRelocAssetAllocSize(asset_id);

        /* Loaded outside this block (ndsRelocLoadIfGameStatusCompact). */
        if (asset_id == NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS)
        {
            asset_size = 0u;
        }
        total = NDS_RELOC_ALIGN(total);
        if (asset_size != 0)
        {
            total += asset_size;
        }
        else
        {
            total += sizeof(uintptr_t);
        }
    }
    return total;
}

size_t lbRelocLoadFilesExtern(u32 *ids, u32 len, void **files, void *heap)
{
    u32 i;
    u32 mask = 0;
    u32 header_mask = 0;
    u32 payload_mask = 0;
    u32 word_swap_mask = 0;
    u32 pointer_fixup_mask = 0;
    uintptr_t heap_ptr = (uintptr_t)heap;
    uintptr_t heap_start = (uintptr_t)heap;

    ndsRelocPrepareSceneCache();

    for (i = 0; i < len; i++)
    {
        u32 token = ids[i];
        u32 asset_id = ndsRelocAssetIDForToken(token);
        u32 bit = ndsRelocOpeningRoomBitForAsset(asset_id);
        NDSRelocAssetHeader header;
        size_t asset_size = 0;
        void *file_alloc = (void *)(uintptr_t)token;

        mask |= bit;

        if ((asset_id != NDS_RELOC_ASSET_INVALID) &&
            (ndsRelocAssetReadHeader(asset_id, &header) != FALSE))
        {
            header_mask |= bit;
            asset_size = (size_t)NDS_RELOC_ALIGN(header.data_size);

            if ((heap != NULL) && (asset_size != 0) &&
                (asset_id == NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS))
            {
                /* lbRelocGetAllocSize sized this entry as a placeholder. The
                 * compact load uses the free bytes above the cursor as its
                 * window, so any elastic cache hands them back first. */
                void *image;

                ndsTaskmanElasticReleaseAll();
                image = ndsRelocLoadIfGameStatusCompact(token, asset_id, bit,
                                                        &header);

                heap_ptr = NDS_RELOC_ALIGN(heap_ptr) + sizeof(uintptr_t);
                if ((image == NULL) && (gNdsRelocIfCompactFailStage <= 4u))
                {
                    /* The window was never used: the ordinary load, in an
                     * allocation of its own. */
                    image = syTaskmanMalloc(asset_size, 0x10);
                    if ((image != NULL) &&
                        (ndsRelocAssetLoadData(asset_id, image, asset_size,
                                               &header) != FALSE))
                    {
                        NDSRelocLoadedFile *loaded =
                            ndsRelocRegisterLoadedFile(asset_id, bit, image,
                                                       &header);

                        ndsRelocAddStatusBufferFile(token, image);
                        if ((ndsRelocApplyWordByteSwap(loaded) != FALSE) &&
                            (ndsRelocFinalizeLoadedFile(loaded) != FALSE))
                        {
                            ndsRelocNormalizeGroundMapAsset(loaded);
                            ndsRelocNormalizeStageDreamLandSprite(loaded);
                        }
                    }
                }
                if (image != NULL)
                {
                    payload_mask |= bit;
                    file_alloc = image;
                }
                if (files != NULL)
                {
                    files[i] = file_alloc;
                }
                continue;
            }
            if ((heap != NULL) && (asset_size != 0))
            {
                heap_ptr = NDS_RELOC_ALIGN(heap_ptr);
                file_alloc = (void *)heap_ptr;

                if (ndsRelocAssetLoadData(asset_id, file_alloc, asset_size, &header) != FALSE)
                {
                    NDSRelocLoadedFile *loaded;

                    payload_mask |= bit;
                    if (bit != 0u)
                    {
                        gNdsOpeningRoomRelocBytesLoaded += header.data_size;
                        gNdsOpeningRoomRelocLastFileID = asset_id;
                        gNdsOpeningRoomRelocLastSize = header.data_size;
                    }

                    loaded = ndsRelocRegisterLoadedFile(asset_id, bit, file_alloc, &header);
                    ndsRelocAddStatusBufferFile(token, file_alloc);
                    if (ndsRelocApplyWordByteSwap(loaded) != FALSE)
                    {
                        word_swap_mask |= bit;
                        if (ndsRelocFinalizeLoadedFile(loaded) != FALSE)
                        {
                            pointer_fixup_mask |= bit;
                            if (asset_id == NDS_RELOC_ASSET_MV_COMMON)
                            {
                                ndsRelocNormalizeMVCommonMObjSubs(loaded);
                            }
                            if ((asset_id ==
                                 NDS_RELOC_ASSET_OPENING_PORTRAITS_SET1) ||
                                (asset_id ==
                                 NDS_RELOC_ASSET_OPENING_PORTRAITS_SET2))
                            {
                                ndsRelocNormalizeOpeningPortraitsSprites(
                                    loaded);
                            }
                            if (asset_id ==
                                NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE)
                            {
                                ndsRelocNormalizeIFAnnounceMarioSprites(
                                    loaded);
                            }
                            if (asset_id == NDS_RELOC_ASSET_MN_VS_RESULTS)
                            {
                                ndsRelocNormalizeVSResultsSprites(loaded);
                            }
                            if (asset_id ==
                                NDS_RELOC_ASSET_MN_TITLE_FIRE_ANIM)
                            {
                                ndsRelocNormalizeTitleFireSprites(loaded);
                            }
                            ndsRelocNormalizeGroundMapAsset(loaded);
                            ndsRelocNormalizeStageDreamLandSprite(loaded);
                        }
                    }
                    heap_ptr += asset_size;
                }
            }
        }

        if (files != NULL)
        {
            files[i] = file_alloc;
        }
    }

    if (gSCManagerSceneData.scene_curr == nSCKindOpeningRoom)
    {
        gNdsOpeningRoomRelocLoadCount += len;
        gNdsOpeningRoomRelocFileMask |= mask;
        gNdsOpeningRoomRelocHeaderMask |= header_mask;
        gNdsOpeningRoomRelocPayloadMask |= payload_mask;
        gNdsOpeningRoomRelocWordSwapMask |= word_swap_mask;
        gNdsOpeningRoomRelocPointerFixupMask |= pointer_fixup_mask;
        gNdsOpeningRoomRelocContentReady =
            ((gNdsOpeningRoomRelocPayloadMask & NDS_RELOC_OPENING_ROOM_FILE_MASK) ==
             NDS_RELOC_OPENING_ROOM_FILE_MASK) ? 1u : 0u;
        if (((gNdsOpeningRoomRelocWordSwapMask & NDS_RELOC_OPENING_ROOM_FILE_MASK) ==
             NDS_RELOC_OPENING_ROOM_FILE_MASK) &&
            ((gNdsOpeningRoomRelocPointerFixupMask & NDS_RELOC_OPENING_ROOM_FILE_MASK) ==
             NDS_RELOC_OPENING_ROOM_FILE_MASK))
        {
            ndsRelocProbeOpeningRoomSymbols(files);
        }
        /* Full data fixup still requires mixed-width struct fixups and
         * renderer-specific texture/display-list fixups. Keep the existing
         * gate false until those contracts are implemented and verified. */
        gNdsOpeningRoomRelocFixupReady = 0;

        if ((len == NDS_RELOC_OPENING_ROOM_FILE_COUNT) &&
            ((gNdsOpeningRoomRelocFileMask & NDS_RELOC_OPENING_ROOM_FILE_MASK) ==
             NDS_RELOC_OPENING_ROOM_FILE_MASK))
        {
            gNdsOpeningRoomRelocResult = NDS_OPENING_ROOM_RELOC_PASS;
        }
    }
    if (gSCManagerSceneData.scene_curr == nSCKindOpeningMario)
    {
        if ((ndsRelocFindLoadedFileByAsset(
                 NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE) != NULL) &&
            (ndsRelocFindLoadedFileByAsset(
                 NDS_RELOC_ASSET_OPENING_COMMON) != NULL) &&
            (gNdsOpeningMarioSpriteNormalizeCount >= 5u) &&
            (gNdsOpeningMarioSpriteNormalizeFailCount == 0))
        {
            gNdsOpeningMarioRelocResult = NDS_OPENING_MARIO_RELOC_PASS;
        }
    }
    if ((gSCManagerSceneData.scene_curr == nSCKindPlayersVS) &&
        (len == 7u))
    {
        gNdsPlayersVSOriginalLoadedFileCount = len;
        gNdsPlayersVSOriginalRelocResult = NDS_PLAYERS_VS_ORIGINAL_RELOC_PASS;
    }
    if ((gSCManagerSceneData.scene_curr == nSCKindMaps) &&
        (len == 5u))
    {
        gNdsMapsOriginalLoadedFileCount = len;
        gNdsMapsOriginalRelocResult = NDS_MAPS_ORIGINAL_RELOC_PASS;
    }
    /* Seven outside Sudden Death: the announcer's letters load only for it
     * (battleship_scvsbattle.c ndsSCVSBattleLoadCommonFiles). */
    if ((gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
        ((len == 8u) || (len == 7u)))
    {
        gNdsSCVSBattleOriginalLoadedFileCount = len;
        gNdsSCVSBattleOriginalRelocResult =
            NDS_SCVSBATTLE_ORIGINAL_RELOC_PASS;
    }

    return (heap != NULL) ? (size_t)(heap_ptr - heap_start) : 0;
}

/* Byte span of an ALREADY-LOADED reloc file, or 0 if it is not resident.
 *
 * lbRelocGetFileSize cannot answer this and must not be used for it. It sizes
 * an allocation that has yet to be made, so ndsRelocExternTreeAllocSize
 * deliberately returns 0 once the asset has a status node -- a resident file
 * needs no further heap -- and lbRelocGetFileSize then falls back to
 * sizeof(Sprite). Asking it about a loaded file therefore reports 68 bytes for
 * a file that is resident and perfectly intact, which on 2026-08-01 read as
 * "EFCommonEffects1/2/3 are not in the ROM" when all three were loaded and
 * EFCommonEffects2 was 28,432 bytes on disk. Anything bounds-checking an
 * offset against a live file wants this function. */
/* A file's whole loaded image, whatever is resident: the bytes
 * lbRelocGetForceExternHeapFile writes into a caller's heap. A resident copy's
 * span (ndsRelocGetLoadedFileSize) can be a pack's or a cache's, smaller than
 * that image, and lbRelocGetFileSize answers sizeof(Sprite) for a resident
 * file. VS Results sized Ganondorf's podium heap by a resident span 8,080 B
 * and the 10,784-byte opening clip overran his DObjs (owner, 2026-10-11). */
size_t ndsRelocFullLoadSize(const void *file_id)
{
    u32 asset_id = ndsRelocAssetIDForToken(ndsRelocFileID(file_id));
    size_t bytes = ndsRelocAssetAllocSize(asset_id);
    size_t generated = ndsRelocP2GeneratedAllocSize(asset_id);

    if (generated > bytes)
    {
        bytes = generated;
    }
    return bytes;
}

size_t ndsRelocGetLoadedFileSize(const void *file_id)
{
    u32 token = ndsRelocFileID(file_id);
    u32 asset_id = ndsRelocAssetIDForToken(token);
    NDSRelocLoadedFile *loaded = ndsRelocFindLoadedFileByAsset(asset_id);

    return (loaded != NULL) ? (size_t)loaded->data_size : 0u;
}

/* The loaded-file table's change count (every add, replace, removal and
 * reset), for callers that keep an answer derived from it. */
u32 ndsRelocLoadedFilesEpoch(void)
{
    return sNdsRelocLoadedFilesEpoch;
}

void *ndsRelocGetFileData(void *file, const void *symbol)
{
    NDSRelocLoadedFile *loaded = ndsRelocFindLoadedFileByData(file);
    u32 offset;

    if (file == NULL)
    {
        gNdsOpeningRoomRelocSymbolResolveFailCount++;
        return NULL;
    }
    if (loaded == NULL)
    {
        return file;
    }
    if (ndsRelocResolveSymbolOffset(loaded, symbol, &offset) == FALSE)
    {
        gNdsOpeningRoomRelocSymbolResolveFailCount++;
        return NULL;
    }
    if ((loaded->reserved[0] == NDS_RELOC_IF_COMPACT_MARK) &&
        (ndsRelocIfCompactMap(offset, 1u, &offset) == FALSE))
    {
        gNdsOpeningRoomRelocSymbolResolveFailCount++;
        return NULL;
    }
#if (NDS_P2_1P_GAME || NDS_P2_MENU_SHELL) || NDS_P2_COMPACT_BATTLE_FIGHTERS
    if ((loaded->reserved[0] != 0u) &&
        (loaded->reserved[0] != NDS_RELOC_IF_COMPACT_MARK) &&
        (ndsPreviewFileOffset(loaded, offset, 1u, &offset) == FALSE))
    {
        gNdsOpeningRoomRelocSymbolResolveFailCount++;
        return NULL;
    }
#endif
    if (offset >= loaded->data_size)
    {
        gNdsOpeningRoomRelocSymbolResolveFailCount++;
        return NULL;
    }

    if (loaded->asset_id != NDS_RELOC_ASSET_N64_LOGO)
    {
        gNdsOpeningRoomRelocSymbolResolveCount++;
        gNdsOpeningRoomRelocLastSymbolOffset = offset;
    }
    if ((loaded->asset_id == NDS_RELOC_ASSET_GR_PUPUPU_MAP) &&
        (symbol == &llGRPupupuMapMapHeader))
    {
        MPGroundData *ground_data = (MPGroundData *)((u8 *)file + offset);

        gNdsStagePupupuRelocResult = NDS_STAGE_PUPUPU_RELOC_PASS;
        gNdsStagePupupuMapHeaderOffset = offset;
        gNdsStagePupupuGroundDataPtrReady = (ground_data != NULL) ? 1u : 0u;
        if ((ground_data != NULL) && (ground_data->map_geometry != NULL))
        {
            gNdsStagePupupuGeometryPtrReady = 1;
        }
        if ((ground_data != NULL) && (ground_data->wallpaper != NULL))
        {
            gNdsStagePupupuWallpaperPtrReady = 1;
        }
        if ((ground_data != NULL) && (ground_data->map_nodes != NULL))
        {
            gNdsStagePupupuMapNodesPtrReady = 1;
        }
        if (ground_data != NULL)
        {
            gNdsStagePupupuLightAngleXBits =
                ndsFloatBits(ground_data->light_angle.x);
            gNdsStagePupupuLightAngleYBits =
                ndsFloatBits(ground_data->light_angle.y);
            gNdsStagePupupuBGM = ground_data->bgm_id;
        }
    }
    if ((loaded->asset_id == NDS_RELOC_ASSET_GR_INISHIE_MAP) &&
        (symbol == &llGRInishieMapMapHeader))
    {
        MPGroundData *ground_data = (MPGroundData *)((u8 *)file + offset);

        gNdsStageInishieMapHeaderOffset = offset;
        gNdsStageInishieGroundDataPtrReady = (ground_data != NULL) ? 1u : 0u;
        if ((ground_data != NULL) && (ground_data->map_geometry != NULL))
        {
            gNdsStageInishieGeometryPtrReady = 1u;
        }
        if ((ground_data != NULL) && (ground_data->map_nodes != NULL))
        {
            gNdsStageInishieMapNodesPtrReady = 1u;
        }
        if ((gNdsStageInishieGroundDataPtrReady != 0u) &&
            (gNdsStageInishieGeometryPtrReady != 0u))
        {
            gNdsStageInishieRelocResult = NDS_STAGE_INISHIE_RELOC_PASS;
        }
    }
    if ((loaded->asset_id == NDS_RELOC_ASSET_STAGE_DREAM_LAND) &&
        (symbol == &llStageDreamLandSprite))
    {
        gNdsStagePupupuWallpaperPtrReady = 1;
    }
    return (u8 *)file + offset;
}

/* KEEP THIS IDENTICAL TO THE DECLARATION GUARD in nds_preview_pack.h.
 * Two callers arrive from outside the roster flags: the VS CSS compact
 * preview transaction (a RENDERER condition) and wpManagerMakeWeapon's
 * Yoshi egg root mapping under NDS_P2_YOSHI. The second is what stopped
 * NDS_P2_KIRBY_COPYLINK_PROOF -- the only scripted Kirby inhale in the
 * tree -- from linking. Spelled out rather than hidden behind a macro:
 * this file does not include that header here, and an undefined macro in
 * an #if is silently 0, which is a quieter version of the same bug. */
#include "reloc_preview_pack.c"
