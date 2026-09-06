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
- **Variant staging is reproducible through admission.** `admit_fighter.py
  --fighter polygons` now stages all 23 Polygon Main/Model files into
  `NDS_1P_RELOC_FILES`, with source/container ID checks and idempotent updates.
  NLuigi reuses NMarioModel. Eight tests / 115 subtests pass. Source asset IDs
  remain defined for flag-off token lookup; handlers retain their feature guards.
  Remaining campaign work: enable the required native Polygon kinds and base
  MainMotion/ShieldPose dependencies for Race/Zako, then verify their natural
  source setup and waves. The stress-only `NDS_P2_KIND_ADMITTED` macro is not
  the campaign's admission authority. Runtime acceptance remains open.

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


### Working note: the missing big-hit sound (2026-09-04)

Chased statically, and every link in the chain is present, which is why this
needs a runtime read rather than more source reading.

- The contract. `dFTMainHitCollisionFGMs` (decomp `ft/ftmain.c:22-32`) is rows
  of hit-sound kind by columns of hit level, `nGMHitSoundBat` is 7 and
  `nGMHitLevelStrong` is 2 (`gm/gmdef.h:203-223`), so a strong bat hit is
  `[7][2]` = `nSYAudioFGMBatHit`.
- The port's own copy of that table is
  `sNdsFighterDashRunHitCollisionFGMs` (`reloc_backend_ftmain_runtime.c:249`)
  and its Bat row reads `{ 38u, 37u, 52u }` -- so 52 is the id to look for.
- `ftMainPlayHitSFX` (`reloc_backend_ftmain_runtime.c:963`) reads that table
  and plays through `func_800269C0_275C0`. Correct.
- The fields it indexes with are assigned: the port includes the whole decomp
  `ft/ftmain.c` at `battleship_ftmain.c:122`, so `attack_coll->fgm_kind` and
  `fgm_level` are written from the MakeAttack5 motion event at `ftmain.c:251`
  and from SetAttackCollSound at `:309`.
- The decomp's own play site, `lbCommonMakePositionFGM(...)` at
  `ftmain.c:2115`, is not a stub either: the port implements it at
  `reloc_backend_compat_shims.c:11604` with real panning through
  `ndsPlayFGMAtPan`.
- The pack is not the problem: Boundary reports FGM coverage of 408 ids with
  zero exclusions.

**Ruled out, and worth recording because it was the first guess:** the item
attack-event decoder in `battleship_item_link_core.c` does not decode fgm
fields, which looked like the cause for the bat. It is not -- `ITAttackEvent`
(`it/ittypes.h:112-118`) has only timer, angle, damage and size, no fgm fields
at all, so items never carried them and the decoder is complete.

**Also worth flagging:** no cue in `gm/gmsound.h` is named anything like
"scream", so the owner's word is a description rather than an id, and the
sound they mean may be a victim damage VOICE rather than the impact FGM. The
measurement has to answer which.

**Next step.** On a booted ROM, land a strong bat hit and a Luigi Super Jump
Punch sweetspot and read the id actually played -- `gNdsSCVSBattleLastFGM` is
already published beside this path. If it reads 52 the cue is issued and the
gap is in mixing or the sample; if it reads a Punch id the gap is in the
attack collision's declared kind; if nothing is issued the gap is the call
itself.

## The character select loads EVERY fighter eagerly, and that is a roster ceiling

Found 2026-09-04 while investigating a rung-8 Boundary failure. This is separate
from the battle-side four-kind problem in `docs/p2/P2-2-pack-estimator.md`, and
it bites sooner.

`ndsMNPlayersVSSetupFighterFiles` (`src/import/battleship_mnplayersvs.c:496-527`)
calls `ftManagerSetupFilesAllKind` unconditionally for Mario and Fox and then
once per admitted flag, so the character select holds **every roster member's
full main closure at once**. Each of those is one
`syTaskmanMalloc(lbRelocGetFileSize(...))` at `ftmanager.c:285`, sized by the
census in `include/nds/generated/nds_fighter_production.generated.h:10-21`.

| Roster | CSS-resident closure bytes |
| --- | ---: |
| P1 pair | 173,088 |
| rung 7, nine fighters | 832,288 |
| **rung 8, ten fighters** | **904,656** |
| all twelve | 1,188,080 |

The arena is measured at 1,318,912-1,319,008 B, and it also has to carry the
menu scene, the UI kit surfaces and the preview build. Adding Jigglypuff moved
CSS residency by his whole 72,368 B closure.

**This grows linearly with the roster and cannot reach twelve.** Ness (79,216)
and Kirby (204,208) together add 283,424 B on top of rung 8. Kirby alone is more
than three Marios.

The fix is already prescribed by the pack review's secondary-lever section
(`docs/reviews/Review_Deriving_Fighter_Live_After_Setup_Set.md` §7.2): split the
character-select lifetime from the battle lifetime. The select screen needs a
portrait, a name and one preview model at a time — not twelve complete fighter
closures. Scene transitions are legal load boundaries, and the character select
changing its highlighted fighter is one; this is not gameplay-time paging.

