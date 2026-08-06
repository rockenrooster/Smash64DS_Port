// SPDX-License-Identifier: CC0-1.0
//
// Default scanline shader (HLSL companion to scanlines.glsl), dedicated
// to the public domain by the author under CC0 1.0 Universal
// (https://creativecommons.org/publicdomain/zero/1.0/). To the extent
// possible under law, all copyright and related or neighboring rights
// to this work have been waived; it may be used, modified, sublicensed
// and redistributed without restriction or attribution.
//
// Hand-tuned HLSL companion that takes priority over the transpiled
// output PostProcessTranspiler would otherwise synthesize from
// scanlines.glsl. Kept in tree so the bundled built-in matches what
// shipped in earlier Phase 1 builds byte-for-byte.
//
// Vertex entry: VSMain (uses SV_VertexID only — no input layout)
// Pixel entry:  PSMain
// Bindings the LUS D3D11 backend supplies:
//   - t0 : Source           (input game framebuffer SRV)
//   - s0 : SourceSampler    (linear, clamp-to-edge)
//   - b0 : PostProcessUniforms (matches gfx_direct3d_common.h struct)

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

VOut VSMain(uint vid : SV_VertexID) {
    // Fullscreen triangle in clip space (HLSL Y grows downward in render
    // target space, but clip-space Y grows upward — same as GL/Metal).
    // UVs picked so clip-space Y=-1 reads texture V=1 (D3D V grows top→
    // bottom; clip Y grows bottom→top), preserving the rendered source
    // orientation through the pass.
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
    float3 color = Source.Sample(SourceSampler, uv).rgb;

    const float scanMin = 0.75;
    float row = uv.y * SourceSize.y;
    float scan = lerp(scanMin, 1.0, 0.5 + 0.5 * cos(row * 6.283185));
    color *= scan;

    const float maskStrength = 0.10;
    float colMod = fmod(input.position.x, 3.0);
    float3 mask;
    if (colMod < 1.0) {
        mask = float3(1.0 + maskStrength, 1.0 - maskStrength, 1.0 - maskStrength);
    } else if (colMod < 2.0) {
        mask = float3(1.0 - maskStrength, 1.0 + maskStrength, 1.0 - maskStrength);
    } else {
        mask = float3(1.0 - maskStrength, 1.0 - maskStrength, 1.0 + maskStrength);
    }
    color *= mask;

    return float4(color, 1.0);
}
