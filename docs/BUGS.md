**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary (or less) if not fixed yet, always using ** ** to encapsulate agent added text.
(Investigation details live in `docs/p2/BUG_NOTES.md`; this queue stays lean.)

treat anything not going through native renderer a failure.

Owner notes: This isn't meant to be comprehensive, just my quick observations:

Main Menu:
-1P mode not selectable — 1P campaign paused by owner; row waits for 1P to resume.
-VS Mode is perfect and should be used as a reference as well as the main menu for the 
-Data not selectable — Native DATA menu wired 2026-09-07 (three rows, kit font); Characters/VS Record/Sound Test children still to build.


VS mode CSS:
-Yoshi selection not working, upon pressing start to go to SSS it changes to Mario. — Cause found 2026-09-07: walk-build START snapshot forced Mario; now gated to the scripted walk. Build pending.
-Kirby not selectable — Off the shell roster: ten resident fighter closures hang libfat at CSS entry; lazy-load reverted.
-Jigglypuff not selectable — Same roster limit as Kirby.
-Ness not selectable — Same roster limit as Kirby.

For 1P mode CSS, look at the VS Mode CSS since they are VERY similar.

Stages:
-peaches castle: The steep red roof on the upper central tower is missing large portions of its polygon faces. Only several narrow triangular strips/edges render, leaving large transparent holes through the roof. The roof should form a continuous solid red pyramidal/conical surface around the tower. The colors on the geometry that does render appear correct. — Near-plane fan refuted 2026-09-07 (witness 0 at the probe camera); runs all admitted; range-shift path under probe.
-Congo: Cannon Barrel is now not rendering (Invisible, its the barrel with the directional arrow on it). It should only rotate locally on its own axis, while translating horizontally under the stage at a fixed vertical height. There is another unrelated barrel on the left side of the map with no arrow drawn on it.
-Hyrule: stage edge geometry seems to be drawing over stage front face geometry instead of the other way around, maybe backface rendering turned off fixes it? Tornadoes seem to do too much DMG and throw character horizontally. — Range-run scale repaired 2026-09-07 (matrix shift). Back-face draw-through **FIXED** (packets now enter with back-face culling like the N64). Tornado **FIXED** 2026-09-07: descriptor was read four bytes before the map data; now damage 14, angle 90 at file offset 0xBC (hit trace on a live tornado still owed). backface/depth rendering error at geometry boundaries:
Back faces from adjacent stage meshes appear to be rendering through or over front-facing geometry. This is especially visible where green roof sections intersect the white rooftop and gray castle walls: rear/underside triangles become visible on top of surfaces that should occlude them, producing large triangular wedges and incorrect edge boundaries. This looks less like missing or misaligned geometry and more like a backface culling / polygon winding / depth-order issue at intersecting meshes.
-Zebes: crashes. — Crash **FIXED** 2026-09-07 (animation ledger overflowed; enlarged, 450-present run clean). Acid overdraw **FIXED** (actors kept Z-buffer; cliffs occlude the pool). Rising-shape look still to check. Not sure how the describe how the acid is currently behaving, but it should behave like a horizonal DMG plane that moves vertically up and down, but the visual we see doesn't match up at all, it currently looks more like a sphere shape instead of a flat 3d horizontal plane. by the time it goes high enough to damage a player, the player is far beneath and submerged in the acid already. Check source behavior.
-Mushroom kingdom: music sounds garbled. big side platform geometry still missing. Low FPS (20FPS), check native rendering. — Music now PCM16 (IMA encoding was the garble); zero seam misses through 600 presents on 2026-09-07; owner to listen. Side platforms admit; loss is at draw time. 20 FPS open (Yoshi's Island reads 19.9 too).
-Yoshi's Island: stage textures that should use transparency/cutout rendering are displaying as opaque quads. Transparent texels appear as solid white and/or black, producing obvious rectangular backdrops around decorative elements and side platforms. The issue is especially visible on the floating platforms to the right and the central decorative/effect textures, which should render with clean cutout edges instead of boxed-in opaque backgrounds. Platforms and main floor path geometry missing. spinning sprites and heart sparke sprites have no transparency. — Clouds on their native arm since 2026-09-07; graded-alpha upload written but not yet reached at the bind (investigating). Floor and platforms admit and draw at GO! in today's capture (yoster-w1); owner to point at the missing surface. Effect sprites: per-stage particle banks pending. 
-SectorZ: Arwing hazards not working right or not following correct paths. — Candidate 2026-09-07: flight-path SYInterp descriptor lanes fixed in the event32 normalizer; probe pending.
-saffron city: transparency around the pokemon garage door hazard thing is missing. There is a white wall that covers the bottom part of screen at wide carmera views (slike when camera shows whole stage when fighters are far apart). — Gate draws on its actor arm since 2026-09-07; door alpha investigating. White wall is the source's own layer-3 haze panel below y -888 (drawn on N64 too); owner to compare with an N64 capture.


