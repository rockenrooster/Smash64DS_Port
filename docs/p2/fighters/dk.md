# Donkey Kong — P2-3 fighter 2 (heavy grappler archetype)

Status: **state machine landed and selectable; acceptance sweep open** ·
Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftdonkey/`

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

## Move inventory (swept 2026-08-24 against the source)

The whole DK state machine is present, cargo included, and the unit doc's old
"not started" was wrong. `src/import/battleship_donkey.c` includes all eleven
behavior TUs verbatim -- `ftdonkeyspecialn/hi/lw` and every one of the nine
`ftdonkeythrowf*` cargo TUs -- and `ftmain.c:64` pulls `ftdonkeystatus.h`, whose
descriptor table carries all 30 statuses the `ftdonkey.h` enum declares:

    AppearR/L, SpecialN{,Air}{Start,Loop,End,Full}, Special{,Air}Hi,
    SpecialLw{Start,Loop,End}, ThrowFWait, ThrowFWalk{Slow,Middle,Fast},
    ThrowFTurn, ThrowFKneeBend, ThrowFFall, ThrowFLanding, ThrowFDamage,
    ThrowFF, ThrowAirFF, HeavyThrow{F,B,F4,B4}

So the cargo ladder is not implementation work; it is UNVERIFIED work. The
bounded proof that closed P2-3r3 exercised Giant Punch, Spinning Kong, Hand
Slap and a driven KO -- it does not touch grab, carry, walk-while-carrying, the
four cargo throws, or mash-out. That is what the cargo matrix below still owes.

## Acceptance

- [x] Move inventory sweep vs `ftdonkey` data (2026-08-24, above).
- [ ] Cargo matrix: carry × {walk, jump, edge, throw×4, mash-out, KO} cases
      verified.
- [ ] Giant Punch charge-store semantics equivalent.
- [ ] Budgets + stress measurement banked; CSS live; owner feel pass.
