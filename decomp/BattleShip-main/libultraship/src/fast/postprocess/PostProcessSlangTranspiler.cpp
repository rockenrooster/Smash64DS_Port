// Implemented against the public glslang + SPIRV-Cross C++ APIs and
// the libretro slang shader format docs. No code copied from
// RetroArch or any GPL-licensed shader runtime.

#include "fast/postprocess/PostProcessSlangTranspiler.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

#ifdef LUS_POSTPROCESS_TRANSPILER
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>
#include <spirv_cross.hpp>
#include <spirv_glsl.hpp>
#include <spirv_hlsl.hpp>
#include <spirv_msl.hpp>
#endif

namespace Fast {

#ifdef LUS_POSTPROCESS_TRANSPILER

namespace {

bool sGlslangInitialized = false;

bool EnsureGlslangInitialized() {
    if (sGlslangInitialized) {
        return true;
    }
    if (!glslang::InitializeProcess()) {
        return false;
    }
    sGlslangInitialized = true;
    return true;
}

// Some public `.slang` files omit `#version 450`; prepend a default
// so glslang accepts them. The check looks for the literal `#version`
// token at the start of a non-whitespace line.
std::string EnsureVersionDirective(const std::string& src) {
    size_t pos = 0;
    while (pos < src.size()) {
        // Skip leading whitespace on the line.
        while (pos < src.size() && (src[pos] == ' ' || src[pos] == '\t')) {
            ++pos;
        }
        if (pos >= src.size()) {
            break;
        }
        if (src[pos] == '\n' || src[pos] == '\r') {
            ++pos;
            continue;
        }
        if (src.compare(pos, 8, "#version") == 0) {
            return src;
        }
        break;
    }
    return std::string("#version 450\n") + src;
}

bool CompileStageToSpirv(EShLanguage stage, const std::string& source,
                         std::vector<unsigned int>& spirv, std::string& errOut) {
    if (!EnsureGlslangInitialized()) {
        errOut = "glslang::InitializeProcess() failed";
        return false;
    }
    const std::string prepped = EnsureVersionDirective(source);
    glslang::TShader shader(stage);
    const char* strs[] = { prepped.c_str() };
    shader.setStrings(strs, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);
    shader.setEntryPoint("main");
    shader.setSourceEntryPoint("main");
    const TBuiltInResource* resources = GetDefaultResources();
    if (!shader.parse(resources, 100, false, EShMsgDefault)) {
        errOut = std::string("slang ") +
                 (stage == EShLangVertex ? "vertex" : "fragment") +
                 " parse failed: " + shader.getInfoLog();
        return false;
    }
    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(EShMsgDefault)) {
        errOut = std::string("slang ") +
                 (stage == EShLangVertex ? "vertex" : "fragment") +
                 " link failed: " + program.getInfoLog();
        return false;
    }
    glslang::SpvOptions opts;
    glslang::GlslangToSpv(*program.getIntermediate(stage), spirv, &opts);
    if (spirv.empty()) {
        errOut = "SPIR-V module is empty";
        return false;
    }
    return true;
}

// Pull the std140 offset + logical size of every member of a UBO or
// push-constant block. Walks the block's struct type and queries each
// member's offset via spv reflection.
void CollectBlockMembers(const spirv_cross::Compiler& compiler,
                         const spirv_cross::Resource& block,
                         std::vector<PostProcessSlangUboMember>& out,
                         uint32_t& totalBytes) {
    const auto& type = compiler.get_type(block.base_type_id);
    for (uint32_t i = 0; i < static_cast<uint32_t>(type.member_types.size()); ++i) {
        PostProcessSlangUboMember m;
        m.name = compiler.get_member_name(block.base_type_id, i);
        m.offsetBytes = compiler.type_struct_member_offset(type, i);
        // Logical declared size of the member (mat4 → 64, vec4 → 16,
        // etc.). For arrays SPIRV-Cross gives the array stride; we
        // multiply by the array length.
        const auto& memberType = compiler.get_type(type.member_types[i]);
        size_t memberSize = compiler.get_declared_struct_member_size(type, i);
        m.sizeBytes = static_cast<uint32_t>(memberSize);
        (void)memberType; // Unused; reserved for future arrayness checks.
        const uint32_t end = m.offsetBytes + m.sizeBytes;
        if (end > totalBytes) {
            totalBytes = end;
        }
        out.push_back(std::move(m));
    }
    // Sort by offset for predictable iteration order downstream.
    std::sort(out.begin(), out.end(),
              [](const PostProcessSlangUboMember& a, const PostProcessSlangUboMember& b) {
                  return a.offsetBytes < b.offsetBytes;
              });
}

bool ReflectFragment(const std::vector<unsigned int>& spirvFrag,
                     PostProcessSlangArtifact& out, std::string& errOut) {
    try {
        spirv_cross::Compiler reflector(spirvFrag);
        auto resources = reflector.get_shader_resources();
        if (resources.uniform_buffers.size() + resources.push_constant_buffers.size() > 1) {
            errOut = "slang shader declares more than one uniform/push block "
                     "(Phase 3C supports a single UBO or single push_constant)";
            return false;
        }
        out.uboMembers.clear();
        out.uboTotalBytes = 0;
        if (resources.uniform_buffers.size() == 1) {
            CollectBlockMembers(reflector, resources.uniform_buffers[0],
                                out.uboMembers, out.uboTotalBytes);
        } else if (resources.push_constant_buffers.size() == 1) {
            CollectBlockMembers(reflector, resources.push_constant_buffers[0],
                                out.uboMembers, out.uboTotalBytes);
        }
        out.samplers.clear();
        out.samplers.reserve(resources.sampled_images.size());
        for (const auto& img : resources.sampled_images) {
            PostProcessSlangSamplerBinding b;
            b.name = reflector.get_name(img.id);
            b.descriptorSet =
                reflector.get_decoration(img.id, spv::DecorationDescriptorSet);
            b.binding = reflector.get_decoration(img.id, spv::DecorationBinding);
            out.samplers.push_back(std::move(b));
        }
        // Sort samplers by (set, binding) for stable iteration.
        std::sort(out.samplers.begin(), out.samplers.end(),
                  [](const PostProcessSlangSamplerBinding& a,
                     const PostProcessSlangSamplerBinding& b) {
                      if (a.descriptorSet != b.descriptorSet) {
                          return a.descriptorSet < b.descriptorSet;
                      }
                      return a.binding < b.binding;
                  });
        return true;
    } catch (const std::exception& e) {
        errOut = std::string("SPIRV-Cross reflect: ") + e.what();
        return false;
    }
}

bool EmitGlsl(EShLanguage stage, const std::vector<unsigned int>& spirv,
              std::string& outGlsl, std::string& errOut) {
    try {
        spirv_cross::CompilerGLSL compiler(spirv);
        spirv_cross::CompilerGLSL::Options opts;
        opts.version = 330;
        opts.es = false;
        opts.vulkan_semantics = false;
        opts.enable_420pack_extension = false;
        compiler.set_common_options(opts);
        // Suppress `layout(binding = N)` decorations on the GL3.3
        // output — GLSL 330 doesn't accept binding= on sampler/UBO
        // declarations; the chain binds by name via glUniform1i /
        // glUniformBlockBinding instead.
        auto resources = compiler.get_shader_resources();
        for (auto& img : resources.sampled_images) {
            compiler.unset_decoration(img.id, spv::DecorationBinding);
            compiler.unset_decoration(img.id, spv::DecorationDescriptorSet);
        }
        for (auto& ubo : resources.uniform_buffers) {
            compiler.unset_decoration(ubo.id, spv::DecorationBinding);
            compiler.unset_decoration(ubo.id, spv::DecorationDescriptorSet);
        }
        (void)stage;
        outGlsl = compiler.compile();
        return true;
    } catch (const std::exception& e) {
        errOut = std::string("SPIRV-Cross GLSL: ") + e.what();
        return false;
    }
}

bool EmitHlsl(EShLanguage stage, const std::vector<unsigned int>& spirv,
              std::string& outHlsl, std::string& errOut) {
    try {
        spirv_cross::CompilerHLSL compiler(spirv);
        spirv_cross::CompilerHLSL::Options opts;
        opts.shader_model = 50;
        opts.use_entry_point_name = true;
        compiler.set_hlsl_options(opts);
        // Force each UBO to b0 and samplers to t0+/s0+ in declaration
        // order; the chain (Phase 3D) supplies them in the same order.
        auto resources = compiler.get_shader_resources();
        uint32_t uboSlot = 0;
        for (auto& ubo : resources.uniform_buffers) {
            compiler.set_decoration(ubo.id, spv::DecorationDescriptorSet, 0);
            compiler.set_decoration(ubo.id, spv::DecorationBinding, uboSlot++);
        }
        for (auto& push : resources.push_constant_buffers) {
            compiler.set_decoration(push.id, spv::DecorationDescriptorSet, 0);
            compiler.set_decoration(push.id, spv::DecorationBinding, uboSlot++);
        }
        // Sort sampled images by their existing (set,binding) and
        // remap to flat slots 0..N-1.
        std::vector<spirv_cross::Resource> imgs(resources.sampled_images.begin(),
                                                resources.sampled_images.end());
        std::sort(imgs.begin(), imgs.end(),
                  [&compiler](const spirv_cross::Resource& a, const spirv_cross::Resource& b) {
                      const uint32_t aSet = compiler.get_decoration(a.id, spv::DecorationDescriptorSet);
                      const uint32_t bSet = compiler.get_decoration(b.id, spv::DecorationDescriptorSet);
                      if (aSet != bSet) {
                          return aSet < bSet;
                      }
                      return compiler.get_decoration(a.id, spv::DecorationBinding) <
                             compiler.get_decoration(b.id, spv::DecorationBinding);
                  });
        for (uint32_t i = 0; i < static_cast<uint32_t>(imgs.size()); ++i) {
            compiler.set_decoration(imgs[i].id, spv::DecorationDescriptorSet, 0);
            compiler.set_decoration(imgs[i].id, spv::DecorationBinding, i);
        }
        if (stage == EShLangVertex) {
            compiler.rename_entry_point("main", "VSMain", spv::ExecutionModelVertex);
        } else {
            compiler.rename_entry_point("main", "PSMain", spv::ExecutionModelFragment);
        }
        outHlsl = compiler.compile();
        return true;
    } catch (const std::exception& e) {
        errOut = std::string("SPIRV-Cross HLSL: ") + e.what();
        return false;
    }
}

bool EmitMsl(EShLanguage stage, const std::vector<unsigned int>& spirv,
             std::string& outMsl, std::string& errOut) {
    try {
        spirv_cross::CompilerMSL compiler(spirv);
        spirv_cross::CompilerMSL::Options opts;
        opts.platform = spirv_cross::CompilerMSL::Options::macOS;
        opts.set_msl_version(2, 2);
        opts.argument_buffers = false;
        compiler.set_msl_options(opts);

        const spv::ExecutionModel execModel =
            (stage == EShLangVertex) ? spv::ExecutionModelVertex : spv::ExecutionModelFragment;

        auto resources = compiler.get_shader_resources();

        // Assign UBO to buffer(0) and remaining buffer-class blocks
        // sequentially (buffer(1), buffer(2), ...).
        uint32_t bufferSlot = 0;
        for (auto& ubo : resources.uniform_buffers) {
            spirv_cross::MSLResourceBinding bind{};
            bind.stage = execModel;
            bind.desc_set = compiler.get_decoration(ubo.id, spv::DecorationDescriptorSet);
            bind.binding = compiler.get_decoration(ubo.id, spv::DecorationBinding);
            bind.msl_buffer = bufferSlot++;
            bind.msl_texture = 0;
            bind.msl_sampler = 0;
            compiler.add_msl_resource_binding(bind);
        }
        for (auto& push : resources.push_constant_buffers) {
            spirv_cross::MSLResourceBinding bind{};
            bind.stage = execModel;
            bind.desc_set = compiler.get_decoration(push.id, spv::DecorationDescriptorSet);
            bind.binding = compiler.get_decoration(push.id, spv::DecorationBinding);
            bind.msl_buffer = bufferSlot++;
            bind.msl_texture = 0;
            bind.msl_sampler = 0;
            compiler.add_msl_resource_binding(bind);
        }

        // Assign textures/samplers in (set, binding) order to slots 0..N-1.
        std::vector<spirv_cross::Resource> imgs(resources.sampled_images.begin(),
                                                resources.sampled_images.end());
        std::sort(imgs.begin(), imgs.end(),
                  [&compiler](const spirv_cross::Resource& a, const spirv_cross::Resource& b) {
                      const uint32_t aSet = compiler.get_decoration(a.id, spv::DecorationDescriptorSet);
                      const uint32_t bSet = compiler.get_decoration(b.id, spv::DecorationDescriptorSet);
                      if (aSet != bSet) {
                          return aSet < bSet;
                      }
                      return compiler.get_decoration(a.id, spv::DecorationBinding) <
                             compiler.get_decoration(b.id, spv::DecorationBinding);
                  });
        for (uint32_t i = 0; i < static_cast<uint32_t>(imgs.size()); ++i) {
            spirv_cross::MSLResourceBinding bind{};
            bind.stage = execModel;
            bind.desc_set = compiler.get_decoration(imgs[i].id, spv::DecorationDescriptorSet);
            bind.binding = compiler.get_decoration(imgs[i].id, spv::DecorationBinding);
            bind.msl_buffer = 0;
            bind.msl_texture = i;
            bind.msl_sampler = i;
            compiler.add_msl_resource_binding(bind);
        }

        if (stage == EShLangVertex) {
            compiler.rename_entry_point("main", "postprocess_vertex", execModel);
        } else {
            compiler.rename_entry_point("main", "postprocess_fragment", execModel);
        }
        outMsl = compiler.compile();
        return true;
    } catch (const std::exception& e) {
        errOut = std::string("SPIRV-Cross MSL: ") + e.what();
        return false;
    }
}

} // namespace

