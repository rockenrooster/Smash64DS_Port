# P2-2p8: the event-32 ledger forgets a range by leaving holes (2026-09-30)

Run files are in `artifacts/performance/2026-09-26_p2-2p8-ftr-item-tail/`
under the arm names below. Same ROM per table; the arms differ only in the
`.data` word `gNdsAObjEvent32ForgetHoles` (0 = the previous compaction).

## Why

`ndsAObjEvent32ForgetRange` retires the ledger entries of a buffer that is
about to be overwritten (`ndsRelocPrepareFighterAnimHeapOverwrite`, a fighter
heap that held an event-32 clip). The 09-27 page/block filter skips it when no
entry can lie in the range, which leaves ~4 calls a match that do remove a
retired file's few hundred commands. Each compacted the ledger in order and
rebuilt the whole index (8,192 slots cleared, every entry re-hashed):
~150-200K ticks, on whichever load reuses the heap. Timed step by step on the
lab (temporary timers): Dream Land's 1.5M P99 frame after the clip prefetch
(`2026-09-30_p2-2p8-clip-prefetch`) is Samus's second entry clip paying it.

Refuted first: removing entries one by one with linear probing's
backward-shift deletion and a swap from the end (`fg*`) -- Dream Land's forget
169K -> 195K; the index is dense around a file's addresses.

## Change

`src/import/battleship_sys_objanim.c`: a removed entry leaves a hole -- its
ledger command becomes NULL and its index slot a tombstone that lookups probe
past and inserts reuse. Every live entry keeps its position and its slot, so
every lookup returns what it did before. Holes are compacted out (with the one
rebuild) only when an append would not fit the stage's ledger bound;
tombstones are rebuilt away once they fill a quarter of the index.

## Result (same ROM)

Lab ROM (`tb0_*` off / `tb1_*` on), WORK-H from frame 64; "top fetch" is the
largest motion-fetch time of any frame:

| Stage | P50 | P95 | P99 | top fetch | removals / rebuilds / compactions |
|---|---:|---:|---:|---:|---|
| Dream Land | 924,928 / 924,800 | 1,251,584 / **1,248,768** | 1,511,872 / **1,473,600** | 379K / 304K | 728 / 0 / 0 |
| Sector Z | 1,155,520 / 1,155,776 | 1,575,680 / **1,573,888** | 1,869,696 / 1,872,960 | 378K / 299K | 1,499 / 1 / 1 |
| Castle | 999,808 / 1,000,320 | 1,328,320 / **1,326,464** | 1,670,784 / 1,672,896 | 328K / 372K | 1,329 / 1 / 0 |
| Saffron | 1,083,392 / 1,082,560 | 1,440,704 / **1,438,720** | 1,739,392 / 1,740,032 | 488K / 487K | 508 / 0 / 1 |
| Jungle | 928,256 / 927,424 | 1,265,088 / **1,263,680** | 1,590,272 / 1,583,104 | 301K / 293K | 1,448 / 0 / 2 |
| Yoshi's Island | 1,123,072 / 1,123,584 | 1,463,360 / **1,461,568** | 1,817,280 / 1,808,960 | 270K / 269K | 264 / 0 / 0 |

P95 -1.4K..-2.8K on all six; Dream Land P99 -38.3K. Normalize failures
unchanged (Jungle's 74 in both arms), no index overflow. Gate ROM (`tg0_*` /
`tg1_*`): P50 908,288 / 908,032 -> 907,840 / 907,904, P95 1,220,096 /
1,221,184 -> 1,220,736 / 1,220,736 (flat; the gate's forgets do not land on
its tail). Replay digests IDENTICAL for every pair, and gate `pg1_a` vs
`tg1_a` (across builds).

The remaining 270-490K fetch frames are non-stream event-32 clip loads
(storage read, byte swap, cache store, finalize ~65-70K each), not the ledger.
