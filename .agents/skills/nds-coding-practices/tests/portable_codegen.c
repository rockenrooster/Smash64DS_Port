/* Compile-only probes with externally supplied inputs, not timing tests.
 * Caller must meet fixed_math.h preconditions. No SDK headers are included.
 */
#include "../examples/fixed_math.h"
#include "../examples/video_copy16.h"
#include "../examples/shared_mailbox.h"
#include "../examples/pxi/protocol.h"

int32_t probe_from_int(int32_t v) { return q20_12_from_int(v); }
int32_t probe_to_int(int32_t v) { return q20_12_to_int_trunc_zero(v); }
int32_t probe_mul_trunc(int32_t a, int32_t b) { return q20_12_mul_trunc_zero(a,b); }
int32_t probe_mul_round(int32_t a, int32_t b) { return q20_12_mul_round_away(a,b); }
int32_t probe_div(int32_t a, int32_t b) { return q20_12_div_trunc(a,b); }
int16_t probe_saturate(int32_t v) { return saturate_s16(v); }
bool probe_copy16(volatile uint16_t *dst, const uint16_t *src, size_t n)
{
    return nds_copy16_cpu(dst,src,n);
}
uint32_t probe_protocol(uint32_t value) { return nds_pxi_demo_answer(value); }
size_t probe_layout(void) { return sizeof(struct NdsMailbox); }
