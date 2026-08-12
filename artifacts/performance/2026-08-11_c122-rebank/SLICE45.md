# Slice 45 — where the c122 tail actually is, and two hypotheses it kills

Cycle 122, after the slice-43 withdrawal. Gate bank `WORK-H` P50 936,512 /
P95 1,225,280 (`build-c122-gate`, `NDS_R2_BOTH_CPU=1`, 1600 samples from frame
438, `-RingDump`, DLDI ON). **Gap to 1,120,380: 106,436.**

## The tail decomposition, measured on the GATE ROM

`REBANK.md` split the P95-*setting band* (ranks 60–100). This ranks all 1,600
gate frames by `WORK-H` and compares the top 80 against ranks 400–1200, which
is the population the percentile is actually made of:

| lane | top-80 median | control median | delta | share | presence |
|---|---:|---:|---:|---:|---|
| `WORK-H` | 1,377,312 | 936,512 | 440,800 | 100% | — |
| `GCRA` (`gcRunAll`) | 693,184 | 324,128 | +369,056 | 83.7% | 75/80 |
| ↳ `SINT` interrupt | 317,408 | 153,760 | **+163,648** | **37.1%** | 55/80 |
| ↳ `GCRA` remainder (per row) | 101,280 | 86,400 | +14,880 | 3.4% | — |
| ↳ `SHDT` hit detect | 91,296 | 4,544 | +86,752 | 19.7% | 50/80 |
| ↳ `SPHD` physics | 67,616 | 59,488 | +8,128 | 1.8% | — |
| ↳ `SCPU` CPU AI | 35,424 | 44,704 | −9,280 | −2.1% | 17/80 |
| `MISC` | 123,808 | 104,864 | +18,944 | 4.3% | 28/80 |
| `FTR` fighters | 299,936 | 299,328 | **+608** | 0.1% | **0/80** |
| `STG` stage | 168,992 | 168,704 | **+288** | 0.1% | **0/80** |

`FTR` and `STG` are flat at 0/80 — the renderer is not the tail, which is the
same verdict `REBANK.md` reached from a different band.

**CORRECTION, and it is a trap worth naming.** A first pass here read the
`gcRunAll` remainder as +110,336 (25% of the tail) by subtracting the nested
buckets' MEDIANS from `GCRA`'s median. Medians do not add. Computed per row and
then medianed — the only correct way — the remainder is **+14,880 on 3/80
frames**, and on the c123 bank +9,472 on 9/80. There is no large unbucketed lane
inside `gcRunAll`; the instrument's existing buckets already name the tail.

Likewise `OTHR` appears to have a −116,800 ceiling, which would clear the gate
on its own. It does not exist. `OTHR = ALL − sum(named)` and `named` does NOT
include `WAIT`, so **`OTHR` contains the idle wait**: `OTHR − WAIT` measured
across all 1,600 frames runs 17,600..21,120 and is +192 on the top 80, i.e.
dead flat. Capping `OTHR` is capping idle time. Only `WORK-H` = `(ALL − WAIT) −
HUD` is spendable.

## No single lane clears the gate — capping each at its own median

A perfect fix, therefore an upper bound. Re-taking the 80th of 1,600:

| lane capped at its median | P95 | gain | vs 1,120,380 |
|---|---:|---:|---:|
| current | 1,226,816 | — | +106,436 |
| `SINT` | 1,141,184 | 85,632 | +20,804 |
| `SHDT` | 1,188,096 | 38,720 | +67,716 |
| `MISC` | 1,192,576 | 34,240 | +72,196 |
| `GCRA` remainder | 1,209,792 | 17,024 | +89,412 |
| `SPHD` | 1,211,072 | 15,744 | +90,692 |
| `SINT` + `SHDT` | **1,069,312** | 157,504 | **−51,068** |
| `SINT` + `GCRA` remainder | **1,116,064** | 110,752 | **−4,316** |
| `SINT` + `GCRA` rem + `SHDT` | 1,047,136 | 179,680 | −73,244 |

The gate needs a bundle. `SINT` is in every combination that reaches it.

## Hypothesis 1, KILLED: the arena is short and the match streams

`REBANK.md` and `reloc_backend_assets.c:6247` both say a full anim-cache arena
re-reads its refused assets from ROM for the rest of the match, and the arena
was resized 200,704 → 262,144 earlier the same day **from Boundary's numbers**,
which is the exact mistake cycle 105 recorded ("92,160 was sized against a
41-asset list measured on the Boundary arm"). Read on the gate arm instead —
no build, `-ExtraGlobals` on the banked ROM:

```
ArenaReservedBytes 262,144   ArenaUsedBytes 257,200 (98.1%)
Overflows 0   Rejects 0   RejectedUniqueBytes 0   RejectedUniqueCount 0
Hits 351   Misses 32   Fills 32
ForceLoadTotal 383   ForceLoadDistinct 87   (warm list has 85)
PayloadReadCount 151   TaskmanGeneralHeapFreeMin 70,776
WarmLoaded 83   WarmFailed 0   PrebakeSkips 351   PrebakeDecline* all 0
```

**The cache is healthy on the gate arm**: zero rejects, zero overflows, 351 of
383 force-loads hit, every hit skips the AObj16 normalize, and the heap
low-water clears the 32,768 floor by 38,008. `SWRM` P95 is 640 ticks, so the
warm walk is exhausted long before the window. The goal's zero-reject invariant
holds and needs no work.

This also had to be checked against the instrument, because the top-80
attribution was taken on `build-c122-profile-nodraw` and a profiler ROM's extra
BSS could have starved the arena and manufactured the streaming it was being
used to find. It did not: **every counter above is bit-identical on the two
ROMs**, and `WORK-H` P95 is 1,225,536 against the gate's 1,225,280. The
attribution is not an artifact.

## Hypothesis 2, KILLED: the relocation fixups are the gate

`reloc_backend_assets.c:3746` (R2-06 E8) states it flatly — *"This function is
the whole reason the gate is missed"*, *"all 16 of those frames are cache HITS
that still relocate"* — and refuses to choose a repair without its own phase
split. Built it (`NDS_R2_RELOC_FIXUP_TIMING=1`) and ran the gate:

| phase | ticks, whole run | share |
|---|---:|---:|
| `Sprites` | 22,436,224 | 90.7% |
| `AObj16` | 924,032 | 3.7% |
| `External` | 568,000 | 2.3% |
| `Internal` | 437,312 | 1.8% |
| `Attributes` + `WeaponAttributes` | 97,024 | 0.4% |
| **total** | **24,727,808** | 644 calls |

`FinalizeMaxTicks` is **21,767,168** — a single call is 88% of the whole total,
the one-off battle-interface sprite conversion at setup. Strip it and the
recurring finalize is 2,960,640 ticks over 643 calls: **~4,604 per call, ~1,850
per frame.** The five relocation passes are not the tail, and neither of E8's
two proposed repairs (cache the post-fixup image / give every resident
animation its own destination buffer) is worth building. E8's own instrument
retires E8's conclusion.

Its diagnosis was window-era (128 frames, 16 load frames), which
`whole-match-instrument-only` already marks unusable — this is why.

## What the cost actually is

`ndsRelocAssetIDForToken` is **the largest non-idle symbol in the whole-match
tail**: +41,731 cyc/frame on 71 of the 80 costliest frames, 11,721,422 cycles a
match, 28% of that concentrated in the tail. It is not reached from
`ndsRelocFinalizeLoadedFile` — that is why the phase split above cannot see it.
It is reached from `ndsRelocRemoveFighterAObj16StatusAliases`
(`reloc_backend_assets.c:3126`), which resolved it **for every node in both
status tables** on every force-load, when `nodes[i].addr == data` — one pointer
compare — rejects almost all of them.

The resolver is ~550 instructions of if-chain and, on a miss, two pointer scans
over all 143 + 158 Mario/Fox animation ids (R2-06 E11). A node whose `addr` does
not match is always a miss for this test, so every one of those scans was
discarded. Slice 45 gates the resolve behind the pointer compare.

**Exact, not an approximation.** `&&` short-circuits and the two operands
commute: `ndsRelocFileID` returns `(u32)(uintptr_t)file_id`, the *address* of a
link-time global rather than anything read from it, so the chain has no memory
input and no side effect — its body contains no assignment or counter, and the
two scans it calls are pure loops. The surviving predicate is unchanged, so the
set of removed nodes is identical.

This is deliberately **not** Task 74's memo, which is a dead lane
(`reloc_backend_assets.c:1850`): that added three lookup arrays in `.main.bss`
and lost to a chain of branch-predictable link-time immediates that was already
resident. Nothing is added here — the call count goes down. It is the shape
`a-flat-function-only-lever-is-not-entering-it` describes: a flat function's
only lever is not entering it.

Route-gated (`gNdsR2RelocAliasRoute`, `.data`, `aligned(32)`) so both arms come
from ONE binary at identical placement — R2-06 E11 measured +15,744 P95 from a
relink of a change that added negative bytes, so a cross-build pair cannot read
a cut this size. `gNdsR2RelocAliasVisits` / `gNdsR2RelocAliasResolves` are the
engagement proof; their ratio is the cut.

## Result: KEEP, -12,160 P95

One binary, `builds/build-c122-alias`, 1600 frames from 438,
`NDS_R2_BOTH_CPU=1`, DLDI ON, arms selected by `-SetGlobals`:

| arm | Visits | Resolves | `WORK-H` P50 | `WORK-H` P95 |
|---|---:|---:|---:|---:|
| control (`route=0`) | 16,067 | 16,002 (99.6%) | 936,448 | 1,227,456 |
| candidate (`route=1`) | 16,067 | 1,143 (7.1%) | 936,704 | **1,215,296** |
| delta | 0 | **-14,859 (-92.9%)** | +256 | **-12,160** |

Engagement is proven on BOTH arms from the same run that produced the buckets,
which is what the standing rule asks for: the control resolves 99.6% of its
visits and the candidate 7.1%, against an identical 16,067 visits, so the two
arms walked the same nodes and differ only in how many they resolved. P50 moves
+256 (0.03%) -- this is tail work, not body work, exactly as the 71/80 presence
predicted. The control arm reproduces the c122 bank to within 2,176, which is
what makes a 12,160 delta readable.

`NDS_R2_RELOC_ALIAS_ROUTE` now defaults to 0, so the reorder ships with the
test folded out.

## Files

- `animcache-gate.{json,log}`, `animcache-profilerom.{json,log}` — hypothesis 1.
- `prebake-gate.{json,log}` — AObj16 prebake health.
- `fixuptiming.{json,log}` — hypothesis 2, the phase split E8 demanded.
- `split-top80-nodraw.txt` — the corrected top-80 attribution and leaf pass.
- `alias-{cand,ctl}.{json,log}` — the slice 45 A/B.
