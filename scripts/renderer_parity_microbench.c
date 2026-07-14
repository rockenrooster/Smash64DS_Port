#include <stdint.h>

#include "renderer_parity_corpus.generated.h"


typedef struct RendererParityResult
{
    uint32_t rgb_case_count;
    uint32_t rgb_mismatch_count;
    uint32_t rgb_reference_mismatch_count;
    uint32_t hardware_light_demoted;
    uint32_t t16_case_count;
    uint32_t t16_mismatch_count;
    uint32_t t16_naive_mismatch_count;
    uint32_t texture_matrix_demoted;
    uint32_t sink;
} RendererParityResult;


static int32_t rendererParityFloorShift(int64_t value, uint32_t shift)
{
    uint64_t magnitude;
    uint64_t mask = ((uint64_t)1u << shift) - 1u;

    if (value >= 0)
    {
        return (int32_t)((uint64_t)value >> shift);
    }
    magnitude = (uint64_t)(-value);
    return -(int32_t)((magnitude + mask) >> shift);
}


static uint32_t rendererParitySourceRgb5(const RendererParityRgbCase *test)
{
    int32_t dot;
    uint32_t diffuse_numer;
    uint32_t shade;

    dot = (int32_t)test->nz * 127;
    if (dot <= 0)
    {
        diffuse_numer = 0u;
    }
    else if (dot > (127 * 127))
    {
        diffuse_numer = 127u;
    }
    else
    {
        diffuse_numer = (uint32_t)dot / 127u;
    }
    shade = 0x4cu + (0xffu * diffuse_numer) / 127u;
    if (shade > 0xffu)
    {
        shade = 0xffu;
    }
    return shade >> 3;
}


static uint32_t rendererParityDeviceRgb5(
    const RendererParityRgbCase *test,
    const RendererParityDsMaterial *material)
{
    int32_t normal[3];
    int32_t light[3] = { 0, 0, 127 * 4 };
    int32_t dot = 0;
    uint32_t vertex;
    uint32_t lane;
    uint32_t raw_diffdot;
    int32_t diffdot;

    normal[0] = (int32_t)test->nx * 4;
    normal[1] = (int32_t)test->ny * 4;
    normal[2] = (int32_t)test->nz * 4;
    for (lane = 0u; lane < 3u; lane++)
    {
        dot += rendererParityFloorShift(
            (int64_t)light[lane] * normal[lane], 9u);
    }
    vertex = ((uint32_t)material->ambient_material << 9) *
        (uint32_t)material->light_color;
    if (dot > 0)
    {
        raw_diffdot = (uint32_t)dot & 0x7ffu;
        diffdot = (raw_diffdot >= 0x400u) ?
            (int32_t)raw_diffdot - 0x800 : (int32_t)raw_diffdot;
        vertex += ((uint32_t)material->diffuse_material *
                   (uint32_t)material->light_color *
                   (uint32_t)diffdot) & 0xfffffu;
    }
    vertex >>= 14;
    return (vertex > 31u) ? 31u : vertex;
}


static uint32_t rendererParityPackRgb15(uint32_t channel)
{
    return channel | (channel << 5) | (channel << 10);
}


static int32_t rendererParitySourceT16(
    int32_t coord, uint32_t scale, uint32_t origin, int32_t offset)
{
    return rendererParityFloorShift((int64_t)coord * scale, 17u) -
        ((int32_t)origin * 4) + offset;
}


static int32_t rendererParityDeviceT16(
    int32_t coord, int32_t coefficient, int32_t bias)
{
    return rendererParityFloorShift(
        (int64_t)coord * coefficient + bias, 12u);
}


static uint32_t rendererParityHash(uint32_t hash, uint32_t value)
{
    hash ^= value;
    hash *= 16777619u;
    return hash;
}


