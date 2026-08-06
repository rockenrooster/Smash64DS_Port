// Implemented from the public libretro slang shader format spec at
// https://github.com/libretro/slang-shaders/blob/master/README.md.
// No code copied from RetroArch or any GPL-licensed shader runtime.
//
// Phase 3E canary: exercises the `OriginalHistory<N>` semantic so
// the chain's game-FB history ring + per-frame capture get a smoke
// test from the in-game picker. Blends the current game FB with the
// previous frame to produce a mild persistence/ghost-trail effect —
// visually distinct from `slang-scanlines` (uniform scanline darkening
// vs. motion-trailing ghost). If the picker selects `slang-persistence`
// and the output is a sharp game image with no trailing, the history
// ring didn't capture — the previous frame is reading as zeros.

#version 450

layout(set = 0, binding = 0, std140) uniform UBO {
    mat4 MVP;
    vec4 OutputSize;
    vec4 SourceSize;
    vec4 OriginalSize;
    uint FrameCount;
} global;

layout(set = 0, binding = 1) uniform sampler2D Source;
// OriginalHistory1 = the game framebuffer as it was one frame ago.
// The chain's per-frame capture writes the live game FB into the
// history ring before any pass runs, so this sampler always points
// at last frame's contents (zeros on the very first frame).
layout(set = 0, binding = 2) uniform sampler2D OriginalHistory1;

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
    vec3 cur  = texture(Source, vTexCoord).rgb;
    vec3 prev = texture(OriginalHistory1, vTexCoord).rgb;
    // 70/30 blend: motion is dampened, not erased. Stronger weight
    // toward `prev` would smear text; lighter would erase the trail.
    FragColor = vec4(mix(cur, prev, 0.3), 1.0);
}
