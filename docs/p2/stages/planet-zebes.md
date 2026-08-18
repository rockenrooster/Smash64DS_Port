# Planet Zebes — P2-4 stage 5

Status: not started · Reference: BattleShip stage data via `docs/DECOMP_MAP.md`.

## Content inventory

- **Layout**: uneven main terrain over an open pit, several pass-through
  platforms at varying heights, one moving platform (verify), asymmetric
  ledges.
- **Hazards**:
  - **Acid**: rises from below on a schedule to varying heights (sometimes
    flooding most of the stage), damages and launches upward on contact;
    rise/fall timing table, damage, and launch values from source. The
    launch is survival-relevant (acid can save recoveries — players use it).
- **Set pieces**: cavern background, Metroid-esque ambience props.
- **Music**: Planet Zebes (Metroid) track.
- **Visual treatment**: acid = animated translucent plane (scrolling texture,
  reduced update rate fine); cavern as dark baked geometry + BG layer.

## DS notes / risks

- Acid is a full-width dynamic hurt-surface with a height function —
  implement as stage-owned surface, not a particle; its contact test joins
  the fighter ground/hurt seam.
- Translucent full-width plane per frame: fill-rate/polygon budget check on
  DS (single quad strip, not per-cell geometry).
- Acid interplay with items (floating? destroyed?) — verify when P2-5 lands;
  leave a hook row.

## Acceptance

- [ ] Collision parity sweep.
- [ ] Acid schedule/heights/damage/launch equivalent.
- [ ] Music + SSS entry; owner visual pass with screenshot.
- [ ] 4-CPU stress measurement banked.
