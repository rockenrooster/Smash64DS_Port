# The battlepack default flip, measured on the shipping configuration: cadence 96.23% on the stress gate, eight matches NO-FREEZE, and the root pair reproduces bit-exactly

**Date:** 2026-08-17 · **Branch:** `codex/r2-runtime2` · **HEAD `798007f30d8`**
**3 lab builds** (`build-tick-hud-buckets`, `build-c247-pubgate-packon`,
`build-c248-soak-packon`) **+ one determinism relink of the published pair**,
**2 emulator runs + Boundary**,
**0 production source edits by this cycle; the one edit under test —
`Makefile`, the two `?= 0 -> ?= 1` default lines — was already in the tree.**
**UNITS: 1 project tick = 1 `cpuGetTiming()` tick = 2 ARM9 cycles.**

This is the run `SHIP_CADENCE.md` §5 was blocked on:

> **OWNER DECISION, 2026-08-17: FLIP IT, BUT SOAK IT FIRST.** The default moves
> to 1 only after a fresh freeze soak and a full Boundary on the flipped
> default are run and reported.

`PREDICTION_C247.md` in this directory was written before anything was built or
run on the flipped defaults, and is quoted against its measurement in §2.

---

## 0. Outcome

```text
THE FLIP        Makefile: NDS_R2_BATTLEPACK ?= 1 and
                NDS_R2_BATTLEPACK_KEEP_CACHE ?= 1.  Already applied in the
                working tree when this cycle started; this cycle did not
                author it and does not commit it.

CADENCE         c247 = the PUBLISHED target, the owner's both-CPU stress arm,
                on the flipped defaults.  Whole match, mode 163, one minute,
                DLDI on, denominator = the guest's own presented-frame counter
                read at Results entry.

                  1,966 / 2,043 two-VBlank = 96.23%   viol=0   max 19
                  >=95% needs 1,941, so the margin is +25 frames

                against c245, the identical arm with the pack OFF:
                1,945 / 2,043 = 95.20%, margin +4.  THE FLIP MOVES THE GATE
                MARGIN FROM +4 FRAMES TO +25, a 6.25x widening.

PREDICTED       1,958 (95.84%), declared range 1,950-1,966, written to
                PREDICTION_C247.md before the build.  MEASURED 1,966 -- inside
                the range but at its exact upper edge.  The additive isolation
                model of SHIP_CADENCE.md §2.1 under-predicted by 8 frames and
                is corrected in §2.2 rather than left standing.

SOAK            NO-FREEZE.  EIGHT completed successive matches in one emulator
                session (the acceptance battery asks for three), 7 START
                restarts, 12 battle-scene entries, FOUR Sudden Deaths -- read
                from gNdsSCVSBattleSuddenDeathPrepareCount, not inferred.
                Arena 1,548,288 of a 1,564,672 grantable ceiling; every
                allocator failure counter 0; general-heap low-water 52,768
                against the mandated 32,768 floor.

DETERMINISM     The root pair reproduces BIT-EXACTLY.  The c247 build writes
                the published ROM into the project root; deleting both halves
                and re-running `make TARGET=smash64ds-battle-playable-hwtri`
                with no overrides returned
                  .nds  5F3D1FE3...D20C   12,538,880 B   (identical)
                  .elf  DDB055C2...9E10   10,282,628 B   (identical)
                to the 14:10 link.  smash64ds.nds never rebuilt, unchanged at
                54C07FAC...C68A.

BOUNDARY        See section 5.

THE HEAP PRICE  CONFIRMED A SECOND TIME, ON A DIFFERENT RUN SHAPE, TO 16 BYTES.
                Makefile:345-347 predicted the pack costs 17,600 B of general
                heap.  The c239/c246 tick pair measured 17,600 exactly.  This
                soak (8 matches) against the 2026-08-13 pack-off soak (3
                matches) measures 70,384 - 52,768 = 17,616.
```

---

## 1. What was built, and the flag-identity proof

Every build in this cycle used the tree's own defaults; no `NDS_R2_BATTLEPACK`
or `NDS_R2_BATTLEPACK_KEEP_CACHE` was ever passed on a command line, so each
one is a test of the *default*, which is the thing under review.

| build | command | writes | `BATTLEPACK` / `KEEP_CACHE` in its generated header |
|---|---|---|---|
| `build-tick-hud-buckets` | `make p1-tick` | build dir | **1 / 1** |
| `build-c247-pubgate-packon` | `make TARGET=smash64ds-battle-playable-hwtri BUILD=build-c247-pubgate-packon NDS_R2_BOTH_CPU=1` | **project root** | **1 / 1** |
| `build-c248-soak-packon` | built by `soak-freeze-watch.ps1` | build dir | **1 / 1** |
| published relink | `make TARGET=smash64ds-battle-playable-hwtri` | **project root** | **1 / 1** (`builds/build`) |

**The instrument is flag-identical to the published ROM, and this is a diff
rather than an assertion.** `builds/build/nds_build_config.h` (the published
target's resolved configuration) against
`builds/build-tick-hud-buckets/nds_build_config.h`:

```text
9c9
< #define NDS_TICK_HUD 0
---
> #define NDS_TICK_HUD 1
```

One line, and it is the instrument's own switch. `docs/VERIFYING.md` §7 of
`SHIP_CADENCE.md` recorded a `scripts/check-shipping-basis.ps1` as the
structural fix for the basis defect; that checker is still unwritten, but the
diff above is the check it would perform, run by hand for these two targets.

**`make p1-tick` was used rather than a bare
`make TARGET=smash64ds-battle-playable-tickhud-hwtri`.** They are the same
TARGET with the same flags; the alias supplies `BUILD=build-tick-hud-buckets`,
which is the directory `sample-tick-hud-buckets.ps1` measures from and the one
`docs/VERIFYING.md` "Building For P1" names. A bare invocation defaults to
`BUILD := build` (`Makefile:46`) — the directory that currently holds the
*published* target's incremental state — and would have destroyed it, forcing
the determinism relink in §3 to be a full rebuild rather than a link.

**The gate arm is one line from the published configuration too.**
`builds/build/nds_build_config.h` against
`builds/build-c247-pubgate-packon/nds_build_config.h`:

```text
77c77
< #define NDS_R2_BOTH_CPU 0
---
> #define NDS_R2_BOTH_CPU 1
```

So `c247` is the published battle configuration plus the owner's stress arm and
nothing else. It is the gate configuration, not a stand-in for it — the same
argument `SHIP_CADENCE.md` §1 makes for `c245`, and the reason a published
target was built here at all.

---

## 2. Cadence on the shipping configuration

`scripts/probe-present-cadence.ps1 -Hits 1 -EndBreak mnVSResultsStartScene`,
i.e. **three GDB stops for a whole match**. DLDI on, DS console mode, JIT off,
`LimitFPS=false`, runner slot 6. 76.7 s of wall clock, 3% of the 2,700 s
ceiling.

| build | target | `TICK_HUD` | `SHIP_TELEM` | **pack** | `BOTH_CPU` | 2 | 3 | 4 | 5+ | presented | two-VBlank | max |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `c240-cadence-draw0` | tickhud | 1 (`DRAW=0`) | 0 | 1 | 1 | 1,939 | 95 | 7 | 2 | 2,043 | 94.91% | 19 |
| `c241-shipcadence` | proof | 0 | 1 | 1 | 1 | 1,955 | 82 | 4 | 2 | 2,043 | 95.69% | 18 |
| `c242-shipexact` | proof | 0 | 1 | 0 | 1 | 1,942 | 90 | 9 | 2 | 2,043 | 95.06% | 18 |
| `c245-pubgate` | published | 0 | 0 | 0 | 1 | 1,945 | 88 | 8 | 2 | 2,043 | 95.20% | 18 |
| **`c247-pubgate-packon`** | **published** | **0** | **0** | **1** | **1** | **1,966** | **71** | **4** | **2** | **2,043** | **96.23%** | **19** |
| `c244-shipboundary` | proof | 0 | 1 | 0 | 0 | 2,010 | 30 | 1 | 2 | 2,043 | 98.38% | 18 |

`viol=0` on every arm — no presented interval below two VBlanks anywhere.
`presented` is the guest's own `gNdsBattlePlayablePacingPresentedFrames` read
after the battle loop exited, so the denominator is the whole match. >=95% of
2,043 requires 1,941.

`python artifacts/performance/2026-08-17_ship-cadence/cadence.py` regenerates
this table from the captures; the `c247` row was appended to its `ARMS` list.

### 2.1 The isolation, one flag pair wide

`c245 -> c247` differ by the two battlepack defaults and nothing else — same
target, same stress arm, same invocation, generated headers identical apart
from those two lines and the two lines' downstream `BLOB_BYTES`/`DISPATCH`
constants.

```text
two-VBlank      1,945 -> 1,966     +21 frames
3-VBlank           88 ->    71     -17
4-VBlank            8 ->     4      -4
5+-VBlank           2 ->     2       0
total VBlanks   4,210 -> 4,186     -24
```

Internal consistency check: 17 frames saved one VBlank each and 4 saved two
each is 25 VBlanks; the measured total fell by 24, the missing one being the
5+ pair, which got **one VBlank worse** (24 VBlanks across the pair at max 18,
against 25 at max 19).

**Reported rather than buried: the single worst frame of the match is one
VBlank worse with the pack on** (max 19 against 18). Twenty-one frames moved
onto cadence and one already-bad frame got marginally worse. `viol` stays 0 and
the 5+ population stays at exactly 2 frames.

Whole-match timer ticks, same two runs, same 2,043 presented frames:
`gNdsBattlePlayablePacingTimerTicks` **2,370,832,576 -> 2,344,520,960**, i.e.
the pack removes **26,311,616 ticks from the match**, a mean of **12,879 ticks
per presented frame**. That is a whole-match mean over all 2,043 frames and is
*not* comparable to the banked `-34,304` rank-80 figure or the `+3,968` P50,
both of which are read over the 1,600-frame gameplay window on the tick-HUD
instrument. Three different windows, three different statistics; each is
labelled here so none of them gets quoted as the others.

### 2.2 The prediction, and where the model was wrong

```text
PREDICTED   1,958 of 2,043 = 95.84%,  declared range 1,950 - 1,966
MEASURED    1,966 of 2,043 = 96.23%   -- inside the range, at its upper edge
```

The prediction came from `SHIP_CADENCE.md` §2.1's isolation of the pack at
**+13 frames**, measured as `c242 -> c241` on the *proof* ROM, added to `c245`.
On the *published* ROM the same flag pair is worth **+21**.

`PREDICTION_C247.md` named the reason the range needed to be wide on exactly
this axis before the number existed: the +13 was measured with
`NDS_SHIP_TELEMETRY 1` and was being applied to a binary with it at 0, and
"the pack's benefit need not be exactly additive across that step". It is not.
The correction:

```text
pack, measured on the proof ROM      (SHIP_TELEMETRY 1)   +13 frames
pack, measured on the published ROM  (SHIP_TELEMETRY 0)   +21 frames
```

**The additive isolation chain in `SHIP_CADENCE.md` §2.1 is therefore valid
only within one telemetry arm, and the correction is that the pack is worth
~1.6x more on the shipping binary than the proof binary said.** The direction
is the benign one — the published ROM is the cheaper binary, so a frame sitting
marginally over the two-VBlank boundary is more likely to be rescued by a fixed
saving — but the *size* was not predictable from the proof arm, and any future
figure carried across that step should be treated as a lower bound rather than
an estimate.

This does not disturb `SHIP_CADENCE.md`'s own sum check (1,939 + 16 − 13 + 3 =
1,945), every term of which was measured inside its own arm.

