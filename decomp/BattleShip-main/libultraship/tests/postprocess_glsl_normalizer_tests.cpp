// Implemented from the libretro single-file GLSL conventions documented at
// https://github.com/libretro/glsl-shaders/blob/master/README.md and the
// plan in docs/crt_shader_plan_2026-05-11.md §3 / §4. Tests use synthetic
// shader text — no shader-corpus file contents are reproduced. No code
// copied from RetroArch or any GPL-licensed shader runtime.

#include <gtest/gtest.h>

#include <string>

#include "fast/postprocess/PostProcessGlslNormalizer.h"
#include "fast/postprocess/PostProcessTranspiler.h"
#include "fast/postprocess/PostProcessTypes.h"

namespace {

bool Contains(const std::string& haystack, const char* needle) {
    return haystack.find(needle) != std::string::npos;
}

size_t CountOccurrences(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return 0;
    size_t count = 0;
    size_t pos = 0;
    while ((pos = haystack.find(needle, pos)) != std::string::npos) {
        ++count;
        pos += needle.size();
    }
    return count;
}

} // namespace

// A FRAGMENT-only post-process shader that already follows the LUS
// schema must round-trip through NormalizeUserGlsl with at most a
// `#version` reset — the body shouldn't be mutated, the preamble
// shouldn't double-up the schema uniforms.
TEST(PostProcessGlslNormalizer, PassesThroughLusSchemaShaderUnchanged) {
    const std::string src = R"(#version 330 core
in vec2 vTexCoord;
out vec4 fragColor;
uniform sampler2D Source;
uniform vec2 SourceSize;
void main() {
    fragColor = texture(Source, vTexCoord);
}
)";
    const std::string out = Fast::NormalizeUserGlsl(src);
    EXPECT_TRUE(Contains(out, "#version 330 core"));
    // Each schema uniform must appear exactly once after normalization —
    // the user's redeclarations are stripped and the preamble's win.
    EXPECT_EQ(CountOccurrences(out, "uniform sampler2D Source"), 1u);
    EXPECT_EQ(CountOccurrences(out, "uniform vec2 SourceSize"), 1u);
    EXPECT_EQ(CountOccurrences(out, "out vec4 fragColor"), 1u);
}

// Identifier rewrites: legacy libretro names (Texture / TextureSize /
// TEX0 / FragColor / texture2D) all map to our schema. A local
// variable that happens to be named with one of the alias prefixes
// (e.g. `TextureSize` as a local) is the documented limitation; we
// do NOT test that here.
TEST(PostProcessGlslNormalizer, RewritesLibretroIdentifiers) {
    const std::string src = R"(#version 120
varying vec2 TEX0;
uniform sampler2D Texture;
uniform vec2 TextureSize;
void main() {
    vec4 c = texture2D(Texture, TEX0);
    float row = TEX0.y * TextureSize.y;
    gl_FragColor = c * mix(0.7, 1.0, sin(row));
}
)";
    const std::string out = Fast::NormalizeUserGlsl(src);
    EXPECT_FALSE(Contains(out, "texture2D("));
    EXPECT_FALSE(Contains(out, "gl_FragColor"));
    // After identifier rewrite, every TEX0 / Texture / TextureSize /
    // FragColor reference reads against our schema names.
    EXPECT_TRUE(Contains(out, "vTexCoord"));
    EXPECT_TRUE(Contains(out, "fragColor"));
    // Texture-as-Source rewrite landed; no leftover `Texture(` call.
    // (Whole-word: `Texture` body of texture2D() invocation rewrites
    //  to `Source`, then texture2D -> texture, leaving `texture(Source`.)
    EXPECT_TRUE(Contains(out, "texture(Source"));
}

