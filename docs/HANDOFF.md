# Handoff

Current: 2026-08-22 — **P2-1/P2-2 AUTOMATED ACCEPTANCE IS GREEN; P2-3 LUIGI
PRODUCTION IS LANDED AND BOUNDARY STILL HAS THREE GREEN ARMS.** The fresh
post-Luigi/source-entry profile passed all three arms. P2-1/P2-2 still have only
their owner visual/play residuals; P2-3 is the active implementation frontier.

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
     Four level-3 CPUs, Dream Land, one-minute Time. The accepted run covers
     frames 1..1973 / guest clock 60→1, proves 0 humans / 4 CPUs / 4 fighter
     GObjs / active mask `0xF`, and owns the P2-2 memory + native-Low-detail
     budget gate.
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

1. **P2-1 implementation and automated phase-close verification are green.**
   The 2026-08-21 Boundary closeout completed one exact shell lap and the
   shell-driven two-fighter regression without faults; the same profile also
   retained the four-CPU arm green. Only the owner's visual re-check remains.
   The implementation plan is `docs/p2/P2-1-vs-shell.md`; do not reopen
   historical P2-1i/P2-1L findings as coding work unless verification finds a
   regression.
2. **P2-2 implementation and automated acceptance are green; owner visual/play
   acceptance remains.** BattleShip's VSBattle creation,
   fighter-vs-fighter engagement/catch/hit walks, CPU targeting, multi-target
   camera, KO scoring and Sudden Death are already N-player source imports. The
   DS-owned bridge now keeps player instance 0..3 separate from generated
   Mario/Fox owner kind 0/1, and the audit has also restored source `ftparam`
   battle-stat/stale/damage attribution semantics, source grounded damage-
   velocity projection for DownBounce/Passive/PassiveStand, source effective
   hit-status aggregation (hurtboxes + star + timed special), the source
   hit-status/colour-animation lifecycle and common Mario/Fox VS colanim table,
   source centered-stick facing semantics and hitlag truncation/order, source `player_num` lookup,
   battle-entry registry lifetime, four-way Results registration and all four
   live CSS preview slots. The lower-screen HUD is now four-wide and consumes
   live source `ifCommon` timer/damage/stock state while routing only those
   steady display GObjs off the gameplay screen; its sub-OBJ Bank-I palette and
   graphics writes use DMA-safe hardware widths. Source effect/particle pool
   capacities are restored. The standing four-CPU run is accepted and is now
   Boundary arm 3: the fresh 2026-08-22 run is `ALL` P50/P95
   1,678,144/2,797,888 ticks and `WORK-H` P50/P95 1,603,904/2,066,688; heap free low-water
   40,400 B, effects 17/38 active, particles 33/112 + 11/24 + 14/80, no rejects
   or hard allocation/AObj failures. The source Low-detail native fighter owner
   is the first mitigation (fresh plan `build=684`, `hit=6665`, mismatch 0). The
   measured owner-based byte law is in `docs/p2/P2-2-four-fighters.md`. The
   remaining item is explicitly visual: four-way camera framing, lower-screen
   HUD presentation, Team Battle feel and Results/Sudden Death presentation.
   Do not claim the owner accepted those until they actually do.
3. **P2-3 is active.** Luigi's source-derived loader/native-owner/CSS/HUD path
   and focused special/projectile proof are landed. Mario's pipe and Fox's
   Arwing now use build-time converted DS-native entry geometry with zero generic
   entry fallback while keeping BattleShip's live DObj/visibility timeline.
   The next structurally new fighter is Donkey Kong. Keep admission fighter-by-
   fighter; do not turn this into a roster mega-import. **Owner review rows
   P2-3r1..r3 (board, 2026-08-22):** the pipe is fixed (lit normals + light
   seed; the rise to 420 is the source's own motion), "Luigi has issues" waits
   on specifics, and the stopped DK agent's `NDS_P2_DONKEY` proof work sits
   uncommitted in the tree until compiled under the flag.
4. **Performance remains debt; the structural cut is landed and default-on
   (board rows P2-2p1..p4, promoted 2026-08-23, Boundary GREEN on the
   promoted tree).** Four-CPU arm before: `WORK-H` P50/P95 1,600,832 /
   2,069,824; without any fighter draw 993,792 / 1,462,144 (the four draws
   were 607K at P50). `NDS_R2_FIGHTER_PACKET=1` — DMA replay of each
   fighter's recorded GX stream, flash tint patch, per-packet texture
   residency, deferred DMA wait, material pre-check — reads 1,281,728 /
   1,866,432 with the FTR lane 665,920 → 306,560, entry-series captures
   pixel-identical; `NDS_R2_FIGHTER_PACKET=0` is the control arm. The
   published `smash64ds.nds` carries it (owner visual pass pending). The
   replay credits the record frame's presented-work counters on every hit
   (batches, prepares, binds, matrix loads, vertex loads), so harness
   contracts stay exact. The remaining gap is the source lanes (SRC 594K
   P50 / 1,033K P95, OTHR 360K), parked behind the sacrifice order.

The phase-close run also fixed verifier drift rather than bypassing it:
`verify-all.ps1 -NoBuild` now resolves retained per-harness artifacts through
the shared build-output resolver, the wallpaper fixture recognizes exact
repeated source rows instead of scratch-buffer pointer identity, the native
fighter owner fixture pins both High and Low BattleShip light-command censuses,
and the four-CPU sparse marker is proven to observe the same published
frame-complete tuple as the universal marker. `DECOMP_PRISTINE=PASS` remained
green throughout.

## Standing operational facts

- **Republish the free-play ROM after every fix batch** (owner, 2026-08-22:
  "periodically create the freeplay ROM so I can help playtest"): a plain
  `make TARGET=smash64ds` writes the root `smash64ds.nds` (human input, walk
  compiled out, flag-identical to the gate's shell config). A Luigi-enabled
  lab twin is `make TARGET=smash64ds-p2-shell-freeplay-hwtri
  BUILD=builds/build-p2-shell-freeplay-luigi NDS_P2_LUIGI=1`; it is not the
  gate configuration and stays in `builds/`.
- Clean checkout builds through `build.ps1`, not bare `make` (four of six
  `.inc` are gitignored and `build.ps1`'s generator is not run by `make`).
  Never pass `-j`, never override `MAKEFLAGS`, one build at a time.
- `smash64ds` is already in the free-play shell target block and the current
  published root ROM is the P2 shell baseline; the old P2-1M "publish not done"
  restart item is closed by the later P2-1N publish work.
- Shell target relationship: `smash64ds` is the published walk-free shell
  configuration. `smash64ds-p2-shell-freeplay-hwtri` is its non-published lab
  twin; `smash64ds-p2-shell-hwtri` adds the scripted realtime shell walk used
  by Boundary arm 2/cadence probing; `smash64ds-p2-shell-loop-hwtri` is the
  fast-logic loop arm. The diagnostic names do not publish.
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
- **Current P2 owner directive is no snapshot.** Do not run
  `New-Smash64DSSnapshot.ps1` unless the owner explicitly re-enables snapshots.

## Start-of-cycle commands

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

Then read `docs/P2_EXECUTION_BOARD.md` and take the highest unowned red row.
