// Smoke tests for the GLSL->HLSL/MSL post-process transpiler.
// Implemented from the plan in docs/crt_shader_plan_2026-05-11.md §3.2.
// No code copied from RetroArch or any GPL-licensed shader runtime.

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "fast/postprocess/PostProcessGlslNormalizer.h"
#include "fast/postprocess/PostProcessTranspiler.h"
#include "fast/postprocess/PostProcessTypes.h"

namespace {

constexpr const char* kMinimalShader = R"(#version 330 core
in vec2 vTexCoord;
out vec4 fragColor;
uniform sampler2D Source;
uniform vec2 SourceSize;
uniform vec2 OutputSize;
uniform vec2 InputSize;
uniform int FrameCount;
void main() {
    vec3 c = texture(Source, vTexCoord).rgb;
    float row = vTexCoord.y * SourceSize.y;
    c *= mix(0.75, 1.0, 0.5 + 0.5 * cos(row * 6.283185));
    fragColor = vec4(c, 1.0);
}
)";

} // namespace

TEST(PostProcessTranspiler, FillsBothLanguagesFromGlsl) {
    Fast::PostProcessSource src;
    src.name = "smoke";
    src.glsl = kMinimalShader;

    // Transpiler implementation lives in libultraship (the test links
    // against). Whether SynthesizeMissing actually does the transpile is
    // a libultraship build-config decision (LUS_ENABLE_POSTPROCESS_TRANSPILER);
    // here we just assert the visible contract: success populates the
    // language slots, failure leaves them empty with a non-empty errOut.
    std::string err;
    const bool ok = Fast::PostProcessTranspiler::SynthesizeMissing(src, err);
    if (ok) {
        EXPECT_FALSE(src.hlsl.empty()) << "HLSL slot was not populated";
        EXPECT_FALSE(src.msl.empty()) << "MSL slot was not populated";
        // Spot-check that the backend conventions are in the output. We
        // don't pin exact strings (SPIRV-Cross output is implementation-
        // defined), only the entry-point names the backend looks up.
        EXPECT_NE(src.hlsl.find("VSMain"), std::string::npos);
        EXPECT_NE(src.hlsl.find("PSMain"), std::string::npos);
        EXPECT_NE(src.msl.find("postprocess_vertex"), std::string::npos);
        EXPECT_NE(src.msl.find("postprocess_fragment"), std::string::npos);
    } else {
        EXPECT_FALSE(err.empty());
        EXPECT_TRUE(src.hlsl.empty());
        EXPECT_TRUE(src.msl.empty());
    }
}

TEST(PostProcessTranspiler, PreservesExistingSiblings) {
    Fast::PostProcessSource src;
    src.name = "preserved";
    src.glsl = kMinimalShader;
    src.hlsl = "/* user-authored HLSL */";
    src.msl = "/* user-authored MSL */";

    std::string err;
    const bool ok = Fast::PostProcessTranspiler::SynthesizeMissing(src, err);
    EXPECT_TRUE(ok) << err;
    EXPECT_EQ(src.hlsl, "/* user-authored HLSL */");
    EXPECT_EQ(src.msl, "/* user-authored MSL */");
}

// End-to-end smoke test for the libretro single-file path: a combined
// VS+FS GLSL 120 source with libretro identifier names should normalize
// to the canonical schema and transpile to all three backends without
// edits.
TEST(PostProcessTranspiler, AcceptsLibretroSingleFileShader) {
    constexpr const char* kLibretroSrc = R"(
#version 120

#if defined(VERTEX)
attribute vec4 VertexCoord;
attribute vec2 TexCoord;
varying vec2 texCoord;
uniform mat4 MVPMatrix;
void main() {
    gl_Position = MVPMatrix * VertexCoord;
    texCoord = TexCoord;
}
#elif defined(FRAGMENT)
varying vec2 texCoord;
uniform sampler2D Texture;
uniform vec2 TextureSize;
void main() {
    vec4 c = texture2D(Texture, texCoord);
    float row = texCoord.y * TextureSize.y;
    c.rgb *= mix(0.7, 1.0, 0.5 + 0.5 * cos(row * 6.283185));
    gl_FragColor = c;
}
#endif
)";

    const std::string normalized = Fast::NormalizeUserGlsl(kLibretroSrc);
    EXPECT_NE(normalized.find("#version 330 core"), std::string::npos);
    // Libretro identifiers should be gone, our schema present.
    EXPECT_EQ(normalized.find("texture2D("), std::string::npos)
        << "legacy texture2D() left unrewritten";

    Fast::PostProcessSource src;
    src.name = "libretro-test";
    src.glsl = normalized;
    std::string err;
    const bool ok = Fast::PostProcessTranspiler::SynthesizeMissing(src, err);
    EXPECT_TRUE(ok) << err;
    if (ok) {
        EXPECT_FALSE(src.hlsl.empty());
        EXPECT_FALSE(src.msl.empty());
        EXPECT_NE(src.hlsl.find("PSMain"), std::string::npos);
        EXPECT_NE(src.msl.find("postprocess_fragment"), std::string::npos);
    }
}

// Optional: point LUS_TEST_USER_SHADER at any GLSL file on disk and the
// suite will normalize + transpile it. Failures dump the normalized
// output so a diff against the original shows what got rewritten.
TEST(PostProcessTranspiler, AcceptsUserSuppliedShaderFromEnv) {
    const char* path = std::getenv("LUS_TEST_USER_SHADER");
    if (!path || path[0] == '\0') {
        GTEST_SKIP() << "LUS_TEST_USER_SHADER not set";
    }
    std::ifstream in(path);
    ASSERT_TRUE(in.is_open()) << "could not open " << path;
    std::ostringstream ss;
    ss << in.rdbuf();
    const std::string raw = ss.str();
    const std::string norm = Fast::NormalizeUserGlsl(raw);

    Fast::PostProcessSource src;
    src.name = "user-shader";
    src.glsl = norm;
    std::string err;
    const bool ok = Fast::PostProcessTranspiler::SynthesizeMissing(src, err);
    if (!ok) {
        std::ofstream(std::string("/tmp/normalized.glsl")) << norm;
        FAIL() << "transpile failed for " << path << ": " << err
               << "  (normalized form written to /tmp/normalized.glsl)";
    }
    EXPECT_FALSE(src.hlsl.empty());
    EXPECT_FALSE(src.msl.empty());
}

// Regression: libretro single-file shaders sometimes alias the
// varying via `#define <local> TEX0` then redeclare the local name
// (`IN vec2 <local>;`). After the normalizer's text-rewrite of
// `TEX0 -> vTexCoord`, the user's redeclaration would expand to
// `in vec2 vTexCoord;` and collide with the preamble's. Stripping
// all user `in/varying/out` declarations sidesteps the macro chain.
TEST(PostProcessTranspiler, AcceptsTexCoordAliasIndirection) {
    constexpr const char* kSrc = R"(
#version 120
#if defined(VERTEX)
attribute vec2 TexCoord;
varying vec2 Coord;
void main() {
    gl_Position = vec4(0.0);
    Coord = TexCoord;
}
#elif defined(FRAGMENT)
#define Coord TEX0
varying vec2 Coord;
uniform sampler2D Texture;
uniform vec2 TextureSize;
void main() {
    vec3 c = texture2D(Texture, Coord).rgb;
    float r = Coord.y * TextureSize.y;
    c *= 0.5 + 0.5 * cos(r);
    gl_FragColor = vec4(c, 1.0);
}
#endif
)";
    const std::string normalized = Fast::NormalizeUserGlsl(kSrc);
    Fast::PostProcessSource src;
    src.name = "alias-indirection";
    src.glsl = normalized;
    std::string err;
    EXPECT_TRUE(Fast::PostProcessTranspiler::SynthesizeMissing(src, err)) << err;
}

// Phase 2D: a fragment shader referencing both Source and Original
// (libretro multipass halation-style) should transpile cleanly, and
// the emitted HLSL/MSL should bind Original to the second sampler
// slot — t1/s1 in HLSL, texture(1)/sampler(1) in MSL.
TEST(PostProcessTranspiler, BindsOriginalToSecondSlot) {
    constexpr const char* kSrc = R"(#version 330 core
in vec2 vTexCoord;
out vec4 fragColor;
uniform sampler2D Source;
uniform sampler2D Original;
uniform vec2 SourceSize;
uniform vec2 OriginalSize;
void main() {
    vec3 bloom = texture(Source, vTexCoord).rgb;
    vec3 base = texture(Original, vTexCoord).rgb;
    fragColor = vec4(base + bloom * 0.5, 1.0);
}
)";
    Fast::PostProcessSource src;
    src.name = "halation-test";
    src.glsl = kSrc;
    std::string err;
    ASSERT_TRUE(Fast::PostProcessTranspiler::SynthesizeMissing(src, err)) << err;
    EXPECT_FALSE(src.hlsl.empty());
    EXPECT_FALSE(src.msl.empty());
    // HLSL register convention: Source at t0/s0, Original at t1/s1.
    // SPIRV-Cross emits the SamplerState companion as `_<name>_sampler`.
    EXPECT_NE(src.hlsl.find("Source : register(t0)"), std::string::npos);
    EXPECT_NE(src.hlsl.find("Original : register(t1)"), std::string::npos);
    // MSL: explicit slot attributes from the resource-binding overrides.
    EXPECT_NE(src.msl.find("[[texture(0)]]"), std::string::npos);
    EXPECT_NE(src.msl.find("[[texture(1)]]"), std::string::npos);
}

