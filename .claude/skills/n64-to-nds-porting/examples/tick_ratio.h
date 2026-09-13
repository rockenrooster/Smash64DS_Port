#ifndef PORT_TICK_RATIO_H
#define PORT_TICK_RATIO_H

/* Account for a rational number of source ticks per caller-defined clock pulse.
 * A pulse is NOT automatically a DS presentation or an exact 1/60 second.
 * Use the project's canonical clock/rate. This helper does no input sampling,
 * catch-up policy, timer access, sleeping, or rendering.
 */
#include <stdbool.h>
#include <stdint.h>

typedef struct PortTickRatio {
    uint32_t whole, remainder, denominator, phase, debt;
} PortTickRatio;

/* Setup-time divides only. Initialization intentionally clears accumulated time. */
static inline bool port_tick_init(PortTickRatio *c, uint32_t numerator,
                                  uint32_t denominator)
{
    if (c == 0 || numerator == 0u || denominator == 0u)
        return false;
    c->whole = numerator / denominator;
    c->remainder = numerator % denominator;
    c->denominator = denominator;
    c->phase = 0u;
    c->debt = 0u;
    return true;
}

/* Once per elapsed pulse, not once per successfully drawn frame.
 * Overflow leaves the complete clock unchanged. Caller must handle failure.
 * No divide or wide arithmetic in this steady-state operation.
 */
static inline bool port_tick_pulse(PortTickRatio *c)
{
    if (c == 0 || c->denominator == 0u || c->phase >= c->denominator ||
        c->remainder >= c->denominator)
        return false;
    uint32_t add = c->whole;
    uint32_t phase;
    if (c->phase >= c->denominator - c->remainder) {
        if (add == UINT32_MAX)
            return false;
        ++add;
        phase = c->phase - (c->denominator - c->remainder);
    } else {
        phase = c->phase + c->remainder;
    }
    if (add > UINT32_MAX - c->debt)
        return false;
    c->phase = phase;
    c->debt += add;
    return true;
}

/* Call after one source update was actually completed. No silent tick dropping. */
static inline bool port_tick_commit_one(PortTickRatio *c)
{
    if (c == 0 || c->debt == 0u)
        return false;
    --c->debt;
    return true;
}
#endif
