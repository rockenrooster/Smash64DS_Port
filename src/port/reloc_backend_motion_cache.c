/* Relocation asset loading, the motion cache (R2-04): the elastic cache and its
 * arena, the resident figatree pack, the ring, the prefetch and the learned
 * successors, and the preload barrier. A slice of reloc_backend.c. */

#if NDS_TICK_HUD
/* R2-04 E0. Sizes the animation-cache opportunity: see the block inside
 * ndsRelocForceLoadFighterAObj16File for why AnimForceResident answering 0 does
 * not settle it. */
#define NDS_R204_ANIM_ID_SPAN \
    ((NDS_RELOC_ASSET_FOX_ANIM_LAST - NDS_RELOC_ASSET_MARIO_ANIM_WAIT) + 1u)
volatile u32 gNdsR204AnimForceLoadTotal;
volatile u32 gNdsR204AnimForceLoadDistinct;
volatile u32 gNdsR204AnimForceLoadRepeat;
/* Non-static so the sampler can read the set itself, not just its size: the
 * warm list R2-04 needs is the ID set, and a count cannot be turned back into
 * one. 301 bits, 10 words. */
volatile u32 gNdsR204AnimSeen[(NDS_R204_ANIM_ID_SPAN + 31u) / 32u];
#endif

/* R2-04 E1. Task 75's absorption: keep each fighter animation's payload resident
 * so the frame that needs a move does not re-walk NitroFS and re-read the
 * cartridge for it.
 *
 * Keyed by asset_id, not by destination heap. E0 measured the destination-keyed
 * check (AnimForceResident) at 0 across two windows because the destination is
 * caller-owned and reused, while 53 of 82 force-loads (64.6%) repeat an asset
 * already loaded once. The match's working set is 29 distinct animations, so
 * NDS_R2_ANIM_CACHE_ENTRIES only has to cover that, not the 301-ID space --
 * which could not fit anyway against NDS_RELOC_LOADED_FILE_CAPACITY of 96.
 *
 * What is stored is the BYTE-SWAPPED, PRE-FIXUP image.
 * ndsRelocApplyInternalPointerFixups writes absolute pointers derived from
 * loaded->data, so a fixed-up image is position-dependent and must never be
 * replayed into a different heap. A hit copies the swapped bytes and re-runs the
 * fixups against the real destination, which is correct by construction and
 * still removes the directory walk and the cartridge read -- the part Task 71
 * profiled inside the frame.
 *
 * Every failure path degrades to the uncached load, so a full cache, a failed
 * allocation or an unexpected size is a performance outcome and never a
 * correctness one. */
/* 128, not 64. Cycle 105 read gNdsR204AnimForceLoadDistinct on the BOTH-CPU gate
 * arm over a whole 60-second match: the working set is 85 distinct animations,
 * not the 29 the E1 comment above was written against. 64 is below that, and the
 * entry count is a HARD refusal in both store paths -- ndsR2AnimWarmLoadOne
 * counts it as a warm failure and the miss path never fills -- so at 64 the last
 * 21 animations of the match could not be cached whatever the arena had left. */
/* 256 since the elastic cache (2026-09-28): with the compact ground maps it
 * reaches its byte cap on most rosters, and at ~2.1 KB a clip 128 entries
 * would refuse stores long before the bytes ran out. */
#define NDS_R2_ANIM_CACHE_ENTRIES 256u

/* THIS CACHE MUST NEVER CALL syTaskmanMalloc, AND THAT IS NOT A STYLE RULE.
 *
 * 2026-07-29: the owner's "lots of freeze bugs that seem random" was this. A soak
 * caught it 3.5 minutes into a two-CPU match with the backtrace
 *
 *   syMallocSet (gSYTaskmanGeneralHeap, 3472, 16)  decomp/src/sys/malloc.c:30
 *     <- syTaskmanMalloc <- ndsR2AnimCacheStore <- ndsRelocForceLoadFighterAObj16File
 *     <- lbRelocGetForceExternHeapFile (llFTMarioAnimAttackAirDFileID)
 *     <- ftMainSetStatus (status_id=213) <- ftCommonAttackAirCheckInterruptCommon
 *
 * and malloc.c:30 is `while (TRUE);`. The source allocator does NOT return NULL
 * when its region overflows -- it spins forever. So the `payload == NULL` reject
 * paths below, which were written to make a full cache a performance outcome
 * rather than a correctness one, were DEAD CODE: the allocation could not fail,
 * it could only hang the console.
 *
 * Two properties of gSYTaskmanGeneralHeap make that inevitable rather than
 * unlucky. It is a bump region (SYMallocRegion: start/ptr/end) with no free, and
 * unlike the graphics heap nothing ever calls syMallocReset on it -- so every
 * byte this cache took was gone for the rest of the run, and the cache has no
 * eviction and bounds its ENTRY COUNT while leaving its BYTES unbounded. It also
 * meant the cached payload pointers would have dangled through any future reset.
 *
 * The fix is to stop borrowing the game's heap. A speculative cache gets its own
 * declared arena, and overflowing it returns NULL through the existing reject
 * path, which is what that path was always for.
 *
 * SIZE IS BOUNDED BY THE TASKMAN ARENA SEARCH, NOT BY THE WORKING SET. This was
 * first written at 128 KiB, sized from the 41 warm assets = 91,104 bytes above,
 * with the note that "a link failure on BSS is a loud, safe failure". **That was
 * wrong, and it shipped a ROM that could not start a battle.** BSS here does not
 * overflow a link; it silently competes with the runtime `calloc` that sizes
 * gSYTaskmanGeneralHeap. src/port/diagnostics.c:7403 searches downward from
 * 0x150000 in 0x1000 steps but FLOORS AT 0x130000, then drops to a coarse list
 * beginning at 0x100000 -- so crossing that floor costs 196,608 bytes in one
 * step, not the 0x1000 the loop implies.
 *
 * Measured, shipped hwtri ROM: without this arena the fine search secured
 * 1,286,144 bytes. With 128 KiB of it, all 33 fine steps failed
 * (TASKARENA=1048576,33) and battle start died in the exact way this comment was
 * written to prevent -- ftManagerSetupFilesAllKind(fkind=1) asking 116,752
 * against 58,024 free. The arena meant to stop a heap-exhaustion hang caused a
 * bigger one, two levels down, at a seam it never mentions.
 *
 * The static-BSS attempt was tried twice and failed twice, and BOTH failures were
 * the same misunderstanding: this buffer is not free, and its budget belongs to
 * the tightest configuration rather than the shipped one.
 *
 *   - 128 KiB: shipped arena 1,286,144 -> 1,048,576 (all 33 fine steps failed),
 *     battle start dead, 116,752 asked against 58,024 free. 131,072 was
 *     affordable in ISOLATION -- 1,286,144 - 131,072 still clears the 1,107,392
 *     battle start needs. The cliff, not the size, was fatal.
 *   - 32 KiB: failed by THIRTY-TWO BYTES. Sized against the shipped build's
 *     40,960 of headroom; the tick-HUD target starts from 1,277,952 and has only
 *     32,768, and arena plus counters is 32,800. Measured BSS also cleared the
 *     flag that had been blamed -- 1,742,216 against 1,709,416 is this arena and
 *     nothing else, so NDS_R2_BOTH_CPU contributes none.
 *   - 16 KiB fit, and then MEASURED THE COST OF FITTING: WORK-H P95 1,096,768 ->
 *     1,204,352, +107,584, gate missed by 84,352, with Fills=2 against
 *     Rejects=44 and 76 overflows in 128 frames. A cache that small is not a
 *     cache, and a miss is not free: ndsRelocAssetLoadHeaderAndData does a real
 *     fopen/fread/fclose through NitroFS, which on the owner's DLDI setup is SD
 *     I/O inside a gameplay frame. P50 moved only +14,080 while P95 moved
 *     +107,584 -- a tail-shaped regression, which is exactly what a handful of
 *     frames doing file reads looks like.
 *
 * So the arena comes from gSYTaskmanGeneralHeap instead, which is where a buffer
 * sized by measured game data belongs. That costs the boot-time search NOTHING,
 * so it cannot cross the 0x130000 cliff or trip the Task 36 replay admission
 * guard (include/nds/nds_renderer.h:124-134) that shares that constant -- and
 * 92,160 is affordable there in a way it never was in BSS. Reserved lazily on
 * first store, so ftManagerSetupFilesAllKind has already taken the fighters'
 * 116,752 bytes and this can never be the allocation that starves battle start.
 *
 * Sized to the R2-04 E5 working set below: 41 warm assets = 91,104 bytes, rounded
 * to 92,160. Overflow still degrades to the uncached load, and the SAFETY of this
 * fix does not depend on the size at all -- even a zero-byte arena would be safe,
 * because the reject path is live. Only the tick cost depends on it.
 *
 * Two general lessons, in TASK_STANDING_RULES: never call an allocator that
 * cannot fail from a code path that is allowed to fail, and never fix a heap
 * problem with a static buffer without pricing it against the heap.
 *
 * CYCLE 105 RE-SIZED IT, AND THE OLD VALUE WAS THE GATE ARM'S TAIL. 92,160 was
 * sized against a 41-asset list measured on the Boundary arm. Read on the
 * both-CPU gate arm over a whole match, that arena is 100% FULL and has been
 * refusing loads all match:
 *
 *   ArenaReservedBytes 92,160   ArenaUsedBytes 92,160   Overflows 142
 *   Rejects 142   OverflowLastSize 912   OverflowLastUsed 92,160
 *   ForceLoadTotal 353 = Hits 206 + Misses 147   ForceLoadDistinct 85
 *
 * A refused asset is not merely uncached: every later use of it re-runs
 * ndsRelocAssetLoadHeaderAndData, which is a real fopen/fread/fclose through
 * NitroFS -- on DLDI, SD I/O inside a gameplay frame. Frames 478 -> 2007 did
 * gNdsRelocAssetPayloadReadCount +111 against 113 frames whose SINT bucket
 * exceeded 400,000 ticks, and the median SINT is 169,248. That is the whole-match
 * P95 tail: capping SINT at its own median takes WORK-H P95 1,638,582 ->
 * 1,333,093. The reads are also why AUD and HUD -- buckets whose WORK cannot vary
 * with the match -- read 4.9x and 2.5x on those frames: the cartridge stalls
 * everything, not just the caller.
 *
 * Sized to the MEASURED 85-asset working set below: aligned data totals 197,184
 * bytes, so 200,704 covers it with 3,520 spare. The budget it spends from is
 * gSYTaskmanGeneralHeap, whose low-water free read 154,776 with the old 92,160
 * already taken -- i.e. a pool of ~246,936 -- so the reserve's request plus
 * NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE (233,472) still fits, and the run's own
 * gNdsTaskmanGeneralHeapFreeMin is the check on that arithmetic rather than this
 * comment. Overflow still degrades to the uncached load. */
/* RESIZED 2026-08-11 from 200,704 to 262,144 (docs/RAM_RECOVERY_PLAN.md Phase
 * 6). The 200,704 above was correct for the 85-asset set it was measured
 * against, but the match outgrew it: the arena ran 200,384/200,704 = 99.85% full
 * and refused live assets, and because this is a bump allocator with no
 * eviction every refusal re-read that asset from the ROM for the rest of the
 * match. That streaming was the largest single owner of the P95 tail.
 *
 * The new size is measured, not estimated. gNdsR2AnimCacheRejectedUniqueBytes
 * (added for this) counts the DISTINCT assets refused, because the reject COUNT
 * cannot size anything -- a refused asset is re-requested every time it is
 * needed, so Boundary's 15 rejects were only 9 distinct assets:
 *
 *   used high-water            200,384   (bump allocator: used IS the high-water)
 *   + unique refused, aligned   18,336   (9 assets, track overflow 0)
 *   = ANIM_REQUIRED_BYTES      218,720
 *   chosen                     262,144   -> 43,424 spare (19.9%)
 *
 * The old campaign estimate was ~82 KiB; the real shortfall is 18,016, so that
 * figure was 4.5x too large and sizing from it would have wasted ~64 KiB.
 *
 * Affordable because Phases 1-2 recovered 169,152 bytes of static main RAM,
 * which the taskman probe converted into arena: gNdsTaskmanGeneralHeapFreeMin
 * went 15,120 -> 133,628, i.e. from 17,648 BELOW the 32,768 safety floor to
 * 100,860 above it. Spending 61,440 of that leaves the floor with ~39 KiB of
 * margin, and the run's own gNdsTaskmanGeneralHeapFreeMin is the check on this
 * arithmetic rather than this comment. Overflow still degrades to the uncached
 * load. */
/* SLICE 1 PHASE 5, 2026-08-15. When the resident figatree pack is on, the arena
 * stops being a raw-file cache and becomes the pack's home. The two cannot both
 * exist: the arena-internal budget is
 *
 *     262,144   this reservation, reclaimed outright by residency
 *   +  39,420   gSYTaskmanGeneralHeap free-min 72,188 less the 32,768 floor
 *   ---------
 *     301,564   (BATTLEPACK_POOL.md section 1)
 *
 * and one fighter's blob is 287,904 (Fox) or 271,728 (Mario). 290,816 leaves the
 * Fox blob 2,912 bytes of bump slack and projects a general-heap low-water of
 * 43,516 against the mandated 32,768 floor -- 10,748 of margin. Both fighters
 * would need ~559,632 and do not fit, which is why this slice packs one and the
 * other keeps the generic path.
 *
 * CONSEQUENCE, STATED BECAUSE IT IS NOT FREE: the un-packed fighter loses this
 * cache. Its acquisitions were 95% hits and become ROM loads. That is a
 * PERFORMANCE outcome and never a correctness one -- every failure path here
 * degrades to the on-demand load -- but it is real, it is unmeasured on the gate
 * arm, and the remedy is to pack both fighters, which needs the taskman arena
 * itself grown out of the 146,560 B that RAM_RECOVERY_PLAN Phase 2 recovered. */
/* The ARM9 data cache line is 32 bytes and the FAT/DLDI read path may maintain
 * the destination by line, so the blob is placed on a line boundary and the
 * reserve rounded up to one. */
#define NDS_BATTLEPACK_LINE_BYTES 32u
#define NDS_BATTLEPACK_RESERVE_BYTES \
    ((NDS_R2_BATTLEPACK_BLOB_BYTES + (2u * NDS_BATTLEPACK_LINE_BYTES) - 1u) & \
     ~(NDS_BATTLEPACK_LINE_BYTES - 1u))
/* 4,096 of raw cache behind the pack. It is nearly nothing, and that is the
 * honest state of the one-fighter fit: the budget above is 301,564 and the Fox
 * blob is 287,904 of it. Projected general-heap low-water 42,236 against the
 * 32,768 floor.
 *
 * MEASURED 2026-08-15 (BATTLEPACK_GATE.md): that 4,096 is not "nearly nothing",
 * it is the whole verdict. The carve did not shrink the raw cache, it DELETED it
 * -- Fills 17 -> 2, Rejects 0 -> 126, hits 338 -> 30 -- and the 111 net-new
 * uncached acquisitions cost 3,873,969 ticks each, which is the entire
 * +2,261,376 at rank-80. So this pair cannot price the deletion's BENEFIT: the
 * starvation swamps it. NDS_R2_BATTLEPACK_KEEP_CACHE builds the arm that can. */
/* 163,840 of raw cache behind the pack, NOT the control's 262,144, and the
 * reason is the heap rather than the design. Blob + full cache = 550,080 needs
 * an arena of 0x18f000, and the measured grantable ceiling is 1,564,672 (see
 * NDS_TASKMAN_ARENA_SIZE): that arm starved the general heap to 6,076 B and the
 * battle never started. 287,936 + 163,840 = 451,776 against a 0x17a000 arena
 * projects a general-heap low-water of ~54,588, i.e. 21,820 clear of the floor.
 *
 * THE ACCEPTANCE TEST IS gNdsR2AnimCacheRejects == 0, not the byte count. A
 * smaller cache is equivalent to the control's exactly when it still refuses
 * nothing, and with Fox served from the pack this holds only Mario's working set
 * -- against a 192,240 B both-fighter arena high-water (slice 46). If Rejects is
 * non-zero the arm did not isolate the deletion and its ticks price starvation
 * again, which is the whole thing this configuration exists to avoid.
 *
 * THAT TEST PASSES ON THE STRESS BATTERY, not just on one match (2026-08-15,
 * artifacts/performance/2026-08-15_battlepack-arena-price/ARENA_PRICE.md).
 * Over 12 battle-scene entries, 7 completed matches and 4 Sudden Deaths:
 * Rejects 0, Overflows 0, ReserveFailCount 0, and the arena bump high-water
 * 425,072 of the 451,776 reserved -- i.e. Mario's half used 137,136 of its
 * 163,840, 83.7%, with 26,704 spare. That spare is the real margin on this
 * constant; 163,840 is not a round number to be trimmed casually. */
#define NDS_R2_ANIM_CACHE_PACK_RAW_BYTES 163840u
/* A declined BattlePack is not "a BattlePack with zero blob bytes". It is the
 * ordinary raw-cache configuration. P2-3f9 made the blob carve roster-aware,
 * but left the raw-cache size compile-time-selected by NDS_R2_BATTLEPACK. On a
 * four-distinct-kind match that correctly declined Fox's one-kind pack while
 * accidentally keeping only the 163,840-byte behind-pack cache. The 2026-08-30
 * four-CPU run consequently recorded 520 misses / 516 rejects and streamed
 * fighter animations from NitroFS in live frames.
 *
 * Restore the qualified standalone reservation whenever the runtime declines
 * the pack. Source animation ownership and load/fixup semantics are unchanged;
 * this only restores the raw cache that a no-pack route owns. The pack-present
 * route keeps its independently measured smaller cache.
 *
 * BUGS.md four-CPU close, 2026-09-01: source-precision fighter matrices added
 * 4,160 B to the static image even after their working storage was overlaid on
 * the existing hierarchy workspace. That moved this exact four-kind match to a
 * 25,088 B general-heap low-water, 512 B under the 25,600 B GObj safety latch.
 * Its whole-match raw cursor reached 57,888 B with zero rejects in this
 * 262,144 B arena. Return one 4 KiB page from THIS pack-declined cache only;
 * the two-kind pack-present configuration and its 163,840 B qualified margin
 * do not move. Raw-cache exhaustion already recycles to the on-demand source
 * loader, so this is a capacity/performance adaptation, never missing state. */
#define NDS_R2_ANIM_CACHE_STANDALONE_RAW_BYTES 258048u
/* THE CEILING, NOT THE RESERVATION (P2-3f9). The largest possible reservation
 * is still pack + its behind-pack cache; the standalone raw route is smaller.
 * `gNdsR2AnimCacheArenaReservedBytes` reports the roster-selected real one. */
#define NDS_R2_ANIM_CACHE_ARENA_BYTES \
    (NDS_BATTLEPACK_RESERVE_BYTES + NDS_R2_ANIM_CACHE_PACK_RAW_BYTES)

