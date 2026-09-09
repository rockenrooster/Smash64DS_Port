**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary (or less) if not fixed yet, always using ** ** to encapsulate agent added text.
(Investigation details live in `docs/p2/BUG_NOTES.md`; this queue stays lean.)

treat anything not going through native renderer a failure.

**Every newly built ROM, including diagnostics, must exclude non-native renderer implementations. Runtime defaults and zero observed fallbacks are insufficient.**

Owner notes: This isn't meant to be comprehensive, just my quick observations:

Main Menu:
-1P mode not selectable — **Paused by owner; do not resume campaign work.**
-VS Mode is perfect and should be used as a reference as well as the main menu for the
-Data not selectable — **Native entry is documented; children and natural-path acceptance remain unverified. Keep behind match repairs.**

VS mode CSS:
-Kirby not selectable — **Shared CSS residency/storage blocker; integrate compact previews and classify the libfat stop before enabling the roster.**
-Jigglypuff not selectable — **Same shared CSS residency/storage blocker; no independent rendering fix established.**
-Ness not selectable — **Same shared CSS residency/storage blocker; no independent rendering fix established.**
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
    -neutral b attack should have transparency/alpha
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
-Shield texture: Shield texture slices are rendered in the wrong order, causing the shield image to be horizontally scrambled instead of forming a continuous circular texture. — **OPEN, and this description RETIRES the earlier banding diagnosis: scrambled slices are a TEXEL ORDER defect, which the A5I3-to-A3I5 palette widening landed today does not address at all. Two premises that change carried are now dead -- the source alpha is NOT flat (11 distinct alpha levels and 13 intensity levels, measured from the IA8 bytes and pinned in test_native_shield_reflector_packets.py), so A3I5 trades away real alpha precision rather than getting those bits free. Leading cause, with four prior recurrences in this repo: the converter reads the 8-bit source as `payload[offset + (source_index ^ 3)]`, a byte-lane correction for a word-swapped O2R payload, and a wrong lane rule permutes texels within each group of four -- horizontal scrambling while every count, size and checksum stays valid. Two other places the order can break and are being tested with it: the quad's texture coordinates, and an atlas upload strided by texture width instead of atlas width. A3I5 stays for now, pending that verdict.**

Stages:
-peaches castle: Foreground castle roof renders as separated red triangular strips/wedges instead of a continuous tiled roof surface; roof geometry/texture slices are fragmented and misaligned.
-Zebes: Acid Plane is flat now. New issue is that I can see the acid THROUGH the floor of the stage like its transparent, but the ACID is what is transparent and always is drawing OVER the stage when it shouldn't be the stage and acid should act like 3D objects that can cover each other depending on what part is drawing over the other. Stage lights on the ground floor have a flat transparency (upsidown trapezoid shape) instead of looking like a real light source with a gradient.
-Mushroom kingdom: STILL MISSING THE LARGE SIDE BRICK PLATFORMS. the one on the left side should be drawing under the left warp pipe so the pipe doesn't look like its floating. also keep in mind that the BG on this map is an animated 2D BG with sprites that moves around the the original SMB so that might be the FPS cost and aloso be getting mixed up with the foreground objects.
-Yoshi's Island: TRANPARENCY STILL NOT BEING APPLIED IN TEXTURES/SPRITES (both spinning textures, all three clouds (which are also the wrong color), the white sparkle textures around the heart). also new thing i noticed is that part of the BGM sounds garbled, but like only one instrument. STILL MISSING THE MAIN PLATFORMS AND MAIN FLOOR PATH. — **Floor: the MAIN FLOOR draws in the 2026-09-09 witness screenshot (artifacts/visibility/0909-witness3-yoster.png) -- green top surface and layered dirt sides, both fighters standing on it. PLATFORMS: not visible in that shot and NOT established either way, since the camera may simply not frame them -- do not read the floor's presence as covering them, which is the error that had to be retracted on Mushroom Kingdom the same day. 164 of 164 emitted triangles reach hardware with every decline witness at zero. Same shape as the Castle roof: Same shape as the Castle roof: all 19 drawable roots are listed in the descriptor and emitted 1:1, the missing floor root 0x49A0 carries 155 commands / 77 triangles / 22 runs, every run passes every static gate, and the stage records zero native failures. Not a coverage or omission defect; it dies at draw. Still open: the garbled instrument, and see the transparency note below.** **2026-09-08: sparkles never admitted to the particle bank (P1 seam list only); cloud colour loses its env tint in the I4 converter.**
-SectorZ: Arwings are visible now but are rotated the wrong way (facing the BG, they should be facing left or right depending on they are moving left or moving right) and i can "see" the "platform collision lines".
-saffron city: the pokemon garage door hazard is always open and the pokemon don't visibly spawn.
