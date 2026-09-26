#ifndef NDS_DAMAGE_FLY_MDUST_TEXTURE_H
#define NDS_DAMAGE_FLY_MDUST_TEXTURE_H
#include <stdint.h>

/* Four disjoint A5I3 planes preserve all RGB5 intensities and all alpha5
 * values. Exactly one plane can cover a texel; the other three are transparent.
 * lane=3 reads the runtime's word-swapped asset, lane=0 the source BE bytes. */
static inline uint8_t ndsDamageFlyMDustLayerTexel(
    const uint8_t *source, unsigned pixel, unsigned lane, unsigned band)
{
    unsigned intensity = source[(pixel * 2u) ^ lane] >> 3;
    unsigned alpha = source[(pixel * 2u + 1u) ^ lane] >> 3;
    return (intensity >> 3) == band ?
        (uint8_t)((alpha << 3) | (intensity & 7u)) : 0u;
}
#endif
