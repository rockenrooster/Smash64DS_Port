# Task 22R wallpaper closeout

## Verdict

`KEEP PRODUCTION KEY + NEUTRAL CENSUS / REVERT THRESHOLD-4 WRITER / DO NOT
ENTER DMA PHASE`.

The threshold-4 packed dirty-span writer is exact but fails its natural-KO
P95 gate. It remains fully removed. Production already has a complete exact
final key, double-buffered X/Y maps in existing scratch, packed full-row
stores, repeated-row copies, and full-row DMA. No new cache, framebuffer,
writer, selector, allocation, or profile-0 diagnostic is retained.

## Artifact identity

- same-ROM profile-1 A/B ROM / ELF:
  `285867B3D8182E0991D2A5C17234DB2E934722670B2597D9A4A38ABB6185BAD6` /
  `8177EE126A52DD666EB4AB2D3DA622C25E27C4A45E885E9788D570B6A91F7B3B`
- profile-2 oracle ROM / ELF:
  `B5E03AD6A11D6DB87C0A6BDE5B8A955DF986CE80A97718F3BFD084DF8322850E` /
  `85B282CE836A3C81C42045B0703D95D9CA68245336AEEFEDF23AAC5CCAF1AD41`
- post-revert census ROM / ELF:
  `3556D08FB1C31C2420F0505E5FC29B0E33E101500EA033BC963DC9E8031F87E7` /
  `1CA31FDE94E3F8F5CBAFE2C71A617AA7FE020756648D1128E3B9FD44F694CFF4`
- current profile-0 ROM / ELF:
  `757ED78612607BEB8780BF197CC701570926B52EBDD745368DC32B6D44AC89E4` /
  `5F77112C0487DFF4B053AAA4227279B2472D31F72F70D7CE7B57CF0617046639`

The current ELF has zero Task-22, wallpaper phase-0.5, or wallpaper-oracle
symbols. The candidate run-mode selector, threshold, writer, and scratch plan
are absent from `Makefile`, `include`, `src`, and `scripts`.

## Phase A census

The retained profile-1 census reports setup, X-map, Y-map, physical-write,
commit, 192-row closure, changed-X runs, thresholds, scalar/packed stores,
full-row DMA, repeated-row copy, and total final writes. The A/B run topology
is identical. Aggregates over each synchronized eight-frame window are:

| Phase | Full / incremental rows | Changed X / runs / longest | >=4 runs / pixels | Scalar / DMA / copy pixels | Final writes |
|---|---:|---:|---:|---:|---:|
| Countdown | 331 / 1,205 | 618 / 523 / 3 | 0 / 0 | 90,547 / 84,736 / 0 | 175,283 |
| Early | 706 / 830 | 918 / 448 / 32 | 29 / 303 | 92,411 / 180,736 / 0 | 273,147 |
| Whispy | 760 / 776 | 991 / 565 / 256 | 1 / 256 | 75,635 / 194,560 / 0 | 270,195 |
| Natural KO | 232 / 1,304 | 248 / 156 / 5 | 18 / 79 | 14,266 / 59,392 / 0 | 73,658 |

Every phase has exactly `full + incremental = 8 * 192` rows and
`scalar + 2*packed + DMA + copy = final writes`. The candidate's observed
packed-dirty selection changes no map/run topology. Late/rebirth/stress
expansion is not required to decide this candidate: one relevant KO P95
regression is already an unconditional REVERT, and extra phases cannot rescue
it.

## Phase B complete key and invalidation ownership

The retained `NDSSObjWallpaperFinalCache` key is complete for whole-final
reuse:

1. validity;
2. source asset ID;
3. owner scene;
4. owner generation;
5. loaded-data pointer;
6. bitmap offset;
7. source platform epoch;
8. source layout fingerprint;
9. destination overlay epoch;
10. origin X;
11. origin Y;
12. X scale Q16;
13. Y scale Q16;
14. combine mode;
15. mapping version;
16. checked map-slot bound.

An exact key hit skips map construction and all writes. A source-key hit with
a changed camera alternates the two exact X/Y map slots, compares the maps,
and falls through to the current incremental writer. Profile 2 recomputes the
recurrence results and every final pixel. No second axis cache is warranted:
it would widen invalidation/data layout after the representative writer has
already failed and would require retail falsification.

## Phase C same-ROM A/B

| Phase | Wallpaper A P50/P95/max | Wallpaper B P50/P95/max | Writer A P50/P95/max | Writer B P50/P95/max |
|---|---:|---:|---:|---:|
| Countdown | 342,656 / 425,856 / 425,856 | 336,128 / 411,264 / 411,264 | 294,176 / 376,192 / 376,192 | 283,904 / 356,992 / 356,992 |
| Early | 403,904 / 428,032 / 428,032 | 377,920 / 400,896 / 400,896 | 355,040 / 379,008 / 379,008 | 326,560 / 348,608 / 348,608 |
| Whispy | 383,520 / 433,984 / 433,984 | 374,592 / 419,456 / 419,456 | 334,496 / 384,000 / 384,000 | 321,280 / 364,864 / 364,864 |
| Natural KO | 109,440 / 366,784 / 366,784 | 109,504 / 370,944 / 370,944 | 62,496 / 316,160 / 316,160 | 61,952 / 315,968 / 315,968 |

Moving-camera windows improve, but natural-KO wallpaper regresses
`+64/+4,160/+4,160` P50/P95/max while its physical writer changes only
`-544/-192/-192`. The task's `+2,000` KO gate forces independent REVERT.

The profile-2 run recorded 136,192 recurrence-map checks and 14,942,208 final
pixel checks with zero mismatches and no first failure. Its committed JSON
binds the exact ROM/ELF/mode but predates the later `wallpaperOracle` JSON
field; those totals remain the contemporaneous verifier/ledger result, not a
value reconstructed from the JSON. The current verifier now exports the
eight-field oracle row and fails on any map/pixel mismatch.

## Phase D and closeout

Phase D is not admissible: it is explicitly conditional on first retaining a
CPU span writer, and Phase C reverted that writer. Retail is authoritative for
VRAM stores, cache maintenance, and DMA; no device A/B exists and the user
declined further repeats. No emulator timing is promoted into a DMA claim.

The post-revert census conserves frame 445 as 10,570 scalar plus 10,496 DMA
pixels equals 21,066 final pixels, across 192 rows. GBI fixtures cover the
default-off lab identity, exact recurrence/pixel oracle, span histogram, and
final-write conservation. The current production ROM is byte-identical to the
green Task-20R/Task-27 checkpoint.
