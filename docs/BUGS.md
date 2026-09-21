**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 100 char summary (or less) if not fixed yet, always using ** ** to encapsulate agent added text.
(Investigation details live in `docs/p2/BUG_NOTES.md`; this queue stays lean.)
**Owner: removal from this file means FIXED; absence never means a symptom was unreal.**

treat anything not going through native renderer a failure.

**Every newly built ROM, including diagnostics, must exclude non-native renderer implementations. Runtime defaults and zero observed fallbacks are insufficient.**

Owner notes: This isn't meant to be comprehensive, just my quick observations:

Main Menus:
-Make "Characters" and "VS Record" not selectable, "Characters" and "VS Record" work is Owner deferred.
-VS options
    -make Damage percentage 5x instead of 3x.
-CSS Bugs still present and need to be fixed:
    -music pauses/reset when rendering new 3d fighter previews (moving around cursor) **8 BGM suspends per visit: the closure loads in ONE frame, 8.6 frames long. Slice it.**
    -delay between cursor hover and 3d fighter preview rendering. **Same cause: the 13-tic dwell debounces a BLOCKING load. 70 tics waited per visit.**

-Yoshi:
    -Yoshi's guard/shield (egg) is invisible **Owner: intermittent. Heap latch under 25,600 free caps GObjs mid-match.**
-Link:
    -slash damage VFX playing at incorrect locations.
-Pikachu
    -neutral b attack VFX that walks terrain still has hard edges. **TLUT is a luminance ramp with 1-bit alpha: intensity IS the coverage.**
    -down B effect doesn't render all related VFX, missing blue exp on pikachu. **Was the generic spark; real maker routed, script 0x74 packed.**
    -Pokeball Spawn Intro not playing VFX. **3 producer defects fixed; effect builds, renderer declines it: no native bake.**
    -face color is slightly different from body color. (lighting difference???)
-Samus
-Kirby
    -Grab attack Up/down slam doesn't work correctly, victim teleports to another location (world origin???). **Kirby's own ThrowF status restored; it was aliased to the common one.**
    -Kirby face color is slightly different from body color. (lighting difference??? pink face color looks more correct I think...)
    -Neutral A punch flurry VFX not drawing at correct locations.
    -Kirby neutral B , then A attack to spit out fighter, Star VFX is invisible. **Both star makers were #define NULL; restored. Constructs OK, likely GObj-starved.**
    -Kirby fox hat not working correctly. Fox hat visible but, pistol shot crashes. **Crash was a halt on a declined draw; fixed. NEW: a flash over Kirby, cause unknown.**
    -Kirby has a wierd pose on results screen.
-Jigglypuff
    -face color is slightly different from body color. (lighting difference???)
-Captain falcon
-Ness
-General
    -Results screen not showing 1st place emblem
    -All Fighters are not doing correct poses/animations on results screen
    -No contest results screen, all fighters should be doing the clapping animations.
Stages:
-peaches castle: 
    -Foreground castle roof renders ALL geometry now but the texture is missing on the now visible geometry. A continuous tiled roof surface is almost achieved. **Owner Deferred**
-Zebes: Acid plane Color is accurate now but texture blending are all visibly too HARD. Edges are too defined instead of a gradient/smooth transistion. Stage lights on the ground floor have a flat/hard transparency (hard upsidown trapezoid shape) instead of looking like a real light source with a gradient that tapers to fully transparent towards the top. **Owner Deferred**
-Yoshi's Island: 
-SectorZ:
-Saffron city: the pokemon garage door hazard is always open for some reason. It should close and open periodically. **Gate LOGIC + collision cycle correctly (traced 1,900 frames); the gate MODEL never moves -- rendering gap, not hazard logic.** 

Audio:
