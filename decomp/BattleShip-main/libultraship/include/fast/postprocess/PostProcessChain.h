// Implemented from the plan in docs/crt_shader_plan_2026-05-11.md §3.3.
// No code copied from RetroArch or any GPL-licensed shader runtime.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "PostProcessPreset.h"
#include "PostProcessSlangTranspiler.h"
#include "PostProcessSourceLoader.h"
#include "PostProcessTypes.h"

namespace Fast {
class GfxRenderingAPI;

// Owns the off-screen framebuffer(s) and compiled programs for the
// post-process / user-shader pipeline. The interpreter holds one of
// these and forwards Init / OnResize / Run calls from its existing
// per-frame entry points.
//
// Phase 1 was single-pass: one program, one output FBO. Phase 2
// generalizes to N passes wired into a linear chain. Each pass owns
// its own compiled program plus (for intermediates) its own
// sampleable FBO. The final pass writes into the chain's mDstFb
// (sized at viewport dimensions) — that's the texture the GUI
// samples via ImGui::Image.
//
// Per-pass FBO sizing follows the libretro .glslp convention: each
// pass declares scale_type / scale per-axis, computed off either the
// previous pass output size (`source`), the viewport size
// (`viewport`), or an absolute pixel count (`absolute`).
class PostProcessChain {
  public:
    PostProcessChain() = default;
    ~PostProcessChain() = default;

    PostProcessChain(const PostProcessChain&) = delete;
    PostProcessChain& operator=(const PostProcessChain&) = delete;

    // Allocate the chain's final-output FBO. Safe against a backend
    // that returns SupportsPostProcess() == false; in that case the
    // chain stays disarmed and Run() is a no-op that returns srcFb.
    void Init(GfxRenderingAPI* rapi);

    // Resize the chain's output FBO to viewport dims. Idempotent.
    // Intermediate FBOs are sized per-frame in Run() because their
    // dimensions depend on the runtime input/viewport sizes.
    void OnResize(GfxRenderingAPI* rapi, uint32_t dstWidth, uint32_t dstHeight);

    // Single-pass convenience: equivalent to LoadPasses with one
    // entry and default config (scale_type=source, scale=1.0).
    // Preserves the call site the interpreter uses for plain `.glsl`.
    bool LoadShader(GfxRenderingAPI* rapi, const PostProcessSource& src);

    // Multi-pass entry. `sources[i]` is the loaded + normalized +
    // transpiled GLSL for pass i; `configs[i]` carries that pass's
    // scale/filter/etc. metadata. The two vectors must be the same
    // size and at least 1 entry. `externalTextures` is an optional
    // list of pre-decoded RGBA8 images (libretro `.glslp` `textures
    // = "..."` declarations) — uploaded once to the backend via
    // CreatePostProcessStaticTexture and bound per-pass at the
    // sampler slot the transpiler reserved for each name. Returns
    // true on success; on failure the chain is left in its prior
    // state (any partial backend resources allocated during the call
    // are rolled back).
    bool LoadPasses(GfxRenderingAPI* rapi,
                    const std::vector<PostProcessSource>& sources,
                    const std::vector<PostProcessPresetPass>& configs,
                    const std::vector<PostProcessShaderExternalTexture>& externalTextures = {});

    // Phase 3D-1: load one or more `.slang` shaders via the backend's
    // slang program path. Mirrors LoadPasses in lifecycle (compile up
    // front, roll back on failure, drop prior load on success) but
    // uses CreatePostProcessSlangProgram with the slang's authored
    // vertex + fragment pair and reflected UBO/sampler layout.
    //
    // Returns true on success; on failure the chain stays in its
    // prior state. On a backend without slang support
    // (CreatePostProcessSlangProgram defaults to -1), the first
    // artifact compile fails and the chain rejects cleanly so callers
    // can fall back to a .glsl/.glslp preset.
    //
    // Phase 3D-1 stops here: programs are compiled and held, but the
    // Run path doesn't yet dispatch them. The chain reports
    // IsActive() == false when only slang passes are loaded, so
    // callers must not yet rely on slang rendering. Phase 3D-2 wires
    // the actual draw.
    bool LoadSlangPasses(GfxRenderingAPI* rapi,
                         const std::vector<PostProcessSlangArtifact>& artifacts,
                         const std::vector<PostProcessPresetPass>& configs,
                         const std::vector<std::string>& diagnosticNames);

    // Tear down all passes, releasing programs and intermediate FBOs.
    // Drop back to passthrough. The chain's final-output FBO (mDstFb)
    // is kept around for reuse on the next load.
    void UnloadShader(GfxRenderingAPI* rapi);

    // Execute the pipeline. Returns the FBO id whose color texture
    // should be exposed to the GUI for the rest of the frame. If the
    // chain is inactive the return value is srcFb (a passthrough).
    int Run(GfxRenderingAPI* rapi, int srcFb, const PostProcessParams& params);