// The combined VS+FS libretro convention with COMPAT_VARYING
// declarations must produce valid FS source: the FS-side
// COMPAT_VARYING is stripped (per schema rule), so the VS-side
// `<varying> = <expr>;` assignment must be lifted into a `#define`
// in the FS — otherwise the FS references an undeclared identifier.
//
// This single-`#if defined(VERTEX) ... #elif defined(FRAGMENT) ... #endif`
// shape is the simplest libretro form; the `NestedVsIfdef` test below
// covers the more common shape that nests `#if __VERSION__ >= 130 ...
// #endif` inside the VERTEX block.
TEST(PostProcessGlslNormalizer, InlinesFlatVsVaryingIntoFs) {
    const std::string src = R"(#version 120
#if defined(VERTEX)
COMPAT_VARYING vec2 onex;
attribute vec4 VertexCoord;
attribute vec2 TexCoord;
varying vec2 TEX0;
uniform vec2 TextureSize;
void main() {
    gl_Position = VertexCoord;
    TEX0 = TexCoord;
    onex = vec2(1.0 / TextureSize.x, 0.0);
}
#elif defined(FRAGMENT)
COMPAT_VARYING vec2 onex;
varying vec2 TEX0;
uniform sampler2D Texture;
uniform vec2 TextureSize;
void main() {
    vec4 c = texture2D(Texture, TEX0 + onex);
    gl_FragColor = c;
}
#endif
)";
    const std::string out = Fast::NormalizeUserGlsl(src);
    // The FS references onex; the normalizer must have lifted the VS
    // assignment into a `#define onex (...)` macro.
    EXPECT_TRUE(Contains(out, "#define onex"))
        << "VS-half varying assignment was not inlined as a #define; FS "
           "would fail to compile because COMPAT_VARYING was stripped.";
    // The macro body should reference our schema's SourceSize (rewritten
    // from the original `TextureSize`) — the rewrite happens on the
    // whole buffer before extraction, so the inlined expression uses
    // the canonical name.
    EXPECT_TRUE(Contains(out, "SourceSize.x"));
    // The FS-side COMPAT_VARYING declaration was stripped (schema rule).
    EXPECT_FALSE(Contains(out, "COMPAT_VARYING vec2 onex"));
}

// REGRESSION: real libretro single-file shaders (crt-geom, broadcast,
// CRT-Beam, etc.) nest preprocessor blocks inside `#if defined(VERTEX)`:
//
//   #if defined(VERTEX)
//   #if __VERSION__ >= 130
//   #define COMPAT_VARYING out
//   ...
//   #endif    <-- inner
//   ...
//   COMPAT_VARYING vec2 sinangle;       <-- declaration
//   void main() {
//       sinangle = ...;                 <-- assignment we need to lift
//   }
//   #elif defined(FRAGMENT)
//   ...
//   #endif    <-- outer
//
// ExtractVsVaryings used to terminate `vsBlock` at the FIRST `#endif`
// after `#if defined(VERTEX)`, which here is the inner one. That cut
// off both the COMPAT_VARYING declarations and the main() body, so
// `declaredVaryings` came back empty and no `#define`s were emitted.
// Under our `#define FRAGMENT 1` preamble, the FS branch then read
// `sinangle` / `cosangle` / `stretch` as undeclared identifiers and
// glslang rejected the shader.
//
// The fix is a depth-aware walk that pairs each nested `#if*` with
// its matching `#endif`, so the VS block is treated as everything
// between `#if defined(VERTEX)` and either `#elif defined(FRAGMENT)`
// at the same depth or the matching outer `#endif`.
TEST(PostProcessGlslNormalizer, InlinesNestedVsVaryingIntoFs) {
    const std::string src = R"(#version 120

#if defined(VERTEX)
#if __VERSION__ >= 130
#define COMPAT_VARYING out
#define COMPAT_ATTRIBUTE in
#else
#define COMPAT_VARYING varying
#define COMPAT_ATTRIBUTE attribute
#endif

COMPAT_ATTRIBUTE vec4 VertexCoord;
COMPAT_ATTRIBUTE vec2 TexCoord;
COMPAT_VARYING vec2 TEX0;
COMPAT_VARYING vec2 sinangle;

uniform vec2 TextureSize;

void main() {
    gl_Position = VertexCoord;
    TEX0 = TexCoord;
    sinangle = vec2(0.5, 0.0) + TextureSize * 0.0;
}

#elif defined(FRAGMENT)

#if __VERSION__ >= 130
#define COMPAT_VARYING in
#define COMPAT_TEXTURE texture
#else
#define COMPAT_VARYING varying
#define COMPAT_TEXTURE texture2D
#endif

COMPAT_VARYING vec2 TEX0;
COMPAT_VARYING vec2 sinangle;

uniform sampler2D Texture;
uniform vec2 TextureSize;

void main() {
    vec4 c = COMPAT_TEXTURE(Texture, TEX0 + sinangle);
    gl_FragColor = c;
}

#endif
)";
    const std::string out = Fast::NormalizeUserGlsl(src);
    EXPECT_TRUE(Contains(out, "#define sinangle"))
        << "Nested #if inside the VS block prevented ExtractVsVaryings "
           "from finding `sinangle = ...;` (the VS block was truncated "
           "at the first inner #endif). Without the #define, the FS sees "
           "`sinangle` as undeclared and glslang rejects the shader.";
    // FS-side declaration was stripped by the schema rule, so the macro
    // is the only definition left in the output.
    EXPECT_FALSE(Contains(out, "COMPAT_VARYING vec2 sinangle"));
}

// Once the nested-VS bug is fixed the normalized output must also
// successfully transpile through glslang+SPIRV-Cross: the macro
// expansion provides a defined value for the FS's `sinangle`
// reference. This test pins the end-to-end contract — when it goes
// green, the libretro `crt-geom` family of shaders is unblocked.
TEST(PostProcessGlslNormalizer, NestedVsVaryingShaderTranspiles) {
    const std::string src = R"(#version 120

#if defined(VERTEX)
#if __VERSION__ >= 130
#define COMPAT_VARYING out
#define COMPAT_ATTRIBUTE in
#else
#define COMPAT_VARYING varying
#define COMPAT_ATTRIBUTE attribute
#endif

COMPAT_ATTRIBUTE vec4 VertexCoord;
COMPAT_ATTRIBUTE vec2 TexCoord;
COMPAT_VARYING vec2 TEX0;
COMPAT_VARYING vec2 sinangle;
uniform vec2 TextureSize;

void main() {
    gl_Position = VertexCoord;
    TEX0 = TexCoord;
    sinangle = vec2(0.5, 0.5);
}

#elif defined(FRAGMENT)

#if __VERSION__ >= 130
#define COMPAT_VARYING in
#define COMPAT_TEXTURE texture
#else
#define COMPAT_VARYING varying
#define COMPAT_TEXTURE texture2D
#endif

COMPAT_VARYING vec2 TEX0;
COMPAT_VARYING vec2 sinangle;
uniform sampler2D Texture;
uniform vec2 TextureSize;

void main() {
    vec4 c = COMPAT_TEXTURE(Texture, TEX0 + sinangle);
    gl_FragColor = c;
}

#endif
)";
    const std::string normalized = Fast::NormalizeUserGlsl(src);

    Fast::PostProcessSource ps;
    ps.name = "nested-vs-shader";
    ps.glsl = normalized;
    std::string err;
    const bool ok = Fast::PostProcessTranspiler::SynthesizeMissing(ps, err);
    EXPECT_TRUE(ok) << "transpile failed; normalizer left an undeclared "
                       "identifier in the FS body. Error: " << err;
}

