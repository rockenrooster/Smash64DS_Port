#ifndef NDS_BATTLEPACK_ANIM_H
#define NDS_BATTLEPACK_ANIM_H

#include <stddef.h>

#include <PR/ultratypes.h>

/* Native Battle Kernel slice 1 phase 5 -- the resident figatree pack.
 *
 * WHAT THIS DELETES. Every action change used to reach
 * `ndsRelocForceLoadFighterAObj16File`, and even a CACHE HIT there still
 * memcpy'd the whole payload into the fighter's own heap, re-registered it as a
 * loaded file, re-ran the finalize/fixup chain, re-normalized the AObj16 lanes,
 * stripped the stale alias status nodes and wrote three status entries. That
 * sequence ran 299 times in the measured 1,600-frame gate match and was present
 * on 62 of the 80 frames that set P95 against 174 of the 1,520 body frames --
 * 6.8x (`artifacts/performance/2026-08-14_native-battle-kernel/
 * BATTLEPACK_ANIMATION.md` sections 2.1-2.2).
 *
 * A packed clip needs none of it. The blob is position independent and already
 * in the exact bytes the parser consumes, so the acquisition collapses to a
 * binary search that returns a pointer into it.
 *
 * WHERE THE BLOB LIVES, AND WHY IT IS NOT `.incbin` ANY MORE. The first build
 * linked it into `.rodata`. That is disqualified and the reason invalidates how
 * static growth was being priced here: +288,992 B of ARM9 image MEASURED
 * `gNdsTaskmanArenaChosenSize` falling 0x150000 -> 0x140000 with
 * `gNdsTaskmanArenaAllocFailCount` 16, because the taskman arena is one
 * `calloc` from the libnds heap and `.rodata` and the arena come out of the
 * same bytes. `check-boot-headroom.ps1` reported 66,784 B of PROVEN headroom on
 * that same arm -- the ladder records boot/no-boot, not arena size.
 *
 * The blob is therefore a NitroFS payload streamed into the taskman arena at
 * scene setup, in place of the 262,144 B raw-file animation cache it replaces.
 * Zero static image, so the arena stays 0x150000 and every ladder row stays
 * valid. The residency loader lives in `reloc_backend_assets.c` because that is
 * where the arena and its heap-generation ownership test already live; this
 * module owns only the data model.
 *
 * WHY A POINTER INTO A BLOB IS A LEGAL FIGATREE. `lbCommonAddFighterPartsFigatree`
 * walks one word per DObj slot and resolves each through
 * `ndsRelocResolvePointerFromFileBase`, which treats a word that is not already
 * inside a known file as a byte offset from the CONTAINING file's base. The pack
 * blob registers as one such file (`ndsBattlePackContains`), so a slot table of
 * blob-relative byte offsets resolves with no fixups at all.
 */

/* The clip's figatree table, or NULL when this asset is not resident. The
 * returned pointer is into pack storage and is stable for the life of the HEAP
 * GENERATION: unlike `figatree_heap` it is never reused by the next action, but
 * a scene rewind reclaims the arena under it, which is what `ndsBattlePackDrop`
 * exists for. Every caller reaches this behind the same generation test the raw
 * animation cache uses. */
void *ndsBattlePackFindFigatree(u32 asset_id);

/* Publish a streamed blob as the resident pack. Validates magic, version and
 * the self-declared extent against `bytes` before anything can look a clip up,
 * and refuses rather than publishing on any mismatch -- the class this guards is
 * a stale or half-written asset, which the parser answers with a freeze rather
 * than a diagnostic. Returns FALSE and leaves the pack unpublished on refusal.
 */
s32 ndsBattlePackAdopt(void *base, u32 bytes);

/* Unpublish. Called wherever the arena the blob lives in stops being ours, so a
 * lookup after a scene rewind returns NULL and the acquisition takes the
 * generic path instead of a pointer into memory the next scene already owns. */
void ndsBattlePackDrop(void);

/* TRUE when [ptr, ptr+size) lies inside the pack, reporting the blob base and
 * extent so the resolver can rebase slot words against it. */
s32 ndsBattlePackContains(const void *ptr, size_t size, const void **out_base,
                          size_t *out_size);

/* Engagement, published from code so `--gc-sections` cannot quietly drop them.
 * Hits are the acquisitions the pack served; Misses are the fighter-animation
 * acquisitions that fell through to the generic loader, which is the negative
 * control -- the un-packed fighter must keep a non-zero count. */
extern volatile u32 gNdsBattlePackClips;
extern volatile u32 gNdsBattlePackBytes;
extern volatile u32 gNdsBattlePackHits;
extern volatile u32 gNdsBattlePackMisses;
extern volatile u32 gNdsBattlePackState;
/* Residency, published by the loader in reloc_backend_assets.c. Steps is the
 * number of streamed chunks; ResidentBytes is the adopted extent; LoadFails
 * separates "the blob never arrived" from "the blob arrived and no clip was
 * asked for", which read identically in Hits alone. */
extern volatile u32 gNdsBattlePackLoadSteps;
extern volatile u32 gNdsBattlePackLoadFails;
extern volatile u32 gNdsBattlePackResidentBytes;
extern volatile u32 gNdsBattlePackDrops;

#define NDS_BATTLEPACK_STATE_UNCHECKED 0u
#define NDS_BATTLEPACK_STATE_READY 1u
#define NDS_BATTLEPACK_STATE_BAD_MAGIC 2u
#define NDS_BATTLEPACK_STATE_BAD_VERSION 3u
#define NDS_BATTLEPACK_STATE_BAD_EXTENT 4u

#endif /* NDS_BATTLEPACK_ANIM_H */
