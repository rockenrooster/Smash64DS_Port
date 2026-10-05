# 2026-10-05 Stage templates v8: baked worlds as MTX_MULT_4x3

The GXFIFO DMA cost follows the words a list sends (`../2026-10-05_dma-stall`:
stage segments -10.4K a frame on the gate when skipped). Every baked stage
world is affine (column 3 = 0, 0, 0, 1.0), so `scripts/stages/compile_nds_stage_gx.py`
now emits it as MTX_MULT_4x3 (12 parameter words) instead of MTX_MULT_4x4 (16);
the runtime validator (`ndsStageGxValidate`) checks the 12 words against the
world it builds and that world's column 3. Template version 8.

Words a frame's full program sends (all nine VS stages):

| stage | v7 | v8 |
|---|---|---|
| Castle | 3,609 | 3,489 |
| Sector Z | 5,746 | 5,702 |
| Congo Jungle | 5,418 | 5,230 |
| Zebes | 4,598 | 4,462 |
| Hyrule | 5,494 | 5,334 |
| Yoshi's Island | 4,850 | 4,662 |
| Dream Land | 4,890 | 4,782 |
| Saffron | 6,005 | 6,001 |
| Mushroom Kingdom | 5,298 | 5,106 |

Gate (`build-gate-1005m`, cross-ROM against `build-gate-1005k` gate-k1):
813,568/1,125,568, 100 over, replay digest identical; paired -0.4K (within
cross-ROM spread -- the expected Dream Land saving is ~0.2K). Same-frame
captures at frame 300 are pixel-identical (the later frames' captures caught
different presented frames, the UI-lag caveat). The transform is the same by
construction.
