/* Standalone ARM9 / Calico example: a tiny IRQ-owned snapshot.
 * Same CPU only. Not an ARM9/ARM7 or DMA cache-coherence protocol.
 * This program owns its VBlank callback; integrate with an existing owner
 * rather than overwriting a game's registered callback.
 */
#include <nds.h>
#include <stdint.h>

struct VBlankSnapshot {
    uint32_t frame;
    uint16_t scanline;
    uint16_t reserved;
};
static struct VBlankSnapshot g_snapshot;

static void on_vblank(void)
{
    ++g_snapshot.frame;
    g_snapshot.scanline = (uint16_t)(REG_VCOUNT & 0x01ffu);
}

static struct VBlankSnapshot snapshot_read(void)
{
    /* Calico's irqLock/irqUnlock preserve prior state and provide compiler
     * ordering. Only the copy is protected; never do I/O or waits here.
     */
    const IrqState previous = irqLock();
    const struct VBlankSnapshot result = g_snapshot;
    irqUnlock(previous);
    return result;
}

int main(void)
{
    irqSet(IRQ_VBLANK, on_vblank);
    lcdSetIrqMask(DISPSTAT_IE_VBLANK, DISPSTAT_IE_VBLANK);
    irqEnable(IRQ_VBLANK);

    while (pmMainLoop()) {
        scanKeys();
        if ((keysDown() & KEY_START) != 0) break;
        const struct VBlankSnapshot snapshot = snapshot_read();
        (void)snapshot; /* Use a coherent copy outside the critical section. */
        threadWaitForVBlank();
    }

    /* Remove only our callback; leave runtime VBlank waits enabled. */
    irqClear(IRQ_VBLANK);
    return 0;
}
