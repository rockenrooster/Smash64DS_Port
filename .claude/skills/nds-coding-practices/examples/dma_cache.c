/* ARM9 outbound DMA and inbound cache patterns (C11).
 * Caller owns channel reservation, real buffer capacities, DMA-accessible
 * placement, disjoint physical ranges, mapping, and completion/lifetime.
 * Neither ITCM nor DTCM is DMA-readable, even after a cache flush.
 * In the reviewed Calico ARM9 startup, the main stack defaults to DTCM.
 * Outbound destination here must be uncached/device memory. Cached main-RAM
 * destinations need the inbound protocol as well. Not an ARM7 helper.
 */
#include <nds.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DCACHE_LINE_BYTES 32u
/* Deliberately exclude the special zero-encoded maximum-length transfer. */
#define ARM9_DMA_MAX_WORDS 0x1fffffu

/* true: started, or a zero-byte no-op; false: rejected without starting DMA.
 * This validation does NOT prove ownership, capacity or DMA addressability.
 * Keep the source alive and unchanged until dma_wait() completes.
 */
static inline bool dma_publish_words_async(unsigned channel,
                                          const void *source,
                                          void *destination,
                                          size_t byte_count)
{
    if (byte_count == 0u) return true; /* No cache/MMIO access, even for NULL. */
    if (channel >= 4u || source == NULL || destination == NULL) return false;
    if ((((uintptr_t)source | (uintptr_t)destination | byte_count) & 3u) != 0u)
        return false;
    if (byte_count / 4u > ARM9_DMA_MAX_WORDS) return false;
    if (byte_count > UINTPTR_MAX - (uintptr_t)source ||
        byte_count > UINTPTR_MAX - (uintptr_t)destination) return false;
    if (dmaBusy((uint8_t)channel)) return false;

    DC_FlushRange(source, (uint32_t)byte_count);
    dmaCopyWordsAsynch((uint8_t)channel, source, destination,
                      (uint32_t)byte_count);
    return true;
}

static inline bool dma_wait(unsigned channel)
{
    if (channel >= 4u) return false;
    /* Explicit wait. MMIO polling can stall; this is not useful CPU overlap. */
    while (dmaBusy((uint8_t)channel)) { }
    return true;
}

/* Entire allocation is owned by one transfer, not rounded across neighbors. */
struct __attribute__((aligned(DCACHE_LINE_BYTES))) InboundBuffer {
    uint8_t bytes[256];
};
_Static_assert(sizeof(struct InboundBuffer) % DCACHE_LINE_BYTES == 0,
               "inbound buffer must own complete cache lines");

static inline bool invalidate_owned_range(void *destination, size_t byte_count)
{
    if (byte_count == 0u) return true;
    if (destination == NULL ||
        (((uintptr_t)destination | byte_count) & (DCACHE_LINE_BYTES - 1u)) != 0u ||
        byte_count > UINTPTR_MAX - (uintptr_t)destination) return false;
#if SIZE_MAX > UINT32_MAX
    if (byte_count > UINT32_MAX) return false; /* Real narrowing on wider hosts. */
#endif
    DC_InvalidateRange(destination, (uint32_t)byte_count);
    return true;
}

/* Before handoff: old bytes are disposable; the producer will overwrite all
 * bytes later consumed. Do not touch this allocation until producer completion.
 * Partial writes requiring old bytes to survive need clean-before/invalidate-
 * after, or staging; do not discard dirty bytes that must be preserved.
 */
static inline bool prepare_inbound_buffer(struct InboundBuffer *buffer)
{
    return invalidate_owned_range(buffer, sizeof *buffer);
}

/* Caller has already waited for external producer completion. */
static inline bool finish_inbound_buffer(struct InboundBuffer *buffer)
{
    return invalidate_owned_range(buffer, sizeof *buffer);
}
