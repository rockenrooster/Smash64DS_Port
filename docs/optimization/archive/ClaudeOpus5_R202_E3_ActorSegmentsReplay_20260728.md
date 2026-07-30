# R2-02 E3 — Whispy and the flowers were never dynamic, and the mask never learned

> ## RETRACTED 2026-07-28. VERDICT IS **HOLD, NOT KEEP**. THE FLOWERS BREAK.
>
> A synchronized crop of the flower band against the default ROM shows **both
> flower beds destroyed** with `NDS_R2_STAGE_ACTORS=1`: `flowers_back` and
> `flowers_front` collapse into a thin smear of specks floating across the tree
> trunk, and the front row along the dirt path is gone entirely. Those are two
> of the four segments this cut admitted. Whispy's blossoms are fine — they
> belong to the tree layers, which were already replaying.
>
> **§4's "stage visually intact ... no tearing, no dropped or duplicated
> geometry" was wrong.** It was written from the candidate screenshot alone,
> checking that elements were *present*, instead of diffing the changed subset
> against the control. That is precisely the failure the
> *measure-the-subset-that-changed* rule exists to prevent, and it was made by
> the same task that had just written a new rule about not trusting aggregates.
>
> **What the automated gates missed and why.** Boundary passed and
> required-region detail read 62.681% against the default's 62.750% — 7 pixels
> in 7,200 — because the flower beds are not inside the required region. The
> 1,828-frame invariance proof in §4 is still sound on its own terms: the
> *prepared vertex data* really is constant. It does not cover what broke, which
> from the symptom (geometry collapsed to a line) is matrix or matrix-stack
> state, not vertex data.
>
> **Cause established 2026-07-28 by E4** — see
> `ClaudeOpus5_R202_E4_ActorSegmentsRefuted_20260728.md`. It is not the
> `Task51EnsureWorld` pop/push pairing this header first guessed;
> `NDS_TASK51_STAGE_NATIVE` defaults to `0` and is compiled out of every build,
> so that function never ran. The actual cause is that a **dynamic** binding's
> captured stream is a `MATRIX_LOAD4x4` per triangle of
> projection × view × model. Replaying it pins that geometry to the camera the
> capture frame happened to have, which is why the flowers sit in a fixed screen
> band no matter where the camera goes. The replay segment mask has to name
> exactly the segments whose bindings are all rigid; E3 broke that invariant.
>
> **Nothing shipped.** `NDS_R2_STAGE_ACTORS` has since been deleted, the
> published ROMs are at defaults and Boundary-green at 62.750%. **R2-02's gate
> is therefore NOT met** — without E3, `STG` P50 is 224,320 against the 180K
> budget — and R2-03 was started prematurely on the strength of this file.
>
> Everything below is the original text, kept per the never-rename rule. The
> measurements are real; the verdict was not earned.

**Date:** 2026-07-28
**Phase:** R2-02 (`Smash64DS_Runtime2_SwitchPlan.md` §7), third landed cut — the
one that closes the gate.
**Flag:** `NDS_R2_STAGE_ACTORS` (default `0`), measured with
`NDS_R2_STAGE_DIRECT=1 NDS_R2_STAGE_DMA=1` held on **both** arms.
**Verdict: KEEP.** `STG` P50 **224,320 → 173,120** — **inside the 180K gate** —
down on **128 of 128 frames**, and 2-VBlank frames go **12 → 235 of 566**.

---

## 1. The E2 report predicted the wrong cut, and the census said so

E2 closed by naming E3: fold `ndsRendererNativeStageBeginRun`'s GX state writes
into the capture, collapse 21 DMA setups into one, worth "roughly 40,000". That
was reasoned from the R2-00b symbol census and it was wrong on both halves.

A Task 103 run on the current binary measured the replay path directly:
`BeginRun` inside `ndsRendererTask36ReplayRun` is **15,916 ticks/frame across 33
runs**, the DMA is 10,644, the tail 3,640 — the whole replay is **30,200** and
there was never 40,000 in it to take.

The same run found where the stage's time actually is. `NDS_TASK36_REPLAY_SEGMENT_MASK`
admits three of the eight segments; the other five take the generic path, and
per-segment counters priced them:

| segment | ticks/frame | runs | triangles | ticks/triangle |
|---|---:|---:|---:|---:|
| whispy_eyes | 6,165 | 1 | 4 | 1,541 |
| whispy_mouth | 10,628 | 4 | 8 | 1,329 |
| flowers_back | 10,012 | 4 | 6 | 1,669 |
| flowers_front | 18,544 | 6 | 9 | 2,060 |
| **actor subtotal** | **45,349** | **15** | **27** | **1,680** |
| layer1 | 22,738 | 6 | 76 | 299 |
| **generic total** | **68,547** | **21** | **103** | |

**Twenty-seven triangles were costing 1,680 ticks each.** Whispy's eyes and
mouth and two flower beds — 8% of the stage's geometry — were 20% of the entire
stage bucket. The replay path next to them runs 99 triangles for 30,200.

The per-triangle price is structural, not accidental: a `PROJECTED_NO_Z` run
rebuilds and reloads a full 4×4 no-Z projection **per triangle**
(`ndsRendererNativeStageTask36LoadNoZProjection`), and `BeginRun` re-runs its
matrix and state head **per run** — 25,703 ticks/frame across the 21 generic
runs, measured separately. Neither cost scales with triangles, which is exactly
why four runs carrying six triangles cost as much as the whole rigid replay.

## 2. What changed: one stale constant

Nothing about the emit path was rewritten. The mask was widened.

`NDS_TASK36_REPLAY_SEGMENT_MASK` names the segments whose bindings appear in
`NDS_RENDERER_TASK36_RIGID_BINDING_MASK`, and the correspondence is exact:

| segment | owner | bindings | rigid | replayed |
|---|---|---|---|---|
| 0 layer0 | 0 | 0–19 | yes | yes |
| 1 whispy_eyes | 4 | 20 | no | **now** |
| 2 whispy_mouth | 5 | 21–24 | no | **now** |
| 3 flowers_back | 6 | 25–28 | no | **now** |
| 4 layer1 | 1 | 29 | no | no |
| 5 layer2 | 2 | 30–32 | yes | yes |
| 6 flowers_front | 7 | 33–38 | no | **now** |
| 7 layer3 | 3 | 39–41 | yes | yes |

That was the right rule when it was written. **Task 51 made it stale.**
`ndsRendererNativeStageTask51EnsureWorld` replaced the per-frame CPU compose for
the *non*-rigid bindings with a `MULT4x3` of
`sNdsNativeStageBakedWorldMatrices[]` — a generated constant table. Since Task
51 landed, those bindings have composed nothing per frame; the port has been
drawing Whispy and the flowers from a baked matrix and paying the dynamic path
for the privilege. Nobody widened the mask, and the two masks are named for the
same thing, so nothing pointed at the gap.

`NDS_TASK36_REPLAY_WORD_CAPACITY` goes 4,608 → 6,144 to hold the extra stream.

**layer1 (segment 4) is deliberately still excluded.** Its six runs submit
through submit classes 0 and 6 — the raw composed matrix, `binding_composed[29]`
— which is the camera composition and genuinely moves. Freezing it would nail
the layer to the camera. Moving layer1 onto the segment-bracket path is
generator work and is the next R2-02 cut if one is wanted; it is 22,738
ticks/frame and the gate no longer needs it.

### 2.1 A latent defect the widening exposed

`ndsRendererTask36ReplayOpcode` has no case for `NDS_TASK29_GX_MATRIX_MULT4x3`,
so `ndsRendererTask36ReplayRecord` **silently dropped it**. Task 51 added that
class for the Task 49 differ and did not add it here, and the only sites that
emit it were outside the replay mask — so the omission was invisible until the
mask moved. Widening the mask without noticing would have baked a stream with no
world matrix in it and drawn Whispy at the origin.

Dropping an unencoded class is correct for every *other* class in the enum:
they are state (DISP3DCNT, texture bind and params, poly format, alpha test)
that `ndsRendererNativeStageBeginRun` re-issues live at replay, so recording it
would only duplicate a write. A matrix class is not in that set. The recorder
now **faults the capture** on an unencoded matrix class instead of dropping it,
so the next class appended to the enum cannot repeat this silently.

## 3. Measurement

One tree, two builds, `NDS_R2_STAGE_DIRECT=1 NDS_R2_STAGE_DMA=1` on both,
`NDS_R2_STAGE_ACTORS` off versus on. 128-frame ring dump, frames 439–566,
melonDS-Accurate `DE80E46B…`, git `b62e397028`.

