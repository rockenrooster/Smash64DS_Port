# Scene transitions: the hold, verified on r65b (2026-09-30)

Owner, after r64 (the r63 black cover): "instead of just black screens for
every transition, we just keep what currently on the screen until the next
screen is ready, then switch". After r65a: "much better".

## Mechanism

Nothing on the DS display is a framebuffer: BG0 re-renders the last swapped
GX list from texture VRAM every frame, and the 2D layers and OBJ scan their
VRAM live. So the old frame stays on screen exactly as long as nothing writes
those, and the hold is a discipline on writers, not a copy (display capture
needs a free LCDC A-D bank; none is free across these scenes).

- The hold starts after the leaving frame's present (menu shell, source
  menus) or at the battle's scene exit, where r63 committed black.
- EXIT phase, while the old scene tears down: its display teardown (BG0 off,
  overlay layer mask, title fire, backdrop, OBJ shadow uploads) is recorded,
  not performed.
- LOAD phase, from `ndsPlatformBeginSceneTransition`: the next scene's first
  display write -- a 2D layer, an OBJ tenant, bank D under a visible BG3, a
  texture or palette upload under a visible BG0 (`--wrap=glTexImage2D`,
  `glColorTableEXT`, `glColorSubTableEXT`, and the scene texture reset) --
  calls the Thaw: the r63 cover plus the recorded teardown. The black now
  lasts only while that scene builds its first frame; its first complete
  present releases it, as before.
- EndFrame presents nothing during a hold (no flush, OBJ, fade, wallpaper or
  bank D commit of a half-built scene); 600 presents end it regardless.
- Results' OBJ tenant enters at the Thaw; the UI kit's OAM clear on exit is
  uploaded there.

## r65a: the Results frame lost its text and tint

r65a `results-css` 0030 (`r65a-hold01-frames.json`): the held Results frame
had no stats, no dark tint and no "YOSHI WINS!". It was the leaving frame
itself: a list flushed at EndFrame entry vb N shows in the capture at vb N+2,
with that frame's OAM commit. The source never draws that frame
(`taskman.c:994` breaks right after the update that calls
`syTaskmanSetLoadScene`); the port's source-menu pump drew it, after Results'
exit block had moved `scene_curr` to the character select, and the SObj
backend keys the Results OBJ tenant on `scene_curr`
(`sprite_preview_backend.c:814`). r62 showed it for one scanout; r63's cover
hid it. Under the hold the pump now breaks before the draw, as the source does,
and holds the frame drawn before it.

## r65b result

Same driver and recipe as r63 (`builds/codex-transitions/run-transitions.ps1`,
Mario/Kirby/Fox/Yoshi, one-minute Time, Castle; every scanout at its VBlank).
ROM `p2p8-playtest-r65b` `AE31B994...`, run `r65b-hold01-*`. Classified
against the previous scanout (SAME = pixel-identical, both screens):

| Handoff | Old frame held | Black | Then |
|---|---|---|---|
| CSS -> SSS | 42 (0000-0041) | 12 (0042-0053) | complete SSS from 0054 |
| SSS -> battle | 156 (0000-0155) | 24 (0156-0179) | the source's 12-tic lbFade from 0180 |
| battle -> Results | 47 (0096-0142; the sub screen's FPS line changes at 0140) | 85 (0143-0227) | Results wallpaper fade from 0228 |
| Results -> CSS | 15 (0030-0044), text and tint intact | 19 (0045-0063) | complete CSS from 0064 |

battle -> Results: the 85 black scanouts are Results' own start, black
through tic 80 in the port since r63 (the source shows a photo of the last
battle frame there, a separate content item); the hold covers the load before
it. `r65b-hold-handoffs.png`: per row, the first and last held scanout, the
first black one and the next scene's first.

## Gate

Gate ROM (default Dream Land roster, items on), two runs each, each pair
identical run to run (`artifacts/performance/2026-09-26_p2-2p8-ftr-item-tail/`
`g8_a/b`, `g9_c/d`):

| Build | P50 | P95 | P99 | two-VBlank | heap low-water |
|---|---:|---:|---:|---:|---:|
| `build-p2p8-b8` (control, `8bfa1993872`) | 917,824 | 1,235,520 | 1,501,696 | 1,755 | 317,356 |
| `build-p2p8-b9` (the hold) | 922,240 | 1,240,256 | 1,504,320 | 1,745 | 315,308 |

Replay digest identical over 1,972 samples; 0 native failures. The gate run
makes no transition (`gNdsTransitionHoldCount` 0); per battle frame the hold
adds one Thaw call that returns, one test in EndFrame and one in the tint-tile
service, so the +4.4K/+4.7K is the image's growth moving placement (the size
drift seen before), not work. (`g9_a/b` were run with the lean route poked
off by mistake and are not comparable.)

Per-scanout PNG directories (`r65a-hold01/`, `r65b-hold01/`) stay local; the
frames JSON and event logs here index them.
