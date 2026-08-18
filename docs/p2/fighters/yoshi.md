# Yoshi — P2-3 fighter 7

Status: not started · Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftyoshi/`

## Role

The rules-exception fighter: unique shield, armored double jump, no third
jump. Exercises every "special-case the shared systems" seam.

## Moveset uniques

- **Egg Lay (B)**: swallows the opponent and turns them into an egg —
  opponent-side state (egg struggle/mash-out, damage-in-egg rules). Two-body
  state machine like DK cargo; reuse that seam pattern.
- **Egg Throw (Up-B)**: lobbed egg projectile, arc control; **no recovery
  height** — Yoshi's recovery is his armored double jump instead.
- **Yoshi Bomb (Down-B)**: ground-pound with star spray on landing.
- **Egg Shield**: shield is an egg — does not shrink like normal shields;
  its own poke/break rules (verify exact 64 semantics in source).
- **Double-jump armor**: heavy knockback armor during the second jump — the
  recovery identity; exact armor thresholds from source.

## Assets & audio

Round model, 6 costumes (verify count), Yoshi voice samples, egg VFX,
announcer clip.

## DS notes / risks

- Shield exception must live at the shared shield seam as a declared
  variant, not a Yoshi-local copy of shield code.
- Armor is a knockback-pipeline exception — verify it composes with items
  (Star invincibility, later) and Sudden Death.
- Egg (opponent) state must handle KO-in-egg, timer expiry, thrower KO'd.

## Acceptance

- [ ] Move inventory sweep vs `ftyoshi` data.
- [ ] Egg Lay two-body matrix (mash-out, KO, edge cases) equivalent.
- [ ] Egg Shield + DJ armor thresholds equivalent.
- [ ] Budgets + stress measurement banked; CSS live; owner feel pass.
