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

## Banked: the hurtbox reject's walk, rearranged (build `b4`)

The whole-match profile (`p2p8-prof-gate6`) put 2,316 cycles of
`ndsP2HbRejectPoints`' own time on a test (13,791 a match), besides 1,259 a
call in the local build: most of it data-cache misses and 48-byte copies. In
the 67 heaviest frames that are not lean materializations, the kernel and its
local build are +53K ticks over a median frame -- the largest compute delta
there. `src/port/nds_p2_hurtbox_reject.c` now:

- checks a DObj's cache slot before reading its FTParts, composes each level
  straight into its slot and hands the caller a pointer (five copies of a
  3x4 matrix per level gone), and reuses the joint's slot for 1/s_min instead
  of hashing it again;
- hashes the slot multiplicatively: a fighter's DObjs are 136 bytes apart,
  so `(ptr >> 4) & 63` put joints eight or nine places apart on one slot;
- compares the attack and damage memos inline instead of calling `memcmp`;
- gives the damage memo 16 slots: an FTDamageColl is 11 words, so a victim's
  eleven colls take eleven distinct slots (misses 8,499 -> 2,150 a match);
- clears a slot's 1/s_min whenever a root's world is stored (the old walk left
  it from the slot's previous DObj -- a joint on the root would have read it).

Same values tested: rejects/passes/local rejects 13,867 / 286 / 373 in every
arm, shadow mode (`gNdsP2HurtboxRejectMode=2`) 0 flips, replay IDENTICAL.

| Run | P50 | P95 | band work | band SRC |
|---|---:|---:|---:|---:|
| `b4k0_a`/`_b` (same ROM, previous walk) | 909,568 / 909,440 | 1,234,752 / 1,235,520 | 1,223,313 / 1,223,594 | 625,460 / 625,747 |
| `b4k1_a`/`_b` (same ROM, this walk) | 909,376 / 909,440 | **1,232,576 / 1,232,576** | 1,220,643 / 1,220,672 | 623,327 / 623,237 |
| `b6_a`/`_b` (shipped build, previous walk deleted) | 910,656 / 910,272 | 1,231,808 / 1,231,808 | 1,217,843 / 1,217,844 | 614,817 / 614,804 |

Census (`b4cen`, removed before commit): 15,410 composes a match, 3,546 of
them into a slot holding another DObj's world of the same epoch; 4,550 tests
found the joint's own world cached. Of the locals built from a DObj that had
been built before, 51% had the same TRS bits -- but a memo of the six table
lookups keyed on the rotate bits (5,030 hits a match, `b5r0`/`b5r1`) moved
P95 by nothing and band work by -2K (why the hits bought so little was not
measured). Refuted and removed (it cost 2 KB of RAM).

## Banked: the AObj ledger's hash, and a per-coll box for hurtbox re-tests (build `b10`)

- `src/import/battleship_sys_objanim.c`: the event32 ledger's index hashed a
  command pointer with two xor-folds, which keep adjacent words on adjacent
  slots; a script's commands are adjacent words, so linear probing walked
  whole scripts -- 23.6 probes a lookup and 30 an insert on the gate, at a
  load under 20%. The multiplicative hash (top bits of the product) takes
  1.2 and 1.1. Hits and misses are unchanged (1,057 / 3,243 a match): the
  ledger's keys are unique, so a probe returns the same index whatever the
  hash. Word `gNdsAObjEvent32HashMul`.
- `src/port/nds_p2_hurtbox_reject.c`: a coll is tested once per attack coll
  reaching its fighter, so within an epoch most tests after the first found
  the joint's world cached and still paid the head, the walk, 1/s_min, the
  transform and the extents. The separation test's pieces (center, extents,
  row sums, 1/s_min) now stay in the coll's damage-memo entry for the epoch;
  a re-test runs the same test on them (5,324 a match) and falls to the full
  path only when it does not separate. Word `gNdsP2HbBoxCache`. +1 KB of
  BSS (the memo array grows by exactly one D-cache way).

| Run | P50 | P95 | P99 | band work |
|---|---:|---:|---:|---:|
| `b7h0_a`/`_b` (same ROM, folds) | 909,952 / 909,248 | 1,234,112 / 1,233,152 | 1,450,368 / 1,449,664 | 1,217,426 / 1,217,235 |
| `b7h1_a`/`_b` (multiplicative) | 909,632 / 909,632 | **1,228,992 / 1,230,720** | 1,442,816 / 1,441,856 | 1,215,948 / 1,215,937 |
| `b9x0_a`/`_b` (same ROM, no box) | 911,424 / 911,936 | 1,234,688 / 1,234,112 | 1,447,680 / 1,451,328 | 1,221,312 / 1,221,877 |
| `b9x1_a`/`_b` (box) | 910,208 / 910,848 | **1,233,280 / 1,231,808** | 1,442,624 / 1,445,568 | 1,218,794 / 1,217,979 |
| `b10_a`/`_b` (shipped build) | 911,488 / 911,488 | 1,235,136 / 1,235,136 | 1,439,872 / 1,439,872 | 1,219,888 / 1,219,888 |

Replay IDENTICAL on every pair and against `b6`; shadow mode 0 flips; rejects
and passes unchanged. Cross-build the shipped build reads P95 +3.3K over
`b6` (1,231,808): the same-ROM pairs above are the evidence for each change;
the difference between builds is layout (its SRC median moved +0.9K with no
sim change in between).

## Refuted: more spare lean lists (`b8`)

A lab log of every lean materialization (frame, slot, which key words moved)
showed Donkey re-materializing 13 lists a match in-match (~460K ticks each,
key words 0 and 3: material rows and the drawn-DL set), Link 5 (~700K) and
every fighter twice back to back at spawn. With up to three spare buffers
per slot (the gate's heap low-water is ~357K free), materializations fell
only 39 -> 33: Donkey's misses are states none of five buffers held, not an
LRU depth problem. The grown slot state cost P50 +4.6K cross-build despite
padding. Removed; the log is kept as a patch in the session notes only.
Replaying the log against the gate's rows: removing Donkey's in-match
materializations would be P95 -14K, all in-match ones -20K; spreading every
one over two frames (half each) -14K.
