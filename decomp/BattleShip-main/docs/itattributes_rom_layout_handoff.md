# ITAttributes ROM-Layout Rewrite — Handoff (2026-04-20)

Open investigation. The decomp's `ITAttributes` declaration is structurally
wrong relative to the actual ROM data layout. `src/it/itmanager.c:280-299`
is a point-fix for `type` only. Every other Region B field may be reading
garbage; the game has been papering over this.

## What triggered this

A whole-codebase IDO-bitfield audit (methodology in
`docs/debug_ido_bitfield_layout.md`) flagged ITAttributes Region B with
~24 field mismatches between the port's LE block and what IDO compiles
from the decomp source. The prior WPAttributes playbook
(`docs/bugs/wpattributes_bitfield_padding_2026-04-20.md`) assumes the
decomp declaration matches ROM. For ITAttributes it doesn't.

## Ground-truth facts (ROM, reloc file 251)

ROM stride between items = `0x48` (72 bytes), not `0x50` (80 bytes) as
the port asserts. `_Static_assert(sizeof(ITAttributes) == 0x50)` is wrong.
`smash_sfx` / `vel_scale` / `spin_speed` at port offsets `0x48..0x4F`
read past ITAttributes into AttackEvents or the next item's bytes.

Verified Region B layout for u32 at offset `0x3C` across all 13 common
items (`debug_tools/reloc_extract/reloc_extract.py 251`):

```
bits 31-22: knockback_weight (10)
bits 21-14: shield_damage    (8)
bits 13-10: type             (4)   -- this is the bug-doc's "type" field
bits  9-0:  <low10, mostly zero, sometimes set — unknown>
```

Per-item decode table (matches nITType* enum ✓):

| Item      | W_0x3C     | kb_w | shld_dmg | type | low10  |
|-----------|------------|------|----------|------|--------|
| Capsule   | 0x27050C40 | 156  | 20       | 3    | 0x040  |
| Tomato    | 0x37001400 | 220  | 0        | 5    | 0x000  |
| Heart     | 0x37001400 | 220  | 0        | 5    | 0x000  |
| Star      | 0x30001000 | 192  | 0        | 4    | 0x000  |
| Sword     | 0x270F0400 | 156  | 60       | 1    | 0x000  |
| Bat       | 0x270F0400 | 156  | 60       | 1    | 0x000  |
| Harisen   | 0x27118400 | 156  | 70       | 1    | 0x000  |
| LGun      | 0x27028800 | 156  | 10       | 2    | 0x000  |
| FFlower   | 0x27078800 | 156  | 30       | 2    | 0x000  |
| Hammer    | 0x25079400 | 148  | 30       | 5    | 0x000  |
| MSBomb    | 0x27028C00 | 156  | 10       | 3    | 0x000  |
| BombHei   | 0x27078C00 | 156  | 30       | 3    | 0x000  |
| Pokeball  | 0x270F0C00 | 156  | 60       | 3    | 0x000  |

## Surprising finding — W_0x40 is constant

u32 at offset `0x40` = `0x0E4390E4` for every item in file 251. This
cannot be per-item bitfield data. The port declaration currently places
Word B3 (`priority + rehit flags + knockback_base + :14`) here. All of
those reads produce the same value for every item — likely wrong, but
not catastrophically observable because the port hasn't verified combat
behavior against the original against every item.

## u32 at 0x44 — varies, pattern unclear

Per-item values of W_0x44 (raw BE):

| Item      | W_0x44     |
|-----------|------------|
| Capsule   | 0x28000078 |
| Tomato    | 0x32000064 |
| Heart     | 0x32000000 |
| Star      | 0x32000000 |
| Sword     | 0x32000064 |
| Bat       | 0x37000064 |
| Harisen   | 0x23000032 |
| LGun      | 0x3200008C |
| FFlower   | 0x28000000 |
| Hammer    | 0x32000000 |
| MSBomb    | 0x2D000078 |
| BombHei   | 0x32000000 |
| Pokeball  | 0x28000014 |

The bytes split cleanly as two u16s: `0xXX00` and `0x00YY`. High byte
of first u16 ∈ {0x23, 0x25, 0x28, 0x2D, 0x30, 0x32, 0x37}. Second u16
∈ {0x0000, 0x0014, 0x0032, 0x0064, 0x0078, 0x008C}. Plausible
hypotheses: `spin_speed` (%)  / `vel_scale` (%) / some bounce or
lifetime value. Needs correlation with observed gameplay.

## u32 at 0x38 — mostly zero top half

Pattern across items: `0x0000_XXXX`. Values:
`0x104A, 0x184A, 0x1836, 0x1802, 0x1840, 0x1850, 0x1866, 0x1A0A`.
The decomp/port declare this as Word B1 (`angle:10 | kb_scale:10 |
damage:8 | element:4`). IDO compiles the decomp to pack angle into a
preceding u16 pad gap; the remaining `kb_scale + damage + element`
would occupy the full u32. But ROM has the top half always zero, so
kb_scale and damage are always 0 from this word. Either (a) these
items don't have direct hitboxes (mostly true: only Sword/Bat/Harisen
do direct damage), or (b) these fields live elsewhere.

## What's proven broken vs. suspected-latent

| Field           | Status                                       |
|-----------------|----------------------------------------------|
| type            | WORKAROUND at itmanager.c:280 reads correctly|
| knockback_weight| ROM layout known (at 0x3C [31:22]), port reads wrong slot |
| shield_damage   | ROM layout known (at 0x3C [21:14]), port reads wrong slot |
| angle / kb_scale / damage / element | IDO packing + port layout both probably wrong; ROM position unknown |
| knockback_base / priority / rehit flags / can_* | Port reads u32@0x40 which is identical across items → all items get identical values → combat gating may be broken but masked |
| hit_sfx / attack_count / can_setoff | Same (Port Word B2 layout reads some of these) |
| hitstatus | Port Word B4 offset wrong; real position unknown |
| drop_sfx / throw_sfx | Port Word B4 offset wrong; real position unknown |
| smash_sfx / vel_scale / spin_speed | Port sizeof too large — reads past ROM into next record |

## Consumers that may be reading garbage

`itmanager.c:329-398` copies many of the above fields from `attr->X`
into `ip->X`/`ip->attack_coll.X`. `ftmain.c:403` reads
`fp->attr->smash_sfx`. `wpmanager.c:227-251` reads many flags. Each
one needs review after the struct is rewritten.

## Proposed next-session plan

1. **Extract 3-4 well-characterized items' full 72-byte ROM records.**
   Good candidates: Sword (direct attacker), Pokeball (thrown container),
   Star (touch-damage), Heart (consume/no-hit). Known gameplay values:
   Sword damage ≈ 21, Star grants invincibility on touch, Heart heals
   100 HP, Pokeball has no direct damage.
2. **Disassemble the original N64 ROM at known reader sites**
   (`itManagerMakeItem`, `ftParamPlayVoice`, etc.) using mips_to_c or
   rabbitizer on `baserom.us.z64` directly. The original game code reads
   each field from its actual ROM offset — that's ground truth,
   independent of the decomp's mis-declared struct.
3. **Build a ROM-offset field table** field-by-field from the
   disassembly + empirical ROM values.
4. **Rewrite ITAttributes** — port LE branch first, then reconsider
   whether the decomp BE branch should stay (accuracy-to-decomp-source
   vs. accuracy-to-ROM, per CLAUDE.md rule 5 on decomp preservation).
   Likely stay with BE branch as-is and diverge in LE branch only.
5. **Shrink sizeof 0x50 → 0x48** and reroute `smash_sfx` / `vel_scale`
   / `spin_speed` consumers either to their real ROM location (if they
   exist in ROM) or to sensible defaults (if they don't).
6. **Retire `itmanager.c:280-299` workaround** once `type` reads
   correctly natively.

## Tooling

- ROM extraction:
  `python3 debug_tools/reloc_extract/reloc_extract.py extract baserom.us.z64 251 /tmp/itcommon.bin`
- IDO probing (for reader disassembly if wanted):
  see `docs/debug_ido_bitfield_layout.md`
- Original-ROM disassembly: rabbitizer can read `baserom.us.z64`
  directly given known function addresses. `lbRelocGetFileData` ≈
  `0x80172F80` region; grep the reloc table dump for exact addresses.

## Related

- `docs/bugs/itattributes_type_field_offset_2026-04-20.md` — prior
  point fix for `type`; describes the `attr->type` → `*(u32*)(attr+0x3C)`
  workaround. Keep this in place until the full rewrite lands.
- `docs/bugs/wpattributes_bitfield_padding_2026-04-20.md` — the
  playbook that inspired this audit. NB: WP can be fixed mechanically
  because its decomp declaration matches ROM; IT cannot.
- `docs/debug_ido_bitfield_layout.md` — general audit methodology.
  Section "Known audit targets" lists IT as "Audit remaining fields" —
  partly done in audit 2026-04-20, remaining work is this handoff.
