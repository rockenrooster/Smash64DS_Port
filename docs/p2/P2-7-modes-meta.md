# P2-7 — Modes & Meta (training, unlocks, records, options, save, polish)

Everything that makes it the *whole* game rather than its modes. Mostly
independent slices; several can start earlier opportunistically (save data as
soon as records exist to save).

## Work breakdown

1. **Save data.** DLDI/FAT save file next to the ROM (homebrew reality; no
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
