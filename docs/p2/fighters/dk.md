# Donkey Kong — P2-3 fighter 2 (heavy grappler archetype)

Status: not started · Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftdonkey/`

## Role

First structurally new archetype: super-heavyweight, huge hurtbox, no
projectile, and the game's only carry-grab. Proves the pipeline on a fighter
that shares almost nothing with Mario/Fox beyond `ftcommon`.

## Moveset uniques

- **Cargo carry**: grab leads to a carry state — DK walks/jumps while holding
  the victim, victim mashes out, four directional cargo throws. A whole extra
  state machine on both fighters; the hardest single item in the roster
  schedule. Get its ownership right at the shared grab seam (`ftcommon`), not
  as DK-local hacks.
- **Giant Punch (B)**: chargeable in steps, charge is storable across states,
  release armor? (verify), fully-charged properties from source.
- **Spinning Kong (Up-B)**: long horizontal recovery, multi-hit, low vertical.
- **Hand Slap (Down-B)**: ground-only quake, hits grounded opponents only,
  repeatable rhythm.
- Heaviest class: knockback resistance, big ledge-grab reach, slow jumps.

## Assets & audio

Big model — watch the polygon/texture budget (largest fighter silhouette);
bongo/jungle voice set, announcer clip, 4 costumes.

## DS notes / risks

- Carry state must interact correctly with platforms, edges (walking off
  while carrying), KO boundaries (both fighters), throws near blast zones,
  and Sudden Death — enumerate these cases in the acceptance sweep.
- Giant Punch charge persistence across knockdowns/KOs per source.
- Large model = matrix/draw cost outlier candidate; measure vs the
  per-fighter draw budget early, LOD if needed.

## Acceptance

- [ ] Move inventory sweep vs `ftdonkey` data.
- [ ] Cargo matrix: carry × {walk, jump, edge, throw×4, mash-out, KO} cases
      verified.
- [ ] Giant Punch charge-store semantics equivalent.
- [ ] Budgets + stress measurement banked; CSS live; owner feel pass.
