# Jigglypuff — P2-3 fighter 9

Status: not started · Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftpurin/`
(`purin` = Jigglypuff's Japanese name — the decomp uses it throughout.)

## Role

Multi-jump aerial drift specialist; mechanically small after Ness/Yoshi, so a
quick close-out before Kirby. Unlockable (gating P2-7).

## Moveset uniques

- **Five midair jumps** — shares the multi-jump machinery Kirby will reuse;
  land it here first.
- **Pound (B)**: horizontal drift attack, air-stall/recovery tool.
- **Sing (Up-B)**: radius sleep on grounded opponents, no recovery
  properties; sleep state (opponent-side) lands here and is reused by items
  later if applicable.
- **Rest (Down-B)**: instant sleep with a 1-frame-active point-blank KO
  hitbox — frame-1 exactness matters; this move is a community benchmark.
- Lightest fighter, slowest fall, strongest air drift; shield-break launch
  (instant upward KO on shield break — verify the special-case in source).

## Assets & audio

Tiny model (cheapest in roster), 4 costumes (flower/bow/hat), Jigglypuff
voice samples, sleep VFX ("Zzz"), announcer clip.

## DS notes / risks

- Rest's frame-1 hitbox and Sing's wake rules are exactness-sensitive; take
  them from frame data directly.
- Sleep (opponent state) interacts with damage/mash-wake scaling — shared
  seam, will be reused (Mr. Saturn? no — but Sing + future items), keep it
  in `ftcommon` equivalent.

## Acceptance

- [ ] Move inventory sweep vs `ftpurin` data.
- [ ] Rest frame-1 KO reproduces (replay-verified); Sing radius/duration
      equivalent.
- [ ] Multi-jump count/decay equivalent.
- [ ] Budgets + stress measurement banked; CSS live; owner feel pass.
