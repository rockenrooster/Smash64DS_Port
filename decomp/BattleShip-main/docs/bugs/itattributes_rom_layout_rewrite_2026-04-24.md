# ITAttributes ROM-layout rewrite — 2026-04-24 — FIXED

## Symptom

No thrown item (Pokeball, Capsule, Egg, Barrel, Link's Bomb, GShell, RShell,
Monster Ball) ever registered a hit on a fighter mid-flight. Native weapons
(Mario fireball, Pikachu thunder jolt, Samus gun, Link boomerang, and
item-spawned weapons like Star Rod bullets / L-Gun ammo / Fire Flower flames)
hit fighters correctly.

Secondary, previously-masked symptoms:
- Dropped items clipped through platforms (tops sitting on the floor).
- A-press routing sent Throw items to `ftCommonItemShootSetStatus` and crashed
  with SIGBUS fault\_addr=0x6400000064.

## Root cause

`ITAttributes` was mis-laid-out in the port. Three compounding errors:

1. **Upstream decomp field positions were partly wrong.** `type` and
   `hitstatus` were declared in an extra final u32 ("Word B4" / "Word B5" in
   the port's naming) that does not exist in ROM. They actually live in the
   same u32 as `priority`, `kb_base`, and the can_* flags at offset `0x3C`.

2. **The port's LE branch added artificial `u32 : N` pads** to force clang to
   mimic IDO's layout. Those pads — `:11`, `:16`, `:14`, `_pad_before_combat_bits`
   — forced clang to *start new u32 storage units* where IDO's natural packing
   did not, inflating sizeof from the real `0x48` to `0x50` and shifting every
   field from `0x1E` onward by 2 bytes. `attr->size` was reading from
   `0x34` (yielding `0x5A45 / 2 = 11554.5` — half the stage) when ROM has
   `size` at `0x32`. `attr->attack_count` was reading bits 12-13 of `0x3C`
   which under IDO's real layout are the low pad bits, consistently yielding
   `attack_count = 0` → `ftMainSearchHitItem`'s per-attack loop never ran.

3. **IDO-specific packing rule missed.** IDO packs a plain `u16 spin_speed`
   declared after a `u32 vel_scale : 9` bitfield into the low 16 bits of the
   same u32 storage unit (the "pad-gap packing" rule — see memory
   `project_ido_bitfield_pad_packing.md`). The port placed `spin_speed` at
   `0x4C` and added a separate `portFixupStructU16(attr, 0x4C, 1)` for it;
   ROM has it at `0x46`, inside the u32 at `0x44`.

## Fix

Ground truth collected by disassembling `itManagerMakeItem` (`0x8016E174`)
from `baserom.us.z64` with rabbitizer and decoding every `lw / sll / srl /
andi` sequence against `$v1 = attr`. Cross-checked bit positions against ROM
values extracted from reloc file 251 for every item.

Rewrote `struct ITAttributes` in `src/it/ittypes.h` to match IDO's actual
layout: `sizeof = 0x48`, no artificial pads, `type / hitstatus` moved into
Word `0x3C` alongside `priority / kb_base / can_*`, `spin_speed` declared as
`u32 : 16` bitfield packed into `0x44`. Bit counts per word are verified
against upstream decomp field-width sums: Word `0x3C` = 28 data bits + 4 pad
bits (not 2).

`src/it/itmanager.c` now reads every attribute directly:
- `portFixupStructU16(attr, 0x14, 8)` replaces the old `0x20, 6` + `0x4C, 1`
  pair. Covers the eight u32 words spanning `0x14..0x33` that hold plain
  s16/u16 halves needing the post-BSWAP32 half-rotation.
- The `ITAttributes.type` bypass-override at `itmanager.c:280-299` is deleted
  (`attr->type` now reads bits 10-13 of `0x3C` natively).
- The `map_coll.bottom` negation workaround is deleted (`attr->map_coll_bottom`
  now reads the actual signed-negative value from ROM offset `0x2E`; the
  previous workaround was compensating for the old struct reading
  `map_coll_width`'s positive bytes as if they were `bottom`).
- `BITFIELD_SEXT16` calls retained only for `attack_offset0_x` (still a
  bitfield); the other five offsets are plain `s16` in the new layout and
  don't need explicit sign extension.

## Verification

Every item in file 251 decodes to sensible values: `attack_count = 1`
uniformly, `type` matches `nITType*` enum per category (Box/Taru = Damage,
Sword/Bat = Swing, LGun/FFlower = Shoot, Throwables = Throw, Star = Touch,
Consumables = Consume), `damage` / `size` / `kb_base` per-item values match
gameplay expectations, `map_coll_bottom` is already signed-negative in ROM
(Capsule=-100, Taru=-236, Sword=-422, MSBomb=-60).

Thrown barrels/eggs/capsules/Link's bombs now hit fighters mid-flight. Items
rest on platforms correctly. Action routing by item `type` works.

## Related / superseded

- `docs/bugs/itattributes_type_field_offset_2026-04-20.md` — surgical `type`
  override; superseded, override removed.
- `docs/bugs/item_map_coll_bottom_sign_2026-04-20.md` — `map_coll_bottom`
  negation workaround; superseded, negation removed.
- `docs/itattributes_rom_layout_handoff.md` — this handoff is the work item
  that got done.
- `docs/debug_ido_bitfield_layout.md` — the audit methodology used here.

## Tooling notes

Disassembly: `/tmp/rvenv/bin/python debug_tools/rom_disasm/disasm.py
0x8016E174 --rom baserom.us.z64 --count 800 --base v1`. Bitfield decoder at
`debug_tools/rom_disasm/decode_bitfields.py`. ROM extraction via
`debug_tools/reloc_extract/reloc_extract.py extract baserom.us.z64 251
/tmp/itcommon.bin`. Per-item word values at `0x34/0x38/0x3C` and per-word
bit-position breakdowns are in the git blame of this commit's changes to
`src/it/ittypes.h`.
