# Task 72 — One open per load: −94,464 on `SRC` P95

**Date:** 2026-07-26
**Status:** KEEP. Boundary verifier green (`artifacts/task72-verify-boundary.log`,
"Boundary verification profile passed", exit 0), including the
`battle_playable_realtime` harness that exercises the changed load path.
**Inputs:** `artifacts/task72-oneopen.json` against
`artifacts/task69-ring-task66rom.json` — same instrument (`-RingDump`), same
128-frame window, same plain tick-HUD configuration.

Task 71 found that the frames setting the `WORK-H` P95 are loading fighter
animations off the cartridge inside the frame that needs the move, and that
`strncasecmp` — the NitroFS directory walk behind every open-by-path — was the
second largest riser on such a frame at 30,484 ticks/frame.

## 1. The redundancy

The on-demand animation load ran:

```c
if (ndsRelocAssetReadHeader(asset_id, &header) == FALSE) goto fail;
if (ndsRelocAssetLoadData(asset_id, heap, asset_size, &header) == FALSE) goto fail;
```

`ndsRelocAssetReadHeader` opens the file by path, parses the header, closes it.
`ndsRelocAssetLoadData` then opens **the same file by the same path**, parses
**the same header** through `ndsRelocAssetReadHeaderFromFile`, and overwrites
`header` through its `out_header`. `asset_size` comes from
`ndsRelocAssetAllocSize`, not from the header, so nothing between the two calls
consulted the first read. It was a validity probe that cost a full directory
walk.

## 2. The change

`ndsRelocAssetLoadHeaderAndData()` (`src/nds/nds_reloc_assets.c`) does both from
one open. One call site changed: the Mario/Fox animation load at
`src/port/reloc_backend_assets.c:5456`.

The counters are bumped exactly as the two-call sequence bumped them, one header
and one payload. That is not cosmetic: `gNdsRelocAssetHeaderReadCount` is read as
a per-event delta by `verify-battle-playable-down-air-stall.ps1`, and a load that
stopped counting its header would read as a load that never happened.

Two other sites keep the two-call form deliberately. `:5618` needs the header to
size its allocation before the payload read. `:5247` would change heap-advance
semantics on the failure path — today a header failure leaves `*heap_ptr`
untouched while a payload failure has already advanced it. Neither is the path
Task 71 measured, so neither earns the risk yet.

## 3. Result

| bucket | base P50 | new P50 | Δ P50 | base P95 | new P95 | Δ P95 |
|---|---|---|---|---|---|---|
| `SRC` | 317,312 | 316,544 | −768 | 948,800 | **854,336** | **−94,464** |
| `WORK-H` | 1,371,264 | 1,368,448 | −2,816 | 1,985,024 | **1,905,536** | **−79,488** |
| `STG` | 386,624 | 381,312 | −5,312 | 392,576 | 388,480 | −4,096 |
| `FTR` | 576,704 | 578,176 | +1,472 | 1,013,248 | 1,015,808 | +2,560 |
| `ALL` | 1,680,064 | 1,680,128 | +64 | 2,240,512 | 2,240,576 | +64 |

`SRC` P95 falls **10.0%**. Against `PROJECT_GOAL.md`'s P95 ≤ 1,120,000, the gap
goes from 865,024 to **785,536** — **9.2% closed** by removing one duplicated
`fopen`.

The P50 barely moves, which is the expected shape: most frames load nothing. This
is a burst fix, and the gate is a burst statistic.

`ALL` is flat at +64 because it is VBlank-quantized and 79,488 ticks is well
under one 560,190-tick period. Per the standing rule, that is not evidence the
saving is absent — `WORK-H` is the search quantity precisely for this case.

## 4. What is not attributable to the change

`STG` moves −5,312 at P50 and `FTR` +1,472, in buckets this change cannot touch.
`STG`'s spread is 1.02, so −1.4% is larger than its run-to-run variation: this is
cache placement shifting under a new function, the sensitivity Task 37 documented.

That wobble is roughly ±5,000 and bounded by the untouched buckets. The measured
effect is 94,464 in the one bucket the change targets, so attribution is not in
doubt — but the honest headline is `SRC` P95 −94,464, with the `WORK-H` figure of
−79,488 already netting the placement noise in the other buckets.

## 5. What remains

The `strncasecmp` cost is halved, not removed: the surviving open still walks the
directory. The asset identity is numeric before the open, so the path lookup is
avoidable entirely. `ndsRelocAssetIDForToken` — roughly a hundred comparisons plus
two linear scans of the Mario and Fox animation tables per relocation, +9,306
ticks/frame — is untouched. And preloading a match's animations at match start
removes the read, copy and relocation from gameplay altogether, which is the
option `PROJECT_GOAL.md` most directly endorses.

Task 71 sized the whole on-demand-load lever at roughly 170,000 of P95. This took
79,488 of it with a five-line change.

## 6. Task 73 addendum — the residency shortcut does not exist

`lbRelocGetForceExternHeapFile` calls `ndsRelocForceLoadFighterAObj16File`
unconditionally: unlike `lbRelocGetStatusBufferFile`, which consults
`ndsRelocFindStatusNode` and returns early, the force path has no residency check
at all. That invited an obvious question — 26 of 128 frames load an animation, but
two fighters in a one-minute match do not use 26 distinct new moves, so are the
same animations being re-read?

Counted rather than assumed, because "Force" plausibly means the caller wants
pristine data restored and the renderer does mutate loaded fighter data. Two
observation-only counters, the load path itself untouched
(`build-task73-animcensus`, `artifacts/task73-animcensus.json`):

```
animLoad:16   animResident:0
```

**16 loads over 128 frames, and zero of them targeted a heap that already held
that asset.** The destination is a reused slot holding a different asset each
time, so nothing is being re-fetched redundantly and a residency cache would
return exactly zero.

Worth noting without leaning on it: 16 animation loads plus the 10 animation-lock
fallback frames equals the 26 frames Task 70 measured above 1.5x median `SRC`. The
arithmetic is tidy but the frame identities were not cross-checked, so it is a
lead, not a result.

The consequence is that the remaining on-demand-load cost cannot be skipped, only
moved. Preloading a match's animations means holding them resident, which is a
RAM budget decision rather than a code change — `PROJECT_GOAL.md` endorses the
trade, but the size of it is the owner's call. The two cheap items are untouched
and independent of that: the surviving open still resolves by path though the
asset identity is numeric before it, and `ndsRelocAssetIDForToken` is still a
hundred comparisons plus two linear table scans per relocation.
