# Handoff

Current: 2026-08-19 — **BOUNDARY'S BATTLE ARM RUNS THROUGH THE SHELL, AND
`smash64ds.nds` IS THE BASE ROM.** (Board row P2-1M.)

## State

- **Boundary is two arms.** `verify-all.ps1 -Profile Boundary -List` is the
  membership authority; `docs/VERIFYING.md` owns the definition.
  1. `p2_shell_loop` — `scripts/verify-p2-shell-loop.ps1`, target
     `smash64ds-p2-shell-loop-hwtri`. Twenty full laps of the VS shell.
     Scene-boundary instrument at `NDS_HARNESS_FAST_LOGIC=1`; **no tick figure
     from it is a cadence figure.**
  2. `p2_battle_realtime` — `scripts/verify-battle-playable-realtime-harness.ps1`
     with `-P2ShellFlow`, target `smash64ds-p2-shell-hwtri`, build
     `build-p2-shell`. Mode `163`: Mario human vs level-3 CPU Fox, Dream Land,
     one-minute Time, items off — **reached through the shell** (title → menus →
     character select → stage select → battle). Renamed and rebased from
     `battle_playable_realtime` at row P2-1M (owner, 2026-08-19).
- **The registry `Harness` field is still `battle_playable_realtime`, and that
  is deliberate.** `Harness` names the *scene* (mode 163, `nds_scene_harness.h`,
  the Makefile mapping) and is unchanged; `Name` names the *verifier row*, and
  only the row moved. `check-harness-registry.ps1` cross-checks header/Makefile
  modes against `Harness`, so the rename cost it nothing (0 drift).
- **The P1-named proof target left the routine gate.**
  `smash64ds-battle-playable-proof-hwtri` is still in the Makefile and still the
  default for the dozen specialized probes and metric verifiers that boot
  straight into a battle (`probe-ko-blast.ps1`,
  `verify-battle-playable-camera-containment.ps1`, `verify-battle-playable-
  down-air-stall.ps1`, …). Nothing routine builds it. `check-gbi-decode-
  fixtures.ps1`'s pin on that target-selection line is intact by design — the
  shell target is selected by a `-P2ShellFlow` branch beside it, not instead of
  it.
- **P1 stays frozen.** `smash64ds-battle-playable-hwtri.nds`, 12,530,688 B,
  SHA-256 `576F51ED…E723`. Nothing routine rebuilds it, and this row did not
  touch it. (The board's old standing-rule pin of `2F47C8AC…CB2F` named a
  different build and is retired.)
- **Both halves of the battle arm now run the same ROM.** They did not before:
  the GDB half built the proof target while the screenshot half captured the
  frozen P1 artifact at the repo root, so the picture the gate accepted was
  never the program it had just asserted about.

## Next

1. **Publish `smash64ds.nds` from the shell configuration — NOT DONE, and it
   has a prerequisite this row found.** The owner ruled `smash64ds` is the base
   ROM; the Makefile change is small (`smash64ds` joins the free-play shell
   flag block, which retires into it). **But `smash64ds` today builds
   `NDS_DEV_SCENE_HARNESS ?= normal` with the shell flags off, and
   `scripts/verify-runtime.ps1` — 3,026 lines, the whole `runtime` arm of the
   Latest profile — asserts the original opening-movie → Title boot chain in
   that normal scene against the root `smash64ds.nds`/`.elf`.** Publishing the
   shell from that name without re-homing `verify-runtime.ps1` onto its own
   non-published `normal` target would leave the published ROM covered by a
   verifier that no longer describes it, which the publish law forbids. Do that
   slice first, then publish. Scene kind 27 (Startup) *is* still entered by the
   shell (`MSSCENE 1 curr=27`), so part of that verifier's premise survives —
   measure which part rather than assuming.
2. **Free-play delivery cadence is now standing** (owner, 2026-08-19): rebuild
   and hand the owner `smash64ds.nds` after each verified fix batch, and batch
   owner questions *before* any ROM-affecting build. Recorded in
   `docs/VERIFYING.md` → "How A P2 Row Runs".
3. **P2-1L round-5 visual findings** remain the open presentation work; P2-2
   (four-fighter engine) is the next phase. `P2-2a` (fighter array
   generalization) first — everything else in the phase depends on the
   2-fighter assumptions being gone. `P2-2f` (the 4-CPU stress arm) is the row
   that joins Boundary at the P2-2 close, exactly as `P2-1g` did.
4. **One decision stays parked on the board** — the untracked
   `smash64ds_P1.nds` at the repo root. It makes every Boundary run red until
   it is ruled on: `check-published-roms.ps1` enforces the two-ROM contract, so
   a run relocates that file to `builds/` and restores it hash-verified in a
   `try`/`finally`. Do not delete it; it is the owner's file.

## Standing operational facts

- Clean checkout builds through `build.ps1`, not bare `make` (four of six
  `.inc` are gitignored and `build.ps1`'s generator is not run by `make`).
  Never pass `-j`, never override `MAKEFLAGS`, one build at a time.
- The three P2 shell targets, and the single flag between them:
  `smash64ds-p2-shell-hwtri` (Boundary arm 2 + the cadence probe; shipping
  flags **plus** `NDS_P2_MENU_WALK := 1`), `smash64ds-p2-shell-loop-hwtri` (the
  twenty-lap arm; adds `NDS_HARNESS_FAST_LOGIC := 1`), and
  `smash64ds-p2-shell-freeplay-hwtri` (walk-free, `NDS_BOOT_DIAG_TEXT := 0` —
  this is the one that retires into `smash64ds` when item 1 above lands).
  None of them publishes.
- `gNdsMenuShellWalkBudget` makes the lap count a runtime poke, so a three-lap
  smoke and the twenty-lap gate come from ONE linked ROM.
- **The shell walk costs ~69 s before the battle starts** — title 150 + mode
  select 301 + VS mode 1,811 + character select 1,612 + stage select 244 =
  4,118 presented frames at 60 Hz (banked:
  `artifacts/verification/2026-08-19_p2-shell.txt`). Every wait in the battle
  arm is a *guest* anchor, not wall-clock, so this costs the arm only time; its
  GDB capture ceiling is 600 s for that reason.
- The shell's own character/stage select commits the mode-163 descriptor —
  `CSSLIVE s0=0/0/1 s1=1/1/3 pl=1 cp=1 time=1 gkind=06` (Mario human, Fox CPU
  level 3, Time mode, Dream Land), read back out of the live battle state. That
  is the evidence the rebased arm verifies the same fight.
- Preserve: mode 163, renderer mode 9, mip 0, static textures, source
  countdown, Dream Land water frame 0, Task 16 `1/1/1`. Never edit `decomp/`.
- Push hygiene: owner-name scan is `git grep -l -i -e <owner-given-name> HEAD`;
  the single surviving hit (sm64 IDO `usr/lib/copt`) is a false positive.
- Run `New-Smash64DSSnapshot.ps1` after verified progress.

## Start-of-cycle commands

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

Then read `docs/P2_EXECUTION_BOARD.md` and take the highest unowned red row.
