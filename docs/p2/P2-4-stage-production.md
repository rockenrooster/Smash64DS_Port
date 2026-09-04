# P2-4 — Stage Production (pipeline + the remaining 8 VS stages)

Same industrialization as P2-3, for stages. Dream Land (P1) is the exemplar;
every stage must meet the Stage Completeness Standard in `PROJECT_GOAL.md`.
Highly stage-specific renderers are explicitly fine.

## Pipeline generalization (first slice)

1. **Collision import**: ground/wall/ceiling/platform geometry, ledge grab
   points, pass-through flags, blast zones, spawn/respawn points from
   BattleShip map data (`gr/`, `mp/` and per-stage data via `DECOMP_MAP.md`).
2. **Geometry/visual build**: DS-budget stage mesh, baked lighting/vertex
   colors, background treatment (3D, 2D BG layers, or hybrid per stage —
   cheapest recognizable wins), within the P2-2 per-stage budget.
3. **Hazard seam**: stage hazards are stage-owned update hooks with
   mechanically equivalent behavior from BattleShip (`gr/grcommon/` + per-stage
   logic) — no generic hazard interpreter.
4. **Camera/bounds + music** per stage; SSS art (map icon, name).
5. **Acceptance harness**: collision parity sweep (probe walk of surfaces,
   ledges, blast lines vs imported data), hazard behavior checks, 4-CPU
   stress measurement on the stage.

## Stage order (owner-ratified, hazard complexity ascending)

| # | Stage | File | Hazards / notable |
|---|---|---|---|
| 1 | Yoshi's Island | `stages/yoshis-island.md` | No hazards; cloud platforms — cheapest full pipeline pass |
| 2 | Peach's Castle | `stages/peachs-castle.md` | Bumper, sliding platform |
| 3 | Congo Jungle | `stages/congo-jungle.md` | Barrel cannon underneath |
| 4 | Hyrule Castle | `stages/hyrule-castle.md` | Tornado; large multi-terrain layout |
| 5 | Planet Zebes | `stages/planet-zebes.md` | Rising/falling acid |
| 6 | Sector Z | `stages/sector-z.md` | Arwing strafing runs; biggest stage — perf risk |
| 7 | Saffron City | `stages/saffron-city.md` | Pokémon door spawns; rooftop gaps |
| 8 | Mushroom Kingdom | `stages/mushroom-kingdom.md` | Warp pipes, POW, Piranhas, walk-off — most bespoke systems |

1P-only venues (`final-destination`, `meta-crystal`, `duel-zone`,
`race-to-the-finish`) and the 24 bonus boards (`stages/bonus-stages.md`) reuse
this pipeline but are P2-6 scope.

## Standing rules for every stage row

- Inspect the stage's BattleShip data/logic before building; hazards are
  gameplay (mechanical equivalence), backgrounds are presentation (visual
  doctrine applies — timebox exactness, record deltas with screenshots).
- Whispy precedent: P1's Dream Land wind implementation is the reference for
  how a hazard integrates with fighter physics.
- Each stage lands fully (collision, hazards, music, SSS entry) before the
  next starts; each closes with a 4-CPU stress measurement on it, and the
  hardest-stage argmax updates the standing stress config.
- Music per stage through the existing streaming path; sound-RAM budget per
  P2-2.

## Exit criteria

- [ ] Pipeline reproducible from BattleShip data by `make`.
- [ ] All 8 VS stages landed per unit DoD; SSS fully populated (Mushroom
      Kingdom unlock-gated in P2-7, selectable in dev builds).
- [ ] Collision parity sweeps green on every stage.
- [ ] Stress config re-argmaxed including stages; board updated.

## Measured bespoke-code ranking (2026-09-03)

The order above was set by expected hazard complexity. The stage logic
translation units have since been measured — `wc -l` over
`decomp/BattleShip-main/decomp/src/gr/grcommon/` — and the ranking is not the
same:

| Lines | TU | Stage | Plan position |
|---:|---|---|---|
| 66 | `grcastle.c` | Peach's Castle | 2 |
| 202 | `grjungle.c` | Congo Jungle | 3 |
| 250 | `grzebes.c` | Planet Zebes | 5 |
| 268 | `gryoster.c` | Yoshi's Island | 1 (in flight) |
| 298 | `gryamabuki.c` | Saffron City | 7 |
| 477 | `grhyrule.c` | Hyrule Castle | 4 |
| 588 | `grinishie.c` | Mushroom Kingdom | 8 |
| 701 | `grpupupu.c` | Dream Land | done (P1) |
| 1131 | `grsector.c` | Sector Z | 6 |

Two disagreements worth acting on, both cheap to honour because nothing has
started on either stage: **Planet Zebes is cheaper than Hyrule Castle**
(250 against 477), and **Sector Z is the most expensive stage in the game**
at 1,131 lines — 1.9× Mushroom Kingdom, which the plan called the most
bespoke. The plan's own note already flagged Sector Z as the perf risk; the
line count says it is the code risk too. Proposed order, measured:
Yoshi's Island, Peach's Castle, Congo Jungle, Planet Zebes, Hyrule Castle,
Saffron City, Mushroom Kingdom, Sector Z. The owner ratified the original
order, so this is a proposal on the record, not a change already made.

