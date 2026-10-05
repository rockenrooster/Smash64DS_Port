# 2026-10-05 KO pillar palettes: rewrites deferred to the swap VBlank

The KO blast pillar draws ENVCOLOR particles through palette variants baked per
(sheet, prim, env) (`ndsRendererParticleEnvVariant`,
`src/nds/nds_renderer_textures_effects.c`). A gdb trace of the gate's KO burst
(frames 1,160-1,210, every ENVCOLOR quad's key) found up to 13 distinct keys a
frame -- particles of different ages sit at different points of the prim ramp
-- and 44 over the burst, so the eight-entry round robin rebaked 124 times in
19 frames. Each rebake was a `glColorTableEXT` on a live name: libnds frees the
palette block, allocates a new one and maps banks F/G to LCD mid-frame. 357 of
the match's 394 mid-match texture/palette uploads were these rebakes.

Now 16 entries each allocate their palette once. A miss takes the least
recently used entry that no draw of the frame being built references, bakes
into that entry's staging copy and queues it; `ndsRendererParticleEnvVariantCommit`
copies the queued palettes in at the first VBlank after the flush (the swap),
when the replaced frame has stopped rendering and the new one has not started.
The trace replayed against this policy: 56 bakes over the burst, no frame runs
out of entries. 1,024 bytes of palette RAM (was 512).

Same-ROM A/B, `build-gate-1005n`, word `gNdsParticleEnvVariantDeferred`
(0 = the old eight-entry immediate round robin):

| run | WORK P50 | WORK P95 | two-VBlank presents |
|---|---:|---:|---:|
| gate-n0 (immediate) | 807,936 | 1,125,184 | 1,852 / 1,961 |
| gate-n1 (deferred, default) | 807,680 | 1,118,528 | 1,857 / 1,961 |

Paired (`pairab.py`, frames >= 60): replay digest identical on all frames,
P95 -7.9K, over-budget frames 101 -> 97. KO burst frames 1,181-1,196 drop
47K-146K each (1,189: 1,393,536 -> 1,247,872).

Captures at frames 1,180/1,186/1,190 in both modes (local only,
`artifacts/visibility/2026-10-05_envdefer`, not committed): the pillar region
is pixel-identical; the few differing pixels are small sparkles and Link's
outline in the middle of the screen, where the UI-lag caveat applies.
