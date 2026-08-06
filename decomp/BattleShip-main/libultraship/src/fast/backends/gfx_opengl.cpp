#include "ship/window/Window.h"
#ifdef ENABLE_OPENGL

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <map>
#include <unordered_map>

#ifndef _LANGUAGE_C
#define _LANGUAGE_C
#endif

#ifdef __MINGW32__
#define FOR_WINDOWS 1
#else
#define FOR_WINDOWS 0
#endif

#include "fast/backends/gfx_opengl.h"
#include "ship/window/gui/Gui.h"
#include <prism/processor.h>
#include <fstream>
#include "ship/Context.h"
#include "ship/resource/factory/ShaderFactory.h"
#include "fast/interpreter.h"
#include "ship/config/ConsoleVariable.h"

namespace Fast {
int GfxRenderingAPIOGL::GetMaxTextureSize() {
    GLint max_texture_size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    return max_texture_size;
}

const char* GfxRenderingAPIOGL::GetName() {
    return "OpenGL";
}

GfxClipParameters GfxRenderingAPIOGL::GetClipParameters() {
    return { false, mFrameBuffers[mCurrentFrameBuffer].invertY };
}

static void VertexArraySetAttribs(ShaderProgram* prg) {
    size_t numFloats = prg->numFloats;
    size_t pos = 0;

    for (int i = 0; i < prg->numAttribs; i++) {
        if (prg->attribLocations[i] >= 0) {
            glEnableVertexAttribArray(prg->attribLocations[i]);
            glVertexAttribPointer(prg->attribLocations[i], prg->attribSizes[i], GL_FLOAT, GL_FALSE,
                                  numFloats * sizeof(float), (void*)(pos * sizeof(float)));
        }
        pos += prg->attribSizes[i];
    }
}

void GfxRenderingAPIOGL::SetUniforms(ShaderProgram* prg) const {
    glUniform1i(prg->frameCountLocation, mFrameCount);
    glUniform1f(prg->noiseScaleLocation, mCurrentNoiseScale);
}

void GfxRenderingAPIOGL::SetPerDrawUniforms() {
    if (mCurrentShaderProgram->usedTextures[0] || mCurrentShaderProgram->usedTextures[1]) {
        GLint filtering[2] = { textures[mCurrentTextureIds[0]].filtering, textures[mCurrentTextureIds[1]].filtering };
        glUniform1iv(mCurrentShaderProgram->texture_filtering_location, 2, filtering);

        GLint width[2] = { textures[mCurrentTextureIds[0]].width, textures[mCurrentTextureIds[1]].width };
        glUniform1iv(mCurrentShaderProgram->texture_width_location, 2, width);

        GLint height[2] = { textures[mCurrentTextureIds[0]].height, textures[mCurrentTextureIds[1]].height };
        glUniform1iv(mCurrentShaderProgram->texture_height_location, 2, height);
    }
}

void GfxRenderingAPIOGL::UnloadShader(ShaderProgram* old_prg) {
    if (old_prg != nullptr && old_prg == mLastLoadedShader) {
        for (unsigned int i = 0; i < old_prg->numAttribs; i++) {
            if (old_prg->attribLocations[i] >= 0) {
                glDisableVertexAttribArray(old_prg->attribLocations[i]);
            }
        }
        mLastLoadedShader = nullptr;
    }
}

void GfxRenderingAPIOGL::LoadShader(ShaderProgram* new_prg) {
    // if (!new_prg) return;
    mCurrentShaderProgram = new_prg;
    if (new_prg != mLastLoadedShader) {
        glUseProgram(new_prg->openglProgramId);
        VertexArraySetAttribs(new_prg);
        mLastLoadedShader = new_prg;
    }
    SetUniforms(new_prg);
}

#define RAND_NOISE "((random(vec3(floor(gl_FragCoord.xy * noise_scale), float(frame_count))) + 1.0) / 2.0)"

static const char* shader_item_to_str(uint32_t item, bool with_alpha, bool only_alpha, bool inputs_have_alpha,
                                      bool first_cycle, bool hint_single_element) {
    if (!only_alpha) {
        switch (item) {
            case SHADER_0:
                return with_alpha ? "vec4(0.0, 0.0, 0.0, 0.0)" : "vec3(0.0, 0.0, 0.0)";
            case SHADER_1:
                return with_alpha ? "vec4(1.0, 1.0, 1.0, 1.0)" : "vec3(1.0, 1.0, 1.0)";
            case SHADER_INPUT_1:
                return with_alpha || !inputs_have_alpha ? "vInput1" : "vInput1.rgb";
            case SHADER_INPUT_2:
                return with_alpha || !inputs_have_alpha ? "vInput2" : "vInput2.rgb";
            case SHADER_INPUT_3:
                return with_alpha || !inputs_have_alpha ? "vInput3" : "vInput3.rgb";
            case SHADER_INPUT_4:
                return with_alpha || !inputs_have_alpha ? "vInput4" : "vInput4.rgb";
            case SHADER_TEXEL0:
                return first_cycle ? (with_alpha ? "texVal0" : "texVal0.rgb")
                                   : (with_alpha ? "texVal1" : "texVal1.rgb");
            case SHADER_TEXEL0A:
                return first_cycle
                           ? (hint_single_element ? "texVal0.a"
                                                  : (with_alpha ? "vec4(texVal0.a, texVal0.a, texVal0.a, texVal0.a)"
                                                                : "vec3(texVal0.a, texVal0.a, texVal0.a)"))
                           : (hint_single_element ? "texVal1.a"
                                                  : (with_alpha ? "vec4(texVal1.a, texVal1.a, texVal1.a, texVal1.a)"
                                                                : "vec3(texVal1.a, texVal1.a, texVal1.a)"));
            case SHADER_TEXEL1A:
                return first_cycle
                           ? (hint_single_element ? "texVal1.a"
                                                  : (with_alpha ? "vec4(texVal1.a, texVal1.a, texVal1.a, texVal1.a)"
                                                                : "vec3(texVal1.a, texVal1.a, texVal1.a)"))
                           : (hint_single_element ? "texVal0.a"
                                                  : (with_alpha ? "vec4(texVal0.a, texVal0.a, texVal0.a, texVal0.a)"
                                                                : "vec3(texVal0.a, texVal0.a, texVal0.a)"));
            case SHADER_TEXEL1:
                return first_cycle ? (with_alpha ? "texVal1" : "texVal1.rgb")
                                   : (with_alpha ? "texVal0" : "texVal0.rgb");
            case SHADER_COMBINED:
                return with_alpha ? "texel" : "texel.rgb";
            case SHADER_NOISE:
                return with_alpha ? "vec4(" RAND_NOISE ", " RAND_NOISE ", " RAND_NOISE ", " RAND_NOISE ")"
                                  : "vec3(" RAND_NOISE ", " RAND_NOISE ", " RAND_NOISE ")";
        }
    } else {
        switch (item) {
            case SHADER_0:
                return "0.0";
            case SHADER_1:
                return "1.0";
            case SHADER_INPUT_1:
                return "vInput1.a";
            case SHADER_INPUT_2:
                return "vInput2.a";
            case SHADER_INPUT_3:
                return "vInput3.a";
            case SHADER_INPUT_4:
                return "vInput4.a";
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
    return "";
}

bool get_bool(prism::ContextTypes* value) {
    if (std::holds_alternative<int>(*value)) {
        return std::get<int>(*value) == 1;
    }
    return false;
}

prism::ContextTypes* append_formula(prism::ContextTypes* _, prism::ContextTypes* a_arg, prism::ContextTypes* a_single,
                                    prism::ContextTypes* a_mult, prism::ContextTypes* a_mix,
                                    prism::ContextTypes* a_with_alpha, prism::ContextTypes* a_only_alpha,
                                    prism::ContextTypes* a_alpha, prism::ContextTypes* a_first_cycle) {
    auto c = std::get<prism::MTDArray<int>>(*a_arg);
    bool do_single = get_bool(a_single);
    bool do_multiply = get_bool(a_mult);
    bool do_mix = get_bool(a_mix);
    bool with_alpha = get_bool(a_with_alpha);
    bool only_alpha = get_bool(a_only_alpha);
    bool opt_alpha = get_bool(a_alpha);
    bool first_cycle = get_bool(a_first_cycle);
    std::string out = "";
    if (do_single) {
        out += shader_item_to_str(c.at(only_alpha, 3), with_alpha, only_alpha, opt_alpha, first_cycle, false);
    } else if (do_multiply) {
        out += shader_item_to_str(c.at(only_alpha, 0), with_alpha, only_alpha, opt_alpha, first_cycle, false);
        out += " * ";
        out += shader_item_to_str(c.at(only_alpha, 2), with_alpha, only_alpha, opt_alpha, first_cycle, true);
    } else if (do_mix) {
        out += "mix(";
        out += shader_item_to_str(c.at(only_alpha, 1), with_alpha, only_alpha, opt_alpha, first_cycle, false);
        out += ", ";
        out += shader_item_to_str(c.at(only_alpha, 0), with_alpha, only_alpha, opt_alpha, first_cycle, false);
        out += ", ";
        out += shader_item_to_str(c.at(only_alpha, 2), with_alpha, only_alpha, opt_alpha, first_cycle, true);
        out += ")";
    } else {
        out += "(";
        out += shader_item_to_str(c.at(only_alpha, 0), with_alpha, only_alpha, opt_alpha, first_cycle, false);
        out += " - ";
        out += shader_item_to_str(c.at(only_alpha, 1), with_alpha, only_alpha, opt_alpha, first_cycle, false);
        out += ") * ";
        out += shader_item_to_str(c.at(only_alpha, 2), with_alpha, only_alpha, opt_alpha, first_cycle, true);
        out += " + ";
        out += shader_item_to_str(c.at(only_alpha, 3), with_alpha, only_alpha, opt_alpha, first_cycle, false);
    }
    return new prism::ContextTypes{ out };
}

std::optional<std::string> opengl_include_fs(const std::string& path) {
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Type = (uint32_t)Ship::ResourceType::Shader;
    init->ByteOrder = Ship::Endianness::Native;
    init->Format = RESOURCE_FORMAT_BINARY;
    auto res = std::static_pointer_cast<Ship::Shader>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResource(path, true, init));
    if (res == nullptr) {
        return std::nullopt;
    }
    auto inc = static_cast<std::string*>(res->GetRawPointer());
    return *inc;
}

std::string GfxRenderingAPIOGL::BuildFsShader(const CCFeatures& cc_features) {
    prism::Processor processor;
    prism::ContextItems mContext = {
        { "VERTEX_SHADER", false },
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
        { "FILTER_THREE_POINT", FILTER_THREE_POINT },
        { "FILTER_LINEAR", FILTER_LINEAR },
        { "FILTER_NONE", FILTER_NONE },
        { "srgb_mode", mSrgbMode },
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
        { "o_three_point_filtering", mCurrentFilterMode == FILTER_THREE_POINT },
        { "append_formula", (InvokeFunc)append_formula },
#ifdef __APPLE__
        { "GLSL_VERSION", "#version 410 core" },
        { "attr", "in" },
        { "opengles", false },
        { "core_opengl", true },
        { "texture", "texture" },
        { "vOutColor", "vOutColor" },
#elif defined(USE_OPENGLES)
        { "GLSL_VERSION", "#version 300 es\nprecision mediump float;" },
        { "attr", "in" },
        { "opengles", true },
        { "core_opengl", false },
        { "texture", "texture" },
        { "vOutColor", "vOutColor" },
#else
        { "GLSL_VERSION", "#version 130" },
        { "attr", "varying" },
        { "opengles", false },
        { "core_opengl", false },
        { "texture", "texture2D" },
        { "vOutColor", "gl_FragColor" },
#endif
    };
    processor.populate(mContext);
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Type = (uint32_t)Ship::ResourceType::Shader;
    init->ByteOrder = Ship::Endianness::Native;
    init->Format = RESOURCE_FORMAT_BINARY;
    const char* shaderName = Fast::gfx_get_shader(cc_features.shader_id);
    std::string path = "shaders/opengl/default.shader.glsl";

    if (nullptr != shaderName) {
        path = std::string(shaderName) + ".glsl";
    }

    auto res = static_pointer_cast<Ship::Shader>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResource(path, true, init));

    if (res == nullptr) {
        SPDLOG_ERROR("Failed to load default fragment shader, missing f3d.o2r?");
        abort();
    }

    auto shader = static_cast<std::string*>(res->GetRawPointer());
    processor.load(*shader);
    processor.bind_include_loader(opengl_include_fs);
    auto result = processor.process();
    // SPDLOG_INFO("=========== FRAGMENT SHADER ============");
    // SPDLOG_INFO(result);
    // SPDLOG_INFO("========================================");
    return result;
}

static size_t numFloats = 0;

static prism::ContextTypes* UpdateFloats(prism::ContextTypes* _, prism::ContextTypes* num) {
    numFloats += std::get<int>(*num);
    return nullptr;
}

static std::string BuildVsShader(const CCFeatures& cc_features) {
    numFloats = 4;
    prism::Processor processor;
    prism::ContextItems mContext = { { "VERTEX_SHADER", true },
                                     { "o_textures", M_ARRAY(cc_features.usedTextures, bool, 2) },
                                     { "o_clamp", M_ARRAY(cc_features.clamp, bool, 2, 2) },
                                     { "o_fog", cc_features.opt_fog },
                                     { "o_grayscale", cc_features.opt_grayscale },
                                     { "o_alpha", cc_features.opt_alpha },
                                     { "o_inputs", cc_features.numInputs },
                                     { "update_floats", (InvokeFunc)UpdateFloats },
#ifdef __APPLE__
                                     { "GLSL_VERSION", "#version 410 core" },
                                     { "attr", "in" },
                                     { "out", "out" },
                                     { "opengles", false }
#elif defined(USE_OPENGLES)
                                     { "GLSL_VERSION", "#version 300 es" },
                                     { "attr", "in" },
                                     { "out", "out" },
                                     { "opengles", true }
#else
                                     { "GLSL_VERSION", "#version 110" },
                                     { "attr", "attribute" },
                                     { "out", "varying" },
                                     { "opengles", false }
#endif
    };
    processor.populate(mContext);

    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Type = (uint32_t)Ship::ResourceType::Shader;
    init->ByteOrder = Ship::Endianness::Native;
    init->Format = RESOURCE_FORMAT_BINARY;
    const char* shaderName = Fast::gfx_get_shader(cc_features.shader_id);
    std::string path = "shaders/opengl/default.shader.glsl";

    if (nullptr != shaderName) {
        path = std::string(shaderName) + ".glsl";
    }

    auto res = static_pointer_cast<Ship::Shader>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResource(path, true, init));

    if (res == nullptr) {
        SPDLOG_ERROR("Failed to load default vertex shader, missing f3d.o2r?");
        abort();
    }

    auto shader = static_cast<std::string*>(res->GetRawPointer());
    processor.load(*shader);
    processor.bind_include_loader(opengl_include_fs);
    auto result = processor.process();
    // SPDLOG_INFO("=========== VERTEX SHADER ============");
    // SPDLOG_INFO(result);
    // SPDLOG_INFO("========================================");
    return result;
}

ShaderProgram* GfxRenderingAPIOGL::CreateAndLoadNewShader(uint64_t shader_id0, uint64_t shader_id1) {
    CCFeatures cc_features;
    gfx_cc_get_features(shader_id0, shader_id1, &cc_features);
    const auto fs_buf = BuildFsShader(cc_features);
    const auto vs_buf = BuildVsShader(cc_features);
    const GLchar* sources[2] = { vs_buf.data(), fs_buf.data() };
    const GLint lengths[2] = { (GLint)vs_buf.size(), (GLint)fs_buf.size() };
    GLint success;

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &sources[0], &lengths[0]);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint max_length = 0;
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &max_length);
        char error_log[1024];
        // fprintf(stderr, "Vertex shader compilation failed\n");
        glGetShaderInfoLog(vertex_shader, max_length, &max_length, &error_log[0]);
        // fprintf(stderr, "%s\n", &error_log[0]);
        abort();
    }

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &sources[1], &lengths[1]);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint max_length = 0;
        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &max_length);
        char error_log[1024];
        fprintf(stderr, "Fragment shader compilation failed\n");
        glGetShaderInfoLog(fragment_shader, max_length, &max_length, &error_log[0]);
        fprintf(stderr, "%s\n", &error_log[0]);
        abort();
    }

    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    size_t cnt = 0;

    struct ShaderProgram* prg = &mShaderProgramPool[std::make_pair(shader_id0, shader_id1)];
    prg->attribLocations[cnt] = glGetAttribLocation(shader_program, "aVtxPos");
    prg->attribSizes[cnt] = 4;
    ++cnt;

    for (int i = 0; i < 2; i++) {
        if (cc_features.usedTextures[i]) {
            char name[32];
            snprintf(name, sizeof(name), "aTexCoord%d", i);
            prg->attribLocations[cnt] = glGetAttribLocation(shader_program, name);
            prg->attribSizes[cnt] = 2;
            ++cnt;

            for (int j = 0; j < 2; j++) {
                if (cc_features.clamp[i][j]) {
                    snprintf(name, sizeof(name), "aTexClamp%s%d", j == 0 ? "S" : "T", i);
                    prg->attribLocations[cnt] = glGetAttribLocation(shader_program, name);
                    prg->attribSizes[cnt] = 1;
                    ++cnt;
                }
            }
        }
    }

    if (cc_features.opt_fog) {
        prg->attribLocations[cnt] = glGetAttribLocation(shader_program, "aFog");
        prg->attribSizes[cnt] = 4;
        ++cnt;
    }

    if (cc_features.opt_grayscale) {
        prg->attribLocations[cnt] = glGetAttribLocation(shader_program, "aGrayscaleColor");
        prg->attribSizes[cnt] = 4;
        ++cnt;
    }

    for (int i = 0; i < cc_features.numInputs; i++) {
        char name[16];
        snprintf(name, sizeof(name), "aInput%d", i + 1);
        prg->attribLocations[cnt] = glGetAttribLocation(shader_program, name);
        prg->attribSizes[cnt] = cc_features.opt_alpha ? 4 : 3;
        ++cnt;
    }

    prg->openglProgramId = shader_program;
    prg->numInputs = cc_features.numInputs;
    prg->usedTextures[0] = cc_features.usedTextures[0];
    prg->usedTextures[1] = cc_features.usedTextures[1];
    prg->usedTextures[2] = cc_features.used_masks[0];
    prg->usedTextures[3] = cc_features.used_masks[1];
    prg->usedTextures[4] = cc_features.used_blend[0];
    prg->usedTextures[5] = cc_features.used_blend[1];
    prg->numFloats = numFloats;
    prg->numAttribs = cnt;

    prg->frameCountLocation = glGetUniformLocation(shader_program, "frame_count");
    prg->noiseScaleLocation = glGetUniformLocation(shader_program, "noise_scale");
    prg->texture_width_location = glGetUniformLocation(shader_program, "texture_width");
    prg->texture_height_location = glGetUniformLocation(shader_program, "texture_height");
    prg->texture_filtering_location = glGetUniformLocation(shader_program, "texture_filtering");

    LoadShader(prg);

    if (cc_features.usedTextures[0]) {
        GLint sampler_location = glGetUniformLocation(shader_program, "uTex0");
        glUniform1i(sampler_location, 0);
    }
    if (cc_features.usedTextures[1]) {
        GLint sampler_location = glGetUniformLocation(shader_program, "uTex1");
        glUniform1i(sampler_location, 1);
    }
    if (cc_features.used_masks[0]) {
        GLint sampler_location = glGetUniformLocation(shader_program, "uTexMask0");
        glUniform1i(sampler_location, 2);
    }
    if (cc_features.used_masks[1]) {
        GLint sampler_location = glGetUniformLocation(shader_program, "uTexMask1");
        glUniform1i(sampler_location, 3);
    }
    if (cc_features.used_blend[0]) {
        GLint sampler_location = glGetUniformLocation(shader_program, "uTexBlend0");
        glUniform1i(sampler_location, 4);
    }
    if (cc_features.used_blend[1]) {
        GLint sampler_location = glGetUniformLocation(shader_program, "uTexBlend1");
        glUniform1i(sampler_location, 5);
    }

    return prg;
}

struct ShaderProgram* GfxRenderingAPIOGL::LookupShader(uint64_t shader_id0, uint64_t shader_id1) {
    auto it = mShaderProgramPool.find(std::make_pair(shader_id0, shader_id1));
    return it == mShaderProgramPool.end() ? nullptr : &it->second;
}

void GfxRenderingAPIOGL::ShaderGetInfo(struct ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]) {
    *numInputs = prg->numInputs;
    usedTextures[0] = prg->usedTextures[0];
    usedTextures[1] = prg->usedTextures[1];
}

GLuint GfxRenderingAPIOGL::NewTexture() {
    GLuint ret;
    glGenTextures(1, &ret);
    textures.resize(std::max(textures.size(), (size_t)ret + 1));
    return ret;
}

void GfxRenderingAPIOGL::DeleteTexture(uint32_t texID) {
    glDeleteTextures(1, &texID);
}

void GfxRenderingAPIOGL::SelectTexture(int tile, GLuint texture_id) {
    if (mLastActiveTexture != tile) {
        mLastActiveTexture = tile;
        glActiveTexture(GL_TEXTURE0 + tile);
    }
    if (mLastBoundTextures[tile] != texture_id) {
        mLastBoundTextures[tile] = texture_id;
        glBindTexture(GL_TEXTURE_2D, texture_id);
    }
    mCurrentTextureIds[tile] = texture_id;
    mCurrentTile = tile;
}

void GfxRenderingAPIOGL::UploadTexture(const uint8_t* rgba32_buf, uint32_t width, uint32_t height) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba32_buf);
    textures[mCurrentTextureIds[mCurrentTile]].width = width;
    textures[mCurrentTextureIds[mCurrentTile]].height = height;
}

#ifdef USE_OPENGLES
#define GL_MIRROR_CLAMP_TO_EDGE 0x8743
#endif

static uint32_t gfx_cm_to_opengl(uint32_t val) {
    switch (val) {
        case G_TX_NOMIRROR | G_TX_CLAMP:
            return GL_CLAMP_TO_EDGE;
        case G_TX_MIRROR | G_TX_WRAP:
            return GL_MIRRORED_REPEAT;
        case G_TX_MIRROR | G_TX_CLAMP:
            return GL_MIRROR_CLAMP_TO_EDGE;
        case G_TX_NOMIRROR | G_TX_WRAP:
            return GL_REPEAT;
    }
    return 0;
}

void GfxRenderingAPIOGL::SetSamplerParameters(int tile, bool linear_filter, uint32_t cms, uint32_t cmt) {
    if (mLastActiveTexture != tile) {
        mLastActiveTexture = tile;
        glActiveTexture(GL_TEXTURE0 + tile);
    }
    const GLint filter = linear_filter && mCurrentFilterMode == FILTER_LINEAR ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    textures[mCurrentTextureIds[tile]].filtering = !linear_filter ? FILTER_LINEAR : FILTER_THREE_POINT;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gfx_cm_to_opengl(cms));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gfx_cm_to_opengl(cmt));
}

void GfxRenderingAPIOGL::SetDepthTestAndMask(bool depth_test, bool z_upd) {
    mCurrentDepthTest = depth_test;
    mCurrentDepthMask = z_upd;
}

void GfxRenderingAPIOGL::SetZmodeDecal(bool zmode_decal) {
    mCurrentZmodeDecal = zmode_decal;
}

void GfxRenderingAPIOGL::SetViewport(int x, int y, int width, int height) {
    glViewport(x, y, width, height);
}

void GfxRenderingAPIOGL::SetScissor(int x, int y, int width, int height) {
    glScissor(x, y, width, height);
}

void GfxRenderingAPIOGL::SetUseAlpha(bool use_alpha) {
    int8_t val = use_alpha ? 1 : 0;
    if (mLastBlendEnabled != val) {
        mLastBlendEnabled = val;
        if (use_alpha) {
            glEnable(GL_BLEND);
        } else {
            glDisable(GL_BLEND);
        }
    }
}

void GfxRenderingAPIOGL::DrawTriangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) {
    if (mCurrentDepthTest != mLastDepthTest || mCurrentDepthMask != mLastDepthMask) {
        mLastDepthTest = mCurrentDepthTest;
        mLastDepthMask = mCurrentDepthMask;

        if (mCurrentDepthTest || mLastDepthMask) {
            glEnable(GL_DEPTH_TEST);
            glDepthMask(mLastDepthMask ? GL_TRUE : GL_FALSE);
            glDepthFunc(mCurrentDepthTest ? (mCurrentZmodeDecal ? GL_LEQUAL : GL_LESS) : GL_ALWAYS);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
    }

    if (mCurrentZmodeDecal != mLastZmodeDecal) {
        mLastZmodeDecal = mCurrentZmodeDecal;
        if (mCurrentZmodeDecal) {
            // SSDB = SlopeScaledDepthBias 120 leads to -2 at 240p which is the same as N64 mode which has very little
            // fighting
            const int n64modeFactor = 120;
            const int noVanishFactor = 100;
            GLfloat SSDB = -2;
            switch (Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger(CVAR_Z_FIGHTING_MODE, 0)) {
                // scaled z-fighting (N64 mode like)
                case 1:
                    if (mFrameBuffers.size() >
                        mCurrentFrameBuffer) { // safety check for vector size can probably be removed
                        SSDB = -1.0f * (GLfloat)mFrameBuffers[mCurrentFrameBuffer].height / n64modeFactor;
                    }
                    break;
                // no vanishing paths
                case 2:
                    if (mFrameBuffers.size() >
                        mCurrentFrameBuffer) { // safety check for vector size can probably be removed
                        SSDB = -1.0f * (GLfloat)mFrameBuffers[mCurrentFrameBuffer].height / noVanishFactor;
                    }
                    break;
                // disabled
                case 0:
                default:
                    SSDB = -2;
            }
            glPolygonOffset(SSDB, -2);
            glEnable(GL_POLYGON_OFFSET_FILL);
        } else {
            glPolygonOffset(0, 0);
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
    }

    SetPerDrawUniforms();

    // printf("flushing %d tris\n", buf_vbo_num_tris);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * buf_vbo_len, buf_vbo, GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 3 * buf_vbo_num_tris);
}

void GfxRenderingAPIOGL::Init() {
#if !defined(__linux__) && !defined(__OpenBSD__)
    glewInit();
#endif

    glGenBuffers(1, &mOpenglVbo);
    glBindBuffer(GL_ARRAY_BUFFER, mOpenglVbo);

#if defined(__APPLE__) || defined(USE_OPENGLES)
    glGenVertexArrays(1, &mOpenglVao);
    glBindVertexArray(mOpenglVao);
#endif

#ifndef USE_OPENGLES // not supported on gles
    glEnable(GL_DEPTH_CLAMP);
#endif
    glDepthFunc(GL_LEQUAL);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    mFrameBuffers.resize(1); // for the default screen buffer

    glGenRenderbuffers(1, &mPixelDepthRb);
    glBindRenderbuffer(GL_RENDERBUFFER, mPixelDepthRb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1, 1);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glGenFramebuffers(1, &mPixelDepthFb);
    glBindFramebuffer(GL_FRAMEBUFFER, mPixelDepthFb);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mPixelDepthRb);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    mPixelDepthRbSize = 1;

    glGetIntegerv(GL_MAX_SAMPLES, &mMaxMsaaLevel);
}

void GfxRenderingAPIOGL::OnResize() {
}

void GfxRenderingAPIOGL::StartFrame() {
    mFrameCount++;
}

void GfxRenderingAPIOGL::EndFrame() {
    glFlush();
}

void GfxRenderingAPIOGL::FinishRender() {
}

int GfxRenderingAPIOGL::CreateFramebuffer() {
    GLuint clrbuf;
    glGenTextures(1, &clrbuf);
    glBindTexture(GL_TEXTURE_2D, clrbuf);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Register clrbuf in `textures` so SelectTextureFb-bound FB samples have
    // populated width/height entries. SetPerDrawUniforms reads
    // textures[mCurrentTextureIds[0]].{width,height} into the shader's
    // texture_width/texture_height uniforms; without this, the GLSL fragment
    // shader's `clamp(vTexCoord, 0.5/texSize, ...)` divides by zero whenever
    // an FB-passthrough draw uses the clamp path (1P stage-clear stripes,
    // VS results photo wipe), producing NaN sample coords and a black/hung
    // sample on every GL driver. D3D11 sets tex.width/height in its
    // UpdateFramebufferParameters; Metal queries texture.get_width() in its
    // shader, so neither needs this.
    textures.resize(std::max(textures.size(), (size_t)clrbuf + 1));
    textures[clrbuf].width = 1;
    textures[clrbuf].height = 1;
    textures[clrbuf].filtering = FILTER_LINEAR;

    GLuint clrbufMsaa;
    glGenRenderbuffers(1, &clrbufMsaa);

    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1, 1);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    GLuint fbo;
    glGenFramebuffers(1, &fbo);

    size_t i = mFrameBuffers.size();
    mFrameBuffers.resize(i + 1);

    mFrameBuffers[i].fbo = fbo;
    mFrameBuffers[i].clrbuf = clrbuf;
    mFrameBuffers[i].clrbufMsaa = clrbufMsaa;
    mFrameBuffers[i].rbo = rbo;

    return i;
}

void GfxRenderingAPIOGL::DestroyFramebuffer(int fbId) {
    if (fbId < 0 || (size_t)fbId >= mFrameBuffers.size()) {
        return;
    }
    FramebufferOGL& fb = mFrameBuffers[fbId];
    if (fb.fbo == 0 && fb.clrbuf == 0 && fb.clrbufMsaa == 0 && fb.rbo == 0) {
        return; // Already destroyed.
    }
    if (fb.fbo != 0) {
        glDeleteFramebuffers(1, &fb.fbo);
    }
    if (fb.clrbuf != 0) {
        glDeleteTextures(1, &fb.clrbuf);
        // Drop the matching entry in our textures table; SelectTextureFb
        // never references this id again because the chain releases the
        // FBO before it could be sampled.
        if ((size_t)fb.clrbuf < textures.size()) {
            textures[fb.clrbuf].width = 0;
            textures[fb.clrbuf].height = 0;
        }
    }
    if (fb.clrbufMsaa != 0) {
        glDeleteRenderbuffers(1, &fb.clrbufMsaa);
    }
    if (fb.rbo != 0) {
        glDeleteRenderbuffers(1, &fb.rbo);
    }
    fb.fbo = 0;
    fb.clrbuf = 0;
    fb.clrbufMsaa = 0;
    fb.rbo = 0;
    fb.width = 0;
    fb.height = 0;
    fb.has_depth_buffer = false;
    fb.msaa_level = 0;
}

void GfxRenderingAPIOGL::UpdateFramebufferParameters(int fb_id, uint32_t width, uint32_t height, uint32_t msaa_level,
                                                     bool opengl_invertY, bool render_target, bool has_depth_buffer,
                                                     bool can_extract_depth) {
    FramebufferOGL& fb = mFrameBuffers[fb_id];

    width = std::max(width, 1U);
    height = std::max(height, 1U);
    msaa_level = std::min(msaa_level, (uint32_t)mMaxMsaaLevel);

    glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);

    if (fb_id != 0) {
        // Post-process intermediates may have switched format (sRGB or
        // float). When that happens the color attachment must be
        // reallocated even if size/msaa are unchanged.
        const bool formatChanged = (fb.postProcessFormat != fb.lastPostProcessFormat);
        if (fb.width != width || fb.height != height || fb.msaa_level != msaa_level || formatChanged) {
            GLenum internalFmt = GL_RGB8;
            GLenum srcFmt = GL_RGB;
            GLenum srcType = GL_UNSIGNED_BYTE;
            switch (fb.postProcessFormat) {
                case PostProcessFboFormat::Default:
                    internalFmt = GL_RGB8;
                    srcFmt = GL_RGB;
                    srcType = GL_UNSIGNED_BYTE;
                    break;
                case PostProcessFboFormat::Srgb:
                    internalFmt = GL_SRGB8_ALPHA8;
                    srcFmt = GL_RGBA;
                    srcType = GL_UNSIGNED_BYTE;
                    break;
                case PostProcessFboFormat::Float16:
                    internalFmt = GL_RGBA16F;
                    srcFmt = GL_RGBA;
                    srcType = GL_HALF_FLOAT;
                    break;
            }
            if (msaa_level <= 1) {
                glBindTexture(GL_TEXTURE_2D, fb.clrbuf);
                glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, width, height, 0, srcFmt, srcType, NULL);
                glBindTexture(GL_TEXTURE_2D, 0);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb.clrbuf, 0);
            } else {
                glBindRenderbuffer(GL_RENDERBUFFER, fb.clrbufMsaa);
                glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa_level, internalFmt, width, height);
                glBindRenderbuffer(GL_RENDERBUFFER, 0);
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, fb.clrbufMsaa);
            }
            fb.lastPostProcessFormat = fb.postProcessFormat;
        }

        if (has_depth_buffer &&
            (fb.width != width || fb.height != height || fb.msaa_level != msaa_level || !fb.has_depth_buffer)) {
            glBindRenderbuffer(GL_RENDERBUFFER, fb.rbo);
            if (msaa_level <= 1) {
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
            } else {
                glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa_level, GL_DEPTH24_STENCIL8, width, height);
            }
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
        }

        if (!fb.has_depth_buffer && has_depth_buffer) {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fb.rbo);
        } else if (fb.has_depth_buffer && !has_depth_buffer) {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
        }
    }

    fb.width = width;
    fb.height = height;
    fb.has_depth_buffer = has_depth_buffer;
    fb.msaa_level = msaa_level;
    fb.invertY = opengl_invertY;

    // Keep textures[clrbuf] dimensions in sync — see CreateFramebuffer for why.
    if (fb_id != 0 && fb.clrbuf != 0 && (size_t)fb.clrbuf < textures.size()) {
        textures[fb.clrbuf].width = (uint16_t)std::min<uint32_t>(width, UINT16_MAX);
        textures[fb.clrbuf].height = (uint16_t)std::min<uint32_t>(height, UINT16_MAX);
    }
}

void GfxRenderingAPIOGL::StartDrawToFramebuffer(int fb_id, float noise_scale) {
    FramebufferOGL& fb = mFrameBuffers[fb_id];

    if (noise_scale != 0.0f) {
        mCurrentNoiseScale = 1.0f / noise_scale;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);
    mCurrentFrameBuffer = fb_id;
}

void GfxRenderingAPIOGL::ClearFramebuffer(bool color, bool depth) {
    if (mLastScissorEnabled != 0) {
        mLastScissorEnabled = 0;
        glDisable(GL_SCISSOR_TEST);
    }
    glDepthMask(GL_TRUE);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear((color ? GL_COLOR_BUFFER_BIT : 0) | (depth ? GL_DEPTH_BUFFER_BIT : 0));
    glDepthMask(mCurrentDepthMask ? GL_TRUE : GL_FALSE);
    if (mLastScissorEnabled != 1) {
        mLastScissorEnabled = 1;
        glEnable(GL_SCISSOR_TEST);
    }
}

void GfxRenderingAPIOGL::ResolveMSAAColorBuffer(int fb_id_target, int fb_id_source) {
    FramebufferOGL& fb_dst = mFrameBuffers[fb_id_target];
    FramebufferOGL& fb_src = mFrameBuffers[fb_id_source];
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb_dst.fbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fb_src.fbo);

    // Disabled for blit
    if (mLastScissorEnabled != 0) {
        mLastScissorEnabled = 0;
        glDisable(GL_SCISSOR_TEST);
    }

    glBlitFramebuffer(0, 0, fb_src.width, fb_src.height, 0, 0, fb_dst.width, fb_dst.height, GL_COLOR_BUFFER_BIT,
                      GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, mCurrentFrameBuffer);

    if (mLastScissorEnabled != 1) {
        mLastScissorEnabled = 1;
        glEnable(GL_SCISSOR_TEST);
    }
}

void* GfxRenderingAPIOGL::GetFramebufferTextureId(int fb_id) {
    return (void*)(uintptr_t)mFrameBuffers[fb_id].clrbuf;
}

// --- Post-process / user-shader pipeline ----------------------------------
//
// Implemented from the plan in docs/crt_shader_plan_2026-05-11.md §7.1.
// No code copied from RetroArch or any GPL-licensed shader runtime.

