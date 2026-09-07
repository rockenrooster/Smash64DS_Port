**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary (or less) if not fixed yet.
(Investigation details live in `docs/p2/BUG_NOTES.md`; this queue stays lean.)

treat anything not going through native renderer a failure.

Owner notes: This isn't meant to be comprehensive, just my quick observations:

Main Menu:
-1P mode not selectable — 1P campaign paused by owner; row waits for 1P to resume.
-VS Mode is perfect and should be used as a reference as well as the main menu for the 
-Data not selectable — Native DATA menu wired 2026-09-07 (three rows, kit font); Characters/VS Record/Sound Test children still to build.


VS mode CSS:
-Yoshi selection not working, upon pressing start to got to SSS it changes to Mario.
-Kirby not selectable — Off the shell roster: ten resident fighter closures hang libfat at CSS entry; lazy-load reverted.
-Jigglypuff not selectable — Same roster limit as Kirby.
-Ness not selectable — Same roster limit as Kirby.

For 1P mode CSS, look at the VS Mode CSS since they are VERY similar.

Stages:
-peaches castle: missing some Geometry on the steeply sloped castle roof still, colors are correct though. 
-Congo: Cannon Barrel is now not rendering (its the barrel with the directional arrow on it). — Barrel draws on its native arm but sits still at the spawn platform; its path script investigating (2026-09-07). Cannon Barrel rotation doesn't rotate in place, instead it rotates around the world origin so the barrel goes around the whole map when it rotates instead of just rotating in local space. 
-Hyrule: stage edge geometry seems to be drawing over stage front face geometry instead of the other way around, maybe backface rendering turned off fixes it? Tornadoes seem to do too much DMG and throw character horizontally. — Range-run scale repaired 2026-09-07 (matrix shift); depth order and tornado contract investigating.
-Zebes: crashes. Not sure how the describe how the acid is currently behaving, but it should behave like a horizonal DMG plane that moves vertically up and down, but the visual we see doesn't match up at all, it currently looks more like a sphere shape instead of a flat 3d horizontal plane. by the time it goes high enough to damage a player, the player is far beneath and submerged in the acid already. Check source behavior.
-Mushroom kingdom: music sounds garbled. big side platform geometry still missing. Low FPS (20FPS), check native rendering. 
-Yoshi's Island: side cloud sprites/textures have no transparency. Platforms and main floor path geometry missing. spinning sprites and heart sparke sprites have no transparency. — Clouds on their native arm since 2026-09-07; cloud alpha, floor geometry investigating; effect sprites need a larger particle atlas (decision pending).
-SectorZ: Arwing hazards not working right or not following correct paths. — Candidate 2026-09-07: flight-path SYInterp descriptor lanes fixed in the event32 normalizer; probe pending.
-saffron city: transparency around the pokemon garage door hazard thing is missing. There is a white wall that covers the bottom part of screen at wide carmera views (slike when camera shows whole stage when fighters are far apart). — Gate draws on its actor arm since 2026-09-07; door alpha and the white wall investigating.


