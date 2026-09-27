# P2-2p8: ROM reads bounce only unaligned edges (2026-09-26)

**Outcome: BANKED.** Every ARM9 ROM read (motion clips, FGM samples and
envelopes, reloc assets) goes through `ndsAudioStorageReadCard`, one blocking
PXI round trip per request to the ARM7 media service. A destination that was
not 32-byte aligned took the 512-byte bounce path for its whole length: a 20 KB
clip was 40 round trips. Now only the head (up to the next line) and the tail
bounce; the aligned middle is one direct request.

## Instrument

`gNdsAudioStorageWaitTicks64` / `gNdsAudioStorageWaitMaxTicks64` (new, ARM9
time inside `pxiSendAndReceive`, /64 system ticks) plus the existing request/
read/byte counters, zeroed at the first frame marker (`run-s6.ps1 -ExtraSets`)
so they price frames 1..1973 of the canonical four-CPU match.

| | control `F2603E76` | candidate `5EEE8AFD` |
|---|---:|---:|
| PXI requests | 18,651 | 2,233 |
| bytes read / via bounce | 3,580,347 / 2,817,211 (79%) | 3,580,663 / 18,071 |
| ARM9 blocked ticks | 69,430,144 (35,208/frame) | 35,785,856 (18,147/frame) |
| per request / max | 3,723 / 105,600 | 16,026 / 214,080 |
| WORK-H P50 / P95 / P99 | 1,336,256 / 1,962,112 / 2,793,280 | 1,333,504 / 1,892,160 / 2,384,256 |
| SHDT P99 | 549,504 | 381,184 |
| VBlanks 2/3/4/5+ | 268/1435/215/55 | 269/1460/213/31 |
| mean ALL (FPS) | 1,702,795 (19.68) | 1,680,577 (19.94) |

Both ROMs are `6ab8a45578a` + the counters; the candidate adds the split.
Same build dir, route 1 / admit 2. Replay digest IDENTICAL over 1,972 frames
(against `BC3500EA`, which the control also matches).

What is left, 18K ticks/frame, is transfer time (~10 ticks/byte): the 3.58 MB
the match still reads after GO. Removing it is Phase 3's job (resident motions,
FGM heads/ARM7 voices), not the transport's.

Host: `scripts/sfx/test_audio_storage.py` (6 pass) now asserts an unaligned
20,000-byte read is three calls and leaves neighbouring bytes untouched.

Files: `iowait-route1*` control, `split-route1*` candidate.
