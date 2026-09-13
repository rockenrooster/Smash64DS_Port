/* Freestanding target-code checks, not a linked Nintendo DS application. */
#include "../examples/n64_data.h"
#include "../examples/n64_numeric.h"
#include "../examples/tick_ratio.h"
uint16_t cg_color(uint16_t x) { return port_rgba5551_to_ds(x); }
uint32_t cg_pair(int16_t x, int16_t y) { return port_pack_s16_pair(x, y); }
uint32_t cg_be32(const uint8_t *p) { return port_be32(p); }
int32_t cg_q16(int32_t x) { return port_q16_to_q12_rne(x); }
bool cg_v16(int16_t x, int16_t *out) { return port_s16_to_v16(x, 8, out); }
bool cg_tick(PortTickRatio *c) { return port_tick_pulse(c); }
bool cg_matrix(const uint8_t *p, size_t n, unsigned i, int32_t *out)
{ return port_n64_mtx_element(p, n, i, out); }
