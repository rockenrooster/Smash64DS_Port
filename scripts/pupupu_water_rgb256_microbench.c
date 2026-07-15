#include <stdint.h>

#include "pupupu_water_rgb256_corpus.generated.h"


#define PUPUPU_WATER_RGB256_CLASS_KEY_MASK 0x1fffu
#define PUPUPU_WATER_RGB256_CLASS_INDEX_SHIFT 13u
#define PUPUPU_WATER_RGB256_OWNER_LARGE 0u
#define PUPUPU_WATER_RGB256_LARGE_WIDTH 128u
#define PUPUPU_WATER_RGB256_LARGE_HEIGHT 128u
#define PUPUPU_WATER_RGB256_SMALL_WIDTH 32u
#define PUPUPU_WATER_RGB256_SMALL_HEIGHT 64u


typedef struct PupupuWaterRgb256Scratch
{
    uint8_t pair_phase_lut[256][16];
    uint16_t palette[PUPUPU_WATER_RGB256_PALETTE_ENTRIES];
    uint8_t source0_s[128];
    uint8_t source1_s[128];
    uint8_t source0_t[128];
    uint8_t source1_t[128];
    uint8_t axis_representative_s[128];
    uint8_t axis_representative_t[128];
    uint32_t class_table[256];
    uint8_t source_indices0[1024];
    uint8_t source_indices1[1024];
} PupupuWaterRgb256Scratch;


typedef struct PupupuWaterRgb256Result
{
    uint32_t case_count;
    uint32_t map_bytes;
    uint32_t map_hash_mismatches;
    uint32_t palette_hash_mismatches;
    uint32_t scratch_bytes;
    uint32_t sink;
} PupupuWaterRgb256Result;


static const uint16_t sPupupuWaterRgb256AlphaPhasePrefix[17] = {
    0x0000u, 0x0001u, 0x0401u, 0x0405u, 0x0505u, 0x0525u,
    0x8525u, 0x85a5u, 0xa5a5u, 0xa5a7u, 0xada7u, 0xadafu,
    0xafafu, 0xafbfu, 0xefbfu, 0xefffu, 0xffffu
};


static uint32_t pupupuWaterRgb256Expand5To8(uint32_t value)
{
    value &= 0x1fu;
    return (value << 3) | (value >> 2);
}


static uint32_t pupupuWaterRgb256BlendValue(
    uint16_t texel0, uint16_t texel1, uint32_t fraction)
{
    uint32_t inverse = 0x100u - fraction;
    uint32_t red =
        (((pupupuWaterRgb256Expand5To8(texel0) * inverse) +
          (pupupuWaterRgb256Expand5To8(texel1) * fraction)) >> 8) >> 3;
    uint32_t green =
        (((pupupuWaterRgb256Expand5To8(texel0 >> 5) * inverse) +
          (pupupuWaterRgb256Expand5To8(texel1 >> 5) * fraction)) >> 8) >> 3;
    uint32_t blue =
        (((pupupuWaterRgb256Expand5To8(texel0 >> 10) * inverse) +
          (pupupuWaterRgb256Expand5To8(texel1 >> 10) * fraction)) >> 8) >> 3;
    uint32_t alpha_coverage =
        ((((texel0 >> 15) & 1u) * 0x100u * inverse) +
         (((texel1 >> 15) & 1u) * 0x100u * fraction)) >> 8;

    return red | (green << 5) | (blue << 10) | (alpha_coverage << 15);
}


void smash64dsPupupuWaterRgb256Prepare(
    PupupuWaterRgb256Scratch *scratch, uint32_t fraction)
{
    uint32_t pair;

    scratch->palette[0] = 0u;
    for (pair = 0u; pair < 256u; pair++)
    {
        uint32_t output_index = gPupupuWaterRgb256PairToIndex[pair];
        uint32_t alpha_mask = 0u;
        uint32_t phase;

        if (output_index != 0u)
        {
            uint32_t index0 = pair >> 4;
            uint32_t index1 = pair & 0x0fu;
            uint32_t value = pupupuWaterRgb256BlendValue(
                gPupupuWaterRgb256SourcePalette[index0],
                gPupupuWaterRgb256SourcePalette[index1], fraction);
            uint32_t prefix_count = ((value >> 15) + 7u) >> 4;

            if (prefix_count > 16u)
            {
                prefix_count = 16u;
            }
            alpha_mask = sPupupuWaterRgb256AlphaPhasePrefix[prefix_count];
            scratch->palette[output_index] = (uint16_t)(value & 0x7fffu);
        }
        for (phase = 0u; phase < 16u; phase++)
        {
            scratch->pair_phase_lut[pair][phase] =
                ((alpha_mask >> phase) & 1u) != 0u ?
                (uint8_t)output_index : 0u;
        }
    }
}


void smash64dsPupupuWaterRgb256ExpandCi4(
    const uint8_t *packed, uint8_t *indices)
{
    uint32_t offset;

    for (offset = 0u; offset < 512u; offset++)
    {
        uint8_t value = packed[offset];

        indices[offset << 1] = value >> 4;
        indices[(offset << 1) + 1u] = value & 0x0fu;
    }
}


static void pupupuWaterRgb256BuildAxis(
    PupupuWaterRgb256Scratch *scratch,
    uint8_t *source0_axis,
    uint8_t *source1_axis,
    uint8_t *axis_representative,
    uint32_t owner,
    uint32_t extent,
    int32_t delta,
    uint32_t is_s)
{
    uint32_t index;

    for (index = 0u; index < 256u; index++)
    {
        scratch->class_table[index] = 0u;
    }
    for (index = 0u; index < extent; index++)
    {
        uint32_t source0;
        int32_t shifted = (int32_t)index + delta;
        uint32_t source1;
        uint32_t class_value;
        uint32_t key;
        uint32_t stored_key;
        uint32_t slot;

        if ((owner == PUPUPU_WATER_RGB256_OWNER_LARGE) && (is_s != 0u))
        {
            uint32_t local = index & 31u;

            source0 = (((index >> 5) & 1u) != 0u) ? 31u - local : local;
        }
        else
        {
            source0 = index & 31u;
        }
        if (shifted < 0)
        {
            shifted = 0;
        }
        else if ((uint32_t)shifted >= extent)
        {
            shifted = (int32_t)extent - 1;
        }
        source1 = (uint32_t)shifted & 31u;
        class_value = source0 | (source1 << 5);
        source0_axis[index] = (uint8_t)source0;
        source1_axis[index] = (uint8_t)source1;
        key = class_value | ((index & 3u) << 10);
        stored_key = key + 1u;
        slot = (key * 0x9e3779b1u) >> 24;
        while (scratch->class_table[slot] != 0u)
        {
            uint32_t entry = scratch->class_table[slot];

            if ((entry & PUPUPU_WATER_RGB256_CLASS_KEY_MASK) == stored_key)
            {
                axis_representative[index] =
                    (uint8_t)(entry >> PUPUPU_WATER_RGB256_CLASS_INDEX_SHIFT);
                break;
            }
            slot = (slot + 1u) & 255u;
        }
        if (scratch->class_table[slot] == 0u)
        {
            scratch->class_table[slot] =
                (index << PUPUPU_WATER_RGB256_CLASS_INDEX_SHIFT) | stored_key;
            axis_representative[index] = (uint8_t)index;
        }
    }
}


void smash64dsPupupuWaterRgb256Generate(
    PupupuWaterRgb256Scratch *scratch,
    uint32_t *output_words,
    const uint8_t *indices0,
    const uint8_t *indices1,
    uint32_t owner,
    int32_t delta_s,
    int32_t delta_t)
{
    uint32_t width = (owner == PUPUPU_WATER_RGB256_OWNER_LARGE) ?
        PUPUPU_WATER_RGB256_LARGE_WIDTH : PUPUPU_WATER_RGB256_SMALL_WIDTH;
    uint32_t height = (owner == PUPUPU_WATER_RGB256_OWNER_LARGE) ?
        PUPUPU_WATER_RGB256_LARGE_HEIGHT : PUPUPU_WATER_RGB256_SMALL_HEIGHT;
    uint8_t *output = (uint8_t *)output_words;
    uint32_t y;

    pupupuWaterRgb256BuildAxis(
        scratch, scratch->source0_s, scratch->source1_s,
        scratch->axis_representative_s,
        owner, width, delta_s, 1u);
    pupupuWaterRgb256BuildAxis(
        scratch, scratch->source0_t, scratch->source1_t,
        scratch->axis_representative_t,
        owner, height, delta_t, 0u);

    for (y = 0u; y < height; y++)
    {
        uint32_t representative_y = scratch->axis_representative_t[y];

        if (representative_y != y)
        {
            uint32_t *destination = output_words + ((y * width) >> 2);
            const uint32_t *source =
                output_words + ((representative_y * width) >> 2);
            uint32_t word;

            for (word = 0u; word < (width >> 2); word++)
            {
                destination[word] = source[word];
            }
        }
        else
        {
            uint32_t source0_row = (uint32_t)scratch->source0_t[y] << 5;
            uint32_t source1_row = (uint32_t)scratch->source1_t[y] << 5;
            uint32_t row = y * width;
            uint32_t x;

            for (x = 0u; x < width; x++)
            {
                uint32_t representative_x =
                    scratch->axis_representative_s[x];

                if (representative_x != x)
                {
                    output[row + x] = output[row + representative_x];
                }
                else
                {
                    uint32_t index0 = indices0[
                        source0_row + scratch->source0_s[x]];
                    uint32_t index1 = indices1[
                        source1_row + scratch->source1_s[x]];
                    uint32_t pair = (index0 << 4) | index1;
                    uint32_t phase = ((y & 3u) << 2) | (x & 3u);

                    output[row + x] = scratch->pair_phase_lut[pair][phase];
                }
            }
        }
    }
}


static uint32_t pupupuWaterRgb256Hash(const uint8_t *payload, uint32_t bytes)
{
    uint32_t hash = 2166136261u;
    uint32_t index;

    for (index = 0u; index < bytes; index++)
    {
        hash ^= payload[index];
        hash *= 16777619u;
    }
    return hash;
}


void smash64dsPupupuWaterRgb256Microbench(
    PupupuWaterRgb256Result *result,
    PupupuWaterRgb256Scratch *scratch,
    uint32_t *output_words)
{
    uint32_t map_hash_mismatches = 0u;
    uint32_t palette_hash_mismatches = 0u;
    uint32_t map_bytes = 0u;
    uint32_t sink = 2166136261u;
    uint32_t index;

    if ((result == 0) || (scratch == 0) || (output_words == 0))
    {
        return;
    }
    for (index = 0u; index < PUPUPU_WATER_RGB256_CASE_COUNT; index++)
    {
        const PupupuWaterRgb256Case *test = &gPupupuWaterRgb256Cases[index];
        uint32_t width =
            (test->owner == PUPUPU_WATER_RGB256_OWNER_LARGE) ? 128u : 32u;
        uint32_t height =
            (test->owner == PUPUPU_WATER_RGB256_OWNER_LARGE) ? 128u : 64u;
        uint32_t bytes = width * height;
        uint32_t map_hash;
        uint32_t palette_hash;

        smash64dsPupupuWaterRgb256Prepare(scratch, test->fraction);
        smash64dsPupupuWaterRgb256ExpandCi4(
            gPupupuWaterRgb256Textures[test->texture0],
            scratch->source_indices0);
        smash64dsPupupuWaterRgb256ExpandCi4(
            gPupupuWaterRgb256Textures[test->texture1],
            scratch->source_indices1);
        smash64dsPupupuWaterRgb256Generate(
            scratch, output_words, scratch->source_indices0,
            scratch->source_indices1,
            test->owner, test->delta_s, test->delta_t);
        map_hash = pupupuWaterRgb256Hash((const uint8_t *)output_words, bytes);
        palette_hash = pupupuWaterRgb256Hash(
            (const uint8_t *)scratch->palette,
            PUPUPU_WATER_RGB256_PALETTE_ENTRIES * sizeof(uint16_t));
        map_hash_mismatches += map_hash != test->map_fnv1a;
        palette_hash_mismatches += palette_hash != test->palette_fnv1a;
        map_bytes += bytes;
        sink ^= map_hash;
        sink *= 16777619u;
        sink ^= palette_hash;
        sink *= 16777619u;
    }
    result->case_count = PUPUPU_WATER_RGB256_CASE_COUNT;
    result->map_bytes = map_bytes;
    result->map_hash_mismatches = map_hash_mismatches;
    result->palette_hash_mismatches = palette_hash_mismatches;
    result->scratch_bytes = sizeof(*scratch);
    result->sink = sink;
}


#ifdef PUPUPU_WATER_RGB256_HOST_MAIN
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


static PupupuWaterRgb256Scratch sHostScratch;
static uint32_t sHostOutput[PUPUPU_WATER_RGB256_MAX_MAP_BYTES / 4u];
static volatile uint32_t sHostTimingSink;


static int pupupuWaterRgb256CompareU64(const void *left, const void *right)
{
    uint64_t a = *(const uint64_t *)left;
    uint64_t b = *(const uint64_t *)right;

    return (a > b) - (a < b);
}


static uint64_t pupupuWaterRgb256TimeCase(
    const PupupuWaterRgb256Case *test)
{
    uint32_t repeat = 8u;
    clock_t begin;
    clock_t elapsed;
    uint32_t iteration;

    smash64dsPupupuWaterRgb256Prepare(&sHostScratch, test->fraction);
    smash64dsPupupuWaterRgb256ExpandCi4(
        gPupupuWaterRgb256Textures[test->texture0],
        sHostScratch.source_indices0);
    smash64dsPupupuWaterRgb256ExpandCi4(
        gPupupuWaterRgb256Textures[test->texture1],
        sHostScratch.source_indices1);
    do
    {
        repeat <<= 1;
        begin = clock();
        for (iteration = 0u; iteration < repeat; iteration++)
        {
            smash64dsPupupuWaterRgb256Generate(
                &sHostScratch, sHostOutput, sHostScratch.source_indices0,
                sHostScratch.source_indices1,
                test->owner, test->delta_s, test->delta_t);
            sHostTimingSink ^= sHostOutput[iteration & 4095u];
        }
        elapsed = clock() - begin;
    }
    while ((elapsed < (CLOCKS_PER_SEC / 200)) && (repeat < 8192u));
    return ((uint64_t)elapsed * 1000000000ull) /
        ((uint64_t)CLOCKS_PER_SEC * repeat);
}


static uint64_t pupupuWaterRgb256TimePalette(void)
{
    uint32_t repeat = 32768u;
    uint32_t iteration;
    clock_t begin = clock();
    clock_t elapsed;

    for (iteration = 0u; iteration < repeat; iteration++)
    {
        smash64dsPupupuWaterRgb256Prepare(
            &sHostScratch, gPupupuWaterRgb256Cases[iteration %
                PUPUPU_WATER_RGB256_CASE_COUNT].fraction);
        sHostTimingSink ^=
            sHostScratch.pair_phase_lut[iteration & 255u][iteration & 15u];
    }
    elapsed = clock() - begin;
    return ((uint64_t)elapsed * 1000000000ull) /
        ((uint64_t)CLOCKS_PER_SEC * repeat);
}


static uint64_t pupupuWaterRgb256TimeDecode(void)
{
    uint32_t repeat = 32768u;
    uint32_t iteration;
    clock_t begin = clock();
    clock_t elapsed;

    for (iteration = 0u; iteration < repeat; iteration++)
    {
        smash64dsPupupuWaterRgb256ExpandCi4(
            gPupupuWaterRgb256Textures[iteration % 3u],
            sHostScratch.source_indices0);
        sHostTimingSink ^= sHostScratch.source_indices0[iteration & 1023u];
    }
    elapsed = clock() - begin;
    return ((uint64_t)elapsed * 1000000000ull) /
        ((uint64_t)CLOCKS_PER_SEC * repeat);
}


