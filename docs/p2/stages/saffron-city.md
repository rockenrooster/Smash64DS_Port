# Saffron City — P2-4 stage 7

Status: not started · Reference: BattleShip stage data via `docs/DECOMP_MAP.md`;
door Pokémon under `it/` (stage-spawn actors — verify where source keeps them).

## Content inventory

- **Layout**: Silph Co. rooftop main platform flanked by two smaller
  building rooftops with fatal gaps between them; small floating platform;
  ledge-heavy, gap-KO identity.
- **Hazards**:
  - **Pokémon door**: the rooftop door opens on a schedule and releases a
    Pokémon that attacks (set commonly cited as Venusaur, Charmander,
    Porygon, Chansey-with-eggs — **verify the exact set and behaviors in
    source**; Chansey may drop egg items → P2-5 hook).
- **Set pieces**: city skyline, flying Pidgeys/props (background), blinking
  signage (animated texture, reduced rate fine).
- **Music**: Saffron City (Pokémon) track.
- **Visual treatment**: buildings = boxes with signage textures — cheap;
  skyline as BG layers.

## DS notes / risks

- Door Pokémon are spawned stage actors with hitboxes and lifetimes —
  final reuse of the stage-actor seam before Mushroom Kingdom's bespoke set.
- Gap falls between buildings: blast-zone vs pit semantics exact (players
  fall through gaps constantly here).
- If Chansey drops eggs pre-P2-5, stub the egg as damage-only until items
  land (recorded delta), or sequence the stage after P2-5 starts.

## Acceptance

- [ ] Collision parity sweep (three rooftops, gaps, floating platform).
- [ ] Door schedule + Pokémon set/behavior equivalent (source-verified list).
- [ ] Music + SSS entry; owner visual pass with screenshot.
- [ ] 4-CPU stress measurement banked.

## Source pins (verified 2026-09-03)

Internal name `Yamabuki`, kind `nGRKindYamabuki` (`gr/grdef.h:18`). Paths
relative to `decomp/BattleShip-main/decomp/src/`.

- Map `relocData/264_GRYamabukiMap.c`: layer table `:51-59`, geometry `:60`,
  BGM `:82`; attributes and events `:99`, `:139`, `:179`, `:187`, `:227`,
  `:233`, `:272`, `:303`, `:343`, `:349`.
- Collision `dStageYamabukiFile2_MPGeometryData_0x6E8C`
  (`relocData/112_StageYamabukiFile2.c:1342`) -- vertices `:1246`, ids `:1281`,
  links `:1288`, lines `:1295`, map objects `:1303`.
- Logic `gr/grcommon/gryamabuki.c`, 298 lines: `UpdateSleep:46`,
  `CheckNear:56` (grounded and standing on the detect line, `:64-65`),
  `MakeMonster:74` (position from the Monster map object `:84-85`, no immediate
  repeat `:93-97`, spawn `:101`), `Far:105` (x = 1600), `Near:112` (x = 960),
  `AddOffset:119`, `Open:126` / `Close:132` / `OpenEntry:138`,
  `UpdateWait:145` (gate cue `nSYAudioFGMYamabukiGate`, `:156-159`),
  `UpdateOpen:175` (tracks the monster, clamped 960 to 1600, `:185-196`),
  `Clear:202`, `ClosedWait:208`, `YakumonoPos:220`, `ProcUpdate:226`,
  `MakeGate:246`, `InitVars:268`, `MakeGround:290`.
- Parameters `GRCommonGroundVarsYamabuki` (`gr/grvars.h:212-225`) and
  `dGRYamabukiMonsterMapObjKinds:17-24`.
- Seams: two, neither Whispy's. The Pokemon are **items** --
  `itManagerMakeItemSetupCommon(NULL, item_id + nITKindGroundMonsterStart,
  ..., ITEM_FLAG_PARENT_GROUND)` (`:101`), so this stage depends on P2-5
  slice 1. The gate is a moving yakumono, `mpCollisionSetYakumonoPosID(3)`
  (`:222`).
- Music `nSYAudioBGMYamabuki = 7`. Icon `llMNMapsSaffronCitySprite`
  (`mn/mnmaps.c:518`), name `llMNMapsSaffronCityTextSprite` (`:590`).
- Risk: yakumono index 3 and DObj index 3 are hard-coded (`:108`, `:115`,
  `:186`, `:222`, `:272`) and must line up with the imported map-object slots
  (`112_StageYamabukiFile2.c:1295`, `:1303`), or the gate desynchronises from
  the collider it is supposed to be.