static const char kPostProcessVertexShader[] =
    "#version 330 core\n"
    "in vec2 aPos;\n"
    "out vec2 vTexCoord;\n"
    "uniform int FlipY;\n"
    "void main() {\n"
    "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
    "    vec2 uv = aPos * 0.5 + 0.5;\n"
    "    if (FlipY != 0) uv.y = 1.0 - uv.y;\n"
    "    vTexCoord = uv;\n"
    "}\n";

bool GfxRenderingAPIOGL::SupportsPostProcess() {
    return true;
}

GLuint GfxRenderingAPIOGL::CompilePostProcessProgram(const std::string& fsSource, std::string& errOut) {
    auto compile = [&](GLenum type, const char* src) -> GLuint {
        GLuint sh = glCreateShader(type);
        glShaderSource(sh, 1, &src, nullptr);
        glCompileShader(sh);
        GLint ok = GL_FALSE;
        glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            GLint len = 0;
            glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
            std::string log(std::max(len, 1) - 1, '\0');
            if (len > 0) {
                glGetShaderInfoLog(sh, len, nullptr, log.data());
            }
            errOut = std::move(log);
            glDeleteShader(sh);
            return 0;
        }
        return sh;
    };
    GLuint vs = compile(GL_VERTEX_SHADER, kPostProcessVertexShader);
    if (vs == 0) {
        return 0;
    }
    GLuint fs = compile(GL_FRAGMENT_SHADER, fsSource.c_str());
    if (fs == 0) {
        glDeleteShader(vs);
        return 0;
    }
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    // Tie the only vertex attribute to slot 0; the FS doesn't have any so
    // there is no fragment-output binding to worry about (single FS
    // output is assumed at location 0).
    glBindAttribLocation(prog, 0, "aPos");
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::string log(std::max(len, 1) - 1, '\0');
        if (len > 0) {
            glGetProgramInfoLog(prog, len, nullptr, log.data());
        }
        errOut = std::move(log);
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

int GfxRenderingAPIOGL::CreatePostProcessProgram(const PostProcessSource& src) {
    if (src.glsl.empty()) {
        SPDLOG_ERROR("Post-process shader '{}' has no GLSL source", src.name);
        return -1;
    }
    std::string err;
    GLuint prog = CompilePostProcessProgram(src.glsl, err);
    if (prog == 0) {
        SPDLOG_ERROR("Post-process shader '{}' failed to compile/link: {}", src.name, err);
        return -1;
    }

    PostProcessProgramOGL slot{};
    slot.program = prog;
    slot.name = src.name;
    slot.sourceLocation = glGetUniformLocation(prog, "Source");
    slot.sourceSizeLocation = glGetUniformLocation(prog, "SourceSize");
    slot.outputSizeLocation = glGetUniformLocation(prog, "OutputSize");
    slot.inputSizeLocation = glGetUniformLocation(prog, "InputSize");
    slot.originalLocation = glGetUniformLocation(prog, "Original");
    slot.originalSizeLocation = glGetUniformLocation(prog, "OriginalSize");
    slot.frameCountLocation = glGetUniformLocation(prog, "FrameCount");
    slot.frameDirectionLocation = glGetUniformLocation(prog, "FrameDirection");
    slot.aliasLocations.reserve(src.aliasNames.size());
    slot.aliasSizeLocations.reserve(src.aliasNames.size());
    for (const std::string& alias : src.aliasNames) {
        slot.aliasLocations.push_back(glGetUniformLocation(prog, alias.c_str()));
        const std::string sizeName = alias + "Size";
        slot.aliasSizeLocations.push_back(glGetUniformLocation(prog, sizeName.c_str()));
    }

    for (size_t i = 0; i < mPostProcessPrograms.size(); ++i) {
        if (mPostProcessPrograms[i].program == 0) {
            mPostProcessPrograms[i] = std::move(slot);
            return (int)i;
        }
    }
    mPostProcessPrograms.push_back(std::move(slot));
    return (int)(mPostProcessPrograms.size() - 1);
}

void GfxRenderingAPIOGL::DestroyPostProcessProgram(int progId) {
    if (progId < 0 || (size_t)progId >= mPostProcessPrograms.size()) {
        return;
    }
    auto& slot = mPostProcessPrograms[progId];
    if (slot.program != 0) {
        glDeleteProgram(slot.program);
        slot = PostProcessProgramOGL{};
    }
}

// Phase 3D-1: compile a slang program (authored VS + FS) and pre-
// allocate its UBO. Sampler texture units are pinned at link time via
// glUniform1i so the run path (Phase 3D-2) only needs to bind
// textures, not re-poke uniform values. The legacy LUS-schema path
// (CreatePostProcessProgram) is untouched.
int GfxRenderingAPIOGL::CreatePostProcessSlangProgram(const PostProcessSlangProgramSource& src) {
    if (src.vsGlsl.empty() || src.fsGlsl.empty()) {
        SPDLOG_ERROR("Slang post-process shader '{}' missing VS or FS GLSL", src.name);
        return -1;
    }

    auto compile = [&](GLenum type, const char* source) -> GLuint {
        GLuint sh = glCreateShader(type);
        glShaderSource(sh, 1, &source, nullptr);
        glCompileShader(sh);
        GLint ok = GL_FALSE;
        glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            GLint len = 0;
            glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
            std::string log(std::max(len, 1) - 1, '\0');
            if (len > 0) {
                glGetShaderInfoLog(sh, len, nullptr, log.data());
            }
            SPDLOG_ERROR("Slang post-process shader '{}' {} compile failed: {}",
                         src.name,
                         (type == GL_VERTEX_SHADER) ? "vertex" : "fragment",
                         log);
            glDeleteShader(sh);
            return 0;
        }
        return sh;
    };

    GLuint vs = compile(GL_VERTEX_SHADER, src.vsGlsl.c_str());
    if (vs == 0) {
        return -1;
    }
    GLuint fs = compile(GL_FRAGMENT_SHADER, src.fsGlsl.c_str());
    if (fs == 0) {
        glDeleteShader(vs);
        return -1;
    }
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    // Slang vertex stages declare attributes via `layout(location=N)
    // in ...;` so we let GLSL's explicit-attrib-location handling
    // bind them; no glBindAttribLocation needed.
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint linked = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::string log(std::max(len, 1) - 1, '\0');
        if (len > 0) {
            glGetProgramInfoLog(prog, len, nullptr, log.data());
        }
        SPDLOG_ERROR("Slang post-process shader '{}' link failed: {}", src.name, log);
        glDeleteProgram(prog);
        return -1;
    }

    // Pin each sampler uniform to texture unit i in declaration order.
    // The slang shader's GLSL declarations had their binding= and
    // descriptor-set= decorations stripped during transpile, so unit
    // assignment lives at the program level (here) and the Run path
    // only needs glActiveTexture(GL_TEXTURE0 + i) + glBindTexture.
    glUseProgram(prog);
    for (size_t i = 0; i < src.samplerNames.size(); ++i) {
        const GLint loc = glGetUniformLocation(prog, src.samplerNames[i].c_str());
        if (loc >= 0) {
            glUniform1i(loc, static_cast<GLint>(i));
        }
        // Missing samplers (loc == -1) are fine — the driver optimised
        // the binding out because the shader never sampled it. The run
        // path can still bind a texture at the unit; it's just unused.
    }
    glUseProgram(0);

    // Bind the UBO (named "UBO" by slang convention) to binding point 0
    // on this program and allocate a dedicated buffer of the declared
    // size. The Run path memcpys frame data here and reuses the same
    // glBuffer every frame.
    const GLuint uboBindingPoint = 0;
    GLuint ubo = 0;
    if (src.uboBytes > 0) {
        const GLuint blockIdx = glGetUniformBlockIndex(prog, "UBO");
        if (blockIdx != GL_INVALID_INDEX) {
            glUniformBlockBinding(prog, blockIdx, uboBindingPoint);
        }
        glGenBuffers(1, &ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(src.uboBytes),
                     nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    PostProcessSlangProgramOGL slot{};
    slot.program = prog;
    slot.name = src.name;
    slot.ubo = ubo;
    slot.uboBytes = src.uboBytes;
    slot.uboBindingPoint = uboBindingPoint;
    slot.samplerNames = src.samplerNames;

    for (size_t i = 0; i < mPostProcessSlangPrograms.size(); ++i) {
        if (mPostProcessSlangPrograms[i].program == 0) {
            mPostProcessSlangPrograms[i] = std::move(slot);
            return static_cast<int>(i);
        }
    }
    mPostProcessSlangPrograms.push_back(std::move(slot));
    return static_cast<int>(mPostProcessSlangPrograms.size() - 1);
}

void GfxRenderingAPIOGL::DestroyPostProcessSlangProgram(int progId) {
    if (progId < 0 || (size_t)progId >= mPostProcessSlangPrograms.size()) {
        return;
    }
    auto& slot = mPostProcessSlangPrograms[progId];
    if (slot.program != 0) {
        glDeleteProgram(slot.program);
    }
    if (slot.ubo != 0) {
        glDeleteBuffers(1, &slot.ubo);
    }
    slot = PostProcessSlangProgramOGL{};
}

GLuint GfxRenderingAPIOGL::EnsurePostProcessSlangVao() {
    if (mPostProcessSlangVao != 0 && mPostProcessSlangVbo != 0) {
        return mPostProcessSlangVao;
    }
    // Fullscreen triangle for slang: vec4 Position (clip space) +
    // vec2 TexCoord (covers [0,1] after the rasterizer interpolates).
    // The slang VS multiplies Position by an identity MVP (provided
    // by the chain), so the vertices land in clip space directly.
    static const GLfloat kSlangVerts[] = {
        // Position (xyzw)            // TexCoord (uv)
        -1.0f, -1.0f, 0.0f, 1.0f,     0.0f, 0.0f,
         3.0f, -1.0f, 0.0f, 1.0f,     2.0f, 0.0f,
        -1.0f,  3.0f, 0.0f, 1.0f,     0.0f, 2.0f,
    };
    glGenVertexArrays(1, &mPostProcessSlangVao);
    glBindVertexArray(mPostProcessSlangVao);
    glGenBuffers(1, &mPostProcessSlangVbo);
    glBindBuffer(GL_ARRAY_BUFFER, mPostProcessSlangVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kSlangVerts), kSlangVerts, GL_STATIC_DRAW);
    // location 0 = vec4 Position.
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat),
                          reinterpret_cast<void*>(0));
    // location 1 = vec2 TexCoord.
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat),
                          reinterpret_cast<void*>(4 * sizeof(GLfloat)));
    return mPostProcessSlangVao;
}

