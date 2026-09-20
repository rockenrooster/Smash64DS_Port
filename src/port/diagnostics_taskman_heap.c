/* Do not include <malloc.h> here: BattleShip has its own sys/malloc.h earlier
 * on this target's include path. newlib's mallinfo ABI is ten 32-bit size_t
 * fields on ARM9; declare only the one query this diagnostic needs. */
typedef struct NDSNewlibMallinfo
{
    u32 arena;
    u32 ordblks;
    u32 smblks;
    u32 hblks;
    u32 hblkhd;
    u32 usmblks;
    u32 fsmblks;
    u32 uordblks;
    u32 fordblks;
    u32 keepcost;
} NDSNewlibMallinfo;
extern NDSNewlibMallinfo mallinfo(void);

/* 2026-09-14 build-p2-fidelity-02 with the shipped 0xA000 reserve reported
 * arena 1,380,096 B, BattleShip general-heap low-water 118,752 B and the old
 * live-byte diagnostic 33,288 B (builds/codex-heap-stress.out). That reading
 * was NOT a 12 KiB-reserve arm. The diagnostic below now measures top-chunk
 * depletion, including fragmentation; the stress verifier requires its
 * measured high-water plus one 4 KiB allocator page to fit this reserve. */
#if NDS_P2_MENU_SHELL && NDS_P2_1P_GAME
/* The all-content shell needs the three overlapping VSBattle entry-thread
 * stacks.  Recent full-match/four-CPU measurements peak at 32,960 bytes of
 * libc top-chunk depletion; 0x9100 leaves 4,160 bytes above that peak, still
 * exceeding the mandatory one-page allocator margin below, and returns 3,840
 * bytes to BattleShip's taskman arena. */
#define NDS_TASKMAN_LIBC_RUNTIME_RESERVE 0x9100u
#else
#define NDS_TASKMAN_LIBC_RUNTIME_RESERVE 0xA000u
#endif
#define NDS_TASKMAN_LIBC_RUNTIME_MARGIN 0x1000u

/* Slack requested from `calloc` so the arena base can be rounded UP to the
 * cache-set period without running past the block. It must be at least the
 * alignment, which is 1,024 -- see the long note at the rounding site: this
 * pins every arena allocation's cache set and line phase so that a `.data` or
 * `.bss` size change cannot re-phase the whole heap and move WORK-H by tens of
 * thousands of ticks with no code change. */
#define NDS_TASKMAN_ARENA_ALIGN_SLACK 0x400u

/* Bytes the sub-page refinement below recovered above the last 4 KiB page
 * that fit (0 when the page boundary was already the ceiling). Read by the
 * four-fighter stress arm beside the chosen size and the page fail count. */
__attribute__((used)) volatile u32 gNdsTaskmanArenaRefineBytes;
__attribute__((used)) volatile u32 gNdsTaskmanLibcRuntimeHighWater;
__attribute__((used)) volatile u32 gNdsTaskmanLibcTopChunkMin;
static u32 sNdsTaskmanLibcInitialTop;
static u32 sNdsTaskmanLibcLastSampleFrame = 0xffffffffu;

void ndsTaskmanSampleLibcHeapNow(void)
{
    NDSNewlibMallinfo info;
    u32 top;

    info = mallinfo();
    top = info.keepcost;
    if (top < gNdsTaskmanLibcTopChunkMin)
    {
        gNdsTaskmanLibcTopChunkMin = top;
    }
    gNdsTaskmanLibcRuntimeHighWater =
        (sNdsTaskmanLibcInitialTop > gNdsTaskmanLibcTopChunkMin) ?
        (sNdsTaskmanLibcInitialTop - gNdsTaskmanLibcTopChunkMin) : 0u;
}

static void ndsTaskmanSampleLibcHeap(void)
{
    u32 frame = gNdsBattlePlayablePacingPresentedFrames;

    /* ndsTaskmanSampleGraphicsHeap can run several times in one draw while a
     * fighter temporarily advances and restores the graphics heap. Keep that
     * battle hook to one sample per presented frame; menus/loaders call the
     * unconditional entry at their own lifetime boundaries. */
    if (frame == sNdsTaskmanLibcLastSampleFrame)
    {
        return;
    }
    sNdsTaskmanLibcLastSampleFrame = frame;
    ndsTaskmanSampleLibcHeapNow();
}

