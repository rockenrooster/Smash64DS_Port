# P2-2p8: fighter clip prefetch in the frame's idle time, and a ring that steps over clips in use (2026-09-30)

Run files (rows, run logs, JSON) are in
`artifacts/performance/2026-09-26_p2-2p8-ftr-item-tail/` under the arm names
below. Same ROM per table; the arms differ only in `.data` words.

## Why

After `2026-09-30_p2-2p8-direct-clip-read` a blocking clip read costs ~31K
ticks and 36-44% of status changes still read. A lab-only clip trace (now kept
behind `NDS_LAB_CLIP_TRACE`, built with `CPPFLAGS=-DNDS_LAB_CLIP_TRACE=1`)
shows ~700 fetches a match over 171-213 distinct clips (~350-390 KB) against a
119-212 KB ring: two thirds of the reads are a fighter's first use of a clip,
one third re-reads of clips the FIFO ring dropped -- each fighter's Wait
(fetched 34-55 times a match) was read ~5 times.

## Change

1. **The ring steps over clips in use** (`gNdsR2AnimCacheClock`): a clip fetched
   since the ring's cursor last passed it is stepped over once instead of
   overwritten (CLOCK over variable sizes, no bytes moved).
2. **Successor prefetch in the idle time** (`gNdsR2AnimPrefetch`):
   `scripts/motion/generate_clip_successors.py` turns the lab clip traces (27
   runs: the lab roster on all nine stages, Mario/Fox/Luigi/Captain and
   Pikachu/Yoshi/Ness/Purin on all nine) into
   `include/nds/generated/nds_clip_successors.generated.h` (787 clips, the two
   likeliest next clips each, stream-pack clips seen at least twice). An
   installed clip is queued; in the frame's idle time before its presentation
   VBlank (`ndsR2AnimPrefetchIdle`, called from the VBlank wait, bounded by
   `ndsPlatformTicksToPresentVBlank`) its successors are read into ring slots
   with the ARM7's asynchronous read and landed as ordinary entries. A fetch
   of a clip still in flight waits for that read. Every path that takes ring
   bytes waits for a read covering them first (ring allocation, elastic
   give-back, arena drop); the heap reset drains them all
   (`battleship_sys_malloc.c`).

A clip's bytes are the pack's whichever way they arrive, so neither can change
a frame's result: replay digests are identical on every pair below.

## Refuted on the way

- **Prefetch issued at the install** (same ROM, seven stages, `pf0_*`/`pf1_*`):
  blocking reads -55%, but P95 +5K..+16K. The issue and landing added ~6K
  ticks a status change, and the next blocking read queued behind the
  prefetches on the ARM7 (+8K a read).
- **Two-queue ring** (probation + protected, simulated): worse than FIFO at
  every split.
- **ForgetRange by swap-remove** (`fg*`): the event-32 ledger's range forget
  (below) removing entries one by one with index deletion costs more than the
  ordered compaction plus one rebuild (Dream Land 169K -> 195K). Reverted.

## Result (same ROM)

Stepping ring alone (`ck0_*` off / `ck1_*` on, prefetch off): reads
294 -> 270 (DL), 281 -> 262 (SZ), 266 -> 254 (Castle), 356 -> 328 (Saffron);
P50/P95 flat.

Idle-time prefetch (`pz0_*` off / `pz1_*` on, stepping ring on in both), lab
ROM, WORK-H from frame 64; "tail fetch" is the mean motion-fetch time of the
P93-97 frames:

| Stage / roster | P50 | P95 | P99 | blocking reads | fetch ticks | tail fetch |
|---|---:|---:|---:|---:|---:|---:|
| Dream Land | 922,240 / 921,472 | 1,252,672 / **1,243,392** | 1,451,712 / 1,505,088 | 281 / 158 | 13.35M / 10.43M | 22,750 / 19,423 |
| Sector Z | 1,156,864 / 1,155,072 | 1,576,448 / **1,574,912** | 1,872,896 / 1,865,920 | 265 / 137 | 13.61M / 9.62M | 27,151 / 21,276 |
| Castle | 999,744 / 997,696 | 1,331,712 / **1,323,904** | 1,663,296 / 1,667,264 | 255 / 132 | 13.03M / 8.84M | 24,200 / 15,748 |
| Saffron | 1,081,728 / 1,081,152 | 1,444,288 / **1,433,344** | 1,749,440 / 1,735,680 | 334 / 179 | 15.88M / 11.08M | 26,089 / 18,024 |
| Jungle | 926,336 / 925,888 | 1,282,304 / **1,257,536** | 1,617,344 / 1,599,296 | 348 / 174 | 16.94M / 11.50M | 45,809 / 30,083 |
| Zebes | 1,019,584 / 1,021,632 | 1,410,176 / **1,408,320** | 1,742,144 / 1,736,064 | 296 / 158 | 13.71M / 9.52M | 29,405 / 17,597 |
| Hyrule | 956,544 / 954,240 | 1,343,168 / **1,338,944** | 1,706,368 / 1,696,320 | 260 / 121 | 12.67M / 9.04M | 15,299 / 14,435 |
| Yoshi's Island | 1,124,096 / 1,121,984 | 1,468,672 / **1,460,032** | 1,824,000 / 1,809,664 | 352 / 177 | 15.26M / 10.18M | 28,188 / 18,282 |
| Mushroom | 1,111,360 / 1,110,144 | 1,415,168 / **1,410,112** | 1,701,504 / 1,696,384 | 294 / 157 | 13.70M / 9.52M | 20,764 / 15,562 |
| Mario/Fox/Luigi/Captain, DL | 954,112 / 953,600 | 1,228,928 / **1,222,912** | 1,413,504 / 1,414,784 | 209 / 115 | 14.80M / 11.74M | 27,068 / 17,489 |
| same, Sector Z | 1,235,712 / 1,235,264 | 1,556,672 / **1,555,072** | 1,766,144 / 1,763,072 | 161 / 95 | 11.86M / 9.89M | 22,582 / 15,272 |
| Pikachu/Yoshi/Ness/Purin, DL | 949,696 / 948,480 | 1,325,376 / **1,319,616** | 1,621,248 / 1,612,224 | 223 / 146 | 14.30M / 11.75M | 28,890 / 25,201 |
| same, Sector Z | 1,016,832 / 1,014,016 | 1,434,944 / **1,425,088** | 1,653,568 / 1,659,072 | 225 / 128 | 13.00M / 9.53M | 24,259 / 13,365 |

P95 falls on all thirteen (mean -7.5K), blocking reads -41..-54%, fetch time
-17..-36%. 281-488 prefetches a lab match, 45-61% used.

Gate ROM (`pg0_*` off / `pg1_*` on, Dream Land): P50 910,272 / 910,400 ->
910,144 / 909,760; **P95 1,227,264 / 1,229,184 -> 1,222,080 / 1,222,976**;
P99 1,468,032 / 1,468,480 -> 1,474,816 / 1,475,712; two-VBlank 87.3% ->
87.5-87.7%. Reads 222 -> 105; 162 prefetches, 120 used.

## Open: the Dream Land P99 is a ForgetRange that moved

Dream Land's P99 rises (+53K lab, +7K gate). Timed step by step (lab-only
timers, removed): the frame is Samus's second entry clip (a non-stream event-32
clip) loading into her heap, and the extra ~150K is `ndsAObjEvent32ForgetRange`
in `ndsRelocPrepareFighterAnimHeapOverwrite` -- the ledger forget over the
heap that still holds her first entry clip (full ledger scan, ~700 entries
removed, index rebuilt). Without the prefetch no forget over that heap shows
in the fetch (its largest there is 6K) and one more pin rescue runs (3 vs 2);
a rescue into that heap runs the same forget, so it most likely lands there
instead -- the prefetch changes which clips get evicted, not the work. The
fix is the forget itself (off the frame), not the prefetch.