void GfxRenderingAPIOGL::RunPostProcessSlang(int progId, int dstFb,
                                             const uint8_t* uboData, uint32_t uboBytes,
                                             const int* samplerFbIds, uint32_t samplerCount,
                                             const PostProcessParams& params) {
    if (progId < 0 || (size_t)progId >= mPostProcessSlangPrograms.size()) {
        return;
    }
    const PostProcessSlangProgramOGL& slot = mPostProcessSlangPrograms[progId];
    if (slot.program == 0) {
        return;
    }
    if (dstFb < 0 || (size_t)dstFb >= mFrameBuffers.size()) {
        return;
    }
    const FramebufferOGL& dstFbInfo = mFrameBuffers[dstFb];
    if (dstFbInfo.fbo == 0) {
        return;
    }

    // Fixed-function state: identical to the legacy post-process
    // path. Mirrors the cache-invalidation pattern so the next
    // regular draw sees consistent state.
    if (mLastScissorEnabled != 0) {
        glDisable(GL_SCISSOR_TEST);
        mLastScissorEnabled = 0;
    }
    if (mLastBlendEnabled != 0) {
        glDisable(GL_BLEND);
        mLastBlendEnabled = 0;
    }
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    mLastDepthTest = -1;
    mLastDepthMask = -1;
    mLastZmodeDecal = -1;

    glBindFramebuffer(GL_FRAMEBUFFER, dstFbInfo.fbo);
    mCurrentFrameBuffer = dstFb;
    glViewport(0, 0, dstFbInfo.width, dstFbInfo.height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

#ifndef USE_OPENGLES // GLES has no GL_FRAMEBUFFER_SRGB toggle — sRGB encode is implicit when the attachment is sRGB-formatted.
    if (dstFbInfo.postProcessFormat == PostProcessFboFormat::Srgb) {
        glEnable(GL_FRAMEBUFFER_SRGB);
    }
#endif

    // Upload the chain-built UBO blob. The buffer was allocated at
    // CreatePostProcessSlangProgram time so we just refill.
    if (slot.ubo != 0 && uboData != nullptr && uboBytes > 0) {
        glBindBuffer(GL_UNIFORM_BUFFER, slot.ubo);
        const GLsizeiptr writeBytes =
            static_cast<GLsizeiptr>(std::min<uint32_t>(uboBytes, slot.uboBytes));
        glBufferSubData(GL_UNIFORM_BUFFER, 0, writeBytes, uboData);
        glBindBufferBase(GL_UNIFORM_BUFFER, slot.uboBindingPoint, slot.ubo);
    }

    // Bind sampler textures in declaration order. Each entry's
    // samplerFbIds[i] is the FBO whose color texture goes on unit i.
    // -1 means "use a fallback" — we pick TU0's source if available,
    // else leave whatever was bound.
    GLuint fallbackTex = 0;
    if (samplerCount > 0 && samplerFbIds != nullptr && samplerFbIds[0] >= 0 &&
        (size_t)samplerFbIds[0] < mFrameBuffers.size()) {
        fallbackTex = mFrameBuffers[samplerFbIds[0]].clrbuf;
    }
    for (uint32_t i = 0; i < samplerCount; ++i) {
        const int fbId = samplerFbIds ? samplerFbIds[i] : -1;
        GLuint tex = fallbackTex;
        if (fbId >= 0 && (size_t)fbId < mFrameBuffers.size() &&
            mFrameBuffers[fbId].clrbuf != 0) {
            tex = mFrameBuffers[fbId].clrbuf;
        }
        if (tex == 0) {
            continue;
        }
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, tex);
        // Phase 3D-2 minimal sampler state: linear / clamp-to-edge on
        // every slot. Per-slot libretro filter_linearN / wrap_modeN
        // routing comes with Phase 3D-3 multipass.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        params.srcFilterLinear ? GL_LINEAR : GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                        params.srcFilterLinear ? GL_LINEAR : GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        if (i < SHADER_MAX_TEXTURES) {
            mLastBoundTextures[i] = tex;
        }
    }
    glActiveTexture(GL_TEXTURE0);
    mLastActiveTexture = 0;

    glUseProgram(slot.program);
    mLastLoadedShader = nullptr;

    EnsurePostProcessSlangVao();
    glBindVertexArray(mPostProcessSlangVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Restore the regular-draw VAO/VBO bindings.
#if defined(__APPLE__) || defined(USE_OPENGLES)
    glBindVertexArray(mOpenglVao);
#else
    glBindVertexArray(0);
#endif
    glBindBuffer(GL_ARRAY_BUFFER, mOpenglVbo);
}

GLuint GfxRenderingAPIOGL::EnsurePostProcessVao() {
    if (mPostProcessVao != 0 && mPostProcessVbo != 0) {
        return mPostProcessVao;
    }
    // Fullscreen triangle in NDC. Extends past the [-1,1] viewport on two
    // edges so a single triangle covers the entire output without the
    // diagonal seam of a two-triangle quad.
    static const GLfloat kFullscreenTriangle[] = {
        -1.0f, -1.0f, 3.0f, -1.0f, -1.0f, 3.0f,
    };
    glGenVertexArrays(1, &mPostProcessVao);
    glBindVertexArray(mPostProcessVao);
    glGenBuffers(1, &mPostProcessVbo);
    glBindBuffer(GL_ARRAY_BUFFER, mPostProcessVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kFullscreenTriangle), kFullscreenTriangle, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);
    return mPostProcessVao;
}

int GfxRenderingAPIOGL::CreatePostProcessStaticTexture(uint32_t width, uint32_t height,
                                                       const uint8_t* rgba8) {
    if (width == 0 || height == 0 || rgba8 == nullptr) {
        return -1;
    }
    GLuint tex = 0;
    glGenTextures(1, &tex);
    if (tex == 0) {
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)width, (GLsizei)height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, rgba8);
    // Sampler state is set per RunPostProcess call from the per-binding
    // filterLinear / wrapMode — leave the texture object defaults here.
    glBindTexture(GL_TEXTURE_2D, 0);
    for (size_t i = 0; i < mPostProcessStaticTextures.size(); ++i) {
        if (mPostProcessStaticTextures[i] == 0) {
            mPostProcessStaticTextures[i] = tex;
            return (int)i;
        }
    }
    mPostProcessStaticTextures.push_back(tex);
    return (int)(mPostProcessStaticTextures.size() - 1);
}

void GfxRenderingAPIOGL::DestroyPostProcessStaticTexture(int textureId) {
    if (textureId < 0 || (size_t)textureId >= mPostProcessStaticTextures.size()) {
        return;
    }
    GLuint& slot = mPostProcessStaticTextures[textureId];
    if (slot != 0) {
        glDeleteTextures(1, &slot);
        slot = 0;
    }
}

void GfxRenderingAPIOGL::SetPostProcessFramebufferMipmapped(int fb_id, bool mipmapped) {
    if (fb_id < 0 || (size_t)fb_id >= mFrameBuffers.size()) {
        return;
    }
    mFrameBuffers[fb_id].postProcessMipmapped = mipmapped;
}

void GfxRenderingAPIOGL::GeneratePostProcessMipmaps(int fb_id) {
    if (fb_id < 0 || (size_t)fb_id >= mFrameBuffers.size()) {
        return;
    }
    const FramebufferOGL& fb = mFrameBuffers[fb_id];
    if (fb.clrbuf == 0) {
        return;
    }
    // OpenGL textures allocated via glTexImage2D are mutable; the
    // first glGenerateMipmap call also reserves storage for levels
    // 1..N, so the FBO does not need to be pre-allocated as
    // mipmapped. Set the min filter to MIPMAP_LINEAR on the
    // implicit texture object so the call is mipmap-complete; the
    // per-call sampler override in RunPostProcess restores whatever
    // filter the next bind needs.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fb.clrbuf);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    // Restore the cached binding so the next regular draw's
    // texture-bind cache stays accurate. mLastBoundTextures[0] is
    // whatever the chain or main pass set last; clearing it forces
    // the next SelectTexture to re-bind.
    mLastBoundTextures[0] = 0;
}

