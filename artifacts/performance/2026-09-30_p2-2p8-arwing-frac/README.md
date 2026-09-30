# Sector Z Arwing: the flight table (2026-09-30)

## Why

Owner (2026-09-30): the Arwing is what drives Sector Z's P95. It is a moving
platform that fires lasers, so its TraI path is gameplay state on every tick.
While it flies (about 40-45% of a match) it costs ~230K ticks a frame: ~104K
in syInterpGetFracFrame's arc-length bisection (after `bed131310f8`, ~62
quartics a call through libgcc) and ~125K drawing its eight FoxSpecial3
roots. The bisection is exact float work; no cheaper exact arithmetic exists
(see `../2026-09-30_p2-2p8-interp-kernel/README.md`).

But the Arwing flies one of eight authored patterns (grsector.c
`dGRSectorArwingSectorDescs`), each started from frame 0 by
`grSectorArwingAddAnim`, so every flight of a pattern asks the same
(segment, t) questions in the same order.

## Change

- `src/import/battleship_sys_interp.c`: after the memo misses, the call looks
  up (segment hash h1/h2, t's bits) in the flight table. The key is the memo's
  own: h1/h2 hash every value syInterpGetFracFrame reads (kind, point count,
  segment id, length, the segment's two keyframes, its five quartic
  coefficients). An entry is used only on an exact three-way match; anything
  else is computed as before, so the table cannot change a result. Same-ROM
  A/B word `gNdsArwingFracTable` (0 = never consult it).
- `ndsGRSectorSetupInitAll` loads `nitro:/stages/sector_arwing_frac.bin` into
  the scene heap (residency keyed on `gNdsTaskmanHeapGeneration`), only when
  96 KB stays free after it -- the checked allocator halts on an overflow.
- `assets/stages/sector_arwing_frac.bin` (tracked) is staged into NitroFS by
  the Makefile when `NDS_P2_STAGE_SECTOR=1`.

## Capture (reproducible)

`NDS_INTERP_FRAC_CAPTURE=1` (lab only, with the four-CPU lab target) records
every memo-path call's h1, h2, t and result, cache-flushed so the debugger
reads what the CPU wrote. `tools/arwcap.ps1` forces a flight pattern at flight
start (`func_ovl2_80107D50`), shortens the wait to the next flight, and dumps
the buffer; one run per pattern, owner roster (Kirby/Fox/Yoshi/Pikachu):

| pattern | calls | | pattern | calls |
| --- | ---: | --- | --- | ---: |
| 0 | 1,200 | | 4 | 750 |
| 1 | 950 | | 5 | 1,200 |
| 2 | 4,243 | | 6 | 1,200 |
| 3 | 3,200 | | 7 | 1,200 |

`scripts/stages/generate_sector_arwing_frac.py` merges the dumps (it refuses
two results for one input): 13,943 calls -> **12,663 entries in 126 segments,
103,336 bytes**. Patterns 5-7 evaluate three tracks at one t per tick.

## Results (lab ROM `build-p2p8-lab-cw`, Sector Z, run-s6, frames >= 64)

Table v0 (pattern 2 truncated at 4,096 of its 4,243 calls; the rest computed):

| arm | roster | table | P50 | P95 | P99 |
| --- | --- | --- | ---: | ---: | ---: |
| `tab0_sz` | owner | 0 | 1,071,168 | 1,453,440 | 1,663,424 |
| `tab0b_sz` | owner | 0 | 1,071,296 | 1,454,784 | 1,663,808 |
| `tab1_sz` | owner | 1 | 1,038,272 | 1,389,632 | 1,587,648 |
| `tab1b_sz` | owner | 1 | 1,038,208 | 1,387,648 | 1,584,512 |
| `tabd0_sz` | lab default | 0 | 1,141,056 | 1,553,216 | 1,887,168 |
| `tabd1_sz` | lab default | 1 | 1,075,520 | 1,474,624 | 1,825,024 |

Owner roster P50 -33K, **P95 -65.5K**, P99 -77K; the lab default roster P50
-65.5K, **P95 -78.6K**. Every one of the owner match's 1,950 calls was
answered by the table (2,766 of 2,912 with the default roster; the rest are
Samus's rolls). Oracle (`tabo_sz`, the source run beside every call): 1,950
compares, **0 mismatches**. Replay digest table 0 vs 1: IDENTICAL for both
rosters. Native failures 0 (owner) / 119 (default, the stage's standing
count) in both arms.

Table v1 (committed, all 12,663 entries), oracle on both rosters (`tavo_sz`,
`tavdo_sz`; the oracle's own cost inflates their timings): owner roster 1,950
compares, 1,950 table hits, **0 mismatches**; default roster 2,912 compares,
2,766 table hits, **0 mismatches**. Low-water 99,336 / 59,600.

Heap: free at load ~796 KB; the table costs ~102 KB for the match. Low-water
100,552 (owner roster) and 60,784 (default roster), both with the table
loaded; the 25,600 GObj-cap floor holds. If the heap is needed elsewhere, the
next step is to hold only the active pattern (largest 34 KB), read
asynchronously when grSectorArwingUpdateWait picks it; whether the lead
before the flight's first call covers the read is not measured yet.