Note the interaction with the battle side: the CSS heap is rewound on scene
entry (`src/port/reloc_backend_assets.c:9014`, "a bump region that every scene
entry rewinds"), so these bytes are not what starves the *match*. They are their
own ceiling, on their own screen, and either one can block the full roster.

### The fix, specified (2026-09-04)

**The source is eager too.** `decomp/BattleShip-main/decomp/src/mn/mnplayers/mnplayersvs.c:4759-4762`,
inside `mnPlayersVSFuncStart`, is literally
`for (i = nFTKindPlayableStart; i <= nFTKindPlayableEnd; i++) ftManagerSetupFilesAllKind(i);`.
So the port is faithfully reproducing N64 behaviour, and that is exactly the
problem: the N64 could afford all twelve resident and the DS cannot. Going lazy
here is a sanctioned DS adaptation under `PROJECT_GOAL.md` — mechanical
equivalence of the *screen*, not of its loading strategy — and not a divergence
to be justified.

**What the preview actually consumes.** `ftManagerSetupFilesAllKind`
(`ftmanager.c:352-361`) does two halves: `MainKind` allocates the closure
(`:285`), `Kind` only binds status-buffer pointers (`:300-338`, no allocation).
The preview needs Main for attributes and geometry (`ftmanager.c:693-694`, then
`lbCommonSetupFighterPartsDObjs` at `:766-778`) and the Kind bindings for
model/motion/specials.

**The change:**

- Delete the eager ladder at `battleship_mnplayersvs.c:496-527` and its proof
  twin at `:1156-1159`.
- Convert `ndsMNPlayersVSPreviewPrepareResidentKind(s)` (`:131-372`) from
  prewarm-all to an on-demand `ensure(kind)` on selection change, with the
  ensure inserted in `ndsMNPlayersVSPreviewSync` before `:786`.
- **Add an eviction path — none exists today.** `ftManagerSetupFilesAllKind` only
  ever loads (`battleship_ftmanager.c:80-92`). This needs an `ftManagerEvictKind`:
  status-buffer rewind, extern-heap free, `*p_file_main = NULL`, and
  particle/effect release.
- Keep the four `figatree_heap`s (`:529-541`) — sized to max and reused per
  slot, never freed per switch.
- **Residency must be N >= live preview slots, not N = 1.** Two to four previews
  are live at once; a single-kind cache would thrash. LRU-evict only
  non-visible kinds.

**What breaks, and must be handled:**

- Rapid puck sweeping calls `UpdateFighter` on every kind change
  (`mnPlayersVSPuckProcUpdate:3560-3566`), which assumed a zero-I/O rebuild.
  Lazy loading puts a NitroFS read mid-frame — a BGM stall and a visible hitch.
  This is the hard part of the change, not the eviction.
- Hidden-slot logic (`mnPlayersVSUpdateFighter:2342-2354`) assumes `MakeFighter`
  never faults on missing files.
- Costume, shade, particle and deferred-effect paths read Main/Kind data; an
  evicted kind with a stale `fkind` is a bad pointer.
- The residency telemetry masks (`:92-96`), rebuild payload counters
  (`:101-103`) and owner-image prewarm (`:174-323`) all assume entry-time
  residency and become per-switch guards.

### The character select ends 21,772 B higher on its second visit (2026-09-04)

Separate from the eager-load ceiling above, and newer. A Boundary shell lap
recorded `HIGHWATER PlayersVS` at **1,245,628 then 1,267,400** — the scene's
bump pointer at exit (`nds_scene_manager.c:243-250`) ends 21,772 B higher the
second time, and the verifier fails it against an 8,192 B band. Every other
scene is flat: Maps 154,800 twice, Title/ModeSelect/VSMode spread 0, and
VSBattle actually went *down*.

**This is new.** Historical laps were all flat: 844,760 spread 0 on 2026-08-21,
and 727,348 and 608,380 spread 0 on 2026-08-25. The absolute value has climbed
with the roster (608K, 727K, 845K, now 1,245K) but the *spread* only appeared
now.

Ruled out by a read-only pass:

- **Deferred effect descs.** They genuinely do resolve on the second visit once
  a fighter-special file is resident — but `ndsEFManagerRetryDeferredDescs`
  (`battleship_efmanager.c:1274-1303`) only assigns `desc->proc_display` and
  bumps `gNdsEFDescDeferRecoverCount`. No `syTaskmanMalloc`, no reloc, no arena.
  Zero bytes.
- **Particles.** The DS shim (`reloc_backend_compat_shims.c:16329-16346`) returns
  0/1 and never mallocs; the source's `syTaskmanMalloc(script_size, 8)` at
  `efparticle.c:102-103` is behind `NDS_IMPORT_BATTLESHIP_FTMANAGER`.
- **Figatree heaps** are fixed size, **owner images** are 0 for Mario/Fox, and
  the **anim arena** reservation does not move.

**Surviving hypothesis: one extra fighter file loaded on the second visit** — a
single `syTaskmanMalloc(lbRelocGetFileSize(...))` from the ladder
(`battleship_mnplayersvs.c:496-527`). It is the only CSS allocation with
per-file granularity anywhere near 21 KB, and the battle plus Sudden Death plus
the Results podium loader plausibly leave a different LoadOnce state behind.

**Confirm with a counter that already exists**, no new instrumentation:
`gNdsFighterMarioFoxLoadedFileCount` (reset `taskman_seam_core.c:474`,
incremented `reloc_backend_fighter_model.c:1589`). Read it at both CSS exits; a
delta of one, times that asset's `lbRelocGetFileSize`, should equal 21,772.
Cross-check that `gNdsFTManagerFigatreeHeapMeasured`,
`gNdsNativeOwnerImageLoadCount` and `gNdsEFDescDeferRecoverCount` are unchanged.
