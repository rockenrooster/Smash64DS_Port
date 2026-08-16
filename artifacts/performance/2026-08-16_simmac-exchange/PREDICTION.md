# Registered before the first arm finished. 2026-08-16, build `build-c208-simmac`.

Written from the linked ELF alone, with `arm0-run.log` still empty. Every number
below is falsifiable by the runs that follow it in this directory.

## What the ELF already proves, with no run at all

`arm-none-eabi-objdump -d builds/build-c208-simmac/…tickhud-hwtri.elf`:

| body | bytes | own insns | helper calls per entry |
|---|---:|---:|---|
| `ndsR2SimMacBaseGetWorldPosition` (the decomp float `gmCollisionGetWorldPosition`) | 196 | 80 | **9 `__aeabi_fmul` + 9 `__aeabi_fadd`** |
| `ndsR2SimMacBaseCompose` (the decomp float `func_ovl2_800ED490`) | 580 | 227 | **36 `__aeabi_fmul` + 27 `__aeabi_fadd`** |
| `ndsR2SimMacShadowTransform` (fixed) | 1,320 | 330 | 1 × `ndsR2CfxLoadF32` |
| `ndsR2SimMacShadowCompose` (fixed) | 988 | 247 | 2 × `ndsR2CfxLoadF32` |
| `ndsR2CfxLoadF32` | 448 | 112 | none |

Two independent confirmations fall straight out of that table:

1. The helper counts reproduce `SIMSIDE.md` §1's dynamic measurement **to the
   unit** — 18 and 63 — from a different build by a different method.
2. Both fixed bodies are ARM state (`e92d47f0 push …`), contain
   `smull`/`smlal` and contain **no `__aeabi_lmul`, no soft float, no
   `__udivmoddi4`, no divide and no root**. The refusals are satisfied by
   construction rather than by a guard.

**And the footprint is already visibly against the transform.** The fixed
transform is 1,320 B + 448 B shared against a 196 B float body it would replace:
**9.0×**. The fixed compose is 988 B + 448 B shared against 580 B: **2.5×**.

## The arithmetic that drives the prediction

Every float that crosses the f32 boundary costs one `ndsR2CollisionF32ToFixed`
or `…FixedToF32`, so the ratio that matters is CONVERSIONS PER DELETED FLOAT
OPERATION, and the two bodies sit at opposite ends of it:

| body | floats in | floats out | edge conversions | float ops deleted | conv/op |
|---|---:|---:|---:|---:|---:|
| transform | 15 | 3 | 18 | 18 | **1.00** |
| compose | 24 | 12 | 36 | 63 | **0.57** |

Gross soft-float library bill per entry, from `SIMSIDE.md` §3's measured
marginal-80 per-call rates (`fmul` 13.23 tk, `fadd` 18.98 tk):

```text
transform  9(13.23) + 9(18.98)  =   289.9 tk/entry   (SIMSIDE 13,091/45.2 = 289.6)
compose   36(13.23) + 27(18.98) =   988.8 tk/entry   (SIMSIDE 18,759/19.0 = 987.3)
```

## PREDICTION

Replacement cost, at roughly 0.5 tk per ARM instruction with call overhead:

* transform ≈ **180–260 tk/evaluation** → **R = 1.1×–1.6×**
* compose ≈ **235–320 tk/evaluation** → **R = 3.1×–4.2×**

Combined over the two bodies' full 31,850 tk/fr lane, at the mid-points
(1.35× and 3.6×):

```text
18,759 (1 - 1/3.6)  + 13,091 (1 - 1/1.35)  =  13,548 + 3,394  =  16,942 tk/fr
                                           =  0.179x of +94,481
```

**So the prediction is that the "warm MAC" subset does NOT convert at one rate,
and that sizing it by function was the error**: the compose converts near the
5.14× prior's neighbourhood and the point transform barely converts at all, and
the difference is not warmth, transcendentals or Thumb — it is the number of
f32 boundary crossings per deleted operation, which is a property of the
FUNCTION SIGNATURE and is knowable before any build.

## What would falsify it

* A transform R above 2.0× falsifies the boundary-crossing account.
* A compose R below 2.0× says the edge conversion is far dearer than 15
  integer instructions and the whole leaf route is dead, chain or no chain.
* Any whole-match invariant differing between arms falsifies the INSTRUMENT
  (the arms cannot play different fights by construction) and voids every
  number taken from it.
