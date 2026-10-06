# 2026-10-05 GX texgen in lean lists; the old executor compiled out (T1, D1)

Owner, 2026-10-05: "Delete Old machinery." Clean four-CPU lab
(`NDS_LAB_FOURCPU_WORDS=1`), items on. Gate = Dream Land gate roster;
`link` = 4 x Link, Dream Land; `sz` = Sector Z gate roster; `yo` = 4 x Yoshi,
Dream Land. WORK-H from the ring dump, frames >= 64 (`runsum.py`).

## T1 (`build-lab-clean1005t1`, base F3b)

The 1P campaign's environment-mapped owners (Metal Mario, the Polygon team)
were the old executor's last regular users: lean declined them on Capacity
(a Polygon: 28 texgen groups, 484 sites). A lean list for an owner at or past
the Metal Mario slot now lets the geometry engine derive the coordinates:
the bind takes TEXIMAGE_PARAM texgen mode 2 (normal source), the corners
carry no TEXCOORD, and each group's texture matrix is patched per frame
(`ndsFighterPacketPatchTexgen`'s hardware branch). The struct holds 32 groups
(was 8); the VS owners keep the CPU path, so their words are unchanged.

| Pair | P50 | P95 | over | digest | paired median |
|---|---|---|---|---|---|
| gate `gate-f3b` -> `gate-t1` | 817,152 -> 818,688 | 1,110,912 -> 1,118,592 | 93 -> 94 | identical | +1.0K |
| 4 x Link `link-f3b` -> `link-t1` | 867,776 -> 869,184 | 1,230,144 -> 1,230,464 | 203 -> 201 | identical | +0.8K |

The cost is the larger list header (+672 B a list) and layout. The late
campaign walk (Metal Mario -> Race -> Polygon team -> Master Hand -> ending)
then drew every fighter lean: 0 declines, 0 skipped draws.

## D1 (`build-lab-clean1005d1`, base T1)

The old fighter executor (`ndsFighterMarioFoxDLAllDrawForSlot` and the
production/hierarchy owner it drives) is compiled only where the lean path is
not (`include/nds/nds_ftr_lean_live.h`); a lean decline now skips the draw
and counts it (`gNdsFtrLeanSkippedDraws`). Text -34.9 KB, BSS -19.2 KB.

| Pair | P50 | P95 | over | digest | paired median |
|---|---|---|---|---|---|
| gate `gate-t1` -> `gate-d1` | 818,688 -> 815,616 | 1,118,592 -> 1,105,344 | 94 -> 88 | identical | -2.4K |
| Sector Z `sz-f3` -> `sz-d1` | 903,168 -> 901,184 | 1,268,992 -> 1,236,672 | 223 -> 201 | identical | -0.4K |
| 4 x Yoshi `yo-f3` -> `yo-d1` | 892,416 -> 892,288 | 1,267,520 -> 1,265,280 | 248 -> 242 | identical | -1.1K |

Sector Z's P95 drop is mostly its tail, not the median (single-run P95
spread is about +/-5K).

The VS Results out-of-memory (Yoshi / Kirby / Captain Falcon / Ness on Dream
Land, halt at tic 120 when the Results audio thread allocates) no longer
reproduces on the walk ROM built from D1: Results ran 400 frames, the arena
low-water 48,532 bytes after the audio thread.
