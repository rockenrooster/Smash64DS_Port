#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include "../examples/dma_cache.c"
#include "../examples/fixed_math.h"
#include "../examples/video_copy16.h"
#include "../examples/shared_mailbox.h"
#include "../examples/pxi/protocol.h"

#define CHECK(c) do { if (!(c)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); exit(1); \
} } while (0)

static char calls[256];
static size_t call_count;
static bool busy;
static uint32_t last_bytes;
static const void *last_base;
static uint8_t last_channel;
static void record(char operation) {
    CHECK(call_count + 1u < sizeof calls);
    calls[call_count++] = operation; calls[call_count] = '\0';
}
void DC_FlushRange(const void *base, uint32_t bytes) {
    record('F'); last_base = base; last_bytes = bytes;
}
void DC_InvalidateRange(void *base, uint32_t bytes) {
    record('I'); last_base = base; last_bytes = bytes;
}
void dmaCopyWordsAsynch(uint8_t channel, const void *src, void *dst, uint32_t bytes) {
    (void)src; (void)dst; record('D'); last_channel = channel; last_bytes = bytes;
}
int dmaBusy(uint8_t channel) { (void)channel; record('B'); return busy; }
static void reset_calls(void) { call_count = 0; calls[0] = '\0'; busy = false; }

static void test_dma(void) {
    uint32_t source[8] = {0}; uint32_t destination[8] = {0};
    reset_calls();
    CHECK(dma_publish_words_async(99, NULL, NULL, 0));
    CHECK(call_count == 0); /* No status read, flush, or transfer for no-op. */
    CHECK(!dma_publish_words_async(4, source, destination, 4));
    CHECK(!dma_publish_words_async(0, NULL, destination, 4));
    CHECK(!dma_publish_words_async(0, source, NULL, 4));
    CHECK(!dma_publish_words_async(0, source, destination, 3));
    CHECK(!dma_publish_words_async(0, (const char *)source + 1, destination, 4));
    CHECK(!dma_publish_words_async(0, source, destination,
                                  ((size_t)ARM9_DMA_MAX_WORDS + 1u) * 4u));
    CHECK(!dma_wait(4));
    CHECK(call_count == 0);
    busy = true;
    CHECK(!dma_publish_words_async(1, source, destination, sizeof source));
    CHECK(strcmp(calls, "B") == 0);
    reset_calls();
    CHECK(dma_publish_words_async(2, source, destination, sizeof source));
    CHECK(strcmp(calls, "BFD") == 0);
    CHECK(last_channel == 2 && last_bytes == sizeof source && last_base == source);
    CHECK(dma_wait(2));
    CHECK(strcmp(calls, "BFDB") == 0);
    /* Host destination is intentionally mocked, not a hardware memory proof. */
}

static void test_cache(void) {
    struct InboundBuffer buffer;
    reset_calls();
    CHECK(invalidate_owned_range(NULL, 0));
    CHECK(!invalidate_owned_range(NULL, 32));
    CHECK(!invalidate_owned_range(buffer.bytes + 1, 32));
    CHECK(!invalidate_owned_range(&buffer, 31));
    CHECK(call_count == 0);
    CHECK(prepare_inbound_buffer(&buffer));
    CHECK(strcmp(calls, "I") == 0 && last_base == &buffer);
    CHECK(last_bytes == sizeof buffer);
    CHECK(finish_inbound_buffer(&buffer));
    CHECK(strcmp(calls, "II") == 0);
}

static void test_copy(void) {
    const uint16_t source[] = {0, 1, 0xABCD, 0xFFFF};
    uint16_t destination[] = {9,9,9,9,0x1234};
    CHECK(nds_copy16_cpu(NULL, NULL, 0));
    CHECK(!nds_copy16_cpu(NULL, source, 1));
    CHECK(!nds_copy16_cpu(destination, NULL, 1));
    CHECK(!nds_copy16_cpu(destination, source, SIZE_MAX));
    CHECK(nds_copy16_cpu(destination, source, 4));
    CHECK(memcmp(destination, source, sizeof source) == 0);
    CHECK(destination[4] == 0x1234);
}

