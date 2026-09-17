**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary (or less) if not fixed yet, always using ** ** to encapsulate agent added text.
(Investigation details live in `docs/p2/BUG_NOTES.md`; this queue stays lean.)
**This file is the owner's OPEN-defect list, not a history. Owner, 2026-09-17: "BUGS.md is for my obvious findings, if anything is removed from it, that means it was fixed." So an entry that is GONE was FIXED -- absence is never evidence a symptom was unreal or unreported. Agents annotate in place with ** **; only the owner removes an entry. A board row with no matching entry here is ambiguous, not stale: ask rather than infer.**

treat anything not going through native renderer a failure.

**Every newly built ROM, including diagnostics, must exclude non-native renderer implementations. Runtime defaults and zero observed fallbacks are insufficient.**

Owner notes: This isn't meant to be comprehensive, just my quick observations:

Main Menus:
-1P mode not selectable.
    1P CSS is invisible in smash64ds-p2-shell-hwtri.nds 1P campaign rom (purposeful?).
-Data goes to solid blue screen.
-VS options
    -Items switch missing, should be added now.
    -Damage percentage should go up and down faster, like 3x faster.


-CSS Bugs still present and need to be fixed:
    -Low FPS/Flashing during gate openings **MEASURED, CAUSE FOUND, not yet fixed. 159 of 1,651 CSS frames (9.6%) miss 60 Hz and the worst presents at 6.7 FPS (nine VBlanks). The door slide re-reads the WHOLE 7,738 B panel from NitroFS as an underlay, per sliding slot, per tic, for ~21 tics -- and ndsUiKitBlitSurfaces has no cache, so every call is a real file read. nds_ui_kit.h:216 says outright that one NitroFS open costs more than a whole frame; ndsMenuShellCssStepDoors bypasses the per-frame budget its own neighbour ndsMenuShellCssSyncPanels exists to enforce. Needs a cached panel band or a row-ranged blit.**
    -fighter 3d previews not visible for:
        -Yoshi **FIX IMPLEMENTED, please look. Yoshi's CSS preview pack declared source_bytes=45,488 while his native owner expects asset_data_size=44,256. ndsRendererValidateNativeFighterOwner compares that field BEFORE it looks at a single root, so the owner was declined outright (reject code 3) and a declined owner draws nothing. The 1,232 B difference is the welded pair DLs: Yoshi is the only character-select fighter in OWNER_DL_PAIR_MODE, so he is the only one whose two producers disagreed -- the other eleven match exactly. In-match Yoshi was always fine because the battle pack declares the raw length. Now one shared helper answers both ends and a standing check (check_preview_pack_owner_sizes.py) fails if they ever drift again.**
    -music pauses/reset when rendering new 3d fighter previews (moving around cursor) **MEASURED, CAUSE FOUND, not yet fixed. Eight BGM suspend/resume pairs per CSS visit (suspend=8 resume=8, seammiss=0 error=0 -- balanced, so it pauses rather than dies). The shipping CSS loads a whole fighter closure in ONE frame and fences the music around it; the worst such frame is 4,808,448 ticks = 8.6 frames = 143 ms of silence. A properly sliced loader already exists in this same function but is compiled out -- it sits under #else of NDS_PLAYERS_VS_COMPACT_PREVIEW, so only the oracle/profile build gets it. Slicing the shipping arm removes this and the item below together.**
    -delay between cursor hover and 3d fighter preview rendering. **MEASURED, CAUSE FOUND, not yet fixed. SAME ROOT as the music item. NDS_PLAYERS_VS_PREVIEW_DWELL_TICKS is 13 (~217 ms) and is deliberately sized so a full-speed sweep across the roster never starts a load -- a sound debounce for a BLOCKING load, which is why deliberate hovering feels slow. Measured 70 tics = 1.17 s of pure waiting in one visit. The loads it gates never amortise either: 10 acquires, ZERO cache hits, 7 loads, 7 retires. Slice the load and the dwell can drop.**
    -Kirby not selectable **FIX IMPLEMENTED, verification in progress. Same cause for all three below: the shell roster ladder had rungs for eight fighters, Jigglypuff sat on the unreachable rung 8, and Kirby and Ness had no rung at all. Rung 8 was pulled on 2026-09-04 because the character select then read ten full closures at once and hung in libfat; that note's own release condition ("raise this again once the character select stops loading every roster member at once") was met on 2026-09-09 and the ladder was simply never re-raised. Extended to rung 9 (Ness) and 10 (Kirby); the rung-10 ROM builds clean and plays the character select, but the scripted lap then dies in a wandered ARM9 after leaving it, so the shipping default is NOT raised until that is attributed.**
    -Jigglypuff not selectable **SAME CAUSE AND SAME FIX as Kirby above (rung 8).**
    -Ness not selectable **SAME CAUSE AND SAME FIX as Kirby above (rung 9).**
