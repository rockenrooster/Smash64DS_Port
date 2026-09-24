# P2-2p8 Phase 1 slice 6: the other eight VS kinds on the lean path

Slice 6 puts Mario, Fox, Luigi, Yoshi, Captain, Pikachu, Purin and Ness on the lean fighter path in VS battles, first at low detail (the four-fighter rosters), then at high detail (two-fighter lab arms). It also closes the high-detail Capacity declines of Donkey and Link.

- **Low detail:** all eight kinds draw lean on 100% of attempts, except Ness's yo-yo smashes. Those are named `validate` declines (158 of 1,739 in `cppn`, 127 of 1,816 in `mfpn`); the fix needs generator work (open item 1).
- **High detail:** in the two-fighter lab arms every kind draws lean on 100% of attempts except Ness's yo-yo (57 `validate` of 1,771). Donkey's and Link's lists that outgrow an entry (up to 3,595 / 3,754 words against 3,460) draw as **wide** lists, and Captain HIGH draws lean. Route 2 compares them all: 20,467 oracle runs, 0 mismatches.
- **Step 7 items:**
  - Mario's and Fox's electric skeleton (program 0xFE) draws lean.
  - Fox's blaster sidecar is submitted after the lean list (63 = 63 and 79 = 79 blaster draws, route 1 = route 0).
  - Yoshi's model-part programs 1 and 2 draw lean.
  - Pikachu's and Purin's accessories need nothing beyond the generic material path (100%, oracle 0 mismatches).
  - Captain HIGH's alpha cutout is admitted **without an approximation**; see "Owner item: D5".
- **Gates:** every roster has an identical replay digest, 0 route-2 oracle mismatches and native failures r1 = r0. The stop rule did not fire: FTR and WORK-H fall by the same amount in every roster.
- **Default roster:** WORK-H improves on slice 5, but FTR P50 is 1,248 ticks (0.7%) above slice 5's. That excess is the lab-only tick-HUD census span; section 2 has the evidence and it is listed as a deviation.
- **Shipping static RAM:** −32 B (`.main`). ITCM +32. DTCM, `.main.rw` and `.main.bss` are unchanged.

Rosters (four-CPU stress target `smash64ds-p2-fourcpu-tickhud-hwtri`, admit word 2, 1,972 samples, runner slot 9):

| roster | kinds (slots 0-3) | build dir | ROM sha256 |
|---|---|---|---|
| default (regression) | Donkey, Samus, Link, Kirby | `build-p2p8-s6` | `531e1b4b...` |
| `mfly` | Mario, Fox, Luigi, Yoshi | `build-p2p8-s6-mfly` | `62d94f5b...` |
| `cppn` | Captain, Pikachu, Purin, Ness | `build-p2p8-s6-cppn` | `c24e3e84...` |
| `mfpn` (electric skeleton) | Mario, Fox, Pikachu, Ness | `build-p2p8-s6-mfpn` | `b5b8e074...` |
| `hi-dklk` (high detail, two fighters) | Donkey, Link | `build-p2p8-s6-hi-dklk` | `c0155980...` |
| `hi-mafo` | Mario, Fox | `build-p2p8-s6-hi-mafo` | `468b7b74...` |
| `hi-luyo` | Luigi, Yoshi | `build-p2p8-s6-hi-luyo` | `fd8244b0...` |
| `hi-capi` | Captain, Pikachu | `build-p2p8-s6-hi-capi` | `302edf8c...` |
| `hi-puns` | Purin, Ness | `build-p2p8-s6-hi-puns` | `1cb869fb...` |
| `hi-saki` (regression) | Samus, Kirby | `build-p2p8-s6-hi-saki` | `c5f3271b...` |

Full hashes: `rom-sha256.txt`. Every gate ROM was re-made after its first build and reproduced its sha256. One exception, `mfly`'s first build, straight after the shell build, did not; its re-make is stable across two re-makes and is the ROM measured (`build-c6-mfly*.log`).

## 1. Gate tables

The four-fighter rosters are measured on arms `c6-*`. The two-fighter high-detail arms are measured on `c7-hi-*`: their `c6` ROMs carried a first cut of the lab oracle's wide-list placement that crashed route 2 (deviation 2).

### c6 (ROM sha256 531E1B4B4A8E58D9)

| gate | result |
|---|---|
| digest route 1 vs 0 | IDENTICAL over 1972 shared frames (control 1972, candidate 1972); by sequence: SEQUENCE IDENTICAL |
| digest route 2 vs 0 | IDENTICAL over 1972 shared frames (control 1972, candidate 1972); by sequence: SEQUENCE IDENTICAL |
| native failures r0 / r1 / r2 | 39 / 39 / 39 |
| route 2 oracle | 6,606 runs, 13,620,081 words, mismatches [0, 0, 0, 0, 0, 0, 0, 0] |
| verify arm (bit 16) | 165 re-selected lists (134 on variant entries), 0 mismatches; 140 kept plans resolved anyway, 0 differences; digest IDENTICAL |
| fighter uploads after GO r0 / r1 | 0 / 0 |
| heap low-water r0 / r1 (general, B) | 122,412 / 122,412 |
| libc top chunk min r0 / r1 (B) | 18,016 / 18,016 |
| Fox blaster draws r0 / r1 | 0 / 0 |
| VBlanks 2/3/4/5+ r0 | 145 / 1080 / 566 / 182 |
| VBlanks 2/3/4/5+ r1 | 206 / 1382 / 327 / 58 |

| counter | r0 P50 | r0 P95 | r0 P99 | r1 P50 | r1 P95 | r1 P99 | mean r1-r0 |
|---|---:|---:|---:|---:|---:|---:|---:|
| FTR | 343,360 | 1,001,408 | 1,316,854 | 184,800 | 268,797 | 778,296 | -185703 |
| WORK-H | 1,560,384 | 2,460,960 | 3,092,221 | 1,397,184 | 2,014,192 | 2,700,612 | -185348 |
| STG | 322,496 | 368,000 | 373,285 | 323,008 | 368,128 | 373,843 | +461 |

| row | kind | route 1 lean draws / attempts | share | declines | programs | high draws | words max | r2 oracle runs | r2 mismatches |
|---|---|---:|---:|---|---|---:|---:|---:|---|
| 0 | Donkey | 1,725 / 1,725 | 100.00% | none | 0: 1725 | 0 | 2,467 | 1660 | 0 |
| 1 | Samus | 1,685 / 1,685 | 100.00% | none | 0: 1628, 1: 6, 2: 9, 3: 42 | 0 | 2,313 | 1663 | 0 |
| 2 | Link | 1,824 / 1,824 | 100.00% | none | 0: 1775, 1: 33, 3: 16 | 0 | 2,634 | 1735 | 0 |
| 3 | Kirby | 1,628 / 1,628 | 100.00% | none | 0: 1565, 13: 63 | 0 | 1,403 | 1548 | 0 |

