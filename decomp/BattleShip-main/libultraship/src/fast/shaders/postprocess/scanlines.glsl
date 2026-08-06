#version 330 core

// SPDX-License-Identifier: CC0-1.0
//
// Default scanline shader for the LUS post-process runtime, dedicated
// to the public domain by the author under CC0 1.0 Universal
// (https://creativecommons.org/publicdomain/zero/1.0/). To the extent
// possible under law, all copyright and related or neighboring rights
// to this work have been waived; it may be used, modified, sublicensed
// and redistributed without restriction or attribution. Original work
// for this repository; not derived from any third-party CRT shader
// implementation.
//
// Demonstrates the standard runtime uniform schema (see
// PostProcessTypes.h):
//
//   sampler2D Source     -- input game framebuffer
//   vec2      SourceSize -- input texture pixel dimensions
//   vec2      OutputSize -- output framebuffer pixel dimensions
//   vec2      InputSize  -- reserved; same as SourceSize in Phase 1
//   int       FrameCount -- monotonic frame counter
//
// User shaders dropped into ./shaders/<name>.glsl receive the same set.

in vec2 vTexCoord;
out vec4 fragColor;

uniform sampler2D Source;
uniform vec2 SourceSize;
uniform vec2 OutputSize;
uniform vec2 InputSize;
uniform int FrameCount;

void main() {
    vec3 color = texture(Source, vTexCoord).rgb;

    // Per-source-row scanline darkening. SourceSize.y * vTexCoord.y gives
    // a continuous "row" coordinate; a unit-period cosine creates a smooth
    // oscillation between adjacent rows. The output is remapped from
    // [-1,1] to [scanMin,1].
    const float scanMin = 0.75;
    float row = vTexCoord.y * SourceSize.y;
    float scan = mix(scanMin, 1.0, 0.5 + 0.5 * cos(row * 6.283185));
    color *= scan;

    // Subpixel mask: cheap triad pattern based on output column position.
    // Keeps overall brightness near unity by raising one channel and
    // dropping the other two by the same magnitude.
    const float maskStrength = 0.10;
    float colMod = mod(gl_FragCoord.x, 3.0);
    vec3 mask;
    if (colMod < 1.0) {
        mask = vec3(1.0 + maskStrength, 1.0 - maskStrength, 1.0 - maskStrength);
    } else if (colMod < 2.0) {
        mask = vec3(1.0 - maskStrength, 1.0 + maskStrength, 1.0 - maskStrength);
    } else {
        mask = vec3(1.0 - maskStrength, 1.0 - maskStrength, 1.0 + maskStrength);
    }
    color *= mask;

    fragColor = vec4(color, 1.0);
}