/* R2-04 E4. The match's animation working set, measured rather than guessed:
 * gNdsR204AnimSeen dumped at frame 1928 (roughly two thirds through the 3,600
 * tick match) after 230 force-loads, of which 189 (82.2%) were repeats of these
 * 41 assets. 14 Mario, 27 Fox, 91,104 bytes -- 12.5% of the 728,064 the full
 * 301-ID space would need, which E3 showed does not fit the arena.
 *
 * An asset missing from this list is a performance outcome, never a correctness
 * one: it simply takes the on-demand path it takes today. The list is derived
 * from observed play, so gNdsR204AnimForceLoadRepeat/Total is its regression
 * check -- if that ratio falls, the list has drifted from what the match uses.
 *
 * CYCLE 105 REPLACED THE LIST, BECAUSE IT WAS MEASURED ON THE WRONG ARM. The 41
 * above came off a Boundary match (Mario human vs one level-3 Fox). The gate arm
 * is NDS_R2_BOTH_CPU, where Mario is also a level-3 CPU, and two CPUs playing
 * their whole movesets touch a substantially wider set: gNdsR204AnimSeen dumped
 * at presented frame 2038 of the 1,600-sample window holds 85 IDs, and the old
 * list covered only 27 of them. The 14 it warmed that this match never asks for
 * were spending arena on assets nobody used.
 *
 * That is why the list moved rather than merely growing. The union of the two
 * lists is 99 assets = 229,024 bytes, which does NOT fit the heap budget (see
 * NDS_R2_ANIM_CACHE_ARENA_BYTES); the 85 measured here do, at 197,184. A Boundary
 * asset dropped from this list is a performance outcome and never a correctness
 * one -- it takes the on-demand path, and with the arena no longer full it is
 * then cached on that first use, which the old arena could not do.
 *
 * gNdsR204AnimForceLoadRepeat/Total remains the regression check on the list.
 *
 * SLICE 46 REPLACED IT AGAIN, FOR THE SAME REASON ONE LAYER DOWN. Cycle 105
 * fixed the ARM the list was measured on; it did not fix the fact that a list
 * measured once drifts. Dumped from the c123 gate ROM, `gNdsR204AnimSeen` holds
 * **87** ids and the 85-entry list above overlapped only **57** of them:
 *
 *   used AND warmed   57      warmed but NEVER used   28   (arena spent on nothing)
 *   used but NOT warm 30      <- these streamed from ROM inside the match
 *
 * The 30 are the misses. `gNdsR2AnimCacheMisses` reads 32 on the same run, and
 * they are not spread: the miss-only symbols `strncasecmp` and
 * `ndsRelocApplyWordByteSwap` sit on 22 of the 80 costliest frames, and capping
 * `SINT` on the 32 highest-`SINT` frames is worth **-32,512 WORK-H P95**.
 *
 * This SHRINKS the arena rather than growing it, which is why it needs no RAM
 * budget at all: the old list cached 83 warm entries and then filled 32 more on
 * demand -- 115 residents, 257,200 bytes, 98.1% of the arena -- where the
 * measured set is 87 at roughly 194,500. The 28 never-used warms were paying
 * for themselves twice, in arena bytes and in the countdown steps they consumed
 * while the assets the match actually wanted went unwarmed.
 *
 * An asset missing from this list is still a performance outcome and never a
 * correctness one, so tailoring it to the measured match is safe in the way a
 * gameplay change would not be: a miss simply takes the on-demand path.
 * `gNdsR204AnimForceLoadRepeat/Total` remains the drift check -- re-dump
 * `gNdsR204AnimSeen` and re-diff rather than assuming this list is still true.
 *
 * OWNER BUG CLOSURE 2026-08-16: that warning fired. After GX compose became the
 * shipping default, a fresh HUMAN-MARIO Boundary-style match ended with 46
 * distinct force-loaded assets and 12 IDs absent from the 87-ID gate list:
 * 20b/20f/217/219/221/222/23c/23d, 2c0, 2fe, 30d/30e. The old stepped loader
 * consequently took 14 misses and six payload reads after frame 535. Add that
 * natural-play set here, plus the complete common Catch/CatchPull/ThrowF/ThrowB
 * quartet for both Mario (231..234) and Fox (2c0..2c3). The Mario quartet is
 * deliberate even when a deterministic CPU seed does not choose it: the owner
 * reported the visible failure while manually doing Mario's back throw, and
 * those are exactly dFTMarioMotionDescs[146..149]. The barrier below moves this
 * whole union before BGM/countdown; arena usage is verified by the retained
 * runtime counters rather than assumed from this comment.
 */
static const u16 sNdsR204AnimWarmList[] = {
    0x1f3u, 0x1f4u, 0x1f5u, 0x1fbu, 0x1fdu, 0x1feu, 0x1ffu, 0x200u,
    0x201u, 0x202u, 0x203u, 0x204u, 0x205u, 0x206u, 0x207u, 0x209u,
    0x20bu, 0x20eu, 0x20fu, 0x212u, 0x216u, 0x217u, 0x218u, 0x219u,
    0x21au, 0x21cu, 0x21du, 0x21fu, 0x220u, 0x221u, 0x222u, 0x223u,
    0x228u, 0x229u, 0x22au, 0x22du, 0x22eu, 0x22fu, 0x230u, 0x231u,
    0x232u, 0x233u, 0x234u, 0x23cu, 0x23du, 0x26cu, 0x26du, 0x270u,
    0x271u, 0x272u, 0x27bu, 0x27cu, 0x27du, 0x27eu, 0x27fu, 0x280u,
    0x282u, 0x284u, 0x285u, 0x28au, 0x28cu, 0x28du, 0x28eu, 0x28fu,
    0x290u, 0x291u, 0x292u, 0x293u, 0x294u, 0x295u, 0x296u, 0x298u,
    0x2a0u, 0x2a7u, 0x2a8u, 0x2a9u, 0x2abu, 0x2acu, 0x2afu, 0x2b5u,
    0x2b9u, 0x2bcu, 0x2bdu, 0x2bfu, 0x2c0u, 0x2c1u, 0x2c2u, 0x2c3u,
    0x2f7u, 0x2fau, 0x2fbu, 0x2fcu, 0x2feu, 0x302u, 0x30bu, 0x30cu,
    0x30du, 0x30eu, 0x311u, 0x312u, 0x313u, 0x314u, 0x316u, 0x317u,
    0x318u, 0x319u
};

typedef struct NDSR2AnimCacheEntry {
    u32 asset_id;
    u32 size;
    void *payload;
    NDSRelocAssetHeader header;
    /* R2-06 E13. TRUE when this payload's script region already carries the
     * ndsRelocNormalizeFighterAObj16File transform, so the load path can set
     * format_fixups_applied and skip it. Zero for anything the prebake declined,
     * which is why the load path reads the flag rather than assuming. */
    u16 aobj16_ready;
    /* Fetched since the ring's cursor last passed it (see
     * ndsR2AnimCacheRawRingAllocAligned). */
    u8 referenced;
    /* Read by the prefetch and not fetched yet (counters only). */
    u8 prefetched;
} NDSR2AnimCacheEntry;

#define NDS_R2_ANIM_CACHE_READY_RAW 0u
#define NDS_R2_ANIM_CACHE_READY_PREBAKE 1u
#define NDS_R2_ANIM_CACHE_READY_STREAM 2u

static sb32 ndsR2AnimPrebakeAObj16(u32 asset_id, void *payload, u32 size,
                                   const NDSRelocAssetHeader *header);

static NDSR2AnimCacheEntry sNdsR2AnimCache[NDS_R2_ANIM_CACHE_ENTRIES];
/* The entries' asset-id index (ndsR2AnimCacheFind). */
#define NDS_R2_ANIM_CACHE_INDEX_SLOTS 512u
#define NDS_R2_ANIM_CACHE_INDEX_NONE 0xffffffffu
_Static_assert(NDS_R2_ANIM_CACHE_ENTRIES * 2u <= NDS_R2_ANIM_CACHE_INDEX_SLOTS,
               "anim cache index load factor");
static u16 sNdsR2AnimCacheIndex[NDS_R2_ANIM_CACHE_INDEX_SLOTS];
static u32 sNdsR2AnimCacheCount;
/* Each entry's ring extent and CLOCK mark, apart from the 32-byte entries
 * (P2-2p8, 2026-10-05). A ring allocation scans every entry up to nine times
 * (eight CLOCK steps and the eviction) for byte ranges overlapping its slot;
 * over the entries that was one line fill per entry per scan -- ~150 entries
 * missed nine times, ~40K cycles in a frame that fetches a clip, 7.5% of the
 * gate's frames and most of its heaviest. Here the scans read 5 bytes an entry
 * that stay in the data cache across them. Extents are 16-byte units from
 * sNdsR2AnimCacheExtentOrigin (the block's base when reserved, which the
 * elastic yield only moves forward), Hi rounded up: every payload and every
 * ring slot starts 16-aligned, so the unit test is the byte test. An entry
 * outside the units' range reads as never overlapping (and is scanned as
 * the entries were before). */
static u16 sNdsR2AnimCacheLo[NDS_R2_ANIM_CACHE_ENTRIES];
static u16 sNdsR2AnimCacheHi[NDS_R2_ANIM_CACHE_ENTRIES];
static u8 sNdsR2AnimCacheRef[NDS_R2_ANIM_CACHE_ENTRIES];
static uintptr_t sNdsR2AnimCacheExtentOrigin;
#define NDS_R2_ANIM_EXTENT_NONE 0xffffu

static inline u32 ndsR2AnimExtentUnitFloor(const void *p)
{
    const uintptr_t a = (uintptr_t)p;

    if ((a < sNdsR2AnimCacheExtentOrigin) ||
        (((a - sNdsR2AnimCacheExtentOrigin) >> 4) >= NDS_R2_ANIM_EXTENT_NONE))
    {
        return NDS_R2_ANIM_EXTENT_NONE;
    }
    return (u32)((a - sNdsR2AnimCacheExtentOrigin) >> 4);
}

static inline u32 ndsR2AnimExtentUnitCeil(const void *p)
{
    const uintptr_t a = (uintptr_t)p;

    if ((a < sNdsR2AnimCacheExtentOrigin) ||
        (((a - sNdsR2AnimCacheExtentOrigin + 15u) >> 4) >=
         NDS_R2_ANIM_EXTENT_NONE))
    {
        return NDS_R2_ANIM_EXTENT_NONE;
    }
    return (u32)((a - sNdsR2AnimCacheExtentOrigin + 15u) >> 4);
}

/* Entry `i`'s extent from its payload and size, mark cleared. */
static void ndsR2AnimCacheSetExtent(u32 i)
{
    const u8 *s = (const u8 *)sNdsR2AnimCache[i].payload;
    const u32 lo = ndsR2AnimExtentUnitFloor(s);
    const u32 hi = ndsR2AnimExtentUnitCeil(s + sNdsR2AnimCache[i].size);

    if ((lo == NDS_R2_ANIM_EXTENT_NONE) || (hi == NDS_R2_ANIM_EXTENT_NONE))
    {
        sNdsR2AnimCacheLo[i] = NDS_R2_ANIM_EXTENT_NONE;
        sNdsR2AnimCacheHi[i] = NDS_R2_ANIM_EXTENT_NONE;
    }
    else
    {
        sNdsR2AnimCacheLo[i] = (u16)lo;
        sNdsR2AnimCacheHi[i] = (u16)hi;
    }
    sNdsR2AnimCacheRef[i] = 0u;
}
/* Reserved from gSYTaskmanGeneralHeap, NOT static BSS -- see the header comment on
 * NDS_R2_ANIM_CACHE_ARENA_BYTES for why BSS cannot afford this. */
static u8 *sNdsR2AnimCacheArena;
static u32 sNdsR2AnimCacheArenaBytes;
static u32 sNdsR2AnimCacheArenaUsed;
/* The motion prefetch (after ndsR2AnimDirectReadEntry): an ARM7 read may be
 * landing in the ring, and nothing may take those bytes until it has. */
void ndsR2AnimPrefetchDrain(void);
static void ndsR2AnimPrefetchRetire(const u8 *lo, const u8 *hi);
/* Prefetched clips dropped before any fetch used them, and first uses. */
__attribute__((used)) volatile u32 gNdsR2AnimPrefetchWasted;
__attribute__((used)) volatile u32 gNdsR2AnimPrefetchUsed;
/* CSS/setup callers only need a handful of imminent fighter clips resident.
 * Reserving the battle arena there would require ~452 KiB plus KEEP_FREE and
 * correctly declines in the menu scene.  A raw-only arena is intentionally
 * small and never owns BattlePack storage; the next scene generation drops it
 * before battle reserves the normal full arena. */
static sb32 sNdsR2AnimCacheArenaRawOnly;
/* The taskman-heap generation this block was reserved under. Ownership is this
 * value matching gNdsTaskmanHeapGeneration; see ndsR2AnimCacheArenaStillOwned. */
static u32 sNdsR2AnimCacheArenaGeneration;
/* The block is the general heap's elastic top (see ndsR2AnimCacheArena-
 * CarveElastic), not an allocation. */
static sb32 sNdsR2AnimCacheArenaElastic;
volatile u32 gNdsR2AnimCacheHits;
volatile u32 gNdsR2AnimCacheMisses;
volatile u32 gNdsR2AnimCacheFills;
volatile u32 gNdsR2AnimCacheBytes;
volatile u32 gNdsR2AnimCacheRejects;
/* The two ways ownership can fail, kept apart on purpose. A generation mismatch
 * is the ORDINARY second-entry case and should read non-zero on any run that
 * reaches Sudden Death or a rematch -- a zero there means the contract never
 * engaged and the run proves nothing. A range fault is not ordinary: it means
 * the block left the live region without a scene rewind. */
volatile u32 gNdsR2AnimCacheArenaGenerationMismatches;
volatile u32 gNdsR2AnimCacheArenaRangeFaults;
/* Engagement proof. An arena that never fills and an arena that overflows every
 * frame are indistinguishable from the reject count alone, and this campaign has
 * shipped a flag that silently never fired. */
volatile u32 gNdsR2AnimCacheArenaUsedBytes;
volatile u32 gNdsR2AnimCacheArenaOverflows;
volatile u32 gNdsR2AnimCacheArenaReservedBytes;
volatile u32 gNdsR2AnimCacheArenaReserveCount;
volatile u32 gNdsR2AnimCacheArenaReserveFailCount;
volatile u32 gNdsR2AnimCacheArenaInvalidations;
volatile u32 gNdsR2AnimCacheRawRecycles;
/* Stores of an asset this match already stored once: each is a clip the cache
 * held and gave up, then read from storage again. Cleared by
 * ndsR2AnimCachePreloadMatch. */
#define NDS_R2_ANIM_STORED_BITS 72u
static u32 sNdsR2AnimStoredThisMatch[NDS_R2_ANIM_STORED_BITS];
__attribute__((used)) volatile u32 gNdsR2AnimCacheRestores;
#if NDS_TICK_HUD
/* Tick-HUD census: one bit per fighter clip fetched this match (hit or read),
 * cleared with the stores above. */
__attribute__((used)) volatile u32 gNdsAnimUseBits[NDS_R2_ANIM_STORED_BITS];
#endif
/* ANIM_REQUIRED_BYTES, measured. The plan (docs/RAM_RECOVERY_PLAN.md 0.3/6.1)
 * forbids sizing the arena from the old ~82 KiB estimate, and the reject COUNT
 * cannot size it either: a refused asset is re-requested every time it is
 * needed, so 15 rejects is 15 attempts over an unknown number of distinct
 * assets. Summing every refusal would therefore overcount badly.
 *
 * These track the DISTINCT assets that were refused and their bytes. The arena
 * is a bump allocator that never frees, so gNdsR2AnimCacheArenaUsedBytes is
 * already its high-water; the requirement is
 *     UsedBytes + RejectedUniqueBytes + per-entry 16-byte alignment slack.
 * 64 ids is far above the 15 refusals seen on Boundary and the 38 on the
 * both-CPU arm; Overflow is the tell if that assumption ever breaks. */
#define NDS_R2_ANIM_REJECT_TRACK_MAX 64u
static u32 sNdsR2AnimRejectedIds[NDS_R2_ANIM_REJECT_TRACK_MAX];
static u32 sNdsR2AnimRejectedCount;
volatile u32 gNdsR2AnimCacheRejectedUniqueBytes;
volatile u32 gNdsR2AnimCacheRejectedUniqueCount;
volatile u32 gNdsR2AnimCacheRejectedTrackOverflow;

/* Record a refusal once per distinct asset. Linear scan: the set is tiny and
 * this only runs on the failure path, which is by definition rare. */
static void ndsR2AnimNoteRejected(u32 asset_id, u32 size)
{
    u32 i;

    for (i = 0u; i < sNdsR2AnimRejectedCount; i++)
    {
        if (sNdsR2AnimRejectedIds[i] == asset_id)
        {
            return;
        }
    }
    if (sNdsR2AnimRejectedCount >= NDS_R2_ANIM_REJECT_TRACK_MAX)
    {
        NDS_DIAG(gNdsR2AnimCacheRejectedTrackOverflow++);
        return;
    }
    sNdsR2AnimRejectedIds[sNdsR2AnimRejectedCount++] = asset_id;
    gNdsR2AnimCacheRejectedUniqueCount = sNdsR2AnimRejectedCount;
    gNdsR2AnimCacheRejectedUniqueBytes += (size + 15u) & ~15u;
}
/* WHY the last overflow overflowed. Overflows 126 beside UsedBytes 3,728 and
 * ReservedBytes 92,160 is not self-consistent -- a 92 KiB arena holding 3.7 KiB
 * cannot refuse an animation of a few KiB -- and on 2026-08-02 that ambiguity
 * cost a reading. These two make the arithmetic checkable from the soak: the
 * request that was refused, and the arena's own cursor at that instant. */
volatile u32 gNdsR2AnimCacheArenaOverflowLastSize;
volatile u32 gNdsR2AnimCacheArenaOverflowLastUsed;

/* Take the arena from the heap it is protecting, once, and only out of what is
 * genuinely spare.
 *
 * The reservation happens LAZILY, on the first store rather than at battle start,
 * and that ordering is the safety property: by the time any animation wants to be
 * cached, ftManagerSetupFilesAllKind has already taken its 116,752 bytes for the
 * fighters. So this can never be the allocation that starves battle start -- the
 * failure mode that a 128 KiB static array caused and that cost the owner a ROM
 * that could not start a match.
 *
 * NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE is then the second half of that guarantee:
 * syTaskmanMalloc is only called when the request AND that reserve both fit, so
 * the cache declines rather than consuming the last of the heap. Declining is
 * free -- it degrades to the on-demand load, which is what the port did before
 * this cache existed. */
/* Startup is not the final heap peak: countdown threads and item actors are
 * created later. Keep the ledger's runtime reserve, not merely a setup floor
 * that can fall below ifCommonSetMaxNumGObj's 25 KiB latch after countdown. */
#define NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE NDS_RELOC_MEMORY_LEDGER_RESERVE_BYTES

