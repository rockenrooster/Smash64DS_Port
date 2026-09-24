# Phase 2 stage GX bring-up, 2026-09-24

ROM: `D4AEFF8E9F0E7143BAD024BE3D4B6C500B88CB3708A65808B3BEA21B97C63F77`.
Four CPUs on Dream Land, same ROM, `gNdsP2StageProg=0/1` set at boot.
Repo melonDS slot 9, interpreter from boot, software renderer.
Captured through `scripts/capture-melonds.ps1 -ExactTimeRemain 3300`.
The harness reports actual tics 3300 / 3298 for both arms.

Inspected both first captures; the full game surface is pixel-identical in
both pairs (120,000 pixels per pair). The actual 400x300 top-screen surface in
these 416x664 captures starts at window (8,53). Use
`compare-capture-pair.ps1 -IncludeWindowChrome -CropX 8 -CropY 53 -CropW 400 -CropH 300`.
The helper's default y=56 includes the first two differing rows of the FPS text
below the game: 18 pixels, bbox (67,298)-(186,300) in that default crop. Locating
that text explains the initial mismatch; no game pixels were excluded to pass.

This proves the initial 45-triangle batch at these two live-camera states.
Other camera states, remaining segments, other VS stages and full-match
qualification are still owed. Performance receipt:
`artifacts/performance/2026-09-24_p2-2p8-phase2-stage/README.md`.

175-triangle follow-up: `layers-side-by-side.png` and `layers-pixel-diff.png`
show the pre-fix comparison (771 changed pixels). The precision repair's image
is `layers-precision-t3300.png`, diff `layers-precision-diff.png`: 35 changed
pixels, with maximum projected-corner error 0.0042 DS pixels. Owner response
2026-09-24: "looks good". Approval covers the shown Dream Land view only.

Full-stage follow-up: the first `full-stage-*` pair captured desktop wallpaper
and is INVALID. The exact-frame helper was repaired to capture HWND content
with PrintWindow. `full-stage-r1-*` and `source-depth-control-*` are inspected,
method-matched captures at tics 3300/3298. The old reference had incorrectly
routed Dream Land's no-Z cross-binding flowers through source-Z emission;
the repaired reference and compiled path differ at only 2 pixels at tic 3300
(0.0017%, max channel delta 16). See the performance receipt for identities.
