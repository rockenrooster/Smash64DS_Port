# Handoff

Current: 2026-08-25 — **THE FOUR-NAME ROSTER SHIPS, AND FOUR DISTINCT KINDS NOW
RUN A WHOLE STRESS MATCH — ON A LAB ARM THAT GIVES UP THE FIGATREE PACK TO DO
IT.** Luigi and Donkey Kong are selectable and marked unfinished; the shipped
configuration still cannot host four distinct fighters (board row P2-3r13).
P2-1/P2-2 still have only their owner visual/play residuals.

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
3. **P2-3 is active, and the owner's 2026-08-23 batch is now mostly landed.**
   The character select carries the in-progress roster -- Luigi AND Donkey Kong
   selectable, portraits dimmed under the source question-mark plate -- A on a
   slot's 3D preview cycles that slot's costume, pose slots are released when a
   fighter is destroyed, and a fighter's asset load no longer kills the BGM.
   `NDS_P2_SHELL_ROSTER` defaults to 2 and is still a MEASURED level: the
   per-fighter native-owner tables left the ARM9 binary for NitroFS images
   (P2-3r4), which moved roster-2 arena headroom from 13,840 B (battle aborted
   in `ifCommonCountdownMakeInterface`) to 44,848 B, worst case 28,772 B for a
   Luigi-versus-Donkey match with both images resident. **Size the next fighter
   against 28,772, not 44,848.** The remaining lever is the SHARED Mario/Fox
   table set, 64,147 B of binary a Luigi-versus-DK match never uses.
   **Intros are fixed** (P2-3r5): the port-owned fighter display had omitted the
   source `is_invisible` gate. **Mario's pipe is fixed end-to-end** (P2-3r6);
   see the board row for the four-defect chain. The final leak was not depth:
   entry PAL16 upload forced DS `COLOR0_TRANSPARENT` although both pipe textures
   use palette index 0 as opaque green, punching literal holes through the rim
   and inner wall. Generated entry textures now carry the canonical RGBA5551
   colour-0 transparency bit. Packet-on +10/+20 captures show a continuous
   rim/body/opening, +104 has no terminal slab, and the one-lap shell verifier
   is green (11 entries, zero faults, 39,432 B free). **The CSS preview's
   disconnected body parts are fixed** (P2-3r7): its CSS-only `glViewport`
   writes bypassed `ndsRendererFighterPacketDmaWait()` and cut into the last
   preview fighter's draining packet DMA. **Rail: a GX writer outside
   `nds_renderer.c` must call that wait first.**
   **The four-distinct-kind stress arm runs** (P2-3r11): "two fighter GObjs"
   was an instrument that could only count Mario and Fox, and the wall was
   `ftManagerSetupFilesMainKind(Donkey)` asking 77,360 B with 8,300 B free and
   halting in `ndsSyMallocOverflowHalt`. Four distinct kinds need ~175 KB more
   arena than two mirrored; the lab arm pays it with `NDS_R2_BATTLEPACK := 0`
   plus a 32,768 B cache trim, so **every tick figure from that arm is pack-off
   and is not comparable to the mirror roster's.** **The SHIPPED configuration
   still cannot host four distinct fighters** -- open board row P2-3r13, and the
   owner's call rather than an implementation detail.

4. **P2-3 background.** Luigi's production path is landed and he ANIMATES
   (P2-3r2). The bounded fast proof route is repaired end-to-end and **both
   bounded proofs are GREEN on one tree (`9c412271f0f`, 2026-08-23): kind 2
   (full DK moveset, Giant Punch/Spinning Kong/Hand Slap, blaster
   projectile, driven KO, mask 0x7ffff, retries 0) and kind −1 (reflector
   0xff, specials 0xfff).** The stall was never collision (board row P2-3r3
   has the five-step chain: attacker facing, reflector slot collapse,
   same-update laser lifetime, accidental-KO skip, frozen top-HUD stock
   sampling). **DK's one live blocker is realtime:** status 68 re-decoded as
   nFTCommonStatusDownBounceU (knockdown bounce — NOT Dokan; the old note
   misread the enum). Rebuild the realtime DK ROM on this tree and
   re-observe; suspect DK's DownBounce anim-end never fires. Keep admission
   fighter-by-fighter.
5. **Performance remains debt; the structural cut is landed and default-on
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
   contracts stay exact. P2-2p5 (collision getter early-out, in-place
   kind-1 matrix convert, exact scale compare) took the arm to 1,264,512 /
   1,836,800. **P2-2p6 is CLOSED and default-on (owner "do both",
   2026-08-23):** the fighter pose engine + 30 Hz body hold + Q12 clock —
   oracle 0 mismatches over 189,251 joint compares, four-CPU arm now
   `WORK-H` P50/P95 **1,244,608 / 1,777,408**, three-arm Boundary green on
   the promoted tree, freeplay `smash64ds.nds` republished on it. The
   remaining gap to 1.12M is the soft-float caller census lanes and walk #1
   of `ndsFTParamsInvalidateSubtree` (~8K), per the board row.

The phase-close run also fixed verifier drift rather than bypassing it:
`verify-all.ps1 -NoBuild` now resolves retained per-harness artifacts through
the shared build-output resolver, the wallpaper fixture recognizes exact
repeated source rows instead of scratch-buffer pointer identity, the native
fighter owner fixture pins both High and Low BattleShip light-command censuses,
and the four-CPU sparse marker is proven to observe the same published
frame-complete tuple as the universal marker. `DECOMP_PRISTINE=PASS` remained
green throughout.

## Standing operational facts

- **Republish the free-play ROM after every fix batch** (owner, 2026-08-22): a plain
  `make TARGET=smash64ds` writes the root `smash64ds.nds` (human input, walk
  compiled out, flag-identical to the gate's shell config, four-name roster).
- Clean checkout builds through `build.ps1`, not bare `make` (four of six
  `.inc` are gitignored). Never pass `-j`, never override `MAKEFLAGS`, one
  build at a time.
- `smash64ds` is the published free-play shell target; the P2-1M "publish not
  done" restart item is closed by the later P2-1N publish work.
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
