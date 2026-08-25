# Handoff

Current: 2026-08-25 — **THE TWO FIGHTER-SHAPED STATIC CHECKERS ARE GREEN AND IN
BOUNDARY** (rows P2-3f1/f3), and each found a real defect on the way:
`check-architecture.ps1` was terminating on its FIRST `Write-Error` under
`$ErrorActionPreference='Stop'`, hiding a second failure behind the one three
docs recorded; `check-decomp-header-mirror.py` could not fold a single
per-fighter status enum, which is why Captain's and Link's Attack100 ordinals
sat one low. **Luigi had no entry at all** (P2-3f2): `ftCommonAppearSetStatus`
dropped him into `EntryNull` while the renderer's Luigi pipe arm already
existed. **Captain Falcon pipeline slice 1 is landed** (P2-3f4) — the manifest
generalised on one tuple entry; his native model is BLOCKED on an alpha-test
decoder extension, see `docs/p2/fighters/falcon.md`. Earlier: stress arm on the
four landed kinds (P2-3r15), Stock last-stock (P2-3r14), DK cargo (P2-3r10).

## State

- **Boundary is three arms.** `verify-all.ps1 -Profile Boundary -List` is the
  membership authority; `docs/VERIFYING.md` owns the definition.
  1. `p2_shell_loop` — `scripts/verify-p2-shell-loop.ps1`, target
     `smash64ds-p2-shell-loop-hwtri`. The phase-close default is one complete
     VS-shell lap (the owner retired the historical 20-lap requirement on
     2026-08-19); higher loop counts remain available only for an explicit soak.
     Scene-boundary instrument at `NDS_HARNESS_FAST_LOGIC=1`; **no tick figure
     from it is a cadence figure.**
  2. `p2_battle_realtime` — `scripts/verify-battle-playable-realtime-harness.ps1`
     with `-P2ShellFlow`, target `smash64ds-p2-shell-hwtri`, build
     `build-p2-shell`. Mode `163`: Mario human vs level-3 CPU Fox, Dream Land,
     one-minute Time, items off — **reached through the shell** (title → menus →
     character select → stage select → battle). Renamed and rebased from
     `battle_playable_realtime` at row P2-1M (owner, 2026-08-19).
  3. `p2_fourcpu_stress` — `scripts/verify-p2-four-fighter-stress.ps1`, target
     `smash64ds-p2-fourcpu-tickhud-hwtri`, build `build-p2-fourcpu-tickhud`.
     Four level-3 CPUs, Dream Land, one-minute Time — **Mario/Fox/Luigi/Donkey
     since row P2-3r15**; `NDS_P2_FOUR_CPU_ROSTER=0` rebuilds the mirror
     control. Frames 1..1973 / guest clock 60→1, 0 humans / 4 CPUs / 4 fighter
     GObjs / mask `0xF`, all four slots drawing, plus the P2-2 memory +
     native-Low-detail budget gate. **It asserts no tick gate on purpose:** four
     distinct kinds sit ~3x outside 1.12M, which is P2 debt, not a per-run
     verdict; a guard red by construction protects nothing.
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

## Next

1. **P2-1 implementation and automated phase-close verification are green.**
   The 2026-08-21 Boundary closeout completed one exact shell lap and the
   shell-driven two-fighter regression without faults; the same profile also
   retained the four-CPU arm green. Only the owner's visual re-check remains.
   The implementation plan is `docs/p2/P2-1-vs-shell.md`; do not reopen
   historical P2-1i/P2-1L findings as coding work unless verification finds a
   regression.
2. **P2-2 implementation and automated acceptance are green; owner visual/play
   acceptance remains.** BattleShip's VSBattle creation, engagement/catch/hit
   walks, CPU targeting, multi-target camera, KO scoring and Sudden Death are
   N-player source imports; the DS bridge keeps player instance 0..3 separate
   from generated owner kind, and the audit restored source `ftparam`
   stat/stale/damage attribution, grounded damage-velocity projection,
   hit-status aggregation and colanim lifecycle, centered-stick facing, hitlag
   order, battle-entry registry lifetime, four-way Results and all four live
   CSS preview slots. The lower-screen HUD is four-wide on live source
   `ifCommon` state with DMA-safe Bank-I writes; source effect/particle
   capacities are restored. **The full audit and the measured byte law live in
   `docs/p2/P2-2-four-fighters.md`; the standing arm's current figures live in
   the board rows, not here — every pre-2026-08-25 number on that arm is the
   MIRROR roster (P2-3r15).** The remaining item is explicitly visual: four-way
   camera framing, lower-screen HUD presentation, Team Battle feel and
   Results/Sudden Death presentation. Do not claim the owner accepted those
   until they actually do.
3. **P2-3 is active, and the owner's 2026-08-23 batch is now mostly landed.**
   The character select carries the in-progress roster -- Luigi AND Donkey Kong
   selectable, portraits dimmed under the source question-mark plate -- A on a
   slot's 3D preview cycles that slot's costume, pose slots are released when a
   fighter is destroyed, and a fighter's asset load no longer kills the BGM.
   `NDS_P2_SHELL_ROSTER` defaults to 2; the per-fighter native-owner tables left
   the ARM9 binary for NitroFS images (P2-3r4). Roster-2 headroom figures
   (28,772 / 44,848) predate P2-3r13's arena raise -- resize against the current
   run; the SHARED Mario/Fox table set (64,147 B a Luigi-vs-DK match never uses)
   is still unspent.
   **Intros are fixed** (P2-3r5, the missing source `is_invisible` gate) and
   **Mario's pipe is fixed end-to-end** (P2-3r6, a four-defect chain ending in a
   forced DS `COLOR0_TRANSPARENT`); the board rows carry both. **The CSS
   preview's disconnected body parts are fixed** (P2-3r7): its CSS-only
   `glViewport` writes bypassed `ndsRendererFighterPacketDmaWait()` and cut into
   the last preview fighter's draining packet DMA. **Rail: a GX writer outside
   `nds_renderer.c` must call that wait first.**
   **Four distinct kinds run in the SHIPPING configuration** (P2-3r11 + r13;
   full narrative on those board rows). `NDS_R2_BATTLEPACK := 0` and the cache
   trim are gone, so **four-kind tick figures are pack-on and comparable**
   (`ALL` P50/P95 1,964,992 / 3,085,696 on the promoted gate arm). **Rail:
   never raise `NDS_TASKMAN_ARENA_SIZE` without returning at least as much
   static image first** -- the step-down loop cannot tell an ambitious target
   from an exhausted heap. **Measured and left on the table:** the per-context
   graphics heap peaks at **96 B of 53,248** (two contexts = 106,496 B of
   arena), overflow 0; re-read it on Results / Sudden Death / pause zoom.
   **DK's cargo matrix is verified** (P2-3r10, `docs/p2/fighters/dk.md`): grab,
   carry, walk/turn/jump/edge/land, the ONE cargo release (`ThrowFF`/
   `ThrowAirFF`; `HeavyThrow*` are heavy-ITEM throws, P2-5), mash-out,
   KO-while-carried and Giant Punch charge, each traced to its source owner AND
   to the linked image. **One defect, fixed at its seam:** a `#define` in
   `battleship_ftcommon_damage.c` sent `ftDonkeyThrowFDamageSetStatus` to a
   compat stub, so the source setter was compiled and then dropped by
   `--gc-sections`. **Rail: a source function defined in a `battleship_*.o` but
   absent from the linked ELF is stranded unless it has an in-TU caller.**
   **VS Stock's last-stock path is fixed** (P2-3r14): `ftCommonDeadCheckRebirth`
   reached the weak `ftCommonSleepSetStatus` no-op in a driven Stock match, and
   the eliminated fighter then sat in `DeadDown` with `camera_mode` **Default**
   — the one mode `gmCameraUpdateFighterBounds` does NOT skip — dragging the
   camera to where it died for the rest of the match. `ftcommonsleep.c` is
   imported; the weak twins are deleted. **Rail: a source TU the port skips
   strands its callees too** — `ifCommonPlayerStockStealMakeInterface` was
   `--gc-sections`ed out for want of its only caller. **Not proven yet:** the
   team stock steal (reachable via STOCK+TEAM in the shell).
   **Backing out of the stage select killed the menu BGM** (P2-3r16):
   `ndsMNPlayersVSPreviewInit` sets up four kinds' file trees synchronously and
   was unbracketed, so the SECOND character-select entry overran the ADPCM seam
   and `ndsAudioBgmFailPlayback` stopped the stream. Bracketed with the
   P2-3r12 suspend/resume pair. **Rail: that overrun is marginal, so it moves
   with code placement** — an unrelated few-instruction change flipped this arm
   from green to red and back, which is why it is bracketed, not tuned.

4. **Captain Falcon is roster #3 and slice 1 is landed** (P2-3f4).
   `docs/p2/fighters/falcon.md` carries the complete law-7 inventory (160
   NitroFS resources, 152 animations, 19 statuses, 19 item motions, the audio
   name list, six costume indices) and the remainder in dependency order.
   **The next step is item 1 there and it is a real pipeline extension, not a
   fighter task:** Falcon's high-detail model is the first to use
   `G_SETOTHERMODE_L` (0xE2, one alpha-tested surface) and `G_SETBLENDCOLOR`
   (0xF9); `generate_nds_native_owners.py` models neither and raises
   `unsupported control opcode 0xe2`. The low-detail model already decodes
   clean with the five pins recorded there. Do not start the runtime slice
   before the model lands — nothing can create him without an owner slot.
   **P2-3 background:** Luigi ANIMATES (P2-3r2) and now enters through his pipe
   (P2-3f2). **DK's one open realtime suspicion:** status 68 is
   nFTCommonStatusDownBounceU (knockdown bounce — NOT Dokan); re-observe on a
   realtime DK ROM, suspect DownBounce anim-end never fires. Keep admission
   fighter-by-fighter.
5. **Performance remains debt; the structural cuts are landed and default-on**
   (board rows P2-2p1..p6, promoted 2026-08-23, Boundary GREEN on the promoted
   tree; per-lever numbers and controls are in those rows). `NDS_R2_FIGHTER_PACKET=1`
   (DMA replay of each fighter's recorded GX stream, crediting the record
   frame's presented-work counters so harness contracts stay exact;
   `=0` is the control) plus P2-2p5's collision/matrix cuts plus P2-2p6's
   fighter pose engine + 30 Hz body hold + Q12 clock took the MIRROR four-CPU
   arm from `WORK-H` P50/P95 1,600,832 / 2,069,824 to **1,244,608 / 1,777,408**;
   the published `smash64ds.nds` carries all of it (owner visual pass pending).
   **The arm is now the four-kind roster (P2-3r15), so the live gap to 1.12M is
   larger than those figures imply** — re-measure before sizing a lever. The
   named remainders are the soft-float caller census lanes and walk #1 of
   `ndsFTParamsInvalidateSubtree` (~8K).

## Standing operational facts

- **Republish the free-play ROM after every fix batch** (owner, 2026-08-22): a plain
  `make TARGET=smash64ds` writes the root `smash64ds.nds` (human input, walk
  compiled out, flag-identical to the gate's shell config, four-name roster).
  Current: **17,012,736 B, SHA-256 `5B962053…F579`** (2026-08-25, carries
  P2-3r14 + P2-3r16). `NDS_P2_FOUR_CPU_ROSTER` is 0 there — the P2-3r15 default
  applies to the stress target only and does not reach the shipped ROM.
- Clean checkout builds through `build.ps1`, not bare `make` (four of six
  `.inc` are gitignored). Never pass `-j`, never override `MAKEFLAGS`, one
  build at a time.
- Shell target relationship: `smash64ds` is the published walk-free shell
  configuration (`smash64ds-p2-shell-freeplay-hwtri` retired at P2-1M — the
  published name IS that configuration). `smash64ds-p2-shell-hwtri` adds the
  scripted walk used by Boundary arm 2/cadence probing;
  `smash64ds-p2-shell-loop-hwtri` is the fast-logic loop arm. Neither publishes.
- `gNdsMenuShellWalkBudget` makes the lap count a runtime poke: one ROM serves a smoke and a soak.
- **The shell walk costs ~69 s before the battle starts** — 4,118 presented
  frames at 60 Hz (banked: `artifacts/verification/2026-08-19_p2-shell.txt`).
  Every wait in the battle arm is a *guest* anchor, not wall-clock, so this
  costs the arm only time; its GDB capture ceiling is 600 s for that reason.
- The shell's own character/stage select commits the mode-163 descriptor
  (`CSSLIVE`: Mario human, Fox CPU, Time mode, Dream Land), read back out of the
  live battle state — the evidence the rebased arm verifies the same fight.
- Preserve: mode 163, renderer mode 9, mip 0, static textures, source
  countdown, Dream Land water frame 0, Task 16 `1/1/1`. Never edit `decomp/`.
- Push hygiene: owner-name scan is `git grep -l -i -e <owner-given-name> HEAD`; the one hit (sm64 IDO `usr/lib/copt`) is a false positive.
- **Current P2 owner directive is no snapshot.** Do not run
  `New-Smash64DSSnapshot.ps1` unless the owner explicitly re-enables snapshots.

## Start-of-cycle commands

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

Then read `docs/P2_EXECUTION_BOARD.md` and take the highest unowned red row.
