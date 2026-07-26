# Task 76 — The sizing open: −115,712 on `SRC` P95

**Date:** 2026-07-26
**Status:** KEEP. Boundary verifier green (`artifacts/task76-verify-boundary.log`,
"Boundary verification profile passed", exit 0).
**Inputs:** `artifacts/task76-onewalk.json` against `artifacts/task72-oneopen.json`
— same instrument (`-RingDump`), same 128-frame window from frame 439, same
plain tick-HUD configuration.

Task 72 removed a duplicated `fopen` from the on-demand fighter-animation load
and closed 9.2% of the gap to `PROJECT_GOAL.md`'s P95 target. Its closing
section said the surviving open still walked the NitroFS directory. That was
right, and it undercounted: there were **two** walks left, not one.

## 1. The third walk

`ndsRelocForceLoadFighterAObj16File` opened with

```c
asset_size = ndsRelocAssetAllocSize(asset_id);
if (asset_size == 0u) { ... }
...
memset(heap, 0, asset_size);
if (ndsRelocAssetLoadHeaderAndData(asset_id, heap, asset_size, &header) == FALSE)
```

`ndsRelocAssetAllocSize` answers its question by opening the file by path,
parsing the header, and returning `NDS_RELOC_ALIGN(header.data_size)`. Then
`ndsRelocAssetLoadHeaderAndData` opens the same file by the same path and parses
the same header again. The sizing call was doing a full directory walk to learn a
number the load itself reports, and a second one to answer "does this asset
exist?" — a question the in-memory asset table answers with no I/O at all.

So the pre-Task-72 load did three walks, Task 72 took it to two, and this takes
it to one. Task 72's arithmetic in §5 ("the `strncasecmp` cost is halved") was
therefore optimistic about the denominator; the correct statement was that it
took one walk of three.

## 2. The change

`ndsRelocAssetLoadIntoZeroedHeap()` (`src/nds/nds_reloc_assets.c`) does size,
zero and payload from one open. It takes the caller's allocation granularity,
zeroes `dst` over `data_size` rounded up to it — the exact region the caller's
`memset` covered — and reports that size back so the caller's failure path can
still size itself.

The zero happens after the header is known and before the payload read, so the
bytes left in `dst` are identical to the old memset-then-load order, and every
failure path reachable once the header is known leaves `dst` fully zeroed, which
is what the caller's `fail:` label did.

The existence check that `ndsRelocAssetAllocSize == 0` used to provide is now
`ndsRelocAssetGetPath(asset_id) == NULL`, a table lookup with no file I/O.

`NDS_RELOC_ALIGN_BYTES` was extracted so the caller passing the granularity as a
value cannot drift from the macro that rounds with it.

One behavioural difference, stated rather than hidden: if the *open itself*
fails for an asset that is present in the table, `heap` is now left holding the
previous animation instead of zeros. That path means a corrupt ROM —
`gNdsRelocAssetOpenFailCount` is verifier-checked to be zero — and both outcomes
are wrong; it is a change in which kind of wrong.

## 3. Result

| bucket | base P50 | new P50 | Δ P50 | base P95 | new P95 | Δ P95 |
|---|---|---|---|---|---|---|
| `SRC` | 316,544 | 316,928 | +384 | 854,336 | **738,624** | **−115,712** |
| `WORK-H` | 1,368,448 | 1,365,248 | −3,200 | 1,905,536 | **1,861,952** | **−43,584** |
| `WORK` | 1,384,768 | 1,384,000 | −768 | 1,999,616 | 1,873,408 | −126,208 |
| `FTR` | 578,176 | 577,792 | −384 | 1,015,808 | 1,013,760 | −2,048 |
| `STG` | 381,312 | 380,160 | −1,152 | 388,480 | 388,032 | −448 |
| `OTHR` | 339,776 | 342,912 | +3,136 | 542,656 | 540,288 | −2,368 |
| `HUD` | 1,088 | 1,088 | +0 | 341,184 | 344,640 | +3,456 |
| `ALL` | 1,680,128 | 1,680,064 | −64 | 2,240,576 | 2,240,576 | +0 |

`SRC` P95 falls **13.5%**, and its worst frame falls 1,156,160 → 1,034,368. The
gate quantity `WORK-H` P95 falls **43,584** — smaller than `SRC`'s drop because
the frame that sets `WORK-H`'s P95 is not the frame that sets `SRC`'s, and
because `HUD` P95 rose 3,456 against it. Against the 1,120,000 target the gap
goes from 785,536 to **741,952**.

VBlank histogram moves the right way: 3-interval frames 469 → 472, 5+ frames
7 → 5, out of 567.

`ALL` is identical at P50 and P95, as expected — 43,584 ticks is well under one
560,190-tick VBlank period, and per the standing rule that is not evidence the
saving is absent.

## 4. Attribution

The change touches one bucket. `SRC` moves −115,712 at P95; every untouched
bucket moves by at most 3,456, inside the ±8,000 placement noise Task 37
documented and Task 74 was killed by. `WORK-H`'s −43,584 already nets that
noise, and it is 5.4× the floor, so this is not a Task 74 repeat.

The P50 is flat (`SRC` +384) for the same reason as Task 72: most frames load
nothing. This is a burst fix and the gate is a burst statistic.

The number of frames showing an `SRC` excursion above 1.5× median is **26 before
and 26 after**. That is the expected shape — the loads still happen at the same
frames, each one is cheaper — and it is a useful control: a change that had
merely moved work elsewhere would have moved that count.

## 5. What remains

One walk per load survives, and it is now the only one. The asset identity is
numeric before the open, so resolving by NitroFS file id rather than by path
would remove it entirely — that needs devkitPro/calico API knowledge this task
did not spend, and a build-time asset-id → NitroFS-id table.

Task 71 sized the whole on-demand-load lever at roughly 170,000 of P95 from a
single profiled frame. Task 72 took 79,488 and this took 43,584, so on that
sizing the remainder is thin — but the sizing came from one frame and the
`SRC` P95 is still 738,624 against a 316,928 median, so the burst is far from
explained. Only 1 of the 26 excursion frames has ever been profiled.

Task 75's preload remains the structural fix: it removes the read, the copy and
the relocation from gameplay rather than making each cheaper. Its blocker is
unchanged (301 animations, 96 registration slots, and the renderer's
`ndsRelocFindLoadedFileContaining` aliasing hazard), but this task narrows what
it would still win, because the walk it was going to remove is now one third of
its former cost.

## 6. Note on the header counter

`gNdsRelocAssetHeaderReadCount` now advances once per animation load rather than
twice. `verify-battle-playable-down-air-stall.ps1` computes a per-event delta
from it, but only to **print** it in its `DOWN_AIR_ENTRY` / `DOWN_AIR_EVENT`
lines — nothing compares it. Task 72's remark that a load which stopped counting
its header "would read as a load that never happened" is true for a human
reading that report and not for any assertion. The counter still advances once
per real load, which is the reading that matters.