### c6-mfly (ROM sha256 62D94F5B003287EB)

| gate | result |
|---|---|
| digest route 1 vs 0 | IDENTICAL over 1972 shared frames (control 1972, candidate 1972); by sequence: SEQUENCE IDENTICAL |
| digest route 2 vs 0 | IDENTICAL over 1972 shared frames (control 1972, candidate 1972); by sequence: SEQUENCE IDENTICAL |
| native failures r0 / r1 / r2 | 327 / 327 / 327 |
| route 2 oracle | 6,391 runs, 11,613,645 words, mismatches [0, 0, 0, 0, 0, 0, 0, 0] |
| verify arm (bit 16) | 290 re-selected lists (287 on variant entries), 0 mismatches; 287 kept plans, 0 differences; digest IDENTICAL |
| fighter uploads after GO r0 / r1 | 0 / 0 |
| heap low-water r0 / r1 (general, B) | 123,132 / 123,132 |
| libc top chunk min r0 / r1 (B) | 18,112 / 18,112 |
| Fox blaster draws r0 / r1 | 63 / 63 |
| VBlanks 2/3/4/5+ r0 | 92 / 993 / 703 / 185 |
| VBlanks 2/3/4/5+ r1 | 100 / 1422 / 357 / 94 |

| counter | r0 P50 | r0 P95 | r0 P99 | r1 P50 | r1 P95 | r1 P99 | mean r1-r0 |
|---|---:|---:|---:|---:|---:|---:|---:|
| FTR | 342,912 | 713,526 | 1,000,976 | 182,400 | 261,206 | 441,697 | -188847 |
| WORK-H | 1,626,912 | 2,537,085 | 3,273,077 | 1,455,296 | 2,194,032 | 2,821,996 | -187058 |
| STG | 376,480 | 383,616 | 385,856 | 376,512 | 383,552 | 386,624 | +42 |

| row | kind | route 1 lean draws / attempts | share | declines | programs | high draws | words max | r2 oracle runs | r2 mismatches |
|---|---|---:|---:|---|---|---:|---:|---:|---|
| 0 | Mario | 1,766 / 1,766 | 100.00% | none | 0: 1766 | 0 | 1,738 | 1676 | 0 |
| 1 | Fox | 1,773 / 1,773 | 100.00% | none | 0: 1773 | 0 | 1,803 | 1734 | 0 |
| 2 | Luigi | 1,678 / 1,678 | 100.00% | none | 0: 1678 | 0 | 1,715 | 1582 | 0 |
| 3 | Yoshi | 1,542 / 1,542 | 100.00% | none | 0: 1419, 1: 109, 2: 14 | 0 | 2,421 | 1399 | 0 |

### c6-cppn (ROM sha256 C24E3E84A15D21C6)

| gate | result |
|---|---|
| digest route 1 vs 0 | IDENTICAL over 1972 shared frames (control 1972, candidate 1972); by sequence: SEQUENCE IDENTICAL |
| digest route 2 vs 0 | IDENTICAL over 1972 shared frames (control 1972, candidate 1972); by sequence: SEQUENCE IDENTICAL |
| native failures r0 / r1 / r2 | 2536 / 2536 / 2536 |
| route 2 oracle | 6,367 runs, 11,909,481 words, mismatches [0, 0, 0, 0, 0, 0, 0, 0] |
| verify arm (bit 16) | 233 re-selected lists (231 on variant entries), 0 mismatches; 259 kept plans, 0 differences; digest IDENTICAL |
| fighter uploads after GO r0 / r1 | 0 / 0 |
| heap low-water r0 / r1 (general, B) | 114,064 / 114,064 |
| libc top chunk min r0 / r1 (B) | 19,592 / 19,592 |
| Fox blaster draws r0 / r1 | 0 / 0 |
| VBlanks 2/3/4/5+ r0 | 79 / 1000 / 544 / 350 |
| VBlanks 2/3/4/5+ r1 | 91 / 1255 / 377 / 250 |

| counter | r0 P50 | r0 P95 | r0 P99 | r1 P50 | r1 P95 | r1 P99 | mean r1-r0 |
|---|---:|---:|---:|---:|---:|---:|---:|
| FTR | 315,584 | 782,848 | 1,060,425 | 169,536 | 285,296 | 678,332 | -168786 |
| WORK-H | 1,618,240 | 3,042,966 | 3,903,981 | 1,468,576 | 2,713,814 | 3,469,852 | -168908 |
| STG | 378,912 | 386,816 | 389,797 | 379,520 | 387,328 | 390,739 | +535 |

| row | kind | route 1 lean draws / attempts | share | declines | programs | high draws | words max | r2 oracle runs | r2 mismatches |
|---|---|---:|---:|---|---|---:|---:|---:|---|
| 0 | Captain | 1,711 / 1,711 | 100.00% | none | 0: 1711 | 0 | 1,893 | 1658 | 0 |
| 1 | Pikachu | 1,701 / 1,701 | 100.00% | none | 0: 1701 | 0 | 2,310 | 1622 | 0 |
| 2 | Purin | 1,693 / 1,693 | 100.00% | none | 0: 1693 | 0 | 1,609 | 1602 | 0 |
| 3 | Ness | 1,581 / 1,739 | 90.91% | validate 158 | 0: 1581 | 0 | 1,783 | 1485 | 0 |

### c6-mfpn (ROM sha256 B5B8E0746BF863F0)

| gate | result |
|---|---|
| digest route 1 vs 0 | IDENTICAL over 1972 shared frames (control 1972, candidate 1972); by sequence: SEQUENCE IDENTICAL |
| digest route 2 vs 0 | IDENTICAL over 1972 shared frames (control 1972, candidate 1972); by sequence: SEQUENCE IDENTICAL |
| native failures r0 / r1 / r2 | 1926 / 1926 / 1926 |
| route 2 oracle | 6,444 runs, 12,115,935 words, mismatches [0, 0, 0, 0, 0, 0, 0, 0] |
| fighter uploads after GO r0 / r1 | 3 / 4 |
| heap low-water r0 / r1 (general, B) | 118,152 / 118,152 |
| libc top chunk min r0 / r1 (B) | 18,192 / 18,048 |
| Fox blaster draws r0 / r1 | 79 / 79 |
| VBlanks 2/3/4/5+ r0 | 106 / 796 / 715 / 356 |
| VBlanks 2/3/4/5+ r1 | 115 / 1215 / 444 / 199 |

