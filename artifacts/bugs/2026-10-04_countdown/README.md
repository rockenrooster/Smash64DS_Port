# Countdown traffic light (owner r75) — 2026-10-04

Owner: "3,2,1,go countdown object detail doesn't quite match n64. Red, orange,
and blue bulbs look flat and missing slight details, and the black 'go' text on
the blue bulb isn't very legible."

Cause: the dim lamps (IFCommonGameStatus RedDim/YellowDim/BlueDim) are I4
coverage discs at 7/15 (about 47%) in the lamp's prim colour, drawn OVER the
housing; the housing's sockets carry each bulb's shading and specular
highlight. The DS atlas baked them opaque (coverage premultiplied into RGB,
A3 forced to 7), which hid the sockets and left flat discs. The GO lettering
(ShadowGo, IA8 15x11) was point-sampled to 12x9, which drops every fifth
source column and row and broke the O's right stroke.

Fix (src/nds/nds_ifcommon_oam.c): the dim lamps keep their coverage as the
texel's A3 alpha in their own colour; the GO lettering is area-sampled over
the exact 1.25x1.25 source box with graded alpha; the fixed traffic palette is
rebuilt (42 source colours -> 31, max RGB555 squared error 12) so it holds
the three lamp colours. scripts/check_ifcommon_hybrid_oam.py models the same
contract and pins the palette.

- model-before-6x.png / model-after-6x.png: the checker's DS model of the box.
- lab-capture-after.png: lab ROM, countdown "1" (third lamp lit).
- go-point-vs-area.png: GO lettering at 12x9, point (old) vs area-sampled.
