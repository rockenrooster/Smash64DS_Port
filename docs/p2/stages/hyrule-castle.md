# Hyrule Castle — P2-4 stage 4

Status: not started · Reference: BattleShip stage data via `docs/DECOMP_MAP.md`.

## Content inventory

- **Layout**: the largest walk-around layout so far — main courtyard, ramp,
  raised left tower ledge, right-side tent/roof section with gaps; mostly
  hard floors, few pass-throughs (verify which).
- **Hazards**:
  - **Whirlwind (tornado)**: spawns periodically at ground positions,
    wanders, lifts and flings fighters with rising damage; spawn cadence,
    positions, and launch from source. (Wind precedent: P1's Whispy — but
    this one carries and launches, a stronger coupling with fighter
    physics.)
- **Set pieces**: castle architecture, distant background.
- **Music**: Hyrule Castle (Zelda) track.
- **Visual treatment**: stone texture set; large static geometry — bake
  aggressively.

## DS notes / risks

- Size: widest camera pulls in the VS set until Sector Z — draw-distance
  and stage-geometry submission budget checkpoint; if the stage mesh needs
  chunked culling, build it here and Sector Z inherits it.
- Tornado's carry state vs DI/mash rules — source-exact; it's a KO setup
  players know frame-deep.
- Big flat areas + 4 fighters spread = camera at maximum zoom-out; fighter
  LOD trigger check.

## Acceptance

- [ ] Collision parity sweep (large-surface set, tent gaps).
- [ ] Tornado spawn cadence/motion/launch equivalent.
- [ ] Camera bounds at 4-fighter maximum spread verified.
- [ ] Music + SSS entry; owner visual pass with screenshot.
- [ ] 4-CPU stress measurement banked (expected stress-config candidate).

## Source pins (verified 2026-09-03)

Internal name `Hyrule`, kind `nGRKindHyrule` (`gr/grdef.h:15`). Paths relative
to `decomp/BattleShip-main/decomp/src/`.

- Map file info `{&llGRHyruleMapFileID, &llGRHyruleMapMapHeader}`
  (`mn/mnmaps.c:35`).
- Collision `dStageHyruleFile2_MPGeometryData_0x599C`
  (`relocData/113_StageHyruleFile2.c:702`) -- vertices `:624`, ids `:652`,
  links `:659`, lines `:666`, map objects `:671`. Display layers `Layer0:493`,
  `Layer1:615`, `Layer3[11]:804`.
- Logic `gr/grcommon/grhyrule.c`, 477 lines, one hazard -- the tornado, a
  seven-state machine: `MakeEffect:30`, `MakeTwister:56`, `Sleep:118`,
  `Wait:128` (spawns at a random `nMPMapObjKindTwister` position),
  `Summon:155` (80 frames, arms the obstacle and plays the cue),
  `DecLifetime:179`, `GetLR:193` (**steers toward whichever side holds more
  fighters**), `Move:230`, `Turn:291` (120 frames), `Stop:307` (holds while
  the fighter is in `nFTCommonStatusTwister`, then clears the obstacle and
  ejects), `Subside:334`, `ProcUpdate:351`, `InitVars:386`, `MakeGround:421`,
  `CheckGetDamageKind:432` (`|dx| < 300`, `-300 < dy < 600`, giving
  `nGMHitEnvironmentTwister`), `CheckGetPosition:468` (exported to the CPU
  only while moving or turning -- `ft/ftcomputer.c:4927`).
- Parameters: `GRCommonGroundVarsHyrule` (`gr/grvars.h:193-210`), waits and
  velocities `:123`, `:164-170`, `:267-278`, position ids `:392-409`.
- Seam: **not** Whispy's. Ground-obstacle plus damage-kind
  (`grhyrule.c:172` into `nFTCommonStatusTwister`, `ft/def.h:563`).
- Music `nSYAudioBGMHyrule = 9`. Icon `llMNMapsHyruleCastleSprite`
  (`mn/mnmaps.c:517`), name `llMNMapsHyruleCastleTextSprite` (`:587`).
- Risk: `grHyruleTwisterInitVars` **hangs forever** if the Twister map-object
  count is 0 or above 10 (`:394-401`, a `while (TRUE) syDebugPrintf`). See the
  standing rule in `docs/p2/P2-4-stage-production.md`.
