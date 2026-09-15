# P2-2p8 direct NitroROM FGM ranges

Date: 2026-09-15

Verdict: **KEEP. FGM live-start filesystem tail falls; P2-2p8 remains RED.**

After direct BGM refill and resident BPS1-directory cuts, the post-checkpoint
ARM9 census still put `get_fat` and `f_lseek` on six of the seven worst frames.
`ndsAudioFgmPlayAtPan` ran on 23/128 frames; those frames cost 1,044,225 cycles
per frame more than controls and carried 89,256 cycles/frame of `get_fat`
premium plus 54,675 of `f_lseek`. The FGM backend already owns one validated,
immutable NitroFS pack and exact sample/envelope byte ranges, so live starts now
resolve that pack to a Calico NitroROM file ID and read those exact ranges
directly. The existing stdio stream remains the failure fallback. Sample bytes,
cache replacement, envelopes, channel selection and source-visible FGM state are
unchanged.

## Same-ROM route A/B

The measurement ROM is SHA-256
`9094EBC3EDC2A9336138D1C313CD53A75A260937F2D90F8FECBEE74F4CE2BB3C`.
It carried a temporary 32-byte route cell solely for this A/B; that cell is not
in the final shipping source. Route 0 used the established stdio range reads and
route 1 used direct NitroROM ranges, so both arms had identical code/data
placement.

Frames 1400..1527:

| bucket | stdio P50 / P95 | direct P50 / P95 | delta P50 / P95 |
|---|---:|---:|---:|
| WORK-H | 1,708,672 / 2,488,320 | **1,707,968 / 2,432,384** | -704 / **-55,936** |
| SRC | 575,360 / 1,129,600 | 575,360 / **1,110,528** | 0 / **-19,072** |
| GCRA | 569,600 / 1,123,904 | 569,600 / **1,104,832** | 0 / **-19,072** |
| SINT | 232,256 / 560,448 | **232,192 / 545,792** | -64 / **-14,656** |

Engagement is exact. The stdio arm records 230 live stdio range reads; the
direct arm records 231 direct reads, zero stdio reads and zero direct fallbacks.
Both arms report 296 supported plays, two read/play failures and zero pool
exhaustion. The two failures therefore predate the route and are not hidden by
the optimization.

The full 1,972-sample same-ROM pair keeps the direction:

| bucket | stdio P50 / P95 | direct P50 / P95 | delta P50 / P95 |
|---|---:|---:|---:|
| WORK-H | 1,653,568 / 2,452,480 | **1,651,264 / 2,446,272** | **-2,304 / -6,208** |
| SRC | 564,288 / 1,156,288 | **562,752 / 1,141,568** | -1,536 / **-14,720** |
| GCRA | 558,528 / 1,150,592 | **557,184 / 1,131,840** | -1,344 / **-18,752** |
| SINT | 261,760 / 682,560 | **260,928 / 676,864** | -832 / -5,696 |

Paired WORK-H median is -256 ticks; 1,040/1,972 frames win and 96 tie. Five-plus
VBlank presents move 254 -> **246**. The direct arm services 327 live ranges,
zero stdio reads and zero direct fallbacks while both arms perform 418 supported
plays with the same two read/play failures.

## Final hard-on build

The temporary route cell was then removed and direct NitroROM reads became the
normal FGM range path. Final four-CPU ROM SHA-256:
`9F47350F16AB1EBDCD088D1161F8E6D76D5160A7DBB671C8B6E08E9237F0EEB8`.
The standing stress verifier now fails unless FGM direct reads engage, stdio
range reads stay zero and direct fallbacks stay zero.

Final one-minute Donkey/Samus/Link/Kirby Dream Land stress:

| bucket | P50 | P95 |
|---|---:|---:|
| ALL | 2,237,696 | 2,798,272 |
| FTR | 392,512 | 768,064 |
| AUD | 3,392 | 124,032 |
| SRC | 562,048 | 1,145,664 |
| GCRA | 556,608 | 1,140,032 |
| SINT | 259,904 | 679,424 |
| WORK-H | **1,657,472** | **2,438,912** |

The final run records FGM direct/fallback/stdio **329/0/0**, BGM direct/fallback
**333/0**, BPS1 directory hits/fallbacks **681/0**, animation-cache reserve
failure/skips **1/1353**, native failures/direct rejects **0/0**, SyMalloc
overflow 0 and objman panic 0. General-heap low-water remains **111,680 B**, or
86,080 B above the 25,600 B floor. Cadence is 115/798/804/256 for 2/3/4/5+
VBlanks. The product target remains open: only 5.83% of presents are 2-VBlank
and WORK-H P95 remains far above 1.12M ticks.

The final source no longer contains the FGM A/B route. Permanent evidence is
this report plus `control128.*`, `candidate128.*`, `control-stress.*`,
`candidate-stress.*`, and `final-stress*` in this directory.

The final shell-driven Mario/Fox Dream Land realtime arm also passes on the
hard-on source, including the native-only link check, Pupupu realtime pacing
smoke, top-screen content checks and published-ROM contract. Its P2 shell ROM is
SHA-256 `86043CB0ACF4EB78576F41281420B574D4C98D5A769FA7F2D5D11D4C6BA43AEA`;
the refreshed root `smash64ds.nds` is
`FE959D60FA9C1E5D540A759AD066F96D4BA44C56F35B82B01D7AB7DB9692D045`.
Raw output: `twofighter-realtime.txt`.
