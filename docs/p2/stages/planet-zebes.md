# Planet Zebes — P2-4 stage 5

Status: layer-1-only topology in native admission probe 2026-09-07 (see block below) · Reference: BattleShip stage data via `docs/DECOMP_MAP.md`.

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

## Native admission status (2026-09-07)

MEASURED. Zebes captures layer 1 only (no layer-0 DObjs), so the layer-0 order check declined every frame with reason 4 — fixed in-tree by returning TRUE on zero rows (`src/port/renderer_adapter_stage.c:2195-2212`); `summary-a4.txt` reads `zebes-a4 ... stage_reject_reason=4`, earlier runs exited without output. Shot `artifacts/visibility/2026-09-06_stage-admission-zebes-a4-shot1.png`. Probe `builds/resume-20260905/stage-qa/stage-admission-all.ps1`.
- Gaps: acid logic runs (hazard registry, schedule, damage) but the acid GObj has no native route — draw is generic DL-links only. Visual: MObj copy-normalize is complete; the `0x6B` SPLIT-material interpretation (TLUT/frame/lfrac mapping) stays open. Detail in `builds/resume-20260905/agents-0906/zebes_acid.final.md` and `zebes_acid_visual.final.md`.
- Byte lanes: `ndsRelocNormalizeGroundDataBounds` layer_mask + fog/emblem (`src/port/reloc_backend_assets.c:8910-8930`); wallpaper Sprite header (see `docs/BUGS.md`).
