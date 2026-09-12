**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary (or less) if not fixed yet, always using ** ** to encapsulate agent added text.
(Investigation details live in `docs/p2/BUG_NOTES.md`; this queue stays lean.)

treat anything not going through native renderer a failure.

**Every newly built ROM, including diagnostics, must exclude non-native renderer implementations. Runtime defaults and zero observed fallbacks are insufficient.**

Owner notes: This isn't meant to be comprehensive, just my quick observations:

Main Menus:
-1P mode not selectable
-VS Mode is perfect and should be used as a reference as well as the main menu for the
-Data goes to solid blue screen.

VS mode CSS:
-Kirby not selectable — **Shared CSS residency/storage blocker; integrate compact previews and classify the libfat stop before enabling the roster.**
-Jigglypuff not selectable — **Same shared CSS residency/storage blocker; no independent rendering fix established.**
-Ness not selectable — **Same shared CSS residency/storage blocker; no independent rendering fix established.**
-CSS still has some problems:
    -Low FPS during gate openings
    -REGRESSION: fighter previews not visible for:
        -Luigi
        -DK
        -Link
        -Samus
        -Captain Falcon
        -Yoshi
        -Pikachu
-Yoshi:
    -Up B egg shells are not rendering.
    -Grab attacks turn yoshi invisible.
    -B attack turns yoshi invisible and egg is also invisible 
    -character intro is invisible (egg)
-Link:
    -Selecting Link crashes sometimes
    -character intro should have transparency
    -neutral B makes link invisible when throwing and catching the boomerang
    -Up B effects aren't rending properly
    -Grab attack also is invisible
-Pikachu
    -neutral b attack should have transparency/alpha and no hard edges.
    -down B effect doesn't render.
    -electric damage effects seem to be missing overall
    -strong side a effects not rendering.
-Samus
    -shield rolling is invisible.
    -down B is invisible.
-captain falcon
    -falcon punch effect not rendering.
    -falcon kick effect not rendering.
For 1P mode CSS, look at the VS Mode CSS since they are VERY similar.

**Match rendering regressions (owner, 2026-09-08):**
-Shield texture: Shield is rendering behind/intersecting the fighter instead of visually enclosing them — it should composite over Mario, or be moved far enough toward the camera to fully cover his silhouette
Stages:
-peaches castle: Foreground castle roof renders ALL geometry now but the texture is missing on the now visible geometry. A continuous tiled roof surface is almost achieved
-Zebes: Acid plane Color is accurate now but texture blending are all visibly too HARD. Edges are too defined instead of a gradient/smooth transistion. Stage lights on the ground floor have a flat/hard transparency (hard upsidown trapezoid shape) instead of looking like a real light source with a gradient that tapers to fully transparent towards the top.
-Mushroom kingdom: Side platforms are now visible but untextured.
-Yoshi's Island: Rotating textures still expose opaque white texture-card backgrounds instead of transparency. Cloud platforms are pretty much correct now. Heart sparkle sprites have no transparency and the quads are fully opaque. Main platforms and main floor/path geometry are still completely missing as you can sede the background though it.
-SectorZ: Crahses.
-saffron city: the pokemon garage door hazard is always open for some reason. It should close and open periodically. Pokemon are missing the VFX for their attacks.

Audio:
-Yoshi's Island: part of the BGM sounds garbled, but like only one instrument. **09-09: all four ranked causes measured DEAD -- per-instrument lookup/waves/books clean over 1,443 notes, loop seam has no predictor state, mix headroom is dominated by a different program, and the port's linear resampler prices 41.85 dB against the engine's own 4-tap Lagrange at 41.17 dB. Needs an owner mute/solo listen to name the instrument.**
-**Found 09-09, not yet reported by owner: the BGM renderer deletes every pitch-bend event. Congo Jungle 89 bends, Saffron 88, Peach's Castle 5, Dream Land 3. Spec in scratch/stage_bgm_census.md.** All nine stages otherwise play the correct source sequence from the correct bank, staged and looping.
