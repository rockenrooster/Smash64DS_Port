// Implemented from the plan in docs/crt_shader_plan_2026-05-11.md §3 / §4
// and the libretro single-file GLSL conventions documented at
// https://github.com/libretro/glsl-shaders/blob/master/README.md. No code
// copied from RetroArch or any GPL-licensed shader runtime.

#include "fast/postprocess/PostProcessGlslNormalizer.h"

#include <cctype>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

namespace Fast {

namespace {

bool IsIdChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

// In-place whole-word replace. Identifier-style boundary check on both
// sides — `Texture` matches `Texture` and `Texture.xy` but not
// `TextureSize` or `MyTexture`.
void RewriteIdentifier(std::string& src, const std::string& from, const std::string& to) {
    if (from.empty() || from == to) {
        return;
    }
    size_t pos = 0;
    while ((pos = src.find(from, pos)) != std::string::npos) {
        const bool leftOk = (pos == 0) || !IsIdChar(src[pos - 1]);
        const size_t end = pos + from.size();
        const bool rightOk = (end >= src.size()) || !IsIdChar(src[end]);
        if (leftOk && rightOk) {
            src.replace(pos, from.size(), to);
            pos += to.size();
        } else {
            pos += 1;
        }
    }
}

// Token-aware "does this line contain identifier X as a whole word".
bool LineHasIdentifier(const std::string& line, const char* token) {
    const size_t n = std::strlen(token);
    size_t pos = 0;
    while ((pos = line.find(token, pos)) != std::string::npos) {
        const bool leftOk = (pos == 0) || !IsIdChar(line[pos - 1]);
        const bool rightOk = (pos + n >= line.size()) || !IsIdChar(line[pos + n]);
        if (leftOk && rightOk) {
            return true;
        }
        pos += 1;
    }
    return false;
}

bool LineStartsWithKeyword(const std::string& line, const char* keyword) {
    const size_t firstNonWs = line.find_first_not_of(" \t");
    if (firstNonWs == std::string::npos) {
        return false;
    }
    const size_t n = std::strlen(keyword);
    if (line.size() - firstNonWs < n) {
        return false;
    }
    if (line.compare(firstNonWs, n, keyword) != 0) {
        return false;
    }
    const size_t after = firstNonWs + n;
    if (after >= line.size()) {
        return true;
    }
    const char c = line[after];
    // Keyword must terminate (whitespace / EOL / common punctuation). This
    // avoids matching `in` inside `int` or `varying` inside `varying2`.
    return !IsIdChar(c);
}

// Schema names AFTER identifier rewrites — these are the canonical
// identifiers our preamble declares. Any line that declares one of them
// is dropped so we can re-introduce them with consistent types and
// (in the transpile path) explicit Vulkan bindings.
constexpr const char* kSchemaIdentifiers[] = {
    "Source", "SourceSize", "OutputSize", "InputSize", "FrameCount",
    "FrameDirection", "MVPMatrix", "vTexCoord", "fragColor",
    // Phase 2D: libretro multipass-shader bindings.
    "Original", "OriginalSize",
};

// Per-alias check: strip the user's `uniform sampler2D <alias>` and
// `uniform vec2 <alias>Size` declarations so the preamble's
// canonical, binding-explicit versions don't collide. The preamble
// injects both the sampler and the matching `<alias>Size` vec2 for
// every alias / external-texture name — chain-side bookkeeping
// populates them from PostProcessExtraBinding::{width,height} every
// frame.
bool IsAliasDeclarationLine(const std::string& line, const std::vector<std::string>& aliases) {
    if (!LineStartsWithKeyword(line, "uniform")) {
        return false;
    }
    for (const std::string& a : aliases) {
        if (a.empty()) {
            continue;
        }
        if (LineHasIdentifier(line, a.c_str())) {
            return true;
        }
        // `<alias>Size` would not whole-word match `<alias>` (the
        // trailing 'S' is an id-char), so check the size identifier
        // explicitly to catch user declarations like
        // `uniform vec2 MyAliasSize;`.
        const std::string sizeName = a + "Size";
        if (LineHasIdentifier(line, sizeName.c_str())) {
            return true;
        }
    }
    return false;
}

bool IsSchemaDeclarationLine(const std::string& line) {
    // Strip every top-level `in` / `varying` / `IN` / `attribute` / `out`
    // / `OUT` declaration. A fragment-shader post-process pass only has
    // one varying input (vTexCoord) and one output (fragColor) — both
    // are provided by the runtime preamble. Anything else the user
    // declared either matches that role (and would duplicate our
    // declaration) or is wired to a vertex stage that doesn't exist in
    // our fixed stock VS. Either way, dropping the user line and
    // letting the preamble win is the correct outcome.
    //
    // This catches edge cases like `IN vec2 Coord;` paired with
    // `#define Coord TEX0` — after the text-rewrite stage above
    // `TEX0 → vTexCoord` would make the user's declaration expand to
    // `in vec2 vTexCoord;` and collide with our preamble. By stripping
    // the declaration unconditionally we side-step the macro-chain.
    if (LineStartsWithKeyword(line, "in") || LineStartsWithKeyword(line, "varying") ||
        LineStartsWithKeyword(line, "IN") || LineStartsWithKeyword(line, "attribute") ||
        LineStartsWithKeyword(line, "out") || LineStartsWithKeyword(line, "OUT") ||
        // libretro's GLSL-version-portability macros. Both expand to
        // varying/in/out/attribute depending on __VERSION__; stripping the
        // line at the macro-prefix avoids a chain `COMPAT_VARYING vec4
        // TEX0;` -> `varying vec4 vTexCoord;` -> collision with the
        // preamble after our TEX0 -> vTexCoord rewrite.
        LineStartsWithKeyword(line, "COMPAT_VARYING") ||
        LineStartsWithKeyword(line, "COMPAT_ATTRIBUTE")) {
        return true;
    }
    // Uniforms: only strip the schema-aliased ones — the user is
    // allowed to keep their own custom uniforms (shader parameters
    // etc.) so the preamble doesn't have to enumerate them.
    if (!LineStartsWithKeyword(line, "uniform")) {
        return false;
    }
    for (const char* id : kSchemaIdentifiers) {
        if (LineHasIdentifier(line, id)) {
            return true;
        }
    }
    return false;
}

// Preamble emitted at the top of the normalized output. Order matters
// only for legibility; the `#define FRAGMENT` makes libretro single-file
// shaders select their FRAGMENT branch when compiled under our forced
// `#version 330 core`.
constexpr const char* kPreamble =
    "#version 330 core\n"
    "// Preamble injected by LUS PostProcessGlslNormalizer.\n"
    "#define FRAGMENT 1\n"
    // PRECISION is libretro's portability shim for GLSL-ES precision
    // qualifiers; outside ES it expands to nothing. Declaring it here
    // (rather than text-rewriting) keeps the user's own `#define
    // PRECISION` lines syntactically valid — they just redefine an
    // already-empty macro, which the GLSL preprocessor permits.
    "#define PRECISION\n"
    "uniform sampler2D Source;\n"
    "uniform sampler2D Original;\n"
    "uniform vec2 SourceSize;\n"
    "uniform vec2 OutputSize;\n"
    "uniform vec2 InputSize;\n"
    "uniform vec2 OriginalSize;\n"
    "uniform int FrameCount;\n"
    "uniform float FrameDirection;\n"
    "in vec2 vTexCoord;\n"
    "out vec4 fragColor;\n";

// Extract `<varying_name> = <expr>;` assignments out of the VERTEX
// branch's main() so we can inline them into the FRAGMENT half as
// `#define <name> (<expr>)`. The libretro single-file convention has
// the VS compute simple uniform-derived varyings (e.g.
// `onex = vec2(SourceSize.z, 0.0)`) and ship them across to the FS
// via `COMPAT_VARYING vec2 onex;`. Our pipeline has a fixed stock VS
// that only emits vTexCoord, so without this fixup any FS that reads
// such a varying fails to compile (`undeclared identifier`).
//
// Limitations the caller should be aware of:
//   * Assignments that target a swizzle (`TEX0.xy = TexCoord.xy;`)
//     are skipped — `TEX0` is already remapped to vTexCoord and we
//     can't macro-define a swizzle target. The FS-side varying line
//     for that target gets stripped by the schema rule above, which
//     is the right behavior here.
//   * Expressions that read VS-only attributes (VertexCoord, COLOR,
//     TexCoord) are kept verbatim and will fail at FS compile time
//     because those identifiers don't exist there. We can't fix this
//     without a full GLSL parser — when it happens, the failure is
//     surfaced via the normalized-source dump in MakeSource().
//   * Multi-statement varying derivations (intermediate locals, if /
//     for blocks) aren't reconstructed. The dump-on-failure path is
//     the user-visible escape hatch.
struct VsVaryingAssignment {
    std::string name;
    std::string type; // Informational; the `#define` doesn't carry it.
    std::string expr;
};

void ExtractVsVaryings(const std::string& src,
                       std::vector<VsVaryingAssignment>& out) {
    out.clear();

    // Step 1 — locate the VERTEX block.
    const size_t vsStart = src.find("#if defined(VERTEX)");
    const size_t vsStartAlt = src.find("#ifdef VERTEX");
    size_t vsBegin = std::min(vsStart, vsStartAlt);
    if (vsBegin == std::string::npos) {
        return;
    }
    // VS block ends at the matching `#elif defined(FRAGMENT)` at the
    // SAME nesting depth, or at the matching outer `#endif` that
    // closes the `#if defined(VERTEX)` opener. Real libretro shaders
    // (crt-geom, crt-hyllian, broadcast, …) nest `#if __VERSION__ >=
    // 130 ... #endif` and `#ifdef GL_ES ... #endif` inside the VS
    // block; a depth-blind first-`#endif` search would close at the
    // first inner `#endif` and miss every COMPAT_VARYING declaration
    // (which sit between the inner blocks and the FS marker), so
    // ExtractVsVaryings would silently produce no `#define` macros
    // and the FS would fail to compile against the stripped
    // varyings. Track preprocessor depth line-by-line and stop at
    // depth 0.
    size_t vsEnd = src.size();
    {
        int depth = 1; // Past the `#if defined(VERTEX)` line.
        size_t cursor = vsBegin;
        // Advance cursor past the opening directive's line.
        const size_t firstNl = src.find('\n', vsBegin);
        cursor = (firstNl == std::string::npos) ? src.size() : firstNl + 1;
        while (cursor < src.size()) {
            const size_t nextNl = src.find('\n', cursor);
            const size_t lineEnd = (nextNl == std::string::npos) ? src.size() : nextNl;
            // Find the first non-whitespace char on this line and check
            // whether it begins a preprocessor directive that affects
            // depth.
            size_t scan = cursor;
            while (scan < lineEnd && (src[scan] == ' ' || src[scan] == '\t')) {
                ++scan;
            }
            if (scan < lineEnd && src[scan] == '#') {
                ++scan;
                while (scan < lineEnd && (src[scan] == ' ' || src[scan] == '\t')) {
                    ++scan;
                }
                auto matchesKw = [&](const char* kw) -> bool {
                    const size_t n = std::strlen(kw);
                    if (scan + n > lineEnd) return false;
                    if (src.compare(scan, n, kw) != 0) return false;
                    if (scan + n == lineEnd) return true;
                    const char c = src[scan + n];
                    return !IsIdChar(c);
                };
                // Order matters: `#ifdef`/`#ifndef`/`#if` must be
                // checked from longest to shortest so the prefix match
                // doesn't false-positive shorter keywords.
                if (matchesKw("ifdef") || matchesKw("ifndef") || matchesKw("if")) {
                    ++depth;
                } else if (matchesKw("endif")) {
                    --depth;
                    if (depth == 0) {
                        vsEnd = cursor;
                        break;
                    }
                } else if (depth == 1 && matchesKw("elif")) {
                    // `#elif` at the same level as the opener — this
                    // is the FRAGMENT branch starting (or, in the
                    // legacy single-file convention, a flip-VERTEX
                    // case). Either way, the VS body ends here.
                    vsEnd = cursor;
                    break;
                } else if (depth == 1 && matchesKw("else")) {
                    // `#else` at the opener's level closes the VS
                    // body — the FS body begins on the next line.
                    vsEnd = cursor;
                    break;
                }
            }
            if (nextNl == std::string::npos) {
                break;
            }
            cursor = nextNl + 1;
        }
    }
    const std::string vsBlock = src.substr(vsBegin, vsEnd - vsBegin);

    // Step 2 — collect the names of every COMPAT_VARYING declared in
    // the VS block. We use this set to filter assignments later so we
    // don't mistakenly inline a local-variable assignment that happens
    // to share an identifier with something else.
    std::vector<std::string> declaredVaryings;
    std::vector<std::string> declaredTypes;
    {
        std::istringstream in(vsBlock);
        std::string line;
        while (std::getline(in, line)) {
            const size_t firstNonWs = line.find_first_not_of(" \t");
            if (firstNonWs == std::string::npos) continue;
            if (line.compare(firstNonWs, 14, "COMPAT_VARYING") != 0) continue;
            // Parse "COMPAT_VARYING <type> <name>;"
            size_t pos = firstNonWs + 14;
            while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) ++pos;
            size_t typeBegin = pos;
            while (pos < line.size() && IsIdChar(line[pos])) ++pos;
            const std::string type = line.substr(typeBegin, pos - typeBegin);
            while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) ++pos;
            size_t nameBegin = pos;
            while (pos < line.size() && IsIdChar(line[pos])) ++pos;
            const std::string name = line.substr(nameBegin, pos - nameBegin);
            if (!name.empty() && !type.empty()) {
                declaredVaryings.push_back(name);
                declaredTypes.push_back(type);
            }
        }
    }
    if (declaredVaryings.empty()) {
        return;
    }

    // Step 3 — find void main() { ... } inside the VS block. We tolerate
    // formatting variance (`void  main()`, newlines before `{`).
    const size_t mainKw = vsBlock.find("void main");
    if (mainKw == std::string::npos) {
        return;
    }
    const size_t openBrace = vsBlock.find('{', mainKw);
    if (openBrace == std::string::npos) {
        return;
    }
    // Walk forward tracking brace depth to find the matching close.
    int depth = 0;
    size_t closeBrace = std::string::npos;
    for (size_t i = openBrace; i < vsBlock.size(); ++i) {
        const char c = vsBlock[i];
        if (c == '{') ++depth;
        else if (c == '}') {
            --depth;
            if (depth == 0) {
                closeBrace = i;
                break;
            }
        }
    }
    if (closeBrace == std::string::npos) {
        return;
    }
    const std::string mainBody = vsBlock.substr(openBrace + 1, closeBrace - openBrace - 1);

    // Step 4 — line-by-line: look for `<name> = <expr>;` where <name>
    // is one of our declared varyings (whole-word match, no swizzle).
    std::istringstream bodyIn(mainBody);
    std::string line;
    while (std::getline(bodyIn, line)) {
        const size_t firstNonWs = line.find_first_not_of(" \t");
        if (firstNonWs == std::string::npos) continue;
        // Identifier on LHS.
        size_t pos = firstNonWs;
        while (pos < line.size() && IsIdChar(line[pos])) ++pos;
        if (pos == firstNonWs) continue;
        const std::string lhs = line.substr(firstNonWs, pos - firstNonWs);
        // Skip swizzle / member access — those aren't macro-able.
        while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) ++pos;
        if (pos >= line.size() || line[pos] != '=') continue;
        // Be defensive against `==` and compound-assign operators
        // (`+=`, `*=`, etc.) — we only want a plain `=`.
        if (pos + 1 < line.size() && line[pos + 1] == '=') continue;
        // Also skip if the previous non-whitespace char is one of the
        // compound-assign operator-starters (the loop above already
        // skipped whitespace, so the immediate char before pos in the
        // original line is what matters).
        ++pos; // step past '='
        // Right-hand side runs until the trailing semicolon.
        const size_t semi = line.find(';', pos);
        if (semi == std::string::npos) continue;
        // Trim leading/trailing whitespace on the expression.
        size_t exprBegin = pos;
        while (exprBegin < semi && (line[exprBegin] == ' ' || line[exprBegin] == '\t')) ++exprBegin;
        size_t exprEnd = semi;
        while (exprEnd > exprBegin && (line[exprEnd - 1] == ' ' || line[exprEnd - 1] == '\t')) --exprEnd;
        if (exprBegin >= exprEnd) continue;
        const std::string expr = line.substr(exprBegin, exprEnd - exprBegin);

