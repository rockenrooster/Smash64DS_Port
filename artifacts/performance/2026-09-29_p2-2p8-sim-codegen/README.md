# P2-2p8: status-change costs, the evaluator's multiplies, and motion reads (2026-09-29)

Solo, no subagents. Gate = the four-CPU tick-HUD ROM on Dream Land
(DK/Samus/Link/Kirby); lab = the NDS_LAB_FOURCPU_SWEEP ROM. Run files are in
`artifacts/performance/2026-09-26_p2-2p8-ftr-item-tail/` under the arm names
below; `ab.txt` holds every summary line quoted here.

## Where a status change's cost goes (gate profile `p2p8-prof-gate5`)

695 status changes a match (0.39 a frame; P90-98 frames carry 1.2-1.6). A
per-symbol regression of each frame's cycles on its `ftMainSetStatus` count
puts ~111K ticks on a change: the pose first play 36K (`ndsFtPoseParse`
11.1K, `ndsFtPosePlay` 8.3K, `ndsFtPoseBindEntry` 4.8K, the flattened-walk
re-walk in `ndsFTParamsInvalidateSubtree` 3.6K, `ndsFtPoseUpdate` 2.6K), the
lean renderer's events and materializations 23K, the motion fetch 17K (plus
the read's wait on the ARM7, which the profile books as idle), the install
10K. Counterfactual: 40K cheaper per change is -41K of P95.

The gate reads 220 clips a match from storage (461,576 B, ~28K ticks of wall
time each) against 481 cache hits. Its raw motion cache is 350,816 B and
wraps once a match: only 26 of the reads re-read a clip the cache had held
(`gNdsR2AnimCacheRestores`); the rest are a clip's first use. The lab ROMs
(full content, 128-141 KB caches) wrap 4-6 times and re-read ~140 of ~350.

## Banked: SMULL for the Q12 cubic (`include/nds/nds_anim_fixed.h`)

`ndsR2AnimEvalQ`'s cubic arm compiled to 115 instructions, 144 cycles an
evaluation (265K a match in `ndsFtPosePlay`): from the clamps GCC proved the
`(s32)` truncations of t, t^2, t^3 and the Hermite terms are no-ops, kept the
wider values and multiplied them 64 x 64 (UMULL plus two MLA per product).
An empty asm (`NDS_R2_AQ_OPAQUE`) withdraws that knowledge, so every product
is one SMULL/SMLAL: the kernel is 119 instructions for all three curves
(190 before), no UMULL/MLA left. The float-AObj cubic
(`ndsR2CubicValueFixed`, `src/import/battleship_sys_objanim.c`) takes the same
barriers. The values are unchanged: `evalq_equiv_*.c` compares the old and new
header over 300M random inputs, clamping cases included -- 0 differences and
equal saturation counts.

## Banked: three exact trims on the status-change path

- `src/nds/nds_ft_pose.c`: the parser's Q30 reciprocal (a Thumb call per Cubic
  track into battleship_ftanim.c) comes from a 256-entry table filled once
  from that same function. Word `gNdsFtPoseRecipLut`.
- `src/import/battleship_ftmain.c`: the setter touches the fighter's topology
  only through the hidden-part loop and the TransN re-link, both driven by the
  anim_desc bits above the five flag bits. With none set in the old or the
  new word the flattened invalidation walk is kept instead of dropped (511 of
  702 changes on the gate). Word `gNdsFtStatusFlatKeep`.
- `src/nds/nds_shield_pose.c`: the shared ShieldPose base rows are decoded
  again only when another blob or heap generation wrote them (5,265 of the
  gate's row decodes, 16,011 on Saffron). Word `gNdsShieldPoseBaseMemo`.

| Run | P50 | P95 | P99 | two-VBlank | SRC median |
|---|---:|---:|---:|---:|---:|
| gate, previous build, words off (`rf0a`/`rf0b`) | 918,336 / 918,208 | 1,248,640 / 1,249,088 | 1,454,208 | 86.2% / 86.4% | 412,608 |
| gate, SMULL build, words off (`b2x_a`/`b2x_b`) | 913,152 / 913,024 | 1,240,384 / 1,240,128 | 1,459,904 | 86.9% / 86.8% | 409,344 |
| gate, SMULL build, words on (`b2y_a`/`b2y_b`) | **912,384 / 912,064** | **1,233,984 / 1,233,984** | 1,460,352 | 87.3% / 87.2% | 408,128 |
| Saffron lab, previous / SMULL off / SMULL on (`rl0_g7`, `b2lx_g7`, `b2ly_g7`) | 1,093,824 / 1,090,112 / **1,089,088** | 1,476,352 / 1,472,832 / **1,467,968** | | 50.5% / 51.3% / 51.9% | |

Replay IDENTICAL across every pair (previous build against the SMULL build,
words off against on, gate and Saffron). Native failures unchanged (0 gate;
the lab rosters' counts are the six owed owners).

## Refuted: refresh a hit clip to the ring's cursor (LRU approximation)

A hit on a clip the ring's cursor reaches within the next 1/2^word of the
arena moved the clip to the cursor. Gate: reads 220 -> 229 (half ring) / 219
(quarter); P95 +3.6K / -1.2K. Lab (half ring): Saffron reads 347 -> 337, Yoshi's
Island 358 -> 347, P95 -5.1K / -2.5K with the shield memo in the same arm.
Replay IDENTICAL. On the gate the ring wraps once a match, so there is almost
nothing for recency to save; on the lab ROMs the working set is larger than
the cache. Removed; the restore counter stays.

## Instrument: which clips a match uses (`gNdsAnimUseBits`)

Tick-HUD builds set one bit per fighter clip fetched (hit or read), cleared
at `ndsR2AnimCachePreloadMatch`. On the lab rosters a four-kind match fetches
178-216 distinct clips (391-492 KB); a one-kind match 72-80. Per kind,
30-55% of a match's clips recur in every observed match of that kind (Kirby
15 of ~55 over seven matches; DK 28 of ~53 over four; Pikachu 33 of ~60 over
three).

## Banked: two smaller ones (build `b3`)

- `include/nds/nds_r2_collision_fixed.h`: the hurtbox reject's local build
  held the Q15 sines and cosines as int64, so its twelve rotation products
  were 64 x 64 multiplies. The pair products are exact in an int32 (at most
  2^30) and each triple product is one widening multiply; UMULL 21 -> 7 in
  `ndsR2CfxBuildLocal`. A host test over 20M random TRS inputs (zero and
  signed-zero angles, random bit patterns, unit and non-unit scales) matches
  the old header byte for byte; `scripts/check-r2-collision-fixed.ps1` passes.
- `src/nds/nds_stage_gx.exec.inc`: the whole-match profile charged 8K ticks a
  frame to one instruction, the first `GFX_CONTROL |= bits` after a stage
  span's DMA was armed (595 cycles, 27 spans a frame: the register read waits
  for the transfer). The next span's DISP3DCNT and ALPHA_TEST_REF writes now
  go before the flush arms DMA0 -- the same writes in the same order. Word
  `gNdsStageGxStateFirst`. The wait mostly moved to the next bus access: STG
  median -0.5K.

| Run | P50 | P95 | SRC median | STG median |
|---|---:|---:|---:|---:|
| `b2y_a`/`b2y_b` (previous build) | 912,384 / 912,064 | 1,233,984 / 1,233,984 | 408,128 | 163,072 |
| `b3s0_a`/`b3s0_b` (local build, state after) | 911,488 / 911,232 | 1,232,832 / 1,234,176 | 406,400 | 163,520 |
| `b3s1_a`/`b3s1_b` (state first) | **910,656 / 910,656** | **1,231,872 / 1,232,448** | 406,336 | 163,072 |

Replay IDENTICAL on every pair.
