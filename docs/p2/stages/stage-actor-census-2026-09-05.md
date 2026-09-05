# Stage actor census (2026-09-05)

Read-only agent sweep (Muse Spark 1.3 Contributor, swarm-probe), reported at
LOW confidence: the risk ranking and the creator/decision-chain citations were
traced; DL sizes and offsets marked UNVERIFIED were not measured. Treat every
row as a pointer to verify against the cited source before building on it.
The finding that matters: seven mechanical stage actors (Jungle barrel, Yoster
clouds, Zebes acid, Inishie scale, Sector Arwings, Hyrule tornado, Yamabuki
gate) run their source logic on the DS but have NO render path -- their GObjs
never submit -- so the hazard acts without being seen. Owner: P2-4 native
actors (board row P2-4).

Read-only census. decomp/ treated as reference only. Descriptors under `scripts/stages/native_stage_descriptors/*.py` define STATIC packets; each docstring names excluded runtime actors. Decision chain traced: `ndsStageGCDrawAllLoopClassifyGObj` (`src/port/reloc_backend_movement.c:12672`) admits only `gGRCommonLayerGObjs[0..3]` and `gGRCommonStruct.pupupu.map_gobj[0..3]` as stage; `ndsRendererAdapterCommitNativeStageDisplay` (`src/port/renderer_adapter_stage.c:3394`) commits only `workspace->segments[i]` (sources LAYER / PUPUPU_MAP per `:2111-2130`); legacy per-node submit is `ndsRendererAdapterSubmitStageDObjNode` (`:6089`) gated by `ndsRendererAdapterStageDObjDrawable` (`:1196`); items via `ndsStageGCDrawAllLoopSubmitItemDObj` -> `ndsRendererAdapterSubmitItemDObjTree` (`reloc_backend_movement.c:12377-12426`) gated by `ndsStageGCDrawAllLoopIsItemDisplay` (`:11820`); effects gated by `ndsStageGCDrawAllLoopIsEffectDisplay` (`:11887`). `gcDrawDObjTreeForGObj / gcDrawDObjTreeDLLinksForGObj / gcDrawDObjDLHead0` are record-only shims (`src/port/opening_movie_backend.c:4516-4554`). Ground-kind actor GObjs match none of the above => nothing today.

### Dream Land / Pupupu (VS gkind Pupupu)
Descriptor `scripts/stages/native_stage_descriptors/dreamland.py:161-177` bakes actors INTO packet as map0..3 owners (`stage_actors` file 152 @ `0x10F0`/`0x1770`/`0x2A80`/`0x31F8`). No excluded model actor.
| actor | creator (file:line) | DObjDesc symbol (file, offset) | size | animation | mechanical? | DS path today (function) | proposed path |
|---|---|---|---|---|---|---|---|
| Whispy eyes | `decomp/.../gr/grcommon/grpupupu.c:666` `grPupupuMakeMapGObj(&llGRPupupuMapMapHead...)` | `llGRPupupuMapWhispyEyesTransformKindsDObjDesc` (GRPupupuMap/file 152, `reloc_data_symbols.us.txt:4005` = `0x10F0`) | UNVERIFIED | `llGRPupupuMapWhispyEyesLeft/RightTurn/BlinkAnimJoint + MatAnimJoint` (`grpupupu.c:74-77`, offsets `reloc_data_symbols.us.txt:4010-4015`) | yes (wind push `grPupupuWhispySetWindPush` `:165`, status machine `:206-347`) | native owner packet (`PrepareNativeStageOwner` `renderer_adapter_stage.c:3050` + `CommitNativeStageDisplay` `:3394`) | keep native packet (already baked; mechanical + animated) |
| Whispy mouth | `grpupupu.c:667` map_gobj[1] | `llGRPupupuMapWhispyMouthTransformKindsDObjDesc` (file 152, `:4007` = `0x1770`) | UNVERIFIED | 8 mouth Anim/MatAnim rows (`grpupupu.c:81-96`, offsets `:4016-4031`) | yes (same wind) | native owner packet (same) | keep native packet |
| Flowers back/front | `grpupupu.c:668-669` map_gobj[2..3] | `llGRPupupuMapFlowersBack/FrontTransformKindsDObjDesc` (file 152, `:4008-4009` = `0x2A80`/`0x31F8`) | UNVERIFIED | wind-loop status (`grpupupu.c:56-64`), no joint cited | no (wind-reactive decor) | native owner packet (same; map3 link 16 `dreamland.py:173-174`) | keep native packet |
| Leaves/dust FX | `grpupupu.c:215` `grPupupuWhispyLeavesMakeEffect`, `:474` `...DustMakeEffect` | none (particle `xf`, not DObjDesc) | UNVERIFIED | particle | no | UNVERIFIED (particle runtime flag-gated; `battleship_grpupupu_ground.c:520-526` notes `NDS_R2_PARTICLE_RUNTIME=0` stub) | baked sprite (`PROJECT_GOAL.md:143` sprite-based effects) |

### Yoster (Yoshi's Island)
Excluded per `scripts/stages/native_stage_descriptors/yoster.py:44-51`: three cloud platforms.
| actor | creator | DObjDesc (file, offset) | size | animation | mechanical? | DS path today | proposed |
|---|---|---|---|---|---|---|---|
| Cloud x3 (solid/evaporate platforms) | `decomp/.../gr/grcommon/gryoster.c:199` `grYosterInitAll`, display `:216` `gcAddGObjDisplay(map_gobj, gcDrawDObjTreeForGObj, 6...)`, child `:239` `gcAddChildForDObj(coll_dobj, ...&llGRYosterMapCloudDisplayList)`, MObj `:244` `...&llGRYosterMap_4B8_MObjSub`; import gate `src/import/battleship_gryoster_ground.c:39` declares `gcSetupCustomDObjs`, setup dispatch via `src/import/battleship_grpupupu_ground.c:546` `ndsGRYosterSetupInitAll()` (full per-cloud import body UNVERIFIED) | template `dStageYosterFile3_DObjDesc_0x0100[5]` (`decomp/.../relocData/154_StageYosterFile3.c:37`), `llGRYosterMapMapHead` (`reloc_data_symbols.us.txt:4135` = `0x100`); cloud DL `llGRYosterMapCloudDisplayList` (`:4138` = `0x580`) | `DL_0x0000[16]` + `DL_0x0080[16]` + `DL_0x0580[29]` = 61 Gfx words (244 bytes) (`154_StageYosterFile3.c:26-32,168`); verts UNVERIFIED | `dGRYosterCloudMatAnimJoints` = Solid/ Evaporate (`gryoster.c:12`), offsets `0x670`/`0x690` (`:4139-4140`); per-frame `grYosterProcUpdate` `:178-194` | yes (yakumono platforms, stand check `:50-53`, on/off `:81/:142`, pos `:134`) | nothing (ground kind, link 6, not layer/map => `ClassifyGObj` `:12672` FALSE, `CommitNative` `:3394` no segment) | native owner packet (shared template x3 + collision-synced MatAnim; sprite would break stand/collision telegraph; `PROJECT_GOAL.md:427-439`) |

### Castle (Peach's Castle)
Excluded per `scripts/stages/native_stage_descriptors/castle.py:59-73`: bumper item + Lakitu effect.
| actor | creator | DObjDesc | size | animation | mechanical? | DS path today | proposed |
|---|---|---|---|---|---|---|---|
| Bumper | `decomp/.../gr/grcommon/grcastle.c:57` `itManagerMakeItemSetupCommon(NULL, nITKindGBumper...)`, steered `:12-22` `grCastleBumperProcUpdate`; import `src/import/battleship_grcastle_ground.c:20` notes item, setup `ndsGRCastleSetupInitAll` `:102-109` calls `ndsBaseGRCommonSetupInitAll` (model import UNVERIFIED, item bank-owned) | none in stage file (item desc, not stage DObjDesc) | UNVERIFIED | item anim (UNVERIFIED which joint) | yes (bumper hazard) | legacy N64-DObj item path (`IsItemDisplay` `:11820` + `SubmitItemDObjTree` `:12424`) | keep legacy item path (already renders; item system owns model) |
| Lakitu (L/R) | `decomp/.../ef/efground.c:40-106` descs (per `castle.py:65-67`), DObj `llGRCastleMapLakituDObjDesc` @ `0x4118` (`reloc_data_symbols.us.txt:4078`), anims `...LakituRAnimJoint 0x4220` / `...LAnimJoint 0x4370` (`:4079-4080`) | `dStageCastleFile2` Lakitu DObj, file 106 (`106_StageCastleFile2.c` container; exact Lakitu DL binding UNVERIFIED; nearby `DL_0x3F20[12]`/`DL_0x3FF8[15]` at `:872-882` NOT proven to be Lakitu) | UNVERIFIED | R/L AnimJoints above | no (transport/effect) | legacy effect path if kind==effect (`IsEffectDisplay` `:11887` + `SubmitEffectDObj`), else nothing — exact GObj kind UNVERIFIED | baked sprite if effect path fails (decorative carrier; `PROJECT_GOAL.md:143-144`) |

### Jungle (Kongo Jungle)
Excluded per `scripts/stages/native_stage_descriptors/jungle.py:15-20`: barrel cannon `grJungleMakeTaruCann (grjungle.c:107)`.
| actor | creator | DObjDesc | size | animation | mechanical? | DS path today | proposed |
|---|---|---|---|---|---|---|---|
| Taru cannon (barrel) | `decomp/.../gr/grcommon/grjungle.c:107` `grJungleMakeTaruCann`, display `:117` `gcAddGObjDisplay(tarucann_gobj, gcDrawDObjTreeForGObj, 6...)`, setup `:119` `grModelSetupGroundDObjs(...&llGRJungleMapMapHead...)`, joints `:122` `...TaruCannDefaultAnimJoint`; import gate `src/import/battleship_grpupupu_ground.c:574` `ndsGRJungleSetupInitAll()` + `src/import/battleship_grjungle_ground.c:99-106` (model body import UNVERIFIED) | `dStageJungleFile3_DObjDesc_0x0A98[3]` (`decomp/.../relocData/158_StageJungleFile3.c:60`); symbols `llGRJungleMapMapHead 0xA98` / `...TaruCannDefaultAnimJoint 0xB20` (`reloc_data_symbols.us.txt:4097-4098`); descriptor says "file 158 @ 0xA98" vs symbols say map file — mapping UNVERIFIED | `DL_0x0910[31]` + `DL_0x0A08[2]` + `DL_0x0A18[16]` = 49 Gfx (196 bytes); `Vtx_0x0830[3]` + `Vtx_0x0860[11]` = 14 verts (`158_StageJungleFile3.c:34-55`) | Default (`0x0B28[11]`), Fill (`0x0B68[32]`), Shoot (`0x0BF8[57]`) (`:74-142`; `grjungle.c:49/55`) | yes (capture `grJungleTaruCannCheckGetDamageKind` `:142-184`, update `:59-101`) | nothing (ground kind link 6; `ClassifyGObj` FALSE; needs matrix kind `0x28` + flag 4 per `jungle.py:16-18` which packet lacks) | native owner packet (animated + mechanical + custom matrix; static layer/sprite would break aim/capture telegraph) |

### Hyrule Castle
Descriptor `scripts/stages/native_stage_descriptors/hyrule.py:9-12`: tornado/actors/effects independent.
| actor | creator | DObjDesc | size | animation | mechanical? | DS path today | proposed |
|---|---|---|---|---|---|---|---|
| Tornado | `decomp/.../gr/grcommon/grhyrule.c:56` `grHyruleMakeTwister`, DObj `:74` `gcAddDObjForGObj(twister_gobj, NULL)` (no model), FX `:80` `grHyruleTwisterMakeEffect`, obstacle `:172` `ftMainCheckAddGroundObstacle`; import `src/import/battleship_grhyrule_ground.c:97-99` `ndsGRHyruleSetupInitAll` -> base only (twister import UNVERIFIED); guard `battleship_grpupupu_ground.c:605-617` | none (NULL desc + `LBParticle`) | UNVERIFIED | positional update `:230-301`, no AnimJoint cited | yes (`grHyruleTwisterCheckGetDamageKind`, launch + wind) | nothing (ground-kind GObj fails `IsEffectDisplay` `:11889` which requires `nGCCommonKindEffect`; `CommitNative` has no segment) | baked sprite column (particle/telegraph only, no DObj model; `PROJECT_GOAL.md:143` sprite effects; preserves telegraph without DObj path) |

### Zebes (Planet Zebes)
Excluded per `scripts/stages/native_stage_descriptors/zebes.py:19-23`: acid + Ridley/ship FX.
| actor | creator | DObjDesc | size | animation | mechanical? | DS path today | proposed |
|---|---|---|---|---|---|---|---|
| Acid pool | `decomp/.../gr/grcommon/grzebes.c:71` `grZebesMakeAcid`, display `:83` `gcAddGObjDisplay(map_gobj, gcDrawDObjTreeDLLinksForGObj, 12...)`, desc `:87`, MObj `:93`, joints `:98-99`, hazard `:219` `ftMainCheckAddGroundHazard`; import `src/import/battleship_grzebes_ground.c:70` defines `llGRZebesMapAcidDObjDesc NDS_RELOC_LVALUE(0xb08u)`, setup gate `:103+` (full acid import UNVERIFIED) | `llGRZebesMapAcidDObjDesc 0xB08`, `...AcidMObjSub 0x8C0`, `...AcidAnimJoint 0xB90`, `...AcidMatAnimJoint 0xBD0`, `...AcidGRAttackColl 0xBC` (`reloc_data_symbols.us.txt:4058-4062`); bank UNVERIFIED (map file 105 vs file 3 claim in `zebes.py:20-21`) | UNVERIFIED (acid DL not isolated in `105_StageZebesFile2.c` grep) | Acid AnimJoint + MatAnimJoint above; level machine `:118-207` | yes (rising hazard, `grZebesAcidCheckGetDamageKind` `:225-236`) | nothing (ground kind link 12; `ClassifyGObj` FALSE) | native owner packet (height-animated hazard; sprite/static would break height telegraph) |
| Ridley/ship BG FX | `decomp/.../ef/efground.c:385-388` + `424-427`, wired `:1017-1022` (per `zebes.py:21-23`) | UNVERIFIED | UNVERIFIED | UNVERIFIED | no (background) | UNVERIFIED (effect path if kind==effect else nothing) | baked sprite / static layer (non-colliding backdrop) |

### Sector Z
Excluded per `scripts/stages/native_stage_descriptors/sector.py:14-16` + `docs/p2/P2-4-stage-production.md:531-537`: Arwings via FoxSpecial3 (`grsector.c:1087-1123`).
| actor | creator | DObjDesc | size | animation | mechanical? | DS path today | proposed |
|---|---|---|---|---|---|---|---|
| Arwing ships xN + lasers | `decomp/.../gr/grcommon/grsector.c:23-43` `dGRSectorArwingSectorDescs` (`llGRSectorMapArwing0-7SectorDesc`) + `dGRSectorArwingAnimJoints` (`llGRSectorMapArwing0-5AnimJoint`), weapons `:161-207` Laser2D/3D descs, entry joint `:462` `llFoxSpecial3_2EB4_AnimJoint`; import `src/import/battleship_grsector_ground.c:121` stages Arwing DObjDesc, setup `:150-152` base only (ship composition import UNVERIFIED) | `llGRSectorMapArwing0SectorDesc 0x0000` .. `...7SectorDesc 0x11D0` (`reloc_data_symbols.us.txt:4111-4118`); `...Arwing0AnimJoint 0x0000` .. `...5AnimJoint 0x1DE4` (`:4119-4124`); file = 153/stage-sector per `P2-4:537` (exact .c UNVERIFIED) | UNVERIFIED | Arwing AnimJoints above + `grSectorArwingAddAnim` `:330-339` | yes (lasers are `WPDesc` weapons `:161-207`, collision flags `:403-406`) | nothing verified (sector ships not in layer/map capture; FoxSpecial3 native admits in `renderer_adapter_stage.c:4726-4747` cover entry FX only — stage Arwings UNVERIFIED) | native owner packet (multiple animated ships + live weapons; cannot be static/sprite without breaking fire telegraphs) |

### Yamabuki (Saffron City)
Excluded per `scripts/stages/native_stage_descriptors/yamabuki.py:25-33`: gate + monster/item actors.
| actor | creator | DObjDesc | size | animation | mechanical? | DS path today | proposed |
|---|---|---|---|---|---|---|---|
| Gate door | `decomp/.../gr/grcommon/gryamabuki.c:246` `grYamabukiMakeGate`, display `:252` `gcAddGObjDisplay(gate_gobj, gcDrawDObjTreeDLLinksForGObj, 6...)`, base `:257` `...&llGRYamabukiMapMapHead`; import `src/import/battleship_gryamabuki_ground.c:72` notes Monster read, setup `:81-83` base only (gate import UNVERIFIED) | `llGRYamabukiMapMapHead 0x8A0` (`reloc_data_symbols.us.txt:4178`) | UNVERIFIED | Open `llGRYamabukiMapGateOpenAnimJoint 0x9B0` (`:128` / `:4176`), Close `0xA20` (`:134` / `:4177`) | semi (door timing + yakumono pos `:220-224`; spawner gate) | nothing (ground kind link 6) | native owner packet (door anim tied to spawn timing) |
| Monster (random ground item) | `gryamabuki.c:74` `grYamabukiGateMakeMonster` -> `:101` `itManagerMakeItemSetupCommon(NULL, item_id + nITKindGroundMonsterStart...)` | item descs (not stage DObjDesc) | UNVERIFIED | item anim | yes (monster attacks; `dGRYamabukiMonsterAttackKind` `:14`) | legacy item path (`IsItemDisplay` + `SubmitItemDObjTree`) | keep legacy item path |

### Inishie (Mushroom Kingdom)
Excluded per `scripts/stages/native_stage_descriptors/inishie.py:21-30`: scale, Pakkun, PowerBlock.
| actor | creator | DObjDesc | size | animation | mechanical? | DS path today | proposed |
|---|---|---|---|---|---|---|---|
| Scale beam + 2 platforms | `decomp/.../gr/grcommon/grinishie.c:345` `grInishieMakeScale`, display `:358` link 6, desc `:359` `...&llGRInishieMapScaleDObjDesc`, platforms `:370-372` `gcAddDObjForGObj(...&llGRInishieMapMapHead)` + `gcDrawDObjDLHead0`; port `src/import/battleship_grinishie_scale.c:908-946` verified: `gcAddGObjDisplay(...,gcDrawDObjTreeForGObj,6)` `:908`, `grModelSetupGroundDObjs(...+llGRInishieMapScaleDObjDesc)` `:911-916`, two `gcDrawDObjDLHead0` platform GObjs `:939-945`; example cite `battleship_grinishie_scale.c:914` | `llGRInishieMapScaleDObjDesc 0x380` + `llGRInishieMapMapHead 0x5F0` (`reloc_data_symbols.us.txt:4083-4084`); file `155_StageInishieFile3.c:124` `DObjDesc_0x0380[6]` | beam DLs `DL_0x01C8[18]`+`DL_0x0258[18]`+`0300[5]`+`0328[3]`+`0340[8]` (`155:94-122`); Vtx `0048[8]`+`00C8[4]`+`0108[4]`+`0148[2]`+`0168[4]`+`01A8[2]` = 24 verts (`155:64-92`); platform DL UNVERIFIED | Retract `llGRInishieMapScaleRetractAnimJoint 0x734` (`:4085`; `155:168`; applied `grinishie.c:260-261`), update `:118-337` | yes (see-saw collision, weights `:90-130`) | nothing (3 ground GObjs, not layer/map => `ClassifyGObj` FALSE; `CommitNative` no segment) | native owner packet (multi-GObj animated + collision; static/sprite breaks balance telegraph) |
| Pakkun x2 (piranha items) | `grinishie.c:413` `grInishieMakePakkun` -> `:427` `itManagerMakeItemSetupCommon(NULL, nITKindPakkun...)`; import `src/import/battleship_grinishie_ground.c:102` notes two | `dStageInishieFile3_DObjDesc_0x0C30[3]` (`155:247`), joint `0x0CC8[9]` (`155:263`) — item use UNVERIFIED | `DL_0x0B40[30]` (`155:242`); Vtx `0B00[4]` (`155:237`) | joints `0x0CC8`/`0x0CF8[62]`/`0x0E04[12]` (`155:263-375`) | yes (bite hazard) | legacy item path | keep legacy item path |
| PowerBlock item | `grinishie.c:507` `grInishieMakePowerBlock` -> `:465` `...nITKindPowerBlock...`, hazard `:538` `ftMainCheckAddGroundHazard`; guard `battleship_grpupupu_ground.c:642-654` | `dStageInishieFile3_DObjDesc_0x11F8[3]` (`155:410`), joints `0x1288[72]`/`0x13B8[20]` (`155:424-507`) | `DL_0x10D0[37]` (`155:405`); Vtx `0F90[16]`+`1090[4]` = 20 (`155:395-400`) | above + `...PowerBlockGRAttackColl` (`grinishie.c:532`) | yes (hitblock hazard) | legacy item path | keep legacy item path |

### Small / 1P-static stages (no gr*Make* actors)
Per descriptors, packet-only; no runtime model actor to render.
- PupupuSmall: `pupupusmall.py:21-27` no gr* TU; layers via `grDisplayMakeGeometryLayer`. Nothing to census.
- YosterSmall: `yostersmall.py:21-28` no gr* TU (Yoshi Team are fighters, not stage). Nothing.
- Metal: `metal.py:26-31` no gr* TU. Nothing (wallpaper sprite excluded by rule, not actor).
- Zako: `zako.py:16-22` no gr* TU (Polygon Team are fighters). Nothing.
- Last: `last.py:21-25` no gr* TU; outside packet are Master Hand boss wallpaper FX `sc1PGameBossMakeWallpaperEffect (sc1pgameboss.c:853, arms :461-549/:681-746/:923-962)` + camera `:399-460` (per `last.py:27-35`). DS path UNVERIFIED (1P scene; VS HW loop gated `reloc_backend_movement.c:13302-13303`). Proposed: baked sprite (background FX, non-colliding).

### Bonus1 Mario (Break the Targets)
Excluded per `scripts/stages/native_stage_descriptors/bonus1_mario.py:18-31`: targets (platforms/bumpers explicitly absent on Mario row).
| actor | creator | DObjDesc | size | animation | mechanical? | DS path today | proposed |
|---|---|---|---|---|---|---|---|
| Targets x10 | `decomp/.../sc/sc1pmode/sc1pbonusstage.c:434` `sc1PBonusStageMakeTargets` via `dSC1PBonusStageTargetDescs (:19-26)`, called `:507-510`; count guard `== SCBATTLE_BONUSGAME_TASK_MAX (:461-466)`; port `src/import/battleship_sc1pbonusstage.c:9,42` indexes descs (full GObj import UNVERIFIED) | `dSC1PBonusStageTargetDescs` Mario row (stage file UNVERIFIED; offsets UNVERIFIED) | UNVERIFIED | UNVERIFIED (break anim presumed; source not read) | yes (break tasks define clear) | nothing in VS battle path (1P scene; HW submit gated `nSCKindVSBattle` `:13302`); 1P render UNVERIFIED | native owner packet per target (breakable + countable; static layer would freeze break state; sprite would lose break telegraph) |
| Platforms / bumpers | explicitly none on Mario: bumper row `{0x0,0x0}` (`:109-113`), Bonus2 platforms only `:539-598` gated `:733-739` (per `bonus1_mario.py:24-28`) | n/a | n/a | n/a | n/a | n/a | none |

### Bonus3 (Race to the Finish)
Actors per `decomp/.../gr/grbonus/grbonus3.c` + `src/import/battleship_grbonus3.c:1-48`.
| actor | creator | DObjDesc | size | animation | mechanical? | DS path today | proposed |
|---|---|---|---|---|---|---|---|
| Bumpers (items) | `grbonus3.c:15` `grBonus3MakeBumpers` spawns `nITKindGBumper` per DObjDesc past sentinel + joint `:26/:36`; port owns names (`battleship_grbonus3.c:7-9`), `grBonus3MakeGround :98-106` | `llGRBonus3MapBumpersDObjDesc 0x0`, `...BumpersAnimJoint 0x110` (`reloc_data_symbols.us.txt:4328-4331`; `battleship_grbonus3.c:88-94` notes unstaged, file GRBonus3Map) | UNVERIFIED | per-bumper AnimJoint above | yes (bumper hazard) | legacy item path | keep legacy item path |
| Taru bombs (timed item spawns) | `grbonus3.c:59` `grBonus3TaruBombMakeActor` + update `:43-55` (spawn `nITKindTaruBomb` every 180 ticks `:53-55`), count guard `:65-72` | spawn pos from `nMPMapObjKind1PGameBonus3TaruBomb` map object, not DObjDesc | UNVERIFIED | item anim | yes (explosive hazard) | legacy item path | keep legacy item path |
| Finish | `grbonus3.c:92` `grBonus3FinishMakeActor` + `:80-95` detect grounded + floor mat `nMPMaterialDetect` => announce Complete + FGM (no positions; `battleship_grbonus3.c:40-45`) | none (collision-material region) | n/a | none | yes (win rule, no model) | nothing to render (logic only) | no render path (logic only) |

### Risk ranking (mechanical + unrendered first)
1. Jungle barrel — cannon capture/damage, animated, custom matrix; nothing today. `grjungle.c:107-184`; size 49 Gfx/14 verts proven.
2. Yoster clouds x3 — standing platforms + vanish timing; nothing today. `gryoster.c:199-254`.
3. Zebes acid — rising kill hazard; nothing today. `grzebes.c:71-219`.
4. Inishie scale beam/platforms — see-saw collision; nothing today (port creates 3 GObjs that never submit). `grinishie.c:345-387`; `battleship_grinishie_scale.c:908-946`.
5. Sector Arwings + lasers — live weapons; nothing verified. `grsector.c:23-43,161-207,1087+`.
6. Hyrule tornado — roaming launcher; nothing (NULL-DObj + particles). `grhyrule.c:56-172`.
7. Yamabuki gate — spawn door timing; nothing today. `gryamabuki.c:246-264`.
8. Castle bumper — hazard but legacy item path exists. `grcastle.c:57`.
9. Inishie Pakkun/PowerBlock, Yamabuki monster, Bonus3 bumpers/bombs — hazards but legacy item path exists.
10. Bonus1 targets — mechanical but 1P-only; needs owner packet, not battle path.
11. Castle Lakitu, Zebes Ridley/ship, Last boss FX, Whispy leaves/dust/vapor — decorative/effect; sprite or existing effect path.
12. Dream Land Whispy/flowers — already in native packet; no work.
13. PupupuSmall/YosterSmall/Metal/Zako — no actors by descriptor; no work.
Size/offset gaps marked UNVERIFIED above were not measured in this pass (relocData DL/Vtx not fully walked; 1P scene render not traced).

## Render path design (read-only probe, 2026-09-05, LOW confidence)

