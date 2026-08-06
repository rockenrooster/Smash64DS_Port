#pragma once

#if defined(ENABLE_DX11) || defined(ENABLE_DX12)

#ifdef __cplusplus
#include "../interpreter.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include "gfx_rendering_api.h"
#include "d3d11.h"
#include "d3dcompiler.h"

namespace Fast {

struct PerFrameCB {
    uint32_t noise_frame;
    float noise_scale;
    uint32_t padding[2]; // constant buffers must be multiples of 16 bytes in size
};

struct PerDrawCB {
    struct Texture {
        uint32_t width;
        uint32_t height;
        uint32_t linear_filtering;
        uint32_t padding;
    } mTextures[SHADER_MAX_TEXTURES];
};

struct Coord {
    int x, y;
};

struct TextureData {
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> resource_view;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_state;
    uint32_t width;
    uint32_t height;
    bool linear_filtering;
};

struct FramebufferDX11 {
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depth_stencil_view;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> depth_stencil_srv;
    uint32_t texture_id;
    bool has_depth_buffer;
    uint32_t msaa_level;

    // Post-process intermediates may override the default RGBA8 color
    // format (sRGB or float). `last` tracks the format the texture is
    // currently allocated with so UpdateFramebufferParameters can
    // force a re-allocation when the chain switches a slot's format.
    PostProcessFboFormat postProcessFormat = PostProcessFboFormat::Default;
    PostProcessFboFormat lastPostProcessFormat = PostProcessFboFormat::Default;
    // Phase 2.2: chain marks this FBO when a downstream
    // mipmap_input pass needs to sample it through a mip chain.
    // D3D11 mip allocation is immutable so the texture must be
    // recreated with MipLevels > 1 and
    // D3D11_RESOURCE_MISC_GENERATE_MIPS at the next
    // UpdateFramebufferParameters; `last` tracks the allocated state
    // so we re-allocate when the flag changes.
    bool postProcessMipmapped = false;
    bool lastPostProcessMipmapped = false;
};

struct ShaderProgramD3D11 {
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout;
    Microsoft::WRL::ComPtr<ID3D11BlendState> blend_state;

