# Items: Poké Ball + 13 Pokémon (P2-5 class 6, biggest, last)

Status: not started · Reference: `decomp/BattleShip-main/decomp/src/it/itmonster/`
(file names use Japanese species names) + ball logic in `itcommon`/`itground`.

The ball is a throwable; the payload is 13 mini-actors. Batch through the
item pipeline one Pokémon at a time — each is a self-contained actor with
spawn animation, behavior script, hitboxes, VFX/SFX, and cry.

## Roster (BattleShip file → species → behavior sketch, verify each in source)

| File | Species | Behavior |
|---|---|---|
| `itpippi` | Clefairy | Metronome — mimics another summon's effect |
| `itdogas` | Koffing | Smog cloud damage field |
| `itiwark` | Onix | Rises then slams/drills downward |
| `itkabigon` | Snorlax | Leaps up, body-slams down huge |
| `itkamex` | Blastoise | Hydro Pump push/damage stream |
| `itlizardon` | Charizard | Flamethrower sweeps both sides |
| `itmlucky` | Chansey | Drops healing eggs (egg = container item) |
| `itnyars` | Meowth | Pay Day coin spray |
| `itmew` | Mew | Rare roll; flies off (bonus points only) |
| `itsawamura` | Hitmonlee | Flying kick across the stage |
| *(locate)* | Beedrill | Horizontal drill-sting flyby |
| *(locate)* | Goldeen | Splash — does nothing (the joke) |
| *(locate)* | Starmie | Swift star volley |

Three files above weren't in the first directory listing — enumerate
`itmonster/` fully at implementation and correct this table (likely
`itspear`/`ittosakinto`/`ithitodeman`-style names).

## System features

- Ball throw → impact → open → summon lifecycle; summon ownership (points
  and damage credit to thrower), rarity table (Mew), despawn.
- Summon hitboxes are un-hurtable actors (fighters can't damage them —
  verify exceptions).
- Cries (sampled SFX) + per-summon VFX within pool caps.

## DS notes / risks

- Concurrency: multiple active summons (4 fighters, high spawn rate) is the
  class's stress case — cap per original, measure the worst legal
  combination; this likely defines the final "all items" stress config.
- Snorlax/Onix are screen-dominant actors — draw budget check.
- 13 small actors = pipeline discipline test; resist hand-writing them.

## Acceptance

- [ ] Ball lifecycle + rarity table equivalent.
- [ ] All 13 behaviors equivalent (per-species checklist rows).
- [ ] Credit/attribution + Mew bonus scoring correct.
- [ ] Worst-case summon combination measured; stress config updated
      ("all items" now fully true).
