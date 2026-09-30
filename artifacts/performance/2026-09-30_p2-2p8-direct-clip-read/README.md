# P2-2p8: a fighter clip miss reads whole sectors straight into the ring (2026-09-30)

Run files (rows, run logs, JSON) are in
`artifacts/performance/2026-09-26_p2-2p8-ftr-item-tail/` under the arm names
below. Same ROM per table; the arms differ only in the `.data` word
`gNdsR2AnimDirectRead` (0 = the previous miss path).

## Why

The lab's status-price columns (`battleship_ftmain.c`, NDS_LAB_STATUS_PRICE)
split a status change (SCPU, ~66-69K ticks each on the four-CPU lab) into the
motion fetch (SHDT ~22-26K), the new clip's first play (MCAP ~21K), the
figatree install (SPRM ~11K), resets (MPRO ~1-2K) and the rest (~10-13K). The
P93-97 frames carry ~1.1 changes against ~0.35 at the median, so the setter is
the largest single P95 item: removing it outright is -60K..-79K P95 by stage,
the fetch alone -20K..-36K (`lf4_g*`, `if_g1` rows).

The fetch is expensive only when it reads: 36-44% of changes read a clip from
storage (SPHD), and a fitted read costs 43-51K ticks against ~5K for a cached
fetch. A census (`ac_g*`) put the ARM9's own blocked time at 27-37K ticks a read
(`gNdsRelocAssetFighterStreamReadTicks64`): the ARM7 storage thread does the
read while the ARM9 waits in the PXI round trip.

The read was four sector-read calls and two round trips for a ~2.3 KB clip:
clips sit at 16-byte offsets, so the ARM9 reader sent the 32-byte-multiple body
and then a second request for the last bytes, and the ARM7 read the body as a
partial head sector, a multi-sector run and a partial tail sector (the tail
sector read twice). The clip was then copied from the fighter's heap into the
ring, and the heap registered as a loaded file.

## Change

`ndsRelocAssetFighterStreamClipSpan` / `ndsRelocAssetReadFighterStreamClipSpan`
(`src/nds/nds_reloc_assets.c`) read the whole sectors a clip touches in one
request (the pack, like every NitroFS file, is 512-aligned in the ROM image).
On a cache miss `ndsR2AnimDirectReadEntry` (`src/port/reloc_backend_assets.c`)
takes a 32-byte aligned ring slot of the span, reads into it, moves the clip to
the slot's start, gives the tail sector's spare bytes back to the ring, and
makes the READY_STREAM entry the store made; the fetch then pins it exactly as
a zero-copy hit. Any refusal happens before an entry exists and the previous
miss path runs.

## Result (same ROM)

Lab ROM (`build-p2p8-lab-cb`), WORK-H from frame 64; per-read cost is the SHDT
fit on (changes, reads); I/O is the ARM9's blocked time per read:

| Stage | Arm | P50 | P95 | P99 | reads | SHDT/read | I/O/read |
|---|---|---:|---:|---:|---:|---:|---:|
| Dream Land | off `dr0_g6` / on `dr1_g6` | 924,672 / 923,712 | 1,256,000 / **1,245,888** | 1,454,592 / 1,466,944 | 290 / 294 | 43,227 / **30,609** | 28,244 / 21,536 |
| Sector Z | `dr0_g1` / `dr1_g1` | 1,155,904 / 1,152,256 | 1,576,128 / **1,571,520** | 1,892,864 / 1,871,424 | 281 / 281 | 51,111 / **31,825** | 35,892 / 23,902 |
| Castle | `dr0_g0` / `dr1_g0` | 1,002,560 / 1,000,704 | 1,333,376 / **1,330,304** | 1,683,072 / 1,665,984 | 266 / 266 | 43,443 / **31,600** | 27,531 / 21,970 |
| Saffron | `dr0_g7` / `dr1_g7` | 1,085,248 / 1,082,176 | 1,451,008 / **1,432,512** | 1,738,560 / 1,753,472 | 351 / 357 | 47,951 / **31,869** | 33,072 / 22,467 |

Gate ROM (`build-p2p8-b4`, Dream Land; its larger heap caches more, 223 reads
a match):

| Arm | P50 | P95 | P99 | two-VBlank | I/O/read |
|---|---:|---:|---:|---:|---:|
| off `dg0_a` / `dg0_b` | 911,296 / 910,400 | 1,229,056 / 1,228,480 | 1,439,168 / 1,439,552 | 87.2% / 87.1% | 29,722 / 29,577 |
| on `dg1_a` / `dg1_b` | **908,224 / 908,224** | **1,226,688 / 1,227,968** | 1,441,984 / 1,471,680 | 87.3% / 87.6% | 21,541 / 22,146 |

Replay digest (`compare-replay-digest.py --sequence --resync 4`) IDENTICAL for
every pair above, and against the previous build's runs (`if_g1`, `sxl1_g0`,
`sxl1_g7`, `oa1_a`). Native failures unchanged (Sector Z's known 119, Saffron 3,
else 0); heap low-water unchanged. 9-12 refusals a match (clips outside the
stream pack or before the arena exists) take the old path.

## What the trace says next (not banked)

A lab-only trace of every clip fetch (`ct[0-3]_g{0,1,5,6,7,8}`, removed from
the source) gives ~700 fetches a match over 171-213 distinct clips (~350-390
KB) against a 119-212 KB elastic ring. Simulated on those traces (the FIFO ring
model reproduces the measured misses within 1-6%):

| Policy | DL | SZ | Castle | Saffron | YI | Mushroom |
|---|---:|---:|---:|---:|---:|---:|
| FIFO ring (shipped) | 298 | 268 | 255 | 349 | 361 | 323 |
| LRU | 263 | 246 | 230 | 315 | 330 | 291 |
| ring, cursor steps over a clip hit since its last pass | 262 | 248 | 233 | 307 | 326 | 285 |
| + prefetch the 2 likeliest successors (table from the other 5 stages) | 164 | 147 | 133 | 158 | 175 | 165 |
| Belady (optimal, needs the future) | 195 | 178 | 198 | 213 | 225 | 205 |

Every fighter's Wait clip is re-read ~5 times a match under FIFO. A two-queue
split (probation + protected) is worse than FIFO at every split. Successor
prefetch would be an ARM7 async read (`ndsAudioStorageReadAsync` exists for the
BGM) issued at the install, so the ARM9 never waits on it.
