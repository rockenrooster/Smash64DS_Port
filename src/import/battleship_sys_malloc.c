/* Compile the original BattleShip arena allocator unchanged -- except for what
 * it does when an arena is full.
 *
 * decomp/src/sys/malloc.c:30 is literally `while (TRUE);` after a
 * syDebugPrintf. On the N64 that was a developer assert next to a devkit
 * console. Here it is shipped code with nowhere to print, so every arena
 * exhaustion in this port presents as a total, silent, permanent ARM9 hang --
 * interrupts still enabled and still being serviced, VBlanks still counting,
 * the main loop simply never returning. Captured twice on 2026-07-29: once in
 * ndsR2AnimCacheStore mid-match, once in ftManagerSetupFilesMainKind at battle
 * start (docs/BUGS.md). It is the shared mechanism behind the "random freezes".
 *
 * decomp/ is read-only, so the fix is here. The success path is byte-for-byte
 * the original: the pre-check below is the same arithmetic the original does
 * after committing, so when it passes, battleship_syMallocSet cannot fail.
 * Only the overflow arm changes, and only so that it names itself -- it
 * publishes the arena, the request and the headroom, then halts in a symbol a
 * debugger can break on, instead of an anonymous `b .` inside reference source.
 *
 * It deliberately still halts. syTaskmanMalloc's decomp callers do not check
 * for NULL -- ftManagerSetupFilesMainKind writes through the result
 * immediately -- so returning NULL globally would trade a hang for a wild
 * write, which is worse and harder to find. A caller that CAN degrade must ask
 * ndsSyMallocWouldFit() first and take its own fallback; that is the
 * class-level guard, and reloc_backend_assets.c's animation cache is its first
 * user.
 */

/* Deliberately NOT include/sys/malloc.h: the decomp malloc.h that malloc.c
 * pulls in declares the same struct tag under a different guard, so including
 * both redefines SYMallocRegion. The types below come from that one. */
#include <stdint.h>
#include <PR/ultratypes.h>

#define syMallocSet battleship_syMallocSet
#include "../../decomp/BattleShip-main/decomp/src/sys/malloc.c"
#undef syMallocSet

void *battleship_syMallocSet(SYMallocRegion *bp, size_t size, u32 alignment);

volatile u32 gNdsSyMallocOverflowCount;
volatile u32 gNdsSyMallocOverflowArenaID;
volatile u32 gNdsSyMallocOverflowRequest;
volatile u32 gNdsSyMallocOverflowAlignment;
volatile u32 gNdsSyMallocOverflowHeadroom;
volatile u32 gNdsSyMallocOverflowCallerLR;

sb32 ndsSyMallocWouldFit(const SYMallocRegion *bp, size_t size, u32 alignment)
{
    uintptr_t aligned;

    if (bp == NULL)
    {
        return FALSE;
    }
    aligned = (uintptr_t)bp->ptr;
    if (alignment != 0u)
    {
        uintptr_t offset = (uintptr_t)alignment - 1u;

        aligned = (aligned + offset) & ~offset;
    }
    return ((aligned + size) <= (uintptr_t)bp->end) ? TRUE : FALSE;
}

/* Named on purpose: this is the symbol a GDB attach or the freeze soak lands
 * on, so an out-of-memory freeze is identified from the PC alone without
 * needing a symbolized backtrace through decomp. */
void __attribute__((noinline, used)) ndsSyMallocOverflowHalt(void)
{
    for (;;)
    {
        __asm__ volatile("" ::: "memory");
    }
}

void *syMallocSet(SYMallocRegion *bp, size_t size, u32 alignment)
{
    /* ndsSyMallocWouldFit mirrors the `bp->end < bp->ptr` test that
     * decomp/src/sys/malloc.c applies AFTER committing bp->ptr. There are
     * deliberately two copies of that expression; if the decomp one ever
     * changes, this one has to change with it. */
    if ((bp != NULL) && (ndsSyMallocWouldFit(bp, size, alignment) == FALSE))
    {
        gNdsSyMallocOverflowCount++;
        gNdsSyMallocOverflowArenaID = bp->id;
        gNdsSyMallocOverflowRequest = (u32)size;
        gNdsSyMallocOverflowAlignment = alignment;
        gNdsSyMallocOverflowHeadroom =
            (u32)((uintptr_t)bp->end - (uintptr_t)bp->ptr);
        gNdsSyMallocOverflowCallerLR =
            (u32)(uintptr_t)__builtin_return_address(0);
        ndsSyMallocOverflowHalt();
    }
    return battleship_syMallocSet(bp, size, alignment);
}
