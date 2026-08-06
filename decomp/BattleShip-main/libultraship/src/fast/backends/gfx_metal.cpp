//
//  gfx_metal.cpp
//  libultraship
//
//  Created by David Chavez on 16.08.22.
//

#ifdef __APPLE__

#include "fast/backends/gfx_metal.h"

#include <vector>
#include <algorithm>
#include <unordered_map>
#include <queue>
#include <time.h>
#include <math.h>
#include <cmath>
#include <stddef.h>

#ifndef _LANGUAGE_C
#define _LANGUAGE_C
#endif

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include <Metal/Metal.hpp>

#include <SDL_render.h>
#include <imgui_impl_metal.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include "fast/backends/gfx_metal_shader.h"

#include "libultraship/libultra/abi.h"
#include "ship/Context.h"
#include "ship/config/ConsoleVariable.h"

#define ARRAY_COUNT(arr) (int32_t)(sizeof(arr) / sizeof(arr[0]))

// MARK: - Helpers
namespace Fast {

static MTL::SamplerAddressMode gfx_cm_to_metal(uint32_t val) {
    switch (val) {
        case G_TX_NOMIRROR | G_TX_CLAMP:
            return MTL::SamplerAddressModeClampToEdge;
        case G_TX_MIRROR | G_TX_WRAP:
            return MTL::SamplerAddressModeMirrorRepeat;
        case G_TX_MIRROR | G_TX_CLAMP:
            return MTL::SamplerAddressModeMirrorClampToEdge;
        case G_TX_NOMIRROR | G_TX_WRAP:
            return MTL::SamplerAddressModeRepeat;
    }

    return MTL::SamplerAddressModeClampToEdge;
}

// MARK: - ImGui & SDL Wrappers

bool GfxRenderingAPIMetal::NonUniformThreadGroupSupported() {
#ifdef __IOS__
    // iOS devices with A11 or later support dispatch threads
    return mDevice->supportsFamily(MTL::GPUFamilyApple4);
#else
    // macOS devices with Metal 2 support dispatch threads
    return mDevice->supportsFamily(MTL::GPUFamilyMac2);
#endif
}

bool GfxRenderingAPIMetal::MetalInit(SDL_Renderer* renderer) {
    mRenderer = renderer;
    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();

    mLayer = (CA::MetalLayer*)SDL_RenderGetMetalLayer(renderer);
    mLayer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);

    mDevice = mLayer->device();
    mCommandQueue = mDevice->newCommandQueue();

    for (size_t i = 0; i < kMaxVertexBufferPoolSize; i++) {
        MTL::Buffer* new_buffer = mDevice->newBuffer(256 * 32 * 3 * sizeof(float) * 50, MTL::ResourceStorageModeShared);
        mVertexBufferPool[i] = new_buffer;
    }

    autorelease_pool->release();
    mNonUniformThreadgroupSupported = NonUniformThreadGroupSupported();

    return ImGui_ImplMetal_Init(mDevice);
}

static void SetupScreenFramebuffer(uint32_t width, uint32_t height);

void GfxRenderingAPIMetal::NewFrame() {
    int width, height;
    SDL_GetRendererOutputSize(mRenderer, &width, &height);
    SetupScreenFramebuffer(width, height);

    MTL::RenderPassDescriptor* current_render_pass = mFramebuffers[0].mRenderPassDescriptor;
    ImGui_ImplMetal_NewFrame(current_render_pass);
}

void GfxRenderingAPIMetal::SetupFloatingFrame() {
    // We need the descriptor for the main framebuffer and to clear the existing depth attachment
    // so that we can set ImGui up again for our floating windows. Helps avoid Metal API validation issues.
    MTL::RenderPassDescriptor* current_render_pass = mFramebuffers[0].mRenderPassDescriptor;
    current_render_pass->setDepthAttachment(nullptr);
    ImGui_ImplMetal_NewFrame(current_render_pass);
}

void GfxRenderingAPIMetal::RenderDrawData(ImDrawData* drawData) {
    auto framebuffer = mFramebuffers[0];

    // Workaround for detecting when transitioning to/from full screen mode.
    MTL::Texture* screen_texture = mTextures[framebuffer.mTextureId].texture;
    int fb_width = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
    int fb_height = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
    if (screen_texture->width() != fb_width || screen_texture->height() != fb_height)
        return;

    ImGui_ImplMetal_RenderDrawData(drawData, framebuffer.mCommandBuffer, framebuffer.mCommandEncoder);
}

// MARK: - Metal Graphics Rendering API

const char* GfxRenderingAPIMetal::GetName() {
    return "Metal";
}

int GfxRenderingAPIMetal::GetMaxTextureSize() {
    return mDevice->supportsFamily(MTL::GPUFamilyApple3) ? 16384 : 8192;
}

void GfxRenderingAPIMetal::Init() {
    // Create the default framebuffer which represents the window
    FramebufferMetal& fb = mFramebuffers[CreateFramebuffer()];
    fb.mMsaaLevel = 1;

    // Check device for supported msaa levels
    for (uint32_t sample_count = 1; sample_count <= METAL_MAX_MULTISAMPLE_SAMPLE_COUNT; sample_count++) {
        if (mDevice->supportsTextureSampleCount(sample_count)) {
            mMsaaNumQualityLevels[sample_count - 1] = 1;
        } else {
            mMsaaNumQualityLevels[sample_count - 1] = 0;
        }
    }

    // Compute shader for retrieving depth values
    const char* depth_shader = R"(
        #include <metal_stdlib>
        using namespace metal;

        struct CoordUniforms {
            uint2 coords[1024];
        };

        kernel void depthKernel(depth2d<float, access::read> depth_texture [[ texture(0) ]],
                                     constant CoordUniforms& query_coords [[ buffer(0) ]],
                                     device float* output_values [[ buffer(1) ]],
                                     ushort2 thread_position [[ thread_position_in_grid ]]) {
            uint2 coord = query_coords.coords[thread_position.x];
            output_values[thread_position.x] = depth_texture.read(coord);
        }

        kernel void convertToRGB5A1(texture2d<half, access::read> inTexture [[ texture(0) ]],
                                    device short* outputBuffer [[ buffer(0) ]],
                                    uint2 gid [[ thread_position_in_grid ]]) {
            uint index = gid.x + (inTexture.get_width() * gid.y);
            half4 pixel = inTexture.read(gid);
            uint r = pixel.r * 0x1F;
            uint g = pixel.g * 0x1F;
            uint b = pixel.b * 0x1F;
            uint a = pixel.a > 0;
            outputBuffer[index] = (r << 11) | (g << 6) | (b << 1) | a;
        }
    )";

    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();

    NS::Error* error = nullptr;
    MTL::Library* library =
        mDevice->newLibrary(NS::String::string(depth_shader, NS::UTF8StringEncoding), nullptr, &error);

    if (error != nullptr)
        SPDLOG_ERROR("Failed to compile shader library: {}",
                     error->localizedDescription()->cString(NS::UTF8StringEncoding));

    mDepthComputeFunction = library->newFunction(NS::String::string("depthKernel", NS::UTF8StringEncoding));
    mConvertToRgb5a1Function = library->newFunction(NS::String::string("convertToRGB5A1", NS::UTF8StringEncoding));

    library->release();

    // Allocate a 1x1 transparent-black RGBA texture for use as a fallback when a fragment
    // shader sampler slot would otherwise default to mTextures[0] (the screen drawable).
    // Without this, a CC mode that declares TEXEL1 but whose tile 1 was never loaded ends
    // up sampling the screen color buffer mid-render — a visible feedback loop on Metal.
    // OpenGL's GLD driver substitutes a zero texture in the same situation; this matches.
    {
        mFallbackTextureId = NewTexture();
        TextureDataMetal& fallback = mTextures[mFallbackTextureId];

        MTL::TextureDescriptor* desc =
            MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatRGBA8Unorm, 1, 1, false);
        desc->setStorageMode(MTL::StorageModeShared);
        fallback.texture = mDevice->newTexture(desc);
        fallback.width = 1;
        fallback.height = 1;
        const uint8_t zero_pixel[4] = { 0, 0, 0, 0 };
        fallback.texture->replaceRegion(MTL::Region::Make2D(0, 0, 1, 1), 0, zero_pixel, 4);

        MTL::SamplerDescriptor* sd = MTL::SamplerDescriptor::alloc()->init();
        sd->setMinFilter(MTL::SamplerMinMagFilterNearest);
        sd->setMagFilter(MTL::SamplerMinMagFilterNearest);
        sd->setSAddressMode(MTL::SamplerAddressModeClampToEdge);
        sd->setTAddressMode(MTL::SamplerAddressModeClampToEdge);
        sd->setRAddressMode(MTL::SamplerAddressModeClampToEdge);
        fallback.sampler = mDevice->newSamplerState(sd);
        fallback.filtering = FILTER_LINEAR;
        fallback.linear_filtering = false;
        sd->release();

        // Point every sampler slot at the fallback by default. Real ImportTexture calls
        // overwrite per-slot indices as they happen.
        for (int i = 0; i < SHADER_MAX_TEXTURES; i++) {
            mCurrentTextureIds[i] = mFallbackTextureId;
        }
    }

    autorelease_pool->release();
}

struct GfxClipParameters GfxRenderingAPIMetal::GetClipParameters() {
    return { true, false };
}

void GfxRenderingAPIMetal::UnloadShader(struct ShaderProgram* old_prg) {
}

void GfxRenderingAPIMetal::LoadShader(struct ShaderProgram* new_prg) {
    mShaderProgram = (struct ShaderProgramMetal*)new_prg;
}

struct ShaderProgram* GfxRenderingAPIMetal::CreateAndLoadNewShader(uint64_t shader_id0, uint64_t shader_id1) {
    CCFeatures cc_features;
    gfx_cc_get_features(shader_id0, shader_id1, &cc_features);

    size_t numFloats = 0;
    std::string buf;
    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();

    MTL::VertexDescriptor* vertex_descriptor =
        gfx_metal_build_shader(buf, numFloats, cc_features, mCurrentFilterMode == FILTER_THREE_POINT);

    NS::Error* error = nullptr;
    MTL::Library* library =
        mDevice->newLibrary(NS::String::string(buf.data(), NS::UTF8StringEncoding), nullptr, &error);

    if (error != nullptr)
        SPDLOG_ERROR("Failed to compile shader library, error {}",
                     error->localizedDescription()->cString(NS::UTF8StringEncoding));

    MTL::RenderPipelineDescriptor* pipeline_descriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    MTL::Function* vertexFunc = library->newFunction(NS::String::string("vertexShader", NS::UTF8StringEncoding));
    MTL::Function* fragmentFunc = library->newFunction(NS::String::string("fragmentShader", NS::UTF8StringEncoding));

    pipeline_descriptor->setVertexFunction(vertexFunc);
    pipeline_descriptor->setFragmentFunction(fragmentFunc);
    pipeline_descriptor->setVertexDescriptor(vertex_descriptor);

    pipeline_descriptor->colorAttachments()->object(0)->setPixelFormat(mSrgbMode ? MTL::PixelFormatBGRA8Unorm_sRGB
                                                                                 : MTL::PixelFormatBGRA8Unorm);
    pipeline_descriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
    if (cc_features.opt_alpha) {
        pipeline_descriptor->colorAttachments()->object(0)->setBlendingEnabled(true);
        pipeline_descriptor->colorAttachments()->object(0)->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
        pipeline_descriptor->colorAttachments()->object(0)->setDestinationRGBBlendFactor(
            MTL::BlendFactorOneMinusSourceAlpha);
        pipeline_descriptor->colorAttachments()->object(0)->setRgbBlendOperation(MTL::BlendOperationAdd);
        pipeline_descriptor->colorAttachments()->object(0)->setSourceAlphaBlendFactor(MTL::BlendFactorZero);
        pipeline_descriptor->colorAttachments()->object(0)->setDestinationAlphaBlendFactor(MTL::BlendFactorOne);
        pipeline_descriptor->colorAttachments()->object(0)->setAlphaBlendOperation(MTL::BlendOperationAdd);
        pipeline_descriptor->colorAttachments()->object(0)->setWriteMask(MTL::ColorWriteMaskAll);
    } else {
        pipeline_descriptor->colorAttachments()->object(0)->setBlendingEnabled(false);
        pipeline_descriptor->colorAttachments()->object(0)->setWriteMask(MTL::ColorWriteMaskAll);
    }

    struct ShaderProgramMetal* prg = &mShaderProgramPool[std::make_pair(shader_id0, shader_id1)];
    prg->shader_id0 = shader_id0;
    prg->shader_id1 = shader_id1;
    prg->usedTextures[0] = cc_features.usedTextures[0];
    prg->usedTextures[1] = cc_features.usedTextures[1];
    prg->usedTextures[2] = cc_features.used_masks[0];
    prg->usedTextures[3] = cc_features.used_masks[1];
    prg->usedTextures[4] = cc_features.used_blend[0];
    prg->usedTextures[5] = cc_features.used_blend[1];
    prg->numInputs = cc_features.numInputs;
    prg->numFloats = numFloats;

