# R2-03 E6 — MatrixPrep is one function, and it is a float-input matrix builder

**Date:** 2026-07-28
**Phase:** R2-03 (fighter direct draw)
**Verdict:** located. 46% of MatrixPrep is `BuildDObjLocalMatrix` at 1,061
ticks/call.

## 1. Why split before building

E4 measured MatrixPrep at 91,338 ticks/frame — four times MaterialPrep and 63%
of the owner-preparation gap. E5 then spent a full cycle on the *smaller* bucket
and proved its obvious lever banks nothing: the UV loop it targeted is ~119
writes/frame against a 21,504 bucket.

That is the standing rule about profiling the whole owner before optimising a
loop inside it, applied one level down. So split this bucket first.

## 2. Instrument

Task 91 counters inside `ndsRendererAdapterPrepareNativeOwnerMatrices`, on the
tick-HUD ROM the P95 gate actually measures (profile level 0, where the M2
ledger cannot compile). Two stops 128 presented frames apart, differenced.

The function is a per-frame camera fetch, then per selected binding a DObj
world-matrix build and one affine multiply. `Bindings` and `Calls` normalise
them, because a per-binding cost and a per-frame cost look identical in a total.

## 3. First split — it is not the multiply

| component | ticks/frame | share | per unit |
|---|---|---|---|
| **DObj world build** | **90,040** | **84%** | 3,062 / binding |
| camera fetch | 8,490 | 8% | 4,684 / call |
| affine multiply | 4,997 | 5% | 170 / binding |

97% of MatrixPrep accounted. 29.4 bindings and 1.8 calls per frame.

The world build costs **18x the multiply it feeds**. Any plan that started by
optimising the matrix arithmetic would have been aimed at 5% of the bucket.

## 4. Second split — the prefix cache is already working

The DObj world cache is a linear-probed hash reset once a frame, so a lookup is
cheap and the cost must be in the miss path. Splitting that:

| component | ticks/frame | share of world build |
|---|---|---|
| **`BuildDObjLocalMatrix`** | **49,306** | **55%** |
| compose + store prefix | 25,466 | 28% |
| chain walk + lookups | ~15,268 | 17% |

| counter | per frame |
|---|---|
| entry-cache hits | 0.0 |
| ancestor-cache hits | 28.1 |
| cold starts | 1.9 |
| local matrices built | 46.5 |
| summed chain depth | 183.8 |

Two things to read carefully here.

**Entry-cache hits are zero, and that is correct, not a bug.** Each binding's
own world matrix is requested exactly once per frame, so it can never be found
already cached under its own key. The cache earns its keep on *ancestors*.

**The cache is already absorbing 75% of the work.** Summed chain depth is 183.8
node-visits per frame but only 46.5 local matrices are actually built. There is
no large win left in caching harder; 1.9 cold starts per frame is one per
fighter, which is the floor.

## 5. Where the 1,061 ticks go

`ndsRendererAdapterBuildDObjLocalMatrix` walks the DObj's `xobjs` and dispatches
on `xobj->kind` into the **source's** `syMatrix*` routines, then converts the
result with `ndsRendererAdapterMtxFromN64`.

Those routines take `f32` angles. `syMatrixRotRpyR` is not `__sinf` — it uses
the source sin table — but the index comes from `macros.h:28`:

```c
#define SINTABLE_RAD_TO_ID(x)  ((s32)((x) * ((f32)ARRAY_COUNT(gSYSinTable) / PI32)))
```

One float multiply and one float-to-int per angle, three angles per RPY
rotation, on a CPU with no FPU. Translations read `dobj->translate.vec.f.*`,
also `f32`. The port already carries a hand-rolled software converter for this
shape (`ndsRendererAdapterFloatPow2ToS32`). Then the N64-format result is
converted again into DS 20.12.

So each local matrix pays: software float scaling of the inputs, a table lookup,
integer matrix assembly in N64 format, and a second conversion into DS fixed
point.

## 6. The lever

A port-side fixed-point local-matrix builder that takes the DObj's rotation and
translation and emits DS 20.12 directly, skipping both the float angle scaling
and the N64 intermediate.

`AGENTS.md` licenses exactly this — "generated, precomputed, manually rewritten,
and fighter/stage/move-specific DS implementations are encouraged when they are
faster and mechanically equivalent" — and `decomp/` staying read-only does not
freeze the algorithm, only the reference copy of it.

Ceiling is the full 49,306 ticks/frame, 4.4% of the 1.12M gate. Realistically
less, because the table lookup and matrix assembly are not free in any
representation; the recoverable part is the float scaling and the double
conversion.

Two constraints on any such builder:

- Rotation is float-sourced and the sin table is 2,048 entries over pi. A
  fixed-point index computation must land on the *same* table entry as the float
  one for every angle the match produces, or poses shift. That is a bit-exactness
  claim about a named quantity and belongs in a falsifier before the builder is
  written, not after.
- `nGCMatrixKind*` has upwards of twenty cases here. A replacement that covers
  only the common ones must fall through to the existing path for the rest
  rather than approximate them.

## 6a. E7 — half of those builds are redundant, and that cut needs no exactness argument

§6's lever requires proving a fixed-point angle index lands on the same sin-table
entry as the float one for every angle the match produces. That is a claim about
poses and expensive to get wrong. Before making it, ask E5's cheaper question:
**is this work redundant at all?**

A fighter animation does not necessarily move every joint every frame. Any joint
whose local matrix comes out identical to the last time it was built was rebuilt
for nothing — and reusing it replays the exact bytes the current code produced,
so there is no numerical equivalence question whatsoever.

Probe: hash all sixteen matrix elements per `BuildDObjLocalMatrix` return, store
per DObj, count agreement with the previous build.

| | first attempt | after hash fix |
|---|---|---|
| hits/frame | 15.2 | **21.9** |
| misses/frame | 15.2 | 23.8 |
| evictions/frame | 16.9 | 2.0 |

The first attempt keyed slots on `(ptr >> 4)`. DObjs come from a pool, so the low
address bits are strided and 37% of calls evicted a live entry — which floors the
hit rate the probe can report. Knuth multiplicative hashing on the pointer, taking
high bits, drops eviction to 2.0/frame and the real rate appears.

**~48% of local-matrix builds are redundant** (21.9 of 45.7 non-evicted).

Worth roughly 48% of the clean-build 49,306 = **~23,700 ticks/frame**, 2.1% of
the gate. Two design notes for whoever builds it:

- **Key on the inputs, not the output.** Comparing the eight source floats
  (`translate.vec.f.*`, `rotate.a`, `rotate.vec.f.*`, scale) is far cheaper than
  building the matrix to discover it was unchanged. Keying on the output would
  pay the 1,061 ticks before learning it was wasted.
- **Size the table to the working set, not the address space.** 46.4 builds per
  frame over two fighters means ~46 distinct DObjs; 64 entries of
  {pointer, inputs, matrix} is about 6.4 KB, against ~50 KB for the 512-entry
  probe table. The DObj world cache next door is per-frame and cannot be reused
  for this — the whole point is to survive the frame boundary.

This and §6 compose: the memo removes half the calls, and a fixed-point builder
would make the surviving half cheaper. The memo is strictly the safer of the two
and should land first.

## 7. Cross-check with the other buckets

Same run, per presented frame: Total 502,869, OwnerPrep 128,066, MatrixPrep
107,210, MaterialPrep 20,126, Validate 10,147, Reset 6,162, Walk 3,032. The
MatrixPrep figure is higher than E4's 91,338 because this build carries the
census probes themselves; the *ratios* are what this experiment claims, not the
absolute, and the 97% accounting is what makes them trustworthy.

## 8. Reproduce

```
make BUILD=build-r2-03-e6-mtxsplit \
  TARGET=smash64ds-battle-playable-tickhud-hwtri NDS_TASK91_DRAW_PHASE_CENSUS=1
```

Two GDB stops on `ndsBattlePlayableFrameCompleteMarker` at presented frames 500
and 628, differencing `gNdsTask91Mtx*`. Counters accumulate from boot, so a
single absolute read is dominated by load and title frames.
