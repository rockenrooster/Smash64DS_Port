# Mushroom Kingdom — P2-4 stage 8 (most bespoke; unlockable)

Status: not started · Reference: BattleShip stage data via `docs/DECOMP_MAP.md`.

## Content inventory

- **Layout**: the only walk-off stage — side blast lines at ground level, no
  ledges at the extremes; brick platforms, center gap bridged by moving lift
  platforms (verify configuration), classic SMB tile look.
- **Hazards/interactives** (most bespoke systems in the VS set):
  - **Warp pipes**: enterable, teleport between pipe pairs (occupancy and
    exit rules from source).
  - **POW block**: hittable, quakes all grounded opponents (uses/reset rules
    from source).
  - **Piranha Plants**: emerge from pipes on timers, bite hitboxes.
- **Set pieces**: SMB flat-tile aesthetic, overworld backdrop.
- **Music**: Mushroom Kingdom (SMB overworld arrangement) track.
- **Unlock**: unlockable stage — condition verified from source, gated in
  P2-7 (selectable in dev builds).
- **Visual treatment**: flat 2D-tile identity — candidate for BG-layer
  construction with minimal 3D (the stage practically asks for the DS's 2D
  hardware; visual doctrine explicitly allows it).

## DS notes / risks

- Walk-off blast lines change KO/camera semantics (no ledge play at edges,
  camera clamps differently) — the one stage that exercises those paths.
- Pipe warp = fighter teleport state with occupancy — small bespoke state
  machine; keep stage-owned.
- Scheduled last precisely because its systems (warp, POW, Piranhas, lifts)
  are one-offs; nothing downstream depends on them.

## Acceptance

- [ ] Collision parity sweep (walk-offs, tiles, lifts).
- [ ] Pipes/POW/Piranhas/lifts equivalent (source-verified behaviors).
- [ ] Walk-off camera + KO semantics verified.
- [ ] Music + SSS entry (unlock-gated by P2-7); owner visual pass.
- [ ] 4-CPU stress measurement banked.

## Source pins (verified 2026-09-03)

Internal name `Inishie`, kind `nGRKindInishie` (`gr/grdef.h:21`). Paths
relative to `decomp/BattleShip-main/decomp/src/`.

- Map `relocData/260_GRInishieMap.c`: header
  `dGRInishieMap_MapHeader_0x0014:40`, layer table `:42-48`, geometry `:49`,
  BGM `:71`, POW attack collision
  `dGRInishieMap_PowerBlock_GRAttackColl:89` = `{1, 20, 90, 130, 0, 30, 0}`.
- Collision `dStageInishieFile2_MPGeometryData_0x6698`
  (`relocData/107_StageInishieFile2.c:1579`).
- Logic `gr/grcommon/grinishie.c`, 588 lines, **three** systems:
  - Seesaw platforms: `UpdateFighterStatsGA:61`, `GetPressure:90` (sums the
    weight of every fighter whose floor line matches), `ScaleUpdateWait:118`
    (alternating altitude and acceleration; falls above 1100), `Fall:224`,
    `Step:252`, `Retract:266`, `ScaleProcUpdate:320`, `MakeScale:345`.
    Parameters `dGRInishieScaleLineGroups:17`, `ScaleMapObjKinds:14`.
  - POW block: `PBUpdateWait:432` (arms on battle start), `PBSetWait:442`,
    `PBUpdateMake:449` (spawns `nITKindPowerBlock` at a cached position,
    `:465`), `PBUpdateDamage:477`, `PBProcUpdate:488`, `MakePowerBlock:507`,
    `SetDamage:536`, `CheckGetDamageKind:545`.
  - Piranhas: `PakkunSetWait:402`, `MakePakkun:413` -- two `nITKindPakkun` at
    the PakkunL and PakkunR map objects, item-owned once spawned.
- Seams: moving yakumono displacement (`:340-341`) for the seesaws,
  `ftMainCheckAddGroundHazard` for the POW, and item spawning for the POW and
  both Piranhas -- so this stage depends on P2-5 slice 1.
- Music `nSYAudioBGMInishie = 2`; its 20-second warning variant,
  `nSYAudioBGMInishieHurry = 3`, is the next enumerator. Icon
  `llMNMapsMushroomKingdomSprite` (`mn/mnmaps.c:519`), name
  `llMNMapsMushroomKingdomTextSprite` (`:591`).
- Risk: `grInishieMakePowerBlock` **hangs forever** if the POW map-object
  count is 0 or above 10 (`:515-522`). See the standing rule in
  `docs/p2/P2-4-stage-production.md`.