    // Prepoluate pipeline state cache with program and available msaa levels
    for (int i = 0; i < ARRAY_COUNT(mMsaaNumQualityLevels); i++) {
        if (mMsaaNumQualityLevels[i] == 1) {
            int msaa_level = i + 1;
            pipeline_descriptor->setSampleCount(msaa_level);
            MTL::RenderPipelineState* pipeline_state = mDevice->newRenderPipelineState(pipeline_descriptor, &error);

            if (!pipeline_state || error != nullptr) {
                // Pipeline State creation could fail if we haven't properly set up our pipeline descriptor.
                // If the Metal API validation is enabled, we can find out more information about what
                // went wrong.  (Metal API validation is enabled by default when a debug build is run
                // from Xcode)
                SPDLOG_ERROR("Failed to create pipeline state, error {}",
                             error->localizedDescription()->cString(NS::UTF8StringEncoding));
            }

            prg->pipeline_state_variants[msaa_level] = pipeline_state;
        }
    }

    LoadShader((struct ShaderProgram*)prg);

    vertexFunc->release();
    fragmentFunc->release();
    library->release();
    pipeline_descriptor->release();
    autorelease_pool->release();

    return (struct ShaderProgram*)prg;
}

struct ShaderProgram* GfxRenderingAPIMetal::LookupShader(uint64_t shader_id0, uint64_t shader_id1) {
    auto it = mShaderProgramPool.find(std::make_pair(shader_id0, shader_id1));
    return it == mShaderProgramPool.end() ? nullptr : (struct ShaderProgram*)&it->second;
}

void GfxRenderingAPIMetal::ShaderGetInfo(struct ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]) {
    struct ShaderProgramMetal* p = (struct ShaderProgramMetal*)prg;

    *numInputs = p->numInputs;
    usedTextures[0] = p->usedTextures[0];
    usedTextures[1] = p->usedTextures[1];
}

uint32_t GfxRenderingAPIMetal::NewTexture() {
    mTextures.resize(mTextures.size() + 1);
    return (uint32_t)(mTextures.size() - 1);
}

void GfxRenderingAPIMetal::DeleteTexture(uint32_t texID) {
}

void GfxRenderingAPIMetal::SelectTexture(int tile, uint32_t texture_id) {
    mCurrentTile = tile;
    mCurrentTextureIds[tile] = texture_id;
}

void GfxRenderingAPIMetal::UploadTexture(const uint8_t* rgba32_buf, uint32_t width, uint32_t height) {
    TextureDataMetal* texture_data = &mTextures[mCurrentTextureIds[mCurrentTile]];

    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();

    MTL::TextureDescriptor* texture_descriptor =
        MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatRGBA8Unorm, width, height, true);
    texture_descriptor->setArrayLength(1);
    texture_descriptor->setMipmapLevelCount(1);
    texture_descriptor->setSampleCount(1);
    texture_descriptor->setStorageMode(MTL::StorageModeShared);

    MTL::Region region = MTL::Region::Make2D(0, 0, width, height);

    MTL::Texture* texture = texture_data->texture;
    if (texture_data->texture == nullptr || texture_data->texture->width() != width ||
        texture_data->texture->height() != height) {
        if (texture_data->texture != nullptr)
            texture_data->texture->release();

        texture = mDevice->newTexture(texture_descriptor);
    }

    NS::UInteger bytes_per_pixel = 4;
    NS::UInteger bytes_per_row = bytes_per_pixel * width;
    texture->replaceRegion(region, 0, rgba32_buf, bytes_per_row);
    texture_data->texture = texture;

    autorelease_pool->release();
}

void GfxRenderingAPIMetal::SetSamplerParameters(int tile, bool linear_filter, uint32_t cms, uint32_t cmt) {
    TextureDataMetal* texture_data = &mTextures[mCurrentTextureIds[tile]];
    texture_data->linear_filtering = linear_filter;
    texture_data->filtering = !linear_filter ? FILTER_LINEAR : FILTER_THREE_POINT;

    // This function is called twice per texture, the first one only to set default values.
    // Maybe that could be skipped? Anyway, make sure to release the first default sampler
    // state before setting the actual one.
    if (texture_data->sampler != nullptr) {
        texture_data->sampler->release();
    }

    MTL::SamplerDescriptor* sampler_descriptor = MTL::SamplerDescriptor::alloc()->init();
    MTL::SamplerMinMagFilter filter = linear_filter && mCurrentFilterMode == FILTER_LINEAR
                                          ? MTL::SamplerMinMagFilterLinear
                                          : MTL::SamplerMinMagFilterNearest;
    sampler_descriptor->setMinFilter(filter);
    sampler_descriptor->setMagFilter(filter);
    sampler_descriptor->setSAddressMode(gfx_cm_to_metal(cms));
    sampler_descriptor->setTAddressMode(gfx_cm_to_metal(cmt));
    sampler_descriptor->setRAddressMode(MTL::SamplerAddressModeRepeat);

    texture_data->sampler = mDevice->newSamplerState(sampler_descriptor);
    sampler_descriptor->release();
}

void GfxRenderingAPIMetal::SetDepthTestAndMask(bool depth_test, bool depth_mask) {
    mCurrentDepthTest = depth_test;
    mCurrentDepthMask = depth_mask;
}

void GfxRenderingAPIMetal::SetZmodeDecal(bool zmode_decal) {
    mCurrentZmodeDecal = zmode_decal;
}

void GfxRenderingAPIMetal::SetViewport(int x, int y, int width, int height) {
    FramebufferMetal& fb = mFramebuffers[mCurrentFramebuffer];

    fb.mViewport->originX = x;
    fb.mViewport->originY = mRenderTargetHeight - y - height;
    fb.mViewport->width = width;
    fb.mViewport->height = height;
    fb.mViewport->znear = 0;
    fb.mViewport->zfar = 1;

    fb.mCommandEncoder->setViewport(*fb.mViewport);
}

void GfxRenderingAPIMetal::SetScissor(int x, int y, int width, int height) {
    FramebufferMetal& fb = mFramebuffers[mCurrentFramebuffer];
    TextureDataMetal tex = mTextures[fb.mTextureId];

    // clamp to viewport size as metal does not support larger values than viewport size
    fb.mScissorRect->x = std::max(0, std::min<int>(x, tex.width));
    fb.mScissorRect->y = std::max(0, std::min<int>(mRenderTargetHeight - y - height, tex.height));
    fb.mScissorRect->width = std::max(0, std::min<int>(width, tex.width));
    fb.mScissorRect->height = std::max(0, std::min<int>(height, tex.height));

    fb.mCommandEncoder->setScissorRect(*fb.mScissorRect);
}

void GfxRenderingAPIMetal::SetUseAlpha(bool use_alpha) {
    // Already part of the pipeline state from shader info
}

void GfxRenderingAPIMetal::DrawTriangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) {
    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();
    bool textures_changed = false;

    auto& current_framebuffer = mFramebuffers[mCurrentFramebuffer];

    if (current_framebuffer.mLastDepthTest != mCurrentDepthTest ||
        current_framebuffer.mLastDepthMask != mCurrentDepthMask) {
        current_framebuffer.mLastDepthTest = mCurrentDepthTest;
        current_framebuffer.mLastDepthMask = mCurrentDepthMask;

        MTL::DepthStencilDescriptor* depth_descriptor = MTL::DepthStencilDescriptor::alloc()->init();
        depth_descriptor->setDepthWriteEnabled(mCurrentDepthMask);
        depth_descriptor->setDepthCompareFunction(
            mCurrentDepthTest ? (mCurrentZmodeDecal ? MTL::CompareFunctionLessEqual : MTL::CompareFunctionLess)
                              : MTL::CompareFunctionAlways);

        MTL::DepthStencilState* depth_stencil_state = mDevice->newDepthStencilState(depth_descriptor);
        current_framebuffer.mCommandEncoder->setDepthStencilState(depth_stencil_state);

        // setDepthStencilState retains; drop our +1 from newDepthStencilState
        // or every depth-mode change this frame leaks a state object.
        depth_stencil_state->release();
        depth_descriptor->release();
    }

    if (current_framebuffer.mLastZmodeDecal != mCurrentZmodeDecal) {
        current_framebuffer.mLastZmodeDecal = mCurrentZmodeDecal;

        current_framebuffer.mCommandEncoder->setTriangleFillMode(MTL::TriangleFillModeFill);
        current_framebuffer.mCommandEncoder->setCullMode(MTL::CullModeNone);
        current_framebuffer.mCommandEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);

        // SSDB = SlopeScaledDepthBias 120 leads to -2 at 240p which is the same as N64 mode which has very little
        // fighting
        const int n64modeFactor = 120;
        const int noVanishFactor = 100;
        float SSDB = -2;
        switch (Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger(CVAR_Z_FIGHTING_MODE, 0)) {
            case 1: // scaled z-fighting (N64 mode like)
                SSDB = -1.0f * (float)mRenderTargetHeight / n64modeFactor;
                break;
            case 2: // no vanishing paths
                SSDB = -1.0f * (float)mRenderTargetHeight / noVanishFactor;
                break;
            case 0: // disabled
            default:
                SSDB = -2;
        }
        current_framebuffer.mCommandEncoder->setDepthBias(0, mCurrentZmodeDecal ? SSDB : 0, 0);
    }

    MTL::Buffer* vertex_buffer = mVertexBufferPool[mCurrentVertexBufferPoolIndex];
    if (mCurrentVertexBufferOffset + sizeof(float) * buf_vbo_len > vertex_buffer->length()) {
        // The per-frame pool slot is full — writing past it would corrupt
        // adjacent heap/driver memory. Dropping the draw is visually wrong
        // for one frame but safe.
        SPDLOG_ERROR("Metal vertex buffer overflow: offset={} + {} > capacity={}; dropping draw",
                     mCurrentVertexBufferOffset, sizeof(float) * buf_vbo_len, vertex_buffer->length());
        autorelease_pool->release();
        return;
    }
    memcpy((char*)vertex_buffer->contents() + mCurrentVertexBufferOffset, buf_vbo, sizeof(float) * buf_vbo_len);

    if (!current_framebuffer.mHasBoundVertexShader) {
        current_framebuffer.mCommandEncoder->setVertexBuffer(vertex_buffer, 0, 0);
        current_framebuffer.mHasBoundVertexShader = true;
    }

    current_framebuffer.mCommandEncoder->setVertexBufferOffset(mCurrentVertexBufferOffset, 0);

    if (!current_framebuffer.mHasBoundFragShader) {
        current_framebuffer.mCommandEncoder->setFragmentBuffer(mFrameUniformBuffer, 0, 0);
        current_framebuffer.mHasBoundFragShader = true;
    }

    for (int i = 0; i < SHADER_MAX_TEXTURES; i++) {
        if (mShaderProgram->usedTextures[i]) {
            uint32_t tid = mCurrentTextureIds[i];
            MTL::Texture* tex = mTextures[tid].texture;
            MTL::SamplerState* smp = mTextures[tid].sampler;
            // Backstop: if the binding for a slot the shader actually uses would alias the
            // screen drawable (mTextures[0]) or has no MTL::Texture/sampler yet, redirect
            // to the 1x1 black fallback so the shader samples zeros instead of the live
            // color buffer it is rendering into.
            if (tid == 0 || tex == nullptr || smp == nullptr) {
                tid = mFallbackTextureId;
                tex = mTextures[tid].texture;
                smp = mTextures[tid].sampler;
            }

            if (current_framebuffer.mLastBoundTextures[i] != tex) {
                current_framebuffer.mLastBoundTextures[i] = tex;
                current_framebuffer.mCommandEncoder->setFragmentTexture(tex, i);
            }
            if (current_framebuffer.mLastBoundSamplers[i] != smp) {
                current_framebuffer.mLastBoundSamplers[i] = smp;
                current_framebuffer.mCommandEncoder->setFragmentSamplerState(smp, i);
            }
        }

        if (mCurrentFilterMode == FILTER_THREE_POINT) {
            mDrawUniforms.textureFiltering[i] = mTextures[mCurrentTextureIds[i]].filtering;
            textures_changed = true;
        }
    }

    if (textures_changed) {
        current_framebuffer.mCommandEncoder->setFragmentBytes(&mDrawUniforms, sizeof(DrawUniforms), 1);
    }

    if (current_framebuffer.mLastShaderProgram != mShaderProgram) {
        current_framebuffer.mLastShaderProgram = mShaderProgram;

        MTL::RenderPipelineState* pipeline_state =
            mShaderProgram->pipeline_state_variants[current_framebuffer.mMsaaLevel];
        current_framebuffer.mCommandEncoder->setRenderPipelineState(pipeline_state);
    }

    current_framebuffer.mCommandEncoder->drawPrimitives(MTL::PrimitiveTypeTriangle, 0.f, buf_vbo_num_tris * 3);
    mCurrentVertexBufferOffset += sizeof(float) * buf_vbo_len;

    autorelease_pool->release();
}

void GfxRenderingAPIMetal::OnResize() {
}

void GfxRenderingAPIMetal::StartFrame() {
    mFrameUniforms.frameCount++;
    if (mFrameUniforms.frameCount > 150) {
        // No high values, as noise starts to look ugly
        mFrameUniforms.frameCount = 0;
    }

    if (!mFrameUniformBuffer) {
        mFrameUniformBuffer = mDevice->newBuffer(sizeof(FrameUniforms), MTL::ResourceCPUCacheModeDefaultCache);
    }
    if (!mCoordUniformBuffer) {
        mCoordUniformBuffer = mDevice->newBuffer(sizeof(CoordUniforms), MTL::ResourceCPUCacheModeDefaultCache);
    }

    mCurrentVertexBufferOffset = 0;

    mFrameAutoreleasePool = NS::AutoreleasePool::alloc()->init();
}

