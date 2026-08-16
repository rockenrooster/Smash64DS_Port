# The fixed camera arm is not slower. The TOGGLE is. Whichever arm the ROM boots into is the fast one, in either direction, for the rest of the match.

**Date:** 2026-08-16 · **Branch:** `codex/r2-runtime2` · **HEAD `df035595bdc`**
**Zero builds.** Nine emulator runs on `builds/build-c205-camtoggle` — **the ROM the owner
played**, not the tick-HUD instrument. No source changed, no ROM published.
**UNITS: presented-frame interval in VBlanks. 1 VBlank = 560,190 ARM9 ticks; the locked
cadence is 2 VBlanks = 29.91 FPS.**

```text
ANSWER    THE FIXED ARM IS NOT WORSE.  Whole match, arm chosen at frame 1, two
          gdb stops, 2,043 presented frames each:
                     iv2     iv3   iv4   iv5+   max   VBlanks
            FLOAT   1,953     72     5     13    26     4,277
            FIXED   1,956     69     5     13    26     4,273
          The fixed arm is 3 frames BETTER at the locked cadence and finishes
          the same 2,043 frames in 4 FEWER VBlanks.  Both arms reproduce
          BIT-IDENTICALLY on a repeat run.  P50 = 2, P95 = 2, P99 = 3 on both.
          Section 2.

SO WHY    THE PENALTY FOLLOWS THE TOGGLE, NOT THE ARM, AND IT IS SYMMETRIC.
          Boot FLOAT then flip to FIXED: 355 of 800 fixed frames slip to 3
          VBlanks.  Boot FIXED then flip to FLOAT: 395 of 600 float frames slip.
          Same match, same binary, opposite arms, near-identical block-for-block
          penalty.  Whichever arm the ROM has been running since battle start is
          the fast one.  Section 3.

TRANSIENT NOT A TRANSIENT IN ONE DIRECTION AND SIX FRAMES IN THE OTHER.
          Flipping AWAY from the boot arm: the very next frame slips and cadence
          NEVER returns to baseline -- still elevated 600 frames later, to the
          end of the match.  Flipping BACK to the boot arm: 6 slipped frames,
          then baseline; 20 consecutive on-cadence frames complete 21 frames
          after the flip.  Section 3.2.

CONTROL   IT IS NOT THE DEBUGGER AND IT IS NOT THE POKE.  A NULL FLIP -- poke
          0->1 at frame 400 and 1->0 at frame 401, two writes, net arm unchanged
          -- reads 113 slip-VBlanks over 1,000 frames against the no-poke
          control's 114.  Per-frame gdb stops are also innocent: they cost 151
          slip-VBlanks (float) and 147 (fixed) over 1,900 frames, matching the
          two-stop runs.  Section 4.

FOR THE   The toggle ROM still shows the right PICTURE for each arm -- that is
OWNER     what it was built for and it is unaffected.  Its FPS readout after a
          flip is NOT.  Judging the two arms by flipping mid-match will always
          show the arm you just switched TO as the slow one, which is exactly
          the reported "it seems to alternate which one is slow".  Section 5.

OPEN      THE MECHANISM IS NOT IDENTIFIED.  One candidate is measured DEAD (the
          particle camera cache is 2 hits + 1 miss per frame on both sides of a
          flip) and the debugger write is excluded by the null-flip control.
          Stated as a bound, not a mechanism.  Section 6.
```

---

## 0. Why this exists

`AGENTS.md` requires every device A/B report to carry the 2/3/4/5+ VBlank-interval
histogram, the max interval, and P50/P95. **The camera A/B
(`../2026-08-16_camera-fixedpoint/CAMERA_Q20_12.md`) did not carry one** — it measured
`WORK-H` ticks on the tick-HUD instrument and never measured presented cadence on the
playable ROM. The owner then played `build-c205-camtoggle` and reported that the picture is
fine (*"otherwise it looks fine"*) but that **FPS is worse on the fixed arm**, and that
*"it seems to alternate which one is slow"*. That omission is what let the question reach
them unresolved.

Nothing in the tree produced that histogram for an arbitrary arm on a proof ROM, so
`scripts/probe-present-cadence.ps1` was written for it (§7).

---

## 1. Basis, stated once

