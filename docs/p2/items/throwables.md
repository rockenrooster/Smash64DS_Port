# Items: Throwables — Shells, Bob-omb, Motion-Sensor Bomb, Bumper (P2-5 class 4)

Status: not started · Reference: `decomp/BattleShip-main/decomp/src/it/itground/`
+ `itfighter/` (thrown-state physics).

Items whose thrown/placed state is the item — self-propelled or trap actors.

## Members & behavior (source-exact)

- **Green Shell**: slides on throw/hit, ricochets off walls, hurts everyone
  in its path including the thrower after bounces (ownership decay rules
  from source); stompable to launch.
- **Red Shell**: homing — patrols the surface hunting the nearest fighter,
  hard to stop (despawn rules from source); a persistent area-denial actor.
- **Bob-omb**: **walks on its own** when left idle (wander → fuse → blast),
  huge explosion, chain-reacts; famous Sudden Death rain (verify: Bob-omb
  rain in stalling Sudden Death is original behavior — implement with it).
- **Motion-Sensor Bomb**: plants flush on any surface (wall/ceiling too? —
  verify), arms after delay, proximity-triggers a strong blast; near-invisible
  when planted — readability is gameplay-relevant, keep the original's
  visibility level.
- **Bumper**: placeable version of Peach's Castle's bumper — sits on the
  ground where thrown, bounces fighters on contact, timed despawn. Shares
  the stage bumper's contact behavior (one implementation, two owners).

## DS notes / risks

- Red Shell + Bob-omb are autonomous surface-walkers — they reuse the
  terrain-following machinery family (Pikachu's Thunder Jolt precedent).
- Concurrent shells at 4 fighters = engagement load; caps per original.
- Sudden Death Bob-omb rain is a scripted spawn mode in the match driver —
  wire when this class lands.

## Acceptance

- [ ] Per-item motion/fuse/trigger/blast tables equivalent.
- [ ] Ownership/attribution decay (shell hits thrower) equivalent.
- [ ] Sudden Death rain behavior verified.
- [ ] Stress measurement with throwables active banked.
