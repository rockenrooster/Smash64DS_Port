//
//  gfx_metal.h
//  libultraship
//
//  Created by David Chavez on 16.08.22.
//
#pragma once
#ifdef __APPLE__

#include "gfx_rendering_api.h"
#include "../interpreter.h"

#include <imgui_impl_sdl2.h>
#include <simd/simd.h>

static constexpr size_t kMaxVertexBufferPoolSize = 3;
static constexpr size_t METAL_MAX_MULTISAMPLE_SAMPLE_COUNT = 8;
static constexpr size_t MAX_PIXEL_DEPTH_COORDS = 1024;

namespace MTL {
class Texture;
class SamplerState;
class CommandBuffer;
class RenderPassDescriptor;
class RenderCommandEncoder;
class SamplerState;
class ScissorRect;
class Device;
class Function;
class Buffer;
class RenderPipelineState;
class CommandQueue;
class Viewport;
class Library;
} // namespace MTL

namespace CA {
class MetalDrawable;
class MetalLayer;
} // namespace CA

namespace NS {
class AutoreleasePool;
}

static size_t cantor(uint64_t a, uint64_t b) {
    return (a + b) * (a + b + 1) / 2 + b;
}

struct hash_pair_shader_ids {
    size_t operator()(const std::pair<uint64_t, uint32_t>& p) const {
        auto value1 = p.first;
        auto value2 = p.second;
        return cantor(value1, value2);
    }
};

namespace Fast {

struct ShaderProgramMetal {
    uint64_t shader_id0;
    uint64_t shader_id1;

    uint8_t numInputs;
    uint8_t numFloats;
    bool usedTextures[SHADER_MAX_TEXTURES];

    // hashed by msaa_level
    MTL::RenderPipelineState* pipeline_state_variants[9];
};

// Compiled post-process program. The pipeline state expects:
//   - vertex function:   "postprocess_vertex" (uses [[vertex_id]] only)
//   - fragment function: "postprocess_fragment"
//   - fragment binding texture(0)=Source, sampler(0)=SourceSampler,
//     buffer(0) = PostProcessUniformsMetal
// Samplers are pulled from the backend-wide (filter, wrap) cache in
// RunPostProcess so per-pass libretro filter_linearN / wrap_modeN
// settings take effect; this struct no longer owns one.
//
// Metal pins the color-attachment pixel format into the pipeline state
// at creation time, so when a pass writes into an sRGB / float FBO we
// need a separately-built pipeline. The library / vs / fs MTL::Function
// handles stay live so additional variants can be built lazily in
// RunPostProcess; `pipelineVariants` caches the result keyed by
// PostProcessFboFormat. The Default variant is built up front for
// load-time validation + fast path.
struct PostProcessProgramMetal {
    MTL::Library* library;
    MTL::Function* vsFn;
    MTL::Function* fsFn;
    MTL::RenderPipelineState* pipelineVariants[3]; // indexed by (int)PostProcessFboFormat
    std::string name;
};

// Phase 3D-3: compiled slang post-process program for the Metal
// backend. Distinct from PostProcessProgramMetal because:
//   - The vertex stage is authored (uses [[stage_in]] vertex
//     attributes), not a [[vertex_id]] stub.
//   - The UBO size is per-program; we hold an MTL::Buffer sized
//     to the slang artifact's declared bytes and refill each Run.
//   - The slang VS gets its vertex data from a shared MTL::Buffer
//     (mPostProcessSlangVbo) at vertex buffer index 30 — chosen
//     so SPIRV-Cross's MSL emit of `constant UBO& global
//     [[buffer(0)]]` keeps UBO at buffer 0 without colliding.
//
// The pipelineVariants[] cache works the same as the legacy
// PostProcessProgramMetal: build Default up front, build sRGB /
// Float16 lazily when first targeted by RunPostProcessSlang.
struct PostProcessSlangProgramMetal {
    // VS and FS each come from their own MTL::Library (SPIRV-Cross
    // emits each stage as standalone MSL). Both libraries are
    // retained for the slot's lifetime — releasing one while a
    // function from it is still bound to a pipeline state is UB,
    // even if the pipeline appears to retain the function.
    MTL::Library* vsLibrary = nullptr;
    MTL::Library* fsLibrary = nullptr;
    MTL::Function* vsFn = nullptr;
    MTL::Function* fsFn = nullptr;
    MTL::RenderPipelineState* pipelineVariants[3] = {};
    MTL::Buffer* ubo = nullptr;
    uint32_t uboBytes = 0;
    std::string name;
    std::vector<std::string> samplerNames;
};

// Layout must match the `PostProcessUniforms` struct emitted by the
// transpiler. The bundled hand-written MSL companions
// (scanlines.msl / crt-lottes.msl) declare a shorter struct (no
// originalSize) — MSL ignores the trailing bytes the runtime writes,
// so the two layouts coexist as long as hand-written shaders only
// read the prefix they declared.
struct PostProcessUniformsMetal {
    simd::float2 sourceSize;
    simd::float2 outputSize;
    simd::float2 inputSize;
    simd::float2 originalSize;   // Phase 2D: game-FB dimensions.
    simd::int1 frameCount;
    simd::float1 frameDirection;
};

struct TextureDataMetal {
    MTL::Texture* texture;
    MTL::Texture* msaaTexture;
    MTL::SamplerState* sampler;
    uint32_t width;
    uint32_t height;
    uint32_t filtering;
    bool linear_filtering;
};

struct FramebufferMetal {
    MTL::CommandBuffer* mCommandBuffer;
    MTL::RenderPassDescriptor* mRenderPassDescriptor;
    MTL::RenderCommandEncoder* mCommandEncoder;

