// Implemented against the public glslang and SPIRV-Cross C++ APIs and
// against the libretro slang shader format docs. No code copied from
// RetroArch or any GPL-licensed shader runtime.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "fast/postprocess/PostProcessSlangSource.h"

namespace Fast {

// One member of the slang UBO (or push-constant block, which we
// collapse into the same logical "uniforms" view). Captured by SPIR-V
// reflection so the runtime knows where each semantic / parameter
// value goes when uploading the per-frame UBO blob.
//
// `offsetBytes` is the std140-derived offset within the UBO. `sizeBytes`
// is the type's logical size (mat4=64, vec4=16, vec2=8, float/int=4,
// uint=4) — the runtime uses it for the upload memcpy. Component
// arithmetic (vec2 vs vec4 for size uniforms) lives one layer up.
struct PostProcessSlangUboMember {
    std::string name;
    uint32_t    offsetBytes = 0;
    uint32_t    sizeBytes   = 0;
};

// One sampled-image binding extracted from the slang fragment stage.
// Slang convention pins descriptor set 0; `binding` is the slot
// number the shader declares (Source typically lands at binding=2 in
// slang's libretro layout — set=0 binding=0 is reserved for the UBO).
struct PostProcessSlangSamplerBinding {
    std::string name;
    uint32_t    descriptorSet = 0;
    uint32_t    binding       = 0;
};

// One stage's compiled output, ready to hand to a Fast3D backend.
// Empty members mean "not synthesized for this stage" — when the
// transpiler is disabled at build time or the target backend isn't
// applicable, the slot is empty without erroring.
struct PostProcessSlangCompiledStage {
    std::string glsl;
    std::string hlsl;
    std::string msl;
};

// Complete compile artifact: per-stage backend source + reflection.
// The UBO members are ordered by std140 offset (= declaration order
// in well-formed shaders); samplers are in declaration order.
struct PostProcessSlangArtifact {
    PostProcessSlangCompiledStage vertex;
    PostProcessSlangCompiledStage fragment;
    std::vector<PostProcessSlangUboMember>      uboMembers;
    std::vector<PostProcessSlangSamplerBinding> samplers;
    // Total byte size the runtime should allocate for the UBO upload
    // buffer. Includes std140 trailing padding so the buffer is large
    // enough for the largest declared offset + that member's size.
    uint32_t                                    uboTotalBytes = 0;
};

// Compile a parsed `.slang` shader into a backend-ready artifact.
// Implementation gated by the same LUS_ENABLE_POSTPROCESS_TRANSPILER
// CMake option that gates Phase 1's transpiler — when off, Compile()
// returns false with an explanatory errOut and the artifact is left
// in its default-constructed state.
//
// Phase 3C limitations (documented; relaxed in later phases):
//   - Single pass only. Multi-pass requires the chain to know about
//     PassOutputN bindings (Phase 3D scope).
//   - One UBO descriptor (slang's standard `UBO` or `Push` block).
//     Shaders declaring both UBO and push_constant simultaneously
//     are rejected for now.
//   - No `#include` resolution; the slang source must be self-
//     contained.
//   - No OriginalHistoryN / PassFeedbackN binding capture (Phase 3E).
class PostProcessSlangTranspiler {
  public:
    static bool Compile(const PostProcessSlangSource& src,
                        PostProcessSlangArtifact& out, std::string& errOut);
};

} // namespace Fast
