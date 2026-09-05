# P2-7 — Modes & Meta (training, unlocks, records, options, save, polish)

Everything that makes it the *whole* game rather than its modes. Mostly
independent slices; several can start earlier opportunistically (save data as
soon as records exist to save).

## Status 2026-09-05 (code-first, unbuilt)

Every row of the pin sheet below that reads NOT PRESENT or STUB is now in
source behind `NDS_P2_1P_GAME`, by whole-TU import of the decomp scene, unless
named here: save data (item 1, `nds_backup.c` + `lbbackup.c`), the unlock
message and the save-driven masks (items 2 and 9 -- the open cartridge is a
harness gate, published builds boot the save), Training (item 3), the DATA
menus (item 4), Options / Backup Clear / Sound Test with the mixer's
mono-stereo switch and BGM fade (item 5), the attract demo and How to Play
wired from the title idle timer (items 6-8), and the shell bridge that routes
Mode Select's 1P GAME / OPTION / DATA rows to registered source scenes. The
CSS shows the save's own locked cells (`2f4956653e8`). Open: the intro
cinematic (deferred by owner), and every runtime check in
`docs/VERIFYING.md` items 4b-4e.

## Work breakdown

1. **Save data.** LANDED 2026-09-04, unbuilt: `src/nds/nds_backup.c` +
   `src/import/battleship_lbbackup.c` (see that file's header). Original plan:
   DLDI/FAT save file next to the ROM (homebrew reality; no
   retail backup chip). Versioned format: unlocks, VS records, 1P high
   scores, bonus-stage times, options. Corruption-safe write (write-new,
   rename); works on retail flashcart + melonDS.
2. **Unlock system.** Conditions below are **read from source**, not
   remembered; the earlier from-memory list had three of seven wrong. Bits
   are `lb/lbdef.h:134-140`, applied in `mn/mncommon/mnmessage.c:284-301`
   which sets `unlock_mask`, sets `fighter_mask` for a newcomer, and calls
   `lbBackupWrite()`.
   - **Luigi** — clear Bonus 1 with all ten tasks for every starter
     (`bonus1_task_count == 10` across `LBBACKUP_CHARACTER_MASK_STARTER`),
     `sc/sc1pmode/sc1pbonusstage.c:1215-1224`.
   - **Ness** — 1P at Normal or above, **zero continues**, and a *stock
     setting below 3*, i.e. the menu's 1 or 2, not "3 stock"
     (`sc1pmanager.c:162-165`). The plan previously said 3 stock.
   - **Captain Falcon** — US build wants the 1P run under **12 minutes**,
     `gSC1PManagerTotalTimeTics < I_MIN_TO_TICS(12)`; the source comment says
     "12 minutes instead of reported 20", and 20 is the JP figure
     (`sc1pmanager.c:171-177`). The plan previously said 20.
   - **Jigglypuff** — any 1P clear; it is the unconditional fallback after the
     Ness and Falcon checks (`sc1pmanager.c:182-185`).
   - **Mushroom Kingdom** — two paths, both requiring `ground_mask == ALL`
     *and* `is_spgame_complete` for all eight starters: the 1P path at
     `sc1pmanager.c:556-569` and the VS path at `mnvsresults.c:3286-3300`.
   - **Item Switch** — `vs_itemswitch_battles >= 100` (`mnvsresults.c:3281`).
   - **Sound Test** — both bonus stages cleared 10/10 by **all twelve**
     fighters, not the starters (`sc1pmanager.c:189-219`).
   The challenger fight is one stock against a CPU whose level is softened by
   `challenger_level_drop`; a loss raises that by 2 up to 9, and a lost Luigi
   returns to the Bonus 1 player select (`sc1pmanager.c:506-554`,
   `sc1pgame.c:1102`). Dev builds keep everything unlocked via flag.
3. **Training mode.** CPU stance control, item spawning, speed/camera per
   original, combo/damage readouts (`mn/` + training logic in source).
4. **Records/Data screens.** VS records table, 1P bests, bonus times,
   character use stats — whatever the original tracks, backed by the save.
5. **Options.** Sound (stereo/mono, music/SFX volume, Sound Test), Backup
   Clear. Screen Adjust is N64-specific — drop (record as intentional delta).
6. **Attract flow.** Title-idle demo battles (CPU vs CPU with the replay
   determinism discipline), How to Play screen.
7. **Intro cinematic** (deferred here by owner decision): recreate the
   opening in-engine within visual doctrine (timeboxed approximation,
   skippable). Master Hand desk scene + character vignettes.
8. **DS platform polish.** Lid-close sleep, soft-reset safety, low-battery
   save safety, clean boot on retail flashcart, icon/banner metadata
   (original branding — owner ruling 2026-08-18).
9. **Menu completion.** 1P GAME / OPTIONS / DATA entries go live; Bonus
   Practice (BTT/BTP select) menu; CSS/SSS unlock-gating flips from dev-open
   to save-driven.

## Risks

- Unlock conditions and records are exactness-sensitive (players know them);
  they are cheap data but need source verification rows.
- Save write on DLDI during gameplay = card I/O on hot frames — write only on
  scene boundaries (results/menu), never mid-match.
- Attract demos must not desync — they reuse the replay verifier machinery.

## Exit criteria

- [ ] Fresh-cart experience equals the original: everything locked, unlocks
      earn correctly, challengers approach, records persist across power
      cycles on retail hardware.
- [ ] Training + Data + Options + Sound Test complete and cadence-clean.
- [ ] Intro + attract loop shipped; owner visual pass.
- [ ] Full-game soak: scripted long session (menus, 1P run, VS matches,
      training) with flat heap watermarks and zero exceptions.
- [ ] P2 close: stress gate green on final content (`PROJECT_GOAL.md` gate),
      `smash64ds.nds` published, owner retail play-through accepted.

## Source pin sheet (delegated probe, 2026-09-03)

Gathered by a read-only agent sweep of the decomp source and of `src/`,
reported at high confidence with a citation on every line. It has **not**
been re-verified line by line here: treat each row as a pointer to check
rather than as settled fact, and verify anything load-bearing against the
cited file before building on it. Rows marked UNVERIFIED are the probe
saying the source did not answer.

```
TRAINING ?º?«?¬?ä?º?ü VS:
- TRAIN menu 6 rows CP/Item/Speed/View/Reset/Exit | decomp/src/sc/sc1pmode/sc1ptrainingmode.c:52-60 | PORT: NOT PRESENT (stubs only)
- TRAIN main enum CP/Item/Speed/View/Reset/Exit | decomp/src/sc/scdef.h:401-415 | PORT: NOT PRESENT
- TRAIN CP opts Stand/Walk/Evade/Jump/Attack | decomp/src/sc/scdef.h:421-427 | PORT: NOT PRESENT
- TRAIN dummy maps to nFTComputerBehavior Stand/Walk/Evade/Jump/Default | decomp/src/sc/sc1pmode/sc1ptrainingmode.c:63-70 + src/ft/ftdef.h:1251-1255 | PORT: NOT PRESENT
- TRAIN battle: game_type Training, time INFINITE, show_score FALSE, items 0 | decomp/src/sc/sc1pmode/sc1ptrainingmode.c:587-591 | PORT: NOT PRESENT
- TRAIN slots: 1 MAN +1 COM level 3, pl_count 1 cp_count 1 | decomp/src/sc/sc1pmode/sc1ptrainingmode.c:593-615 | PORT: NOT PRESENT
- TRAIN select writes training_man/com fkind+costume | decomp/src/mn/mnplayers/mnplayers1ptraining.c:2913-2917 | PORT: STUB src/port/title_backend.c:427 NDS_SCENE_STUB
- TRAIN stage via maps_training_gkind | decomp/src/mn/mnmaps/mnmaps.c:1397 | PORT: ABSENT per src/nds/nds_menu_shell_sss.c:56-59
- TRAIN item spawn max 4, vel.y 30, y+200, wait 8, A-button | decomp/src/sc/sc1pmode/sc1ptrainingmode.c:393-406 + src/sc/scdef.h:97-100 | PORT: NOT PRESENT
- TRAIN speed Full/2Thirds/Half/Quarter | decomp/src/sc/scdef.h:457-462 + sc1ptrainingmode.c:416-430 | PORT: NOT PRESENT
- TRAIN view Normal/CloseUp, magnify_wait 180, player zoom | decomp/src/sc/scdef.h:468-471 + sc1ptrainingmode.c:433-457 + scdef.h:92 | PORT: NOT PRESENT
- TRAIN damage 3-digit + combo 2-digit displays | decomp/src/sc/scdef.h:77-82 + sc1ptrainingmode.c:790,874,1038-1039 | PORT: NOT PRESENT
- TRAIN Reset/Exit via A-button reload scene | decomp/src/sc/sc1pmode/sc1ptrainingmode.c:461-489 | PORT: STUB src/port/title_backend.c:444
- TRAIN scene file only weak LoadWallpaper shim | src/port/battle_playable_compat_stubs.c:137 | PORT: STUB ONLY
UNLOCK:
- unlock enum 7: Luigi/Ness/Captain/Purin/Inishie/SoundTest/ItemSwitch | decomp/src/lb/lbdef.h:188-197 | PORT: NOT PRESENT
- unlock masks + NEWCOMERS/PRIZE groups | decomp/src/lb/lbdef.h:134-154 | PORT: NOT PRESENT
- unlock_mask u8 + fighter_mask u16 live in LBBackupData | decomp/src/lb/lbtypes.h:277-278 | PORT: FORCED src/port/scene_harness.c:55-56
- apply: unlock_mask|=ID, fighter_mask|=fkind for 4 newcomers, lbBackupWrite | decomp/src/mn/mncommon/mnmessage.c:284-301 | PORT: NOT PRESENT
- Luigi: Bonus1 10/10 for every STARTER | decomp/src/sc/sc1pmode/sc1pbonusstage.c:1215-1224 | PORT: NOT PRESENT
- Ness: 1P Normal+, continues 0, stock_count<3 | decomp/src/sc/sc1pmode/sc1pmanager.c:162-165 | PORT: NOT PRESENT
- Falcon: US total<12min, JP total<20min | decomp/src/sc/sc1pmode/sc1pmanager.c:171-177 | PORT: NOT PRESENT
- Purin: unconditional fallback after Ness/Falcon | decomp/src/sc/sc1pmode/sc1pmanager.c:182-185 | PORT: NOT PRESENT
- Inishie 1P path: ground ALL + is_spgame_complete all STARTERs | decomp/src/sc/sc1pmode/sc1pmanager.c:556-569 | PORT: NOT PRESENT
- Inishie VS path: same check at results | decomp/src/mn/mnvsmode/mnvsresults.c:3286-3300 | PORT: NOT PRESENT
- ItemSwitch: vs_itemswitch_battles>=100 | decomp/src/mn/mnvsmode/mnvsresults.c:3281 | PORT: NOT PRESENT
- counters: ground_mask|=gkind + itemswitch_battles++ per VS | decomp/src/mn/mnvsmode/mnvsresults.c:205-209 | PORT: NOT PRESENT
- SoundTest: bonus1+bonus2 10/10 for ALL 12 | decomp/src/sc/sc1pmode/sc1pmanager.c:194-219; fired sc1pbonusstage.c:1235-1249 | PORT: NOT PRESENT
- challenger: 1-stock scene, win->unlock msg, loss level_drop+2 max 9, Luigi loss->Bonus1Players | decomp/src/sc/sc1pmode/sc1pmanager.c:506-554 | PORT: NOT PRESENT
- challenger CPU level minus level_drop | decomp/src/sc/sc1pmode/sc1pgame.c:1102 | PORT: NOT PRESENT
- port forces fully-unlocked cart, no gating | src/nds/nds_menu_shell_css.c:30 + src/port/scene_harness.c:114-117 | PORT: DEV-OPEN ONLY
RECORDS/HISCORE:
- VSRecord per fighter: ko[12]/time/dmg given+taken/SD/games/tallies | decomp/src/lb/lbtypes.h:240-252 | PORT: NOT PRESENT
- 1PRecord: hiscore/continues/bonuses/best_difficulty/bonus times+counts/complete | decomp/src/lb/lbtypes.h:254-265 | PORT: NOT PRESENT
- VS write caps: time 1000min, dmg 999999, SD/KO 9999 | decomp/src/mn/mnvsmode/mnvsresults.c:217-257 | PORT: NOT PRESENT
- 1P hiscore save if score greater + complete flag | decomp/src/sc/sc1pmode/sc1pmanager.c:226-250 | PORT: NOT PRESENT
- bonus best time only if time_passed smaller | decomp/src/sc/sc1pmode/sc1pbonusstage.c:1122-1145 | PORT: NOT PRESENT
- DATA menu 3 opts Characters/VSRecord/SoundTest, SoundTest gated | decomp/src/mn/mndef.h:112-118 + mndata/mndata.c:586-618 | PORT: NOT PRESENT
- VSRecord screen reads vs_records damage/time/KO/rankings | decomp/src/mn/mndata/mnvsrecord.c:1120-1123,1168,1488-1506 | PORT: NOT PRESENT
OPTIONS:
- OPTIONS 3 rows Sound/ScreenAdjust/BackupClear | decomp/src/mn/mndef.h:124-130 | PORT: NOT PRESENT
- OPTIONS write: screenflash + mono/stereo then lbBackupWrite | decomp/src/mn/mnoption/mnoption.c:818-824 | PORT: NOT PRESENT
- SOUND row = mono/stereo toggle only (volumes UNVERIFIED) | decomp/src/mn/mnoption/mnoption.c:423-430 + 808 | PORT: NOT PRESENT
- FLASH toggle is_allow_screenflash | decomp/src/mn/mnoption/mnoption.c:810,820 + lbtypes.h:271 | PORT: NOT PRESENT
- ScreenAdjust writes screen_adjust_h/v | decomp/src/mn/mnoption/mnscreenadjust.c:262-265 | PORT: NOT PRESENT
- BackupClear 6 targets Newcomers/1PHigh/BonusTime/VSRecord/Prize/All | decomp/src/mn/mnoption/mnbackupclear.c:78-99 + lb/lbbackup.c:126-189 | PORT: NOT PRESENT
- SoundTest rows Music/Sound/Voice | decomp/src/mn/mndata/mnsoundtest.c:692 +845-974 | PORT: NOT PRESENT
SAVE:
- LBBackupData: vs+1P records, sound, adjust, masks, ground, battles, error, boot, signature, checksum | decomp/src/lb/lbtypes.h:268-289 | PORT: NO SAVE
- checksum = sum bytes*(i+1) excl checksum | decomp/src/lb/lbbackup.c:13-23 | PORT: NOT PRESENT
- valid iff checksum match + signature==666 | decomp/src/lb/lbbackup.c:26-33 | PORT: NOT PRESENT
- write dual slots ALIGN(size,0x0)+ALIGN(size,0x10) | decomp/src/lb/lbbackup.c:36-41 | PORT: STUB src/port/reloc_backend_compat_shims.c:17728
- read slot0, fallback slot1, else defaults+write | decomp/src/lb/lbbackup.c:44-63 | PORT: STUB src/port/reloc_backend_compat_shims.c:17720
- medium SRAM PI_DOM2 via syDmaRead/WriteSram | decomp/src/sys/dma.c:132-157 | PORT: NO FAT/DLDI (UNVERIFIED for DS target)
- ApplyOptions sets audio quality + video offsets | decomp/src/lb/lbbackup.c:66-74 | PORT: STUB src/port/reloc_backend_compat_shims.c:17724
- CorrectErrors resets locked fighters/stages/items | decomp/src/lb/lbbackup.c:77-123 | PORT: NOT PRESENT
- boot counter + write on title path | decomp/src/mn/mncommon/mntitle.c:1556 + import mirror src/import/battleship_mntitle.c:368-370 | PORT: RAM ONLY
ATTRACT/DEMO/HOWTO:
- title picks 2 demo_fkind shuffled no-repeat via demo_mask_prev | decomp/src/mn/mncommon/mntitle.c:302-335 | PORT: NOT PRESENT
- demo state demo_mask_prev/first/fkind/gkind_order/extend_wait | decomp/src/sc/sctypes.h:384-386,410-411 | PORT: NOT PRESENT
- trigger: idle 650 tics (1190 if extend_wait) -> ProceedDemoNext | decomp/src/mn/mncommon/mntitle.c:712-723 | PORT: NOT PRESENT (5-min idle noted src/nds/nds_menu_shell_mode_vs.c:14)
- demo setup: game_type Demo, stage cycle, all COM lv9, dmg 0-30/40-100 | decomp/src/sc/sccommon/scautodemo.c:546-579 | PORT: NOT PRESENT
- demo fighters: first2 from title pick, rest shuffled unlocked | decomp/src/sc/sccommon/scautodemo.c:511-533 | PORT: NOT PRESENT
- NO input recording; scripted CPUvCPU (recorded/demo format UNVERIFIED) | scautodemo.c:566-571 | PORT: NOT PRESENT
- HowToPlay is scexplain scene + voice announce | decomp/src/sc/sccommon/scexplain.c:794 | PORT: NOT PRESENT (trigger timing UNVERIFIED)
```
