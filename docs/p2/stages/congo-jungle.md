# Congo Jungle — P2-4 stage 3

Status: not started · Reference: BattleShip stage data via `docs/DECOMP_MAP.md`.

## Content inventory

- **Layout**: large wooden main platform, two lower side platforms, upper
  platforms; open underside.
- **Hazards/interactives**:
  - **Barrel Cannon**: patrols beneath the stage on a path, rotates; a
    fallen fighter enters it, aims with rotation, fires on input (or
    timeout? — verify) — a rescue/KO-mixup mechanic. Two-body-ish state
    (fighter-in-barrel), input semantics and launch power from source.
- **Set pieces**: jungle backdrop, waterfall (animated background — reduced
  rate per visual doctrine is fine).
- **Music**: Congo Jungle (DK) track.
- **Visual treatment**: wood/vine textures, dark palette; waterfall as
  scrolling 2D layer candidate.

## DS notes / risks

- Barrel is fighter-state machinery owned by the stage — implement via the
  stage-actor seam with a fighter-capture state (reuses capture plumbing
  from grabs/eggs).
- Barrel path timing + rotation rate are gameplay (recovery planning);
  source-exact.
- Underside camera: fights extend far below the deck — bounds check.

## Acceptance

- [ ] Collision parity sweep.
- [ ] Barrel: entry, aim, fire, cooldown, path/rotation timing equivalent.
- [ ] Music + SSS entry; owner visual pass with screenshot.
- [ ] 4-CPU stress measurement banked.