---

## 3. The determinism control

The `c247` build writes the published ROM **and ELF** into the project root
(`Makefile:56`, `NDS_PUBLISH_USER_ROM`; `Makefile:2138` pins the basename
regardless of `BUILD`). The Makefile prints its own warning when this happens:

> `NOTE: BUILD=builds/build-c247-pubgate-packon is writing the PUBLISHED ROM
> smash64ds-battle-playable-hwtri.nds into the project root. Run
> make TARGET=smash64ds-battle-playable-hwtri with no overrides afterwards, or
> the root ROM stays this lab build.`

| stage | root `…-hwtri.nds` | bytes |
|---|---|---:|
| before anything (14:10 link, the flipped-default publish) | `5F3D1FE3…D20C` | 12,538,880 |
| after `make p1-tick` | `5F3D1FE3…D20C` | 12,538,880 |
| after the `c247` build | `171CE52C…BECB` | 12,530,688 |
| **after the no-override relink** | **`5F3D1FE3…D20C`** | **12,538,880** |

`smash64ds.nds` was never rebuilt and is `54C07FAC…C68A` at every stage.

**The relink had to be forced, and this is the trap worth writing down.** The
`c247` build leaves the root `.elf`/`.nds` *newer* than `builds/build`'s object
files, so a plain re-`make` finds the published outputs up to date, relinks
nothing, exits 0, and leaves the lab ROM sitting at the published path. Both
halves of the root pair were therefore deleted before the relink — both, never
one; restoring one half against a mismatched other is what produced a
`Connection timed out` mid-Boundary earlier in this campaign, because
`verify-battle-playable-realtime-harness.ps1:316` reads the root ROM while GDB
resolves breakpoints from the root ELF.

