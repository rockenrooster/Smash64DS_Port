#ifndef SSB64_NDS_FIXED_CONVERT_H
#define SSB64_NDS_FIXED_CONVERT_H

#include <stdint.h>

/* One out-of-line copy of each float <-> fixed conversion (ARM state, in
 * battleship_sys_utils.c), for code that runs a few times a frame: inlined,
 * nds_r2_collision_mtx.h's forms cost 20-40 instructions a site and turned
 * the camera, particle-transform and wallpaper kernels into 1.5-3 KB of cold
 * main-RAM code (P2-2p8 2026-10-06 profile: CPI 5-8). The results are those
 * forms' own: ndsR2CollisionF32ToFixed (round half away, INT32_MIN past the
 * range) and ndsR2CollisionFixedToF32 (round to nearest even). */
int32_t ndsF32ToFixed(float value, unsigned int frac_bits);
float ndsFixedToF32(int32_t value, unsigned int frac_bits);
float ndsFixed64ToF32(int64_t value, unsigned int frac_bits);

#endif
