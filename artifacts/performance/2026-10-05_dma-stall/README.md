# 2026-10-05 GXFIFO DMA cost on the official gate

Experiment only (`build-gate-dmaexp`, source reverted after the build): a lab
word `gNdsLabSkipGxDma` skipped GXFIFO DMA starts by source -- bit 1 the stage
segments (`ndsStageGxFlush`), bit 2 the lean fighter lists
(`ndsFtrLeanPacketSubmit`), bit 4 every other site. The CPU work is unchanged
(the frames draw without that geometry); the replay digest is identical in
every arm. Same ROM, 1,960 frames each:

| run | skipped | P50 | P95 | over | paired vs on |
|---|---|---|---|---|---|
| dma-on / d2-on | none | 815,808 / 815,744 | 1,126,464 / 1,126,848 | 104 | -- |
| dma-skip | all | 789,888 | 1,103,040 | 86 | -25,408 |
| d2-stage | stage | 804,160 | 1,115,904 | 92 | -10,368 |
| d2-fighter | lean lists | 799,488 | 1,114,048 | 93 | -14,784 |
| d2-other | others | 814,848 | 1,126,336 | 103 | -256 |

SRC is unchanged in every arm: the cost is render-side bus stalls behind the
DMA's 112-word bursts (plus the spin before the next list starts), so it
scales with the words a list sends. Follow-up: lean lists with VTX_10 corners
(`../2026-10-05_lean-vtx10`).