| bucket | off P50 | on P50 | ΔP50 | off P95 | on P95 | ΔP95 |
|---|---|---|---|---|---|---|
| **`STG`** | **224,320** | **173,120** | **−51,200** | **232,640** | **181,248** | **−51,392** |
| `WORK` | 1,206,528 | 1,152,000 | −54,528 | 1,667,200 | 1,624,384 | −42,816 |
| `WAIT` | 469,568 | 189,376 | −280,192 | 542,080 | 547,712 | +5,632 |
| `MISC` | 47,680 | 46,464 | −1,216 | 164,416 | 164,096 | −320 |
| `FTR` | 547,840 | 547,456 | −384 | 996,800 | 999,104 | +2,304 |
| `SRC` | 324,544 | 324,352 | −192 | 678,400 | 677,696 | −704 |
| `BG` / `AUD` / `HUD` | — | — | 0 | — | — | ≤ +2,304 |

**Paired per-frame, matched by ring slot:**

| bucket | frames down | frames up | median Δ |
|---|---|---|---|
| **`STG`** | **128** | **0** | **−51,296** |
| `WORK` | 114 | 14 | −52,448 |
| `MISC` | 112 | 12 | −1,056 |
| `FTR` | 59 | 68 | +224 |
| `SRC` | 68 | 59 | −96 |
| `BG` / `AUD` / `HUD` | — | — | 0 |

`STG` down on every frame at 8× the 5,000–7,000 placement floor, with every
other bucket's median at or within noise of zero. The `WAIT` collapse is the
saving showing up as idle time, not a second mechanism.

**VBlank interval histogram** — the metric §4 names as the point of the switch:

| | 2 | 3 | 4 | 5+ | max | slips |
|---|---|---|---|---|---|---|
| off | **12** | 540 | 9 | 5 | 18 | 0 |
| on | **235** | 316 | 12 | 3 | 18 | 0 |

**235 of 566 presented frames now land in two VBlanks** — 41.5%, against 2.1%
before. E2 moved twelve frames across that line; E3 moved two hundred and
twenty-three. On the shorter 32-frame window the median frame is a 2-VBlank
frame outright (`ALL` P50 1,680,000 → 1,120,128). The shipping ROM's own
telemetry HUD reads **FPS 28.9 / UP 57.9** on the E3 Boundary capture — a
half-second average, so corroboration rather than evidence, but it is the number
the histogram predicts.

Realized 51,200 against the 45,349 the actor segments cost on the generic path.
The extra ~5,900 is owner-side: those four segments now take E1a's prepared-
segment reuse instead of `InitStats` + `ApplyStateSpan` + `PrepareRun`.

**Engagement**, read from the same runs that produced the buckets:

| | segment mask | captured words | capture outcome |
|---|---|---|---|
| off | `0xa1` (0, 5, 7) | 3,916 | `READY` |
| on | `0xef` (0,1,2,3,5,6,7) | 5,597 | `READY` |

`READY` on the candidate is the part that matters: the capture completed with
all seven segments, no `capture_fault`, and 547 words of headroom under the
raised capacity.

## 4. Correctness

E3 bakes fifteen runs' command streams once and replays them for the rest of the
match. That is only sound if nothing those runs emit changes between frames.
Their four inputs:

1. **World matrix** — `sNdsNativeStageBakedWorldMatrices[]`, a generated
   constant, since Task 51. Settled by construction.
2. **`frame->projection` and the projected-depth sequence** — layer0/2/3 have
   been replaying their own per-triangle no-Z projection loads correctly for
   many tasks, which is a standing proof that neither moves in this Boundary.
3. **Draw order** — layer1 stays generic with a fixed 76 triangles, so the
   depth counter reaches each actor run at the same value every frame.
4. **Prepared dense data** — the packed colours and texture coordinates the
   material state produces. Whispy has a material animation, so this one needed
   an answer rather than an argument.

`NDS_R2_STAGE_ACTORS_PROOF` (lab, default 0, built with `ACTORS=0` so the
prepare path still runs) hashes exactly the prepared-run and prepared-dense data
those fifteen runs consume, once a frame, and counts the frames the hash differs
from the previous one.

```text
gNdsR2ActorPreparedFrameCount = 1,828
gNdsR2ActorPreparedChangeCount =     0
gNdsR2ActorPreparedHash       = 0xC01BA0B6
```

**1,828 presented frames — the whole one-minute match — and the hash never
moved once.** 1,827 opportunities to differ, zero taken. Whispy's material
animation does not reach the prepared colours or texture coordinates, so what
E3 bakes was already a constant that the port was recomputing 60 times a second.
That is a falsifier that ran and did not fire, not an argument that it would
not.

Other gates:

- **Boundary green** on the candidate configuration (mode 163, one-minute), ROM
  built `NDS_R2_STAGE_DIRECT=1 NDS_R2_STAGE_DMA=1 NDS_R2_STAGE_ACTORS=1`.
  "Boundary verification profile passed."
- Required-region detail **62.681%** against the default's 62.778% and E2's
  62.542% — 7 pixels in 7,200, and the captures land on different game frames
  because the candidate runs faster.
- Every named region reports 100% non-clear: `left_bush`, `right_bush`, `fox`,
  `mario`, `stage_body`. Horizontal-detail flat-run limits pass on `left_bush`,
  `stage_body` and `pond`.
- Screenshot `artifacts/visibility/r2-02-e3-actors-on-boundary-20260728.png`:
  canopy, trunks, both flowering bushes, the flower row, the fence, the pond,
  both platforms, Mario and Fox all present and correct. The flower beds and
  Whispy's face are precisely what this cut froze, and they render.
- 0 cadence violations, `vbiTotal` 566 on both arms.

**Owner visual approval is still owed.** `AGENTS.md` makes the owner the visual
oracle for render-side change, and the honest statement of residual risk is:
the hash proves the *prepared data* is constant over the sampled window, and the
screenshot proves the geometry is present, but neither watches Whispy through a
full wind cycle with human eyes.

## 5. Where R2-02 stands against its gate

```text
STG P50   351,488  baseline
          256,704  after E1a  (-94,784)  prepare-run elision
          225,792  after E2   (-30,912)  GXFIFO DMA for the rigid replay
          173,120  after E3   (-51,200)  actor segments admitted to the replay
          180,000  §7 gate                          <-- MET, 6,880 under
```

Total 178,368 ticks/frame, **50.7% of the stage bucket**, across three cuts that
between them add one flag, widen one constant, and delete no capability.

`STG` P95 is **181,248**, 1,248 over the same 180K figure — 0.7%, and well
inside the 5,000–7,000 build-placement floor Task 100 established. §7 asks for
"P50/P95 within budget (provisional 180K)"; P50 is 6,880 under and P95 is inside
the noise of the line. **R2-02's gate is met and R2-03 (Fighter direct draw) is
unblocked.**

What is left in the stage, if anyone comes back for it:

- **layer1, 22,738 ticks/frame for 76 triangles.** Needs the generator to move
  its six runs off submit classes 0/6 onto the segment-bracket path so the
  camera stays outside the captured stream. Worth ~19,000.
- `BeginRun` in the replay path, 15,916/frame across 48 runs. The state it
  writes (poly format, alpha test, texture params, palette base) is all
  expressible as FIFO commands and could join the stream; DISP3DCNT cannot, and
  would still need a per-run poke.
- The per-triangle no-Z projection rebuild, for whatever geometry stays generic.
  Two 64-byte struct copies and a 4×4 conversion per triangle, when only column
  2 changes.

## 6. Evidence

| SHA-256 (first 16) | file |
|---|---|
| `7F7A185DE4B67ED8` | ROM, `NDS_R2_STAGE_ACTORS=0` |
| `42EB8B0798B9D34B` | ROM, `NDS_R2_STAGE_ACTORS=1` |
| `2D10FE35E9096B33` | `artifacts/performance/r2-02-e3-off-438.json` |
| `3A9ECB2C60FEA571` | `artifacts/performance/r2-02-e3-on-438.json` |
| `75104410AFEADD1D` | `artifacts/performance/r2-02-e3-actor-invariance-1700.json` |
| `D981FCA2117D99B8` | `artifacts/performance/r2-02-e3-segment-census-438.json` |
| — | `artifacts/visibility/r2-02-e3-actors-on-boundary-20260728.png` |