void GfxRenderingAPIMetal::EndFrame() {
    std::set<int>::iterator it = mDrawnFramebuffers.begin();
    it++;

    while (it != mDrawnFramebuffers.end()) {
        auto framebuffer = mFramebuffers[*it];

        if (!framebuffer.mHasEndedEncoding)
            framebuffer.mCommandEncoder->endEncoding();

        framebuffer.mCommandBuffer->commit();
        it++;
    }

    auto screen_framebuffer = mFramebuffers[0];
    screen_framebuffer.mCommandEncoder->endEncoding();
    screen_framebuffer.mCommandBuffer->presentDrawable(mCurrentDrawable);
    mCurrentVertexBufferPoolIndex = (mCurrentVertexBufferPoolIndex + 1) % kMaxVertexBufferPoolSize;
    screen_framebuffer.mCommandBuffer->commit();

    mDrawnFramebuffers.clear();

    // Cleanup states
    for (int fb_id = 0; fb_id < (int)mFramebuffers.size(); fb_id++) {
        FramebufferMetal& fb = mFramebuffers[fb_id];

        // DestroyFramebuffer tombstones a slot by setting mTextureId
        // to UINT32_MAX and deleting mViewport / mScissorRect. The
        // memsets below would dereference those null pointers and
        // SEGV at fault_addr=0x10 (= mScissorRect + 16). Skip the
        // per-frame state reset for slots that have no live state.
        if (fb.mTextureId == UINT32_MAX) {
            continue;
        }

        fb.mLastShaderProgram = nullptr;
        fb.mCommandBuffer = nullptr;
        fb.mCommandEncoder = nullptr;
        fb.mHasEndedEncoding = false;
        fb.mHasBoundVertexShader = false;
        fb.mHasBoundFragShader = false;
        for (int i = 0; i < SHADER_MAX_TEXTURES; i++) {
            fb.mLastBoundTextures[i] = nullptr;
            fb.mLastBoundSamplers[i] = nullptr;
        }
        memset(fb.mViewport, 0, sizeof(MTL::Viewport));
        memset(fb.mScissorRect, 0, sizeof(MTL::ScissorRect));
        fb.mLastDepthTest = -1;
        fb.mLastDepthMask = -1;
        fb.mLastZmodeDecal = -1;
    }

    mFrameAutoreleasePool->release();
}

void GfxRenderingAPIMetal::FinishRender() {
}

void GfxRenderingAPIMetal::DestroyFramebuffer(int fbId) {
    if (fbId < 0 || (size_t)fbId >= mFramebuffers.size()) {
        return;
    }
    FramebufferMetal& fb = mFramebuffers[fbId];
    if (fb.mTextureId == UINT32_MAX) {
        return; // Already destroyed.
    }
    if (fb.mDepthTexture != nullptr) {
        fb.mDepthTexture->release();
        fb.mDepthTexture = nullptr;
    }
    if (fb.mMsaaDepthTexture != nullptr) {
        fb.mMsaaDepthTexture->release();
        fb.mMsaaDepthTexture = nullptr;
    }
    // The color attachment lives in mTextures[mTextureId]; release the
    // MTL::Texture so its GPU memory frees. Metal's command buffer /
    // render pass / encoder objects are autoreleased pool-managed, so
    // dropping our pointers is sufficient.
    if (fb.mTextureId < mTextures.size()) {
        TextureDataMetal& tex = mTextures[fb.mTextureId];
        if (tex.texture != nullptr) {
            tex.texture->release();
            tex.texture = nullptr;
        }
        if (tex.msaaTexture != nullptr) {
            tex.msaaTexture->release();
            tex.msaaTexture = nullptr;
        }
        if (tex.sampler != nullptr) {
            tex.sampler->release();
            tex.sampler = nullptr;
        }
        tex.width = 0;
        tex.height = 0;
    }
    delete fb.mScissorRect;
    fb.mScissorRect = nullptr;
    delete fb.mViewport;
    fb.mViewport = nullptr;
    fb.mCommandBuffer = nullptr;
    fb.mRenderPassDescriptor = nullptr;
    fb.mCommandEncoder = nullptr;
    fb.mLastShaderProgram = nullptr;
    fb.mHasDepthBuffer = false;
    fb.mMsaaLevel = 0;
    fb.mRenderTarget = false;
    fb.mTextureId = UINT32_MAX;
}

int GfxRenderingAPIMetal::CreateFramebuffer() {
    uint32_t texture_id = NewTexture();
    TextureDataMetal& t = mTextures[texture_id];

    size_t index = mFramebuffers.size();
    mFramebuffers.resize(mFramebuffers.size() + 1);
    FramebufferMetal& data = mFramebuffers.back();
    data.mScissorRect = new MTL::ScissorRect();
    data.mViewport = new MTL::Viewport();
    data.mTextureId = texture_id;

    uint32_t tile = 0;
    uint32_t saved = mCurrentTextureIds[tile];
    mCurrentTextureIds[tile] = texture_id;
    SetSamplerParameters(0, true, G_TX_WRAP, G_TX_WRAP);
    mCurrentTextureIds[tile] = saved;

    return (int)index;
}

void GfxRenderingAPIMetal::SetupScreenFramebuffer(uint32_t width, uint32_t height) {
    mCurrentDrawable = nullptr;
    mCurrentDrawable = mLayer->nextDrawable();

    bool msaa_enabled = Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger("gMSAAValue", 1) > 1;

    FramebufferMetal& fb = mFramebuffers[0];
    TextureDataMetal& tex = mTextures[fb.mTextureId];

    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();

    if (tex.texture != nullptr)
        tex.texture->release();

    tex.texture = mCurrentDrawable->texture();

    MTL::RenderPassDescriptor* render_pass_descriptor = MTL::RenderPassDescriptor::renderPassDescriptor();
    render_pass_descriptor->colorAttachments()->object(0)->setTexture(tex.texture);
    render_pass_descriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionLoad);
    render_pass_descriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);

    tex.width = width;
    tex.height = height;

    // recreate depth texture only if necessary (size changed)
    if (fb.mDepthTexture == nullptr || (fb.mDepthTexture->width() != width || fb.mDepthTexture->height() != height)) {
        if (fb.mDepthTexture != nullptr)
            fb.mDepthTexture->release();

        // If possible, we eventually we want to disable this when msaa is enabled since we don't need this depth
        // texture However, problem is if the user switches to msaa during game, we need a way to then generate it
        // before drawing.
        MTL::TextureDescriptor* depth_tex_desc =
            MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatDepth32Float, width, height, true);

        depth_tex_desc->setTextureType(MTL::TextureType2D);
        depth_tex_desc->setStorageMode(MTL::StorageModePrivate);
        depth_tex_desc->setSampleCount(1);
        depth_tex_desc->setArrayLength(1);
        depth_tex_desc->setMipmapLevelCount(1);
        depth_tex_desc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);

        fb.mDepthTexture = mDevice->newTexture(depth_tex_desc);
    }

    render_pass_descriptor->depthAttachment()->setTexture(fb.mDepthTexture);
    render_pass_descriptor->depthAttachment()->setLoadAction(MTL::LoadActionLoad);
    render_pass_descriptor->depthAttachment()->setStoreAction(MTL::StoreActionStore);
    render_pass_descriptor->depthAttachment()->setClearDepth(1);

    if (fb.mRenderPassDescriptor != nullptr)
        fb.mRenderPassDescriptor->release();

    fb.mRenderPassDescriptor = render_pass_descriptor;
    fb.mRenderPassDescriptor->retain();
    fb.mRenderTarget = true;
    fb.mHasDepthBuffer = true;

    autorelease_pool->release();
}

void GfxRenderingAPIMetal::UpdateFramebufferParameters(int fb_id, uint32_t width, uint32_t height, uint32_t msaa_level,
                                                       bool opengl_invertY, bool render_target, bool has_depth_buffer,
                                                       bool can_extract_depth) {
    // Screen framebuffer is handled separately on a frame by frame basis
    // see `SetupScreenFramebuffer`.
    if (fb_id == 0) {
        int width, height;
        SDL_GetRendererOutputSize(mRenderer, &width, &height);
        mLayer->setDrawableSize({ CGFloat(width), CGFloat(height) });

        return;
    }

    FramebufferMetal& fb = mFramebuffers[fb_id];
    TextureDataMetal& tex = mTextures[fb.mTextureId];

    width = std::max(width, 1U);
    height = std::max(height, 1U);
    while (msaa_level > 1 && mMsaaNumQualityLevels[msaa_level - 1] == 0) {
        --msaa_level;
    }

    const bool formatChanged = (fb.mPostProcessFormat != fb.mLastPostProcessFormat);
    const bool mipmappedChanged = (fb.mPostProcessMipmapped != fb.mLastPostProcessMipmapped);
    bool diff = tex.width != width || tex.height != height || fb.mMsaaLevel != msaa_level ||
                formatChanged || mipmappedChanged;

    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();

    if (diff || (fb.mRenderPassDescriptor != nullptr) != render_target) {
        MTL::PixelFormat colorFormat = mSrgbMode ? MTL::PixelFormatBGRA8Unorm_sRGB : MTL::PixelFormatBGRA8Unorm;
        switch (fb.mPostProcessFormat) {
            case PostProcessFboFormat::Default:
                // Keep the backend-wide default — respect mSrgbMode for
                // non-post-process FBOs and the chain's mDstFb.
                break;
            case PostProcessFboFormat::Srgb:
                colorFormat = MTL::PixelFormatBGRA8Unorm_sRGB;
                break;
            case PostProcessFboFormat::Float16:
                colorFormat = MTL::PixelFormatRGBA16Float;
                break;
        }
        // Phase 2.2: a chain-marked mipmap_input source needs its
        // texture allocated with a populated mip chain so the
        // GeneratePostProcessMipmaps blit encoder has somewhere to
        // write. Metal textures cap mip count at creation, so the
        // chain must mark before this call. MSAA + mips is not a
        // viable combination on Metal — the chain only marks
        // non-MSAA intermediates.
        const bool mipmapped = fb.mPostProcessMipmapped && msaa_level <= 1;
        NS::UInteger mipLevels = 1;
        if (mipmapped) {
            NS::UInteger maxDim = (width > height) ? width : height;
            mipLevels = 1;
            while (maxDim > 1) {
                maxDim >>= 1;
                ++mipLevels;
            }
        }
        MTL::TextureDescriptor* tex_descriptor = MTL::TextureDescriptor::alloc()->init();
        tex_descriptor->setTextureType(MTL::TextureType2D);
        tex_descriptor->setWidth(width);
        tex_descriptor->setHeight(height);
        tex_descriptor->setSampleCount(1);
        tex_descriptor->setMipmapLevelCount(mipLevels);
        tex_descriptor->setPixelFormat(colorFormat);
        tex_descriptor->setUsage((render_target ? MTL::TextureUsageRenderTarget : 0) | MTL::TextureUsageShaderRead);

        if (tex.texture != nullptr)
            tex.texture->release();

        tex.texture = mDevice->newTexture(tex_descriptor);

        if (msaa_level > 1) {
            tex_descriptor->setTextureType(MTL::TextureType2DMultisample);
            tex_descriptor->setSampleCount(msaa_level);
            tex_descriptor->setStorageMode(MTL::StorageModePrivate);
            tex_descriptor->setUsage(render_target ? MTL::TextureUsageRenderTarget : 0);

            if (tex.msaaTexture != nullptr)
                tex.msaaTexture->release();
            tex.msaaTexture = mDevice->newTexture(tex_descriptor);
        }

        // Metal does not zero-initialize newly allocated textures (unlike OpenGL).
        // Subsequent passes use LoadActionLoad and the game's draws are clipped to
        // its native viewport via scissor; pixels outside that scissor on the
        // upscaled host target are never written. With MSAA enabled, the resolve
        // propagates the unwritten MSAA samples into the resolve target every
        // frame, so a fresh msaaTexture's uninitialized contents become a visible
        // edge band around the rendered image. Clear all newly allocated render
        // targets to opaque black up front so unscissored regions read predictably.
        auto initRenderTargetToBlack = [this](MTL::Texture* texture) {
            if (texture == nullptr) {
                return;
            }
            if ((texture->usage() & MTL::TextureUsageRenderTarget) == 0) {
                return;
            }
            MTL::RenderPassDescriptor* clear_pass = MTL::RenderPassDescriptor::renderPassDescriptor();
            clear_pass->colorAttachments()->object(0)->setTexture(texture);
            clear_pass->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
            clear_pass->colorAttachments()->object(0)->setClearColor(MTL::ClearColor(0.0, 0.0, 0.0, 1.0));
            clear_pass->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
            MTL::CommandBuffer* clear_cb = mCommandQueue->commandBuffer();
            clear_cb->setLabel(NS::String::string("Clear new RT to black", NS::UTF8StringEncoding));
            MTL::RenderCommandEncoder* clear_enc = clear_cb->renderCommandEncoder(clear_pass);
            clear_enc->endEncoding();
            clear_cb->commit();
        };
        initRenderTargetToBlack(tex.texture);
        if (msaa_level > 1) {
            initRenderTargetToBlack(tex.msaaTexture);
        }

        if (render_target) {
            MTL::RenderPassDescriptor* render_pass_descriptor = MTL::RenderPassDescriptor::renderPassDescriptor();

            bool fb_msaa_enabled = (msaa_level > 1);
            bool game_msaa_enabled =
                Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger("gMSAAValue", 1) > 1;

            if (fb_msaa_enabled) {
                render_pass_descriptor->colorAttachments()->object(0)->setTexture(tex.msaaTexture);
                render_pass_descriptor->colorAttachments()->object(0)->setResolveTexture(tex.texture);
                render_pass_descriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionLoad);
                render_pass_descriptor->colorAttachments()->object(0)->setStoreAction(
                    MTL::StoreActionStoreAndMultisampleResolve);
            } else {
                render_pass_descriptor->colorAttachments()->object(0)->setTexture(tex.texture);
                render_pass_descriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionLoad);
                render_pass_descriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
            }

            if (fb.mRenderPassDescriptor != nullptr)
                fb.mRenderPassDescriptor->release();

            fb.mRenderPassDescriptor = render_pass_descriptor;
            fb.mRenderPassDescriptor->retain();
        }

        tex.width = width;
        tex.height = height;

        tex_descriptor->release();
    }

    if (has_depth_buffer && (diff || !fb.mHasDepthBuffer || (fb.mDepthTexture != nullptr) != can_extract_depth)) {
        MTL::TextureDescriptor* depth_tex_desc =
            MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatDepth32Float, width, height, true);
        depth_tex_desc->setTextureType(MTL::TextureType2D);
        depth_tex_desc->setStorageMode(MTL::StorageModePrivate);
        depth_tex_desc->setSampleCount(1);
        depth_tex_desc->setArrayLength(1);
        depth_tex_desc->setMipmapLevelCount(1);
        depth_tex_desc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);

        if (fb.mDepthTexture != nullptr)
            fb.mDepthTexture->release();

        fb.mDepthTexture = mDevice->newTexture(depth_tex_desc);

        if (msaa_level > 1) {
            depth_tex_desc->setTextureType(MTL::TextureType2DMultisample);
            depth_tex_desc->setSampleCount(msaa_level);

            if (fb.mMsaaDepthTexture != nullptr)
                fb.mMsaaDepthTexture->release();

            fb.mMsaaDepthTexture = mDevice->newTexture(depth_tex_desc);
        }
    }

    if (has_depth_buffer && fb.mRenderPassDescriptor != nullptr) {
        if (msaa_level > 1) {
            fb.mRenderPassDescriptor->depthAttachment()->setTexture(fb.mMsaaDepthTexture);
            fb.mRenderPassDescriptor->depthAttachment()->setResolveTexture(fb.mDepthTexture);
            fb.mRenderPassDescriptor->depthAttachment()->setLoadAction(MTL::LoadActionLoad);
            fb.mRenderPassDescriptor->depthAttachment()->setStoreAction(MTL::StoreActionMultisampleResolve);
            fb.mRenderPassDescriptor->depthAttachment()->setClearDepth(1);
        } else {
            fb.mRenderPassDescriptor->depthAttachment()->setTexture(fb.mDepthTexture);
            fb.mRenderPassDescriptor->depthAttachment()->setLoadAction(MTL::LoadActionLoad);
            fb.mRenderPassDescriptor->depthAttachment()->setStoreAction(MTL::StoreActionStore);
            fb.mRenderPassDescriptor->depthAttachment()->setClearDepth(1);
        }
    } else if (fb.mRenderPassDescriptor != nullptr) {
        fb.mRenderPassDescriptor->setDepthAttachment(nullptr);
    }

    fb.mRenderTarget = render_target;
    fb.mHasDepthBuffer = has_depth_buffer;
    fb.mMsaaLevel = msaa_level;
    fb.mLastPostProcessFormat = fb.mPostProcessFormat;
    fb.mLastPostProcessMipmapped = fb.mPostProcessMipmapped;

    autorelease_pool->release();
}

