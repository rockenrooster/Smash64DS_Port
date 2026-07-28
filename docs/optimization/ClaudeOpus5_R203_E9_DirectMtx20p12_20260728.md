# R2-03 E9 — the fighter matrix went through a format nothing needed

**Date:** 2026-07-28
**Phase:** R2-03 (fighter direct draw)
**Verdict:** KEEP. MatrixPrep 122,765 → 110,777, bit-exact. Graduated.

## 1. Where this came from

E6 split MatrixPrep and found 84% in the DObj world build, 55% of that in
`BuildDObjLocalMatrix` at 1,061 ticks/call. E7 measured 48% of those builds
redundant; E8 built the memo to exploit it and **refuted it** — the only correct
key must fold `FTParts->unk_dobjtrans_0x10`, which makes the key nearly as
expensive as the build, and it cost +16,301 ticks/frame.

E8's one useful by-product was the target: 97.5% of local-matrix builds go
through the fighter-parts path, so that path's conversion is the thing to attack,
not the RPY rotation cases E6 had been pointing at.

## 2. The observation

`ndsRendererAdapterBuildFighterPartsMtx` converts

```
Mtx44f (float)  ->  Mtx (N64 16.16, split halves)  ->  NDSRendererMatrix20p12
```

The intermediate is a **pure round trip**. `F2LFixedWExact` packs each value with

```c
COMBINE_INTEGRAL(a, b)   (((a) & 0xffff0000) | (((b) >> 16) & 0xffff))
COMBINE_FRACTIONAL(a, b) (((a) << 16) | ((b) & 0xffff))
```

and `ndsRendererMtxCellS16p16` recombines exactly those halves. High halves in one
word, low halves in the other: nothing is lost, so `recombine(split(x)) == x`.

The whole two-step therefore equals, per cell,
`RoundShiftS32(FloatPow2ToS32(v, 16), 4)`. Working the pairing through, the cell
mapping is the identity — DS cell `(r, c)` is `src[r][c]` for `c < 3`, with the w
column zero except `(3,3)`, which `F2LFixedWExact` pairs with a literal
`0x00010000`, i.e. 1.0 in 16.16.

Computing that directly is **bit-exact by construction**, not by tolerance. Unlike
E6's fixed-point angle lever this needs no fidelity budget and no visual sign-off:
it is the same arithmetic with the packing removed.

## 3. What was actually being paid for

Per call the two-step does 16 `COMBINE` packs, a 64-byte `memset` of the
destination, a 64-byte `Mtx` stack temporary, and 16 calls to
`ndsRendererMtxCellS16p16` — a non-inlined function with bounds checks, invoked
once per cell.

## 4. Verification

Level 2 runs both paths and compares. "Bit-exact by construction" is exactly the
kind of claim E8 proved gets read wrong, so it was checked rather than asserted.

| counter | value |
|---|---|
| direct conversions | 8,108 |
| **mismatches against the two-step** | **0** |
| fallbacks to `syMatrixF2LFixedW` | 0 |

## 5. Measurement

Identical source, identical Task 91 probes, level 0 versus level 1, 128 presented
frames.

| | level 0 | level 1 | delta |
|---|---|---|---|
| `BuildDObjLocalMatrix` | 62,213 | 54,740 | **−7,473** |
| world build | 105,425 | 93,830 | **−11,595** |
| **MatrixPrep** | **122,765** | **110,777** | **−11,988** |
| OwnerPrep | 143,839 | 132,094 | −11,745 |
| census total | 523,514 | 502,725 | −20,789 |

The attributable, repeatable figure is **MatrixPrep −11,988**. The census total
moved further (−20,789); that extra is not claimed here — dropping the 64-byte
`Mtx` temporary and its `memset` has knock-on cache effects that this experiment
did not isolate.

Note the scale: only **12.3 calls/frame** take this branch, so the saving is
~600 ticks per call. That is the packing and sixteen out-of-line cell reads, not
the arithmetic.

## 6. Scope, honestly

`transform_update_mode != 0` is a minority of fighter-parts builds — 12.3 of the
~46 local-matrix builds per frame. The other two branches
(`syMatrixTraRotRpyRSca` and the `BuildFighterTraRotRpyExact` path) still convert
through the same N64 intermediate and were **not** touched here. The same
observation applies to them and is the obvious follow-on, worth roughly the same
per call over ~34 more calls.

## 7. Disposition

Graduated: `override NDS_R2_FIGHTER_MTX_DIRECT := 1` in the published
`smash64ds-battle-playable-hwtri` block, and a non-`override` default in the
tick-HUD/proof block so the instrument stays flag-identical to the shipped ROM.
Default remains 0 so the level-0 arm stays measurable.

---

## 8. E10 — the same cut on the TraRotRpy branch

§6 flagged the other branches as the obvious follow-on. Extending
`ndsRendererAdapterBuildFighterTraRotRpyExact` the same way:

| counter | value |
|---|---|
| direct TraRotRpy conversions | 22,199 |
| **mismatches against the two-step** | **0** |
| fallbacks to the exact/source path | 11 |

Bit-exact again, and this branch is the volume one: **32.5 calls/frame** against
E9's 12.3. Together they cover ~44.8 of the ~46 local-matrix builds per frame.

Worth noting what this branch already was: it computes its rotation from the
sin/cos table in fixed point and *then* packs to N64 split format. The
fixed-point trig E6 proposed as the lever already existed here. The only waste
left was the packing.

| | level 0 | E9 only | E9 + E10 |
|---|---|---|---|
| `BuildDObjLocalMatrix` | 62,213 | 54,740 | **50,778** |
| world build | 105,425 | 93,830 | **90,892** |
| **MatrixPrep** | **122,765** | **110,777** | **108,003** |

### 8a. E10's incremental gain is not distinguishable from noise

Combined the two are **−14,762** on MatrixPrep, which is solid. But E10 *on its
own* adds only **−2,774** over E9 — about 85 ticks per call across 32.5
calls/frame, against E9's ~975 per call across 12.3.

That is below this project's 5,000–7,000 build-placement noise floor, and one
sample per arm cannot separate it from placement luck. Re-running the same ROM
would not help: melonDS is deterministic, so a repeat measures the emulator, not
the noise.

Two plausible reasons the per-call saving is an order of magnitude smaller than
E9's, neither established: the direct builder is a second ~80-line function
under `NDS_RENDERER_ADAPTER_FIGHTER_MATRIX_CODE`, and ITCM is close to full —
the E5 falsifier overflowed it by exactly 100 bytes — so the duplicate may be
evicting hot code it then has to pay for.

**Kept, with the reservation recorded.** It is bit-exact and the combined figure
is real, so reverting gives back a measured gain. But it costs a duplicated
arithmetic body that must stay in sync with
`ndsRendererAdapterBuildFighterTraRotRpyExact` — the two differ only in their
store — and it buys an amount this experiment cannot prove. If ITCM pressure
later forces a choice, this is the first thing to drop: E9 carries most of the
win at a fraction of the code.
