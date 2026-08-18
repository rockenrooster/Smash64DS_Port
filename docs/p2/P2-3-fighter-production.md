# P2-3 — Fighter Production (pipeline + the remaining 10)

Industrializes what built Mario and Fox, then batches the roster through it.
Per PROJECT_GOAL: generic build tooling, specialized runtime output — each
fighter may land as its own native `X_Update()`/`X_Draw()` implementation.

## Pipeline generalization (first slice, before any new fighter)

Inventory how Mario/Fox were produced and turn every manual step into
tooling:

1. **Moveset import**: action/state tables, frame data, hitboxes, knockback,
   physics constants from `ft/ftchar/ft<name>/` + shared `ftcommon` — the
   generator consumes BattleShip data, never hand-copied numbers.
2. **Asset conversion**: model → DS-budget geometry, textures → native DS
   formats within the P2-2 per-fighter budget; costume/team palettes.
3. **Animation bake**: figatree → precomputed DS pose streams (existing P1
   path), **including the item-hold/swing/throw animation set** so P2-5 never
   retrofits fighters.
4. **VFX/SFX/voice**: per-fighter effect assets, voice bank, announcer clip,
   crowd chant; sound-RAM budget enforced at build time.
5. **CSS/UI assets**: portrait, stock icon, name, series emblem; slot
   unlocks on the CSS as each lands.
6. **Equivalence harness**: per-fighter acceptance = scripted move inventory
   run (every ground/air/smash/special/grab/throw/ledge/tumble state visited)
   + CPU-vs-CPU determinism replay + owner feel pass, per `VERIFYING.md`.

Luigi proves the pipeline (variant path); DK proves it on a structurally
different archetype. If either needs manual one-offs, fix the pipeline before
fighter 3.

## Roster order (owner-ratified engineering order)

| # | Fighter | File | Archetype / why this slot |
|---|---|---|---|
| 1 | Luigi | `fighters/luigi.md` | Mario variant — proves variant path cheap |
| 2 | Donkey Kong | `fighters/dk.md` | Heavy grappler; cargo-carry is the hardest new state machine |
| 3 | C. Falcon | `fighters/falcon.md` | Fast faller, no projectile — cheap, exercises speed extremes |
| 4 | Samus | `fighters/samus.md` | Storable charge projectile, heavy floaty |
| 5 | Link | `fighters/link.md` | Boomerang return + bomb pull (item-system adjacency, lands near P2-5) |
| 6 | Pikachu | `fighters/pikachu.md` | Terrain-crawling projectile, double teleport |
| 7 | Yoshi | `fighters/yoshi.md` | Unique shield/armor rules, egg states |
| 8 | Ness | `fighters/ness.md` | PK Thunder controllable projectile + PKT2 recovery, absorb |
| 9 | Jigglypuff | `fighters/jigglypuff.md` | Multi-jump, Rest — simple close-out |
| 10 | Kirby | `fighters/kirby.md` | LAST: copy ability needs everyone's neutral-B + hat assets |

Metal Mario, Giant DK, Fighting Polygons, Master Hand are P2-6 content
(`fighters/variants.md`, `fighters/master-hand.md`) but reuse this pipeline.

## Standing rules for every fighter row

- Inspect `ft/ftchar/ft<name>/` before implementation; shared mechanics live
  in `ftcommon` — port at the owning seam, no per-fighter forks of shared
  defects.
- Projectiles/articles: BattleShip keeps many in `wp/` and `it/itfighter/` —
  check both before calling a special "new code".
- Land SELECTABLE: CSS slot, portraits, announcer, voice, all costumes.
- Measure under the current stress config before closing (budget law +
  stress-config law from `P2_PLAN.md`).
- Unlockable characters (Luigi, Ness, Falcon, Jigglypuff) are selectable in
  dev builds; unlock *gating* arrives in P2-7.

## Exit criteria

- [ ] Pipeline documented and reproducible (a fighter rebuilds from BattleShip
      data + assets by `make`).
- [ ] All 10 fighters landed per the unit DoD (each unit file's checklist).
- [ ] Any-4-fighter combination fits the P2-2 budgets (spot-audited: heaviest
      4 by measured cost).
- [ ] Stress config re-argmaxed over the full roster; board updated.
