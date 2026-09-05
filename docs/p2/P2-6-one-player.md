# P2-6 — 1P Game (full campaign)

The original 1P mode, Mario-first, then every character. Depends on the full
roster (opponents), P2-2 (ally battles are 4-fighter), stages, and items.

## Status 2026-09-05 (code-first, unbuilt)

Every campaign scene is in source behind `NDS_P2_1P_GAME`, by whole-TU import
of the decomp scene: the driver (`sc1pmanager`), the fight itself booted from
the bridge in `battleship_sc1pgame_runtime.c` (the N64 title-signature check
is compiled out by the ninth import-overlay patch), the tally, Bonus 1/2
(`sc1pbonusstage`) and Race (`grbonus3`), intro, challenger, continue, the
1P menus, congratulations, ending, credits (DS glyph seam), Master Hand
(`ftboss` + four status TUs, one shared port header, `sc1pgameboss`) and
the five 1P arenas as native packets. The bridge admits every venue,
Giant DK, Metal Mario (native owner export `dcbcf495405`) and the Polygon
team (admitted `battleship_ftn_polygons.c`; their twelve owner exports are in
flight); Master Hand is admitted too (`9e7a622cd18`: three reloc files staged,
manifest Boss entry; owner export in flight); it still refuses by counter
Master Hand until that export lands,
(the bonus boards and Race are registered blob stages the bridge admits,
`e2e5c8bef5b`). Every 1P reloc symbol is rowed
(`check_reloc_symbol_census.py --flag NDS_P2_1P_GAME` reads 0; the collision
table in `reloc_backend_compat_shims.c` is the verbatim 41-row source table).
Open and owned: the seven mechanical stage actors' render path
(`docs/p2/stages/stage-actor-census-2026-09-05.md`, Jungle barrel builder in
flight) and every runtime check in `docs/VERIFYING.md` 4c; the stage packets
load from NitroFS since `626f30b0c83`.

## Campaign flow (mechanical equivalence to `mn/mn1pmode` + `gm/` 1P logic)

Stage sequence, **read from `dSC1PGameStageDesc`** (`sc/sc1pmode/sc1pgame.c:295-566`),
not from memory: Link (Hyrule) → Yoshi Team ×18 (**YosterSmall**, `:314`) →
Fox (Sector Z) → **Bonus 1: Break the Targets** → Mario + Luigi ×2 with one
ally (**Castle**, `:360`) → Pikachu (Yamabuki) → Giant DK (Jungle, +2 allies)
→ **Bonus 2: Board the Platforms** → Kirby Team ×8 (Pupupu, 2 simultaneous,
`sc1pgame.h:16-17`) → Samus (Zebes) → Metal Mario (Metal) → **Bonus 3: Race to
the Finish** → Fighting Polygon Team ×30 (Zako, `sc1pgame.h:9`) → Master Hand
(Last). Then the challenger fights: Luigi (Castle), Ness (Pupupu), Jigglypuff
(Yamabuki), Captain Falcon (Zebes) — `sc1pgame.c:507-565`.

Two entries in the earlier from-memory list were **wrong** and are corrected
above: the two-on-one Mario Bros. fight is on Peach's Castle, not Mushroom
Kingdom, and the Yoshi team fight is on the *small* Yoshi's Island variant.
Item toggles are `0xFFFFFFFF` on every entry. Enemy stock counts are still
unverified. The driver is `sc/sc1pmode/sc1pmanager.c` (`spgame_stage` loop at
`:318,473,506`); difficulty and stock come from the 1P character select
(`mn/mnplayers/mnplayers1pgame.c:3266-3268`), and a continue halves the score
(`mn/mn1pmode/mn1pcontinue.c:1075`).

There are **58** bonuses, not a short list: `nSC1PGameBonusCheapShot` through
`nSC1PGameBonusTrueFriend` (`sc/scdef.h:319-379`), with their scoring table in
`sc/sc1pmode/sc1pstageclear.c:36-410`.

## Work breakdown

