#ifdef ENABLE_DX11

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>
#include <cmath>

#include <map>
#include <unordered_map>

#include <windows.h>
#include <versionhelpers.h>
#include <wrl/client.h>

#include <dxgi1_3.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#ifndef _LANGUAGE_C
#define _LANGUAGE_C
#endif

#include "fast/backends/gfx_window_manager_api.h"
#include "fast/backends/gfx_direct3d_common.h"

#define DECLARE_GFX_DXGI_FUNCTIONS
#include "fast/backends/gfx_dxgi.h"

#include "fast/backends/gfx_screen_config.h"
#include "ship/window/gui/Gui.h"
#include "ship/Context.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/window/Window.h"

#include "fast/backends/gfx_rendering_api.h"
#include "fast/interpreter.h"

#include <prism/processor.h>
#include "ship/config/ConsoleVariable.h"
#include <ship/resource/factory/ShaderFactory.h>
#include <ship/resource/ResourceManager.h>
#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"

#define DEBUG_D3D 0

// stb_image_write for backbuffer screenshot capture (portFastCaptureBackbufferPNG).
// Define implementation here so the TU owns the single instantiation.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#include "stb_image_write.h"

using namespace Microsoft::WRL; // For ComPtr

namespace Fast {

GfxRenderingAPIDX11::~GfxRenderingAPIDX11() {
}

// Static pointer to the most recently constructed DX11 API instance.
// Used by portFastCaptureBackbufferPNG so the port can capture the back buffer
// without needing a handle to the renderer. libultraship instantiates exactly
// one GfxRenderingAPIDX11 for the lifetime of the process.
static GfxRenderingAPIDX11* sDX11InstanceForCapture = nullptr;

GfxRenderingAPIDX11::GfxRenderingAPIDX11(GfxWindowBackendDXGI* backend) {
    mWindowBackend = backend;
    sDX11InstanceForCapture = this;
}

void GfxRenderingAPIDX11::CreateDepthStencilObjects(uint32_t width, uint32_t height, uint32_t msaa_count,
                                                    ID3D11DepthStencilView** view, ID3D11ShaderResourceView** srv) {
    D3D11_TEXTURE2D_DESC texture_desc;
    texture_desc.Width = width;
    texture_desc.Height = height;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format =
        mFeatureLevel >= D3D_FEATURE_LEVEL_10_0 ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_R24G8_TYPELESS;
    texture_desc.SampleDesc.Count = msaa_count;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | (srv != nullptr ? D3D11_BIND_SHADER_RESOURCE : 0);
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    ComPtr<ID3D11Texture2D> texture;
    ThrowIfFailed(mDevice->CreateTexture2D(&texture_desc, nullptr, texture.GetAddressOf()));

    D3D11_DEPTH_STENCIL_VIEW_DESC view_desc;
    view_desc.Format = mFeatureLevel >= D3D_FEATURE_LEVEL_10_0 ? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_D24_UNORM_S8_UINT;
    view_desc.Flags = 0;
    if (msaa_count > 1) {
        view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
        view_desc.Texture2DMS.UnusedField_NothingToDefine = 0;
    } else {
        view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        view_desc.Texture2D.MipSlice = 0;
    }

    ThrowIfFailed(mDevice->CreateDepthStencilView(texture.Get(), &view_desc, view));

    if (srv != nullptr) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
        srv_desc.Format =
            mFeatureLevel >= D3D_FEATURE_LEVEL_10_0 ? DXGI_FORMAT_R32_FLOAT : DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        srv_desc.ViewDimension = msaa_count > 1 ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Texture2D.MostDetailedMip = 0;
        srv_desc.Texture2D.MipLevels = -1;

        ThrowIfFailed(mDevice->CreateShaderResourceView(texture.Get(), &srv_desc, srv));
    }
}
static bool CreateDeviceFunc(class GfxRenderingAPIDX11* self, bool SoftwareRenderer) {
#if DEBUG_D3D
    UINT device_creation_flags = D3D11_CREATE_DEVICE_DEBUG;
#else
    UINT device_creation_flags = 0;
#endif
    bool CreationFailed = false;
    const char HardwareText[320] = "\nUsing software renderer. Performance issues are to be expected.\n\n"
                                   "Please check your preferred GPU in Windows graphics or GPU driver settings and "
                                   "make sure you have the correct GPU drivers installed.\n\n"
                                   "You can also try to change the graphic backend of the port in its config file.\n"
                                   "Window->Backend->Id\n"
                                   "0 = DX11, 1 = OpenGL";
    const char SoftwareText[33] = "\nUsing software renderer failed.";

    if (SoftwareRenderer) {
        SPDLOG_INFO("Using software renderer.");
    }

    HRESULT res = self->mDX11CreateDevice(
        NULL, SoftwareRenderer ? D3D_DRIVER_TYPE_WARP : D3D_DRIVER_TYPE_HARDWARE, nullptr, device_creation_flags, NULL,
        NULL, D3D11_SDK_VERSION, self->mDevice.GetAddressOf(), &self->mFeatureLevel, self->mContext.GetAddressOf());

    // Get and log name of adapter
    IDXGIDevice* DXGIDevice = nullptr;
    IDXGIAdapter* Adapter = nullptr;
    DXGI_ADAPTER_DESC adapterDesc;
    std::wstring adapterName;
    char adapterNameCStr[128] = "";
    char error_message[512];
    HRESULT res2;

    res2 = self->mDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&DXGIDevice);
    if (SUCCEEDED(res2)) {
        res2 = DXGIDevice->GetAdapter(&Adapter);
        if (SUCCEEDED(res2)) {
            res2 = Adapter->GetDesc(&adapterDesc);
            if (SUCCEEDED(res2)) {
                adapterName = adapterDesc.Description;
                wcstombs(adapterNameCStr, adapterName.c_str(), 128);
            }
            Adapter->Release();
        }
        DXGIDevice->Release();
    }
    SPDLOG_INFO("Using D3D adapter: {0}", adapterNameCStr);

    if (FAILED(res)) {
        CreationFailed = true;
        SPDLOG_WARN("Failed to create a D3D device. HRESULT: 0x{0:08x}", res);
        snprintf(error_message, sizeof(error_message), "Failed to create a D3D device on %s\nHRESULT: 0x%08X%s",
                 adapterNameCStr, res, SoftwareRenderer ? SoftwareText : HardwareText);
    }

    else if (self->mFeatureLevel < D3D_FEATURE_LEVEL_10_0) {
        CreationFailed = true;
        SPDLOG_WARN("D3D adapter doesn't support D3D feature level 10_0 or greater.");
        snprintf(error_message, sizeof(error_message), "%s doesn't support D3D feature level 10_0 or greater.%s",
                 adapterNameCStr, SoftwareRenderer ? SoftwareText : HardwareText);
    }

    else if (self->mFeatureLevel < D3D_FEATURE_LEVEL_10_1) {
        SPDLOG_WARN("D3D adapter doesn't support D3D feature level 10_1 or greater. MSAA setting will be ignored.");

    } else {
        // Check for Compute Shader support
        if (self->mDevice != NULL) {
            D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS features;
            self->mDevice->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &features,
                                               sizeof(D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS));
            if (features.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x == false) {
                CreationFailed = true;
                SPDLOG_WARN("D3D adapter doesn't support compute shaders.");
                snprintf(error_message, sizeof(error_message), "%s doesn't support compute shaders.%s", adapterNameCStr,
                         SoftwareRenderer ? SoftwareText : HardwareText);
            }
        }
    }

    if (CreationFailed) {
        if (self->mContext) {
            self->mContext->Release();
        }
        if (self->mDevice) {
            self->mDevice->Release();
        }
        MessageBoxA(self->mWindowBackend->GetWindowHandle(), error_message, "Warning", MB_OK | MB_ICONWARNING);
        return false;
    }
    return true;
};

void GfxRenderingAPIDX11::Init() {
    // Load d3d11.dll
    mDX11Module = LoadLibraryW(L"d3d11.dll");
    if (mDX11Module == nullptr) {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()), mWindowBackend->GetWindowHandle(), "d3d11.dll not found");
    }
    mDX11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(mDX11Module, "D3D11CreateDevice");

    // Load D3DCompiler_47.dll
    mCompilerModule = LoadLibraryW(L"D3DCompiler_47.dll");
    if (mCompilerModule == nullptr) {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()), mWindowBackend->GetWindowHandle(),
                      "D3DCompiler_47.dll not found");
    }
    mD3dCompile = (pD3DCompile)GetProcAddress(mCompilerModule, "D3DCompile");

    // Create D3D11 mDevice

    mWindowBackend->CreateFactoryAndDevice(DEBUG_D3D, 11, this, CreateDeviceFunc);

    // Create the swap chain
    mWindowBackend->CreateSwapChain(mDevice.Get(), [this]() {
        mFrameBuffers[0].render_target_view.Reset();
        mTextures[mFrameBuffers[0].texture_id].texture.Reset();
        mContext->ClearState();
        mContext->Flush();

        mLastShaderProgram = nullptr;
        mLastVertexBufferStride = 0;
        mLastBlendState.Reset();
        for (int i = 0; i < SHADER_MAX_TEXTURES; i++) {
            mLastResourceViews[i].Reset();
            mLastSamplerStates[i].Reset();
        }
        mLastDepthTest = -1;
        mLastDepthMask = -1;
        mLastZmodeDecal = -1;
        mLastPrimitaveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    });

    // Create D3D Debug mDevice if in debug mode

#if DEBUG_D3D
    ThrowIfFailed(mDevice->QueryInterface(__uuidof(ID3D11Debug), (void**)debug.GetAddressOf()),
                  mWindowBackend->GetWindowHandle(), "Failed to get ID3D11Debug device.");
#endif

    // Create the default framebuffer which represents the window
    FramebufferDX11& fb = mFrameBuffers[CreateFramebuffer()];

    // Check the size of the window
    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc;
    ThrowIfFailed(mWindowBackend->GetSwapChain()->GetDesc1(&swap_chain_desc));
    mTextures[fb.texture_id].width = swap_chain_desc.Width;
    mTextures[fb.texture_id].height = swap_chain_desc.Height;
    fb.msaa_level = 1;

    for (uint32_t sample_count = 1; sample_count <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; sample_count++) {
        ThrowIfFailed(mDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, sample_count,
                                                             &mMsaaNumQualityLevels[sample_count - 1]));
    }

    // Create main vertex buffer

    D3D11_BUFFER_DESC vertex_buffer_desc;
    ZeroMemory(&vertex_buffer_desc, sizeof(D3D11_BUFFER_DESC));

    vertex_buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
    vertex_buffer_desc.ByteWidth = 256 * 32 * 3 * sizeof(float); // Same as buf_vbo size in gfx_pc
    vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertex_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    vertex_buffer_desc.MiscFlags = 0;

    ThrowIfFailed(mDevice->CreateBuffer(&vertex_buffer_desc, nullptr, mVertexBuffer.GetAddressOf()),
                  mWindowBackend->GetWindowHandle(), "Failed to create vertex buffer.");

    // Create per-frame constant buffer

    D3D11_BUFFER_DESC constant_buffer_desc;
    ZeroMemory(&constant_buffer_desc, sizeof(D3D11_BUFFER_DESC));

    constant_buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
    constant_buffer_desc.ByteWidth = sizeof(PerFrameCB);
    constant_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constant_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    constant_buffer_desc.MiscFlags = 0;

    ThrowIfFailed(mDevice->CreateBuffer(&constant_buffer_desc, nullptr, mPerFrameCb.GetAddressOf()),
                  mWindowBackend->GetWindowHandle(), "Failed to create per-frame constant buffer.");

    // Create per-draw constant buffer

    constant_buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
    constant_buffer_desc.ByteWidth = sizeof(PerDrawCB);
    constant_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constant_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    constant_buffer_desc.MiscFlags = 0;

    ThrowIfFailed(mDevice->CreateBuffer(&constant_buffer_desc, nullptr, mPerDrawCb.GetAddressOf()),
                  mWindowBackend->GetWindowHandle(), "Failed to create per-draw constant buffer.");

    // Create compute shader that can be used to retrieve depth buffer values

    const char* shader_source = R"(
sampler my_sampler : register(s0);
Texture2D<float> tex : register(t0);
StructuredBuffer<int2> coord : register(t1);
RWStructuredBuffer<float> output : register(u0);

[numthreads(1, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID) {
    output[DTid.x] = tex.Load(int3(coord[DTid.x], 0));
}
)";

    const char* shader_source_msaa = R"(
sampler my_sampler : register(s0);
Texture2DMS<float, 2> tex : register(t0);
StructuredBuffer<int2> coord : register(t1);
RWStructuredBuffer<float> output : register(u0);

[numthreads(1, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID) {
    output[DTid.x] = tex.Load(coord[DTid.x], 0);
}
)";

#if DEBUG_D3D
    UINT compile_flags = D3DCOMPILE_DEBUG;
#else
    UINT compile_flags = D3DCOMPILE_OPTIMIZATION_LEVEL2;
