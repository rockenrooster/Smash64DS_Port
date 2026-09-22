**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 100 char summary (or less) if not fixed yet, always using ** ** to encapsulate agent added text.
(Investigation details live in `docs/p2/BUG_NOTES.md`; this queue stays lean.)
**Owner: removal from this file means FIXED; absence never means a symptom was unreal.**

treat anything not going through native renderer a failure.

**LADDER RESULT: r39/r40/r41 good, r42 regressed. The cause is the face/body tint route -- it bound a texture behind the prepare tracker's back, so the next textured run reusing its prepare drew with the wrong texture. Every fighter after a tinted one, not just the three.**

**PLAY r43 `587c853bcd7439f2` -- it is r42 plus the one-line repair. If it is clean, every row below is in except Saffron's door.**

**Plan and evidence: docs/p2/REMAINING_BUGS_IMPLEMENTATION_PLAN_2026-09-22.md**

Owner notes: This isn't meant to be comprehensive, just my quick observations:

Main Menus:
-VS options
-CSS Bugs still present and need to be fixed:
    -delay between cursor hover and 3d fighter preview rendering. **NOT FIXED in R36 ** **r37: preview loader spent 1 of its 4 byte units per tic; now spends all four.**
    -Fighter faces not looking right:
        -Yoshi: sometimes one eye is closed **r37: port recorded a texture-part set the source drops; now conditional.**
        -Pikachu: **NOT FIXED in R36 ** face color is different from body color. **r37: prim now multiplies AFTER the shade; worst error 6/31 to 1/31.**
        -Kirby: **NOT FIXED in R36 ** face color is different from body color. **r37: same repair as Pikachu.**
        -Jigglypuff **NOT FIXED in R36 ** face color is different from body color. sometimes one eye is closed. **r37: both repaired; its body was drawing white at full light.**
        -Link: **NOT FIXED in R36 ** sometimes missing textures/color (turns gray). **r37: a retired preview's texture entries were never released, and their keys are addresses.**
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
Stages:
-peaches castle: 
    -Foreground castle roof renders ALL geometry now but the texture is missing on the now visible geometry. A continuous tiled roof surface is almost achieved. **NOT FIXED in R36 ** **r40: a cycle's alpha is (A-B)*C+D and only C/D were tested; MODULATEIA puts the texel in A.**
-Zebes: Acid plane Color is accurate now but texture blending are all visibly too HARD. Edges are too defined instead of a gradient/smooth transistion. Stage lights on the ground floor have a flat/hard transparency (hard upsidown trapezoid shape) instead of looking like a real light source with a gradient that tapers to fully transparent towards the top. **NOT FIXED in R36 ** **r37: the light cone emitted 3 flat alpha bands; subdivided to 20 triangles. Acid unchanged.**
-Yoshi's Island: 
-SectorZ:
-Saffron city: **NOT FIXED in R36 ** the pokemon garage door hazard is always open. It should close and open periodically. **NO DEFECT FOUND after 10 probes. Joints cycle, bindings point at the gate's own DObjs, world cache revalidates each frame. Does it ever close over a full minute?**

Audio:
