// Implemented against the public libretro slang shader format docs.
// Tests use synthetic .slang inputs — no shader-corpus file contents
// are reproduced. No code copied from RetroArch or any GPL-licensed
// shader runtime.

#include <gtest/gtest.h>

#include "fast/postprocess/PostProcessSlangSource.h"

namespace {

// Convenience: assert a substring is present in `haystack`.
bool Contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

} // namespace

TEST(PostProcessSlangSource, ParsesMinimalShader) {
    constexpr const char* kSrc = R"(// minimal slang
#pragma stage vertex
void vmain() {}
#pragma stage fragment
void fmain() {}
)";
    Fast::PostProcessSlangSource out;
    std::string err;
    ASSERT_TRUE(Fast::ParseSlangSource(kSrc, out, err)) << err;
    EXPECT_TRUE(out.name.empty());
    EXPECT_TRUE(out.format.empty());
    EXPECT_TRUE(out.parameters.empty());
    EXPECT_TRUE(Contains(out.vertex, "void vmain()"));
    EXPECT_FALSE(Contains(out.vertex, "void fmain()"));
    EXPECT_TRUE(Contains(out.fragment, "void fmain()"));
    EXPECT_FALSE(Contains(out.fragment, "void vmain()"));
}

TEST(PostProcessSlangSource, StripsStagePragmas) {
    constexpr const char* kSrc = R"(#pragma stage vertex
x
#pragma stage fragment
y
)";
    Fast::PostProcessSlangSource out;
    std::string err;
    ASSERT_TRUE(Fast::ParseSlangSource(kSrc, out, err)) << err;
    EXPECT_FALSE(Contains(out.vertex, "#pragma stage"));
    EXPECT_FALSE(Contains(out.fragment, "#pragma stage"));
}

TEST(PostProcessSlangSource, CommonPreambleAppearsInBothStages) {
    // The line before either #pragma stage is "common" — both the
    // vertex and fragment stage receive it.
    constexpr const char* kSrc = R"(#version 450
layout(set = 0, binding = 0, std140) uniform UBO { mat4 MVP; };
#pragma stage vertex
void vmain() {}
#pragma stage fragment
void fmain() {}
)";
    Fast::PostProcessSlangSource out;
    std::string err;
    ASSERT_TRUE(Fast::ParseSlangSource(kSrc, out, err)) << err;
    EXPECT_TRUE(Contains(out.vertex, "#version 450"));
    EXPECT_TRUE(Contains(out.vertex, "uniform UBO"));
    EXPECT_TRUE(Contains(out.fragment, "#version 450"));
    EXPECT_TRUE(Contains(out.fragment, "uniform UBO"));
    EXPECT_TRUE(Contains(out.vertex, "void vmain()"));
    EXPECT_TRUE(Contains(out.fragment, "void fmain()"));
    EXPECT_FALSE(Contains(out.fragment, "vmain"));
    EXPECT_FALSE(Contains(out.vertex, "fmain"));
}

TEST(PostProcessSlangSource, CapturesNameAndFormat) {
    constexpr const char* kSrc = R"(#pragma name FakeBlur
#pragma format R8G8B8A8_UNORM
#pragma stage vertex
void v() {}
#pragma stage fragment
void f() {}
)";
    Fast::PostProcessSlangSource out;
    std::string err;
    ASSERT_TRUE(Fast::ParseSlangSource(kSrc, out, err)) << err;
    EXPECT_EQ(out.name, "FakeBlur");
    EXPECT_EQ(out.format, "R8G8B8A8_UNORM");
    EXPECT_FALSE(Contains(out.vertex, "#pragma name"));
    EXPECT_FALSE(Contains(out.fragment, "#pragma format"));
}

TEST(PostProcessSlangSource, CapturesSingleParameterWithStep) {
    constexpr const char* kSrc = R"(#pragma parameter crt_curvature "CRT Curvature" 0.25 0.0 1.0 0.01
#pragma stage vertex
void v() {}
#pragma stage fragment
void f() {}
)";
    Fast::PostProcessSlangSource out;
    std::string err;
    ASSERT_TRUE(Fast::ParseSlangSource(kSrc, out, err)) << err;
    ASSERT_EQ(out.parameters.size(), 1u);
    EXPECT_EQ(out.parameters[0].name, "crt_curvature");
    EXPECT_EQ(out.parameters[0].label, "CRT Curvature");
    EXPECT_FLOAT_EQ(out.parameters[0].defaultValue, 0.25f);
    EXPECT_FLOAT_EQ(out.parameters[0].minValue, 0.0f);
    EXPECT_FLOAT_EQ(out.parameters[0].maxValue, 1.0f);
    EXPECT_FLOAT_EQ(out.parameters[0].step, 0.01f);
    EXPECT_FALSE(Contains(out.vertex, "#pragma parameter"));
    EXPECT_FALSE(Contains(out.fragment, "#pragma parameter"));
}

TEST(PostProcessSlangSource, AcceptsParameterWithoutOptionalStep) {
    constexpr const char* kSrc = R"(#pragma parameter gain "Gain" 1.0 0.0 4.0
#pragma stage vertex
void v() {}
#pragma stage fragment
void f() {}
)";
    Fast::PostProcessSlangSource out;
    std::string err;
    ASSERT_TRUE(Fast::ParseSlangSource(kSrc, out, err)) << err;
    ASSERT_EQ(out.parameters.size(), 1u);
    EXPECT_EQ(out.parameters[0].name, "gain");
    EXPECT_FLOAT_EQ(out.parameters[0].defaultValue, 1.0f);
    EXPECT_FLOAT_EQ(out.parameters[0].step, 0.0f); // sentinel for "use default"
}

TEST(PostProcessSlangSource, ParsesMultipleParametersInOrder) {
    constexpr const char* kSrc = R"(#pragma parameter sharpness "Sharpness" 0.5 0.0 1.0 0.05
#pragma parameter scanline_phase "Scanline Phase" 0.0 -3.14159 3.14159 0.01
#pragma parameter mask_strength "Mask Strength" 0.7 0.0 1.0
#pragma stage vertex
void v() {}
#pragma stage fragment
void f() {}
)";
    Fast::PostProcessSlangSource out;
    std::string err;
    ASSERT_TRUE(Fast::ParseSlangSource(kSrc, out, err)) << err;
    ASSERT_EQ(out.parameters.size(), 3u);
    EXPECT_EQ(out.parameters[0].name, "sharpness");
    EXPECT_EQ(out.parameters[1].name, "scanline_phase");
    EXPECT_EQ(out.parameters[2].name, "mask_strength");
    EXPECT_EQ(out.parameters[1].label, "Scanline Phase");
    EXPECT_FLOAT_EQ(out.parameters[1].minValue, -3.14159f);
    EXPECT_FLOAT_EQ(out.parameters[2].step, 0.0f);
}

TEST(PostProcessSlangSource, ParameterLabelWithSpacesAndPunctuation) {
    constexpr const char* kSrc = R"(#pragma parameter abc "A complex, multi-word: label!" 1.0 0.0 2.0
#pragma stage vertex
void v() {}
#pragma stage fragment
void f() {}
)";
    Fast::PostProcessSlangSource out;
    std::string err;
    ASSERT_TRUE(Fast::ParseSlangSource(kSrc, out, err)) << err;
    ASSERT_EQ(out.parameters.size(), 1u);
    EXPECT_EQ(out.parameters[0].label, "A complex, multi-word: label!");
}

TEST(PostProcessSlangSource, RejectsShaderWithoutVertexStage) {
    constexpr const char* kSrc = R"(#pragma stage fragment
void f() {}
)";
    Fast::PostProcessSlangSource out;
    std::string err;
    EXPECT_FALSE(Fast::ParseSlangSource(kSrc, out, err));
    EXPECT_TRUE(Contains(err, "stage vertex"));
}

TEST(PostProcessSlangSource, RejectsShaderWithoutFragmentStage) {
    constexpr const char* kSrc = R"(#pragma stage vertex
void v() {}
)";
    Fast::PostProcessSlangSource out;
    std::string err;
    EXPECT_FALSE(Fast::ParseSlangSource(kSrc, out, err));
    EXPECT_TRUE(Contains(err, "stage fragment"));
}

