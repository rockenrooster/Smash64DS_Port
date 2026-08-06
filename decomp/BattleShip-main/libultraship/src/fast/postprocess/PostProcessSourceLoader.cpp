// Implemented from the plan in docs/crt_shader_plan_2026-05-11.md §3.4
// and the libretro `.glslp` format documented at libretro/glsl-shaders.
// No code copied from RetroArch or any GPL-licensed shader runtime.
#include "fast/postprocess/PostProcessSourceLoader.h"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <system_error>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <stb_image.h>

#include "fast/postprocess/PostProcessGlslNormalizer.h"
#include "fast/postprocess/PostProcessPreset.h"
#include "fast/postprocess/PostProcessSlangSource.h"
#include "fast/postprocess/PostProcessSlangTranspiler.h"
#include "fast/postprocess/PostProcessTranspiler.h"
#include "ship/Context.h"
#include "ship/resource/ResourceManager.h"
#include "ship/resource/File.h"

namespace Fast {

namespace {

bool ReadFilesystemFile(const std::string& path, std::string& outText) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        return false;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    outText = ss.str();
    return true;
}

// Filesystem candidate roots for user-supplied shaders, tried in order:
//   1. <user-data>/shaders/      — the per-user writable location, where
//      end users drop installed shaders. On NON_PORTABLE builds this
//      resolves to the OS app-data dir (macOS ~/Library/Application
//      Support/<app>/shaders, Linux $XDG_DATA_HOME/<app>/shaders, Windows
//      %APPDATA%\<app>\shaders); on portable builds it collapses to
//      "./shaders" which is the same as candidate 2.
//   2. ./shaders/                — relative-to-CWD development fallback so
//      contributors iterating from a source checkout don't have to copy
//      every shader into Application Support between rebuilds.
//
// Empty / duplicate paths are dropped so we don't probe the same path twice
// on portable builds.
std::vector<std::string> UserShaderRoots() {
    std::vector<std::string> roots;
    try {
        std::string userData = Ship::Context::GetPathRelativeToAppDirectory("shaders");
        if (!userData.empty()) {
            roots.push_back(std::move(userData));
        }
    } catch (...) {
        // Context not yet alive — fall through to the cwd-relative root.
    }
    const std::string cwdRoot = "shaders";
    bool dup = false;
    for (const std::string& existing : roots) {
        if (existing == cwdRoot || existing == "./shaders") {
            dup = true;
            break;
        }
    }
    if (!dup) {
        roots.push_back(cwdRoot);
    }
    return roots;
}

bool ReadArchiveFile(const std::string& path, std::string& outText) {
    auto ctx = Ship::Context::GetInstance();
    if (ctx == nullptr) {
        return false;
    }
    auto rm = ctx->GetResourceManager();
    if (rm == nullptr) {
        return false;
    }
    auto file = rm->LoadFileProcess(path);
    if (file == nullptr || !file->IsLoaded || file->Buffer == nullptr) {
        return false;
    }
    const auto& buf = *file->Buffer;
    if (file->BufferOffset >= buf.size()) {
        return false;
    }
    outText.assign(buf.begin() + file->BufferOffset, buf.end());
    return true;
}

std::string JoinPath(const std::string& base, const std::string& rel) {
    if (base.empty()) {
        return rel;
    }
    std::string out = base;
    if (out.back() != '/') {
        out += '/';
    }
    out += rel;
    return out;
}

bool ReadFilesystemBinary(const std::string& path, std::vector<uint8_t>& outBytes) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        return false;
    }
    in.seekg(0, std::ios::end);
    const std::streamoff len = in.tellg();
    in.seekg(0, std::ios::beg);
    if (len <= 0) {
        return false;
    }
    outBytes.resize(static_cast<size_t>(len));
    in.read(reinterpret_cast<char*>(outBytes.data()), len);
    return in.good() || in.eof();
}

// Read a binary file (no text-mode line conversion) for stb_image
// consumption. Same filesystem-then-archive lookup order as
// ReadShaderFile.
bool ReadShaderBinaryFile(const std::vector<std::string>& fsBases, const std::string& arBase,
                          const std::string& rel, std::vector<uint8_t>& outBytes) {
    for (const std::string& fsBase : fsBases) {
        if (ReadFilesystemBinary(JoinPath(fsBase, rel), outBytes)) {
            return true;
        }
    }
    const std::string arPath = JoinPath(arBase, rel);
    auto ctx = Ship::Context::GetInstance();
    if (ctx == nullptr) {
        return false;
    }
    auto rm = ctx->GetResourceManager();
    if (rm == nullptr) {
        return false;
    }
    auto file = rm->LoadFileProcess(arPath);
    if (file == nullptr || !file->IsLoaded || file->Buffer == nullptr) {
        return false;
    }
    const auto& buf = *file->Buffer;
    if (file->BufferOffset >= buf.size()) {
        return false;
    }
    outBytes.assign(buf.begin() + file->BufferOffset, buf.end());
    return true;
}

// Read a file by trying `<fsBase>/<rel>` for each entry in `fsBases` in
// order, then archive `<arBase>/<rel>`. Returns true and populates
// outText on the first success. `outResolvedPath` (optional) receives
// the resolved disk-or-archive location of the hit — useful for
// SPDLOG_INFO at the load entry point so users can confirm which
// shader file actually loaded after a picker selection.
bool ReadShaderFile(const std::vector<std::string>& fsBases, const std::string& arBase,
                    const std::string& rel, std::string& outText,
                    std::string* outResolvedPath = nullptr) {
    for (const std::string& fsBase : fsBases) {
        const std::string candidate = JoinPath(fsBase, rel);
        if (ReadFilesystemFile(candidate, outText)) {
            if (outResolvedPath != nullptr) {
                *outResolvedPath = "fs:" + candidate;
            }
            return true;
        }
    }
    const std::string arPath = JoinPath(arBase, rel);
    if (ReadArchiveFile(arPath, outText)) {
        if (outResolvedPath != nullptr) {
            *outResolvedPath = "archive:" + arPath;
        }
        return true;
    }
    return false;
}

// Render a fsBases list for log messages: "'a','b'" — caller may
// embed inline.
std::string FormatFsBases(const std::vector<std::string>& fsBases) {
    std::string out;
    for (size_t i = 0; i < fsBases.size(); ++i) {
        if (i > 0) {
            out += ", ";
        }
        out += "'";
        out += fsBases[i];
        out += "'";
    }
    return out;
}

// Build the diagnostic display name for an individual pass. Strips
// any leading directory components and trailing extension so log
// messages stay readable for shaders like
// "demo/multipass/blur_horiz.glsl".
std::string ShortenPassName(const std::string& path) {
    const size_t slash = path.find_last_of('/');
    const size_t begin = (slash == std::string::npos) ? 0 : slash + 1;
    const size_t dot = path.find_last_of('.');
    const size_t end = (dot != std::string::npos && dot > begin) ? dot : path.size();
    return path.substr(begin, end - begin);
}

// Decode a PNG (or any stb_image-supported format) into an RGBA8 byte
// buffer. Returns true on success and populates `outTex.rgba8`,
// `outTex.width`, `outTex.height`; logs and returns false otherwise.
// `pngBytes` is the raw file contents (entire image, not streaming).
bool DecodeExternalTexture(const std::string& diagName,
                           const std::vector<uint8_t>& pngBytes,
                           PostProcessShaderExternalTexture& outTex) {
    int w = 0, h = 0, channels = 0;
    stbi_uc* decoded = stbi_load_from_memory(pngBytes.data(),
                                             static_cast<int>(pngBytes.size()),
                                             &w, &h, &channels,
                                             /*desired_channels=*/4);
    if (decoded == nullptr || w <= 0 || h <= 0) {
        SPDLOG_ERROR("Post-process external texture '{}': stbi_load_from_memory failed ({})",
                     diagName, stbi_failure_reason() ? stbi_failure_reason() : "no detail");
        if (decoded != nullptr) {
            stbi_image_free(decoded);
        }
        return false;
    }
    const size_t byteCount = static_cast<size_t>(w) * static_cast<size_t>(h) * 4u;
    outTex.width = static_cast<uint32_t>(w);
    outTex.height = static_cast<uint32_t>(h);
    outTex.rgba8.assign(decoded, decoded + byteCount);
    stbi_image_free(decoded);
    return true;
}

// Run the user-shader pipeline on a raw GLSL string: normalize ->
// transpile to HLSL/MSL via SPIRV-Cross. Returns a fully-populated
// PostProcessSource ready for PostProcessChain::LoadPasses.
//
// `aliasNames` carries the .glslp `aliasN` declarations from earlier
// passes (or this pass and later — the chain leaves slot bindings
// stable per preset). The normalizer and transpiler use them to
// declare matching `uniform sampler2D <name>` bindings in each
// language. Single-pass `.glsl` files pass an empty vector.
PostProcessSource MakeSource(const std::string& displayName, std::string rawGlsl,
                             const std::vector<std::string>& aliasNames) {
    PostProcessSource src;
    src.name = displayName;
    src.aliasNames = aliasNames;
    src.glsl = NormalizeUserGlsl(rawGlsl, aliasNames);
    std::string err;
    if (!PostProcessTranspiler::SynthesizeMissing(src, err)) {
        SPDLOG_WARN("Post-process shader '{}' could not be transpiled: {}", displayName, err);

        // Publish the failure so the in-game diagnostics panel can show
        // why the shader didn't load without the user having to dig
        // through the log.
        Fast::internal::SetPostProcessRuntimeError(
            std::string("transpile failed: ") + err);

        // Inspect-on-failure: dump the post-normalize source next to
        // the shader file (and, if the user shader-data dir resolves,
        // also in a /tmp-style scratch). Filename mirrors the shader's
        // short name so it's trivially correlated. Best-effort — if
        // the write fails we just skip it; the log already has the
        // parse error.
        try {
            std::string sanitized = displayName;
            for (char& c : sanitized) {
                if (c == '/' || c == '\\') c = '_';
            }
            const std::vector<std::string> roots = UserShaderRoots();
            for (const std::string& root : roots) {
                std::filesystem::path dumpDir = std::filesystem::path(root) / "_failed";
                std::error_code ec;
                std::filesystem::create_directories(dumpDir, ec);
                if (ec) continue;
                const std::filesystem::path dumpPath = dumpDir / (sanitized + ".normalized.glsl");
                std::ofstream out(dumpPath, std::ios::binary | std::ios::trunc);
                if (out.is_open()) {
                    out << "// Normalized output sent to glslang for '" << displayName << "'.\n";
                    out << "// Transpile error follows:\n";
                    out << "// " << err << "\n";
                    out << "// ----- 8< -----\n";
                    out << src.glsl;
                    SPDLOG_WARN("Post-process: normalized source dumped to {}", dumpPath.string());
                    break;
                }
            }
        } catch (...) {
            // Dump is best-effort. Ignore.
        }
    }
    return src;
}

// Compile a raw .slang source string into a PostProcessSlangArtifact.
// Returns true on success. On failure the artifact is left in an
// unspecified state and the reason is logged.
bool MakeSlangArtifact(const std::string& diagName, const std::string& rawSlang,
                       PostProcessSlangArtifact& outArt) {
    PostProcessSlangSource parsed;
    std::string err;
    if (!ParseSlangSource(rawSlang, parsed, err)) {
        SPDLOG_ERROR("Slang post-process shader '{}' parse failed: {}", diagName, err);
        return false;
    }
    if (!PostProcessSlangTranspiler::Compile(parsed, outArt, err)) {
        SPDLOG_ERROR("Slang post-process shader '{}' compile failed: {}", diagName, err);
        return false;
    }
    return true;
}

bool LoadSlangSinglePassBundle(const std::string& name,
                               const std::vector<std::string>& fsBases,
                               const std::string& arBase,
                               PostProcessSlangShaderBundle& out) {
    std::string raw;
    if (!ReadShaderFile(fsBases, arBase, name + ".slang", raw)) {
        return false;
    }
    PostProcessSlangArtifact art;
    if (!MakeSlangArtifact(name, raw, art)) {
        return false;
    }
    out.name = name;
    out.artifacts.clear();
    out.configs.clear();
    out.diagnosticNames.clear();
    out.artifacts.push_back(std::move(art));
    out.configs.emplace_back(); // Default scale_type=source, scale=1.0.
    out.diagnosticNames.push_back(name);
    return true;
}