void GfxRenderingAPIMetal::StartDrawToFramebuffer(int fb_id, float noise_scale) {
    FramebufferMetal& fb = mFramebuffers[fb_id];
    mRenderTargetHeight = mTextures[fb.mTextureId].height;

    mCurrentFramebuffer = fb_id;
    mDrawnFramebuffers.insert(fb_id);

    if (fb.mRenderTarget && fb.mCommandBuffer == nullptr && fb.mCommandEncoder == nullptr &&
        fb.mRenderPassDescriptor != nullptr) {
        fb.mCommandBuffer = mCommandQueue->commandBuffer();
        std::string fbcb_label = fmt::format("FrameBuffer {} Command Buffer", fb_id);
        fb.mCommandBuffer->setLabel(NS::String::string(fbcb_label.c_str(), NS::UTF8StringEncoding));

        // Queue the command buffers in order of start draw
        fb.mCommandBuffer->enqueue();

        fb.mCommandEncoder = fb.mCommandBuffer->renderCommandEncoder(fb.mRenderPassDescriptor);
        std::string fbce_label = fmt::format("FrameBuffer {} Command Encoder", fb_id);
        fb.mCommandEncoder->setLabel(NS::String::string(fbce_label.c_str(), NS::UTF8StringEncoding));
        fb.mCommandEncoder->setDepthClipMode(MTL::DepthClipModeClamp);
    }

    if (noise_scale != 0.0f) {
        mFrameUniforms.noiseScale = 1.0f / noise_scale;
    }

    memcpy(mFrameUniformBuffer->contents(), &mFrameUniforms, sizeof(FrameUniforms));
}

void GfxRenderingAPIMetal::ClearFramebuffer(bool color, bool depth) {
    if (!color && !depth) {
        return;
    }

    auto& framebuffer = mFramebuffers[mCurrentFramebuffer];

    // End the current render encoder
    framebuffer.mCommandEncoder->endEncoding();

    // Track the original load action and set the next load actions to Load to leverage the blit results
    MTL::RenderPassColorAttachmentDescriptor* srcColorAttachment =
        framebuffer.mRenderPassDescriptor->colorAttachments()->object(0);
    MTL::LoadAction origLoadAction = srcColorAttachment->loadAction();
    if (color) {
        srcColorAttachment->setLoadAction(MTL::LoadActionClear);
    }

    MTL::RenderPassDepthAttachmentDescriptor* srcDepthAttachment = framebuffer.mRenderPassDescriptor->depthAttachment();
    MTL::LoadAction origDepthLoadAction = MTL::LoadActionDontCare;
    if (depth && framebuffer.mHasDepthBuffer) {
        origDepthLoadAction = srcDepthAttachment->loadAction();
        srcDepthAttachment->setLoadAction(MTL::LoadActionClear);
    }

    // Create a new render encoder back onto the framebuffer
    framebuffer.mCommandEncoder = framebuffer.mCommandBuffer->renderCommandEncoder(framebuffer.mRenderPassDescriptor);

    std::string fbce_label = fmt::format("FrameBuffer {} Command Encoder After Clear", mCurrentFramebuffer);
    framebuffer.mCommandEncoder->setLabel(NS::String::string(fbce_label.c_str(), NS::UTF8StringEncoding));
    framebuffer.mCommandEncoder->setDepthClipMode(MTL::DepthClipModeClamp);
    framebuffer.mCommandEncoder->setViewport(*framebuffer.mViewport);
    framebuffer.mCommandEncoder->setScissorRect(*framebuffer.mScissorRect);

    // Now that the command encoder is started, we set the original load actions back for the next frame's use
    srcColorAttachment->setLoadAction(origLoadAction);
    if (depth && framebuffer.mHasDepthBuffer) {
        srcDepthAttachment->setLoadAction(origDepthLoadAction);
    }

    // Reset the framebuffer so the encoder is setup again when rendering triangles
    framebuffer.mHasBoundVertexShader = false;
    framebuffer.mHasBoundFragShader = false;
    framebuffer.mLastShaderProgram = nullptr;
    for (int i = 0; i < SHADER_MAX_TEXTURES; i++) {
        framebuffer.mLastBoundTextures[i] = nullptr;
        framebuffer.mLastBoundSamplers[i] = nullptr;
    }
    framebuffer.mLastDepthTest = -1;
    framebuffer.mLastDepthMask = -1;
    framebuffer.mLastZmodeDecal = -1;
}

void GfxRenderingAPIMetal::ResolveMSAAColorBuffer(int fb_id_target, int fb_id_source) {
    int source_texture_id = mFramebuffers[fb_id_source].mTextureId;
    MTL::Texture* source_texture = mTextures[source_texture_id].texture;

    int target_texture_id = mFramebuffers[fb_id_target].mTextureId;
    MTL::Texture* target_texture =
        target_texture_id == 0 ? mCurrentDrawable->texture() : mTextures[target_texture_id].texture;

    // Workaround for detecting when transitioning to/from full screen mode.
    if (source_texture->width() != target_texture->width() || source_texture->height() != target_texture->height()) {
        return;
    }

    // When the target buffer is our main window buffer, we need to perform the blit operation on the target
    // buffer instead of the source buffer
    if (fb_id_target != 0) {
        // Copy over the source framebuffer's texture to the target
        auto& source_framebuffer = mFramebuffers[fb_id_source];
        source_framebuffer.mCommandEncoder->endEncoding();
        source_framebuffer.mHasEndedEncoding = true;

        MTL::BlitCommandEncoder* blit_encoder = source_framebuffer.mCommandBuffer->blitCommandEncoder();
        blit_encoder->setLabel(NS::String::string("MSAA Copy Encoder", NS::UTF8StringEncoding));
        blit_encoder->copyFromTexture(source_texture, target_texture);
        blit_encoder->endEncoding();
    } else {
        // End the current render encoder
        auto& target_framebuffer = mFramebuffers[fb_id_target];
        target_framebuffer.mCommandEncoder->endEncoding();

        // Create a blit encoder
        MTL::BlitCommandEncoder* blit_encoder = target_framebuffer.mCommandBuffer->blitCommandEncoder();
        blit_encoder->setLabel(NS::String::string("MSAA Copy Encoder", NS::UTF8StringEncoding));

        // Copy the texture over using the origins and size
        blit_encoder->copyFromTexture(source_texture, target_texture);
        blit_encoder->endEncoding();

        // Update the load action to Load to leverage the blit results
        // The original load action will be set back on the next frame by SetupScreenFramebuffer
        MTL::RenderPassColorAttachmentDescriptor* targetColorAttachment =
            target_framebuffer.mRenderPassDescriptor->colorAttachments()->object(0);
        targetColorAttachment->setLoadAction(MTL::LoadActionLoad);

        // Create a new render encoder back onto the framebuffer
        target_framebuffer.mCommandEncoder =
            target_framebuffer.mCommandBuffer->renderCommandEncoder(target_framebuffer.mRenderPassDescriptor);

        std::string fbce_label = fmt::format("FrameBuffer {} Command Encoder After MSAA Resolve", fb_id_target);
        target_framebuffer.mCommandEncoder->setLabel(NS::String::string(fbce_label.c_str(), NS::UTF8StringEncoding));
    }
}

std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff>
GfxRenderingAPIMetal::GetPixelDepth(int fb_id, const std::set<std::pair<float, float>>& coordinates) {
    auto framebuffer = mFramebuffers[fb_id];

    if (coordinates.size() > mCoordBufferSize) {
        if (mDepthValueOutputBuffer != nullptr)
            mDepthValueOutputBuffer->release();

        mDepthValueOutputBuffer =
            mDevice->newBuffer(sizeof(float) * coordinates.size(), MTL::ResourceOptionCPUCacheModeDefault);
        mDepthValueOutputBuffer->setLabel(NS::String::string("Depth output buffer", NS::UTF8StringEncoding));

        mCoordBufferSize = coordinates.size();
    }

    // zero out the buffer
    memset(mCoordUniformBuffer->contents(), 0, sizeof(CoordUniforms));
    memset(mDepthValueOutputBuffer->contents(), 0, sizeof(float) * coordinates.size());

    // map coordinates to right y axis
    size_t i = 0;
    for (const auto& coord : coordinates) {
        mCoordUniforms.coords[i].x = (uint32_t)(int32_t)coord.first;
        mCoordUniforms.coords[i].y = (uint32_t)(int32_t)(framebuffer.mDepthTexture->height() - 1 - coord.second);
        ++i;
    }

    // set uniform values
    memcpy(mCoordUniformBuffer->contents(), &mCoordUniforms, sizeof(CoordUniforms));

    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();

    auto command_buffer = mCommandQueue->commandBuffer();
    command_buffer->setLabel(NS::String::string("Depth Shader Command Buffer", NS::UTF8StringEncoding));

    NS::Error* error = nullptr;
    MTL::ComputePipelineState* compute_pipeline_state = mDevice->newComputePipelineState(mDepthComputeFunction, &error);

    MTL::ComputeCommandEncoder* compute_encoder = command_buffer->computeCommandEncoder();
    compute_encoder->setComputePipelineState(compute_pipeline_state);
    compute_encoder->setTexture(framebuffer.mDepthTexture, 0);
    compute_encoder->setBuffer(mCoordUniformBuffer, 0, 0);
    compute_encoder->setBuffer(mDepthValueOutputBuffer, 0, 1);

    MTL::Size thread_group_size = MTL::Size::Make(1, 1, 1);
    MTL::Size thread_group_count = MTL::Size::Make(coordinates.size(), 1, 1);

    // We validate if the device supports non-uniform threadgroup sizes
    if (mNonUniformThreadgroupSupported) {
        compute_encoder->dispatchThreads(thread_group_count, thread_group_size);
    } else {
        compute_encoder->dispatchThreadgroups(thread_group_count, thread_group_size);
    }

    compute_encoder->endEncoding();

    command_buffer->commit();
    command_buffer->waitUntilCompleted();

    // Now the depth values can be accessed in the buffer.
    float* depth_values = (float*)mDepthValueOutputBuffer->contents();

    std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff> res;
    {
        size_t i = 0;
        for (const auto& coord : coordinates) {
            res.emplace(coord, depth_values[i++] * 65532.0f);
        }
    }

    compute_pipeline_state->release();
    autorelease_pool->release();

    return res;
}