    MTL::Texture* mDepthTexture;
    MTL::Texture* mMsaaDepthTexture;
    uint32_t mTextureId;
    bool mHasDepthBuffer;
    uint32_t mMsaaLevel;
    bool mRenderTarget;

    // Post-process intermediates may override BGRA8Unorm with sRGB or
    // float. `last` tracks what the underlying MTL::Texture is currently
    // allocated as so UpdateFramebufferParameters can rebuild it when
    // the chain switches formats.
    PostProcessFboFormat mPostProcessFormat = PostProcessFboFormat::Default;
    PostProcessFboFormat mLastPostProcessFormat = PostProcessFboFormat::Default;
    // Phase 2.2: chain marks this FBO when a downstream
    // mipmap_input pass needs to sample it through a mip chain.
    // Metal texture mip count is fixed at creation, so the FBO's
    // MTL::Texture must be re-allocated with mipmapLevelCount > 1
    // at the next UpdateFramebufferParameters when this flag flips.
    bool mPostProcessMipmapped = false;
    bool mLastPostProcessMipmapped = false;

    // State
    bool mHasEndedEncoding;
    bool mHasBoundVertexShader;
    bool mHasBoundFragShader;

    struct ShaderProgramMetal* mLastShaderProgram;
    MTL::Texture* mLastBoundTextures[SHADER_MAX_TEXTURES];
    MTL::SamplerState* mLastBoundSamplers[SHADER_MAX_TEXTURES];
    MTL::ScissorRect* mScissorRect;
    MTL::Viewport* mViewport;