bool LoadSlangPresetBundle(const std::string& name,
                           const std::vector<std::string>& fsBases,
                           const std::string& arBase,
                           PostProcessSlangShaderBundle& out) {
    std::string presetText;
    if (!ReadShaderFile(fsBases, arBase, name + ".slangp", presetText)) {
        return false;
    }
    PostProcessPreset preset;
    std::string err;
    if (!ParseSlangPreset(presetText, std::string(), preset, err)) {
        SPDLOG_ERROR("Slang post-process preset '{}': {}", name, err);
        return false;
    }

    out.name = name;
    out.artifacts.clear();
    out.configs.clear();
    out.diagnosticNames.clear();
    out.artifacts.reserve(preset.passes.size());
    out.configs.reserve(preset.passes.size());
    out.diagnosticNames.reserve(preset.passes.size());

    for (size_t i = 0; i < preset.passes.size(); ++i) {
        const PostProcessPresetPass& passCfg = preset.passes[i];
        std::string raw;
        if (!ReadShaderFile(fsBases, arBase, passCfg.shaderPath, raw)) {
            SPDLOG_ERROR("Slang preset '{}' pass {}: shader '{}' not found "
                         "(searched filesystem {} and archive '{}')",
                         name, i, passCfg.shaderPath,
                         FormatFsBases(fsBases), arBase);
            return false;
        }
        const std::string displayName =
            name + "[" + std::to_string(i) + "/" + ShortenPassName(passCfg.shaderPath) + "]";
        PostProcessSlangArtifact art;
        if (!MakeSlangArtifact(displayName, raw, art)) {
            return false;
        }
        out.artifacts.push_back(std::move(art));
        out.configs.push_back(passCfg);
        out.diagnosticNames.push_back(displayName);
    }
    return true;
}

bool LoadSinglePassBundle(const std::string& name, const std::vector<std::string>& fsBases,
                          const std::string& arBase, PostProcessShaderBundle& out) {
    std::string raw;
    std::string resolved;
    if (!ReadShaderFile(fsBases, arBase, name + ".glsl", raw, &resolved)) {
        return false;
    }
    size_t lineCount = 1;
    for (char c : raw) {
        if (c == '\n') ++lineCount;
    }
    const bool hasVertex   = raw.find("#ifdef VERTEX") != std::string::npos ||
                             raw.find("defined(VERTEX)") != std::string::npos;
    const bool hasFragment = raw.find("#ifdef FRAGMENT") != std::string::npos ||
                             raw.find("defined(FRAGMENT)") != std::string::npos;
    SPDLOG_INFO("Post-process: single-pass '{}.glsl' resolved to '{}' ({} bytes, {} lines, vs_half={}, fs_half={})",
                name, resolved, raw.size(), lineCount,
                hasVertex ? "yes" : "no", hasFragment ? "yes" : "no");

    out.name = name;
    out.sources.clear();
    out.configs.clear();
    out.sources.push_back(MakeSource(name, std::move(raw), {}));
    out.configs.emplace_back(); // Default scale_type=source, scale=1.0.
    return true;
}

