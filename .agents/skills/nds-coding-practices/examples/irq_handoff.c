/*
 * Same-CPU ARM9 IRQ/main snapshot using a sequence lock.
 * This is NOT an ARM9/ARM7 protocol and does not replace cache maintenance.
 */
#include <nds.h>
#include <stdbool.h>
#include <stdint.h>

#define COMPILER_BARRIER() __asm__ volatile ("" ::: "memory")

struct VBlankSnapshot {
    uint32_t frame;
    uint16_t marker;
    uint16_t reserved;
};

static volatile uint32_t g_sequence;
static struct VBlankSnapshot g_snapshot;

static void on_vblank(void)
{
    // Odd sequence means a write is in progress.
    ++g_sequence;
    COMPILER_BARRIER();

    ++g_snapshot.frame;
    g_snapshot.marker = (uint16_t)(REG_VCOUNT & 0x01FFu);

    COMPILER_BARRIER();
    ++g_sequence; // even sequence publishes a complete snapshot
}

static bool snapshot_try_read(struct VBlankSnapshot *out)
{
    const uint32_t before = g_sequence;
    if ((before & 1u) != 0u) {
        return false;
    }

    COMPILER_BARRIER();
    *out = g_snapshot;
    COMPILER_BARRIER();

    const uint32_t after = g_sequence;
    return before == after && (after & 1u) == 0u;
}

int main(void)
{
    irqSet(IRQ_VBLANK, on_vblank);
    irqEnable(IRQ_VBLANK);

    while (pmMainLoop()) {
        struct VBlankSnapshot snapshot;
        while (!snapshot_try_read(&snapshot)) {
            // Retry only a tiny same-CPU snapshot. Never spin here waiting for
            // ARM7, storage, DMA, or a long-running IRQ.
        }

        (void)snapshot;
        swiWaitForVBlank();
    }

    return 0;
}
