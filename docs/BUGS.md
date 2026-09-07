**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary (or less) if not fixed yet.

Owner notes: This isn't meant to be comprehensive, just my quick observations:

Main Menu:
-1P mode not selectable
-VS Mode is perfect and should be used as a reference as well as the main menu for the rest of the menus and how they should run.
-Option:
	-Option menu is super slow FPS and looks different than the VS mode menu.
	-The 3 Option menu buttons don't actually need to do anything except for the Backup Clear option
	Backup clear option menu is also slow
**Owner visual acceptance (2026-09-06):** VS Options, Option and Backup Clear look good. Cadence and save-behavior verification remain separate.
-Data not selectable

VS Mode Menu:
-VS Options crashes — **Candidate verified:** native round trip passes; full regression and cadence acceptance remain.

VS mode CSS:
-Link is only a torso and one arm, like captain falcon used to be at first — **Candidate repaired:** camera-correct reflection state restores full native CSS model; widest verification remains open.
-Yoshi is invisible — **Candidate repaired (2026-09-06):** three seams. (1) `FTCommonPart.flags` sat in the high lane after the file's u32 swap, so every Yoshi joint dispatched as case 0 and drew nothing; `ndsRelocNormalizeFighterCommonPartFlags` restores the byte (host test `test_common_part_flags.py`). (2) The bake published the compiler-extended payload size (0xb1b0) and synthetic weld offsets; it now publishes the raw 0xace0 and the source post-list identities (`test_native_root_identity.py`). (3) With case 1 live, each pre-matrix `dls[0]` became its own contract event: `gcPrepDObjMatrix` now credits it to the joint it precedes by identity, and the collection folds it into that joint's welded post root. Natural CSS selection validates 18/18 roots and native production returns success; evidence `artifacts/performance/2026-09-06_css-yoshi-native/`, `artifacts/visibility/2026-09-06_css-yoshi-pre-list.png`. Battle/stress acceptance and Win2 pose coverage remain open.
-Pikachu is missing ears — **Candidate repaired (2026-09-06):** not matrices, topology or admission (GX clip matrices for the ear roots equal head × ear offset; prepared vertices match the bake; a cull-none poke made the ears appear as back faces). The ears are a 12x1 CI4 ramp tile (`SETTILESIZE` lrs 11, lrt 0, both axes `G_TX_CLAMP`) uploaded as 16x8; the bilinear texcoord offset puts every ear texel at t≈1.1, which the RDP clamps to row 0 but the DS reads from the zeroed padding row, and the PAL16 packer publishes zero as transparent colour 0. `ndsRendererHardwareReplicateClampPadding` now replicates the tile edge across the padding of a clamped axis. Native CSS ears render with black tips: `artifacts/performance/2026-09-06_css-pikachu-ears/` (before / generic control / after). Battle acceptance and a wider texture regression pass remain.
-Kirby not selectable
-Jigglypuff not selectable
-Ness not selectable

For 1P mode CSS, look at the VS Mode CSS since they are VERY similar.


Stages:
-**Missing BG on all eight (candidate repaired 2026-09-06):** every stage wallpaper container shares Dream Land's layout, but only Dream Land's Sprite header was byte-lane normalized after load (`ndsRelocNormalizeStageDreamLandSprite` keyed on one asset id), so the other eight failed the shape gate and seeded nothing. The normalizer now covers every wallpaper asset id. Castle and Hyrule seed READY (state 2, 42,834 opaque pixels, distinct hashes) and draw their skies: `artifacts/visibility/2026-09-06_stage-wallpaper-{castle,hyrule}.png`; probe `builds/resume-20260905/stage-qa/stage-wallpaper-probe.ps1`. Remaining six stages and battle-wide acceptance still to be photographed.
-general: "1P" "2P" "3P" "4P" and "CP" tags are going through slow renderer.
-peaches castle: Missing BG and some geometry on the steeply sloped castle roof. and seems to be going through the slow renderer (sub 15FPS)
-Congo: Missing BG, and Barrel not following correct path, it sometimes circles world origin, when it should ONLY be on a horizontal path below the stage. Moving platforms in the middle of the stage do not move or work, fighters just fall right through them. seems to be going through the slow renderer (sub 15FPS)
  **Moving platforms candidate repaired (2026-09-06):** the layer-1 AObjEvent32 script plans 509 commands and `NDS_AOBJ_EVENT32_PLAN_MAX` was 128, so the normalizer rejected it and the yakumono DObjs never animated (speeds stayed zero, no ride). Capacity raised to 640 with a plan high-water witness; platforms now translate and `gMPCollisionSpeeds[1]` is nonzero. Barrel path and native admission (reject reason 6 after the descriptor preload) remain open.
-Hyrule: Missing BG, no tornadoes, missing middle tower geometry and right small tower geometry. seems to be going through the slow renderer (sub 15FPS).
-Zebes: Missing stage geometry and BG. Acid doesn't look right (wrong color and wrong geometry). seems to be going through the slow renderer (sub 15FPS).
-Mushroom kingdom: no BG, no middle platforms visible, no side platforms visible, music is garbled, pihrana plants garbled. cannot use warp pipes. seems to be going through the slow renderer (sub 15FPS).
-Yoshi's Island: no BG or side clouds. seems to be going through the slow renderer (sub 15FPS).
-SectorZ: no BG, no map geometry, arwing hazards not working right. seems to be going through the slow renderer (sub 15FPS).
-saffron city: no BG, missing lots of map geometry. missing moving platforms. seems to be going through the slow renderer (sub 15FPS).

I'm getting sick and tired of the slow generic renderer being used at all for anything in game matches where the actual game is played, I'm contemplating removing support for it since it keeps being used for performance critical rendering.


I'm seeing a blue "A" button and a green "B" button during gameplay, it looks like it should only be seen when paused.