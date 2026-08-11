# Slice 43 gate — real, fidelity-neutral, and a third of the predicted size

Cycle 119. Two ROMs from one tree, `NDS_R2_BOTH_CPU=1`, 1,600 presented frames
from 438, stride 96, DLDI ON. `builds/build-c119-gxctl`
(`NDS_R2_FIGHTER_GX_COMPOSE=0`) against `builds/build-c119-gxcompose` (=1).

## The control reproduces the banked gate to the tick

`WORK-H` P50 **958,592** / P95 **1,258,112** — identical to the banked c119
figure. A matched control that lands exactly on the baseline is the strongest
available evidence that the tree is clean and that the delta below belongs to
the flag ([[prove-the-control-differs]] in the other direction).

| bucket | control P50 / P95 | candidate P50 / P95 | ΔP50 | ΔP95 |
|---|---|---|---:|---:|
| **`FTR`** (owning bucket) | 302,848 / 306,368 | 296,832 / 300,288 | **−6,016** | **−6,080** |
| `WORK-H` | 958,592 / 1,258,112 | 953,856 / 1,249,088 | −4,736 | **−9,024** |
| `STG` | 190,848 / 198,272 | 189,760 / 197,248 | −1,088 | −1,024 |
| `SRC` | 330,688 / 589,632 | 329,280 / 587,712 | −1,408 | −1,920 |
| `SINT` | 149,760 / 344,576 | 148,672 / 345,792 | −1,088 | +1,216 |
| `ALL` | 1,118,336 / 1,678,912 | 1,118,208 / 1,678,720 | −128 | −192 |

Both arms: `gNdsBattleTextHudP0Damage/P1Damage` **130/51**, `slips=0`, 2,038
frames, same coverage. **The arms played the same match**, so unlike slice 41
this delta is a cost delta ([[route-ab-cannot-price-gameplay-change]]).

## Engagement, whole match

`gNdsR2GxComposeCaptures` **63,218** against `gNdsR2GxComposeRoots` **63,218** —
ratio **1.000** — and `Declines` **0** over 2,038 frames. 110,063 multiplies,
1.74 per binding. The geometry engine does every joint compose; the CPU does
none.

## The verdict, and it is not KEEP yet

**`FTR` moved −6,016 at P50 and −6,080 at P95.** The same figure at both
percentiles is the signature of a flat deletion, and at spread 1.01 `FTR` is the
trustworthy number here; `WORK-H` P95 −9,024 is that −6,000 plus noise, and
`WORK-H` P50 −4,736 sits **under** the ±8,544 floor. One arm barely over the
floor and one arm under it is not a bankable reading.

Not a revert either: proven engaged on every owner, frame-locked pixel-identical
against the control at match tic 3000 (`artifacts/visibility/2026-08-11_gx-compose`),
and it deletes 52 CPU 4x4 multiplies a frame.

## Why the prediction was 3.4x too big, and what it costs to know

Predicted −20,700 at **12.2 cycles per FIFO word**, derived from R2-03 E23
(29 identical 17-word projection pushes skipped for −3,008). Measured counts give
1,377 new words a frame against 544 removed, so +833 net; a −6,016 result puts
the word at **~24 cycles**. E23's write went into an idle geometry engine. These
compete with vertex traffic, and a full FIFO stalls the CPU.

**A FIFO word is not a fixed price — it is priced by what else is in the
queue.** Any future "move it to the GX" estimate has to say which engine state
the measurement came from.

## The scale multiply cannot be made cheaper, and here is why

The 17-word `diag(1,1,1,s)` after each chain is 544 words a frame, the single
largest new cost. Three ways out, all closed:

- **`MTX_SCALE` is 4 words** but scales rows 0..2, and `GL_MODELVIEW` is DS
  matrix mode 2, so it scales the vector matrix too and breaks
  `NDS_R2_FIGHTER_HW_LIGHT`. Expressing the row-3 scale as a uniform scale that
  cancels in the perspective divide has the identical problem.
- **Folding it into the factors** fails on row-vector algebra: for affine `L`,
  `L·V` scales column 3 and `V·L` scales row 3, so `V` does not commute past a
  chain factor.
- **Moving it to the seed** fails because the seed is the rightmost factor and
  right multiplication scales a COLUMN.
