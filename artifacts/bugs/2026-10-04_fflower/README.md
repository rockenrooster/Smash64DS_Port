# Fire Flower head (owner row "not rendering correctly", 2026-10-04)

The item is two roots of file 86: the stem (0x4520, branching to 0x4578;
three leaves) and the head (0x4608: a 178x130 quad on a kind-46 billboard,
a 16x16 CI4 texture mirrored to 32x32, two palettes the material animation
swaps every frame). On the DS a resting flower showed its leaves and a narrow
white shape (`before-resting-7x.png`).

A lab witness at each root's emit (`gNdsLabFFlowerState`, cache-flushed,
`src/nds/nds_native_item_fflower.exec.inc`) showed the head placed and sized
correctly (11x8 DS pixels above the stem) with the right texture state --
CI4 tile, the stem's combiner, the live palette, a 32-entry TLUT -- but a
geometry mode of 0x00220405 (lighting, smooth shading, back-face culling),
where the stem drew with 0x00000005.

Cause: the head list sets no geometry mode before its triangles. On the N64 it
inherits the stem's, drawn just before in the same DObj tree, whose root word
8 (`D9DDFBFF 00000000`) clears lighting, smooth shading and culling; the
head's word 16 restores them only after its triangles. The DS executors re-seed
the display's initial mode at every root (`ndsRendererInitTraversalState`), so
the head drew lit and culled.

Fix: the head's bake applies the stem's word 8 again before its triangles
(`scripts/stages/generate_nds_native_item_wave1_core.py`, `_fflower`) -- the
state the RSP holds there, since the head is the stem's child and always draws
right after it. After: `after-resting-8x.png` (lab ROM, Peach's Castle, a
landed flower: white, orange and red rings over the leaves; head geometry mode
at emit 0x00000005).
