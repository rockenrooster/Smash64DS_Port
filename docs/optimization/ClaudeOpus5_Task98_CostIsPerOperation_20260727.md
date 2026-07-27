# Task 98 — Visual approximation attacks the wrong axis: the cost is per-operation, not per-datum

**Date:** 2026-07-27
**Status:** Three proposed visual levers refuted from existing and new
measurement. No runtime change. One direction survives, narrowed.
**Follows:** Task 96 §5a, which established that visual approximation was never
attempted in this campaign and is the contract's second concession.
**Inputs:** `scripts/generated/dreamland_quantization_report.json`,
`src/nds/nds_native_fighter_owner.generated.inc`,
`artifacts/task81-recensus/census.json`, Task 55 E2, Task 62.

## 1. The question

The owner asked what is actually tradeable visually. Three candidates were
proposed and each was sized before asking for approval. All three are empty, and
they fail for **one shared reason** that is the real result of this task.

## 2. Encoding the same geometry more cheaply — REFUTED

`scripts/dreamland_quantizer.py` (Task 61) already models the DS's cheap vertex
opcodes. Measured on the material-exact `source_welded` candidate (259 vertices,
175 triangles, every UV / `rgba` / `submit_class` / `texture_epoch` /
`run_index` preserved, `material_fit: source_exact`):

| encoding | vertex words | reduction | IoU vs source |
|---|---|---|---|
| VERTEX16 (shipping) | 750 | — | 1.0 |
| + axis reuse (`VERTEX_XY/XZ/YZ`) | 546 | **−27.2%, bit-exact** | 1.0 |
| + VERTEX10 | 375 | −50.0% | 0.9969 |

The decimated candidates are far worse and explain Task 62's failure
independently: `c40` scores an IoU of **0.079**.

Applied to the fighter side — `sNdsNativeFighterPreparedDense`, 541 vertices,
1,082 words/frame, all 541 processed every frame (matching Task 90's 541
re-shade iterations) — axis reuse is much weaker, because skinned organic meshes
do not align to axes the way platforms do:

```
consecutive pairs within action runs   465
  share X   28 (6.0%)   share Y   40 (8.6%)   share Z   30 (6.5%)
  at least one axis reusable   80 (17.2%)  ->  1,082 -> 1,002 words, -7.4%
```

**None of it matters, because words do not cost ticks on this path.** Task 55 E2
already ran the experiment: eliding 355 GX words per frame (−9.1% of the replay
buffer, lossless, geometry intact) moved `ALL` P50 by **+64 — flat, over 128
samples**. Task 55 had predicted ~148,000 ticks from it.

| lever | words/frame | expected ticks |
|---|---|---|
| stage axis reuse (bit-exact) | 204 | ~0 |
| fighter axis reuse (bit-exact) | 80 | ~0 |
| stage VERTEX10 (IoU 0.9969) | 375 | ~0 |
| **Task 55 E2, measured** | **355** | **+64** |

Task 55 separately recorded that raw VTX_10 is infeasible for stage scale
anyway — model coordinates reach ±30,272 against an s10 range of ±511, clipping
91% of X and 100% of Z. Task 61's rebasis matrix is the only thing that clears
that, and per the table it would buy nothing regardless.

## 3. Smaller textures — REFUTED

`renderer: texture + material`, ranked from the Task 81 re-census:

| ticks/f | instr/f | tier | symbol |
|---|---|---|---|
| **40,537** | 12,910 | `.main` | `ndsRendererHardwareResolveOrBindTexture` |
| 16,629 | 7,721 | `.text.hot.draw` | `ndsRendererSyncTextureTile` |
| 12,786 | 6,408 | `.main` | `ndsRendererAdapterBuildNativeMaterialSnapshot` |
| 11,478 | 9,406 | `.itcm` | `ndsRendererNativeEmitProductionRawTexturedRun` |
| 8,161 | 2,722 | `.main` | `ndsRendererNativeApplyMaterial` |
| 6,341 | 1,475 | `.main` | `ndsRendererHardwareTextureKeyHash` |
| 6,319 | 1,401 | `.main` | `ndsRendererHardwareFindTexture` |

143,011 ticks/frame over 29 live symbols, 4.81 cycles/instruction, **79.2%
stall**.

The largest entry is resolve-or-bind, and at 25 binds/frame that is **~1,621
ticks per bind**. Hash, lookup, snapshot, apply-material are all per-bind.
Essentially nothing in this class scales with texel count, so halving texture
resolution changes almost none of it.

## 4. The shared reason, which is the actual finding

**This renderer's cost is per-operation overhead, not per-datum throughput.**

~1,621 ticks to bind a texture regardless of its size. ~5,700 ticks of class
cost per bind. 355 fewer GX words worth nothing. 79% stall in the texture class,
and 52–89% stall in sixteen of seventeen classes frame-wide (Task 81).

Visual approximation as normally understood — *less data per thing* — attacks
the wrong axis. Smaller textures, coarser coordinates, fewer bits per vertex:
none of them reduce the **number of operations**, and the operations are the
cost. This is the same shape that killed five consecutive layout tasks (87, 88,
89, 94, 95) and it now has a name.

The corollary is the useful part: **a visual trade only pays if it removes
operations** — binds, runs, draws, dispatches — not if it shrinks their
payloads.

## 5. What survives

- **Bind-count reduction** is capped low: Task 93 measured 22 distinct texture
  keys behind the 25 binds, so reordering or memoising caps at ~12% without
  merging the textures themselves.
- **Triangle / run count reduction** is the one visual lever whose mechanism is
  not refuted, because fewer triangles means fewer runs and fewer draws, not
  merely less data per draw. Task 62 aimed here and produced an unusable
  measurement (§6), but the material-exact generator it lacked now exists and
  renders pixel-identical to flag-0 at frame 438.

## 6. Correction to Task 62's headline

Task 62's `−297,600 ticks (−29.6%)` was quoted in this session as a measured
prize. It is not usable. `RENDER_SUBMIT`'s ninth field is a derived estimate over
the projected class only —
`6 x ProjectedNoZ + 9 x (Cross + Decal + PrimDepth + RangeOrMatrix)` — and its
`1152 -> 396` is exactly `9 x 44` with **ProjectedNoZ at zero**: all 126 no-Z
triangles stopped being submitted. That matches its own screenshots showing 0%
geometry. The tick saving is substantially work not done because the stage was
not drawn. The task doc does label it rejected-experiment evidence; the number
was read here without the caveat, and this section is the correction.

## 7. Method rule earned

Both refuted levers in §2 and §3 were proposed to the owner **before** their
tick anchor was checked, and both were empty. The anchors already existed —
Task 55 E2 for words, the Task 81 census for texture — and cost minutes to find.

**Find the tick anchor before proposing a lever, not after.** A reduction ratio
(−27.2% of words, −50% of texels) is not a saving; it is a saving only if some
prior measurement ties that quantity to ticks on the same path. Where no anchor
exists, say so and measure it first — as Task 96 did for the `AObj` chain, which
is why that one held up.
