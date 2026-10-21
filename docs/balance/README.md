# Balance

Everything about fighter and stage balance in one place: the balance sheet,
the owner's balance changes in force, and where each change lives. The
owner's brief is [P7_Plan.md](../P7/P7_Plan.md) ("Fighter Balance", "Stage
Balance"); the work items are Board 3 of the
[P7 implementation plan](../P7/P7_Implementation_Plan.md#board-3--balance) and
rows B1-B5, S0 and S1 of [P7_STATUS.md](../P7/P7_STATUS.md).

## The balance sheet

`balance.html` in this folder: every fighter's attacks as the game runs them
(each hitbox's active frames, damage, angle, knockback growth, base and fixed
knockback, size and element; throws' own launches), by fighter or one move
compared across the roster, with fighter scores and outlier attacks. It opens
with a double-click (no server; it renders without scripts too), and
`balance.json` holds the same values.

Rebuild it after any balance change:

    pwsh -File scripts/p7/build-balance-table.ps1 -Open

(or the shortcut in this folder). The generator, `scripts/p7/balance_table.py`,
reads what the build reads: BattleShip's motion scripts for the original
twelve, the P4 export over the staging ROM for the Remix and EXTRA fighters
(with the generator's owner edits below), and `scripts/p6/<name>/` for Ike
and Palutena.

The values are the game's own data, so the page and the JSON stay on this
machine: `.gitignore` keeps them out of the repository (it is public).

## Fighter changes in force

| Fighter | Change | Owner | Where |
|---|---|---|---|
| Wario | jump height 0.75x (B1) | 2026-10-11 | `OWNER_ATTRIBUTE_SCALES` (`scripts/p4/generate_p4_fighter.py`) |
| Bowser | walk and run 0.75x, animations unchanged (fixes the foot slide; B2) | 2026-10-12 | `OWNER_ATTRIBUTE_SCALES` |
| Sheik | every animation 1.1x (B3) | 2026-10-14 | `OWNER_ANIM_RATES` |
| King Dedede | neutral A hitboxes 1.25x (B4) | 2026-10-14 | `OWNER_HITBOX_SCALES` |
| Metal Mario (selectable, VS only) | gravity 1.25x; the dash and run changes reverted (B5) | 2026-10-13, 2026-10-18 | `battleship_ftmanager.c` (a VS copy of his attributes; the 1P boss is the source's) |
| Bowser | +10% damage on every attack but his smashes (throws included) | 2026-10-19 (p7h) | `OWNER_DAMAGE_SCALES`, `OWNER_DAMAGE_SCALE_SKIPS_SMASH` |
| Wario | -5% damage on every attack (throws included) | 2026-10-19 (p7h) | `OWNER_DAMAGE_SCALES` |
| Ike | timing, damage, angles and knockback from the owner's Ultimate attack data | 2026-10-15 (p6r) | `scripts/p6/ike/motions/*.ms`, [card](../P6/characters/ike.md) |
| Ike | dash and run 0.75x of Roy's, the run clips at 0.75 with them | 2026-10-15 (p6r) | `OWNER_ATTRIBUTE_SCALES`, `OWNER_ANIM_RATES` / `OWNER_ANIM_RATE_STATUSES` |
| Palutena | Ultimate's attack data (p6r), then the owner's p7h knockback and size edits: dash attack, tilts, up smash, neutral and up air, Explosive Flame | 2026-10-15, 2026-10-19 | `scripts/p6/palutena/motions/*.ms`, [card](../P6/characters/palutena.md) |

Damage scales apply after staling and round half up (`ndsP4StaledDamage`,
`src/port/nds_p4.c`).

## Stage changes in force

| Stage | Change | Owner | Where |
|---|---|---|---|
| Bowser's Stadium | 1.5x wide (S1): floor, spawns, the bombs (x +-4485), camera sides; blast sides move with the ledges, top and bottom kept | 2026-10-13 | `scripts/p5/stages.json` `owner_scale`, `scripts/p5/owner_scale.py` |
| Final Destination | the N64 original, unchanged | 2026-10-13 | |

## Making a change

1. The value goes in the owner table that owns it (above); generated output
   is never edited by hand.
2. Every balance change is a gameplay change: the replay digests change with
   it, so record a new baseline.
3. Check the CPU still recovers (lower jumps and heavier gravity can break
   it), rebuild the sheet, and note the change in the fighter's card and in
   the table above.