// Parse a .glslp at one of `<fsBase>/<name>.glslp` or `<arBase>/<name>.glslp`,
// load each referenced pass shader through the normalize+transpile
// pipeline, and stuff everything into `out`. The pass shader paths in
// the preset are resolved relative to the preset's own location.
bool LoadPresetBundle(const std::string& name, const std::vector<std::string>& fsBases,
                      const std::string& arBase, PostProcessShaderBundle& out) {
    std::string presetText;
    std::string presetResolved;
    if (!ReadShaderFile(fsBases, arBase, name + ".glslp", presetText, &presetResolved)) {
        return false;
    }
    SPDLOG_INFO("Post-process: preset '{}.glslp' resolved to '{}' ({} bytes)",
                name, presetResolved, presetText.size());
    PostProcessPreset preset;
    std::string err;
    // baseDir is informational; the actual lookup happens against
    // fsBase / arBase below. We pass empty so the preset record
    // doesn't carry stale absolute paths.
    if (!ParsePostProcessPreset(presetText, std::string(), preset, err)) {
        SPDLOG_ERROR("Post-process preset '{}': {}", name, err);
        return false;
    }

    // Collect the preset's named-sampler bindings in declaration order:
    // first the per-pass `aliasN` outputs, then the external
    // `textures = "..."` entries. Each pass's compile sees the same
    // ordered list so the binding slots (2 + index) stay stable across
    // the chain — pass 0 binds slot 2 to the same name pass 4 does, and
    // external textures land at the same slot for every pass that
    // references them.
    std::vector<std::string> aliasNames;
    aliasNames.reserve(preset.passes.size() + preset.textures.size());
    for (const auto& passCfg : preset.passes) {
        if (passCfg.alias.empty()) {
            continue;
        }
        bool dup = false;
        for (const std::string& existing : aliasNames) {
            if (existing == passCfg.alias) {
                dup = true;
                break;
            }
        }
        if (!dup) {
            aliasNames.push_back(passCfg.alias);
        }
    }
    for (const auto& tex : preset.textures) {
        if (tex.name.empty()) {
            continue;
        }
        bool dup = false;
        for (const std::string& existing : aliasNames) {
            if (existing == tex.name) {
                dup = true;
                break;
            }
        }
        if (!dup) {
            aliasNames.push_back(tex.name);
        }
    }

    // Decode every external texture up front. Failure on any one
    // texture aborts the load — better to surface the error
    // immediately than render with a black slot the user will assume
    // is a shader bug. The texture path is resolved relative to the
    // .glslp's directory (filesystem ./shaders/ or archive
    // shaders/postprocess/).
    std::vector<PostProcessShaderExternalTexture> externalTextures;
    externalTextures.reserve(preset.textures.size());
    for (const auto& tex : preset.textures) {
        if (tex.name.empty() || tex.path.empty()) {
            SPDLOG_ERROR("Post-process preset '{}': textures entry missing name or path", name);
            return false;
        }
        std::vector<uint8_t> pngBytes;
        if (!ReadShaderBinaryFile(fsBases, arBase, tex.path, pngBytes)) {
            SPDLOG_ERROR("Post-process preset '{}' texture '{}': '{}' not found "
                         "(searched filesystem {} and archive '{}')",
                         name, tex.name, tex.path, FormatFsBases(fsBases), arBase);
            return false;
        }
        PostProcessShaderExternalTexture loaded;
        loaded.name = tex.name;
        loaded.filterLinear = tex.filterLinear;
        loaded.wrapMode = tex.wrapMode;
        if (!DecodeExternalTexture(name + "/" + tex.name, pngBytes, loaded)) {
            return false;
        }
        externalTextures.push_back(std::move(loaded));
    }

    out.name = name;
    out.sources.clear();
    out.configs.clear();
    out.externalTextures = std::move(externalTextures);
    out.sources.reserve(preset.passes.size());
    out.configs.reserve(preset.passes.size());

    for (size_t i = 0; i < preset.passes.size(); ++i) {
        const PostProcessPresetPass& passCfg = preset.passes[i];
        std::string raw;
        std::string passResolved;
        if (!ReadShaderFile(fsBases, arBase, passCfg.shaderPath, raw, &passResolved)) {
            SPDLOG_ERROR("Post-process preset '{}' pass {}: shader '{}' not found "
                         "(searched filesystem {} and archive '{}')",
                         name, i, passCfg.shaderPath, FormatFsBases(fsBases), arBase);
            return false;
        }
        SPDLOG_INFO("Post-process: preset '{}' pass {} shader '{}' resolved to '{}' ({} bytes)",
                    name, i, passCfg.shaderPath, passResolved, raw.size());
        const std::string displayName =
            name + "[" + std::to_string(i) + "/" + ShortenPassName(passCfg.shaderPath) + "]";
        out.sources.push_back(MakeSource(displayName, std::move(raw), aliasNames));
        out.configs.push_back(passCfg);
    }
    return true;
}

} // namespace

