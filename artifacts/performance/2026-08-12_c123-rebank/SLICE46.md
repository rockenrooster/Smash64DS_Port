# c123 — bank after slice 45, and the warm-preload drift

`build-c123-gate`, `NDS_R2_BOTH_CPU=1`, 1600 samples from frame 438,
`-RingDump`, DLDI ON.

## The bank

| | `WORK-H` P50 | `WORK-H` P95 |
|---|---:|---:|
| c122 (slice 43 withdrawn) | 936,512 | 1,225,280 |
| **c123 (slice 45 in)** | **938,944** | **1,213,440** |
| delta | +2,432 | **−11,840** |

Slice 45's same-binary A/B predicted −12,160; the re-bank landed −11,840, 320
apart. **Gap to 1,120,380: 93,060.**

## Re-attribution on the c123 gate rows

Top 80 `WORK-H` frames against ranks 400–1200, per row (never by subtracting
medians — see the trap below):

| lane | top-80 | control | delta | presence | ceiling |
|---|---:|---:|---:|---|---:|
| `SINT` interrupt | 297,504 | 152,320 | **+145,184** | 51/80 | **−72,448** |
| `SHDT` hit detect | 93,152 | 4,736 | **+88,416** | 49/80 | **−37,760** |
| `MISC` | 123,296 | 103,520 | +19,776 | 28/80 | −36,352 |
| `SPHD` physics | 68,960 | 58,144 | +10,816 | 24/80 | — |
| `GCRA` remainder | 95,904 | 86,432 | +9,472 | 9/80 | −13,376 |
| `OTHR` − `WAIT` | 19,840 | 19,648 | **+192** | **0/80** | — |
| `FTR` / `STG` | — | — | −128 / +192 | **0/80** | — |

`SINT`+`SHDT` capped together is −143,488, which clears the gate; neither does
alone. The renderer is not the tail at any size.

**Two lane-sizing traps, both hit this cycle.** Medians do not add: subtracting
the nested buckets' medians from `GCRA`'s invented a 110,336 "unbucketed" lane
that is +9,472 per row. And `OTHR = ALL − named` where `named` excludes `WAIT`,
so `OTHR` CONTAINS the idle wait — it shows a −116,800 ceiling that is entirely
idle time. Only `WORK-H` = `(ALL − WAIT) − HUD` is spendable.

## Slice 45 confirmed at the symbol level

Fresh top-80 split on `build-c123-profile` (`NDS_TICK_HUD_DRAW=0`):
`ndsRelocAssetIDForToken` **41,731 → 13,217** cyc/frame (−68%), 71/80 → 66/80.

## Slice 46 — the warm preload never finishes, and warms the wrong assets

Two independent defects in the same mechanism, both measured on the gate arm.

**1. The walk does not complete.** `gNdsR2AnimWarmLoaded` reads **83 of 85** at
the end of a whole match. `ndsR2AnimCachePreloadStep` loads exactly one asset
per scene update, which was sized when the list held 41.

**2. The list has drifted off the match.** `gNdsR204AnimSeen` dumped from the
c123 gate ROM holds **87** ids. Against the 85-entry list:

| | count |
|---|---:|
| used AND warmed | 57 |
| **used but NOT warmed** | **30** |
| **warmed but NEVER used** | **28** |

The 30 are the misses — `gNdsR2AnimCacheMisses` reads 32 on the same run. They
are not spread: the miss-only symbols `strncasecmp` and
`ndsRelocApplyWordByteSwap` sit on **22 of the 80** costliest frames, and the
whole FAT family (`armCopyMem32`, `get_fat`, `f_lseek`, `f_read`, `_read_r`,
`validate`, `move_window`) sits on 57–58/80.

**Predicted value: −32,512.** Capping `SINT` at its median on only the 32
highest-`SINT` frames takes P95 1,215,488 → 1,182,976. Those frames occupy
`WORK-H` ranks 0–133, straddling the rank-80 cut, which is why they move the
percentile at all.

**It SHRINKS the arena.** The old list cached 83 warm entries and then filled 32
on demand — 115 residents, 257,200 bytes, 98.1% of a 262,144 arena. The measured
set is 87 at roughly 194,500. The 28 never-used warms cost twice: arena bytes,
and countdown steps spent while the assets the match wanted went unwarmed. So
this needs no RAM budget and cannot threaten the zero-reject invariant.

**The fix, both halves.** Replace the list with the measured 87, and step
`gNdsR2AnimWarmStep` (default 4) assets per update instead of 1.

**The bound on stepping is the BGM packet seam, not loading-time generosity.**
R2-04 E4 loaded all 41 in ONE call at this seam and Boundary refused the build
on the ADPCM smoke (SeamMiss 0→1, `gNdsAudioBgmPlaying` 1→0): the stream is
double-buffered at 8,196 bytes against 44,100 a second, so the main thread owns
~186 ms between seams. E4's failure prices one load at >4.5 ms, so a safe step
is single digits. Four finishes 87 entries in 22 updates for ~18 ms of a 186 ms
budget. **Boundary's BGM smoke is the verifier for this change.**

**A route A/B is legitimate here** in a way it is not for a gameplay change: a
hit and a miss load the same bytes and differ only in provenance. The cache's own
contract says every failure path degrades to the uncached load, a performance
outcome and never a correctness one. `-SetGlobals gNdsR2AnimWarmStep=1` restores
the old cadence at identical placement.