**The control is stronger than "the hash came back".** Both halves reproduced:

```text
.elf  DDB055C2EB4824B65D3EAF40D64B394FD92FEFB816F6769AAA5E6E7B816D9E10   both links
.nds  5F3D1FE3C78720CF666E5F5C8131BCC19158CF615413D8389568292B29D2D20C   both links
```

so the link is byte-reproducible from identical objects at an identical
`NDS_TASK10_GIT_SHORT` (`798007f`), and the 14:10 published ROM is confirmed to
be exactly what `builds/build` produces. Independently: the root ROM contains
**2** occurrences of `battlepack_fox`, against the **0** that
`SHIP_CADENCE.md` §3 measured on the pre-flip published ROM.

### 3.1 HEAD moved during this cycle, and `5F3D1FE3…` is now pinned to a parent commit

The flip was committed by the orchestrator while this cycle was running:

```text
798007f30d8  13:54:47  Re-bank the tick arm at the shipping defaults
603238b168e  14:46:20  Ship the battlepack: NDS_R2_BATTLEPACK and KEEP_CACHE default to 1
9d4bdf4fc4f  14:47:07  Record the discharged battlepack flip and correct SWITCH_READY …
```

Two consequences, both checked rather than assumed.

**Nothing measured here was invalidated.** `git diff 798007f30d8..HEAD` touches
`Makefile`, `docs/HANDOFF.md` and `docs/P1_EXECUTION_BOARD.md`, and the only
non-comment `Makefile` change is exactly the two lines under test:

```text
-NDS_R2_BATTLEPACK ?= 0            +NDS_R2_BATTLEPACK ?= 1
-NDS_R2_BATTLEPACK_KEEP_CACHE ?= 0 +NDS_R2_BATTLEPACK_KEEP_CACHE ?= 1
```

No other flag moved, and the flip was already applied in the working tree when
every build in §1 ran, so each build's flag set equals the committed tree's.

