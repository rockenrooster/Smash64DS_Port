**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary (or less) if not fixed yet, always using ** ** to encapsulate agent added text.
(Investigation details live in `docs/p2/BUG_NOTES.md`; this queue stays lean.)

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


-CSS still has some problems:
    -Low FPS/Flashing during gate openings
    -fighter 3d previews not visible for:
        -Yoshi
    -music pauses/reset when rendering new 3d fighter previews (moving around cursor)
    -delay between cursor hover and 3d fighter preview rendering.
    -Kirby not selectable
    -Jigglypuff not selectable
    -Ness not selectable
-Yoshi:
    -Up B egg shells are not rendering.
    -Grab attacks turn yoshi invisible.
    -B attack turns yoshi invisible and egg is also invisible 
    -character intro is invisible (egg hatching)
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
    -shield rolling is invisible.
    -down B is invisible.
    -B charge/shots are not rendering over the samus gun, can move Z depth to be infront of gun so that the gun is occluded when charging.
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
