# 2026-10-05 Lean parked-list pool (measured, parked: no RAM to give it)

**Idea.** A slot's lean states outnumber its two entries. Sector Z's sweep
match materializes 80 lists (~470K ticks each); 53 of its top 98 frames hold
one. The per-slot heap spare is refused there (the battle's free bytes sit
under its 62,020-byte keep-free floor). A pool of compact slots outside the
general heap holds evicted entries for any battle slot -- entry state, packet,
words (+ Link's texgen site map) and variant records, copied out compactly --
and a miss whose key a parked copy holds copies it back in (re-basing
`words`) instead of materializing. Matching uses the entry rules (tint set,
texture fence, key, variant records). Code: `lean-park-pool.patch` (against
`ed44b7a1bc0`'s `src/nds/nds_renderer_native_common.c` and
`src/nds/nds_audio_fgm.c`); the lab lender was the FGM arena tail a lowered
`gNdsAudioFgmArenaLimit` leaves unused.

**Runs** (clean ROM `build-lab-clean1005k`, same ROM; replay digest identical
in every pair -- row sequences equal, ring seams fell at different rows):

| Pair | P50 | P95 | over 1,120K | revivals / parks |
|---|---|---|---|---|
| Sector Z, FGM 160K -> 64K, no pool (`szbase` -> `szp0`) | 913,280 -> 912,256 | 1,289,344 -> 1,269,184 | 226 -> 221 | -- |
| Sector Z, pool off -> 6 x 16 KB (`szp0` -> `szp16`) | 912,192 -> 911,872 | 1,273,408 -> **1,236,928** | 220 -> 203 | 45 / 76 |
| Dream Land, pool off -> 6 x 16 KB (`dlp0` -> `dlp16`) | 824,000 -> 823,232 | 1,126,144 -> 1,122,560 | 100 -> 99 | 35 / 63 |

**Why it is parked.** The pool pays where the general heap is tight and the
spare is refused (Sector Z -36.5K), and nearly nothing where the heap has room
and the spares already serve (Dream Land -3.6K). Tight-heap stages have no
96 KB to give it (`heap-overlay-probe.txt`, frame 100 / 1,900):

- general-heap free-min: Sector Z 91,348 / 65,208; Dream Land 246,364 /
  182,744 (same roster: the 118 KB is stage);
- the front-end overlay loan VS battles borrow is 88,256 B and full (249 KB
  spills into the heap);
- the FGM arena cannot lend: the four-CPU stress pins up to 133,424 B of its
  163,840 (`include/nds/nds_audio_fgm.h`), and a cue that finds no span fails
  to play. (64 KB did not change direct reads -- 225 vs 220 -- but play
  failures were not counted.)

Revisit when deleting replaced machinery frees main RAM, or if a stage's
heap low-water can carry a 2-3 slot pool.
