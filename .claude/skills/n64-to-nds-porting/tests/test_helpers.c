#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "../examples/n64_data.h"
#include "../examples/n64_numeric.h"
#include "../examples/tick_ratio.h"

#define CHECK(c) do { if (!(c)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); return 1; \
} } while (0)

static void put16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value >> 8); p[1] = (uint8_t)value;
}

static uint32_t next_random(uint32_t *state)
{
    *state = *state * UINT32_C(1664525) + UINT32_C(1013904223);
    return *state;
}

static int32_t oracle_floor(int32_t v, unsigned s)
{
    const int64_t denominator = INT64_C(1) << s;
    int64_t q = (int64_t)v / denominator;
    if ((int64_t)v % denominator < 0) --q;
    return (int32_t)q;
}

static int32_t oracle_rne(int32_t v)
{
    int64_t q = (int64_t)v / 16;
    int64_t r = (int64_t)v % 16;
    if (r < 0) { --q; r += 16; }
    if (r > 8 || (r == 8 && q % 2 != 0)) ++q;
    return (int32_t)q;
}

int main(void)
{
    CHECK(port_span_ok(0, 0, 0));
    CHECK(port_span_ok(SIZE_MAX, SIZE_MAX, 0));
    CHECK(!port_span_ok(SIZE_MAX, SIZE_MAX, 1));
    CHECK(!port_span_ok(10, 11, 0));
    CHECK(!port_span_ok(10, 2, SIZE_MAX));
    uint8_t unaligned[] = {0, 0x80, 0x00, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff};
    CHECK(port_be16(unaligned + 1) == 0x8000u);
    CHECK(port_be_s16(unaligned + 1) == INT16_MIN);
    CHECK(port_be_s16(unaligned + 3) == -1);
    CHECK(port_be_s32(unaligned + 1) == INT32_C(-2147418113));
    CHECK(port_be_s32(unaligned + 5) == INT32_MAX);
    uint8_t minimum[] = {0x80, 0, 0, 0};
    uint8_t negative[] = {0xff, 0xff, 0xff, 0xff};
    CHECK(port_be_s32(minimum) == INT32_MIN);
    CHECK(port_be_s32(negative) == -1);

    PortSourceSpan segments[16] = {{0, 0}};
    segments[3].bytes = unaligned; segments[3].size = sizeof unaligned;
    const uint8_t *resolved = unaligned;
    CHECK(port_resolve_segment(segments, UINT32_C(0x03000001), 4, &resolved));
    CHECK(resolved == unaligned + 1);
    CHECK(!port_resolve_segment(segments, UINT32_C(0x83000001), 4, &resolved));
    CHECK(!port_resolve_segment(segments, UINT32_C(0x03000009), 1, &resolved));
    CHECK(!port_resolve_segment(segments, UINT32_C(0x03000000), 0, &resolved));
    CHECK(!port_resolve_segment(segments, UINT32_C(0x02000000), 1, &resolved));
    CHECK(resolved == unaligned + 1);
    CHECK(!port_resolve_segment(NULL, 0, 4, &resolved));

    uint8_t matrix[65] = {0}; /* Deliberately decode an unaligned Mtx. */
    const int32_t values[16] = {INT32_MIN, INT32_MAX, -1, 0, 1, 65536, -65536,
        32768, -32768, 0x12345678, -1234567, 16, -16, 8, -8, 42};
    for (unsigned i = 0; i < 16; ++i) {
        const uint32_t u = (uint32_t)values[i];
        put16(matrix + 1 + 2*i, (uint16_t)(u >> 16));
        put16(matrix + 33 + 2*i, (uint16_t)u);
    }
    for (unsigned i = 0; i < 16; ++i) {
        int32_t value = 55;
        CHECK(port_n64_mtx_element(matrix + 1, 64, i, &value));
        CHECK(value == values[i]);
    }
    int32_t unchanged = 123;
    CHECK(!port_n64_mtx_element(matrix, 63, 0, &unchanged));
    CHECK(!port_n64_mtx_element(matrix, 64, 16, &unchanged));
    CHECK(unchanged == 123);

    for (uint32_t word = 0; word <= UINT16_MAX; ++word) {
        uint16_t ds = port_rgba5551_to_ds((uint16_t)word);
        uint32_t back = ((uint32_t)(ds & 31u) << 11) |
                        ((uint32_t)((ds >> 5) & 31u) << 6) |
                        ((uint32_t)((ds >> 10) & 31u) << 1) | (ds >> 15);
        CHECK(back == word);
    }
    CHECK(port_rgba5551_to_ds(0xf801u) == 0x801fu); /* opaque red */
    CHECK(port_rgba5551_to_ds(0x07c1u) == 0x83e0u); /* opaque green */
    CHECK(port_rgba5551_to_ds(0x003fu) == 0xfc00u); /* opaque blue */
    CHECK(port_pack_s16_pair(-1, INT16_MIN) == UINT32_C(0x8000ffff));

    uint32_t rng = 1;
    for (unsigned i = 0; i < 10000; ++i) {
        uint32_t u = next_random(&rng);
        int32_t value = u <= INT32_MAX ? (int32_t)u : -1 - (int32_t)(UINT32_MAX - u);
        for (unsigned shift = 0; shift <= 31; ++shift)
            CHECK(port_floor_shift32(value, shift) == oracle_floor(value, shift));
        CHECK(port_q16_to_q12_rne(value) == oracle_rne(value));
    }
    for (unsigned i = 0; i < 16; ++i) {
        CHECK(port_q16_to_q12_rne(values[i]) == oracle_rne(values[i]));
        for (unsigned shift = 0; shift <= 31; ++shift)
            CHECK(port_floor_shift32(values[i], shift) == oracle_floor(values[i], shift));
    }
    for (int32_t v = -4096; v <= 4096; ++v)
        CHECK(port_q16_to_q12_rne(v) == oracle_rne(v));
    for (unsigned scale = 0; scale <= 27; ++scale) {
        int64_t divisor = INT64_C(1) << scale;
        for (int32_t v = INT16_MIN; v <= INT16_MAX; ++v) {
            int64_t numerator = (int64_t)v * 4096;
            int64_t expected = numerator / divisor;
            if (numerator % divisor < 0) --expected;
            int16_t out = 1234;
            bool fits = expected >= INT16_MIN && expected <= INT16_MAX;
            CHECK(port_s16_to_v16((int16_t)v, scale, &out) == fits);
            CHECK(out == (fits ? (int16_t)expected : 1234));
        }
    }
    int16_t packed = 7;
    CHECK(!port_s16_to_v16(0, 28, &packed));
    CHECK(!port_s16_to_v16(0, 0, NULL));
    CHECK(packed == 7);

    PortTickRatio clock;
    CHECK(!port_tick_init(&clock, 0, 1));
    CHECK(!port_tick_init(&clock, 1, 0));
    CHECK(port_tick_init(&clock, 2, 1));
    for (unsigned i = 0; i < 60; ++i) CHECK(port_tick_pulse(&clock));
    CHECK(clock.debt == 120);
    CHECK(port_tick_commit_one(&clock) && clock.debt == 119);
    CHECK(port_tick_init(&clock, 5, 6));
    for (unsigned i = 0; i < 60; ++i) CHECK(port_tick_pulse(&clock));
    CHECK(clock.debt == 50 && clock.phase == 0);
    CHECK(port_tick_init(&clock, UINT32_MAX, UINT32_MAX - 1u));
    for (unsigned i = 0; i < 100; ++i) CHECK(port_tick_pulse(&clock));
    CHECK(clock.debt == 100 && clock.phase == 100);
    for (unsigned i = 0; i < 1000; ++i) {
        uint32_t n = next_random(&rng) % 1000000u + 1u;
        uint32_t d = next_random(&rng) % 1000000u + 1u;
        CHECK(port_tick_init(&clock, n, d));
        for (uint32_t pulses = 1; pulses <= 100; ++pulses) {
            CHECK(port_tick_pulse(&clock));
            CHECK(clock.debt == (uint64_t)n * pulses / d);
            CHECK(clock.phase == (uint64_t)n * pulses % d);
        }
    }
    CHECK(port_tick_init(&clock, UINT32_MAX, 1));
    CHECK(port_tick_pulse(&clock));
    PortTickRatio before = clock;
    CHECK(!port_tick_pulse(&clock));
    CHECK(memcmp(&clock, &before, sizeof clock) == 0);
    CHECK(port_tick_init(&clock, 1, 2));
    CHECK(!port_tick_commit_one(&clock));
    CHECK(port_tick_pulse(&clock) && clock.debt == 0);
    CHECK(port_tick_pulse(&clock) && clock.debt == 1);
    CHECK(port_tick_commit_one(&clock) && clock.debt == 0);
    puts("PASS: portable binary, color, matrix, numeric, and tick helpers");
    return 0;
}
