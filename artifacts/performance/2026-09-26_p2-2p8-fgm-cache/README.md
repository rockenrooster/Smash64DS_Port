# P2-2p8: FGM cache fills (2026-09-26)

**Outcome: BANKED** (LRU tiebreak, line-aligned cache, resident envelopes).
**Async fills: REVERTED** (digest divergence in one binary layout).

## Where the remaining ROM wait was

Per-source counters (new: `gNdsAudioFgmReadTicks64/Bytes`,
`gNdsRelocAssetFighterStreamReadTicks64/Bytes`), zeroed at the first frame
marker, `5EEE8AFD`-class ROM (`iosplit-route1`):

| source | reads | bytes | ARM9 ticks/frame | ticks/read |
|---|---:|---:|---:|---:|
| FGM samples | 342 | 2,547,188 | 11,483 (64%) | 66,211 |
| motion clips | 498 | 1,004,072 | 6,757 (38%) | 26,755 |

342 of 456 plays missed the eight-slot cache. Eviction took the smallest
fitting free slot with ties to the lowest index, so small misses all refilled
one 16 KiB slot. An LRU tiebreak moves misses only 342 -> 332: the match's
cue working set is larger than the cache. The motion cache is a 101 KB FIFO
ring over a ~1.2 MB motion set (509 misses, 197 hits); only residency fixes it.

## Banked change

- LRU tiebreak among equal-capacity free slots; a slot's tag is cleared before
  its refill, so a failed read cannot leave a stale hit.
- Cache aligned to 32 B (was 4): each fill is one direct read instead of a
  bounced head, body and tail.
- All 24 envelopes (448 B) read once at pack load; a play no longer re-reads its
  envelope from ROM. Handles point into the table (their 128 B arrays are gone).

`fgmfinal` `096C3002` against `split` `5EEE8AFD` (both frames 2..1973, route 1):
WORK-H P50/P95/P99 1,333,504/1,892,160/2,384,256 -> 1,328,576/1,869,504/
2,360,192; VBlanks 5+ 31 -> 30; 19.94 -> 20.02 FPS. Replay digest IDENTICAL.
Host test `scripts/sfx/test_fgm_metadata_residency.py` now runs the loader
with the envelope read (three freads, `READ` on each) and compares the resident
table with the pack tail; it was already failing to compile at HEAD for want of
a `ndsAudioFgmDirectRouteInit` stub, which it now has.

## Reverted: asynchronous fills on an ARM9 worker thread

A Calico thread (2 KiB stack, TLS attached from that stack) read misses while
the game thread continued; voices started at the next update. Same-ROM A/B
(`25BF6B1A`, boot word `gNdsAudioFgmAsyncFill`): WORK-H P95 1,866,368 (sync)
-> 1,850,752 (async), both replay-identical, 320 deferred starts, 0 failures.

But binary `3FA02C44` -- the same code without that one `.data` word --
diverges from control at frame 46 deterministically, with or without gdb
pokes (runs `fgmasync`, `repro-nopoke`, `repro-poke`). The worker's stack sits
directly above the mailbox and the cache slot table whose `data` pointers the
worker writes through; TLS takes ~512 B of the 2 KiB and IRQ handlers run on
the interrupted thread's stack. A layout-dependent overflow that scribbles a
slot pointer fits every observation. Not worth a 16K P95 win: removed entirely.
Any future ARM9 thread needs a generously sized stack and a layout-shift test
(two binaries differing only in padding) before its digest is trusted.

Committed evidence: `fgmlru-route1`, `fgmfinal-route1` (json/rows/log). The
async/repro runs are summarised above; their files stay local.
