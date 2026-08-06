// Implemented from the public libretro `.glslp` and `.slangp` preset
// format documentation at https://docs.libretro.com/development/shaders/
// and the example presets in libretro/glsl-shaders / libretro/slang-
// shaders. No code copied from RetroArch or any GPL-licensed shader
// runtime.

#include "fast/postprocess/PostProcessPreset.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

namespace Fast {

namespace {

std::string Trim(const std::string& s) {
    const size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) {
        return std::string();
    }
    const size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::string Unquote(const std::string& s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

bool ParseBool(const std::string& v, bool fallback) {
    std::string t;
    t.reserve(v.size());
    for (char c : v) {
        t.push_back(std::tolower(static_cast<unsigned char>(c)));
    }
    if (t == "true" || t == "1" || t == "yes" || t == "on") {
        return true;
    }
    if (t == "false" || t == "0" || t == "no" || t == "off") {
        return false;
    }
    return fallback;
}

bool ParseScaleType(const std::string& v, PostProcessScaleType& out) {
    if (v == "source") {
        out = PostProcessScaleType::Source;
        return true;
    }
    if (v == "viewport") {
        out = PostProcessScaleType::Viewport;
        return true;
    }
    if (v == "absolute") {
        out = PostProcessScaleType::Absolute;
        return true;
    }
    return false;
}

// Lower-cased libretro tokens. Unrecognized values leave `out` untouched
// so the caller's pre-populated default sticks.
bool ParseWrapMode(const std::string& v, PostProcessWrapMode& out) {
    if (v == "clamp_to_edge" || v == "clamp" || v == "edge") {
        out = PostProcessWrapMode::ClampToEdge;
        return true;
    }
    if (v == "clamp_to_border" || v == "border") {
        out = PostProcessWrapMode::ClampToBorder;
        return true;
    }
    if (v == "repeat" || v == "wrap") {
        out = PostProcessWrapMode::Repeat;
        return true;
    }
    if (v == "mirrored_repeat" || v == "mirror") {
        out = PostProcessWrapMode::MirroredRepeat;
        return true;
    }
    return false;
}

// Split "shader42" -> ("shader", 42). Returns false if the key has no
// numeric suffix (= it's a global key like "shaders" or "textures").
bool SplitIndexed(const std::string& key, std::string& baseOut, int& indexOut) {
    if (key.empty() || !std::isdigit(static_cast<unsigned char>(key.back()))) {
        return false;
    }
    size_t splitPos = key.size();
    while (splitPos > 0 && std::isdigit(static_cast<unsigned char>(key[splitPos - 1]))) {
        splitPos -= 1;
    }
    if (splitPos == 0) {
        return false;
    }
    baseOut = key.substr(0, splitPos);
    indexOut = std::atoi(key.c_str() + splitPos);
    return true;
}

void EnsureCapacity(std::vector<PostProcessPresetPass>& passes, int index) {
    if (index < 0) {
        return;
    }
    while (static_cast<int>(passes.size()) <= index) {
        passes.emplace_back();
    }
}

} // namespace

namespace {

// Find or create an external-texture entry by name. Returns reference
// into the vector — invalidated by subsequent emplace, so callers
// must finish using it before the next call.
PostProcessPresetTexture& UpsertTexture(std::vector<PostProcessPresetTexture>& tex,
                                        const std::string& name) {
    for (auto& t : tex) {
        if (t.name == name) {
            return t;
        }
    }
    tex.emplace_back();
    tex.back().name = name;
    return tex.back();
}

// Lower-case copy. Used for the `wrap_mode` / `<name>_wrap_mode`
// values whose tokens are case-insensitive in practice.
std::string ToLower(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        out.push_back(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

// Return true iff `v` parses cleanly as a decimal number (libretro's
// `.slangp` parameter overrides are always numeric — integer or float,
// optionally signed). Used to filter parameter-override capture from
// truly malformed entries, so a junk key=value line doesn't poison the
// parameterOverrides map and silently override a real parameter later.
bool TryParseFloat(const std::string& v, float& out) {
    if (v.empty()) {
        return false;
    }
    const char* begin = v.c_str();
    char* end = nullptr;
    const float parsed = std::strtof(begin, &end);
    if (end == begin) {
        return false;
    }
    // strtof stops at the first non-numeric char; we want strict
    // numeric. Tolerate trailing whitespace only (already trimmed
    // by caller, but be defensive).
    while (end && *end != '\0') {
        if (!std::isspace(static_cast<unsigned char>(*end))) {
            return false;
        }
        ++end;
    }
    out = parsed;
    return true;
}

bool ParsePresetImpl(const std::string& src, const std::string& baseDir,
                     PostProcessPresetFlavor flavor,
                     PostProcessPreset& out, std::string& errOut) {
    const char* const formatTag =
        (flavor == PostProcessPresetFlavor::Slangp) ? ".slangp" : ".glslp";
    out.flavor = flavor;
    out.baseDir = baseDir;
    out.passes.clear();
    out.textures.clear();
    out.parameterOverrides.clear();

    int declaredShaders = -1;

    std::istringstream in(src);
    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        lineNo += 1;
        // Strip everything from the first `#` onward — libretro presets
        // use `#` for end-of-line comments. Quoted strings don't contain
        // `#` in practice, so we don't worry about it inside quotes.
        const size_t hash = line.find('#');
        if (hash != std::string::npos) {
            line.erase(hash);
        }
        line = Trim(line);
        if (line.empty()) {
            continue;
        }
        const size_t eq = line.find('=');
        if (eq == std::string::npos) {
            errOut = std::string(formatTag) + " line " + std::to_string(lineNo) +
                     ": missing '=' (\"" + line + "\")";
            return false;
        }
        const std::string key = Trim(line.substr(0, eq));
        const std::string val = Unquote(Trim(line.substr(eq + 1)));

        if (key == "shaders") {
            declaredShaders = std::atoi(val.c_str());
            if (declaredShaders < 0) {
                declaredShaders = 0;
            }
            EnsureCapacity(out.passes, declaredShaders - 1);
            continue;
        }
        if (key == "textures") {
            // Semicolon-separated list of external-texture identifiers.
            // Each name `N` later gets its own `N`, `N_linear`,
            // `N_wrap_mode`, `N_mipmap` keys; we provision an empty
            // entry per name here so those subsequent keys land on a
            // stable record.
            size_t start = 0;
            while (start <= val.size()) {
                size_t end = val.find(';', start);
                if (end == std::string::npos) {
                    end = val.size();
                }
                std::string n = Trim(val.substr(start, end - start));
                if (!n.empty()) {
                    UpsertTexture(out.textures, n);
                }
                if (end == val.size()) {
                    break;
                }
                start = end + 1;
            }
            continue;
        }

        // External-texture keys come in two shapes: bare `<name>` for
        // the file path, and `<name>_<suffix>` for the per-texture
        // attributes. Resolve by trying the longest suffix match first.
        {
            bool handledAsTexture = false;
            for (auto& tex : out.textures) {
                const std::string& n = tex.name;
                if (key == n) {
                    tex.path = val;
                    handledAsTexture = true;
                    break;
                }
                if (key == n + "_linear") {
                    tex.filterLinear = ParseBool(val, tex.filterLinear);
                    handledAsTexture = true;
                    break;
                }
                if (key == n + "_wrap_mode") {
                    ParseWrapMode(ToLower(val), tex.wrapMode);
                    handledAsTexture = true;
                    break;
                }
                if (key == n + "_mipmap") {
                    tex.mipmap = ParseBool(val, tex.mipmap);
                    handledAsTexture = true;
                    break;
                }
            }
            if (handledAsTexture) {
                continue;
            }
        }

        std::string base;
        int index = -1;
        if (!SplitIndexed(key, base, index)) {
            // Unknown global key. For .glslp this is the historical
            // "silently drop unsupported keys" path (used by libretro
            // presets that carry preset-format metadata we don't need).
            // For .slangp the same shape encodes parameter overrides
            // for `#pragma parameter` declarations in the referenced
            // .slang sources — capture every numeric-valued entry so
            // the chain can apply it once the .slang files are parsed.
            // Non-numeric unknowns (e.g. `parameters = "FOO;BAR"`) stay
            // dropped under both flavors.
            if (flavor == PostProcessPresetFlavor::Slangp) {
                float numeric = 0.0f;
                if (TryParseFloat(val, numeric)) {
                    out.parameterOverrides[key] = numeric;
                }
            }
            continue;
        }
        EnsureCapacity(out.passes, index);
        PostProcessPresetPass& pass = out.passes[index];

        if (base == "shader") {
            pass.shaderPath = val;
        } else if (base == "filter_linear") {
            pass.filterLinear = ParseBool(val, pass.filterLinear);
        } else if (base == "wrap_mode") {
            std::string lower;
            lower.reserve(val.size());
            for (char c : val) {
                lower.push_back(std::tolower(static_cast<unsigned char>(c)));
            }
            ParseWrapMode(lower, pass.wrapMode);
        } else if (base == "srgb_framebuffer") {
            pass.srgbFramebuffer = ParseBool(val, pass.srgbFramebuffer);
        } else if (base == "float_framebuffer") {
            pass.floatFramebuffer = ParseBool(val, pass.floatFramebuffer);
        } else if (base == "mipmap_input") {
            pass.mipmapInput = ParseBool(val, pass.mipmapInput);
        } else if (base == "frame_count_mod") {
            pass.frameCountMod = std::atoi(val.c_str());
        } else if (base == "alias") {
            pass.alias = val;
        } else if (base == "scale_type") {
            // Sets both axes. Libretro spec: applies after any
            // axis-specific overrides earlier in the file, so do it
            // first and let scale_type_{x,y} override later.
            PostProcessScaleType st;
            if (ParseScaleType(val, st)) {
                pass.scaleTypeX = st;
                pass.scaleTypeY = st;
            }
        } else if (base == "scale_type_x") {
            ParseScaleType(val, pass.scaleTypeX);
        } else if (base == "scale_type_y") {
            ParseScaleType(val, pass.scaleTypeY);
        } else if (base == "scale") {
            const float s = static_cast<float>(std::atof(val.c_str()));
            pass.scaleX = s;
            pass.scaleY = s;
        } else if (base == "scale_x") {
            pass.scaleX = static_cast<float>(std::atof(val.c_str()));
        } else if (base == "scale_y") {
            pass.scaleY = static_cast<float>(std::atof(val.c_str()));
        }
        // Anything else (`parameters`, preset overrides, etc.) silently
        // ignored — adding them later doesn't reshape this function.
    }

    if (out.passes.empty()) {
        errOut = std::string(formatTag) + " declared no passes";
        return false;
    }

    // Trim trailing default-initialized passes that weren't assigned a
    // shader path (some presets declare shaders=N but only fill 0..M-1
    // and we'd otherwise carry phantom passes).
    while (!out.passes.empty() && out.passes.back().shaderPath.empty()) {
        out.passes.pop_back();
    }
    if (out.passes.empty()) {
        errOut = std::string(formatTag) + " had no pass with a shader path";
        return false;
    }
    for (size_t i = 0; i < out.passes.size(); ++i) {
        if (out.passes[i].shaderPath.empty()) {
            errOut = std::string(formatTag) + " pass " + std::to_string(i) +
                     " missing 'shader" + std::to_string(i) + "' entry";
            return false;
        }
    }
    return true;
}

} // namespace

bool ParsePostProcessPreset(const std::string& src, const std::string& baseDir,
                            PostProcessPreset& out, std::string& errOut) {
    return ParsePresetImpl(src, baseDir, PostProcessPresetFlavor::Glslp, out, errOut);
}

bool ParseSlangPreset(const std::string& src, const std::string& baseDir,
                      PostProcessPreset& out, std::string& errOut) {
    return ParsePresetImpl(src, baseDir, PostProcessPresetFlavor::Slangp, out, errOut);
}

} // namespace Fast
