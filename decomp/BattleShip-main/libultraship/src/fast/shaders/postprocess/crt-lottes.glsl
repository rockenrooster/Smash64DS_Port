#version 330 core

// SPDX-License-Identifier: CC0-1.0
//
// Lottes-style CRT shader for the LUS post-process runtime, dedicated
// to the public domain by the author under CC0 1.0 Universal
// (https://creativecommons.org/publicdomain/zero/1.0/). To the extent
// possible under law, all copyright and related or neighboring rights
// to this work have been waived; it may be used, modified, sublicensed
// and redistributed without restriction or attribution.
//
// Acknowledgment (not a legal attribution requirement — CC0 disclaims
// attribution rights — but useful as a heritage reference): the CRT
// emulation technique this shader implements was pioneered by Timothy
// Lottes ("CRT Simulation in a GPU Shader," GDC 2014). His own
// reference implementation is in the public domain. The code below is
// an original implementation written from scratch from the published
// technique description; it is not transcribed from Lottes's source or
// from any of the libretro / RetroArch GPL ports that adapted his work.
//
// Original work for this repository, implementing the classic Lottes CRT
// technique (per-source-row Gaussian scanline + bandlimited cosine
// subpixel mask + gamma in/out); no code copied from RetroArch /
// libretro/glsl-shaders or any GPL source.
//
// No barrel warp by default. The warp's spatially-varying magnification
// makes the scanline period in output pixels drift across the screen,
// which beats against the output pixel grid and produces the curved
// moire fringes the prior revision suffered from. Anyone who wants
// curvature on a CRT preset will need a multipass setup that integrates
// the warp's Jacobian into the scanline kernel — out of scope for the
// single-pass Phase 1 runtime.
//
// Uniform schema matches PostProcessTypes.h.

in vec2 vTexCoord;
out vec4 fragColor;

uniform sampler2D Source;
uniform vec2 SourceSize;
uniform vec2 OutputSize;
uniform vec2 InputSize;
uniform int FrameCount;

// Phosphor mask strength: 0.0 disables, 1.0 = full subpixel separation.
// Smooth cosine across three output columns. Phase-offset for R/G/B so
// the average across the triad stays at 1.0.
const float kMaskStrength = 0.22;

// Scanline base tightness. Wider/narrower gap is determined by the
// fragment's source-row footprint at runtime (bandlimit below); this
// value is the "fully-resolved" gaussian sharpness used when the
// fragment covers a small fraction of a source row.
const float kScanlineK = 5.0;

// Average-brightness gain to compensate for the scanline energy loss.
const float kBrightnessGain = 1.25;

vec3 SrgbToLinear(vec3 c) { return pow(c, vec3(2.2)); }
vec3 LinearToSrgb(vec3 c) { return pow(c, vec3(1.0 / 2.2)); }

// Three-tap horizontal filter.
vec3 FetchScanline(vec2 uv, float row) {
    vec2 rowUv = vec2(uv.x, (row + 0.5) / SourceSize.y);
    vec2 dx = vec2(1.0 / SourceSize.x, 0.0);
    vec3 a = SrgbToLinear(texture(Source, rowUv - dx).rgb);
    vec3 b = SrgbToLinear(texture(Source, rowUv).rgb);
    vec3 c = SrgbToLinear(texture(Source, rowUv + dx).rgb);
    return (a * 0.25) + (b * 0.5) + (c * 0.25);
}

vec3 PhosphorMask(float fragX) {
    float angle = fragX * (6.283185 / 3.0);
    vec3 phase = vec3(0.0, 2.094395, 4.188790);
    vec3 m = 0.5 + 0.5 * cos(angle - phase);
    return mix(vec3(1.0 - kMaskStrength), vec3(1.0 + kMaskStrength), m);
}

void main() {
    vec2 uv = vTexCoord;

    float subY = uv.y * SourceSize.y;
    float row0 = floor(subY);
    float dA = subY - row0;
    float dB = subY - (row0 + 1.0);

    // Bandlimit the scanline Gaussian by the local output-pixel footprint
    // in source rows. fwidth() gives the per-fragment rate of change of
    // subY in screen-space units; deltaY is the box-filter width we
    // convolve our Gaussian with. The closed-form variance of a
    // gaussian * box approximation is sigma^2 += delta^2 / 12, which
    // translates to an effective sharpness of
    //   kEff = k / (1 + delta^2 * k / 6)
    // — at fine resolution kEff -> k (sharp scanlines); at coarse
    // resolution kEff -> 6/delta^2 (the band saturates rather than
    // aliasing into beat fringes).
    float deltaY = fwidth(subY);
    float kEff = kScanlineK / (1.0 + (deltaY * deltaY) * kScanlineK / 6.0);

    float wA = exp(-(dA * dA) * kEff);
    float wB = exp(-(dB * dB) * kEff);

    vec3 color = FetchScanline(uv, row0) * wA
               + FetchScanline(uv, row0 + 1.0) * wB;
    color *= kBrightnessGain;
    color *= PhosphorMask(gl_FragCoord.x);

    fragColor = vec4(LinearToSrgb(color), 1.0);
}
