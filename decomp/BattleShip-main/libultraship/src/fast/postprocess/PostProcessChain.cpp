// Implemented from the plan in docs/crt_shader_plan_2026-05-11.md §3.3
// and the libretro .glslp scale-type conventions documented at
// libretro/glsl-shaders. No code copied from RetroArch or any
// GPL-licensed shader runtime.
#include "fast/postprocess/PostProcessChain.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include <spdlog/spdlog.h>

#include "fast/backends/gfx_rendering_api.h"

namespace Fast {

namespace {

// Parse a libretro slang semantic sampler name with a trailing
// non-negative integer suffix (e.g. "OriginalHistory3" -> ("OriginalHistory", 3),
// "PassFeedback0" -> ("PassFeedback", 0)). Returns true on match.
// Used to discover which `OriginalHistory<N>` / `PassFeedback<N>` /
// `PassOutput<N>` indices a slang artifact references so the chain
// can pre-allocate the matching history ring slots and per-pass
// feedback buffers at load time.
bool MatchSemanticIndex(const std::string& name, const std::string& prefix, int& idxOut) {
    if (name.size() <= prefix.size()) {
        return false;
    }
    if (name.compare(0, prefix.size(), prefix) != 0) {
        return false;
    }
    int idx = 0;
    bool any = false;
    for (size_t i = prefix.size(); i < name.size(); ++i) {
        char c = name[i];
        if (c < '0' || c > '9') {
            return false;
        }
        idx = idx * 10 + (c - '0');
        any = true;
    }
    if (!any) {
        return false;
    }
    idxOut = idx;
    return true;
}

// Pack the standard libretro slang semantic values + (Phase 3D-2) zero
// for any unrecognised UBO member into a byte blob whose layout
// matches the artifact's reflected offsets. Unknown members (user
// `#pragma parameter` declarations) get default-initialised to zero;
// Phase 3F plumbs parameter defaults / overrides here.
std::vector<uint8_t> BuildSlangUbo(const PostProcessSlangArtifact& art,
                                   uint32_t srcW, uint32_t srcH,
                                   uint32_t dstW, uint32_t dstH,
                                   uint32_t origW, uint32_t origH,
                                   uint32_t frameCount) {
    std::vector<uint8_t> blob(art.uboTotalBytes, 0);
    if (blob.empty()) {
        return blob;
    }
    auto writeAt = [&](uint32_t offset, const void* data, uint32_t size) {
        if (size == 0) {
            return;
        }
        if (offset + size > blob.size()) {
            size = (offset >= blob.size()) ? 0u : static_cast<uint32_t>(blob.size() - offset);
        }
        if (size > 0) {
            std::memcpy(blob.data() + offset, data, size);
        }
    };
    // libretro's standard semantic block size convention is vec4 for
    // OutputSize/SourceSize/OriginalSize (xy = width/height, zw =
    // 1/width and 1/height) — but glslang gives us the actual declared
    // size via reflection, so we clamp the write to whatever the
    // shader picked (vec2 shaders, while uncommon, still work).
    auto recip = [](float v) -> float {
        return v == 0.0f ? 0.0f : 1.0f / v;
    };
    for (const auto& m : art.uboMembers) {
        if (m.name == "MVP") {
            static constexpr float kIdentity[16] = {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f,
            };
            writeAt(m.offsetBytes, kIdentity,
                    std::min<uint32_t>(m.sizeBytes, sizeof(kIdentity)));
        } else if (m.name == "OutputSize") {
            const float v[4] = { static_cast<float>(dstW), static_cast<float>(dstH),
                                 recip(static_cast<float>(dstW)),
                                 recip(static_cast<float>(dstH)) };
            writeAt(m.offsetBytes, v, std::min<uint32_t>(m.sizeBytes, sizeof(v)));
        } else if (m.name == "SourceSize") {
            const float v[4] = { static_cast<float>(srcW), static_cast<float>(srcH),
                                 recip(static_cast<float>(srcW)),
                                 recip(static_cast<float>(srcH)) };
            writeAt(m.offsetBytes, v, std::min<uint32_t>(m.sizeBytes, sizeof(v)));
        } else if (m.name == "OriginalSize") {
            const float v[4] = { static_cast<float>(origW), static_cast<float>(origH),
                                 recip(static_cast<float>(origW)),
                                 recip(static_cast<float>(origH)) };
            writeAt(m.offsetBytes, v, std::min<uint32_t>(m.sizeBytes, sizeof(v)));
        } else if (m.name == "FrameCount") {
            writeAt(m.offsetBytes, &frameCount,
                    std::min<uint32_t>(m.sizeBytes, sizeof(uint32_t)));
        }
        // Unknown members (user parameters etc.) stay zero. The
        // blob's underlying storage is already zero-initialised.
    }
    return blob;
}

uint32_t AxisSize(PostProcessScaleType type, float scale, uint32_t source, uint32_t viewport) {
    float pixels = 1.0f;
    switch (type) {
        case PostProcessScaleType::Source:
            pixels = static_cast<float>(source) * scale;
            break;
        case PostProcessScaleType::Viewport:
            pixels = static_cast<float>(viewport) * scale;
            break;
        case PostProcessScaleType::Absolute:
            pixels = scale;
            break;
    }
    const long rounded = std::lround(pixels);
    if (rounded < 1) {
        return 1u;
    }
    return static_cast<uint32_t>(rounded);
}

} // namespace

void PostProcessChain::Init(GfxRenderingAPI* rapi) {
    if (rapi == nullptr) {
        return;
    }
    mBackendSupported = rapi->SupportsPostProcess();
    if (!mBackendSupported) {
        return;
    }
    mDstFb = rapi->CreateFramebuffer();
    mDstWidth = 0;
    mDstHeight = 0;
}

void PostProcessChain::OnResize(GfxRenderingAPI* rapi, uint32_t dstWidth, uint32_t dstHeight) {
    if (rapi == nullptr || !mBackendSupported || mDstFb < 0) {
        return;
    }
    if (dstWidth == 0 || dstHeight == 0) {
        return;
    }
    if (mDstWidth == dstWidth && mDstHeight == dstHeight) {
        return;
    }
    // Sampleable single-sample color, no depth — this is the final
    // output surface the GUI consumes via ImGui::Image.
    rapi->UpdateFramebufferParameters(mDstFb, dstWidth, dstHeight,
                                      /*msaa_level=*/1,
                                      /*opengl_invertY=*/false,
                                      /*render_target=*/true,
                                      /*has_depth_buffer=*/false,
                                      /*can_extract_depth=*/false);
    mDstWidth = dstWidth;
    mDstHeight = dstHeight;
}

bool PostProcessChain::LoadShader(GfxRenderingAPI* rapi, const PostProcessSource& src) {
    PostProcessPresetPass cfg; // Defaults: scaleType=Source, scale=1.0, filter=false.
    return LoadPasses(rapi, { src }, { cfg }, {});
}

bool PostProcessChain::LoadPasses(GfxRenderingAPI* rapi,
                                  const std::vector<PostProcessSource>& sources,
                                  const std::vector<PostProcessPresetPass>& configs,
                                  const std::vector<PostProcessShaderExternalTexture>& externalTextures) {
    if (rapi == nullptr || !mBackendSupported) {
        return false;
    }
    if (sources.empty() || sources.size() != configs.size()) {
        return false;
    }

    // Compile all passes up front so a partial failure doesn't leave
    // the chain in a half-loaded state — if any pass fails, we tear
    // down what we built and keep the prior shader.
    std::vector<Pass> staged;
    staged.reserve(sources.size());
    for (size_t i = 0; i < sources.size(); ++i) {
        const int prog = rapi->CreatePostProcessProgram(sources[i]);
        if (prog < 0) {
            // Roll back the programs we already compiled.
            for (auto& p : staged) {
                rapi->DestroyPostProcessProgram(p.programId);
                if (p.outputFb >= 0) {
                    rapi->DestroyFramebuffer(p.outputFb);
                }
            }
            return false;
        }
        Pass p;
        p.programId = prog;
        p.config = configs[i];
        // Phase 2.2: mark this pass's output as mip-allocated if the
        // next pass has `mipmap_input = true`. Pass i's mipmapInput
        // means pass i samples its Source (pass i-1's output) with
        // mip filtering; we set the producer's flag here so the
        // backend reserves mip storage before the FBO is sized.
        if (i + 1 < configs.size() && configs[i + 1].mipmapInput) {
            p.outputMipmapped = true;
        }
        // Intermediate passes (not the last) own a chain-managed FBO.
        // The last pass writes into mDstFb so the GUI can sample it —
        // mDstFb always stays Default-format because ImGui::Image
        // consumes it as an LDR RGBA8 texture.
        if (i + 1 < sources.size()) {
            p.outputFb = rapi->CreateFramebuffer();
            if (p.outputMipmapped) {
                rapi->SetPostProcessFramebufferMipmapped(p.outputFb, true);
            }
            // libretro `srgb_framebufferN` / `float_framebufferN`. v1
            // treats them as mutually exclusive (the spec doesn't
            // define behavior when both are set); float wins so HDR
            // bloom passes that incorrectly also tag sRGB still get
            // the float storage they need.
            PostProcessFboFormat fmt = PostProcessFboFormat::Default;
            if (p.config.floatFramebuffer) {
                fmt = PostProcessFboFormat::Float16;
            } else if (p.config.srgbFramebuffer) {
                fmt = PostProcessFboFormat::Srgb;
            }
            rapi->SetPostProcessFramebufferFormat(p.outputFb, fmt);
        }
        staged.push_back(std::move(p));
    }

    // Build the ordered alias list. Each pass with a non-empty
    // cfg.alias declares a binding name that any later pass can sample
    // via `uniform sampler2D <name>` at slot 2 + alias_index. First-
    // declared wins on name collision (libretro spec leaves the
    // collision undefined; we pick the deterministic option).
    std::vector<Alias> stagedAliases;
    stagedAliases.reserve(configs.size());
    for (size_t i = 0; i < configs.size(); ++i) {
        const std::string& a = configs[i].alias;
        if (a.empty()) {
            continue;
        }
        bool dup = false;
        for (const Alias& existing : stagedAliases) {
            if (existing.name == a) {
                dup = true;
                break;
            }
        }
        if (!dup) {
            stagedAliases.push_back({ a, i });
        }
    }

    // Upload external textures via the backend's static-texture API.
    // Roll back the just-staged programs / FBOs if any upload fails
    // so the chain doesn't end up half-loaded with phantom slots.
    std::vector<ExternalTexture> stagedTextures;
    stagedTextures.reserve(externalTextures.size());
    for (const auto& tex : externalTextures) {
        if (tex.width == 0 || tex.height == 0 || tex.rgba8.empty()) {
            for (auto& p : staged) {
                rapi->DestroyPostProcessProgram(p.programId);
                if (p.outputFb >= 0) {
                    rapi->DestroyFramebuffer(p.outputFb);
                }
            }
            for (auto& st : stagedTextures) {
                rapi->DestroyPostProcessStaticTexture(st.textureId);
            }
            return false;
        }
        const int id = rapi->CreatePostProcessStaticTexture(tex.width, tex.height,
                                                            tex.rgba8.data());
        if (id < 0) {
            for (auto& p : staged) {
                rapi->DestroyPostProcessProgram(p.programId);
                if (p.outputFb >= 0) {
                    rapi->DestroyFramebuffer(p.outputFb);
                }
            }
            for (auto& st : stagedTextures) {
                rapi->DestroyPostProcessStaticTexture(st.textureId);
            }
            return false;
        }
        ExternalTexture et;
        et.name = tex.name;
        et.textureId = id;
        et.width = tex.width;
        et.height = tex.height;
        et.filterLinear = tex.filterLinear;
        et.wrapMode = tex.wrapMode;
        stagedTextures.push_back(std::move(et));
    }

    UnloadShader(rapi);
    mPasses = std::move(staged);
    mAliases = std::move(stagedAliases);
    mExternalTextures = std::move(stagedTextures);
    mLoadedName = sources.front().name; // Preset / shader display name.

    // Diagnostic surface: one INFO line per successful chain load that
    // enumerates each pass so users debugging "the shader looks wrong"
    // can confirm what actually compiled and how it's chained. Tests
    // in docs/crt_shader_testing_protocol.md key off these lines.
    SPDLOG_INFO("Post-process chain loaded (legacy/.glsl): '{}' — {} pass(es), {} alias(es), {} external texture(s)",
                mLoadedName, mPasses.size(), mAliases.size(), mExternalTextures.size());
    Fast::internal::SetPostProcessRuntimeActive(mLoadedName, "legacy", mPasses.size(),
                                                /*resolvedPath=*/"");
    for (size_t i = 0; i < mPasses.size(); ++i) {
        const PostProcessPresetPass& cfg = mPasses[i].config;
        auto scaleStr = [](PostProcessScaleType t) -> const char* {
            switch (t) {
                case PostProcessScaleType::Source:   return "source";
                case PostProcessScaleType::Viewport: return "viewport";
                case PostProcessScaleType::Absolute: return "absolute";
            }
            return "?";
        };
        const char* fmt = "default";
        if (cfg.floatFramebuffer)      fmt = "float16";
        else if (cfg.srgbFramebuffer)  fmt = "srgb";
        SPDLOG_INFO("  pass {}: prog={} scaleX={}({:.3f}) scaleY={}({:.3f}) filter_linear={} wrap={} mipmap_input={} alias='{}' fb={} fmt={}",
                    i, mPasses[i].programId,
                    scaleStr(cfg.scaleTypeX), cfg.scaleX,
                    scaleStr(cfg.scaleTypeY), cfg.scaleY,
                    cfg.filterLinear ? "true" : "false",
                    static_cast<int>(cfg.wrapMode),
                    cfg.mipmapInput ? "true" : "false",
                    cfg.alias,
                    mPasses[i].outputFb,
                    fmt);
    }
    return true;
}

void PostProcessChain::UnloadShader(GfxRenderingAPI* rapi) {
    if (rapi == nullptr) {
        return;
    }
    if (!mLoadedName.empty()) {
        SPDLOG_INFO("Post-process chain: unloading '{}'", mLoadedName);
        Fast::internal::SetPostProcessRuntimeInactive();
    }
    for (auto& p : mPasses) {
        if (p.programId >= 0 && mBackendSupported) {
            rapi->DestroyPostProcessProgram(p.programId);
        }
        if (p.outputFb >= 0 && mBackendSupported) {
            rapi->DestroyFramebuffer(p.outputFb);
        }
    }
    for (auto& tex : mExternalTextures) {
        if (tex.textureId >= 0 && mBackendSupported) {
            rapi->DestroyPostProcessStaticTexture(tex.textureId);
        }
    }
    for (auto& sp : mSlangPasses) {
        if (sp.programId >= 0 && mBackendSupported) {
            rapi->DestroyPostProcessSlangProgram(sp.programId);
        }
        if (sp.outputFb >= 0 && mBackendSupported) {
            rapi->DestroyFramebuffer(sp.outputFb);
        }
        if (sp.feedbackFb >= 0 && mBackendSupported) {
            rapi->DestroyFramebuffer(sp.feedbackFb);
        }
    }
    for (int fb : mOriginalHistoryFbs) {
        if (fb >= 0 && mBackendSupported) {
            rapi->DestroyFramebuffer(fb);
        }
    }
    mPasses.clear();
    mAliases.clear();
    mExternalTextures.clear();
    mSlangPasses.clear();
    mOriginalHistoryFbs.clear();
    mOriginalHistoryWidth = 0;
    mOriginalHistoryHeight = 0;
    mFrameIndex = 0;
    mLoadedName.clear();
}

bool PostProcessChain::LoadSlangPasses(GfxRenderingAPI* rapi,
                                       const std::vector<PostProcessSlangArtifact>& artifacts,
                                       const std::vector<PostProcessPresetPass>& configs,
                                       const std::vector<std::string>& diagnosticNames) {
    if (rapi == nullptr || !mBackendSupported) {
        return false;
    }
    if (artifacts.empty() || artifacts.size() != configs.size()) {
        return false;
    }
    if (!diagnosticNames.empty() && diagnosticNames.size() != artifacts.size()) {
        return false;
    }

    // Stage everything up front; a partial failure rolls back what we
    // built and leaves the chain on whatever was loaded before.
    std::vector<SlangPass> staged;
    staged.reserve(artifacts.size());
    for (size_t i = 0; i < artifacts.size(); ++i) {
        const PostProcessSlangArtifact& art = artifacts[i];
        PostProcessSlangProgramSource src;
        src.name = diagnosticNames.empty() ? std::string() : diagnosticNames[i];
        src.vsGlsl = art.vertex.glsl;
        src.fsGlsl = art.fragment.glsl;
        src.vsHlsl = art.vertex.hlsl;
        src.fsHlsl = art.fragment.hlsl;
        src.vsMsl  = art.vertex.msl;
        src.fsMsl  = art.fragment.msl;
        src.uboBytes = art.uboTotalBytes;
        src.samplerNames.reserve(art.samplers.size());
        for (const auto& sampler : art.samplers) {
            src.samplerNames.push_back(sampler.name);
        }
        const int prog = rapi->CreatePostProcessSlangProgram(src);
        if (prog < 0) {
            // Backend either doesn't support slang or compile failed.
            // Roll back what we already built.
            for (auto& s : staged) {
                rapi->DestroyPostProcessSlangProgram(s.programId);
                if (s.outputFb >= 0) {
                    rapi->DestroyFramebuffer(s.outputFb);
                }
            }
            return false;
        }
        SlangPass p;
        p.programId = prog;
        p.config = configs[i];
        p.artifact = art;
        // Intermediate passes own a chain-managed FBO; the last
        // pass writes into mDstFb (allocated at Init time and sized
        // by OnResize), matching the legacy mPasses convention.
        if (i + 1 < artifacts.size()) {
            p.outputFb = rapi->CreateFramebuffer();
            PostProcessFboFormat fmt = PostProcessFboFormat::Default;
            if (p.config.floatFramebuffer) {
                fmt = PostProcessFboFormat::Float16;
            } else if (p.config.srgbFramebuffer) {
                fmt = PostProcessFboFormat::Srgb;
            }
            rapi->SetPostProcessFramebufferFormat(p.outputFb, fmt);
        }
        staged.push_back(std::move(p));
    }

    // Phase 3E: scan every artifact's samplers to discover which
    // libretro semantic slots the chain needs to provide:
    //   - OriginalHistory<N>: pre-allocate N game-FB-sized ring slots
    //     so the chain can route the N-frames-ago game FB to the
    //     shader. (OriginalHistory0 == live game FB; needs no slot.)
    //   - PassFeedback<N>: mark pass N as needing a feedback FBO so
    //     this frame's draw can ping-pong against last frame's output.
    int maxHistoryIdx = 0;
    std::vector<bool> passNeedsFeedback(staged.size(), false);
    for (const SlangPass& sp : staged) {
        for (const auto& sampler : sp.artifact.samplers) {
            int idx = 0;
            if (MatchSemanticIndex(sampler.name, "OriginalHistory", idx)) {
                if (idx > maxHistoryIdx) {
                    maxHistoryIdx = idx;
                }
                continue;
            }
            if (MatchSemanticIndex(sampler.name, "PassFeedback", idx)) {
                if (idx >= 0 && (size_t)idx < staged.size()) {
                    passNeedsFeedback[idx] = true;
                }
                continue;
            }
        }
    }
    std::vector<int> stagedHistoryFbs;
    stagedHistoryFbs.reserve(maxHistoryIdx);
    for (int i = 0; i < maxHistoryIdx; ++i) {
        stagedHistoryFbs.push_back(rapi->CreateFramebuffer());
    }
    for (size_t i = 0; i < staged.size(); ++i) {
        if (passNeedsFeedback[i]) {
            staged[i].feedbackFb = rapi->CreateFramebuffer();
        }
    }

    UnloadShader(rapi);
    mSlangPasses = std::move(staged);
    mOriginalHistoryFbs = std::move(stagedHistoryFbs);
    mOriginalHistoryWidth = 0;
    mOriginalHistoryHeight = 0;
    mFrameIndex = 0;
    if (!diagnosticNames.empty()) {
        mLoadedName = diagnosticNames.front();
    }

    SPDLOG_INFO("Post-process chain loaded (.slang): '{}' — {} pass(es), {} history slot(s)",
                mLoadedName, mSlangPasses.size(), mOriginalHistoryFbs.size());
    Fast::internal::SetPostProcessRuntimeActive(mLoadedName, "slang", mSlangPasses.size(),
                                                /*resolvedPath=*/"");
    for (size_t i = 0; i < mSlangPasses.size(); ++i) {
        const PostProcessPresetPass& cfg = mSlangPasses[i].config;
        SPDLOG_INFO("  slang pass {}: prog={} alias='{}' filter_linear={} fb={} feedback_fb={}",
                    i, mSlangPasses[i].programId, cfg.alias,
                    cfg.filterLinear ? "true" : "false",
                    mSlangPasses[i].outputFb,
                    mSlangPasses[i].feedbackFb);
    }
    return true;
}

int PostProcessChain::Run(GfxRenderingAPI* rapi, int srcFb, const PostProcessParams& params) {
    if (!IsActive() || mDstWidth == 0 || mDstHeight == 0) {
        return srcFb;
    }

    // Phase 3D-2: if slang passes are loaded, run them and return.
    // Legacy and slang chains are mutually exclusive on a single
    // LoadPasses / LoadSlangPasses call — UnloadShader runs between
    // them — so we don't have to interleave.
    if (!mSlangPasses.empty()) {
        const int originalFb = srcFb;
        const uint32_t originalWidth = params.srcWidth;
        const uint32_t originalHeight = params.srcHeight;

        // Phase 3E: capture this frame's game FB into the history
        // ring before any pass runs, so later samplers reading
        // OriginalHistory<N> see well-defined data. We always write
        // into the slot (mFrameIndex - 1) % size — i.e. the slot
        // that would be sampled as OriginalHistory1 by this frame's
        // shaders. Equivalent reasoning: bumping mFrameIndex at the
        // start "ages out" the oldest entry and frees up the now-
        // newest slot. Skipped entirely when no shader referenced
        // OriginalHistory (mOriginalHistoryFbs is empty).
        if (!mOriginalHistoryFbs.empty()) {
            mFrameIndex += 1;
            const size_t ringSize = mOriginalHistoryFbs.size();
            const size_t writeSlot = static_cast<size_t>(mFrameIndex - 1) % ringSize;
            // Each history FBO sized to match the game FB so the
            // copy lands without scaling. Reallocate when the source
            // FB dimensions change between frames (window resize +
            // matched-resolution path).
            if (mOriginalHistoryWidth != originalWidth ||
                mOriginalHistoryHeight != originalHeight) {
                for (int fb : mOriginalHistoryFbs) {
                    rapi->UpdateFramebufferParameters(fb,
                                                       originalWidth, originalHeight,
                                                       /*msaa_level=*/1,
                                                       /*opengl_invertY=*/false,
                                                       /*render_target=*/true,
                                                       /*has_depth_buffer=*/false,
                                                       /*can_extract_depth=*/false);
                }
                mOriginalHistoryWidth = originalWidth;
                mOriginalHistoryHeight = originalHeight;
            }
            rapi->CopyFramebuffer(mOriginalHistoryFbs[writeSlot], originalFb,
                                  0, 0,
                                  static_cast<int>(originalWidth),
                                  static_cast<int>(originalHeight),
                                  0, 0,
                                  static_cast<int>(originalWidth),
                                  static_cast<int>(originalHeight));
        }

        int curIn = srcFb;
        uint32_t curW = params.srcWidth;
        uint32_t curH = params.srcHeight;
        const size_t lastIdx = mSlangPasses.size() - 1;
        // Reusable scratch storage so we don't churn allocators per
        // pass — each pass writes its own samplerFbIds prefix.
        std::vector<int> samplerFbIds;
        for (size_t i = 0; i < mSlangPasses.size(); ++i) {
            SlangPass& sp = mSlangPasses[i];
            const bool isLast = (i == lastIdx);
            uint32_t outW, outH;
            int outFb;
            if (isLast) {
                outW = mDstWidth;
                outH = mDstHeight;
                outFb = mDstFb;
            } else {
                outW = AxisSize(sp.config.scaleTypeX, sp.config.scaleX, curW, params.dstWidth);
                outH = AxisSize(sp.config.scaleTypeY, sp.config.scaleY, curH, params.dstHeight);
                if (outW != sp.lastWidth || outH != sp.lastHeight) {
                    rapi->UpdateFramebufferParameters(sp.outputFb, outW, outH,
                                                      /*msaa_level=*/1,
                                                      /*opengl_invertY=*/false,
                                                      /*render_target=*/true,
                                                      /*has_depth_buffer=*/false,
                                                      /*can_extract_depth=*/false);
                    if (sp.feedbackFb >= 0) {
                        rapi->UpdateFramebufferParameters(sp.feedbackFb, outW, outH,
                                                          /*msaa_level=*/1,
                                                          /*opengl_invertY=*/false,
                                                          /*render_target=*/true,
                                                          /*has_depth_buffer=*/false,
                                                          /*can_extract_depth=*/false);
                    }
                    sp.lastWidth = outW;
                    sp.lastHeight = outH;
                }
                outFb = sp.outputFb;
            }

            uint32_t framePassed = params.frameCount;
            if (sp.config.frameCountMod > 0) {
                framePassed = params.frameCount %
                              static_cast<uint32_t>(sp.config.frameCountMod);
            }

            std::vector<uint8_t> uboBlob = BuildSlangUbo(
                sp.artifact, curW, curH, outW, outH,
                originalWidth, originalHeight, framePassed);

            samplerFbIds.assign(sp.artifact.samplers.size(), -1);
            for (size_t s = 0; s < sp.artifact.samplers.size(); ++s) {
                const std::string& n = sp.artifact.samplers[s].name;
                if (n == "Source") {
                    samplerFbIds[s] = curIn;
                    continue;
                }
                if (n == "Original") {
                    samplerFbIds[s] = originalFb;
                    continue;
                }
                int idx = 0;
                if (MatchSemanticIndex(n, "OriginalHistory", idx)) {
                    // K=0 is the live game FB; K>=1 reads from the
                    // ring. If the ring is shallower than requested
                    // (shouldn't happen because load-time scan sized
                    // it), leave the slot at -1 and the backend
                    // picks a fallback.
                    if (idx == 0) {
                        samplerFbIds[s] = originalFb;
                    } else if (idx <= static_cast<int>(mOriginalHistoryFbs.size())) {
                        const size_t ringSize = mOriginalHistoryFbs.size();
                        const size_t readSlot =
                            static_cast<size_t>(mFrameIndex - static_cast<uint64_t>(idx)) % ringSize;
                        samplerFbIds[s] = mOriginalHistoryFbs[readSlot];
                    }
                    continue;
                }
                if (MatchSemanticIndex(n, "PassFeedback", idx)) {
                    if (idx >= 0 && (size_t)idx < mSlangPasses.size()) {
                        const SlangPass& producer = mSlangPasses[idx];
                        if (producer.feedbackFb >= 0) {
                            samplerFbIds[s] = producer.feedbackFb;
                        }
                    }
                    continue;
                }
                if (MatchSemanticIndex(n, "PassOutput", idx)) {
                    // PassOutput<N>: this frame's pass N output.
                    // Only valid when pass N has already run (N < i);
                    // self-reference and forward reference fall back
                    // to whatever feedback / -1 routing produces.
                    if (idx >= 0 && (size_t)idx < i) {
                        samplerFbIds[s] = mSlangPasses[idx].outputFb;
                    } else if (idx >= 0 && (size_t)idx < mSlangPasses.size() &&
                               mSlangPasses[idx].feedbackFb >= 0) {
                        samplerFbIds[s] = mSlangPasses[idx].feedbackFb;
                    }
                    continue;
                }
                // Aliases / external textures fall through unbound
                // for slang — Phase 3F+ wires them.
            }

            PostProcessParams pp = params;
            pp.srcWidth = curW;
            pp.srcHeight = curH;
            pp.originalWidth = originalWidth;
            pp.originalHeight = originalHeight;
            pp.dstWidth = outW;
            pp.dstHeight = outH;
            pp.frameCount = framePassed;
            rapi->RunPostProcessSlang(sp.programId, outFb,
                                      uboBlob.empty() ? nullptr : uboBlob.data(),
                                      static_cast<uint32_t>(uboBlob.size()),
                                      samplerFbIds.empty() ? nullptr : samplerFbIds.data(),
                                      static_cast<uint32_t>(samplerFbIds.size()),
                                      pp);

            curIn = outFb;
            curW = outW;
            curH = outH;
        }

        // Phase 3E: ping-pong each feedback pass's outputFb /
        // feedbackFb at end-of-frame so next frame's draw lands in
        // the (now-stale) slot and this frame's output survives as
        // PassFeedback<idx> for the next pass. lastWidth/lastHeight
        // stay correct because both FBOs are sized identically.
        for (auto& sp : mSlangPasses) {
            if (sp.feedbackFb >= 0) {
                std::swap(sp.outputFb, sp.feedbackFb);
            }
        }
        return mDstFb;
    }

    // The chain's "original" FB stays pinned to the game FB across
    // every pass — that's the sampler 1 binding (`Original`) libretro
    // multipass shaders use to combine the post-bloom buffer with the
    // pre-bloom source. Source rotates pass-to-pass; Original does not.
    const int originalFb = srcFb;
    const uint32_t originalWidth = params.srcWidth;
    const uint32_t originalHeight = params.srcHeight;

    int curIn = srcFb;
    uint32_t curW = params.srcWidth;
    uint32_t curH = params.srcHeight;
    const size_t lastIdx = mPasses.size() - 1;

    // Per-pass extraBindings buffer. Stable storage across the call so
    // each pass can hand the backend a pointer that outlives the
    // RunPostProcess body. Sized once at (alias count + external-
    // texture count). Aliases occupy slots 0..N-1, external textures
    // slots N..N+M-1 (matching PostProcessSourceLoader's aliasNames
    // order). Entries with both sourceFb == -1 and staticTextureId ==
    // -1 signal "not yet produced at this pass index" so backends can
    // fall back safely.
    const size_t aliasN = mAliases.size();
    const size_t texN = mExternalTextures.size();
    std::vector<PostProcessExtraBinding> extraBindings(aliasN + texN);
    for (size_t i = 0; i < aliasN; ++i) {
        extraBindings[i].name = mAliases[i].name;
    }
    for (size_t j = 0; j < texN; ++j) {
        const ExternalTexture& tex = mExternalTextures[j];
        auto& b = extraBindings[aliasN + j];
        b.name = tex.name;
        b.staticTextureId = tex.textureId;
        b.width = tex.width;
        b.height = tex.height;
        b.filterLinear = tex.filterLinear;
        b.wrapMode = tex.wrapMode;
    }

    // libretro semantics: `filter_linearN` and `wrap_modeN` apply to
    // pass N's OUTPUT when later passes sample it. So pass i's `Source`
    // sampler comes from pass[i-1].config. Pass 0 reads from the game
    // FB, which has no producer pass in the chain; fall back to the
    // backend default the params already carry (linear, clamp-to-edge).
    bool                srcFilter = params.srcFilterLinear;
    PostProcessWrapMode srcWrap   = params.srcWrapMode;

    for (size_t i = 0; i < mPasses.size(); ++i) {
        Pass& p = mPasses[i];
        const bool isLast = (i == lastIdx);

        // Phase 2.2: if this pass requested mipmap sampling on its
        // input, refresh the producer's mip chain. The producer is
        // pass[i-1] (or the chain-managed final-output FBO if pass
        // i-1 == lastIdx, but pass 0 is the only pass with no
        // chain-owned input; we can't mip the game FB so log once
        // and fall through with srcUseMipmap=false).
        bool useMipmap = false;
        if (p.config.mipmapInput) {
            if (i == 0) {
                static bool warned = false;
                if (!warned) {
                    SPDLOG_WARN("Post-process: mipmap_input on pass 0 is not supported "
                                "(game FB has no mip storage); sampling level 0 only.");
                    warned = true;
                }
            } else {
                rapi->GeneratePostProcessMipmaps(curIn);
                useMipmap = true;
            }
        }

        // Refresh alias bindings for any alias produced by a pass we
        // already ran (producerPassIdx < i). The producer's outputFb is
        // its chain-managed intermediate (or mDstFb if the producer is
        // the last pass — but the last pass has no later consumers, so
        // that branch only matters for malformed presets and falls back
        // to the producer's own input).
        for (size_t a = 0; a < mAliases.size(); ++a) {
            const Alias& al = mAliases[a];
            if (al.producerPassIdx < i) {
                const Pass& prod = mPasses[al.producerPassIdx];
                extraBindings[a].sourceFb = (al.producerPassIdx == lastIdx) ? mDstFb : prod.outputFb;
                extraBindings[a].width = prod.lastWidth;
                extraBindings[a].height = prod.lastHeight;
                extraBindings[a].filterLinear = prod.config.filterLinear;
                extraBindings[a].wrapMode = prod.config.wrapMode;
            }
            // else: alias's producer is this pass or a later one —
            // leave sourceFb = -1 so the backend falls back to black.
        }

        uint32_t outW, outH;
        int outFb;
        if (isLast) {
            // Last pass renders straight into the chain output, which
            // is already sized at viewport dimensions.
            outW = mDstWidth;
            outH = mDstHeight;
            outFb = mDstFb;
        } else {
            outW = AxisSize(p.config.scaleTypeX, p.config.scaleX, curW, params.dstWidth);
            outH = AxisSize(p.config.scaleTypeY, p.config.scaleY, curH, params.dstHeight);
            if (outW != p.lastWidth || outH != p.lastHeight) {
                rapi->UpdateFramebufferParameters(p.outputFb, outW, outH,
                                                  /*msaa_level=*/1,
                                                  /*opengl_invertY=*/false,
                                                  /*render_target=*/true,
                                                  /*has_depth_buffer=*/false,
                                                  /*can_extract_depth=*/false);
                p.lastWidth = outW;
                p.lastHeight = outH;
            }
            outFb = p.outputFb;
        }

        PostProcessParams pp = params;
        pp.srcWidth = curW;
        pp.srcHeight = curH;
        pp.originalWidth = originalWidth;
        pp.originalHeight = originalHeight;
        pp.dstWidth = outW;
        pp.dstHeight = outH;
        pp.srcFilterLinear = srcFilter;
        pp.srcWrapMode     = srcWrap;
        pp.srcUseMipmap    = useMipmap;
        // libretro `frame_count_modN`: clamps FrameCount to a bounded
        // period so scanline-crawl / bayer-dither shaders that index into
        // a fixed-length pattern see the counter cycle 0..mod-1 forever
        // rather than drift toward UINT32_MAX. Zero (the parser default)
        // means "no modulo" — pass through unchanged.
        if (p.config.frameCountMod > 0) {
            pp.frameCount = params.frameCount %
                            static_cast<uint32_t>(p.config.frameCountMod);
        }
        pp.extraBindings = extraBindings.empty() ? nullptr : extraBindings.data();
        pp.extraBindingsCount = extraBindings.size();
        rapi->RunPostProcess(p.programId, curIn, outFb, originalFb, pp);

        curIn = outFb;
        curW = outW;
        curH = outH;
        // The pass we just ran becomes the producer for the next pass's
        // Source sampler.
        srcFilter = p.config.filterLinear;
        srcWrap   = p.config.wrapMode;
    }
    return mDstFb;
}

bool PostProcessChain::IsActive() const {
    return mBackendSupported && (!mPasses.empty() || !mSlangPasses.empty()) &&
           mDstFb >= 0;
}

} // namespace Fast