void* GfxRenderingAPIMetal::GetFramebufferTextureId(int fb_id) {
    return (void*)mTextures[mFramebuffers[fb_id].mTextureId].texture;
}

void GfxRenderingAPIMetal::SelectTextureFb(int fb_id) {
    int tile = 0;
    SelectTexture(tile, mFramebuffers[fb_id].mTextureId);
}

void GfxRenderingAPIMetal::CopyFramebuffer(int fb_dst_id, int fb_src_id, int srcX0, int srcY0, int srcX1, int srcY1,
                                           int dstX0, int dstY0, int dstX1, int dstY1) {
    if (fb_src_id >= (int)mFramebuffers.size() || fb_dst_id >= (int)mFramebuffers.size()) {
        return;
    }

    FramebufferMetal& source_framebuffer = mFramebuffers[fb_src_id];

    int source_texture_id = source_framebuffer.mTextureId;
    MTL::Texture* source_texture = mTextures[source_texture_id].texture;

    int target_texture_id = mFramebuffers[fb_dst_id].mTextureId;
    MTL::Texture* target_texture = mTextures[target_texture_id].texture;

    // Standalone path: when called between frames (e.g. SSB64 framebuffer-
    // capture bridge from a game-thread task FuncStart), the source FB has
    // no live mCommandBuffer/mCommandEncoder -- EndFrame nulls them. Fall
    // back to a fresh, self-contained command buffer + blit encoder, modeled
    // after ReadFramebufferToCPU below. The blit reads the prior frame's
    // committed pixels (command queues are FIFO so this runs after any
    // pending render commands targeting the source FB).
    if (source_framebuffer.mCommandBuffer == nullptr) {
        NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();

        MTL::CommandBuffer* cb = mCommandQueue->commandBuffer();
        cb->setLabel(NS::String::string("Standalone Copy Framebuffer Command Buffer", NS::UTF8StringEncoding));

        MTL::BlitCommandEncoder* blit_encoder = cb->blitCommandEncoder();
        blit_encoder->setLabel(
            NS::String::string("Standalone Copy Framebuffer Encoder", NS::UTF8StringEncoding));

        MTL::Origin source_origin = MTL::Origin(srcX0, srcY0, 0);
        MTL::Origin target_origin = MTL::Origin(dstX0, dstY0, 0);
        MTL::Size source_size = MTL::Size(srcX1 - srcX0, srcY1 - srcY0, 1);

        blit_encoder->copyFromTexture(source_texture, 0, 0, source_origin, source_size, target_texture, 0, 0,
                                      target_origin);
        blit_encoder->endEncoding();

        cb->commit();
        // Wait so the destination texture is ready to be sampled by any draw
        // submitted in the same frame as the eventual SelectTextureFb call.
        // The standalone path is rare (only at scene-transition boundaries
        // like 1P stage clear or VS results) and the blit itself is cheap.
        cb->waitUntilCompleted();

        autorelease_pool->release();
        return;
    }

    // End the current render encoder
    source_framebuffer.mCommandEncoder->endEncoding();

    // Create a blit encoder
    MTL::BlitCommandEncoder* blit_encoder = source_framebuffer.mCommandBuffer->blitCommandEncoder();
    blit_encoder->setLabel(NS::String::string("Copy Framebuffer Encoder", NS::UTF8StringEncoding));

    MTL::Origin source_origin = MTL::Origin(srcX0, srcY0, 0);
    MTL::Origin target_origin = MTL::Origin(dstX0, dstY0, 0);
    MTL::Size source_size = MTL::Size(srcX1 - srcX0, srcY1 - srcY0, 1);

    // Copy the texture over using the origins and size
    blit_encoder->copyFromTexture(source_texture, 0, 0, source_origin, source_size, target_texture, 0, 0,
                                  target_origin);
    blit_encoder->endEncoding();

    // Track the original load action and set the next load actions to Load to leverage the blit results
    MTL::RenderPassColorAttachmentDescriptor* srcColorAttachment =
        source_framebuffer.mRenderPassDescriptor->colorAttachments()->object(0);
    MTL::LoadAction origLoadAction = srcColorAttachment->loadAction();
    srcColorAttachment->setLoadAction(MTL::LoadActionLoad);

    MTL::RenderPassDepthAttachmentDescriptor* srcDepthAttachment =
        source_framebuffer.mRenderPassDescriptor->depthAttachment();
    MTL::LoadAction origDepthLoadAction = MTL::LoadActionDontCare;
    if (source_framebuffer.mHasDepthBuffer) {
        origDepthLoadAction = srcDepthAttachment->loadAction();
        srcDepthAttachment->setLoadAction(MTL::LoadActionLoad);
    }

    // Create a new render encoder back onto the framebuffer
    source_framebuffer.mCommandEncoder =
        source_framebuffer.mCommandBuffer->renderCommandEncoder(source_framebuffer.mRenderPassDescriptor);

    std::string fbce_label = fmt::format("FrameBuffer {} Command Encoder After Copy", fb_src_id);
    source_framebuffer.mCommandEncoder->setLabel(NS::String::string(fbce_label.c_str(), NS::UTF8StringEncoding));
    source_framebuffer.mCommandEncoder->setDepthClipMode(MTL::DepthClipModeClamp);
    source_framebuffer.mCommandEncoder->setViewport(*source_framebuffer.mViewport);
    source_framebuffer.mCommandEncoder->setScissorRect(*source_framebuffer.mScissorRect);

    // Now that the command encoder is started, we set the original load actions back for the next frame's use
    srcColorAttachment->setLoadAction(origLoadAction);
    if (source_framebuffer.mHasDepthBuffer) {
        srcDepthAttachment->setLoadAction(origDepthLoadAction);
    }

    // Reset the framebuffer so the encoder is setup again when rendering triangles
    source_framebuffer.mHasBoundVertexShader = false;
    source_framebuffer.mHasBoundFragShader = false;
    source_framebuffer.mLastShaderProgram = nullptr;
    for (int i = 0; i < SHADER_MAX_TEXTURES; i++) {
        source_framebuffer.mLastBoundTextures[i] = nullptr;
        source_framebuffer.mLastBoundSamplers[i] = nullptr;
    }
    source_framebuffer.mLastDepthTest = -1;
    source_framebuffer.mLastDepthMask = -1;
    source_framebuffer.mLastZmodeDecal = -1;
}

void GfxRenderingAPIMetal::GfxRenderingAPIMetal::ReadFramebufferToCPU(int fb_id, uint32_t width, uint32_t height,
                                                                      uint16_t* rgba16_buf) {
    if (fb_id >= (int)mFramebuffers.size()) {
        return;
    }

    FramebufferMetal& framebuffer = mFramebuffers[fb_id];
    MTL::Texture* texture = mTextures[framebuffer.mTextureId].texture;

    MTL::Buffer* output_buffer =
        mDevice->newBuffer(sizeof(uint16_t) * width * height, MTL::ResourceOptionCPUCacheModeDefault);
    output_buffer->setLabel(NS::String::string("Pixels output buffer", NS::UTF8StringEncoding));

    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();

    auto command_buffer = mCommandQueue->commandBuffer();
    command_buffer->setLabel(NS::String::string("Read Pixels Shader Command Buffer", NS::UTF8StringEncoding));

    NS::Error* error = nullptr;
    MTL::ComputePipelineState* compute_pipeline_state =
        mDevice->newComputePipelineState(mConvertToRgb5a1Function, &error);

    // Use a compute encoder to convert the pixel data to rgba16 and transfer to a cpu readable buffer
    MTL::ComputeCommandEncoder* compute_encoder = command_buffer->computeCommandEncoder();
    compute_encoder->setComputePipelineState(compute_pipeline_state);
    compute_encoder->setTexture(texture, 0);
    compute_encoder->setBuffer(output_buffer, 0, 0);

    // Use a thread group size and count that covers the whole copy area
    MTL::Size thread_group_size = MTL::Size::Make(1, 1, 1);
    MTL::Size thread_group_count = MTL::Size::Make(width, height, 1);

    // We validate if the device supports non-uniform threadgroup sizes
    if (mNonUniformThreadgroupSupported) {
        compute_encoder->dispatchThreads(thread_group_count, thread_group_size);
    } else {
        compute_encoder->dispatchThreadgroups(thread_group_count, thread_group_size);
    }
    compute_encoder->endEncoding();

    // Use a completion handler to wait for the GPU to be done without blocking the thread
    command_buffer->addCompletedHandler([=](MTL::CommandBuffer* cmd_buffer) {
        // Now the converted pixel values can be copied from the buffer
        uint16_t* values = (uint16_t*)output_buffer->contents();
        memcpy(rgba16_buf, values, sizeof(uint16_t) * width * height);

        output_buffer->release();
    });

    command_buffer->commit();
    // ReadFramebufferToCPU is contract-synchronous (its OpenGL and D3D11
    // peers block via glReadPixels / Map). Without this wait the caller
    // returns to a still-zeroed rgba16_buf because the completion handler
    // hasn't fired yet -- breaks SSB64's wallpaper FB-capture (issue #57)
    // and the OTR_G_READFB GBI handler.
    command_buffer->waitUntilCompleted();

    compute_pipeline_state->release();
    autorelease_pool->release();
}

void GfxRenderingAPIMetal::SetTextureFilter(FilteringMode mode) {
    mCurrentFilterMode = mode;
    gfx_texture_cache_clear();
}

FilteringMode GfxRenderingAPIMetal::GetTextureFilter() {
    return mCurrentFilterMode;
}

ImTextureID GfxRenderingAPIMetal::GetTextureById(int fb_id) {
    return (void*)mTextures[fb_id].texture;
}

void GfxRenderingAPIMetal::SetSrgbMode() {
    mSrgbMode = true;
}

// --- Post-process / user-shader pipeline ----------------------------------
//
// Implemented from the plan in docs/crt_shader_plan_2026-05-11.md §7.1.
// No code copied from RetroArch or any GPL-licensed shader runtime.
//
// Ordering note: Metal command buffers execute in enqueue() order, not
// commit() order. By the time ComposeFinalFrame runs, the source FB's cb
// (mGameFb) was enqueued during game rendering, then StartDrawToFramebuffer(0)
// enqueued FB 0's cb for the GUI pass. A freshly-allocated cb here would
// land *after* FB 0's, so the GUI would sample a stale destination texture.
// We chain the post-process render encoder onto an already-enqueued
// non-screen cb (preferably the source FB's) so its work executes before
// FB 0 reads our output, without needing fences or events.

bool GfxRenderingAPIMetal::SupportsPostProcess() {
    return true;
}

int GfxRenderingAPIMetal::CreatePostProcessProgram(const PostProcessSource& src) {
    if (src.msl.empty()) {
        SPDLOG_ERROR("Post-process shader '{}' has no MSL source. The .glsl "
                     "should have been transpiled by PostProcessTranspiler "
                     "at load time — check earlier log for the parse error.",
                     src.name);
        return -1;
    }

    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
    NS::Error* error = nullptr;
    MTL::Library* library =
        mDevice->newLibrary(NS::String::string(src.msl.c_str(), NS::UTF8StringEncoding), nullptr, &error);
    if (library == nullptr) {
        SPDLOG_ERROR("Post-process '{}': MSL compile failed: {}", src.name,
                     error ? error->localizedDescription()->cString(NS::UTF8StringEncoding) : "unknown");
        pool->release();
        return -1;
    }

    MTL::Function* vsFn = library->newFunction(NS::String::string("postprocess_vertex", NS::UTF8StringEncoding));
    MTL::Function* fsFn = library->newFunction(NS::String::string("postprocess_fragment", NS::UTF8StringEncoding));
    if (vsFn == nullptr || fsFn == nullptr) {
        SPDLOG_ERROR("Post-process '{}': MSL must export postprocess_vertex / postprocess_fragment", src.name);
        if (vsFn) {
            vsFn->release();
        }
        if (fsFn) {
            fsFn->release();
        }
        library->release();
        pool->release();
        return -1;
    }

    // Build the Default-format pipeline up front. Other format variants
    // (sRGB / Float16) are compiled lazily in RunPostProcess when a pass
    // first writes into an FBO with that format.
    MTL::RenderPipelineDescriptor* desc = MTL::RenderPipelineDescriptor::alloc()->init();
    desc->setVertexFunction(vsFn);
    desc->setFragmentFunction(fsFn);
    desc->colorAttachments()->object(0)->setPixelFormat(
        mSrgbMode ? MTL::PixelFormatBGRA8Unorm_sRGB : MTL::PixelFormatBGRA8Unorm);
    desc->colorAttachments()->object(0)->setBlendingEnabled(false);
    desc->colorAttachments()->object(0)->setWriteMask(MTL::ColorWriteMaskAll);
    desc->setSampleCount(1);
    // Destination FB has no depth attachment (PostProcessChain::OnResize
    // passes has_depth_buffer=false), so depth/stencil pixel formats stay
    // at the default of Invalid.

    MTL::RenderPipelineState* pipeline = mDevice->newRenderPipelineState(desc, &error);
    desc->release();

    if (pipeline == nullptr) {
        SPDLOG_ERROR("Post-process '{}': pipeline state failed: {}", src.name,
                     error ? error->localizedDescription()->cString(NS::UTF8StringEncoding) : "unknown");
        vsFn->release();
        fsFn->release();
        library->release();
        pool->release();
        return -1;
    }

    PostProcessProgramMetal slot{};
    slot.library = library;
    slot.vsFn = vsFn;
    slot.fsFn = fsFn;
    slot.pipelineVariants[(int)PostProcessFboFormat::Default] = pipeline;
    slot.pipelineVariants[(int)PostProcessFboFormat::Srgb] = nullptr;
    slot.pipelineVariants[(int)PostProcessFboFormat::Float16] = nullptr;
    slot.name = src.name;

    for (size_t i = 0; i < mPostProcessPrograms.size(); ++i) {
        // Slot is free if it has no Default-format pipeline (the one we
        // always build up front).
        if (mPostProcessPrograms[i].pipelineVariants[(int)PostProcessFboFormat::Default] == nullptr) {
            mPostProcessPrograms[i] = std::move(slot);
            pool->release();
            return (int)i;
        }
    }
    mPostProcessPrograms.push_back(std::move(slot));
    pool->release();
    return (int)mPostProcessPrograms.size() - 1;
}

GfxRenderingAPIMetal::~GfxRenderingAPIMetal() {
    // The post-process sampler cache is keyed on (filter, wrap) — bounded
    // to 8 entries by design — but `newSamplerState` returns a retained
    // MTL::SamplerState that the backend owns. Release each on shutdown
    // so the singleton teardown doesn't leak the cache.
    //
    // MTL ref-counts survive any in-flight GPU work because command
    // buffers retain the samplers they bound, so this is safe even if a
    // frame is still on the GPU when the backend is destroyed.
    for (auto& kv : mPostProcessSamplers) {
        if (kv.second != nullptr) {
            kv.second->release();
        }
    }
    mPostProcessSamplers.clear();
    // Same teardown contract for the static-texture cache — the chain
    // releases known live entries via DestroyPostProcessStaticTexture,
    // but a process-exit teardown that bypasses the chain would leak.
    for (auto*& tex : mPostProcessStaticTextures) {
        if (tex != nullptr) {
            tex->release();
            tex = nullptr;
        }
    }
    mPostProcessStaticTextures.clear();
    // Phase 3D-3: release any leftover slang programs the chain didn't
    // explicitly destroy, plus the shared vertex buffer.
    for (auto& slot : mPostProcessSlangPrograms) {
        for (auto*& variant : slot.pipelineVariants) {
            if (variant != nullptr) {
                variant->release();
                variant = nullptr;
            }
        }
        if (slot.vsFn != nullptr) {
            slot.vsFn->release();
            slot.vsFn = nullptr;
        }
        if (slot.fsFn != nullptr) {
            slot.fsFn->release();
            slot.fsFn = nullptr;
        }
        if (slot.vsLibrary != nullptr) {
            slot.vsLibrary->release();
            slot.vsLibrary = nullptr;
        }
        if (slot.fsLibrary != nullptr) {
            slot.fsLibrary->release();
            slot.fsLibrary = nullptr;
        }
        if (slot.ubo != nullptr) {
            slot.ubo->release();
            slot.ubo = nullptr;
        }
    }
    mPostProcessSlangPrograms.clear();
    if (mPostProcessSlangVbo != nullptr) {
        mPostProcessSlangVbo->release();
        mPostProcessSlangVbo = nullptr;
    }
}

void GfxRenderingAPIMetal::DestroyPostProcessProgram(int progId) {
    if (progId < 0 || (size_t)progId >= mPostProcessPrograms.size()) {
        return;
    }
    auto& slot = mPostProcessPrograms[progId];
    for (auto*& variant : slot.pipelineVariants) {
        if (variant != nullptr) {
            variant->release();
            variant = nullptr;
        }
    }
    if (slot.vsFn != nullptr) {
        slot.vsFn->release();
        slot.vsFn = nullptr;
    }
    if (slot.fsFn != nullptr) {
        slot.fsFn->release();
        slot.fsFn = nullptr;
    }
    if (slot.library != nullptr) {
        slot.library->release();
        slot.library = nullptr;
    }
    slot = PostProcessProgramMetal{};
}

// Phase 3D-3: build a fullscreen-triangle vertex descriptor for slang
// pipelines. Attribute layout matches the slang VBO populated lazily
// on first slang Run: vec4 Position at offset 0, vec2 TexCoord at
// offset 16, stride 24. Bound at high vertex-buffer index
// (kPostProcessSlangVertexBufferIndex) so SPIRV-Cross's `[[buffer(0)]]`
// UBO declaration has its own slot.
static MTL::VertexDescriptor* MakeSlangVertexDescriptor(uint32_t bufferIndex) {
    MTL::VertexDescriptor* vd = MTL::VertexDescriptor::alloc()->init();
    vd->attributes()->object(0)->setFormat(MTL::VertexFormatFloat4);
    vd->attributes()->object(0)->setOffset(0);
    vd->attributes()->object(0)->setBufferIndex(bufferIndex);
    vd->attributes()->object(1)->setFormat(MTL::VertexFormatFloat2);
    vd->attributes()->object(1)->setOffset(16);
    vd->attributes()->object(1)->setBufferIndex(bufferIndex);
    vd->layouts()->object(bufferIndex)->setStride(24);
    vd->layouts()->object(bufferIndex)->setStepRate(1);
    vd->layouts()->object(bufferIndex)->setStepFunction(MTL::VertexStepFunctionPerVertex);
    return vd;
}

int GfxRenderingAPIMetal::CreatePostProcessSlangProgram(const PostProcessSlangProgramSource& src) {
    if (src.vsMsl.empty() || src.fsMsl.empty()) {
        SPDLOG_ERROR("Slang post-process '{}': missing VS or FS MSL", src.name);
        return -1;
    }
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
    NS::Error* error = nullptr;

    // Slang VS + FS live in separate sources (each was emitted by
    // SPIRV-Cross from its own SPIR-V module). Compile each into its
    // own MTL::Library and pull the entry-point function out.
    auto compileSource = [&](const std::string& msl, const char* fnName,
                             MTL::Library*& outLib, MTL::Function*& outFn) -> bool {
        MTL::Library* lib = mDevice->newLibrary(
            NS::String::string(msl.c_str(), NS::UTF8StringEncoding), nullptr, &error);
        if (lib == nullptr) {
            SPDLOG_ERROR("Slang post-process '{}': MSL compile failed ({}): {}", src.name, fnName,
                         error ? error->localizedDescription()->cString(NS::UTF8StringEncoding) : "unknown");
            return false;
        }
        MTL::Function* fn = lib->newFunction(NS::String::string(fnName, NS::UTF8StringEncoding));
        if (fn == nullptr) {
            SPDLOG_ERROR("Slang post-process '{}': MSL missing entry '{}'", src.name, fnName);
            lib->release();
            return false;
        }
        outLib = lib;
        outFn = fn;
        return true;
    };

    PostProcessSlangProgramMetal slot{};
    // Both libraries are retained for the slot's lifetime.
    if (!compileSource(src.vsMsl, "postprocess_vertex", slot.vsLibrary, slot.vsFn)) {
        pool->release();
        return -1;
    }
    if (!compileSource(src.fsMsl, "postprocess_fragment", slot.fsLibrary, slot.fsFn)) {
        slot.vsFn->release();
        slot.vsLibrary->release();
        pool->release();
        return -1;
    }

    MTL::VertexDescriptor* vd = MakeSlangVertexDescriptor(kPostProcessSlangVertexBufferIndex);

    MTL::RenderPipelineDescriptor* desc = MTL::RenderPipelineDescriptor::alloc()->init();
    desc->setVertexFunction(slot.vsFn);
    desc->setFragmentFunction(slot.fsFn);
    desc->setVertexDescriptor(vd);
    desc->colorAttachments()->object(0)->setPixelFormat(
        mSrgbMode ? MTL::PixelFormatBGRA8Unorm_sRGB : MTL::PixelFormatBGRA8Unorm);
    desc->colorAttachments()->object(0)->setBlendingEnabled(false);
    desc->colorAttachments()->object(0)->setWriteMask(MTL::ColorWriteMaskAll);
    desc->setSampleCount(1);

    MTL::RenderPipelineState* pipeline = mDevice->newRenderPipelineState(desc, &error);
    desc->release();
    vd->release();

    if (pipeline == nullptr) {
        SPDLOG_ERROR("Slang post-process '{}': pipeline state failed: {}", src.name,
                     error ? error->localizedDescription()->cString(NS::UTF8StringEncoding) : "unknown");
        slot.vsFn->release();
        slot.fsFn->release();
        slot.vsLibrary->release();
        slot.fsLibrary->release();
        pool->release();
        return -1;
    }
    slot.pipelineVariants[(int)PostProcessFboFormat::Default] = pipeline;
    slot.name = src.name;
    slot.samplerNames = src.samplerNames;

    if (src.uboBytes > 0) {
        const NS::UInteger uboBytes = (src.uboBytes + 15) & ~15u;
        slot.ubo = mDevice->newBuffer(uboBytes, MTL::ResourceStorageModeShared);
        slot.uboBytes = static_cast<uint32_t>(uboBytes);
    }

    int handle = -1;
    for (size_t i = 0; i < mPostProcessSlangPrograms.size(); ++i) {
        if (mPostProcessSlangPrograms[i].pipelineVariants[(int)PostProcessFboFormat::Default] == nullptr) {
            mPostProcessSlangPrograms[i] = std::move(slot);
            handle = (int)i;
            break;
        }
    }
    if (handle < 0) {
        mPostProcessSlangPrograms.push_back(std::move(slot));
        handle = (int)mPostProcessSlangPrograms.size() - 1;
    }
    pool->release();
    return handle;
}

void GfxRenderingAPIMetal::DestroyPostProcessSlangProgram(int progId) {
    if (progId < 0 || (size_t)progId >= mPostProcessSlangPrograms.size()) {
        return;
    }
    auto& slot = mPostProcessSlangPrograms[progId];
    for (auto*& variant : slot.pipelineVariants) {
        if (variant != nullptr) {
            variant->release();
            variant = nullptr;
        }
    }
    if (slot.vsFn != nullptr) {
        slot.vsFn->release();
        slot.vsFn = nullptr;
    }
    if (slot.fsFn != nullptr) {
        slot.fsFn->release();
        slot.fsFn = nullptr;
    }
    if (slot.vsLibrary != nullptr) {
        slot.vsLibrary->release();
        slot.vsLibrary = nullptr;
    }
    if (slot.fsLibrary != nullptr) {
        slot.fsLibrary->release();
        slot.fsLibrary = nullptr;
    }
    if (slot.ubo != nullptr) {
        slot.ubo->release();
        slot.ubo = nullptr;
    }
    slot = PostProcessSlangProgramMetal{};
}

int GfxRenderingAPIMetal::CreatePostProcessStaticTexture(uint32_t width, uint32_t height,
                                                         const uint8_t* rgba8) {
    if (width == 0 || height == 0 || rgba8 == nullptr) {
        return -1;
    }
    MTL::TextureDescriptor* desc = MTL::TextureDescriptor::alloc()->init();
    desc->setTextureType(MTL::TextureType2D);
    desc->setWidth(width);
    desc->setHeight(height);
    desc->setSampleCount(1);
    desc->setMipmapLevelCount(1);
    desc->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
    desc->setUsage(MTL::TextureUsageShaderRead);
    MTL::Texture* tex = mDevice->newTexture(desc);
    desc->release();
    if (tex == nullptr) {
        return -1;
    }
    MTL::Region region = MTL::Region::Make2D(0, 0, width, height);
    tex->replaceRegion(region, 0, rgba8, width * 4u);

    for (size_t i = 0; i < mPostProcessStaticTextures.size(); ++i) {
        if (mPostProcessStaticTextures[i] == nullptr) {
            mPostProcessStaticTextures[i] = tex;
            return (int)i;
        }
    }
    mPostProcessStaticTextures.push_back(tex);
    return (int)(mPostProcessStaticTextures.size() - 1);
}

void GfxRenderingAPIMetal::DestroyPostProcessStaticTexture(int textureId) {
    if (textureId < 0 || (size_t)textureId >= mPostProcessStaticTextures.size()) {
        return;
    }
    MTL::Texture*& slot = mPostProcessStaticTextures[textureId];
    if (slot != nullptr) {
        slot->release();
        slot = nullptr;
    }
}

void GfxRenderingAPIMetal::SetPostProcessFramebufferMipmapped(int fb_id, bool mipmapped) {
    if (fb_id < 0 || (size_t)fb_id >= mFramebuffers.size()) {
        return;
    }
    mFramebuffers[fb_id].mPostProcessMipmapped = mipmapped;
}

