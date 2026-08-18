# Mushroom Kingdom — P2-4 stage 8 (most bespoke; unlockable)

Status: not started · Reference: BattleShip stage data via `docs/DECOMP_MAP.md`.

## Content inventory

- **Layout**: the only walk-off stage — side blast lines at ground level, no
  ledges at the extremes; brick platforms, center gap bridged by moving lift
  platforms (verify configuration), classic SMB tile look.
- **Hazards/interactives** (most bespoke systems in the VS set):
  - **Warp pipes**: enterable, teleport between pipe pairs (occupancy and
    exit rules from source).
  - **POW block**: hittable, quakes all grounded opponents (uses/reset rules
    from source).
  - **Piranha Plants**: emerge from pipes on timers, bite hitboxes.
- **Set pieces**: SMB flat-tile aesthetic, overworld backdrop.
- **Music**: Mushroom Kingdom (SMB overworld arrangement) track.
- **Unlock**: unlockable stage — condition verified from source, gated in
  P2-7 (selectable in dev builds).
- **Visual treatment**: flat 2D-tile identity — candidate for BG-layer
  construction with minimal 3D (the stage practically asks for the DS's 2D
  hardware; visual doctrine explicitly allows it).

## DS notes / risks

- Walk-off blast lines change KO/camera semantics (no ledge play at edges,
  camera clamps differently) — the one stage that exercises those paths.
- Pipe warp = fighter teleport state with occupancy — small bespoke state
  machine; keep stage-owned.
- Scheduled last precisely because its systems (warp, POW, Piranhas, lifts)
  are one-offs; nothing downstream depends on them.

## Acceptance

- [ ] Collision parity sweep (walk-offs, tiles, lifts).
- [ ] Pipes/POW/Piranhas/lifts equivalent (source-verified behaviors).
- [ ] Walk-off camera + KO semantics verified.
- [ ] Music + SSS entry (unlock-gated by P2-7); owner visual pass.
- [ ] 4-CPU stress measurement banked.
