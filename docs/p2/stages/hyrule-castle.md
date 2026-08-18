# Hyrule Castle — P2-4 stage 4

Status: not started · Reference: BattleShip stage data via `docs/DECOMP_MAP.md`.

## Content inventory

- **Layout**: the largest walk-around layout so far — main courtyard, ramp,
  raised left tower ledge, right-side tent/roof section with gaps; mostly
  hard floors, few pass-throughs (verify which).
- **Hazards**:
  - **Whirlwind (tornado)**: spawns periodically at ground positions,
    wanders, lifts and flings fighters with rising damage; spawn cadence,
    positions, and launch from source. (Wind precedent: P1's Whispy — but
    this one carries and launches, a stronger coupling with fighter
    physics.)
- **Set pieces**: castle architecture, distant background.
- **Music**: Hyrule Castle (Zelda) track.
- **Visual treatment**: stone texture set; large static geometry — bake
  aggressively.

## DS notes / risks

- Size: widest camera pulls in the VS set until Sector Z — draw-distance
  and stage-geometry submission budget checkpoint; if the stage mesh needs
  chunked culling, build it here and Sector Z inherits it.
- Tornado's carry state vs DI/mash rules — source-exact; it's a KO setup
  players know frame-deep.
- Big flat areas + 4 fighters spread = camera at maximum zoom-out; fighter
  LOD trigger check.

## Acceptance

- [ ] Collision parity sweep (large-surface set, tent gaps).
- [ ] Tornado spawn cadence/motion/launch equivalent.
- [ ] Camera bounds at 4-fighter maximum spread verified.
- [ ] Music + SSS entry; owner visual pass with screenshot.
- [ ] 4-CPU stress measurement banked (expected stress-config candidate).