static void ndsTaskmanLibcResetAfterShrink(void)
{
    NDSNewlibMallinfo info = mallinfo();

    sNdsTaskmanLibcInitialTop = info.keepcost;
    gNdsTaskmanLibcTopChunkMin = info.keepcost;
    gNdsTaskmanLibcRuntimeHighWater = 0u;
    sNdsTaskmanLibcLastSampleFrame = 0xffffffffu;
}

/* The taskman arena and libnds both allocate from the same newlib heap.  The
 * arena chooser used to accept the first calloc that fit, which can consume
 * the heap's entire top chunk.  That is not enough: battle rendering still
 * performs small, legitimate libnds allocations later (for example a
 * vramBlock split when a new fighter texture becomes resident).
 *
 * Once the largest arena ACTUALLY fits, shrink that SAME allocation by two
 * 4 KiB pages with realloc. The expanded four-player run exhausted the
 * old one-page reserve: libnds' 28-byte VRAM block allocation returned NULL
 * and its unchecked store aborted (2026-09-06, frame 416..512). Freeing the successful probe and allocating a
 * smaller replacement is subtly wrong here: newlib's top-chunk history lets
 * the next successful probe grow by the amount supposedly reserved, so the
 * persistent arena can land at the exact same size.  Shrinking the live block
 * cannot self-cancel; it returns a real tail chunk to libc for later libnds
 * metadata allocations while preserving the calloc-zeroed arena prefix.
 *
 * The four-distinct-kind stress measured 46,732 B general-heap low-water before
 * this one-page trade.  The accepted 1,972-sample run with the shrink measured
 * 36,260 B, still 10,660 B above the verifier's 25,600 B safety floor. */

