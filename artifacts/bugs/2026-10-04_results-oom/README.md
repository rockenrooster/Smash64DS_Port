# VS Results freeze after a 4P Sector Z match (owner r75) — 2026-10-04

Owner row: Sector Z, four level-9 CPUs (Ness/Kirby/Fox/Yoshi), all items very
high, 2-minute match: "Results screen also froze afterwards after extended
runtime."

Probe: scratchpad `vsfx.ps1 -Gkind 1 -K0 11 -K1 8 -K2 1 -K3 6 -Toggles -1
-Rate 5 -Minutes 2 -StayResults` (VS walk ROM, descriptor poked at
`scVSBattleStartScene`; breaks on `ndsSyMallocOverflowHalt` and exceptions).

## Two faults, in order

1. **Tied places read past an array.** `mnVSResultsGetSpot` indexes
   `aheads[place - players_ahead]`; `place` is a dense rank, so with ties
   above a player the index goes to -1..-3. IDO lays the frame out so that
   read lands in `places[4..2]` (all 1); GCC put a padding word there, the
   spot came back as garbage and `mnVSResultsSetPlayerTagPosition`
   data-aborted. Fix: the call resolves to an N64-layout copy
   (`src/import/battleship_mnvsresults_spot.c`, one table holding both arrays
   in IDO's order; the source definition is declared weak).
2. **The arena ran out at Results tic 120** making the audio thread's 4 KiB
   stack: `HALT malloc-overflow scene=24 ... free=556` in
   `gcAddGObjProcess` from `mnVSResultsMakeAudioThread` (walk-all3). Census of
   the scene's large allocations: Results files 130 KB, EFCommon effects
   94.7 KB, four figatree heaps x 11,792 B, transition file 47,600 B, and the
   two display lists at their source 20,000 B each. Results' display procs
   only write state and fill words (the fighters draw natively): measured
   peaks DL0 848 B, DL1 144 B. The rebudget now gives Results 4,096 + 512 B,
   32,768 B back with ~4x headroom.

## After (sz4p04, walk-all4)

The full match plus sudden death (presented-frame STAT lines: 0 native
failures, 0 particle rejects, 0 quad misses), then `RESULTS reached`, scene 24
ran to its end and the walk went on to the VS character select (scene 16). No
halt.