#endif

    ComPtr<ID3DBlob> cs, error_blob;
    HRESULT hr;

    hr = mD3dCompile(shader_source, strlen(shader_source), nullptr, nullptr, nullptr, "CSMain", "cs_4_0", compile_flags,
                     0, cs.GetAddressOf(), error_blob.GetAddressOf());

    if (FAILED(hr)) {
        char* err = (char*)error_blob->GetBufferPointer();
        MessageBoxA(mWindowBackend->GetWindowHandle(), err, "Error", MB_OK | MB_ICONERROR);
        throw hr;
    }

    ThrowIfFailed(mDevice->CreateComputeShader(cs->GetBufferPointer(), cs->GetBufferSize(), nullptr,
                                               mComputeShader.GetAddressOf()));

    hr = mD3dCompile(shader_source_msaa, strlen(shader_source_msaa), nullptr, nullptr, nullptr, "CSMain", "cs_4_1",
                     compile_flags, 0, mComputeShaderMsaaBlob.GetAddressOf(), error_blob.ReleaseAndGetAddressOf());

    if (FAILED(hr)) {
        char* err = (char*)error_blob->GetBufferPointer();
        MessageBoxA(mWindowBackend->GetWindowHandle(), err, "Error", MB_OK | MB_ICONERROR);
        throw hr;
    }

    // Create ImGui

    Ship::GuiWindowInitData window_impl;
    window_impl.Dx11 = { mWindowBackend->GetWindowHandle(), mContext.Get(), mDevice.Get() };
    Ship::Context::GetInstance()->GetWindow()->GetGui()->Init(window_impl);
}

int GfxRenderingAPIDX11::GetMaxTextureSize() {
    return D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
}

const char* GfxRenderingAPIDX11::GetName() {
    return "DirectX 11";
}

struct GfxClipParameters GfxRenderingAPIDX11::GetClipParameters() {
    return { true, false };
}

void GfxRenderingAPIDX11::UnloadShader(struct ShaderProgram* old_prg) {
}

void GfxRenderingAPIDX11::LoadShader(struct ShaderProgram* new_prg) {
    mShaderProgram = (struct ShaderProgramD3D11*)new_prg;
}

struct ShaderProgram* GfxRenderingAPIDX11::CreateAndLoadNewShader(uint64_t shader_id0, uint64_t shader_id1) {
    CCFeatures cc_features;
    gfx_cc_get_features(shader_id0, shader_id1, &cc_features);

    char* buf;
    size_t len, numFloats;

    auto shader = gfx_direct3d_common_build_shader(numFloats, cc_features, false,
                                                   mCurrentFilterMode == FILTER_THREE_POINT, mSrgbMode);

    buf = shader.data();
    len = shader.size();

    ComPtr<ID3DBlob> vs, ps;
    ComPtr<ID3DBlob> error_blob;

#if DEBUG_D3D
    UINT compile_flags = D3DCOMPILE_DEBUG;
#else
    UINT compile_flags = D3DCOMPILE_OPTIMIZATION_LEVEL2;
#endif

    auto logCompileFailure = [&](const char* stage) {
        const char* err = (error_blob != nullptr) ? (const char*)error_blob->GetBufferPointer() : "No D3D compiler error blob";

        SPDLOG_ERROR(
            "DX11 shader compile failed at stage {} shader_id0=0x{:016X} shader_id1=0x{:016X}: {}",
            stage, shader_id0, shader_id1, err
        );
        SPDLOG_ERROR("DX11 shader source for failed compile:\n{}", shader);
    };

    HRESULT hr = mD3dCompile(buf, len, nullptr, nullptr, nullptr, "VSMain", "vs_4_0", compile_flags, 0,
                             vs.GetAddressOf(), error_blob.GetAddressOf());

    if (FAILED(hr)) {
        logCompileFailure("VS");
        return nullptr;
    }

    hr = mD3dCompile(buf, len, nullptr, nullptr, nullptr, "PSMain", "ps_4_0", compile_flags, 0, ps.GetAddressOf(),
                     error_blob.GetAddressOf());

    if (FAILED(hr)) {
        logCompileFailure("PS");
        return nullptr;
    }

    struct ShaderProgramD3D11* prg = &mShaderProgramPool[std::make_pair(shader_id0, shader_id1)];

    ThrowIfFailed(mDevice->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr,
                                              prg->vertex_shader.GetAddressOf()));
    ThrowIfFailed(mDevice->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr,
                                             prg->pixel_shader.GetAddressOf()));

    // Input Layout

    D3D11_INPUT_ELEMENT_DESC ied[16];
    uint8_t ied_index = 0;
    ied[ied_index++] = {
        "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0
    };
    for (UINT i = 0; i < 2; i++) {
        if (cc_features.usedTextures[i]) {
            ied[ied_index++] = {
                "TEXCOORD", i, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0
            };
            if (cc_features.clamp[i][0]) {
                ied[ied_index++] = { "TEXCLAMPS",
                                     i,
                                     DXGI_FORMAT_R32_FLOAT,
                                     0,
                                     D3D11_APPEND_ALIGNED_ELEMENT,
                                     D3D11_INPUT_PER_VERTEX_DATA,
                                     0 };
            }
            if (cc_features.clamp[i][1]) {
                ied[ied_index++] = { "TEXCLAMPT",
                                     i,
                                     DXGI_FORMAT_R32_FLOAT,
                                     0,
                                     D3D11_APPEND_ALIGNED_ELEMENT,
                                     D3D11_INPUT_PER_VERTEX_DATA,
                                     0 };
            }
        }
    }
    if (cc_features.opt_fog) {
        ied[ied_index++] = {
            "FOG", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0
        };
    }
    if (cc_features.opt_grayscale) {
        ied[ied_index++] = { "GRAYSCALE",
                             0,
                             DXGI_FORMAT_R32G32B32A32_FLOAT,
                             0,
                             D3D11_APPEND_ALIGNED_ELEMENT,
                             D3D11_INPUT_PER_VERTEX_DATA,
                             0 };
    }
    for (unsigned int i = 0; i < cc_features.numInputs; i++) {
        DXGI_FORMAT format = cc_features.opt_alpha ? DXGI_FORMAT_R32G32B32A32_FLOAT : DXGI_FORMAT_R32G32B32_FLOAT;
        ied[ied_index++] = { "INPUT", i, format, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
    }

    ThrowIfFailed(mDevice->CreateInputLayout(ied, ied_index, vs->GetBufferPointer(), vs->GetBufferSize(),
                                             prg->input_layout.GetAddressOf()));

    // Blend state

    D3D11_BLEND_DESC blend_desc;
    ZeroMemory(&blend_desc, sizeof(D3D11_BLEND_DESC));

    if (cc_features.opt_alpha) {
        blend_desc.RenderTarget[0].BlendEnable = true;
        blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
        blend_desc.RenderTarget[0].DestBlendAlpha =
            D3D11_BLEND_ONE; // We initially clear alpha to 1.0f and want to keep it at 1.0f
        blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    } else {
        blend_desc.RenderTarget[0].BlendEnable = false;
        blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    }

    ThrowIfFailed(mDevice->CreateBlendState(&blend_desc, prg->blend_state.GetAddressOf()));

    // Save some values

    prg->shader_id0 = shader_id0;
    prg->shader_id1 = shader_id1;
    prg->numInputs = cc_features.numInputs;
    prg->numFloats = numFloats;
    prg->usedTextures[0] = cc_features.usedTextures[0];
    prg->usedTextures[1] = cc_features.usedTextures[1];
    prg->usedTextures[2] = cc_features.used_masks[0];
    prg->usedTextures[3] = cc_features.used_masks[1];
    prg->usedTextures[4] = cc_features.used_blend[0];
    prg->usedTextures[5] = cc_features.used_blend[1];

    return (struct ShaderProgram*)(mShaderProgram = prg);
}

struct ShaderProgram* GfxRenderingAPIDX11::LookupShader(uint64_t shader_id0, uint64_t shader_id1) {
    auto it = mShaderProgramPool.find(std::make_pair(shader_id0, shader_id1));
    return it == mShaderProgramPool.end() ? nullptr : (struct ShaderProgram*)&it->second;
}

void GfxRenderingAPIDX11::ShaderGetInfo(struct ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]) {
    struct ShaderProgramD3D11* p = (struct ShaderProgramD3D11*)prg;

    *numInputs = p->numInputs;
    usedTextures[0] = p->usedTextures[0];
    usedTextures[1] = p->usedTextures[1];
}

uint32_t GfxRenderingAPIDX11::NewTexture() {
    mTextures.resize(mTextures.size() + 1);
    return (uint32_t)(mTextures.size() - 1);
}

void GfxRenderingAPIDX11::DeleteTexture(uint32_t texID) {
    // glDeleteTextures(1, &texID);
}

void GfxRenderingAPIDX11::SelectTexture(int tile, uint32_t texture_id) {
    mCurrentTile = tile;
    mCurrentTextureIds[tile] = texture_id;
}

static D3D11_TEXTURE_ADDRESS_MODE gfx_cm_to_d3d11(uint32_t val) {
    if ((val & G_TX_MIRROR) && (val & G_TX_CLAMP)) {
        return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
    }
    if (val & G_TX_CLAMP) {
        return D3D11_TEXTURE_ADDRESS_CLAMP;
    }
    return (val & G_TX_MIRROR) ? D3D11_TEXTURE_ADDRESS_MIRROR : D3D11_TEXTURE_ADDRESS_WRAP;
}

void GfxRenderingAPIDX11::UploadTexture(const uint8_t* rgba32_buf, uint32_t width, uint32_t height) {
    // Create texture

    TextureData* texture_data = &mTextures[mCurrentTextureIds[mCurrentTile]];
    texture_data->width = width;
    texture_data->height = height;

    D3D11_TEXTURE2D_DESC texture_desc;
    ZeroMemory(&texture_desc, sizeof(D3D11_TEXTURE2D_DESC));

    texture_desc.Width = width;
    texture_desc.Height = height;
    texture_desc.Usage = D3D11_USAGE_IMMUTABLE;
    texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0; // D3D11_RESOURCE_MISC_GENERATE_MIPS ?
    texture_desc.ArraySize = 1;
    texture_desc.MipLevels = 1;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;

    D3D11_SUBRESOURCE_DATA resource_data;
    resource_data.pSysMem = rgba32_buf;
    resource_data.SysMemPitch = width * 4;
    resource_data.SysMemSlicePitch = resource_data.SysMemPitch * height;

    ThrowIfFailed(
        mDevice->CreateTexture2D(&texture_desc, &resource_data, texture_data->texture.ReleaseAndGetAddressOf()));

    // Create shader resource view from texture

    ThrowIfFailed(mDevice->CreateShaderResourceView(texture_data->texture.Get(), nullptr,
                                                    texture_data->resource_view.ReleaseAndGetAddressOf()));
}

void GfxRenderingAPIDX11::SetSamplerParameters(int tile, bool linear_filter, uint32_t cms, uint32_t cmt) {
    D3D11_SAMPLER_DESC sampler_desc;
    ZeroMemory(&sampler_desc, sizeof(D3D11_SAMPLER_DESC));

    sampler_desc.Filter = linear_filter && mCurrentFilterMode == FILTER_LINEAR ? D3D11_FILTER_MIN_MAG_MIP_LINEAR
                                                                               : D3D11_FILTER_MIN_MAG_MIP_POINT;

    sampler_desc.AddressU = gfx_cm_to_d3d11(cms);
    sampler_desc.AddressV = gfx_cm_to_d3d11(cmt);
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.MinLOD = 0;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

    TextureData* texture_data = &mTextures[mCurrentTextureIds[tile]];
    texture_data->linear_filtering = linear_filter;

    // This function is called twice per texture, the first one only to set default values.
    // Maybe that could be skipped? Anyway, make sure to release the first default sampler
    // state before setting the actual one.
    texture_data->sampler_state.Reset();

    ThrowIfFailed(mDevice->CreateSamplerState(&sampler_desc, texture_data->sampler_state.GetAddressOf()));
}

void GfxRenderingAPIDX11::SetDepthTestAndMask(bool depth_test, bool depth_mask) {
    mCurrentDepthTest = depth_test;
    mCurrentDepthMask = depth_mask;
}

void GfxRenderingAPIDX11::SetZmodeDecal(bool zmode_decal) {
    mCurrentZmodeDecal = zmode_decal;
}

void GfxRenderingAPIDX11::SetViewport(int x, int y, int width, int height) {
    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = x;
    viewport.TopLeftY = mRenderTargetHeight - y - height;
    viewport.Width = width;
    viewport.Height = height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    mContext->RSSetViewports(1, &viewport);
}

void GfxRenderingAPIDX11::SetScissor(int x, int y, int width, int height) {
    D3D11_RECT rect;
    rect.left = x;
    rect.top = mRenderTargetHeight - y - height;
    rect.right = x + width;
    rect.bottom = mRenderTargetHeight - y;

    mContext->RSSetScissorRects(1, &rect);
}

void GfxRenderingAPIDX11::SetUseAlpha(bool use_alpha) {
    // Already part of the pipeline state from shader info
}

