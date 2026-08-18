# Peach's Castle — P2-4 stage 2

Status: not started · Reference: BattleShip stage data via `docs/DECOMP_MAP.md`.

## Content inventory

- **Layout**: castle rooftop main deck with angled side ramps, raised side
  platforms, and the tower structure; asymmetric ledges.
- **Hazards/interactives**:
  - **Bumper**: fixed above center stage — strong fixed-knockback bounce on
    contact; exact knockback/priority from source (it interrupts combos and
    recoveries; players know its feel).
  - **Sliding platform**: the platform that traverses beneath/beside the
    stage on a rail (verify exact path/timing from source data) — moving
    collision carrier: fighters/items must ride it correctly.
- **Set pieces**: castle towers, background Lakitu? (verify background-only
  props).
- **Music**: Peach's Castle (SMB medley) track.
- **Visual treatment**: bright low-poly architecture — near-direct
  conversion; skybox as 2D BG.

## DS notes / risks

- First *moving platform carrier* in P2 — riding logic (fighter velocity
  inheritance, items later) lands here; get it at the shared platform seam,
  Dream Land had none.
- Bumper is the first stage-owned hitbox — wire through the engagement
  system as a stage actor, the pattern every later hazard reuses.

## Acceptance

- [ ] Collision parity sweep incl. moving platform ride/dismount cases.
- [ ] Bumper knockback equivalent (source values).
- [ ] Platform path/timing equivalent.
- [ ] Music + SSS entry; owner visual pass with screenshot.
- [ ] 4-CPU stress measurement banked.
