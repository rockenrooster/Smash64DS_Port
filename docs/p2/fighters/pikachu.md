# Pikachu — P2-3 fighter 6

Status: not started · Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftpikachu/`

## Role

The terrain-following projectile and the double-teleport recovery — two
mechanics nothing earlier in the order exercises. Small hurtbox extreme.

## Moveset uniques

- **Thunder Jolt (B)**: hops along the ground, follows terrain contours,
  crawls down walls and around ledges, dissipates on time/impact; air
  version arcs then crawls on landing. Terrain-following = per-step collision
  queries — precompute per-stage crawl paths if profiling demands (allowed by
  the optimization doctrine; behavior must stay equivalent).
- **Thunder (Down-B)**: cloud spawns at top, bolt descends to Pikachu, hits
  along its length; self-hit interplay (bolt striking Pikachu has its own
  hit) per source; screen-length article.
- **Quick Attack (Up-B)**: two chained teleport segments with distinct angle
  choice, brief vulnerability rules, no hitbox (in 64 — verify).
- Small, fast, strong edge game; famous u-smash/b-throw KO power.

## Assets & audio

Small model (cheapest draw in roster), 4 costumes (party-hat variants),
voice = actual "Pika" samples (identity-critical), announcer clip.

## DS notes / risks

- Thunder's tall bolt: fill-rate + effect pool; consider 2D-composited bolt
  (billboard doctrine) — visual doctrine allows it if telegraphs stay exact.
- Quick Attack across platforms/walls — teleport collision resolution
  equivalence.
- Thunder Jolt on moving/irregular stages (Congo barrel area, Zebes acid
  slopes) — per-stage crawl verification rows when those stages land.

## Acceptance

- [ ] Move inventory sweep vs `ftpikachu` data.
- [ ] Thunder Jolt crawl paths equivalent on Dream Land + each landed stage.
- [ ] Thunder bolt/self-hit semantics equivalent.
- [ ] Quick Attack segment/angle rules equivalent.
- [ ] Budgets + stress measurement banked; CSS live; owner feel pass.
