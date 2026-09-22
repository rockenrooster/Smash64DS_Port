**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 100 char summary (or less) if not fixed yet, always using ** ** to encapsulate agent added text.
(Investigation details live in `docs/p2/BUG_NOTES.md`; this queue stays lean.)
**Owner: removal from this file means FIXED; absence never means a symptom was unreal.**

treat anything not going through native renderer a failure.

**LADDER RESULT: r39/r40/r41 clean; r42, r43 and r44 (three tint-route variants) all regressed. Tint route REMOVED again.**

**PLAY r45 `36afbc1cd172e755` = r41 + one CSS fix: a revisited fighter drew with zeroed UVs (eyes/faces/Link gray).**

**Plan and evidence: docs/p2/REMAINING_BUGS_IMPLEMENTATION_PLAN_2026-09-22.md**

Owner notes: This isn't meant to be comprehensive, just my quick observations:

Main Menus:
-VS options
-CSS Bugs still present and need to be fixed:
    -delay between cursor hover and 3d fighter preview rendering. **NOT FIXED in R36 ** **r37: preview loader spent 1 of its 4 byte units per tic; now spends all four.**
    -Fighter faces not looking right:
        -Yoshi: sometimes one eye is closed **r37: port recorded a texture-part set the source drops; now conditional.** **r45: a revisit reused a UV cache the reloaded image lost; emulator r41 0 vs r45 6 rebuilds.**
        -Pikachu: **NOT FIXED in R36 ** face color is different from body color. **STILL OPEN: the repair was right, its plumbing broke every fighter. Removed.**
        -Kirby: **NOT FIXED in R36 ** face color is different from body color. **STILL OPEN, same as Pikachu.**
        -Jigglypuff **NOT FIXED in R36 ** face color is different from body color. sometimes one eye is closed. **Eye: r45 UV-cache fix (revisits drew zeroed UVs); colour STILL OPEN.**
        -Link: **NOT FIXED in R36 ** sometimes missing textures/color (turns gray). **r37: a retired preview's texture entries were never released, and their keys are addresses.** **r45: revisits also drew zeroed UVs.**
-Yoshi:
    -**NOT FIXED in R36** Yoshi's guard/shield (egg) is invisible **r37: its descriptor mapped through a data-only resolver, so no effect object was ever created.**
-Link:
-Pikachu
    -**NOT FIXED in R36 ** face color is different from body color. 
    -**NOT FIXED in R36 ** down B effect doesn't render all related VFX, missing blue exp on pikachu. **r37: its texture was packed but off the quad sheet, so it drew zero pixels.**
    
-Samus
-Kirby
    -**NOT FIXED in R36 ** face color is different from body color. 
-Jigglypuff
    -**NOT FIXED in R36 ** face color is different from body color. 
-Captain falcon
-Ness
-General
    -regression in r36 VFX sometimes do not play depending on fighter combination. **r37: 8 dedicated glow textures could not hold a 4-fighter frame; now 16.**
    -** R45 UPDATE, some maps crash, (tested with kirby and fox)**
Stages:
-peaches castle: 
    -Foreground castle roof renders ALL geometry now but the texture is missing on the now visible geometry. A continuous tiled roof surface is almost achieved. **NOT FIXED in R36 ** **r40: a cycle's alpha is (A-B)*C+D and only C/D were tested; MODULATEIA puts the texel in A.**
-Zebes: Acid plane Color is accurate now but texture blending are all visibly too HARD. Edges are too defined instead of a gradient/smooth transistion. Stage lights on the ground floor have a flat/hard transparency (hard upsidown trapezoid shape) instead of looking like a real light source with a gradient that tapers to fully transparent towards the top. **NOT FIXED in R36 ** **r37: the light cone emitted 3 flat alpha bands; subdivided to 20 triangles. Acid unchanged.**
-Yoshi's Island: 
-SectorZ:
-Saffron city: **NOT FIXED in R36 ** the pokemon garage door hazard is always open. It should close and open periodically. **Transform chain VERIFIED by injection: the door's matrix tracks its joint, geometry submits, timing matches source. Only pixels unverified. Does it ever close over a full minute?** 

Audio:
