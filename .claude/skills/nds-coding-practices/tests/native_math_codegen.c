/* Real-SDK ARM9 native-helper probes. Compile/inspect, do not host-execute MMIO.
 * Native function range, zero-divisor, rounding and ownership rules still apply.
 */
#include <nds.h>

s32 probe_native_mul(s32 a, s32 b) { return mulf32(a,b); }
s32 probe_native_div(s32 a, s32 b) { return divf32(a,b); }
s32 probe_native_sqrt(s32 a) { return sqrtf32(a); }
s32 probe_native_sin(int angle) { return sinLerp(angle); }
