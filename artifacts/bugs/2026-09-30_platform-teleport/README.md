# A fighter on a platform that switches off teleports down (2026-09-30)

Owner (r59): "Arwing platform mechanics aren't working right: when it does a
barrel roll, instead of the fighter falling off and then falling down, it just
teleports the fighter to the ground immediately. Similar thing happens with
Yoshi's Island's clouds: when the clouds dissipate, the fighter teleports down
a distance and then starts falling instead of starting falling AT the clouds Y
height."

## Mechanism

Both stages switch a moving platform's collision off with
`mpCollisionSetYakumonoOffID`: a Yoshi's Island cloud when it evaporates
(`gryoster.c:138-145`), the Sector Z Arwing when a barrel-roll pattern starts
(`grsector.c:423-450` clear `is_arwing_line_active`; `:1010-1016` switch
yakumono 1 off).

The source then drops a fighter standing on it where it stands:
`mpProcessCheckTestFloorCollisionNew` (`mpprocess.c:915-934`) asks
`mpCollisionCheckExistLineID` about the fighter's floor line, which answers
FALSE once the line's yakumono is Off or Hidden (`mpcollision.c:4094-4113`),
so the fighter takes `mpProcessSetCollProjectFloorID` and falls from its
current position.

The port's `mpCollisionCheckExistLineID` (`src/port/reloc_backend_mp_collision.c`)
checked only that the line id is in the geometry. The switched-off line still
"existed", so the floor test ran on it, failed (the port's floor lookup does
refuse an Off yakumono), and the function's edge branch set
`translate->y = edge.y - map_coll.bottom` from `mpCollisionGetFloorEdgeL/R`,
which report (0, 0, 0) for a switched-off line. The fighter was put at world
y = 0 and fell from there.

## Evidence (four-CPU lab ROM, gdb, `platprobe.ps1`)

A breakpoint on `mpCollisionSetYakumonoOffID` prints every fighter's ground
state, floor line and position, then the same on the next four stage ticks
(`platjump.py` lists the grounded fighters that go airborne; `jumps.txt`).

- Control: lab ROM `264D0B71...` (HEAD `33933c17fca` source).
- Fix: lab ROM `8220F668...` (this change).

| Run | Event | Fighter y: stood -> next ticks |
|---|---|---|
| control, Yoshi's Island, default roster | cloud yakumono 3 off, f=165 | 361.0 -> **0.0**, -3.0, -9.0, -18.0 |
| control, Yoshi's Island, owner roster | same | 361.0 -> **0.0**, -2.4, -7.2, -14.4 |
| fix, Yoshi's Island, default roster | same | 361.0 -> 361.0, 358.0, 352.0, 343.0 |
| fix, Yoshi's Island, owner roster | same, plus clouds at f=1018 and f=1247 | 361.0 -> 361.0, 358.6, 353.8, 346.6 (and 361.0 -> 361.0, 357.0, 349.0, 337.0) |

Sector Z: the Arwing switched off at f=0 (before fighters), 758/795 (owner
roster) and 786/970 (default roster) in both builds; in none of those did a
CPU stand on the Arwing, so the Sector Z case is the same code path but not
observed here. Owner playtest owed.

## Fix

`mpCollisionCheckExistLineID` now answers as the source does: the line's
yakumono (found through the memoised ITCM group lookup, which also validates
the id) must exist and have a status below `nMPYakumonoStatusOff`. The AI
(`ftcomputer.c`), items, weapons and the wall/edge processes ask the same
question, so they also stop treating an evaporated cloud or a rolling Arwing
as solid. The function stays in ITCM (20 -> 64 B; the lab build fits).

Replay digests change wherever a platform switches off with something on it
or an AI consults it (Yoshi's Island, Sector Z, Mushroom Kingdom, Saffron);
comparisons across this commit need a control built after it.
