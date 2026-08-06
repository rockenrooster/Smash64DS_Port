// Implemented from the public libretro slang shader format docs at
// https://github.com/libretro/slang-shaders/blob/master/README.md. No
// code copied from RetroArch or any GPL-licensed shader runtime.

#include "fast/postprocess/PostProcessSlangSource.h"

#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace Fast {

namespace {

bool IsHSpace(char c) {
    return c == ' ' || c == '\t';
}

// Strip leading and trailing ASCII whitespace (incl. CR/LF) in place.
std::string TrimEdges(const std::string& s) {
    const size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) {
        return std::string();
    }
    const size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// Walk `s` from `pos` skipping horizontal whitespace.
void SkipHSpace(const std::string& s, size_t& pos) {
    while (pos < s.size() && IsHSpace(s[pos])) {
        ++pos;
    }
}

// Read the next whitespace-delimited token starting at `pos`. If the
// first non-whitespace char is `"`, reads through the matching close
// quote (returning the contents without the quotes); otherwise reads
// until the next whitespace. Returns true if a token was read.
// On return `pos` points one past the token's last consumed character.
bool ReadToken(const std::string& s, size_t& pos, std::string& tokOut) {
    SkipHSpace(s, pos);
    if (pos >= s.size()) {
        return false;
    }
    if (s[pos] == '"') {
        const size_t start = pos + 1;
        size_t end = start;
        while (end < s.size() && s[end] != '"') {
            ++end;
        }
        tokOut = s.substr(start, end - start);
        pos = (end < s.size()) ? end + 1 : end;
        return true;
    }
    const size_t start = pos;
    while (pos < s.size() && !IsHSpace(s[pos])) {
        ++pos;
    }
    tokOut = s.substr(start, pos - start);
    return !tokOut.empty();
}

// Recognize "#pragma <name>"; returns the trailing argument substring
// (everything after the pragma name, trimmed) when matched. On no
// match returns false. `lineTrimmed` must have leading whitespace
// already stripped.
bool MatchPragma(const std::string& lineTrimmed, const std::string& name,
                 std::string& argsOut) {
    constexpr const char* kPrefix = "#pragma";
    constexpr size_t kPrefixLen = 7;
    if (lineTrimmed.size() < kPrefixLen) {
        return false;
    }
    if (lineTrimmed.compare(0, kPrefixLen, kPrefix) != 0) {
        return false;
    }
    size_t pos = kPrefixLen;
    SkipHSpace(lineTrimmed, pos);
    if (lineTrimmed.size() < pos + name.size()) {
        return false;
    }
    if (lineTrimmed.compare(pos, name.size(), name) != 0) {
        return false;
    }
    // The matched pragma name must be a whole word — `#pragma names`
    // mustn't match `#pragma name`.
    const size_t afterName = pos + name.size();
    if (afterName < lineTrimmed.size() && !IsHSpace(lineTrimmed[afterName])) {
        return false;
    }
    size_t argsPos = afterName;
    SkipHSpace(lineTrimmed, argsPos);
    argsOut = lineTrimmed.substr(argsPos);
    argsOut = TrimEdges(argsOut);
    return true;
}

// Parse a `#pragma parameter` body (everything after the word
// "parameter"). Returns true on success.
bool ParseParameterBody(const std::string& args,
                        PostProcessSlangParameter& outParam,
                        std::string& errOut) {
    size_t pos = 0;
    std::string tok;

    if (!ReadToken(args, pos, tok) || tok.empty()) {
        errOut = "#pragma parameter: missing name";
        return false;
    }
    outParam.name = tok;

    if (!ReadToken(args, pos, tok)) {
        errOut = "#pragma parameter " + outParam.name + ": missing label";
        return false;
    }
    outParam.label = tok;

    auto readFloat = [&](const char* fieldName, float& outF) -> bool {
        std::string s;
        if (!ReadToken(args, pos, s) || s.empty()) {
            errOut = "#pragma parameter " + outParam.name + ": missing " + fieldName;
            return false;
        }
        const char* begin = s.c_str();
        char* end = nullptr;
        outF = std::strtof(begin, &end);
        if (end == begin) {
            errOut = "#pragma parameter " + outParam.name + ": " + fieldName +
                     " not numeric (\"" + s + "\")";
            return false;
        }
        return true;
    };

    if (!readFloat("default", outParam.defaultValue)) {
        return false;
    }
    if (!readFloat("min", outParam.minValue)) {
        return false;
    }
    if (!readFloat("max", outParam.maxValue)) {
        return false;
    }
    // Step is optional. ReadToken returns false on EOL; that's fine.
    std::string stepTok;
    if (ReadToken(args, pos, stepTok) && !stepTok.empty()) {
        const char* begin = stepTok.c_str();
        char* end = nullptr;
        const float f = std::strtof(begin, &end);
        if (end == begin) {
            errOut = "#pragma parameter " + outParam.name +
                     ": step not numeric (\"" + stepTok + "\")";
            return false;
        }
        outParam.step = f;
    }
    return true;
}

enum class Stage { Common, Vertex, Fragment };

} // namespace