void GfxRenderingAPIMetal::GeneratePostProcessMipmaps(int fb_id) {
    if (fb_id < 0 || (size_t)fb_id >= mFramebuffers.size()) {
        return;
    }
    FramebufferMetal& fb = mFramebuffers[fb_id];
    if (!fb.mPostProcessMipmapped || fb.mTextureId == UINT32_MAX) {
        return;
    }
    MTL::Texture* tex = mTextures[fb.mTextureId].texture;
    if (tex == nullptr || tex->mipmapLevelCount() <= 1) {
        return;
    }
    // Ensure any pending host encoder on this FB has finished — the
    // blit pass writes into the texture's mip 1..N and races a still-
    // live render-target encoder writing mip 0. Each pass's RunPost-
    // Process is invoked on a host command buffer that already has
    // its prior encoder ended (the chain runs sequentially), so this
    // is mostly defensive.
    if (!fb.mHasEndedEncoding && fb.mCommandEncoder != nullptr) {
        fb.mCommandEncoder->endEncoding();
        fb.mHasEndedEncoding = true;
    }
    MTL::CommandBuffer* cb = (fb.mCommandBuffer != nullptr) ? fb.mCommandBuffer
                                                            : mCommandQueue->commandBuffer();
    MTL::BlitCommandEncoder* enc = cb->blitCommandEncoder();
    enc->generateMipmaps(tex);
    enc->endEncoding();
    if (fb.mCommandBuffer == nullptr) {
        // We created a one-shot cb because no host cb was attached
        // (rare; only on the very first pass when the chain runs
        // before any draw). Commit it so the mip data is live before
        // the next encoder samples the texture.
        cb->commit();
    }
}

void GfxRenderingAPIMetal::SetPostProcessFramebufferFormat(int fb_id, PostProcessFboFormat fmt) {
    if (fb_id < 0 || (size_t)fb_id >= mFramebuffers.size()) {
        return;
    }
    // Record the desired format; UpdateFramebufferParameters notices the
    // mismatch against mLastPostProcessFormat and reallocates the
    // underlying MTL::Texture with the new pixel format.
    mFramebuffers[fb_id].mPostProcessFormat = fmt;
}

// Phase 3D-3: dispatch a compiled slang program. Mirrors the legacy
// RunPostProcess but routes vertex data through a bound MTL::Buffer
// (not [[vertex_id]]) and uploads a chain-built UBO blob to buffer(0)
// on both stages instead of the fixed PostProcessUniformsMetal struct.
void GfxRenderingAPIMetal::RunPostProcessSlang(int progId, int dstFb,
                                                const uint8_t* uboData, uint32_t uboBytes,
                                                const int* samplerFbIds, uint32_t samplerCount,
                                                const PostProcessParams& params) {
    if (progId < 0 || (size_t)progId >= mPostProcessSlangPrograms.size()) {
        return;
    }
    PostProcessSlangProgramMetal& slot = mPostProcessSlangPrograms[progId];
    if (slot.pipelineVariants[(int)PostProcessFboFormat::Default] == nullptr) {
        return;
    }
    if (dstFb < 0 || (size_t)dstFb >= mFramebuffers.size()) {
        return;
    }
    FramebufferMetal& dst = mFramebuffers[dstFb];
    if (dst.mRenderPassDescriptor == nullptr) {
        return;
    }
    MTL::Texture* dstTexture =
        (dst.mTextureId != UINT32_MAX) ? mTextures[dst.mTextureId].texture : nullptr;
    if (dstTexture == nullptr) {
        return;
    }

    // Re-use the legacy host-cb hunt to find a non-screen command
    // buffer to append our encoder to. The first sampler entry's
    // source FBO is the most likely owner; otherwise scan drawn FBs.
    MTL::CommandBuffer* hostCb = nullptr;
    FramebufferMetal* hostFb = nullptr;
    int seedFb = (samplerCount > 0 && samplerFbIds != nullptr) ? samplerFbIds[0] : -1;
    if (seedFb >= 0 && (size_t)seedFb < mFramebuffers.size()) {
        FramebufferMetal& candidate = mFramebuffers[seedFb];
        if (candidate.mCommandBuffer != nullptr) {
            hostCb = candidate.mCommandBuffer;
            hostFb = &candidate;
        }
    }
    if (hostCb == nullptr) {
        for (int id : mDrawnFramebuffers) {
            if (id == 0 || id == dstFb) {
                continue;
            }
            if ((size_t)id >= mFramebuffers.size()) {
                continue;
            }
            FramebufferMetal& candidate = mFramebuffers[id];
            if (candidate.mCommandBuffer != nullptr) {
                hostCb = candidate.mCommandBuffer;
                hostFb = &candidate;
                break;
            }
        }
    }
    if (hostCb == nullptr || hostFb == nullptr) {
        SPDLOG_WARN("Slang post-process: no live non-screen command buffer; skipping pass for '{}'",
                    slot.name);
        return;
    }
    if (!hostFb->mHasEndedEncoding && hostFb->mCommandEncoder != nullptr) {
        hostFb->mCommandEncoder->endEncoding();
        hostFb->mHasEndedEncoding = true;
    }

    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    // Build the shared slang VBO once. 3 vertices of {vec4 Position,
    // vec2 TexCoord} in clip space + [0,2] UVs.
    if (mPostProcessSlangVbo == nullptr) {
        static const float kSlangVerts[] = {
            -1.0f, -1.0f, 0.0f, 1.0f,  0.0f, 0.0f,
             3.0f, -1.0f, 0.0f, 1.0f,  2.0f, 0.0f,
            -1.0f,  3.0f, 0.0f, 1.0f,  0.0f, 2.0f,
        };
        mPostProcessSlangVbo = mDevice->newBuffer(kSlangVerts, sizeof(kSlangVerts),
                                                   MTL::ResourceStorageModeShared);
    }

    MTL::RenderPassColorAttachmentDescriptor* color = dst.mRenderPassDescriptor->colorAttachments()->object(0);
    MTL::LoadAction origLoad = color->loadAction();
    color->setLoadAction(MTL::LoadActionDontCare);

    MTL::RenderCommandEncoder* enc = hostCb->renderCommandEncoder(dst.mRenderPassDescriptor);
    enc->setLabel(NS::String::string("Slang post-process pass", NS::UTF8StringEncoding));

    MTL::Viewport vp = { 0.0, 0.0, (double)dstTexture->width(), (double)dstTexture->height(), 0.0, 1.0 };
    enc->setViewport(vp);

    // Lazy-build the pipeline variant for the destination's color
    // format, mirroring the legacy path.
    const PostProcessFboFormat dstFmt = dst.mPostProcessFormat;
    MTL::RenderPipelineState* variant = slot.pipelineVariants[(int)dstFmt];
    if (variant == nullptr) {
        MTL::PixelFormat mtlFmt = MTL::PixelFormatBGRA8Unorm;
        switch (dstFmt) {
            case PostProcessFboFormat::Default:
                mtlFmt = mSrgbMode ? MTL::PixelFormatBGRA8Unorm_sRGB : MTL::PixelFormatBGRA8Unorm;
                break;
            case PostProcessFboFormat::Srgb:
                mtlFmt = MTL::PixelFormatBGRA8Unorm_sRGB;
                break;
            case PostProcessFboFormat::Float16:
                mtlFmt = MTL::PixelFormatRGBA16Float;
                break;
        }
        MTL::VertexDescriptor* vd = MakeSlangVertexDescriptor(kPostProcessSlangVertexBufferIndex);
        MTL::RenderPipelineDescriptor* desc = MTL::RenderPipelineDescriptor::alloc()->init();
        desc->setVertexFunction(slot.vsFn);
        desc->setFragmentFunction(slot.fsFn);
        desc->setVertexDescriptor(vd);
        desc->colorAttachments()->object(0)->setPixelFormat(mtlFmt);
        desc->colorAttachments()->object(0)->setBlendingEnabled(false);
        desc->colorAttachments()->object(0)->setWriteMask(MTL::ColorWriteMaskAll);
        desc->setSampleCount(1);
        NS::Error* error = nullptr;
        variant = mDevice->newRenderPipelineState(desc, &error);
        desc->release();
        vd->release();
        if (variant == nullptr) {
            SPDLOG_ERROR("Slang post-process '{}': pipeline variant for format {} failed: {}",
                         slot.name, (int)dstFmt,
                         error ? error->localizedDescription()->cString(NS::UTF8StringEncoding) : "unknown");
            variant = slot.pipelineVariants[(int)PostProcessFboFormat::Default];
        } else {
            slot.pipelineVariants[(int)dstFmt] = variant;
        }
    }
    enc->setRenderPipelineState(variant);

    // Sampler binding plan: a single sampler reused at every slot
    // for Phase 3D-3 scope. Per-slot libretro filter/wrap routing
    // arrives with multipass slang chains in later phases.
    auto wrapModeToMetal = [](PostProcessWrapMode m) -> MTL::SamplerAddressMode {
        switch (m) {
            case PostProcessWrapMode::ClampToEdge:    return MTL::SamplerAddressModeClampToEdge;
            case PostProcessWrapMode::ClampToBorder:  return MTL::SamplerAddressModeClampToBorderColor;
            case PostProcessWrapMode::Repeat:         return MTL::SamplerAddressModeRepeat;
            case PostProcessWrapMode::MirroredRepeat: return MTL::SamplerAddressModeMirrorRepeat;
        }
        return MTL::SamplerAddressModeClampToEdge;
    };
    auto getSampler = [&](bool linear, PostProcessWrapMode wrap) -> MTL::SamplerState* {
        const uint32_t key =
            (static_cast<uint32_t>(wrap) << 1) | (linear ? 1u : 0u);
        auto it = mPostProcessSamplers.find(key);
        if (it != mPostProcessSamplers.end()) {
            return it->second;
        }
        MTL::SamplerDescriptor* sd = MTL::SamplerDescriptor::alloc()->init();
        const MTL::SamplerMinMagFilter f =
            linear ? MTL::SamplerMinMagFilterLinear : MTL::SamplerMinMagFilterNearest;
        sd->setMinFilter(f);
        sd->setMagFilter(f);
        sd->setMipFilter(MTL::SamplerMipFilterNotMipmapped);
        const MTL::SamplerAddressMode addr = wrapModeToMetal(wrap);
        sd->setSAddressMode(addr);
        sd->setTAddressMode(addr);
        if (wrap == PostProcessWrapMode::ClampToBorder) {
            sd->setBorderColor(MTL::SamplerBorderColorTransparentBlack);
        }
        MTL::SamplerState* sampler = mDevice->newSamplerState(sd);
        sd->release();
        mPostProcessSamplers.emplace(key, sampler);
        return sampler;
    };
    MTL::SamplerState* sampler = getSampler(params.srcFilterLinear, params.srcWrapMode);

    // Bind samplerFbIds[i]'s texture at slot i. Fallback to the
    // first valid texture if any entry is -1 so the shader's
    // sample() calls don't read undefined memory.
    MTL::Texture* fallbackTex = nullptr;
    for (uint32_t i = 0; i < samplerCount; ++i) {
        const int fbId = samplerFbIds ? samplerFbIds[i] : -1;
        if (fbId >= 0 && (size_t)fbId < mFramebuffers.size()) {
            const FramebufferMetal& f = mFramebuffers[fbId];
            if (f.mTextureId != UINT32_MAX) {
                MTL::Texture* t = mTextures[f.mTextureId].texture;
                if (t != nullptr) {
                    fallbackTex = t;
                    break;
                }
            }
        }
    }
    for (uint32_t i = 0; i < samplerCount; ++i) {
        const int fbId = samplerFbIds ? samplerFbIds[i] : -1;
        MTL::Texture* tex = fallbackTex;
        if (fbId >= 0 && (size_t)fbId < mFramebuffers.size()) {
            const FramebufferMetal& f = mFramebuffers[fbId];
            if (f.mTextureId != UINT32_MAX) {
                MTL::Texture* t = mTextures[f.mTextureId].texture;
                if (t != nullptr) {
                    tex = t;
                }
            }
        }
        if (tex != nullptr) {
            enc->setFragmentTexture(tex, (NS::UInteger)i);
            enc->setFragmentSamplerState(sampler, (NS::UInteger)i);
        }
    }

    // UBO upload — bind to buffer(0) on both stages so the
    // SPIRV-Cross-emitted `constant UBO& global [[buffer(0)]]`
    // declaration finds it. Use setVertexBytes / setFragmentBytes
    // (inline) for small blobs; spill to a persistent buffer for
    // larger ones. Slang shaders rarely exceed 4KB so the inline
    // path covers almost everything.
    if (uboData != nullptr && uboBytes > 0) {
        constexpr uint32_t kSetBytesThreshold = 4096;
        if (uboBytes <= kSetBytesThreshold) {
            enc->setVertexBytes(uboData, uboBytes, 0);
            enc->setFragmentBytes(uboData, uboBytes, 0);
        } else if (slot.ubo != nullptr) {
            // Refill the per-program persistent buffer.
            std::memcpy(slot.ubo->contents(), uboData,
                        std::min<uint32_t>(uboBytes, slot.uboBytes));
            slot.ubo->didModifyRange(NS::Range::Make(0, slot.uboBytes));
            enc->setVertexBuffer(slot.ubo, 0, 0);
            enc->setFragmentBuffer(slot.ubo, 0, 0);
        }
    }

    // Bind the slang vertex buffer at the high index the descriptor
    // routes attributes through.
    enc->setVertexBuffer(mPostProcessSlangVbo, 0, kPostProcessSlangVertexBufferIndex);

    enc->drawPrimitives(MTL::PrimitiveTypeTriangle, (NS::UInteger)0, (NS::UInteger)3);
    enc->endEncoding();

    color->setLoadAction(origLoad);

    pool->release();
}

