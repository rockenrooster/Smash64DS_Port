#ifndef NDS_PARTICLE_VIEW_H
#define NDS_PARTICLE_VIEW_H

#include <stdint.h>
#include <limits.h>

/* A normalized affine camera in Q12, and a world centre in Q8 or Q12.
 * Evaluate once per particle, before constructing its four view-space corners.
 * The input is not changed on failure. Translation is in source world units;
 * the renderer's world/256 homogeneous convention is applied only at GX load. */
static inline int ndsParticleViewCenter(const int32_t view[4][4],
                                      int32_t center[3], unsigned fraction_bits)
{
    int32_t result[3];
    unsigned axis;

    if ((fraction_bits != 8u) && (fraction_bits != 12u)) { return 0; }
    for (axis = 0u; axis < 3u; axis++)
    {
        int64_t sum = (int64_t)view[0][axis] * center[0] +
                      (int64_t)view[1][axis] * center[1] +
                      (int64_t)view[2][axis] * center[2] +
                      (int64_t)view[3][axis] * (1u << fraction_bits);
        int64_t value = sum >> 12;

        if ((value < INT32_MIN) || (value > INT32_MAX)) { return 0; }
        result[axis] = (int32_t)value;
    }
    center[0] = result[0];
    center[1] = result[1];
    center[2] = result[2];
    return 1;
}

#endif