**But the published ROM hash no longer reproduces from `HEAD`.**
`Makefile:1601` compiles `NDS_TASK10_GIT_SHORT` in from
`git rev-parse --short=7 HEAD`. Every build and the determinism relink in §3
ran at `798007f`; `HEAD` is now `9d4bdf4`. So:

```text
5F3D1FE3…D20C  reproduces from 798007f30d8 -- proven twice, ROM and ELF
               does NOT reproduce from 9d4bdf4fc4f -- the stamp differs
```

The root ROM currently on disk *is* `5F3D1FE3…` and is the one Boundary
validated in §5. The next relink at the current `HEAD` will produce a
different, equally valid published ROM with a new hash. **Anything that pins
`5F3D1FE3…` as "the published hash" must name `798007f30d8` beside it, or it
will read as a reproducibility failure the first time someone rebuilds.**
This is the same mechanism `SHIP_CADENCE.md` §6 recorded; it has now fired.

---

## 4. The acceptance soak

`scripts/soak-freeze-watch.ps1 -Build build-c248-soak-packon -MinutesToRun 11
-PollSeconds 5 -IdenticalFramesToTrip 16 -PressStartSeconds 60
-PressStartOnResults -RunnerSlot 6 -SaveFramesTo … -JsonOut …`

The shape is the one that passed the 2026-08-13 R2-07 acceptance battery
(`../2026-08-13_c-stress/STRESS_GATE.md`, run 2): detected presses rather than
timed ones, so a START tap can only ever land on Results and never pause a live
match. Build config: `NDS_R2_BATTLEPACK 1`, `NDS_R2_BATTLEPACK_KEEP_CACHE 1`,
`NDS_R2_BOTH_CPU 1`, `NDS_R2_SOAK_MATCH_MINUTES 0` — the canonical one-minute
match, which is the owner's 2026-08-05 ruling for both gate arms. 660 s of wall
clock, 132 distinct frames, freeze threshold 80 s.

### 4.1 Verdict and the battery's own four questions

```text
verdict: NO-FREEZE
```

| SwitchPlan §7 asks | counter | value |
|---|---|---:|
| whole stress match start to finish | `gNdsSCVSBattlePlacementInitCount` | **12** battle-scene entries |
| START at Results restarts the match | `gNdsVSResultsRematchCount` | **7** |
| successive matches | `gNdsVSResultsStartCount` | **8** completed matches |
| Sudden Death exercised | `gNdsSCVSBattleSuddenDeathPrepareCount` | **4** |

**Sudden Death is measured, not inferred.** `scVSBattleStartSuddenDeath` ran
four times. 8 matches + 4 Sudden Deaths = 12 battle entries, and
`gNdsRendererSceneTextureVramResetCount` independently reads **12**, so the
scene-owned texture-VRAM reset ran on every one of them;
`gNdsRendererBattleStaticTextureViolationCount` is **0**, so no static cache
was discarded while still marked prepared.

The battery asked for three successive matches. This run produced **eight**,
and the harness's own closing line explains why it stopped there rather than at
a limit: *"battle completed and Results is up. A passive soak CANNOT reach
match 9: Results exits on START only."* The match timer was confirmed in-guest
at 1 minute.

### 4.2 The counters the flip makes load-bearing

`NDS_R2_BATTLEPACK_KEEP_CACHE=1` grows the taskman arena, so these are the
numbers that decide whether the flip is safe rather than merely fast.

| counter | measured | threshold | margin |
|---|---:|---:|---:|
| `gNdsTaskmanArenaChosenSize` | **1,548,288** | 1,564,672 grantable ceiling (`Makefile:353-358`) | **16,384 B under** |
| `gNdsTaskmanArenaAllocFailCount` | **0** | must be 0 | — |
| `gNdsR2AnimCacheArenaReserveFailCount` | **0** | must be 0 | — |
| `gNdsR2AnimCacheRejects` | **0** | must be 0 | — |
| `gNdsSyMallocOverflowCount` | **0** | must be 0 | — |
| `gNdsTaskmanGeneralHeapFreeMin` | **52,768** | 32,768 floor | **+20,000 B** |
| `gNdsBattlePackLoadFails` | **0** | must be 0 | — |