Decision taken from this: **design A** -- a per-actor native owner packet
generated by the fighter owner pipeline from the actor's DObjDesc, executed by
the existing hierarchy executor with the actor GObj's live matrix and its own
(small) slot gate, so the static stage slab stays frozen. Design B (a dynamic
owner inside the stage packet) would re-run the stage prepare every frame for
one rotating actor. The same path covers Yoster clouds, Zebes acid, the Inishie
scale, the Sector Arwings and the Yamabuki gate; the Hyrule tornado has no DObj
(particles only) and the item-backed hazards keep the legacy item path.

Read `AGENTS.md:23` (`decomp/` read-only), `PROJECT_GOAL.md:80` (fastest correct wins), `:100-117` (specialization encouraged, tooling generic), `:153-176` (compute once). Read-only probe, no edits.

(1) Fighter owner path today
- Generator `scripts/fighters/generate_nds_native_owners.py:2` walks source DLs, preserves light prefixes; owners list `P2_RUNTIME_OWNERS` at `:1562-1574` (luigi/donkey/captain/samus/link/pikachu/yoshi/ness/purin/kirby/mmario + frozen mario/fox).
- Output packet `src/nds/nds_native_fighter_owner.generated.inc:2` (`32 roots, 49 epochs, 67 runs, 626 triangles`), dense verts `:3`, task27 program `:10-12` (`NDS_TASK27_JOINT`, `ROOT`, `EPOCH`, `RUN`).
- Runtime hierarchy `src/port/renderer_adapter_matrix.c:6116` `ndsRendererAdapterPrepareNativeOwnerHierarchy(slot, fp, root, matrix_bindings, binding_count, cobj, workspace)`.
- Caller `src/port/renderer_adapter_fighter.c:3394-3405` passes `owner_slot, fp, root, native_owner_matrix_bindings, collection.selected_count`, camera from `gGCCurrentCamera`, workspace `sNdsRendererAdapterNativeOwnerWorkspace`.
- Gate `:6130-6143`: `expected_joint_count = (slot==0)?25u:27u`, `expected_binding_count = (slot==0)?14u:18u`; reject if `slot>1`, `fp/root/bindings/workspace NULL`, `binding_count != expected`, `is_use_animlocks`, `shuffle_tics`.
- Topology `:6153-6159` `ndsRendererAdapterCollectFighterTopology`, must equal expected joint count.
- XObj gate `:6201-6205`: `if ((xobj->kind != nGCMatrixKindNull) && (xobj->kind != NDS_RENDERER_ADAPTER_FIGHTER_PARTS_MTX_KIND)) return FALSE`; `0x4B` defined `:71` as fighter-parts kind.
- Local matrices `:6218-6227` `ndsRendererAdapterBuildDObjLocalMatrix` per joint, then camera `:6242-6247` `ndsRendererAdapterGetHierarchyCameraMatrices`.
- Matrix kinds supported `:2461-2484`: `case nGCMatrixKindRotRpyR: syMatrixRotRpyR`, `TraRotRpyR`, `TraRotRpyRSca`; billboard `:2514-2523` `case 33:` … `case 40: ndsRendererAdapterBuildBillboardMtx`; fighter-parts `:2536-2542`.
- Fighter routing `src/port/reloc_backend_movement.c:12917-12919` `else if (ndsStageGCDrawAllLoopIsSelectedFighter(display)) gNdsStageGCDrawAllLoopFighterDisplayCallbackCount++`; `ndsStageGCDrawAllLoopIsSelectedFighter` at `:12718-12733` requires `gobj->id == nGCCommonKindFighter` plus pool pointer.

(2) Stage packet path today
- Generator `scripts/stages/generate_nds_native_stage.py:2-13` expands exact eight callback owners into packet segments/DObj topology/bindings/runs/dense verts/texture epochs/material events; falsifier on drift.
- Jungle descriptor `scripts/stages/native_stage_descriptors/jungle.py:15-20` says barrel excluded, needs actor capture/matrix support beyond four layer callbacks; packet counts `:22-26` (`34 DObjs, 30 bindings, 72 runs, 60 texture epochs, 182 triangles`), slab `:24` `12479 bytes`; ` jungle.py:138` `owner_specs` four layers only (links 4/6/13/17), `139-142` no material sources, segment partition.
- Baked tables `src/nds/nds_native_stage_jungle.generated.inc:9-24` (`SEGMENT_COUNT 4u, DOBJ_COUNT 34u, BINDING_COUNT 30u, RUN_COUNT 72u`), `32-34` segment0 program bytes.
- Selection `src/nds/nds_native_stage_select.inc:1-7` hand-written selector over generated packets; `54-60` gkind mirror; pointer-packet rationale `:12-24` (u8 binding fields overflow if concatenated).
- Prepare once per frame `src/port/renderer_adapter_stage.c:3034` comment: `236,039 ticks/frame ... inside ndsRendererAdapterPrepareNativeStageOwner, at one call per frame`; entry `:3050` `ndsRendererAdapterPrepareNativeStageOwner(camera_gobj_ptr)`; steady reuse `:3095-3126` Task44 generation compare.
- Commit `src/port/renderer_adapter_stage.c:3394-3410` `ndsRendererAdapterCommitNativeStageDisplay(display_gobj, link_id)` matches `workspace->segments[i]`, then `ndsRendererAdapterCommitNativeStageMaterials` + `ndsRendererCommitNativeStageSegment`.
- Baked world path `src/nds/nds_renderer_native_owners.c:1872-1885` Task51 emits baked constant world via `MTX_MULT4x3`, source `sNdsNativeStageBakedWorldMatrices`; use `:1903` `baked = sNdsNativeStageBakedWorldMatrices[binding_index]`; publish `:3597` `binding_composed = frame->binding_composed`.
- Classify `src/port/reloc_backend_movement.c:12672-12716` admits only `gGRCommonLayerGObjs[0..3]` (`:12689-12703`) and `gGRCommonStruct.pupupu.map_gobj[0..3]` (`:12704-12714`); else `FALSE`.
- Record `reloc_backend_movement.c:12905-12924`: if classify true → layer/map capture mask; elif fighter → fighter count; else `gNdsStageGCDrawAllLoopNonStageCaptureCount++`.
- Display hook `:12926-12953` calls `ndsRendererAdapterCommitNativeStageDisplay` only when native armed; DObj-draw hook `:12991-13019` returns after weapon/item/effect submits when classify false. Barrel ground-kind link 6 matches none → nothing. Census `docs/p2/stages/stage-actor-census-2026-09-05.md:13` traces same chain.

(3) Barrel needs
- Creator `decomp/BattleShip-main/decomp/src/gr/grcommon/grjungle.c:107-131` `grJungleMakeTaruCann`: `:112` map_head from collision minus `llGRJungleMapMapHead`; `:115` `gcMakeGObjSPAfter(nGCCommonKindGround, NULL, nGCCommonLinkIDGround, ...)`; `:117` `gcAddGObjDisplay(tarucann_gobj, gcDrawDObjTreeForGObj, 6, ...)`; `:119` `grModelSetupGroundDObjs(tarucann_gobj, DObjDesc @ MapHead+map_head, NULL, dGRJungleTaruCannTransformKinds)`; `:120` play-anim process; `:122-123` `gcAddAnimJointAll(... llGRJungleMapTaruCannDefaultAnimJoint)`; `:125` `grJungleTaruCannProcUpdate`; `:126` `ftMainCheckAddGroundObstacle(..., grJungleTaruCannCheckGetDamageKind)`.
- Transform table `grjungle.c:12-16`: `{ 0x28, nGCMatrixKindRotRpyR, 0x00 }, { nGCMatrixKindTraRotRpyRSca, nGCMatrixKindNull, 0x00 }`. `0x28`=40 decimal lands in billboard range `renderer_adapter_matrix.c:2514-2523` (`case 33`…`case 40` → `BuildBillboardMtx`), second XObj `RotRpyR` at `:2461-2465`. DObj link builder `decomp/.../gr/grmodelsetup.c:19-36` adds `tk1/tk2` XObjs when != Null, copies translate/rotate/scale `:37-39`.
- DObj count: `decomp/.../relocData/158_StageJungleFile3.c:60-64` `dStageJungleFile3_DObjDesc_0x0A98[3]` = root `{0,NULL}`, child `{16385, DL_0x0A08}`, terminator `{18,NULL}`. So live tree 1 root + 1 child (terminator UNVERIFIED meaning; id-mask logic `grmodelsetup.c:21` `id & 0xFFF`).
- Size: `:44-46` `DL_0x0910[31]`, `:49-51` `DL_0x0A08[2]`, `:55-57` `DL_0x0A18[16]` (comment `:54` prefix fragment, no EndDL); `:34-41` `Vtx_0x0830[3]` + `Vtx_0x0860[11]` =14 verts. Matches census 49 Gfx/14 verts.
- Per-frame matrix: Default joint `:74-86` writes TRAX/TRAY/TRAZ (3540.0, -1597.5, loop ±3540); Rotate update `grjungle.c:73-89` `dobj->rotate.vec.f.z += tarucann_rotate_step`, reset `:86`; Move `:59-71` picks step ±0.07. Exports `:193-202` `grJungleTaruCannGetPosition` (translate), `GetRotate` (rotate.z). Fighter uses rotate `decomp/.../ft/ftcommon/ftcommontarucann.c:108` `angle = I_CLC_RTOD32(grJungleTaruCannGetRotate())...`; CPU aims `ftcomputer.c:6010-6016` reads both.
- Capture: `grjungle.c:142-190` box ±280 (`:165` `dist_x<280 && dist_y<280`), occupancy check `:167-181`, returns `nGMHitEnvironmentTaruCann` `:182`, adds Fill anim `:184`. Fighter status `ftcommontarucann.c:80` sets `nFTCommonStatusTaruCann`. Import `src/import/battleship_grjungle_ground.c:58-69` pins offsets MapHead 0xA98, Default 0xB20, Fill 0xB68, Shoot 0xBF8; gate `src/import/battleship_grpupupu_ground.c:574` calls `ndsGRJungleSetupInitAll`; setup `:99-107` runs base only. Logic runs, render missing.
- Anim: Default `:74-86` [11 words], Fill `:98-131` [32 words, ROTZ/TRAY/SCAX/SCAY], Shoot `:142-199` [57 words, ROTZ/TRAY/SCA]. ChildTarget `grjungle.c:37-44` `grJungleTaruCannAddAnimOffset` uses `DObjGetStruct(...)->child` + `gcAddDObjAnimJoint/Parse/Play`; Fill `:46-50`, Shoot `:52-56`.
- Textures/materials: `:22-31` LUT 16 + Tex CI4 64x64; no MObj table in file 158 (UNVERIFIED absence — Yoster/Zebes have explicit MObj, barrel file shows only mobjlink joint arrays `:67-71`, `:89-95`, `:134-139`). Jungle packet `jungle.py:11-13` says all four map material tables NULL. So barrel likely untextured-lit + one CI4 texture; exact material slot UNVERIFIED without walking `DL_0x0910`.
- Symbol/file mapping UNVERIFIED: prompt asks `105_GRJungleMap / 153_StageJungleFile3 rows`; on disk symbols live `decomp/.../tools/reloc_data_symbols.us.txt` equivalent lines census cites `:4097-4100` (MapHead 0xA98 etc.), typed file is `158_StageJungleFile3.c`, map `261_GRJungleMap.c`, geometry `108_StageJungleFile2.c` per `jungle.py:58-90`. 105/153 numbers not observed; do not build on them.

