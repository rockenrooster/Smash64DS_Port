# 2026-10-05 The unlit GO lamp's lettering

Owner (BUGS.md, General): "3,2,1,go countdown object detail doesn't quite match
n64 ... Unlit 'GO' still looks pretty bad."

The countdown lamp's GO lettering is one 15x11 IA8 sprite (IFCommonGameStatus
0x21878) drawn in two colourings: black before GO (`ShadowInitial`, prim/env
0/0/0) and navy at GO (`ShadowGo`). The traffic atlas
(`ndsIFCommonFillTrafficAtlas`, `src/nds/nds_ifcommon_oam.c`) baked
`ShadowInitial` like the opaque rod and housing: any texel at 3% coverage became
opaque black. At the 0.8x footprint (12x9) the letters' strokes cover nearly
every texel, so the unlit lamp showed a black plate with the letters as blue
gaps -- the source sprite's alpha is zero between the letters.

Both colourings now take the 1.25x1.25 area filter and keep coverage as the
texel's A3 alpha in their own colour, as `ShadowGo` and the dim lamps already
did. `scripts/check_ifcommon_hybrid_oam.py` models the same branch and passes
(its pinned palette, colour count and visible-texel counts are unchanged).
Lab ROM capture at presented frame 150 (`artifacts/visibility/2026-10-05_playtest/goLamp-f150.png`,
local only -- ROM-derived pixels are not committed): dark G and O letters on the
blue lamp instead of the black plate.
