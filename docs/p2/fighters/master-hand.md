# Master Hand — P2-6 boss

Status: not started · Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/`

## Role

The campaign's final boss. Not a fighter variant: HP-based (no knockback, no
hitstun), scripted attack patterns, flies outside blast zones freely. Budget
it like a new fighter, not a stage prop.

## Mechanics (all from `ftboss` — the list below is orientation, not spec)

- HP pool by difficulty; defeat at 0 HP → explosion/fly-off → campaign clear.
- Attack script set: finger walk/poke/flick, slap sweeps, fist drop, drill
  dive, finger-gun shots, grab (squeezes the player for big damage —
  two-body grab seam again), fly-off-screen swoop re-entries.
- Pattern selection/AI cadence per difficulty from source.
- Player-side: Master Hand cannot be grabbed/thrown; some attacks pass
  through platforms? (FD is flat — verify what matters).
- Intro: hand flies in over Final Destination; distinctive laugh SFX.

## Assets & audio

Glove model with articulated fingers (highest joint count of any non-fighter
actor — check the matrix budget), gun-shot/laugh/swoosh SFX, boss music =
Final Destination track (`stages/final-destination.md`).

## DS notes / risks

- His long sweeping motions cover the whole stage — camera must frame player
  + hand without the fighter-bounds logic fighting the boss position; boss
  camera rules from source.
- Off-screen states: he leaves the play volume by design — culling and
  effect budgets must tolerate a huge actor entering/leaving.
- HP HUD element (his damage displays as HP, player still shows %) on the
  bottom-screen HUD.

## Acceptance

- [ ] Full pattern set present with per-difficulty behavior from source.
- [ ] Grab attack two-body flow correct; un-grabbable rule holds.
- [ ] Whole fight within cadence/tick envelope on Final Destination
      (measured, all difficulties spot-checked).
- [ ] Owner plays the fight; feel pass.
