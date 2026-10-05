# 2026-10-05 Stage commit accounting and wall-sweep readiness

Two exact trims, each behind a same-ROM A/B word, on `build-gate-1005j`.

1. `ndsStageGxCommitFast` (src/nds/nds_stage_gx.exec.inc): the per-run
   texture accounting (ready/bind counts, last format, the frame summary's
   bind count) is summed once after the run walk, and the per-segment class
   accounting adds only the at most two classes the segment's runs hold
   (precomputed with the segment) instead of read-modify-writing all eight.
   The official-gate profile put the commit's accounting and run loop at
   ~19K of its ~72K cycles a frame. Word `gNdsStageGxFastLean`.
2. `ndsMPWallSweepStaticMiss` (src/port/reloc_backend_mp_collision.c): the
   geometry checks (kind groups, topology, vertex info, yakumono table,
   ground data and the geometry's arrays) are proved once per geometry and
   heap generation instead of on each of ~100 calls a frame; topology resets
   and rebuilds clear the proof. Word `gNdsMPWallMissReadyCache`.

## Results (official gate ROM, 1,960 frames, replay digest identical in all)

| run | words | P50 | P95 | over 2 VB | paired vs j0 |
|---|---|---|---|---|---|
| gate-j0 | both 0 | 820,224 | 1,133,056 | 108 | -- |
| gate-js | lean 1, ready 0 | 818,880 | 1,131,904 | 106 | -1,472 |
| gate-jw | lean 0, ready 1 | 818,368 | 1,131,136 | 106 | -1,856 |
| gate-j1 | both 1 (shipped) | 817,024 | 1,130,560 | 105 | -3,392 |