bool LoadPostProcessShader(const std::string& name, PostProcessShaderBundle& out) {
    if (name.empty()) {
        return false;
    }
    const std::vector<std::string> fsBases = UserShaderRoots();
    constexpr const char* kArBase = "shaders/postprocess";

    // Multi-pass presets take priority — a directory containing both a
    // `<name>.glslp` and a `<name>.glsl` is unusual (the .glsl would
    // typically be one of the passes), and if both exist the preset
    // is the more-explicit intent.
    if (LoadPresetBundle(name, fsBases, kArBase, out)) {
        return true;
    }
    if (LoadSinglePassBundle(name, fsBases, kArBase, out)) {
        return true;
    }
    SPDLOG_ERROR("Post-process shader '{}' not found "
                 "(tried filesystem {} and archive '{}/{}.{{glslp,glsl}}')",
                 name, FormatFsBases(fsBases), kArBase, name);
    return false;
}

bool LoadPostProcessSlangShader(const std::string& name,
                                PostProcessSlangShaderBundle& out) {
    if (name.empty()) {
        return false;
    }
    const std::vector<std::string> fsBases = UserShaderRoots();
    constexpr const char* kArBase = "shaders/postprocess";

    // .slangp wins over .slang for the same reason .glslp wins over
    // .glsl — a single-file .slang in a directory next to a .slangp
    // is normally one of the preset's passes, not an independent
    // shader. If both share a stem the preset is the more-explicit
    // intent.
    if (LoadSlangPresetBundle(name, fsBases, kArBase, out)) {
        return true;
    }
    if (LoadSlangSinglePassBundle(name, fsBases, kArBase, out)) {
        return true;
    }
    // Not finding either is the common case (user picked a .glslp
    // shader; the interpreter falls through to the legacy loader).
    // Log at debug, not error.
    SPDLOG_DEBUG("Slang post-process shader '{}' not found "
                 "(tried filesystem {} and archive '{}/{}.{{slangp,slang}}')",
                 name, FormatFsBases(fsBases), kArBase, name);
    return false;
}

std::vector<std::string> ListBuiltinPostProcessShaders() {
    // Mirrors what GenerateF3DO2R packages from
    // libultraship/src/fast/shaders/postprocess/. Hardcoded rather
    // than enumerated at runtime so the menu picker stays stable
    // across archive contents and so a port that ships a stripped
    // f3d.o2r still gets a usable default selection.
    //
    // Limited to the two CC0 single-pass shaders we ship and verify
    // each build. The Phase 3 slang canaries (`slang-scanlines`,
    // `slang-persistence`) are intentionally excluded — they exercise
    // a code path that isn't reliable on this branch yet (the slang
    // stage splitter rejects `#version` lines that aren't at file
    // top, which the bundled samples don't currently honor).
    return { "scanlines", "crt-lottes" };
}

namespace {

// Internal helper: walk `<root>` and append discovered shader short
// names into `byFolder`. `relPrefix` is the folder name as the picker
// should display it ("" for the loose entries directly in `root`,
// "crt" for `<root>/crt/`, etc.); we only descend one level deep
// because libretro's distribution layout (and the downloader's
// extract layout from the UX plan §4) is one-deep, and a recursive
// walk would surface internal "_blur/" helper directories that aren't
// individually loadable.
//
// A shader appearing under two extensions (.glslp and .glsl) is
// recorded once — the .glslp wins, matching LoadPostProcessShader's
// priority — so the picker offers one entry per logical shader.
void ScanShaderDir(const std::filesystem::path& root, const std::string& relPrefix,
                   std::vector<UserPostProcessShaderFolder>& folders, bool recurse) {
    std::error_code ec;
    if (!std::filesystem::exists(root, ec) || !std::filesystem::is_directory(root, ec)) {
        return;
    }

    UserPostProcessShaderFolder bucket;
    bucket.displayName = relPrefix.empty() ? std::string("(loose)") : relPrefix;
    // We surface one entry per shader stem, with preference order
    // .slangp > .slang > .glslp > .glsl — the loader probes in the
    // same order, so the picker entry maps to whichever file the
    // loader will actually consume.
    std::vector<std::string> slangOnly;
    std::vector<std::string> glslpOnly;
    std::vector<std::string> glslOnly;

    for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
        if (ec) {
            break;
        }
        std::error_code stEc;
        if (entry.is_directory(stEc)) {
            if (!recurse) {
                continue;
            }
            const std::string childRel = relPrefix.empty()
                ? entry.path().filename().string()
                : relPrefix + "/" + entry.path().filename().string();
            ScanShaderDir(entry.path(), childRel, folders, /*recurse=*/false);
            continue;
        }
        if (!entry.is_regular_file(stEc)) {
            continue;
        }
        const std::string ext = entry.path().extension().string();
        const std::string stem = entry.path().stem().string();
        if (stem.empty()) {
            continue;
        }
        const std::string shaderRef = relPrefix.empty() ? stem : (relPrefix + "/" + stem);
        if (ext == ".slangp") {
            bucket.shaderNames.push_back(shaderRef);
        } else if (ext == ".slang") {
            slangOnly.push_back(shaderRef);
        } else if (ext == ".glslp") {
            glslpOnly.push_back(shaderRef);
        } else if (ext == ".glsl") {
            glslOnly.push_back(shaderRef);
        }
    }

