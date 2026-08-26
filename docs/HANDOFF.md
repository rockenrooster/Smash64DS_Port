# Handoff

Current: 2026-08-26 — **P2-3f16 LANDED: all five landed fighters now enter with
their source audio.** BattleShip uses 214 MarioDokan for Mario AND Luigi's pipe,
191 FoxAppearArwing for Fox, 59 ContainerSmash for DK's barrel, and Falcon's
180/181 were already packed. The three missing cues are offline AOT renders;
214 keeps its two rests and 214/191 keep structural wave-loop metadata without
becoming hardware-looped whole cues. Pack **1,733,284 → 1,770,008 B**, **178 →
181 entries**, cache 204,800 unchanged, **327,144 B left**. Static pack/id/runtime
fixtures pass. **Real DS proof:** `2026-08-26_p2-3f16-entry-audio-trace.txt`,
shipping shell / fast-logic 0 / Luigi+Fox+Falcon+DK. `LastID` reaches 214, 191
and 59 with `PlayFailCount=0`, `IncludedLookupFailCount=0`, `UnsupportedCallCount=0`
and an advancing supported-play count. This is the positive half: allowlisted
pack drift would increment the failure counters instead of the miss ring.
The measured roster-wide miss census is now only 95/84/94/1/83; source also
names 128/17/630/631/635/110 outside that lap. **Falcon's 356 is SIZED, not
fixed** — 14,284 B (`falcon.md`). Open: **P2-3f11** (admitting Falcon to
`p2_fourcpu_stress` stops the slot-3 fighter drawing, mask 0x7, so that arm is
NOT the argmax) and **P2-2p7** (that arm runs at ~5 FPS on every roster and its
sampler subtracts the evidence — read it before trusting a banked four-CPU tick
figure). P2-3r17 stays DEFERRED.

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
     character select → stage select → battle). Renamed/rebased at P2-1M.
  3. `p2_fourcpu_stress` — `scripts/verify-p2-four-fighter-stress.ps1`, target
     `smash64ds-p2-fourcpu-tickhud-hwtri`, build `build-p2-fourcpu-tickhud`.
     Four level-3 CPUs, Dream Land, one-minute Time — **Mario/Fox/Luigi/Donkey
     since P2-3r15**; `NDS_P2_FOUR_CPU_ROSTER=0` rebuilds the mirror control.
     **Still NOT the argmax** (that is Mario/Fox/Captain/Donkey = 348,320 B —
     Luigi is the cheapest kind, not Mario); admitting Falcon here costs the
     slot-3 fighter its triangles, row **P2-3f11**. 0 humans / 4 CPUs
     / 4 fighter GObjs / mask `0xF`, all four slots drawing, plus the P2-2
     memory + native-Low-detail budget gate. **It asserts no tick gate on
     purpose:** four kinds sit far outside 1.12M — P2 debt (row P2-2p7: ~6x,
     and the sampler's `2^22` correction hides it), not a per-run verdict.
- **The P1-named proof target left the routine gate.**
  `smash64ds-battle-playable-proof-hwtri` is still in the Makefile and still the
  default for the dozen specialized probes and metric verifiers that boot
  straight into a battle (`probe-ko-blast.ps1`,
  `verify-battle-playable-camera-containment.ps1`, …). Nothing routine builds
  it. `check-gbi-decode-fixtures.ps1`'s pin on that target-selection line is
  intact by design — the shell target is selected by a `-P2ShellFlow` branch
  beside it, not instead of it.
- **P1 stays frozen.** `smash64ds-battle-playable-hwtri.nds`, 12,530,688 B,
  SHA-256 `576F51ED…E723`. Nothing routine rebuilds it.

## Next

1. **P2-1 implementation and automated phase-close verification are green.**
   Only the owner's visual re-check remains. The implementation plan is
   `docs/p2/P2-1-vs-shell.md`; do not reopen historical P2-1i/P2-1L findings as
   coding work unless verification finds a regression.
2. **P2-2 implementation and automated acceptance are green; owner visual/play
   acceptance remains.** BattleShip's VSBattle creation, engagement/catch/hit
   walks, CPU targeting, multi-target camera, KO scoring and Sudden Death are
   N-player source imports; the DS bridge keeps player instance 0..3 separate
   from generated owner kind. The lower-screen HUD is four-wide on live source
   `ifCommon` state with DMA-safe Bank-I writes; source effect/particle
   capacities are restored. **The full audit and the measured byte law live in
   `docs/p2/P2-2-four-fighters.md`; the standing arm's figures live in the board
   rows — every pre-2026-08-25 number on that arm is the MIRROR roster
   (P2-3r15).** The remaining item is explicitly visual: four-way camera
   framing, lower-screen HUD, Team Battle feel and Results/Sudden Death
   presentation. Do not claim the owner accepted those until they actually do.
3. **P2-3 is active, and the owner's 2026-08-23 batch is now mostly landed.**
   The character select carries the in-progress roster, A on a slot's 3D preview
   cycles its costume, pose slots are released on destroy, and a fighter's asset
   load no longer kills the BGM. The per-fighter native-owner tables left the
   ARM9 binary for NitroFS images (P2-3r4); the SHARED Mario/Fox table set
   (64,147 B a Luigi-vs-DK match never uses) is still unspent.
   **Intros** (P2-3r5), **Mario's pipe** (P2-3r6) and **the CSS preview's
   disconnected body parts** (P2-3r7) are fixed; the board rows carry all three.
   **Rail: a GX writer outside `nds_renderer.c` must call
   `ndsRendererFighterPacketDmaWait()` first** -- the CSS's own `glViewport`
   writes cut into the last preview fighter's draining packet DMA.
   **Four distinct kinds run in the SHIPPING configuration** (P2-3r11 + r13 +
   f9; full narrative on those board rows). **Rail: never raise
   `NDS_TASKMAN_ARENA_SIZE` without returning at least as much static image
   first** -- the step-down loop cannot tell an ambitious target from an
   exhausted heap. **The graphics-heap and BattlePack levers are SPENT** by
   P2-3f9; `ALL` P50/P95 on the gate arm is 1,965,184 / 3,085,888, flat within
   192 ticks across that change.
   **DK's cargo matrix is verified** (P2-3r10, `docs/p2/fighters/dk.md`).
   **Rail: a source function defined in a `battleship_*.o` but absent from the
   linked ELF is stranded unless it has an in-TU caller** -- a `#define` sent
   `ftDonkeyThrowFDamageSetStatus` to a compat stub and `--gc-sections` took it.
   **VS Stock's last-stock path is fixed** (P2-3r14) and **backing out of the
   stage select no longer kills the menu BGM** (P2-3r16). **Rails from them: a
   source TU the port skips strands its callees too**, and the CSS preview's synchronous file setup is bracketed with the P2-3r12 audio suspend/resume pair — never tuned. **Not proven yet:** the team stock steal.

4. **Captain Falcon is roster #3 and only an owner FEEL PASS remains**
   (P2-3f4/f5/f8/f13). `docs/p2/fighters/falcon.md` is the authority. Landed: the
   two model opcodes, his status table, `ftcaptainspecialn/lw/hi.c`, Falcon
   Dive's victim TU, the two `mpcommon` seams, his two-status entry ladder, the
   native owner (slot 4), full CSS/HUD/asset admission at
   `NDS_P2_SHELL_ROSTER=3`, and his audio bank.
   **He is the first owner with ZERO cross-matrix runs** (the reserved GX band
   stays Donkey's 16..25) and **the first whose LOW model carries a root light
   preamble his HIGH does not**. **Budget:** +20,064 B of ARM9 image, 18,800 B of owner images,
   **100,160 B unique per-kind arena** (Mario 54,048 / Luigi 41,552 /
   Donkey 77,360 / Captain 100,160 / Fox 116,752 — so the argmax drops LUIGI,
   not Mario). **The four-kind roster FITS (P2-3f9, low-water 419,052 B) and
   now RUNS (P2-3f10): the "~30x" was a data abort in his entry effect.**
   Three live defects were found and fixed at their seams: his `FTAttributes`
   mixed-u16 lanes had no normalizer arm (**Luigi's landed at P2-3f12**),
   the HUD's portrait and stock palette bands overlapped at slot 8,
   and the effect-desc deferral table was sized for the Mario/Fox roster.
   **Rails: a weak twin beside a strong body is CORRECT** (dispatch tables name
   every fighter's setter unconditionally — check `nm`, never `src/`), and **a
   generated include needs an explicit Makefile prerequisite** — `-MMD` writes
   `D:/…` where make expands `/d/…`, so the edge is invisible and the parallel
   build races.
   **P2-3 background:** Luigi ANIMATES (P2-3r2) and enters through his pipe
   (P2-3f2). **DK's one open realtime suspicion:** status 68 is
   nFTCommonStatusDownBounceU (NOT Dokan); re-observe on a realtime DK ROM,
   suspect DownBounce anim-end never fires. Keep admission fighter-by-fighter.
5. **Performance remains debt; the structural cuts are landed and default-on**
   (board rows P2-2p1..p6, promoted 2026-08-23; per-lever numbers and controls
   are in those rows). `NDS_R2_FIGHTER_PACKET=1` (DMA replay of each fighter's
   recorded GX stream, crediting the record frame's presented-work counters so
   harness contracts stay exact; `=0` is the control) plus P2-2p5's
   collision/matrix cuts plus P2-2p6's pose engine + 30 Hz body hold + Q12 clock
   took the MIRROR four-CPU arm from `WORK-H` P50/P95 1,600,832 / 2,069,824 to
   **1,244,608 / 1,777,408**;
   the published `smash64ds.nds` carries all of it (owner visual pass pending).
   **The arm is now the argmax roster (P2-3f10) and P2-2p7 says its banked
   levels are understated, so re-measure before sizing any lever.** Named
   remainders: the soft-float caller census lanes and walk #1 of
   `ndsFTParamsInvalidateSubtree` (~8K).

## Standing operational facts

- **Republish the free-play ROM after every fix batch** (owner, 2026-08-22): plain
  `make TARGET=smash64ds` writes root `smash64ds.nds` (human input, walk compiled
  out, flag-identical to the gate's shell config, **FIVE**-name roster).
  Current: **18,116,608 B, SHA-256 `457F5D0A…C91A`** (2026-08-25, adds P2-3f14
  and P2-3f15 — DK's and Luigi's banks, +221,184 B of pack; the board's
  `SHA-256` line is the authority); asserted from
  `builds/build/nds_build_config.h`: `NDS_P2_CAPTAIN 1`, `NDS_P2_MENU_WALK 0`,
  `NDS_P2_FOUR_CPU_ROSTER 0`, `NDS_P2_SHELL_ARGMAX_ROSTER 0`, and its staged
  `nitrofs/audio/fgm_phase_pack_ima.bin` byte-identical to `assets/`.
  `2FB213CC…CA74` crashes on Falcon's entry and `C7FF35D7…E171` hangs on four
  heavy kinds — do not hand either to the owner.
  **Boundary does NOT build it** — rebuild by hand, and `rm` the root
  `.elf`/`.nds` pair first if a lab build wrote them.
- Clean checkout builds through `build.ps1`, not bare `make` (four of six `.inc` are gitignored). Never pass `-j`, never override `MAKEFLAGS`, one build at a time.
- Shell targets: `smash64ds` is the published walk-free shell configuration
  (`smash64ds-p2-shell-freeplay-hwtri` retired at P2-1M).
  `smash64ds-p2-shell-hwtri` adds the scripted walk used by Boundary arm
  2/cadence probing; `smash64ds-p2-shell-loop-hwtri` is the fast-logic loop arm.
  Neither publishes. `NDS_P2_SHELL_ARGMAX_ROSTER=1` (P2-3f9) seeds the shell's
  CSS with the four heaviest kinds and is how a four-kind shell match is run.
- **The shell walk costs ~69 s before the battle starts** — 4,118 presented frames at 60 Hz (`artifacts/verification/2026-08-19_p2-shell.txt`); every wait in the battle arm is a *guest* anchor, so this costs only time.
- The shell's own character/stage select commits the mode-163 descriptor
  (`CSSLIVE`), read back out of the live battle state — the evidence the rebased
  arm verifies the same fight.
- Preserve: mode 163, renderer mode 9, mip 0, static textures, source countdown, Dream Land water frame 0, Task 16 `1/1/1`. Never edit `decomp/`.
- Push hygiene: owner-name scan is `git grep -l -i -e <owner-given-name> HEAD`; the one hit (sm64 IDO `usr/lib/copt`) is a false positive.
- **A GDB POKE CANNOT DRIVE THIS GAME'S STATE, and gdb READS of hot globals lie
  the same way** (P2-3f9). melonDS's stub reads and writes main RAM *behind* the
  ARM9 dcache. A probe wrote the CSS's `sCss*` arrays as aligned 32-bit stores,
  read them straight back, and the commit one instruction later published the
  OLD roster. Reads lie too: a real overflow halt published `count=0 request=0
  headroom=0 caller_lr=0` while the same stop's backtrace read
  `syMallocSet(size=12140, alignment=16)`. **A poke only sticks where the CPU
  has not touched the line yet** — why `gNdsMenuShellWalkBudget` works and
  per-frame state does not. Make the guest's own code write it: a build flag,
  not a breakpoint. Trust frame args and `gSYTaskmanGeneralHeap`; a row of
  zeroes means unreadable, never "no overflow".
- **Long verifier runs must be launched DETACHED** (`Start-Process pwsh
  -RedirectStandardOutput <log> -WindowStyle Hidden -PassThru`, no `-Wait`): a
  wrapper held open by an agent tool call is reaped at that tool's lifetime and
  takes melonDS with it — 40 min into the stress arm (2026-08-25).
- **Banking a verifier log: `Start-Process pwsh -RedirectStandardOutput <log> -NoNewWindow -Wait`, from a PowerShell host.** `Invoke-VerifyScriptOnce` replays child output with `[Console]::Out.Write`, so `Tee-Object` banks five lines and a `>` redirect INSIDE PowerShell banks two; and run through the Bash tool the recursive-make probe dies `Error 127` before any arm starts.
- **Current P2 owner directive is no snapshot.** Do not run
  `New-Smash64DSSnapshot.ps1` unless the owner explicitly re-enables snapshots.

## Start-of-cycle commands

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

Then read `docs/P2_EXECUTION_BOARD.md` and take the highest unowned red row.
