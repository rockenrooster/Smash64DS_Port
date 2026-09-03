# Congo Jungle — P2-4 stage 3

Status: not started · Reference: BattleShip stage data via `docs/DECOMP_MAP.md`.

## Content inventory

- **Layout**: large wooden main platform, two lower side platforms, upper
  platforms; open underside.
- **Hazards/interactives**:
  - **Barrel Cannon**: patrols beneath the stage on a path, rotates; a
    fallen fighter enters it, aims with rotation, fires on input (or
    timeout? — verify) — a rescue/KO-mixup mechanic. Two-body-ish state
    (fighter-in-barrel), input semantics and launch power from source.
- **Set pieces**: jungle backdrop, waterfall (animated background — reduced
  rate per visual doctrine is fine).
- **Music**: Congo Jungle (DK) track.
- **Visual treatment**: wood/vine textures, dark palette; waterfall as
  scrolling 2D layer candidate.

## DS notes / risks

- Barrel is fighter-state machinery owned by the stage — implement via the
  stage-actor seam with a fighter-capture state (reuses capture plumbing
  from grabs/eggs).
- Barrel path timing + rotation rate are gameplay (recovery planning);
  source-exact.
- Underside camera: fights extend far below the deck — bounds check.

## Acceptance

- [ ] Collision parity sweep.
- [ ] Barrel: entry, aim, fire, cooldown, path/rotation timing equivalent.
- [ ] Music + SSS entry; owner visual pass with screenshot.
- [ ] 4-CPU stress measurement banked.

## Source pins (verified 2026-09-03)

Internal name `Jungle`, kind `nGRKindJungle` (`gr/grdef.h:13`). Paths relative
to `decomp/BattleShip-main/decomp/src/`.

- Map `relocData/261_GRJungleMap.c`: header `dGRJungleMap_MapHeader_0x0014:31`,
  layer table `:33-39`, throw descriptor `dGRJungleMap_TaruCannThrow_HitDesc:80`.
- Collision `dStageJungleFile2_MPGeometryData_0x9AFC`
  (`relocData/108_StageJungleFile2.c:855`).
- Logic `gr/grcommon/grjungle.c`, 202 lines, one hazard -- the barrel cannon:
  `AddAnimOffset:37`, `AddAnimFill:47`, `AddAnimShoot:53`, `UpdateMove:59`
  (counts down, then rotates by a random plus/minus 0.07 step, wait 90),
  `UpdateRotate:74`, `ProcUpdate:92`, `MakeTaruCann:107`, `MakeGround:134`,
  `CheckGetDamageKind:142` (280-unit box at `:165`, giving
  `nGMHitEnvironmentTaruCann` at `:182`), pose exports `:193` and `:199`.
- Seam: **not** Whispy's velocity push. The cannon is the ground-obstacle
  capture seam -- `ftMainCheckAddGroundObstacle` (`ft/ftmain.c:1592`) into
  `ftCommonTaruCannSetStatus` / `ShootFighter`
  (`ft/ftcommon/ftcommontarucann.c:60`, `:97`).
- Music `nSYAudioBGMJungle = 5`. Stage-select icon
  `llMNMapsCongoJungleSprite` (`mn/mnmaps.c:516`), name
  `llMNMapsCongoJungleTextSprite` (`:585`).
- Risk: the shoot math reads `gMPCollisionGroundData` together with the throw
  hit descriptor (`ftcommontarucann.c:100`), and the cannon's `map_head` is
  `map_nodes - &llGRJungleMapMapHead` (`grjungle.c:112`). Both need the exact
  link symbols or capture and launch break.
