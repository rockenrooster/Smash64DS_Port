**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
These bugs should be fixed for P1 delivery:

- **FIXED — Fox beam/flash sat low; the +84 bore correction was compensating a stale pose, and is now 0.**
  Owner, 2026-08-15, on `builds/build-c198-bore0/smash64ds-battle-playable-proof-hwtri.nds`
  (SHA-256 `95d75cf6d69a949ceed7a95124c6543b54b4a02f882e963e8e0bafbe6d5ec997`), offset 0:
  *"fox beam is perfect!"*
  The 2026-08-14 owner-confirmed *"perfect"* at bore **84** is **superseded**: it was tuned against a
  gun-joint pose that the shipped figatree parser left a whole frame stale (`64c41c361a7` repaired
  the segment-phase regression that caused it). `NDS_FOX_BLASTER_BORE_OFFSET_Y` is now **0** and
  build-overridable. Beam draw, muzzle/impact glow and the weapon attack collision read that one
  constant, so the visual and the hitbox moved together in both directions and never desynced.
  Owner, 2026-08-15, when asked whether the acceptance covered gameplay as well as the visual:
  *"i said it was perfect, that includes the mario crouching avoiding the beam"*
  So crouch-avoids-beam is **accepted at bore 0 on the repaired tree**, by play, and is not open.
  `FOX_BORE_COLLISION_V5.md`'s contrary "1.181 units of overlap" is marked **stale** in that
  artifact: both of its terms are runtime poses captured 2026-08-12, inside the segment-phase
  defect window, and it also measured one sphere from two different edges (details there).