1. **Campaign driver**: difficulty (Very Easy–Very Hard) + stock selection,
   stage sequencing, inter-stage results (score tally), continue flow
   (halved score), Game Over, character-complete congratulations + ending,
   credits ("A Brief History of…" staff-roll shooting minigame — part of the
   game's identity, keep it).
2. **Scoring/bonus system**: full original bonus list (No Damage Clear,
   Speedster, Pacifist, etc.) from source tables; high-score persistence
   (save stub until P2-7).
3. **Variants** (`fighters/variants.md`): Metal Mario (`ftmmario` — Mario
   moveset, metal material/SFX, stat overrides), Giant DK (`ftgdonkey`),
   Fighting Polygons (`ftn*` ×12 — low-poly models are a gift to the DS;
   reduced movesets per source).
4. **Master Hand** (`fighters/master-hand.md`): HP boss, scripted attack
   patterns, no hitstun/knockback physics, `ftboss` reference.
5. **1P-only venues**: `stages/final-destination.md`, `stages/meta-crystal.md`,
   `stages/duel-zone.md`, `stages/race-to-the-finish.md` through the P2-4
   pipeline.
6. **Bonus stages** (`stages/bonus-stages.md`): Break the Targets + Board the
   Platforms. Logic is shared (`gr/grbonus/`); boards are per-fighter data.
   Mario's two boards land first (campaign needs them); the other 22 ride as
   their fighters' campaigns come online and back the standalone Bonus
   Practice mode (P2-7 menu entry).
7. **Team/ally battles**: 2v1 and 1+allies fights on the P2-2 engine; ally
   CPU behavior per source.
8. **Every-character campaigns**: after Mario's is accepted, remaining 11 are
   data (opponent = player substitutions, endings, bonus boards) — batch rows.

## Risks

- Master Hand is a bespoke actor (own moveset interpreter/scripts) — budget
  it like a new fighter, not a stage prop.
- Sequential-team fights (18 Yoshis) stress spawn/despawn paths — reuse
  respawn machinery, watch heap watermarks across waves.
- Race to the Finish is the only scrolling course in the game — camera and
  KO-boundary semantics differ; keep it stage-owned code.

## Exit criteria

- [ ] Mario campaign start-to-credits on every difficulty, mechanically
      equivalent (owner plays it through).
- [ ] All variants + Master Hand per unit DoD.
- [ ] Bonus 1/2/3 + score/bonus tally + continues + endings + credits.
- [ ] All-character campaigns landed (batch rows).
- [ ] Every new screen holds its cadence; stress config unaffected (1P fights
      are ≤ the 4-CPU stress envelope) — spot-verify Polygon ×3 + items.

## Source pin sheet (delegated probe, 2026-09-03)

Gathered by a read-only agent sweep of the decomp source and of `src/`,
reported at high confidence with a citation on every line. It has **not**
been re-verified line by line here: treat each row as a pointer to check
rather than as settled fact, and verify anything load-bearing against the
cited file before building on it. Rows marked UNVERIFIED are the probe
saying the source did not answer.

```
SCOPE: docs/p2/P2-6-one-player.md:8 says campaign flow = `mn/mn1pmode` + `gm/` 1P logic, Mario-first then every character.
TABLE: `decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pgame.c:295` = `SC1PGameStage dSC1PGameStageDesc[]`.
ORDER: sc1pgame.c:297 Link on `nGRKindHyrule` | PORT: absent (no dSC1PGameStageDesc in src/).
ORDER: sc1pgame.c:312-325 Yoshi Team x18 (`SC1PGAME_STAGE_YOSHI_TEAM_COUNT`) on `nGRKindYosterSmall` (`0x80` flash) | PORT: absent.
ORDER: sc1pgame.c:327 Fox on `nGRKindSector` | PORT: absent.
ORDER: sc1pgame.c:342 Bonus1 Break the Targets (placeholder gkind `nGRKindCastle`) | PORT: stub only (see bonus rows).
ORDER: sc1pgame.c:357 Mario+Luigi x2 +1 ally on `nGRKindCastle`, trait `nFTComputerTraitMarioBros`/`Ally` | PORT: absent.
ORDER: sc1pgame.c:372 Pikachu on `nGRKindYamabuki` | PORT: absent.
ORDER: sc1pgame.c:387 Giant DK (`nFTKindGDonkey`) on `nGRKindJungle` +2 allies | PORT: absent.
ORDER: sc1pgame.c:402 Bonus2 Board the Platforms (placeholder `nGRKindCastle`) | PORT: stub only.
ORDER: sc1pgame.c:417 Kirby Team x8 (`SC1PGAME_STAGE_KIRBY_TEAM_COUNT`), 2-at-once (`sc1pgame.h:17`) on `nGRKindPupupu` | PORT: absent.
ORDER: sc1pgame.c:432 Samus on `nGRKindZebes` | PORT: absent.
ORDER: sc1pgame.c:447 Metal Mario (`nFTKindMMario`) on `nGRKindMetal` | PORT: absent.
ORDER: sc1pgame.c:462 Bonus3 Race to Finish on `nGRKindBonus3`, 3 enemies, trait `nFTComputerTraitBonus3` | PORT: absent (only `grBonus3MakeGround` stub in src/import/battleship_grpupupu_ground.c:580).
ORDER: sc1pgame.c:477 Polygon Team x30 (`SC1PGAME_STAGE_MAX_TEAM_COUNT=30`, sc1pgame.h:9) on `nGRKindZako` | PORT: absent.
ORDER: sc1pgame.c:492 Final: `nFTKindBoss` on `nGRKindLast` | PORT: absent (only `nGRKindLast` harness default in src/port/scene_harness.c:78-79).
CHALLENGERS: sc1pgame.c:507-565 Luigi/Castle, Ness/Pupupu, Purin/Yamabuki, Captain/Zebes | PORT: absent.
CHALLENGER-TABLE: `decomp/.../sc/sc1pmode/sc1pmanager.c:59` = `dSC1PManagerChallangerFighterKinds {Luigi,Ness,Purin,Captain}` | PORT: absent.
STAGE-ENUM: `decomp/.../src/sc/scdef.h:291-316` = `SC1PGameStageKind` Link..Boss + ChallengerStart/End | PORT: absent.
ITEMS: sc1pgame.c:301,316,331,... every entry `0xFFFFFFFF` item_toggles | PORT: absent.
ENEMY-STOCK-COUNTS: UNVERIFIED (P2-6 doc:25 says still unverified; table holds opponent_count/ally_count only).
DRIVER: `decomp/.../sc/sc1pmode/sc1pmanager.c:318` = `while (spgame_stage <= nSC1PGameStageCommonEnd)` loop; intro:360-366, bonus branch:368-379, game:381-395, continue:403-443, clear:444-473 | PORT: stub `NDS_SCENE_STUB(sc1PManagerUpdateScene)` in src/port/title_backend.c:442.
ALLY-PICK-Mario: sc1pmanager.c:326-338 random ally ex-self ex-bit0, Luigi costume fix | PORT: absent.
ALLY-PICK-DK: sc1pmanager.c:340-350 two distinct random allies | PORT: absent.
KIRBY-FINAL-COPY: sc1pmanager.c:352-358 `sc1PManagerGetShuffledKirbyCopy` + `dSC1PManagerKirbyTeamModelPartIDs` | PORT: absent.
BOSS-UNIT: `decomp/.../src/ft/ftchar/ftboss/ftboss.c` + `ftboss.h:13-18` status/motion tables + ~40 status files (ftbossappear.c..ftbossyubideppou3.c) | PORT: absent (no ftboss in src/).
BOSS-DIFF: `ftboss.h:6` `FTBOSS_ATTACK_WAIT_MAX 120`; `:7` wait scaled by CPU level (`/100`); `:651-663` ftmanager.c boss struct init + next-attack wait | PORT: absent.
BOSS-DIFF2: HP value / no-hitstun-no-knockback exact lines UNVERIFIED (only `ftcommondead.c:538 if fkind==Boss`, `ftcomputer.c:7806/7837 if fp->fkind != Boss`, `ftmain.c:3708/3884` Boss arms seen) | PORT: partial guard `fkind != nFTKindBoss` in src/port/reloc_backend_ftmain_runtime.c:1709,1951.
METAL-UNIT: `decomp/.../src/ft/ftchar/ftmmario/ftmmario.c:1-25` (data pointers only; Mario-moveset reuse = behaviour file UNVERIFIED) | PORT: case arms only src/port/reloc_backend_compat_shims.c:7203-7204.
METAL-DIFF: `decomp/.../src/ft/ftmanager.c:577-578` `case nFTKindMMario: knockback_resist_passive=30.0F` | PORT: absent (metal material/SFX/stat table UNVERIFIED).
GIANT-UNIT: `decomp/.../src/ft/ftchar/ftgdonkey/ftgdonkey.c:1-16` (data pointers only) | PORT: case arms src/port/reloc_backend_compat_shims.c:1998,3786,10396.
GIANT-DIFF: ftmanager.c:587-588 `case nFTKindGDonkey: knockback_resist_passive=48.0F`; DK-only throw/weight arms (ftcommondamage.c:436,818; ftcommonthrow.c:37) | PORT: absent (scale/table UNVERIFIED).
POLYGON-UNITS: `decomp/.../src/ft/ftchar/ftn{mario,luigi,donkey,link,samus,yoshi,kirby,fox,pikachu,purin,captain,ness,ness?}/ftn*.c` = 12 low-poly variants, data-pointer-only .c files (e.g. ftnmario.c:4-13) | PORT: case arm `nFTKindNMario` src/port/reloc_backend_compat_shims.c:7204 only.
POLYGON-DIFF: reduced movesets per source UNVERIFIED (setup path sc1pgame.c:1160-1201 `+nFTKindNStart` variations, trait `nFTComputerTraitPolyTeam` sc1pgame.c:487) | PORT: absent.
YOSHI-SPAWN: sc1pgame.c:1113-1158 6 variations cycled over 18 (`SC1PGAME_STAGE_YOSHI_VARIATIONS_COUNT` sc1pgame.h:12-13) | PORT: absent.
KIRBY-COPY-TEAM: sc1pgame.c:1203-1227 copy kinds `dSC1PGameKirbyTeamCopyKinds` sc1pgame.c:27-36 | PORT: absent.
DIFFICULTY-IDS: `decomp/.../src/sc/scdef.h:280-289` VeryEasy/Easy/Normal/Hard/VeryHard + EnumCount | PORT: absent.
DIFFICULTY-EFFECT: `decomp/.../src/sc/sc1pmode/sc1pgame.c:45-292` `dSC1PGameComputerDesc[]` per-stage enemy/ally level+handicap x5 dificultades | PORT: absent.
DIFFICULTY-SELECT: `decomp/.../src/mn/mnplayers/mnplayers1pgame.c:3266` `spgame_difficulty=sMNPlayers1PGameLevelValue`; `:3268` `spgame_stock_count=sMNPlayers1PGameStockValue` | PORT: stub `mnPlayers1PGameStartScene` src/port/title_backend.c:426.
CONTINUE-HALVE: `decomp/.../src/mn/mn1pmode/mn1pcontinue.c:1075` `gSCManagerSceneData.spgame_score *= 0.5F` | PORT: stub `mnPlayers1PGameContinueStartScene` src/port/title_backend.c:425.
CONTINUE-FLOW: sc1pmanager.c:403-443 lose->continue overlay; Yes:412-429 restock+`spgame_stage--`+level-guard/drop (cap 9); No:431-442 `sc1PManagerTrySaveBackup(FALSE)`->Startup/OpeningRoom | PORT: absent beyond stub.
CONTINUE-LEVELDROP: sc1pmanager.c:82-86 `gSC1PManagerLevelDrop`/`sSC1PManagerLevelGuard`; applied sc1pgame.c:950-957 `level -= drop, floor 1` | PORT: absent.
GAMEOVER: mn1pcontinue.c:1040-1056 No-path makes room-fade+`MakeGameOverText/GameOver`, BGM `nSYAudioBGM1PGameOver`+voice `AnnounceGameOver` | PORT: absent.
BONUS-COUNT: scdef.h:319-379 `nSC1PGameBonusCheapShot..TrueFriend` +EnumCount = 58 bonuses | PORT: absent.
BONUS-SCORES: `decomp/.../src/sc/sc1pmode/sc1pstageclear.c:36-415` `dSC1PStageClearBonusData[]` (e.g. :39 CheapShot -99; :373 YoshiRainbow 50000 US; :400 DKPerfect 50000) | PORT: absent.
BONUS-TRACK: sc1pgame.h:22-64 bonus stat globals (Tomato/Heart/Star/ShieldBreaker/GiantImpact/Mew/BrosCalamity/attack/defend arrays); reset sc1pgame.c:1278-1310 | PORT: partial shims `gSC1PGameBonusStarCount/GiantImpact/ShieldBreaker` + `ftParamUpdate1PGame{Attack,Damage}Stats` src/port/reloc_backend_compat_shims.c:394-395,4471,9537.
BONUS1-LOGIC: `decomp/.../src/sc/sc1pmode/sc1pbonusstage.c:19` `dSC1PBonusStageTargetDescs[]` per-fighter target boards (Bonus1Mario first) + shared `gr/grbonus/` | PORT: partial `sc1PBonusStageInitBonus2/MakeBonus1Ground` imported in src/import/battleship_grpupupu_ground.c:55-56,582-587.
BONUS-TASK-MAX: scdef.h:5 `SCBATTLE_BONUSGAME_TASK_MAX 10` (all-targets/platforms for SoundTest unlock, sc1pmanager.c:196-219) | PORT: absent.
BONUS3-RACE: `decomp/.../src/gr/grbonus/grbonus3.c` (+grbonus3.h) scrolling course; only scrolling course per P2-6 doc:67 UNVERIFIED in source read | PORT: stub `grBonus3MakeGround` src/import/battleship_grpupupu_ground.c:580.
ENDING: `decomp/.../src/mv/mvending/mvending.c:551` `mvEndingStartScene()`; called sc1pmanager.c:477-483 after `sc1PManagerTrySaveBackup(TRUE)` (:475) | PORT: stub `mvEndingStartScene` src/port/title_backend.c:437.
CREDITS: `decomp/.../src/sc/sccommon/scstaffroll.c:2311` `scStaffrollStartScene()`; called sc1pmanager.c:485-490 | PORT: stub `scStaffrollStartScene` src/port/title_backend.c:447.
CONGRA: `decomp/.../src/mn/mncommon/mncongra.c:402` `mnCongraStartScene()` (US-only guard sc1pmanager.c:492-499) | PORT: absent (no congra stub found).
UNLOCK-FLOW: sc1pmanager.c:158-186 Ness/Captain/Purin challenger rules; :506-554 challenger battle+message/level-drop; :556-580 Mushroom-Kingdom/Inishie check | PORT: absent.
VENUE-YosterSmall: `decomp/.../src/gr/grdef.h:28` Small Yoshi's Island (1P Game) | PORT: absent.
VENUE-MetaCrystal: grdef.h:29 Meta Crystal | PORT: absent.
VENUE-DuelZone: grdef.h:30 Duel Zone | PORT: absent.
VENUE-Race: grdef.h:31 Race to the Finish | PORT: absent.
VENUE-FinalDest: grdef.h:32 Final Destination | PORT: absent (harness-only `nGRKindLast` default).
PORT-SUMMARY: src/ has only stubs/shims (title_backend.c:425-442; battle_playable_compat_stubs.c:98-145; compat_shims bonus/case arms); no 1P driver, stage table, scoring tally, continue, venues, boss AI, or bonus boards.
```

## Port delta inventory (2026-09-04)

What already exists versus what has to be written. The pin sheet above locates
the source; this locates the port, and the two headline facts are that **the
fighter data layer is already compiled in** and **no campaign behaviour is**.

- **Every 1P fighter's data slot is real, not a stub.**
  `src/import/battleship_ftchar_data_slots.c` textually includes
  `ftboss/ftboss.c` (:7), `ftgdonkey/ftgdonkey.c` (:11),
  `ftmmario/ftmmario.c` (:16) and all twelve `ftn*/ftn*.c` polygon files
  (:17-29), and `include/reloc_data_ftdata_symbols.h` carries their file IDs --
  `llBossMain*` (:7-9), ~30 `llFTBossAnim*FileID` (:21-51), `llMMarioMain`
  (:1817-1819), `llGDonkeyMain` (:1800), ten `llFTN*AnimPose1PFileID`
  (:1077-1086). So Master Hand, Metal Mario, Giant DK and the Fighting Polygon
  Team already have their data pointers and animation ids in the build. What
  is missing is behaviour: no `ftboss*.c` status file is imported, and the only
  port-side mentions of these kinds are guards and `case` arms.
- **The campaign driver is absent entirely.** `sc1pgame.c` (2,920 lines),
  `sc1pmanager.c` (587), `sc1pgameboss.c` (1,024), `sc1pintro.c` (2,044) and
  `sc1pchallenger.c` (381) have no port counterpart. `src/port/title_backend.c`
  carries `sc1PManagerUpdateScene` (:442), `sc1PIntroStartScene` (:441),
  `sc1PChallengerStartScene` (:440), `sc1PBonusStageStartScene` (:439),
  `sc1PStageClearStartScene` (:443), `mvEndingStartScene` (:437),
  `scStaffrollStartScene` (:447) and `mnCongraStartScene` (:415) as
  `NDS_SCENE_STUB`, whose whole body is `ndsSceneBoundary()`.
- **Four 1P hooks are weak stubs waiting for a real body**, which is the
  cheapest kind of work in this phase because the call sites already exist:
  `sc1PGameSetPlayerDefeatStats` and `sc1PGameSpawnEnemyTeamNext`
  (`src/port/battle_playable_compat_stubs.c:98,104`), plus
  `sc1PGameBossInitWallpaper` (:141) and `sc1PGameSetCameraZoom` (:145).
  Replace the weak definition; do not add a second one.
- **Scoring is partly wired already.** `gSC1PGameBonusStarCount` and
  `...GiantImpact` are real (`reloc_backend_compat_shims.c:394-395`), and
  `gSC1PGameBonusTomato/HeartCount` are incremented on the 1P path
  (`battleship_ftcommon_get.c:40-41,155,165`), as is
  `gSC1PGameBonusMewCatcher` (`battleship_item_map_core.c:167,253`).
  `gSC1PGameBonusShieldBreaker` is weak
  (`battleship_ftcommon_shieldbreakfly.c:26`). The 58-entry
  `dSC1PStageClearBonusData` table (`sc1pstageclear.c:36-415`) and the tally
  screen are what is missing.
- **The 1P venues are enum-only.** `include/sc/scene.h:179-184` declares
  `nGRKindYosterSmall/Metal/Zako/Bonus3/Last`, but no ground exists:
  `grBonus3MakeGround` returns `ndsGRNonPupupuGroundStub()`
  (`battleship_grpupupu_ground.c:709`), and `sc1PBonusStageInitBonus2` (:711)
  and `sc1PBonusStageMakeBonus1Ground` (:716) only bump a stub counter. Their
  map payloads are not in the reloc manifests either, so these need the P2-4
  stage-production path, not just code.
- The bonus-stage Target item is the one piece already written and merely
  unlinked: `battleship_item_target.c` calls `sc1PBonusStageUpdateTargetCount`
  (:80) and names `gSC1PBonusStageItemFile`, which is why it is held out of
  `CFILES` until this phase lands.

**Build order, by how much each step unblocks.** 1. The ladder tables and
driver skeleton (`dSC1PGameStageDesc`/`dSC1PGameComputerDesc`,
`sc1pgame.c:27-292`, plus the `sc1PManagerUpdateScene` loop at
`sc1pmanager.c:253-318`) onto `ndsSceneManagerRequest` and
`ndsMatchConfigApply` -- every other item is a caller of this. 2. Scoring and
the stage-clear tally, since its counters are already half-real. 3. The team
spawn hooks, replacing the two weak stubs. 4. Variant stats only -- Metal
resist, Giant scale/resist, polygon traits -- because the slots and animation
manifests already exist. 5. Bonus 1 and 2 boards. 6. The 1P venues, Race last.

## Import order (inventory 2026-09-04, smallest bootable slice first)

**Status 2026-09-05 (code-first, unbuilt):** 1 driver + bridge for every
ladder stage LANDED; 2 tally LANDED; 3 team hooks LANDED through the sc1pgame.c
include; 4 variants: Giant DK and Metal Mario admitted (owner export and reloc
closures in flight), the Polygon team queued; 5 bonus boards LANDED (Mario's
Break the Targets descriptor done, 24 boards in flight); 6 venues: the five
arenas wired, Race in flight; 7 boss IMPORTED (kind-to-table wiring, GC
wallpaper enumerators and the bullet attribute rows still open); 8 menus and
tail LANDED (1P menus, intro, challenger, ending, credits with DS glyphs,
Congratulations, unlock message). The campaign scenes are registered and
routed (`nds_scene_manager.c`, `taskman_seam_harness.c`); the bridge still
has to start its task with `dSC1PGameTaskmanSetup` and mark the fight
`nSCKind1PGame` (sc1pmanager.c never sets it around `sc1PGameStartScene`).

Source sizes: `sc1pgame.c` 2920 lines (included whole by
`src/import/battleship_sc1pgame_runtime.c`, behind `NDS_P2_1P_GAME`),
`sc1pmanager.c` 587, `sc1pgameboss.c` 1024, `sc1pintro.c` 2044,
`sc1pchallenger.c` 381, `sc1pbonusstage.c` 1257, `sc1pstageclear.c` 2276
(imported whole as `battleship_sc1pstageclear.c` on 2026-09-05, after
`scripts/menus/stage_reloc_file.py` staged its three sprite files; that script
stages any sprite reloc file in one command),
`mn1pmode.c` 887, `mn1pcontinue.c` 1290, `mnplayers1pgame.c` 3543,
`mnplayers1pbonus.c` 2931, `grbonus3.c` 106, `ftboss/` ~36 status files,
`wpbossbullet.c` 188, `mvending.c` 560, `scstaffroll.c` 2339, `mncongra.c` 432.
No `gm1p*` exists; 1P reuses the ported `gmcommon/gmcamera/gmcollision`. The
1P-only venues (YosterSmall, Metal, Zako, Last, Race) have no dedicated ground
TU: they are enum arms in `grmainsetup.c:37` / `grwallpaper.c:281-293` over the
VS grounds, so they ride the P2-4 stage path. Dispatch is already wired:
`battleship_scmanager.c:6` `case nSCKind1PGame` reaches the
`sc1PManagerUpdateScene` stub at `title_backend.c:442`.

1. **Driver to the first stage.** `sc1pmanager.c` loop + `sc1pgame.c`
   setup/spawn/team-next + `mnplayers1pgame.c` difficulty/stock onto
   `ndsMatchConfigApply` (`nds_match_config.c:410`). Boots Link/Hyrule
   (`dSC1PGameStageDesc[0]`, sc1pgame.c:298-310) with Mario in the player slot;
   every later step calls this.
2. **Tally.** `sc1pstageclear.c` data + screen (stage the reloc rows), and
   wire the counters that already exist: `gSC1PGameBonusStarCount`/GiantImpact
   (`compat_shims.c:394-395`), Tomato/Heart (`battleship_ftcommon_get.c:40-41`),
   MewCatcher (`battleship_item_map_core.c:167,253`), the weak ShieldBreaker
   (`battleship_ftcommon_shieldbreakfly.c:26`).
3. **Team hooks.** Replace the weak stubs `sc1PGameSetPlayerDefeatStats`,
   `SpawnEnemyTeamNext`, `BossInitWallpaper`, `SetCameraZoom`
   (`battle_playable_compat_stubs.c:98-145`): unlocks Yoshi x18, Kirby x8,
   Polygon x30 waves, the Mario Bros ally and Giant DK's allies.
4. **Variant stats** (slots and manifests exist): Metal resist 30.0
   (`ftmanager.c:577-578`), Giant scale/resist 48.0 (`:587-588`), polygon
   traits (`sc1pgame.c:487`). No models yet.
5. **Bonus 1/2.** `sc1pbonusstage.c` + `sc1pbonusstagefiles.c`; link the
   Target item's provider (`battleship_item_target.c:80`); Mario boards first.
6. **1P venues** through the P2-4 descriptor path: YosterSmall, Metal, Zako,
   Last; Race (`grbonus3.c`) last, it is the only scrolling course.
7. **Boss.** `ftboss/` statuses + `wpbossbullet.c` + `sc1pgameboss.c`; needs
   the Last venue from 6.
8. **Menus and tail.** `mn1pmode.c`, `mn1pcontinue.c` (score halves at
   `:1075`), `sc1pintro.c`, `sc1pchallenger.c`, `mvending.c`,
   `scstaffroll.c`, `mncongra.c`, `mnplayers1pbonus.c`. Closes Mario
   start-to-credits.

### Battle display ownership — 2026-09-05

`syTaskmanStartTask` now reclaims the DS 3D layer for every registered BATTLE
scene, including Bonus Practice and Training, independently of arena rebudgeting.
The existing platform helper waits for that scene's first presented frame before
showing BG0. `test_taskman_battle_display.py` executes the actual wrapper on the
host and covers registered/unregistered scenes, initial/repeated entry and both
arena states; removing the enable call fails. ROM visibility is still unverified.
