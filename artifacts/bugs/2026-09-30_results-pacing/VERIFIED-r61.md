# Results pacing fixes A-D on r61 (2026-09-30)

Candidate: `builds/p2p8-playtest-r61/smash64ds.nds`, SHA-256
`342A4297A744357A08B618AAC106FFBDC3C21F23156413AED38D39C4A0B23582` (63,177,728 B).
Carries report section 5 A-D as implemented (plus, in the same ROM, the
switched-off platform fix and the event32 margin). A: tile-plan memo
(`nds_results_oam.c`, 32 slots keyed on height<<16|width). B: resampling steps
once per baked cell, and the two 4-bit quantizers' `/17` as an exact
multiply-shift (`(x * 3856) >> 16 == x / 17` for every x < 1,000, checked).
C: wallpaper intensity validated/indexed by nibble (`nds_native_wallpaper.c`).
D: one 512-byte FNT sector kept by the ARM9 storage callback
(`nds_audio_storage.c`), dropped on ROM open/close; counters
`gNdsAudioStorageFntPage{Hits,Fills}`.

Same recipe as the r58 runs (driver copied to `builds/results-verify/`, slot 0,
roster 0,8,1,6, one-minute FFA Time on stage 0, breakpoint-free window mode;
`r61-ffa-window2-{start,end}.json`, `r61-ffa-window2.txt`). The first attempt
(`r61-ffa-window1.txt`) stopped on a metric name absent from the shipping ELF.

| Metric | r58 (`r58-ffa-window1`) | r61 (`r61-ffa-window2`) |
|---|---|---|
| Recorder intervals 2-420 (VBlanks x count) | 1x354, 2x58, 3x2, 4x1, 6x2, 10x1, 18x1 | 1x410, 2x3, 3x3, 5x1, 6x1, 17x1 |
| Interval max | 18 | 17 |
| FuncStart ticks | 80,120,704 | 20,740,160 |
| Setup-files ticks (4 calls) | 56,087,680 | 13,016,768 |
| Transition through first update | 85,159,616 (~2.54 s) | 25,780,032 (~0.77 s) |
| Storage requests / waited ticks (whole run) | 33,908 / 3,690,310 | 6,533 / 1,036,579 |
| FNT page hits / fills | - | 27,631 / 1,283 |
| BGM seam misses / error stops / timer drops | not read | 0 / 0 / 0 |
| OAM provenance failures, malloc overflows | 0, 0 | 0, 0 |

Steady state now presents every VBlank (58 two-VBlank frames -> 3). The one
long iteration left is still tic 120 (the four fighters' construction, first
OBJ bakes and first texture preparation); the report's section 5 closing note
names the next step for it (prewarm in the idle tics before 120, or
producer-baked OBJ texels). Owner listening test of the Results audio owed:
the BGM stream reported no seam miss in this run, so the reported stutter is
not a stream underrun here.
