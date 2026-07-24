# Task 55 — Stage geometry reduction: E0 stream census + lever analysis

**Date:** 2026-07-24
**Branch:** `codex/task55-stage-geom-reduction` (E0)
**Parent:** `a463975` (master — Task 53 replay live + Task 54 STOP docs)
**Source data:** `artifacts/performance/2026-07-23_task49-differ-task53-B-flagon1-stage.json`
(Task 49 GX differ capture of the live replay stream, frame 445, owner 0
STAGE, 8 rigid Dream Land layer0 bindings, 2,229 entries / 2,996 words,
0 overflow / 0 fault). This is the exact `owner->words[]` the replay loop
drains — the ~720K GX-throughput floor Task 54 proved invariant.

**Outcome:** the spec's two named levers (VTX_10, stripify) are NOT the
highest-value safe cut. A **third lever the capture exposes — redundant
state-write elision** — cuts ~20.6% of stage words **losslessly** and is the
chosen reduction for E1. VTX_10 is infeasible (coordinates out of s10 range);
stripify is small (5.6%) and topology-limited.

## 1. Per-class word census (the 2,996-word stream)

| class | name | entries | words | %words |
|---|---|---|---|---|
| 20 | VERTEX16 | 606 | 1,212 | 40.5% |
| 18 | COLOR | 606 | 606 | 20.2% |
| 19 | TEX_COORD | 591 | 591 | 19.7% |
| 0 | CONTROL | 216 | 216 | 7.2% |
| 9 | MATRIX_LOAD4X4 | 8 | 128 | 4.3% |
| 6 | TEXTURE_BIND | 41 | 82 | 2.7% |
| 16 | BEGIN | 54 | 54 | 1.8% |
| 1 | ALPHA_TEST | 36 | 36 | 1.2% |
| 5 | TEXTURE_PARAM | 36 | 36 | 1.2% |
| 15 | POLY_FORMAT | 27 | 27 | 0.9% |
| 7 | MATRIX_MODE | 8 | 8 | 0.3% |
| | **TOTAL** | **2,229** | **2,996** | **100%** |

Groupings: the per-vertex bundle (VERTEX16 + COLOR + TEX_COORD) is 2,409 words
(80.3%) — this is the spec's "~81% vertex." But VERTEX16 alone is 40.5%,
and **COLOR + TEX_COORD together are 39.9%** (1,197 words), almost as large.

## 2. The 54 BEGINs are all GL_TRIANGLES (mode word 0)

verts/prim: min 3, max 114, mean 11.22. Distribution: 15 prims of exactly 3
verts (isolated triangles), 26 in [4,10], 12 in [11,50], 1 of 114.
Per-binding: binding 1 = 216 verts/35 prims; binding 3 = 198 verts/3 prims
(one 66-vert primitive); binding 5 = 30/3; binding 7 = 162/13.

## 3. Lever 1 — VTX_10: INFEASIBLE (coordinates out of range)

Stage vertex model-space coords (decoded from the VERTEX16 words; these are
model-space — the geometry engine applies the rigid model matrix):

| axis | min | max | range | |axis|>511 (s10 clip) |
|---|---|---|---|---|
| X | −29,472 | 30,272 | 59,744 | **554/606 (91.4%)** |
| Y | −11,013 | 24,688 | 35,701 | **522/606 (86.1%)** |
| Z | −4,022 | 4,095 | 8,117 | **606/606 (100%)** |

VTX_10 packs x/y/z into one 32-bit word at **10-bit signed precision each
(range −512..511)**. The Dream Land stage occupies a model-space volume far
exceeding a ±512 cube. **91% of X and 100% of Z would clip/corrupt.**
VTX_10 is not a usable lever for these coordinates. (It is appropriate for
small objects/HUD quads that fit a ±512 cube — not a full stage.) The native
`sm64ds-decomp` reference uses `GFX_VERTEX10` (0x4000490) for box/quad tests,
`GFX_VERTEX16` (2-word) for full geometry — consistent with this finding.

## 4. Lever 2 — stripify: small (5.6% words) and topology-limited

606 vertex slots, **427 unique positions** (29.5% are exact duplicates of an
earlier vertex — the rigid topology does share some vertices, but not densely).
Of 148 adjacent triangle-pairs *within primitives*, **42 (28%) share an edge**
(strip-convertible). Greedy maximal-strip ceiling (in current triangle order):

- 202 triangles, 606 verts → stripified ≈ 522 verts → **84 verts saved**
- **168 words (5.6% of 2,996)** → ~40K ticks off ~720K (≈2.4% of ALL).

Below the spec's 10% threshold, lossless but limited, and requires a
strip-generation reorder of the corner table. Not the primary lever.

## 5. Lever 3 (the real find) — redundant state-write elision: LOSSLESS 20.6%

The geometry engine's `GFX_COLOR` (`0x4000480`) and `GFX_TEX_COORD`
(`0x4000488`) are **persistent state registers**: a vertex uses the currently-
held color/texcoord until the register is rewritten. Confirmed:
- `ndsRendererHardwareWriteColorWord` (`nds_renderer.c:1176`): `GFX_COLOR = value;`
  a plain persistent write; no reset anywhere (frame-wide `GFX_STATUS`/color-
  reset grep is empty).
- `ndsRendererHardwareEndBatch` (`nds_renderer.c:13351`): does
  `glDisable(GL_ALPHA_TEST)` + bookkeeping only — does **not** reset color or
  texcoord. The comment there: "glEnd() as a dummy FIFO write. A later glBegin()
  starts the next primitive group, so only restore state owned by the logical
  source-triangle batch here."
- `ndsRendererNativeStageEmitNoZVertex` (`nds_renderer.c:20448`): writes
  `GFX_COLOR` **unconditionally for every vertex**, then the vertex. So
  consecutive vertices with the same `packed_color` re-write the identical value.

Measured redundancy across the captured stream (consecutive same-value writes):

| state | writes | runs (value changes) | redundant | %redundant |
|---|---|---|---|---|
| COLOR | 606 | 50 | 556 | **91.7%** |
| TEX_COORD | 591 | 529 | 62 | 10.5% |
| **total** | 1,197 | | **618** | |

**618 of 1,197 state words are redundant (same value as the immediately
preceding write of the same class).** Eliding them at capture time shrinks
`owner->words[]` by **618 words = 20.6% of the 2,996-word stage stream**, and
produces a **bit-identical render** (the geometry engine holds the value; the
omitted writes would have changed nothing).

Predicted ALL delta: 20.6% of the ~720K GX-throughput floor ≈ **~148K ticks
off STG+OTHR, ~8.8% off ALL P50 (1.68M).** Lossless, no fidelity tradeoff.

### Per-binding detail (why it's safe)

- COLOR: only **9 unique values** across the whole stage, grouped into 50 runs.
  Most bindings draw long vertex runs under a single color; the per-vertex
  re-write is purely the generic-emit path being state-naive.
- TEX_COORD: 125 unique, 529 runs — lower redundancy (textures vary per-vertex
  UV), so the gain there is smaller (62 words). Still free.

## 6. Why this lever is strictly better than the spec's two named ones

| lever | words cut | % of stream | lossless? | predicted ALL delta |
|---|---|---|---|---|
| **state-write elision (chosen)** | **618** | **20.6%** | **yes** | **~8.8%** |
| stripify | 168 | 5.6% | yes | ~2.4% |
| VTX_10 | — | — | no (infeasible) | — |
| state + stripify (composed) | ~786 | ~26.2% | yes | ~11% |

State-elision dominates. It is the highest-value **safe** cut. Stripify can be
composed on top as an additive second commit (lossless, +5.6%) — but the
state elision alone clears the 10% gate by 2× and is simpler.

## 7. E0 verdict — PROCEED; chosen lever: redundant state-write elision

The spec's E0 STOP condition ("achievable safe cut < ~10% words, or neither
lever is safe") is **not met** — state-write elision is a 20.6% lossless cut.
VTX_10 is dropped (infeasible). Stripify is noted as an optional additive
second commit but is not the primary lever (5.6%, topology-limited, and
requires a corner-table strip reorder with its own correctness surface).

**Mode 1 for E1 = redundant COLOR/TEX_COORD elision at capture time.**

### Implementation seam (for E1)

- **One writer:** `src/nds/nds_renderer.c`. The capture path
  `ndsRendererTask36ReplayCapture` (`nds_renderer.c:~4450`, the recorder that
  builds `owner->words[]`) is where elision lands. When recording a COLOR or
  TEX_COORD word, skip it if its value equals the last recorded value of the
  same class (track a per-owner `last_color` / `last_texcoord`). The replay
  loop (`ndsRendererTask36ReplayRun`, :20314) is unchanged — it just drains a
  shorter buffer.
- **Per-frame cost: zero** — elision happens once at capture (frame 0); every
  subsequent frame replays the smaller buffer.
- **Capacity:** a smaller buffer is fine, but the capture bounds/asserts at
  :4460-4482 must not assume the old word count (they use
  `NDS_TASK36_REPLAY_WORD_CAPACITY` as the ceiling, so they stay correct).
- **Override-trap guard:** thread `NDS_TASK55_STAGE_GEOM` into the tick-HUD
  measurement target or mode 1 is silently ignored (Task 53 hit this) — prove
  the built ROM took mode 1 (preproc/objdump) before trusting numbers.

### Fidelity posture (recorded for E2)

This is a **lossless** render change — the Task 49 differ Tier 1 is expected to
report the elided words as "divergence" (words differ by construction), but
Tier 2 (effective screen-space transform) should be **0.0 px** because the
rendered output is identical (same final color/texcoord per vertex). State
hash EXACT is the hard gate. Owner visual A/B is required by doctrine but is
expected to be indistinguishable (it is literally the same render with fewer
redundant register writes).
