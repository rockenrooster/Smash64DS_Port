# Tracker resets and stage tallies ahead of the GX DMA start: measured, no effect, 2026-10-04

The gate profile (`task37-census/gate-prof01`) showed high CPI on the CPU-side
work right after a GXFIFO DMA start: the stage segment commit's tally block
(CPI 13-44) and the lean submit's ndsRendererForgetLibndsTexture (~100 cycles
an instruction). Hypothesis: main-RAM stores waiting behind the DMA's bursts.
Both blocks (CPU-side words only, no FIFO write between) moved ahead of the
DMA start in ndsStageGxCommitFast/ndsStageGxFlush and ndsFtrLeanPacketSubmit
(build `build-gate-1004d`).

| arm | WORK P50 | WORK P95 | > 1.12M |
|---|---:|---:|---:|
| control (`../2026-10-04_owner-validate-pool/gate-v1`) | 845,952 | 1,178,752 | 151 |
| reordered (`gate-d1`) | 846,016 | 1,183,552 | 153 |

Paired by frame: median -64, mean -151 (noise). Digest identical. The cost
those lines carried moved with the next main-RAM access rather than leaving:
the wait is the DMA's, not the instructions'. Reverted.