void GfxRenderingAPIDX11::DrawTriangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) {

    if (mLastDepthTest != mCurrentDepthTest || mLastDepthMask != mCurrentDepthMask) {
        mLastDepthTest = mCurrentDepthTest;
        mLastDepthMask = mCurrentDepthMask;

        mDepthStencilState.Reset();

        D3D11_DEPTH_STENCIL_DESC depth_stencil_desc;
        ZeroMemory(&depth_stencil_desc, sizeof(D3D11_DEPTH_STENCIL_DESC));

        depth_stencil_desc.DepthEnable = mCurrentDepthTest || mCurrentDepthMask;
        depth_stencil_desc.DepthWriteMask =
            mCurrentDepthMask ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
        depth_stencil_desc.DepthFunc = mCurrentDepthTest
                                           ? (mCurrentZmodeDecal ? D3D11_COMPARISON_LESS_EQUAL : D3D11_COMPARISON_LESS)
                                           : D3D11_COMPARISON_ALWAYS;
        depth_stencil_desc.StencilEnable = false;

        ThrowIfFailed(mDevice->CreateDepthStencilState(&depth_stencil_desc, mDepthStencilState.GetAddressOf()));
        mContext->OMSetDepthStencilState(mDepthStencilState.Get(), 0);
    }

    if (mLastZmodeDecal != mCurrentZmodeDecal) {
        mLastZmodeDecal = mCurrentZmodeDecal;

        mRasterizerState.Reset();

        D3D11_RASTERIZER_DESC rasterizer_desc;
        ZeroMemory(&rasterizer_desc, sizeof(D3D11_RASTERIZER_DESC));

        rasterizer_desc.FillMode = D3D11_FILL_SOLID;
        rasterizer_desc.CullMode = D3D11_CULL_NONE;
        rasterizer_desc.FrontCounterClockwise = true;
        rasterizer_desc.DepthBias = 0;
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
        rasterizer_desc.SlopeScaledDepthBias = mCurrentZmodeDecal ? SSDB : 0.0f;
        rasterizer_desc.DepthBiasClamp = 0.0f;
        rasterizer_desc.DepthClipEnable = false;
        rasterizer_desc.ScissorEnable = true;
        rasterizer_desc.MultisampleEnable = false;
        rasterizer_desc.AntialiasedLineEnable = false;

        ThrowIfFailed(mDevice->CreateRasterizerState(&rasterizer_desc, mRasterizerState.GetAddressOf()));
        mContext->RSSetState(mRasterizerState.Get());
    }

    bool textures_changed = false;

    for (int i = 0; i < SHADER_MAX_TEXTURES; i++) {
        if (mShaderProgram->usedTextures[i]) {
            if (mLastResourceViews[i].Get() != mTextures[mCurrentTextureIds[i]].resource_view.Get()) {
                mLastResourceViews[i] = mTextures[mCurrentTextureIds[i]].resource_view.Get();
                mContext->PSSetShaderResources(i, 1, mTextures[mCurrentTextureIds[i]].resource_view.GetAddressOf());

                if (mCurrentFilterMode == FILTER_THREE_POINT) {
                    mPerDrawCbData.mTextures[i].width = mTextures[mCurrentTextureIds[i]].width;
                    mPerDrawCbData.mTextures[i].height = mTextures[mCurrentTextureIds[i]].height;
                    mPerDrawCbData.mTextures[i].linear_filtering = mTextures[mCurrentTextureIds[i]].linear_filtering;
                    textures_changed = true;
                }

                if (mLastSamplerStates[i].Get() != mTextures[mCurrentTextureIds[i]].sampler_state.Get()) {
                    mLastSamplerStates[i] = mTextures[mCurrentTextureIds[i]].sampler_state.Get();
                }
            }
        }
        mContext->PSSetSamplers(i, 1, mTextures[mCurrentTextureIds[i]].sampler_state.GetAddressOf());
    }

    // Set per-draw constant buffer

    if (textures_changed) {
        D3D11_MAPPED_SUBRESOURCE ms;
        ZeroMemory(&ms, sizeof(D3D11_MAPPED_SUBRESOURCE));
        mContext->Map(mPerDrawCb.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
        memcpy(ms.pData, &mPerDrawCbData, sizeof(PerDrawCB));
        mContext->Unmap(mPerDrawCb.Get(), 0);
    }

    // Set vertex buffer data

    D3D11_MAPPED_SUBRESOURCE ms;
    ZeroMemory(&ms, sizeof(D3D11_MAPPED_SUBRESOURCE));
    mContext->Map(mVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
    memcpy(ms.pData, buf_vbo, buf_vbo_len * sizeof(float));
    mContext->Unmap(mVertexBuffer.Get(), 0);

    uint32_t stride = mShaderProgram->numFloats * sizeof(float);
    uint32_t offset = 0;

    if (mLastVertexBufferStride != stride) {
        mLastVertexBufferStride = stride;
        mContext->IASetVertexBuffers(0, 1, mVertexBuffer.GetAddressOf(), &stride, &offset);
    }

    if (mLastShaderProgram != mShaderProgram) {
        mLastShaderProgram = mShaderProgram;
        mContext->IASetInputLayout(mShaderProgram->input_layout.Get());
        mContext->VSSetShader(mShaderProgram->vertex_shader.Get(), 0, 0);
        mContext->PSSetShader(mShaderProgram->pixel_shader.Get(), 0, 0);

        if (mLastBlendState.Get() != mShaderProgram->blend_state.Get()) {
            mLastBlendState = mShaderProgram->blend_state.Get();
            mContext->OMSetBlendState(mShaderProgram->blend_state.Get(), 0, 0xFFFFFFFF);
        }
    }

    if (mLastPrimitaveTopology != D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST) {
        mLastPrimitaveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        mContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    mContext->Draw(buf_vbo_num_tris * 3, 0);
}

void GfxRenderingAPIDX11::OnResize() {
    // create_render_target_views(true);
}

void GfxRenderingAPIDX11::StartFrame() {
    // Set per-frame constant buffer
    ID3D11Buffer* buffers[2] = { mPerFrameCb.Get(), mPerDrawCb.Get() };
    mContext->PSSetConstantBuffers(0, 2, buffers);

    mPerFrameCbData.noise_frame++;
    if (mPerFrameCbData.noise_frame > 150) {
        // No high values, as noise starts to look ugly
        mPerFrameCbData.noise_frame = 0;
    }
}

void GfxRenderingAPIDX11::EndFrame() {
    mContext->Flush();
}

void GfxRenderingAPIDX11::FinishRender() {
}

int GfxRenderingAPIDX11::CreateFramebuffer() {
    uint32_t texture_id = NewTexture();
    TextureData& t = mTextures[texture_id];

    size_t index = mFrameBuffers.size();
    mFrameBuffers.resize(mFrameBuffers.size() + 1);
    FramebufferDX11& data = mFrameBuffers.back();
    data.texture_id = texture_id;

    uint32_t tile = 0;
    uint32_t saved = mCurrentTextureIds[tile];
    mCurrentTextureIds[tile] = texture_id;
    SetSamplerParameters(0, true, G_TX_WRAP, G_TX_WRAP);
    mCurrentTextureIds[tile] = saved;

    return (int)index;
}

void GfxRenderingAPIDX11::DestroyFramebuffer(int fbId) {
    if (fbId < 0 || (size_t)fbId >= mFrameBuffers.size()) {
        return;
    }
    FramebufferDX11& fb = mFrameBuffers[fbId];
    // .Reset() on the ComPtrs drops our refcount; D3D11 frees the
    // underlying object when no more views reference it.
    fb.render_target_view.Reset();
    fb.depth_stencil_view.Reset();
    fb.depth_stencil_srv.Reset();
    // The color attachment lives in mTextures[fb.texture_id]; clear it
    // too so the GPU memory frees with the rest of the slot. The
    // texture_id slot itself stays tombstoned — D3D11's NewTexture() is
    // monotonic and slot reuse isn't worth adding for the bookkeeping
    // saving.
    if (fb.texture_id < mTextures.size()) {
        TextureData& tex = mTextures[fb.texture_id];
        tex.texture.Reset();
        tex.resource_view.Reset();
        tex.sampler_state.Reset();
        tex.width = 0;
        tex.height = 0;
    }
    fb.has_depth_buffer = false;
    fb.msaa_level = 0;
}

void GfxRenderingAPIDX11::UpdateFramebufferParameters(int fb_id, uint32_t width, uint32_t height, uint32_t msaa_level,
                                                      bool opengl_invertY, bool render_target, bool has_depth_buffer,
                                                      bool can_extract_depth) {
    FramebufferDX11& fb = mFrameBuffers[fb_id];
    TextureData& tex = mTextures[fb.texture_id];

    width = ((width) > (1U) ? (width) : (1U));
    height = ((height) > (1U) ? (height) : (1U));
    // We can't use MSAA the way we are using it on feature level 10_0 hardware, so disable it altogether.
    msaa_level = mFeatureLevel < D3D_FEATURE_LEVEL_10_1 ? 1 : msaa_level;
    while (msaa_level > 1 && mMsaaNumQualityLevels[msaa_level - 1] == 0) {
        --msaa_level;
    }

    const bool formatChanged = (fb.postProcessFormat != fb.lastPostProcessFormat);
    const bool mipmappedChanged = (fb.postProcessMipmapped != fb.lastPostProcessMipmapped);
    bool diff = tex.width != width || tex.height != height || fb.msaa_level != msaa_level ||
                formatChanged || mipmappedChanged;

    if (diff || (fb.render_target_view.Get() != nullptr) != render_target) {
        if (fb_id != 0) {
            DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
            switch (fb.postProcessFormat) {
                case PostProcessFboFormat::Default:
                    colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
                    break;
                case PostProcessFboFormat::Srgb:
                    colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
                    break;
                case PostProcessFboFormat::Float16:
                    colorFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
                    break;
            }
            // Phase 2.2: when a downstream mipmap_input pass will
            // sample this texture, allocate the full mip chain and
            // tag with GENERATE_MIPS so ID3D11DeviceContext::
            // GenerateMips fills it later. MSAA + mips is illegal
            // in D3D11, so the mipmap flag implicitly forces
            // msaa_level=1 — the chain only marks post-process
            // intermediates anyway, which are already non-MSAA.
            const bool mipmapped = fb.postProcessMipmapped && msaa_level <= 1;
            D3D11_TEXTURE2D_DESC texture_desc;
            texture_desc.Width = width;
            texture_desc.Height = height;
            texture_desc.Usage = D3D11_USAGE_DEFAULT;
            texture_desc.BindFlags =
                (msaa_level <= 1 ? D3D11_BIND_SHADER_RESOURCE : 0) | (render_target ? D3D11_BIND_RENDER_TARGET : 0);
            texture_desc.Format = colorFormat;
            texture_desc.CPUAccessFlags = 0;
            texture_desc.MiscFlags = mipmapped ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;
            texture_desc.ArraySize = 1;
            texture_desc.MipLevels = mipmapped ? 0 : 1; // 0 = full chain
            texture_desc.SampleDesc.Count = msaa_level;
            texture_desc.SampleDesc.Quality = 0;

            ThrowIfFailed(mDevice->CreateTexture2D(&texture_desc, nullptr, tex.texture.ReleaseAndGetAddressOf()));

            if (msaa_level <= 1) {
                ThrowIfFailed(mDevice->CreateShaderResourceView(tex.texture.Get(), nullptr,
                                                                tex.resource_view.ReleaseAndGetAddressOf()));
            }
        } else if (diff || (render_target && tex.texture.Get() == nullptr)) {
            DXGI_SWAP_CHAIN_DESC1 desc1;
            IDXGISwapChain1* swap_chain = mWindowBackend->GetSwapChain();
            ThrowIfFailed(swap_chain->GetDesc1(&desc1));
            if (desc1.Width != width || desc1.Height != height) {
                fb.render_target_view.Reset();
                tex.texture.Reset();
                ThrowIfFailed(swap_chain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, desc1.Flags));
            }
            ThrowIfFailed(
                swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)tex.texture.ReleaseAndGetAddressOf()));
        }
        if (render_target) {
            ThrowIfFailed(mDevice->CreateRenderTargetView(tex.texture.Get(), nullptr,
                                                          fb.render_target_view.ReleaseAndGetAddressOf()));
        }

        tex.width = width;
        tex.height = height;
        fb.lastPostProcessFormat = fb.postProcessFormat;
        fb.lastPostProcessMipmapped = fb.postProcessMipmapped;
    }

    if (has_depth_buffer &&
        (diff || !fb.has_depth_buffer || (fb.depth_stencil_srv.Get() != nullptr) != can_extract_depth)) {
        fb.depth_stencil_srv.Reset();
        CreateDepthStencilObjects(width, height, msaa_level, fb.depth_stencil_view.ReleaseAndGetAddressOf(),
                                  can_extract_depth ? fb.depth_stencil_srv.GetAddressOf() : nullptr);
    }
    if (!has_depth_buffer) {
        fb.depth_stencil_view.Reset();
        fb.depth_stencil_srv.Reset();
    }

    fb.has_depth_buffer = has_depth_buffer;
    fb.msaa_level = msaa_level;
}

