# Saffron City — P2-4 stage 7

Status: not started · Reference: BattleShip stage data via `docs/DECOMP_MAP.md`;
door Pokémon under `it/` (stage-spawn actors — verify where source keeps them).

## Content inventory

- **Layout**: Silph Co. rooftop main platform flanked by two smaller
  building rooftops with fatal gaps between them; small floating platform;
  ledge-heavy, gap-KO identity.
- **Hazards**:
  - **Pokémon door**: the rooftop door opens on a schedule and releases a
    Pokémon that attacks (set commonly cited as Venusaur, Charmander,
    Porygon, Chansey-with-eggs — **verify the exact set and behaviors in
    source**; Chansey may drop egg items → P2-5 hook).
- **Set pieces**: city skyline, flying Pidgeys/props (background), blinking
  signage (animated texture, reduced rate fine).
- **Music**: Saffron City (Pokémon) track.
- **Visual treatment**: buildings = boxes with signage textures — cheap;
  skyline as BG layers.

## DS notes / risks

- Door Pokémon are spawned stage actors with hitboxes and lifetimes —
  final reuse of the stage-actor seam before Mushroom Kingdom's bespoke set.
- Gap falls between buildings: blast-zone vs pit semantics exact (players
  fall through gaps constantly here).
- If Chansey drops eggs pre-P2-5, stub the egg as damage-only until items
  land (recorded delta), or sequence the stage after P2-5 starts.

## Acceptance

- [ ] Collision parity sweep (three rooftops, gaps, floating platform).
- [ ] Door schedule + Pokémon set/behavior equivalent (source-verified list).
- [ ] Music + SSS entry; owner visual pass with screenshot.
- [ ] 4-CPU stress measurement banked.