/* True while the reserved block is still ours, decided by the heap GENERATION
 * and not by where the cursor happens to sit.
 *
 * gSYTaskmanGeneralHeap is a bump region that every scene entry rewinds. This
 * used to infer ownership from the cursor -- "our block ends at or before
 * `ptr`, so it has not been reclaimed" -- which is a heuristic, and it
 * false-positives exactly on the path that matters. A second entry into the
 * battle scene (Sudden Death, or START-rematch) rewinds the heap and then
 * allocates; as soon as the new scene's own allocations push the cursor back
 * past our stale block, the test starts passing again and every cached payload
 * pointer is handed out pointing into memory the new scene already owns. That
 * is silent corruption, and strictly worse than the hang the cache was moved to
 * the heap to avoid.
 *
 * `gNdsTaskmanHeapGeneration` cannot be fooled that way: it is bumped at the
 * two primitives that move the cursor backwards, so a mismatch is proof the
 * block is dead no matter where the cursor is now. The range test is retained
 * BELOW it as a secondary corruption check -- if the block ever falls outside
 * the live region while the generation still matches, something other than a
 * scene rewind has moved it, and that is worth failing on too. */
static sb32 ndsR2AnimCacheArenaStillOwned(void)
{
    const u8 *cursor = (const u8 *)gSYTaskmanGeneralHeap.ptr;

    if ((sNdsR2AnimCacheArena == NULL) || (sNdsR2AnimCacheArenaBytes == 0u))
    {
        return FALSE;
    }
    if (sNdsR2AnimCacheArenaGeneration != gNdsTaskmanHeapGeneration)
    {
        gNdsR2AnimCacheArenaGenerationMismatches++;
        return FALSE;
    }
    if (sNdsR2AnimCacheArenaElastic != FALSE)
    {
        /* An elastic block lives ABOVE the cursor: every allocation that would
         * reach it takes the bytes back first (battleship_sys_malloc.c). */
        if ((sNdsR2AnimCacheArena < cursor) ||
            ((sNdsR2AnimCacheArena + sNdsR2AnimCacheArenaBytes) >
             (const u8 *)gSYTaskmanGeneralHeap.end))
        {
            gNdsR2AnimCacheArenaRangeFaults++;
            return FALSE;
        }
        return TRUE;
    }
    if ((sNdsR2AnimCacheArena < (const u8 *)gSYTaskmanGeneralHeap.start) ||
        ((sNdsR2AnimCacheArena + sNdsR2AnimCacheArenaBytes) > cursor))
    {
        /* Same generation but out of range: not a scene rewind. Fail closed and
         * count it separately so the two causes are never conflated. */
        gNdsR2AnimCacheArenaRangeFaults++;
        return FALSE;
    }
    return TRUE;
}

/* Defined below with the residency loader; declared here because THIS is the
 * one seam where the arena stops being ours, and the pack lives in it. Binding
 * the invalidation to the writer rather than to each reader is the shape the
 * gMPCollisionGeometry fix settled on. */
static void ndsBattlePackResidencyDrop(void);

static void ndsRelocRecordAuthoritativeForceFile(void *heap, void *file);
void ndsFtPoseRelocateScripts(GObj *gobj, const void *base, u32 size,
                              intptr_t delta);

/* The clip goes to the fighter's own heap and every pointer into it follows
 * (see the block at sNdsR2AnimPins). A pin of a rewound scene is only
 * forgotten: its fighter and heap are gone. */
static void ndsR2AnimPinRescue(NDSR2AnimPin *pin)
{
    const u32 state = pin->state;
    u8 *heap = pin->heap;
    const u8 *payload = pin->payload;
    const u32 size = pin->size;
    const intptr_t delta = (intptr_t)heap - (intptr_t)payload;
    NDSRelocLoadedFile *loaded;
    u32 j;

    pin->state = NDS_R2_ANIM_PIN_FREE;
    if ((state == NDS_R2_ANIM_PIN_FREE) ||
        (pin->generation != gNdsTaskmanHeapGeneration) ||
        (heap == NULL) || (pin->fp == NULL))
    {
        NDS_DIAG(gNdsR2AnimZeroCopyDrops++);
        return;
    }
    ndsRelocPrepareFighterAnimHeapOverwrite(pin->asset_id, heap);
    memcpy(heap, payload, size);
    loaded = ndsRelocRegisterLoadedFile(pin->asset_id, 0, heap, &pin->header);
    if (loaded != NULL)
    {
        ndsRelocMarkLoadedFileRelativeOffsets(loaded);
    }
    ndsRelocRecordAuthoritativeForceFile(heap, heap);
    if ((const u8 *)pin->fp->figatree == payload)
    {
        pin->fp->figatree = (void *)heap;
    }
    if (state == NDS_R2_ANIM_PIN_BOUND)
    {
        ndsFtPoseRelocateScripts(pin->gobj, payload, size, delta);
        for (j = 0u; j < ARRAY_COUNT(pin->fp->joints); j++)
        {
            DObj *dobj = pin->fp->joints[j];
            const u8 *script = (dobj != NULL) ?
                (const u8 *)dobj->anim_joint.event16 : NULL;

            if ((script != NULL) && (script >= payload) &&
                (script < payload + size))
            {
                dobj->anim_joint.event16 =
                    (AObjEvent16 *)(void *)((u8 *)(uintptr_t)script + delta);
            }
        }
    }
    NDS_DIAG(gNdsR2AnimZeroCopyRescues++);
}

static void ndsR2AnimPinsRescueRange(const u8 *start, const u8 *end)
{
    u32 i;

    for (i = 0u; i < NDS_R2_ANIM_PIN_SLOTS; i++)
    {
        NDSR2AnimPin *pin = &sNdsR2AnimPins[i];

        if ((pin->state != NDS_R2_ANIM_PIN_FREE) && (pin->payload < end) &&
            (start < pin->payload + pin->size))
        {
            ndsR2AnimPinRescue(pin);
        }
    }
}

static void ndsR2AnimPinsDropHeap(const void *heap)
{
    u32 i;

    for (i = 0u; i < NDS_R2_ANIM_PIN_SLOTS; i++)
    {
        if ((sNdsR2AnimPins[i].state != NDS_R2_ANIM_PIN_FREE) &&
            (sNdsR2AnimPins[i].heap == (const u8 *)heap))
        {
            sNdsR2AnimPins[i].state = NDS_R2_ANIM_PIN_FREE;
        }
    }
}

/* The fighter is being destroyed (ndsFtPoseRelease): its heap is not a place
 * a later rescue may write. */
void ndsR2AnimPinsDropGObj(GObj *gobj)
{
    u32 i;

    for (i = 0u; i < NDS_R2_ANIM_PIN_SLOTS; i++)
    {
        if ((sNdsR2AnimPins[i].state != NDS_R2_ANIM_PIN_FREE) &&
            (sNdsR2AnimPins[i].gobj == gobj))
        {
            sNdsR2AnimPins[i].state = NDS_R2_ANIM_PIN_FREE;
            NDS_DIAG(gNdsR2AnimZeroCopyDrops++);
        }
    }
}

/* lbCommonAddFighterPartsFigatree bound `figatree` for `gobj`: a pending pin
 * of that clip in this fighter's heap is now the binding, and this fighter
 * owns it; any other pin of the fighter is not.
 *
 * Owned by the binder, not by whoever the fetch found. ndsR2AnimPinTake names
 * the first linked fighter with the heap, and the Polygon Team's next polygon
 * (sc1PGameSpawnEnemyTeamNext) takes its predecessor's heap -- and FTStruct --
 * in the same frame that predecessor's GObj is still linked. The pin then named
 * the old GObj: this bind never marked it, a later rescue moved fp->figatree
 * into the heap without moving the joints' scripts (it moves them only for a
 * bound pin), and the new polygon played on from cache bytes the next clip
 * overwrote -- the Polygon Team's TraI crash. */
void ndsR2AnimZeroCopyNoteBind(GObj *gobj, const void *figatree)
{
    FTStruct *fp = (gobj != NULL) ? ftGetStruct(gobj) : NULL;
    u32 i;

    for (i = 0u; i < NDS_R2_ANIM_PIN_SLOTS; i++)
    {
        NDSR2AnimPin *pin = &sNdsR2AnimPins[i];

        if (pin->state == NDS_R2_ANIM_PIN_FREE)
        {
            continue;
        }
        if ((fp != NULL) && (pin->payload == (const u8 *)figatree) &&
            (pin->heap == (u8 *)fp->figatree_heap))
        {
            pin->gobj = gobj;
            pin->fp = fp;
            pin->state = NDS_R2_ANIM_PIN_BOUND;
        }
        else if (pin->gobj == gobj)
        {
            pin->state = NDS_R2_ANIM_PIN_FREE;
        }
    }
}

/* A pending pin for the battle fighter whose figatree heap is `heap`, or NULL
 * (not a fighter heap, or no free slot): the caller then copies. */
static const u8 *ndsR2AnimPinTake(void *heap, const NDSR2AnimCacheEntry *cached,
                                  u32 asset_id)
{
    GObj *gobj;
    u32 i;

    for (gobj = gGCCommonLinks[nGCCommonLinkIDFighter]; gobj != NULL;
         gobj = gobj->link_next)
    {
        FTStruct *fp = ftGetStruct(gobj);

        if ((fp == NULL) || (fp->figatree_heap != heap))
        {
            continue;
        }
        for (i = 0u; i < NDS_R2_ANIM_PIN_SLOTS; i++)
        {
            NDSR2AnimPin *pin = &sNdsR2AnimPins[i];

            if ((pin->state == NDS_R2_ANIM_PIN_FREE) ||
                (pin->generation != gNdsTaskmanHeapGeneration))
            {
                pin->heap = (u8 *)heap;
                pin->fp = fp;
                pin->gobj = gobj;
                pin->payload = (const u8 *)cached->payload;
                pin->size = cached->size;
                pin->asset_id = asset_id;
                pin->generation = gNdsTaskmanHeapGeneration;
                pin->header = cached->header;
                pin->state = NDS_R2_ANIM_PIN_PENDING;
                return pin->payload;
            }
        }
        return NULL;
    }
    return NULL;
}

static void ndsR2AnimCacheArenaDropForReset(void)
{
    ndsR2AnimPrefetchDrain();
    /* Every pinned clip leaves the arena first (a rewound scene's pins are
     * only forgotten -- ndsR2AnimPinRescue checks the generation). */
    if (sNdsR2AnimCacheArena != NULL)
    {
        ndsR2AnimPinsRescueRange(sNdsR2AnimCacheArena,
                                 sNdsR2AnimCacheArena +
                                     sNdsR2AnimCacheArenaBytes);
    }
    /* BEFORE the pointers are cleared: the pack's storage is inside this block,
     * so a lookup that survived the rewind would otherwise hand the parser a
     * figatree pointing into memory the next scene already owns. */
    ndsBattlePackResidencyDrop();
    sNdsR2AnimCacheArena = NULL;
    sNdsR2AnimCacheArenaBytes = 0u;
    sNdsR2AnimCacheArenaUsed = 0u;
    sNdsR2AnimCacheArenaRawOnly = FALSE;
    sNdsR2AnimCacheArenaElastic = FALSE;
    sNdsR2AnimCacheArenaGeneration = 0u;
    /* Drops every entry, which is what makes the payload pointers unreachable.
     * The entries are the only holders of those pointers. */
    sNdsR2AnimCacheCount = 0u;
    memset(sNdsR2AnimCacheIndex, 0, sizeof(sNdsR2AnimCacheIndex));
    gNdsR2AnimCacheArenaUsedBytes = 0u;
    gNdsR2AnimCacheArenaReservedBytes = 0u;
    gNdsR2AnimCacheBytes = 0u;
    gNdsR2AnimCacheArenaInvalidations++;
}

/* Invalidate before ANY use, not only on the lookup path. Called at the top of
 * every cache entry point: a stale generation must never survive into a find, a
 * store or a preload step, and the preload path in particular used to touch the
 * cursor and the warm list without consulting ownership at all. Cheap -- one
 * compare when the cache is empty, which is the state this leaves it in. */
static void ndsR2AnimCacheValidateGeneration(void)
{
    if ((sNdsR2AnimCacheArena != NULL) &&
        (ndsR2AnimCacheArenaStillOwned() == FALSE))
    {
        ndsR2AnimCacheArenaDropForReset();
    }
}

/* P2-3f9 -- THE PACK IS ONE FIGHTER'S CLIPS, AND IT STOPS PAYING AS THE ROSTER
 * GROWS.
 *
 * `battlepack_fox.bin` (Makefile: NDS_BATTLEPACK_BLOB) is Fox's prebuilt
 * animation set, and its reserve is 287,936 B of the SAME scene arena every
 * distinct fighter kind in the match has to fit in. On the two-fighter shipping
 * arm that trade is excellent: measured -34,304 ticks. On four distinct kinds
 * P2-3r13 measured it at **-64 ticks at P50** -- because the pack serves one
 * fighter in four and the other three take the on-demand path either way -- for
 * the same 287,936 B. That is the wrong side of the trade, and it is the reason
 * the argmax four-kind roster could not be seated: measured 2026-08-25, a shell
 * -driven Luigi/Fox/Captain/Donkey match halted in ndsSyMallocOverflowHalt with
 * 5,824 B free.
 *
 * So the carve is now a per-match decision taken where the roster is already
 * known. `gSCManagerBattleState` is populated by ndsMatchConfigApply long before
 * scVSBattleStartBattle runs, and this reservation is lazy (first pack preload
 * step, or the first cache store), so both callers read a settled roster.
 *
 * THE RULE: carve only when the packed kind is actually in the match AND the
 * match has at most two distinct kinds. Anything else keeps the raw cache and
 * returns the reserve to the arena. Declining costs the packed fighter its
 * prebuilt clips and nothing else -- every path here degrades to the on-demand
 * load, which is a PERFORMANCE outcome and never a correctness one. An unknown
 * roster keeps today's behaviour rather than guessing.
 *
 * `NDS_BATTLEPACK_FIGHTER_KIND` is pinned to the blob the Makefile builds. If
 * the pack ever holds a different fighter, this constant moves with it. */
/* Defined whether or not the pack is built, and `used` so --gc-sections cannot
 * drop them: a verifier that names a symbol only some configurations carry
 * fails on "Missing ELF symbol" instead of on the thing it guards. */
__attribute__((used)) volatile u32 gNdsBattlePackCarveDeclineCount;
__attribute__((used)) volatile u32 gNdsBattlePackCarveMatchKinds;

#define NDS_BATTLEPACK_FIGHTER_KIND nFTKindFox
#define NDS_BATTLEPACK_CARVE_MAX_KINDS 2u

static sb32 ndsR2BattlePackCarveWorthIt(void)
{
    s32 i;
    u32 seen = 0u;
    u32 distinct = 0u;
    sb32 has_packed_kind = FALSE;
    sb32 has_content = FALSE;

    if (gSCManagerBattleState == NULL)
    {
        return TRUE;
    }
    for (i = 0; i < (s32)GMCOMMON_PLAYERS_MAX; i++)
    {
        u32 fkind = (u32)gSCManagerBattleState->players[i].fkind;

        if (gSCManagerBattleState->players[i].pkind == nFTPlayerKindNot)
        {
            continue;
        }
        if (fkind > (u32)nFTKindPlayableEnd)
        {
            continue;
        }
        /* A content plays its own clips, never the pack's: its kind is its
         * parent's, so four Palutenas (Fox) took the pack's carve, its
         * 128 KB keep-free declined it, the decline latched, and the match
         * ran with no motion cache at all (2026-10-18; every Fox-parent
         * content: Falco, Wolf, Peach, Sonic, Palutena). It neither wants
         * the pack nor counts as one of its kinds. */
        if (ndsP4MatchContent(i) != 0u)
        {
            has_content = TRUE;
            continue;
        }
        if ((seen & (1u << fkind)) == 0u)
        {
            seen |= 1u << fkind;
            distinct++;
        }
        if (fkind == (u32)NDS_BATTLEPACK_FIGHTER_KIND)
        {
            has_packed_kind = TRUE;
        }
    }
    gNdsBattlePackCarveMatchKinds = distinct;
    if (distinct == 0u)
    {
        /* No roster yet -- do not spend the decision on an empty state --
         * unless every player is a content (no pack kind at all). */
        return (has_content != FALSE) ? FALSE : TRUE;
    }
    return ((has_packed_kind != FALSE) &&
            (distinct <= NDS_BATTLEPACK_CARVE_MAX_KINDS)) ? TRUE : FALSE;
}

/* What this match still has to load AFTER the reservation is taken.
 *
 * The reservation runs early in battle setup and the fighters' file trees are
 * the largest thing that follows it, so a reservation that fits by itself can
 * still starve them -- which is exactly what a ten-fighter ROM does: measured
 * 2026-09-03, Kirby versus Fox reserved 451,776 for pack plus cache, then
 * halted loading Fox's 115,440-byte tree with 40,636 bytes left. The old fit
 * test only kept the 32,768-byte GObj latch free, which says nothing about the
 * fighters.
 *
 * Ask the DS extern-tree size provider for each distinct, not-yet-resident
 * Main. Its converted allocation can differ from the N64 census. */
static u32 ndsR2AnimCacheMatchFighterBytes(void)
{
    s32 i;
    u32 seen = 0u;
    u32 total = 0u;

    if (gSCManagerBattleState == NULL)
    {
        return 0u;
    }
    for (i = 0; i < (s32)GMCOMMON_PLAYERS_MAX; i++)
    {
        u32 fkind = (u32)gSCManagerBattleState->players[i].fkind;
        FTData *data;
        NDSRelocLoadedFile *loaded;
        u32 fallback_before;
        size_t bytes;

        if (gSCManagerBattleState->players[i].pkind == nFTPlayerKindNot)
        {
            continue;
        }
        if (fkind >= (u32)nFTKindEnumCount)
        {
            continue;
        }
        /* A P4 player's kind is its parent's (Banjo and Dedede are Captain):
         * its own pack is loaded at setup, and the parent's Main never is.
         * Sizing the parent's tree here charged a match for a file it does
         * not load, and the size walk loaded the parent's guard-pose
         * package as a side effect -- 3.3 KB in the match's last 2 KB. */
        if (ndsP4MatchContent(i) != 0u)
        {
            continue;
        }
        if ((seen & (1u << fkind)) != 0u)
        {
            continue;
        }
        seen |= 1u << fkind;
        data = dFTManagerDataFiles[fkind];
        if (data == NULL)
        {
            return UINT32_MAX;
        }
        loaded = ((data->p_file_main != NULL) && (*data->p_file_main != NULL)) ?
            ndsRelocFindLoadedFileContaining(*data->p_file_main, 1u) : NULL;
        if ((loaded != NULL) &&
            (loaded->owner_generation == sNdsRelocSceneGeneration))
        {
            continue;
        }
        /* The DS extern-tree allocation can differ from the N64 census.
         * Ask the same provider the pending source load will call. */
        fallback_before = gNdsRelocFileSizeFallbackCount;
        bytes = lbRelocGetFileSize(data->file_main_id);
        if ((gNdsRelocFileSizeFallbackCount != fallback_before) ||
            (bytes > UINT32_MAX - total))
        {
            return UINT32_MAX;
        }
        total += (u32)bytes;
    }
    return total;
}

