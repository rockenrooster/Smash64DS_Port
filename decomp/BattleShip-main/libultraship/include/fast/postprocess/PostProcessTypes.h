// Implemented from the libretro/Mednafen public shader-uniform conventions
// (Source/TextureSize/OutputSize/FrameCount) and the plan in
// docs/crt_shader_plan_2026-05-11.md §3.2. No code copied from RetroArch
// or any GPL-licensed shader runtime.
#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#include "PostProcessPreset.h"

namespace Fast {

// Per-pass output framebuffer pixel format. libretro `.glslp` exposes
// two booleans (`srgb_framebufferN`, `float_framebufferN`); we collapse
// them into a single enum since they're mutually exclusive in practice
// (libretro spec: setting both is undefined; real shaders only set one).
//
// Default = the backend's regular 8-bit color format (RGBA8 / RGB8 /
//   BGRA8Unorm depending on backend). Used for the chain's final pass
//   (mDstFb) regardless of what the .glslp says, because the GUI
//   consumes mDstFb through ImGui::Image which expects a sampleable
//   LDR texture.
// Srgb = sRGB-encoded 8-bit. Hardware auto-converts sRGB->linear on
//   sample and linear->sRGB on write (when the destination encoding is
//   live — GL requires GL_FRAMEBUFFER_SRGB enabled, D3D11/Metal handle
//   it via the RTV/colorAttachment format).
// Float16 = half-float per channel. No encoding; HDR linear math.
enum class PostProcessFboFormat {
    Default,
    Srgb,
    Float16,
};

// Per-frame inputs the runtime supplies to each post-process pass.
//
// Backends are responsible for converting these into whatever uniform
// storage the underlying API wants (a uniform block, push constants,
// loose glUniform calls, etc.). The set is intentionally minimal in
// Phase 1 and matches the "common five" uniforms found in essentially
// every single-file CRT/scanline shader.
//
// Resolution-uniform conventions match libretro / Mednafen:
//   - source = the texture being sampled (FBO at internal render res)
//   - input  = the original video signal the source was generated from
//              (for N64 emulation: 320x240, the screen res the GBI
//              commands target)
//   - dst    = the framebuffer the pass writes into (window)
// CRT shaders that emit per-source-row scanlines rely on the
// `source / input` ratio to convert from texture coordinates into
// game-pixel coordinates; passing identical values for both collapses
// the scanline period to one output pixel and aliases against the LCD
// subpixel grid as a moire pattern.
struct PostProcessParams {
    uint32_t srcWidth;          // Source texture pixel dimensions (the
    uint32_t srcHeight;         //   current pass's input sampler 0).
    uint32_t inputWidth;        // Original video-signal dimensions —
    uint32_t inputHeight;       //   N64 native 320x240 for SSB64.
    uint32_t originalWidth;     // Game-FB pixel dimensions (the
    uint32_t originalHeight;    //   sampler 1 / "Original" texture). Same
                                //   as src{Width,Height} on pass 0; for
                                //   later passes Source is the prior pass
                                //   output, Original stays the game FB.
    uint32_t dstWidth;          // Output framebuffer dimensions (the
    uint32_t dstHeight;         //   surface the pass renders into).
    uint32_t frameCount;        // Monotonic frame counter, wraps.
    float    frameDeltaSeconds; // Wall-clock delta since previous frame.

    // Sampler state for the `Source` binding (slot 0). The chain
    // populates these from the producer pass's libretro filter_linear /
    // wrap_mode keys — i.e. pass i's Source sampler is set from
    // pass[i-1].config. Pass 0 reads from the game FB, which has no
    // producer; the chain leaves the defaults (linear, clamp-to-edge)
    // and that input is rendered into by the regular game pipeline at
    // its own filtering settings anyway.
    //
    // The `Original` binding (slot 1) is intentionally pinned to the
    // backend default (linear, clamp-to-edge) — libretro's `.glslp` has
    // no per-pass control over the original FB sampler.
    bool                srcFilterLinear = true;
    PostProcessWrapMode srcWrapMode     = PostProcessWrapMode::ClampToEdge;

    // libretro `mipmap_inputN`: the source sampler for this pass should
    // pick from a mip chain on the input texture. The chain populates
    // the mip chain (via GfxRenderingAPI::GeneratePostProcessMipmaps)
    // before calling RunPostProcess; this flag tells the backend to
    // pair the producer texture with a mip-aware min filter
    // (LINEAR_MIPMAP_LINEAR) rather than plain LINEAR. Sampling a
    // texture without a populated mip chain with a mipmap filter is
    // legal but falls back to the base level — the chain only sets
    // this when it has actually generated mipmaps.
    bool                srcUseMipmap   = false;

