# 2026-10-05 Yoshi's Island: player 1 dies at match start

Owner (BUGS.md, Yoshi's Island): "at match start, 1P immediately dies at
lower bounds."

Clean four-CPU lab ROM (`NDS_LAB_FOURCPU_WORDS=1`), `gNdsLabFourCpuGkind=5`,
the preset roster (P1 is Donkey Kong). gdb stops at presented frames
(`scratchpad stcap.ps1`; the logs here are its text output).

## Before (`yi-start.txt`, `yi-p1.txt`, `yi-p1b.txt`)

* Map object 0 (P1's spawn) is (629, -96); the floor under it is the main
  stage, line 6, at y = -299.6.
* Frames 3-45 (status Entry): P1 at (629, -244), `floor_line_id` 2.
* Frame 120: status OttottoWait at (-3280, 391), line 2; the cloud yakumono 3
  (at (-3960, 780)) has pressure 150, timer 90.
* Frame 170: Fall, no floor; frames 200-250 a failed recovery; frame 300
  falls = 1. The cloud evaporated under P1 during the countdown
  (`grYosterUpdateCloudSolid`: 120 ticks of standing).
* `yi-p1-win0.txt`: the same with today's collision windows off
  (`gNdsMPSweepSegmentWindow` / `gNdsMPPointSegmentStart` /
  `gNdsMPWallMissOnePass` = 0): not the 10-05 collision change.
* `yi-yak.txt`: the three cloud yakumono DObjs at frame 1 -- status On (1),
  translations (7335, -60), (3855, 1920), (-3960, 780).

## Cause

`ftManagerMakeFighter` projects the spawn onto the floor with
`mpCollisionCheckProjectFloor` (mpcollision.c:2651), which queries each
yakumono group in its own frame: a group whose platform is switched off is
skipped, and a moving one is queried at the point less its DObj translation.
The port's `ndsMPProjectFloorGeometry` read every group's vertices as world
positions. Cloud 3's line, local to its DObj, lay under x = 629 in those raw
coordinates, so P1 was given line 2 at y = -244. The appear's end restores that
floor line (ftcommonentry.c), and the first grounded update walked P1 along
line 2 to its edge, onto the cloud.

## Fix and proof

`ndsMPProjectFloorGeometry` (`src/port/reloc_backend_mp_collision.c`) follows
the source group by group: off platforms are skipped, moving ones are queried
in their frame, and a vertical first-bracketing segment offers no floor (the
source's 0/0). `yi-fix2.txt`, same ROM build with the fix: frame 1 line 6 at
(629, -299.6); frame 110 Wait at the spawn. Official gate (Dream Land) with
this and the day's other fixes: replay digest identical
(`../../performance/2026-10-05_playtest-gate`).
