# Stage GX segment fast path, 2026-10-04

`ndsStageGxCommitFast` (`src/nds/nds_stage_gx.exec.inc`) commits a compiled
stage segment in one pass over its patches and one DMA, instead of
`ndsStageGxDraw` once per run with a span flush at every alpha edge. It takes a
segment whose runs are contiguous in words and patches, with no hidden binding,
no 1P cull and no run near the near plane (one union box per binding); anything
else, or a mid-pass decline (with the painter depth restored), goes to the
per-run loop, which then forgets the segment's freshness proof. Same-ROM A/B
word `gNdsStageGxFast` (default 1).

## Equivalence (lab four-CPU ROM, gate configuration)

`stghash.ps1` hashes every word the stage program sends to the FIFO, the span
state sequence, and each segment's closing painter depth and counters, per logic
frame (lab-only `gNdsLabStageGxHash` / `gNdsLabStageGxStateHash`):

| arm | logic frames | identical | fast commits | declines |
|---|---:|---:|---:|---:|
| `gNdsStageGxFast=0` vs `=1` | 901 (600..2400) | **901** | 9,600 | 0 |

`stage-hash-fast0.txt`, `stage-hash-fast1.txt`, compared by `hashcmp.py`.

## Gate (official target, same ROM `build-gate-1004b`, ring dump, 1,960 samples)

| | WORK P50 | WORK P95 | > 1.12M | STG P50 | two-VBlank |
|---|---:|---:|---:|---:|---:|
| `gNdsStageGxFast=0` | 965,440 | 1,327,872 | 392 | 170,560 | 1,556 / 1,961 |
| `gNdsStageGxFast=1` | **948,288** | **1,316,224** | **354** | **154,688** | **1,597 / 1,961** |

Replay digest (DGSA/DGSB) identical on all 1,960 rows. `gate-fast0.*`,
`gate-fast1.*`. HEAD before the change (`build-gate-1004a`, `../2026-10-04_gate-head`):
960,064 / 1,324,672, 384 over, STG P50 166,656 -- the new ROM's off arm reads
~4K higher on STG from layout alone.

The lab arms (`gateab.ps1`, per-frame stops, lab hash cost in both) read
STG P50 247,232 -> 228,096 and WORK P50 1,095,488 -> 1,079,936.

## Build note

`ndsRendererNativeStageBeginRun` is now `always_inline`: GCC stopped inlining
its single call when the commit function grew, and the out-of-line copy (tagged
`.itcm`, 2,000 B) overflowed ITCM on the lab link.
