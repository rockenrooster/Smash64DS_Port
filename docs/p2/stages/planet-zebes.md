# Planet Zebes — P2-4 stage 5

Status: not started · Reference: BattleShip stage data via `docs/DECOMP_MAP.md`.

## Content inventory

- **Layout**: uneven main terrain over an open pit, several pass-through
  platforms at varying heights, one moving platform (verify), asymmetric
  ledges.
- **Hazards**:
  - **Acid**: rises from below on a schedule to varying heights (sometimes
    flooding most of the stage), damages and launches upward on contact;
    rise/fall timing table, damage, and launch values from source. The
    launch is survival-relevant (acid can save recoveries — players use it).
- **Set pieces**: cavern background, Metroid-esque ambience props.
- **Music**: Planet Zebes (Metroid) track.
- **Visual treatment**: acid = animated translucent plane (scrolling texture,
  reduced update rate fine); cavern as dark baked geometry + BG layer.

## DS notes / risks

- Acid is a full-width dynamic hurt-surface with a height function —
  implement as stage-owned surface, not a particle; its contact test joins
  the fighter ground/hurt seam.
- Translucent full-width plane per frame: fill-rate/polygon budget check on
  DS (single quad strip, not per-cell geometry).
- Acid interplay with items (floating? destroyed?) — verify when P2-5 lands;
  leave a hook row.

## Acceptance

- [ ] Collision parity sweep.
- [ ] Acid schedule/heights/damage/launch equivalent.
- [ ] Music + SSS entry; owner visual pass with screenshot.
- [ ] 4-CPU stress measurement banked.

## Source pins (verified 2026-09-03)

Internal name `Zebes`, kind `nGRKindZebes` (`gr/grdef.h:14`). Paths relative
to `decomp/BattleShip-main/decomp/src/`.

- Map `relocData/257_GRZebesMap.c`: header `dGRZebesMap_MapHeader_0x0014:27`,
  layer table `:29-35`, acid attack collision
  `dGRZebesMap_Acid_GRAttackColl:76`.
- Collision `dStageZebesFile2_MPGeometryData_0x6160`
  (`relocData/105_StageZebesFile2.c:1799`); display layer `:1686`.
- Logic `gr/grcommon/grzebes.c`, 250 lines, one hazard -- the acid, a
  five-state timer: `SetLevelStep:54` (`step = (target + rand*250 - cur) /
  240`), `SetRandomWait:62`, `MakeAcid:71`, `UpdateWait:118`,
  `UpdateRumble:127` (quake, 18 frames), `UpdateNormal:139`,
  `UpdateShake:152`, `UpdateRise:167` (accumulates into the DObj's Y over 240
  frames, cycling the 16 attribute rows), `ProcUpdate:190`, `MakeGround:213`,
  `CheckGetDamageKind:225`, `GetLevelInfo:245`.
- Parameters: `dGRZebesAcidAttributes[16]:13` -- `{base, min, max, level}` per
  cycle.
- Seam: **not** Whispy's. The acid is the ground-*hazard* callback seam,
  `ftMainCheckAddGroundHazard` (`grzebes.c:219`) plus `nGMHitEnvironmentAcid`
  (`:236`).
- Music `nSYAudioBGMZebes = 1`. Icon `llMNMapsPlanetZebesSprite`
  (`mn/mnmaps.c:516`), name `llMNMapsPlanetZebesTextSprite` (`:586`).
- Risk: the damage test sums **two** DObj Y offsets (`:233`) while the rise
  writes only one (`:171`), so the port has to preserve the acid's
  scene-graph parenting or hits will not line up with what is drawn.