TEST(PostProcessTranspiler, BindsAliasSamplersToHigherSlots) {
    // A pass that samples both Source and an alias-bound earlier-pass
    // output via the libretro `aliasN` convention. The runtime
    // populates PostProcessSource::aliasNames so the normalizer and
    // transpiler reserve slot 2 (= 2 + 0) for the first alias.
    constexpr const char* kSrc = R"(#version 330 core
in vec2 vTexCoord;
out vec4 fragColor;
uniform sampler2D Source;
uniform sampler2D LinearizePass;
void main() {
    vec3 a = texture(Source, vTexCoord).rgb;
    vec3 b = texture(LinearizePass, vTexCoord).rgb;
    fragColor = vec4(a + b, 1.0);
}
)";
    Fast::PostProcessSource src;
    src.name = "alias-test";
    src.glsl = Fast::NormalizeUserGlsl(kSrc, { "LinearizePass" });
    src.aliasNames = { "LinearizePass" };
    std::string err;
    ASSERT_TRUE(Fast::PostProcessTranspiler::SynthesizeMissing(src, err)) << err;
    EXPECT_FALSE(src.hlsl.empty());
    EXPECT_FALSE(src.msl.empty());
    // HLSL: alias lands at t2/s2 (Source=0, Original=1, alias=2+idx).
    EXPECT_NE(src.hlsl.find("LinearizePass : register(t2)"), std::string::npos);
    // MSL: texture(2) attribute via add_msl_resource_binding.
    EXPECT_NE(src.msl.find("[[texture(2)]]"), std::string::npos);
}

// Phase 2.1: every alias gets a matching `vec2 <name>Size` uniform
// declaration in the normalized GLSL, AND a corresponding member at
// the tail of the transpiled UBO. The runtime backends populate
// these from PostProcessExtraBinding::{width,height} per frame.
TEST(PostProcessTranspiler, EmitsAliasSizeUniforms) {
    constexpr const char* kSrc = R"(#version 330 core
in vec2 vTexCoord;
out vec4 fragColor;
uniform sampler2D Source;
uniform sampler2D LinearizePass;
void main() {
    // Reference the alias's size so the optimizer can't strip it.
    vec3 a = texture(Source, vTexCoord).rgb;
    vec3 b = texture(LinearizePass, vTexCoord / LinearizePassSize).rgb;
    fragColor = vec4(a + b, 1.0);
}
)";
    const std::string normalized =
        Fast::NormalizeUserGlsl(kSrc, { "LinearizePass" });
    // The normalizer must declare both the sampler AND the matching
    // Size vec2 in the preamble.
    EXPECT_NE(normalized.find("uniform sampler2D LinearizePass;"),
              std::string::npos);
    EXPECT_NE(normalized.find("uniform vec2 LinearizePassSize;"),
              std::string::npos);

    Fast::PostProcessSource src;
    src.name = "alias-size-test";
    src.glsl = normalized;
    src.aliasNames = { "LinearizePass" };
    std::string err;
    ASSERT_TRUE(Fast::PostProcessTranspiler::SynthesizeMissing(src, err)) << err;
    EXPECT_FALSE(src.hlsl.empty());
    EXPECT_FALSE(src.msl.empty());
    // The trailing UBO member appears as `LinearizePassSize` in both
    // backends' output (SPIRV-Cross preserves the source identifier).
    EXPECT_NE(src.hlsl.find("LinearizePassSize"), std::string::npos)
        << "HLSL transpile dropped the alias-size UBO member";
    EXPECT_NE(src.msl.find("LinearizePassSize"), std::string::npos)
        << "MSL transpile dropped the alias-size UBO member";
}

