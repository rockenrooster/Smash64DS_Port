# 2026-10-05 Entry ramp palettes: rewrites deferred to the swap VBlank

The entry-effect ramp palettes (`ndsRendererEntryRampPalette`,
`src/nds/nds_renderer_native_common.c`: ENV->PRIM blended through a texture's
grey steps, Link's Spin Attack nine oranges, the glows, the Poke Ball rays)
re-baked a miss with `glColorTableEXT` on a live name mid-frame -- the same
free/allocate and bank F/G remap as the KO pillar variants before
`8485fc0749e`. A gdb trace of the gate match (`__wrap_glColorTableEXT` call
sites) found 11 such uploads, ten of them in one frame (Link's spin start).

Now each of the 12 entries allocates a 32-entry palette once; a miss takes the
least recently used entry that no draw of the frame being built references,
bakes into the entry's staging words and queues it; the copy lands at the
VBlank that swaps the frame in (`ndsRendererCommitDeferredPalettes`, which
also commits the KO pillar variants). A scene reset drops queued rewrites of
palettes it retired. Same-ROM A/B word `gNdsEntryRampDeferred` (0 = the
immediate round robin).

Gate, `build-gate-1005o`:

| run | WORK P50 | WORK P95 | two-VBlank presents |
|---|---:|---:|---:|
| gate-o0 (immediate) | 806,080 | 1,112,896 | 1,860 / 1,961 |
| gate-o1 (deferred, default) | 806,400 | 1,118,336 | 1,857 / 1,961 |

Replay digest identical; paired median +0. The P95 and present counts differ
by single-run spread (same ROM twice reads ~5K apart at P95): the change is a
correctness one on this match, with no frame-level cost or saving outside
noise.