// REGRESSION: ExtractVsVaryings used to run on the pre-rewrite source,
// so the macro it emitted carried the original libretro identifier
// names (`TextureSize`, `TEX0`). The FS body has those rewritten away
// (to `SourceSize`, `vTexCoord`); when the FS expands the macro, the
// substituted text named identifiers that no longer existed and
// glslang rejected the shader. The fix runs ExtractVsVaryings on the
// post-rewrite buffer so macros and the FS body share one name-space.
TEST(PostProcessGlslNormalizer, VsVaryingMacroBodyHonorsIdentifierRewrite) {
    // Shape modelled on libretro broadcast.glsl / zfast_crt_nogeo.glsl:
    // a VS that computes `invdims = 1.0/TextureSize;` (a varying named
    // `invdims` whose derivation references the libretro `TextureSize`
    // alias). The FS reads `invdims` and expects the macro to expand
    // into something that resolves under our schema.
    const std::string src = R"(#version 120
#if defined(VERTEX)
COMPAT_VARYING vec2 invdims;
attribute vec4 VertexCoord;
attribute vec2 TexCoord;
varying vec2 TEX0;
uniform vec2 TextureSize;
void main() {
    gl_Position = VertexCoord;
    TEX0 = TexCoord;
    invdims = 1.0 / TextureSize;
}
#elif defined(FRAGMENT)
COMPAT_VARYING vec2 invdims;
varying vec2 TEX0;
uniform sampler2D Texture;
uniform vec2 TextureSize;
void main() {
    gl_FragColor = texture2D(Texture, TEX0 + invdims);
}
#endif
)";
    const std::string out = Fast::NormalizeUserGlsl(src);
    // The macro must reference our schema name (`SourceSize`) — the
    // original `TextureSize` is no longer declared anywhere in the
    // normalized output.
    EXPECT_TRUE(Contains(out, "#define invdims"));
    EXPECT_TRUE(Contains(out, "SourceSize"));
    // After rewrite + extraction, no stale `TextureSize` token should
    // remain — every prior occurrence either belonged to a stripped
    // declaration or got rewritten.
    EXPECT_FALSE(Contains(out, "TextureSize"))
        << "Macro body or surviving body line still references the "
           "pre-rewrite identifier — FS expansion will refer to an "
           "undeclared symbol.";
    Fast::PostProcessSource ps;
    ps.name = "macro-rewrite";
    ps.glsl = out;
    std::string err;
    EXPECT_TRUE(Fast::PostProcessTranspiler::SynthesizeMissing(ps, err)) << err;
}

// REGRESSION: a libretro shader that declares a sampler the runtime
// has no binding for (e.g. `uniform sampler2D PassPrev3Texture;`)
// must still load. The Vulkan-targeted preprocessor auto-promotes
// the line to `layout(set=0, binding=N) uniform sampler2D
// PassPrev3Texture;` so glslang accepts it. The runtime never binds
// data, so sampling returns zero — a visual artifact rather than a
// hard load failure.
TEST(PostProcessGlslNormalizer, UserSamplerWithoutAliasStillTranspiles) {
    const std::string src = R"(#version 330 core
in vec2 vTexCoord;
out vec4 fragColor;
uniform sampler2D Source;
uniform sampler2D PassPrev3Texture;
void main() {
    vec4 a = texture(Source, vTexCoord);
    vec4 b = texture(PassPrev3Texture, vTexCoord);
    fragColor = a + b * 0.25;
}
)";
    const std::string normalized = Fast::NormalizeUserGlsl(src);
    Fast::PostProcessSource ps;
    ps.name = "user-sampler";
    ps.glsl = normalized;
    std::string err;
    EXPECT_TRUE(Fast::PostProcessTranspiler::SynthesizeMissing(ps, err)) << err;
}

// `#ifdef VERTEX` is the older spelling of `#if defined(VERTEX)`. The
// extractor must recognise both — some libretro shaders still ship the
// short form.
TEST(PostProcessGlslNormalizer, AcceptsIfdefVertexSpelling) {
    const std::string src = R"(#version 120
#ifdef VERTEX
COMPAT_VARYING vec2 onex;
void main() {
    onex = vec2(0.25, 0.0);
}
#endif
#ifdef FRAGMENT
COMPAT_VARYING vec2 onex;
uniform sampler2D Texture;
void main() {
    gl_FragColor = texture2D(Texture, onex);
}
#endif
)";
    const std::string out = Fast::NormalizeUserGlsl(src);
    EXPECT_TRUE(Contains(out, "#define onex"))
        << "ExtractVsVaryings missed the `#ifdef VERTEX` short spelling.";
}

// Compound-assign operators (`+=`, `*=`, `==`) must not be treated as
// plain assignments — otherwise we'd lift a partial expression as the
// macro body and the FS would see a malformed `#define`.
TEST(PostProcessGlslNormalizer, IgnoresCompoundAssignAndComparison) {
    const std::string src = R"(#version 120
#if defined(VERTEX)
COMPAT_VARYING vec2 acc;
COMPAT_VARYING float pivot;
void main() {
    acc = vec2(1.0);
    acc += vec2(0.5);   // not an assignment we want to lift.
    if (pivot == 1.0) {
        // not an assignment.
    }
}
#elif defined(FRAGMENT)
COMPAT_VARYING vec2 acc;
COMPAT_VARYING float pivot;
uniform sampler2D Texture;
void main() {
    gl_FragColor = texture2D(Texture, acc) * pivot;
}
#endif
)";
    const std::string out = Fast::NormalizeUserGlsl(src);
    // Only the FIRST `acc = ...;` should be lifted.
    EXPECT_EQ(CountOccurrences(out, "#define acc"), 1u);
    // `acc +=` would expand into something like `#define acc +` which
    // would corrupt subsequent uses — make sure it didn't.
    EXPECT_FALSE(Contains(out, "#define acc +"));
    // Comparison `pivot == 1.0` must not generate a `#define pivot` —
    // there was no plain `pivot = ...;` in main().
    EXPECT_FALSE(Contains(out, "#define pivot"));
}

