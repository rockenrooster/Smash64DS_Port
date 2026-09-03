# Sector Z — P2-4 stage 6 (biggest stage; perf checkpoint)

Status: not started · Reference: BattleShip stage data via `docs/DECOMP_MAP.md`.

## Content inventory

- **Layout**: the Great Fox — the largest stage in the game: long hull deck,
  tail fin, wing surfaces as platforms, under-wing pocket; distinct ledge set.
- **Hazards**:
  - **Arwing**: flies in periodically, hovers, fires laser bursts across the
    deck (lasers hit both ways? — verify friendly-fire semantics), then
    leaves; spawn cadence/paths/laser damage from source. The Arwing body is
    solid? (verify — riding it is a known novelty).
- **Set pieces**: space/planet background, engine glow.
- **Music**: Sector Z (Star Fox) track.
- **Visual treatment**: one big mostly-static ship — bake everything; space
  backdrop as 2D BG layers; engine glow as billboard effects.

## DS notes / risks

- **The perf checkpoint of P2-4**: largest camera volume + widest fighter
  spreads. Inherits Hyrule's chunked culling if built; expected stress-config
  stage candidate alongside Hyrule.
- Arwing = moving stage actor with hitboxes and (verify) rideable collision —
  the most actor-like hazard; reuses the stage-actor seam from Peach's
  bumper/Congo barrel.
- Long flat deck = degenerate broadphase case (everyone colinear) — check
  the P2-2 broadphase doesn't degrade.

## Acceptance

- [ ] Collision parity sweep (hull, wings, under-wing, fin).
- [ ] Arwing cadence/path/laser behavior equivalent (+ ride rule verified).
- [ ] Camera bounds at maximum spread; fighter LOD behavior verified.
- [ ] Music + SSS entry; owner visual pass with screenshot.
- [ ] 4-CPU stress measurement banked (stress-config candidate).

## Source pins (verified 2026-09-03)

Internal name `Sector`, kind `nGRKindSector` (`gr/grdef.h:12`). Paths relative
to `decomp/BattleShip-main/decomp/src/`.

**This is the most expensive stage in the game.** `gr/grcommon/grsector.c` is
**1,131 lines** -- 1.9 times Mushroom Kingdom and 4.5 times Congo Jungle --
with roughly fifteen hazard update functions across two independent weapon
pipelines. Plan it last regardless of where the ratified order puts it.

- Map `relocData/262_GRSectorMap.c`: header
  `dGRSectorMap_MapHeader_0x0014:46`, layers wired `:50-51` and `:55`, BGM
  `:77`, nodes `:78`, descriptors and attributes `:94`, `:124`.
- Collision `dStageSectorFile2_MPGeometryData_0x8AD8`
  (`relocData/109_StageSectorFile2.c:2020`); display layers `:568`, `:1903`.
- Logic, by system:
  - Patrol and pilot: `interpAnimAxes:230`, `attachJointAnim:330`,
    `Sleep:349`, `Wait:358` (rolls the pattern and spawns), `ZNear:413`,
    line toggles `:423` and `:438`, `pilotSwap:453`, `pilotFSM:476`,
    `patrolDispatch:1023`, `flightAnims:1044`, `groundProc:1067`, `init:1087`,
    `makeGround:1123`.
  - 2D lasers: `pickLaserCount:523`, target authorisation `:533`, map and hit
    `:561` and `:574`, aim and rotate `:583` and `:608`, hop and reflector
    `:630` and `:646`, `spawn2D:663` via `wpManagerMakeWeapon` (`:685`,
    `:708`), explosion `:722` and `:733`.
  - 3D lasers: `orient3DLaser:277`, map, hit and absorb `:762`, `:779`,
    `:789`, `aimed3DSpawn:798` (`:872`), fire and cue `:887`, ammunition
    sequencer `:899`, ambient `:980`.
  - Arwing body as collision: `grSectorArwingUpdateCollisions:991` with
    `mpCollisionSetYakumonoOnID` / `PosID` / `OffID` (`:1005-1014`, `:1038`,
    `:1118`), position from `map_dobjs[0] + target_x` and `map_dobjs[1]`,
    gated on `is_arwing_line_active` and `is_arwing_z_near`.
- Parameters: `dGRSectorArwingSectorDescs:23`, `LaserCounts:36`,
  `MapPositionsX:47`, `PilotIDs:68`, `WaitTimers:129`, `TransformKinds:140`,
  plus `gr/grcommon/grsector.h:8-39`.
- Seams: `wpManagerMakeWeapon` for both laser pipelines -- they reach a
  fighter as ordinary weapons with their own attack collisions -- and
  `mpCollisionSetYakumono*` for the ship body. Neither is Whispy's push.
- Music `nSYAudioBGMSector = 4`. Icon `llMNMapsSectorZSprite`
  (`mn/mnmaps.c:515`), name `llMNMapsSectorZTextSprite` (`:584`).
- Risks: the yakumono id-1 gate plus the `weapon_head` / `map_head` /
  `map_file` linkage (`:161-208`, `:1092-1119`) and the joint tables
  (`:23-158`, `:950-962`) all have to be reproduced or the Arwing is
  intangible and the lasers spawn with null attributes. There is also a
  **US-only transform kind**, `0x53` against `0x52` (`:142-146`) -- the only
  region-dependent value found anywhere in the eight stages.
