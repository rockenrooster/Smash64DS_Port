# Scene transitions: verified on r63 (2026-09-30)

The investigation (REPORT.md, r58 captures) proposed a loading cover at the
shared scene exit. It was integrated as proposed and captured on r62; that run
showed three remaining exposures, fixed for r63. Same driver and recipe as the
r58 sequence (`builds/codex-transitions/run-transitions.ps1`, roster
Mario/Kirby/Fox/Yoshi, one-minute Time, Castle): every scanout captured at its
VBlank, registers read at `ndsPlatformVBlankInterrupt`.

| ROM | SHA-256 | run |
|---|---|---|
| r62 (cover as proposed) | `8D016172…` | `r62-sequence01-*` |
| r63 (+ the three fixes below) | `C01350D6…` | `r63-sequence01-*` |

## What r62 still showed

1. **Results -> CSS, two partial scanouts** (r62 `results-css` 0029/0030): the
   text vanished, then the wallpaper, before the black. `ndsMNVSResultsSetLoadScene`
   retired the Results OBJ tenant at the START press (an immediate `oamUpdate`
   mid-frame), and the source-menu harness hid BG2/BG3 after its loop, both
   before `ndsPlatformBeginSceneTransition` committed the cover.
2. **CSS -> SSS, one partial scanout** (r62 `css-sss` 0038): the CSS exit hides
   its previews' BG0 before the cover.
3. **Results tic 80, the wallpaper full-bright for two scanouts** (r62
   `battle-results` 0225 partial rows, 0226 whole; `r62-results-tic80-wallpaper-flash.png`),
   then black and the source fade. Also in the r58 baseline (0328-0333). The
   source makes the wallpaper and its black Tint2 in the same tic
   (`mnvsresults.c:3235-3242`); the port wrote the converted wallpaper into the
   visible BG2 bitmap row by row, while the tint (an OBJ plane) reaches the
   screen only at the frame's OAM commit.

## The r63 fixes

- The frame whose update asks for the next scene commits the cover instead of
  releasing it: the source-menu harness (`sSYTaskmanStatus == LoadScene`) and
  the menu shell (`sMenuLeaving`). Everything after that frame is under black.
- Results' OBJ tenant retires in `ndsPlatformBeginSceneTransition`, under the
  cover, not at the START press.
- A converted wallpaper upload hides its BG layer while it writes
  (`ndsPlatformHideOriginalSpriteOverlayUntilCommit`); the next EndFrame shows
  it with that frame's OBJ and brightness commits.

## r63 result

| Handoff | r63 scanouts | Evidence |
|---|---|---|
| CSS -> SSS | complete CSS through 0036, black from 0037 (vb 525 cover, scene switch at 526), complete SSS from 0054 | `r63-css-sss-handoff.png` |
| SSS -> battle | complete SSS, black from 0006 until the source's 12-tic lbFade (0178-0188) | `r63-sss-battle-handoff.png` |
| battle -> Results | Time Up hold, the source's interface hide (HUD-less stage 0095-0097), black from 0098; Results black through tic 80 (the photo is a separate content item), wallpaper fades in from black at 0228, no flash | `r63-battle-results-handoff.png`, `r63-results-tic80-wallpaper.png` |
| Results -> CSS | complete Results through 0029, black from 0030, complete CSS | `r63-results-css-handoff.png` |

Per-scanout luminance of the Results wallpaper window (0..255, top screen):
r62 0224 0.0, 0225 48.4, 0226 93.0, 0227 0.0, 0228 6.9; r63 0224..0227 0.0,
0228 6.9 (the source fade's first step), unchanged after.

Gameplay is untouched: the gate ROM carrying the cover (`build-p2p8-b6`)
replays digest-identical to the control over 1,972 samples; P50/P95 915,776 /
1,233,408 against 916,096 / 1,233,856. The Results FFA pacing run on r62
(`../2026-09-30_results-pacing/r62-ffa-window1-end.json`): steady two-VBlank
frames 3 of 419, BGM seam misses 0; the transition adds two VBlank waits
(26.9M ticks vs 25.8M on r61).

Per-scanout PNG directories (`r58-*`, `r62-sequence01/`, `r63-sequence01/`,
~160 MB) stay local; the frames JSON, event logs and contact sheets here
index them.
