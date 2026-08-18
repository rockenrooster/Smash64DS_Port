# Kirby — P2-3 fighter 10 (last: copy needs everyone)

Status: not started · Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/`

## Role

Scheduled last because **Copy requires every other fighter's neutral-B to
exist**. Everything else about Kirby is ordinary multi-jump lightweight
(machinery already landed with Jigglypuff).

## Moveset uniques

- **Inhale (B)**: vacuum windbox → swallow (two-body state, reuse the
  DK/Yoshi seam) → choice: spit as star projectile, or **Copy**.
- **Copy**: gains the victim's neutral-B (11 copyable specials + the matching
  hat model per character; loses on damage threshold/taunt per source).
  Implementation = per-fighter copy entries produced by the P2-3 pipeline as
  each fighter lands (each fighter's neutral-B must be callable from Kirby's
  state context — plan for that in the pipeline from Luigi onward, so
  Kirby-day is assembly, not surgery).
- **Final Cutter (Up-B)**: rise/fall slash with a landing shockwave
  projectile.
- **Stone (Down-B)**: transform, invincible? (heavy armor — verify exact 64
  rules), can cancel; hat/copy kept through Stone.
- Five puffs (multi-jump), lightest-class, crouch is nearly flat.

## Assets & audio

Round model + **12 hat variants** (one per copyable fighter incl. himself for
mirror matches) — an asset batch, budget it; 4+ costumes; Kirby voice + the
copied-B voice line variants where the original had them; announcer clip.

## DS notes / risks

- Copy is a code-size and asset bomb if done as forks — it must be "call the
  existing per-fighter special through the seam", which is why the pipeline
  carries a copy-entry requirement from fighter 1.
- Copy × item-hold × multi-jump state interactions: run the full inventory
  sweep per copied power (12 sweeps, scripted).
- Hat attach uses the item/hand attach transform path where possible.

## Acceptance

- [ ] Move inventory sweep vs `ftkirby` data.
- [ ] All 11 copy powers + hats verified (scripted per-power sweep, incl.
      loss rules).
- [ ] Inhale two-body matrix (spit star, copy, escape, edge/KO cases).
- [ ] Budgets (hat batch included) + stress measurement banked; CSS live;
      owner feel pass.
