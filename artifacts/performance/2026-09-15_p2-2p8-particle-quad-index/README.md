# P2-2p8 particle quad first-row index — KEEP

Date: 2026-09-15

## Why this cut exists

The post-pose ARM9 profile put `lbParticleDrawTextures` among the remaining hot
symbols. Its hottest PCs landed in `ndsParticleQuadFrameFor()`: every particle
lookup restarted a linear scan at row zero even though the generated atlas has
only 47 rows and is already sorted by `(texture_id, frame)`. The measured
1400..1527 window executed about 26,454 iterations of that compare loop.

The generator now emits a 256-byte `u8 texture_id -> first row` directory. The
runtime jumps directly to the selected texture's first row and preserves the
old nearest-earlier-frame rule within that texture. `0xff` means the texture is
not present in the atlas.

## Correctness

The producer rejects a frame table too large for the u8 row index and proves
the old scan and indexed lookup equivalent for every runtime input pair:
**256 texture keys x 256 frame keys = 65,536 cases**. Generation aborts on the
first mismatch. `check-nds-particle-banks.ps1` additionally pins the generated
directory contract and the runtime consumer.

The ordinary particle-bank checker remains GREEN:

- 110/119 reachable scripts;
- 41/47 reachable texture IDs packed;
- 42/46 quad textures in 47 atlas frame rows;
- generated source/table checksums unchanged (`a2a1e85f` / `0badfd59`).

## Same-ROM timing evidence

The measurement-only route retained the historical scan in a cold/noinline
helper so route selection did not put both loops in the hot function. Both arms
of each pair used the same ROM and natural four-CPU workload. The route and its
counters were removed before the shipping build.

### First same-ROM build — `5B4D52FA...22E4`

Frames 1400..1527 (128 samples):

- WORK-H P50/P95: **1,710,528/2,445,632 -> 1,713,024/2,390,464**
- paired WORK-H: **81 wins / 1 tie / 46 losses**, median **-1,600**, mean
  **-1,893 ticks/frame**
- SRC P95: **1,060,992 -> 1,033,280**
- GCRA P95: **1,055,296 -> 1,027,328**
- semantic witnesses are identical: 8,533 quad emits, 54 misses, 2,606
  strides, 8,699 visible draws, 8,533 submits, 112 submit failures; pose work
  is identical as well.

Whole 1,972-sample match:

- WORK-H P50/P95: **1,640,320/2,380,032 -> 1,640,320/2,376,576**
- paired WORK-H: **1,116 wins / 88 ties / 768 losses**, median **-640**, mean
  **-695 ticks/frame**
- SRC P95: **1,068,736 -> 1,066,368**
- 5+ VBlank presents: **220 -> 216**
- semantic witnesses remain identical: 11,849 emits, 78 misses, 3,789
  strides, 12,067 visible, 11,849 submits, 140 submit failures.

### Independent rebuilt same-ROM pair — `81DE617D...A88928`

Frames 1400..1527:

- control engagement: **6,093 scan lookups**, 1 setup/index lookup;
- candidate engagement: **6,094 indexed lookups**, 0 scan lookups;
- WORK-H P50/P95: **1,710,144/2,388,544 -> 1,709,312/2,390,272**;
- paired WORK-H: **96 wins / 0 ties / 32 losses**, median **-2,048**, mean
  **-2,183 ticks/frame**;
- WORK P50/P95: **1,756,800/2,514,240 -> 1,754,432/2,507,072**.

The isolated P95 sign is not stable across the two 128-frame runs (+1,728 on
the rebuild versus -55,168 on the first build), so this is **not** claimed as a
large tail-P95 cut. The KEEP decision is the reproducible paired per-frame
median/mean saving, the whole-match paired improvement, the tiny fixed 256-byte
precompute, and exhaustive semantic equivalence.

## Final hard-on checkpoint

The measurement route is gone. Final standing-stress ROM:

`FED52999008D6EFB9DAC8412E4C1479A79FAF5D1F53C7D1CB91328E1F7FE607F`

Natural 59/60-second four-CPU coverage, frames 2..1973 (1,972 samples):

- WORK-H P50/P95: **1,635,712 / 2,377,152**
- SRC P95: **1,066,496**
- GCRA P95: **1,058,496**
- SINT P95: **600,896**
- VBlank 2/3/4/5+: **118 / 838 / 797 / 220**, cadence violations **0**
- native failures/direct rejects: **0 / 0**
- pose bind/full/track-overflow/run-mask-fallback: **677/0/0/0**
- BPS1 payload reads/misses/failures: **676/5/0**
- BPS1 directory bytes/hits/fallback/failure: **9320/681/0/0**
- BGM direct/fallback: **329/0**
- FGM direct/fallback/stdio: **329/0/0**
- graphics heap overflow/no-room: **0/0**
- weapon pool refusals: **0**
- general heap low-water: **111,680 B**, **86,080 B** above the safety floor.

P2-2p8 remains RED against the product target. This checkpoint removes a
repeated generated-table search; it does not close the much larger remaining
whole-frame/SRC cost.