bool ParseSlangSource(const std::string& src, PostProcessSlangSource& out,
                      std::string& errOut) {
    out = PostProcessSlangSource{};

    bool sawVertexStage = false;
    bool sawFragmentStage = false;

    std::string commonBuf;
    std::string vertexBuf;
    std::string fragmentBuf;

    Stage stage = Stage::Common;

    std::istringstream in(src);
    std::string rawLine;
    int lineNo = 0;
    while (std::getline(in, rawLine)) {
        ++lineNo;
        std::string trimmed = TrimEdges(rawLine);

        // Slang-specific pragmas are stripped from output. Everything
        // else (including non-stage pragmas glslang understands like
        // `#pragma optimize(on)`) flows through unchanged so glslang
        // sees byte-identical text to what the author wrote.
        std::string pragmaArgs;
        if (MatchPragma(trimmed, "stage", pragmaArgs)) {
            std::string s = pragmaArgs;
            // Normalize: trim trailing tokens after the stage name.
            size_t p = 0;
            std::string stageTok;
            ReadToken(s, p, stageTok);
            if (stageTok == "vertex") {
                stage = Stage::Vertex;
                sawVertexStage = true;
            } else if (stageTok == "fragment") {
                stage = Stage::Fragment;
                sawFragmentStage = true;
            } else {
                errOut = std::string(".slang line ") + std::to_string(lineNo) +
                         ": unknown #pragma stage \"" + stageTok + "\"";
                return false;
            }
            continue;
        }
        if (MatchPragma(trimmed, "name", pragmaArgs)) {
            out.name = pragmaArgs;
            continue;
        }
        if (MatchPragma(trimmed, "format", pragmaArgs)) {
            out.format = pragmaArgs;
            continue;
        }
        if (MatchPragma(trimmed, "parameter", pragmaArgs)) {
            PostProcessSlangParameter param;
            std::string err;
            if (!ParseParameterBody(pragmaArgs, param, err)) {
                errOut = std::string(".slang line ") + std::to_string(lineNo) +
                         ": " + err;
                return false;
            }
            out.parameters.push_back(std::move(param));
            continue;
        }

        // Non-pragma line: route to the current stage buffer plus
        // newline. Use the original (untrimmed) line so authors keep
        // their indentation in the glslang error positions.
        const std::string lineWithNl = rawLine + "\n";
        switch (stage) {
            case Stage::Common:
                commonBuf += lineWithNl;
                break;
            case Stage::Vertex:
                vertexBuf += lineWithNl;
                break;
            case Stage::Fragment:
                fragmentBuf += lineWithNl;
                break;
        }
    }

    if (!sawVertexStage) {
        errOut = ".slang missing `#pragma stage vertex`";
        return false;
    }
    if (!sawFragmentStage) {
        errOut = ".slang missing `#pragma stage fragment`";
        return false;
    }

    // Common preamble lands in BOTH stage buffers, in source order.
    out.vertex = commonBuf + vertexBuf;
    out.fragment = commonBuf + fragmentBuf;
    return true;
}

} // namespace Fast
