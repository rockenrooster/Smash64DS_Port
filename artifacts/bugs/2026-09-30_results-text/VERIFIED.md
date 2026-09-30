# Results bottom text: integrated repair, verified on r58 (2026-09-30)

Candidate `builds/p2p8-playtest-r58/smash64ds.nds`, SHA-256
`2C22DE8E17D5D016459F868D4D56D261AE64929985E4C3A504DC326F39C56439`
(HEAD `f40c5623211` plus this repair and the menu FGM update). The repair is
the investigator's proposal (`proposed-fix.diff`) with three integration
changes in `src/nds/nds_results_oam.c`:

- palette staging copies only the allocated banks, as whole words, by CPU in
  `ndsResultsOamCommit` (after the VBlank wait), instead of a DMA of all 16
  banks (other OBJ owners' banks stay untouched; no DMA channel contention);
- a dirty flag commits a palette change even on a frame with no OAM change;
- a second tint over already-tinted sprites is counted
  (`gNdsResultsOamTintStacked`) instead of re-tinting from the untinted
  colours; the source schedule never produces one.

Captures: the investigator's driver (`builds/codex-results-text/run.ps1`,
slot 0, natural input, Mario/Kirby/Fox/Yoshi, Castle, one-minute time match),
no index/priority probes.

| run | tic | bottom text | capture |
| --- | --- | --- | --- |
| FFA | 121-160 | YOSHI WINS! full brightness | `r58-text-ffa-tic121.png` .. `tic160.png` |
| FFA | 200 | YOSHI WINS! visible, dimmed by the tint (r57: gone) | `r58-text-ffa-tic200.png` |
| FFA | 300 | YOSHI WINS! visible, dimmed; statistics and badge in source colours (r57: gone) | `r58-text-ffa-tic300.png` |
| No Contest | 2, 10 | NO CONTEST | `r58-text-nc-tic002.png`, `tic010.png` |
| No Contest | 40, 100 | NO CONTEST visible, dimmed (r57: gone) | `r58-text-nc-tic040.png`, `tic100.png` |

Hardware OAM (`analyze-oam.py r58-text-ffa`): indexed foreground ids 93..115
(60..115 at tic 300), tint cells 116..127 only; palette banks 0-5 before the
tint, `[0, 5, 6, 7, 8, 9]` after it (banks 6-9 the tinted copies), 10 of 16.

Not covered here: Teams / Stock result layouts and the full twelve-name set,
a second Results visit; the owner playtest of r58 is the acceptance for those.