| counter | r0 P50 | r0 P95 | r0 P99 | r1 P50 | r1 P95 | r1 P99 | mean r1-r0 |
|---|---:|---:|---:|---:|---:|---:|---:|
| FTR | 348,224 | 821,904 | 1,151,019 | 186,560 | 294,678 | 678,282 | -205220 |
| WORK-H | 1,683,904 | 2,903,094 | 3,622,069 | 1,503,296 | 2,565,744 | 3,271,633 | -205091 |
| STG | 377,664 | 385,728 | 388,051 | 377,920 | 386,048 | 388,243 | +257 |

| row | kind | route 1 lean draws / attempts | share | declines | programs | high draws | words max | r2 oracle runs | r2 mismatches |
|---|---|---:|---:|---|---|---:|---:|---:|---|
| 0 | Mario | 1,739 / 1,739 | 100.00% | none | 0: 1725, skeleton 0xFE: 14 | 0 | 1,738 | 1601 | 0 |
| 1 | Fox | 1,731 / 1,731 | 100.00% | none | 0: 1721, skeleton 0xFE: 10 | 0 | 1,803 | 1664 | 0 |
| 2 | Pikachu | 1,726 / 1,726 | 100.00% | none | 0: 1726 | 0 | 2,310 | 1628 | 0 |
| 3 | Ness | 1,689 / 1,816 | 93.01% | validate 127 | 0: 1689 | 0 | 1,783 | 1551 | 0 |

### c7-hi-dklk (ROM sha256 C015598099A59C88)

| gate | result |
|---|---|
| digest route 1 vs 0 | IDENTICAL over 1972 shared frames (control 1972, candidate 1972); by sequence: SEQUENCE IDENTICAL |
| digest route 2 vs 0 | IDENTICAL over 1972 shared frames (control 1972, candidate 1972); by sequence: SEQUENCE IDENTICAL |
| native failures r0 / r1 / r2 | 266 / 266 / 266 |
| route 2 oracle | 3,418 runs, 12,486,870 words, mismatches [0, 0, 0, 0, 0, 0, 0, 0] |
| fighter uploads after GO r0 / r1 | 0 / 0 |
| heap low-water r0 / r1 (general, B) | 159,864 / 159,864 |
| libc top chunk min r0 / r1 (B) | 21,248 / 21,248 |
| Fox blaster draws r0 / r1 | 0 / 0 |
| VBlanks 2/3/4/5+ r0 | 700 / 1049 / 131 / 93 |
| VBlanks 2/3/4/5+ r1 | 1011 / 773 / 127 / 62 |

| counter | r0 P50 | r0 P95 | r0 P99 | r1 P50 | r1 P95 | r1 P99 | mean r1-r0 |
|---|---:|---:|---:|---:|---:|---:|---:|
| FTR | 203,328 | 1,056,701 | 1,242,912 | 108,672 | 993,558 | 1,026,533 | -98494 |
| WORK-H | 1,188,352 | 2,200,387 | 2,829,415 | 1,097,952 | 2,024,890 | 2,654,182 | -98613 |
| STG | 373,312 | 380,992 | 383,269 | 373,312 | 381,184 | 383,424 | +114 |

| row | kind | route 1 lean draws / attempts | share | declines | programs | high draws | words max | r2 oracle runs | r2 mismatches |
|---|---|---:|---:|---|---|---:|---:|---:|---|
| 0 | Donkey | 1,758 / 1,758 | 100.00% | none | 0: 1758 | 1,758 | 3,595 | 1698 | 0 |
| 1 | Link | 1,848 / 1,848 | 100.00% | none | 0: 1699, 1: 74, 3: 75 | 1,848 | 3,754 | 1720 | 0 |

### c7-hi-mafo (ROM sha256 468B7B745B38DBBB)

| gate | result |
|---|---|
| digest route 1 vs 0 | IDENTICAL over 1972 shared frames (control 1972, candidate 1972); by sequence: SEQUENCE IDENTICAL |
| digest route 2 vs 0 | IDENTICAL over 1972 shared frames (control 1972, candidate 1972); by sequence: SEQUENCE IDENTICAL |
| native failures r0 / r1 / r2 | 0 / 0 / 0 |
| route 2 oracle | 3,432 runs, 8,519,108 words, mismatches [0, 0, 0, 0, 0, 0, 0, 0] |
| fighter uploads after GO r0 / r1 | 0 / 0 |
| heap low-water r0 / r1 (general, B) | 198,772 / 198,772 |
| libc top chunk min r0 / r1 (B) | 21,088 / 21,088 |
| Fox blaster draws r0 / r1 | 47 / 47 |
| VBlanks 2/3/4/5+ r0 | 898 / 1020 / 46 / 9 |
| VBlanks 2/3/4/5+ r1 | 1308 / 648 / 14 / 3 |

| counter | r0 P50 | r0 P95 | r0 P99 | r1 P50 | r1 P95 | r1 P99 | mean r1-r0 |
|---|---:|---:|---:|---:|---:|---:|---:|
| FTR | 176,448 | 457,715 | 575,401 | 89,472 | 139,338 | 162,799 | -98933 |
| WORK-H | 1,135,648 | 1,545,098 | 1,932,867 | 1,038,368 | 1,373,798 | 1,623,020 | -99072 |
| STG | 373,248 | 380,800 | 383,379 | 373,376 | 381,149 | 383,397 | +147 |

| row | kind | route 1 lean draws / attempts | share | declines | programs | high draws | words max | r2 oracle runs | r2 mismatches |
|---|---|---:|---:|---|---|---:|---:|---:|---|
| 0 | Mario | 1,856 / 1,856 | 100.00% | none | 0: 1856 | 1,856 | 2,508 | 1733 | 0 |
| 1 | Fox | 1,733 / 1,733 | 100.00% | none | 0: 1733 | 1,733 | 2,456 | 1699 | 0 |

### c7-hi-luyo (ROM sha256 FD8244B0813C2B10)