    // Merge each lower-priority bucket only if its name isn't
    // already covered by a higher-priority extension.
    auto mergeIfNew = [&](const std::vector<std::string>& src) {
        for (const std::string& name : src) {
            bool covered = false;
            for (const std::string& existing : bucket.shaderNames) {
                if (existing == name) {
                    covered = true;
                    break;
                }
            }
            if (!covered) {
                bucket.shaderNames.push_back(name);
            }
        }
    };
    mergeIfNew(slangOnly);
    mergeIfNew(glslpOnly);
    mergeIfNew(glslOnly);

    if (!bucket.shaderNames.empty()) {
        std::sort(bucket.shaderNames.begin(), bucket.shaderNames.end());
        folders.push_back(std::move(bucket));
    }
}

} // namespace

std::vector<UserPostProcessShaderFolder> ListUserPostProcessShaders() {
    std::vector<UserPostProcessShaderFolder> folders;
    const std::vector<std::string> roots = UserShaderRoots();

    // Create the per-user shaders dir on first call so a fresh
    // install has somewhere to drop files into. Best-effort: ignore
    // mkdir failures — the picker still surfaces builtins and any
    // CWD-relative fallback content.
    if (!roots.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(roots.front(), ec);
    }

    // Dedup keyed on short name so a shader installed in both the
    // user-data root and the cwd fallback shows once, with the
    // user-data copy taking effect (matches the loader).
    std::vector<UserPostProcessShaderFolder> raw;
    for (const std::string& root : roots) {
        ScanShaderDir(std::filesystem::path(root), std::string(), raw, /*recurse=*/true);
    }

    // Merge raw entries by displayName, deduping shader names.
    for (auto& bucket : raw) {
        bool merged = false;
        for (auto& existing : folders) {
            if (existing.displayName == bucket.displayName) {
                for (const std::string& name : bucket.shaderNames) {
                    bool dup = false;
                    for (const std::string& have : existing.shaderNames) {
                        if (have == name) {
                            dup = true;
                            break;
                        }
                    }
                    if (!dup) {
                        existing.shaderNames.push_back(name);
                    }
                }
                std::sort(existing.shaderNames.begin(), existing.shaderNames.end());
                merged = true;
                break;
            }
        }
        if (!merged) {
            folders.push_back(std::move(bucket));
        }
    }
    return folders;
}

namespace {

// Probe each user-shader root for `<name>.lus.json` and return the
// first absolute path that exists, or empty when no sidecar is
// installed. `name` may contain a `subdir/stem` separator since the
// picker stores shader refs that way for shaders below the root.
std::string FindSidecarPath(const std::string& name) {
    const std::vector<std::string> roots = UserShaderRoots();
    for (const std::string& root : roots) {
        const std::string candidate = JoinPath(root, name) + ".lus.json";
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec) && std::filesystem::is_regular_file(candidate, ec)) {
            return candidate;
        }
    }
    return std::string();
}

