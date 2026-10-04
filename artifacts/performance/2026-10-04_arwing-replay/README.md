# Sector Z's Arwing: the executor's replay branch alone (owner r75 row, 2026-10-04)

Owner: "Slow FPS when Arwing hazard is active in 4p battle with all items
(veryhigh) ... goes down to 19FPS." Follows `../2026-10-04_arwing-two-pass`.

## Measurement before the change

Lab profile of the owner's match after the two-pass change
(`artifacts/task37-census/sz-szprof02`, split on `grSectorArwingUpdatePatrol`):
the Arwing frames' premium is 201,256 cycles a frame. Its largest rows are the
15 KB entry-effect executor (+42.8K at 7.9 cycles an instruction), the adapter
(+17.3K), memset (+15K: a stats and config clear per root), the stage DL
submit (+11.8K) and the packet replay (+11K, mostly the DMA wait). Every root is
already a recorded packet replayed under the state cache, so the executor runs
only its replay branch, but reaches it through every other owner's code.

## Change

`ndsRendererReplayNativeEntryEffectFox` (src/nds/nds_renderer_native_common.c)
is that branch alone for owner 161 with no materials: the static-proven checks
that read live state, the state match, the modelview store, the composed
matrix where an override needs it, the split matrix load, the ramp palettes and
the packet DMA, in the executor's order and with its stores. It returns FALSE
whenever the executor would take another branch. The Arwing's second pass
(`ndsRendererAdapterSubmitArwingRootReplay`, src/port/renderer_adapter_stage.c)
seeds the stats and config exactly as `ndsRendererAdapterTryNativeEntryEffect`
does (shared inline helpers) and falls back to the ordinary submit on FALSE.
Same-ROM A/B word `gNdsEntryEffectFoxReplay`.

## Result (same lab ROM, both runs to presented frame 820)

`ab-fr1-vs-fr0.txt`; the battle runs two logic updates per presented frame, so
the rows line up; all 820 have the same Arwing status.

| Arwing flying (325 frames) | A | B | A-B |
|---|---|---|---|
| WORK p50 | 1,126,208 | 1,169,984 | -43,776 |
| WORK p95 | 1,520,960 | 1,552,384 | -31,424 |
| WORK mean | 1,201,525 | 1,216,769 | -15,244 |
| MiscActor mean | 75,077 | 115,737 | -40,660 |
| CAM mean | 72,705 | 59,796 | +12,908 |

The CAM bucket absorbs part of the saving: the Arwing's body packet (99 lit
triangles) is still in the geometry engine when the next FIFO writer arrives,
and the replay leaves it less CPU work to hide behind.

Replay counters: 1,981 replays, 5 declines (the first frames, while packets
record), 0 native failures.

## Output equivalence

Window captures of the two arms cannot be paired by presented frame on this
emulator (the window lags the core by a variable number of frames: arm A's
"logic 1740" capture is pixel-identical to arm B's "logic 1720"). The lab build
instead folds what every FoxSpecial3 packet replay hands the GX -- the root,
both matrices, every packet word and the light/othermode stats the replay reads
(`ndsLabFoxGxHash`, lab-only) -- into one hash per frame, at the executor's
branch and at the replay submit alike. `gx-hash-fh1-vs-fh0.txt`: logic frames
1000-1900, 411 frames with Arwing replays, 411 identical, 0 native failures in
either arm.