// Aliases supplied via the .glslp `aliasN` mechanism get sampler +
// `<name>Size` declarations injected by the preamble. The user's own
// declaration of either name (via `uniform sampler2D <alias>;` or
// `uniform vec2 <alias>Size;`) must be stripped so the schema slot
// wins. Covers Phase 2.1 + 2H.
TEST(PostProcessGlslNormalizer, InjectsAliasSamplerAndSize) {
    const std::string src = R"(#version 330 core
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
    const std::string out = Fast::NormalizeUserGlsl(src, { "BloomPass" });
    EXPECT_EQ(CountOccurrences(out, "uniform sampler2D BloomPass"), 1u);
    EXPECT_EQ(CountOccurrences(out, "uniform vec2 BloomPassSize"), 1u);
}

// Multiple aliases are emitted in declaration order so the transpiler's
// resource-binding overrides (which assign slot 2+i to alias i) line
// up with what the chain populates per frame.
TEST(PostProcessGlslNormalizer, InjectsMultipleAliasesInOrder) {
    const std::string src = R"(#version 330 core
in vec2 vTexCoord;
out vec4 fragColor;
uniform sampler2D Source;
void main() {
    fragColor = texture(Source, vTexCoord);
}
)";
    const std::string out = Fast::NormalizeUserGlsl(src,
        { "FirstPass", "SecondPass", "ThirdPass" });
    const size_t firstPos  = out.find("uniform sampler2D FirstPass");
    const size_t secondPos = out.find("uniform sampler2D SecondPass");
    const size_t thirdPos  = out.find("uniform sampler2D ThirdPass");
    ASSERT_NE(firstPos, std::string::npos);
    ASSERT_NE(secondPos, std::string::npos);
    ASSERT_NE(thirdPos, std::string::npos);
    EXPECT_LT(firstPos, secondPos);
    EXPECT_LT(secondPos, thirdPos);
}

// `#pragma parameter` lines are accepted and stripped (no glslang
// warning), and the schema preamble is unaffected. This is the line
// that lets a libretro shader's parameter-default `#define` block
// take over without UI surfacing the slider.
TEST(PostProcessGlslNormalizer, StripsPragmaParameterLines) {
    const std::string src = R"(#version 330 core
#pragma parameter sharpness "Sharpness" 0.5 0.0 1.0 0.05
#pragma parameter mask "Mask" 0.7 0.0 1.0
in vec2 vTexCoord;
out vec4 fragColor;
uniform sampler2D Source;
void main() { fragColor = texture(Source, vTexCoord); }
)";
    const std::string out = Fast::NormalizeUserGlsl(src);
    EXPECT_FALSE(Contains(out, "#pragma parameter"));
}

// The FRAGMENT-only convention `#ifdef FRAGMENT ... #endif` (no VERTEX
// half) must still produce a usable FS — i.e. the preamble's `#define
// FRAGMENT 1` selects the FS body. Sanity check: a simple FS-only
// shader that references our schema compiles end-to-end.
TEST(PostProcessGlslNormalizer, FragmentOnlyShaderTranspiles) {
    const std::string src = R"(#version 120
#ifdef FRAGMENT
varying vec2 TEX0;
uniform sampler2D Texture;
void main() {
    gl_FragColor = texture2D(Texture, TEX0);
}
#endif
)";
    const std::string normalized = Fast::NormalizeUserGlsl(src);
    Fast::PostProcessSource ps;
    ps.name = "fs-only";
    ps.glsl = normalized;
    std::string err;
    EXPECT_TRUE(Fast::PostProcessTranspiler::SynthesizeMissing(ps, err)) << err;
}
