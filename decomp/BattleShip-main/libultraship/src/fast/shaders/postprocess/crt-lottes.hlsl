// SPDX-License-Identifier: CC0-1.0
//
// Lottes-style CRT shader (HLSL companion to crt-lottes.glsl), dedicated
// to the public domain by the author under CC0 1.0 Universal
// (https://creativecommons.org/publicdomain/zero/1.0/). To the extent
// possible under law, all copyright and related or neighboring rights
// to this work have been waived; it may be used, modified, sublicensed
// and redistributed without restriction or attribution.
//
// The CRT emulation technique implemented here was pioneered by Timothy
// Lottes ("CRT Simulation in a GPU Shader," GDC 2014); his reference
// implementation is public domain. This file is an original
// implementation, not transcribed from Lottes's source.
//
// Hand-tuned HLSL companion that takes priority over the transpiled
// output PostProcessTranspiler would otherwise synthesize from
// crt-lottes.glsl. See the .glsl file for the full technique reference
// and authorship note.

Texture2D Source : register(t0);
SamplerState SourceSampler : register(s0);

cbuffer PostProcessUniforms : register(b0) {
    float2 SourceSize;
    float2 OutputSize;
    float2 InputSize;
    int FrameCount;
    float FrameDirection;
};

struct VOut {
    float4 position : SV_Position;
    float2 vTexCoord : TEXCOORD0;
};

static const float kMaskStrength = 0.22;
static const float kScanlineK = 5.0;
static const float kBrightnessGain = 1.25;

float3 SrgbToLinear(float3 c) { return pow(c, float3(2.2, 2.2, 2.2)); }
float3 LinearToSrgb(float3 c) { return pow(c, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2)); }

float3 FetchScanline(float2 uv, float row) {
    float2 rowUv = float2(uv.x, (row + 0.5) / SourceSize.y);
    float2 dx = float2(1.0 / SourceSize.x, 0.0);
    float3 a = SrgbToLinear(Source.Sample(SourceSampler, rowUv - dx).rgb);
    float3 b = SrgbToLinear(Source.Sample(SourceSampler, rowUv).rgb);
    float3 c = SrgbToLinear(Source.Sample(SourceSampler, rowUv + dx).rgb);
    return a * 0.25 + b * 0.5 + c * 0.25;
}

float3 PhosphorMask(float fragX) {
    float angle = fragX * (6.283185 / 3.0);
    float3 phase = float3(0.0, 2.094395, 4.188790);
    float3 m = 0.5 + 0.5 * cos(angle - phase);
    return lerp(float3(1.0 - kMaskStrength, 1.0 - kMaskStrength, 1.0 - kMaskStrength),
                float3(1.0 + kMaskStrength, 1.0 + kMaskStrength, 1.0 + kMaskStrength), m);
}

VOut VSMain(uint vid : SV_VertexID) {
    static const float2 positions[3] = {
        float2(-1.0, -1.0),
        float2( 3.0, -1.0),
        float2(-1.0,  3.0),
    };
    static const float2 texCoords[3] = {
        float2(0.0, 1.0),
        float2(2.0, 1.0),
        float2(0.0, -1.0),
    };
    VOut o;
    o.position = float4(positions[vid], 0.0, 1.0);
    o.vTexCoord = texCoords[vid];
    return o;
}

float4 PSMain(VOut input) : SV_Target {
    float2 uv = input.vTexCoord;

    float subY = uv.y * SourceSize.y;
    float row0 = floor(subY);
    float dA = subY - row0;
    float dB = subY - (row0 + 1.0);

    // Bandlimit the scanline Gaussian by the output pixel's source-row
    // footprint. See crt-lottes.glsl for the variance-addition derivation.
    float deltaY = abs(ddx(subY)) + abs(ddy(subY));
    float kEff = kScanlineK / (1.0 + (deltaY * deltaY) * kScanlineK / 6.0);

    float wA = exp(-(dA * dA) * kEff);
    float wB = exp(-(dB * dB) * kEff);

    float3 color = FetchScanline(uv, row0) * wA
                 + FetchScanline(uv, row0 + 1.0) * wB;
    color *= kBrightnessGain;
    color *= PhosphorMask(input.position.x);

    return float4(LinearToSrgb(color), 1.0);
}