__attribute__((used)) volatile u32 gNdsR2AnimCacheMatchFighterBytes;
__attribute__((used)) volatile u32 gNdsR2AnimCachePackDroppedForFightersCount;
__attribute__((used)) volatile u32 gNdsR2AnimCacheArenaReserveFailSkips;
static u32 sNdsR2AnimCacheSetupGeneration;
static u32 sNdsR2AnimCacheReserveFailGeneration;
static sb32 sNdsR2AnimCacheReserveFailLatched;

/* Once a post-setup reservation cannot fit, this scene generation cannot make
 * it fit later: gSYTaskmanGeneralHeap is a bump allocator and pending fighter
 * bytes are explicitly reserved in the fit calculation.  Before this latch a
 * four-kind match that had no cache room repeated the full roster/heap sizing
 * path on every animation store (1,354 times in the 2026-09-15 P2-2p8 stress).
 * Keep one observed failure for the verifier, then make later attempts in the
 * same generation a constant-time decline. */
static sb32 ndsR2AnimCacheArenaReserveDecline(void)
{
    gNdsR2AnimCacheArenaReserveFailCount++;
    sNdsR2AnimCacheReserveFailGeneration = gNdsTaskmanHeapGeneration;
    sNdsR2AnimCacheReserveFailLatched = TRUE;
    return FALSE;
}

/* THE ELASTIC MOTION CACHE (P2-2p8, 2026-09-28).
 *
 * The raw cache used to be a block allocated below the cursor and sized by
 * what the heap spared after the fighters and a fixed 128 KiB keep-free: 51 KB
 * on the four-CPU gate's Dream Land and 0 B for a four-kind roster on the
 * full-content image, where every status change then read its clip from
 * storage mid-frame (~30K ticks). Most of that keep-free is never used -- the
 * match's low-water sits well above the 25,600 B floor -- but a static block
 * cannot give bytes back when the match does grow.
 *
 * The elastic cache borrows the free bytes at the TOP of the heap instead,
 * without allocating them: `end` does not move, so the source's GObj latch
 * and every admission check still read the true free space. An allocation
 * that would reach the borrowed range asks for it back first (the allocator
 * calls ndsR2AnimCacheElasticYield); the entries in the returned range are
 * dropped and the block's base moves up. Cached templates are copied into a
 * fighter's own heap before use, so nothing ever points into the block and
 * giving bytes back is always safe. */
__attribute__((used)) volatile u32 gNdsR2AnimCacheElasticCarves;
__attribute__((used)) volatile u32 gNdsR2AnimCacheElasticDrops;
/* Bytes kept below the block at the carve, so the next small allocations do
 * not each take a slice back; and the extra slice a yield returns. */
#define NDS_R2_ANIM_CACHE_ELASTIC_FLOOR 12288u
#define NDS_R2_ANIM_CACHE_ELASTIC_SLACK 8192u
#define NDS_R2_ANIM_CACHE_ELASTIC_MIN 4096u
/* The elastic block's own cap: it gives bytes back on demand, so it is not
 * bounded by the static reservation's 258,048. */
#define NDS_R2_ANIM_CACHE_ELASTIC_MAX_BYTES (448u * 1024u)

void ndsTaskmanElasticRegister(void *base, u8 *(*yield)(u8 *need_top));
static void ndsR2AnimCacheEvictRawRange(u32 offset, u32 size);

static u8 *ndsR2AnimCacheElasticYield(u8 *need_top)
{
    u8 *arena = sNdsR2AnimCacheArena;
    u32 bytes = sNdsR2AnimCacheArenaBytes;
    uintptr_t want;
    u32 delta;

    if ((arena == NULL) || (sNdsR2AnimCacheArenaElastic == FALSE) ||
        (sNdsR2AnimCacheArenaGeneration != gNdsTaskmanHeapGeneration))
    {
        return (u8 *)gSYTaskmanGeneralHeap.end;
    }
    want = ((uintptr_t)need_top + NDS_R2_ANIM_CACHE_ELASTIC_SLACK + 31u) &
        ~(uintptr_t)31u;
    if ((want + NDS_R2_ANIM_CACHE_ELASTIC_MIN) >= ((uintptr_t)arena + bytes))
    {
        ndsR2AnimCacheArenaDropForReset();
        NDS_DIAG(gNdsR2AnimCacheElasticDrops++);
        return (u8 *)gSYTaskmanGeneralHeap.end;
    }
    if (want <= (uintptr_t)arena)
    {
        return arena;
    }
    delta = (u32)(want - (uintptr_t)arena);
    ndsR2AnimPrefetchRetire(arena, arena + delta);
    ndsR2AnimCacheEvictRawRange(0u, delta);
    sNdsR2AnimCacheArena = arena + delta;
    sNdsR2AnimCacheArenaBytes = bytes - delta;
    sNdsR2AnimCacheArenaUsed = (sNdsR2AnimCacheArenaUsed > delta) ?
        (sNdsR2AnimCacheArenaUsed - delta) : 0u;
    gNdsR2AnimCacheArenaReservedBytes = sNdsR2AnimCacheArenaBytes;
    gNdsR2AnimCacheArenaUsedBytes = sNdsR2AnimCacheArenaUsed;
    return sNdsR2AnimCacheArena;
}

/* `available` is the true free space above the aligned cursor. */
static sb32 ndsR2AnimCacheArenaCarveElastic(size_t available,
                                           u32 fighter_bytes)
{
    uintptr_t top = (uintptr_t)gSYTaskmanGeneralHeap.end & ~(uintptr_t)31u;
    u32 bytes;

    if ((fighter_bytes > available) ||
        ((available - fighter_bytes) <=
         (NDS_R2_ANIM_CACHE_ELASTIC_FLOOR + NDS_R2_ANIM_CACHE_ELASTIC_MIN + 32u)))
    {
        return ndsR2AnimCacheArenaReserveDecline();
    }
    bytes = (u32)(available - fighter_bytes - NDS_R2_ANIM_CACHE_ELASTIC_FLOOR -
                  32u);
    if (bytes > NDS_R2_ANIM_CACHE_ELASTIC_MAX_BYTES)
    {
        bytes = NDS_R2_ANIM_CACHE_ELASTIC_MAX_BYTES;
    }
    bytes &= ~31u;
    sNdsR2AnimCacheReserveFailLatched = FALSE;
    sNdsR2AnimCacheArena = (u8 *)(top - bytes);
    sNdsR2AnimCacheExtentOrigin = (uintptr_t)sNdsR2AnimCacheArena & ~(uintptr_t)15u;
    sNdsR2AnimCacheArenaBytes = bytes;
    sNdsR2AnimCacheArenaUsed = 0u;
    sNdsR2AnimCacheArenaRawOnly = TRUE;
    sNdsR2AnimCacheArenaElastic = TRUE;
    sNdsR2AnimCacheArenaGeneration = gNdsTaskmanHeapGeneration;
    gNdsR2AnimCacheArenaReservedBytes = bytes;
    gNdsR2AnimCacheArenaUsedBytes = 0u;
    gNdsR2AnimCacheArenaReserveCount++;
    NDS_DIAG(gNdsR2AnimCacheElasticCarves++);
    ndsTaskmanElasticRegister(sNdsR2AnimCacheArena, ndsR2AnimCacheElasticYield);
    return TRUE;
}

static sb32 ndsR2AnimCacheArenaEnsure(void)
{
    void *block;
    u32 reserve = 0u;
    u32 raw_bytes = (u32)NDS_R2_ANIM_CACHE_STANDALONE_RAW_BYTES;
    u32 arena_bytes;
    u32 fighter_bytes;
    uintptr_t aligned;
    size_t available;

    /* CSS may already own its explicitly sized setup cache. Preserve it;
     * a fresh lazy carve must wait for the post-setup warmer in every scene.
     * Results also loads animations before its mandatory sprite surface, so
     * treating only battle constructors as unfinished steals that surface's
     * reserve. Initial reads retain the existing uncached load path. */
    if (ndsR2AnimCacheArenaStillOwned() != FALSE)
    {
        return TRUE;
    }
    if (sNdsR2AnimCacheSetupGeneration != gNdsTaskmanHeapGeneration)
    {
        return FALSE;
    }
    if ((sNdsR2AnimCacheReserveFailLatched != FALSE) &&
        (sNdsR2AnimCacheReserveFailGeneration == gNdsTaskmanHeapGeneration))
    {
        gNdsR2AnimCacheArenaReserveFailSkips++;
        return FALSE;
    }
    if (sNdsR2AnimCacheArena != NULL)
    {
        /* Had a block, no longer owns it: the heap was rewound under us. Every
         * cached pointer is stale, so drop the whole cache before re-reserving. */
        ndsR2AnimCacheArenaDropForReset();
    }
    if (ndsR2BattlePackCarveWorthIt() != FALSE)
    {
        reserve = (u32)NDS_BATTLEPACK_RESERVE_BYTES;
        raw_bytes = (u32)NDS_R2_ANIM_CACHE_PACK_RAW_BYTES;
    }
    else
    {
        gNdsBattlePackCarveDeclineCount++;
    }
    arena_bytes = reserve + raw_bytes;
    fighter_bytes = ndsR2AnimCacheMatchFighterBytes();
    gNdsR2AnimCacheMatchFighterBytes = fighter_bytes;
    aligned = ((uintptr_t)gSYTaskmanGeneralHeap.ptr + NDS_RELOC_ALIGN_BYTES - 1u) &
              ~((uintptr_t)NDS_RELOC_ALIGN_BYTES - 1u);
    available = ((uintptr_t)gSYTaskmanGeneralHeap.end >= aligned) ?
        (uintptr_t)gSYTaskmanGeneralHeap.end - aligned : 0u;
    if (reserve == 0u)
    {
        return ndsR2AnimCacheArenaCarveElastic(available, fighter_bytes);
    }
    if ((fighter_bytes > available) ||
        (NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE >= available - fighter_bytes))
    {
        return ndsR2AnimCacheArenaReserveDecline();
    }
    available -= fighter_bytes + NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE;
    /* Give the fighters right of way. The pack is a performance optimisation
     * whose every failure path degrades to the on-demand loader, so trading it
     * for a match that can actually load is not a fidelity decision -- the
     * alternative is a halt. After dropping the pack, the raw cache below is
     * capped to the remaining space; a smaller cache retains its miss path. */
    if ((reserve != 0u) &&
        (ndsSyMallocWouldFit(&gSYTaskmanGeneralHeap,
                             (size_t)arena_bytes + fighter_bytes +
                                 NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE,
                             NDS_RELOC_ALIGN_BYTES) == FALSE))
    {
        reserve = 0u;
        raw_bytes = (u32)NDS_R2_ANIM_CACHE_STANDALONE_RAW_BYTES;
        arena_bytes = raw_bytes;
        gNdsR2AnimCachePackDroppedForFightersCount++;
    }
    /* A full standalone cache also starved Fox: 258,048 bytes were reserved
     * before his 116,752-byte tree, leaving only 75,788. Keep as much cache as
     * fits after pending trees and the setup floor, rather than discarding the
     * cache outright. Its existing miss path remains source-correct. */
    if ((reserve == 0u) && (arena_bytes > available))
    {
        arena_bytes = (u32)available & ~(NDS_RELOC_ALIGN_BYTES - 1u);
    }
    if (arena_bytes == 0u)
    {
        return ndsR2AnimCacheArenaReserveDecline();
    }
    if (ndsSyMallocWouldFit(&gSYTaskmanGeneralHeap,
                            (size_t)arena_bytes + fighter_bytes +
                                NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE,
                            NDS_RELOC_ALIGN_BYTES) == FALSE)
    {
        return ndsR2AnimCacheArenaReserveDecline();
    }
    block = syTaskmanMalloc((size_t)arena_bytes, NDS_RELOC_ALIGN_BYTES);
    if (block == NULL)
    {
        return ndsR2AnimCacheArenaReserveDecline();
    }
    sNdsR2AnimCacheReserveFailLatched = FALSE;
    sNdsR2AnimCacheArena = block;
    sNdsR2AnimCacheExtentOrigin = (uintptr_t)block & ~(uintptr_t)15u;
    sNdsR2AnimCacheArenaBytes = arena_bytes;
    /* RawOnly is what tells the pack loader there is no carved region to stream
     * into; it is TRUE here for exactly the same reason it is TRUE for the
     * setup arena, and the loader's existing arm handles it. */
    sNdsR2AnimCacheArenaRawOnly = (reserve == 0u) ? TRUE : FALSE;
    /* CARVE THE PACK'S REGION FIRST, at reservation, not by being the first
     * caller of the bump allocator. Ordering was the first design and it lost:
     * fighter setup stores 3,728 bytes of animation into this arena before the
     * first scene update runs a pack step, so the 287,936-byte request was
     * refused by 848 bytes and `gNdsBattlePackHits` read 0 with
     * `gNdsR2AnimCacheArenaOverflowLastSize 287936` naming the victim
     * (soak 2026-08-15_015724). Now the blob owns [0, RESERVE) of every arena
     * generation and no store can get in front of it. */
    sNdsR2AnimCacheArenaUsed = reserve;
    /* Stamp the generation the block was taken under. Must be read AFTER the
     * allocation: syTaskmanMalloc cannot rewind the heap, so this is the same
     * value either way, but taking it here keeps the store adjacent to the
     * pointer it qualifies. */
    sNdsR2AnimCacheArenaGeneration = gNdsTaskmanHeapGeneration;
    /* Report what was actually taken, not the build-time maximum: since P2-3f9
     * this reservation is roster-dependent, and a ledger that always printed
     * the constant would hide the whole decision. */
    gNdsR2AnimCacheArenaReservedBytes = arena_bytes;
    gNdsR2AnimCacheArenaUsedBytes = sNdsR2AnimCacheArenaUsed;
    gNdsR2AnimCacheArenaReserveCount++;
    return TRUE;
}

/* CSS pins its initial clips in a scene-generation cache separate from the
 * battle pack. Size them with the same provider as the warm loader below;
 * roster additions must not silently exceed a historical fixed carve. */
static size_t ndsR2AnimCachePayloadBytes(u32 asset_id, sb32 *stream_ready,
                                         u32 *stream_size)
{
    size_t bytes;

    *stream_ready = FALSE;
    *stream_size = 0u;
    *stream_ready = ndsRelocAssetLoadFighterStreamClip(asset_id, NULL, stream_size);
    if (*stream_ready != FALSE)
    {
        return *stream_size;
    }
    bytes = ndsRelocP2GeneratedAllocSize(asset_id);
    if (bytes != 0u)
    {
        return bytes;
    }
    return ndsRelocAssetAllocSize(asset_id);
}

/* BattleShip's submotion descriptor tables are source data, not fighter-file
 * residency. CSS lazy acquisition may have zero, one, or four live Mains when
 * the animation arena is first requested, so deriving the working set through
 * FTData::p_file_main makes the reservation depend on acquisition order.
 *
 * Keep the CSS manifest at the behavioral source seam instead. Row 0 is the
 * DemoNull/Opening2 pose ftManagerMakeFighter touches before CSS applies its
 * Selected status. Every admitted Selected pose is compiled directly into
 * battleship_scsubsysdata_ft.c and therefore consumes no raw animation-cache
 * storage in this roster. If that source contract changes, the Selected loader
 * remains the required place to add the new cached identity.
 *
 * These declarations name the exact BattleShip tables linked by the fighter
 * subsystem; they exist independently of whether the corresponding Main has
 * been acquired into one of CSS's four resident blocks. */
extern FTMotionDesc dFTMarioSubMotionDescs[];
extern FTMotionDesc dFTFoxSubMotionDescs[];
#if NDS_P2_DONKEY
extern FTMotionDesc dFTDonkeySubMotionDescs[];
#endif
extern FTMotionDesc dFTSamusSubMotionDescs[];
#if NDS_P2_LUIGI
extern FTMotionDesc dFTLuigiSubMotionDescs[];
#endif
#if NDS_P2_LINK
extern FTMotionDesc dFTLinkSubMotionDescs[];
#endif
extern FTMotionDesc dFTYoshiSubMotionDescs[];
#if NDS_P2_CAPTAIN
extern FTMotionDesc dFTCaptainSubMotionDescs[];
#endif
extern FTMotionDesc dFTKirbySubMotionDescs[];
extern FTMotionDesc dFTPikachuSubMotionDescs[];
extern FTMotionDesc dFTPurinSubMotionDescs[];
extern FTMotionDesc dFTNessSubMotionDescs[];

static const FTMotionDesc * const
sNdsR2CssInitialMotionDescs[nFTKindPlayableEnd + 1] = {
    [nFTKindMario] = dFTMarioSubMotionDescs,
    [nFTKindFox] = dFTFoxSubMotionDescs,
#if NDS_P2_DONKEY
    [nFTKindDonkey] = dFTDonkeySubMotionDescs,
#endif
    [nFTKindSamus] = dFTSamusSubMotionDescs,
#if NDS_P2_LUIGI
    [nFTKindLuigi] = dFTLuigiSubMotionDescs,
#endif
#if NDS_P2_LINK
    [nFTKindLink] = dFTLinkSubMotionDescs,
#endif
    [nFTKindYoshi] = dFTYoshiSubMotionDescs,
#if NDS_P2_CAPTAIN
    [nFTKindCaptain] = dFTCaptainSubMotionDescs,
#endif
    [nFTKindKirby] = dFTKirbySubMotionDescs,
    [nFTKindPikachu] = dFTPikachuSubMotionDescs,
    [nFTKindPurin] = dFTPurinSubMotionDescs,
    [nFTKindNess] = dFTNessSubMotionDescs,
};

/* Size every admitted source row-0 clip before lazy fighter acquisition begins.
 * Asset-ID dedup handles any shared source identity. Payload bytes come from the
 * warm loader's own provider and the total uses the bump allocator's exact
 * 16-byte placement rule, so the reservation cannot drift from what loading
 * actually consumes. */
static u32 ndsR2AnimCacheSetupBytes(void)
{
    u32 ids[nFTKindPlayableEnd + 1];
    u32 count = 0u;
    u32 total = 0u;
    s32 kind;

    for (kind = nFTKindPlayableStart; kind <= nFTKindPlayableEnd; kind++)
    {
        const FTMotionDesc *motion_desc = sNdsR2CssInitialMotionDescs[kind];
        u32 asset_id;
        u32 i;
        u32 stream_size;
        sb32 stream_ready;
        size_t bytes;

        if (motion_desc == NULL)
        {
            continue;
        }
        asset_id = ndsRelocAssetIDForToken(
            (u32)(uintptr_t)motion_desc[0].anim_file_id);
        if ((asset_id == NDS_RELOC_ASSET_INVALID) ||
            (ndsRelocIsFighterAnimID(asset_id) == FALSE))
        {
            return 0u;
        }
        for (i = 0u; i < count && ids[i] != asset_id; i++) {}
        if (i != count) { continue; }
        ids[count++] = asset_id;
        bytes = ndsR2AnimCachePayloadBytes(asset_id, &stream_ready, &stream_size);
        if ((bytes == 0u) || (total > UINT32_MAX - 15u) ||
            (bytes > UINT32_MAX - total - 15u))
        {
            return 0u;
        }
        total = ((total + 15u) & ~15u) + (u32)bytes;
    }
    /* A P4 content's menu clips are not reserved here (S15): fourteen
     * contents' five rows came to 450 KB, more than the select has. Its
     * preview reads them when it plays them, into the slot's figatree heap,
     * which the select sizes for that content's clips. */
    return total;
}

/* THE VS SELECT'S OWN RESERVE. The 128 KiB above protects a match's later
 * growth (countdown threads, item actors); the select has none of that.
 * Measured 2026-10-07 on the all-content walk ROM. This reservation found
 * 176,352 B free against 52,000 + 128 KiB, so it declined and every preview
 * was off (owner report on play-1007i: the code had grown 3.3 KB). What the
 * select must still allocate after it:
 *   - the four slots' figatree heaps, 4 x 11,792 B;
 *   - the UI surface cache, 9,768 B;
 *   - the preview fighters' objects, 25,880 B for two previews (about 40 KiB
 *     for four).
 * That is about 97 KiB, inside 104 KiB. The door underlay (31,376 B,
 * nds_menu_shell_css.c) is a cache with a streamed fallback and now keeps
 * 48 KiB free behind itself. The lean pool sizes itself to what is left. */
#define NDS_R2_ANIM_CACHE_CSS_KEEP_FREE (104u * 1024u)

static sb32 ndsR2AnimCacheArenaEnsureSetupWith(u32 keep_free)
{
    void *block;
    u32 bytes;

    ndsR2AnimCacheValidateGeneration();
    if (ndsR2AnimCacheArenaStillOwned() != FALSE)
    {
        return TRUE;
    }
    if (sNdsR2AnimCacheArena != NULL)
    {
        ndsR2AnimCacheArenaDropForReset();
    }
    bytes = ndsR2AnimCacheSetupBytes();
    if ((bytes == 0u) ||
        (ndsSyMallocWouldFit(&gSYTaskmanGeneralHeap,
                            (size_t)bytes + keep_free,
                            NDS_RELOC_ALIGN_BYTES) == FALSE))
    {
        gNdsR2AnimCacheArenaReserveFailCount++;
        return FALSE;
    }
    block = syTaskmanMalloc((size_t)bytes,
                            NDS_RELOC_ALIGN_BYTES);
    if (block == NULL)
    {
        gNdsR2AnimCacheArenaReserveFailCount++;
        return FALSE;
    }
    sNdsR2AnimCacheArena = block;
    sNdsR2AnimCacheExtentOrigin = (uintptr_t)block & ~(uintptr_t)15u;
    sNdsR2AnimCacheArenaBytes = bytes;
    sNdsR2AnimCacheArenaUsed = 0u;
    sNdsR2AnimCacheArenaRawOnly = TRUE;
    sNdsR2AnimCacheArenaGeneration = gNdsTaskmanHeapGeneration;
    gNdsR2AnimCacheArenaReservedBytes = bytes;
    gNdsR2AnimCacheArenaUsedBytes = 0u;
    gNdsR2AnimCacheArenaReserveCount++;
    return TRUE;
}

static sb32 ndsR2AnimCacheArenaEnsureSetup(void)
{
    return ndsR2AnimCacheArenaEnsureSetupWith(NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE);
}

s32 ndsR2AnimCacheReserveCSSWorkingSet(void)
{
    return ndsR2AnimCacheArenaEnsureSetupWith(NDS_R2_ANIM_CACHE_CSS_KEEP_FREE);
}

/* Bump allocation from the cache's own arena. Returns NULL on overflow, which is
 * the whole point: see the comment on NDS_R2_ANIM_CACHE_ARENA_BYTES. */
static void *ndsR2AnimCacheArenaAlloc(u32 size)
{
    u32 aligned;

    if (ndsR2AnimCacheArenaEnsure() == FALSE)
    {
        gNdsR2AnimCacheArenaOverflows++;
        gNdsR2AnimCacheArenaOverflowLastSize = size;
        gNdsR2AnimCacheArenaOverflowLastUsed = sNdsR2AnimCacheArenaUsed;
        return NULL;
    }
    aligned = (sNdsR2AnimCacheArenaUsed + 15u) & ~15u;
    if ((size == 0u) || (aligned > sNdsR2AnimCacheArenaBytes) ||
        (size > (sNdsR2AnimCacheArenaBytes - aligned)))
    {
        gNdsR2AnimCacheArenaOverflows++;
        gNdsR2AnimCacheArenaOverflowLastSize = size;
        gNdsR2AnimCacheArenaOverflowLastUsed = sNdsR2AnimCacheArenaUsed;
        return NULL;
    }
    sNdsR2AnimCacheArenaUsed = aligned + size;
    gNdsR2AnimCacheArenaUsedBytes = sNdsR2AnimCacheArenaUsed;
    return &sNdsR2AnimCacheArena[aligned];
}

/* TRUE when no allocation of any size can succeed. Exact, not a threshold: this
 * is ndsR2AnimCacheArenaAlloc's own refusal test with `size` at its minimum.
 *
 * It exists because ndsR2AnimWarmLoadOne calls ndsRelocAssetAllocSize -- a real
 * fopen plus header read -- BEFORE it asks the arena for bytes, so a full arena
 * paid one NitroFS open per remaining warm entry to learn something it already
 * knew. That was tolerable when the arena had 19.9% spare; with the resident
 * pack occupying it, it would be the whole rest of the list. */
static sb32 ndsR2AnimCacheArenaExhausted(void)
{
    u32 aligned;

    if (ndsR2AnimCacheArenaEnsure() == FALSE)
    {
        return TRUE;
    }
    aligned = (sNdsR2AnimCacheArenaUsed + 15u) & ~15u;
    return (aligned >= sNdsR2AnimCacheArenaBytes) ? TRUE : FALSE;
}

/* Give back the most recent allocation. Only valid for the immediately preceding
 * alloc with nothing in between, which is exactly the shape of the warm loader's
 * failure path -- it used to leak the buffer it had just taken. */
static void ndsR2AnimCacheArenaRelease(void *payload, u32 size)
{
    u8 *bytes = payload;

    if ((sNdsR2AnimCacheArena != NULL) && (bytes != NULL) &&
        ((bytes + size) == &sNdsR2AnimCacheArena[sNdsR2AnimCacheArenaUsed]))
    {
        sNdsR2AnimCacheArenaUsed = (u32)(bytes - sNdsR2AnimCacheArena);
        gNdsR2AnimCacheArenaUsedBytes = sNdsR2AnimCacheArenaUsed;
    }
}

/* ---- Slice 1 phase 5: the resident figatree pack's setup-time load --------
 *
 * The blob is a NitroFS payload streamed into the arena above, NOT `.incbin`
 * into `.rodata`: +288,992 B of ARM9 image was MEASURED to push
 * gNdsTaskmanArenaChosenSize 0x150000 -> 0x140000 with 16 alloc failures,
 * because the arena is one calloc from the same libnds heap the image bounds.
 * See the NDS_R2_ANIM_CACHE_ARENA_BYTES comment and BATTLEPACK_POOL.md.
 *
 * IT IS STEPPED. It rides ndsR2AnimCachePreloadStep so each read remains a
 * bounded chunk even though battle startup now drains the steps before BGM:
 *
 *   1. Historical safety. E4 loaded 41 assets in ONE call AFTER BGM was live
 *      and Boundary refused the build on the ADPCM smoke. The new startup
 *      barrier runs before mpCollisionSetPlayBGM, but keeping bounded chunks
 *      prevents this helper from regressing if another caller steps it live.
 *   2. Lazy reservation is a SAFETY property, not an optimisation. Taking the
 *      arena on the first step rather than before fighter setup means
 *      ftManagerSetupFilesAllKind has already taken its 116,752 bytes. The
 *      pre-BGM barrier is deliberately reached only after source battle setup,
 *      so this can never be the allocation that starves fighter construction.
 *
 * Every failure degrades to the generic acquisition path: the pack is simply
 * not published, ndsBattlePackFindFigatree returns NULL, and the load runs as
 * it does today. A partial blob is never published. */

#define NDS_BATTLEPACK_PATH "nitro:/animation/battlepack_fox.bin"
/* 40 B of header padded to the 48 B directory offset (generate_battlepack_anim
 * BLOB_DIR_OFF). Read as words so the u32 fields are naturally aligned -- an
 * unaligned ARM9 LDR rotates the word instead of faulting. */
#define NDS_BATTLEPACK_HEADER_WORDS 12u
/* One bounded chunk per preload step. 287,904 / 16,384 = 18 steps. */
#define NDS_BATTLEPACK_STEP_BYTES 16384u

#define NDS_BATTLEPACK_LOAD_IDLE 0u
#define NDS_BATTLEPACK_LOAD_STREAMING 1u
#define NDS_BATTLEPACK_LOAD_DONE 2u
#define NDS_BATTLEPACK_LOAD_FAILED 3u

static u8 *sNdsBattlePackDst;
static u32 sNdsBattlePackTotal;
static u32 sNdsBattlePackCursor;
static u32 sNdsBattlePackLoadState;

static void ndsBattlePackResidencyDrop(void)
{
    ndsBattlePackDrop();
    sNdsBattlePackDst = NULL;
    sNdsBattlePackTotal = 0u;
    sNdsBattlePackCursor = 0u;
    sNdsBattlePackLoadState = NDS_BATTLEPACK_LOAD_IDLE;
    gNdsBattlePackResidentBytes = 0u;
}

/* Retry a load that failed, without re-streaming one that succeeded. */
static void ndsBattlePackResidencyRearm(void)
{
    if (sNdsBattlePackLoadState == NDS_BATTLEPACK_LOAD_FAILED)
    {
        sNdsBattlePackLoadState = NDS_BATTLEPACK_LOAD_IDLE;
    }
}

static void ndsBattlePackResidencyFail(void)
{
    /* Hand the reserve to the raw cache rather than stranding 288 KiB of arena
     * on a degraded run -- but ONLY while the cache has taken nothing yet, or
     * rewinding the cursor would alias entries that already live above it. */
    if ((sNdsR2AnimCacheArena != NULL) &&
        (sNdsR2AnimCacheArenaUsed == NDS_BATTLEPACK_RESERVE_BYTES))
    {
        sNdsR2AnimCacheArenaUsed = 0u;
        gNdsR2AnimCacheArenaUsedBytes = 0u;
    }
    ndsBattlePackResidencyDrop();
    sNdsBattlePackLoadState = NDS_BATTLEPACK_LOAD_FAILED;
    gNdsBattlePackLoadFails++;
}

/* TRUE while the pack still needs steps, so the caller holds the warm walk off
 * the arena until the blob owns its bytes. */
static sb32 ndsBattlePackResidencyStep(void)
{
    u32 want;

    /* P2-3f9. ASK BEFORE RESERVING, NOT AFTER. The carve is now a per-match
     * decision (see ndsR2BattlePackCarveWorthIt), and this function's lazy
     * `ndsR2AnimCacheArenaEnsure()` below is what takes it. Checking the arena's
     * RawOnly flag alone is NOT enough here: on the first step the arena does
     * not exist yet, so RawOnly reads FALSE, Ensure would build a raw-only
     * arena, and this body would then stream 287,904 bytes of blob into a
     * 163,840-byte block. Same predicate, asked first, and the load state
     * latches so the question is answered once per scene. */
    if (ndsR2BattlePackCarveWorthIt() == FALSE)
    {
        if (sNdsBattlePackLoadState != NDS_BATTLEPACK_LOAD_FAILED)
        {
            ndsBattlePackResidencyFail();
        }
        return FALSE;
    }
    /* A setup-only arena has no carved BattlePack region.  This can only be
     * reached if a caller starts the full match warm walk in the same scene;
     * fail the optional pack open rather than aliasing raw animation entries.
     * A normal scene transition invalidates the arena and restores IDLE. */
    if (sNdsR2AnimCacheArenaRawOnly != FALSE)
    {
        if (sNdsBattlePackLoadState != NDS_BATTLEPACK_LOAD_FAILED)
        {
            ndsBattlePackResidencyFail();
        }
        return FALSE;
    }
    if ((sNdsBattlePackLoadState == NDS_BATTLEPACK_LOAD_DONE) ||
        (sNdsBattlePackLoadState == NDS_BATTLEPACK_LOAD_FAILED))
    {
        return FALSE;
    }
    if (sNdsBattlePackLoadState == NDS_BATTLEPACK_LOAD_IDLE)
    {
        u32 head[NDS_BATTLEPACK_HEADER_WORDS];
        u32 total;
        uintptr_t base;

        /* The reserve exists only inside a live arena, so take ownership first.
         * This is also where the lazy reservation happens -- after source fighter
         * setup, at the first preload step, never before battle allocations. */
        if (ndsR2AnimCacheArenaEnsure() == FALSE)
        {
            ndsBattlePackResidencyFail();
            return FALSE;
        }
        if (ndsRelocAssetReadRawRange(NDS_BATTLEPACK_PATH, 0u, head,
                                      (u32)sizeof(head)) == FALSE)
        {
            ndsBattlePackResidencyFail();
            return FALSE;
        }
        /* head[2] is blob_bytes. Magic and version are re-checked by
         * ndsBattlePackAdopt against the bytes that actually landed; all this
         * needs is a length that fits the carved reserve. A blob larger than
         * the build-time NDS_R2_BATTLEPACK_BLOB_BYTES the reserve was sized
         * from means the NitroFS payload and the ARM9 image disagree, which is
         * a build defect and is refused rather than truncated. */
        total = head[2];
        if ((total < (u32)sizeof(head)) ||
            (total > (NDS_BATTLEPACK_RESERVE_BYTES -
                      NDS_BATTLEPACK_LINE_BYTES)))
        {
            ndsBattlePackResidencyFail();
            return FALSE;
        }
        base = ((uintptr_t)sNdsR2AnimCacheArena +
                (NDS_BATTLEPACK_LINE_BYTES - 1u)) &
               ~(uintptr_t)(NDS_BATTLEPACK_LINE_BYTES - 1u);
        sNdsBattlePackDst = (u8 *)base;
        sNdsBattlePackTotal = total;
        /* From zero, not from sizeof(head): re-reading the 48 header bytes
         * through the same path costs nothing and keeps the CPU from having
         * written any byte the file read may later invalidate a line under. */
        sNdsBattlePackCursor = 0u;
        sNdsBattlePackLoadState = NDS_BATTLEPACK_LOAD_STREAMING;
    }
    want = sNdsBattlePackTotal - sNdsBattlePackCursor;
    if (want > NDS_BATTLEPACK_STEP_BYTES)
    {
        want = NDS_BATTLEPACK_STEP_BYTES;
    }
    if (ndsRelocAssetReadRawRange(NDS_BATTLEPACK_PATH, sNdsBattlePackCursor,
                                  &sNdsBattlePackDst[sNdsBattlePackCursor],
                                  want) == FALSE)
    {
        ndsBattlePackResidencyFail();
        return FALSE;
    }
    sNdsBattlePackCursor += want;
    gNdsBattlePackLoadSteps++;
    if (sNdsBattlePackCursor < sNdsBattlePackTotal)
    {
        return TRUE;
    }
    if (ndsBattlePackAdopt(sNdsBattlePackDst, sNdsBattlePackTotal) == FALSE)
    {
        ndsBattlePackResidencyFail();
        return FALSE;
    }
    sNdsBattlePackLoadState = NDS_BATTLEPACK_LOAD_DONE;
    gNdsBattlePackResidentBytes = sNdsBattlePackTotal;
    return FALSE;
}


/* An exact index over the entries (P2-2p8, 2026-09-29): a low-byte hint
 * answered a hit, but every miss scanned all entries, and most finds miss --
 * the preload steps ask after clips not yet held (1,348 finds a gate match at
 * 872 ticks each). Open addressing on the asset id, 512 slots for at most 256
 * entries, slot = entry index + 1 (0 empty), kept in step with every store,
 * removal and reset. Entries are unique per asset, so it returns the entry a
 * scan would. (The hint-and-scan arm and its A/B word left 2026-10-18.) */

static inline u32 ndsR2AnimCacheIndexHome(u32 asset_id)
{
    return (asset_id * 0x9E3779B1u) >> 23;
}

static u32 ndsR2AnimCacheIndexSlot(u32 asset_id)
{
    u32 slot = ndsR2AnimCacheIndexHome(asset_id);
    u32 probes;

    for (probes = 0u; probes < NDS_R2_ANIM_CACHE_INDEX_SLOTS; probes++)
    {
        const u32 entry = sNdsR2AnimCacheIndex[slot];

        if (entry == 0u)
        {
            break;
        }
        if ((entry <= sNdsR2AnimCacheCount) &&
            (sNdsR2AnimCache[entry - 1u].asset_id == asset_id))
        {
            return slot;
        }
        slot = (slot + 1u) & (NDS_R2_ANIM_CACHE_INDEX_SLOTS - 1u);
    }
    return NDS_R2_ANIM_CACHE_INDEX_NONE;
}

static void ndsR2AnimCacheIndexInsert(u32 asset_id, u32 index)
{
    u32 slot = ndsR2AnimCacheIndexHome(asset_id);

    while (sNdsR2AnimCacheIndex[slot] != 0u)
    {
        slot = (slot + 1u) & (NDS_R2_ANIM_CACHE_INDEX_SLOTS - 1u);
    }
    sNdsR2AnimCacheIndex[slot] = (u16)(index + 1u);
}

/* Linear probing's deletion: empty the slot, then pull back every later
 * member of the run whose home does not lie cyclically in (hole, it]. */
static void ndsR2AnimCacheIndexDelete(u32 hole)
{
    u32 next = hole;

    sNdsR2AnimCacheIndex[hole] = 0u;
    for (;;)
    {
        u32 home;
        u32 entry;

        next = (next + 1u) & (NDS_R2_ANIM_CACHE_INDEX_SLOTS - 1u);
        entry = sNdsR2AnimCacheIndex[next];
        if (entry == 0u)
        {
            return;
        }
        home = ndsR2AnimCacheIndexHome(sNdsR2AnimCache[entry - 1u].asset_id);
        if ((hole <= next) ? ((hole < home) && (home <= next)) :
                             ((hole < home) || (home <= next)))
        {
            continue;
        }
        sNdsR2AnimCacheIndex[hole] = (u16)entry;
        sNdsR2AnimCacheIndex[next] = 0u;
        hole = next;
    }
}

static NDSR2AnimCacheEntry *ndsR2AnimCacheFind(u32 asset_id)
{
    u32 slot;

    /* The ownership test belongs on the READ path too, not only where the arena is
     * reserved. Every entry's payload points into the reserved block, so once the
     * heap has been rewound a hit would hand back a pointer into memory the next
     * scene is already reusing -- silent corruption, and strictly worse than the
     * hang this whole change set exists to remove. A miss is free.
     *
     * Cheap by construction: two pointer compares against a bump cursor, and only
     * when the cache is non-empty. */
    ndsR2AnimCacheValidateGeneration();
    if (sNdsR2AnimCacheCount == 0u)
    {
        return NULL;
    }
    slot = ndsR2AnimCacheIndexSlot(asset_id);
    return (slot == NDS_R2_ANIM_CACHE_INDEX_NONE) ? NULL :
        &sNdsR2AnimCache[sNdsR2AnimCacheIndex[slot] - 1u];
}

static void ndsR2AnimCacheRemoveEntry(u32 index)
{
    if (index >= sNdsR2AnimCacheCount)
    {
        return;
    }
    if (sNdsR2AnimCache[index].prefetched != 0u)
    {
        NDS_DIAG(gNdsR2AnimPrefetchWasted++);
    }
    if (gNdsR2AnimCacheBytes >= sNdsR2AnimCache[index].size)
    {
        gNdsR2AnimCacheBytes -= sNdsR2AnimCache[index].size;
    }
    else
    {
        gNdsR2AnimCacheBytes = 0u;
    }
    {
        const u32 slot = ndsR2AnimCacheIndexSlot(sNdsR2AnimCache[index].asset_id);

        if (slot != NDS_R2_ANIM_CACHE_INDEX_NONE)
        {
            ndsR2AnimCacheIndexDelete(slot);
        }
    }
    sNdsR2AnimCacheCount--;
    if (index != sNdsR2AnimCacheCount)
    {
        /* The last entry moves down: its slot, found on its probe run by
         * the old position it names, now names `index`. */
        u32 slot = ndsR2AnimCacheIndexHome(
            sNdsR2AnimCache[sNdsR2AnimCacheCount].asset_id);

        while (sNdsR2AnimCacheIndex[slot] != 0u)
        {
            if (sNdsR2AnimCacheIndex[slot] == sNdsR2AnimCacheCount + 1u)
            {
                sNdsR2AnimCacheIndex[slot] = (u16)(index + 1u);
                break;
            }
            slot = (slot + 1u) & (NDS_R2_ANIM_CACHE_INDEX_SLOTS - 1u);
        }
        sNdsR2AnimCache[index] =
            sNdsR2AnimCache[sNdsR2AnimCacheCount];
        sNdsR2AnimCacheLo[index] = sNdsR2AnimCacheLo[sNdsR2AnimCacheCount];
        sNdsR2AnimCacheHi[index] = sNdsR2AnimCacheHi[sNdsR2AnimCacheCount];
        sNdsR2AnimCacheRef[index] = sNdsR2AnimCacheRef[sNdsR2AnimCacheCount];
    }
}

static void ndsR2AnimCacheEvictRawRange(u32 offset, u32 size)
{
    const u8 *write_start = &sNdsR2AnimCacheArena[offset];
    const u8 *write_end = write_start + size;
    u32 i = 0u;

    ndsR2AnimPinsRescueRange(write_start, write_end);

    {
        const u32 lo = ndsR2AnimExtentUnitFloor(write_start);
        const u32 hi = ndsR2AnimExtentUnitCeil(write_end);

        if ((lo != NDS_R2_ANIM_EXTENT_NONE) && (hi != NDS_R2_ANIM_EXTENT_NONE))
        {
            while (i < sNdsR2AnimCacheCount)
            {
                if ((sNdsR2AnimCacheLo[i] < hi) && (lo < sNdsR2AnimCacheHi[i]) &&
                    (sNdsR2AnimCacheHi[i] != NDS_R2_ANIM_EXTENT_NONE))
                {
                    ndsR2AnimCacheRemoveEntry(i);
                }
                else
                {
                    i++;
                }
            }
            return;
        }
    }
    while (i < sNdsR2AnimCacheCount)
    {
        const u8 *entry_start = sNdsR2AnimCache[i].payload;
        const u8 *entry_end = entry_start + sNdsR2AnimCache[i].size;

        if ((write_start < entry_end) && (entry_start < write_end))
        {
            ndsR2AnimCacheRemoveEntry(i);
        }
        else
        {
            i++;
        }
    }
}

/* Raw templates are copied into each fighter's authoritative heap before any
 * fixup or use, so the cache owns no live object pointers. Keep the arena as a
 * variable-size circular log: wrapping evicts only byte ranges the incoming
 * template overwrites instead of invalidating every unrelated clip at once. */
/* THE RING STEPS OVER CLIPS IN USE (P2-2p8, 2026-09-30).
 *
 * The ring overwrote whatever its cursor reached, so a clip every fighter
 * keeps returning to -- each one's Wait, fetched 34-55 times a match -- was
 * dropped on every lap and read from storage again (~5 times a match each,
 * ~31K ticks a read). A clip fetched since the cursor last passed it is now
 * stepped over once (its mark is cleared) and the new clip goes after it:
 * the CLOCK approximation of LRU, over variable sizes and without moving a
 * byte. The bytes skipped before such a clip stay unused until the next lap.
 * Same ROM, four lab stages: 4-8% fewer reads (replayed on the lab traces:
 * 7-12%; LRU 8-12%). Bounded: after NDS_R2_ANIM_CACHE_CLOCK_STEPS steps (each clears at
 * least one mark) the range is evicted as before. Only which clips stay
 * cached changes; a clip's bytes are the same wherever they come from. */
__attribute__((used)) volatile u32 gNdsR2AnimCacheClockSkips;
#define NDS_R2_ANIM_CACHE_CLOCK_STEPS 8u

/* `align` (a power of two, >= 16) aligns the returned ADDRESS: the arena's
 * base is only guaranteed NDS_RELOC_ALIGN_BYTES. */
static void *ndsR2AnimCacheRawRingAllocAligned(u32 size, u32 align)
{
    uintptr_t base;
    u32 aligned;
    u32 first;

    if (ndsR2AnimCacheArenaEnsure() == FALSE)
    {
        return NULL;
    }
    base = (uintptr_t)sNdsR2AnimCacheArena;
    first = (u32)(((base + align - 1u) & ~(uintptr_t)(align - 1u)) -
                  (uintptr_t)sNdsR2AnimCacheArena);
    if ((size == 0u) || (first > sNdsR2AnimCacheArenaBytes) ||
        (size > (sNdsR2AnimCacheArenaBytes - first)))
    {
        gNdsR2AnimCacheArenaOverflows++;
        gNdsR2AnimCacheArenaOverflowLastSize = size;
        gNdsR2AnimCacheArenaOverflowLastUsed = sNdsR2AnimCacheArenaUsed;
        return NULL;
    }
    aligned = (u32)(((base + sNdsR2AnimCacheArenaUsed + align - 1u) &
                     ~(uintptr_t)(align - 1u)) - base);
    if ((aligned > sNdsR2AnimCacheArenaBytes) ||
        (size > (sNdsR2AnimCacheArenaBytes - aligned)))
    {
        aligned = first;
        gNdsR2AnimCacheRawRecycles++;
    }
    {
        u32 steps;

        for (steps = 0u; steps < NDS_R2_ANIM_CACHE_CLOCK_STEPS; steps++)
        {
            const u8 *lo = sNdsR2AnimCacheArena + aligned;
            const u8 *hi = lo + size;
            const u8 *past = NULL;
            u32 i;
            const u32 ulo = ndsR2AnimExtentUnitFloor(lo);
            const u32 uhi = ndsR2AnimExtentUnitCeil(hi);

            if ((ulo != NDS_R2_ANIM_EXTENT_NONE) &&
                (uhi != NDS_R2_ANIM_EXTENT_NONE))
            {
                for (i = 0u; i < sNdsR2AnimCacheCount; i++)
                {
                    if ((sNdsR2AnimCacheRef[i] != 0u) &&
                        (sNdsR2AnimCacheLo[i] < uhi) &&
                        (ulo < sNdsR2AnimCacheHi[i]) &&
                        (sNdsR2AnimCacheHi[i] != NDS_R2_ANIM_EXTENT_NONE))
                    {
                        NDSR2AnimCacheEntry *e = &sNdsR2AnimCache[i];
                        const u8 *t = (const u8 *)e->payload + e->size;

                        sNdsR2AnimCacheRef[i] = 0u;
                        e->referenced = 0u;
                        if ((past == NULL) || (t > past))
                        {
                            past = t;
                        }
                    }
                }
            }
            else for (i = 0u; i < sNdsR2AnimCacheCount; i++)
            {
                NDSR2AnimCacheEntry *e = &sNdsR2AnimCache[i];
                const u8 *s = (const u8 *)e->payload;
                const u8 *t = s + e->size;

                if ((e->referenced != 0u) && (s < hi) && (lo < t))
                {
                    e->referenced = 0u;
                    sNdsR2AnimCacheRef[i] = 0u;
                    if ((past == NULL) || (t > past))
                    {
                        past = t;
                    }
                }
            }
            if (past == NULL)
            {
                break;
            }
            NDS_DIAG(gNdsR2AnimCacheClockSkips++);
            aligned = (u32)((((uintptr_t)past + align - 1u) &
                             ~(uintptr_t)(align - 1u)) - base);
            if ((aligned > sNdsR2AnimCacheArenaBytes) ||
                (size > (sNdsR2AnimCacheArenaBytes - aligned)))
            {
                aligned = first;
                gNdsR2AnimCacheRawRecycles++;
            }
        }
    }
    ndsR2AnimPrefetchRetire(&sNdsR2AnimCacheArena[aligned],
                            &sNdsR2AnimCacheArena[aligned] + size);
    ndsR2AnimCacheEvictRawRange(aligned, size);
    sNdsR2AnimCacheArenaUsed = aligned + size;
    gNdsR2AnimCacheArenaUsedBytes = sNdsR2AnimCacheArenaUsed;
    return &sNdsR2AnimCacheArena[aligned];
}

static void *ndsR2AnimCacheRawRingAlloc(u32 size)
{
    return ndsR2AnimCacheRawRingAllocAligned(size, 16u);
}

/* Called with the payload already byte-swapped and not yet fixed up. */
static void ndsR2AnimCacheStore(u32 asset_id, const void *data, u32 size,
                                const NDSRelocAssetHeader *header,
                                u32 ready_kind)
{
    NDSR2AnimCacheEntry *entry;
    void *payload;

    if ((data == NULL) || (size == 0u) || (header == NULL))
    {
        gNdsR2AnimCacheRejects++;
        return;
    }
    if (sNdsR2AnimCacheCount >= NDS_R2_ANIM_CACHE_ENTRIES)
    {
        if ((ndsR2AnimCacheArenaEnsure() != FALSE) &&
            (sNdsR2AnimCacheArenaRawOnly != FALSE))
        {
            /* A full metadata table can occur before the byte ring wraps when
             * clips are tiny. Evict one template; payload ownership is still
             * governed by the ring range and no live object points at it. */
            ndsR2AnimCacheRemoveEntry(0u);
        }
        else
        {
            gNdsR2AnimCacheRejects++;
            return;
        }
    }
    payload = ((ndsR2AnimCacheArenaEnsure() != FALSE) &&
               (sNdsR2AnimCacheArenaRawOnly != FALSE)) ?
        ndsR2AnimCacheRawRingAlloc(size) : ndsR2AnimCacheArenaAlloc(size);
    if (payload == NULL)
    {
        ndsR2AnimNoteRejected(asset_id, size);
        gNdsR2AnimCacheRejects++;
        return;
    }
    memcpy(payload, data, size);
    entry = &sNdsR2AnimCache[sNdsR2AnimCacheCount++];
    entry->asset_id = asset_id;
    entry->referenced = 0u;
    entry->prefetched = 0u;
    ndsR2AnimCacheIndexInsert(asset_id, sNdsR2AnimCacheCount - 1u);
    entry->size = size;
    entry->payload = payload;
    ndsR2AnimCacheSetExtent(sNdsR2AnimCacheCount - 1u);
    entry->header = *header;
    /* Written unconditionally, and that is not defensive tidiness: entries are
     * reused across a scene rewind, so leaving this field alone would let a slot
     * keep a stale TRUE from a previous match's asset and skip a transform that
     * had never been applied to these bytes. */
    if (ready_kind == NDS_R2_ANIM_CACHE_READY_STREAM)
    {
        entry->aobj16_ready = NDS_R2_ANIM_CACHE_READY_STREAM;
    }
    else
    {
        entry->aobj16_ready =
            (ndsR2AnimPrebakeAObj16(asset_id, payload, size, header) != FALSE)
                ? NDS_R2_ANIM_CACHE_READY_PREBAKE :
                  NDS_R2_ANIM_CACHE_READY_RAW;
    }
    gNdsR2AnimCacheFills++;
    gNdsR2AnimCacheBytes += size;
    if (asset_id < (NDS_R2_ANIM_STORED_BITS * 32u))
    {
        u32 word = asset_id >> 5;
        u32 bit = 1u << (asset_id & 31u);

        if ((sNdsR2AnimStoredThisMatch[word] & bit) != 0u)
        {
            NDS_DIAG(gNdsR2AnimCacheRestores++);
        }
        sNdsR2AnimStoredThisMatch[word] |= bit;
    }
}

/* A MISS READ STRAIGHT INTO THE RING (P2-2p8, 2026-09-30).
 *
 * The stream miss read a clip into the fighter's figatree heap, copied it
 * into the ring as a template, registered the heap as a loaded file and
 * returned the heap -- and the next fetch of the clip was a zero-copy hit on
 * the ring's copy. This reads the clip's whole sectors into the ring in one
 * storage command (ndsRelocAssetReadFighterStreamClipSpan), moves the clip to
 * the start of its slot, gives back the tail sector's spare bytes, and makes
 * the entry the READY_STREAM template the store would have made; the caller
 * then pins it exactly as it pins a hit. The heap copy, its registration and
 * the second storage request go; the bytes the parser reads are the pack's
 * own either way. Any refusal returns NULL before an entry exists and the
 * established miss path runs. */
__attribute__((used)) volatile u32 gNdsR2AnimDirectReads;
__attribute__((used)) volatile u32 gNdsR2AnimDirectRefusals;

/* A READY_STREAM entry for `size` clip bytes at `payload` in the ring. */
static NDSR2AnimCacheEntry *ndsR2AnimCacheAddStreamEntry(u32 asset_id,
                                                         u8 *payload, u32 size)
{
    NDSR2AnimCacheEntry *entry;

    if (sNdsR2AnimCacheCount >= NDS_R2_ANIM_CACHE_ENTRIES)
    {
        /* As the store does: a full table before the ring wraps. */
        ndsR2AnimCacheRemoveEntry(0u);
    }
    entry = &sNdsR2AnimCache[sNdsR2AnimCacheCount++];
    entry->asset_id = asset_id;
    entry->referenced = 0u;
    entry->prefetched = 0u;
    ndsR2AnimCacheIndexInsert(asset_id, sNdsR2AnimCacheCount - 1u);
    entry->size = size;
    entry->payload = payload;
    ndsR2AnimCacheSetExtent(sNdsR2AnimCacheCount - 1u);
    memset(&entry->header, 0, sizeof(entry->header));
    entry->header.file_id = asset_id;
    entry->header.data_size = size;
    entry->header.reloc_intern_offset = 0xffffu;
    entry->header.reloc_extern_offset = 0xffffu;
    entry->aobj16_ready = NDS_R2_ANIM_CACHE_READY_STREAM;
    gNdsR2AnimCacheFills++;
    gNdsR2AnimCacheBytes += size;
    if (asset_id < (NDS_R2_ANIM_STORED_BITS * 32u))
    {
        u32 word = asset_id >> 5;
        u32 bit = 1u << (asset_id & 31u);

        if ((sNdsR2AnimStoredThisMatch[word] & bit) != 0u)
        {
            NDS_DIAG(gNdsR2AnimCacheRestores++);
        }
        sNdsR2AnimStoredThisMatch[word] |= bit;
    }
    return entry;
}

static const NDSR2AnimCacheEntry *ndsR2AnimDirectReadEntry(u32 asset_id)
{
    u8 *slot;
    u32 head;
    u32 size;
    u32 span;

    if ((ndsRelocAssetFighterStreamClipSpan(asset_id, &head, &size, &span,
                                            NULL) == FALSE) ||
        (ndsR2AnimCacheArenaEnsure() == FALSE) ||
        (sNdsR2AnimCacheArenaRawOnly == FALSE))
    {
        NDS_DIAG(gNdsR2AnimDirectRefusals++);
        return NULL;
    }
    slot = (u8 *)ndsR2AnimCacheRawRingAllocAligned(span, 32u);
    if ((slot == NULL) ||
        (ndsRelocAssetReadFighterStreamClipSpan(asset_id, slot, span) ==
         FALSE))
    {
        /* The slot's range was evicted and holds nothing: no entry names it. */
        NDS_DIAG(gNdsR2AnimDirectRefusals++);
        return NULL;
    }
    if (head != 0u)
    {
        memmove(slot, slot + head, size);
    }
    sNdsR2AnimCacheArenaUsed = (u32)(slot - sNdsR2AnimCacheArena) + size;
    gNdsR2AnimCacheArenaUsedBytes = sNdsR2AnimCacheArenaUsed;
    return ndsR2AnimCacheAddStreamEntry(asset_id, slot, size);
}

/* THE MOTION PREFETCH (P2-2p8, 2026-09-30).
 *
 * A clip read blocks the ARM9 for the whole ARM7 round trip (~21K ticks
 * after ndsR2AnimDirectReadEntry), and most reads are a fighter's FIRST use
 * of a clip, which no eviction policy can avoid. What follows a clip is
 * predictable: the table (scripts/motion/generate_clip_successors.py, from
 * lab clip traces) names the two clips that most often followed each one in
 * the same fighter's fetches. An installed clip is queued; in the frame's
 * idle time before its presentation VBlank (ndsR2AnimPrefetchIdle, called
 * from the VBlank wait) its successors are read into ring slots
 * ASYNCHRONOUSLY (ndsAudioStorageReadAsync) and landed as ordinary
 * READY_STREAM entries. A fetch of a clip still in flight waits for that read
 * instead of starting another. Issued at the install instead, the reads
 * queued the next blocking read behind them on the ARM7 and the issue and
 * landing added ~6K ticks a status change: blocking reads -55%, P95 +5..16K
 * (same ROM, seven stages). In the idle time both costs are off the frame.
 *
 * Nothing the game computes depends on it: a clip's bytes are the pack's
 * whether read now, earlier or never. An ARM7 write must never land in bytes
 * something else owns, so every path that takes ring bytes first waits for a
 * read covering them (ndsR2AnimPrefetchRetire: the ring allocation, the
 * elastic give-back, the arena drop) and the heap reset waits for all of them
 * (ndsR2AnimPrefetchDrain, battleship_sys_malloc.c). Two requests at most in
 * flight: the ARM7 mailbox holds 32 (FGM fills use 16, a sync read and its
 * resends 4). */
#include <nds/nds_audio_storage.h>
#include <nds/generated/nds_clip_successors.generated.h>

#define NDS_R2_ANIM_PREFETCH_SLOTS 2u
typedef struct NDSR2AnimPrefetch {
    /* One D-cache line of its own: the ARM7 writes its reply here. */
    NdsAudioStorageRequest request;
    u8 *slot;
    u32 asset_id;
    u32 head;
    u32 size;
    u32 span;
    u32 generation;
    u32 busy;
} NDSR2AnimPrefetch;
static NDSR2AnimPrefetch sNdsR2AnimPrefetchSlots[NDS_R2_ANIM_PREFETCH_SLOTS];
__attribute__((used)) volatile u32 gNdsR2AnimPrefetchIssued;
__attribute__((used)) volatile u32 gNdsR2AnimPrefetchLanded;
__attribute__((used)) volatile u32 gNdsR2AnimPrefetchWaits;
__attribute__((used)) volatile u32 gNdsR2AnimPrefetchRetired;
__attribute__((used)) volatile u32 gNdsR2AnimPrefetchRefused;
/* Refusals by cause: [0] no stream span, [1] no raw ring, [2] ring slot,
 * [3] storage request. */
__attribute__((used)) volatile u32 gNdsR2AnimPrefetchRefusedWhy[4];

/* Spin until the ARM7 has finished with this request (landed or failed). */
static s32 ndsR2AnimPrefetchWait(NDSR2AnimPrefetch *p)
{
    s32 r;

    while ((r = ndsAudioStorageReadAsyncPoll(&p->request)) == 0)
    {
    }
    return r;
}

/* Every in-flight read whose slot meets [lo, hi) finishes and is discarded:
 * those bytes are about to belong to someone else. */
static void ndsR2AnimPrefetchRetire(const u8 *lo, const u8 *hi)
{
    u32 i;

    for (i = 0u; i < NDS_R2_ANIM_PREFETCH_SLOTS; i++)
    {
        NDSR2AnimPrefetch *p = &sNdsR2AnimPrefetchSlots[i];

        if ((p->busy != 0u) && (p->slot < hi) && (lo < p->slot + p->span))
        {
            (void)ndsR2AnimPrefetchWait(p);
            p->busy = 0u;
            NDS_DIAG(gNdsR2AnimPrefetchRetired++);
        }
    }
}

void ndsR2AnimPrefetchDrain(void)
{
    u32 i;

    for (i = 0u; i < NDS_R2_ANIM_PREFETCH_SLOTS; i++)
    {
        NDSR2AnimPrefetch *p = &sNdsR2AnimPrefetchSlots[i];

        if (p->busy != 0u)
        {
            (void)ndsR2AnimPrefetchWait(p);
            p->busy = 0u;
            NDS_DIAG(gNdsR2AnimPrefetchRetired++);
        }
    }
}

/* Land every finished read as a cache entry; a read of `wanted` still in
 * flight is waited for (the fetch needs it now). */
static void ndsR2AnimPrefetchPoll(u32 wanted)
{
    u32 i;

    for (i = 0u; i < NDS_R2_ANIM_PREFETCH_SLOTS; i++)
    {
        NDSR2AnimPrefetch *p = &sNdsR2AnimPrefetchSlots[i];
        s32 r;

        if (p->busy == 0u)
        {
            continue;
        }
        r = ndsAudioStorageReadAsyncPoll(&p->request);
        if ((r == 0) && (p->asset_id == wanted))
        {
            r = ndsR2AnimPrefetchWait(p);
            NDS_DIAG(gNdsR2AnimPrefetchWaits++);
        }
        if (r == 0)
        {
            continue;
        }
        p->busy = 0u;
        if ((r < 0) || (p->generation != gNdsTaskmanHeapGeneration) ||
            (ndsR2AnimCacheArenaStillOwned() == FALSE) ||
            (p->slot < sNdsR2AnimCacheArena) ||
            ((p->slot + p->span) >
             (sNdsR2AnimCacheArena + sNdsR2AnimCacheArenaBytes)) ||
            (ndsR2AnimCacheFind(p->asset_id) != NULL))
        {
            NDS_DIAG(gNdsR2AnimPrefetchRetired++);
            continue;
        }
        if (p->head != 0u)
        {
            memmove(p->slot, p->slot + p->head, p->size);
        }
        if (sNdsR2AnimCacheArenaUsed ==
            (u32)(p->slot - sNdsR2AnimCacheArena) + p->span)
        {
            /* Nothing was placed after it: the spare sector bytes go back. */
            sNdsR2AnimCacheArenaUsed =
                (u32)(p->slot - sNdsR2AnimCacheArena) + p->size;
            gNdsR2AnimCacheArenaUsedBytes = sNdsR2AnimCacheArenaUsed;
        }
        ndsR2AnimCacheAddStreamEntry(p->asset_id, p->slot, p->size)
            ->prefetched = 1u;
        NDS_DIAG(gNdsR2AnimPrefetchLanded++);
    }
}

/* Installed clips whose successors the idle time has not read yet. */
#define NDS_R2_ANIM_PREFETCH_QUEUE 8u
static u16 sNdsR2AnimPrefetchQueue[NDS_R2_ANIM_PREFETCH_QUEUE];
static u32 sNdsR2AnimPrefetchQueueCount;
/* Idle-time budget: stop this many ticks before the presentation VBlank
 * (the longest step -- a ring allocation or an 11.5 KB landing -- is well
 * under it), and start a read only with time left for it to land: one left
 * in flight at the frame's end queues the next frame's first blocking read
 * behind it on the ARM7. */
#define NDS_R2_ANIM_PREFETCH_IDLE_MARGIN 40000u
#define NDS_R2_ANIM_PREFETCH_ISSUE_MARGIN 90000u
__attribute__((used)) volatile u32 gNdsR2AnimPrefetchIdleCalls;
__attribute__((used)) volatile u32 gNdsR2AnimPrefetchIdleStops;
u32 ndsPlatformTicksToPresentVBlank(void);

/* LEARNED SUCCESSORS (2026-10-13). The measured table names the cast's
 * clips only; a P4 content's (and Kirby's powers', ids past the cast's) never
 * had a successor, so every one of their misses blocked: four Meta Knights
 * read storage on 240 of 413 fetches, over 95 clip ids. Per fighter (its
 * figatree heap), the clip fetched after each clip, direct-mapped by id; a
 * clip the measured table lacks prefetches the one that followed it last
 * time. A wrong guess costs ring bytes and an ARM7 read, never a different
 * frame -- a clip's bytes are the pack's wherever they come from. */
#define NDS_R2_ANIM_LEARN_WAYS 256u
#define NDS_R2_ANIM_LEARN_FIGHTERS 4u
static u16 sNdsR2AnimLearnKey[NDS_R2_ANIM_LEARN_WAYS];
static u16 sNdsR2AnimLearnNext[NDS_R2_ANIM_LEARN_WAYS];
static const void *sNdsR2AnimLearnHeap[NDS_R2_ANIM_LEARN_FIGHTERS];
static u16 sNdsR2AnimLearnLast[NDS_R2_ANIM_LEARN_FIGHTERS];
static u32 sNdsR2AnimLearnVictim;
__attribute__((used)) volatile u32 gNdsR2AnimLearnIssued;

static void ndsR2AnimLearnNote(const void *heap, u32 asset_id)
{
    u32 f;

    if (asset_id > 0xffffu)
    {
        return;
    }
    for (f = 0u; f < NDS_R2_ANIM_LEARN_FIGHTERS; f++)
    {
        if (sNdsR2AnimLearnHeap[f] == heap)
        {
            break;
        }
    }
    if (f == NDS_R2_ANIM_LEARN_FIGHTERS)
    {
        f = sNdsR2AnimLearnVictim;
        sNdsR2AnimLearnVictim = (f + 1u) % NDS_R2_ANIM_LEARN_FIGHTERS;
        sNdsR2AnimLearnHeap[f] = heap;
        sNdsR2AnimLearnLast[f] = 0u;
    }
    if ((sNdsR2AnimLearnLast[f] != 0u) &&
        ((u32)sNdsR2AnimLearnLast[f] != asset_id))
    {
        const u32 way = (u32)sNdsR2AnimLearnLast[f] &
            (NDS_R2_ANIM_LEARN_WAYS - 1u);

        sNdsR2AnimLearnKey[way] = sNdsR2AnimLearnLast[f];
        sNdsR2AnimLearnNext[way] = (u16)asset_id;
    }
    sNdsR2AnimLearnLast[f] = (u16)asset_id;
}

static void ndsR2AnimPrefetchNote(u32 asset_id)
{
    u32 i;

    if (sNdsR2AnimCacheArenaElastic == FALSE)
    {
        return;
    }
    for (i = 0u; i < sNdsR2AnimPrefetchQueueCount; i++)
    {
        if (sNdsR2AnimPrefetchQueue[i] == asset_id)
        {
            return;
        }
    }
    if (sNdsR2AnimPrefetchQueueCount == NDS_R2_ANIM_PREFETCH_QUEUE)
    {
        /* The oldest install is the least likely to matter now. */
        memmove(&sNdsR2AnimPrefetchQueue[0], &sNdsR2AnimPrefetchQueue[1],
                (NDS_R2_ANIM_PREFETCH_QUEUE - 1u) * sizeof(u16));
        sNdsR2AnimPrefetchQueueCount--;
    }
    sNdsR2AnimPrefetchQueue[sNdsR2AnimPrefetchQueueCount++] = (u16)asset_id;
}

/* The row of `asset_id` in the successor table, or NDS_CLIP_SUCCESSOR_ROWS. */
static u32 ndsR2AnimPrefetchRow(u32 asset_id)
{
    u32 lo = 0u;
    u32 hi = NDS_CLIP_SUCCESSOR_ROWS;

    while (lo < hi)
    {
        const u32 mid = (lo + hi) >> 1;

        if (sNdsClipSuccessorKey[mid] < asset_id)
        {
            lo = mid + 1u;
        }
        else
        {
            hi = mid;
        }
    }
    return ((lo < NDS_CLIP_SUCCESSOR_ROWS) &&
            (sNdsClipSuccessorKey[lo] == asset_id)) ?
        lo : NDS_CLIP_SUCCESSOR_ROWS;
}

static sb32 ndsR2AnimPrefetchInFlight(u32 asset_id)
{
    u32 i;

    for (i = 0u; i < NDS_R2_ANIM_PREFETCH_SLOTS; i++)
    {
        if ((sNdsR2AnimPrefetchSlots[i].busy != 0u) &&
            (sNdsR2AnimPrefetchSlots[i].asset_id == asset_id))
        {
            return TRUE;
        }
    }
    return FALSE;
}

/* The next clip worth reading: the oldest queued install's first successor
 * that is neither cached nor in flight. Installs with none left leave the
 * queue. 0 = nothing to read. */
static u32 ndsR2AnimPrefetchNextCandidate(void)
{
    while (sNdsR2AnimPrefetchQueueCount != 0u)
    {
        const u32 row = ndsR2AnimPrefetchRow(sNdsR2AnimPrefetchQueue[0]);
        u32 k;

        if (row != NDS_CLIP_SUCCESSOR_ROWS)
        {
            for (k = 0u; k < 2u; k++)
            {
                const u32 next = sNdsClipSuccessorNext[row][k];

                if ((next != 0u) && (ndsR2AnimCacheFind(next) == NULL) &&
                    (ndsR2AnimPrefetchInFlight(next) == FALSE))
                {
                    return next;
                }
            }
        }
        else
        {
            const u32 clip = sNdsR2AnimPrefetchQueue[0];
            const u32 way = clip & (NDS_R2_ANIM_LEARN_WAYS - 1u);
            const u32 next = sNdsR2AnimLearnNext[way];

            if (((u32)sNdsR2AnimLearnKey[way] == clip) && (next != 0u) &&
                (ndsR2AnimCacheFind(next) == NULL) &&
                (ndsR2AnimPrefetchInFlight(next) == FALSE))
            {
                NDS_DIAG(gNdsR2AnimLearnIssued++);
                return next;
            }
        }
        memmove(&sNdsR2AnimPrefetchQueue[0], &sNdsR2AnimPrefetchQueue[1],
                (sNdsR2AnimPrefetchQueueCount - 1u) * sizeof(u16));
        sNdsR2AnimPrefetchQueueCount--;
    }
    return 0u;
}

/* One asynchronous read of `next` into a free slot. FALSE: nothing issued. */
static sb32 ndsR2AnimPrefetchIssue(u32 next)
{
    NDSR2AnimPrefetch *p = NULL;
    u32 head;
    u32 size;
    u32 span;
    u32 sector;
    u32 i;
    u8 *slot;

    for (i = 0u; i < NDS_R2_ANIM_PREFETCH_SLOTS; i++)
    {
        if (sNdsR2AnimPrefetchSlots[i].busy == 0u)
        {
            p = &sNdsR2AnimPrefetchSlots[i];
            break;
        }
    }
    if (p == NULL)
    {
        return FALSE;
    }
    if (ndsRelocAssetFighterStreamClipSpan(next, &head, &size, &span,
                                           &sector) == FALSE)
    {
        NDS_DIAG(gNdsR2AnimPrefetchRefused++);
        NDS_DIAG(gNdsR2AnimPrefetchRefusedWhy[0]++);
        return FALSE;
    }
    if ((ndsR2AnimCacheArenaEnsure() == FALSE) ||
        (sNdsR2AnimCacheArenaRawOnly == FALSE))
    {
        NDS_DIAG(gNdsR2AnimPrefetchRefused++);
        NDS_DIAG(gNdsR2AnimPrefetchRefusedWhy[1]++);
        return FALSE;
    }
    slot = (u8 *)ndsR2AnimCacheRawRingAllocAligned(span, 32u);
    if (slot == NULL)
    {
        NDS_DIAG(gNdsR2AnimPrefetchRefused++);
        NDS_DIAG(gNdsR2AnimPrefetchRefusedWhy[2]++);
        return FALSE;
    }
    if (ndsAudioStorageReadAsync(&p->request, sector, slot, span) == 0)
    {
        /* The reserved bytes hold nothing and no entry names them. */
        NDS_DIAG(gNdsR2AnimPrefetchRefused++);
        NDS_DIAG(gNdsR2AnimPrefetchRefusedWhy[3]++);
        return FALSE;
    }
    p->slot = slot;
    p->asset_id = next;
    p->head = head;
    p->size = size;
    p->span = span;
    p->generation = gNdsTaskmanHeapGeneration;
    p->busy = 1u;
    NDS_DIAG(gNdsR2AnimPrefetchIssued++);
    return TRUE;
}

void ndsR2AnimPrefetchIdle(void)
{
    if ((sNdsR2AnimPrefetchQueueCount == 0u) &&
        (sNdsR2AnimPrefetchSlots[0].busy == 0u) &&
        (sNdsR2AnimPrefetchSlots[1].busy == 0u))
    {
        return;
    }
    NDS_DIAG(gNdsR2AnimPrefetchIdleCalls++);
    for (;;)
    {
        u32 next;
        sb32 busy;
        u32 i;

        if (ndsPlatformTicksToPresentVBlank() < NDS_R2_ANIM_PREFETCH_IDLE_MARGIN)
        {
            NDS_DIAG(gNdsR2AnimPrefetchIdleStops++);
            return;
        }
        ndsR2AnimPrefetchPoll(0u);
        next = (ndsPlatformTicksToPresentVBlank() >=
                NDS_R2_ANIM_PREFETCH_ISSUE_MARGIN) ?
            ndsR2AnimPrefetchNextCandidate() : 0u;
        if ((next != 0u) && (ndsR2AnimPrefetchIssue(next) != FALSE))
        {
            continue;
        }
        busy = FALSE;
        for (i = 0u; i < NDS_R2_ANIM_PREFETCH_SLOTS; i++)
        {
            busy |= (sNdsR2AnimPrefetchSlots[i].busy != 0u) ? TRUE : FALSE;
        }
        if ((busy == FALSE) && (next == 0u))
        {
            return;
        }
        if ((busy == FALSE) && (next != 0u))
        {
            /* Refused with every slot free: drop that install's turn. */
            if (sNdsR2AnimPrefetchQueueCount != 0u)
            {
                memmove(&sNdsR2AnimPrefetchQueue[0], &sNdsR2AnimPrefetchQueue[1],
                        (sNdsR2AnimPrefetchQueueCount - 1u) * sizeof(u16));
                sNdsR2AnimPrefetchQueueCount--;
            }
        }
    }
}


volatile u32 gNdsR2AnimWarmLoaded;
volatile u32 gNdsR2AnimWarmBytes;
volatile u32 gNdsR2AnimWarmFailed;
static u32 sNdsR204AnimWarmCursor;

/* R2-04's warm list was measured on the historical Mario/Fox gate. Once P2-3
 * admitted four distinct fighter kinds, blindly loading that same list became
 * actively harmful: an argmax Samus/Fox/Captain/Donkey match spent scarce raw
 * cache bytes on Mario clips that cannot execute, then rejected live Samus /
 * Captain / Donkey clips and fell back to NitroFS during gameplay.
 *
 * This is only a preload filter. It does not alter BattleShip's motion tables,
 * asset identity, force-load semantics, or the cache miss path. Unknown setup
 * state keeps the historical behaviour; once the battle roster is settled, a
 * Mario/Fox warm asset is useful only if that source fighter is actually in the
 * match. P2-3 fighters are still admitted on demand through the exact generic
 * loader until they earn their own measured warm set. */
static sb32 ndsR2AnimWarmAssetMatchesRoster(u32 asset_id)
{
    s32 i;
    u32 wanted_kind;
    u32 seen = 0u;
    u32 distinct = 0u;
    sb32 wanted_present = FALSE;

    if (gSCManagerBattleState == NULL)
    {
        return TRUE;
    }
    if ((asset_id >= NDS_RELOC_ASSET_MARIO_ANIM_WAIT) &&
        (asset_id <= NDS_RELOC_ASSET_MARIO_ANIM_FIRE_FLOWER_AIR))
    {
        wanted_kind = (u32)nFTKindMario;
    }
    else if ((asset_id >= NDS_RELOC_ASSET_FOX_ANIM_FIRST) &&
             (asset_id <= NDS_RELOC_ASSET_FOX_ANIM_LAST))
    {
        wanted_kind = (u32)nFTKindFox;
    }
    else
    {
        return TRUE;
    }

    for (i = 0; i < (s32)GMCOMMON_PLAYERS_MAX; i++)
    {
        u32 fkind;

        if (gSCManagerBattleState->players[i].pkind == nFTPlayerKindNot)
        {
            continue;
        }
        fkind = (u32)gSCManagerBattleState->players[i].fkind;
        if (fkind <= (u32)nFTKindPlayableEnd)
        {
            u32 mask = 1u << fkind;

            if ((seen & mask) == 0u)
            {
                seen |= mask;
                distinct++;
            }
        }
        if (fkind == wanted_kind)
        {
            wanted_present = TRUE;
        }
    }
    /* The historical list is a measured two-kind optimization, not a gameplay
     * requirement. At three or more distinct kinds it displaces the active
     * roster's own clips and turns future action changes into cartridge I/O.
     * Leave the whole raw cache demand-driven on that topology. */
    if (distinct > 2u)
    {
        return FALSE;
    }
    return wanted_present;
}

/* R2-06 E13. Move ndsRelocNormalizeFighterAObj16File off the gameplay frame.
 *
 * WHY THIS ONE. Cycle 107 attributed the load frame exactly
 * (task37_census.py --split-by-symbol ndsRelocFinalizeLoadedFile, 74 load frames
 * against 326 control, premium 650,610 cycles/frame): the reloc + copy family is
 * 23.6% of it and ndsRelocFinalizeLoadedFile alone is 10.0%, of which the AObj16
 * pass is the bulk -- 10,236,800 ticks a match against a 63,115,584 SINT
 * excursion, i.e. 16.2%, or ~29,000 per force-load frame. R2-06 E11's standing
 * rule is that a load-frame saving cannot be banked by making the work faster,
 * only by moving it off the frame, which is what cycle 105's arena fix did for
 * the cartridge read. This does it for the transform.
 *
 * WHY IT IS SOUND, and the two facts that make it so:
 *
 *   1. The transform is position-INDEPENDENT. The swap loop runs from
 *      `table_bytes` to the end, and the normalize walks each script; neither
 *      touches the pointer table, and every quantity either derives from is an
 *      OFFSET (`value - base`). Running it at warm time against the arena base
 *      therefore produces byte-identical script bytes to running it at load time
 *      against the caller's heap.
 *   2. Nothing runs between. ndsRelocFinalizeLoadedFile's order is internal
 *      fixups, then AObj16, then attributes/weapon/sprites/external -- so the
 *      only pass that can change the input is the internal fixup, and that is
 *      what this reproduces.
 *
 * The internal fixups have to run first because the AObj16 pass reads the table
 * as ABSOLUTE pointers, which only exist once they are written. They are an
 * intrusive linked list threaded through the slots themselves -- each raw word
 * is (next_slot_index << 16 | target_word_index) -- so applying them CONSUMES
 * the list. This therefore records every (offset, raw word) pair while walking
 * the list, applies, transforms, and writes the raw words back. The restore is
 * exact by construction and independent of where the slots sit, which is
 * deliberately stronger than restoring a table region and assuming no slot lives
 * past it.
 *
 * Every failure declines the asset with `aobj16_ready` 0 and leaves the payload
 * as it was, so a decline is a performance outcome and never a correctness one --
 * the load path then runs the pass exactly as it does today.
 *
 * Sized by measurement, not by fear: the first arm ran a 512-slot scratch and
 * gNdsR2AObj16PrebakeSlotsMax read 21 over the whole match, so 64 keeps a 3x
 * margin and hands 3,584 bytes back to a heap whose free-min sits 9,368 above
 * the anim cache's own KEEP_FREE reserve. A longer list declines the asset. */
#define NDS_R2_AOBJ16_PREBAKE_SLOTS_MAX 64u

typedef struct NDSR2AObj16PrebakeSlot {
    u32 offset;
    u32 word;
} NDSR2AObj16PrebakeSlot;

static NDSR2AObj16PrebakeSlot
    sNdsR2AObj16PrebakeSlots[NDS_R2_AOBJ16_PREBAKE_SLOTS_MAX];

volatile u32 gNdsR2AObj16PrebakeReady;
volatile u32 gNdsR2AObj16PrebakeSlotsMax;
volatile u32 gNdsR2AObj16PrebakeDeclineKind;
volatile u32 gNdsR2AObj16PrebakeDeclineList;
volatile u32 gNdsR2AObj16PrebakeDeclineFixup;
volatile u32 gNdsR2AObj16PrebakeDeclineFormat;
volatile u32 gNdsR2AObj16PrebakeSkips;

static void ndsR2AObj16PrebakeRestore(void *payload, u32 count)
{
    u32 i;

    for (i = 0u; i < count; i++)
    {
        ndsRelocWriteNative32((u8 *)payload + sNdsR2AObj16PrebakeSlots[i].offset,
                              sNdsR2AObj16PrebakeSlots[i].word);
    }
}

static sb32 ndsR2AnimPrebakeAObj16(u32 asset_id, void *payload, u32 size,
                                   const NDSRelocAssetHeader *header)
{
    NDSRelocLoadedFile view;
    u32 count = 0u;
    u32 guard;
    u16 slot_index;

    if ((payload == NULL) || (size == 0u) || (header == NULL) ||
        (ndsRelocIsFighterAObj16Asset(asset_id) == FALSE))
    {
        NDS_DIAG(gNdsR2AObj16PrebakeDeclineKind++);
        return FALSE;
    }

    slot_index = header->reloc_intern_offset;
    guard = (size / sizeof(u32)) + 1u;
    while (slot_index != 0xffffu)
    {
        u32 offset = (u32)slot_index * sizeof(u32);

        if ((guard == 0u) || ((offset + sizeof(u32)) > size) ||
            (count >= NDS_R2_AOBJ16_PREBAKE_SLOTS_MAX))
        {
            NDS_DIAG(gNdsR2AObj16PrebakeDeclineList++);
            return FALSE;
        }
        guard--;
        sNdsR2AObj16PrebakeSlots[count].offset = offset;
        sNdsR2AObj16PrebakeSlots[count].word =
            (u32)ndsRelocReadNative32((u8 *)payload + offset);
        slot_index = (u16)(sNdsR2AObj16PrebakeSlots[count].word >> 16);
        count++;
    }
    if (count > gNdsR2AObj16PrebakeSlotsMax)
    {
        gNdsR2AObj16PrebakeSlotsMax = count;
    }

    memset(&view, 0, sizeof(view));
    view.asset_id = asset_id;
    view.data = payload;
    view.data_size = size;
    view.reloc_intern_offset = header->reloc_intern_offset;
    view.reloc_extern_offset = header->reloc_extern_offset;

    if (ndsRelocApplyInternalPointerFixups(&view) == FALSE)
    {
        ndsR2AObj16PrebakeRestore(payload, count);
        NDS_DIAG(gNdsR2AObj16PrebakeDeclineFixup++);
        return FALSE;
    }
    if (ndsRelocNormalizeFighterAObj16File(&view) == FALSE)
    {
        ndsR2AObj16PrebakeRestore(payload, count);
        NDS_DIAG(gNdsR2AObj16PrebakeDeclineFormat++);
        return FALSE;
    }

    ndsR2AObj16PrebakeRestore(payload, count);
    NDS_DIAG(gNdsR2AObj16PrebakeReady++);
    return TRUE;
}

/* R2-04 E4. Task 75's absorption proper: make the match's animation streams
 * resident so no gameplay frame pays a NitroFS walk and a cartridge read for a
 * move.
 *
 * Deliberately does NOT go through ndsRelocRegisterLoadedFile. Registering 41
 * assets that all live at one scratch address would consume half of
 * NDS_RELOC_LOADED_FILE_CAPACITY (96) and leave every entry pointing at storage
 * this function owns. Instead each asset gets its own buffer and the word swap is
 * done here: ndsRelocApplyWordByteSwap is a plain big-endian-to-native pass over
 * data_size, and its only side effects are counters for the N64 logo and the
 * opening-room assets, neither of which a fighter animation can be. The cached
 * image is therefore identical to the one the miss path snapshots.
 *
 * Every failure degrades to the on-demand load. */
static void ndsR2AnimWarmLoadOne(u32 asset_id)
{
    NDSRelocAssetHeader header;
    size_t alloc_size;
    size_t loaded_size = 0u;
    void *payload;
    u32 words;
    u32 w;
    u32 stream_size = 0u;
    sb32 stream_ready = FALSE;

    if (ndsR2AnimCacheFind(asset_id) != NULL)
    {
        return;
    }
    /* Already resident in the pack. Warming it into the raw cache would spend a
     * NitroFS open and a second copy of bytes the acquisition path stopped
     * reading the moment the pack answered for this id. */
    if (ndsBattlePackFindFigatree(asset_id) != NULL)
    {
        return;
    }
    if (ndsR2AnimCacheArenaExhausted() != FALSE)
    {
        gNdsR2AnimWarmFailed++;
        return;
    }
    alloc_size = ndsR2AnimCachePayloadBytes(asset_id, &stream_ready, &stream_size);
    if ((alloc_size == 0u) ||
        (sNdsR2AnimCacheCount >= NDS_R2_ANIM_CACHE_ENTRIES))
    {
        gNdsR2AnimWarmFailed++;
        return;
    }
    payload = ndsR2AnimCacheArenaAlloc((u32)alloc_size);
    if (payload == NULL)
    {
        ndsR2AnimNoteRejected(asset_id, (u32)alloc_size);
        gNdsR2AnimWarmFailed++;
        return;
    }
    if (stream_ready != FALSE)
    {
        if (ndsRelocAssetLoadFighterStreamClip(asset_id, payload,
                                                &stream_size) == FALSE)
        {
            ndsR2AnimCacheArenaRelease(payload, (u32)alloc_size);
            gNdsR2AnimWarmFailed++;
            return;
        }
        memset(&header, 0, sizeof(header));
        header.file_id = asset_id;
        header.data_size = stream_size;
        header.reloc_intern_offset = 0xffffu;
        header.reloc_extern_offset = 0xffffu;
        loaded_size = stream_size;
    }
    else if (ndsRelocAssetLoadIntoZeroedHeap(asset_id, payload,
                                             NDS_RELOC_ALIGN_BYTES,
                                             &loaded_size, &header) == FALSE)
    {
        /* Used to return here having already taken the buffer, so a failed warm
         * load permanently consumed arena for nothing. */
        ndsR2AnimCacheArenaRelease(payload, (u32)alloc_size);
        gNdsR2AnimWarmFailed++;
        return;
    }
    if (stream_ready == FALSE)
    {
        /* K0 line 3 again: the warm loader swaps the same payload the miss path
         * would. It normally runs before GO, which is exactly why it is marked --
         * a warm step that slipped past GO would otherwise be invisible. */
        NDS_K0_MARK(gNdsK0AfterGoByteSwaps, asset_id);
        words = (u32)(loaded_size / sizeof(u32));
        for (w = 0u; w < words; w++)
        {
            void *word = (u8 *)payload + (w * sizeof(u32));

            ndsRelocWriteNative32(word, ndsRelocReadBe32(word));
        }
    }
    {
        NDSR2AnimCacheEntry *entry = &sNdsR2AnimCache[sNdsR2AnimCacheCount++];

        entry->asset_id = asset_id;
        entry->referenced = 0u;
        entry->prefetched = 0u;
        ndsR2AnimCacheIndexInsert(asset_id, sNdsR2AnimCacheCount - 1u);
        entry->size = (u32)loaded_size;
        entry->payload = payload;
        ndsR2AnimCacheSetExtent(sNdsR2AnimCacheCount - 1u);
        entry->header = header;
        if (stream_ready != FALSE)
        {
            entry->aobj16_ready = NDS_R2_ANIM_CACHE_READY_STREAM;
        }
        else
        {
            /* AFTER the word swap above and BEFORE anyone can copy this out: the
             * transform's input is the swapped, pre-fixup image, which is exactly
             * what the miss path snapshots too. */
            entry->aobj16_ready =
                (ndsR2AnimPrebakeAObj16(asset_id, payload, (u32)loaded_size,
                                        &header) != FALSE) ?
                    NDS_R2_ANIM_CACHE_READY_PREBAKE :
                    NDS_R2_ANIM_CACHE_READY_RAW;
        }
    }
    gNdsR2AnimWarmLoaded++;
    gNdsR2AnimWarmBytes += (u32)loaded_size;
    gNdsR2AnimCacheBytes += (u32)loaded_size;
}

/* Scene-setup form of the match warmer.  The CSS creates every demo fighter in
 * submotion 0 before applying the selected pose; Luigi/Donkey's submotion-0
 * clips are ordinary reloc animations and were the two remaining live-BGM
 * NitroFS reads after native-owner images moved to setup.  Reuse the battle
 * cache rather than inventing a CSS cache: a later force load then follows the
 * exact cache-hit path already verified for gameplay. */
s32 ndsR2AnimCachePreloadFighterFile(const void *file_id)
{
    u32 token;
    u32 asset_id;

    if (file_id == NULL)
    {
        return FALSE;
    }
    token = ndsRelocFileID(file_id);
    asset_id = ndsRelocAssetIDForToken(token);
    if ((asset_id == NDS_RELOC_ASSET_INVALID) ||
        (ndsRelocIsFighterAnimID(asset_id) == FALSE))
    {
        return FALSE;
    }

    ndsR2AnimCacheValidateGeneration();
    if ((ndsBattlePackFindFigatree(asset_id) != NULL) ||
        (ndsR2AnimCacheFind(asset_id) != NULL))
    {
        return TRUE;
    }
    if (ndsR2AnimCacheArenaEnsureSetup() == FALSE)
    {
        return FALSE;
    }
    ndsR2AnimWarmLoadOne(asset_id);
    return ((ndsBattlePackFindFigatree(asset_id) != NULL) ||
            (ndsR2AnimCacheFind(asset_id) != NULL)) ? TRUE : FALSE;
}

/* R2-04 E5. Arms the warm walk; it does not load anything itself.
 *
 * Historical E4 loaded all 41 in one call AFTER BGM was live and Boundary
 * refused the build on the ADPCM smoke: SeamMissCount 0 -> 1, ErrorStopCount
 * 0 -> 1, OverrunCount 0 -> 1, gNdsAudioBgmPlaying 1 -> 0 with StopCalls still
 * 0. That is why the loader remains chunked. Battle startup now arms this walk
 * and drains those chunks BEFORE mpCollisionSetPlayBGM, so the old audio budget
 * no longer limits total startup work and the visible countdown inherits no
 * warm-load I/O. */
void ndsR2AnimCachePreloadMatch(void)
{
    ndsRendererAdapterPrepareWorldCaches();
    memset(sNdsR2AnimStoredThisMatch, 0, sizeof(sNdsR2AnimStoredThisMatch));
#if NDS_TICK_HUD
    {
        u32 i;

        for (i = 0u; i < NDS_R2_ANIM_STORED_BITS; i++)
        {
            gNdsAnimUseBits[i] = 0u;
        }
    }
#endif
    sNdsR2AnimCacheSetupGeneration = gNdsTaskmanHeapGeneration;
    /* Arm the walk AND settle ownership first. This is the second-entry seam:
     * Sudden Death and the rematch both call it after the heap has been rewound
     * under a cache still holding last match's entries, and it previously only
     * touched the cursor -- so the stale entries survived into the new match and
     * the first find could hand back a pointer into reused memory. */
    ndsR2AnimCacheValidateGeneration();
    sNdsR204AnimWarmCursor = 0u;
    /* And re-arm the pack. The generation test above already dropped it if the
     * heap moved; this covers the case where it did NOT -- a re-entry that
     * reuses the same generation must still not be left with a FAILED state
     * from the previous match's attempt. Re-arming a DONE pack would re-stream
     * 287,904 bytes for nothing, so only a failed load is retried. */
    ndsBattlePackResidencyRearm();
}

/* R2-04 E5. One bounded preload step. This used to run once per countdown scene
 * update; battle startup now drains it before BGM/countdown begins so neither
 * the visible countdown nor live audio carries cartridge I/O.
 *
 * SLICE 46: that stopped being true once the list grew to 85. Measured on the
 * c123 gate arm, `gNdsR2AnimWarmLoaded` is **83 of 85** at the end of a whole
 * match -- the walk never finishes -- and the match takes 32 anim-cache MISSES
 * inside the sample window. They are not cheap and they are not spread: the
 * miss-only symbols `strncasecmp` and `ndsRelocApplyWordByteSwap` sit on 22 of
 * the 80 costliest frames, and capping `SINT` on just the 32 highest-`SINT`
 * frames is worth **-32,512 WORK-H P95**.
 *
 * So step several per call. Four remains the ordinary bounded chunk; the new
 * pre-BGM barrier simply calls this helper repeatedly until the measured list is
 * resident. The bound is retained for fail-open/background callers and to keep
 * each NitroFS burst small, not because battle startup is racing live audio.
 *
 * (The step was a `.data` route word, gNdsR2AnimWarmStep, until 2026-10-06.) */
#define NDS_R2_ANIM_WARM_STEP 4u

volatile u32 gNdsR2AnimPreloadBarrierRuns;
volatile u32 gNdsR2AnimPreloadBarrierSteps;
volatile u32 gNdsR2AnimPreloadBarrierCompleteCount;
volatile u32 gNdsR2AnimPreloadBarrierIncompleteCount;

static sb32 ndsR2AnimCachePreloadComplete(void)
{
    if (sNdsR204AnimWarmCursor <
        (sizeof(sNdsR204AnimWarmList) / sizeof(sNdsR204AnimWarmList[0])))
    {
        return FALSE;
    }
    if ((sNdsBattlePackLoadState != NDS_BATTLEPACK_LOAD_DONE) &&
        (sNdsBattlePackLoadState != NDS_BATTLEPACK_LOAD_FAILED))
    {
        return FALSE;
    }
    return TRUE;
}

void ndsR2AnimCachePreloadStep(void)
{
    /* Cycle 85 SWRM. The one asset load inside SRC, and therefore the honest
     * load signal the owner's loading-state exclusion actually wants -- the
     * banked rule thresholds on SRC itself, which is circular for SRC (board,
     * cycle 81). The guard is inverted rather than early-returning so the
     * bracket has a single exit and the exhausted-cursor frames, which are most
     * of the match, are still charged their validate-and-compare cost. */
#if NDS_TICK_HUD
    u32 warm_start = cpuGetTiming();
#endif

    {
        u32 step = NDS_R2_ANIM_WARM_STEP;
        u32 i;

        ndsR2AnimCacheValidateGeneration();
        /* The pack first, and ALONE while it is streaming. Two reasons: the
         * arena is a bump allocator, so the blob must own the low bytes before
         * any warm payload takes them; and a recovery/background caller should
         * still perform only one bounded I/O class per step. `step = 0` for a
         * 16 KB pack chunk. */
        if (ndsBattlePackResidencyStep() != FALSE)
        {
            step = 0u;
        }
        for (i = 0u; i < step; i++)
        {
            if (sNdsR204AnimWarmCursor >= (sizeof(sNdsR204AnimWarmList) /
                                           sizeof(sNdsR204AnimWarmList[0])))
            {
                break;
            }
            {
                u32 asset_id =
                    sNdsR204AnimWarmList[sNdsR204AnimWarmCursor++];

                if (ndsR2AnimWarmAssetMatchesRoster(asset_id) != FALSE)
                {
                    ndsR2AnimWarmLoadOne(asset_id);
                }
            }
        }
    }
#if NDS_TICK_HUD
    gNdsTickHudSrcAnimWarmTicks += cpuGetTiming() - warm_start;
#endif
}

/* Battle startup owns a loading barrier now. Do not turn this into one giant
 * read: PreloadStep's bounded chunks are also the failure containment boundary.
 * 256 iterations covers the 106-entry warm list even with WarmStep clamped to 1
 * plus the 18-chunk BattlePack stream, with large margin. If a future change
 * breaks that invariant, fail open into the old on-demand path rather than hang
 * before the match. */
s32 ndsR2AnimCachePreloadFinish(void)
{
    u32 steps = 0u;

    NDS_DIAG(gNdsR2AnimPreloadBarrierRuns++);
    while ((ndsR2AnimCachePreloadComplete() == FALSE) && (steps < 256u))
    {
        ndsR2AnimCachePreloadStep();
        steps++;
    }
    NDS_DIAG(gNdsR2AnimPreloadBarrierSteps += steps);
    if (ndsR2AnimCachePreloadComplete() == FALSE)
    {
        NDS_DIAG(gNdsR2AnimPreloadBarrierIncompleteCount++);
        return FALSE;
    }
    NDS_DIAG(gNdsR2AnimPreloadBarrierCompleteCount++);
    return TRUE;
}
