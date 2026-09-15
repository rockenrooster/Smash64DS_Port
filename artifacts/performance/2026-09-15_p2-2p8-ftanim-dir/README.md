# P2-2p8 resident BPS1 directory + failed-cache reserve latch

Date: 2026-09-15

Verdict: **KEEP. The animation/status-change tail falls materially; P2-2p8 remains RED.**

The source-normalized BPS1 fighter-animation stream already replaced most live
fighter O2R parsing, but every clip request still read its 8-byte dense directory
row from NitroROM before reading the payload. The warm/cache path asks once for
size and again for bytes, so it could read the same row twice. The current pack
has 1,165 dense rows and a 9,320-byte directory. The retained change reads that
bounded immutable directory once when BPS1 opens and serves later row lookups
from RAM. A wider/failing directory keeps the existing per-row NitroROM path;
payload reads, validation and parser semantics are unchanged.

The additional static directory also exposed a separate repeated failure: on the
four-distinct-kind stress there is no longer enough post-setup heap for the tiny
raw animation cache, yet `ndsR2AnimCacheArenaEnsure()` recalculated that same
impossible reservation on every store. The scene heap is a monotonic bump
allocator and the fit check already includes pending fighter bytes, so the final
change latches the first post-setup failure for that heap generation. Later
attempts decline in constant time until the generation changes.

## Same-ROM directory A/B

The comparison ROM is SHA-256
`84E270A8CB952572ED8AF1E1C1321CD44018CEF0833555719A20F14866FE3A50`,
with build-config SHA-256
`23314B62FBBECCAC413A9D677555EB1233137F84C26D5D9DF0177F801F5415F3`.
`gNdsRelocAssetFighterStreamDirRoute.route` was poked at the first frame marker,
so both arms carry identical code/data placement and the same resident 9,320-byte
directory. Route 0 uses the old per-row reads; route 1 consumes the resident rows.

Frames 1400..1527:

| bucket | route 0 P50 / P95 | route 1 P50 / P95 | delta P50 / P95 |
|---|---:|---:|---:|
| WORK-H | 1,716,224 / 2,566,848 | **1,700,608 / 2,438,272** | **-15,616 / -128,576** |
| SRC | 572,032 / 1,145,216 | 572,224 / **1,083,456** | +192 / **-61,760** |
| GCRA | 566,528 / 1,139,584 | 566,656 / **1,077,760** | +128 / **-61,824** |
| SINT | 230,528 / 586,688 | **229,120 / 559,296** | -1,408 / **-27,392** |

Engagement is exact: route 0 records **504 directory fallback reads**; route 1
records **508 resident-directory hits and 0 fallback reads**. Both arms perform
the same **504 BPS1 payload reads, 4 stream misses and 0 stream failures**. The
short 1406..1413 slice independently moves WORK-H P95
2,163,328 -> **2,049,664** while replacing 471 per-row reads with resident hits.

## Final one-minute four-CPU stress

The final candidate adds the generation-scoped failed-reserve latch. ROM SHA-256:
`8A87623C3A2818F954A706D970085D9E7ADCBBC796A99BD0B2ACC92DB05732C7`.
The standard Donkey/Samus/Link/Kirby Dream Land arm covers frames 2..1973 and
source clock 60 -> 1 (59/60 seconds). Against the retained direct-BGM checkpoint:

| bucket | BGM checkpoint P50 / P95 | final P50 / P95 | delta P50 / P95 |
|---|---:|---:|---:|
| ALL | 2,237,760 / 2,798,400 | **2,237,696 / 2,798,208** | -64 / -192 |
| FTR | 394,176 / 768,640 | **392,960 / 758,336** | **-1,216 / -10,304** |
| SRC | 561,536 / 1,247,040 | **561,088 / 1,149,952** | -448 / **-97,088** |
| GCRA | 555,648 / 1,241,024 | **555,584 / 1,139,520** | -64 / **-101,504** |
| SINT | 260,160 / 817,216 | 260,416 / **680,448** | +256 / **-136,768** |
| AUD | 3,392 / 123,712 | 3,392 / **123,520** | 0 / -192 |
| WORK-H | 1,650,688 / 2,505,280 | 1,654,464 / **2,441,408** | +3,776 / **-63,872** |

Cadence changes from 120/803/755/295 to **115/793/818/247** for 2/3/4/5+
VBlanks. The important tail population improves: 5+ VBlank presents fall by 48,
while 2-VBlank cadence is still only 115/1,973 = **5.83%**, far below the >=95%
product target.

The final run records directory bytes/hits/fallback/load-failure
**9320/681/0/0**, BPS1 payload reads/misses/failures **676/5/0**, and BGM direct
reads/fallbacks **333/0**. The extra static directory lowers general-heap
low-water to **111,680 B**, still **86,080 B** above the 25,600 B floor. On this
four-kind topology the old 5,216-byte raw animation cache no longer reserves;
that costs 14 prior cache hits but eliminates hundreds of directory ROM reads.
The failed-reserve latch changes 1,354 full reserve attempts into **1 measured
failure + 1,353 constant-time skips**. Native failures/direct rejects, SyMalloc
overflow, objman panic and animation-stream failures all remain zero.

## Cross-configuration check

The final source was rebuilt as the P2 shipping shell and run through the
shell-driven two-fighter realtime arm. `twofighter-realtime.txt` records
`battle_playable Pupupu realtime pacing smoke passed` and the published-ROM
contract passes. Its locked-30 warning remains expected while P2-2p8 is RED.

The full Boundary umbrella is still blocked by the pre-existing owner input
`decomp/alt_assets/` at the architecture rule; that read-only input is preserved.
