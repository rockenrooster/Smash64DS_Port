# Prediction, written before the first emulator run

Date: 2026-08-15. Branch `codex/r2-runtime2`. Base HEAD `771cd4b8312`.
Written after both arms were *built* and before either was *run*. Nothing below
was informed by a tick reading on this tree.

**UNITS: 2 profile cycles = 1 project tick.** Requirement **+32,593 net at
rank-80** (bank `c170` 1,177,920 raw / 1,152,973 net; apparatus 24,947).

## What is being measured

`NDS_R2_FIGHTER_GX_COMPOSE` (slice 43), withdrawn 2026-08-11 at `e03ae311204`
because of the one-frame fighter blink, whose only named blocker — the GX
position/vector matrix-stack leak — was repaired 2026-08-15 at `f3b63e46a06`.

Arms, both fresh at HEAD, `smash64ds-battle-playable-tickhud-hwtri`,
`NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1`:

- **A** `build-c184-gxc-a` — flag 0. Config byte-identical to
  `build-c183-gxstackfix` except `NDS_TASK10_GIT_SHORT` ("ba2c5e5" -> "771cd4b").
- **B** `build-c184-gxc-b` — flag 1 via the new `NDS_R2_FIGHTER_GX_COMPOSE_LAB=1`
  escape, which the published block cannot see (probed: published target reads 0
  with `LAB=1` set).

## The predictions

1. **Engagement.** `gNdsR2GxComposeDeclines` **0** whole match;
   `gNdsR2GxComposeCaptures` > 0 and `gNdsR2GxComposeRoots` > 0. All three read
   **0** on arm A (the symbols do not exist there — the `#if` removes them, so
   the check is that `nm` cannot find them, which is a stronger negative control
   than a zero). If `Declines` is non-zero the CPU compose is still running for
   some bindings and the delta is not the slice's.
2. **The stack stays flat at flag 1.** `gNdsHardwareRendererStatus` bits 8..12
   (position/vector stack level) read **0** and bit 15 (error) reads **0** on
   every sampled frame. This is the acceptance condition the flag was withdrawn
   on. `MATRIX_STORE`/`MATRIX_RESTORE` address absolute levels and do not move
   the stack pointer, so the prediction is *flat*, not "flat within 3".
3. **Ticks.** The historical −13,632 P95 / −10,624 P50 was taken 2026-08-11
   against a 1,258,112 P95. On this tree:
   - **WORK-H P50: −6,000 to −14,000** (point estimate **−10,600**). P50's
     cross-build floor is ~5,700 and P50 kept its sign in all three of
     `VERIFYING.md`'s calibration pairs, so P50 is the discriminator here.
   - **rank-80: −6,000 to −20,000** (point estimate **−13,600**), i.e.
     **0.18x–0.61x** of the +32,593 requirement. **This does not close the gate
     on its own and I am not going to present it as if it might.**
   - **The honest caveat, stated up front: the cross-build `WORK-H` P95 floor is
     ≥14,080 with unreliable sign** (`VERIFYING.md`, cycle-100 calibration), and
     the point estimate is *below* it. A P95/rank-80 number from this pair is
     therefore not by itself a verdict; the verdict has to rest on P50, on the
     two percentiles agreeing in sign, and on the engagement counters.
4. **Mechanism, so the number can be checked against something.** The flag
   deletes the CPU chain multiply in
   `ndsRendererAdapterComposeOwnerWorldsFlat` for the fighter bindings and pays
   FIFO words instead: 12 `MATRIX_MULT4x3` words per local, plus one
   `MATRIX_RESTORE` or seed load and one `MATRIX_STORE` per binding, plus a
   4-row `MATRIX_MULT4x4` world scale per root. `ndsRendererAdapterBuild-
   DObjLocalMatrix` (5,137 tk/fr, 57.14 calls/marginal frame) **still runs** —
   only the composition is moved. So the deleted work is bounded above by the
   fighter share of `MtxMulAffine20p12` (18,909) + `MtxMul20p12` (10,611) =
   **29,520 tk/fr at rank-80**, and the added work is a FIFO-word bill at the
   slice's own measured ~24 cyc/word. A net of −13,600 implies the CPU side was
   ~2x the FIFO side; if the measured delta is positive, the FIFO price is the
   first thing to read, not the last.
5. **Pixels.** Frame-locked captures identical between the arms.
   **Confidence here is the lowest of the five**: slice 43's original
   pixel-identical claim was taken *before* the blink was found, the blink is a
   periodic one-frame event on a 32-frame period, and a single frame-locked pair
   cannot see it. Any pixel difference stops this being a free
   correctness-unlocked win and becomes `BLOCKED(decision: ...)` for the owner.
6. **Invariants.** Equal to the `c170`/`c174`/`c175`/`c176`/`c183` bank on both
   arms: `gNdsBattleTextHudP1Damage` 76, `gNdsDamageSparkScaleCount` 15,
   `gNdsShieldAnimJointAttachCount` 1,352, `gNdsAObjEvent32NormalizedHighWater`
   1,266, `gNdsBattlePackHits` 197, `gNdsObjAnimRunawayCount` 0. A divergence
   means the arms did not play the same fight and no tick delta is quotable
   (`route-ab-cannot-price-gameplay-change`).

## Retraction record this cycle is answerable to

Four predictions have been retracted in this campaign; the last one
(`K-CLOSE`) was 2.2x low on its price half because it never counted the library
its kernel called. The analogous error available here is **pricing the deleted
CPU multiplies and not the FIFO words they are replaced by**, which is why item
4 states both sides.
