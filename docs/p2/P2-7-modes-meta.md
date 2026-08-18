# P2-7 — Modes & Meta (training, unlocks, records, options, save, polish)

Everything that makes it the *whole* game rather than its modes. Mostly
independent slices; several can start earlier opportunistically (save data as
soon as records exist to save).

## Work breakdown

1. **Save data.** DLDI/FAT save file next to the ROM (homebrew reality; no
   retail backup chip). Versioned format: unlocks, VS records, 1P high
   scores, bonus-stage times, options. Corruption-safe write (write-new,
   rename); works on retail flashcart + melonDS.
2. **Unlock system.** Original conditions + challenger-approaching 1-stock
   fights: Luigi (clear Bonus 1 with all 8 starters), Ness (1P Normal, 3
   stock, no continues), Falcon (1P under 20 min), Jigglypuff (clear 1P),
   Mushroom Kingdom (1P clear with all 8 starters + VS condition), Item
   Switch (100 VS matches), Sound Test (all Bonus 1+2 boards cleared).
   **Verify every condition against source at implementation** — the list
   above is from memory and marked accordingly. Dev builds keep everything
   unlocked via flag.
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