// User-declared `uniform vec2 <alias>Size;` must be stripped — the
// preamble + transpiler re-inject canonical, binding-explicit
// declarations and a duplicate would either shadow the schema slot
// or land at an unexpected binding number.
TEST(PostProcessTranspiler, StripsUserDeclaredAliasSize) {
    constexpr const char* kSrc = R"(#version 330 core
in vec2 vTexCoord;
out vec4 fragColor;
uniform sampler2D Source;
uniform sampler2D BloomPass;
uniform vec2 BloomPassSize;
void main() {
    vec3 b = texture(BloomPass, vTexCoord / BloomPassSize).rgb;
    fragColor = vec4(b, 1.0);
}
)";
    const std::string normalized =
        Fast::NormalizeUserGlsl(kSrc, { "BloomPass" });
    // Exactly one `BloomPassSize` declaration should survive — the
    // one the normalizer's preamble emits. The user's redundant
    // declaration is dropped along with the alias-sampler line.
    size_t count = 0;
    size_t pos = 0;
    while ((pos = normalized.find("uniform vec2 BloomPassSize", pos)) !=
           std::string::npos) {
        ++count;
        pos += 1;
    }
    EXPECT_EQ(count, 1u) << "preamble + user declarations should not duplicate";
}

// Optional: point LUS_TEST_USER_SHADER_DIR at a directory and the
// suite walks every `.glsl` directly inside it (non-recursive),
// normalizing + transpiling each. The test is informational — it
// always passes — but it dumps a per-file PASS/FAIL summary plus
// the aggregate reject percentage so a CI run can flag regressions
// when the normalizer changes.
//
// Usage:
//   LUS_TEST_USER_SHADER_DIR=/path/to/libretro/glsl-shaders \
//     ./build/libultraship/tests/lus_tests \
//     --gtest_filter=PostProcessTranspiler.AcceptsUserSuppliedShaderDirectory
//
// We deliberately don't fail on rejects because (a) some inputs
// genuinely require multi-pass support we haven't shipped yet and
// (b) the percentage is what matters across the corpus, not any one
// shader. Use the normalizer-tests file for hard regression assertions
// on specific shader shapes.
TEST(PostProcessTranspiler, AcceptsUserSuppliedShaderDirectory) {
    const char* dir = std::getenv("LUS_TEST_USER_SHADER_DIR");
    if (!dir || dir[0] == '\0') {
        GTEST_SKIP() << "LUS_TEST_USER_SHADER_DIR not set";
    }
    std::filesystem::path root(dir);
    std::error_code ec;
    if (!std::filesystem::is_directory(root, ec)) {
        GTEST_SKIP() << "LUS_TEST_USER_SHADER_DIR is not a directory: " << dir;
    }

    size_t total = 0;
    size_t passed = 0;
    std::vector<std::string> failures;

    for (auto& entry : std::filesystem::directory_iterator(root, ec)) {
        if (ec) break;
        if (!entry.is_regular_file()) continue;
        const auto& path = entry.path();
        if (path.extension() != ".glsl") continue;
        ++total;

        std::ifstream in(path);
        if (!in.is_open()) {
            failures.push_back(path.filename().string() + " : open failed");
            continue;
        }
        std::ostringstream ss;
        ss << in.rdbuf();
        const std::string raw = ss.str();
        const std::string norm = Fast::NormalizeUserGlsl(raw);

        Fast::PostProcessSource src;
        src.name = path.stem().string();
        src.glsl = norm;
        std::string err;
        if (Fast::PostProcessTranspiler::SynthesizeMissing(src, err)) {
            ++passed;
        } else {
            // Trim the error to a single line so the summary stays
            // legible. Most glslang errors are noisy — the first line
            // (parse-error location + symbol) is enough to triage.
            std::string firstLine = err.substr(0, err.find('\n'));
            failures.push_back(path.filename().string() + " : " + firstLine);
        }
    }

    if (total == 0) {
        GTEST_SKIP() << "no .glsl files in " << dir;
    }

    const double passRate = (100.0 * static_cast<double>(passed)) /
                             static_cast<double>(total);
    std::cerr << "[corpus] " << passed << " / " << total
              << " shaders transpiled (" << passRate << "%)\n";
    if (!failures.empty()) {
        // Cap the dump to keep CI logs sane — first 25 are usually
        // representative.
        const size_t cap = std::min<size_t>(failures.size(), 25);
        std::cerr << "[corpus] failures (showing " << cap << " of "
                  << failures.size() << "):\n";
        for (size_t i = 0; i < cap; ++i) {
            std::cerr << "  - " << failures[i] << "\n";
        }
    }
    // Informational: never fail. Use the normalizer-tests file for
    // hard regression assertions.
    SUCCEED();
}

TEST(PostProcessTranspiler, RejectsEmptyGlsl) {
    Fast::PostProcessSource src;
    src.name = "empty";

    std::string err;
    const bool ok = Fast::PostProcessTranspiler::SynthesizeMissing(src, err);
    EXPECT_FALSE(ok);
    EXPECT_FALSE(err.empty());
}