static uint32_t rng_state = 0x6A09E667u;
static uint32_t next_u32(void) {
    rng_state = rng_state * 1664525u + 1013904223u; return rng_state;
}
static void test_math(void) {
    CHECK(q20_12_from_int(-524288) == INT32_MIN);
    CHECK(q20_12_from_int(524287) == 2147479552);
    CHECK(q20_12_to_int_trunc_zero(-4095) == 0);
    CHECK(q20_12_to_int_trunc_zero(-4097) == -1);
    CHECK(q20_12_mul_trunc_zero(-1, 2048) == 0);
    CHECK(q20_12_mul_round_away(-1, 2048) == -1);
    CHECK(q20_12_mul_round_away(1, 2048) == 1);
    CHECK(q20_12_mul_trunc_zero(INT32_MIN, 4096) == INT32_MIN);
    CHECK(q20_12_mul_trunc_zero(INT32_MAX, 4096) == INT32_MAX);
    CHECK(q20_12_div_trunc(-4096, 12288) == -1365);
    CHECK(saturate_s16(INT32_MIN) == INT16_MIN);
    CHECK(saturate_s16(INT32_MAX) == INT16_MAX);
    CHECK(saturate_s16(-123) == -123);
    for (unsigned i = 0; i < 10000; ++i) {
        const int32_t a = (int32_t)(next_u32() % 200001u) - 100000;
        const int32_t b = (int32_t)(next_u32() % 200001u) - 100000;
        const int64_t product = (int64_t)a * b;
        const int64_t magnitude = product < 0 ? -product : product;
        const int64_t sign = product < 0 ? -1 : 1;
        CHECK(q20_12_mul_trunc_zero(a,b) == sign * (magnitude / 4096));
        CHECK(q20_12_mul_round_away(a,b) == sign * ((magnitude + 2048) / 4096));
        if (b != 0) CHECK(q20_12_div_trunc(a,b) == ((int64_t)a * 4096) / b);
    }
}

static void test_protocol(void) {
    CHECK(nds_pxi_demo_answer(0) == 0);
    CHECK(nds_pxi_demo_answer(NDS_PXI_DEMO_VALUE_MAX) == NDS_PXI_DEMO_VALUE_MAX);
    CHECK(nds_pxi_demo_answer(NDS_PXI_DEMO_STOP) == NDS_PXI_DEMO_STOPPED);
    CHECK(nds_pxi_demo_answer(NDS_PXI_DEMO_STOP + 1u) == NDS_PXI_DEMO_ERROR);
    CHECK(nds_pxi_demo_answer(0x02000000u) == NDS_PXI_DEMO_ERROR);
    CHECK(nds_pxi_demo_answer(UINT32_MAX) == NDS_PXI_DEMO_ERROR);
    for (unsigned i = 0; i < 10000; ++i) {
        const uint32_t value = next_u32() & NDS_PXI_DEMO_VALUE_MAX;
        CHECK(nds_pxi_demo_answer(value) == value);
        CHECK(nds_pxi_demo_answer(value) < (1u << 26));
    }
}
int main(void) {
    CHECK(sizeof(struct NdsMailbox) == 128);
    CHECK(_Alignof(struct NdsMailbox) == 32);
    CHECK(offsetof(struct NdsMailbox, producer_sequence) == 64);
    CHECK(offsetof(struct NdsMailbox, consumer_sequence) == 96);
    test_dma(); test_cache(); test_copy(); test_math(); test_protocol();
    puts("PASS: DMA/cache call contracts, CPU-copy logic, mailbox layout, fixed math (10000 pairs), PXI protocol (10000 values)");
    return 0;
}
