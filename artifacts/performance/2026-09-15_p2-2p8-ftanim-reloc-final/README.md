# P2-2p8 phase H — BPS1 NitroFS placement KEEP

The fighter-animation stream bytes did not change. The retained change moves
`ftanim_stream_pack.bin` from the historical final `zz_stream` NitroFS entry to
`animation/`, immediately after the existing battlepack. This matters on the
authoritative DLDI profile path because Calico's `argv[0]` NitroROM backend reads
the `.nds` through libfat. A backward `lseek` restarts libfat's cluster walk at
the ROM file's first cluster, so a hot payload physically near the end of the
30 MB ROM makes every backward animation seek traverse most of the file.

## Same-ROM placement A/B

ROM SHA-256 `84B22AB5ABE98B13591F2C603FD23C8EFA10F09BC0DD2B93A0833F46A5009103`
contains two byte-identical BPS1 copies solely for measurement:

- early: `/animation/ftanim_stream_pack.bin` at `0x001EA000`
- late control: `/zz_stream/ftanim_stream_pack.bin` at `0x01CBC600`

The runtime route selects only the payload file id. Directory metadata, clip
indices, destination buffers and decoded bytes are identical.

Frames 1400..1527:

| metric | late | early | delta |
|---|---:|---:|---:|
| WORK-H P95 | 2,527,744 | 2,437,120 | **-90,624** |
| SRC P95 | 1,101,952 | 1,023,808 | **-78,144** |
| SINT P95 | 569,856 | 518,272 | **-51,584** |
| BPS1 payload reads/misses/failures | 504/4/0 | 504/4/0 | identical |

Whole one-minute four-CPU match, presented frames 2..1973:

| metric | late | early | delta |
|---|---:|---:|---:|
| WORK-H P50 | 1,655,488 | 1,642,560 | **-12,928** |
| WORK-H P95 | 2,451,520 | 2,351,360 | **-100,160** |
| SRC P95 | 1,161,856 | 1,066,304 | **-95,552** |
| SINT P95 | 693,376 | 597,376 | **-96,000** |
| VBlank 2/3/4/5+ | 115/796/805/257 | 115/827/817/214 | 5+ **-43** |
| BPS1 payload reads/misses/failures | 676/5/0 | 676/5/0 | identical |

The route counters prove exact engagement: the control records 672 late reads
plus four startup early reads before the route poke; the candidate records
676 early reads and zero late reads.

## Final single-copy checkpoint

Shipping source contains only `/animation/ftanim_stream_pack.bin`; the lab route
and duplicate late payload are removed. Final four-CPU ROM SHA-256 is
`73AE16BD8F871D1337A543FFF02169A4C231E791E59FEAD7F48AECBBA0FAC90E` and packs
the stream at `0x001EC000`.

The guarded one-minute stress passes with WORK-H P50/P95
**1,695,104/2,414,656**, SRC P95 **1,059,264**, directory
bytes/hits/fallback/fail **9320/681/0/0**, BPS1 reads/misses/failures
**676/5/0**, FGM direct/fallback/stdio **329/0/0**, BGM direct/fallback
**337/0**, native failures/rejects **0/0**, allocator/panic **0/0**, and general
heap low-water **112,192 B** (86,592 B above the floor). P2-2p8 remains RED;
only 101/1,973 presents are 2-VBlank.

The same-ROM A/B is the performance verdict. The final single-copy run proves
the retained packaging/runtime state and resource guards; its absolute timing
is not used as a cross-build price.