Peach's Castle being the cheapest is real and worth saying plainly: at 66
lines `grcastle.c` is the smallest stage TU in the game — three functions,
one of which is a two-line bumper follower.

## Stages that need the item subsystem

Three stages spawn their hazards **as items**, so they cannot close before
P2-5's manager can make a stage-kind item. Measured by grepping
`itManagerMake` across `gr/grcommon/`:

- `grcastle.c:57` — the bumper is `nITKindGBumper`.
- `grinishie.c:427,465` — the Piranhas are `nITKindPakkun`, the POW block is
  `nITKindPowerBlock`.
- `gryamabuki.c:101` — Saffron's Pokémon are
  `item_id + nITKindGroundMonsterStart`.

`itManagerMakeItemSetupCommon` with `ITEM_FLAG_PARENT_GROUND` is the shared
call in all three. The port's maker currently refuses every kind but
`nITKindLinkBomb` (`src/import/battleship_item_link_core.c:532-537`), so
**P2-5 slice 1 is a hard prerequisite for Peach's Castle**, not merely a
later phase. The other five stages have no item dependency.

## Peach's Castle source pins (verified 2026-09-03)

Internal name `Castle`; kind `nGRKindCastle` (`gr/grdef.h:11`). Paths below
are relative to `decomp/BattleShip-main/decomp/src/`.

- Map: `relocData/259_GRCastleMap.c`, header `dGRCastleMap_header:27`,
  four-row display-layer table `:29-35`, node root `:59`.
- Collision: `MPGeometryData dStageCastleFile2_MPGeometryData_0x2D58`
  (`relocData/106_StageCastleFile2.c:636`), referenced `259_GRCastleMap.c:36`.
- Bounds, in the map header not the logic TU (`259_GRCastleMap.c:50-69`):
  camera `4800 / -1300 / 4000 / -4000`, blast `9500 / -4000 / 9000 / -9000`,
  altitude warning `-1900`, with separate team-mode camera and blast rows.
  Dream Land's control values are `255_GRPupupuMap.c:48-59`.
- Logic: `gr/grcommon/grcastle.c` — `grCastleBumperProcUpdate:12` (follows
  the moving ground in x only), `grCastleInitAll:25`, `grCastleMakeGround:61`.
- Sliding platform: it is **not** code. `grCastleInitAll:45` calls
  `gcAddAnimJointAll(ground_gobj, gMPCollisionGroundData->map_nodes, 0.0F)`
  and the sweep is animation data —
  `relocData/156_StageCastleFile3.c:28-40`, a looping TraX `SetValBlock`
  of `0` → `-1050` over 599 → `1050` over 1200 → `0` over 600, then 2400
  frames of hold. So the platform is the joint animation seam, not a hazard
  update hook, and it needs no bespoke update function.
- Hazard seams: neither of Castle's two differs from an existing one, and
  neither is Whispy's. Whispy pushes a fighter's velocity directly
  (`grpupupu.c:165` → `ftParamSetVelPush`, `:197`); the bumper is an item
  actor and the platform is animation. Nothing new to build at the fighter
  seam.
- Music: `nSYAudioBGMCastle = 6`, counted in `gm/gmsound.h:31-37` from
  `nSYAudioBGMPupupu = 0` (Pupupu, Zebes, Inishie, InishieHurry, Sector,
  Jungle, Castle). No `REGION_US` arm falls inside the BGM range.
  (`nSYAudioBGMYoster = 8` by the same count.)
- Stage-select art: icon `&llMNMapsPeachsCastleSprite` (`mn/mnmaps.c:515`),
  name plate `&llMNMapsPeachsCastleTextSprite` (`:583`), file info row
  `{&llGRCastleMapFileID, &llGRCastleMapMapHeader}` (`:31`).

