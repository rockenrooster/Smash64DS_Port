# Bonus Stages — Break the Targets ×12 + Board the Platforms ×12

Status: not started · Reference: `decomp/BattleShip-main/decomp/src/gr/grbonus/`
+ per-board data via `docs/DECOMP_MAP.md`.

24 per-fighter mini-boards sharing two rule sets. Logic lands once; boards are
data through the P2-4 pipeline.

## Shared logic (once)

- **Break the Targets (Bonus 1)**: 10 targets, timer counts up, break all /
  fail on KO; target hit detection from any damage source (projectiles,
  items? — verify what counts in source).
- **Board the Platforms (Bonus 2)**: 10 landing platforms, stand-to-claim
  semantics (verify exact claim rule), timer, fail on KO.
- Results/records: best times per fighter per mode feed Records (P2-7 save);
  campaign inserts the player-fighter's boards as Bonus 1/2 with its scoring.
- Practice entry: standalone Bonus Practice menu (P2-7) selects
  fighter × mode.

## Board production order

Mario's two boards first (campaign dependency, P2-6). The other 22 batch as
their fighter lands (each board is that fighter's movement puzzle — it can't
be verified before its fighter exists). Board = collision + target/platform
placements + camera volume + backdrop; expect several boards per day through
the pipeline once it's warm.

| Fighter | BTT | BTP | | Fighter | BTT | BTP |
|---|---|---|---|---|---|---|
| Mario | ☐ | ☐ | | Kirby | ☐ | ☐ |
| Fox | ☐ | ☐ | | Pikachu | ☐ | ☐ |
| DK | ☐ | ☐ | | Jigglypuff | ☐ | ☐ |
| Samus | ☐ | ☐ | | Ness | ☐ | ☐ |
| Link | ☐ | ☐ | | Falcon | ☐ | ☐ |
| Yoshi | ☐ | ☐ | | Luigi | ☐ | ☐ |

## DS notes / risks

- Boards are movement-tech oracles: each one silently verifies its fighter's
  jumps/specials (Samus bomb-jumps, Ness PKT2, Yoshi DJ) — failures here are
  usually fighter bugs, not board bugs. That makes them cheap regression
  fixtures; wire the timer records into the replay verifier.
- Sound Test unlock depends on clearing all boards (P2-7) — records must
  distinguish per-board completion.

## Acceptance

- [ ] Both rule sets equivalent (targets, claims, timers, fail paths).
- [ ] All 24 boards landed and cleared start-to-finish (scripted or owner).
- [ ] Records feed save data; campaign insertion works (Mario first).
- [ ] Cadence held; screenshots per board set recorded.
