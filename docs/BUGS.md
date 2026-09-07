**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary (or less) if not fixed yet.
(Investigation details live in `docs/p2/BUG_NOTES.md`; this queue stays lean.)

Owner notes: This isn't meant to be comprehensive, just my quick observations:

Main Menu:
-1P mode not selectable — 1P campaign paused by owner; row waits for 1P to resume.
-VS Mode is perfect and should be used as a reference as well as the main menu for the 
-Data not selectable — Records/Data screen is P2 step 7 (modes and meta); not started.


VS mode CSS:
-Yoshi CSS 3d preview freezes after selected.
-Kirby not selectable — Off the shell roster: ten resident fighter closures hang libfat at CSS entry; lazy-load reverted.
-Jigglypuff not selectable — Same roster limit as Kirby.
-Ness not selectable — Same roster limit as Kirby.

For 1P mode CSS, look at the VS Mode CSS since they are VERY similar.

Stages:
-peaches castle: BG needs slightly more scale i can sometimes see the edges of the BG during normal gameplay when falling off of map. missing some Geometry on the steeply sloped castle roof still, colors are correct though. 
-Congo: BG needs slightly more scale i can sometimes see the edges of the BG during normal gameplay when falling off of map. Barrel rotation doesn't rotate in place, instead it rotates around the world origin so the barrel goes around the whole map when it rotates instead of just rotating in local space.
-Hyrule: BG needs slightly more scale i can sometimes see the edges of the BG during normal gameplay when falling off of map. Missing geometry was added, but the existing geometry's scale was messed up and needs to be fixed.
-Zebes: BG needs slightly more scale i can sometimes see the edges of the BG during normal gameplay when falling off of map. Acid doesn't look right (wrong color and wrong geometry).
-Mushroom kingdom: BG needs slightly more scale i can sometimes see the edges of the BG during normal gameplay when falling off of map. music is garbled. Missing geometry was added, but the existing geometry's scale was messed up and needs to be fixed.
-Yoshi's Island: BG needs slightly more scale i can sometimes see the edges of the BG during normal gameplay when falling off of map. no side clouds. Platforms and main floor path geometry missing. spinning sprites and heart sparke sprites have no transparency.
-SectorZ: BG needs slightly more scale i can sometimes see the edges of the BG during normal gameplay when falling off of map. missing map geometry was added but some of it was at the wrong scale, arwing hazards not working right or following correct paths. 
-saffron city: BG needs slightly more scale i can sometimes see the edges of the BG during normal gameplay when falling off of map. missing map geometry was added but some geometry is still missing or is at the wrong scale. transparency around the pokemon garage door hazard thing is missing.

treat anything not going through native rendering a failure.
