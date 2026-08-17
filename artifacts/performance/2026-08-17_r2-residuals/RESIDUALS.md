# R2-08 residuals closed: the pin is verifiable, the drift fix is compile-proven, and the −4 cadence frames are attributed — 1 to the switch, 3 to cross-run spread

**Date:** 2026-08-17 · **Branch:** `codex/r2-runtime2` · **HEAD `83978bb4ed8`**
**3 builds** (`make p1`, `build-c253-posprobe`, `build-c254-pubgate-r1ctl`) plus one
restore relink, **1 emulator run**, **2 checkers**,
**1 production edit: `DECOMP_PIN.txt`. Nothing committed.**
**UNITS: 1 project tick = 1 `cpuGetTiming()` tick = 2 ARM9 cycles.**

Basis is `../2026-08-17_r2-switch/R2_SWITCH.md`. Its §8 handed forward three open
items; all three are addressed here, and **one of its premises is refuted**.

**HEAD was not what the brief said.** The brief named `843fe40f4d2` as HEAD; the
actual HEAD at cycle start was `83978bb4ed8` (*"Track this cycle's evidence: the
docs cited paths that were not in the repo"*), one commit later. That mattered
enough to change how Task 1 was executed, and §1 records it rather than papering
over it.

---

## 0. Outcome

```text
THE PIN         REFUTED THE PREMISE IT WAS BUILT ON.  `make p1` at HEAD
   (§1)         83978bb4ed8 reproduced the previous cycle's published pair
                BIT-FOR-BIT -- 2F47C8AC...CB2F / 12,530,688 B and
                54338F7A...0967 / 10,285,736 B -- even though the stamp moved
                e419cf8 -> 83978bb.  NDS_TASK10_GIT_SHORT does NOT reach the
                published ROM: 0 occurrences in the .nds AND the .elf, against
                1 occurrence in the tick-HUD .nds (byte offset 917,152), which
                is the positive control.  It is also the Makefile's ONLY
                $(shell git ...) and there is no __DATE__/__TIME__.
                So the published ROM is HEAD-independent and deterministic.

                The pin's real defect was never the hash -- it was the COMMIT.
                OUTPUT_BUILT_AT_COMMIT read e419cf819f5, at which the switch was
                still UNCOMMITTED in the working tree; a clean checkout of that
                commit builds the PRE-switch ROM 5F3D1FE3...D20C / 12,538,880 B.
                Now 843fe40f4d2 (the switch commit).  OUTPUT_BYTES and
                OUTPUT_SHA256 were left ALONE: they already equalled this
                build's output exactly, and that equality is the result.
                Parser: 22 keys, 0 invalid lines, 0 required missing.

DRIFT FIX       COMPILES, AND EMITS A REAL STORE.  build-c253-posprobe with
   (§2)         NDS_R2_POSITION_PROBE=1 exits 0, 0 diagnostics naming
                taskman_seam.c:53xx.  nm: 0222d0e4 B
                gNdsPositionProbeUpdateInPresent.  Disassembly of
                ndsR2HostBattleUpdateOnce: push / ldr r3,[pc,#200] /
                str r0,[r3,#0], and the literal at 0x2055c4c is e4 d0 22 02 =
                0x0222d0e4 -- the same address.  arg0 (update_index) stored to
                the probe global, FIRST statement, immediately before
                battle_status_before, which is where Runtime 1 publishes it.
                NEGATIVE CONTROL: the published ELF's copy of the same function
                has no such store -- push / ldr r5 / ldr r3 / ldrb r4 straight
                into battle_status_before.

CADENCE         THE -4 IS ATTRIBUTED, AND IT IS MOSTLY NOT THE SWITCH.
   (§3)         c254 = published target, THIS HEAD, NDS_R2_LAB_R1_PATH=1
                NDS_R2_BOTH_CPU=1, whole match:
                2=1963 3=74 4=4 5+=2 pres=2043 max=19 viol=0 vbl=4189
                = 96.08%, margin +22 over the 1,941 bar.

                vs c250 (R2): 1,962.  Their configs differ in NDS_R2_PATH and
                the stamp ONLY -- and the stamp is proven inert -- so this is
                the at-HEAD single-flag A/B.  THE SWITCH COSTS 1 FRAME OF
                2,043 = 0.05 points.

                vs c247 (R1): 1,966.  Their configs differ in the stamp ALONE,
                and the source diff outside artifacts/docs is two files whose
                changes are both inert on this arm.  So c247 and c254 are the
                same binary measured twice -- and they differ by 3 FRAMES
                (and by 3 VBlanks, 4,186 vs 4,189).
                THE SAME-BINARY CROSS-RUN SPREAD IS 3 FRAMES, WHICH IS LARGER
                THAN THE SWITCH'S OWN 1.

                PREDICTION HELD.  Pre-registered in PREDICTION.txt before the
                build: 1,952-1,972, denominator 2,043, viol=0, 4/5+ unchanged.
                Landed 1,963 / 2,043 / 0 / 4 and 2.  The stated falsifier
                (c254 >= 1,975) did NOT trigger.

                REPORTED WITHOUT SPIN: R1 at this HEAD is 1 frame BETTER than
                R2.  The direction favours Runtime 1.  It is 1/2,043 against a
                measured 3-frame same-binary spread, so it is not a finding in
                either direction -- but it is not rounded away either.

ANIM CACHE      STILL OPEN, AND DELIBERATELY NOT RUN.  A 5-minute R1 arm needs
   (§4)         its own build (NDS_R2_SOAK_MATCH_MINUTES is baked into the ROM;
                c254 carries 0), and the brief said not to spend a second build.
                ONE THING WAS SETTLED FOR FREE: the cache is NOT R2-only.  The
                R1 ELF and an R2 ELF each carry 18 gNdsR2AnimCache* state
                symbols and 8 ndsR2AnimCache* functions, including the global
                preload entry points.  So the control is constructible rather
                than vacuous, and shared machinery WEAKENS -- without settling
                -- "the switch causes it".

ROOT ROMs       Rewritten twice and restored bit-exactly.  BOTH halves deleted
   (§5)         before the restore relink.  Final root pair is identical to the
                make p1 link.  smash64ds.nds never rebuilt.
                Published ROM contract passed.  Tick-HUD parity: 54 flags,
                2 allowlisted, 0 drift.
```

---

## 1. §8 item — the pin, and the premise that did not survive contact

### 1.1 What was actually run

`make p1` (`Makefile:3521-3523`) at HEAD `83978bb4ed8`, exit 0, no `-j`,
`MAKEFLAGS` untouched. Full recompile — the generated config changed, so every
TU rebuilt.

| | bytes | SHA-256 |
|---|---:|---|
| `smash64ds-battle-playable-hwtri.nds` **before** | 12,530,688 | `2F47C8AC0730B0DAC6AA1B7A482B8599F33CA3E831B3AB6A7B0AA974A6ACCB2F` |
| `smash64ds-battle-playable-hwtri.nds` **after** | 12,530,688 | `2F47C8AC0730B0DAC6AA1B7A482B8599F33CA3E831B3AB6A7B0AA974A6ACCB2F` |
| `smash64ds-battle-playable-hwtri.elf` **before/after** | 10,285,736 | `54338F7A7FD2419C5A9446D47642E64BF5FAA50EE2CCDEC97817A06B15BE0967` |

**The two hashes are the same, and that is the finding.** The previous cycle
built that pair at HEAD `e419cf8`; this cycle built it at `83978bb4ed8`. The
generated config moved:

```text
#define NDS_TASK10_GIT_SHORT "e419cf8"   ->   "83978bb"
```

and the output did not move at all.

### 1.2 Why, measured two independent ways

**(a) The stamp is not in the artifact.** A literal byte search of each file for
each of the three stamps in play:

| file | `83978bb` | `e419cf8` | `798007f` |
|---|---:|---:|---:|
| `smash64ds-battle-playable-hwtri.nds` (published) | **0** | 0 | 0 |
| `smash64ds-battle-playable-hwtri.elf` (published) | **0** | 0 | 0 |
| `builds/build-tick-hud-buckets/…tickhud-hwtri.nds` | **1** | 0 | 0 |

**The positive control fires.** The tick-HUD ROM contains its own stamp at byte
offset **917,152** — it renders it on the HUD, which is the `GIT e419cf8` legend
in `R2_SWITCH.md` §6's soak frame. The same search on the same file family finds
nothing in the published pair. This is an absence with a control that can be
non-zero and is, not a bare grep.

**(b) `NDS_TASK10_GIT_SHORT` is the only HEAD-derived input.** `Makefile:1601` is
the sole `$(shell git ...)` in the file, and there is no `__DATE__`, `__TIME__`,
or `$(shell date ...)` anywhere in it. So there is no second channel by which
HEAD or wall-clock could reach the image.

### 1.3 The defect that was actually there

`OUTPUT_BUILT_AT_COMMIT` read `e419cf819f56980c2b270df659f05e91a6703ae9`. At that
commit the switch was **still uncommitted in the working tree** — `843fe40f4d2`
is the commit that landed it. A clean checkout of `e419cf8` therefore builds the
**pre-switch** ROM, `5F3D1FE3…D20C` / 12,538,880 B, and the pin's hash would never
appear. That is the unverifiable pin the brief diagnosed; the mechanism was just
different from the one assumed.

Now `OUTPUT_BUILT_AT_COMMIT=843fe40f4d2d5e8fdcd38210ce2e65ab4870e519` — the switch
commit, i.e. the **first** commit whose source tree produces this hash.

**The verifying build ran at `83978bb4ed8`, not at `843fe40f4d2`, so that value
carries one inference and it is named here.** `git diff --name-only 843fe40f4d2
83978bb4ed8` returns 20 files, **all** under `artifacts/performance/` — no build
input. Combined with §1.2's result that the stamp is inert, a build at
`843fe40f4d2` produces the same pair. If the orchestrator wants that as a
measurement rather than an inference, it is one more `make p1` from a checkout of
`843fe40f4d2`, and this cycle did not spend it.

### 1.4 What the key means now, and why it survives its own refutation

The brief asked me to state that committing the pin advances HEAD and therefore
that verification means checking out `843fe40f4d2` and building rather than
building at whatever HEAD is current.

**The second half is exactly right and is the reason to keep the key. The first
half is now moot for this artifact.** Committing the pin will advance HEAD, but
it cannot change the published ROM's hash, because the stamp never reaches it.
`OUTPUT_BUILT_AT_COMMIT` is therefore **not a hash qualifier** — it is a record
of the source state that produces the hash. That is still load-bearing: build at
a HEAD *before* `843fe40f4d2` and you get a different, pre-switch ROM. The
comment block in `DECOMP_PIN.txt` was rewritten to say this, and to record the
retracted claim explicitly rather than silently deleting it.

`OUTPUT_BYTES` and `OUTPUT_SHA256` were **not edited**: they already read
`12530688` and `2F47C8AC…CB2F`, exactly this build's output.

### 1.5 The parser, and who actually reads this file

`build.ps1:32-61` (`Read-PinFile`) skips blanks and `#` comments, splits on the
first `=`, and requires 13 named keys while tolerating extras. The edited file was
run through a faithful re-implementation of that function:

```text
PARSED_KEYS=22   INVALID_LINES=0   REQUIRED_MISSING=0
PIN_MATCHES_ROM_BYTES=True   PIN_MATCHES_ROM_SHA=True
```

22 keys and 0 missing — the same reading the previous cycle recorded, so the
enlarged comment block (which is all `#` lines) costs the parser nothing.

**`scripts/check-published-roms.ps1` does not read the pin at all** — no
`OUTPUT_*` key appears anywhere under `scripts/`. `build.ps1:612-618` is its only
consumer and only `Write-Warning`s on a mismatch. So a wrong pin never fails a
build or a verifier; it only misinforms a reader. That is worth knowing before
anyone treats a green Boundary as pin coverage.

---

## 2. §8 item — the drift fix, compile-proven

`make TARGET=smash64ds-battle-playable-proof-hwtri BUILD=build-c253-posprobe
NDS_R2_POSITION_PROBE=1`, exit 0. Generated config: `NDS_R2_POSITION_PROBE 1`,
`NDS_R2_PATH 1`. **0** compiler diagnostics naming `taskman_seam.c:53xx`.

This is a lab target, so it did not touch the root pair — re-hashed after it and
unchanged.

### 2.1 The symbol, with a control on each side

| ELF | `NDS_R2_POSITION_PROBE` | total symbols | `gNdsPositionProbeUpdateInPresent` | `ndsR2HostBattleUpdateOnce` |
|---|---:|---:|---|---|
| `builds/build-c253-posprobe/…proof-hwtri.elf` | 1 | 13,893 | **`0222d0e4 B`** | `02055b80 T` |
| root `smash64ds-battle-playable-hwtri.elf` | 0 | 13,802 | **absent** (expected) | `020504ac T` |

The symbol totals are printed deliberately: **the first attempt at this read
returned "ABSENT" for every query because `arm-none-eabi-nm` is not on
PowerShell's `PATH`**, and a failed tool and a real absence look identical. Re-run
through `/c/devkitPro/devkitARM/bin/arm-none-eabi-nm.exe`, with a nonzero symbol
count as the sanity line and `ndsR2HostBattleUpdateOnce` as a must-be-present
control on both sides. The absence in the published ELF is only reportable
because the same command found 13,802 other symbols in that same file.

### 2.2 The instruction, which is the actual proof

`ndsR2HostBattleUpdateOnce` in the candidate:

```text
02055b80 <ndsR2HostBattleUpdateOnce>:
 2055b80:  b570   push {r4, r5, r6, lr}
 2055b82:  4b32   ldr  r3, [pc, #200]   @ (2055c4c)
 2055b84:  4c32   ldr  r4, [pc, #200]   @ (2055c50)
 2055b86:  6018   str  r0, [r3, #0]          <-- the drift fix
 2055b88:  6823   ldr  r3, [r4, #0]          <-- battle_status_before begins here
 2055b8a:  2b00   cmp  r3, #0
 2055b8e:  7c5d   ldrb r5, [r3, #17]
```

and the literal pool it loads from:

```text
 2055c4c  e4d02202 3c921d02 ece61402 ccd02202
```

`e4 d0 22 02` little-endian is **`0x0222d0e4`**, the address `nm` gives for
`gNdsPositionProbeUpdateInPresent`. So `str r0,[r3,#0]` writes arg0 —
`update_index` — into the probe global, as the **first** thing the function does,
immediately ahead of the `battle_status_before` read. That is the position
`taskman_seam.c:5332-5338` claims and the position Runtime 1 publishes it in.
The store cannot be elided: the global is `volatile`.

The negative control, the same function in the published ELF at flag 0:

```text
020504ac <ndsR2HostBattleUpdateOnce>:
 20504ac:  b570   push {r4, r5, r6, lr}
 20504ae:  4d1d   ldr  r5, [pc, #116]
 20504b0:  682b   ldr  r3, [r5, #0]          <-- straight into battle_status_before
 20504b6:  7c5c   ldrb r4, [r3, #17]
```

No leading store. **The control differs.** `R2_SWITCH.md` §8's *"It does not
compile-prove the drift fix"* is discharged: the line reached a compiler and
produced the intended instruction.

---

## 3. §8 item — the at-HEAD Runtime 1 control, and the attribution of the −4

### 3.1 The arm, and its engagement proof taken before the run

`make TARGET=smash64ds-battle-playable-hwtri BUILD=build-c254-pubgate-r1ctl
NDS_R2_LAB_R1_PATH=1 NDS_R2_BOTH_CPU=1`, exit 0. Config against the published
canonical — **exactly two lines**:

```text
77c77
< #define NDS_R2_BOTH_CPU 0
> #define NDS_R2_BOTH_CPU 1
93c93
< #define NDS_R2_PATH 1
> #define NDS_R2_PATH 0
```

nm on the resulting ELF (13,793 symbols): `ndsR2Battle*` **0**, `ndsR2Host*`
**0** — the escape hatch really produced a Runtime 1 binary. Symbols that must
exist on both paths read **1** each and prove the ELF is readable rather than
empty: `ndsBattlePlayableFrameCompleteMarker`, `main`, `ndsPlatformReadInput`,
and both globals the probe itself reads —
`gNdsBattlePlayablePacingPresentIntervalBucket` and
`gNdsBattlePlayablePacingPresentedFrames`.

The published target writes the root pair, so `Resolve-Smash64DSBuildOutput`
(`scripts/lib/build-output.ps1:25-31`) routes the probe to the **root** ROM,
which at that moment was c254's. §5 covers the restore.

### 3.2 The run

`scripts/probe-present-cadence.ps1 -Build build-c254-pubgate-r1ctl -Target
smash64ds-battle-playable-hwtri -Hits 1 -EndBreak mnVSResultsStartScene
-TimeoutSeconds 2700 -RunnerSlot 6`. Three GDB stops for a whole match; 90 s of
the 2,700 s ceiling used (3%). Denominator is the guest's own
`gNdsBattlePlayablePacingPresentedFrames`, read after the battle loop exited, so
it is the whole match and not a window.

```text
PCADHIST DONE n=1 2=1963 3=74 4=4 5=2 max=19 min=2 viol=0 vbl=4189 pres=2043 tk=2350396160
```

| build | path | HEAD | 2 | 3 | 4 | 5+ | presented | two-VBlank | max | viol | VBlanks |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `c247-pubgate-packon` | R1 | `798007f` | 1,966 | 71 | 4 | 2 | 2,043 | 96.23% | 19 | 0 | 4,186 |
| `c250-pubgate-r2` | **R2** | `e419cf8` | 1,962 | 75 | 4 | 2 | 2,043 | 96.04% | 18 | 0 | 4,189 |
| **`c254-pubgate-r1ctl`** | **R1** | `83978bb` | **1,963** | **74** | **4** | **2** | **2,043** | **96.08%** | **19** | **0** | **4,189** |

≥95% of 2,043 is 1,941, so the margins are +25, +21 and **+22**.

### 3.3 The attribution, which is what this arm was built for

**Against c250 — this is the single-flag A/B.** The two generated configs differ
in `NDS_R2_PATH` and in `NDS_TASK10_GIT_SHORT`, and **§1.2 measured the stamp
inert in this artifact**, so the stamp cannot have moved the link placement. The
comparison is one flag:

> **R2 1,962 vs R1 1,963 — the switch costs 1 two-VBlank frame of 2,043, or 0.05
> points.**

**Against c247 — this is the noise floor.** Their configs differ in the stamp
**alone**. The source diff `798007f..83978bb` outside `artifacts/`, `docs/` and
`*.md` is exactly two files, and both changes are inert on this arm:

- `Makefile` — the two switch blocks, skipped when `NDS_R2_LAB_R1_PATH=1`; both
  configs read `NDS_R2_PATH 0`, which is the only thing those blocks set;
- `src/port/taskman_seam.c` — the §2 drift fix, inside `#if
  NDS_R2_POSITION_PROBE`, which is 0 in both.

So c247 and c254 are the same binary, and:

> **The same binary measured twice, about two hours apart, gave 1,966 and 1,963 —
> a 3-frame spread — and 4,186 against 4,189 total VBlanks.**

**Therefore the −4 that `R2_SWITCH.md` §4 reported decomposes as roughly 3 frames
of cross-run spread plus 1 frame from the switch, and the switch's own frame is
smaller than the spread.** §4's read that the −4 "has the shape of placement, not
of work" was directionally right about the conclusion; the mechanism is
cross-run measurement variation, and the stamp-driven link-placement story it
offered is wrong, because the stamp does not reach this binary.

**One caveat on the c247 comparison, stated rather than buried.** c247's ROM was
overwritten by later published-target builds, so "same binary" is an inference
from the config diff plus the source diff plus §1.2, not a hash comparison. The
c250 comparison — the one that prices the switch — does not depend on it.

### 3.4 The prediction, and the direction it went

`PREDICTION.txt` was written before the c254 build existed. It predicted the
denominator at 2,043 (**exact**), c254 in 1,952–1,972 (**1,963**), `viol=0`
(**0**), and the 4/5+ populations in the same neighbourhood (**4 and 2, identical
on all three arms**). The named falsifier — c254 at or above 1,975, i.e. Runtime 1
materially better — **did not trigger**.

**R1 at this HEAD is nonetheless 1 frame better than R2, and that is stated
plainly.** The direction favours Runtime 1. It is 1 frame in 2,043 against a
same-binary spread of 3, so it does not support a claim in either direction — but
it is not a null, and it is not rounded into one.

**`tk` is not interpreted here.** c254 reads `tk=2,350,396,160` against c250's
`2,346,416,512`, +3,979,648. Both arms record `vbl=4189`, i.e. the same elapsed
guest time, so that delta is the span between the two GDB stops rather than work
done. It is recorded, not converted into a cost.

---

## 4. §8 item — the animation-cache overflows, still open, and why no build was spent

`R2_SWITCH.md` §7.3 measured `gNdsR2AnimCacheArenaOverflows`/`Rejects` at 0 on the
1-minute arm and 2 then 6 on the 5-minute arms, isolated **match duration** as the
tracking variable, and stated that no Runtime 1 arm existed at 5 minutes.

**It still does not, and that was a deliberate choice.** The brief permitted a
5-minute R1 soak only if it cost no extra build. It costs one:
`NDS_R2_SOAK_MATCH_MINUTES` is baked into the image (§7.2 of that document proves
it — its two 5-minute runs shared one ROM with `-NoBuild`), and c254 carries
`NDS_R2_SOAK_MATCH_MINUTES 0`. So the arm is a build plus a 12-minute soak, and it
was not run.

**One sub-question was settled for free, from symbols already on disk.** The
animation cache is **not Runtime-2-only**:

| ELF | path | `gNdsR2AnimCache*` state | `ndsR2AnimCache*` functions |
|---|---|---:|---:|
| `builds/build-c253-posprobe/…proof-hwtri.elf` | R2 | 18 | 8 |
| `smash64ds-battle-playable-hwtri.elf` (c254, at root at the time) | **R1** | **18** | **8** |

The R1 build carries the whole apparatus, including the three global preload
entry points `ndsR2AnimCachePreloadMatch`, `…PreloadStep`, `…PreloadFinish`, the
arena helpers `…ArenaAlloc`/`…ArenaEnsure`/`…ArenaDropForReset`/`…ArenaStillOwned`,
`ndsR2AnimCacheFind`, and the `sNdsR2AnimCache*` state.

Two consequences. The R1 control **is constructible and would not be vacuous** —
the counters it would read exist on that path. And shared machinery **weakens**
the hypothesis that the switch causes the overflows, since the same cache runs on
both paths. It does not settle it: **symbol presence proves the code is linked,
not that it is exercised**, and no counter was read on an R1 5-minute match.

---

## 5. Root ROMs, and the restore

Everything that ran, in order, with the root pair hashed around it:

| point | `…hwtri.nds` | `…hwtri.elf` |
|---|---|---|
| cycle start | `2F47C8AC…CB2F` 12,530,688 | `54338F7A…0967` 10,285,736 |
| after `make p1` (HEAD `83978bb`) | `2F47C8AC…CB2F` **unchanged** | `54338F7A…0967` **unchanged** |
| after `build-c253-posprobe` (lab target) | `2F47C8AC…CB2F` unchanged | `54338F7A…0967` unchanged |
| after `build-c254-pubgate-r1ctl` (**published target**) | `171CE52C…BECB` 12,530,688 | `3697982E…EA12` 10,282,832 |
| after the restore relink | **`2F47C8AC…CB2F` 12,530,688** | **`54338F7A…0967` 10,285,736** |

`smash64ds.nds` was never rebuilt and reads `54C07FAC…C68A` / 11,915,264
throughout. It is the P2 ROM and is not part of P1.

**Both halves were deleted before the relink**, then:

```powershell
Remove-Item .\smash64ds-battle-playable-hwtri.nds,.\smash64ds-battle-playable-hwtri.elf
make TARGET=smash64ds-battle-playable-hwtri BUILD=build-battle-playable-canonical-hwtri-harness
```

This is the trap `e419cf8` recorded and `R2_SWITCH.md` §5 restates: after a lab
build has written the root pair, a bare re-make finds the root pair newer than
the objects, **relinks nothing, exits 0**, and leaves the lab ROM published.
Deleting both halves — never one — is what makes the recovery real.

**The restore reproduced bit-exactly on both halves**, which is now the third
independent bit-exact reproduction of this pair: `make p1` at `e419cf8` (previous
cycle), `make p1` at `83978bb` (§1.1), and this relink. That is the determinism
evidence behind §1.2's conclusion.

Closing checkers, both green and both building nothing:

```text
Published ROM contract passed: smash64ds-battle-playable-hwtri.nds, smash64ds.nds
Tick-HUD parity passed: 54 make flags compared, 2 allowlisted differences, 0 drift.
```

---

## 6. What this does NOT say

- **It does not claim the pin was verified by building at `843fe40f4d2`.** The
  verifying build ran at `83978bb4ed8`. The step from there to `843fe40f4d2`
  rests on `git diff --name-only` returning only `artifacts/performance/` paths,
  plus §1.2's measurement that the stamp is inert. That is an inference, named as
  one in §1.3, and one `make p1` from a checkout of `843fe40f4d2` would convert
  it into a measurement.
- **It does not claim every published artifact is HEAD-independent.** §1.2's
  result is about `smash64ds-battle-playable-hwtri.nds`. The tick-HUD ROM
  demonstrably *does* carry the stamp, so any pin covering that artifact would
  behave the way the retracted comment described.
- **It does not run the drift fix.** §2 proves the statement compiles and emits
  the intended store. No ROM with `NDS_R2_POSITION_PROBE=1` was executed, so the
  probe's captures were not observed to carry a nonzero index.
- **It does not re-declare the cadence gate.** §3 measures 96.08% on an R1
  control and leaves the shipping figure where `R2_SWITCH.md` §4 put it, 96.03%
  on the switched published ROM. The `≥95%` verdict and the choice of the
  2,043-frame population remain the owner's, per `SHIP_CADENCE.md` §4.1.
- **It does not establish the noise floor to better than one comparison.** §3.3's
  3-frame spread comes from two runs of one binary — c247's ROM no longer exists
  to hash, and neither arm was repeated. It is a lower bound on the spread, not a
  characterised distribution, and it should not be quoted as "the" noise floor
  without a repeat.
- **It does not attribute the animation-cache overflows.** §4 adds only that the
  cache is present on both paths. No Runtime 1 arm was run at a 5-minute match
  length, so "long matches overflow the cache" and "long matches overflow it on
  the R2 path" are still not separated. The shipping one-minute configuration
  reads 0.
- **It does not re-run Boundary or any soak.** `R2_SWITCH.md` §3 and §7 stand
  unchanged; the root pair this cycle ends with is bit-identical to the one they
  were measured on, which is why they still apply.
- **It does not commit, push, or snapshot.** The tree is left dirty for the
  orchestrator, alongside the owner's own uncommitted `PROJECT_GOAL.md` and the
  four untracked probe/analysis files, none of which were touched.

---

## 7. Files

| file | what |
|---|---|
| `PREDICTION.txt` | the §3 prediction, written before the c254 build existed |
| `c254-cadence.txt` / `c254-run.log` | the R1 control's histogram and run (stderr log is 0 bytes) |
| `make-p1.log` | the pin's verifying build at HEAD `83978bb4ed8` |
| `make-c253-posprobe.log` | the `NDS_R2_POSITION_PROBE=1` compile proof |
| `make-c254-r1ctl.log` | the R1 control build |
| `make-restore.log` | the relink that restored the published pair bit-exactly |
| `check-published-roms.log`, `check-tickhud-parity.log` | the closing checkers |

## 8. Reproduce

```powershell
# the pin's verifying build, and the determinism result
make p1
Get-FileHash .\smash64ds-battle-playable-hwtri.nds -Algorithm SHA256

# the stamp is absent from the published pair and present in the tick-HUD ROM.
# This one was run through the Bash tool -- `grep -a` on the raw bytes. Do not
# substitute `Select-String -Encoding Byte`; that parameter does not exist in
# PowerShell 7 and the command was never run in that form.
grep -c -a -o 83978bb smash64ds-battle-playable-hwtri.nds        # 0
grep -c -a -o 83978bb smash64ds-battle-playable-hwtri.elf        # 0
grep -o -b 83978bb builds/build-tick-hud-buckets/smash64ds-battle-playable-tickhud-hwtri.nds
#   -> 917152:83978bb   (the positive control)

# the drift fix, compile-proven (lab target -- does not touch the root pair)
make TARGET=smash64ds-battle-playable-proof-hwtri BUILD=build-c253-posprobe NDS_R2_POSITION_PROBE=1
& 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe' builds\build-c253-posprobe\smash64ds-battle-playable-proof-hwtri.elf | Select-String gNdsPositionProbeUpdateInPresent
& 'C:\devkitPro\devkitARM\bin\arm-none-eabi-objdump.exe' -d --start-address=0x02055b80 --stop-address=0x02055b90 builds\build-c253-posprobe\smash64ds-battle-playable-proof-hwtri.elf

# the R1 control -- THIS WRITES THE ROOT PAIR, hash it first
make TARGET=smash64ds-battle-playable-hwtri BUILD=build-c254-pubgate-r1ctl NDS_R2_LAB_R1_PATH=1 NDS_R2_BOTH_CPU=1
pwsh -NoProfile -File scripts\probe-present-cadence.ps1 `
     -Build build-c254-pubgate-r1ctl -Target smash64ds-battle-playable-hwtri `
     -Hits 1 -EndBreak mnVSResultsStartScene -TimeoutSeconds 2700 -RunnerSlot 6 `
     -Artifact artifacts\performance\2026-08-17_r2-residuals\c254-cadence.txt

# restore -- DELETE BOTH HALVES FIRST or make relinks nothing and exits 0
Remove-Item .\smash64ds-battle-playable-hwtri.nds,.\smash64ds-battle-playable-hwtri.elf
make TARGET=smash64ds-battle-playable-hwtri BUILD=build-battle-playable-canonical-hwtri-harness
```

**Use the full path to `arm-none-eabi-nm`.** It is not on PowerShell's `PATH` in
this environment, and a failed invocation prints nothing — which reads exactly
like a symbol that is not there. §2.1 nearly published that as an absence.
