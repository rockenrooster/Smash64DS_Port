#ifndef SSB64_NDS_RELOC_ASSETS_H
#define SSB64_NDS_RELOC_ASSETS_H

#include <stddef.h>

#include <PR/ultratypes.h>

#define NDS_RELOC_ASSET_INIT_PASS 0x4e465349u /* NFSI */

typedef struct NDSRelocAssetHeader {
    u32 file_id;
    u16 reloc_intern_offset;
    u16 reloc_extern_offset;
    u32 extern_file_ids_num;
    u32 data_size;
} NDSRelocAssetHeader;

struct MObjSub;

void ndsRelocAssetsInit(void);
/* R2-04 E4/E5. Makes the match's animation streams resident. PreloadMatch arms
 * the walk at the battle-start seam; PreloadStep advances one bounded chunk.
 * PreloadFinish drains those bounded chunks before battle BGM/countdown starts,
 * so gameplay never inherits the old background warm walk. The chunking stays
 * because the same routine is also safe for callers that elect to step it.
 * Declared unconditionally so this header does not depend on the generated
 * config defines being visible before it; definitions/callers are guarded by
 * NDS_R2_ANIM_CACHE. */
void ndsR2AnimCachePreloadMatch(void);
void ndsR2AnimCachePreloadStep(void);
s32 ndsR2AnimCachePreloadFinish(void);
const char *ndsRelocAssetGetPath(u32 asset_id);
s32 ndsRelocAssetReadHeader(u32 asset_id, NDSRelocAssetHeader *out_header);
s32 ndsRelocAssetReadExternFileIDs(u32 asset_id, u16 *out_file_ids,
                                   u32 capacity, u32 *out_count);
s32 ndsRelocAssetLoadData(u32 asset_id, void *dst, size_t dst_capacity,
                           NDSRelocAssetHeader *out_header);
/* Both from one open. Prefer this wherever a caller wants the header and the
 * payload: the two-call form walks the NitroFS directory twice. */
s32 ndsRelocAssetLoadHeaderAndData(u32 asset_id, void *dst,
                                   size_t dst_capacity,
                                   NDSRelocAssetHeader *out_header);
/* A byte range out of a NitroFS payload that is NOT an o2r asset -- no asset
 * table entry, no o2r header, no file id. Slice 1's resident figatree pack is
 * the only such payload the animation path reads, and it is streamed in chunks
 * because the seam's budget is one BGM packet.
 *
 * File I/O stays in this module rather than being re-implemented in
 * reloc_backend_assets.c: that TU has no <stdio.h> and the open/short-read
 * counters that the after-GO zero assertions read live here. */
s32 ndsRelocAssetReadRawRange(const char *path, u32 offset, void *dst,
                              u32 bytes);
/* The same raw range read, with the NitroFS open hoisted out of the loop.
 *
 * WHY IT EXISTS (P2-1c residual, closed by P2-1d). An open by path on NitroFS
 * is a directory walk -- the same walk Task 71 measured `strncasecmp` at
 * 30,484 ticks/frame on -- and a caller that reads one payload in N staged
 * chunks pays it N times for one file. The UI kit's pack was twelve opens for
 * 24 KB before this, and the digit block P2-1d adds would have made it
 * twenty-three.  Open once, read the chunks, close.
 *
 * `stream` is opaque state the caller owns; a failed Open leaves it closed and
 * every later call on it returns FALSE, so a caller may check once at the end.
 * Close is idempotent and safe on a stream that never opened. Reads count
 * through gNdsRelocAssetPayloadReadCount exactly as the one-shot form does, so
 * a run's payload-read total does not change shape when a caller switches. */
typedef struct NdsRelocAssetStream {
    void *file; /* FILE*, kept void so callers need no <stdio.h> */
} NdsRelocAssetStream;

s32 ndsRelocAssetStreamOpen(NdsRelocAssetStream *stream, const char *path);
s32 ndsRelocAssetStreamRead(NdsRelocAssetStream *stream, u32 offset, void *dst,
                            u32 bytes);
void ndsRelocAssetStreamClose(NdsRelocAssetStream *stream);
/* Size, zero and load from one open. Prefer this over sizing with
 * ndsRelocAssetAllocSize and then loading: that pair walks the NitroFS
 * directory twice for a size the load's own header already carries. `align` is
 * the caller's allocation granularity; *out_alloc_size receives data_size
 * rounded up to it, and dst is zeroed over exactly that region. */
s32 ndsRelocAssetLoadIntoZeroedHeap(u32 asset_id, void *dst, u32 align,
                                    size_t *out_alloc_size,
                                    NDSRelocAssetHeader *out_header);
s32 ndsRelocGetLoadedAssetView(u32 asset_id, const void **out_data,
                               u32 *out_size);
