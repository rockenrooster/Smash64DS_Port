# R2-08 landed: the Boundary configuration is produced by the Runtime 2 battle path, both published ROMs are rebuilt from it, and the pin is updated in the same change

**Date:** 2026-08-17 · **Branch:** `codex/r2-runtime2` · **HEAD `e419cf819f5`**
**5 builds** (published pair via `make p1`, `build-c249-r1ctl`,
`build-c250-pubgate-r2`, `build-c251-soak-r2-1min`, `build-c252-soak-r2-5min`),
**Boundary + 3 emulator runs**,
**3 production edits: two Makefile blocks and one drift fix. Nothing committed.**
**UNITS: 1 project tick = 1 `cpuGetTiming()` tick = 2 ARM9 cycles.**

`SHIP_CADENCE.md` §5 recorded the hold — *"R2-08 (`NDS_R2_PATH=1`) is HELD until
the gate settles"*. `BATTLEPACK_FLIP.md` settled it: 96.23% two-VBlank on the
published target under the owner's both-CPU stress arm, margin +25 frames,
soak NO-FREEZE over eight matches, Boundary green.

**HEAD moved during this cycle**, from `9d4bdf4fc4f` to `e419cf819f5`
(*"Pin the published ROM to its build commit and record a helper defect"*).
Every build here carries the `e419cf8` stamp. That commit's own finding is
load-bearing for §5 and is honoured there rather than rediscovered.

---

## 0. Outcome

```text
THE SWITCH      Makefile, two blocks: `override NDS_R2_PATH := 1` behind an
                NDS_R2_LAB_R1_PATH escape hatch, plus a results-lab guard the
                runbook does not have and without which the build breaks.
                NDS_R2_PATH ?= 0 at Makefile:685 is UNCHANGED -- the family
                default stays off and the published targets carry the override.

RESOLUTION      published 1 · tick-HUD 1 · proof 1 · results-lab 0 · P2 0
                published + NDS_R2_LAB_R1_PATH=1 -> 0
                Tick-HUD parity: 54 flags, 2 allowlisted, 0 drift -- green
                before the edit, after it, and after every build.

ENGAGEMENT      SAME TARGET, BOTH DIRECTIONS.  Boundary's own proof build
                (PATH=1) has ndsR2BattleRun + all 8 ndsR2Host*; build-c249-r1ctl,
                the same target with the escape hatch, has ZERO of either --
                while ndsBattlePlayableFrameCompleteMarker, main and
                ndsPlatformReadInput read 1 in BOTH, so the control is a real
                ELF rather than an unreadable one.  The root published ELF
                carries ndsR2BattleRun at 0x0208840c.

BOUNDARY        Boundary verification profile passed.   exit 0
   (§6 item 1)  18,964,809 bytes captured whole; Exception: 0, =FAIL 0,
                FAILED 0 across the ENTIRE file.
                DECOMP_PRISTINE=PASS ds_markers=0 decomp_patch_pipeline=absent
                "Published ROM contract passed" -- and that check reads the ROOT
                ROM, which IS the R2 ROM, so Boundary's screenshot arm is now
                real R2 coverage.  That closes the gap SWITCH_READY §3 named.

CADENCE         1,962 / 2,043 two-VBlank = 96.03%   viol=0   max 18
   (§6 item 3)  against c247, the same arm on Runtime 1: 1,966 = 96.23%, max 19.
                >=95% needs 1,941, so the margin is +21 frames (was +25).
                -4 frames of 2,043 = -0.20 points; 4- and 5+-VBlank populations
                IDENTICAL; the worst frame of the match IMPROVED 19 -> 18;
                total VBlanks +3 of 4,186 (+0.07%).
                REPORTED AS NO REGRESSION, NOT AS A WIN -- and not as a clean
                null: the two arms were built at different HEADs, so this is not
                an at-HEAD control.  §4 names the arm that would settle it and
                records that it was not run.

SOAK            1-MINUTE ARM: NO-FREEZE.  Match timer confirmed IN-GUEST at
   (§6 item 4)  1 minute.  6 completed matches (battery asks >=3), 6 START
                rematches, 4 Sudden Deaths read from
                gNdsSCVSBattleSuddenDeathPrepareCount, 11 battle entries =
                1 + 6 + 4 exactly, and VramResetCount independently 11.
                Arena 1,548,288 (16,384 B under the grantable ceiling), heap
                low-water 51,904 (+19,136 over the floor), every allocator,
                panic, runaway and cadence-violation counter 0.
                5-MINUTE ARM: NO-FREEZE, exit 0, on the second attempt.  Match
                timer confirmed IN-GUEST at 5 minutes and time_limit=5 read from
                the guest's own battle state.  2 completed 5-minute matches, 2
                START rematches, 3 entries = 1 + 2, VramReset 3.  Same counters
                clean; heap low-water 52,368.
                THE FIRST ATTEMPT PRINTED "verdict: NO-FREEZE" AND THEN
                OVERRODE ITSELF TO SOAK-UNDERCOVERED (exit 2): I ran it passive,
                so the match ended at t+333 s of 720 and the tail watched
                Results.  My '^verdict:' grep could not see the correction --
                it has no colon -- and the EXIT CODE is what caught it.  Re-run
                in the rematch shape on the SAME ROM, no rebuild.
                ONE COUNTER PAIR IS NON-ZERO AND IS NOT SMOOTHED OVER:
                gNdsR2AnimCacheArenaOverflows/Rejects read 0 on the 1-minute arm
                and 2 then 6 on the 5-minute runs.  It tracks MATCH DURATION,
                not wall time, entry count, or the loop selector -- the 1-minute
                arm did MORE entries in the same wall time with zero.  Nothing
                failed to allocate; the cache declined to grow.  Whether the
                SWITCH causes it is NOT established (no R1 5-minute control),
                and the shipping one-minute configuration reads 0.  §7.3.

PUBLISHED       smash64ds-battle-playable-hwtri.nds
                  5F3D1FE3...D20C  12,538,880  ->  2F47C8AC...CB2F  12,530,688
                smash64ds.nds unchanged 54C07FAC...C68A (P2, not part of P1).
                The ROM SHRANK 8,192 B: the Runtime 1 loop is no longer
                compiled into this arm.
                Restored bit-exactly after the cadence build overwrote it --
                BOTH halves, .nds and .elf.

THE PIN         DECOMP_PIN.txt was stale by two generations.  Updated, and it
                now also records OUTPUT_BUILT_AT_COMMIT, because
                NDS_TASK10_GIT_SHORT is HEAD-derived and compiled into the
                image: the published hash changes on every commit, including
                the one that lands this change.  That is commit e419cf8's own
                correction, applied rather than rediscovered.

NOT MINE        §6 item 2 (visual) and item 5 (retail) are the owner's.  §6
                supplies a before/after pair on the same capture arm and claims
                nothing.
```

---

## 1. The switch, and the flag-identity proof

Two Makefile insertions and one drift fix. Both blocks or neither:
`check-tickhud-parity.ps1` compares `make print-benchmark-flags` for the
published and tick-HUD targets and allows exactly two keys to differ, and
`BENCH_MAKE_R2_PATH` is emitted at `Makefile:4203`, so a one-sided edit fails it.

```make
ifneq ($(NDS_R2_LAB_R1_PATH),1)
override NDS_R2_PATH := 1
endif
```

- **published block**, `Makefile:1684-1691`, after `override NDS_TASK36_HW_COMPOSE := 2`.
- **tick-HUD/proof block**, `Makefile:1924-1939`, same insertion **wrapped in
  `ifneq ($(TARGET),smash64ds-results-lab-hwtri)`**.

`NDS_R2_PATH ?= 0` at `Makefile:685` stays 0: SwitchPlan §5 keeps the family
default off and the published targets carry the override. The `override` form
defeats a command-line 0, so the Runtime 1 oracle needs an escape hatch or a
control arm would silently build identical to the candidate — that is
`NDS_R2_LAB_R1_PATH`, and §2 uses it.

### 1.1 The trap the runbook missed, and it would have broken the build

`SWITCH_READY.md` §6.1 prescribes the same line in both blocks and stops there.
The tick-HUD/proof block's filter (`Makefile:1887`) also covers
**`smash64ds-results-lab-hwtri`**, whose harness is overridden to
`results_playable` (`Makefile:1896-1900`). Both `taskman_seam.c:16-18` and
`src/nds/r2/nds_r2_battle.c:45-47` `#error` when `NDS_R2_PATH=1` under any
harness but `battle_playable`. An unguarded insertion stops the Results lab
compiling. `check-tickhud-parity.ps1:27-28` compares only the published and
tick-HUD targets, so the exclusion cannot hide drift.

### 1.2 Resolution, measured per target rather than asserted

`make print-benchmark-flags` (outside the build graph, builds nothing):

| target | `BENCH_MAKE_R2_PATH` |
|---|---:|
| `smash64ds-battle-playable-hwtri` (published P1) | **1** |
| `smash64ds-battle-playable-tickhud-hwtri` (the instrument) | **1** |
| `smash64ds-battle-playable-proof-hwtri` (GDB proof) | **1** |
| `smash64ds-results-lab-hwtri` | **0** — the guard |
| `smash64ds` (P2) | **0** — family default untouched |
| published **+ `NDS_R2_LAB_R1_PATH=1`** | **0** — the escape hatch works |

```text
Tick-HUD parity passed: 54 make flags compared, 2 allowlisted differences, 0 drift.
```

Green **before** the edit and **after** it, and again after every build in this
cycle. Also green: `check-gbi-decode-fixtures.ps1` (which pins the
`SHIP_TELEMETRY`/`TICK_HUD` contiguous run at `Makefile:1906-1911` that the
insertions deliberately avoid), `check-harness-registry.ps1` (3 mappings, 4
verifier scripts, 0 drift), `check-melonds-policy.ps1`.

### 1.3 The generated configs, as diffs

Pre-switch published config against post-switch published config:

```text
93c93
< #define NDS_R2_PATH 0
> #define NDS_R2_PATH 1
187c187
< #define NDS_TASK10_GIT_SHORT "798007f"
> #define NDS_TASK10_GIT_SHORT "e419cf8"
```

Post-switch published against the tick-HUD sibling — **one line**:

```text
9c9
< #define NDS_TICK_HUD 0
> #define NDS_TICK_HUD 1
```

So the instrument every measurement runs on is flag-identical to the published
ROM apart from its own switch. That is a diff, not an assertion.

### 1.4 The drift fix

`SWITCH_READY.md` §2 found one divergence between the Runtime 1 loop and the R2
host surface: `666e99a2148` added
`gNdsPositionProbeUpdateInPresent = update_in_iteration;` to Runtime 1
(`taskman_seam.c:8066`) and not to `ndsR2HostBattleUpdateOnce`, which took only
the hurtbox capture. Fixed at `taskman_seam.c:5332-5338`, in the same position
Runtime 1 publishes it — immediately before `battle_status_before` is read.

**This changes no shipped binary.** `NDS_R2_POSITION_PROBE ?= 0`, and the global
itself is declared inside the same `#if` (`taskman_seam.c:28-35`), so with the
flag at 0 the global and the assignment compile out together. `nm` finds it in
neither the published ELF nor the control, which is the expected reading rather
than a missing symbol.

---

## 2. Engagement, both directions, before any number was read

The R2-06 E0 rule. Same target, same `nm`, opposite results.

| ELF | `NDS_R2_PATH` | `ndsR2Battle*` | `ndsR2Host*` |
|---|---:|---:|---:|
| `builds/build-battle-playable-proof-hwtri-harness/…proof-hwtri.elf` (Boundary's own build) | 1 | **1** | **8** |
| `builds/build-c249-r1ctl/…proof-hwtri.elf` (**same target**, `NDS_R2_LAB_R1_PATH=1`) | 0 | **0** | **0** |
| root `smash64ds-battle-playable-hwtri.elf` (the published ROM) | 1 | **1** (`ndsR2BattleRun` @ `0x0208840c`) | **8** |

**The control could have failed, and it is proven to be a real ELF rather than
an unreadable one.** Symbols that must exist on both paths read 1 in both:
`ndsBattlePlayableFrameCompleteMarker`, `main`, `ndsPlatformReadInput`. The
control carries 13,816 symbols and links at 10,296,528 B. Only the
discriminators moved.

Independent confirmations on the published ROM:

- generated `nds_build_config.h` reads `#define NDS_R2_PATH 1`;
- `gNdsBattlePlayablePacingPresentIntervalBucket` is present at `0x0222d7e4`,
  which is the counter §4's probe reads;
- `battlepack_fox` appears **2** times in the root ROM, against the **0**
  `SHIP_CADENCE.md` §3 measured on the pre-flip ROM — the committed pack default
  is live in what ships;
- the ROM **shrank 8,192 B** (12,538,880 → 12,530,688), which is the direction
  the switch requires: `taskman_seam.c:7952` selects `ndsR2BattleRun()` and the
  ~1,150-line Runtime 1 loop is no longer compiled into this arm.

---

## 3. §6 item 1 — Boundary verifier green on the Runtime 2 battle path

```powershell
cmd /c "pwsh -NoProfile -ExecutionPolicy Bypass -File scripts\verify-all.ps1 -Profile Boundary -RunnerSlot 3 > …\boundary-r2path.log 2>&1"
```

Nothing else was running: `melonDS`, `arm-none-eabi-gdb` and `make` all
confirmed absent before launch, and no published target was built during it.

```text
verdict     Boundary verification profile passed.
exit code   0
log         18,964,809 bytes captured whole
Exception:  0 occurrences across the entire file
=FAIL 0     FAILED 0     "failed" 0
DECOMP_PRISTINE=PASS pinned_historical_files=10 ds_markers=0 decomp_patch_pipeline=absent
```

**The redirect is `cmd`'s, not `Tee-Object`.** `docs/VERIFYING.md:399` records
that `verify-all.ps1:167` writes each child verifier's stdout with
`[Console]::Out.Write`, straight to the console handle, so a PowerShell pipeline
captures the driver's lines and none of the run — which once produced a 90-byte
log for a failing Boundary. The brief asked for a Tee; the repo rule wins, and it
is the rule that actually delivers what the brief wanted: a whole-run file that
can be grepped for a multi-line throw.

The only case-insensitive `error` hit in 18.9 MB is a checker's own heading,
line 5: `R2-03 E64b/E65 fixed-point cubic -- host error bound vs the decomp float`.

Result lines worth quoting:

```text
Published ROM contract passed: smash64ds-battle-playable-hwtri.nds, smash64ds.nds
Harness registry check passed: 3 harness mappings, 4 verifier scripts, 0 drift.
Task 9 float ITCM passed: … itcm=31904/32768 free=864 omitted=[]
battle_playable Pupupu realtime pacing smoke passed: frames=212 fps=288/573
    ticks=246629440 rprof=0 gxram=465/1096 gxstat=0x6000000/ctrl=0x19 … aobj32=40/293/reuse8/fail0
M3_NATIVE_STAGE_CHECK_OK … sha256=eda2dbd6…5942c
```

**"Published ROM contract passed" is the load-bearing line for this cycle.** The
realtime harness reads the *root* `smash64ds-battle-playable-hwtri.nds`
(`verify-battle-playable-realtime-harness.ps1:316`), and that ROM is now the R2
ROM — so Boundary's screenshot arm is real R2 coverage. **That closes the gap
`SWITCH_READY.md` §3 named**, where the GDB arm carried the R2 path but the
screenshot arm captured a `NDS_R2_PATH 0` published ROM.

`itcm=31904/32768 free=864` is unmoved by the switch, so the rank-80 ITCM
knapsack banked on 2026-08-17 is undisturbed. The pacing smoke reads
`ticks=246,629,440` against `BATTLEPACK_FLIP.md`'s pre-switch `246,632,384` —
**−2,944 on a 246M-tick figure**, i.e. flat, which is what §6.4 predicts.

Both root ROMs were re-hashed after the run and are unchanged.

---

## 4. §6 item 3 — cadence on the switched published ROM

`build-c250-pubgate-r2` is `TARGET=smash64ds-battle-playable-hwtri` with
**`NDS_R2_BOTH_CPU=1` and nothing else** on the command line. Its generated
config against the published one is a one-line diff:

```text
77c77
< #define NDS_R2_BOTH_CPU 0
> #define NDS_R2_BOTH_CPU 1
```

**That is the gate configuration, not a stand-in for it.**
`scripts/probe-present-cadence.ps1 -Hits 1 -EndBreak mnVSResultsStartScene`, i.e.
three GDB stops for a whole match. DLDI on, DS console mode, JIT off, runner
slot 6. Denominator is the guest's own `gNdsBattlePlayablePacingPresentedFrames`
read after the battle loop exited, so it is the whole match.

| build | path | 2 | 3 | 4 | 5+ | presented | two-VBlank | max | viol |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| `c247-pubgate-packon` | **R1** | 1,966 | 71 | 4 | 2 | 2,043 | **96.23%** | 19 | 0 |
| **`c250-pubgate-r2`** | **R2** | **1,962** | **75** | **4** | **2** | **2,043** | **96.03%** | **18** | **0** |

```text
PCADHIST DONE n=1 2=1962 3=75 4=4 5=2 max=18 min=2 viol=0 vbl=4189 pres=2043 tk=2346416512
```

**Read this as no regression, not as a win, and not as a clean null.** ≥95% of
2,043 requires 1,941, so the margin is **+21 frames** against c247's +25. The
switch moved **−4 frames of 2,043 (−0.20 points)**; four frames left the
two-VBlank bucket for three-VBlank. Against that: `viol=0`, the 4- and
5+-VBlank populations are **identical** (4 and 2), the **worst frame of the match
improved, max 19 → 18**, and total VBlanks moved 4,186 → 4,189, **+3 of 4,186 =
+0.07%**.

**The honest caveat, stated rather than buried: this is not an at-HEAD control.**
`c247` was built at HEAD `798007f` and `c250` at `e419cf8`, so
`NDS_TASK10_GIT_SHORT` differs and the two binaries do not share a link
placement. SwitchPlan §7 and R2-06 E0 both say the switch cannot move the
histogram, because all fourteen non-zero `NDS_R2_*` renderer overrides ship on
both sides and only the loop selector changes; R2-06 E0 measured its tick deltas
inside the placement floor and pointing in opposite directions. A −4/+3 move of
this size is the shape of placement, not of work — **but this cycle did not run
the arm that would prove it**, namely the published target at *this* HEAD with
`NDS_R2_LAB_R1_PATH=1 NDS_R2_BOTH_CPU=1`. That arm is one build and one probe
run, and it is the first thing the next cycle should spend if the −4 matters to
anyone. Nothing here is sized against it.

---

## 5. The published pair, the pin, and the determinism control

`make p1` (`Makefile:3497`) — the published ROM then the tick-HUD sibling, two
sequential sub-makes, no `-j`, `MAKEFLAGS` untouched.

| | bytes | SHA-256 |
|---|---:|---|
| `smash64ds-battle-playable-hwtri.nds` **before** | 12,538,880 | `5F3D1FE3C78720CF666E5F5C8131BCC19158CF615413D8389568292B29D2D20C` |
| `smash64ds-battle-playable-hwtri.nds` **after** | **12,530,688** | **`2F47C8AC0730B0DAC6AA1B7A482B8599F33CA3E831B3AB6A7B0AA974A6ACCB2F`** |
| `smash64ds-battle-playable-hwtri.elf` **after** | 10,285,736 | `54338F7A7FD2419C5A9446D47642E64BF5FAA50EE2CCDEC97817A06B15BE0967` |
| `smash64ds.nds` (never rebuilt) | 11,915,264 | `54C07FAC80C50418949908701F7C2BDBF27512C5F96AC09086FABBB0DF6AC68A` |
| tick-HUD sibling, `builds/build-tick-hud-buckets` | 12,545,024 | — |

`smash64ds.nds` is the P2 ROM and is not part of P1; it was deliberately not
rebuilt, and `NDS_R2_PATH` resolves to 0 for it either way.

**The determinism control passed, on both halves.** The `c250` cadence build
writes the published pair into the project root, so after that run **both halves
were deleted** — both, never one; restoring one against a mismatched other is
what produced a `Connection timed out` mid-Boundary earlier in this campaign —
and the pair was relinked from the `make p1` objects:

```text
.nds  2F47C8AC…CB2F   12,530,688   identical to the make p1 link
.elf  54338F7A…0967   10,285,736   identical to the make p1 link
```

Deleting both halves first is not optional: commit `e419cf8` records that the
Makefile's own printed advice (*"Run `make TARGET=…` with no overrides
afterwards"*) does not work, because a lab build leaves the root pair newer than
the objects, so the re-make relinks nothing, exits 0, and leaves the lab ROM at
the published path.

### 5.1 The pin, and why it now carries a commit

`DECOMP_PIN.txt:20-22` was stale by two generations — it read the 11,428,864-byte
`4D795B4E…A090` ROM. Updated from this build's own output, and
`scripts/check-published-roms.ps1` passes.

**A new key was added, and it is the correction commit `e419cf8` asks for.**
`NDS_TASK10_GIT_SHORT` is `git rev-parse --short=7 HEAD` (`Makefile:1601`) and is
compiled into the image, so **the published ROM's hash changes on every commit**.
A bare hash therefore reads as a reproducibility failure the moment anything
lands — including the commit that lands this very change. The pin now records:

```text
OUTPUT_BUILT_AT_COMMIT=e419cf819f56980c2b270df659f05e91a6703ae9
OUTPUT_NAME=smash64ds-battle-playable-hwtri.nds
OUTPUT_BYTES=12530688
OUTPUT_SHA256=2F47C8AC0730B0DAC6AA1B7A482B8599F33CA3E831B3AB6A7B0AA974A6ACCB2F
```

`build.ps1:36-59` skips `#` comments and requires a fixed key list while
tolerating extra keys; the edited file was run through that parser and yields 22
keys with 0 required keys missing. `build.ps1:616` only `Write-Warning`s on a
mismatch, so a stale pin never stops a build — it only makes the board's
*"Reproducible public artifact"* row red.

**One reason to trust the pin across build directories.** `build.ps1:595`
publishes from the default `BUILD=build` while `make p1` used
`build-battle-playable-canonical-hwtri-harness`. The build-directory name occurs
**10 times in the ELF and 0 times in the `.nds`**, so the artifact the pin
records is build-directory independent. That is one absence check on a binary I
own, not a proof; it is stated as evidence of that strength.

**`SWITCH_READY.md` §6.3 is wrong on this tree and the correction stands.** It
claims NitroFS directory ordering makes ROM hashes irreproducible. The root pair
reproduces bit-exactly at a fixed HEAD, twice now — once in `BATTLEPACK_FLIP.md`
§3 and once here.

---

## 6. §6 item 2 — the picture. This is the owner's call and is not marked

After the switch, Boundary's screenshot arm captures the R2 published ROM, so
this is now automatic coverage rather than a lab-build proxy. The run before the
switch and the run after it used the same capture arm at the same delay, which
gives a before/after on the same instrument:

| file | ROM | captured |
|---|---|---|
| `../../visibility/2026-08-17_r2-switch/before-r1path-published-5F3D1FE3.png` | pre-switch, `NDS_R2_PATH 0` | 14:45:37 |
| `../../visibility/2026-08-17_r2-switch/after-r2path-published-2F47C8AC.png` | **post-switch, `NDS_R2_PATH 1`** | 15:09:17 |

Both read the same scene at the same moment: Dream Land with tree, three
platforms, flower beds, pond and moving sky; Fox on the upper-left platform and
Mario on the ground right; `FPS 29.9  UP 59.7`, `TIME 01:00`,
`P1 [MARIO] DMG -- STOCK x1`, `CPU L3 [FOX] DMG -- STOCK x1`. No flash, tear,
corrupt tile, missing fighter or stuck frame in either. The fighters' limbs
differ by a frame or two, which is capture timing rather than content — the
delay is wall-clock, not frame-locked.

**No visual verdict is recorded here.** SwitchPlan `:385` assigns item 2 to the
owner's eye, and `CLAUDE.OPUS.md` rail 5 forbids an agent concluding it.

Three further frames come from §7's soak filmstrip, which is the R2 path driven
through the whole P1 flow rather than one entry frame:

- `soak-r2-t231-match-bothcpu.png` — mid-match, both level-3 CPUs engaged
  (`CPU L3 [MARIO] 37%`, `CPU L3 [FOX] 50%`), TIME 00:29, tick HUD live with
  `VBI 2:994 3:135 4:5 5+:1 max:18` and the ROM's own `GIT e419cf8` stamp.
- `soak-r2-t179-results-mariowins.png` — **Results on the R2 path**: `MARIO WINS`,
  both fighters posed, the KOs/TKO/Pts/Place panel reading 1st and 2nd.
- `soak-r2-t663-final.png` — the last frame of the eleven-minute chain.

---

## 7. §6 item 4 — stability soak on the Runtime 2 path, at both match lengths

SwitchPlan `:556-559` asks for the demo loop at **1-minute and 5-minute match
lengths**. The 5-minute run is an **owner-instructed acceptance exception** to the
standing *"never launch the five-minute configuration"* rule in `AGENTS.md`, and
it applies to this gate only. Both arms are `NDS_R2_BOTH_CPU=1` — Mario CPU vs
Fox CPU, the stress shape — and both build the tick-HUD target, whose generated
config carries `NDS_R2_PATH 1`.

### 7.1 The 1-minute arm — `build-c251-soak-r2-1min`

`-MinutesToRun 11 -PollSeconds 5 -IdenticalFramesToTrip 16 -PressStartSeconds 60
-PressStartOnResults -MatchMinutes 0 -RunnerSlot 6`. Detected presses rather than
timed ones, so a START tap can only land on Results and never pause a live match.
Freeze threshold 80 s (16 polls × 5 s), 129 filmstrip frames.

```text
verdict: NO-FREEZE
match timer confirmed in-guest: 1 minute(s)
```

The match length is proven from inside the guest, not from the flag — this is the
3600-tick match §6 item 4 names, not a soak-lengthened one.

| SwitchPlan §7 asks | counter | value |
|---|---|---:|
| successive matches (battery asks ≥ 3) | `gNdsVSResultsStartCount` | **6** |
| START at Results restarts the match | `gNdsVSResultsRematchCount` | **6** |
| whole stress match start to finish | `gNdsSCVSBattlePlacementInitCount` | **11** |
| Sudden Death exercised | `gNdsSCVSBattleSuddenDeathPrepareCount` | **4** |

**The entry count reconciles exactly**: 1 initial entry + 6 START rematches + 4
Sudden Deaths = 11, and `gNdsRendererSceneTextureVramResetCount` independently
reads **11**, so the scene-owned texture-VRAM reset ran on every entry. Sudden
Death is measured from `scVSBattleStartSuddenDeath`'s own counter, not inferred
from a screenshot.

| counter | measured | threshold | margin |
|---|---:|---:|---:|
| `gNdsTaskmanArenaChosenSize` | **1,548,288** | 1,564,672 grantable ceiling | **16,384 B under** |
| `gNdsTaskmanGeneralHeapFreeMin` | **51,904** | 32,768 floor | **+19,136 B** |

`ChosenSize` is the requested `0x17A000` exactly — the arena search did not fall
back. The heap low-water clears the mandated floor by 19,136 B and never
approached the 25,600 B `ifCommonSetMaxNumGObj` cap. It sits 864 B under
`BATTLEPACK_FLIP.md`'s pack-on 52,768 across a different match count, i.e. the
same population, not a leak.

**Every failure counter is 0**, and each one can be non-zero:
`gNdsTaskmanArenaAllocFailCount`, `gNdsR2AnimCacheArenaOverflows`,
`gNdsR2AnimCacheRejects`, `gNdsR2AnimCacheArenaReserveFailCount`,
`gNdsBattlePackLoadFails`, `gNdsSyMallocOverflowCount`,
`gNdsRendererBattleStaticTextureViolationCount`, `gNdsObjmanPanicCount`,
`gNdsObjAnimRunawayCount`, `gNdsRelocResolveMisalignCount`,
`gNdsBattlePlayablePacingCadenceViolationCount`.

### 7.2 The 5-minute arm — `build-c252-soak-r2-5min`

The build differs from the 1-minute arm's by **exactly one line**:

```text
78c78
< #define NDS_R2_SOAK_MATCH_MINUTES 0
> #define NDS_R2_SOAK_MATCH_MINUTES 5
```

**It took two runs, and the first one is reported rather than discarded, because
the harness caught a mistake I made in its shape.**

**Attempt 1 — passive, `-PressStartSeconds 0 -MinutesToRun 12`.** The first
verdict line read `verdict: NO-FREEZE`. The harness then **overrode itself**:

> `verdict CORRECTED to SOAK-UNDERCOVERED -- the 5-minute match ended during a
> 12-minute passive soak, so the tail watched Results rather than gameplay.
> Re-run with -MatchMinutes 12 or higher; the default resolves this
> automatically, so this run was given an explicit value that was too small.`

Exit code 2 (`soak-freeze-watch.ps1:1785`). **My `^verdict:` grep did not see the
correction, because the correction line has no colon** — the same shape as the
"output filters hide harness failures" lesson. What caught it was reading the
harness's exit code, and that is the only reason this section is not a false
NO-FREEZE. The correction is right: the match ended at ~t+333 s of a 720 s run,
so 54% of the soak watched a Results screen.

Attempt 1 is not wasted evidence — it is the arm that proves a **single
uninterrupted 5-minute match** completes: `match timer confirmed in-guest:
5 minute(s)`, the guest's own `gSCManagerTransferBattleState.time_limit` reads
**5**, and frame `soak5-r2-t333-results-foxwins.png` shows `FOX WINS` at
`TIME 00:00`. No freeze across the whole 720 s.

**Attempt 2 — the same ROM, `-NoBuild`, in the rematch shape**
(`-MatchMinutes 5 -MinutesToRun 12 -PressStartSeconds 60 -PressStartOnResults`),
so the tail is gameplay rather than Results. No rebuild was needed: the match
length is baked into the ROM and both runs used the same one.

```text
verdict: NO-FREEZE          exit code 0
match timer confirmed in-guest: 5 minute(s)
gSCManagerTransferBattleState.time_limit = 5
```

**Exactly one line of that log contains the word `verdict`, checked without a
colon filter this time.** 142 filmstrip frames.

| counter | value | reading |
|---|---:|---|
| `gNdsVSResultsStartCount` | **2** | two completed 5-minute matches |
| `gNdsVSResultsRematchCount` | **2** | START at Results restarted it, twice |
| `gNdsSCVSBattlePlacementInitCount` | **3** | 1 initial + 2 rematches |
| `gNdsRendererSceneTextureVramResetCount` | **3** | matches entries exactly |
| `gNdsSCVSBattleSuddenDeathPrepareCount` | 0 | neither 5-minute match tied |
| `gNdsTaskmanArenaChosenSize` | 1,548,288 | 16,384 B under the ceiling |
| `gNdsTaskmanGeneralHeapFreeMin` | **52,368** | +19,600 B over the 32,768 floor |

`gNdsTaskmanArenaAllocFailCount`, `gNdsR2AnimCacheArenaReserveFailCount`,
`gNdsSyMallocOverflowCount`, `gNdsBattlePackLoadFails`,
`gNdsRendererBattleStaticTextureViolationCount`, `gNdsObjmanPanicCount`,
`gNdsObjAnimRunawayCount`, `gNdsRelocResolveMisalignCount` and
`gNdsBattlePlayablePacingCadenceViolationCount` are all **0**.

Sudden Death is covered by the 1-minute arm's **4** occurrences, not by this one.

### 7.3 One counter pair is non-zero, and it is reported rather than smoothed

`gNdsR2AnimCacheArenaOverflows` and `gNdsR2AnimCacheRejects` are **0 on the
1-minute arm** and **non-zero on both 5-minute runs**:

| arm | match length | entries | wall | `ArenaUsedBytes` | `Fills` | `Hits` | **Overflows** | **Rejects** |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| `c251` 1-minute | 1 min | 11 | 11 min | — | — | — | **0** | **0** |
| `c252` attempt 1 | 5 min | 1 | 12 min | 291,664 | 16 | 888 | **2** | **2** |
| `c252` attempt 2 | 5 min | 3 | 12 min | 441,920 | 41 | 1,937 | **6** | **6** |

**What the comparison isolates.** The 1-minute and the 5-minute rematch arms ran
for the same wall time on the same ROM family and the same R2 path; the
1-minute arm did **more** battle entries (11 against 3) and had **zero**
overflows. So the variable that tracks this is **match duration between scene
teardowns**, not wall time, not entry count, and not the loop selector.

**What it is not.** `ReserveFailCount`, `ArenaAllocFailCount` and
`SyMallocOverflowCount` are all 0, so nothing *failed* to allocate — the
animation cache declined to grow and those clips took the uncached path, which
is a cost, not a corruption. `gNdsObjAnimRunawayCount` is 0, cadence violations
are 0, and both runs are NO-FREEZE.

**What is NOT established: that the switch causes it.** No Runtime 1 arm was run
at a 5-minute match length, so this cycle cannot separate "long matches overflow
the anim cache" from "long matches overflow it *on the R2 path*". The
one-line-wide control that would settle it is the same `NDS_R2_LAB_R1_PATH=1`
arm §4 already needs. **It does not touch the shipping Boundary configuration**,
which is the one-minute match, and that arm reads 0.

---

## 8. What this does NOT say

- **It does not declare the P1 performance gate passed.** §4 measures the cadence
  arm at **96.03%** on the switched published configuration under the owner's
  both-CPU stress arm, against 96.23% before the switch. The `≥95%` verdict, and
  the choice of the 2,043-frame population, are the owner's — recorded in
  `SHIP_CADENCE.md` §4.1.
- **It does not prove the −4 cadence frames are placement.** §4 says why they
  have the shape of placement and names the exact arm that would settle it — the
  published target at *this* HEAD with `NDS_R2_LAB_R1_PATH=1 NDS_R2_BOTH_CPU=1`.
  That arm was not run. Nothing in this cycle is sized against the −4.
- **It does not re-bank the tick arm.** No 1,600-sample `-RingDump` gate reading
  was taken on the R2 path. `SHIP_CADENCE.md` §4's rank-80 figures remain the
  sizing basis and were measured on Runtime 1.
- **It does not discharge §6 item 2 or item 5.** The visual gate is the owner's
  eye and the retail play test is the owner's hardware. §6 supplies material for
  the former and claims nothing.
- **It does not attribute the animation-cache overflows.** §7.3 measures them,
  isolates the variable that tracks them (match duration) and states plainly
  that no Runtime 1 arm was run at a 5-minute match length, so "long matches
  overflow the anim cache" and "long matches overflow it on the R2 path" are
  not separated. They do not appear at all in the shipping one-minute
  configuration.
- **It does not compile-prove the drift fix.** `NDS_R2_POSITION_PROBE` is 0 in
  every build this cycle produced, so the added statement was never fed to the
  compiler. It is character-for-character the construct Runtime 1 already carries
  in the same translation unit with a `u32` parameter of the same kind, which is
  an argument, not a measurement. One build with `NDS_R2_POSITION_PROBE=1` closes it.
- **It does not commit, push, or snapshot.** The tree is left dirty for the
  orchestrator, alongside the owner's own uncommitted `PROJECT_GOAL.md` and the
  four untracked probe/analysis files, none of which were touched.
- **Each arm was run once.** The cadence probe is guest-deterministic
  (`SHIP_CADENCE.md` §0 measured 3 stops and 2,038 stops producing identical
  histograms), which is why one run is accepted, but no arm was re-run to prove
  self-reproduction.

---

## 9. Reproduce

```powershell
# the switch is already in the tree; confirm it resolves per target
.\scripts\check-tickhud-parity.ps1          # 54 flags, 2 allowlisted, 0 drift
make print-benchmark-flags TARGET=smash64ds-results-lab-hwtri   # BENCH_MAKE_R2_PATH=0

# the published pair + the flag-identical instrument
make p1

# engagement, both directions
arm-none-eabi-nm smash64ds-battle-playable-hwtri.elf | Select-String ndsR2Battle
make TARGET=smash64ds-battle-playable-proof-hwtri BUILD=build-c249-r1ctl NDS_R2_LAB_R1_PATH=1
arm-none-eabi-nm builds\build-c249-r1ctl\smash64ds-battle-playable-proof-hwtri.elf | Select-String ndsR2Battle

# Boundary -- cmd's redirect, NOT Tee-Object (docs/VERIFYING.md:399)
cmd /c "pwsh -NoProfile -ExecutionPolicy Bypass -File scripts\verify-all.ps1 -Profile Boundary -RunnerSlot 3 > artifacts\performance\2026-08-17_r2-switch\boundary-r2path.log 2>&1"

# cadence -- BACK UP BOTH ROOT ROMs FIRST, this writes the published pair
make TARGET=smash64ds-battle-playable-hwtri BUILD=build-c250-pubgate-r2 NDS_R2_BOTH_CPU=1
pwsh -NoProfile -File scripts\probe-present-cadence.ps1 `
     -Build build-c250-pubgate-r2 -Target smash64ds-battle-playable-hwtri `
     -Hits 1 -EndBreak mnVSResultsStartScene -TimeoutSeconds 2700 -RunnerSlot 6 `
     -Artifact artifacts\performance\2026-08-17_r2-switch\c250-cadence.txt

# restore the published pair -- DELETE BOTH HALVES FIRST or make relinks nothing
Remove-Item .\smash64ds-battle-playable-hwtri.nds,.\smash64ds-battle-playable-hwtri.elf
make TARGET=smash64ds-battle-playable-hwtri BUILD=build-battle-playable-canonical-hwtri-harness

# the two soak arms
pwsh -NoProfile -File scripts\soak-freeze-watch.ps1 -Build build-c251-soak-r2-1min `
     -MinutesToRun 11 -PollSeconds 5 -IdenticalFramesToTrip 16 `
     -PressStartSeconds 60 -PressStartOnResults -MatchMinutes 0 -RunnerSlot 6 `
     -SaveFramesTo artifacts\visibility\2026-08-17_r2-switch\soak-1min `
     -JsonOut artifacts\performance\2026-08-17_r2-switch\soak-r2-1min.json

# the 5-minute arm. USE THE REMATCH SHAPE, NOT -PressStartSeconds 0: a passive
# soak whose match ENDS spends its tail on Results and the harness correctly
# overrides its own verdict to SOAK-UNDERCOVERED (§7.2). Same ROM either way.
pwsh -NoProfile -File scripts\soak-freeze-watch.ps1 -Build build-c252-soak-r2-5min `
     -MinutesToRun 12 -PollSeconds 5 -IdenticalFramesToTrip 16 `
     -MatchMinutes 5 -PressStartSeconds 60 -PressStartOnResults -RunnerSlot 6 `
     -SaveFramesTo artifacts\visibility\2026-08-17_r2-switch\soak-5min-rematch `
     -JsonOut artifacts\performance\2026-08-17_r2-switch\soak-r2-5min-rematch.json
```

**Read the harness's EXIT CODE, and grep for `verdict` without a colon.**
`soak-freeze-watch.ps1` can print `verdict: NO-FREEZE` and then override itself
on a line that begins `verdict CORRECTED to …`; only the exit code and an
uncoloned grep see it.

## 10. Files

| file | what |
|---|---|
| `boundary-r2path.log` | the whole 18.9 MB Boundary run |
| `c250-cadence.txt` / `c250-run.log` | the cadence arm's histogram and run |
| `soak-r2-1min.json` / `.log` | the 1-minute soak verdict, samples and counters |
| `soak-r2-5min.json` / `.log` | the 5-minute soak, attempt 1 — SOAK-UNDERCOVERED, kept as evidence |
| `soak-r2-5min-rematch.json` / `.log` | the 5-minute soak in the rematch shape — the one that counts |
| `make-p1.log`, `make-c249-r1ctl.log`, `make-c250-pubgate-r2.log`, `make-restore.log` | the builds |
| `../../visibility/2026-08-17_r2-switch/` | before/after published captures + 3 cited soak frames + both filmstrips |
