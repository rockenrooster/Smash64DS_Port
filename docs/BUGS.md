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
    -Changing Damage percentage still feels bad to use. maybe have it be 1 at a time but change at like 10 per sec? **Done: 1 per step, repeat every 6 updates (~10/sec), row-local cadence.**
-CSS Bugs still present and need to be fixed:
    -delay between cursor hover and 3d fighter preview rendering. **Owner: lower priority now. Dwell is 2 cold / 1 warm; residual is a 30,160 B owner-image read.**

-Yoshi:
    -Yoshi's guard/shield (egg) is invisible **Latch cause found: Kirby's copy-hat load, not general pressure. Egg bake still owed.**
-Link:
    -slash damage VFX playing at incorrect locations. **Root XObj kind 0x45 took a translate-bearing fallback: world contact applied twice.**
-Pikachu
    -neutral b attack VFX that walks terrain still has hard edges. **GROUND fixed (owner). AIR jolt REGRESSED in r24: shares asset 342; suspect VRAM/bind interaction with the new A5I3 names.**
    -down B effect doesn't render all related VFX, missing blue exp on pikachu. **Was the generic spark; real maker routed, script 0x74 packed.**
    -Pokeball Spawn Intro not playing VFX. **Bake existed. The gate wanted an item GObj; the entry ball is an effect. Admitted.**
    -face color is slightly different from body color. (lighting difference???) **Not lighting: N64 clamps shade then multiplies prim; DS folds prim first, so lit bodies wash white.**
    -when I choose pikachu, in match, fox opponent is frozen and cannot be hit. Sudden death works funnily enough though.
-Samus
-Kirby
    -Kirby face color is slightly different from body color. (lighting difference??? pink face color looks more correct I think...) **r25: fixed facing LEFT, wrong facing RIGHT, fixed in ledge-balance. Facing-dependent = the normal transform under a mirrored modelview.**
    -Neutral A punch flurry VFX not drawing at correct locations. **Effect table read from a .bss address: all five maker args were garbage.**
    -Kirby neutral B , then A attack to spit out fighter, Star VFX is invisible. **Makers restored. NOT GObj-starved after all: ~10 slots free post-latch. Renderer side unexamined.**
    -Kirby has a wierd pose on results screen. **It was EggLay: no Results animation was packed, so every fighter held row 0.**
-Jigglypuff
    -face color is slightly different from body color. (lighting difference???)
-Captain falcon
-Ness
-General
    -Results screen not showing 1st place emblem **Emblem dead at dispatch: native display refuses non-battle scenes. Badge path not localized.**
    -All Fighters are not doing correct poses/animations on results screen **142 submotion payloads were in no NitroFS list. 47/47 cells route now.**
    -No contest results screen, all fighters should be doing the clapping animations. **Kind was always correct; the Claps figatree could not load.**
Stages:
-peaches castle: 
    -Foreground castle roof renders ALL geometry now but the texture is missing on the now visible geometry. A continuous tiled roof surface is almost achieved. **Owner Deferred**
-Zebes: Acid plane Color is accurate now but texture blending are all visibly too HARD. Edges are too defined instead of a gradient/smooth transistion. Stage lights on the ground floor have a flat/hard transparency (hard upsidown trapezoid shape) instead of looking like a real light source with a gradient that tapers to fully transparent towards the top. **Owner Deferred**
-Yoshi's Island: 
-SectorZ:
-Saffron city: the pokemon garage door hazard is always open for some reason. It should close and open periodically. **Gate replayed a baked constant world matrix, frozen CLOSED; the always-open look is layer geometry. Bindings now live. UPDATE crashes with Kirby**

Audio:
