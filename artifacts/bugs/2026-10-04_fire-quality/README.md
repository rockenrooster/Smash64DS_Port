# Fire quality, Yoshi's egg, Kirby's inhale (owner r74/r75) — 2026-10-04

Owner rows: "Fire texture/sprite looks low quality (Fire flower, charmander,
charzard)"; "Yoshi Up B egg explosions don't look the same (shrapnel pattern,
explosion VFX)".

- Fire: the item bank's flame (224) and smoke (225) cells were 16x16 and 8x8 in
  the shared quad sheet (2:1 and 4:1 under the source's 32x32). They are now
  source size. The Fire Flower, Hitokage and Lizardon all start these two item
  scripts, so all three change together.
- Yoshi's egg explosion (efManagerYoshiEggExplodeMakeEffect, script 3 of
  particles_unk2, which makes 0-2) never had its fighter bank packed, so the
  bank registered empty and only efcommon's shell shards drew. Packed now.
- Kirby's inhale wind (script 12 of particles_unk0, which makes 6-11) and the
  0x4C/0x4D motion effects: same gap, same fix.

The quad sheet is seven 128x64 A3I5 sheets (56,640 of 57,344 texels with
Yoster's rows). Grey cells pack onto their own sheets and coloured cells fill
the rest in hue order, so the four newly admitted efcommon textures did not cost
the existing ones: mean premultiplied error over the common set 3.93 -> 4.00,
DamageNormalLight (33) within 15%.

Captures (lab four-CPU ROM, Kirby/Yoshi/Fox/Ness, Castle, fire flowers only):
`before/` (r74 sheet), `vfx01/` (banks packed, 7 sheets), `vfx02/` (final
sheet: m0 countdown, m1 Fire Flower item, m2 flames, m3 egg explosion, m4/m5
other bursts and the item spawn swirl).