void GfxRenderingAPIOGL::SetPostProcessFramebufferFormat(int fb_id, PostProcessFboFormat fmt) {
    if (fb_id < 0 || (size_t)fb_id >= mFrameBuffers.size()) {
        return;
    }
    // Record the desired format; the next UpdateFramebufferParameters
    // notices the mismatch against lastPostProcessFormat and reallocates
    // the color attachment.
    mFrameBuffers[fb_id].postProcessFormat = fmt;
}

void GfxRenderingAPIOGL::RunPostProcess(int progId, int srcFb, int dstFb, int originalFb,
                                        const PostProcessParams& params) {
    if (progId < 0 || (size_t)progId >= mPostProcessPrograms.size()) {
        return;
    }
    const PostProcessProgramOGL& slot = mPostProcessPrograms[progId];
    if (slot.program == 0) {
        return;
    }
    if (srcFb < 0 || (size_t)srcFb >= mFrameBuffers.size()) {
        return;
    }
    if (dstFb < 0 || (size_t)dstFb >= mFrameBuffers.size()) {
        return;
    }
    const FramebufferOGL& srcFbInfo = mFrameBuffers[srcFb];
    const FramebufferOGL& dstFbInfo = mFrameBuffers[dstFb];
    if (srcFbInfo.clrbuf == 0 || dstFbInfo.fbo == 0) {
        return;
    }

    // Fixed-function state for a fullscreen pass. No scissor, no depth,
    // no blend. The next regular draw resets viewport / blend / depth.
    if (mLastScissorEnabled != 0) {
        glDisable(GL_SCISSOR_TEST);
        mLastScissorEnabled = 0;
    }
    if (mLastBlendEnabled != 0) {
        glDisable(GL_BLEND);
        mLastBlendEnabled = 0;
    }
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    mLastDepthTest = -1;
    mLastDepthMask = -1;
    mLastZmodeDecal = -1;

    glBindFramebuffer(GL_FRAMEBUFFER, dstFbInfo.fbo);
    mCurrentFrameBuffer = dstFb;
    glViewport(0, 0, dstFbInfo.width, dstFbInfo.height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // sRGB intermediate FBOs need GL_FRAMEBUFFER_SRGB enabled so the
    // hardware encodes the fragment's linear output back to sRGB on
    // write. For non-sRGB destinations the state is ignored by spec
    // (encoding only happens when the attachment is sRGB-formatted),
    // so we leave it enabled afterwards rather than thrashing state.
    // GLES has no equivalent toggle — sRGB encode is implicit when the
    // attachment is sRGB-formatted, so the call is unneeded there.
#ifndef USE_OPENGLES
    if (dstFbInfo.postProcessFormat == PostProcessFboFormat::Srgb) {
        glEnable(GL_FRAMEBUFFER_SRGB);
    }
#endif

    // Sample source FB's color texture on TU0. Filter + wrap mode come
    // from the producer pass's libretro `filter_linearN` / `wrap_modeN`
    // — passed through PostProcessParams by the chain. Pass 0 sees the
    // chain's defaults (linear / clamp-to-edge).
    // Phase 2.2: pick a mipmap-aware min filter when the chain has
    // pre-populated this pass's input texture with a mip chain via
    // GeneratePostProcessMipmaps. Mag is single-level by construction.
    GLint srcMinFilter = params.srcFilterLinear ? GL_LINEAR : GL_NEAREST;
    GLint srcMagFilter = srcMinFilter;
    if (params.srcUseMipmap) {
        srcMinFilter = params.srcFilterLinear ? GL_LINEAR_MIPMAP_LINEAR
                                              : GL_NEAREST_MIPMAP_NEAREST;
    }
    GLint srcWrap = GL_CLAMP_TO_EDGE;
    switch (params.srcWrapMode) {
        case PostProcessWrapMode::ClampToEdge:    srcWrap = GL_CLAMP_TO_EDGE; break;
        case PostProcessWrapMode::ClampToBorder:
#ifdef USE_OPENGLES
            // GLES core has no CLAMP_TO_BORDER (3.2 added it as an ext but
            // we can't assume coverage). Fall back to edge clamp — shaders
            // that depend on the transparent-black border behaviour will
            // see edge-sampling artifacts but the pass still renders.
            srcWrap = GL_CLAMP_TO_EDGE;
#else
            srcWrap = GL_CLAMP_TO_BORDER;
#endif
            break;
        case PostProcessWrapMode::Repeat:         srcWrap = GL_REPEAT; break;
        case PostProcessWrapMode::MirroredRepeat: srcWrap = GL_MIRRORED_REPEAT; break;
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, srcFbInfo.clrbuf);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, srcMinFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, srcMagFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, srcWrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, srcWrap);
    mLastBoundTextures[0] = srcFbInfo.clrbuf;

    // Original (game FB) on TU1 for multipass shaders that combine
    // post-bloom Source with the pre-bloom Original. We bind it
    // unconditionally; shaders that don't reference Original simply
    // ignore the binding.
    GLuint originalTex = srcFbInfo.clrbuf;
    if (originalFb >= 0 && (size_t)originalFb < mFrameBuffers.size() &&
        mFrameBuffers[originalFb].clrbuf != 0) {
        originalTex = mFrameBuffers[originalFb].clrbuf;
    }
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, originalTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    mLastBoundTextures[1] = originalTex;

    // Alias / external-texture bindings at TUs 2+. Each entry's
    // sampler state comes from the producer pass's libretro
    // filter_linearN / wrap_modeN. sourceFb == -1 means "producer
    // pass hasn't run yet at this point in the chain" — bind the
    // game FB (Original) as a defensive fallback so the shader's
    // texture() reads something sane rather than a stale slot.
    auto glWrap = [](PostProcessWrapMode m) -> GLint {
        switch (m) {
            case PostProcessWrapMode::ClampToEdge:    return GL_CLAMP_TO_EDGE;
            case PostProcessWrapMode::ClampToBorder:
#ifdef USE_OPENGLES
                return GL_CLAMP_TO_EDGE;
#else
                return GL_CLAMP_TO_BORDER;
#endif
            case PostProcessWrapMode::Repeat:         return GL_REPEAT;
            case PostProcessWrapMode::MirroredRepeat: return GL_MIRRORED_REPEAT;
        }
        return GL_CLAMP_TO_EDGE;
    };
    for (size_t i = 0; i < params.extraBindingsCount; ++i) {
        const auto& eb = params.extraBindings[i];
        GLuint tex = originalTex; // defensive fallback
        if (eb.staticTextureId >= 0 &&
            (size_t)eb.staticTextureId < mPostProcessStaticTextures.size() &&
            mPostProcessStaticTextures[eb.staticTextureId] != 0) {
            tex = mPostProcessStaticTextures[eb.staticTextureId];
        } else if (eb.sourceFb >= 0 && (size_t)eb.sourceFb < mFrameBuffers.size() &&
                   mFrameBuffers[eb.sourceFb].clrbuf != 0) {
            tex = mFrameBuffers[eb.sourceFb].clrbuf;
        }
        const GLenum unit = GL_TEXTURE2 + (GLenum)i;
        glActiveTexture(unit);
        glBindTexture(GL_TEXTURE_2D, tex);
        const GLint flt = eb.filterLinear ? GL_LINEAR : GL_NEAREST;
        const GLint wrp = glWrap(eb.wrapMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, flt);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, flt);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrp);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrp);
        if (i < (size_t)SHADER_MAX_TEXTURES) {
            mLastBoundTextures[i] = tex;
        }
    }
    // Leave the active texture on TU0 so the next regular draw's
    // glActiveTexture call doesn't have to undo us.
    glActiveTexture(GL_TEXTURE0);
    mLastActiveTexture = 0;

    glUseProgram(slot.program);
    mLastLoadedShader = nullptr; // Force LoadShader to re-glUseProgram next frame.
    if (slot.sourceLocation >= 0) {
        glUniform1i(slot.sourceLocation, 0);
    }
    if (slot.originalLocation >= 0) {
        glUniform1i(slot.originalLocation, 1);
    }
    for (size_t i = 0; i < slot.aliasLocations.size(); ++i) {
        if (slot.aliasLocations[i] >= 0) {
            glUniform1i(slot.aliasLocations[i], (GLint)(2 + i));
        }
    }
    for (size_t i = 0; i < slot.aliasSizeLocations.size(); ++i) {
        if (slot.aliasSizeLocations[i] < 0) {
            continue;
        }
        float w = 1.0f;
        float h = 1.0f;
        if (i < params.extraBindingsCount) {
            const auto& eb = params.extraBindings[i];
            w = (float)eb.width;
            h = (float)eb.height;
        }
        glUniform2f(slot.aliasSizeLocations[i], w, h);
    }
    if (slot.originalSizeLocation >= 0) {
        glUniform2f(slot.originalSizeLocation, (float)params.originalWidth,
                    (float)params.originalHeight);
    }
    if (slot.sourceSizeLocation >= 0) {
        glUniform2f(slot.sourceSizeLocation, (float)params.srcWidth, (float)params.srcHeight);
    }
    if (slot.inputSizeLocation >= 0) {
        glUniform2f(slot.inputSizeLocation, (float)params.inputWidth, (float)params.inputHeight);
    }
    if (slot.outputSizeLocation >= 0) {
        glUniform2f(slot.outputSizeLocation, (float)params.dstWidth, (float)params.dstHeight);
    }
    if (slot.frameCountLocation >= 0) {
        glUniform1i(slot.frameCountLocation, (int)params.frameCount);
    }
    if (slot.frameDirectionLocation >= 0) {
        glUniform1f(slot.frameDirectionLocation, 1.0f);
    }
    // FlipY uniform on the vertex shader compensates for the V-axis
    // mismatch between source FBO sampling and ImGui::Image's default
    // UV interpretation (which treats V=0 as the top of the displayed
    // quad). Both mGameFb (rendered with invertY=true, game-top at
    // pixel-bottom = V=0) and mGameFbMsaaResolved (MSAA-blitted from
    // mGameFb, same pixel layout) already have game-top at V=0, so
    // the post-process pass writes them passthrough — bottom vertex
    // of the fullscreen tri samples V=0 (game-top) and lands at
    // GL-pixel-bottom of mDstFb, which is V=0 when ImGui samples mDstFb.
    // Standard-orientation source FBs (V=0 = image-bottom) would need
    // FlipY=1; none of the inputs the chain currently runs on are of
    // that flavour, so the uniform is wired to 0.
    GLint flipYLoc = glGetUniformLocation(slot.program, "FlipY");
    if (flipYLoc >= 0) {
        glUniform1i(flipYLoc, 0);
    }

    EnsurePostProcessVao();
    glBindVertexArray(mPostProcessVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Restore the driver-visible VAO/VBO bindings the regular draw path
    // assumes. On Apple / GLES the backend keeps mOpenglVao bound at all
    // times; elsewhere it uses the default VAO (0) and an always-bound
    // mOpenglVbo on GL_ARRAY_BUFFER.
#if defined(__APPLE__) || defined(USE_OPENGLES)
    glBindVertexArray(mOpenglVao);
#else
    glBindVertexArray(0);
#endif
    glBindBuffer(GL_ARRAY_BUFFER, mOpenglVbo);
}

