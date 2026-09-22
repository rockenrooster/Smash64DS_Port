**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 100 char summary (or less) if not fixed yet, always using ** ** to encapsulate agent added text.
(Investigation details live in `docs/p2/BUG_NOTES.md`; this queue stays lean.)
**Owner: removal from this file means FIXED; absence never means a symptom was unreal.**

treat anything not going through native renderer a failure.

**Every newly built ROM, including diagnostics, must exclude non-native renderer implementations. Runtime defaults and zero observed fallbacks are insufficient.**

**09-22: play r36 `1CECBEF5`. 16 FIXED, 2 deferred, 1 open. Detail in docs/p2/BUG_NOTES.md**
**Earlier arena alarm withdrawn: measured free-min 48,216, GObj latch never fires.**

Owner notes: This isn't meant to be comprehensive, just my quick observations:

Main Menus:
-Make "Characters" and "VS Record" not selectable, "Characters" and "VS Record" work is Owner deferred. **FIXED**
-VS options
-CSS Bugs still present and need to be fixed:
    -delay between cursor hover and 3d fighter preview rendering. **Owner deferred. 5 of 12 previews draw within 24 tics; the 7 slow ones are named in BUG_NOTES.**

-Yoshi:
    -Yoshi's guard/shield (egg) is invisible **FIXED**
-Link:
-Pikachu
    -neutral b attack VFX that walks terrain still has hard edges. **FIXED**
    -down B effect doesn't render all related VFX, missing blue exp on pikachu. **FIXED**
    -Pokeball Spawn Intro not playing VFX. **FIXED: the item ball's ray call was commented out; not a render fault.**
    -face color is slightly different from body color. (lighting difference???) **FIXED: packet replay re-derived the shade word without the clamp.**
    -when I choose pikachu, in match, fox opponent is frozen and cannot be hit. Sudden death works funnily enough though. **OPEN. Alive + hittable over 5 matches here; can't reproduce. Need stage, settings, does it animate?**
-Samus
-Kirby
    -Kirby face color is slightly different from body color. (lighting difference??? pink face color looks more correct I think...) **FIXED with the Pikachu face/body row.**
    -Neutral A punch flurry VFX not drawing at correct locations. **FIXED in r26**
    -Kirby neutral B , then A attack to spit out fighter, Star projectile VFX is invisible. **FIXED**
    -Kirby has a wierd pose on results screen. **FIXED**
-Jigglypuff
    -face color is slightly different from body color. (lighting difference???) **FIXED with the Pikachu face/body row.**
-Captain falcon
-Ness
    **FIXED: hovering Ness at the CSS hung the ROM; a stale header truncated its owner image.**
-General
    -All Fighters are not doing correct poses/animations on results screen **FIXED**
    -No contest results screen, all fighters should be doing the clapping animations. **FIXED with the poses row.**
Stages:
-peaches castle: 
    -Foreground castle roof renders ALL geometry now but the texture is missing on the now visible geometry. A continuous tiled roof surface is almost achieved. **Owner Deferred**
-Zebes: Acid plane Color is accurate now but texture blending are all visibly too HARD. Edges are too defined instead of a gradient/smooth transistion. Stage lights on the ground floor have a flat/hard transparency (hard upsidown trapezoid shape) instead of looking like a real light source with a gradient that tapers to fully transparent towards the top. **Owner Deferred**
-Yoshi's Island: 
-SectorZ:
-Saffron city: the pokemon garage door hazard is always open for some reason. It should close and open periodically. **FIXED: gate replayed a baked constant matrix, frozen closed; now cycling.**

Audio:
