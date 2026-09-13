/* Compile with the actual ARM9 SDK, never tests/mocks.
 * These exported call sites keep the reusable DMA/cache helpers exercised.
 */
#include "../examples/dma_cache.c"

bool probe_dma_publish(unsigned ch, const void *src, void *dst, size_t bytes)
{
    return dma_publish_words_async(ch, src, dst, bytes);
}
bool probe_dma_wait(unsigned ch) { return dma_wait(ch); }
bool probe_cache_range(void *dst, size_t bytes)
{
    return invalidate_owned_range(dst,bytes);
}
bool probe_inbound_prepare(struct InboundBuffer *b) { return prepare_inbound_buffer(b); }
bool probe_inbound_finish(struct InboundBuffer *b) { return finish_inbound_buffer(b); }
