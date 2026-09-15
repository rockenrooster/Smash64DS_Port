# P2-2p8 phase I — fighter pose live-track mask KEEP

The compact fighter pose player used to scan all ten possible animation-track
slots on every evaluated joint, then reject absent slots and `kind == None`.
The retained path keeps a 10-bit `eval_mask` per joint. A bit becomes live only
when the source AObj16 script assigns Step, Linear or Cubic state; TraI's
interpolation descriptor may allocate a `kind None` slot and deliberately does
not join the mask. The authored script, Q-form evaluator and output stores are
unchanged.

## Correctness oracle

The route was first run against the generic source player in the existing pose
oracle build. It completed **190,062 field comparisons**, with **0 value
mismatches** and **0 pose mismatches**. The run also recorded zero pose-track
overflow. See `pose-oracle.txt`.

## Same-ROM timing A/B

Measurement ROM SHA-256:
`F543D29B3B7AE39FA74573B2671D8EFB1B5DC7E73C655C8352463E62539350BC`.
The temporary route cell selected either the historical ten-slot scan or the
live-bit walk without changing the binary.

Frames 1400..1527:

| metric | scan control | live mask | delta |
|---|---:|---:|---:|
| WORK-H P50 | 1,713,728 | 1,705,792 | **-7,936** |
| WORK-H P95 | 2,437,824 | 2,411,200 | **-26,624** |
| SRC P95 | 1,055,040 | 1,036,224 | **-18,816** |
| SINT P95 | 526,528 | 520,960 | **-5,568** |

Paired WORK-H improves on 86/128 frames (median **-2,976**, mean **-3,813**);
SRC improves on 113/128 and SINT on 125/128. Both arms perform exactly 504
binds, 11,800 pose updates, 104,345 joint evaluations, 425,926 track
evaluations and 43,745 script steps, with zero bind or track overflow.

Whole one-minute four-CPU match, frames 2..1973:

| metric | scan control | live mask | delta |
|---|---:|---:|---:|
| WORK-H P50 | 1,641,984 | 1,641,792 | **-192** |
| WORK-H P95 | 2,370,432 | 2,353,088 | **-17,344** |
| SRC P95 | 1,067,776 | 1,062,912 | **-4,864** |
| SINT P95 | 601,344 | 595,584 | **-5,760** |

Paired WORK-H improves on **1,526/1,972** frames (median **-3,200**, mean
**-3,478**); SRC improves on **1,811/1,972** and SINT on **1,907/1,972**.
Both arms again have identical authored work: 677 binds, 15,345 updates,
137,269 joint evaluations, 561,141 track evaluations, 58,534 script steps and
zero overflow.

## Final hard-on checkpoint

The temporary route cell and historical scan branch were removed. Final ROM
SHA-256 is
`DF8C031D1B5480A5C2C4560DAFD1C4687F33A338793A213A330903FDA6FC473A`.
The standing one-minute verifier passes on that ROM with:

- WORK-H P50/P95 **1,645,760 / 2,348,160**;
- SRC P95 **1,057,920**, SINT P95 **593,536**;
- pose binds/full/track-overflow **677/0/0**;
- BPS1 reads/misses/failures **676/5/0**, directory **9320/681/0/0**;
- BGM direct/fallback **329/0**, FGM direct/fallback/stdio **329/0/0**;
- native failures/direct rejects **0/0**;
- general-heap low-water **111,680 B**;
- VBlank 2/3/4/5+ histogram **114/827/810/222**.

`verify-p2-four-fighter-stress.ps1` now permanently asserts pose-track overflow
is zero, because a scratch-only overflow track cannot be represented by the
persistent mask. P2-2p8 remains RED against the 1.12M-tick / >=95% two-VBlank
product gate; this is a retained incremental CPU cut, not milestone closure.

The shell-driven Mario/Fox route was rebuilt from the same hard-on source after
the performance proof. `smash64ds-p2-shell-hwtri.nds` SHA-256
`A02834D0F1D246826DA844B05C24BAB69C5621AB3ED2974ABCF5127E8FBCA193`
passes the natural Pupupu realtime smoke, native-only link checks, required
texture-detail regions and the stable live capture. The Boundary wrapper still
stops before runtime on the unrelated owner `decomp/alt_assets/` architecture
rule; the direct registered runtime verifier is therefore the scoped shell
acceptance for this slice.
