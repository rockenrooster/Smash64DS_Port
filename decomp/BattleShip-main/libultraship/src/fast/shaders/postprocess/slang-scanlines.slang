// Implemented from the public libretro slang shader format spec at
// https://github.com/libretro/slang-shaders/blob/master/README.md. No
// code copied from RetroArch or any GPL-licensed shader runtime.
//
// A synthetic single-pass scanline shader that exists solely as the
// LUS slang pipeline's "does the runtime actually work end-to-end?"
// canary. Produces a horizontal scanline band pattern visually
// distinguishable from the legacy `scanlines.glsl` shader by tinting
// the dark bands slightly blue — if the picker shows scanlines
// without the blue cast, the legacy pipeline loaded; if the bands
// are blue-tinted, the slang pipeline loaded.

#version 450

layout(set = 0, binding = 0, std140) uniform UBO {
    mat4 MVP;
    vec4 OutputSize;
    vec4 SourceSize;
    vec4 OriginalSize;
    uint FrameCount;
} global;

layout(set = 0, binding = 1) uniform sampler2D Source;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
void main() {
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
void main() {
    vec3 c = texture(Source, vTexCoord).rgb;
    // Per-row scanline period derived from the source texture's
    // declared height (one band per source pixel row).
    float row = vTexCoord.y * global.SourceSize.y;
    float scan = 0.5 + 0.5 * cos(row * 6.283185);
    float lightness = mix(0.55, 1.0, scan);
    // Dark bands pick up a small cool-blue cast so the slang vs
    // legacy path is visually distinguishable at a glance.
    vec3 tint = mix(vec3(0.85, 0.92, 1.10), vec3(1.0), scan);
    FragColor = vec4(c * lightness * tint, 1.0);
}
