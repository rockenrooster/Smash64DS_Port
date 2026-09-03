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
