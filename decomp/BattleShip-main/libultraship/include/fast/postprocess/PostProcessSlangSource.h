// Implemented from the public libretro slang shader format docs at
// https://github.com/libretro/slang-shaders/blob/master/README.md and
// the format-overview docs in that repo. No code copied from RetroArch
// or any GPL-licensed shader runtime.
#pragma once

#include <string>
#include <vector>

namespace Fast {

// One `#pragma parameter <name> "Label" default min max [step]`
// declaration extracted from a .slang shader. Drives both the UBO
// upload at runtime and the menu sliders the port exposes.
struct PostProcessSlangParameter {
    std::string name;          // Identifier the shader code references.
    std::string label;         // Human-readable name for UI display.
    float       defaultValue = 0.0f;
    float       minValue     = 0.0f;
    float       maxValue     = 0.0f;
    // libretro spec: the step is optional; when omitted RetroArch
    // synthesizes one based on the range. We store 0 to flag "use
    // the caller's default" so a downstream UI can pick its own
    // heuristic (commonly (max-min)/100) without losing the
    // declaration's intent.
    float       step         = 0.0f;
};

// Result of splitting a .slang file's source into its vertex /
// fragment stages and lifting out the in-source metadata.
//
// Stage routing (libretro slang spec):
//   - Lines before any `#pragma stage` are common preamble — both the
//     vertex and the fragment stage receive them, in order.
//   - After `#pragma stage vertex`, lines flow into the vertex stage
//     only, until the next `#pragma stage fragment` (or EOF).
//   - After `#pragma stage fragment`, lines flow into the fragment
//     stage only.
//   - The slang-specific `#pragma stage` / `#pragma name` / `#pragma
//     format` / `#pragma parameter` lines are all stripped from the
//     output stage buffers — none of them are valid GLSL and glslang
//     would reject them. Their semantic content is captured in the
//     struct fields below.
//
// `vertex` and `fragment` are GLSL 4.50 (Vulkan-flavor) source ready
// to hand to glslang once `#version` / `#extension` headers are
// prepended by the compile path.
struct PostProcessSlangSource {
    std::string name;     // From `#pragma name`; empty if absent.
    std::string format;   // From `#pragma format`; empty if absent.
    std::vector<PostProcessSlangParameter> parameters;
    std::string vertex;
    std::string fragment;
};

// Parse `.slang` source text. Returns true on success; on failure
// `errOut` carries a one-line reason and `out` is left in an
// unspecified state.
//
// Success requires at least one `#pragma stage vertex` and one
// `#pragma stage fragment` to be present. Slang shaders without
// explicit stage markers don't compile in libretro's pipeline either;
// refusing them here surfaces the issue at parse time rather than
// inside glslang.
bool ParseSlangSource(const std::string& src, PostProcessSlangSource& out,
                      std::string& errOut);

} // namespace Fast
