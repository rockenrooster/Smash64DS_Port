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
 * A packed clip needs none of it. The blob is `const`, position independent and
 * already in the exact bytes the parser consumes, so the acquisition collapses
 * to a binary search that returns a pointer into it.
 *
 * WHY A POINTER INTO A BLOB IS A LEGAL FIGATREE. `lbCommonAddFighterPartsFigatree`
 * walks one word per DObj slot and resolves each through
 * `ndsRelocResolvePointerFromFileBase`, which treats a word that is not already
 * inside a known file as a byte offset from the CONTAINING file's base. The pack
 * blob registers as one such file (`ndsBattlePackContains`), so a slot table of
 * blob-relative byte offsets resolves with no fixups at all.
 */

/* The clip's figatree table, or NULL when this asset is not resident. The
 * returned pointer is into read-only pack storage and is stable for the life of
 * the program: unlike `figatree_heap` it is never reused by the next action. */
void *ndsBattlePackFindFigatree(u32 asset_id);

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

#define NDS_BATTLEPACK_STATE_UNCHECKED 0u
#define NDS_BATTLEPACK_STATE_READY 1u
#define NDS_BATTLEPACK_STATE_BAD_MAGIC 2u
#define NDS_BATTLEPACK_STATE_BAD_VERSION 3u
#define NDS_BATTLEPACK_STATE_BAD_EXTENT 4u

#endif /* NDS_BATTLEPACK_ANIM_H */
