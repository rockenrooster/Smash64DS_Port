// Implemented from the plan in docs/crt_shader_plan_2026-05-11.md §3.2.
// No code copied from RetroArch or any GPL-licensed shader runtime.
#pragma once

#include <string>

#include "fast/postprocess/PostProcessTypes.h"

namespace Fast {

// Synthesizes the per-backend shader-language slots inside a
// PostProcessSource from its `glsl` field, when those slots are empty.
//
// Build configuration:
//   - LUS_ENABLE_POSTPROCESS_TRANSPILER=ON (default for desktop targets in
//     libultraship/cmake/dependencies/common.cmake) links glslang +
//     SPIRV-Cross and defines LUS_POSTPROCESS_TRANSPILER for this TU.
//   - When the macro is undefined, SynthesizeMissing() is a no-op that
//     returns false; callers should treat this as "the bundled triplet
//     must already include hand-written hlsl/msl siblings", which is the
//     pre-transpiler workflow.
//
// Input contract on `inout.glsl`:
//   - GLSL 330 core fragment shader using the schema documented in
//     `libultraship/src/fast/shaders/postprocess/scanlines.glsl`.
//     Specifically: declares `in vec2 vTexCoord`, `out vec4 fragColor`,
//     and loose uniforms `Source` / `SourceSize` / `OutputSize` /
//     `InputSize` / `FrameCount` and optionally `FrameDirection`.
//
// Output:
//   - On success the empty slots are filled with a complete shader
//     "bundle" containing the matching backend's stock fullscreen-
//     triangle vertex stub plus a fragment body derived from the user's
//     GLSL. Entry-point names and resource bindings match the
//     conventions hard-coded in each backend (see
//     gfx_direct3d11.cpp / gfx_metal.cpp).
//   - Existing non-empty slots are left untouched, so a shader that
//     ships a hand-written .hlsl / .msl sibling keeps that version as
//     the authoritative source.
//
// Failure mode: returns false and writes a one-line reason into
// `errOut`. `inout` is left in a consistent state — slots populated
// before the call remain populated; slots the function failed to
// synthesize remain empty.
class PostProcessTranspiler {
  public:
    static bool SynthesizeMissing(PostProcessSource& inout, std::string& errOut);
};

} // namespace Fast
