# 2026-10-05 Lean fighter lists: VTX_10 corners

## Why

A same-ROM experiment (`build-gate-dmaexp`, a lab word skipping GXFIFO DMA
starts; receipt `../2026-10-05_dma-stall`) priced the GX list DMAs on the
official gate: skipping all of them was paired -25.4K ticks a frame (P95
-23.4K), split stage segments -10.4K, the four lean fighter lists -14.8K,
everything else -0.3K. The cost is CPU bus stalls behind the DMA's 112-word
bursts (and the spin before the next list), so it scales with list words.

A gdb dump of the four live lean lists at frame 1,200 (8,197 words) put VTX_16
at 33.7% of the words, NORMAL 16.8%, TEXCOORD 13.2%, command headers 15%,
LOAD4x3 8.2%. Every corner coordinate is a whole source unit at 4.12 (unit << 4;
largest |unit| 292), so VTX_10 (4.6) holds it exactly in one parameter word.

## Change

Route-1 lists are materialized with VTX_10 corners (`ndsFtrLeanMatCorners`)
when every corner fits; otherwise the list is walked again at VTX_16
(`nNDSFtrLeanDeclineVtx10` -> `NDS_FTR_LEAN_ENTRY_VTX16` in
`ndsFtrLeanMaterializeFor`). A VTX_10 corner's value is 4x its VTX_16 value,
so the list's per-root LOAD4x3 translation rows (`ndsFtrLeanKernelCompose`,
`NDS_FTR_LEAN_SITES_VTX10`) and P' row 3 (`ndsFtrLeanPacketPatch`) carry the
4x as well: the clip coordinates scale uniformly by 4, which the perspective
divide, the homogeneous clip and the z/w depth buffer cancel; the vector
matrix (normals, lighting) is unscaled. Same-ROM A/B word `gNdsFtrLeanVtx10`.

## Results (official gate ROM `build-gate-1005k`, 1,960 frames)

| run | word | P50 | P95 | over 2 VB |
|---|---|---|---|---|
| gate-k0 | VTX_16 | 816,192 | 1,130,496 | 104 |
| gate-k1 | VTX_10 (shipped) | 814,656 | 1,127,552 | 104 |

Paired median -2,560 over 1,902 frames; replay digest identical.

Captures (same ROM, frames 300/700/1,200/1,600, kept local under
`artifacts/visibility/2026-10-05_vtx10`, not committed): 12-325 DS top-screen
pixels differ per frame, all isolated fighter edge pixels (the 4x scale's
rounding of the translation rows); no visible change.
