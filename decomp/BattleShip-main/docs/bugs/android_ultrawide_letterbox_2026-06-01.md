# Android Ultrawide Letterbox

**Issue:** [#209](https://github.com/JRickey/BattleShip/issues/209)
**Status:** Fixed 2026-06-01

## Symptoms

On ultrawide Android devices (reported as 20:9), the game could launch with a
black bar and then relaunch into an apparent 4:3 or otherwise incorrect aspect.

## Root Cause

The widescreen renderer path was already capable of arbitrary wider-than-4:3
aspects: `Interpreter::GetWidescreenClipXScale()` reads the live
`mGameWindowViewport` and applies `(4/3) / window_aspect` to 3D clip-space X.
The failure was Android presentation policy around that renderer path:

- The manifest did not explicitly opt both activities into resizeable,
  wide-aspect presentation, so Android could compatibility-letterbox on
  tall/wide devices.
- libultraship fullscreen defaulted to exclusive `SDL_WINDOW_FULLSCREEN` unless
  `gSdlWindowedFullscreen` was set. On Android that is not useful; it can ask
  SDL/Android for a fixed fullscreen mode rather than preserving the native
  surface dimensions the OS gives us.

## Fix

- Manifest now declares `android:resizeableActivity="true"` and
  `android:maxAspectRatio="2.4"` for the app, `BootActivity`, and
  `BattleShipActivity` (20:9 is approximately 2.22).
- Android boot now forces `gSdlWindowedFullscreen=1`, which makes LUS use
  `SDL_WINDOW_FULLSCREEN_DESKTOP` and keep the OS/native surface aspect.

No fixed 20:9 math was added. The existing LUS dynamic aspect transform remains
the source of truth for 16:9, 20:9, and similar ultrawide displays.

## Audit Hook

If ultrawide regresses, first log `mGameWindowViewport`, `mCurDimensions`, and
the active fullscreen flag on Android. If `mGameWindowViewport` is already
letterboxed, suspect Android activity/window policy. If it is full 20:9 but the
game image is 4:3, inspect `gAdvancedResolution.Enabled`, `gLowResMode`, and
`gSdlWindowedFullscreen`.
