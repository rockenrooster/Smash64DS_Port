# P2-2p8: levers priced and refuted (2026-09-29)

Solo, no subagents. Gate = four-CPU tick-HUD ROM on Dream Land (the fixed
gate roster); lab = the NDS_LAB_FOURCPU_SWEEP ROM (`gNdsLabFourCpuGkind` picks
the stage: 1 Sector Z, 5 Yoshi's Island, 6 Dream Land, 7 Saffron). Every arm
below is a same-ROM A/B word unless marked, and every source change was
reverted. Baseline gate `g0a` (HEAD `2b461f5ae26`'s sources): WORK-H P50/P95
926,912 / 1,260,032, replay identical to the previous gate run.

## Refuted

| lever | measurement | outcome |
|---|---|---|
| Three lean spares (LRU, `gNdsFtrLeanSpareCount` 1 vs 3) | gate: materializations 39 -> 33, materialize ticks 14.97M -> 12.70M, spare hits 19 -> 27; P95 1,264,832 / 1,263,296 -> 1,264,448 / 1,264,064, P50 +1.3K; heap low-water 358,860 -> 234,988 | no P95 change for 124 KB of heap; reverted (`spares-ab.txt`) |
| Hurtbox box memo (per damage box and latch epoch: centre, box extent, radius coefficients; `gNdsP2HbBoxMemo`) | gate: 5,609 memo hits of 14,153 tests, decisions identical (13,867 rejects, 286 passes, 373 local), replay IDENTICAL; P95 1,271,360 / 1,272,768 -> 1,272,384 / 1,270,784 | no gain: a repeated test was already cheap (world cache and damage memo hits); the first test per joint per epoch is the cost. Reverted (`boxmemo-ab.txt`) |
| Skip the lean kernel for a fighter whose input is unchanged (hitlag freeze) | lab census (`pc_*`): 4.8-5.6% of fighter draws have a bit-identical kernel input; frames >= P95 carry no more of them than the median | counterfactual P95 0 on Dream Land; not built |
| Q12 lean kernel compose (32-bit MLA basis, one rounding per level; `gNdsFtrLeanKernelQ12`) | lab Dream Land FTR median 146,624 -> 153,216, Sector Z 149,888 -> 156,864; replay IDENTICAL | SLOWER: the range guards and the wide-path tracking cost more than the 64-bit round-shifts they replace. Reverted (`kernelq12-ab.txt`) |

The spare/box-memo/Q12 builds also showed the cross-build layout effect again:
the box memo's memo-OFF arm read P95 +11K and STG median +2.3K against `g0a`
with no stage change (2.7 KB of new BSS). Only same-ROM A/B figures are quoted.

## Attribution kept for the next lever

- **Lean materialization** (gate profile `p2p8-prof-gate3`, `trigcf.py`): 34
  frames, P95 1,276,650 -> 1,241,245 if their excess over neighbours went, ->
  1,258,906 if spread over two frames. Each costs ~386K ticks (39 a match, 23
  distinct keys a match). The excess is diffuse: texture resolve and
  identity ~47K, corners 38K, run prepare 24K, variant learning 23K,
  materializer self 16K (CPI 7.8), shade 8.5K, state deltas 8K.
- **Stage phases on Dream Land** (lab `NDS_TASK103_STAGE_RUN_PHASE=1`,
  `t103_dl_g6`, lab ticks a frame): prepare 79.9K = matrices 63.4K (27 live
  bindings: world 23.9K, compose 10.7K; 10.9 MVP recalcs 16.4K; the two
  cameras 7.4K) + admission/validate/material/config/owner 15.8K; display
  commits 104.7K over 7.9 segments, 85.9K of it in 53.3 GX runs (1,611 a run);
  6,914 GX words and 26.6 DMAs a frame.
- **Rigid stage sets** (lab census `rc_*`): Dream Land and Yoshi's Island's
  rigid sets are never demoted in a match; Sector Z and Saffron have none (the
  pinned mask is 0), so every binding there takes the live world path.
- **MISC on Dream Land** (lab `NDS_P2_MISC_SPLIT=1`, `ms_dl_g6`, medians):
  MISC 170.8K = MPRT 37.4K + MPRO 33.3K + MCAP 32.6K + MCAM 23.9K. MPRO by
  kind (means): interface 17.1K over 14.5 procs, other 7.6K, particle procs
  4.3K, effects 1.6K. MCAP carries the capture hooks and the native effect
  submits the NDL dispatch makes there.
- **Hurtbox kernel** (Saffron profile `p2p8-prof-yb`): 2,300 cycles a test,
  20,535 tests a match in 379 frames; frames >= P95 average 66 tests (72K ticks).
  Half of a test is the joint world chain (compose, local build, slot walk).
</content>
</invoke>