void GfxRenderingAPIOGL::SelectTextureFb(int fb_id) {
    // glDisable(GL_DEPTH_TEST);
    int tile = 0;
    SelectTexture(tile, mFrameBuffers[fb_id].clrbuf);
}

bool GfxRenderingAPIOGL::FbNeedsSampleVFlip(int fb_id) {
    // Empirically: on Linux/NVIDIA OpenGL (driver 595.x), sampling an
    // invertY=true FBO as a GL_TEXTURE_2D returns rows in the SAME
    // direction as the rendered Y-negated storage — V=0 already reads
    // "game top". No flip is needed; applying one renders the
    // FB-passthrough wallpaper / VS photo-wipe upside-down. D3D11 and
    // Metal also use the default-false (no Y negation on the render side,
    // sampling matches storage). Issue #157's original "OpenGL needs a
    // V-flip" diagnosis turned out platform-specific to a different
    // driver/OS and didn't hold on Linux NVIDIA. The safer default is no
    // flip on any backend until a future report identifies one that
    // genuinely needs it; the fb_id bounds check is kept so the method
    // remains a safe per-FB hook for that future case.
    if (fb_id < 0 || (size_t)fb_id >= mFrameBuffers.size()) {
        return false;
    }
    return false;
}

void GfxRenderingAPIOGL::CopyFramebuffer(int fb_dst_id, int fb_src_id, int srcX0, int srcY0, int srcX1, int srcY1,
                                         int dstX0, int dstY0, int dstX1, int dstY1) {
    if (fb_dst_id >= (int)mFrameBuffers.size() || fb_src_id >= (int)mFrameBuffers.size()) {
        return;
    }

    FramebufferOGL src = mFrameBuffers[fb_src_id];
    const FramebufferOGL& dst = mFrameBuffers[fb_dst_id];

    // Adjust y values for non-inverted source frame buffers because opengl uses bottom left for origin
    if (!src.invertY) {
        int temp = srcY1 - srcY0;
        srcY1 = src.height - srcY0;
        srcY0 = srcY1 - temp;
    }

    // Flip the y values
    if (src.invertY != dst.invertY) {
        std::swap(srcY0, srcY1);
    }

    // Disabled for blit
    if (mLastScissorEnabled != 0) {
        mLastScissorEnabled = 0;
        glDisable(GL_SCISSOR_TEST);
    }

    // For msaa enabled buffers we can't perform a scaled blit to a simple sample buffer
    // First do an unscaled blit to a msaa resolved buffer
    if (src.height != dst.height && src.width != dst.width && src.msaa_level > 1) {
        // Start with the main buffer (0) as the msaa resolved buffer
        int fb_resolve_id = 0;
        FramebufferOGL fb_resolve = mFrameBuffers[fb_resolve_id];

        // If the size doesn't match our source, then we need to use our separate color msaa resolved buffer (2)
        if (fb_resolve.height != src.height || fb_resolve.width != src.width) {
            fb_resolve_id = 2;
            fb_resolve = mFrameBuffers[fb_resolve_id];
        }

        glBindFramebuffer(GL_READ_FRAMEBUFFER, src.fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb_resolve.fbo);

        glBlitFramebuffer(0, 0, src.width, src.height, 0, 0, src.width, src.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        // Switch source buffer to the resolved sample
        fb_src_id = fb_resolve_id;
        src = fb_resolve;
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, src.fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst.fbo);

    // The 0 buffer is a double buffer so we need to choose the back to avoid imgui elements
    if (fb_src_id == 0) {
        glReadBuffer(GL_BACK);
    } else {
        glReadBuffer(GL_COLOR_ATTACHMENT0);
    }

    glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffers[mCurrentFrameBuffer].fbo);

    glReadBuffer(GL_BACK);

    if (mLastScissorEnabled != 1) {
        mLastScissorEnabled = 1;
        glEnable(GL_SCISSOR_TEST);
    }
}

void GfxRenderingAPIOGL::ReadFramebufferToCPU(int fb_id, uint32_t width, uint32_t height, uint16_t* rgba16_buf) {
    if (fb_id >= (int)mFrameBuffers.size()) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffers[fb_id].fbo);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, (void*)rgba16_buf);
    glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffers[mCurrentFrameBuffer].fbo);
}

std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff>
GfxRenderingAPIOGL::GetPixelDepth(int fb_id, const std::set<std::pair<float, float>>& coordinates) {
    std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff> res;

    FramebufferOGL& fb = mFrameBuffers[fb_id];

    // When looking up one value and the framebuffer is single-sampled, we can read pixels directly
    // Otherwise we need to blit first to a new buffer then read it
    if (coordinates.size() == 1 && fb.msaa_level <= 1) {
        uint32_t depth_stencil_value;
        glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);
        int x = coordinates.begin()->first;
        int y = coordinates.begin()->second;
#ifndef USE_OPENGLES // not supported on gles. Runs fine without it, but this may cause issues
        glReadPixels(x, fb.invertY ? fb.height - y : y, 1, 1, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8,
                     &depth_stencil_value);
#endif
        res.emplace(*coordinates.begin(), (depth_stencil_value >> 18) << 2);
    } else {
        if (mPixelDepthRbSize < coordinates.size()) {
            // Resizing a renderbuffer seems broken with Intel's driver, so recreate one instead.
            glBindFramebuffer(GL_FRAMEBUFFER, mPixelDepthFb);
            glDeleteRenderbuffers(1, &mPixelDepthRb);
            glGenRenderbuffers(1, &mPixelDepthRb);
            glBindRenderbuffer(GL_RENDERBUFFER, mPixelDepthRb);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, coordinates.size(), 1);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mPixelDepthRb);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            mPixelDepthRbSize = coordinates.size();
        }

        glBindFramebuffer(GL_READ_FRAMEBUFFER, fb.fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mPixelDepthFb);

        glDisable(GL_SCISSOR_TEST); // needed for the blit operation

        {
            size_t i = 0;
            for (const auto& coord : coordinates) {
                int x = coord.first;
                int y = coord.second;
                if (fb.invertY) {
                    y = fb.height - y;
                }
                glBlitFramebuffer(x, y, x + 1, y + 1, i, 0, i + 1, 1, GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
                                  GL_NEAREST);
                ++i;
            }
        }

        glBindFramebuffer(GL_READ_FRAMEBUFFER, mPixelDepthFb);
        std::vector<uint32_t> depth_stencil_values(coordinates.size());
#ifndef USE_OPENGLES // not supported on gles. Runs fine without it, but this may cause issues
        glReadPixels(0, 0, coordinates.size(), 1, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, depth_stencil_values.data());
#endif
        {
            size_t i = 0;
            for (const auto& coord : coordinates) {
                res.emplace(coord, (depth_stencil_values[i++] >> 18) << 2);
            }
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, mCurrentFrameBuffer);

    return res;
}

void GfxRenderingAPIOGL::SetTextureFilter(FilteringMode mode) {
    gfx_texture_cache_clear();
    mCurrentFilterMode = mode;
}

FilteringMode GfxRenderingAPIOGL::GetTextureFilter() {
    return mCurrentFilterMode;
}

void GfxRenderingAPIOGL::SetSrgbMode() {
    mSrgbMode = true;
}

ImTextureID GfxRenderingAPIOGL::GetTextureById(int id) {
    return reinterpret_cast<ImTextureID>(id);
}
} // namespace Fast
#endif

#pragma clang diagnostic pop