        // Match against the declared-varying list.
        for (size_t i = 0; i < declaredVaryings.size(); ++i) {
            if (declaredVaryings[i] == lhs) {
                VsVaryingAssignment va;
                va.name = lhs;
                va.type = declaredTypes[i];
                va.expr = expr;
                out.push_back(std::move(va));
                break;
            }
        }
    }
}

} // namespace

std::string NormalizeUserGlsl(const std::string& src) {
    return NormalizeUserGlsl(src, {});
}

std::string NormalizeUserGlsl(const std::string& src,
                              const std::vector<std::string>& aliasNames) {
    // Step 1 — rewrite libretro / legacy identifier names to our schema.
    // Done as a whole-buffer pass before line-walking so that downstream
    // strip decisions see canonical names regardless of which alias the
    // author used.
    std::string body = src;
    // Sampler-name aliases drawn from the public libretro shader-format
    // portability conventions (libretro/glsl-shaders README + the
    // libretro shader-spec doc pages). `Texture` and `OrigTexture` are
    // the documented legacy spellings of what later became `Source` and
    // `Original`. `s_p` is an older alternate spelling for the source
    // sampler that some shaders still ship. Listed here for portability;
    // every rewrite is identifier-by-identifier and not derived from
    // reading any specific shader's source.
    //
    // Order matters: the longer identifier (`OrigTexture`) must be
    // rewritten before its prefix pattern (`Texture`) so the whole-word
    // match grabs the longer form first.
    RewriteIdentifier(body, "OrigTexture", "Original");
    RewriteIdentifier(body, "OrigInputSize", "OriginalSize");
    RewriteIdentifier(body, "Texture", "Source");
    RewriteIdentifier(body, "s_p", "Source");
    // `Source` reads against a `SourceSize` query — but if the file used
    // `TextureSize` we already rewrote it twice (Texture → Source then
    // SourceSize already present). The order matters: rewrite the
    // composite name BEFORE the substring it contains so we don't
    // produce `SourceSize` from `Texture` + later code. The
    // RewriteIdentifier function is whole-word so this is safe — listed
    // for clarity:
    RewriteIdentifier(body, "TextureSize", "SourceSize");
    // Texture-coordinate varyings. libretro VS exports `TexCoord`; FS
    // sees it as `texCoord` (older naming) or `TEX0` (newer).
    RewriteIdentifier(body, "TEX0", "vTexCoord");
    RewriteIdentifier(body, "TexCoord", "vTexCoord");
    RewriteIdentifier(body, "texCoord", "vTexCoord");
    // Fragment output. libretro's combined-file convention uses
    // `FragColor` (named `out`) at __VERSION__ >= 130, falling back to
    // `gl_FragColor` (built-in) at < 130. We force #version 330 so the
    // `out FragColor` path is selected; rewrite that name to ours.
    RewriteIdentifier(body, "FragColor", "fragColor");
    // gl_FragColor: removed in GLSL 330 core, so any user shader that
    // still references it would fail to compile under our forced version
    // anyway. We rewrite for consistency when authors fall through to
    // the legacy branch by accident.
    RewriteIdentifier(body, "gl_FragColor", "fragColor");
    // Legacy GLSL <= 130 / ES sampler-typed lookup names. GLSL 330 core
    // has the unified `texture()` overload set. The libretro `tex2D` /
    // `IN` / `OUT` macros are handled by the shader's own `#if
    // __VERSION__ >= 130` block, which selects the modern names under
    // our forced version. Direct uses still need rewriting:
    RewriteIdentifier(body, "texture2D", "texture");
    RewriteIdentifier(body, "texture3D", "texture");
    RewriteIdentifier(body, "textureCube", "texture");

    // Step 1.5 — pull VS-half varying assignments out of the
    // POST-REWRITE body. Running this on the rewritten buffer means
    // each emitted `#define <name> (<expr>)` references the same
    // identifier names the FS body sees after rewrite (`SourceSize`,
    // `vTexCoord`, `Source`, ...), so when the FS expands the macro
    // it never names an identifier that was renamed away.
    //
    // Edge case: if the user declared `COMPAT_VARYING vec2 TextureSize;`
    // (i.e. `TextureSize` as a per-vertex varying, not a uniform), the
    // rewrite turned it into `COMPAT_VARYING vec2 SourceSize;`. The
    // resulting `#define SourceSize (...)` shadows the preamble's
    // `uniform vec2 SourceSize;` for FS reads — which is the desired
    // behaviour, because the shader expected the VS-computed value.
    std::vector<VsVaryingAssignment> vsVaryings;
    ExtractVsVaryings(body, vsVaryings);

    // Step 2 — walk lines, drop our own future preamble's worth of
    // declarations plus user `#version` / `#pragma parameter` directives.
    std::istringstream in(body);
    std::string line;
    std::vector<std::string> out;
    out.reserve(body.size() / 32 + 16);
    while (std::getline(in, line)) {
        const size_t firstNonWs = line.find_first_not_of(" \t");
        if (firstNonWs != std::string::npos) {
            // Strip user `#version`; we force our own.
            if (line.compare(firstNonWs, 8, "#version") == 0) {
                continue;
            }
            // Strip `#pragma parameter`; the UI doesn't surface these
            // yet, and glslang warns on unknown pragmas. The `#define`
            // fall-back blocks in the shaders themselves carry the
            // default values, so nothing visual changes.
            if (line.compare(firstNonWs, 17, "#pragma parameter") == 0) {
                continue;
            }
        }
        if (IsSchemaDeclarationLine(line)) {
            continue;
        }
        if (!aliasNames.empty() && IsAliasDeclarationLine(line, aliasNames)) {
            continue;
        }
        out.push_back(line);
    }

    std::string normalized;
    normalized.reserve(body.size() + 512);
    normalized += kPreamble;
    // Inline VS-half varying assignments as #define macros. The VS
    // computed these per-vertex and shipped them across the rasterizer
    // as varyings; the FS reads them via `COMPAT_VARYING <type>
    // <name>;` declarations that we strip below to avoid colliding
    // with our preamble. Re-introducing them as macros gives the FS
    // the same per-pixel value the per-vertex / per-pixel
    // interpolation would have produced when the expression is
    // viewport-uniform (the libretro convention for these varyings).
    // Inserted before the alias declarations so a user shader doing
    // `<varying>Size` resolution doesn't shadow our preamble's
    // alias-Size uniform.
    if (!vsVaryings.empty()) {
        normalized += "// VS->FS varying inlines (LUS PostProcessGlslNormalizer).\n";
        for (const VsVaryingAssignment& va : vsVaryings) {
            // Wrap the expression in parens so subsequent member access
            // (`onex.x`) parses correctly as `(expr).x`.
            normalized += "#define ";
            normalized += va.name;
            normalized += " (";
            normalized += va.expr;
            normalized += ")\n";
        }
    }
    // Append per-alias sampler declarations after the canonical
    // preamble so they share the schema's strip / re-inject contract.
    // Names come from the .glslp `aliasN` keys (plus any
    // `textures = "..."` external-texture names) via the chain, in
    // declaration order — slot binding (2 + i) follows the same
    // ordering, so the transpiler can match without an out-of-band
    // sync. The matching `<alias>Size` vec2 is emitted alongside the
    // sampler so shaders that read filtered dimensions (libretro
    // multipass / lookup-texture shaders) see real values; the
    // transpiler appends the same names to the UBO block, and the
    // backends populate them from PostProcessExtraBinding::{width,
    // height} each frame.
    for (const std::string& alias : aliasNames) {
        if (alias.empty()) {
            continue;
        }
        normalized += "uniform sampler2D ";
        normalized += alias;
        normalized += ";\n";
        normalized += "uniform vec2 ";
        normalized += alias;
        normalized += "Size;\n";
    }
    for (const auto& l : out) {
        normalized += l;
        normalized += '\n';
    }
    return normalized;
}

} // namespace Fast
