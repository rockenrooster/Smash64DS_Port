# Items: Passives — Maxim Tomato, Heart Container, Star (P2-5 class 5)

Status: not started · Reference: `decomp/BattleShip-main/decomp/src/it/itground/`.

Touch/consume items with no combat state — the smallest class.

## Members & behavior (source-exact)

- **Maxim Tomato**: heals a fixed amount (~100? exact from source) on
  pickup; consumed instantly (verify: instant vs held in 64 — I believe
  instant on pickup).
- **Heart Container**: full heal to 0%; slow-falls from spawn height
  (distinct float-down physics).
- **Star (Starman)**: invincibility for a duration — the invincibility state
  already exists (respawn invulnerability) but Star layers the flashing
  rainbow visual + the classic Starman jingle **overlaying BGM** (music duck/
  restore behavior like Hammer's — shared audio-override seam), and it must
  compose with armor/shield rules per source.

## DS notes / risks

- Trivial mechanically; the Star's audio override and its interaction with
  Yoshi DJ armor / respawn invuln stacking are the only real rows.
- Rainbow flash on DS palettes: approximate within visual doctrine
  (recognizable flashing, screenshot for the record).

## Acceptance

- [ ] Heal amounts + Heart float physics equivalent.
- [ ] Star duration, stacking rules, jingle override/restore equivalent.
- [ ] Stress impact negligible (spot measurement).