-Yoshi:
    -Up B egg shells are not rendering.
    -Grab attacks turn yoshi invisible. **FIX IMPLEMENTED, not yet seen on screen. Two Yoshi root programs now carry the 19-root vector that drawing hidden part 4 (joint 9, DL 0x2800) forces: Catch, and Throw which also swaps joint 7 to part 1. Built and linked into smash64ds.nds. Please try a forward and a back throw and say whether Yoshi stays visible.**
    -B attack turns yoshi invisible and egg is also invisible **SAME CAUSE, SAME FIX. EggLay motions 202-206 carry the same 0x18000000, so the Catch program above covers them too. Please try Neutral-B and say whether Yoshi and the egg both draw.**
    -character intro is invisible (egg hatching) **CAUSE FOUND, fix not yet written. It is not the anim-desc mechanism (Appear1/2 carry only 0x40000000, no DL) - I ruled that out and wrongly stopped there. efManagerYoshiEggEscapeMakeEffect (efmanager.c:5441) calls ftParamHideModelPartAll from C, so Yoshi's whole body is hidden ON PURPOSE and the egg is meant to draw in its place - but dEFManagerYoshiEggEscapeEffectDesc draws from gFTDataYoshiModel and has NO native owner, so nothing draws. Yoshi's SHIELD is the same cause (ftcommonguard1.c:391 / ftcommonguard2.c:23, a separate egg, also unowned), so one YoshiModel effect owner fixes both.**
-Link:
    -character intro column VFX should have transparency
    -neutral B makes link invisible when throwing and catching the boomerang
    -Up B effects aren't rending properly
-Pikachu
    -neutral b attack should have transparency/alpha and no hard edges.
    -down B effect doesn't render and sometimes crashes.
    -electric damage effects seem to be missing overall
    -strong side a effects not rendering.
-Samus
    -shield rolling is invisible. **NOT a root-vector bug: RollF/RollB carry only 0x40000000 (TransN, no DL). Cause is elsewhere.**
    -down B is invisible. **NOT a root-vector bug: Bomb is FTANIM_FLAG_NONE and its morph collapse is already baked as MorphUnfold/MorphBall. Cause is elsewhere.**
    -B charge/shots are not rendering over the samus gun, can move Z depth to be infront of gun so that the gun is occluded when charging.
-Samus **(not reported, found by audit): forward smash should make Samus vanish. Now derived rather than suspected — all five FSmash motions carry 0x00180000, which installs drawing hidden parts 11 and 12 (joints 24/25, DLs 0x2c20 and 0x2ce8). The live vector is 16 roots against a canonical 14, and NEITHER offset is resident in either detail, so no owner can match and a declined owner draws nothing. Catch escapes this only because its own motion overrides both joints. A sweep of all 26 fighters found exactly three drawing hidden parts in the game and this was the last uncovered one. FIX IMPLEMENTED — an FSmash root program now carries the 16-root vector, built and linked. Please still confirm on hardware, and note that the useful answer is now the opposite one: if Samus did NOT vanish on forward smash before this, something in the chain is wrong and worth knowing.**
-captain falcon

-General
    -green Impact wave effect is not drawing/occluding properly. It should draw "around" a fighter, so the part of the effect that is behind the fighter gets occluded properly. right now it always draws over a fighter, even the part that should be behind the fighter
    -Score alert counters should render on bottom screen (Score -1, Score +1 etc)

Stages:
-peaches castle: Foreground castle roof renders ALL geometry now but the texture is missing on the now visible geometry. A continuous tiled roof surface is almost achieved.
-Zebes: Acid plane Color is accurate now but texture blending are all visibly too HARD. Edges are too defined instead of a gradient/smooth transistion. Stage lights on the ground floor have a flat/hard transparency (hard upsidown trapezoid shape) instead of looking like a real light source with a gradient that tapers to fully transparent towards the top.
-Mushroom kingdom: Left side platform is now visible but mostly untextured.
-Yoshi's Island: Rotating textures still expose opaque hard edge white texture-card backgrounds instead of using transparency. Cloud platforms are pretty much correct now. sparkle sprites surrounding the central Heart have no transparency and the quads are fully opaque and have hard edges. Main platforms and main floor/path geometry are  completely missing, you can see the background right through it.
-SectorZ: Crashes sometimes during characer intro.
-Saffron city: the pokemon garage door hazard is always open for some reason. It should close and open periodically. Pokemon are missing the VFX for their attacks.

Audio:
