# Kirby's entry Warp Star (2026-10-04)

Owner (r74 BUGS.md, Kirby): "in kirby's intro, we don't see the Star that
Kirby rides on".

## Two faults

1. **The star lived one update.** `efManagerKirbyEntryStarMakeEffect`
   (efmanager.c:5195) writes `&llKirbySpecial2EntryStar{R,L}AnimJoint` into
   its desc on every call, after the init-time offset resolver. The port
   declared both as `static uintptr_t`, so the written value was a RAM
   address: the effect's AnimJoint table read garbage, no joint got a script,
   the GObj's `anim_frame` stayed 0, and `efManagerNoStructProcUpdate` ejected
   it on its first update (gdb: made at presented frame 75, ejected at 75,
   child `anim_joint` NULL, no normalizer refusal). Fixed with the
   address-as-offset form the Poke Ball symbols already use
   (`battleship_efmanager_symbols.h`).
2. **Its texture was a column.** The star list (KirbySpecial2 0x1CF8) is a
   textured quad whose texture is a 16x32 RGBA32 half star, mirrored in S by
   its tile (cms mirror|clamp, maskS 16, tile 32x32, 2,048 bytes loaded). The
   entry generator sized the load from the tile line as 8x64: an RGBA32 tile
   splits each texel over TMEM's two banks, so a qword of tile line holds four
   texels, not two. 8 texels is under the 16-texel mask, the mirror was not
   materialized, and the quad drew a yellow bar. `generate_nds_entry_effects.py`
   now reads four texels a qword for 32-bit tiles; the regenerated texture is
   the full 32x32 star (only `sNdsEntryEffectTexels53` and its row change).

The maker and the generator are shared by VS and 1P.

## Evidence (walk ROM, 1P stage 0 as Kirby)

- `ks02/f100.png`, `f108.png`: before -- Kirby flies in with nothing under him.
- `ks16/f099..f111`: after -- the star flies in, Kirby rides it down, lands.