Port-side registration points, all already shaped by the Yoshi's Island arm
next to them: the `grCommonSetupInitAll` arm
(`src/import/battleship_grpupupu_ground.c:486-491` Dream Land, `:530-538`
Yoshi's Island, and Castle's stub at `:549`), the ground-data load
(`src/port/reloc_backend_compat_shims.c:16426-16431` Dream Land,
`:16518-16523` Yoshi's Island), the stage-select slot tables
(`src/nds/nds_menu_shell_sss.c:123-128`, `:134-138`, `:175-188`, `:219-230`,
`:241-252`), the reloc asset rows (`src/nds/nds_reloc_assets.c:101`, `:104`)
and the token rows (`src/port/reloc_backend_assets.c:2543-2550`) — where a
row of the **address** shape is needed alongside the numeric id, because
`ndsRelocFileID` does not dereference (`:1373-1376`); the two-shape pattern
is at `:2613-2617`.

## Standing rule: a map-object miscount is a silent boot hang

Two stages answer a bad map-object import with an infinite loop, verbatim in
the source and therefore verbatim in any faithful port:

- `gr/grcommon/grhyrule.c:394-401` -- `grHyruleTwisterInitVars` spins in
  `while (TRUE) { syDebugPrintf("Twister positions are error!"); ... }` when
  `mpCollisionGetMapObjCountKind(nMPMapObjKindTwister)` returns 0 or more
  than 10.
- `gr/grcommon/grinishie.c:515-522` -- `grInishieMakePowerBlock` does the same
  for `nMPMapObjKindPowerBlock`.

On DS there is no console to read, so this presents as a stage that boots to
black and never returns, with no exception and nothing in a log -- the exact
failure shape that has cost this project multi-hour hunts before. Before
running either stage, assert the map-object count for its hazard kind at
import time, in the tooling, where the number is checkable. Do not "fix" the
loop: it is source behaviour, and a port that quietly continues past a bad
count builds a stage whose hazards are in the wrong places.

## Independent confirmation of the ranking

The line-count ranking above was re-derived a second way, by counting hazard
update functions per stage from the source: Congo Jungle 3, Saffron City 4,
Planet Zebes 6, Hyrule Castle 8, Mushroom Kingdom 9 across two systems plus
the Piranha spawner, Sector Z roughly 15 across two weapon pipelines. Both
methods put Planet Zebes below Hyrule Castle and Sector Z last, which is the
same pair of swaps. Saffron City ranks cheaper by function count than by line
count because its cost is item coupling rather than logic.

Per-stage source pins now live in each stage's own file under
`docs/p2/stages/`.

## How a stage gets accepted (verified 2026-09-03)

There is **no collision parity sweep in the tree**, and no verifier profile can
run a stage that lives behind a default-off flag. `scripts/verify-all.ps1:6-7`
offers exactly two profiles, and their plans
(`scripts/lib/harness-registry.ps1:89-92`) name only `runtime`,
`p2_shell_loop`, `p2_battle_realtime` and `p2_fourcpu_stress` — of which only
`p2_battle_realtime` is documented against a stage at all, and that stage is
Dream Land (`harness-registry.ps1:32-35`).

So a flagged stage is accepted by hand, and this is the recipe:

1. Build a lab ROM: `TARGET=<lab> BUILD=build-<lab> NDS_P2_STAGE_CASTLE=1`.
   Lab output stays under `builds/<BUILD>` — only `smash64ds` and
   `smash64ds-battle-playable-hwtri` publish to the repo root
   (`Makefile:67`, `scripts/lib/build-output.ps1:25-31`).
2. Launch through a harness that takes `-NoBuild` with explicit `-Rom` and
   `-Elf`, the shape `scripts/verify-battle-playable-platform-semantics.ps1:30-39`
   uses. Copy the ROM and ELF into the lab directory under the target's own
   name first: the shared melonDS DLDI image is an append-only lab cache
   (`scripts/lib/melonds.ps1:21-28`), and a ROM launched under a name that
   looks like that image opens zero NitroFS files.
3. Read the proof. Nothing in `scripts/` prints these, so it is a manual GDB
   read of globals defined at `src/port/diagnostics_renderer_census.c:3591-3599`:

   - `gNdsSCVSBattleStageGKind` — the discriminator. The kinds are ordered from
     `nGRKindCastle` (`gr/grdef.h:11-17`), so **Castle is 0**, Sector Z 1,
     Congo Jungle 2, Zebes 3, Hyrule 4, **Yoshi's Island 5**, **Dream Land 6**.
   - `gNdsSCVSBattleStageMask` — `0xFF` when all eight load steps passed. Each
     stage's arm sets bits `1<<0` through `1<<7`
     (`reloc_backend_compat_shims.c:16474-16501` Dream Land,
     `:16539-16566` Yoster, `:16606-16633` Castle).
   - `gNdsStageOptInAssetMask` and `gNdsStageOptInExternalFixupFailCount`
     (`src/port/reloc_backend_assets.c`) — which of the opt-in stages' files
     were actually fixed up, and whether any fixup failed.

   The single most discriminating check is the bounds, because they come from
   the source `MPGroundData` and differ per stage: Castle is camera
   `4800/-1300/4000/-4000`, blast `9500/-4000/9000/-9000`, `alt_warning -1900`;
   Yoshi's Island is `4300/-2000/7000/-4300`, `8200/-4000/10500/-7800`, `-2500`.
   Reading either back proves the stage loaded its own ground data rather than
   Dream Land's.

**The parity sweep the exit criteria ask for does not have to be written from
scratch.** Its two halves already exist: the live side is the line-geometry
reader in `verify-battle-playable-platform-semantics.ps1:105-126,294-298`,
which walks `gMPCollisionGeometry->vertex_links/vertex_id/vertex_data` over
GDB and is stage-general even though it is invoked Dream Land-pinned; the
imported side is the source-layer resolver in
`check-stage-aot-falsifier.ps1:155-264`, which today asserts Dream Land's own
57 DObjs / 42 display-list refs / 4 MObjs. Blast lines have an oracle too —
`probe-ko-vfx.ps1:672-674` already reads `camera_bound_top` and `map_bound_*`
to place KO probes.

## The native stage packet is a pipeline job, not a per-stage job (2026-09-03)

Law 8 forbids a completed unit from drawing through the generic renderer, so
every stage needs a native packet. Reading the generator, the runtime and the
checker end to end says that is one large generalization rather than eight
small transcriptions, and the shape of P2-4 should reflect that.

`scripts/stages/generate_nds_native_stage.py` is Dream Land throughout, not
Dream Land-first:

- `EXPECTED_*` at `:61-84` are twenty-odd frozen oracles — 8 callbacks, 57
  DObjs, 42 bindings, 886 commands, 302 source vertices, 202 triangles, 54
  runs, 49 epochs, 4 material events, and submit/cross/state tuples — every one
  a Dream Land count.
- `OWNER_SPECS` `:1437-1477` hardcodes eight owners with their DObj offsets,
  descriptor counts, display links and callbacks; `MATERIAL_SOURCES`
  `:1776-1781` hardcodes four `(file, mobj, dobj)` triples.
- `generate()` `:2565-2970`, `validate_packet()` `:2973-3222` and
  `build_generated_segment0_program()` `:3234-3499` all read those globals
  directly, and the segment-0 program requires the literal tuple
  `(OWNER_LAYER0, 4, 0, 20, 0, 26)`.
- `O2R_INPUTS` `:267-300` and `TEXT_INPUTS` `:302-341` pin a SHA-256 for every
  input file, plus an `EXPECTED_INCLUDE_SHA256` for the emitted include.

The runtime is pinned the same way. `src/port/renderer_adapter_matrix.c:474-478`
fixes `STAGE_SEGMENT_COUNT 8`, `DOBJ_COUNT 57`, `BINDING_COUNT 42`,
`ASSET_COUNT 4`; `renderer_adapter_stage.c:2953-2958` hardcodes the four Dream
Land asset ids and sizes; and
`ndsRendererAdapterBuildNativeStageTopologyStamp` `:2297-2311` rejects anything
whose DObj count is not 57 and binding count not 42.

And the checker is the biggest of the three: `check_nds_native_stage.py` runs
per-binding oracles, a depth-trace hash, a command replay of all 886 commands,
a twelve-perturbation fail-closed suite and a double-generate byte-equality
check, all against pinned Dream Land constants.

So the work is: thread a stage descriptor through the generator, the checker
and the three runtime files so counts, assets, oracles and checksums are
per-stage rather than global; keep Dream Land's oracles frozen as the
regression control; and only then bake a second stage. Doing it per stage
means eight copies of a subsystem that already resists copying — and the
attempt to shortcut it for Yoshi's Island produced an owner with invented link
and callback constants that was reverted the same day.

**Consequence for the phase order.** A stage can land its gameplay half,
collision, hazards, bounds, music and stage-select entry without this. It
cannot be *complete* without it. Landing stages 3 through 8 first and baking
packets afterwards is therefore the cheaper order only if the packet work is
genuinely shared — which the reading above says it is.

## P2-4n1: the native-packet parameterisation plan (delegated probe, 2026-09-03)

A read-only sweep of the generator, the checker and the three runtime files,
reported at high confidence with a citation on every line. It has not been
re-verified line by line here, so check anything load-bearing against the
cited file before building on it. The ordering argument at the end is the
part to act on: the generator goes first because the checker and the runtime
both consume packet bytes and hashes it produces, and Dream Land's frozen
include hash is the control that makes the rest safe.

```
Pipeline job summary read: `docs/p2/P2-4-stage-production.md:247-295`
Law 8: completed unit no generic renderer: `docs/P2_PLAN.md:86-91`, `docs/p2/P2-4-stage-production.md:248-249`
Seven gameplay arms behind flags, still generic packet path: `src/port/reloc_backend_assets.c:2640`, `:2654`, `:2677`, `:2690`, `:2700`, `:2711`, `:2716`, `src/nds/nds_menu_shell_sss.c:137-172`, `src/import/battleship_grhyrule_ground.c:28`
Generator Dream Land-only root cause: `scripts/stages/generate_nds_native_stage.py:291-298` stage_map GRPupupuMap, `:314-317` grpupupu.c, `:1459-1476` OWNER_SPECS, `:1486-1491` MATERIAL_SOURCES

Globals -> per-stage fields:
Generator `scripts/stages/generate_nds_native_stage.py`:
- counts `:60-83`: EXPECTED_CALLBACKS `:60`, DOBJS `:61`, BINDINGS `:62`, COMMANDS `:63`, VERTEX_COMMANDS `:64`, SOURCE_VERTICES `:65`, MODIFYVTX `:66`, DENSE `:70`, TRI_CMDS `:71`, TRIANGLES `:72`, RUNS `:73`, EPOCHS `:74`, MATERIAL_EVENTS `:75`, SUBMIT_CLASSES `:76`, CROSS_RUNS `:77`, CROSS_TRIS `:78`, CROSS_CORNERS `:79`, STATE_EVENTS `:80`, STATE_DELTAS `:81`, SYNC `:82`, SPANS `:83`
- pins `:99` EXPECTED_INCLUDE_SHA256, `:266-299` O2R_INPUTS, `:301-340` TEXT_INPUTS, `:1437-1443` OWNER_* ids, `:1459-1476` OWNER_SPECS, `:1486-1491` MATERIAL_SOURCES, `:88` GENERATED_SEGMENT_INDEX, `:187-198` SEGMENT0_EFFECT_MACROS
- literals inside logic: `:1799` `(3,3,10,10)`, `:2727-2736` expected_segments, `:2750-2755` material partition, `:3016-3020` `(OWNER_LAYER0,4,0,20,0,26)`, `:3037` `range(123)`, `:3039` `(1,)*22`, `:3043` `54`, `:3045` `108/27`, `:3056` `(78,30)`
Checker `scripts/stages/check_nds_native_stage.py`:
- `:38-45` EXPECTED_ROOTS, `:47-51` COMMANDS, `:53-57` VERTEX_COMMANDS, `:59-63` SOURCE_VERTICES, `:65-69` TRIANGLE_COMMANDS, `:71-75` TRIANGLES, `:77-81` RUNS, `:83-87` EPOCHS, `:89-99` EXPECTED_SEGMENTS, `:101-105` RUN_TRIANGLES, `:107-116` RUN_CLASSES, `:118-123` DENSE_FIRST_VISIT_OFFSETS, `:125` DEPTH_TRACE_HASH, `:127-130` SEGMENT0_BINDING_COMPOSED, `:131-134` SEGMENT0 checksums, `:135-146` EFFECT_MACROS, `:147-158` EFFECT_COUNTS, `:160-162` CROSS_RUNS/TRIS/CORNERS, `:165-176` CACHE_CLONES, `:178-184` REPLAY_CLASSES
Runtime emitted counts `src/nds/nds_native_stage_owner.generated.inc:4-26`: ASSET `:4`, SEGMENT `:5`, DOBJ `:6`, BINDING `:7`, SOURCE_CMD `:8`, VERTEX `:9`, SOURCE_VERT `:10`, CLONE `:11`, DENSE `:12`, TRI_CMD `:13`, TRI `:14`, CORNER `:15`, CROSS `:16-18`, RUN `:19`, EPOCH `:20`, MATERIAL `:21`, POLICY `:22`, DELTA `:23`, SEQ `:24`, SPAN `:25`, SLAB `:26`, plus `:27-46` SEGMENT0 program/checksums/masks
Adapter fixed maxima `src/port/renderer_adapter_matrix.c:473-478`: SEGMENT 8 `:474`, DOBJ 57 `:475`, BINDING 42 `:476`, ASSET 4 `:477`, MATERIAL 4 `:478`; sizes workspace `:480-495`
Stage hardcoded Dream Land `src/port/renderer_adapter_stage.c:2953-2958`: ids `0x67,0x68,0x98,0xff` `:2953-2955`, sizes `0x2fc0,0x43f0,0x3700,0x00c0` `:2956-2958`; reject topology `:2297-2311`, specifically `:2306-2308` dobj!=57 / binding!=42
Native validator `src/nds/nds_renderer_native_owners.c`: tables `sNdsNativeStageDObjs` `:482`, Assets `:498`, Segments `:505`, Bindings `:537`, Runs `:563`, Epochs `:573`, MaterialEvents `:590`, Spans `:606`, Corners `:669`, Vertices `:677`; tallies `:750-759` raw 66 / no-z 126 / range 10 / cross; cert `:317-353` source/table/hot/dense checksums + triangle 54 `:337` + epochs 22 `:338`

Functions reading globals directly:
Generator: `generate` `:2275` reads O2R_INPUTS `:2280`, OWNER_SPECS `:2304,2324,2356`, EXPECTED_BINDINGS `:2309`, EXPECTED_DOBJS `:2334`; `build_material_events` `:1762` reads MATERIAL_SOURCES `:1769`; `validate_packet` `:2683` reads all EXPECTED_* `:2697-2709,2712,2723,2763-2768,2849-2854`; `build_generated_segment0_program` `:2944` reads GENERATED_SEGMENT_INDEX `:2945`, OWNER_SPECS `:3106`, literal `:3016-3020`; `render_include` `:3278` calls build `:3279`, emits defines `:3286-3313` from packet + EXPECTED_CROSS_* `:3305-3310`
Checker: `verify_packet` `:403` reads ROOTS/COMMANDS/.../SEGMENTS/material tuple; `verify_generated_segment0_program` `:750` reads cert `:757-792`, composed `:797-800`, dense `:839-844`, instr `:869-922`; `verify_command_replay` `:1072` reads OWNER_SPECS `:1079,1099`, 886 `:1200-1204`; `verify_fail_closed` `:1236` 12 mutations `:1241-1322`, commit 8 `:1336-1342`; `verify_consumed_fields_manifest` `:217` reads live-operand order `:234-249`, census 8/4 `:260-277`, segment0 21/20/26 `:280-318`; `verify_task26_execution_shape` `:379`; `main` `:1346` double-generate `:1350-1352`, include SHA `:1366-1368`, stale `:1370-1371`
Runtime: `ndsRendererAdapterBuildNativeStageTopologyStamp` `:2297` reads DOBJ/BINDING/ASSET/SEGMENT counts `:2306-2333`; `ndsRendererAdapterPrepareNativeStageOwner` `:2951` reads asset_ids/sizes `:2953-2958`; loops/bounds reading same macros `:2247`, `:2265`, `:2363`, `:2418`, `:2506-2508`, `:2577`, `:2698`, `:2724`, `:2749`, `:2828`; `ndsRendererNativeStageValidateGeneratedSegment0` `:303` reads cert + HotRuns `:362-382`; `ndsRendererNativeStageValidateTopologyFull` `:463` reads all sNdsNativeStage* `:480-747` + summary `:750-763`; `ndsRendererNativeStagePrepareRun` `:962` reads Runs `:969`, Epochs `:1020`, validation cache `:1102-1136`; `ndsRendererNativeStagePrepareGeneratedSegment0` `:1263` reads cold cert `:1269`, macro `:1367`

Second stage descriptor shape:
- dataclass `StageDescriptor`: name, grKind, o2r_inputs:dict, text_inputs:dict, owner_specs:tuple, material_sources:tuple, expected_counts:dict, segment0_golden:dict, include_sha:str. Modeled on existing `InputSpec` `:256-263`, `OwnerSpec` `:1446-1454`, `MaterialSource` `:1479-1483`, `Packet` `:1627-1647`
- lives: `scripts/stages/native_stage_descriptors/dreamland.py` frozen + `yoster.py` next, registry `scripts/stages/native_stage_descriptors/__init__.py`; generator takes `--stage dreamland` default, no flag change = byte-identical output
- runtime lives: keep `src/nds/nds_native_stage_owner.generated.inc:1-2` Dream Land frozen; emit `src/nds/nds_native_stage_<kind>.generated.inc` namespaced `sNdsNativeStageYoster*` + `NDS_NATIVE_STAGE_YOSTER_*`; selector struct `{counts, ptrs}` indexed by `gNdsSCVSBattleStageGKind` pattern `docs/p2/P2-4-stage-production.md:218-224`; workspace maxima = max over stages, active = descriptor current
Dream Land regression freeze:
- keep EXPECTED_* `:60-83`, EXPECTED_INCLUDE_SHA256 `:99`, checker oracles `:38-184`, slab `12663` `check_nds_native_stage.py:746`, `generate_nds_native_stage.py:2773`, stale check `:1370-1371`
- new stage gets own `EXPECTED_YOSTER_*` + own sha, never edits Dream Land literals; CI runs Dream Land checker unchanged, plus per-stage checker entry

Order keeping tree building:
1. generator descriptor threading with Dream Land default, output byte-identical, checker still green. Reason: unblocks rest without breaking stale check `:1370-1371`
2. runtime maxima + indirection, default bound to Dream Land tables, no behavior change. Reason: compiles before new tables exist
3. checker parameterization per descriptor, Dream Land path asserts old constants. Reason: validates step 1-2
4. emit second stage packet, add validator + asset rows behind existing `NDS_P2_STAGE_*` flag shape `src/port/reloc_backend_assets.c:2640-2716`. Reason: flagged, off-tree invisible
5. native owner selection by stage kind, then gameplay hookup

Estimates (each estimate):
- generator: medium, threading + CLI + Dream Land alias proving byte-equality (estimate)
- checker: largest, per-binding oracles + replay 886 `:1200-1204` + 12 perturbations `:1236-1343` + depth hash `:629-631` duplicated per stage (estimate)
- runtime: medium-large, workspace maxima + stamp `:2297-2311` + asset tables `:2953-2958` + validator `:463-779` to descriptor pointers (estimate)
First must be generator (estimate): checker/runtime consume packet bytes/hashes; no truth without it; Dream Land frozen hash is control enabling rest
```


## OWNER PLAYTEST, 2026-09-04 -- the eight stages as they actually ship

First time the owner could reach these in `smash64ds.nds`. Verbatim
observations, grouped; the shared defect is listed first because it is one
cause, not eight.

**EVERY stage is missing its background.** Peach's Castle, Yoshi's Island,
Congo Jungle, Hyrule Castle, Planet Zebes, Saffron City and Mushroom Kingdom
were all reported missing BG, and Sector Z is "missing everything, all I see is
a blue background and collisions of the map". Dream Land has its background, so
this is the opt-in stage path not doing what the Dream Land path does. Treat it
as one investigation, not eight.

Per stage, beyond the background:

- **Sector Z** -- worst of the set. No map, no background, no music; only the
  collision is there.
- **Congo Jungle** -- no music. The moving platforms do not move, and the
  barrel's movement is wrong.
- **Mushroom Kingdom** -- music "doesn't sound right". The Piranha Plants do
  not look right. Some map geometry missing.
- **Peach's Castle** -- missing geometry, specifically the cloud hazards.
- **Yoshi's Island** -- missing geometry, the same cloud hazards.
- **Hyrule Castle** -- missing geometry.
- **Planet Zebes** -- missing map geometry; collision is fine and the hazard
  works, but the acid visual needs work.
- **Saffron City** -- missing some map geometry.

Two more from the stage-select screen itself:

- Peach's Castle's preview background is **Hyrule Castle's**.
- No stage has its render preview at all -- the screen shows only the preview
  background.

**Completion rule (owner, 2026-09-04):** a stage or fighter counts as added
when the applicable `P2_PLAN.md` laws pass, the way Mario and Fox already do.
Anything complete must then be selectable with the dimmed "locked" state and
the question-mark plate removed.

## Wiring a second native stage — the Yoster worked example (2026-09-04)

Only Dream Land has native stage geometry. `renderer_adapter_matrix.c:514-525`
binds all eight gkind arms to `&sNdsRendererAdapterNativeStageDreamLand`, and its
own comment says so: *"Every slot binds frozen Dream Land descriptor until step 4
adds second stage row."* Every other stage therefore mismatches its asset ids and
draws zero native triangles — one cause behind the owner's missing-geometry
reports on all eight.

**CORRECTION, 2026-09-04: the three-step list below is WRONG and will not
compile.** It was written from a probe and verified afterwards; the
verification found two blockers it missed. Keep reading for the corrected
list. The three steps are still necessary — they are just nowhere near
sufficient.

**BLOCKER A — the generated include has no per-stage symbol namespace, and
the generator cannot produce one.** `generate_nds_native_stage.py:4413`
namespaces the *path*, not the symbols. The emitted file hardcodes 40+
`NDS_NATIVE_STAGE_*` macros, 12 `NDSNativeStage*` typedefs and 16
`sNdsNativeStage*` arrays. Both includes land in one translation unit
(`src/nds/nds_renderer_assets.c:219`), so a second one is struct
redefinition, macro redefinition and duplicate `static const` definitions.
Namespacing is new emitter work — a rename pass plus emit-once guards for
the shared typedefs — not a promotion.

**BLOCKER A: CLEARED 2026-09-04 (P2-4n1 step 5).**
`namespace_include_lines()` in the generator rewrites a descriptor with a
non-empty `symbol_prefix`/`macro_prefix` into its own namespace and drops the
shared typedef block; Dream Land keeps both empty, so the pass cannot touch
it and `--stage dreamland --check` still renders
`eda2dbd6ee323c3eb33a323be46b61676d2f63057e315e6f288537f76555942c`.
`NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE` is deliberately left shared (the
build sets it with `-D`). Two Dream Land literals were leaking into the
per-stage emitter and are fixed with it: the corner array was declared
`[606]` for every stage, and the segment-program assert compared every
stage's run count against `26u` — the latter made the emitted Yoster include
**uncompilable**, invisible only because it linked nowhere.

**BLOCKER B — the renderer selects no tables per stage.**
`src/nds/nds_renderer_native_owners.c` makes **260** direct references to
the fixed `sNdsNativeStage*` arrays and **107** to the fixed
`NDS_NATIVE_STAGE_*` counts. `:480-493` walks `i < NDS_NATIVE_STAGE_DOBJ_COUNT`
(57, Dream Land's) against `sNdsNativeStageDObjs[i]` while the adapter fills
only Yoster's 28 — an out-of-bounds read past the fill plus a guaranteed
`FALSE`. **As listed, this change buys zero Yoster triangles.** The
`{counts, ptrs}` selector this needs does not exist.

*Correction to the 260 (measured 2026-09-04):* 260 is every `sNdsNativeStage*`
token in the file, and 178 of them are runtime state, not generated tables —
`sNdsNativeStageOwnerExecution` 140, `sNdsNativeStageValidationCache` 26,
`sNdsNativeStagePreparedDense` 10, `sNdsNativeStageTopologyFaultInjected` 2.
The generated tables are referenced **82** times there (plus 4 in
`nds_renderer_textures_effects.c` and 1 in `nds_renderer_assets.c`). The 107
macro count is exact.

**BLOCKER B: SUBSTRATE LANDED 2026-09-04 (P2-4n1 step 5).**
`src/nds/nds_native_stage_select.inc` holds the `{counts, ptrs}` packet, one
`static const` instance per linked stage, an 8-entry gkind registry and the
active pointer, resolved once per `ndsRendererPrepareNativeStageOwner`. The
82+107 references are routed through it by redefining the generated names
after the includes, so the consumer text is unchanged; the ten declarations
that need a compile-time length take `NDS_NATIVE_STAGE_MAX_*` instead, which
are **enum constants, not macros** — a macro maximum expands at use, i.e.
after the redirect, and produces seven "variably modified … at file scope"
errors. Under `NDS_NATIVE_STAGE_MULTI == 0` nothing is redirected, so a
Dream-Land-only ROM is unchanged. Three more Dream Land literals had to move
into the packet: `ValidateTopologyFull`'s submit census `66u/126u/10u`
(Yoster's is 35/87/42), which would have failed every Yoster frame, and the
`segment_index == 0` specialisation, now gated on the packet's
`has_generated_segment0` so Yoster's own certificate is never checked against
Dream Land's program.

Also missing from the list: `override NDS_P2_STAGE_YOSTER := 1` belongs in
the `smash64ds-battle-playable-hwtri` block (`Makefile:2260-3282`) — the
three existing overrides are all in the scene-walk target; `build.ps1` needs
a second `--stage yoster` invocation near `:556-559` and a second
`$generatedOutputs` entry at `:582`; `yoster.py:135`'s `include_sha` must be
re-minted after namespacing; and `.gitignore:41`, the publish manifest and
`scripts/publish/audit_minimal.ps1:33` all enumerate only the Dream Land
`.inc`.

What the original list got right: the descriptor's shape and values, the
maxima headroom, index 5 being Yoster, no Makefile rule, and no checker
change for the table retarget itself.

**Yoshi's Island is still the cheapest way in, because its descriptor already exists**
(`scripts/stages/native_stage_descriptors/yoster.py:133`, registered at
`__init__.py:70`). Three changes reach the runtime:

1. **Add a second adapter descriptor** beside
   `sNdsRendererAdapterNativeStageDreamLand` (`renderer_adapter_matrix.c:498-507`).
   The values are already generated at `yoster.py:315-321`:
   `adapter_segment_count=4`, `adapter_dobj_count=28`, `adapter_binding_count=19`,
   `adapter_asset_count=4`, `adapter_material_count=2`,
   `adapter_asset_ids=(0x6E, 0x6F, 0x9A, 0x107)`,
   `adapter_asset_sizes=(0x5230, 0xB930, 0x6B0, 0x00C0)`.
2. **Retarget table index 5** (Yoster's gkind, per `include/sc/scene.h:284-291`
   and `decomp/.../gr/grdef.h:11-18`) from the Dream Land alias to it.
3. **Promote the generated include.** The generator writes non-Dream-Land stages
   to `builds/native-stage-<name>/` and only Dream Land to `src/`
   (`generate_nds_native_stage.py:4412-4422`, `yoster.py:105-110`). Yoster's
   `.inc` has to land in `src/nds/` under an
   `NDS_NATIVE_STAGE_YOSTER_*` / `sNdsNativeStageYoster*` namespace.

**No maxima change is needed for Yoster** — the adapter caps
segments/DObjs/bindings/assets/materials at 8/57/42/4/4 and Yoster's 4/28/19/4/2
fits inside. Packet checks alone did not cover the C integration; the shared
source audit below now runs in both stage checker arms.

### Capture/draw source corrections — 2026-09-04

`nds_native_stage_select.inc`, `nds_renderer_native_owners.c` and
`nds_renderer_assets.c` now remove the remaining Dream Land assumptions:

- Capture resolves materials and rigidity from the loaded stage, before the
  draw packet is selected. Previously the first Yoster capture read Dream
  Land's binding 20 from a 19-binding topology and failed.
- Final admission uses each packet's submit census; its duplicate literal
  66/126/10 gate rejected Yoster's otherwise valid 35/87/42 packet.
- Raw/range draws use the run's own composed binding with a fresh matrix
  generation. The fixed binding-29 matrices are deleted (128 resident bytes).
  Failed world-matrix setup returns failure before emitting vertices.
- Yoster's rigid mask is `0x78014`: bindings 2/4 and 15-18. Its layer-0
  AnimJoint table at source offset `0x1150` animates every other drawable
  binding or its parent; capturing all 19 as rigid froze that animation.
- Dream Land's cull override and three replay slots remain scoped to Dream
  Land. Other packets execute their own native runs live.

`verify_multistage_runtime` in `check_nds_native_stage.py` audits the capture,
admission and matrix-routing seams; the pre-fix source fails its admission
control. The consumed-field manifest now classifies the two new per-stage
`workspace.binding_count` reads from step 6. Packet hashes are unchanged.
ROM execution, cadence and visual acceptance remain deferred by the owner's
code-first constraint; these are source corrections, not runtime closure.

**Cost of the remaining seven is not uniform.** Static-layer stages reuse the
generic path; stages with dynamic actors need per-stage work — Sector Z composes
its Arwings dynamically through FoxSpecial3 rather than from static owners
(`grsector.c:1087-1123`), and Yoster's own clouds are excluded from its
descriptor (`yoster.py:44-51`). Sector Z's inputs are pinned and ready when its
turn comes: camera bounds `11000/-6500/14000/-14000` (`262_GRSectorMap.c:73-76`),
fog `{0,0,0x32}` alpha 0 (`:58-59`), light `{0, 90, -0.17453294}` (`:68`),
geometry roots in `109_StageSectorFile2.c`, actors in `153_StageSectorFile3.c`.