bool PostProcessSlangTranspiler::Compile(const PostProcessSlangSource& src,
                                         PostProcessSlangArtifact& out,
                                         std::string& errOut) {
    out = PostProcessSlangArtifact{};
    if (src.vertex.empty() || src.fragment.empty()) {
        errOut = "slang source missing vertex or fragment stage";
        return false;
    }

    std::vector<unsigned int> vsSpirv;
    if (!CompileStageToSpirv(EShLangVertex, src.vertex, vsSpirv, errOut)) {
        return false;
    }
    std::vector<unsigned int> fsSpirv;
    if (!CompileStageToSpirv(EShLangFragment, src.fragment, fsSpirv, errOut)) {
        return false;
    }

    // Reflect off the fragment stage — slang's UBO + samplers all live
    // in the fragment scope by convention. (Vertex declares them too,
    // but the layout is shared.)
    if (!ReflectFragment(fsSpirv, out, errOut)) {
        return false;
    }

    if (!EmitGlsl(EShLangVertex, vsSpirv, out.vertex.glsl, errOut)) {
        return false;
    }
    if (!EmitGlsl(EShLangFragment, fsSpirv, out.fragment.glsl, errOut)) {
        return false;
    }
    if (!EmitHlsl(EShLangVertex, vsSpirv, out.vertex.hlsl, errOut)) {
        return false;
    }
    if (!EmitHlsl(EShLangFragment, fsSpirv, out.fragment.hlsl, errOut)) {
        return false;
    }
    if (!EmitMsl(EShLangVertex, vsSpirv, out.vertex.msl, errOut)) {
        return false;
    }
    if (!EmitMsl(EShLangFragment, fsSpirv, out.fragment.msl, errOut)) {
        return false;
    }
    return true;
}

#else  // LUS_POSTPROCESS_TRANSPILER

bool PostProcessSlangTranspiler::Compile(const PostProcessSlangSource& /*src*/,
                                         PostProcessSlangArtifact& /*out*/,
                                         std::string& errOut) {
    errOut = "post-process transpiler disabled at build time";
    return false;
}

#endif // LUS_POSTPROCESS_TRANSPILER

} // namespace Fast