| gate | result |
|---|---|
| digest route 1 vs 0 | IDENTICAL over 1972 shared frames (control 1972, candidate 1972); by sequence: SEQUENCE IDENTICAL |
| digest route 2 vs 0 | IDENTICAL over 1972 shared frames (control 1972, candidate 1972); by sequence: SEQUENCE IDENTICAL |
| native failures r0 / r1 / r2 | 0 / 0 / 0 |
| route 2 oracle | 3,289 runs, 9,224,092 words, mismatches [0, 0, 0, 0, 0, 0, 0, 0] |
| fighter uploads after GO r0 / r1 | 0 / 0 |
| heap low-water r0 / r1 (general, B) | 151,368 / 151,368 |
| libc top chunk min r0 / r1 (B) | 21,344 / 21,344 |
| Fox blaster draws r0 / r1 | 0 / 0 |
| VBlanks 2/3/4/5+ r0 | 665 / 1182 / 99 / 27 |
| VBlanks 2/3/4/5+ r1 | 1049 / 857 / 49 / 18 |

| counter | r0 P50 | r0 P95 | r0 P99 | r1 P50 | r1 P95 | r1 P99 | mean r1-r0 |
|---|---:|---:|---:|---:|---:|---:|---:|
| FTR | 174,144 | 593,414 | 656,351 | 101,760 | 170,509 | 630,756 | -94830 |
| WORK-H | 1,173,216 | 1,757,555 | 2,400,957 | 1,097,120 | 1,556,733 | 2,183,881 | -94671 |
| STG | 375,488 | 383,168 | 385,747 | 375,744 | 383,325 | 385,893 | +162 |

| row | kind | route 1 lean draws / attempts | share | declines | programs | high draws | words max | r2 oracle runs | r2 mismatches |
|---|---|---:|---:|---|---|---:|---:|---:|---|
| 0 | Luigi | 1,771 / 1,771 | 100.00% | none | 0: 1771 | 1,771 | 2,576 | 1651 | 0 |
| 1 | Yoshi | 1,740 / 1,740 | 100.00% | none | 0: 1676, 1: 52, 2: 12 | 1,740 | 3,306 | 1638 | 0 |

### c7-hi-capi (ROM sha256 302EDF8CDB69B5C6)

| gate | result |
|---|---|
| digest route 1 vs 0 | IDENTICAL over 1972 shared frames (control 1972, candidate 1972); by sequence: SEQUENCE IDENTICAL |
| digest route 2 vs 0 | IDENTICAL over 1972 shared frames (control 1972, candidate 1972); by sequence: SEQUENCE IDENTICAL |
| native failures r0 / r1 / r2 | 0 / 0 / 0 |
| route 2 oracle | 3,568 runs, 10,560,504 words, mismatches [0, 0, 0, 0, 0, 0, 0, 0] |
| fighter uploads after GO r0 / r1 | 0 / 0 |
| heap low-water r0 / r1 (general, B) | 122,960 / 122,960 |
| libc top chunk min r0 / r1 (B) | 21,056 / 21,056 |
| Fox blaster draws r0 / r1 | 0 / 0 |
| VBlanks 2/3/4/5+ r0 | 91 / 1618 / 221 / 43 |
| VBlanks 2/3/4/5+ r1 | 1120 / 773 / 59 / 21 |

| counter | r0 P50 | r0 P95 | r0 P99 | r1 P50 | r1 P95 | r1 P99 | mean r1-r0 |
|---|---:|---:|---:|---:|---:|---:|---:|
| FTR | 412,928 | 660,630 | 906,259 | 91,712 | 152,358 | 617,069 | -312646 |
| WORK-H | 1,393,056 | 1,950,752 | 2,563,353 | 1,073,696 | 1,566,640 | 2,218,728 | -311692 |
| STG | 381,696 | 390,080 | 392,147 | 381,760 | 390,301 | 392,659 | +127 |

| row | kind | route 1 lean draws / attempts | share | declines | programs | high draws | words max | r2 oracle runs | r2 mismatches |
|---|---|---:|---:|---|---|---:|---:|---:|---|
| 0 | Captain | 1,868 / 1,868 | 100.00% | none | 0: 1868 | 1,868 | 2,678 | 1820 | 0 |
| 1 | Pikachu | 1,827 / 1,827 | 100.00% | none | 0: 1827 | 1,827 | 3,255 | 1748 | 0 |

### c7-hi-puns (ROM sha256 1CB869FBB462FBBA)

| gate | result |
|---|---|
| digest route 1 vs 0 | IDENTICAL over 1972 shared frames (control 1972, candidate 1972); by sequence: SEQUENCE IDENTICAL |
| digest route 2 vs 0 | IDENTICAL over 1972 shared frames (control 1972, candidate 1972); by sequence: SEQUENCE IDENTICAL |
| native failures r0 / r1 / r2 | 855 / 855 / 855 |
| route 2 oracle | 3,345 runs, 7,639,880 words, mismatches [0, 0, 0, 0, 0, 0, 0, 0] |
| fighter uploads after GO r0 / r1 | 0 / 0 |
| heap low-water r0 / r1 (general, B) | 125,152 / 125,152 |
| libc top chunk min r0 / r1 (B) | 18,536 / 18,536 |
| Fox blaster draws r0 / r1 | 0 / 0 |
| VBlanks 2/3/4/5+ r0 | 1066 / 816 / 65 / 26 |
| VBlanks 2/3/4/5+ r1 | 1380 / 533 / 42 / 18 |

| counter | r0 P50 | r0 P95 | r0 P99 | r1 P50 | r1 P95 | r1 P99 | mean r1-r0 |
|---|---:|---:|---:|---:|---:|---:|---:|
| FTR | 162,592 | 498,954 | 596,244 | 83,840 | 169,424 | 505,092 | -88619 |
| WORK-H | 1,084,384 | 1,644,080 | 2,356,047 | 1,004,224 | 1,493,123 | 2,153,985 | -88867 |
| STG | 379,008 | 386,368 | 389,587 | 378,944 | 386,269 | 388,864 | +60 |

| row | kind | route 1 lean draws / attempts | share | declines | programs | high draws | words max | r2 oracle runs | r2 mismatches |
|---|---|---:|---:|---|---|---:|---:|---:|---|
| 0 | Purin | 1,813 / 1,813 | 100.00% | none | 0: 1813 | 1,813 | 2,217 | 1729 | 0 |
| 1 | Ness | 1,714 / 1,771 | 96.78% | validate 57 | 0: 1714 | 1,714 | 2,382 | 1616 | 0 |

### c7-hi-saki (ROM sha256 C5F3271BA8E92657)

