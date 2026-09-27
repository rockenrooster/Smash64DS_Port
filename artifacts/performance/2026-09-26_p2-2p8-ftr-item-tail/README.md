# P2-2p8: lean FTR tail regression from the compact packet layout (2026-09-26)

**Outcome: BANKED.** The four-CPU stress WORK-H P95 regression between slice 6/7
(~2.02M) and the Phase 2/3 checkpoints (~2.6M) was one change: `c116fffa03e`
shrank each fighter's packet region from 8,840 to 6,528 words to give the
compiled stage GX body a 36,992-byte tail of `gSYFramebufferSets`. Reverting
that layout (stage body back in the general heap) returns FTR to its slice 6
behaviour exactly and moves WORK-H P95 **2,626,368 -> 1,955,392**.

## Mechanism

A lean entry is half a region minus the 960-word `NDSFighterPacket` header.
At 6,528 words that is 2,304 list words; this roster's low-detail lists are
Donkey 2,467, Samus 2,313, Link 2,634, Kirby 1,403. Every draw in the stress is
low detail, so for three of four fighters:

- each materialization walked into the half entry, failed Capacity near the end,
  then walked again wide (`ndsFtrLeanMaterializeFor`; `wide_high` is only
  remembered for high detail): per-materialization cost Link 880,828 ->
  1,485,278, Donkey 543,687 -> 981,770 ticks;
- a wide list keeps no variant records and no second entry: Link learned 0
  variants (was 6, 69 switches) and materialized 78 times (was 15); Donkey 26 -> 47.

Totals: materializations 62 -> 147, variant switches 144 -> 75, entry hits
174 -> 89, event ticks 40.3M -> 173.2M. The code comment's premise (HIGH census
peak 3,754 < 5,568 wide words) was true but irrelevant: capacity is per half.

Ruled out on the way (same match, same ROM family): NDL runtime (`cur-ndl0`,
`gNdsP2Ndl=0` at boot), ITCM placement (symbol diff), texture fences (fence
re-records unchanged at 11).

## Change

- `nds_renderer_preamble.c`: one 8,840-word region per slot; compact layout,
  `ndsRendererStageGxBuffer` and the region-size switch deleted.
- `nds_stage_gx.exec.inc`: body from the general heap only, when free >= body +
  25,600 floor + 36,420 B (the heavy roster's measured in-match growth after the
  first update, phase3-residency probe 92984); otherwise decline and draw the
  stage uncompiled. The previous heap margin (25,632) ignored later growth.
- `compile_nds_stage_gx.py`: the 36,992 B body cap is now a heap ceiling (name
  and message only; output bytes unchanged; 13 host tests pass).

## Measurement (canonical four-CPU stress, route 1 / admit 2, same build dir)

Control `0BD4523E` (A8 BGM, `builds/p2p8-cur-0bd4523e/`), candidate `BC3500EA`
(control + this change). Both `2ab84bdc435` + the A8 working tree.

| | control | candidate |
|---|---:|---:|
| WORK-H P50 / P95 / P99 | 1,343,808 / 2,626,368 / 3,379,648 | 1,332,160 / 1,955,392 / 2,786,944 |
| FTR P50 / P95 | 184,640 / 1,204,992 | 185,984 / 269,696 |
| STG P50 / P95 | 263,616 / 271,360 | 263,040 / 271,296 |
| FPS (mean ALL) | 18.96 | 19.72 |
| VBlanks 2/3/4/5+ (max) | 268/1372/183/150 (11) | 274/1433/212/54 (10) |
| two-VBlank share | 13.58% | 13.89% |
| materializations / variants | 147 / 6 | 62 / 12 |
| native failures | 0 | 0 |
| stage program | tail, loaded | heap, 1 load, 32,140 B, 0 declines |
| general heap low-water (lab) | 122,412 | 122,412 |

Replay digest IDENTICAL over all 1,972 frames (`compare-replay-digest.py`).
The candidate's lean counters equal slice 6's `s6-route1` exactly (62
materializations, 12 variants, 144 switches, rejects [8, 42]).

**Not measured:** the shipping heavy roster. Its last measured first-update
free was 46,092 B (`FB299B25`, before A8's +16,384 B), below any body + 62,020,
so there the stage program is expected to decline (uncompiled stage, higher
STG) instead of using the tail. That roster is RESERVE_RED and fails hat
admission regardless; Phase 3 RAM (FGM cache, MF) is what funds it.

## Files

`cur-route1*` control, `heap-route1*` candidate, `s6-route1*` slice 6 ROM
(`531E1B4B`, same match), `cur-ndl0*` control with NDL off. Tools: `run-s6.ps1`
(route/admit/boot pokes, lean counters), `kinds6.py` (per-kind lean table).