void smash64dsRendererParityMicrobench(RendererParityResult *out)
{
    uint32_t binding_mismatches[RENDERER_PARITY_RGB_BINDING_COUNT] = { 0u };
    uint32_t rgb_mismatches = 0u;
    uint32_t rgb_reference_mismatches = 0u;
    uint32_t t16_mismatches = 0u;
    uint32_t t16_naive_mismatches = 0u;
    uint32_t sink = 2166136261u;
    uint32_t i;
    const RendererParityDsMaterial reference = { 31u, 10u, 31u };

    if (out == 0)
    {
        return;
    }
    for (i = 0u; i < RENDERER_PARITY_RGB_CASE_COUNT; i++)
    {
        const RendererParityRgbCase *test = &gRendererParityRgbCases[i];
        const RendererParityDsMaterial *material =
            &gRendererParityDsMaterials[test->binding];
        uint32_t expected = rendererParitySourceRgb5(test);
        uint32_t actual = rendererParityDeviceRgb5(test, material);
        uint32_t reference_actual = rendererParityDeviceRgb5(test, &reference);

        sink = rendererParityHash(sink, rendererParityPackRgb15(expected));
        sink = rendererParityHash(sink, rendererParityPackRgb15(actual));
        if (expected != actual)
        {
            rgb_mismatches++;
            binding_mismatches[test->binding]++;
        }
        if (expected != reference_actual)
        {
            rgb_reference_mismatches++;
        }
    }
    for (i = 0u; i < RENDERER_PARITY_RGB_BINDING_COUNT; i++)
    {
        sink = rendererParityHash(sink, binding_mismatches[i]);
        if (binding_mismatches[i] !=
            (uint32_t)gRendererParityBindingMismatchCounts[i])
        {
            rgb_mismatches = 0xffffffffu;
        }
    }

    for (i = 0u; i < RENDERER_PARITY_T16_CASE_COUNT; i++)
    {
        const RendererParityT16Case *test = &gRendererParityT16Cases[i];
        const RendererParityT16Transform *transform =
            &gRendererParityT16Transforms[test->transform_index];
        int32_t expected_s = rendererParitySourceT16(
            test->s, transform->scale_s, transform->origin_s,
            transform->offset);
        int32_t expected_t = rendererParitySourceT16(
            test->t, transform->scale_t, transform->origin_t,
            transform->offset);
        int32_t actual_s = rendererParityDeviceT16(
            test->s, transform->matrix_s, transform->bias_s);
        int32_t actual_t = rendererParityDeviceT16(
            test->t, transform->matrix_t, transform->bias_t);
        int32_t translation_s =
            -((int32_t)transform->origin_s * 4) + transform->offset;
        int32_t translation_t =
            -((int32_t)transform->origin_t * 4) + transform->offset;
        int32_t naive_s = rendererParityDeviceT16(
            test->s, (int32_t)transform->scale_s >> 5,
            translation_s * 4096);
        int32_t naive_t = rendererParityDeviceT16(
            test->t, (int32_t)transform->scale_t >> 5,
            translation_t * 4096);

        sink = rendererParityHash(sink, (uint32_t)expected_s);
        sink = rendererParityHash(sink, (uint32_t)expected_t);
        sink = rendererParityHash(sink, (uint32_t)actual_s);
        sink = rendererParityHash(sink, (uint32_t)actual_t);
        if ((expected_s != actual_s) || (expected_t != actual_t) ||
            (transform->exact_synthesis == 0u))
        {
            t16_mismatches++;
        }
        if ((expected_s != naive_s) || (expected_t != naive_t))
        {
            t16_naive_mismatches++;
        }
    }

    out->rgb_case_count = RENDERER_PARITY_RGB_CASE_COUNT;
    out->rgb_mismatch_count = rgb_mismatches;
    out->rgb_reference_mismatch_count = rgb_reference_mismatches;
    out->hardware_light_demoted = (rgb_mismatches != 0u);
    out->t16_case_count = RENDERER_PARITY_T16_CASE_COUNT;
    out->t16_mismatch_count = t16_mismatches;
    out->t16_naive_mismatch_count = t16_naive_mismatches;
    out->texture_matrix_demoted = (t16_mismatches != 0u);
    out->sink = sink;
}


#ifdef RENDERER_PARITY_HOST_MAIN
static uint32_t rendererParityResultMatchesContract(
    const RendererParityResult *result)
{
    return
        (result->rgb_case_count == RENDERER_PARITY_RGB_CASE_COUNT) &&
        (result->rgb_mismatch_count ==
         RENDERER_PARITY_RGB_EXPECTED_MISMATCH_COUNT) &&
        (result->rgb_reference_mismatch_count ==
         RENDERER_PARITY_RGB_REFERENCE_MISMATCH_COUNT) &&
        (result->hardware_light_demoted ==
         RENDERER_PARITY_RGB_DEMOTE_HARDWARE_LIGHT) &&
        (result->t16_case_count == RENDERER_PARITY_T16_CASE_COUNT) &&
        (result->t16_mismatch_count ==
         RENDERER_PARITY_T16_EXPECTED_MISMATCH_COUNT) &&
        (result->t16_naive_mismatch_count ==
         RENDERER_PARITY_T16_NAIVE_MISMATCH_COUNT) &&
        (result->texture_matrix_demoted ==
         RENDERER_PARITY_T16_DEMOTE_TEXTURE_MATRIX);
}


#include <stdio.h>

int main(void)
{
    RendererParityResult result;

    smash64dsRendererParityMicrobench(&result);
    printf(
        "renderer parity RGB15: %lu cases, %lu mismatches, hardware-light=%s\n",
        (unsigned long)result.rgb_case_count,
        (unsigned long)result.rgb_mismatch_count,
        result.hardware_light_demoted ? "DEMOTED" : "ELIGIBLE");
    printf(
        "renderer parity t16: %lu cases, %lu mismatches, naive=%lu, "
        "texture-matrix=%s\n",
        (unsigned long)result.t16_case_count,
        (unsigned long)result.t16_mismatch_count,
        (unsigned long)result.t16_naive_mismatch_count,
        result.texture_matrix_demoted ? "DEMOTED" : "SYNTHESIZED-ELIGIBLE");
    printf(
        "renderer parity contract: %s sink=%08lx corpus=%s\n",
        rendererParityResultMatchesContract(&result) ? "PASS" : "FAIL",
        (unsigned long)result.sink,
        RENDERER_PARITY_CORPUS_SHA256);
    return rendererParityResultMatchesContract(&result) ? 0 : 1;
}
#endif