    // Optional named bindings for libretro `aliasN` and (later phases)
    // external `textures = "..."`. Backends bind these at sampler /
    // texture slots starting at 2 (slots 0/1 are Source/Original); the
    // i-th entry binds to slot 2 + i. Order matches the slot order the
    // transpiler reserved in the HLSL/MSL emit, which in turn comes
    // from PostProcessSource::aliasNames at compile time.
    const struct PostProcessExtraBinding* extraBindings = nullptr;
    size_t extraBindingsCount = 0;
};

// One named-sampler binding flowed through PostProcessParams. Backends
// look up a texture for the binding using one of two mutually-exclusive
// sources:
//
//   sourceFb >= 0          — sample the color texture of FBO `sourceFb`
//                            (used by `.glslp` aliasN bindings whose
//                            producer pass already ran in this frame)
//   staticTextureId >= 0   — sample a backend-managed static 2D texture
//                            (used by `.glslp` external `textures =
//                            "..."` declarations; loaded once at
//                            LoadPasses time and reused every frame)
//
// If both are -1 the binding is "not yet available" (alias producer
// hasn't run at this pass index). Backends should bind a defensive
// fallback (1x1 black, or the Original FB) so the shader's
// `texture(<name>, ...)` reads something sane rather than the slot's
// stale contents from a prior pass.
struct PostProcessExtraBinding {
    std::string         name;            // Diagnostic / matches transpiler slot
    int                 sourceFb = -1;   // FBO id whose color texture we sample
    int                 staticTextureId = -1; // CreatePostProcessStaticTexture handle
    uint32_t            width    = 1;
    uint32_t            height   = 1;
    bool                filterLinear = true;
    PostProcessWrapMode wrapMode = PostProcessWrapMode::ClampToEdge;
};

// Per-backend source text for a single fragment-shader pass.
//
// PostProcessSourceLoader populates `glsl` from a `<name>.glsl` on disk
// (or in f3d.o2r), then either copies hand-tuned `<name>.hlsl` /
// `<name>.msl` siblings into the matching slots or hands off to
// PostProcessTranspiler::SynthesizeMissing to fill them from `glsl`.
// Backends consume the slot matching their target API and ignore the
// others.
struct PostProcessSource {
    std::string name; // Diagnostic label (typically the shader filename).
    std::string glsl; // GLSL 330 core fragment-shader source.
    std::string hlsl; // HLSL SM 5.0 fragment-shader source.
    std::string msl;  // MSL 2.2 fragment-shader source.

    // Ordered list of named-sampler bindings the transpiler reserved
    // slots for, starting at slot 2 (Source=0, Original=1). The
    // normalizer pre-declares these as `uniform sampler2D <name>` and
    // `uniform vec2 <name>Size` in the GLSL preamble; HLSL/MSL emit
    // map them to register t2+/s2+ and texture(2+)/sampler(2+).
    //
    // Populated by the loader from the preset's alias list (and, in
    // later phases, external `textures = "..."` declarations). Empty
    // for single-pass shaders or .glslp presets without any aliases.
    std::vector<std::string> aliasNames;
};

// Per-backend stage source + binding plan for a single `.slang` pass,
// flattened from a PostProcessSlangArtifact for handoff to the
// rendering API. The chain converts (artifact + diagnostic name) into
// this struct via PostProcessChain's internal helper and hands it to
// the backend's CreatePostProcessSlangProgram.
//
// Distinct from PostProcessSource because the slang path has its own
// vertex stage (authored by the slang shader, not the LUS stock stub)
// and uses a UBO-keyed uniform model rather than loose uniforms with
// the fixed LUS schema. Backends keep both surfaces side-by-side
// until the legacy .glsl/.glslp surface retires (out of Phase 3 scope).
//
// Empty per-language slots mean "this backend isn't applicable" — the
// GL backend reads vsGlsl/fsGlsl and ignores HLSL/MSL, etc.
struct PostProcessSlangProgramSource {
    std::string name;   // Diagnostic label.
    std::string vsGlsl; // GLSL 330 core vertex source.
    std::string fsGlsl; // GLSL 330 core fragment source.
    std::string vsHlsl; // HLSL SM 5.0 vertex source.
    std::string fsHlsl; // HLSL SM 5.0 fragment source.
    std::string vsMsl;  // MSL 2.2 vertex source.
    std::string fsMsl;  // MSL 2.2 fragment source.

    // Total UBO bytes the runtime will upload per pass. Backends use
    // this to allocate the per-program constant buffer / glBuffer at
    // create time so the run-path memcpy has a fixed target.
    uint32_t uboBytes = 0;

    // Sampler binding names in declaration order (matches the order
    // the transpiler emitted in the per-backend source). Slot i in
    // the bound texture array maps to samplerNames[i]; the chain
    // resolves each name to a texture id at run time. For Phase 3D-1
    // backends only need the names to call glGetUniformLocation /
    // D3DReflect / MTL argument lookup; nothing is bound yet.
    std::vector<std::string> samplerNames;
};

} // namespace Fast