| gate | result |
|---|---|
| digest route 1 vs 0 | IDENTICAL over 1972 shared frames (control 1972, candidate 1972); by sequence: SEQUENCE IDENTICAL |
| digest route 2 vs 0 | IDENTICAL over 1972 shared frames (control 1972, candidate 1972); by sequence: SEQUENCE IDENTICAL |
| native failures r0 / r1 / r2 | 195 / 195 / 195 |
| route 2 oracle | 3,415 runs, 8,176,701 words, mismatches [0, 0, 0, 0, 0, 0, 0, 0] |
| fighter uploads after GO r0 / r1 | 0 / 0 |
| heap low-water r0 / r1 (general, B) | 128,044 / 128,044 |
| libc top chunk min r0 / r1 (B) | 21,088 / 21,088 |
| Fox blaster draws r0 / r1 | 0 / 0 |
| VBlanks 2/3/4/5+ r0 | 1235 / 692 / 42 / 4 |
| VBlanks 2/3/4/5+ r1 | 1484 / 454 / 32 / 3 |

| counter | r0 P50 | r0 P95 | r0 P99 | r1 P50 | r1 P95 | r1 P99 | mean r1-r0 |
|---|---:|---:|---:|---:|---:|---:|---:|
| FTR | 146,496 | 209,309 | 445,388 | 81,344 | 119,069 | 341,004 | -65336 |
| WORK-H | 1,051,520 | 1,445,110 | 1,842,540 | 986,976 | 1,337,318 | 1,754,963 | -65633 |
| STG | 372,544 | 380,480 | 382,656 | 372,480 | 380,352 | 382,483 | -50 |

| row | kind | route 1 lean draws / attempts | share | declines | programs | high draws | words max | r2 oracle runs | r2 mismatches |
|---|---|---:|---:|---|---|---:|---:|---:|---|
| 0 | Samus | 1,749 / 1,749 | 100.00% | none | 0: 1676, 1: 41, 2: 6, 3: 26 | 1,749 | 3,286 | 1727 | 0 |
| 1 | Kirby | 1,771 / 1,771 | 100.00% | none | 0: 1734, 1: 37 | 1,771 | 2,276 | 1688 | 0 |

## 2. The default roster against slice 5

| route 1 | slice 5 (`a5-route1`) | slice 6 (`c6-route1`) | delta |
|---|---:|---:|---:|
| FTR P50 / P95 / P99 | 183,552 / 268,333 / 766,360 | 184,800 / 268,797 / 778,296 | +1,248 / +464 / +11,936 |
| FTR mean | 194,898 | 195,620 | +722 |
| WORK-H P50 / P95 / P99 | 1,404,128 / 2,031,187 / 2,690,529 | 1,397,184 / 2,014,192 / 2,700,612 | -6,944 / -16,995 / +10,083 |
| WORK-H mean | 1,455,092 | 1,450,160 | -4,932 |

The draw's own parts are flat. Over the match, head -265K, guard +65K, patch -26K, submit +160K and kernel +74K. **`book_ticks` is +5.46M, +795 ticks a draw.** That is the span between the patch and the submit. It holds only the tick-HUD census (`ndsFtrPreWalkCensus`, which reads every root's DObj) and the verifier marks, and **shipping compiles none of it**.

Its cost moves with layout, not code:
- `c5` and `c6` have the same book-span code, yet book is 19.57M against 23.65M.
- Between those two builds the kernel moved the other way, 133.95M against 129.57M.

The same exchange explains the slice-5 comparison, so it is reported rather than claimed or chased. The regression rows stay in the table.

## 3. Per-kind coverage, declines and cost

Route 1 lean share and named declines per kind are in the per-roster tables (section 1). What is new this slice:

| kind | low detail | high detail | named declines | notes |
|---|---|---|---|---|
| Mario | 100% (`mfly`, `mfpn`) | 100% (`hi-mafo`) | none | electric skeleton 0xFE: 14 draws in `mfpn` |
| Fox | 100% (`mfly`, `mfpn`) | 100% (`hi-mafo`) | none | skeleton 10 draws; blaster sidecar 63 / 79 / 47 draws = route 0 |
| Luigi | 100% (`mfly`) | 100% (`hi-luyo`) | none | |
| Yoshi | 100% (`mfly`) | 100% (`hi-luyo`) | none | model-part programs 1 and 2: 109 + 14 draws low, 52 + 12 high |
| Captain | 100% (`cppn`) | 100% (`hi-capi`) | none | HIGH alpha cutout admitted (section 8); HIGH oracle 1,820 runs, 0 mismatches |
| Pikachu | 100% (`cppn`, `mfpn`) | 100% (`hi-capi`) | none | |
| Purin | 100% (`cppn`) | 100% (`hi-puns`) | none | texture-fence mirror: 24 records in `cppn` route 2 |
| Ness | 90.91% (`cppn`), 93.01% (`mfpn`) | 96.78% (`hi-puns`) | `validate` 158 / 127 / 57: the yo-yo smashes | open item 1 |
| Donkey | 100% (default) | 100% (`hi-dklk`), wide | none | HIGH Capacity closed; wide lists oracle-compared (1,698 runs, 0 mismatches) |
| Link | 100% (default) | 100% (`hi-dklk`), wide | none | HIGH Capacity closed; 1,720 runs, 0 mismatches; `donor_memo` 900 (the boomerang, as slice 4) |
| Samus, Kirby | 100% (default) | 100% (`hi-saki`) | none | regression arms |

Per-draw cost of the lean draw, route 1 (ticks; `percost6.py`). The guard window holds the event path, and "event" is its share of it:

| roster | kind | detail | draws | head | guard | kernel | patch | submit | sum | of guard: event |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| c6 | Donkey | low | 1,725 | 20,867 | 13,796 | 20,192 | 5,519 | 2,687 | 63,061 | 9,883 |
| c6 | Samus | low | 1,685 | 13,222 | 5,180 | 17,357 | 3,467 | 2,482 | 41,708 | 2,329 |
| c6 | Link | low | 1,824 | 13,860 | 11,614 | 22,421 | 11,925 | 2,579 | 62,398 | 8,746 |
| c6 | Kirby | low | 1,628 | 12,129 | 4,619 | 15,109 | 3,107 | 2,070 | 37,034 | 2,115 |
| c6-mfly | Mario | low | 1,766 | 17,859 | 6,042 | 18,587 | 3,589 | 2,381 | 48,458 | 2,250 |
| c6-mfly | Fox | low | 1,773 | 11,384 | 4,031 | 20,636 | 3,107 | 2,870 | 42,028 | 1,191 |
| c6-mfly | Luigi | low | 1,678 | 10,420 | 5,110 | 18,578 | 2,981 | 2,365 | 39,455 | 2,572 |
| c6-mfly | Yoshi | low | 1,542 | 12,238 | 10,976 | 30,467 | 3,476 | 2,564 | 59,721 | 8,015 |
| c6-cppn | Captain | low | 1,711 | 18,682 | 6,961 | 20,423 | 3,998 | 2,751 | 52,814 | 2,789 |
| c6-cppn | Pikachu | low | 1,701 | 12,443 | 6,676 | 20,986 | 3,595 | 2,435 | 46,134 | 3,617 |
| c6-cppn | Purin | low | 1,693 | 12,545 | 4,559 | 15,325 | 3,151 | 2,268 | 37,848 | 2,133 |
| c6-cppn | Ness | low | 1,581 | 12,462 | 13,248 | 19,953 | 3,720 | 2,429 | 51,811 | 10,586 |
| c6-mfpn | Mario | low | 1,739 | 18,399 | 7,852 | 18,901 | 3,690 | 2,361 | 51,203 | 3,994 |
| c6-mfpn | Fox | low | 1,731 | 11,812 | 5,275 | 20,609 | 3,444 | 2,833 | 43,972 | 2,506 |
| c6-mfpn | Pikachu | low | 1,726 | 11,827 | 6,527 | 20,858 | 3,465 | 2,424 | 45,101 | 3,673 |
| c6-mfpn | Ness | low | 1,689 | 13,034 | 12,890 | 19,920 | 3,026 | 2,295 | 51,166 | 10,300 |
| c7-hi-dklk | Donkey | high | 1,758 | 19,353 | 18,975 | 19,895 | 5,602 | 4,172 | 67,998 | 14,974 |
| c7-hi-dklk | Link | high | 1,848 | 14,449 | 61,488 | 21,873 | 11,227 | 4,008 | 113,045 | 58,830 |
| c7-hi-mafo | Mario | high | 1,856 | 16,318 | 6,716 | 18,521 | 3,621 | 2,390 | 47,566 | 2,795 |
| c7-hi-mafo | Fox | high | 1,733 | 12,704 | 4,150 | 20,412 | 3,348 | 2,888 | 43,501 | 1,337 |
| c7-hi-luyo | Luigi | high | 1,771 | 21,051 | 9,806 | 18,551 | 3,880 | 2,369 | 55,656 | 5,531 |
| c7-hi-luyo | Yoshi | high | 1,740 | 13,014 | 13,601 | 29,843 | 3,249 | 2,563 | 62,271 | 10,702 |
| c7-hi-capi | Captain | high | 1,868 | 14,008 | 6,701 | 19,852 | 2,885 | 2,528 | 45,975 | 3,855 |
| c7-hi-capi | Pikachu | high | 1,827 | 16,881 | 13,459 | 20,285 | 3,488 | 2,480 | 56,593 | 9,480 |
| c7-hi-puns | Purin | high | 1,813 | 17,194 | 8,379 | 15,237 | 3,481 | 2,108 | 46,398 | 4,481 |
| c7-hi-puns | Ness | high | 1,714 | 12,230 | 11,294 | 19,787 | 3,442 | 2,357 | 49,110 | 8,538 |
| c7-hi-saki | Samus | high | 1,749 | 17,205 | 7,746 | 18,099 | 3,471 | 2,480 | 49,001 | 3,774 |
| c7-hi-saki | Kirby | high | 1,771 | 11,805 | 6,997 | 15,284 | 3,092 | 2,049 | 39,227 | 4,442 |

Link HIGH's event path (58.8K a draw) is 122 wide materializations: a wide list holds no variants, so each held state change materializes again. Route 0 re-records at the same events, and FTR P95 is 993,558 in route 1 against 1,056,701 in route 0. See open item 4.

## 4. Captures (route 0 against route 1, same ROM)

The tool is `tools/capture-s6.ps1`, which runs visible melonDS on runner slot 9 with gdb stops. `tools/capcmp6.py` compares the guest viewport only (x 8..407, y 56..655). Pictures and diff masks are in `captures/`.

- **Control.** The same ROM, route 0 twice, `mfpn` skeleton frames: **0 differing pixels**. The harness is deterministic.
- **What differs between routes, in every pair:**
  - the tick-HUD's FPS/UP line at the top of the bottom screen, which is expected;
  - single DS-pixel edge flips on fighter silhouettes. This is the lean layout's known rounding: P' plus LOAD4x3 against the recorded split matrices, clip row 3 within 1 LSB, at most 0.006 px of vertex movement (slice 4, section 2).

Pairs, per roster: 3D-scene pixels that differ (window pixels, top screen above the HUD line; about 2.4 window pixels make one DS pixel), the largest 8-connected cluster, and the share of differing pixels that sit on an edge of the route 0 picture (`tools/capclass6.py`, `captures-classified.txt`; per-frame listings in `captures-c7.txt`):

| pair | frames | differing scene px (total / worst frame) | largest cluster | on edges (worst frame) | verdict |
|---|---|---:|---:|---:|---|
| control: `mfpn` route 0 twice | skeleton stops 307/678/738/1695 | 0 / 0 | 0 | - | identical |
| `mfly` | 150, 260, 420, 640, 900, 1200 | 200 / 79 | 35 | 66.7% (a 6 px frame, max delta 4) | rounding |
| `cppn` | same | 193 / 105 | 38 | 100% | rounding |
| `mfpn` skeleton frames (settle 1) | 306/677/737/1694 | 62 / 28 | 13 | 89.3% | rounding + Mario's run 5 (below) |
| `mfpn` skeleton frames (settle 2) | 307/678/738/1695 | 215 / 86 | 53 | 97.3% | rounding |
| `hi-dklk` (wide lists) | 150 ... 1200 | 196 / 99 | 67 | 93.9% | rounding |
| `hi-mafo` | same | 139 / 60 | 48 | 100% | rounding |
| `hi-luyo` | same | 445 / 192 | 122 (a clipped edge on the top screen border) | 77.6% | rounding |
| `hi-capi` (Captain HIGH) | same | 287 / 206 | 141 | 95.6% | rounding |
| `hi-puns` | same | 89 / 28 | 12 | 96.4% | rounding |

- **What the clusters are.** Clusters longer than a few pixels run along silhouette or polygon edges. The larger ones are also ±1 RGB555 steps of Gouraud shading on smooth surfaces: 1-DS-pixel vertex movement changes where the interpolation rounds. Captain's largest cluster, for example, is mostly (85,48,101) against (89,52,105).
- **What none of them is.** No pixel changes part, texture or cutout.
- **Zooms.** `captures/zoom-c7-largest-components.png` and `captures/zoom-c7-capi.png` (route 0 | route 1 | diff) show the largest clusters. Side by side at 6× nothing is distinguishable.
- **Verdict.** These are differences at the pixel level, and they are the class slice 4 measured and accepted: 1 LSB of clip row 3, at most 0.006 px of vertex movement. They are reported here, not hidden. The owner is the oracle for render-only values (D3/D5).

