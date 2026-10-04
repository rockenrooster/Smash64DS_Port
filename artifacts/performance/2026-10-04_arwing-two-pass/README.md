# Sector Z's Arwing in two passes (owner r75 row, 2026-10-04)

Owner: "Slow FPS when Arwing hazard is active in 4p battle with all items
(veryhigh) ... goes down to 19FPS."

## Measurement before the change

Lab four-CPU ROM, Sector Z, Ness/Kirby/Fox/Yoshi, every item at very high.

- Tick HUD by Arwing status (szperf2): the MiscActor bucket is 849 ticks/frame
  with no Arwing and 133K with it flying; WORK p50 0.98M -> 1.24M.
- Per-root census (`gNdsLabArwingRootCensus`): seven FoxSpecial3 roots a
  frame -- the body (99 triangles) ~25K ticks, six two-triangle glow quads
  12.7-17.8K each, matrix preparation alone 5-10K of every glow.
- ARM9 profile (`artifacts/task37-census/sz-szprof01`, split on
  `grSectorArwingUpdatePatrol`): +230,852 cycles a frame on Arwing frames,
  led by the entry-effect executor (+53.9K at 10 cycles/instruction), its
  adapter (+23.8K, 7.5), the packet replay (+16K, 17.7) and the matrix chain
  (~55K). Memory stall, not work: every list walked 20-25 KB of code (scan,
  stage DL submit, adapter, matrix helpers, a 15 KB executor) against the
  8 KB instruction cache.

## Change

`ndsRendererAdapterSubmitArwingTwoPass` (src/port/renderer_adapter_stage.c),
called for the Sector Arwing from the ground-actor submit
(src/port/reloc_backend_movement.c): the tree is walked once as the scan walks
it, every root's matrices are prepared in one pass with the same
`ndsRendererAdapterPrepareInitialMatrices` call, and then every list is
submitted in source order through the unchanged path, which takes the prepared
matrices. Same-ROM A/B word `gNdsArwingTwoPass`.

## Result (same lab ROM, both runs to presented frame 820, deterministic)

`ab-szt1-vs-szt0.txt` (A = two passes, B = the scan); all 820 rows matched with
the same Arwing status:

| Arwing flying (325 frames) | A | B | A-B |
|---|---|---|---|
| WORK p50 | 1,149,440 | 1,193,280 | -43,840 |
| WORK p95 | 1,534,016 | 1,576,384 | -42,368 |
| WORK mean | 1,198,711 | 1,250,977 | -52,266 |
| MiscActor mean | 100,710 | 139,941 | -39,231 |

Frames without the Arwing: WORK mean -14,021 (SRC; the next frame meets a
warmer cache).

Output: `capdiff-cap1-vs-cap0.txt` -- the top screen of nine Arwing frames
(presented frames 500-700 every 25, status 2) is pixel-identical between the
two modes; 0 native failures, 205 two-pass draws, 0 declines.
`arwing-close-f00500.png` is one of them, the Arwing passing in front.
