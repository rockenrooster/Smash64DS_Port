#ifndef NDS_GX_DMA_H
#define NDS_GX_DMA_H

#include <PR/ultratypes.h>

/* DMA0 geometry-FIFO waits, bounded: a stall that outlasts four frames is
 * recorded for the freeze report and cleared (ndsGxDma0WaitSlow,
 * src/nds/nds_renderer_preamble.c). */
void ndsGxDma0WaitSlow(u32 site);
#define NDS_GX_DMA0_WAIT(site) \
    do \
    { \
        if ((DMA_CR(0) & DMA_BUSY) != 0u) \
        { \
            ndsGxDma0WaitSlow(site); \
        } \
    } while (0)

#endif /* NDS_GX_DMA_H */
