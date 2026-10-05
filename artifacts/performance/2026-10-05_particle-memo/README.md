# Particle draw lookups memoized, 2026-10-05

Render only, exact (same words to the GX):

- `src/import/battleship_lbparticle.c` `ndsParticleTransformForDraw`: a
  transform's two axis magnitudes (two `sqrtf` and nine float products a
  particle) are kept per transform for the draw pass (keyed on a 32-bit pass
  counter: `dLBParticleCurrentTransformID` is a u8). A/B word
  `gNdsParticleXfScaleCache`.
- `src/nds/nds_renderer_textures_effects.c`: one-entry memos of the ENV
  palette variant lookup and the packet binding lookup (a burst shares one
  sheet, prim and env), invalidated by a generation every writer of the sheet,
  variant and binding tables bumps; and the view-space axis conversions kept
  on their source bits. A/B word `gNdsParticleLookupMemo`.

Official gate, same ROM (`build-gate-1005d`, both words 0 `gate-pm0` -> 1
`gate-pm1`): P50 831,360 -> 828,928, P95 1,143,360 -> 1,142,528, paired median
-640; by frame-weight band the paired median is -448 (lower half), -960
(80-90%), -1,088 (top 5%, mean -2,787). Replay digest identical.
