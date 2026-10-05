# Owner texture memo fills for the dedicated A5I3 name, 2026-10-05

`src/nds/nds_renderer_textures_effects.c`: Yoshi's Island's three clouds (and
the rebirth beam) bind the dedicated PRIM-RGB/TEXEL0-alpha name, which is not
a texture-cache entry, so the owner memo never filled and every cloud bind ran
the full resolver (~5K cycles, three a frame). The memo now fills for that
name with the words its bind applied, valid while the name's generation holds
(every prepare and release bumps it). Same GX state. Same-ROM A/B word
`gNdsRendererOwnerTexMemoDedicated`.

Lab sweep ROM (`build-lab-sweepall9`), Yoshi's Island, same ROM, word 0
(`yoshi-dm0`) -> 1 (`yoshi-dm1`): P50 1,037,632 -> 1,026,560, P95 1,381,568
-> 1,370,176, over 587 -> 549, paired median -10,944, replay digest identical.

Also measured the same day and not kept: particle LOD in heavy frames (owner
ruling D12b; `../2026-10-05_particle-lod`, official gate same ROM): P95
1,143,744 -> 1,142,528 (top 5% mean -8.2K, the P95-boundary frames -1.9K;
lower half +0.4K). The frames at P95 carry few particles; not worth a visual
approximation. Patch kept in the session scratchpad.