    uint64_t shader_id0;
    uint64_t shader_id1;
    uint8_t numInputs;
    uint8_t numFloats;
    bool usedTextures[SHADER_MAX_TEXTURES];
};

// Compiled post-process program. The fullscreen-triangle vertex shader
// reads SV_VertexID only (no input layout, no vertex buffer); the pixel
// shader samples t0 with s0 and reads PostProcessUniformsD3D11 from b0.
struct PostProcessProgramD3D11 {
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;
    std::string name;
    // Number of trailing alias / external-texture `vec2 <name>Size`
    // members the transpiled cbuffer reserves for this program (same
    // as PostProcessSource::aliasNames.size()). The runtime cbuffer
    // upload writes the prefix struct then `aliasCount` vec2s, then
    // pads to a multiple of 16 for D3D11 buffer alignment.
    uint32_t aliasCount = 0;
};

// Layout mirrors the fixed prefix of `cbuffer PostProcessUniforms`
// emitted by the transpiler. The bundled hand-written HLSL
// companions (scanlines.hlsl / crt-lottes.hlsl) declare a shorter
// cbuffer (no OriginalSize) — D3D11 ignores the extra bytes written
// by the runtime, so the two layouts coexist as long as hand-written
// shaders only read the prefix they declared.
//
// Phase 2.1 adds a variable-length tail: each alias / external-
// texture name appends a `float2 <name>Size` slot starting at offset
// `kPostProcessUniformsPrefixBytes`, in std140-derived layout (each
// vec2 is 8 bytes, naturally aligned). The fixed prefix size is 40;
// the cbuffer total is rounded up to a multiple of 16 at allocation
// time. See gfx_direct3d11.cpp::RunPostProcess for the writer.
struct PostProcessUniformsD3D11 {
    float SourceSize[2];     // row 0.xy
    float OutputSize[2];     // row 0.zw
    float InputSize[2];      // row 1.xy
    float OriginalSize[2];   // row 1.zw  (Phase 2D: game-FB dimensions)
    int FrameCount;          // row 2.x
    float FrameDirection;    // row 2.y
};
static constexpr size_t kPostProcessUniformsPrefixBytes = 40;
static constexpr size_t kPostProcessUniformsAliasStride = 8; // vec2 in std140

// Phase 3D-3: compiled slang program for D3D11. Distinct from
// PostProcessProgramD3D11 because the slang path has an authored VS
// (consumes vertex data — not SV_VertexID), the UBO size is per-
// program (declared by the .slang's UBO block), and samplers are
// bound in declaration order rather than at fixed Source=0 /
// Original=1 slots.
//
// SPIRV-Cross emits the slang VS with input attributes at
// `TEXCOORD<location>` semantics (locations 0 and 1 for the
// standard slang Position+TexCoord pair). The input layout below
// matches that emit; the per-program slot owns the layout because
// future shaders may declare different attribute layouts.
struct PostProcessSlangProgramD3D11 {
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  pixel_shader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>  input_layout;
    // Per-program cbuffer sized to the slang artifact's declared
    // UBO bytes (rounded up to 16-byte multiple for D3D11). The
    // chain memcpys the pre-built UBO blob into it each frame.
    Microsoft::WRL::ComPtr<ID3D11Buffer>       ubo;
    uint32_t uboBytes = 0;
    std::string name;
    // Sampler-binding plan: each entry's name comes from the
    // artifact's samplers[] in declaration order. Backends bind
    // samplerFbIds[i]'s texture at slot i and a sampler at the
    // same slot.
    std::vector<std::string> samplerNames;
};

class GfxWindowBackendDXGI;

class GfxRenderingAPIDX11 final : public GfxRenderingAPI {
  public:
    GfxRenderingAPIDX11() = default;
    ~GfxRenderingAPIDX11() override;
    GfxRenderingAPIDX11(GfxWindowBackendDXGI* backend);
    const char* GetName() override;
    int GetMaxTextureSize() override;
    GfxClipParameters GetClipParameters() override;
    void UnloadShader(struct ShaderProgram* oldPrg) override;
    void LoadShader(struct ShaderProgram* newPrg) override;
    struct ShaderProgram* CreateAndLoadNewShader(uint64_t shaderId0, uint64_t shaderId1) override;
    struct ShaderProgram* LookupShader(uint64_t shaderId0, uint64_t shaderId1) override;
    void ShaderGetInfo(struct ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]) override;
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

    PFN_D3D11_CREATE_DEVICE mDX11CreateDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> mContext;
    Microsoft::WRL::ComPtr<ID3D11Device> mDevice;
    GfxWindowBackendDXGI* mWindowBackend = nullptr;
    D3D_FEATURE_LEVEL mFeatureLevel;

  private:
    void CreateDepthStencilObjects(uint32_t width, uint32_t height, uint32_t msaa_count, ID3D11DepthStencilView** view,
                                   ID3D11ShaderResourceView** srv);

    HMODULE mDX11Module;

    HMODULE mCompilerModule;
    pD3DCompile mD3dCompile;

    uint32_t mMsaaNumQualityLevels[D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT];

    Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizerState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthStencilState;
    Microsoft::WRL::ComPtr<ID3D11Buffer> mVertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> mPerFrameCb;
    Microsoft::WRL::ComPtr<ID3D11Buffer> mPerDrawCb;
    Microsoft::WRL::ComPtr<ID3D11Buffer> mCoordBuffer;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mCoordBufferSrv;
    Microsoft::WRL::ComPtr<ID3D11Buffer> mDepthValueOutputBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> mDepthValueOutputBufferCopy;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mDepthValueOutputUav;
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> mComputeShader;
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> mComputeShaderMsaa;
    Microsoft::WRL::ComPtr<ID3DBlob> mComputeShaderMsaaBlob;
    size_t mCoordBufferSize;

#if DEBUG_D3D
    Microsoft::WRL::ComPtr<ID3D11Debug> debug;
#endif

    PerFrameCB mPerFrameCbData;
    PerDrawCB mPerDrawCbData;

    std::map<std::pair<uint64_t, uint32_t>, struct ShaderProgramD3D11> mShaderProgramPool;

    std::vector<struct TextureData> mTextures;
    int mCurrentTile;
    uint32_t mCurrentTextureIds[SHADER_MAX_TEXTURES];

    std::vector<FramebufferDX11> mFrameBuffers;

    // Current state

    struct ShaderProgramD3D11* mShaderProgram;

    int32_t mRenderTargetHeight;
    int mCurrentFramebuffer;
    // Default to FILTER_LINEAR so the SetSamplerParameters check
    //   `linear_filter && mCurrentFilterMode == FILTER_LINEAR`
    // succeeds when the game requests bilinear via gDPSetTextureFilter,
    // producing standard 4-tap bilinear filtering on the GPU.
    // The original FILTER_NONE default forced every sampler to point/nearest
    // filtering regardless of the game's request. FILTER_THREE_POINT enables
    // a shader-based N64-accurate 3-tap filter, but standard bilinear matches
    // the look of glide64mk2/mupen better and is what most ports expect.
    FilteringMode mCurrentFilterMode = FILTER_LINEAR;

    // Previous states (to prevent setting states needlessly)

    struct ShaderProgramD3D11* mLastShaderProgram = nullptr;
    uint32_t mLastVertexBufferStride = 0;
    Microsoft::WRL::ComPtr<ID3D11BlendState> mLastBlendState = nullptr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mLastResourceViews[SHADER_MAX_TEXTURES] = { nullptr, nullptr };
    Microsoft::WRL::ComPtr<ID3D11SamplerState> mLastSamplerStates[SHADER_MAX_TEXTURES] = { nullptr, nullptr };

    D3D_PRIMITIVE_TOPOLOGY mLastPrimitaveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

    // Post-process programs. Empty slots have null vertex_shader and are
    // returned to the free list when DestroyPostProcessProgram clears them.
    std::vector<PostProcessProgramD3D11> mPostProcessPrograms;
    // Sampler cache keyed by (filter, wrap) so per-pass libretro
    // `filter_linearN` / `wrap_modeN` settings translate into a small
    // bounded set of D3D11 sampler objects. Default linear/clamp-to-edge
    // is used for the `Original` (slot 1) binding on every call.
    std::unordered_map<uint32_t, Microsoft::WRL::ComPtr<ID3D11SamplerState>>
        mPostProcessSamplers;
    // Constant buffer for PostProcessUniformsD3D11; created lazily.
    Microsoft::WRL::ComPtr<ID3D11Buffer> mPostProcessCb;
    // Current allocation size of mPostProcessCb. Resized when a
    // program with more alias slots than the previous one runs (see
    // gfx_direct3d11.cpp::RunPostProcess for the growth path).
    size_t mPostProcessCbBytes = 0;

    // Static textures uploaded for libretro `.glslp` external
    // `textures = "..."` entries. Index returned by
    // CreatePostProcessStaticTexture is the vector index; empty slots
    // (texture == nullptr) are reused.
    struct StaticTextureD3D11 {
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
    };
    std::vector<StaticTextureD3D11> mPostProcessStaticTextures;

    // Phase 3D-3: slang post-process programs. Independent handle
    // namespace from mPostProcessPrograms. Vertex buffer + cached
    // slang-flavor blend state are shared across all slang programs
    // (the geometry never changes), allocated lazily on first slang
    // Run.
    std::vector<PostProcessSlangProgramD3D11> mPostProcessSlangPrograms;
    Microsoft::WRL::ComPtr<ID3D11Buffer> mPostProcessSlangVbo;
};

std::string gfx_direct3d_common_build_shader(size_t& numFloats, const CCFeatures& cc_features,
                                             bool include_root_signature, bool three_point_filtering, bool use_srgb);
} // namespace Fast
#endif
#endif