| | |
|---|---|
| ROM | `builds/build-c205-camtoggle/smash64ds-battle-playable-proof-hwtri.nds` |
| build config | `NDS_R2_CAMERA_FIXED 0`, `NDS_R2_CAMERA_FIXED_TOGGLE 1`, `NDS_TICK_HUD 0`, `NDS_RENDERER_PROFILE_LEVEL 0`, `NDS_R2_BOTH_CPU 0`, `NDS_R2_BATTLEPACK 0`, `NDS_R2_FIGHTER_GX_COMPOSE 0` |
| match | Boundary `battle_playable_realtime`, mode 163, one minute, no input |
| window | from `scVSBattleStartBattle` to `mnVSResultsFuncStart` — **2,043 presented frames**, every run |
| instrument | the guest's own `gNdsBattlePlayablePacingPresentIntervalBucket[2..5]`, `…IntervalMax/Min`, `…VBlanks`, `…CadenceViolationCount`, plus per-frame differencing of `…VBlanks` |
| arm selection | `gNdsR2CameraFixedEnabled` (`.data`, `0x020fde2c`) poked over gdb — **the identical store the SELECT handler performs** (`nds_platform.c:479-487`) |
| engagement | `gNdsR2CameraFixedLookAtCalls` / `…FloatLookAtCalls` / `…PerspCalls` / `…FloatPerspCalls` on every row |

**What the interval means.** `ndsBattlePlayablePresentRealtimeFrame`
(`src/port/taskman_seam.c:5032-5085`) schedules each present at
`last + NDS_BATTLE_PLAYABLE_PRESENT_VBLANKS` and then measures the interval it actually
got. **2 is on-cadence 30 Hz; every 3/4/5+ is a slip**, and a slip means the loop
iteration — two logical updates plus the draw — exceeded 2 VBlanks.

**No VBlanks are lost or double-counted, measured rather than assumed.** Over the per-frame
runs the elapsed-tick-per-VBlank ratio is **560,189 on the float arm and 560,190 on the
fixed arm** (`gNdsBattlePlayablePacingTimerTicks` differenced against
`…PacingVBlanks`). At interval 2 the mean frame is 1,120,374 and 1,120,369 ticks
respectively. The two counters cannot be disagreeing about how much time passed.

> **Instrument note, and it should be made structural.**
> `gNdsBattlePlayablePacingPresentIntervalBucket` is **not** a member of
> `NDS_BATTLE_PLAYABLE_PACING_GROUP` (`include/nds/nds_startup.h:4184-4198`), so nothing
> flushes it for a debugger read on purpose. It is readable only because `DC_FlushRange`
> cleans whole 32-byte lines and two group members happen to share the array's two lines:
> `…CadenceViolationCount` at `0x0222c220` covers `bucket[0..2]` at `…234/238/23c`, and
> `…IntervalMax`/`Min` at `…24c/250` cover `bucket[3..5]` at `…240/244/248`. **Any relayout
> of `diagnostics.c` silently breaks the one counter set `AGENTS.md` requires in every
> device A/B report.** Not fixed here — the fix changes the ROM, and this cycle had to
> measure the ROM the owner played. One `X()` line closes it.

---

## 2. The answer: whole match, arm fixed from frame 1, two stops

Two gdb stops per run — one at the first frame-complete marker to select the arm, one at
`mnVSResultsFuncStart` to read the guest's own cumulative histogram. No per-frame stops.

| | presented | VBlanks | **iv 2** | **iv 3** | **iv 4** | **iv 5+** | max | min | cadence violations |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| **FLOAT** (`runB0`) | 2,043 | **4,277** | **1,953** | 72 | 5 | 13 | 26 | 2 | 0 |
| **FLOAT** repeat (`runB0r`) | 2,043 | 4,277 | 1,953 | 72 | 5 | 13 | 26 | 2 | 0 |
| **FIXED** (`runB1`) | 2,043 | **4,273** | **1,956** | 69 | 5 | 13 | 26 | 2 | 0 |
| **FIXED** repeat (`runB1r`) | 2,043 | 4,273 | 1,956 | 69 | 5 | 13 | 26 | 2 | 0 |

- **Both arms reproduce bit-identically** across separate emulator sessions, including the
  `tk` field to the tick. The 3-frame difference between the arms is therefore
  deterministic, not sample noise.
- **P50 = 2, P95 = 2, P99 = 3 VBlanks on both arms.** (P95 index 1,941 of 2,043 sorted
  ascending; both arms have more than 1,941 intervals equal to 2. P99 index 2,023 lands in
  the 3 bucket on both.) In FPS: P50 and P95 both **29.91**, P99 **19.94**.
- **Max interval 26 on both**, and it is the same frame: the first present after
  `scVSBattleStartBattle`, which both arms report as `min=26 max=26` at n=1.
- **`gNdsBattlePlayablePacingCadenceViolationCount` is 0 on both** — no present ever landed
  early.

### 2.1 The plain statement the brief asked for

**The fixed arm is not genuinely worse in presented cadence. It is very slightly better:
3 fewer slipped frames out of 2,043 (0.15%) and 4 fewer VBlanks over the match (0.09%).**

The direction agrees with the tick measurement — `CAMERA_Q20_12.md` measured the fixed arm
at **−4,736 tk/fr paired median** — so **quantisation did not invert this cut.** The
`[[all-is-a-quantized-gate]]` failure mode (mean work down, more frames across a quantum
boundary) is a real risk and it is not what happened here: 4,736 tk/fr is 0.85% of a
560,190-tick VBlank, and it bought 3 frames.

**This does not by itself say the camera lane is worth taking** — it says the lane is not
disqualified on cadence. The pixel decision is untouched by this document and remains
`BLOCKED(decision: draw-side precision)` with the owner.

---

## 3. What the owner actually saw: the toggle

### 3.1 Symmetric, sustained, and it does not care which arm

Slip-VBlanks per 100 presented frames (the amount by which the interval exceeded 2, summed):

| run | arm at boot | flips | 1–100 | 101–200 | 201–300 | 301–400 | 401–500 | 501–600 | 601–700 | 701–800 | 801–900 | 901–1000 |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `runC0` | FLOAT | none | 69 | 0 | 2 | 6 | 8 | 4 | 1 | 3 | 9 | 12 |
| `runC1` | FIXED | none | 68 | 0 | 2 | 6 | 7 | 4 | 2 | 3 | 9 | 12 |
| `runE` | FLOAT | 0→1 @400, 1→0 @401 | 69 | 0 | 2 | 6 | 8 | 3 | 1 | 3 | 9 | 12 |
| **`runA`** | FLOAT | **0→1 @400**, 1→0 @800 | 69 | 0 | 2 | 6 | **33** | **34** | **79** | **99** | 9 | 12 |
| **`runD`** | FIXED | **1→0 @400** | 68 | 0 | 2 | 6 | **36** | **36** | **79** | **99** | **96** | **65** |

**`runA` and `runD` flip in opposite directions and produce the same penalty, block for
block** (33/34/79/99 against 36/36/79/99). Both arms are innocent; the flip is not.

Whole-window totals:

- `runA`: **355 of 800** post-flip FIXED frames slipped to 3 VBlanks; its FLOAT frames,
  including the 442 after it flipped back, slipped **57 of 1,242**.
- `runD`: **395 of 600** post-flip FLOAT frames slipped, against `runC0`'s 60 of 1,900 for
  the same arm with no flip.
- `runA` block 9–10 (`9, 12`) is *exactly* `runC0`'s (`9, 12`): flipping back to the boot
  arm restores baseline completely.

### 3.2 The transient, as a number