`ChosenSize` is the requested `0x17A000` exactly — the arena search did not
fall back — and it sits 16,384 B under the measured grantable ceiling, which is
the margin `Makefile:353-358` documents rather than a surprise. The
general-heap low-water clears the mandated floor by 20,000 B and never
approached the 25,600 B `ifCommonSetMaxNumGObj` cap.

### 4.3 The pack's own engagement proof and negative control

| counter | value | reading |
|---|---:|---|
| `gNdsBattlePackHits` | **1,938** | acquisitions the resident pack served — the pack was live |
| `gNdsBattlePackLoadSteps` | **216** | streamed 16 KB chunks; the blob actually arrived |
| `gNdsBattlePackMisses` | **1,523** | **the negative control** — only one fighter is resident, so misses MUST stay non-zero |
| `gNdsBattlePackDrops` | **12** | one reclaim per battle entry, matching `PlacementInitCount` |
| `gNdsBattlePackLoadFails` | **0** | the blob never failed to arrive |
| `gNdsBattlePackState` | **0** | `NDS_BATTLEPACK_STATE_UNCHECKED` (`nds_battlepack_anim.h:109`) |

`State 0` is the post-teardown idle value, read at the Results screen after the
twelfth drop; the **failure** states are 2/3/4 (`BAD_MAGIC` / `BAD_VERSION` /
`BAD_EXTENT`) and none of them appeared. `ResidentBytes 0` and `Clips 0` are
the same post-drop reading. Residency during play is evidenced by `Hits` and
`LoadSteps`, not by these end-of-run values.

### 4.4 The heap price, confirmed against a pack-off control

`Makefile:345-347` predicted, before any of this was run, that the pack costs
**17,600 B** of general heap. Two independent measurements, on different
instruments and different run shapes:

| pair | pack off | pack on | delta |
|---|---:|---:|---:|
| `c239` / `c246` tick arm, 1,600-frame window (`SHIP_CADENCE.md` §4) | 70,736 | 53,136 | **17,600** |
| 2026-08-13 soak (3 matches) / this soak (8 matches) | 70,384 | 52,768 | **17,616** |

Two runs that share no instrument, no window and no match count agree to
**16 bytes** with a prediction written before either.

### 4.5 One non-zero counter pair, controlled rather than waved at

`gNdsAudioFgmReadFailCount` **8** and `gNdsAudioFgmPlayFailCount` **8** — one
per completed match. The 2026-08-13 pack-off soak read **3** and **3** across
**3** matches. Identical per-match rate on both arms, so this is a pre-existing
per-match audio FGM read failure that scales with match count and is
**independent of the flip**. `OpenFail`, `FormatFail`,
`IncludedLookupFail`, `PoolExhaust`, `GenerationMismatch`, `PrematureRetire`
and `StaleStop` are all 0. It is not this cycle's row; it is recorded here so
the next reader does not have to re-derive that it is not a regression.

### 4.6 What the soak's pacing histogram is NOT

The soak's end-of-run dump includes
`gNdsBattlePlayablePacingPresentIntervalBucket[]` reading 2=1,839 3=190 4=12
5+=2 over 2,043 presented frames, max 18, `viol=0`. **That is not a cadence
figure and must not be quoted as one.** It is the *eighth successive match* on
the **tick-HUD instrument with the HUD drawn** (`NDS_TICK_HUD_DRAW 1`), under
a screen capture every 5 s on the interactive desktop. `SHIP_CADENCE.md` §2.1
prices the instrument apparatus alone at 16 frames on the arm that does *not*
draw the HUD. The cadence number for this cycle is `c247`'s 96.23%, taken on
the published binary with three GDB stops and no capture.

Its useful content is the parts that are apparatus-independent: `viol=0` and
max 18 after eight matches and four Sudden Deaths, i.e. nothing pathological
had accumulated by the end of an eleven-minute chain.

---

## 5. Boundary

```powershell
cmd /c "pwsh -NoProfile -ExecutionPolicy Bypass -File scripts\verify-all.ps1 -Profile Boundary -RunnerSlot 3 > …\boundary-packon.log 2>&1"
```

Nothing else was running: `melonDS`, `arm-none-eabi-gdb` and `make` were all
confirmed absent before launch.

```text
verdict     Boundary verification profile passed.
exit code   0
log         18,964,416 bytes captured whole
Exception:  0 occurrences across the entire file
DECOMP_PRISTINE=PASS pinned_historical_files=10 ds_markers=0 decomp_patch_pipeline=absent
```

**The redirect is `cmd`'s, not `Tee-Object`.** `docs/VERIFYING.md`
("Checkpoint Choice") records that `verify-all.ps1:167` writes each child
verifier's stdout with `[Console]::Out.Write`, straight to the console handle,
so a PowerShell pipeline captures the driver's three lines and none of the run
— which on 2026-08-15 produced a 90-byte log for a failing Boundary and cost
two repeats. The brief asked for a Tee; the repo rule wins, and it is the rule
that actually delivers what the brief wanted, an 18.9 MB whole-run file that
can be grepped for a multi-line throw.

The `Exception:` sweep matters independently of the verdict: Boundary executes
more checks than its registry lists, and a child that throws has previously
been visible only as an `Exception:` line inside the child's captured stdout.
Zero, over the whole file.

Checks whose own result lines are worth quoting:

```text
Toolchain usable: recursive make OK, DEVKITPRO=C:/devkitPro DEVKITARM=…/devkitARM
Harness registry check passed: 3 harness mappings, 4 verifier scripts, 0 drift.
Running verifier: battle_playable_realtime [verify-battle-playable-realtime-harness.ps1]
Task 9 float ITCM passed: … itcm=31904/32768 free=864 omitted=[]
Published ROM contract passed: smash64ds-battle-playable-hwtri.nds, smash64ds.nds
battle_playable Pupupu realtime pacing smoke passed: frames=212 fps=288/573
    ticks=246632384 rprof=0 gxram=465/1096 gxstat=0x6000000/ctrl=0x19 … aobj32=40/293/reuse8/fail0
FTANIM_DENSE_BANK=OK
M3_NATIVE_STAGE_CHECK_OK … sha256=eda2dbd6…5942c
```

**"Published ROM contract passed" is the load-bearing one for this cycle**: the
realtime harness reads the *root* `smash64ds-battle-playable-hwtri.nds`
(`verify-battle-playable-realtime-harness.ps1:316`), so Boundary was exercising
the restored `5F3D1FE3…` pack-on published pair, not a lab build. Both root ROMs
were re-hashed after the run and are unchanged.

`itcm=31904/32768 free=864` is unmoved by the flip — the battlepack is a NitroFS
payload and an arena reservation, not ITCM residency, so the rank-80 ITCM
knapsack banked on 2026-08-17 is undisturbed.

---

## 6. What this does NOT say

- **It does not declare the P1 performance gate passed.** It measures the
  cadence arm at **96.23%** on the published battle configuration under the
  owner's both-CPU stress arm. The `>=95%` verdict, and the choice of that
  population, are the owner's — recorded in `SHIP_CADENCE.md` §4.1.
- **It does not re-bank the tick arm on the flip.** The `-34,304` at rank-80
  and the resulting `-18,095` requirement are `SHIP_CADENCE.md` §4's figures,
  measured on the `c239`/`c246` pair over the 1,600-frame window on the
  tick-HUD instrument. This cycle rebuilt that instrument on the flip but did
  **not** run a 1,600-sample gate reading on it. Whoever needs a *sizing* basis
  for the next candidate needs that run; the whole-match mean in §2.1 is not a
  substitute for it.
