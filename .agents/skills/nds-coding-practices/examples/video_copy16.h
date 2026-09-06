#ifndef NDS_PRACTICES_VIDEO_COPY16_H
#define NDS_PRACTICES_VIDEO_COPY16_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Small CPU upload, no DMA channel/cache publication required.
 * Source is a real, aligned uint16_t array; both ranges have enough capacity
 * and do not overlap. Destination is CPU-writable video memory with a stable
 * mapping and a safe update window. This is not a general byte-buffer parser.
 * volatile on the destination preserves explicit halfword stores.
 */
static inline bool nds_copy16_cpu(volatile uint16_t *destination,
                                 const uint16_t *source,
                                 size_t halfword_count)
{
    if (halfword_count == 0u) return true;
    if (destination == NULL || source == NULL ||
        (((uintptr_t)destination | (uintptr_t)source) & 1u) != 0u ||
        halfword_count > SIZE_MAX / sizeof(uint16_t)) return false;
    for (size_t i = 0; i < halfword_count; ++i) destination[i] = source[i];
    return true;
}
#endif
