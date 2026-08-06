#pragma once

#include "stdint.h"
#include "fast/ucodehandlers.h"

#ifdef __cplusplus
extern "C" {
#endif

void GfxSetNativeDimensions(uint32_t width, uint32_t height);
void GfxGetPixelDepthPrepare(float x, float y);
uint16_t GfxGetPixelDepth(float x, float y);

// SSB64 port hook: when set non-zero, Interpreter::AdjXForAspectRatio
// compresses post-projection clip-space X by (4/3) / window_aspect to
// expand the visible 4:3 frustum into the wider window. Default 0 ⇒
// AdjXForAspectRatio is a no-op (4:3 content stretches to fill window).
void GfxSetWidescreenActive(int active);

// SSB64 port: returns the same scale factor AdjXForAspectRatio applies to
// clip-space X. Port glue uses this to compress game-side world-to-screen
// projection results so HUD sprites attached to 3D characters track them on
// screen after the camera frustum widens. Returns 1.0f when widescreen is
// off / window is not wider than 4:3.
float GfxGetWidescreenClipXScale(void);

// SSB64 port hook: when set non-zero AND widescreen is active, the GPU
// scissor is tightened to the original 4:3 sub-region of the wider FB.
// Used to crop scene-specific effects whose mesh geometry exposes
// perspective-foreshortened slants in widescreen (e.g. the OpeningRun
// impact-flash starburst). Default 0 ⇒ no scissor narrowing.
void GfxSetTight4_3ScissorWindow(int active);

#ifdef __cplusplus
}
#endif