s32 ndsRelocCopyMObjSubForAttachment(struct MObjSub *dst,
                                     const struct MObjSub *src);
/* Recover the file returned by lbRelocGetForceExternHeapFile when pristine
 * BattleShip code later hands its original figatree_heap pointer to the port.
 * Non-matching pointers pass through unchanged. */
void *ndsRelocResolveAuthoritativeForceFile(void *file);

extern volatile u32 gNdsRelocAssetInitResult;
extern volatile u32 gNdsRelocAssetHeaderReadCount;
extern volatile u32 gNdsRelocAssetPayloadReadCount;
extern volatile u32 gNdsRelocAssetOpenFailCount;
extern volatile u32 gNdsRelocAssetFormatFailCount;
extern volatile u32 gNdsRelocAssetShortReadCount;
extern volatile u32 gNdsR2AnimPreloadBarrierRuns;
extern volatile u32 gNdsR2AnimPreloadBarrierSteps;
extern volatile u32 gNdsR2AnimPreloadBarrierCompleteCount;
extern volatile u32 gNdsR2AnimPreloadBarrierIncompleteCount;

/* ---- Slice 1 phase 7: the K0 after-GO zero-I/O assertion, per fighter -------
 *
 * `plan.md` K0 requires that once the battle is in GO, the fighter animation
 * path performs no FAT read, no seek, no payload byte-swap, no relocation, no
 * AObj16 normalization, no raw-cache copy and no token/file discovery. Every
 * one of those already had a whole-run counter, and every one of them reads
 * zero on the pack-hit path *by construction* -- the early return in
 * `ndsRelocForceLoadFighterAObj16File` precedes them all. A zero one level
 * downstream of a rejected request is indistinguishable from a working
 * deletion, so these counters are incremented AT EACH SITE, keyed by the
 * asset's own fighter, and gated on the battle actually being in GO.
 *
 * THE CONTROL IS IN THE SAME RUN. Only one fighter's clips fit the arena, so
 * `NDS_R2_BATTLEPACK=1` leaves the other fighter on the generic path: index
 * [NDS_K0_FIGHTER_MARIO] must read NON-ZERO wherever [NDS_K0_FIGHTER_FOX]
 * reads zero. A row that is zero on both arms proves nothing; a row that is
 * zero for the packed fighter while its neighbour counts is a deletion.
 *
 * The spans mirror `reloc_backend_assets.c`'s per-animation defines, which are
 * text-pinned by `check-ft-hitstatus-fixtures.ps1` and therefore stay where
 * they are; that TU carries `_Static_assert`s tying these four values to them,
 * so the two cannot drift. Mario's block ends exactly where Fox's begins. */
#define NDS_K0_MARIO_ANIM_FIRST 0x1f3u
#define NDS_K0_MARIO_ANIM_LAST 0x281u
#define NDS_K0_FOX_ANIM_FIRST 0x282u
#define NDS_K0_FOX_ANIM_LAST 0x31fu

#define NDS_K0_FIGHTER_MARIO 0u
#define NDS_K0_FIGHTER_FOX 1u
#define NDS_K0_FIGHTER_NONE 2u

/* 1 while `gSCManagerBattleState->game_status == nSCBattleGameStatusGo`,
 * published once per logic update by both taskman update loops from a read
 * they already perform. Not a latch: a rematch and a Sudden Death entry each
 * run their own countdown, and the asset work their scene setup does is
 * legitimately pre-GO for that entry. */
extern volatile u32 gNdsK0BattleInGo;

extern volatile u32 gNdsK0AfterGoAcquisitions[2];
extern volatile u32 gNdsK0AfterGoPackHits[2];
extern volatile u32 gNdsK0AfterGoFatReads[2];
extern volatile u32 gNdsK0AfterGoSeeks[2];
extern volatile u32 gNdsK0AfterGoByteSwaps[2];
extern volatile u32 gNdsK0AfterGoRelocs[2];
extern volatile u32 gNdsK0AfterGoNormalizes[2];
extern volatile u32 gNdsK0AfterGoCacheCopies[2];
extern volatile u32 gNdsK0AfterGoTokenResolves[2];
extern volatile u32 gNdsK0AfterGoPathLookups[2];

/* NDS_K0_FIGHTER_NONE for anything that is not a Mario/Fox animation asset, or
 * whenever the battle is not in GO. */
u32 ndsK0AfterGoFighter(u32 asset_id);

#define NDS_K0_MARK(counter_, asset_id_)                                       \
    do                                                                         \
    {                                                                          \
        u32 nds_k0_index_ = ndsK0AfterGoFighter(asset_id_);                    \
                                                                               \
        if (nds_k0_index_ < NDS_K0_FIGHTER_NONE)                               \
        {                                                                      \
            (counter_)[nds_k0_index_]++;                                       \
        }                                                                      \
    } while (0)

#endif
