# Animation cache ring scans on compact extents, 2026-10-05

Frames that fetch a fighter clip (60 of the 800 profiled gate frames,
`artifacts/task37-census/gp2-official`) averaged 2.14M ARM9 cycles against
1.82M for the rest; the ring allocation's own bookkeeping was ~41K of it:
`ndsR2AnimCacheRawRingAllocAligned` 27.3K and `ndsR2AnimCacheEvictRawRange`
13.8K. Each allocation scans every cache entry up to nine times (eight CLOCK
steps and the eviction) for overlapping byte ranges, and each 32-byte entry was
a line fill per scan (~130 entries, a 4 KB data cache).

`src/port/reloc_backend_assets.c`: each entry's extent (16-byte units from the
block's base, end rounded up -- payloads and slots start 16-aligned, so the
unit test is the byte test) and CLOCK mark live in side arrays, 5 bytes an
entry, that stay cached across the scans. Same decisions, same bytes. Same-ROM
A/B word `gNdsR2AnimCacheExtents`.

Official gate, same ROM (`build-gate-1005e`, `gate-ax0` -> `gate-ax1`): paired
median 0 (only fetch frames change), single frames down by up to 87K, P95
1,140,224 -> 1,138,816, over 111 -> 110, replay digest identical.

Rejected the same day: using a streamed clip where its sector read lands
instead of compacting it with a memmove (`gNdsR2AnimClipInPlace`,
`build-gate-1005f`, `gate-cip0` -> `gate-cip1`): paired median 0, single frames
-38K and +40K -- the ring's lost lead bytes cost re-reads -- P95 +2.3K. Not
kept.