    // True iff at least one pass is loaded and the backend supports
    // post-process. The interpreter checks this to decide whether to
    // force the MSAA resolve to land in a sampleable texture (rather
    // than blitting directly to the swap-chain back buffer).
    bool IsActive() const;

    // Diagnostic accessor for the currently-loaded preset / shader name.
    const std::string& GetLoadedName() const {
        return mLoadedName;
    }

    // Number of compiled passes currently live in the chain. Reports
    // both legacy and slang flavors — only one is active at a time, so
    // the sum collapses to whichever was loaded last.
    size_t GetLoadedPassCount() const {
        return mPasses.size() + mSlangPasses.size();
    }

    // Which load path produced the active chain. "none" when nothing
    // is loaded, "legacy" for .glsl / .glslp, "slang" for .slang /
    // .slangp.
    const char* GetLoadedFlavor() const {
        if (!mSlangPasses.empty()) return "slang";
        if (!mPasses.empty())      return "legacy";
        return "none";
    }

  private:
    struct Pass {
        int programId = -1;
        // For intermediate passes (all but the last), `outputFb` is
        // a chain-owned FBO sized per the pass's scale config. The
        // final pass writes into the chain's mDstFb instead, leaving
        // its `outputFb` at -1.
        int outputFb = -1;
        // Last applied size for `outputFb`, so we skip
        // UpdateFramebufferParameters when the size hasn't changed.
        uint32_t lastWidth = 0;
        uint32_t lastHeight = 0;
        // Phase 2.2: true when this pass's output is consumed by a
        // downstream pass that has `mipmap_input = true` and therefore
        // needs the producer texture allocated with mip storage. Set
        // during LoadPasses by walking the config list. The chain calls
        // SetPostProcessFramebufferMipmapped on the producer's outputFb
        // so backends that allocate immutable mip levels (D3D11 /
        // Metal) reserve storage before the first
        // UpdateFramebufferParameters.
        bool outputMipmapped = false;
        PostProcessPresetPass config;
    };

    // libretro `aliasN` produces a named binding sampleable by any
    // later pass. Stored in declaration order so binding-slot indices
    // (2 + i) stay stable from compile time through every Run call.
    // `producerPassIdx` is the index in mPasses whose output the
    // alias names — if a later alias name shadows an earlier one,
    // libretro's spec leaves the result undefined; we keep first-
    // declared wins.
    struct Alias {
        std::string name;
        size_t      producerPassIdx;
    };

    // One uploaded external-texture slot.
    struct ExternalTexture {
        std::string name;
        int         textureId    = -1;
        uint32_t    width        = 1;
        uint32_t    height       = 1;
        bool        filterLinear = true;
        PostProcessWrapMode wrapMode = PostProcessWrapMode::ClampToEdge;
    };

    // Phase 3D-1: one compiled slang pass. Holds the backend program
    // id, the reflection artifact (for the later UBO-upload step), and
    // the same per-pass FBO bookkeeping the legacy Pass uses. Lives
    // separately from mPasses so the legacy .glsl/.glslp run path
    // stays byte-identical until the slang Run path comes online in
    // Phase 3D-2.
    //
    // Phase 3E: feedbackFb is non-negative when at least one later
    // pass samples this pass via the `PassFeedback<idx>` semantic.
    // The chain ping-pongs outputFb and feedbackFb at the end of each
    // frame so this frame's pass renders into the previously-stale
    // slot and last frame's output remains live for `PassFeedback`
    // consumers. -1 means "no feedback consumer; static outputFb".
    struct SlangPass {
        int programId = -1;
        int outputFb = -1;
        int feedbackFb = -1;
        uint32_t lastWidth = 0;
        uint32_t lastHeight = 0;
        PostProcessPresetPass config;
        PostProcessSlangArtifact artifact;
    };

    std::vector<Pass> mPasses;
    std::vector<Alias> mAliases;
    std::vector<ExternalTexture> mExternalTextures;
    std::vector<SlangPass> mSlangPasses;

    // Phase 3E: ring buffer of game-FB snapshots for libretro
    // `OriginalHistory<N>` semantic samplers. Indexed via
    // (mFrameIndex - K) % mOriginalHistoryFbs.size() where K is the
    // history index referenced by the shader; K=0 is the live game
    // FB and bypasses storage. mOriginalHistoryFbs is sized to
    // (maxHistoryIdx) entries — empty when no shader references
    // OriginalHistory.
    std::vector<int> mOriginalHistoryFbs;
    uint32_t mOriginalHistoryWidth = 0;
    uint32_t mOriginalHistoryHeight = 0;
    // Wraps; subtraction is computed modulo the ring size.
    uint64_t mFrameIndex = 0;

    int mDstFb = -1;
    uint32_t mDstWidth = 0;
    uint32_t mDstHeight = 0;
    bool mBackendSupported = false;
    std::string mLoadedName;
};

} // namespace Fast
