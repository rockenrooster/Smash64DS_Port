# Handoff

Current: 2026-08-18 — **P2-1 (VS SHELL) IS CLOSED PENDING THE OWNER'S VISUAL
PASS. BOUNDARY NOW HAS TWO ARMS.**

## State

- **P2-1 closed by row `P2-1g`.** Every row 1a…1g is green and archived to
  `docs/archive/P2_CLOSED_ROWS.md`; the board keeps only what is next. The
  shell is the game's flow now: boot → splash → title → main menu → VS
  menu/rules → character select → stage select → battle → results → **START on
  Results** → character select → … indefinitely.
- **Boundary is two arms** (`verify-all.ps1 -Profile Boundary -List` is the
  authority; `docs/VERIFYING.md` owns the definition):
  1. `p2_shell_loop` — `scripts/verify-p2-shell-loop.ps1`, twenty full laps of
     the shell. Scene-boundary instrument at `NDS_HARNESS_FAST_LOGIC=1`; **no
     tick figure from it is a cadence figure.**
  2. `battle_playable_realtime`, mode `163` — unchanged, and it stays for all
     of P2 as the P1 regression guard (`docs/P2_PLAN.md` law 4).
- **P1 stays frozen and untouched.** `smash64ds-battle-playable-hwtri.nds`
  SHA-256 `576F51ED…E723`, `smash64ds.nds` `54C07FAC…C68A` — both byte-identical
  across P2-1, and rebuilding the P1 target from the P2-1g tree reproduces its
  hash exactly (the negative control for "mode 163 is bit-identical").
- **Menu cadence is measured, not assumed**: every present on every shell
  screen is a clean single-VBlank 60 Hz present, and the worst frame on any
  menu screen is inside the 560,190-tick budget. The instrument is
  `scripts/menus/probe-p2-shell.ps1` (shipping configuration, fast logic 0);
  the loop verifier is not.

## Next

1. **P2-1h first — the original-presentation pass (owner ruling 2026-08-18:
   this is a port, ALL first-party branding and original presentation assets
   ship; no invented placeholders).** Menu collage via the cheapest
   `docs/p2/P2-1c-vram-map.md` option that holds 60 Hz menus; original title
   presentation (logo art, copyright line); boot straight to title (no
   splash — the intro precedes title at P2-7); DS banner = original branding.
2. **P2-2 — four-fighter engine.** The board carries its rows, seeded from
   `docs/p2/P2-2-four-fighters.md`'s own work breakdown. Take `P2-2a` (fighter
   array generalization) first: everything else in the phase depends on the
   2-fighter assumptions being gone. `P2-2f` (the 4-CPU stress arm) is the row
   that joins Boundary at the P2-2 close, exactly as `P2-1g` did here.
3. **One decision stays parked on the board** — the untracked
   `smash64ds_P1.nds` at the repo root. It makes every Boundary run red until
   it is ruled on: the run relocates that file to `builds/` and restores it
   hash-verified. Do not delete it; it is the owner's file. (The menu-artwork
   decision is RULED, 2026-08-18 — row P2-1h.)
4. **The owner's visual pass on the shell screens is still owed** and is not an
   agent's to give; it now rides P2-1h's after-screenshots. Ten dated PNGs are
   in `artifacts/visibility/`
   (`2026-08-18_p2-shell-*` for the seven the P2-1g capture took, plus the
   three P2-1e character-select interaction shots). One run of
   `scripts/menus/capture-p2-shell.ps1` retakes the set.
5. Before implementing: read `docs/p2/P2-2-four-fighters.md`, then BattleShip
   `ft/ftmanager.c` + `ft/ftcommon/` (N-fighter iteration and engagement
   order), `gm/` (rules, teams, results ranking), and `sm64ds-decomp` for the
   sub-screen 2D HUD layer. CodeGraph first per repo rules.

## Standing operational facts (carried from P1 — still load-bearing)

- Clean checkout builds through `build.ps1`, not bare `make` (four of six
  `.inc` are gitignored and `build.ps1`'s generator is not run by `make`).
  `make p1-tick` = measuring ROM, `make p1` = published P1 pair. Never pass
  `-j`, never override `MAKEFLAGS`, one build at a time, never build a
  published target name for lab work.
- The two P2-1 lab targets are phase-named, not row-named:
  `smash64ds-p2-shell-hwtri` (shipping configuration — cadence, screenshots,
  the realtime pass through the menus) and `smash64ds-p2-shell-loop-hwtri`
  (the twenty-lap Boundary arm). Neither publishes.
- `gNdsMenuShellWalkBudget` makes the lap count a runtime poke, so a three-lap
  smoke and the twenty-lap gate come from ONE linked ROM.
- Preserve: mode 163, renderer mode 9, mip 0, static textures, source
  countdown, Dream Land water frame 0, Task 16 `1/1/1`. Never edit `decomp/`.
- Push hygiene: owner-name scan is `git grep -l -i -e <owner-given-name>
  HEAD`; the single surviving hit (sm64 IDO `usr/lib/copt`) is a false
  positive. History's 16 scrubbed Cargo blobs are owner-accepted.
- Run `New-Smash64DSSnapshot.ps1` after verified progress.

## Start-of-cycle commands

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

Then read `docs/P2_EXECUTION_BOARD.md` and take the highest unowned red row.
