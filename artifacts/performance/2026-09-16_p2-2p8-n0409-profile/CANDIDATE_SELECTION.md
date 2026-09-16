# P2-2p8 N04.09 candidate selection

Read with `census.txt` (symbol ranking) and `softfloat-callers.txt` (leaf
attribution) in this directory. Both were taken on a **clean-payload** build, so
they survive the 2026-09-16 re-baseline.

Ticks are `cycles / (2 * regions)` — the ARM9 runs at 2x the tick timer
(`scripts/analyze-profile-region-split.py:32`). `__aeabi_fadd + __aeabi_fmul` is
**90,169 tk/fr**.

## The float class by caller NAME

The census log prints one row per return address, so a function with two call
sites appears twice. Aggregated by name:

| share | tk/fr | caller | gate |
|---|---|---|---|
| 18.79% | 16,940 | `func_ovl2_800ED490` | UNRESOLVED |
| 10.87% | 9,801 | `gmCollisionSetInvertMatrix` | GAMEPLAY |
| 10.80% | 9,738 | `gmCollisionGetFighterPartsWorldPosition` | GAMEPLAY |
| 9.29% | 8,377 | `gmCollisionGetWorldPosition` | GAMEPLAY |
| 5.61% | 5,058 | `gmCollisionTransformMatrixAll` | GAMEPLAY |
| 5.07% | 4,572 | `ndsStageMPAdjustFloorLoopWallSweep` | RENDERER (**mislabelled**) |
| 4.74% | 4,275 | `guMtxCatF` | RENDERER |

**The collision matrix family is 55.36% of the class, about 49,900 tk/fr — 3.5x
the 14,080-tick floor.** That is the lane, not any single symbol.

`func_ovl2_800ED490` is `ndsR2SimMacBaseCompose`
(`src/import/battleship_gmcollision.c:217`), the decomp's unnamed 4x4 float
matrix compose on the collision path.

## Two corrections to how this ranking is read

1. **The RENDERER/GAMEPLAY gate is a hardcoded name list**, not analysis
   (`scripts/census-softfloat-callers.ps1:90-106`). Its `ndsStage` prefix
   fallback labels `ndsStageMPAdjustFloorLoopWallSweep` RENDERER, while
   `docs/optimization/review/06_*.md` marks it collision-FROZEN. Re-gate any
   caller before treating its label as permission.
2. An earlier board cursor called `guMtxCatF` the top caller. That was the
   double-row artifact above; it is 7th.

## Rejected here

- **`guMtxCatF`.** One live site only
  (`renderer_adapter_matrix.c:3980`, the kind-48 arm, 4.961 calls/frame); both
  camera sites are dead behind the shipped fixed camera
  (`NDS_R2_CAMERA_MATRIX_LEAN`, `NDS_R2_CAMERA_FIXED`), and the lbparticle site
  is cold. Deleting the function entirely is <=5,282 tk/fr, **0.38x the floor**.
  A Q20.12 concat is also the wrong mechanism: its look-at and perspective
  producers are float, so it would pay ~32 edge conversions per call against
  ~1,065 ticks removed.
- **`func_ovl2_800ED490` as the first slice.** It is the largest single caller,
  but `docs/optimization/review/12_COMPLETE_SIMULATION_MATH_CHAINS_TO_FIXED_POINT.md`
  Phase 1 rules it out for a first slice on two grounds, both of which still
  hold: the renderer consumes `parts->mtx_translate` from that chain, so a Q
  conversion "moves **drawn geometry** as well as hit results" and needs Tier
  A/B proof on both consumers; and "the board records the fixed-point route
  there as already declined once". The campaign says to prefer chains without a
  cross-subsystem consumer first.

## The governing economics, from Campaign 12

An f32 to Q edge conversion costs **31-42 cycles** against `__aeabi_fmul` at
26.5, so the exchange rate is set by **conversions per deleted operation**, a
property of the candidate's signature:

- **1.00 conv/op cannot pay**
- **0.57 breaks even**
- the 5.14x prior had **conv/op = 0**

Compute conv/op from the signature *before* building. Closed chains beat wrapped
leaves because conv/op falls as the chain lengthens. This is why the lane is
worth 49,900 tk/fr but a single wrapped leaf inside it may be worth nothing.

## Next action

Campaign 12 Phase 0/1 against the current baseline: re-census the simulation
float reservoir on the clean build masked to the gate's own rank-80 frames, then
pick a **closed chain** — velocity update, gravity/terminal velocity, position
integration, or a knockback subchain — with conv/op below 0.57 and no
cross-subsystem consumer. Infrastructure already exists: `NDS_R2_SIM_MAC_SHADOW`
(`battleship_gmcollision.c:213-258`) gives a same-binary shadow arm with
`gNdsR2SimMacShadowArm` and a call counter, which is Campaign 12's Phase 4.

Do not start from the biggest symbol. Start from the chain with the best
conv/op that no other subsystem reads.