Weigh A vs B against `PROJECT_GOAL.md:80,100-117,153-176`
- A) Per-actor native owner via fighter pipeline.
  - Generator inputs: `DObjDesc_0x0A98` + `DL_0x0910/0x0A08/0x0A18` + `Vtx_0x0830/0860` + `Tex_0x0030/LUT_0x0008` + three AnimJoints + `dGRJungleTaruCannTransformKinds`; new actor spec beside `P2_RUNTIME_OWNERS` (`generate_nds_native_owners.py:1562`), not fighter slot 0/1.
  - Runtime: new route for `nGCCommonKindGround` link-6 actor GObj into hierarchy executor (`renderer_adapter_matrix.c:6116`) or sibling; cannot reuse slot 0/1 gate `:6130-6143` (25/27 joints, 14/18 bindings vs barrel ~2 joints/1-2 bindings) nor XObj gate `:6201-6205` (barrel tk1 0x28/billboard rejected today). Needs actor-slot admit + live `matrix_bindings` from actor DObj tree + `BuildDObjLocalMatrix` path already handling `RotRpyR` (`:2461`) and billboard 33-40 (`:2514`).
  - Files: `generate_nds_native_owners.py`, new `nds_native_actor_tarucann.generated.inc`, `renderer_adapter_matrix.c`, `renderer_adapter_fighter.c` (or new `renderer_adapter_actor.c`), `reloc_backend_movement.c` (classify/route), import gate `battleship_grjungle_ground.c`. Estimate ~150-250 lines + generated data (inference, not measured).
  - Per-frame cost: handful matrix loads — 1-2 `BuildDObjLocalMatrix` + camera fetch + 49-Gfx packet emit; no DObj tree re-walk beyond topology collect already in fighter path; steady stage packet untouched, preserves Task44/R2 reuse.
  - First reveal: compile fails on slot/binding-count asserts or topology mismatch; run shows barrel visible but wrong yaw if billboard tk1 mishandled, or fallback counter spikes.
  - Counters: `gNdsRendererM3PreflightAttempt/Success`, `M3Run/TriangleCount`, `NonStageCaptureCount` should drop, new actor admit/draw counts, Task36 `WorldMultCount`, Task103 matrix/commit splits.
- B) Dynamic owner inside stage packet.
  - Packet lacks: live operand kinds are `LIVE_OPERAND_ASSET_BASES/BINDING_COMPOSED/MATERIALS/CONFIG` (`nds_native_stage_jungle.generated.inc:40-44`); `binding_composed`/`binding_world` published per frame (`nds_renderer_native_owners.c:3597-3603`) from baked `sNdsNativeStageBakedWorldMatrices` (`:1903`), Task51 `:1872-1885`. No per-binding live-DObj slot, no AnimJoint re-eval, no texture-epoch delta for actor.
  - Runtime: `renderer_adapter_stage.c:3050` Prepare would read actor DObj translate/rotate.z + child joint each frame, recompose world, override one binding's composed/world, invalidate steady path (`:3095-3126`) and R2 reuse (`nds_renderer_native_owners.c:3071-3095`), re-run material/texture proof per frame.
  - Files: `generate_nds_native_stage.py` (dynamic-owner flag + extra segment/binding), jungle `.inc` regen, `renderer_adapter_stage.c`, `nds_renderer_native_owners.c` (BeginRun/EnsureWorld branch), movement route. Estimate ~100-200 lines + regen (inference).
  - Per-frame cost: same 1-2 matrix composes plus full Prepare invalidation cost (stage Prepare is 60%/236k-tick bucket per `:3032-3036` comment) — risks turning cheap static packet into per-frame rebuild; violates compute-once.
  - First reveal: stage falls back to source (texture proof drop ` :3097-3101`) or whole-stage rebuild counters (`R2StagePrepareBuildCount`, `Task44RevalidateCount`) jump; barrel jitters if anim joint not stepped.

Recommend A. Barrel tiny, live matrix + two-level AnimJoint + capture pose all fit hierarchy executor with actor slot; keeps static stage slab frozen and steady. B dirties hot steady path for one rotating actor.

Coverage of other six:
- Same path A covers: Yoster clouds (DObjDesc + MatAnim, census `gryoster.c:199-254`), Zebes acid (DObjDesc + Anim/MatAnim + MObj, `grzebes.c:71-219`), Inishie scale beam + 2 platforms (multi-GObj, `grinishie.c:345-387`, port `battleship_grinishie_scale.c:908-946`), Sector Arwings + lasers (multiple DObjDesc + AnimJoints + WPDesc weapons, `grsector.c:23-43,161-207,1087+`), Yamabuki gate (door AnimJoints Open/Close, `gryamabuki.c:246-264`). All have DObjDesc models + joints + live matrices; differ only in binding counts / multi-GObj fan-out / MObj / weapon children — generator handles as variants, runtime same actor-slot executor.
- Differs: Hyrule tornado — `NULL` DObj (`grhyrule.c:74` per census) + particles only → sprite column, not owner packet. Castle bumper / Yamabuki monster / Inishie Pakkun/PowerBlock / Bonus3 bumpers/bombs — item GObjs → keep legacy item path (`IsItemDisplay` + `SubmitItemDObjTree`), not stage-actor packet. Decorative FX (Lakitu carrier, Ridley/ship BG, leaves/dust) → sprite/effect path.
