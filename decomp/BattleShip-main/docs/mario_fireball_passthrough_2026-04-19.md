# Mario Fireball passes through opponents — Handoff (2026-04-19) — OPEN

## Symptom

In battle, Mario's neutral-B **Fireball** projectile flies through the opposing fighter without registering a hit. The fireball sprite renders, arcs correctly, and bounces along the ground, but the target takes no damage and has no hitstun. Mario's melee (jabs, kicks, tilts, smashes) hits normally — the bug is isolated to projectile-vs-fighter hit detection.

## Likely related: Samus Charge Shot passthrough

`docs/bugs/samus_charge_shot_hit_detection_2026-04-13.md` documents the same class of failure for Samus's charge shot in the Jungle intro (OPEN — root cause identified as uninitialised `attack_pos[0].pos_prev` / NaN from pool memory, fix attempt did not restore visible behaviour). Mario's fireball is also a `wp` (weapon) entity spawned by a fighter special, and shares the `wpProcessProcWeaponMain` update path and `ftMainSearchHitWeapon` hit-detection code. **Strong hypothesis: same root cause.** If these two share a fix, we should handle both under the charge-shot doc.

## Likely area

- `src/wp/wpmariofireball.c` — fireball spawn + proc_update.
- `src/wp/wpprocess.c` — `wpProcessUpdateHitPositions`, `wpProcessProcWeaponMain`. This is the shared weapon update path where `attack_pos[0].pos_curr` / `pos_prev` get populated.
- `src/ft/ftmain.c` — `ftMainSearchHitWeapon`, the per-fighter hit test against weapons. Gated on `attack_state != nGMAttackStateOff`.
- `src/gm/gmattack.c` — `GMHitBox` interpolation + overlap test.

## Reproduce

Mario vs any fighter, press B on the ground with no charge/hold: Mario throws a fireball that bounces forward. Confirm the fireball visibly passes through the opponent with no knockback and no damage increment on the HUD.

## Diagnostic ideas

- Log `attack_state`, `attack_pos[0].pos_curr`, `attack_pos[0].pos_prev`, and `attack_pos[0].is_active` each frame of the fireball's life from spawn to despawn. Compare against a melee jab that DOES hit.
- If `pos_prev` is NaN / garbage on frame 0, the hit-segment overlap test degenerates and the hit is missed — same failure mode as Samus charge shot.
- If `attack_state` never leaves Off, the search loop is skipping the weapon entirely — inspect spawn-time initialization.

## Next steps

1. Capture the per-frame attack_state / attack_pos log for both Mario fireball (broken) and Mario jab (works) and diff.
2. If the pattern matches Samus charge shot, pursue the fix together and retire both bugs under one entry.
