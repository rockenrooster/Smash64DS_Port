# Fighter Variants — Metal Mario, Giant DK, Fighting Polygon Team (P2-6)

Status: not started · References:
`ft/ftchar/ftmmario/` (Metal Mario), `ft/ftchar/ftgdonkey/` (Giant DK),
`ft/ftchar/ftn*/` (12 polygon fighters: `ftnmario`, `ftnfox`, `ftndonkey`,
`ftnlink`, `ftnsamus`, `ftnyoshi`, `ftnkirby`, `ftnpikachu`, `ftnpurin`,
`ftnness`, `ftncaptain`, `ftnluigi`).

All three are 1P-campaign opponents built as *variants over landed base
fighters* — stat/material/scale overlays through the pipeline's variant path
(proven by Luigi), not new implementations.

## Metal Mario (`ftmmario`)

- Mario's kit with heavy-stat overrides (knockback taken massively reduced,
  fall speed up) — exact multipliers from source.
- Metal material (DS: env-mapped/reflective approximation within visual
  doctrine) + metal footstep/hit SFX set; no voice? (verify).
- Venue: Meta Crystal (`stages/meta-crystal.md`).

## Giant DK (`ftgdonkey`)

- DK scaled up with stat overrides; fought as 1-vs-3 (player + 2 CPU allies)
  — depends on P2-2 team/ally machinery.
- Scale touches hurtbox/hitbox/grab ranges and camera framing — verify the
  source treats it as data scale, mirror that.
- Venue: Congo Jungle.

## Fighting Polygon Team (`ftn*`, ×12)

- Low-poly purple doppelgangers — **natively cheap on DS**; their N64 models
  are already near DS budgets, convert nearly as-is.
- Reduced movesets per source (verify per `ftn*` dir exactly which states
  exist — commonly no specials/reduced kit); very light, easy KOs.
- Fought 3-concurrent from a 30-stock wave pool on Duel Zone — wave
  spawn/despawn reuses respawn machinery; watch heap watermarks across waves.

## Acceptance

- [ ] Each variant's stat/scale/material deltas sourced from its `ft*` dir,
      not invented.
- [ ] Giant DK ally battle and Polygon wave battle correct on the 4-fighter
      engine.
- [ ] Polygon wave of 3 + player + items within stress envelope (measured).
- [ ] Owner visual pass on metal/polygon materials.
