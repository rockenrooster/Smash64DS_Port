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

### Bootstrap status (2026-08-22)

The first pipeline slice is source-derived inventory rather than another
fighter-specific loader table. `scripts/fighters/generate_fighter_production_manifest.py`
reads BattleShip `ftdata.c`, generated `relocData` source names, the US reloc
symbol table, and O2R headers/extern tables and writes the tracked
`fighter_production_manifest.json`. `make p2-fighter-production-manifest`
rebuilds it, the incremental fighter NitroFS make fragment, and the runtime
symbol-address→O2R catalog; `check-fighter-production-manifest.ps1` is the
standing static gate for all three generated products.

The bootstrap proves its extraction against the already-shipping Mario/Fox
content: the source-derived closure is **321 files, exactly the 321 files in
`NDS_MARIOFOX_FIGHTER_RELOC_FILES`**. Luigi is then derived as a variant rather
than described by hand: its core owns Main/MainMotion/Model/Special1, shares
Mario ShieldPose/Special2/Special3, has 12 Luigi-local animation files, and its
motion table still reaches **19 shared Mario item-related animation files**.
That last point is the P2-5 requirement in this plan becoming a build-visible
fact rather than a future retrofit note.

The audit also records six historical Fox semantic FileID mappings that differ
from BattleShip's generated relocData semantic filenames. They predate this
pipeline and remain untouched while P2-2's Boundary is green; the production
pipeline uses the generated source binding rather than copying those values.
Any behavior correction to an already-shipping fighter must be its own
source-reviewed regression slice, not a side effect of landing Luigi.

The second bootstrap slice proves that inventory can drive the actual DS load
seam without changing P2-2: `NDS_P2_LUIGI=1` stages Luigi's 4 unique core files
and 12 local animation files, admits those generated IDs to the relocation
backend, and uses BattleShip's real nine-entry Luigi status table. The flag is
default-off and the non-Luigi compile path retains the already-qualified
Mario/Fox predicates/status stub verbatim. A Luigi-enabled shipping-shell
configuration builds cleanly with all 16 incremental NitroFS resources staged.
That loader-only statement is retained as the provenance of the bootstrap; the
runtime/UI work it listed is now implemented by the fourth slice below.

The third bootstrap slice makes native-model conversion part of that same
source contract instead of a second manual renderer inventory. The shipping
Mario/Fox native-owner generator keeps its frozen two-owner export and hashes,
but its decoder can now inspect an additional P2 owner independently. For Luigi
it proves the exact `LuigiModel` O2R identity, High/Low JointTrees, setup-parts
mask, hierarchy, display-list state/epoch/run streams, dense DS geometry,
cross-matrix GX slots/restores and light preambles, and writes that compact
inventory into the fighter production manifest. The frozen Mario/Fox generated
include and consumed-fields manifest remain byte-identical. Runtime owner-slot
generalization consumes that source-derived data rather than introducing a
Luigi-only renderer fork.

The fourth slice makes Luigi a real staged production fighter rather than only
an asset inventory. `NDS_P2_PROOF_FIGHTER0=4` selects Luigi through the normal
match descriptor, the native-owner tables admit Luigi as a third content owner,
and the CSS/HUD paths consume the generated Luigi portrait/name/emblem data.
BattleShip's shared Mario callbacks remain authoritative for Fireball, Super
Jump Punch and Cyclone; the focused runtime proof additionally pins Luigi's
source index-1 fireball attributes/launch and the US motion-script 25/18-damage
SJP/Cyclone events. Entry effects were moved at the same checkpoint to a
build-time converted DS-native GX path: Mario's pipe and Fox's Arwing keep the
source DObj animation/visibility timeline while runtime generic-entry fallback
is required to stay zero. The post-change three-arm Boundary rerun on
2026-08-22 is green (`p2_shell_loop`, `p2_battle_realtime`,
`p2_fourcpu_stress`), so P2-2 remains the regression floor while P2-3 advances.

Luigi is therefore the **pipeline prover in qualification**, not the next
loader implementation target. The next structurally new implementation target
is Donkey Kong; do not start roster-wide batching until DK proves the same
manifest/native-owner/status/UI path on a non-Mario archetype.

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
- [ ] All 10 fighters landed per the unit DoD (each unit file's checklist) with the same level of polish as Mario/Fox.
- [ ] Any-4-fighter combination fits the P2-2 budgets (spot-audited: heaviest
      4 by measured cost).
- [ ] Stress config re-argmaxed over the full roster; board updated.


## OWNER PLAYTEST, 2026-09-04 -- the roster as it actually ships

- **Pikachu has no ears.** Owner ranks him the most complete after Samus,
  Donkey Kong, Captain Falcon and Luigi, so the ears are the visible gap.
- **Link** has no 3D preview on the character select and no "Link!" selection
  voice.
- **Yoshi** has no 3D preview; his "Yoshi!" selection voice does play.
- **Luigi** is complete once one sound lands, and it is not a Luigi-specific
  one: the big-hit **scream** is missing from the game entirely. Luigi's
  uppercut should play it on a direct hit and does not, and it is the same cue
  a home-run bat connection uses -- so this is one shared SFX, not a Luigi bug,
  and fixing it closes Luigi.
- Kirby, Ness and Jigglypuff are correctly not selectable and remain in
  progress.

**Completion rule (owner, 2026-09-04):** a fighter counts as added when the
applicable `P2_PLAN.md` laws pass, as Mario and Fox do. A complete fighter must
then be selectable with the dimmed "locked" state and the question-mark plate
removed -- so the CSS overlay is a status the laws drive, not a hand-set flag.
