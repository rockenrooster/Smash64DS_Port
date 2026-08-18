Things to Optimize:
Fixed point for everything that was float.
DS Native Formats for all textures.

Mario Fireball (Textures, Animations, Geometry, Offload CPU)
Fireball Firegrind (Textures, Animations, Geometry, Offload CPU)
Impact Wave (Textures, Animations, Geometry, Offload CPU)
Revival Platform (Textures, Animations, Geometry, Offload CPU)
Mario Up B (Textures, Animations, Geometry, Offload CPU)
Fox Laser (Textures, Animations, Geometry, Offload CPU)
Fox Down B (Textures, Animations, Geometry, Offload CPU)
Whispy particle effects (Textures, Animations, Geometry, Offload CPU)

Collision Logic (Textures, Animations, Geometry, Offload CPU)
Animations (30hz Animations, Offload CPU)


Mario Fighter (Textures, 30hz Animations, Offload CPU)
Fox Fighter (Textures, 30hz Animations, Offload CPU)

Stage (Textures, Animations, Geometry, Offload CPU)



**3. The billboard observation — owner, 2026-08-06. Read this before designing
any effect work.** Owner reports that on N64 *every* VFX except the platform is
a camera-facing billboard, at the fighter's own Z depth, always drawn on top,
with alpha on some parts. If that holds, effects need no per-effect 3D geometry
and no depth reasoning at all — which is the same conclusion cycles 88-91
reached from the opposite direction when the G3 packet path was refuted
(effect geometry is per-instance, and static per-layer world Z could not
reproduce the painter order). A billboard model sidesteps both: order is
submission order, and "on top" is a draw-last property rather than a Z one.
This is a design input, not a filed bug, and it is the owner's observation of
the N64 — confirm against BattleShip before building on it.