static u8 *ndsTaskmanArenaBytes(void)
{
    if (sNdsTaskmanArenaBytes == NULL)
    {
        size_t arena_size;

        /* Preserve the largest useful BattleShip taskman arena the DS heap can
         * provide. A coarse 0x150000 -> 0x140000 jump discarded up to 60 KiB
         * even when only a few pages were unavailable. Keep page granularity
         * below 0x130000 too: the expanded campaign used to fall directly to
         * 0xc0000, discarding usable pages exactly where RAM is tightest. */
        for (arena_size = NDS_TASKMAN_ARENA_SIZE;
             arena_size >= 0x40000u;
             arena_size -= 0x1000u)
        {
            sNdsTaskmanArenaAlloc = calloc(1, arena_size + NDS_TASKMAN_ARENA_ALIGN_SLACK);
            if (sNdsTaskmanArenaAlloc != NULL)
            {
                /* Page granularity still leaves up to 4,095 B of the top chunk
                 * unclaimed, and the four-fighter stress arm measured its
                 * general-heap low-water 1,048 B under the 25,600 B floor with
                 * the page-granular arena (2026-09-13). The page above already
                 * failed, so probe upward from the page that fit in 256 B
                 * steps and keep the largest block that still fits. Failed
                 * probes allocate nothing; only the kept block is zeroed. */
                {
                    size_t extra;

                    free(sNdsTaskmanArenaAlloc);
                    sNdsTaskmanArenaAlloc = NULL;
                    for (extra = 0x1000u - 0x100u; extra != 0u;
                         extra -= 0x100u)
                    {
                        sNdsTaskmanArenaAlloc =
                            calloc(1, arena_size + extra + NDS_TASKMAN_ARENA_ALIGN_SLACK);
                        if (sNdsTaskmanArenaAlloc != NULL)
                        {
                            gNdsTaskmanArenaRefineBytes = (u32)extra;
                            arena_size += extra;
                            break;
                        }
                    }
                    if (sNdsTaskmanArenaAlloc == NULL)
                    {
                        sNdsTaskmanArenaAlloc =
                            calloc(1, arena_size + NDS_TASKMAN_ARENA_ALIGN_SLACK);
                    }
                }
                if (sNdsTaskmanArenaAlloc == NULL)
                {
                    gNdsTaskmanArenaAllocFailCount++;
                    continue;
                }
                size_t persistent_size = arena_size -
                    NDS_TASKMAN_LIBC_RUNTIME_RESERVE;
                void *resized = realloc(
                    sNdsTaskmanArenaAlloc, persistent_size + NDS_TASKMAN_ARENA_ALIGN_SLACK);

                if (resized != NULL)
                {
                    sNdsTaskmanArenaAlloc = resized;
                    uintptr_t addr = (uintptr_t)sNdsTaskmanArenaAlloc;

                    /* 1,024 and not 16, and this is a MEASUREMENT fix rather
                     * than a performance one. The ARM9 data cache is 4 KB,
                     * 4-way, 32-byte lines, so a line's set is (addr>>5)&31 and
                     * the set pattern of everything in this arena repeats every
                     * 1,024 bytes. At 16-byte alignment the arena base tracks
                     * `__end__`, which moves whenever `.data` or `.bss` changes
                     * size -- so ANY edit that grows a static re-phases every
                     * GObj, DObj, FTStruct, stage buffer and texture entry
                     * against all 32 sets at once, and moves every 32-byte line
                     * boundary inside them.
                     *
                     * That is not hypothetical. Forcing one 4 KB sine table to
                     * a 4 KB boundary shifted 1,066 KB of `.data`+`.bss` by
                     * 0x117C and cost +49,152 WORK-H P50, +46,336 of it in STG
                     * -- a 2.9-point perturbation of a 26.2% miss rate on a
                     * working set 78x the cache, with nothing colliding with
                     * anything nameable. The 2026-09-16 clean rebuild moved
                     * WORK-H -50,432 with no source change at all, which is the
                     * same mechanism.
                     *
                     * Aligning here makes each allocation's set index and line
                     * phase depend only on its offset WITHIN the arena, so they
                     * are invariant to every `.text`/`.data`/`.bss` size change.
                     * It buys no speed; it makes cross-build A/Bs mean
                     * something. Without it the campaign's 14,080-tick
                     * significance floor understates the real variance of a
                     * static-size-changing edit by about 3.5x.
                     *
                     * Cost is at most 1,023 bytes against a 24,404-byte heap
                     * low-water, and the slack requested above is raised to
                     * match. Evidence:
                     * artifacts/performance/2026-09-16_p2-2p8-placement-hazard/. */
                    sNdsTaskmanArenaBytes =
                        (u8 *)((addr + 0x3ffu) & ~(uintptr_t)0x3ffu);
                    gNdsTaskmanArenaChosenSize = (u32)persistent_size;
                    ndsTaskmanLibcResetAfterShrink();
                    break;
                }
                free(sNdsTaskmanArenaAlloc);
                sNdsTaskmanArenaAlloc = NULL;
            }
            gNdsTaskmanArenaAllocFailCount++;
        }
    }
    return sNdsTaskmanArenaBytes;
}

void *ndsTaskmanArenaStart(void)
{
    return ndsTaskmanArenaBytes();
}

size_t ndsTaskmanArenaSize(void)
{
    return (ndsTaskmanArenaBytes() != NULL) ?
        (size_t)gNdsTaskmanArenaChosenSize : 0u;
}

/* P2-3r13. THE PER-CONTEXT GRAPHICS HEAP, MEASURED INSTEAD OF ASSUMED.
 *
 * battleship_scvsbattle.c already returned the two reservations of this class
 * that were provably dead on DS hardware -- 61,440 B of N64 display-list buffer
 * (16 bytes of 81,920 ever written) and 45,056 B of RDP output buffer (zero) --
 * and left this one at the source's own 0xD000 with the reason recorded: unlike
 * those, the graphics heap has live CPU writers on this port, so "shrinking it
 * is a measured draw-depth question this rebate does not answer". This is that
 * measurement, and it exists because the arena bought its way to four distinct
 * fighter kinds and the next kind has to come from somewhere.
 *
 * The heap is reset at the top of every present, so the figure that matters is
 * a per-FRAME peak. It cannot be read at end of frame alone: the fighter draw
 * saves and restores `gSYTaskmanGraphicsHeap.ptr` around itself (contract
 * capture, rebirth halo, afterimage), so a fighter's own consumption is rolled
 * back before the frame ends and an end-of-frame sample reports everything
 * except the deepest thing in the frame. Sample at the RESTORE points as well;
 * that is where each rolled-back peak is still readable.
 *
 * Overflow is counted rather than trusted: the source only prints a warning
 * (decomp taskman.c:330) and keeps writing, so an undersized heap corrupts
 * whatever follows it instead of failing. A non-zero count on any run means the
 * size below is wrong, and the four-CPU stress harness reads it. */