**Mario's electric skeleton (`mfpn`): a named route 0 defect, not mirrored.**
- **What the route-2 compare saw.** Skeleton frames are always old-path records, never replay hits: the skeleton toggles every other frame. So only the record-under-hit compare sees them. It counts 14 records × 10 words: root 5's TEXIMAGE, PLTT and 8 TEXCOORD words.
- **Cause, traced in route 0** (`diag-mfpn-memo-r0-gdb.log`). Production's run texture memo is keyed on run index, player and owner key, not on the root program. On skeleton frame 676, Mario's run 5 hits the memo row filled by the body, entry `0221f910`, name 49199. It binds the body's 32×32 texture and UVs.
- **Route 1.** The lean materializer never reads the memo. It binds the skeleton's own 32×16 texture. This is the same class as slice 4's `donor_memo` (Link's boomerang): production's memo is wrong, and the lean list is not made to copy it.
- **Visible difference.** The captured skeleton frames (settle 1: frames 676 / 736 / 1693 on screen) differ by 10-28 scene pixels each. Most sit on edges. Mario's skeleton is drawn dark there, so the run-5 texture difference is not separable from the rounding.
- **Upload count.** The lean path's own texture is also the extra fighter upload after GO in `mfpn`: 4 against route 0's 3.

## 5. RAM

- **Shipping static RAM** (non-tick-HUD shell ELF `smash64ds-p2-shell-hwtri`, same tree; pre = the pre-slice sources of the seven edited files, `build-p2p8-s6-shipcheck-pre`):

  | section | pre | post | delta |
  |---|---:|---:|---:|
  | `.main` | 1,611,908 | 1,611,876 | **-32** |
  | `.main.rw` | 228,620 | 228,620 | 0 |
  | `.main.bss` | 904,784 | 904,784 | 0 (`gNdsFtrLeanOracleSourceOk` -4 falls in alignment) |
  | `.itcm` | 30,792 | 30,824 | +32 (the wide-list flush in the submit) |
  | `.dtcm` / `.dtcm.bss` | 8,800 / 2,000 | 8,800 / 2,000 | 0 |

  - The slice's growth (wide lists, fence and tint mirrors, skeleton, Fox gun, Captain) was paid for in the same slice:
    - the oracle routes 2/3 compiled out of shipping (`NDS_FTR_LEAN_ORACLE_ROUTES`: route hooks, topology hash, shadow arm, forced source compose);
    - route 1 inlined into its one caller, as slice 5's shipping image had it;
    - one materialize loop instead of two walks;
    - the tint mirror folded into ApplyTint's own fail-closed path.
  - Every changed symbol is in `ram-symbols-shipcheck.txt`. The shipping ROM re-built from the final tree reproduces `72d08138...`.
- **Gate ROM (tick-HUD, default roster):** ITCM 32,280 → 32,296 (472 B free), DTCM 8,800 + 2,028 unchanged.
- **Heap, general low-water (floor 25,600 B), route 0 = route 1 in every roster:**
  - `c6` 122,412 (slice 5: 122,412)
  - `mfly` 123,132
  - `cppn` 114,064
  - `mfpn` 118,152
  - two-fighter arms: `hi-dklk` 159,864; `hi-mafo` 198,772; `hi-luyo` 151,368; `hi-capi` 122,960; `hi-puns` 125,152; `hi-saki` 128,044
  - libc top chunk min 18,016-21,344.
  - The two-fighter arms carry a 70,720 B lab-only buffer (deviation 2), so their heap is not a shipping figure.

## 6. What changed

- `include/nds/renderer_fighter_lean.h`
  - Every owner slot is a lean kind, and counter rows are battle slots (`NDS_FTR_LEAN_OWNER_KIND`, `k_owner[]`).
  - `nNDSFtrLeanDeclineAlphaTest`.
  - The wide-list flag and `ndsFtrLeanEntryWide`.
  - The mirror counters.
  - `NDS_FTR_LEAN_ORACLE_ROUTES`, `NDS_FTR_LEAN_RUN_INLINE`, `NDS_FTR_LEAN_ROUTE_IS_DRAW()`, `NDS_FTR_LEAN_ORACLE_WIDE`.
- `src/port/renderer_fighter_lean.c`
  - The electric skeleton: program 0xFE for Mario and Fox, and the skeleton in the tuple.
  - Fox's blaster sidecar after the lean submit.
  - Captain HIGH admitted.
  - Wide lists: a Capacity decline is materialized again wide. `wide_high` makes a slot's next high-detail lists go wide directly.
  - The oracle routes compiled out of shipping.
- `src/nds/nds_renderer_native_common.c`
  - Wide entries: capacity, the other entry given up, whole-list flush, no variant records.
  - Variant records placed above Link's texgen map (a latent overlap).
  - The texture-fence mirror: `key[5]` is the virtual record's fence.
  - The tint mirror: a tinted site is `reserved[0]`, as the recorder marks it, and ApplyTint's fail-closed runs ResetShade.
  - The materializer's threshold rule (AlphaTest unless the reference is 0).
  - Lab only: Captain HIGH recorded in the oracle routes, the oracle wide-list buffer, and the oracle hooks compiled out of shipping.
- `src/port/renderer_adapter_fighter.c`
  - The old path's draw-plan key carries `colanim.skeleton_id` (`NDS_FTR_PLAN_DETAIL_KEY`). The plan used to replay the body's roots on skeleton frames (or 0xFE after them) until a status change; the route-2 oracle found it.
  - Head-tick rows by battle slot.
  - The shipping route word.
- `src/port/renderer_adapter_matrix.c`
  - `ndsRendererAdapterComposeNativeRootMatrix` is always-inline. Its fourth call site had outlined it and moved stage binding prep: STG +103K in route 0 and route 1 alike (`c1`).
  - The oracle's forced source compose is lab only.
- `src/port/nds_match_config.c`, `Makefile`: `NDS_LAB_FOURCPU_TWO=1` (lab only) empties roster slots 2/3, so the match is two-fighter and high detail. Empty by default, so no other build's config header changes.

## 7. Deviations

1. **Default roster FTR P50 +1,248 / P95 +464 over slice 5** (section 2). WORK-H P50/P95 fell 6,944 / 16,995. The excess is the lab-only census span, whose cost is set by layout.
2. **Lab oracle for wide lists.**
   - Route 2 keeps the recorder's reference packet in a slot's lower half, so a wide list cannot sit in its own region there.
   - The first cut put it in the absent players' regions (battle slot + 2). Those are **not idle**: `ndsRendererFighterPacketIdleRegion` lends them to a battle-lifetime pool. Donkey's DObjs lived there, and route 2 crashed at frame 105 (`iterations/c6-hi-dklk-route2-run.log`, `diag-dklk-wide-r2-gdb.log`).
   - The fix is a 70,720 B BSS buffer compiled only into the lab two-fighter image. The four-fighter and shipping ROMs are byte-identical before and after it (mfly `62d94f5b...`, shell `72d08138...`).
   - The `c6-hi-*` runs are superseded by `c7-hi-*`.
3. **Captain HIGH is recorded in routes 2/3 (lab).** The old path still draws Captain HIGH direct in routes 0 and 1. In the oracle routes it records and replays, so there is a reference to compare the lean list with.
4. **`uploads after GO`, `mfpn`: 4 in route 1 against 3 in route 0.** That is the skeleton's own run-5 texture (section 4).

## 8. Owner item: D5 (Captain HIGH alpha)

**No approximation was needed, so Captain HIGH ships lean.** The owner should still look at the A/B captures (`captures/cap-c7-hi-capi-*`), which D5 asks for.

- **The test.** Captain HIGH's one alpha-tested surface (root 6, two draws) is `G_AC_THRESHOLD` against `G_SETBLENDCOLOR` (0,0,0,0). The old path maps that to `glEnable(GL_ALPHA_TEST)` and `glAlphaFunc(0)`.
- **Why reference 0 decides nothing.** The DS never draws an alpha-0 fragment, test on or off. melonDS: `if (alpha <= RenderAlphaRef) continue;` with `RenderAlphaRef = (DispCnt & 4) ? AlphaRefVal : 0`, latched at the flush.
- **Why leaving the register out changes nothing.**
  - The register is frame-global.
  - The old path's next batch disables it again, as the lean submit does.
  - Every enable in the tree writes its own reference.
- **The guard.** The materializer declines any threshold run whose reference is not 0 (`nNDSFtrLeanDeclineAlphaTest`). Any other alpha-tested surface therefore still goes to the owner under D5.

Evidence, `hi-capi` (Captain and Pikachu, both high detail):
- **Route 1:** Captain draws lean on 1,868 of 1,868 attempts (`c5`: 0 of 1,868, all `alpha_test`).
- **Route 2:** 1,820 oracle runs against the recorded packets, **0 mismatches**.
- **Digest:** identical.
- **Cost:**

  | route 0 → route 1 | P50 | P95 | P99 |
  |---|---:|---:|---:|
  | FTR | 412,928 → 91,712 | 660,630 → 152,358 | 906,259 → 617,069 |
  | WORK-H | 1,393,056 → 1,073,696 | 1,950,752 → 1,566,640 | 2,563,353 → 2,218,728 |

  Captain HIGH was the most expensive direct draw the old path had left.
- **Captures:** rounding class only (section 4, `captures/cap-c7-hi-capi-*`, `zoom-c7-capi.png`).

## 9. Open items

1. **Ness's yo-yo (USmash/DSmash).** The yo-yo state sets part 30's model part 0 and hides part 3. That gives a 15-root vector the generated plan does not have, so the lean path declines it (`validate`) and the old path draws it with the generic interpreter. The fix is a generated model-part program for Ness in `scripts/fighters/generate_nds_native_owners.py`, then a regen and re-pin. Until then those draws pay a lean event attempt plus the old path: Ness's guard is 12.9-13.2K a draw against 4.6-7.9K for the others.
2. **Production's run texture memo ignores the root program** (section 4). Route 0 shows the body's run-5 texture on Mario's skeleton frames. The fix, the program in the memo's owner key, is an old-path change that also reaches 1P battles, so it is left to the owner. D2 retires the old path.
3. **The old-path plan-key fix (section 6) is shared code.** It also changes 1P battles on skeleton frames, fixing the same stale-plan bug there. If 1P must stay byte-for-byte, the key needs a VS-only gate.
4. **High-detail wide lists** make a slot's next high-detail lists wide (`wide_high`). The other entry is given up while a wide list holds, so a HIGH slot keeps one list and no variants. P95 is still better than route 0 (section 1).
5. **Lab nit.** `oracle_shade_first[5]` still reads the lean site's `reserved[1]` for "tinted". Lean now marks tinted in `reserved[0]`, as the recorder does. The field only latches on a shade mismatch, and there were none; the fix is a one-byte change for the next lab build.

## 10. Files

- **Tools** (`tools/`):
  - builds: `build-s6.ps1`, `build-pre-s6.ps1`, `build-hi-s6.ps1`
  - runs: `run-s6.ps1`
  - gate summaries: `summary6.py`, `tables6.py`, `percost6.py`, `digestseq6.py` (the digest by sequence, for sampler key slips), `ramdelta6.py`
  - captures: `capture-s6.ps1`, `captures-s6.ps1`, `shot-s6.ps1`, `capcmp6.py`, `capclass6.py`
  - gdb diagnostics: `gdbdiag-s6.ps1` with `diag-*.gdb`
  - also `kinds6.py`, `gates6.py`, `listsizes6.py`
- **Runs:**
  - `c6-*` (four-fighter), `c7-hi-*` (two-fighter): `-rows.csv` per frame, `.json` extras, `-run.log`
  - verify arms: `c6-route1-verify`, `c6-mfly-route1-verify`, `c6-cppn-route1-verify`
- **Diagnostics:** `diag-mfpn-other-gdb.log`, `diag-mfpn-prog-gdb.log`, `diag-mfpn-memo-r0-gdb.log`, `diag-dklk-wide-r2-gdb.log`
- **Captures:** `captures/` (pairs, diff masks, zooms), `captures-c7.txt` (per-frame listing), `captures-classified.txt` (clusters and edge share)
- **Tables:** `tables-c6-4fighter.md`, `tables-c7-high.md`, `percost-c6-4fighter.md`, `percost-c7-high.md` (the generated sources of sections 1 and 3)
- **Builds:** `build-*.log` (every build and re-make, with its sha256), `rom-sha256.txt`, `ram-symbols-shipcheck.txt`
- **Earlier iterations:** `iterations/` holds the `c1`-`c5` and `c6-hi` runs and build logs, which led to the fixes above.