    int8_t mLastDepthTest = -1;
    int8_t mLastDepthMask = -1;
    int8_t mLastZmodeDecal = -1;
};

struct FrameUniforms {
    simd::int1 frameCount;
    simd::float1 noiseScale;
};

struct DrawUniforms {
    simd::int1 textureFiltering[SHADER_MAX_TEXTURES];
};

struct CoordUniforms {
    simd::uint2 coords[MAX_PIXEL_DEPTH_COORDS];
};

class GfxRenderingAPIMetal final : public GfxRenderingAPI {
  public:
    ~GfxRenderingAPIMetal() override;
    const char* GetName() override;
    int GetMaxTextureSize() override;
    GfxClipParameters GetClipParameters() override;
    void UnloadShader(ShaderProgram* oldPrg) override;
    void LoadShader(ShaderProgram* newPrg) override;
    ShaderProgram* CreateAndLoadNewShader(uint64_t shaderId0, uint64_t shaderId1) override;
    ShaderProgram* LookupShader(uint64_t shaderId0, uint64_t shaderId1) override;
    void ShaderGetInfo(ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]) override;
    uint32_t NewTexture() override;
    void SelectTexture(int tile, uint32_t textureId) override;
    void UploadTexture(const uint8_t* rgba32Buf, uint32_t width, uint32_t height) override;
    void SetSamplerParameters(int sampler, bool linear_filter, uint32_t cms, uint32_t cmt) override;
    void SetDepthTestAndMask(bool depth_test, bool z_upd) override;
    void SetZmodeDecal(bool decal) override;
    void SetViewport(int x, int y, int width, int height) override;
    void SetScissor(int x, int y, int width, int height) override;
    void SetUseAlpha(bool useAlpha) override;
    void DrawTriangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) override;
    void Init() override;
    void OnResize() override;
    void StartFrame() override;
    void EndFrame() override;
    void FinishRender() override;
    int CreateFramebuffer() override;
    void DestroyFramebuffer(int fbId) override;
    void UpdateFramebufferParameters(int fb_id, uint32_t width, uint32_t height, uint32_t msaa_level,
                                     bool opengl_invertY, bool render_target, bool has_depth_buffer,
                                     bool can_extract_depth) override;
    void StartDrawToFramebuffer(int fbId, float noiseScale) override;
    void CopyFramebuffer(int fbDstId, int fbSrcId, int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0,
                         int dstX1, int dstY1) override;
    void ClearFramebuffer(bool color, bool depth) override;
    void ReadFramebufferToCPU(int fbId, uint32_t width, uint32_t height, uint16_t* rgba16Buf) override;
    void ResolveMSAAColorBuffer(int fbIdTarger, int fbIdSrc) override;
    std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff>
    GetPixelDepth(int fb_id, const std::set<std::pair<float, float>>& coordinates) override;
    void* GetFramebufferTextureId(int fbId) override;
    void SelectTextureFb(int fbId) override;
    void DeleteTexture(uint32_t texId) override;
    void SetTextureFilter(FilteringMode mode) override;
    FilteringMode GetTextureFilter() override;
    void SetSrgbMode() override;
    ImTextureID GetTextureById(int id) override;

    bool SupportsPostProcess() override;
    int CreatePostProcessProgram(const PostProcessSource& src) override;
    void DestroyPostProcessProgram(int progId) override;
    void RunPostProcess(int progId, int srcFb, int dstFb, int originalFb,
                        const PostProcessParams& params) override;
    void SetPostProcessFramebufferFormat(int fb_id, PostProcessFboFormat fmt) override;
    void SetPostProcessFramebufferMipmapped(int fb_id, bool mipmapped) override;
    void GeneratePostProcessMipmaps(int fb_id) override;
    int CreatePostProcessStaticTexture(uint32_t width, uint32_t height,
                                       const uint8_t* rgba8) override;
    void DestroyPostProcessStaticTexture(int textureId) override;
    int CreatePostProcessSlangProgram(const PostProcessSlangProgramSource& src) override;
    void DestroyPostProcessSlangProgram(int progId) override;
    void RunPostProcessSlang(int progId, int dstFb,
                             const uint8_t* uboData, uint32_t uboBytes,
                             const int* samplerFbIds, uint32_t samplerCount,
                             const PostProcessParams& params) override;

    void NewFrame();
    void SetupFloatingFrame();
    void RenderDrawData(ImDrawData* drawData);
    bool MetalInit(SDL_Renderer* renderer);

  private:
    bool NonUniformThreadGroupSupported();
    void SetupScreenFramebuffer(uint32_t width, uint32_t height);
    // Elements that only need to be setup once
    SDL_Renderer* mRenderer;
    CA::MetalLayer* mLayer; // CA::MetalLayer*
    MTL::Device* mDevice;
    MTL::CommandQueue* mCommandQueue;

    int mCurrentVertexBufferPoolIndex = 0;
    MTL::Buffer* mVertexBufferPool[kMaxVertexBufferPoolSize];
    std::unordered_map<std::pair<uint64_t, uint32_t>, struct ShaderProgramMetal, hash_pair_shader_ids>
        mShaderProgramPool;

    std::vector<struct TextureDataMetal> mTextures;
    std::vector<FramebufferMetal> mFramebuffers;
    FrameUniforms mFrameUniforms;
    CoordUniforms mCoordUniforms;
    DrawUniforms mDrawUniforms;
    MTL::Buffer* mFrameUniformBuffer;

    uint32_t mMsaaNumQualityLevels[METAL_MAX_MULTISAMPLE_SAMPLE_COUNT];

    // Depth querying
    MTL::Buffer* mCoordUniformBuffer;
    MTL::Buffer* mDepthValueOutputBuffer;
    size_t mCoordBufferSize;
    MTL::Function* mDepthComputeFunction;
    MTL::Function* mConvertToRgb5a1Function;

    // Current state
    struct ShaderProgramMetal* mShaderProgram;
    CA::MetalDrawable* mCurrentDrawable;
    std::set<int> mDrawnFramebuffers;
    NS::AutoreleasePool* mFrameAutoreleasePool;

    int mCurrentTile;
    uint32_t mCurrentTextureIds[SHADER_MAX_TEXTURES];
    // 1x1 black RGBA texture used as a fallback when a fragment shader's sampler slot
    // would otherwise alias mTextures[0] (the screen drawable). Set up in Init() after the
    // screen framebuffer is created. Mirrors OpenGL/GLD's "zero texture for unbound sampler"
    // substitution, preventing shader-side feedback loops on combine modes that declare
    // TEXEL1 without loading TMEM tile 1.
    uint32_t mFallbackTextureId = 0;

    int32_t mRenderTargetHeight;
    int mCurrentFramebuffer;
    size_t mCurrentVertexBufferOffset;
    FilteringMode mCurrentFilterMode = FILTER_THREE_POINT;

    bool mNonUniformThreadgroupSupported;

    // Post-process programs allocated via CreatePostProcessProgram. Slots
    // with pipeline==nullptr are empty (returned to the free list when
    // DestroyPostProcessProgram clears them).
    std::vector<PostProcessProgramMetal> mPostProcessPrograms;

    // Sampler cache shared by all post-process programs, keyed by an
    // encoded (filter, wrap) pair. Lookup happens per pass so libretro
    // filter_linearN / wrap_modeN translate into a small bounded set of
    // MTL::SamplerState objects.
    std::unordered_map<uint32_t, MTL::SamplerState*> mPostProcessSamplers;

    // Static textures uploaded for libretro `.glslp` external
    // `textures = "..."` entries. Vector index = handle returned by
    // CreatePostProcessStaticTexture. Empty slots (texture == nullptr)
    // are reused.
    std::vector<MTL::Texture*> mPostProcessStaticTextures;

    // Phase 3D-3: slang post-process programs + shared vertex buffer.
    // The VBO holds 3 vertices of {vec4 Position, vec2 TexCoord}
    // interleaved (24 B stride, 72 B total) — created lazily on
    // first slang Run and reused for every slang draw.
    std::vector<PostProcessSlangProgramMetal> mPostProcessSlangPrograms;
    MTL::Buffer* mPostProcessSlangVbo = nullptr;
    // Vertex buffer slot the slang pipelines route their stage_in
    // through. Picked high so it never collides with
    // SPIRV-Cross's `[[buffer(0)]]` UBO binding.
    static constexpr uint32_t kPostProcessSlangVertexBufferIndex = 30;
};

} // namespace Fast

bool Metal_IsSupported();

#endif