void GfxRenderingAPIDX11::StartDrawToFramebuffer(int fb_id, float noise_scale) {
    FramebufferDX11& fb = mFrameBuffers[fb_id];
    mRenderTargetHeight = mTextures[fb.texture_id].height;

    mContext->OMSetRenderTargets(1, fb.render_target_view.GetAddressOf(),
                                 fb.has_depth_buffer ? fb.depth_stencil_view.Get() : nullptr);

    mCurrentFramebuffer = fb_id;

    if (noise_scale != 0.0f) {
        mPerFrameCbData.noise_scale = 1.0f / noise_scale;
    }

    D3D11_MAPPED_SUBRESOURCE ms;
    ZeroMemory(&ms, sizeof(D3D11_MAPPED_SUBRESOURCE));
    mContext->Map(mPerFrameCb.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
    memcpy(ms.pData, &mPerFrameCbData, sizeof(PerFrameCB));
    mContext->Unmap(mPerFrameCb.Get(), 0);
}

void GfxRenderingAPIDX11::ClearFramebuffer(bool color, bool depth) {
    FramebufferDX11& fb = mFrameBuffers[mCurrentFramebuffer];
    if (color) {
        const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        mContext->ClearRenderTargetView(fb.render_target_view.Get(), clearColor);
    }
    if (depth && fb.has_depth_buffer) {
        mContext->ClearDepthStencilView(fb.depth_stencil_view.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    }
}

void GfxRenderingAPIDX11::ResolveMSAAColorBuffer(int fb_id_target, int fb_id_source) {
    FramebufferDX11& fb_dst = mFrameBuffers[fb_id_target];
    FramebufferDX11& fb_src = mFrameBuffers[fb_id_source];

    mContext->ResolveSubresource(mTextures[fb_dst.texture_id].texture.Get(), 0,
                                 mTextures[fb_src.texture_id].texture.Get(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);
}

void* GfxRenderingAPIDX11::GetFramebufferTextureId(int fb_id) {
    return (void*)mTextures[mFrameBuffers[fb_id].texture_id].resource_view.Get();
}

void GfxRenderingAPIDX11::SelectTextureFb(int fbID) {
    int tile = 0;
    SelectTexture(tile, mFrameBuffers[fbID].texture_id);
}

void GfxRenderingAPIDX11::CopyFramebuffer(int fb_dst_id, int fb_src_id, int srcX0, int srcY0, int srcX1, int srcY1,
                                          int dstX0, int dstY0, int dstX1, int dstY1) {
    if (fb_src_id >= (int)mFrameBuffers.size() || fb_dst_id >= (int)mFrameBuffers.size()) {
        return;
    }

    FramebufferDX11& fb_dst = mFrameBuffers[fb_dst_id];
    FramebufferDX11& fb_src = mFrameBuffers[fb_src_id];

    TextureData& td_dst = mTextures[fb_dst.texture_id];
    TextureData& td_src = mTextures[fb_src.texture_id];

    // Textures are the same size so we can do a direct copy or resolve
    if (td_src.height == td_dst.height && td_src.width == td_dst.width) {
        if (fb_src.msaa_level <= 1) {
            mContext->CopyResource(td_dst.texture.Get(), td_src.texture.Get());
        } else {
            mContext->ResolveSubresource(td_dst.texture.Get(), 0, td_src.texture.Get(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);
        }
        return;
    }

    if (srcY1 > (int)td_src.height || srcX1 > (int)td_src.width || srcX0 < 0 || srcY0 < 0 ||
        dstY1 > (int)td_dst.height || dstX1 > (int)td_dst.width || dstX0 < 0 || dstY0 < 0) {
        // Using a source region larger than the source resource or copy outside of the destination resource is
        // considered undefined behavior and could lead to removal of the rendering mDevice
        return;
    }

    D3D11_BOX region;
    region.left = srcX0;
    region.right = srcX1;
    region.top = srcY0;
    region.bottom = srcY1;
    region.front = 0;
    region.back = 1;

    // We can't region copy a multi-sample texture to a single sample texture
    if (fb_src.msaa_level <= 1) {
        mContext->CopySubresourceRegion(td_dst.texture.Get(), dstX0, dstY0, 0, 0, td_src.texture.Get(), 0, &region);
    } else {
        // Setup a temporary texture
        TextureData td_resolved;
        td_resolved.width = td_src.width;
        td_resolved.height = td_src.height;

        D3D11_TEXTURE2D_DESC texture_desc;
        texture_desc.Width = td_src.width;
        texture_desc.Height = td_src.height;
        texture_desc.Usage = D3D11_USAGE_DEFAULT;
        texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texture_desc.CPUAccessFlags = 0;
        texture_desc.MiscFlags = 0;
        texture_desc.ArraySize = 1;
        texture_desc.MipLevels = 1;
        texture_desc.SampleDesc.Count = 1;
        texture_desc.SampleDesc.Quality = 0;

        ThrowIfFailed(mDevice->CreateTexture2D(&texture_desc, nullptr, td_resolved.texture.GetAddressOf()));

        // Resolve multi-sample to temporary
        mContext->ResolveSubresource(td_resolved.texture.Get(), 0, td_src.texture.Get(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);
        // Then copy the region to the destination
        mContext->CopySubresourceRegion(td_dst.texture.Get(), dstX0, dstY0, 0, 0, td_resolved.texture.Get(), 0,
                                        &region);
    }
}

void GfxRenderingAPIDX11::ReadFramebufferToCPU(int fb_id, uint32_t width, uint32_t height, uint16_t* rgba16_buf) {
    if (fb_id < 0 || fb_id >= (int)mFrameBuffers.size() || rgba16_buf == nullptr || width == 0 || height == 0) {
        return;
    }

    FramebufferDX11& fb = mFrameBuffers[fb_id];
    if (fb.texture_id >= mTextures.size()) {
        return;
    }
    TextureData& td = mTextures[fb.texture_id];
    if (!td.texture) {
        return;
    }

    // CopyResource requires identical extents/format. Discover the source
    // texture's true dimensions and bail on multi-sampled sources rather than
    // letting the runtime fail asynchronously (caller needs to Resolve first).
    D3D11_TEXTURE2D_DESC src_desc = {};
    td.texture->GetDesc(&src_desc);
    if (src_desc.SampleDesc.Count > 1) {
        return;
    }

    // Pre-zero the destination so a partial overlap leaves the un-touched
    // tail in a deterministic state.
    std::memset(rgba16_buf, 0, sizeof(uint16_t) * (size_t)width * (size_t)height);

    const uint32_t copy_w = std::min(width, src_desc.Width);
    const uint32_t copy_h = std::min(height, src_desc.Height);

    ID3D11Texture2D* staging = nullptr;

    D3D11_TEXTURE2D_DESC texture_desc;
    texture_desc.Width = src_desc.Width;
    texture_desc.Height = src_desc.Height;
    texture_desc.Usage = D3D11_USAGE_STAGING;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    texture_desc.BindFlags = 0;
    texture_desc.MiscFlags = 0;
    texture_desc.ArraySize = 1;
    texture_desc.MipLevels = 1;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;

    HRESULT hr = mDevice->CreateTexture2D(&texture_desc, nullptr, &staging);
    if (FAILED(hr) || staging == nullptr) {
        return;
    }

    mContext->CopyResource(staging, td.texture.Get());

    D3D11_MAPPED_SUBRESOURCE resource = {};
    hr = mContext->Map(staging, 0, D3D11_MAP_READ, 0, &resource);
    if (FAILED(hr) || resource.pData == nullptr) {
        staging->Release();
        return;
    }

    // Convert directly from the mapped pages into the caller's RGBA5551
    // buffer, walking the source by `resource.RowPitch` (the GPU's natural
    // row stride) and the destination by `width` (the caller's pitch). Read
    // up to `copy_w x copy_h` pixels; anything outside that rect was zeroed
    // above. The previous implementation copied to a `width*height` u32
    // intermediate using `RowPitch` for both stride AND bytes-per-row, which
    // overflows the heap whenever `RowPitch != width*4` (D3D11 always
    // 256-byte-aligns `RowPitch`).
    const uint8_t* src_bytes = static_cast<const uint8_t*>(resource.pData);
    for (uint32_t j = 0; j < copy_h; ++j) {
        const uint32_t* src_row = reinterpret_cast<const uint32_t*>(src_bytes + (size_t)j * resource.RowPitch);
        uint16_t* dst_row = rgba16_buf + (size_t)j * width;
        for (uint32_t i = 0; i < copy_w; ++i) {
            uint32_t pixel = src_row[i];
            uint8_t r = (((pixel & 0xFF) + 4) * 0x1F) / 0xFF;
            uint8_t g = ((((pixel >> 8) & 0xFF) + 4) * 0x1F) / 0xFF;
            uint8_t b = ((((pixel >> 16) & 0xFF) + 4) * 0x1F) / 0xFF;
            uint8_t a = ((pixel >> 24) & 0xFF) ? 1 : 0;
            dst_row[i] = (r << 11) | (g << 6) | (b << 1) | a;
        }
    }

    mContext->Unmap(staging, 0);
    staging->Release();
}

void GfxRenderingAPIDX11::SetTextureFilter(FilteringMode mode) {
    mCurrentFilterMode = mode;
    gfx_texture_cache_clear();
}

FilteringMode GfxRenderingAPIDX11::GetTextureFilter() {
    return mCurrentFilterMode;
}

std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff>
GfxRenderingAPIDX11::GetPixelDepth(int fb_id, const std::set<std::pair<float, float>>& coordinates) {
    FramebufferDX11& fb = mFrameBuffers[fb_id];
    TextureData& td = mTextures[fb.texture_id];

    if (coordinates.size() > mCoordBufferSize) {
        mCoordBuffer.Reset();
        mCoordBufferSrv.Reset();
        mDepthValueOutputBuffer.Reset();
        mDepthValueOutputUav.Reset();
        mDepthValueOutputBufferCopy.Reset();

        D3D11_BUFFER_DESC coord_buf_desc;
        coord_buf_desc.Usage = D3D11_USAGE_DYNAMIC;
        coord_buf_desc.ByteWidth = sizeof(Coord) * coordinates.size();
        coord_buf_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        coord_buf_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        coord_buf_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        coord_buf_desc.StructureByteStride = sizeof(Coord);

        ThrowIfFailed(mDevice->CreateBuffer(&coord_buf_desc, nullptr, mCoordBuffer.GetAddressOf()));

        D3D11_SHADER_RESOURCE_VIEW_DESC coord_buf_srv_desc;
        coord_buf_srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        coord_buf_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        coord_buf_srv_desc.Buffer.FirstElement = 0;
        coord_buf_srv_desc.Buffer.NumElements = coordinates.size();

        ThrowIfFailed(
            mDevice->CreateShaderResourceView(mCoordBuffer.Get(), &coord_buf_srv_desc, mCoordBufferSrv.GetAddressOf()));

        D3D11_BUFFER_DESC output_buffer_desc;
        output_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
        output_buffer_desc.ByteWidth = sizeof(float) * coordinates.size();
        output_buffer_desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
        output_buffer_desc.CPUAccessFlags = 0;
        output_buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        output_buffer_desc.StructureByteStride = sizeof(float);
        ThrowIfFailed(mDevice->CreateBuffer(&output_buffer_desc, nullptr, mDepthValueOutputBuffer.GetAddressOf()));

        D3D11_UNORDERED_ACCESS_VIEW_DESC output_buffer_uav_desc;
        output_buffer_uav_desc.Format = DXGI_FORMAT_UNKNOWN;
        output_buffer_uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        output_buffer_uav_desc.Buffer.FirstElement = 0;
        output_buffer_uav_desc.Buffer.NumElements = coordinates.size();
        output_buffer_uav_desc.Buffer.Flags = 0;
        ThrowIfFailed(mDevice->CreateUnorderedAccessView(mDepthValueOutputBuffer.Get(), &output_buffer_uav_desc,
                                                         mDepthValueOutputUav.GetAddressOf()));

        output_buffer_desc.Usage = D3D11_USAGE_STAGING;
        output_buffer_desc.BindFlags = 0;
        output_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        ThrowIfFailed(mDevice->CreateBuffer(&output_buffer_desc, nullptr, mDepthValueOutputBufferCopy.GetAddressOf()));

        mCoordBufferSize = coordinates.size();
    }

    D3D11_MAPPED_SUBRESOURCE ms;

    if (fb.msaa_level > 1 && mComputeShaderMsaa.Get() == nullptr) {
        ThrowIfFailed(mDevice->CreateComputeShader(mComputeShaderMsaaBlob->GetBufferPointer(),
                                                   mComputeShaderMsaaBlob->GetBufferSize(), nullptr,
                                                   mComputeShaderMsaa.GetAddressOf()));
    }

    // ImGui overwrites these values, so we cannot set them once at init
    mContext->CSSetShader(fb.msaa_level > 1 ? mComputeShaderMsaa.Get() : mComputeShader.Get(), nullptr, 0);
    mContext->CSSetUnorderedAccessViews(0, 1, mDepthValueOutputUav.GetAddressOf(), nullptr);

    ThrowIfFailed(mContext->Map(mCoordBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms));
    Coord* coord_cb = (Coord*)ms.pData;
    {
        size_t i = 0;
        for (const auto& coord : coordinates) {
            coord_cb[i].x = coord.first;
            // We invert y because the gfx_pc assumes OpenGL coordinates (bottom-left corner is origin), while DX's
            // origin is top-left corner
            coord_cb[i].y = td.height - 1 - coord.second;
            ++i;
        }
    }
    mContext->Unmap(mCoordBuffer.Get(), 0);

    // The depth stencil texture can only have one mapping at a time, so unbind from the OM
    ID3D11RenderTargetView* null_arr1[1] = { nullptr };
    mContext->OMSetRenderTargets(1, null_arr1, nullptr);

    ID3D11ShaderResourceView* srvs[2] = { fb.depth_stencil_srv.Get(), mCoordBufferSrv.Get() };
    mContext->CSSetShaderResources(0, 2, srvs);

    mContext->Dispatch(coordinates.size(), 1, 1);

    mContext->CopyResource(mDepthValueOutputBufferCopy.Get(), mDepthValueOutputBuffer.Get());
    ThrowIfFailed(mContext->Map(mDepthValueOutputBufferCopy.Get(), 0, D3D11_MAP_READ, 0, &ms));
    std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff> res;
    {
        size_t i = 0;
        for (const auto& coord : coordinates) {
            res.emplace(coord, ((float*)ms.pData)[i++] * 65532.0f);
        }
    }
    mContext->Unmap(mDepthValueOutputBufferCopy.Get(), 0);

    ID3D11ShaderResourceView* null_arr[2] = { nullptr, nullptr };
    mContext->CSSetShaderResources(0, 2, null_arr);

    return res;
}

ImTextureID GfxRenderingAPIDX11::GetTextureById(int id) {
    return mTextures[id].resource_view.Get();
}

void GfxRenderingAPIDX11::SetSrgbMode() {
    mSrgbMode = true;
}

#define RAND_NOISE "((random(float3(floor(screenSpace.xy * noise_scale), noise_frame)) + 1.0) / 2.0)"

static const char* prism_shader_item_to_str(uint32_t item, bool with_alpha, bool only_alpha, bool inputs_have_alpha,
                                            bool first_cycle, bool hint_single_element) {
    if (!only_alpha) {
        switch (item) {
            default:
            case SHADER_0:
                return with_alpha ? "float4(0.0, 0.0, 0.0, 0.0)" : "float3(0.0, 0.0, 0.0)";
            case SHADER_1:
                return with_alpha ? "float4(1.0, 1.0, 1.0, 1.0)" : "float3(1.0, 1.0, 1.0)";
            case SHADER_INPUT_1:
                return with_alpha || !inputs_have_alpha ? "input.input1" : "input.input1.rgb";
            case SHADER_INPUT_2:
                return with_alpha || !inputs_have_alpha ? "input.input2" : "input.input2.rgb";
            case SHADER_INPUT_3:
                return with_alpha || !inputs_have_alpha ? "input.input3" : "input.input3.rgb";
            case SHADER_INPUT_4:
                return with_alpha || !inputs_have_alpha ? "input.input4" : "input.input4.rgb";
            case SHADER_TEXEL0:
                return first_cycle ? (with_alpha ? "texVal0" : "texVal0.rgb")
                                   : (with_alpha ? "texVal1" : "texVal1.rgb");
            case SHADER_TEXEL0A:
                return first_cycle
                           ? (hint_single_element ? "texVal0.a"
                                                  : (with_alpha ? "float4(texVal0.a, texVal0.a, texVal0.a, texVal0.a)"
                                                                : "float3(texVal0.a, texVal0.a, texVal0.a)"))
                           : (hint_single_element ? "texVal1.a"
                                                  : (with_alpha ? "float4(texVal1.a, texVal1.a, texVal1.a, texVal1.a)"
                                                                : "float3(texVal1.a, texVal1.a, texVal1.a)"));
            case SHADER_TEXEL1A:
                return first_cycle
                           ? (hint_single_element ? "texVal1.a"
                                                  : (with_alpha ? "float4(texVal1.a, texVal1.a, texVal1.a, texVal1.a)"
                                                                : "float3(texVal1.a, texVal1.a, texVal1.a)"))
                           : (hint_single_element ? "texVal0.a"
                                                  : (with_alpha ? "float4(texVal0.a, texVal0.a, texVal0.a, texVal0.a)"
                                                                : "float3(texVal0.a, texVal0.a, texVal0.a)"));
            case SHADER_TEXEL1:
                return first_cycle ? (with_alpha ? "texVal1" : "texVal1.rgb")
                                   : (with_alpha ? "texVal0" : "texVal0.rgb");
            case SHADER_COMBINED:
                return with_alpha ? "texel" : "texel.rgb";
            case SHADER_NOISE:
                return with_alpha ? "float4(" RAND_NOISE ", " RAND_NOISE ", " RAND_NOISE ", " RAND_NOISE ")"
                                  : "float3(" RAND_NOISE ", " RAND_NOISE ", " RAND_NOISE ")";
        }
    } else {
        switch (item) {
            default:
            case SHADER_0:
                return "0.0";
            case SHADER_1:
                return "1.0";
            case SHADER_INPUT_1:
                return "input.input1.a";
            case SHADER_INPUT_2:
                return "input.input2.a";
            case SHADER_INPUT_3:
                return "input.input3.a";
            case SHADER_INPUT_4:
                return "input.input4.a";
            case SHADER_TEXEL0:
                return first_cycle ? "texVal0.a" : "texVal1.a";
            case SHADER_TEXEL0A:
                return first_cycle ? "texVal0.a" : "texVal1.a";
            case SHADER_TEXEL1A:
                return first_cycle ? "texVal1.a" : "texVal0.a";
            case SHADER_TEXEL1:
                return first_cycle ? "texVal1.a" : "texVal0.a";
            case SHADER_COMBINED:
                return "texel.a";
            case SHADER_NOISE:
                return RAND_NOISE;
        }
    }
}

bool prism_get_bool(prism::ContextTypes* value) {
    if (std::holds_alternative<int>(*value)) {
        return std::get<int>(*value) == 1;
    }
    return false;
}

#undef RAND_NOISE

prism::ContextTypes* prism_append_formula(prism::ContextTypes* _, prism::ContextTypes* a_arg,
                                          prism::ContextTypes* a_single, prism::ContextTypes* a_mult,
                                          prism::ContextTypes* a_mix, prism::ContextTypes* a_with_alpha,
                                          prism::ContextTypes* a_only_alpha, prism::ContextTypes* a_alpha,
                                          prism::ContextTypes* a_first_cycle) {
    auto c = std::get<prism::MTDArray<int>>(*a_arg);
    bool do_single = prism_get_bool(a_single);
    bool do_multiply = prism_get_bool(a_mult);
    bool do_mix = prism_get_bool(a_mix);
    bool with_alpha = prism_get_bool(a_with_alpha);
    bool only_alpha = prism_get_bool(a_only_alpha);
    bool opt_alpha = prism_get_bool(a_alpha);
    bool first_cycle = prism_get_bool(a_first_cycle);
    std::string out = "";
    if (do_single) {
        out += prism_shader_item_to_str(c.at(only_alpha, 3), with_alpha, only_alpha, opt_alpha, first_cycle, false);
    } else if (do_multiply) {
        out += prism_shader_item_to_str(c.at(only_alpha, 0), with_alpha, only_alpha, opt_alpha, first_cycle, false);
        out += " * ";
        out += prism_shader_item_to_str(c.at(only_alpha, 2), with_alpha, only_alpha, opt_alpha, first_cycle, true);
    } else if (do_mix) {
        out += "lerp(";
        out += prism_shader_item_to_str(c.at(only_alpha, 1), with_alpha, only_alpha, opt_alpha, first_cycle, false);
        out += ", ";
        out += prism_shader_item_to_str(c.at(only_alpha, 0), with_alpha, only_alpha, opt_alpha, first_cycle, false);
        out += ", ";
        out += prism_shader_item_to_str(c.at(only_alpha, 2), with_alpha, only_alpha, opt_alpha, first_cycle, true);
        out += ")";
    } else {
        out += "(";
        out += prism_shader_item_to_str(c.at(only_alpha, 0), with_alpha, only_alpha, opt_alpha, first_cycle, false);
        out += " - ";
        out += prism_shader_item_to_str(c.at(only_alpha, 1), with_alpha, only_alpha, opt_alpha, first_cycle, false);
        out += ") * ";
        out += prism_shader_item_to_str(c.at(only_alpha, 2), with_alpha, only_alpha, opt_alpha, first_cycle, true);
        out += " + ";
        out += prism_shader_item_to_str(c.at(only_alpha, 3), with_alpha, only_alpha, opt_alpha, first_cycle, false);
    }
    return new prism::ContextTypes{ out };
}

static size_t raw_numFloats = 0;

prism::ContextTypes* update_raw_floats(prism::ContextTypes* _, prism::ContextTypes* num) {
    raw_numFloats += std::get<int>(*num);
    return nullptr;
}

std::optional<std::string> dx_include_fs(const std::string& path) {
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Type = (uint32_t)Ship::ResourceType::Shader;
    init->ByteOrder = Ship::Endianness::Native;
    init->Format = RESOURCE_FORMAT_BINARY;
    auto res = static_pointer_cast<Ship::Shader>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResource(path, true, init));
    if (res == nullptr) {
        return std::nullopt;
    }

    auto inc = static_cast<std::string*>(res->GetRawPointer());
    return *inc;
}

std::string gfx_direct3d_common_build_shader(size_t& numFloats, const CCFeatures& cc_features,
                                             bool include_root_signature, bool three_point_filtering, bool use_srgb) {
    raw_numFloats = 4;

    prism::Processor processor;
    prism::ContextItems mContext = {
        { "SHADER_0", SHADER_0 },
        { "SHADER_INPUT_1", SHADER_INPUT_1 },
        { "SHADER_INPUT_2", SHADER_INPUT_2 },
        { "SHADER_INPUT_3", SHADER_INPUT_3 },
        { "SHADER_INPUT_4", SHADER_INPUT_4 },
        { "SHADER_INPUT_5", SHADER_INPUT_5 },
        { "SHADER_INPUT_6", SHADER_INPUT_6 },
        { "SHADER_INPUT_7", SHADER_INPUT_7 },
        { "SHADER_TEXEL0", SHADER_TEXEL0 },
        { "SHADER_TEXEL0A", SHADER_TEXEL0A },
        { "SHADER_TEXEL1", SHADER_TEXEL1 },
        { "SHADER_TEXEL1A", SHADER_TEXEL1A },
        { "SHADER_1", SHADER_1 },
        { "SHADER_COMBINED", SHADER_COMBINED },
        { "SHADER_NOISE", SHADER_NOISE },
        { "o_c", M_ARRAY(cc_features.c, int, 2, 2, 4) },
        { "o_alpha", cc_features.opt_alpha },
        { "o_fog", cc_features.opt_fog },
        { "o_texture_edge", cc_features.opt_texture_edge },
        { "o_noise", cc_features.opt_noise },
        { "o_2cyc", cc_features.opt_2cyc },
        { "o_alpha_threshold", cc_features.opt_alpha_threshold },
        { "o_invisible", cc_features.opt_invisible },
        { "o_grayscale", cc_features.opt_grayscale },
        { "o_textures", M_ARRAY(cc_features.usedTextures, bool, 2) },
        { "o_masks", M_ARRAY(cc_features.used_masks, bool, 2) },
        { "o_blend", M_ARRAY(cc_features.used_blend, bool, 2) },
        { "o_clamp", M_ARRAY(cc_features.clamp, bool, 2, 2) },
        { "o_inputs", cc_features.numInputs },
        { "o_do_mix", M_ARRAY(cc_features.do_mix, bool, 2, 2) },
        { "o_do_single", M_ARRAY(cc_features.do_single, bool, 2, 2) },
        { "o_do_multiply", M_ARRAY(cc_features.do_multiply, bool, 2, 2) },
        { "o_color_alpha_same", M_ARRAY(cc_features.color_alpha_same, bool, 2) },
        { "o_root_signature", include_root_signature },
        { "o_three_point_filtering", three_point_filtering },
        { "srgb_mode", use_srgb },
        { "append_formula", (InvokeFunc)prism_append_formula },
        { "update_floats", (InvokeFunc)update_raw_floats },
    };
    processor.populate(mContext);
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Type = (uint32_t)Ship::ResourceType::Shader;
    init->ByteOrder = Ship::Endianness::Native;
    init->Format = RESOURCE_FORMAT_BINARY;
    const char* shaderName = Fast::gfx_get_shader(cc_features.shader_id);
    std::string path = "shaders/directx/default.shader.hlsl";

    if (nullptr != shaderName) {
        path = std::string(shaderName) + ".hlsl";
    }

    auto res = static_pointer_cast<Ship::Shader>(Ship::Context::GetInstance()->GetResourceManager()->LoadResource(
        "shaders/directx/default.shader.hlsl", true, init));

    if (res == nullptr) {
        SPDLOG_ERROR("Failed to load default directx shader, missing f3d.o2r?");
        abort();
    }

    auto shader = static_cast<std::string*>(res->GetRawPointer());
    processor.load(*shader);
    processor.bind_include_loader(dx_include_fs);
    auto result = processor.process();
    numFloats = raw_numFloats;
    // SPDLOG_INFO("=========== DX11 SHADER ============");
    // SPDLOG_INFO(result);
    // SPDLOG_INFO("====================================");
    return result;
}

// --------------------------------------------------------------------------
// Backbuffer screenshot capture (called via portFastCaptureBackbufferPNG).
//
// Reads the current swap chain back buffer into a CPU-readable staging
// texture, converts to RGBA8, and writes a PNG to the given path.
//
// Returns true on success, false on failure. Never throws — errors are
// logged via SPDLOG_WARN and the function silently fails.
// --------------------------------------------------------------------------
static bool CaptureBackbufferToPNG_DX11(const char* path) {
    if (path == nullptr) {
        return false;
    }
    GfxRenderingAPIDX11* self = sDX11InstanceForCapture;
    if (self == nullptr) {
        return false;
    }
    if (!self->mDevice || !self->mContext || self->mWindowBackend == nullptr) {
        return false;
    }

    IDXGISwapChain1* swap_chain = self->mWindowBackend->GetSwapChain();
    if (swap_chain == nullptr) {
        return false;
    }

    DXGI_SWAP_CHAIN_DESC1 desc = {};
    if (FAILED(swap_chain->GetDesc1(&desc))) {
        return false;
    }

    ComPtr<ID3D11Texture2D> back_buffer;
    if (FAILED(swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                     reinterpret_cast<void**>(back_buffer.GetAddressOf())))) {
        return false;
    }

    D3D11_TEXTURE2D_DESC bb_desc = {};
    back_buffer->GetDesc(&bb_desc);

    // Create a CPU-readable staging texture matching the back buffer.
    D3D11_TEXTURE2D_DESC staging_desc = bb_desc;
    staging_desc.Usage = D3D11_USAGE_STAGING;
    staging_desc.BindFlags = 0;
    staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    staging_desc.MiscFlags = 0;
    staging_desc.SampleDesc.Count = 1;
    staging_desc.SampleDesc.Quality = 0;

    ComPtr<ID3D11Texture2D> staging;
    if (FAILED(self->mDevice->CreateTexture2D(&staging_desc, nullptr, staging.GetAddressOf()))) {
        SPDLOG_WARN("portFastCaptureBackbufferPNG: CreateTexture2D(staging) failed");
        return false;
    }

    // If the back buffer is multisampled, we'd need to resolve first. The port
    // uses no MSAA on the swap chain (BufferCount=3, SampleDesc.Count=1), so
    // CopyResource is fine. If we ever enable MSAA, add ResolveSubresource.
    if (bb_desc.SampleDesc.Count > 1) {
        SPDLOG_WARN("portFastCaptureBackbufferPNG: MSAA back buffer not supported");
        return false;
    }

    self->mContext->CopyResource(staging.Get(), back_buffer.Get());

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (FAILED(self->mContext->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) {
        SPDLOG_WARN("portFastCaptureBackbufferPNG: Map(staging) failed");
        return false;
    }

    const uint32_t w = bb_desc.Width;
    const uint32_t h = bb_desc.Height;
    std::vector<uint8_t> rgba(static_cast<size_t>(w) * h * 4);

    const uint8_t* src_base = reinterpret_cast<const uint8_t*>(mapped.pData);
    bool supported = true;

    // Handle common swap chain formats. Swap chain is created as
    // DXGI_FORMAT_R8G8B8A8_UNORM (see gfx_dxgi.cpp CreateSwapChain), so the
    // RGBA branch is the hot path. BGRA8 is kept as a fallback for safety.
    switch (bb_desc.Format) {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: {
            for (uint32_t y = 0; y < h; ++y) {
                const uint8_t* src_row = src_base + static_cast<size_t>(y) * mapped.RowPitch;
                uint8_t* dst_row = rgba.data() + static_cast<size_t>(y) * w * 4;
                memcpy(dst_row, src_row, static_cast<size_t>(w) * 4);
                // Force alpha to 0xFF so the PNG is fully opaque regardless
                // of what the renderer left in the alpha channel.
                for (uint32_t x = 0; x < w; ++x) {
                    dst_row[x * 4 + 3] = 0xFF;
                }
            }
            break;
        }
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: {
            for (uint32_t y = 0; y < h; ++y) {
                const uint8_t* src_row = src_base + static_cast<size_t>(y) * mapped.RowPitch;
                uint8_t* dst_row = rgba.data() + static_cast<size_t>(y) * w * 4;
                for (uint32_t x = 0; x < w; ++x) {
                    dst_row[x * 4 + 0] = src_row[x * 4 + 2];
                    dst_row[x * 4 + 1] = src_row[x * 4 + 1];
                    dst_row[x * 4 + 2] = src_row[x * 4 + 0];
                    dst_row[x * 4 + 3] = 0xFF;
                }
            }
            break;
        }
        default:
            SPDLOG_WARN("portFastCaptureBackbufferPNG: unsupported format {}",
                        static_cast<int>(bb_desc.Format));
            supported = false;
            break;
    }

    self->mContext->Unmap(staging.Get(), 0);

    if (!supported) {
        return false;
    }

    int ok = stbi_write_png(path, static_cast<int>(w), static_cast<int>(h), 4,
                            rgba.data(), static_cast<int>(w * 4));
    if (ok == 0) {
        SPDLOG_WARN("portFastCaptureBackbufferPNG: stbi_write_png failed for {}", path);
        return false;
    }
    return true;
}

// --- Post-process / user-shader pipeline ----------------------------------
//
// Implemented from the plan in docs/crt_shader_plan_2026-05-11.md §7.1.
// No code copied from RetroArch or any GPL-licensed shader runtime.
//
// D3D11 has an immediate context model (single global pipeline state), so
// unlike Metal there's no enqueue-order question — draw calls execute in
// submission order. The pass binds dst's RTV, src's SRV at t0, a shared
// linear/clamp sampler at s0, and the post-process uniforms at b0; then
// emits a fullscreen triangle synthesized from SV_VertexID with no input
// layout. After the draw, the regular IA/VS/PS/blend state on the
// immediate context is dirty — the next DrawTriangles call resets every
// piece it cares about via the existing LoadShader / mLast* tracking
// path, so we only have to invalidate `mLastShaderProgram` and the
// resource-view cache.

bool GfxRenderingAPIDX11::SupportsPostProcess() {
    return true;
}

int GfxRenderingAPIDX11::CreatePostProcessProgram(const PostProcessSource& src) {
    if (src.hlsl.empty()) {
        SPDLOG_ERROR("Post-process shader '{}' has no HLSL source. The .glsl "
                     "should have been transpiled by PostProcessTranspiler "
                     "at load time — check earlier log for the parse error.",
                     src.name);
        return -1;
    }

#if DEBUG_D3D
    UINT compileFlags = D3DCOMPILE_DEBUG;
#else
    UINT compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL2;
#endif

    ComPtr<ID3DBlob> vsBlob, psBlob, errBlob;
    HRESULT hr = mD3dCompile(src.hlsl.data(), src.hlsl.size(), nullptr, nullptr, nullptr, "VSMain", "vs_4_0",
                             compileFlags, 0, vsBlob.GetAddressOf(), errBlob.GetAddressOf());
    if (FAILED(hr)) {
        SPDLOG_ERROR("Post-process '{}': VSMain compile failed: {}", src.name,
                     errBlob ? (const char*)errBlob->GetBufferPointer() : "(no blob)");
        return -1;
    }
    errBlob.Reset();
    hr = mD3dCompile(src.hlsl.data(), src.hlsl.size(), nullptr, nullptr, nullptr, "PSMain", "ps_4_0", compileFlags, 0,
                     psBlob.GetAddressOf(), errBlob.GetAddressOf());
    if (FAILED(hr)) {
        SPDLOG_ERROR("Post-process '{}': PSMain compile failed: {}", src.name,
                     errBlob ? (const char*)errBlob->GetBufferPointer() : "(no blob)");
        return -1;
    }

    PostProcessProgramD3D11 slot;
    hr = mDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr,
                                     slot.vertex_shader.GetAddressOf());
    if (FAILED(hr)) {
        SPDLOG_ERROR("Post-process '{}': CreateVertexShader failed (hr=0x{:08X})", src.name, (uint32_t)hr);
        return -1;
    }
    hr = mDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr,
                                    slot.pixel_shader.GetAddressOf());
    if (FAILED(hr)) {
        SPDLOG_ERROR("Post-process '{}': CreatePixelShader failed (hr=0x{:08X})", src.name, (uint32_t)hr);
        return -1;
    }
    slot.name = src.name;
    slot.aliasCount = static_cast<uint32_t>(src.aliasNames.size());

    for (size_t i = 0; i < mPostProcessPrograms.size(); ++i) {
        if (mPostProcessPrograms[i].vertex_shader.Get() == nullptr) {
            mPostProcessPrograms[i] = std::move(slot);
            return (int)i;
        }
    }
    mPostProcessPrograms.push_back(std::move(slot));
    return (int)mPostProcessPrograms.size() - 1;
}

void GfxRenderingAPIDX11::DestroyPostProcessProgram(int progId) {
    if (progId < 0 || (size_t)progId >= mPostProcessPrograms.size()) {
        return;
    }
    mPostProcessPrograms[progId] = PostProcessProgramD3D11{};
}

int GfxRenderingAPIDX11::CreatePostProcessStaticTexture(uint32_t width, uint32_t height,
                                                        const uint8_t* rgba8) {
    if (width == 0 || height == 0 || rgba8 == nullptr) {
        return -1;
    }
    D3D11_TEXTURE2D_DESC td{};
    td.Width = width;
    td.Height = height;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA init{};
    init.pSysMem = rgba8;
    init.SysMemPitch = width * 4u;
    StaticTextureD3D11 slot;
    if (FAILED(mDevice->CreateTexture2D(&td, &init, slot.texture.GetAddressOf()))) {
        return -1;
    }
    if (FAILED(mDevice->CreateShaderResourceView(slot.texture.Get(), nullptr,
                                                  slot.srv.GetAddressOf()))) {
        slot.texture.Reset();
        return -1;
    }
    for (size_t i = 0; i < mPostProcessStaticTextures.size(); ++i) {
        if (mPostProcessStaticTextures[i].texture.Get() == nullptr) {
            mPostProcessStaticTextures[i] = std::move(slot);
            return (int)i;
        }
    }
    mPostProcessStaticTextures.push_back(std::move(slot));
    return (int)(mPostProcessStaticTextures.size() - 1);
}

void GfxRenderingAPIDX11::DestroyPostProcessStaticTexture(int textureId) {
    if (textureId < 0 || (size_t)textureId >= mPostProcessStaticTextures.size()) {
        return;
    }
    mPostProcessStaticTextures[textureId] = StaticTextureD3D11{};
}

void GfxRenderingAPIDX11::SetPostProcessFramebufferMipmapped(int fb_id, bool mipmapped) {
    if (fb_id < 0 || (size_t)fb_id >= mFrameBuffers.size()) {
        return;
    }
    mFrameBuffers[fb_id].postProcessMipmapped = mipmapped;
}

void GfxRenderingAPIDX11::GeneratePostProcessMipmaps(int fb_id) {
    if (fb_id < 0 || (size_t)fb_id >= mFrameBuffers.size()) {
        return;
    }
    const FramebufferDX11& fb = mFrameBuffers[fb_id];
    if (fb.texture_id >= mTextures.size()) {
        return;
    }
    ID3D11ShaderResourceView* srv = mTextures[fb.texture_id].resource_view.Get();
    if (srv == nullptr) {
        return;
    }
    // D3D11 requires the SRV's underlying texture to have been
    // created with D3D11_RESOURCE_MISC_GENERATE_MIPS. The chain
    // arranges that via SetPostProcessFramebufferMipmapped before
    // UpdateFramebufferParameters; if the FBO wasn't tagged we
    // silently no-op (matching the OpenGL behavior).
    if (!fb.postProcessMipmapped) {
        return;
    }
    mContext->GenerateMips(srv);
}

void GfxRenderingAPIDX11::SetPostProcessFramebufferFormat(int fb_id, PostProcessFboFormat fmt) {
    if (fb_id < 0 || (size_t)fb_id >= mFrameBuffers.size()) {
        return;
    }
    mFrameBuffers[fb_id].postProcessFormat = fmt;
}

void GfxRenderingAPIDX11::RunPostProcess(int progId, int srcFb, int dstFb, int originalFb,
                                         const PostProcessParams& params) {
    if (progId < 0 || (size_t)progId >= mPostProcessPrograms.size()) {
        return;
    }
    const PostProcessProgramD3D11& slot = mPostProcessPrograms[progId];
    if (slot.vertex_shader.Get() == nullptr) {
        return;
    }
    if (srcFb < 0 || (size_t)srcFb >= mFrameBuffers.size()) {
        return;
    }
    if (dstFb < 0 || (size_t)dstFb >= mFrameBuffers.size()) {
        return;
    }
    FramebufferDX11& srcFbInfo = mFrameBuffers[srcFb];
    FramebufferDX11& dstFbInfo = mFrameBuffers[dstFb];
    if (dstFbInfo.render_target_view.Get() == nullptr) {
        return;
    }
    TextureData& srcTex = mTextures[srcFbInfo.texture_id];
    TextureData& dstTex = mTextures[dstFbInfo.texture_id];
    if (srcTex.resource_view.Get() == nullptr) {
        // MSAA source can't be sampled; the interpreter should have resolved
        // into a non-MSAA target before reaching this path.
        return;
    }

    // Required cbuffer size: 40-byte prefix + 8 bytes per alias /
    // external-texture `vec2 <name>Size` slot, padded to a multiple of
    // 16 for D3D11 alignment. Grow mPostProcessCb on demand if the
    // current shader needs a larger buffer than the last one.
    const size_t aliasBytes = (size_t)slot.aliasCount * kPostProcessUniformsAliasStride;
    const size_t neededBytes =
        (kPostProcessUniformsPrefixBytes + aliasBytes + 15) & ~15u;
    if (mPostProcessCb.Get() == nullptr || mPostProcessCbBytes < neededBytes) {
        mPostProcessCb.Reset();
        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.ByteWidth = (UINT)neededBytes;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        ThrowIfFailed(mDevice->CreateBuffer(&bd, nullptr, mPostProcessCb.GetAddressOf()));
        mPostProcessCbBytes = neededBytes;
    }

    PostProcessUniformsD3D11 uni{};
    uni.SourceSize[0] = (float)params.srcWidth;
    uni.SourceSize[1] = (float)params.srcHeight;
    uni.OutputSize[0] = (float)params.dstWidth;
    uni.OutputSize[1] = (float)params.dstHeight;
    uni.InputSize[0] = (float)params.inputWidth;
    uni.InputSize[1] = (float)params.inputHeight;
    uni.OriginalSize[0] = (float)params.originalWidth;
    uni.OriginalSize[1] = (float)params.originalHeight;
    uni.FrameCount = (int)params.frameCount;
    uni.FrameDirection = 1.0f;
    D3D11_MAPPED_SUBRESOURCE ms{};
    ThrowIfFailed(mContext->Map(mPostProcessCb.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms));
    auto* dst = static_cast<uint8_t*>(ms.pData);
    std::memset(dst, 0, neededBytes);
    std::memcpy(dst, &uni, sizeof(uni));
    // Append each alias / external-texture `<name>Size` at the std140
    // offset matching the transpiled cbuffer's tail. extraBindings[i]
    // shares the same i-th order the transpiler used; if extraBindings
    // is missing an entry for any reason, leave that slot at 1x1
    // (already memset to 0; bump to 1 below so divisions don't NaN).
    for (uint32_t i = 0; i < slot.aliasCount; ++i) {
        float w = 1.0f;
        float h = 1.0f;
        if (i < params.extraBindingsCount) {
            const auto& eb = params.extraBindings[i];
            w = (float)eb.width;
            h = (float)eb.height;
        }
        float* slotPtr = reinterpret_cast<float*>(
            dst + kPostProcessUniformsPrefixBytes + (size_t)i * kPostProcessUniformsAliasStride);
        slotPtr[0] = w;
        slotPtr[1] = h;
    }
    mContext->Unmap(mPostProcessCb.Get(), 0);

    // Bind destination as the sole render target, no depth.
    ID3D11RenderTargetView* rtv = dstFbInfo.render_target_view.Get();
    mContext->OMSetRenderTargets(1, &rtv, nullptr);

    D3D11_VIEWPORT vp{};
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = (float)dstTex.width;
    vp.Height = (float)dstTex.height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    mContext->RSSetViewports(1, &vp);

    // The regular draw path's cached rasterizer state has
    // ScissorEnable=TRUE (see DrawTriangles), and the last RSSetScissorRects
    // call set a rect in the *game framebuffer* coordinate space. We don't
    // own/replace the rasterizer state here, so without re-setting the
    // scissor the fullscreen post-process triangle is clipped to that stale
    // game-FB rect on the differently-sized mDstFb — typically clipping it
    // away entirely and leaving mDstFb black. Set a scissor that covers the
    // full destination so the pass is unclipped (matches the GL/Metal
    // post-process paths, which set/disable scissor explicitly).
    D3D11_RECT ppScissor{};
    ppScissor.left = 0;
    ppScissor.top = 0;
    ppScissor.right = (LONG)dstTex.width;
    ppScissor.bottom = (LONG)dstTex.height;
    mContext->RSSetScissorRects(1, &ppScissor);

    // Fullscreen triangle has no input data.
    mContext->IASetInputLayout(nullptr);
    UINT stride = 0;
    UINT offset = 0;
    ID3D11Buffer* nullVbo = nullptr;
    mContext->IASetVertexBuffers(0, 1, &nullVbo, &stride, &offset);
    mContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    mContext->VSSetShader(slot.vertex_shader.Get(), nullptr, 0);
    mContext->PSSetShader(slot.pixel_shader.Get(), nullptr, 0);

    // Disable blending so the fragment's alpha=1 lands intact.
    const float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    mContext->OMSetBlendState(nullptr, blendFactor, 0xFFFFFFFFu);
    // No depth/stencil state needed (no DSV bound).

    // Source (this pass's input) at t0, Original (game FB) at t1.
    // Shaders that don't reference Original simply ignore the binding;
    // pass 0 callers pass srcFb == originalFb so it's a no-op there.
    ID3D11ShaderResourceView* originalSrv = srcTex.resource_view.Get();
    if (originalFb >= 0 && (size_t)originalFb < mFrameBuffers.size()) {
        FramebufferDX11& origFbInfo = mFrameBuffers[originalFb];
        if (origFbInfo.texture_id < mTextures.size()) {
            ID3D11ShaderResourceView* maybe =
                mTextures[origFbInfo.texture_id].resource_view.Get();
            if (maybe != nullptr) {
                originalSrv = maybe;
            }
        }
    }
    // Slot 0 (Source) sampler comes from the producer pass's
    // libretro filter_linearN / wrap_modeN, encoded as a small cache
    // key. Slot 1 (Original) stays at the default linear / clamp-to-edge
    // because .glslp has no per-pass override for it.
    auto wrapModeToD3D = [](PostProcessWrapMode m) -> D3D11_TEXTURE_ADDRESS_MODE {
        switch (m) {
            case PostProcessWrapMode::ClampToEdge:    return D3D11_TEXTURE_ADDRESS_CLAMP;
            case PostProcessWrapMode::ClampToBorder:  return D3D11_TEXTURE_ADDRESS_BORDER;
            case PostProcessWrapMode::Repeat:         return D3D11_TEXTURE_ADDRESS_WRAP;
            case PostProcessWrapMode::MirroredRepeat: return D3D11_TEXTURE_ADDRESS_MIRROR;
        }
        return D3D11_TEXTURE_ADDRESS_CLAMP;
    };
    auto getSampler = [&](bool linear, PostProcessWrapMode wrap) -> ID3D11SamplerState* {
        const uint32_t key =
            (static_cast<uint32_t>(wrap) << 1) | (linear ? 1u : 0u);
        auto it = mPostProcessSamplers.find(key);
        if (it != mPostProcessSamplers.end()) {
            return it->second.Get();
        }
        D3D11_SAMPLER_DESC sd = {};
        sd.Filter = linear ? D3D11_FILTER_MIN_MAG_MIP_LINEAR
                           : D3D11_FILTER_MIN_MAG_MIP_POINT;
        const D3D11_TEXTURE_ADDRESS_MODE addr = wrapModeToD3D(wrap);
        sd.AddressU = addr;
        sd.AddressV = addr;
        sd.AddressW = addr;
        sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sd.MinLOD = 0;
        sd.MaxLOD = D3D11_FLOAT32_MAX;
        // Border color stays at zeroed RGBA (matches GL's transparent-
        // black default for CLAMP_TO_BORDER).
        Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
        ThrowIfFailed(mDevice->CreateSamplerState(&sd, sampler.GetAddressOf()));
        ID3D11SamplerState* raw = sampler.Get();
        mPostProcessSamplers.emplace(key, std::move(sampler));
        return raw;
    };
    ID3D11SamplerState* srcSamp = getSampler(params.srcFilterLinear, params.srcWrapMode);
    ID3D11SamplerState* origSamp = getSampler(true, PostProcessWrapMode::ClampToEdge);

    // Alias / external-texture bindings at slots 2..N+1. Cap at 8
    // total PS resource slots so we stay well under the D3D11 input
    // resource limit (128) and don't need a dynamic buffer.
    constexpr size_t kMaxPostProcessSlots = 8;
    ID3D11ShaderResourceView* extraSrvs[kMaxPostProcessSlots] = {};
    ID3D11SamplerState* extraSamps[kMaxPostProcessSlots] = {};
    extraSrvs[0] = srcTex.resource_view.Get();
    extraSrvs[1] = originalSrv;
    extraSamps[0] = srcSamp;
    extraSamps[1] = origSamp;
    size_t totalSlots = 2;
    for (size_t i = 0; i < params.extraBindingsCount && totalSlots < kMaxPostProcessSlots; ++i) {
        const auto& eb = params.extraBindings[i];
        ID3D11ShaderResourceView* srv = originalSrv; // defensive fallback
        if (eb.staticTextureId >= 0 &&
            (size_t)eb.staticTextureId < mPostProcessStaticTextures.size()) {
            ID3D11ShaderResourceView* maybe =
                mPostProcessStaticTextures[eb.staticTextureId].srv.Get();
            if (maybe != nullptr) {
                srv = maybe;
            }
        } else if (eb.sourceFb >= 0 && (size_t)eb.sourceFb < mFrameBuffers.size()) {
            const FramebufferDX11& f = mFrameBuffers[eb.sourceFb];
            if (f.texture_id < mTextures.size()) {
                ID3D11ShaderResourceView* maybe = mTextures[f.texture_id].resource_view.Get();
                if (maybe != nullptr) {
                    srv = maybe;
                }
            }
        }
        extraSrvs[totalSlots] = srv;
        extraSamps[totalSlots] = getSampler(eb.filterLinear, eb.wrapMode);
        ++totalSlots;
    }
    mContext->PSSetShaderResources(0, (UINT)totalSlots, extraSrvs);
    mContext->PSSetSamplers(0, (UINT)totalSlots, extraSamps);
    ID3D11Buffer* cb = mPostProcessCb.Get();
    mContext->VSSetConstantBuffers(0, 1, &cb);
    mContext->PSSetConstantBuffers(0, 1, &cb);

    mContext->Draw(3, 0);

    // Invalidate the per-context state caches used by the regular draw
    // path so the next DrawTriangles call rebinds everything that matters
    // (shader, layout, blend, SRVs at slots 0/1, primitive topology).
    mLastShaderProgram = nullptr;
    for (size_t i = 0; i < SHADER_MAX_TEXTURES; ++i) {
        mLastResourceViews[i].Reset();
        mLastSamplerStates[i].Reset();
    }
    mLastBlendState.Reset();
    mLastPrimitaveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    mLastVertexBufferStride = 0;

    // Detach the SRVs so a subsequent pass that wants to render into
    // any of these textures doesn't trip the RTV-and-SRV-bound warning.
    // Includes the alias slots since they may bind earlier-pass FBOs
    // that a later pass writes into via a re-resolve (rare, but the
    // chain doesn't guarantee otherwise).
    ID3D11ShaderResourceView* nullSrvs[kMaxPostProcessSlots] = {};
    mContext->PSSetShaderResources(0, (UINT)totalSlots, nullSrvs);
}

// Phase 3D-3: compile a slang program (authored VS + FS) for the
// D3D11 backend. SPIRV-Cross emits the slang VS attributes at
// TEXCOORD0 (vec4 Position) + TEXCOORD1 (vec2 TexCoord) with default
// options, and the chain provides matching interleaved vertex data
// via mPostProcessSlangVbo at run time.
int GfxRenderingAPIDX11::CreatePostProcessSlangProgram(const PostProcessSlangProgramSource& src) {
    if (src.vsHlsl.empty() || src.fsHlsl.empty()) {
        SPDLOG_ERROR("Slang post-process shader '{}': missing VS or FS HLSL "
                     "(transpiler should have populated both — check earlier log)",
                     src.name);
        return -1;
    }

#if DEBUG_D3D
    UINT compileFlags = D3DCOMPILE_DEBUG;
#else
    UINT compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL2;
#endif

    ComPtr<ID3DBlob> vsBlob, psBlob, errBlob;
    HRESULT hr = mD3dCompile(src.vsHlsl.data(), src.vsHlsl.size(), nullptr, nullptr, nullptr,
                             "VSMain", "vs_5_0", compileFlags, 0,
                             vsBlob.GetAddressOf(), errBlob.GetAddressOf());
    if (FAILED(hr)) {
        SPDLOG_ERROR("Slang post-process '{}': VSMain compile failed: {}", src.name,
                     errBlob ? (const char*)errBlob->GetBufferPointer() : "(no blob)");
        return -1;
    }
    errBlob.Reset();
    hr = mD3dCompile(src.fsHlsl.data(), src.fsHlsl.size(), nullptr, nullptr, nullptr,
                     "PSMain", "ps_5_0", compileFlags, 0,
                     psBlob.GetAddressOf(), errBlob.GetAddressOf());
    if (FAILED(hr)) {
        SPDLOG_ERROR("Slang post-process '{}': PSMain compile failed: {}", src.name,
                     errBlob ? (const char*)errBlob->GetBufferPointer() : "(no blob)");
        return -1;
    }

    PostProcessSlangProgramD3D11 slot;
    hr = mDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr,
                                     slot.vertex_shader.GetAddressOf());
    if (FAILED(hr)) {
        SPDLOG_ERROR("Slang post-process '{}': CreateVertexShader failed (hr=0x{:08X})",
                     src.name, (uint32_t)hr);
        return -1;
    }
    hr = mDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr,
                                    slot.pixel_shader.GetAddressOf());
    if (FAILED(hr)) {
        SPDLOG_ERROR("Slang post-process '{}': CreatePixelShader failed (hr=0x{:08X})",
                     src.name, (uint32_t)hr);
        return -1;
    }

    // Vertex layout matches the slang VBO populated lazily on first
    // run: {vec4 Position, vec2 TexCoord} interleaved, stride 24.
    // SPIRV-Cross's HLSL output uses TEXCOORD<location> for input
    // attribute semantics by default, so location=0 becomes
    // TEXCOORD0 and location=1 becomes TEXCOORD1.
    const D3D11_INPUT_ELEMENT_DESC kSlangLayout[] = {
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,
          D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 16,
          D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = mDevice->CreateInputLayout(kSlangLayout, ARRAYSIZE(kSlangLayout),
                                    vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                    slot.input_layout.GetAddressOf());
    if (FAILED(hr)) {
        SPDLOG_ERROR("Slang post-process '{}': CreateInputLayout failed (hr=0x{:08X})",
                     src.name, (uint32_t)hr);
        return -1;
    }

    if (src.uboBytes > 0) {
        const UINT bytes = (UINT)((src.uboBytes + 15) & ~15u);
        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.ByteWidth = bytes;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        hr = mDevice->CreateBuffer(&bd, nullptr, slot.ubo.GetAddressOf());
        if (FAILED(hr)) {
            SPDLOG_ERROR("Slang post-process '{}': CreateBuffer(ubo {}B) failed (hr=0x{:08X})",
                         src.name, bytes, (uint32_t)hr);
            return -1;
        }
        slot.uboBytes = bytes;
    }
    slot.name = src.name;
    slot.samplerNames = src.samplerNames;

    for (size_t i = 0; i < mPostProcessSlangPrograms.size(); ++i) {
        if (mPostProcessSlangPrograms[i].vertex_shader.Get() == nullptr) {
            mPostProcessSlangPrograms[i] = std::move(slot);
            return (int)i;
        }
    }
    mPostProcessSlangPrograms.push_back(std::move(slot));
    return (int)mPostProcessSlangPrograms.size() - 1;
}

void GfxRenderingAPIDX11::DestroyPostProcessSlangProgram(int progId) {
    if (progId < 0 || (size_t)progId >= mPostProcessSlangPrograms.size()) {
        return;
    }
    mPostProcessSlangPrograms[progId] = PostProcessSlangProgramD3D11{};
}

void GfxRenderingAPIDX11::RunPostProcessSlang(int progId, int dstFb,
                                              const uint8_t* uboData, uint32_t uboBytes,
                                              const int* samplerFbIds, uint32_t samplerCount,
                                              const PostProcessParams& params) {
    if (progId < 0 || (size_t)progId >= mPostProcessSlangPrograms.size()) {
        return;
    }
    PostProcessSlangProgramD3D11& slot = mPostProcessSlangPrograms[progId];
    if (slot.vertex_shader.Get() == nullptr || slot.pixel_shader.Get() == nullptr) {
        return;
    }
    if (dstFb < 0 || (size_t)dstFb >= mFrameBuffers.size()) {
        return;
    }
    FramebufferDX11& dstFbInfo = mFrameBuffers[dstFb];
    if (dstFbInfo.render_target_view.Get() == nullptr) {
        return;
    }
    TextureData& dstTex = mTextures[dstFbInfo.texture_id];

    // Lazily build the slang vertex buffer (fullscreen triangle:
    // 3 vertices of {vec4 Position, vec2 TexCoord}).
    if (mPostProcessSlangVbo.Get() == nullptr) {
        static const float kSlangVerts[] = {
            -1.0f, -1.0f, 0.0f, 1.0f,  0.0f, 0.0f,
             3.0f, -1.0f, 0.0f, 1.0f,  2.0f, 0.0f,
            -1.0f,  3.0f, 0.0f, 1.0f,  0.0f, 2.0f,
        };
        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_IMMUTABLE;
        bd.ByteWidth = sizeof(kSlangVerts);
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA init = {};
        init.pSysMem = kSlangVerts;
        ThrowIfFailed(mDevice->CreateBuffer(&bd, &init, mPostProcessSlangVbo.GetAddressOf()));
    }

    // Upload the chain-built UBO blob into the program's cbuffer.
    if (slot.ubo.Get() != nullptr && uboData != nullptr && uboBytes > 0) {
        D3D11_MAPPED_SUBRESOURCE ms{};
        ThrowIfFailed(mContext->Map(slot.ubo.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms));
        auto* dst = static_cast<uint8_t*>(ms.pData);
        std::memset(dst, 0, slot.uboBytes);
        std::memcpy(dst, uboData, std::min<uint32_t>(uboBytes, slot.uboBytes));
        mContext->Unmap(slot.ubo.Get(), 0);
    }

    // Set destination as the sole render target, no depth.
    ID3D11RenderTargetView* rtv = dstFbInfo.render_target_view.Get();
    mContext->OMSetRenderTargets(1, &rtv, nullptr);

    D3D11_VIEWPORT vp{};
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = (float)dstTex.width;
    vp.Height = (float)dstTex.height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    mContext->RSSetViewports(1, &vp);

    // See RunPostProcess: the inherited rasterizer state has
    // ScissorEnable=TRUE with a stale game-FB scissor rect. Cover the
    // full destination so the slang pass isn't clipped to black.
    D3D11_RECT ppScissor{};
    ppScissor.left = 0;
    ppScissor.top = 0;
    ppScissor.right = (LONG)dstTex.width;
    ppScissor.bottom = (LONG)dstTex.height;
    mContext->RSSetScissorRects(1, &ppScissor);

    // Bind the slang vertex buffer + input layout.
    UINT stride = 6 * sizeof(float); // vec4 + vec2 = 24
    UINT offset = 0;
    ID3D11Buffer* vbo = mPostProcessSlangVbo.Get();
    mContext->IASetInputLayout(slot.input_layout.Get());
    mContext->IASetVertexBuffers(0, 1, &vbo, &stride, &offset);
    mContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    mContext->VSSetShader(slot.vertex_shader.Get(), nullptr, 0);
    mContext->PSSetShader(slot.pixel_shader.Get(), nullptr, 0);

    const float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    mContext->OMSetBlendState(nullptr, blendFactor, 0xFFFFFFFFu);

    // Build a single sampler reused across all slots — Phase 3D-3
    // single-pass scope. Per-slot libretro filter/wrap routing comes
    // with Phase 3D-3 multipass; for now Source-and-Original on
    // linear/clamp matches the legacy behaviour.
    auto wrapModeToD3D = [](PostProcessWrapMode m) -> D3D11_TEXTURE_ADDRESS_MODE {
        switch (m) {
            case PostProcessWrapMode::ClampToEdge:    return D3D11_TEXTURE_ADDRESS_CLAMP;
            case PostProcessWrapMode::ClampToBorder:  return D3D11_TEXTURE_ADDRESS_BORDER;
            case PostProcessWrapMode::Repeat:         return D3D11_TEXTURE_ADDRESS_WRAP;
            case PostProcessWrapMode::MirroredRepeat: return D3D11_TEXTURE_ADDRESS_MIRROR;
        }
        return D3D11_TEXTURE_ADDRESS_CLAMP;
    };
    auto getSampler = [&](bool linear, PostProcessWrapMode wrap) -> ID3D11SamplerState* {
        const uint32_t key =
            (static_cast<uint32_t>(wrap) << 1) | (linear ? 1u : 0u);
        auto it = mPostProcessSamplers.find(key);
        if (it != mPostProcessSamplers.end()) {
            return it->second.Get();
        }
        D3D11_SAMPLER_DESC sd = {};
        sd.Filter = linear ? D3D11_FILTER_MIN_MAG_MIP_LINEAR
                           : D3D11_FILTER_MIN_MAG_MIP_POINT;
        const D3D11_TEXTURE_ADDRESS_MODE addr = wrapModeToD3D(wrap);
        sd.AddressU = addr;
        sd.AddressV = addr;
        sd.AddressW = addr;
        sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sd.MinLOD = 0;
        sd.MaxLOD = D3D11_FLOAT32_MAX;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
        ThrowIfFailed(mDevice->CreateSamplerState(&sd, sampler.GetAddressOf()));
        ID3D11SamplerState* raw = sampler.Get();
        mPostProcessSamplers.emplace(key, std::move(sampler));
        return raw;
    };

    // Bind samplerFbIds[i]'s SRV at slot i. Use the first valid SRV
    // as the fallback for any -1 entries.
    constexpr size_t kMaxSlangSlots = 8;
    ID3D11ShaderResourceView* srvs[kMaxSlangSlots] = {};
    ID3D11SamplerState* samplers[kMaxSlangSlots] = {};
    ID3D11ShaderResourceView* fallbackSrv = nullptr;
    for (uint32_t i = 0; i < samplerCount && i < kMaxSlangSlots; ++i) {
        const int fbId = samplerFbIds ? samplerFbIds[i] : -1;
        ID3D11ShaderResourceView* srv = nullptr;
        if (fbId >= 0 && (size_t)fbId < mFrameBuffers.size()) {
            const FramebufferDX11& f = mFrameBuffers[fbId];
            if (f.texture_id < mTextures.size()) {
                srv = mTextures[f.texture_id].resource_view.Get();
            }
        }
        if (srv != nullptr && fallbackSrv == nullptr) {
            fallbackSrv = srv;
        }
        srvs[i] = srv;
        samplers[i] = getSampler(params.srcFilterLinear, params.srcWrapMode);
    }
    // Fill any nulls with the fallback so the shader's texture()
    // calls don't trip a D3D11 null-SRV bind warning.
    for (uint32_t i = 0; i < samplerCount && i < kMaxSlangSlots; ++i) {
        if (srvs[i] == nullptr) {
            srvs[i] = fallbackSrv;
        }
    }
    const UINT slotCount = std::min<uint32_t>(samplerCount, kMaxSlangSlots);
    if (slotCount > 0) {
        mContext->PSSetShaderResources(0, slotCount, srvs);
        mContext->PSSetSamplers(0, slotCount, samplers);
    }

    // Bind the slang UBO at both b0 stages (VS uses MVP; FS uses
    // SourceSize / OutputSize / parameters).
    ID3D11Buffer* cb = slot.ubo.Get();
    if (cb != nullptr) {
        mContext->VSSetConstantBuffers(0, 1, &cb);
        mContext->PSSetConstantBuffers(0, 1, &cb);
    }

    mContext->Draw(3, 0);

    // Invalidate the regular-draw caches so the next DrawTriangles
    // re-binds everything.
    mLastShaderProgram = nullptr;
    for (size_t i = 0; i < SHADER_MAX_TEXTURES; ++i) {
        mLastResourceViews[i].Reset();
        mLastSamplerStates[i].Reset();
    }
    mLastBlendState.Reset();
    mLastPrimitaveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    mLastVertexBufferStride = 0;

    // Detach SRVs so a later pass writing into any of these textures
    // doesn't trip the RTV-and-SRV-bound warning.
    ID3D11ShaderResourceView* nullSrvs[kMaxSlangSlots] = {};
    if (slotCount > 0) {
        mContext->PSSetShaderResources(0, slotCount, nullSrvs);
    }
}

} // namespace Fast

// C-callable entry point for the port. Defined outside the Fast namespace so
// the symbol is reachable from port/gameloop.cpp without any namespace dance.
extern "C" int portFastCaptureBackbufferPNG(const char* path) {
    try {
        return Fast::CaptureBackbufferToPNG_DX11(path) ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

#else // !ENABLE_DX11

// Stub when DX11 backend is disabled, so the port always has a symbol to link.
extern "C" int portFastCaptureBackbufferPNG(const char* path) {
    (void)path;
    return 0;
}

#endif
