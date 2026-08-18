# Meta Crystal — P2-6 venue (Metal Mario)

Status: not started · Reference: BattleShip stage data via `docs/DECOMP_MAP.md`.

1P-only: Metal Mario's cavern.

## Content inventory

- **Layout**: irregular crystal-cave terrain — uneven main floor with a
  raised right shelf and a small overhang platform (verify exact surfaces
  from data); tight blast zones relative to layout.
- **Hazards**: none.
- **Set pieces**: crystal formations, cave backdrop.
- **Music**: Meta Crystal theme.
- **Visual treatment**: shiny crystal material (env-map approximation shared
  with Metal Mario's material work — build the metal/crystal material once,
  use twice).

## DS notes / risks

- Terrain is the roughest non-flat collision after Yoshi's Island — good
  regression pair for the slope sweep.
- Pairs with `fighters/variants.md` Metal Mario — schedule together inside
  P2-6.

## Acceptance

- [ ] Collision parity sweep (irregular floor, shelf, overhang).
- [ ] Camera/blast zones equivalent.
- [ ] Music live; campaign-flow load works; owner visual pass.
