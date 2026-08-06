// Implemented from the runtime-diagnostics contract documented in
// libultraship/include/fast/postprocess/PostProcessSourceLoader.h
// (PostProcessRuntimeDiagnostics + Fast::internal::Set* setters). No
// code copied from RetroArch or any GPL-licensed shader runtime.

#include <gtest/gtest.h>

#include "fast/postprocess/PostProcessSourceLoader.h"

namespace {

// Reset the global diagnostics struct between tests so cases don't
// see leftover state from a sibling test that ran first. There's no
// public Reset; calling SetInactive then SetError("") gives us the
// closest thing — active=false, name/flavor/path cleared, lastError
// cleared.
void ResetDiagnostics() {
    Fast::internal::SetPostProcessRuntimeInactive();
    Fast::internal::SetPostProcessRuntimeError("");
}

} // namespace

// SetActive populates every field; the Get snapshot must mirror it.
TEST(PostProcessRuntimeDiagnostics, SetActivePopulatesSnapshot) {
    ResetDiagnostics();
    Fast::internal::SetPostProcessRuntimeActive(
        "crt-lottes", "legacy", 1u, "fs:/tmp/shaders/crt-lottes.glsl");

    const auto snap = Fast::GetPostProcessRuntimeDiagnostics();
    EXPECT_TRUE(snap.active);
    EXPECT_EQ(snap.name, "crt-lottes");
    EXPECT_EQ(snap.flavor, "legacy");
    EXPECT_EQ(snap.passCount, 1u);
    EXPECT_EQ(snap.resolvedPath, "fs:/tmp/shaders/crt-lottes.glsl");
    EXPECT_TRUE(snap.lastError.empty()) << "SetActive must clear stale errors";
}

// Multi-pass and slang-flavor values surface verbatim — the menu
// renders these as-is.
TEST(PostProcessRuntimeDiagnostics, SetActiveAcceptsSlangMultipass) {
    ResetDiagnostics();
    Fast::internal::SetPostProcessRuntimeActive(
        "crt-royale", "slang", 8u, "archive:shaders/postprocess/crt-royale.slangp");

    const auto snap = Fast::GetPostProcessRuntimeDiagnostics();
    EXPECT_TRUE(snap.active);
    EXPECT_EQ(snap.flavor, "slang");
    EXPECT_EQ(snap.passCount, 8u);
    EXPECT_EQ(snap.resolvedPath,
              "archive:shaders/postprocess/crt-royale.slangp");
}

// SetInactive clears active+name+flavor+passCount+resolvedPath but
// MUST preserve lastError so the menu can keep showing why the last
// load failed even after the chain tore itself down.
TEST(PostProcessRuntimeDiagnostics, SetInactivePreservesLastError) {
    ResetDiagnostics();
    Fast::internal::SetPostProcessRuntimeActive(
        "broken-shader", "legacy", 1u, "fs:/tmp/shaders/broken-shader.glsl");
    Fast::internal::SetPostProcessRuntimeError(
        "transpile failed: ERROR: 0:42: 'sinangle' : undeclared identifier");
    Fast::internal::SetPostProcessRuntimeInactive();

    const auto snap = Fast::GetPostProcessRuntimeDiagnostics();
    EXPECT_FALSE(snap.active);
    EXPECT_TRUE(snap.name.empty());
    EXPECT_TRUE(snap.flavor.empty());
    EXPECT_EQ(snap.passCount, 0u);
    EXPECT_TRUE(snap.resolvedPath.empty());
    EXPECT_NE(snap.lastError.find("undeclared identifier"), std::string::npos)
        << "SetInactive must NOT clear lastError — the menu shows the "
           "previous failure until the user picks a new shader.";
}

// SetError without a SetActive surfaces the message anyway — used
// when the loader rejects a shader before the chain has been
// constructed (e.g. the .glsl file simply doesn't exist).
TEST(PostProcessRuntimeDiagnostics, SetErrorBeforeActiveSurfaces) {
    ResetDiagnostics();
    Fast::internal::SetPostProcessRuntimeError("file not found: shaders/missing.glsl");

    const auto snap = Fast::GetPostProcessRuntimeDiagnostics();
    EXPECT_FALSE(snap.active);
    EXPECT_EQ(snap.lastError, "file not found: shaders/missing.glsl");
}

// SetActive after a SetError clears the error — the new load
// succeeded, so the previous failure is no longer informative.
TEST(PostProcessRuntimeDiagnostics, SetActiveClearsPriorError) {
    ResetDiagnostics();
    Fast::internal::SetPostProcessRuntimeError("transpile failed: stale");
    Fast::internal::SetPostProcessRuntimeActive(
        "scanlines", "legacy", 1u, "archive:shaders/postprocess/scanlines.glsl");

    const auto snap = Fast::GetPostProcessRuntimeDiagnostics();
    EXPECT_TRUE(snap.active);
    EXPECT_TRUE(snap.lastError.empty());
}

// SetResolvedPath updates only the resolved path — useful when the
// chain resolves the path lazily after publishing the active state.
TEST(PostProcessRuntimeDiagnostics, SetResolvedPathOverwritesPathOnly) {
    ResetDiagnostics();
    Fast::internal::SetPostProcessRuntimeActive(
        "crt-lottes", "legacy", 1u, "fs:/tmp/initial.glsl");
    Fast::internal::SetPostProcessRuntimeResolvedPath(
        "fs:/Users/me/.local/share/BattleShip/shaders/crt-lottes.glsl");

    const auto snap = Fast::GetPostProcessRuntimeDiagnostics();
    EXPECT_TRUE(snap.active);
    EXPECT_EQ(snap.name, "crt-lottes");
    EXPECT_EQ(snap.passCount, 1u);
    EXPECT_EQ(snap.resolvedPath,
              "fs:/Users/me/.local/share/BattleShip/shaders/crt-lottes.glsl");
}

// Concurrent SetError + Get under repeated calls must not blow up
// (mutex correctness) and must not corrupt the lastError string. We
// don't run a thread here to keep the test deterministic — the
// contract is that Get returns a copy, not a reference, so the menu
// thread's snapshot can't be torn by a writer.
TEST(PostProcessRuntimeDiagnostics, GetReturnsAnIndependentSnapshot) {
    ResetDiagnostics();
    Fast::internal::SetPostProcessRuntimeError("first");
    auto snap = Fast::GetPostProcessRuntimeDiagnostics();
    EXPECT_EQ(snap.lastError, "first");

    Fast::internal::SetPostProcessRuntimeError("second");
    // The previously-captured snap is independent.
    EXPECT_EQ(snap.lastError, "first");

    auto snap2 = Fast::GetPostProcessRuntimeDiagnostics();
    EXPECT_EQ(snap2.lastError, "second");
}
