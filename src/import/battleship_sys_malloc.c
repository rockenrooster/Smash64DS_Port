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
#include <nds/arm9/cache.h>
#include <PR/ultratypes.h>

#define syMallocSet battleship_syMallocSet
#define syMallocInit battleship_syMallocInit
#define syMallocReset battleship_syMallocReset
#include "../../decomp/BattleShip-main/decomp/src/sys/malloc.c"
#undef syMallocReset
#undef syMallocInit
#undef syMallocSet

void *battleship_syMallocSet(SYMallocRegion *bp, size_t size, u32 alignment);
void battleship_syMallocInit(SYMallocRegion *bp, u32 id, void *start,
                             size_t size);
void battleship_syMallocReset(SYMallocRegion *bp);

/* The general heap, by identity. Declared here rather than pulled in from
 * include/sys/taskman.h because this TU deliberately uses the decomp malloc.h
 * definition of SYMallocRegion -- see the note above the includes. */
extern SYMallocRegion gSYTaskmanGeneralHeap;

/* THE TASKMAN-HEAP GENERATION, and the contract that replaces a guess.
 *
 * gSYTaskmanGeneralHeap is a bump region. Every battle entry rewinds it --
 * syTaskmanStartTask calls syTaskmanInitGeneralHeap before allocating anything
 * for the new scene (decomp taskman.c:1244) -- and any pointer taken from it
 * before that rewind is dead afterwards, even though it still points inside the
 * region and still passes every range test.
 *
 * The animation cache used to INFER that from the cursor: "our block ends at or
 * before ptr, so it has not been reclaimed". That is a heuristic and it
 * false-positives exactly when it matters. A second scene entry rewinds the
 * heap and then allocates; once the new scene's allocations push the cursor
 * past the old block, the test passes again and the cache hands back pointers
 * into memory the new scene already owns.
 *
 * A counter cannot be fooled that way. Bumping it here -- at the only two
 * primitives that can move `ptr` backwards, keyed on the region's identity --
 * makes ownership a fact rather than an inference, and it keeps the property
 * the old comment was right to want: it cannot miss a rewind performed by some
 * future code path, because every such path goes through one of these two. */
volatile u32 gNdsTaskmanHeapGeneration;

void syMallocInit(SYMallocRegion *bp, u32 id, void *start, size_t size)
{
    battleship_syMallocInit(bp, id, start, size);
    if (bp == &gSYTaskmanGeneralHeap)
    {
        gNdsTaskmanHeapGeneration++;
    }
}

void syMallocReset(SYMallocRegion *bp)
{
    battleship_syMallocReset(bp);
    if (bp == &gSYTaskmanGeneralHeap)
    {
        gNdsTaskmanHeapGeneration++;
    }
}

volatile u32 gNdsSyMallocOverflowCount;
volatile u32 gNdsSyMallocOverflowArenaID;
volatile u32 gNdsSyMallocOverflowRequest;
volatile u32 gNdsSyMallocOverflowAlignment;
volatile u32 gNdsSyMallocOverflowHeadroom;
volatile u32 gNdsSyMallocOverflowCallerLR;
#if NDS_R2_SECOND_ENTRY_DIAG
void ndsAllocLedgerPublishTop(void);
#endif

/* R2-06 E4 REFUTED this function's call overhead as a cost, so it stays one plain
 * out-of-line function. Forcing the fit test inline at syMallocSet's call site
 * produced a DIFFERENT ROM (sha 8F0CDAAC -> EAEDFED0) whose every sampled tick
 * bucket was byte-identical, SRC P95 471,232 either way -- GCC had already inlined
 * the call at the only hot site, so there was nothing to win. Do not re-propose it;
 * see docs/P1_EXECUTION_BOARD.md "R2-06 E4". */
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
        /* THE SIX WORDS ABOVE WERE UNREADABLE UNTIL THIS LINE.
         * scripts/probe-arena-overflow.ps1 has carried a paragraph since
         * 2026-08-25 explaining that they read back as zeroes -- the GDB stub
         * reads main RAM behind the ARM9 data cache and these are dirty lines
         * the CPU wrote microseconds ago -- and telling the reader to treat a
         * row of zeroes as "unreadable, never as no overflow". That is a
         * workaround for a one-line omission: the halt spins forever, so
         * nothing was ever going to evict them. Flushing here makes the
         * published state mean what it says, including the caller address,
         * which is the field the corrupt backtrace past frame 1 cannot
         * supply. It costs one flush on a path that never returns. */
#if NDS_R2_SECOND_ENTRY_DIAG
        /* And if the allocation ledger is compiled in, rank it here: knowing
         * WHICH request failed is half the answer, and the other half is what
         * had already taken the arena. */
        ndsAllocLedgerPublishTop();
#endif
        DC_FlushAll();
        ndsSyMallocOverflowHalt();
    }
    return battleship_syMallocSet(bp, size, alignment);
}
