# Pikachu's down-B Thunder: the one that would compile, blocked where the egg was

**Assessed, not attempted.** This completes `P2-3f53`: all three remaining
effects are blocked, and the causes are different.

## What it actually is

The board row says "Pikachu Thunder", which **conflates three effects**:

| effect | state |
|---|---|
| **Thunder Jolt** (neutral-B) | **already natively owned** — three dedicated generators (`generate_nds_native_pikachu_thunderjolt.py`, `…_thunderground.py`, `…_thunderjolt_effect.py`), all wired at `check-native-owner-wiring.py:225-227` |
| `dEFManagerPikachuThunderShockEffectDesc` | a **smash attack**, spawned from `ftcommonattacks4.c:49`, not down-B |
| **down-B Thunder** | the weapon pair plus `dEFManagerPikachuThunderTrailEffectDesc` — the actual gap |

The owner's report is *"down B effect doesn't render and sometimes crashes"*, so
the third is the one that matters.

## It resolves cleanly, and its format is fine

`dEFManagerPikachuThunderTrailEffectDesc` (`efmanager.c:640`), file
`&gFTDataPikachuModel`, DObj setup `llPikachuModelThunderTrailDObjDesc` =
**`0x95B0`**, MObjSub `0x9420`
(`src/import/battleship_efmanager_symbols.h:132-133`).

`0x95B0` holds the DLLink shape `{ count 1, gfx, stride 4, 0 }` — the same
structure Vulcan Jab's links use — naming Gfx root **`0x94f8`**. Walking it and
following `G_DL` branches:

```
0x9538 SETTILE fmt=3(IA) siz=2(16b)
0x9540 SETTILE fmt=3(IA) siz=1(8b)
0x9558 DL -> 0x0000
0x95a8 ENDDL  (23 cmds, 1 VTX, 1 TRI)
unsupported formats found: none
```

**IA16 and IA8, both in the supported set.** Unlike Kirby's Vulcan Jab, which
dies on an RGBA32 the DS cannot represent at all, nothing here escapes the
packet compiler.

The `DL -> 0x0000` is not a defect: it is the slot the live MObjSub patches,
which is why this EFDesc has an MObjSub (`0x9420`) where the egg and Vulcan Jab
have `0x0`. That is precisely the Falcon Kick / Falcon Punch pattern the
generator already supports through `material_images` variants compiled per
`texture_id`.

**So this is the one of the three that would compile.**

## And it is blocked exactly where the Yoshi egg was

`PikachuModel` (asset **341**) is **not** an `InputSpec` in
`generate_nds_entry_effects.py` — the resource list ends
`… MBALLRAYS, KIRBY_SPECIAL2, KIRBY_MODEL` (`:1790`). Admitting Thunder means a
new file, new texel arrays and new table rows, which is the shape that cost the
Yoshi egg an arena page.

And the four-CPU gate build is `NDS_P2_PIKACHU 0`:

```
NDS_P2_DONKEY 1   NDS_P2_SAMUS 1   NDS_P2_LINK 1   NDS_P2_KIRBY 1
NDS_P2_PIKACHU 0  NDS_P2_YOSHI 0
```

So every byte of it would be resident in a build where **Pikachu cannot
appear** — the clearest possible case for the per-roster emitter already sized
at 8,458 B (`…/2026-09-17_p2-2p8-entry-effect-roster-residency/`).

## Why this is inferred and not measured, stated plainly

Two measurements bound it without a third build:

1. The Yoshi egg added ~1 KB of resident packet data and **crossed** a page:
   `ArenaChosenSize` −4,096, `AllocFailCount` 84 → 85, 14 texture-bind rejects
   (`…/2026-09-17_p2-3f53-yoshi-egg-owner/`).
2. Returning **768 bytes** did **not** cross back: arena size, alloc failures
   and heap low-water all byte-identical to control
   (`…/2026-09-17_p2-2p8-flatcache-shrink/`).

Together those put the remaining headroom under 1 KB. Thunder needs a new
`InputSpec` plus per-`texture_id` variants, which is more than the egg, not
less. The inference is sound, but it **is** an inference — the cheap way to
settle it is to wire it and read `gNdsTaskmanArenaChosenSize` alone, exactly the
falsifier the egg arm used.

## P2-3f53, complete

| item | cause |
|---|---|
| Falcon Punch / Kick | **done**, checked and registered; the row was stale |
| Yoshi egg | resident budget — implemented, reverted on the page boundary |
| Kirby Vulcan Jab | **RGBA32**, a format the DS hardware does not have |
| Pikachu down-B Thunder | format-clean; resident budget, and its file is absent from the gate roster entirely |

Two of the four are blocked by the **same** constraint, and the per-roster
emitter is the lever that addresses both. That is now the highest-value
unblocked engineering item on this row, and it is the one waiting on an owner
call because it changes how the packet is emitted per configuration.