- **It does not touch `NDS_R2_PATH`.** R2-08 stays HELD per the owner's
  2026-08-17 ruling.
- **It does not commit, push, or snapshot.** The `Makefile` flip remains
  uncommitted in the working tree, as do the owner's other dirty files.
- **It does not measure retail hardware, audio fidelity, or the visual arms**
  of the switch plan's §6 acceptance list.
- **It does not claim the pack is free.** It costs 17,600 B of general heap and
  172,032 B of taskman arena (1,376,256 -> 1,548,288), leaving 16,384 B under
  the measured grantable ceiling. That margin is real but thin, and any future
  arena growth has to be priced against it rather than against boot headroom —
  `check-boot-headroom.ps1` meters the static image and cannot see this.
- **The +21-frame isolation is one measurement, not a repeat.** Each cadence
  arm here was run once. The probe is guest-deterministic (`SHIP_CADENCE.md`
  §0 measured 3 stops and 2,038 stops producing identical histograms), which is
  why one run is accepted, but no arm was re-run to prove self-reproduction.
- **It does not prove the soak's eight matches are repeatable.** One soak was
  run. The battery asks for three matches and got eight, so the margin is
  large, but a second session was not spent to show the count is stable.
- **`gNdsBattlePackMisses` 1,523 is expected, not a defect** — only one fighter
  is resident by design — but this cycle did **not** measure what those 1,523
  fall-throughs cost. That is the slice-2 question, untouched here.

---

## 8. Where this evidence lives, and a hygiene note

`.gitignore:44` ignores `/artifacts/`, and the 2026-08-17 evidence files —
`SHIP_CADENCE.md` included — are **untracked**. They are working-tree evidence,
not committed history. Every file this cycle wrote is therefore invisible to
`git status` and will not be swept into any commit; committing them needs an
explicit `git add -f`. Recorded because the natural check ("`git status` is
clean, so nothing was left behind") is misleading in this directory in both
directions.

---

## 7. Reproduce

```powershell
# the instrument, flag-identical to the published ROM
make p1-tick

# the gate arm -- BACK UP BOTH ROOT ROMs FIRST, this writes the published pair
make TARGET=smash64ds-battle-playable-hwtri BUILD=build-c247-pubgate-packon NDS_R2_BOTH_CPU=1
pwsh -NoProfile -File scripts\probe-present-cadence.ps1 `
     -Build build-c247-pubgate-packon -Target smash64ds-battle-playable-hwtri `
     -Hits 1 -EndBreak mnVSResultsStartScene -TimeoutSeconds 2700 -RunnerSlot 6 `
     -Artifact artifacts\performance\2026-08-17_ship-cadence\c247-cadence.txt

# restore the published pair -- DELETE BOTH HALVES FIRST or make relinks nothing
Remove-Item .\smash64ds-battle-playable-hwtri.nds,.\smash64ds-battle-playable-hwtri.elf
make TARGET=smash64ds-battle-playable-hwtri

# the acceptance soak
pwsh -NoProfile -File scripts\soak-freeze-watch.ps1 -Build build-c248-soak-packon `
     -MinutesToRun 11 -PollSeconds 5 -IdenticalFramesToTrip 16 `
     -PressStartSeconds 60 -PressStartOnResults -RunnerSlot 6 `
     -JsonOut artifacts\performance\2026-08-17_ship-cadence\soak-packon.json

# Boundary -- cmd's redirect, NOT Tee-Object (docs/VERIFYING.md, Checkpoint Choice)
cmd /c "pwsh -NoProfile -ExecutionPolicy Bypass -File scripts\verify-all.ps1 -Profile Boundary -RunnerSlot 3 > artifacts\performance\2026-08-17_ship-cadence\boundary-packon.log 2>&1"

# the table, no emulator needed
python artifacts\performance\2026-08-17_ship-cadence\cadence.py
```
