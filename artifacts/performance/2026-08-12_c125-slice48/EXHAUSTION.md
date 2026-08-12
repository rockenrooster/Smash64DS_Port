# c125 — where the tail stands after slice 48, and what each lane can still pay

> **ARM CORRECTION (2026-08-12): every number in this file is a BOUNDARY-arm
> figure, not a both-CPU one.** `build-c124-slice48` was built
> `NDS_R2_BOTH_CPU 0`; the flag is build-time, so this ROM cannot run the
> both-CPU arm the R2-07 gate names. On that arm HEAD measures **1,207,616,
> +87,236 OVER gate**. **Every lane ceiling below is therefore computed on the
> wrong distribution and must be recomputed before any lane is trusted as
> dead.** See `../2026-08-12_c126-armcheck/ARM_MISLABEL.md`. The board warned
> about exactly this failure mode at `P1_EXECUTION_BOARD.md:154`.

Canonical both-CPU gate on `build-c124-slice48` (1600 samples from frame 438,
`-RingDump`, DLDI ON, `NDS_R2_BOTH_CPU=1`, Boundary GREEN, 0 exceptions):

```
WORK-H   P50 900,736   P95 1,087,296   spread 1.21   ROM sha DE80E46BDCF1FD98
```

**The gate is 1,120,380. The ROM measures 32,000-odd under it.** Two runs of this
ROM returned 1,087,296 to the byte, which is the documented bit-determinism.

**Read `SLICE48.md` before quoting that as a win.** `build-c124-bgmprio-create27`
restores the c123 bank's exact behaviour and already measures 1,101,248 against
the bank's 1,196,224, so **~94,976 of the improvement is link placement** and will
move again on any unrelated edit. The gate is met; it is not *stably* met.

## Lane ceilings — the most each lane could ever pay

Method, and it is the only honest one: for each row subtract
`max(0, lane - median(lane))` from that row's `WORK-H`, then re-take the **80th
largest of 1,600**. Medians do NOT add, so this is never computed by subtracting
medians (the trap that invented a 110,336 lane in c122). Baseline under this
convention is 1,089,152 — 1,856 above the harness's percentile method, so treat
the *deltas* as the result, not the absolute.

| lane | ceiling | P95 if the lane were flat | median | nested in `SRC` |
|---|---:|---:|---:|---|
| `SRC` | **133,056** | 956,096 | 281,536 | — |
| `GCRA` | **133,056** | 956,096 | 276,352 | yes |
| `SINT` | 57,280 | 1,031,872 | 126,400 | yes |
| `SHDT` | 38,912 | 1,050,240 | 4,160 | yes |
| `MISC` | 31,680 | 1,057,472 | 104,448 | yes-adjacent |
| `SPHD` | 17,152 | 1,072,000 | 55,296 | yes |
| `AUD` | 13,312 | 1,075,840 | 2,624 | — |
| `SCPU` | 4,288 | 1,084,864 | 25,152 | yes |
| `SPRM` | 2,944 | 1,086,208 | 1,920 | yes |
| `STG` | 2,048 | 1,087,104 | 168,768 | — |
| `SWRM` / `SPHC` | 64 | — | — | yes |
| `SCAT` / `FTR` / `BG` | **0** | 1,089,152 | — | — |

`GCRA` == `SRC` to the tick, so the whole spendable lane is inside `gcRunAll`.
**`FTR` is 0 for the fourth consecutive measurement** at a median of 303,232 — the
renderer is large and perfectly flat, and is not a P95 lever in any configuration
tested. `STG` 2,048 at a median of 168,768 says the same for the stage.

The `HUD` row the script prints (15,104) is an artefact: `WORK-H` already excludes
`HUD`, so capping it is a hypothetical about a quantity that is not in the metric.
Ignore it.

## Candidates measured and closed this cycle

| candidate | verdict | evidence |
|---|---|---|
| BGM packet/buffer resize | **refuted** | `PACKET_BYTES` is a max bound, not the packet size; loop already byte-addressed |
| BGM worker below main *during the match* | **refuted** | same-binary A/B, **+8,064 the wrong way** |
| BGM worker created below main | **KEPT** | −13,952..−17,792, moved the FAT lane off 14 tail frames |
| Reclaim dead ITCM (slice 49) | **refuted, no build spent** | the 87M assumes admitting PORT functions, and `NDS_TASK37_ITCM_PORT` is 0 because the owner found the enabled build **misbehaves**; eviction alone pays nothing |
| `SHDT` reach bound (slice 47) | **refuted** | `WouldSkip 0` over 2,373 tests — geometrically cannot reject |
| Collision transform chain | **not redundancy** | latches already clear once per fighter per frame; the 13–18x is real engagement |
| `ndsRelocAssetIDForToken` chain reorder | **closed** | E11 proved a −7,667 cut still lost P95 +15,744 to relink |

## What is left, honestly

1. **`ndsAObjEvent32NormalizeScript`** — the largest non-idle, non-softfloat row
   (24,299 cyc/frame on 19/80). `--pc-detail` confirms **85.5% of it is two
   linear pointer scans** over the ≤1,024-entry normalized table
   (`0x02065d88 adds r3,#8` at 159,963 iterations, `ldr` at 3.54 and 5.72
   cyc/insn — cache-cold over 8 KB). A pointer-keyed hash is exact and would
   delete nearly all of it. **But the whole function is 0.10% of the window**,
   and 19/80 presence means the rank-80 frame is only ~24% likely to be one of
   its frames — `cluster-where-the-percentile-lives`. Size it against `SINT`'s
   57,280 ceiling before building.
2. **The same symbol is an unowned correctness cliff.**
   `sNdsAObjEvent32NormalizedCount` reaches **973 of 1,024** in one minute;
   overflow takes `Reject(12)` and silently **skips the animation attach**. A
   longer match or the soak will hit it. The hash does not fix the cap — that
   needs capacity, eviction, or dedup, and it is a bug independent of the tail.
3. **Placement is the biggest quantity in the profile and the least controlled.**
   Memory stall is 1,236,685,107 cycles, 33.8% of the match. The campaign has no
   deliberate data-layout lever, and slice 48 showed an accidental one worth
   94,976. This is where real, *stable* margin would come from.