int main(int argc, char **argv)
{
    PupupuWaterRgb256Result result;
    FILE *map_output;
    FILE *palette_output;
    uint64_t large_times[165];
    uint64_t small_times[157];
    uint32_t large_count = 0u;
    uint32_t small_count = 0u;
    uint32_t index;

    if (argc != 3)
    {
        fprintf(stderr, "usage: %s MAP_OUTPUT PALETTE_OUTPUT\n", argv[0]);
        return 2;
    }
    smash64dsPupupuWaterRgb256Microbench(
        &result, &sHostScratch, sHostOutput);
    if ((result.case_count != PUPUPU_WATER_RGB256_CASE_COUNT) ||
        (result.map_bytes != PUPUPU_WATER_RGB256_ORACLE_BYTES) ||
        (result.map_hash_mismatches != 0u) ||
        (result.palette_hash_mismatches != 0u))
    {
        fprintf(
            stderr,
            "PUPUPU_WATER_RGB256_HOST_FAIL cases=%lu bytes=%lu map=%lu palette=%lu\n",
            (unsigned long)result.case_count,
            (unsigned long)result.map_bytes,
            (unsigned long)result.map_hash_mismatches,
            (unsigned long)result.palette_hash_mismatches);
        return 1;
    }
    map_output = fopen(argv[1], "wb");
    palette_output = fopen(argv[2], "wb");
    if ((map_output == NULL) || (palette_output == NULL))
    {
        fprintf(stderr, "unable to open RGB256 host output files\n");
        if (map_output != NULL)
        {
            fclose(map_output);
        }
        if (palette_output != NULL)
        {
            fclose(palette_output);
        }
        return 1;
    }
    for (index = 0u; index < PUPUPU_WATER_RGB256_CASE_COUNT; index++)
    {
        const PupupuWaterRgb256Case *test = &gPupupuWaterRgb256Cases[index];
        uint32_t bytes = (test->owner == PUPUPU_WATER_RGB256_OWNER_LARGE) ?
            128u * 128u : 32u * 64u;
        uint64_t elapsed_ns;

        smash64dsPupupuWaterRgb256Prepare(&sHostScratch, test->fraction);
        smash64dsPupupuWaterRgb256ExpandCi4(
            gPupupuWaterRgb256Textures[test->texture0],
            sHostScratch.source_indices0);
        smash64dsPupupuWaterRgb256ExpandCi4(
            gPupupuWaterRgb256Textures[test->texture1],
            sHostScratch.source_indices1);
        smash64dsPupupuWaterRgb256Generate(
            &sHostScratch, sHostOutput, sHostScratch.source_indices0,
            sHostScratch.source_indices1,
            test->owner, test->delta_s, test->delta_t);
        if ((fwrite(sHostOutput, 1u, bytes, map_output) != bytes) ||
            (fwrite(
                sHostScratch.palette, sizeof(uint16_t),
                PUPUPU_WATER_RGB256_PALETTE_ENTRIES, palette_output) !=
             PUPUPU_WATER_RGB256_PALETTE_ENTRIES))
        {
            fprintf(stderr, "unable to write RGB256 host outputs\n");
            fclose(map_output);
            fclose(palette_output);
            return 1;
        }
        elapsed_ns = pupupuWaterRgb256TimeCase(test);
        if (test->owner == PUPUPU_WATER_RGB256_OWNER_LARGE)
        {
            large_times[large_count++] = elapsed_ns;
        }
        else
        {
            small_times[small_count++] = elapsed_ns;
        }
    }
    fclose(map_output);
    fclose(palette_output);
    qsort(large_times, large_count, sizeof(large_times[0]),
          pupupuWaterRgb256CompareU64);
    qsort(small_times, small_count, sizeof(small_times[0]),
          pupupuWaterRgb256CompareU64);
    printf(
        "PUPUPU_WATER_RGB256_HOST_PARITY_OK cases=%lu map_bytes=%lu "
        "map_hash_mismatches=%lu palette_hash_mismatches=%lu "
        "scratch_bytes=%lu sink=%08lx\n",
        (unsigned long)result.case_count,
        (unsigned long)result.map_bytes,
        (unsigned long)result.map_hash_mismatches,
        (unsigned long)result.palette_hash_mismatches,
        (unsigned long)result.scratch_bytes,
        (unsigned long)result.sink);
    printf(
        "PUPUPU_WATER_RGB256_HOST_TIMING_ADVISORY "
        "large_median_ns=%llu large_worst_ns=%llu "
        "small_median_ns=%llu small_worst_ns=%llu palette_ns=%llu "
        "ci4_decode_ns=%llu\n",
        (unsigned long long)large_times[large_count / 2u],
        (unsigned long long)large_times[large_count - 1u],
        (unsigned long long)small_times[small_count / 2u],
        (unsigned long long)small_times[small_count - 1u],
        (unsigned long long)pupupuWaterRgb256TimePalette(),
        (unsigned long long)pupupuWaterRgb256TimeDecode());
    printf(
        "PUPUPU_WATER_RGB256_HOST_NOTE timing_is_advisory_not_arm_cycles "
        "map_sha256=%s\n",
        PUPUPU_WATER_RGB256_MAP_SHA256);
    return 0;
}
#endif
