# P2-2p8: what the 95th-percentile frames are made of (2026-09-28)

Purpose: rank the remaining levers for WORK-H P95 by measurement before the
next batch. Nothing here changes the shipped code. Runs are the four-CPU
gate ROM (`build-yos-gatechk`, Dream Land, DK/Samus/Link/Kirby) and the
full-roster lab ROM (`build-p2p8-s1`, `NDS_LAB_FOURCPU_SWEEP`); run files are
in `artifacts/performance/2026-09-26_p2-2p8-ftr-item-tail/` under the arm
names below.

## 1. The tail is the sim, and the sim tail is status changes

Top 5% of frames against the 40-60th percentile band (`sp_*`, lab ROM with
`NDS_TICK_HUD_SRC_SPLIT=1`):

| Bucket (ticks) | Dream Land (`sp_def`) | Yoshi's Island (`sp_lk_g5`) | Saffron (`sp_fp_g7`) |
|---|---|---|---|
| SRC (all sim) | +301.5K | +324.9K | +304.0K |
| SITR (status procs, no AI) | +114.9K | +116.4K | +135.7K |
| SHDT (hit detection) | +79.2K | +72.9K | +53.8K |
| SPRM (params: damage status changes) | +63.6K | +65.9K | +49.8K |
| SOBJ (items, weapons, effects, camera) | +23.1K | +34.5K | +40.4K |
| SPHD (physics, map collision) | +19.4K | +27.6K | +12.3K |
| FTR (fighter render) | +186.0K | +85.0K | +137.0K |
| MISC | +62.5K | +120.3K | +150.2K |

Counterfactuals on the gate run (`gs3`, P95 1,302,784): SRC flattened to its
median gives 1,067,456 (-235K); FTR flattened gives 1,224,576 (-78K).

A lab counter (status changes per frame in the SPHC column, motion storage
reads in SPHD; lab builds only) shows the tail frames carry 1.2-1.6 status
changes against 0.2-0.4 in the median band, and on four-kind full-content
rosters (motion cache 0 B) 1.1-1.4 motion reads:

| Run | P95 | status/frame top 5% | mid | reads/frame top 5% | mid |
|---|---|---|---|---|---|
| `sc_def` (DL) | 1,338,688 | 1.22 | 0.26 | 1.16 | 0.24 |
| `sc_fp_g7` (Saffron) | 1,590,272 | 1.46 | 0.38 | 1.09 | 0.24 |
| `sc_lk_g5` (Yoshi's Island) | 1,478,464 | 1.44 | 0.38 | 1.38 | 0.37 |
| `sc_cy_g2` (Jungle) | 1,466,816 | 1.49 | 0.18 | 1.40 | 0.15 |
| `sc_pp_g3` (Zebes, Pikachu x4) | 1,426,368 | 1.18 | 0.26 | 0.20 | 0.02 |
| `sc_kk_g6` (DL, Kirby x4) | 1,206,016 | 1.57 | 0.35 | 0.26 | 0.02 |

## 2. Levers, priced against this data

| Lever | P95 effect (ceiling from the rows) | Notes |
|---|---|---|
| Every motion read a cache hit (-34K each) | -27K to -50K on four-kind full-content rosters; ~0 where the cache already covers the roster | needs RAM (the cache takes what the heap spares) or residency |
| Hit detection (SHDT) cut to 30% | -40K to -49K | the reject kernel is 13,499 tests/match vs 654 float passes on the gate; ~2.1K ticks a test, compose-bound |
| All 44 lean model rebuilds removed | -40K (21 of them: -16K) | 23 distinct keys vs 44 materializations/match at ~330K ticks; 7 spare buffers refused by the heap floor |

Lean event path on the gate (NDS_FTR_LEAN_KTIME, `ek_gate`): per event,
plan resolve 8.1K, program + validate 19.4K (104 events), material rows +
key 13.6K (244), refresh/identity 5.6K, held-entry switch 5.6K (192), joint
table 3.6K, watch 2.6K; materialize 329K (44). Non-materializing events
average ~41K; they happen on status changes (Rebind 87, Material 140).

## 3. Per-frame (median-level) families, Dream Land profile

`builds/p2p8-dl-census3` (profile frames 200-583), ticks per frame: soft
float 92K (fadd 48K, fmul 22K, fdiv 10K; spread across callers, largest
~7K), pose engine 72K (play 30.6K, parse 16.1K, anim keys 10.9K, update
10.4K), map collision 67K, AI 22K, hit detection 18K. The instrument's own
timer reads cost ~154 cycles each, 87 a frame (~6.7K ticks) in the gate ROM.

In this window's 20 costliest frames, half carry a lean materialization
(~320K cycles a frame on average across the 20); the whole-match analysis
above prices that at a smaller P95 share, because the profiled window is
early in the match, where first-time keys cluster.

## Reproduce

- Tail split: `scratchpad` tool `tailmix`-style comparison on `sp_*` rows;
  counters: build `build_lab.ps1 -Extra NDS_TICK_HUD_SRC_SPLIT=1`.
- Profile split: `python scripts/task37_census.py --elf <dlprof elf>
  --split-top-frames 20 --attribute-leaves __aeabi_fadd,... <profile csv>`.