Interval sequence across each flip (each digit is one presented frame's interval):

```text
runA  flip 0->1 at 400 :  22222 3322333323232222322223334      never returns to a clean run
runD  flip 1->0 at 400 :  22222 33333333322322223222 23334     never returns to a clean run
runA  flip 1->0 at 800 :  333333 222222222222222222222222      6 slipped frames, then clean
```

- **Flipping away from the boot arm: the next frame slips, and cadence never settles.** In
  `runD` it was still elevated 600 frames later, at the end of the match.
- **Flipping back to the boot arm: 6 slipped frames.** 20 consecutive on-cadence frames
  complete at frame 821, **21 frames after the flip**.
- **The arm itself engages within one frame, proven by counters**: at `runD` frame 400 the
  row reads `rt=1 dfx=4 dfl=0`; at frame 401 it reads `rt=0 dfx=0 dfl=2`. There is no
  multi-frame engagement lag to confuse with the cadence effect.

**So the honest answer to "how long should the owner wait after a flip before trusting
their eye" is: for the FPS number, no length of wait is enough.**

---

## 4. Two negative controls, both of which could have failed

**The debugger write is not the cause.** `runE` performs the same `set var` twice — 0→1 at
frame 400 and 1→0 at frame 401 — so two pokes land and the net arm is unchanged. Result:
**113 slip-VBlanks over 1,000 frames against the no-poke control's 114**, and the interval
sequence across frames 396–425 is indistinguishable from the control. A poke costs nothing;
one frame in the other arm costs nothing.

**The per-frame gdb stop is not the cause.** Same arm, 1,900 frames, one stop and nine
memory reads per presented frame:

| | presented | VBlanks | iv 2 | iv 3 | iv 4 | iv 5+ | slip VBlanks |
|---|---:|---:|---:|---:|---:|---:|---:|
| `runC0` FLOAT, per-frame stops | 1,900 | 3,975 | 1,823 | 60 | 5 | 11 | 151 |
| `runC1` FIXED, per-frame stops | 1,900 | 3,971 | 1,826 | 57 | 5 | 11 | 147 |

Both agree with their two-stop counterparts in §2 and with each other, and the fixed arm is
again 3 frames better and 4 VBlanks shorter — **the same margin the two-stop runs measured,
on a different instrument.** That is the strongest thing in this document: two independent
instruments, four runs, one number.

---

## 5. What this means for the toggle ROM

The toggle was built so the owner could judge **the picture** in motion rather than from
stills, and *that* is unaffected: each arm renders its own picture correctly the frame after
SELECT, and the arm indicator on HUD row 3 is correct.

**Its FPS readout after a flip is not usable.** Because the penalty attaches to the
non-boot arm in either direction, an A/B done by pressing SELECT will always show the arm
just switched *to* as the slower one — in both directions, which is exactly the reported
symptom. The owner's observation is real and was correctly reported; its cause is the
instrument.

**To compare the arms on cadence, each arm must be the boot arm.** That is what §2 does,
and it needs no new ROM: `gNdsR2CameraFixedEnabled` is a `.data` word, so a
`-RouteInitial 0|1` run of `scripts/probe-present-cadence.ps1` selects it at frame 1. A
*playable* per-arm build would be `make … NDS_R2_CAMERA_FIXED=1` against the default 0 — two
ROMs, not one toggle.

---

## 6. What is NOT explained, stated as a bound

**The mechanism of the toggle penalty is not identified.** What is known:

- It is **symmetric** in arm and **sustained** (≥600 frames), and it attaches to running
  the non-boot arm rather than to the act of flipping (`runE`).
- It is **not the debugger write** (`runE`) and **not the per-frame stop** (`runC0`/`runC1`).
- It is **not the particle camera cache**: `gNdsParticleCameraCacheHitCount` /
  `…MissCount` read **2 hits and 1 miss per presented frame on both sides of the flip**, on
  every sampled frame of `runD`. The renderer's own camera-matrix cache counters exist as
  symbols but are written only under `NDS_RENDERER_PROFILE_LEVEL >= 2`, which this ROM is
  not, so they read 0 and say nothing.
- The route word shares its 32-byte D-cache line (`0x020fde20`–`0x020fde3f`) with
  `gNdsBattlePlayableFoxCpuEnabled`, `gNdsCameraMatrixLeanEnabled` and the first 16 bytes of
  `dGMCameraPlayerZoomRanges`, which the game camera reads every frame. The line is
  read-only to the guest, therefore clean, therefore a debugger write to it survives — which
  is why the arm changes at all. **This is a candidate, not a finding**, and `runE` argues
  against it: two writes to the same line cost nothing.

**Nothing in this document depends on the mechanism.** §2's answer is measured on runs with
no flip at all.

---

## 7. The instrument

`scripts/probe-present-cadence.ps1` (new). Per presented frame it can record
`gNdsBattlePlayablePacingVBlanks`, `…PresentedFrames`, `…TimerTicks`, the route word, four
engagement counters and any `-ExtraGlobals`; at any chosen frames it toggles the route word;
at start and end it dumps the guest's own interval histogram. `-Hits 1 -RouteInitial 0|1
-EndBreak mnVSResultsFuncStart` is the two-stop whole-match form used for §2.

Three things it encodes rather than leaves to memory:

- **`-FlipAt` is a comma-separated STRING, not `int[]`.** Long runs must be launched
  detached through `cmd /c "pwsh -File …"` because that is the only redirect a force-kill
  cannot eat, and `pwsh -File` binds `-FlipAt 400 800 1200 1600` by taking the first token
  and shifting the rest onto positional parameters — `1600` landed in `-RouteInitial` and
  only its `ValidateRange` caught it. An `int[]` parameter cannot be passed correctly
  through the one launch form the measurement rules require, so it is not offered.
- **`-Hits` must be below the match's presented-frame count** (2,043 for mode 163). The
  counted loop ends by *not* continuing on hit `$Hits`, which is what returns control; if
  the battle loop exits first the marker never fires again and the run sits until its
  timeout. Default 1,900.
- **The summary histogram prints from inside the breakpoint command block**, so a killed or
  timed-out run still carries it, and `set logging enabled on` writes it through.

Reproduce §2:

```powershell
pwsh -File scripts\probe-present-cadence.ps1 -Build build-c205-camtoggle -RunnerSlot 6 `
  -Hits 1 -RouteInitial 0 -EndBreak mnVSResultsFuncStart -TimeoutSeconds 1200 `
  -Artifact artifacts\performance\2026-08-16_camera-cadence\runB0-float-2stop.txt
# ... and -RouteInitial 1 for the fixed arm.
```

## 8. Verification state

- **No build, no flag flipped, no production source edited** by this section. `decomp/`
  untouched.
- Root ROMs unchanged and not rebuilt: `smash64ds.nds` and
  `smash64ds-battle-playable-hwtri.nds` — hashes in the cycle report.
- Nine runs, all on `builds/build-c205-camtoggle`: `runA` (crossover, 2,043 frames),
  `runB0`/`runB0r`/`runB1`/`runB1r` (two-stop, 2,043 each), `runC0`/`runC1` (per-frame,
  1,900 each), `runD` (fixed-boot flip, 1,000), `runE` (null flip, 1,000).
