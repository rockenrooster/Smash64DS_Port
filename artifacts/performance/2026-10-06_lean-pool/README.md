# Parked-list pool behind the lean spare (2026-10-06, q23 -> q24)

Lab counters (`build-lab-diag1006`, NDS_DIAG_COUNTERS=1, frame 1,900):

| config | materializations | distinct keys | key events seen before | spare hits | variants learned |
|---|---|---|---|---|---|
| 4 x Yoshi | 75 | 36 | 376 of 412 (91%) | 5 | 49 |
| Peach's Castle | 45 | 26 | 260 of 286 (91%) | 31 | 7 |

So at least 39 (Yoshi) and 19 (Castle) materializations re-derived a list
the slot had held earlier and lost; each costs ~300-500K ticks
(`materialize_ticks` / materializations).

Change (`src/nds/nds_renderer_native_common.c`): a pool of up to eight
fixed slots (one half-region each, ~17.9 KB) from the battle's general heap,
sized once per heap generation at its first use so that the spare floor
(`NDS_FTR_LEAN_SPARE_KEEP_FREE`) and every slot's not-yet-taken spare stay
free. The spare's own list goes to the pool before a swap overwrites it (and
the victim's list, where the heap refused the spare); a miss whose key a
parked copy holds -- by the entry rules: tint set, texture fence, key,
variant records -- copies it back into the victim, re-bases its `words`,
cleans it for the DMA and frees the pool slot. Same lists, same words: render
only, digests identical. The 2026-10-05 lab pool (`../2026-10-05_lean-park`)
replaced the spare where the heap refused it, which is why it did nothing on
Dream Land; this one stands behind the spare.

Heap: Sector Z general-heap free-min at frame 1,900 is 69,456 B with the pool
(147,600 B without; floor 25,600).

Lab four-CPU ROM, one run each; WORK-H (`runsum.py`), paired (`pairab.py`)
against q23 (`../2026-10-06_lean-matkey`):

| config | P50 q23 -> q24 | P95 q23 -> q24 | P99 q23 -> q24 | paired | digest |
|---|---|---|---|---|---|
| gate (Dream Land) | 776,320 -> 775,616 | 1,062,272 -> 1,060,480 | 1,297,344 -> 1,259,904 | -2,496 | identical |
| Castle (gkind 0) | 810,752 -> 808,704 | 1,168,192 -> 1,161,536 | 1,518,144 -> 1,453,248 | -1,984 | identical |
| Saffron (gkind 7) | 849,664 -> 848,128 | 1,181,952 -> 1,175,360 | 1,452,032 -> 1,452,096 | -2,432 | identical |
| Sector Z (gkind 1) | 858,368 -> 856,000 | 1,165,248 -> 1,157,888 | 1,511,936 -> 1,415,808 | -2,240 | identical |
| 4 x Yoshi | 849,664 -> 845,120 | 1,191,040 -> 1,171,008 | 1,434,496 -> 1,352,448 | -4,160 | identical |

Also in q24: `ndsProjectToViewport` reads the camera matrix bits through a
union (its 4-byte `__builtin_memcpy` at a variable column compiled to a real
`memcpy` call, twelve a projection).
