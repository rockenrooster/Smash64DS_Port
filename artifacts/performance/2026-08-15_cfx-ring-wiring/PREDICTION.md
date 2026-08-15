# Slice 52 wiring — the prediction and the flip budget, written BEFORE the build

Date: 2026-08-15. Branch `codex/r2-runtime2`. HEAD at prediction time `223fb499ca5`.

This file exists so the verdict cannot be written after the fact. Everything
below was fixed before `build-c174-cfxring-a` was compiled.

## 1. What is wired

Three PRODUCERS of the fighter narrow phase move to fixed point behind an f32
boundary. The CONSUMER does not move at all.

| decomp body | after |
|---|---|
| `func_ovl2_800EDBA4` (chain walk) | fixed interior, f32 written per joint |
| `func_ovl2_800EDE00` (inverse) | Q26 cofactor, f32 into `unk_dobjtrans_0x9C` |
| `func_ovl2_800EDE5C` (axis scales) | integer roots, f32 into `vec_scale` |
| `gmCollisionTransformMatrixAll` | reached only through the chain, replaced there |
| `func_ovl2_800ED490` (compose) | reached only through the chain, replaced there |
| `gmCollisionTestRectangle` | **UNCHANGED decomp float** |
| `gmCollisionTestSphere` | **UNCHANGED decomp float** |
| `gmCollisionGetWorldPosition` | **UNCHANGED decomp float** |

**This is a deliberate departure from the briefed ring**, and it is stated as
one. The brief and `STACK.md` §5 assume the 64 bytes of `unk_dobjtrans_0x9C` are
reinterpreted as a fixed `NDSR2CfxFrame`, which is what obliges
`gmCollisionTestSphere` to be converted (§5.1: it writes a shield hit's
knockback ANGLE and normal, continuous values no flip count can express). Keeping
the slot f32 removes that obligation entirely: `TestSphere` is not touched, every
one of the nine readers of `unk_dobjtrans_0x9C` is untouched, and **every
collision decision is still taken by decomp code, by decomp comparisons, on f32
values in the fields the source declared.** The price is that
`gmCollisionTestRectangle` (6,568 tk/fr) and `gmCollisionGetWorldPosition`
(7,021) are left on the table; they are named and sized for a later cycle.

## 2. Predicted tick removal at rank-80 — and it is BELOW the requirement

Basis: `…/2026-08-15_k1-owner-pricing/OWNER_PRICING.md` §5, ticks/frame on the 80
frames that set P95, `build-c172-profile-shipcand`.

```text
DELETED soft float / sqrtf
  func_ovl2_800ED490                                     11,808
  gmCollisionTransformMatrixAll                           4,318
  lbCommonSin + lbCommonCos under the local build       ~ 4,975
  gmCollisionSetInvertMatrix                              6,120
  func_ovl2_800EDE5C (float 1,505 + sqrtf 2,931)          4,436
                                                        -------
                                                         31,657

REPLACED self time (10,774 of it), integer for soft float
  assume 40-60% cheaper                             +4,300 … 6,500

PAID BACK: the f32 boundary, ~690 conversions/frame
  15.3 StoreF32 unk_dobjtrans_0x10 + 15.3 StoreF32 mtx_translate
  + 7.0 LoadF32 at the chain boundary + 6.7 x (12 in + 12 out) invert
  + 6.5 x 12 in scales, at 8-15 ticks per conversion   -5,500 … -10,400

NOT TAKEN, by the design choice above
  gmCollisionTestRectangle                                6,568
  gmCollisionGetWorldPosition                             7,021
  gmCollisionGetFighterPartsWorldPosition                 4,593
```

**Net predicted at rank-80: +25,000 … +32,000, i.e. 0.77x-0.98x the +32,593 net
requirement.** This is a prediction and it is short. The brief's +30,000…+38,000
assumed `gmCollisionTestRectangle` came with the package; under this design it
does not.

**Which frames it comes off**: the ones with a live hitbox. Every replaced body
is 5.2x-11.7x more present on the P95 set than whole-match, so the cut lands on
the frames that set the percentile rather than uniformly.

## 3. The flip budget — stated before the run

**Budget: ZERO decision flips.**

The change perturbs no comparison. It perturbs the last bits of four f32 fields,
and `scripts/check-r2-collision-fixed.ps1` bounds that perturbation end to end,
over the LIVE joint-scale domain **0.9937-2.0479** (widened this cycle from the
stale 1.1138-1.1199), with `unk_dobjtrans_0x10` round-tripped through f32 on
every level, which is the worst case rather than the common one:

```text
T8 live, depth  6, reach +/-64   world 0.0017395  local 0.0010529  vec_scale 0.0001224
T8 live, depth 12, reach +/-64   world 0.0022583  local 0.0014343  vec_scale 0.0001225
bound                                   0.0200           0.0200            0.0200
T6 TestRectangle decisions       200,000 + 100,000 cases, 0 mismatches, 0 declined
                                 smallest margin 0.0002151 world units
```

A flip is therefore *possible* — 132 of 100,000 cases sit inside the 0.0200 bound
and the smallest margin is below the perturbation — but none was produced in
300,000 randomised cases at the live domain.

**Acceptance, and any inequality is a STOP, not noise:**

```text
gNdsCfxFighterDamagePhaseCalls    equal on A and B   (same fight)
gNdsCfxFighterDamagePhaseHits     equal on A and B   (same decisions)
gNdsCfxFighterShieldPhaseCalls    equal on A and B
gNdsCfxFighterShieldPhaseHits     equal on A and B
end-of-match invariant pair       equal on A and B
```

`route-ab-cannot-price-gameplay-change`: a tick delta with a moved hit count is a
changed fight, not a saving.

## 4. Predicted engagement

```text
gNdsCfxRingPrepareCalls   ~= DamagePhaseCalls + ShieldPhaseCalls  (~1,938 whole match)
gNdsCfxRingChainFixed     >  gNdsCfxRingChainDeclined     <- else the mechanism is inert
gNdsCfxRingInvertFixed    ~  one per distinct joint per hitbox-live frame
gNdsCfxRingScaleFixed     ~  gNdsCfxRingInvertFixed
every gNdsCfxRing* counter = 0 on arm A                   <- the negative control
```

If `ChainDeclined` dominates `ChainFixed`, the measured delta is placement and
not arithmetic, and the correct report is that the mechanism did not engage.

## 5. The instrument, and why it is not the usual flag falsifier

The cross-build P95 floor on this ROM is ~17,000 and the measured one-line
candidate spread is +/-24,064. The usual "flag on, dispatch reverted" arm does
not work here: with nothing calling the ring, `--gc-sections` removes the whole
cluster and the "candidate layout" arm is the control layout again.

`NDS_R2_COLLISION_FIXED_DISPATCH` instead flips one initialised word of `.data`,
read through a `volatile` the compiler cannot fold. Both arms compile and link
the same code. **`.text` is therefore expected to be byte-identical between arm A
and arm B**, which is checked rather than assumed — and if it holds, the
placement floor for this comparison is zero rather than +/-17,000.

Arms: A = DISPATCH 0, B = DISPATCH 1, A' = a second run of A's ROM.
Gate arm otherwise: `NDS_R2_BOTH_CPU=1`, DLDI on, `NDS_TICK_HUD_DRAW` default
(1), mode 163 one-minute, `-Samples 1600 -StartFrame 438 -RingDump`.
