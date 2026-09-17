# The 4 KB data cache is worth 1,394,560 tk/fr — and data locality is the first lever whose ceiling EXCEEDS the gap

One constant, one build, one match. `NDS_LAB_NO_DCACHE=1` clears CP15 c1 bit 2
after a full clean, disabling the ARM9 data cache for the whole run. Everything
else is identical, and every divergence witness matches the control —
`gNdsFighterDLAllDrawP0/P1HardwareTriangleCount` 349,031 / 333,618,
`gNdsFtrPlanHit` 6,217, `gNdsGCDrawsActiveMax` 203 — so this is the same
workload.

## Result

| bucket P50 | dcache ON | dcache OFF | ratio |
|---|---:|---:|---:|
| **WORK-H** | **1,588,928** | **2,983,488** | **1.88x** |
| ALL | 1,677,952 | 3,356,672 | 2.00x |
| STG | 344,768 | 828,800 | 2.40x |
| SRC | 555,584 | 983,296 | 1.77x |
| FTR | 355,712 | 609,024 | 1.71x |
| MISC | 249,472 | 398,400 | 1.60x |
| WAIT | 252,928 | 267,584 | 1.06x |

**The 4 KB data cache is currently worth 1,394,560 tk/fr.** It is by a wide
margin the most valuable single component in the system — larger than the entire
gap, larger than any bucket, larger than every optimization this campaign has
attempted put together.

## What it means, and how it overturns "compulsory"

The renderer-streaming sizing concluded that the frame's data traffic is
**compulsory first-touch** and predicted that, if so, the no-cache delta would
merely track word count times main-RAM latency.

It does not. Compulsory traffic would make the cache worth only the difference
between one 23-cycle line fill and its 8 constituent uncached word reads — about
57 cycles per line, or roughly 550,000 tk/fr at the measured 9,644 fills/frame.
**The measured delta is 1,394,560, about 2.5x that.** The cache is servicing a
large volume of genuine hits on resident data, so substantial reuse exists.

That does not make the earlier finding worthless — the renderer's *own* traffic
really is fetched-less-than-used, and its 141.9 KB against 202 KB stands. It
means the reuse lives elsewhere in the frame, and that the "nothing to recover"
conclusion was drawn from one bucket and over-generalised to the whole frame.

## The number that changes the campaign

The cache already captures **71.3%** of the available benefit. The residual —
what a *perfect* data cache would additionally save — is exactly the measured
data stall, **560,739 tk/fr**.

| | tk/fr |
|---|---:|
| WORK-H P50 today | 1,588,928 |
| less all data-miss stall | -560,739 |
| **WORK-H with perfect data locality** | **1,028,189** |
| **gate** | **1,120,000** |
| **margin** | **under by 91,811** |

**Perfect data locality clears the gate with 91,811 ticks to spare.**

Data-locality work therefore has a ceiling of **560,739 = 113% of the 496,382
gap**. Every other class measured in this campaign had a ceiling far below it:

| class | ceiling | % of gap |
|---|---:|---:|
| **data locality** | **560,739** | **113%** |
| leaf levers (spent) | ~90,000 | 18% |
| renderer streaming repack | 78,000 | 15.7% |
| SRC bound pose/transform domain | 70,000 | 14.1% |
| `DObj`/`GObj` repack (blocked) | 40,000 | 8.1% |
| DTCM placement | 15,040 | 3.0% |
| per-fighter geometry / joints | 12,144 | 2.4% |

This is the first lever in the campaign whose ceiling exceeds the requirement.

## What this does and does not license

**It does not** say the gate is reachable. 100% data-cache hit rate is not
achievable, and the residual is spread over the whole frame rather than
concentrated: 254,344 renderer, 158,777 diffuse, 84,304 object graph, 53,931
pose, 39,887 bulk copy, 27,311 I/O, 25,774 collision. Capturing even half of it
needs a broad change in how data is laid out, not one fix.

**It does** mean that data locality is the only remaining class worth spending
builds on, and that the campaign's arithmetic — "no combination of leaf levers
reaches the gate" — was true but was measuring the wrong axis. The axis that
matters is hit rate, and the DS gives three instruments the campaign has barely
used:

- **VRAM as scattered-data memory.** A nonsequential 32-bit VRAM read is **5 bus
  cycles against main RAM's 10**, and an uncached VRAM word costs 5 where a
  main-RAM line fill costs 23 to deliver 4 useful bytes. Banks are 128 KiB each
  against DTCM's 1,992 free bytes. Currently A/B are textures and C/D are main
  BG (`src/nds/nds_platform.c:442-462`), so this is a trade, not free space.
- **Sequentiality.** Sequential 32-bit main-RAM reads are **2 cycles against 10**
  nonsequential — a 5x difference on identical bytes. Address-ordered traversal
  is worth far more than the packing work already priced.
- **MPU regions 2 and 3 are free** (region table at 0x02001380), which is what
  mapping a bank as uncached scattered-data memory would need.

## Method note

`crt0SetupMPU` lives in calico, not this tree, so the cache is disabled from our
own `main()` before `ndsPlatformInit`. The routine is `target("arm")` because
CP15 coprocessor transfers have no Thumb encoding and this build is `-mthumb`;
the first attempt failed to assemble for exactly that reason. `DC_FlushAll()`
runs first, or dirty lines never reach memory.

The arm is lab-only and reverted; `NDS_LAB_NO_DCACHE` defaults to 0 and compiles
the body away entirely.