bool IsBuiltinShader(const std::string& name) {
    const std::vector<std::string> builtins = ListBuiltinPostProcessShaders();
    for (const std::string& b : builtins) {
        if (b == name) {
            return true;
        }
    }
    return false;
}

} // namespace

PostProcessShaderInfo GetPostProcessShaderInfo(const std::string& name) {
    PostProcessShaderInfo info;

    // Bundled shaders ship with neutral resolution scaling — the LUS
    // CC0 originals don't ratio TextureSize / InputSize, so they
    // always render correctly regardless of LowResMode.
    if (IsBuiltinShader(name)) {
        info.compat = PostProcessCompat::Any;
        return info;
    }

    const std::string sidecarPath = FindSidecarPath(name);
    if (sidecarPath.empty()) {
        // No sidecar — default to Native so the picker warns rather
        // than silently shipping a small-corner render.
        return info;
    }

    std::string text;
    if (!ReadFilesystemFile(sidecarPath, text)) {
        SPDLOG_WARN("Post-process: sidecar {} could not be opened — treating as no sidecar (compat=native)", sidecarPath);
        return info;
    }

    try {
        const nlohmann::json json = nlohmann::json::parse(text);
        info.fromSidecar = true;
        if (json.contains("compat") && json["compat"].is_string()) {
            const std::string compat = json["compat"].get<std::string>();
            if (compat == "any") {
                info.compat = PostProcessCompat::Any;
            } else if (compat == "native") {
                info.compat = PostProcessCompat::Native;
            } else {
                SPDLOG_WARN("Post-process: sidecar {} declares unknown compat \"{}\" — treating as native",
                            sidecarPath, compat);
            }
        }
        if (json.contains("label") && json["label"].is_string()) {
            info.label = json["label"].get<std::string>();
        }
        if (json.contains("license") && json["license"].is_string()) {
            info.license = json["license"].get<std::string>();
        }
    } catch (const std::exception& e) {
        SPDLOG_WARN("Post-process: failed to parse sidecar {}: {} — treating as no sidecar", sidecarPath, e.what());
        info = PostProcessShaderInfo();
    }

    return info;
}

namespace {

// Runtime diagnostics state. Protected by a single mutex; the writer
// is the renderer thread (chain load / unload), the reader is the UI
// thread (menu draw). Both happen at human-speed frequencies so a
// global lock here is fine.
std::mutex                       g_runtimeDiagMutex;
PostProcessRuntimeDiagnostics    g_runtimeDiag;

} // namespace

PostProcessRuntimeDiagnostics GetPostProcessRuntimeDiagnostics() {
    std::lock_guard<std::mutex> lock(g_runtimeDiagMutex);
    return g_runtimeDiag;
}

namespace internal {

void SetPostProcessRuntimeActive(const std::string& name, const std::string& flavor,
                                 size_t passCount, const std::string& resolvedPath) {
    std::lock_guard<std::mutex> lock(g_runtimeDiagMutex);
    g_runtimeDiag.active       = true;
    g_runtimeDiag.name         = name;
    g_runtimeDiag.flavor       = flavor;
    g_runtimeDiag.passCount    = passCount;
    g_runtimeDiag.resolvedPath = resolvedPath;
    g_runtimeDiag.lastError.clear();
}

void SetPostProcessRuntimeInactive() {
    std::lock_guard<std::mutex> lock(g_runtimeDiagMutex);
    g_runtimeDiag.active = false;
    g_runtimeDiag.name.clear();
    g_runtimeDiag.flavor.clear();
    g_runtimeDiag.passCount = 0;
    g_runtimeDiag.resolvedPath.clear();
    // Preserve lastError so the menu can still show why the previous
    // load failed even after we tore the chain down.
}

void SetPostProcessRuntimeError(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_runtimeDiagMutex);
    g_runtimeDiag.lastError = msg;
}

void SetPostProcessRuntimeResolvedPath(const std::string& path) {
    std::lock_guard<std::mutex> lock(g_runtimeDiagMutex);
    g_runtimeDiag.resolvedPath = path;
}

} // namespace internal

} // namespace Fast