TEST(PostProcessSlangSource, RejectsUnknownStage) {
    constexpr const char* kSrc = R"(#pragma stage compute
void k() {}
)";
    Fast::PostProcessSlangSource out;
    std::string err;
    EXPECT_FALSE(Fast::ParseSlangSource(kSrc, out, err));
    EXPECT_TRUE(Contains(err, "compute"));
}

TEST(PostProcessSlangSource, RejectsParameterMissingFields) {
    constexpr const char* kSrc = R"(#pragma parameter only_two "Two Fields" 1.0
#pragma stage vertex
void v() {}
#pragma stage fragment
void f() {}
)";
    Fast::PostProcessSlangSource out;
    std::string err;
    EXPECT_FALSE(Fast::ParseSlangSource(kSrc, out, err));
    EXPECT_TRUE(Contains(err, "min"));
}

TEST(PostProcessSlangSource, RejectsParameterNonNumericValue) {
    constexpr const char* kSrc = R"(#pragma parameter junk "Junk" abc 0.0 1.0
#pragma stage vertex
void v() {}
#pragma stage fragment
void f() {}
)";
    Fast::PostProcessSlangSource out;
    std::string err;
    EXPECT_FALSE(Fast::ParseSlangSource(kSrc, out, err));
    EXPECT_TRUE(Contains(err, "default"));
}

// Non-slang pragmas (e.g. glslang's own pragma extensions) MUST pass
// through to the output stages unchanged. We only consume the four
// slang-specific pragmas: stage / name / format / parameter.
TEST(PostProcessSlangSource, PassesThroughForeignPragmas) {
    constexpr const char* kSrc = R"(#pragma optimize(on)
#pragma stage vertex
#pragma debug(off)
void v() {}
#pragma stage fragment
void f() {}
)";
    Fast::PostProcessSlangSource out;
    std::string err;
    ASSERT_TRUE(Fast::ParseSlangSource(kSrc, out, err)) << err;
    EXPECT_TRUE(Contains(out.vertex, "#pragma optimize(on)"));
    EXPECT_TRUE(Contains(out.fragment, "#pragma optimize(on)"));
    EXPECT_TRUE(Contains(out.vertex, "#pragma debug(off)"));
    EXPECT_FALSE(Contains(out.fragment, "#pragma debug(off)"));
}

// Authors often write `#pragma name` followed by the value on the
// same line with no quotes, and may leave trailing whitespace. We
// should capture exactly the trimmed argument string.
TEST(PostProcessSlangSource, NameAndFormatTrimWhitespace) {
    constexpr const char* kSrc = "#pragma name    Some Shader   \n"
                                 "#pragma format    R16G16B16A16_SFLOAT   \n"
                                 "#pragma stage vertex\nvoid v(){}\n"
                                 "#pragma stage fragment\nvoid f(){}\n";
    Fast::PostProcessSlangSource out;
    std::string err;
    ASSERT_TRUE(Fast::ParseSlangSource(kSrc, out, err)) << err;
    EXPECT_EQ(out.name, "Some Shader");
    EXPECT_EQ(out.format, "R16G16B16A16_SFLOAT");
}

// `#pragma names` must NOT match `#pragma name`. Whole-word check.
TEST(PostProcessSlangSource, NameMatchesWholeWordOnly) {
    constexpr const char* kSrc = R"(#pragma names_with_underscore_suffix should_be_ignored
#pragma stage vertex
void v(){}
#pragma stage fragment
void f(){}
)";
    Fast::PostProcessSlangSource out;
    std::string err;
    ASSERT_TRUE(Fast::ParseSlangSource(kSrc, out, err)) << err;
    EXPECT_TRUE(out.name.empty());
    // Unknown pragmas pass through to both stages.
    EXPECT_TRUE(Contains(out.vertex, "#pragma names_with_underscore_suffix"));
    EXPECT_TRUE(Contains(out.fragment, "#pragma names_with_underscore_suffix"));
}
