/*
 * ARM9 cache/DMA patterns.
 *
 * This file is a pattern, not a DMA-channel allocator. The caller must reserve
 * the channel and prove address accessibility, unit alignment, non-overlap,
 * destination mapping, and object lifetime.
 */
#include <nds.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#define DCACHE_LINE_BYTES 32u

static uintptr_t align_down_cache_line(uintptr_t value)
{
    return value & ~(uintptr_t)(DCACHE_LINE_BYTES - 1u);
}

static uintptr_t align_up_cache_line(uintptr_t value)
{
    return (value + DCACHE_LINE_BYTES - 1u) &
           ~(uintptr_t)(DCACHE_LINE_BYTES - 1u);
}

/*
 * ARM9 produces cached main-RAM bytes; DMA consumes them.
 * Source must remain alive and unchanged until dma_wait() succeeds.
 */
static void dma_publish_words_async(uint8_t channel,
                                    const void *source,
                                    void *destination,
                                    size_t byte_count)
{
    assert((((uintptr_t)source | (uintptr_t)destination | byte_count) & 3u) == 0u);
    assert(byte_count <= UINT32_MAX);

    DC_FlushRange(source, (uint32_t)byte_count);
    dmaCopyWordsAsynch(channel, source, destination, (uint32_t)byte_count);
}

static void dma_wait(uint8_t channel)
{
    while (dmaBusy(channel)) {
        // Perform independent bounded work here only when ownership permits.
    }
}

/*
 * Reserve the whole rounded allocation for DMA/device-produced data. Never
 * align outward across unrelated dirty objects: invalidation discards cache
 * lines rather than merging neighboring writes.
 */
struct __attribute__((aligned(DCACHE_LINE_BYTES))) InboundBuffer {
    uint8_t bytes[256];
};

_Static_assert((sizeof(struct InboundBuffer) % DCACHE_LINE_BYTES) == 0,
               "inbound DMA allocation must own complete cache lines");

static void invalidate_owned_range(void *destination, size_t byte_count)
{
    const uintptr_t begin = align_down_cache_line((uintptr_t)destination);
    const uintptr_t end = align_up_cache_line((uintptr_t)destination + byte_count);

    assert((begin & (DCACHE_LINE_BYTES - 1u)) == 0u);
    assert(((end - begin) & (DCACHE_LINE_BYTES - 1u)) == 0u);
    assert((end - begin) <= UINT32_MAX);

    DC_InvalidateRange((void *)begin, (uint32_t)(end - begin));
}

/*
 * Inbound protocol shape:
 *   1. destination owns complete cache lines;
 *   2. invalidate before the external producer starts;
 *   3. start producer;
 *   4. wait for completion;
 *   5. invalidate again;
 *   6. read on ARM9.
 */
static void prepare_inbound_buffer(struct InboundBuffer *buffer)
{
    invalidate_owned_range(buffer, sizeof *buffer);
}

static void finish_inbound_buffer(struct InboundBuffer *buffer)
{
    invalidate_owned_range(buffer, sizeof *buffer);
}

/* Suppress unused warnings when compiling this file as a standalone pattern. */
static void example_only(void)
{
    (void)dma_publish_words_async;
    (void)dma_wait;
    (void)prepare_inbound_buffer;
    (void)finish_inbound_buffer;
}
