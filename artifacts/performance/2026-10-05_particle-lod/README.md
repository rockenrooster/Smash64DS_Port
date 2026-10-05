# 2026-10-05 Heavy-frame particle LOD (owner ruling D12b)

Render only: a presented frame whose elapsed work has reached
`gNdsParticleHeavyLodTicks` (default 1,000,000) when its first particle pass
starts draws half of its generic particle quads (odd pool slots, so a
particle keeps its visibility across consecutive heavy frames). The skipped
quad's transform still runs, so the draw-time LBTransform effects (affine,
Ready -> Finished) are a drawn frame's. Score digits, Whispy's native quads,
the Fox glow and FireGrind are untouched. `gNdsBattleIterationStartTick`
(`src/port/taskman_seam_battle_host.c`) stamps the battle iteration's start.
The change as measured: `particle-lod-v2.patch`.

**Morning (official gate, same ROM, first cut):** `gate-lod0` -> `gate-lod1`,
P95 1,143,744 -> 1,142,528 (top 5% mean -8.2K, P95-boundary frames -1.9K).
Parked then: the gate's P95 frames carry few particles.

**Afternoon (clean lab ROM `build-lab-clean1005q`, same ROM, word 0 ->
1,000,000):**

| Stage | P95 | over 1.12M | LOD frames | median per LOD frame |
|---|---|---|---|---|
| Sector Z (`L0-g1` -> `L1-g1`) | 1,279,936 -> 1,271,872 | 215 -> 213 | 253 | -4.4K |
| Jungle (`L0-g2` -> `L1-g2`) | 1,272,960 -> 1,265,728 | 247 -> 243 | -- | -- |
| Yoshi's Island (`c1-g5` -> `L1-g5`) | 1,259,392 -> 1,252,416 | 272 -> 266 | -- | -- |
| Dream Land (`L0-g6` -> `L1-g6`) | 1,131,392 -> 1,129,664 | 105 -> 104 | 96 | -4.7K |

Replay digest identical everywhere. Calibration (SHDT carried the elapsed
work at the first particle pass in the lab runs): the first pass comes P50
77-99K ticks before the frame's end; 1.0M marks nearly every frame ending
above 1.12M and no frame ending under 1.0M.

Kept: the stages furthest from the gate gain 7-8K of P95 for a particle
density halved in their heaviest ~13% of frames.