__attribute__((used)) volatile u32 gNdsTaskmanGraphicsHeapHighWater;
__attribute__((used)) volatile u32 gNdsTaskmanGraphicsHeapCapacity;
__attribute__((used)) volatile u32 gNdsTaskmanGraphicsHeapOverflowCount;
/* P2-3f9. The counter above cannot see the adapter's material branch table
 * running out of room: that builder CHECKS first and returns FALSE, so the
 * pointer never passes `end` and the sampler has nothing to report. Cutting
 * the reservation from 0xD000 to 0x2000 made that refusal a real possibility
 * rather than a theoretical one, so it is counted at its own site
 * (ndsRendererAdapterPrepareMaterialSegment, reloc_backend_renderer_dl.c) and
 * asserted at 0 by the four-CPU stress harness. A nonzero value means a DObj
 * drew without its material branch. */
__attribute__((used)) volatile u32 gNdsTaskmanGraphicsHeapNoRoomCount;

void ndsTaskmanSampleGraphicsHeap(void)
{
    uintptr_t start = (uintptr_t)gSYTaskmanGraphicsHeap.start;
    uintptr_t end = (uintptr_t)gSYTaskmanGraphicsHeap.end;
    uintptr_t ptr = (uintptr_t)gSYTaskmanGraphicsHeap.ptr;
    u32 used;

    ndsTaskmanSampleLibcHeap();

    if ((start == 0u) || (ptr < start))
    {
        return;
    }
    if (end >= start)
    {
        gNdsTaskmanGraphicsHeapCapacity = (u32)(end - start);
    }
    if (ptr > end)
    {
        gNdsTaskmanGraphicsHeapOverflowCount++;
    }
    used = (u32)(ptr - start);
    if (used > gNdsTaskmanGraphicsHeapHighWater)
    {
        gNdsTaskmanGraphicsHeapHighWater = used;
    }
}

#define NDS_OVERLAY_LIST(X) \
    X(0) X(1) X(2) X(3) X(4) X(5) X(6) X(7) X(8) X(9) \
    X(10) X(11) X(12) X(13) X(14) X(15) X(16) X(17) X(18) X(19) \
    X(20) X(21) X(22) X(23) X(24) X(25) X(26) X(27) X(28) X(29) \
    X(30) X(31) X(32) X(33) X(34) X(35) X(36) X(37) X(38) X(39) \
    X(40) X(41) X(42) X(43) X(44) X(45) X(46) X(47) X(48) X(49) \
    X(50) X(51) X(52) X(53) X(54) X(55) X(56) X(57) X(58) X(59) \
    X(60) X(61) X(62) X(63) X(64) X(65)

/* Overlay linker symbols. These are zeroed compatibility placeholders for the
 * original overlay table; the DS taskman arena is explicit above. */
#define NDS_DEFINE_OVERLAY(n) \
    uintptr_t ovl##n##_ROM_START, ovl##n##_ROM_END; \
    uintptr_t ovl##n##_TEXT_START, ovl##n##_TEXT_END; \
    uintptr_t ovl##n##_DATA_START, ovl##n##_RODATA_END; \
    uintptr_t ovl##n##_BSS_START, ovl##n##_BSS_END; \
    uintptr_t ovl##n##_VRAM;

NDS_OVERLAY_LIST(NDS_DEFINE_OVERLAY)

/* Diagnostic counters for the reloc/fade stubs (still DS-owned). */