void GfxRenderingAPIMetal::RunPostProcess(int progId, int srcFb, int dstFb, int originalFb,
                                          const PostProcessParams& params) {
    if (progId < 0 || (size_t)progId >= mPostProcessPrograms.size()) {
        return;
    }
    PostProcessProgramMetal& slot = mPostProcessPrograms[progId];
    if (slot.pipelineVariants[(int)PostProcessFboFormat::Default] == nullptr) {
        return;
    }
    if (srcFb < 0 || (size_t)srcFb >= mFramebuffers.size()) {
        return;
    }
    if (dstFb < 0 || (size_t)dstFb >= mFramebuffers.size()) {
        return;
    }
    FramebufferMetal& src = mFramebuffers[srcFb];
    FramebufferMetal& dst = mFramebuffers[dstFb];
    if (dst.mRenderPassDescriptor == nullptr) {
        return;
    }
    MTL::Texture* srcTexture = mTextures[src.mTextureId].texture;
    MTL::Texture* dstTexture = mTextures[dst.mTextureId].texture;
    if (srcTexture == nullptr || dstTexture == nullptr) {
        return;
    }

    // Find a host command buffer to append our render encoder to. The cb
    // must already be enqueued ahead of FB 0's GUI cb to satisfy the
    // GPU-side read-after-write ordering described above.
    MTL::CommandBuffer* hostCb = nullptr;
    FramebufferMetal* hostFb = nullptr;
    if (src.mCommandBuffer != nullptr) {
        hostCb = src.mCommandBuffer;
        hostFb = &src;
    } else {
        for (int id : mDrawnFramebuffers) {
            if (id == 0 || id == dstFb) {
                continue;
            }
            if ((size_t)id >= mFramebuffers.size()) {
                continue;
            }
            FramebufferMetal& candidate = mFramebuffers[id];
            if (candidate.mCommandBuffer != nullptr) {
                hostCb = candidate.mCommandBuffer;
                hostFb = &candidate;
                break;
            }
        }
    }
    if (hostCb == nullptr || hostFb == nullptr) {
        SPDLOG_WARN("Post-process: no live non-screen command buffer; skipping pass for '{}'", slot.name);
        return;
    }
    if (!hostFb->mHasEndedEncoding && hostFb->mCommandEncoder != nullptr) {
        hostFb->mCommandEncoder->endEncoding();
        hostFb->mHasEndedEncoding = true;
    }

    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    // Temporarily switch the destination's load action to DontCare — the
    // fullscreen triangle writes every pixel, so a load is wasted bandwidth.
    // Restore after the encoder so any later use of the descriptor (resize
    // path) keeps the default Load semantics it was set up with.
    MTL::RenderPassColorAttachmentDescriptor* color = dst.mRenderPassDescriptor->colorAttachments()->object(0);
    MTL::LoadAction origLoad = color->loadAction();
    color->setLoadAction(MTL::LoadActionDontCare);

    MTL::RenderCommandEncoder* enc = hostCb->renderCommandEncoder(dst.mRenderPassDescriptor);
    enc->setLabel(NS::String::string("Post-process pass", NS::UTF8StringEncoding));

    MTL::Viewport vp = { 0.0, 0.0, (double)dstTexture->width(), (double)dstTexture->height(), 0.0, 1.0 };
    enc->setViewport(vp);

    // Metal pins the color-attachment format into the pipeline state, so
    // we need a separate pipeline variant per (program, dst format).
    // Look up / lazily build the variant matching dst's current format.
    const PostProcessFboFormat dstFmt = dst.mPostProcessFormat;
    MTL::RenderPipelineState* variant = slot.pipelineVariants[(int)dstFmt];
    if (variant == nullptr) {
        MTL::PixelFormat mtlFmt = MTL::PixelFormatBGRA8Unorm;
        switch (dstFmt) {
            case PostProcessFboFormat::Default:
                mtlFmt = mSrgbMode ? MTL::PixelFormatBGRA8Unorm_sRGB : MTL::PixelFormatBGRA8Unorm;
                break;
            case PostProcessFboFormat::Srgb:
                // Intermediate FBOs use BGRA8Unorm_sRGB to match the
                // surrounding allocation in UpdateFramebufferParameters.
                mtlFmt = MTL::PixelFormatBGRA8Unorm_sRGB;
                break;
            case PostProcessFboFormat::Float16:
                mtlFmt = MTL::PixelFormatRGBA16Float;
                break;
        }
        MTL::RenderPipelineDescriptor* desc = MTL::RenderPipelineDescriptor::alloc()->init();
        desc->setVertexFunction(slot.vsFn);
        desc->setFragmentFunction(slot.fsFn);
        desc->colorAttachments()->object(0)->setPixelFormat(mtlFmt);
        desc->colorAttachments()->object(0)->setBlendingEnabled(false);
        desc->colorAttachments()->object(0)->setWriteMask(MTL::ColorWriteMaskAll);
        desc->setSampleCount(1);
        NS::Error* error = nullptr;
        variant = mDevice->newRenderPipelineState(desc, &error);
        desc->release();
        if (variant == nullptr) {
            SPDLOG_ERROR("Post-process '{}': pipeline variant for format {} failed: {}",
                         slot.name, (int)dstFmt,
                         error ? error->localizedDescription()->cString(NS::UTF8StringEncoding) : "unknown");
            // Fall back to the Default variant so we at least render
            // something rather than a black frame.
            variant = slot.pipelineVariants[(int)PostProcessFboFormat::Default];
        } else {
            slot.pipelineVariants[(int)dstFmt] = variant;
        }
    }
    enc->setRenderPipelineState(variant);

    // Slot 0 (Source) sampler comes from the producer pass's libretro
    // filter_linearN / wrap_modeN, encoded as a small cache key. Slot 1
    // (Original) always uses the default linear / clamp-to-edge sampler
    // because .glslp has no per-pass override for Original.
    auto wrapModeToMetal = [](PostProcessWrapMode m) -> MTL::SamplerAddressMode {
        switch (m) {
            case PostProcessWrapMode::ClampToEdge:    return MTL::SamplerAddressModeClampToEdge;
            case PostProcessWrapMode::ClampToBorder:  return MTL::SamplerAddressModeClampToBorderColor;
            case PostProcessWrapMode::Repeat:         return MTL::SamplerAddressModeRepeat;
            case PostProcessWrapMode::MirroredRepeat: return MTL::SamplerAddressModeMirrorRepeat;
        }
        return MTL::SamplerAddressModeClampToEdge;
    };
    auto getSampler = [&](bool linear, PostProcessWrapMode wrap, bool mipmap) -> MTL::SamplerState* {
        // Cache key folds the (linear, wrap, mipmap) tuple into a small
        // int. Mipmap occupies a single bit above the existing
        // (wrap << 1 | linear) layout so the prior keys keep working.
        const uint32_t key =
            (mipmap ? (1u << 8) : 0u) |
            (static_cast<uint32_t>(wrap) << 1) | (linear ? 1u : 0u);
        auto it = mPostProcessSamplers.find(key);
        if (it != mPostProcessSamplers.end()) {
            return it->second;
        }
        MTL::SamplerDescriptor* sd = MTL::SamplerDescriptor::alloc()->init();
        const MTL::SamplerMinMagFilter f =
            linear ? MTL::SamplerMinMagFilterLinear : MTL::SamplerMinMagFilterNearest;
        sd->setMinFilter(f);
        sd->setMagFilter(f);
        if (mipmap) {
            sd->setMipFilter(linear ? MTL::SamplerMipFilterLinear
                                    : MTL::SamplerMipFilterNearest);
        } else {
            sd->setMipFilter(MTL::SamplerMipFilterNotMipmapped);
        }
        const MTL::SamplerAddressMode addr = wrapModeToMetal(wrap);
        sd->setSAddressMode(addr);
        sd->setTAddressMode(addr);
        if (wrap == PostProcessWrapMode::ClampToBorder) {
            sd->setBorderColor(MTL::SamplerBorderColorTransparentBlack);
        }
        MTL::SamplerState* sampler = mDevice->newSamplerState(sd);
        sd->release();
        mPostProcessSamplers.emplace(key, sampler);
        return sampler;
    };
    MTL::SamplerState* srcSampler =
        getSampler(params.srcFilterLinear, params.srcWrapMode, params.srcUseMipmap);
    MTL::SamplerState* origSampler = getSampler(true, PostProcessWrapMode::ClampToEdge, false);

    enc->setFragmentTexture(srcTexture, 0);
    enc->setFragmentSamplerState(srcSampler, 0);

    // Original (game FB) on slot 1 for multipass shaders that combine
    // post-bloom Source with the pre-bloom Original. Falls back to the
    // source texture for pass 0 — caller passes srcFb == originalFb.
    MTL::Texture* originalTexture = srcTexture;
    if (originalFb >= 0 && (size_t)originalFb < mFramebuffers.size()) {
        FramebufferMetal& origFbInfo = mFramebuffers[originalFb];
        if (origFbInfo.mTextureId != UINT32_MAX) {
            MTL::Texture* t = mTextures[origFbInfo.mTextureId].texture;
            if (t != nullptr) {
                originalTexture = t;
            }
        }
    }
    enc->setFragmentTexture(originalTexture, 1);
    enc->setFragmentSamplerState(origSampler, 1);

    // Alias / external-texture bindings at slots 2..N+1. Each entry's
    // sampler comes from the per-pass (filter, wrap) cache. sourceFb
    // == -1 (producer pass hasn't run yet in this chain pass index)
    // falls back to the Original texture so the shader's texture()
    // call reads a sensible value rather than the stale slot.
    for (size_t i = 0; i < params.extraBindingsCount; ++i) {
        const auto& eb = params.extraBindings[i];
        MTL::Texture* texToBind = originalTexture; // defensive fallback
        if (eb.staticTextureId >= 0 &&
            (size_t)eb.staticTextureId < mPostProcessStaticTextures.size() &&
            mPostProcessStaticTextures[eb.staticTextureId] != nullptr) {
            texToBind = mPostProcessStaticTextures[eb.staticTextureId];
        } else if (eb.sourceFb >= 0 && (size_t)eb.sourceFb < mFramebuffers.size()) {
            const FramebufferMetal& f = mFramebuffers[eb.sourceFb];
            if (f.mTextureId != UINT32_MAX) {
                MTL::Texture* t = mTextures[f.mTextureId].texture;
                if (t != nullptr) {
                    texToBind = t;
                }
            }
        }
        const NS::UInteger slot = (NS::UInteger)(2 + i);
        enc->setFragmentTexture(texToBind, slot);
        MTL::SamplerState* s = getSampler(eb.filterLinear, eb.wrapMode, false);
        enc->setFragmentSamplerState(s, slot);
    }

    // Pack the per-frame uniforms followed by the per-pass alias /
    // external-texture `vec2 <name>Size` slots. The shader's MSL
    // `PostProcessUniforms` struct (transpiled by SPIRV-Cross from
    // the GLSL UBO block) lays the trailing vec2s at offsets
    // 40, 48, 56, ... matching std140; we push exactly those bytes
    // into Metal's argument buffer via setFragmentBytes. Total size
    // is padded to a multiple of 16 to match the D3D11 cbuffer rule
    // — Metal accepts any size, but the alignment keeps the layout
    // identical across backends.
    constexpr size_t kPrefixBytes = sizeof(PostProcessUniformsMetal);
    static_assert(kPrefixBytes == 40, "PostProcessUniformsMetal must stay 40 bytes");
    constexpr size_t kAliasStrideBytes = sizeof(simd::float2);
    static_assert(kAliasStrideBytes == 8, "simd::float2 must be 8 bytes for the alias UBO tail");
    const size_t aliasBytes = params.extraBindingsCount * kAliasStrideBytes;
    const size_t totalBytes = (kPrefixBytes + aliasBytes + 15) & ~15u;

    std::vector<uint8_t> uboBytes(totalBytes, 0);
    PostProcessUniformsMetal* uni = reinterpret_cast<PostProcessUniformsMetal*>(uboBytes.data());
    uni->sourceSize = simd::float2{ (float)params.srcWidth, (float)params.srcHeight };
    uni->outputSize = simd::float2{ (float)params.dstWidth, (float)params.dstHeight };
    uni->inputSize = simd::float2{ (float)params.inputWidth, (float)params.inputHeight };
    uni->originalSize = simd::float2{ (float)params.originalWidth, (float)params.originalHeight };
    uni->frameCount = (int)params.frameCount;
    uni->frameDirection = 1.0f;
    for (size_t i = 0; i < params.extraBindingsCount; ++i) {
        const auto& eb = params.extraBindings[i];
        simd::float2* slotPtr = reinterpret_cast<simd::float2*>(
            uboBytes.data() + kPrefixBytes + i * kAliasStrideBytes);
        *slotPtr = simd::float2{ (float)eb.width, (float)eb.height };
    }
    enc->setFragmentBytes(uboBytes.data(), totalBytes, 0);

    enc->drawPrimitives(MTL::PrimitiveTypeTriangle, (NS::UInteger)0, (NS::UInteger)3);
    enc->endEncoding();

    color->setLoadAction(origLoad);

    pool->release();
}

} // namespace Fast

bool Metal_IsSupported() {
#ifdef __IOS__
    // iOS always supports Metal and MTLCopyAllDevices is not available
    return true;
#else
    // MTLCopyAllDevices() returns a retained, autoreleased NSArray. Calling
    // it from a C++ context without an NSAutoreleasePool in scope crashes on
    // some macOS versions because the framework assumes there is one to
    // register the return value against. Wrap the probe in an explicit pool
    // so detection is safe no matter who invoked us.
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    NS::Array* devices = MTLCopyAllDevices();
    NS::UInteger count = (devices != nullptr) ? devices->count() : 0;

    if (devices != nullptr) {
        devices->release();
    }

    pool->release();
    return count > 0;
#endif
}

#endif
