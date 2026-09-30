#include <stdint.h>
typedef int32_t s32; typedef uint32_t u32; typedef int64_t s64; typedef float f32;
extern volatile u32 gNdsR2CubicSaturations;
/* The header as of 7a9fc67dbe8: git show 7a9fc67dbe8:include/nds/nds_anim_fixed.h > old_nds_anim_fixed.h */
#include "old_nds_anim_fixed.h"
s32 eval_old(s32 len, s32 inv, s32 vb, s32 vt, s32 rb, s32 rt, u32 kind) { return ndsR2AnimEvalQ(len, inv, vb, vt, rb, rt, kind); }
