static u8 *ndsTaskmanArenaBytes(void)
{
    if (sNdsTaskmanArenaBytes == NULL)
    {
        static const size_t lower_arena_sizes[] =
        {
            0x100000u,
            0xc0000u,
            0x80000u,
            0x40000u
        };
        size_t arena_size;
        size_t i;

        /* Preserve the largest useful BattleShip taskman arena the DS heap can
         * provide. A coarse 0x150000 -> 0x140000 jump discarded up to 60 KiB
         * even when only a few pages were unavailable, which could erase the
         * verified 128 KiB post-BGM reserve. */
        for (arena_size = NDS_TASKMAN_ARENA_SIZE;
             arena_size >= 0x130000u;
             arena_size -= 0x1000u)
        {
            sNdsTaskmanArenaAlloc = calloc(1, arena_size + 0x10u);
            if (sNdsTaskmanArenaAlloc != NULL)
            {
                uintptr_t addr = (uintptr_t)sNdsTaskmanArenaAlloc;
                sNdsTaskmanArenaBytes = (u8 *)((addr + 0xfu) & ~(uintptr_t)0xfu);
                gNdsTaskmanArenaChosenSize = (u32)arena_size;
                break;
            }
            gNdsTaskmanArenaAllocFailCount++;
        }
        for (i = 0;
             (sNdsTaskmanArenaBytes == NULL) &&
             (i < (sizeof(lower_arena_sizes) /
                   sizeof(lower_arena_sizes[0])));
             i++)
        {
            if (lower_arena_sizes[i] > NDS_TASKMAN_ARENA_SIZE)
            {
                continue;
            }
            sNdsTaskmanArenaAlloc = calloc(
                1, lower_arena_sizes[i] + 0x10u);
            if (sNdsTaskmanArenaAlloc != NULL)
            {
                uintptr_t addr = (uintptr_t)sNdsTaskmanArenaAlloc;
                sNdsTaskmanArenaBytes =
                    (u8 *)((addr + 0xfu) & ~(uintptr_t)0xfu);
                gNdsTaskmanArenaChosenSize = (u32)lower_arena_sizes[i];
            }
            else
            {
                gNdsTaskmanArenaAllocFailCount++;
            }
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
