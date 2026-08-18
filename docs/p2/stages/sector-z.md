# Sector Z — P2-4 stage 6 (biggest stage; perf checkpoint)

Status: not started · Reference: BattleShip stage data via `docs/DECOMP_MAP.md`.

## Content inventory

- **Layout**: the Great Fox — the largest stage in the game: long hull deck,
  tail fin, wing surfaces as platforms, under-wing pocket; distinct ledge set.
- **Hazards**:
  - **Arwing**: flies in periodically, hovers, fires laser bursts across the
    deck (lasers hit both ways? — verify friendly-fire semantics), then
    leaves; spawn cadence/paths/laser damage from source. The Arwing body is
    solid? (verify — riding it is a known novelty).
- **Set pieces**: space/planet background, engine glow.
- **Music**: Sector Z (Star Fox) track.
- **Visual treatment**: one big mostly-static ship — bake everything; space
  backdrop as 2D BG layers; engine glow as billboard effects.

## DS notes / risks

- **The perf checkpoint of P2-4**: largest camera volume + widest fighter
  spreads. Inherits Hyrule's chunked culling if built; expected stress-config
  stage candidate alongside Hyrule.
- Arwing = moving stage actor with hitboxes and (verify) rideable collision —
  the most actor-like hazard; reuses the stage-actor seam from Peach's
  bumper/Congo barrel.
- Long flat deck = degenerate broadphase case (everyone colinear) — check
  the P2-2 broadphase doesn't degrade.

## Acceptance

- [ ] Collision parity sweep (hull, wings, under-wing, fin).
- [ ] Arwing cadence/path/laser behavior equivalent (+ ride rule verified).
- [ ] Camera bounds at maximum spread; fighter LOD behavior verified.
- [ ] Music + SSS entry; owner visual pass with screenshot.
- [ ] 4-CPU stress measurement banked (stress-config candidate).
