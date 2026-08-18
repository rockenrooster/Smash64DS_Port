# Items: Containers — Crate, Barrel, Capsule, Egg (P2-5 class 1)

Status: not started · Reference: `decomp/BattleShip-main/decomp/src/it/itground/`
+ `itmanager.c`/`itmap.c` (spawn/payout).

First class because containers ARE the spawn infrastructure exercised: they
spawn, they hold payloads, they break into more items.

## Members & behavior (source-exact)

- **Crate**: heavy two-hand carry (slows carrier), breaks on damage/impact,
  3-item payload roll; chance to be explosive (blast on break).
- **Barrel**: like crate but **rolls** when thrown/landed on slopes — rolling
  is a moving hazard with its own hitbox; explosive chance.
- **Capsule**: light one-hand throwable, 1-item payload, explosive chance.
- **Egg**: Yoshi-flavored capsule variant (appears on certain stages /
  Chansey — verify spawn sources), same payload machinery.

## System features this class proves

- Spawn scheduler + per-stage spawn regions + payload roll tables.
- Carry-weight modifier on fighters (heavy-carry state — baked anims from
  P2-3 pipeline).
- Break-on-damage / break-on-impact thresholds; explosive variant rolls and
  blast hitboxes (self-damage attribution rules).
- Item-on-item interactions (payload items popping out with velocities).

## DS notes / risks

- Rolling barrel on slope stages (Yoshi's Island!) — slope physics reuse.
- Explosion VFX through effect-pool caps; blasts are the class's perf spike.

## Acceptance

- [ ] Spawn/payout tables equivalent to `itmap`/`itmanager` data.
- [ ] Heavy-carry movement modifiers equivalent.
- [ ] Explosive rolls + blast damage/attribution equivalent.
- [ ] Stress measurement with containers-heavy spawn banked.