## Next candidate after this: `SHDT`, and the blocker is now resolved

`SHDT` is +88,416 on 49/80 with control median 4,736 against a top-80 median of
93,152 — a **19.7x step**, the slice-44 shape. It is 124 of 1600 frames (7.8%),
45 of them in the top 80.

Every internal redundancy is already memoised (chain-prefix, local build,
inverse, `vec_scale`, and the invalidation flattened at cycle 106), so the only
correctness-legal lever is **transforming fewer hurtbox joints** — exact call
deletion, which the frozen-float rule permits.

That needs a per-hurtbox reach bound, and the soundness question was "does
animation translate joints?" — because a bound derived from static bone lengths
is only valid if it does not. **`reloc_backend_compat_shims.c:1504` already
answers it**: the cycle-106 flattening records that for the fighter DObj tree
"animation moves rotations, not joints."

Do **not** take that on faith — make it self-checking. The design:

1. AOT per hurtbox joint: `maxreach = Σ|rest translate|` along its chain to the
   root, a link-time constant of the model.
2. Runtime guard, fail-closed: assert each joint's local translate equals its
   rest translate; any mismatch falls back to the full path for that fighter.
3. Broad phase per (attack collision, hurtbox): skip building the joint's world
   transform when
   `dist(attack_center, victim_root) > attack_radius + hurtbox_radius + maxreach`.

Exact by construction — the surviving tests are unchanged and a skipped joint is
one that provably could not intersect. The guard is what makes it sound without
trusting a comment.

The seam is port-owned: `ftMainProcSearchHitAll`
(`reloc_backend_diagnostic_recorders.c:5678`) already wraps the decomp call, and
`ndsFTParamsInvalidateFighterParts` already owns the per-frame memo policy, so
neither needs a `decomp/` edit.

## Result: KEEP, -17,216, and the re-attribution after it

| arm | `WORK-H` P50 | P95 | Warm | Miss | Hits | arena |
|---|---:|---:|---:|---:|---:|---:|
| c123 bank | 938,944 | 1,213,440 | 83/85 | 32 | 351 | 257,200 |
| new list, `step=1` | 938,048 | 1,204,992 | 85/85 | 2 | 381 | 192,240 |
| **new list, `step=4`** | **938,752** | **1,196,224** | **85/85** | **2** | **381** | **192,240** |

Split **-8,448 list / -8,768 stepping**, the step halves being the SAME binary via
`-SetGlobals`, so no placement floor applies to that half. Rejects 0, Overflows
0, heap free-min 70,776, `gNdsAudioBgmPlaying` 1, `ForceLoadTotal` 383 /
`Distinct` 87 identical to control, and the VBlank histogram improves (4x 31→10,
5+x 21→14). **Boundary GREEN.**

Re-attributing on the new bank, ownership moved:

| lane | delta | presence | ceiling |
|---|---:|---|---:|
| `SHDT` | **+102,816** | **57/80** (was 49/80) | −38,912 |
| `SINT` | +108,384 | 47/80 (was 51/80) | −56,512 |
| `SPHD` | +17,024 | 28/80 | — |
| `MISC` | +16,864 | 26/80 | −29,248 |
| `FTR` / `STG` | −736 / +608 | **0/80** | — |

## Two avenues closed after this, both by measurement

**Every memo in the ROM is healthy** — so no banked optimization is silently
degrading and there is no third cut of this shape:

```
MP endpoint 42,964 hits / 9 fills / 0 overflow    MP yakumono 83,546 / 11
MP kind     68,057 / 13                            MP vertexF32 83,810 / 13 / 0
anim recip  54,526 hits / 0 misses                 tex memo 17,907 / 9 / 0 stale
mat key     59,100 skip / 40 build                 pre-validate reject 0, evict 0
```

**`MISC` is particles, and particles are flat.** The cumulative split reads
particle draw 86,371,776 tk over the window (53,982/frame, 52% of `MISC`), effect
draw 15,569,536 (9,731), weapon draw 2,594,880 (1,622), against a `MISC` tail
premium of only +16,864 on 26/80. That matches the standing "particles flat
~47,000, P50 only" note — the majority of `MISC` is not reachable through P95.

**`SHDT` is smaller than its bucket suggests.** `--pc-detail func_ovl2_800EDBA4`
(the chain walk) reads **2,232 calls over 1600 frames, ~18 per hot frame** — so
essentially every hurtbox joint is walked, confirming the structure — but the
function is 976,281 cycles, **0.03% of the window**, and its hot PCs are cold
`ldr`s at 19–31 cyc/insn, not arithmetic. Tracing the soft-float attribution,
deleting hurtbox transforms outright reaches only ~30% of `SHDT`'s premium, so a
reach bound realistically captures ~15–20K, not the −38,912 ceiling. Size it
against that before spending the interpose.

## Files

- `gate.json`, `gate-rows.csv`, `rebank.log` — the c123 bank.
- `profile/`, `split-top80.txt` — the fresh attribution.
- `animseen.log` — the working-set bitmap dump.
- `warm-{cand,ctl}.{json,log}` — the slice 46 A/B.
