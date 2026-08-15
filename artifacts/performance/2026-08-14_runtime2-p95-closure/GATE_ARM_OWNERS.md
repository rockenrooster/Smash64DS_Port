# The gate re-banks at 1,184,064, and Boundary's red is a regression, not a ceiling

**Date:** 2026-08-14 · **Branch:** `codex/r2-runtime2` · **HEAD `a159069af0d`.**
**Builds spent: 2** (`build-c158-gate`, `build-c159-profile-bothcpu`) plus one
rebuild of the existing Boundary proof target.
**UNITS: 2 profile cycles = 1 project tick.** Tick-HUD buckets are already ticks.
Every table states its window and its divisor.
Companion to `MARGINAL_OWNERS.md` (same directory), which this document extends
onto the gate arm and corrects in two places.

---

## 0. Outcome first

1. **The gate re-banks on the settled HEAD at `WORK-H` P95 `1,184,064` raw /
   `1,159,117` net, P50 `939,136`** — window frames 440–2039, 1,600 samples,
   `NDS_R2_BOTH_CPU=1`, `NDS_TICK_HUD_DRAW=1`, DLDI on. Gap to 1,120,380 is
   **+63,684 raw / +38,737 net**, down from +90,564 / +65,617. §2.
2. **That −26,880 may NOT be read as a cost win.** The end-of-match invariants
   differ — P1 damage **58 → 76** — so the two banks are not the same fight. The
   four owner-confirmed 2026-08-14 fixes changed gameplay outcomes, which is
   what a collision-bore fix is *for*. §2.3.
3. **Boundary's red is a regression, and the inherited "GDB-STUB CEILING, NOT A
   ROM HANG" framing is refuted with a same-session control.** A 1,800 s ceiling
   times out exactly as 120 s and 600 s did — 15x buys nothing — while the
   gate-arm tick-HUD ROM, on the **same stub, same runner slot, same DLDI, same
   HEAD**, booted and finished a longer workload (2,039 frames, 1,600 samples,
   17 ring stops) in **123 seconds**. §1.
4. **Breakpoint 2 is at the function entry** (`0x020250a0`, verified host-side),
   so "it never fired" means the call never happened, not that a guard skipped
   it. And gdb reports `nds_renderer.c:15535`, not `15504` — the inherited
   "misreport" note is wrong on this ELF. Breakpoint **1** hits 0.05 s after the
   attach, so the failure is in battle-scene setup, not in early boot. §1.2–1.3.
   **One of this cycle's own probes was malformed and its conclusion is
   retracted in §1.3**; the helper is fixed so that class cannot recur.
5. **The `gcRunAll` verdict reproduces on a second, different match**: P95-set
   excess `+508,993`, of which `SRC` is `+456,480` (**89.7%**) and `SRC − GCRA`
   is **−22** ticks. Two matches, two populations, same arithmetic proof. §3.
6. **But the ranking *inside* `gcRunAll` does not reproduce.** `SITR` is the only
   bracket large on both matches (+171,234 → **+188,907**); `SHDT` fell 119,920 →
   80,837, `SPHD` 112,833 → 75,236, and **`SPHC` went +62 → +52,780 (59.3x)**.
   `MARGINAL_OWNERS.md` §7 law 1 said to target presence, not the mean; this is
   that law biting a second time, on the bracket table itself. §3.2.

---

## 1. Task A — R0: what the Boundary marker capture actually does

### 1.1 The measurement the board asked for

`docs/P1_EXECUTION_BOARD.md` R0 asked for boot-to-marker **under the stub**,
measured rather than guessed. The Boundary wrapper caps
`-RendererBenchmarkTimeoutSeconds` at 600, so the loop harness was driven
directly with the identical Boundary argument set (`verify-battle-playable-harness.ps1:142-197`
reproduced verbatim, `-RendererProfileLevel 0`, `-FoxCpuMode 1`,
`-Target smash64ds-battle-playable-proof-hwtri`) and its own
`ValidateRange(5,3600)` ceiling raised to **1800**:

```text
GDB marker capture timed out after 1800 seconds (elapsed 1800s).
Temporary breakpoint 1, scVSBattleStartBattle ()
    at src/import/battleship_scvsbattle.c:243          <-- HIT
Temporary breakpoint 2 at 0x20250a0: file src/nds/nds_renderer.c, line 15535.
                                                       <-- never hit in 1800 s
```

**120 s, 600 s and 1800 s all fail identically.** A ceiling is therefore not the
mechanism, and no ceiling may be raised on this evidence.

### 1.2 The breakpoint is sound — checked, not assumed

| check | result |
|---|---|
| `nm -S` on the built proof ELF | exactly one text symbol each: `scVSBattleStartBattle` `0x0207fdec`/`0x16c`, `ndsRendererHardwareArmBattleStaticTextures` `0x020250a0` |
| `gdb -batch … tbreak … info breakpoints` (host only, no target) | bp lands at **`0x020250a0`**, the function's first instruction, `nds_renderer.c:15535` |
| `objdump -d`, live call sites of the arm function | **exactly one**, inside `syTaskmanRunTask` — the `NDS_R2_PATH=0` loop, at the Wait→GO transition (`src/port/taskman_seam.c:8100-8111`) |
| ARM/Thumb mode, vs the last green build | ARM in **both** `build-c158-gate`-era HEAD and `build-c147-ctl` (`bf22a37eec3`); `blx` from Thumb at both call sites. Mode change refuted. |

The function opens with three guarded early returns
(`gNdsRendererBattleStaticTextureEnabled`, `sNdsRendererBattleStaticTexturePrepared`,
`!sNdsRendererBattleStaticTextureArmed`), so it would have been easy to blame a
guard — but the breakpoint is **before** all three. It not firing means
`syTaskmanRunTask` never took the Wait→GO edge.

### 1.3 Where the guest gets to

A bounded probe (900 s) was written to stop at `scVSBattleStartBattle` and then
step the presented-frame marker (`ndsBattlePlayableFrameCompleteMarker`,
`0x0204fb54`, `taskman_seam.c:4645`, one live caller — `syTaskmanRunTask`) on a
schedule, printing guest counters and a host timestamp at each stop.

**RETRACTED: the probe was malformed and its "the frame marker never fires
again" reading is withdrawn.** `Invoke-GdbMarkerScript` took `[string[]]$Commands`,
and this probe built its script with a helper that returns *two* commands per
stop. Binding a jagged array to `[string[]]` **stringifies each inner array into
one space-joined element**, so every multi-command stop was fused onto a single
gdb line. The tell was a stray `game_status` file in the repo root: the fused
line read `shell cmd /c echo TIME=%TIME% printf "… gSCManagerBattleState->game_status …"`
and `cmd` took the `->` as a redirect. The MI path has guarded against exactly
this fusion since 2026-08-03; the batch command list — which every verifier in
the repo uses — had no guard at all.

**Fixed structurally, not documented:** `Commands` is now `[object[]]` and is
flattened in the body, so the jagged form is inexpressible rather than merely
detected; embedded newlines now throw. Demonstrated: the old binding turned a
4-element jagged list into 4 strings with `shell echo T_a printf "tag=a\n"` as
one of them; the new one yields 6.

**Two things survive the retraction, and they are the useful half.** The fused
`shell` still ran, in order, so its host timestamp is sound:

```text
TIME_attach       = 20:25:12.88     (clean, unfused command)
Temporary breakpoint 1, scVSBattleStartBattle () … :243     <-- HIT
TIME_startBattle  = 20:25:12.93     (recovered from the stray file)
```

**Breakpoint 1 hits 0.05 s after the attach.** Read with the harness run — which
reaches breakpoint 1 and never reaches breakpoint 2 in 1,800 s — the failure is
**after `scVSBattleStartBattle`, in battle-scene setup, not in early boot.** That
is a location, not a cause, and it is offered as one.

A probe that may be killed must also `set logging file …` + `set logging enabled on`
(verified on this gdb 14.1): gdb's own `printf` output is block-buffered into the
pipe and is lost with the kill, which is why the per-stop counters were never
recovered even from the lines that were not fused.

### 1.4 The control that makes this a regression

| | Boundary proof arm | gate tick-HUD arm |
|---|---|---|
| build | `build-battle-playable-proof-hwtri-harness` | `build-c158-gate` |
| HEAD | `a159069af0d` | `a159069af0d` |
| runner slot / stub / DLDI | 2 / melonDS gdb / on | 2 / melonDS gdb / on |
| workload | boot → GO → 211 frames → marker dump | boot → **2,039 frames**, 1,600 samples, 17 ring stops |
| wall time | **>1,800 s, never reached GO** | **123 s, complete** |

Same session, same host, same emulator, a strictly *larger* workload finishing in
1/15th of the failed budget. The stub is not slow and the host is not loaded.

Boundary green runs are recorded on 2026-08-13 in **both** configurations, each
inside the **default 120 s** marker ceiling:

| artifact | mtime | configuration | verdict |
|---|---|---|---|
| `…/2026-08-13_c-r2path-recheck/boundary-r2.log` | 08-13 | `$env:NDS_R2_PATH='1'`, HEAD `bf22a37eec3` | `battle_playable Pupupu realtime pacing smoke passed: frames=211` |
| `…/2026-08-13_c-fox-bore/boundary.log` | 08-13 17:20 | default (`NDS_R2_PATH=0`) | `verify-boundary.ps1 -DelaySeconds 3 -RunnerSlot 2 -> EXIT=0` |
| `…/2026-08-13_c-ledger-index/boundary.log` | 08-13 18:50 | default | `EXIT=0`, full 316,148-line log scanned, zero `Exception:` |
| **`…/2026-08-13_c-bore36-bgstretch/boundary.log`** | **08-13 22:01** | **default** | **`Boundary verification profile passed.`** |

So the `NDS_R2_PATH` axis is not the confound: the **default** configuration was
green at 22:01 on 2026-08-13, after `8fc8b47c9ce` (08-13 21:27). Every code
commit since is dated 2026-08-14:

```text
697303ed77c 14:36  battleship_ftanim.c, reloc_backend_compat_shims.c
33d7cc5d3b7 16:54  nds_renderer.c +224, generated particle banks
54d7d7862e4 18:55  battleship_sys_objanim.c
9b6c9e72a25 19:06  the four owner-confirmed fixes (effects/particle/platform/renderer/sprite_preview)
813207773c1 19:06  census alias pin
```

**That is the bisect window, and it is five commits wide.** It is not opened here
— this was an instrument cycle and a bisect is four to five builds. What this
section establishes is only that a bisect is the right instrument and a larger
ceiling is not.

### 1.5 What landed

`scripts/lib/gdb-markers.ps1` now reports elapsed on **success**
(`GDB marker capture: 118.4s elapsed of 120s ceiling (99% used)`) and on both
failure paths, and returns `ElapsedSeconds`/`TimeoutSeconds` on its result
object. R0 asked for exactly this: a capture that passes today tells nobody how
close it came, so the drift that eventually turns Boundary red is invisible until
it *is* a red. `Write-Host` rather than `Write-Output` because every caller
assigns this function's result.

**Boundary is NOT green on the settled HEAD, and §2/§3 below are therefore banked
on an unverified tree.** That is stated here so no later reader has to infer it.

---

## 2. Task B — the gate, re-banked on the settled HEAD

### 2.1 The arm

```text
build          builds/build-c158-gate
target         smash64ds-battle-playable-tickhud-hwtri
flags          NDS_R2_BOTH_CPU=1  NDS_TICK_HUD=1  NDS_TICK_HUD_DRAW=1  DLDI=ON
ROM SHA-256    E31B90F4CF24F10314FD40D5072EBF13CF978EDC665FCF23F45231898265263B
git            a159069af0d + dirty(10)   [owner's own files; see git status]
window         presented frames 440..2039, 1,600 samples, -RingDump, 17 ring stops
wall           123 s      repeated frame LABELS 4/1600 (ring-stop seams, payloads distinct)
rows           c158-gate-rows.csv     json  c158-gate.json
```

### 2.2 The numbers

```text
WORK-H  P50        939,136      (harness)            [939,168 mean-of-middle-two]
        P90      1,096,448      rank 160
        P95      1,184,064 raw  (harness, rank 81)   [1,184,832 at rank 80]
        top-1%   1,562,560      rank 16
        max      5,112,896
        P95 neighbourhood  r77 1,187,200 · r78 1,186,496 · r79 1,186,368 ·
                           r80 1,184,832 · r81 1,184,064 · r82 1,181,184 · r83 1,180,672
        net of apparatus 24,947 (RESIDUE.md §5, owner-approved 2026-08-13)
                 1,159,117
        gap to 1,120,380     +63,684 raw   /   +38,737 net
```

Cadence, **`NDS_TICK_HUD_DRAW=1` — this is the P95 bank arm, not the cadence
acceptance arm** (`plan.md` §6, owner chose (a) on 2026-08-14):

```text
over the 1,600 sampled frames   VBI 2:1359  3:229  4:9  5:1  6:1  10:1   max 10
                                two-VBlank 84.9%   cadence boundary WORK-H 1,116,288
harness's own whole-run count   VBI 2:1735  3:280  4:10  5+:13  max 25  total 2038
                                cadenceViolations 0
```

Both figures carry `NDS_TICK_HUD_DRAW=1`. The `DRAW=0` cadence is §4.4.

### 2.3 The delta against 1,210,944, and why it is not a cost claim

```text
old bank  build-c147-ctl  git bf22a37eec3   P50 924,864   P95 1,210,944
new bank  build-c158-gate git a159069af0d   P50 939,136   P95 1,184,064
delta                                       P50 +14,272   P95   -26,880
```

−26,880 clears the ~17,000 P95 placement floor (`STACK.md` §7 item 3 — three
near-identical arms spanned 16,832), so it is not linker noise. **It is still not
attributable to per-frame work**, because the two runs are not the same match:

| end-of-match invariant | c147 (`bf22a37eec3`) | c158 (`a159069af0d`) |
|---|---:|---:|
| `gNdsBattleTextHudP1Damage` | 58 | **76** |
| `gNdsBattleTextHudP0Damage` / stocks | 0 / 1,1 | 0 / 1,1 |
| `gNdsDamageSparkScaleCount` | 14 | 15 |
| `gNdsAObjEvent32NormalizedHighWater` | 1,177 | 1,266 |
| `gNdsAObjEvent32NormalizeScriptCount` | 183 | 186 |
| `gNdsAObjEvent32NormalizeReuseCount` | 1,574 | 1,609 |
| `gNdsAObjEvent32HashProbeCount` | 3,613 | 4,832 |
| `gNdsShieldAnimJointAttachCount` | 1,344 | 1,352 |
| `gNdsObjAnimRunawayCount` / `ArenaAllocFail` / `RelocResolveMisalign` | 0/0/0 | 0/0/0 |
| `gNdsTaskmanGeneralHeapFreeMin` | 70,592 | 70,464 |

Damage 58 → 76 is a **different fight**. Fox's shared beam/flash/collision bore
moved to 84 on 2026-08-14 with the owner's confirmation, so hits land
differently; that is the fix working, not a regression. The repo's own rule
(`route-ab-cannot-price-gameplay-change`) forbids reading ticks across arms whose
end-of-match counters diverge, and it applies to a re-bank exactly as it applies
to an A/B. **So: the bank moves to 1,184,064; the 26,880 is not banked as a win
and no lane may be sized from it.**

What *can* be said about the four fixes, from the linked images rather than from
the ticks — ELF sections, never the `.nds` (NitroFS packs directory entries
nondeterministically):

```text
.main (text)      921,720 -> 925,264     +3,544 B
.main.rw          137,428 -> 137,428          +0
.main.bss       1,471,824 -> 1,453,392   -18,432 B   (HW wallpaper decode cache 153,600 -> 135,000)
.itcm / .dtcm / .text.hot / .text.hot.draw    identical
fake_heap_start  0x0226d904 -> 0x02269ee4   -14,880 B  (heap grows; RAM headroom improves)
```

+3,544 B of text on a ROM whose measured one-line placement spread is ±24,064 is
inside the noise by construction; it cannot be separated from the −26,880 and no
attempt is made to.

---

## 3. The gate arm at bracket granularity — reproduced on a second match

Window: `c158-gate-rows.csv`, 1,600 samples, frames 440–2039, `DRAW=1`.
Reproduced by `scripts/census-tick-hud-p95-set.py` (new this cycle, validated
against `MARGINAL_OWNERS.md` §2.3 to the tick before it was used here).

### 3.1 The structure holds

| bracket | P95-set mean | 2-VBlank mean | **excess** | ratio | share | c147 excess |
|---|---:|---:|---:|---:|---:|---:|
| **WORK-H** | 1,435,193 | 926,200 | **+508,993** | 1.55x | 100% | +520,718 |
| **SRC** | 770,069 | 313,588 | **+456,480** | 2.46x | **89.7%** | +479,816 |
| ` ` `GCRA` | 764,892 | 308,389 | +456,503 | 2.48x | 89.7% | +479,885 |
| ` ` SRC outside GCRA | 5,177 | 5,199 | **−22** | 1.00x | −0.0% | −68 |
| ` ` ` ` `SITR` (=SINT−SCPU) | 293,962 | 105,055 | **+188,907** | 2.80x | 37.1% | +171,234 |
| ` ` ` ` `SCPU` | 33,039 | 41,708 | **−8,669** | 0.79x | −1.7% | +7,222 |
| ` ` ` ` `SHDT` | 86,408 | 5,571 | +80,837 | **15.51x** | 15.9% | +119,920 |
| ` ` ` ` `SPHD` | 137,195 | 61,959 | +75,236 | 2.21x | 14.8% | +112,833 |
| ` ` ` ` `SPHC` | 53,686 | 905 | **+52,780** | **59.29x** | 10.4% | **+62** |
| ` ` ` ` `SPRM` | 45,670 | 1,834 | +43,836 | 24.90x | 8.6% | +49,377 |
| ` ` ` ` `SCAT` | 11,399 | 1,222 | +10,177 | 9.33x | 2.0% | +91 |
| ` ` ` ` GCRA remainder | 103,533 | 90,134 | +13,399 | 1.15x | 2.6% | +19,146 |
| `MISC` | 143,087 | 116,769 | +26,318 | 1.23x | 5.2% | +16,414 |
| `AUD` | 23,626 | 7,391 | +16,235 | 3.20x | 3.2% | +15,775 |
| `FTR` | 299,514 | 290,431 | +9,083 | 1.03x | 1.8% | +7,987 |
| `STG` | 174,872 | 174,245 | +627 | 1.00x | 0.1% | +546 |
| `OTHR − WAIT` | — | — | +249 | 1.01x | 0.0% | +198 |

Both completeness identities close to **0** (`WORK-H` = named + `OTHR` − `WAIT`;
`GCRA` = the sum of its six children plus the remainder), frame by frame.

**`SRC − GCRA` = −22 on a second, different match.** `MARGINAL_OWNERS.md` §5
proved the nesting from two populations of one match; this is a third population
from another match. `gcRunAll` is the whole of the simulation excess, and the
draw side is `FTR` 1.03x · `STG` 1.00x · `MISC` 1.23x = **+36,028, 7.1%**.

### 3.2 What does NOT reproduce: the ranking inside `gcRunAll`

`SPHC` went from `+62` to `+52,780` (59.3x) and `SCAT` from `+91` to `+10,177`;
`SHDT` fell a third, `SPHD` a third, `SCPU` went negative. These are the
*event-driven* brackets — grabs, catches, live hitboxes — and the two runs are
different fights (§2.3). **`SITR` is the only bracket that is large on both**
(+171,234 → +188,907, 2.57x → 2.80x). A Phase 4 package sized off any of the
others is sized off one match's events.

---

## 4. Task C — the first v3 stall capture on the gate arm

### 4.1 The capture

```text
build       builds/build-c159-profile-bothcpu
flags       NDS_R2_BOTH_CPU=1  NDS_TICK_HUD=1  NDS_TICK_HUD_DRAW=0  NDS_TASK37_PROFILE=1
            NDS_TASK37_PROFILE_START=438  _FRAMES=1600  _PER_FRAME_REGION=1
emulator    emulators/melonds-attributor/melonDS.exe   (profile-v3, no gdb attached)
window      presented frames 438..2038, 1,601 regions, DLDI on, 560 s wall
output      v3-gate-arm/{arm9-profile.csv (3.62 GiB), .meta.txt, .regions.csv, census.{txt,json}}
UNITS       whole match ticks/frame = cycles / (2 x 1601) = cycles / 3,202
            P95-set   ticks/frame = cycles / (2 x 80)    = cycles / 160
            band      ticks/frame = cycles / (2 x 60)    = cycles / 120
```

`stall_partition_residual = -15` on 3,793,606,361 cycles (0.0000004%);
`timestamp_discontinuities = 1` (the Boundary capture had 0) — noted, not
material at this residual. `cart_spin = gx_paid = gx_blamed = 0`, as on Boundary.

**This arm is `DRAW=0` and is therefore NOT the P95 bank arm of §2** (`DRAW=1`).
The bridge between them is stated and checked: the profile's own non-idle
`P95(rank 80)` is **1,174,997 ticks** against the tick-HUD arm's `WORK-H`
`P95(rank 80)` of **1,184,832** — 0.8% apart on two different binaries and two
different instruments. Nothing below is transplanted across arms; the tick-HUD
brackets stay §3's, the per-function ranking stays this section's.

### 4.2 Whole-match stall classes — the gate arm, for the first time

ticks/frame, divisor 3,202. Boundary column from
`../2026-08-14_icache-temporal/v3-baseline` for shape only (different arm).

| class | gate arm | Boundary arm | |
|---|---:|---:|---|
| `icache_fill` | **354,678** | 339,275 | |
| `dcache_fill` | **269,944** | 252,032 | |
| `halt_wait` (idle) | 214,226 | 238,788 | gate arm idles 10% less |
| `issue` | **208,689** | 181,256 | +15.1%; instructions +9.8% |
| `write_buffer` | 48,914 | 47,549 | |
| `interlock` | 43,122 | 39,942 | |
| `bus_contention` | 36,738 | 36,282 | |
| `dma_hold` | 8,450 | 8,701 | |
| **total cycles/2/1601** | **1,184,762** | — | vs tick-HUD `ALL` mean 1,210,010 |

### 4.3 The P95 frames — mask, falsifier, and the class that owns the excess

Mask A = the **80** regions with the largest `total_cycles - halt_wait`
(threshold 1,174,997 tk; **never** on `total_cycles`, which is VBlank-quantized —
1,427 of 1,601 regions sit at exactly 2 VBlanks).
Mask B = the falsifier: the same band with its **20 most expensive frames
removed** (`[1,174,997 .. 1,436,968]` tk, 60 frames).

```text
ranked sum (issue+icache+dcache+wbuf+interlock+bus)
  whole match   962,086 tk/fr        P95 set  1,351,835 tk/fr     excess  +389,749  (1.41x)
  band (60)                                   1,248,305 tk/fr     excess  +286,220
```

**Which class holds the P95 excess — the question nobody could answer on this arm:**

| class | excess on the 80 | share |
|---|---:|---:|
| **`icache_fill`** | **+155,795** | **40.0%** |
| `dcache_fill` | +96,800 | 24.8% |
| `issue` | +94,029 | 24.1% |
| `write_buffer` | +21,431 | 5.5% |
| `interlock` | +18,075 | 4.6% |
| `bus_contention` | +3,618 | 0.9% |

Instruction fetch is the largest single component of the gate arm's P95 excess.
It is still not a majority: the three-way split `plan.md` §0 recorded on the
Boundary arm survives here (40 / 25 / 24 against 35 / 23 / 33).

### 4.4 Cadence on the `DRAW=0` arm — the owner-decided acceptance reading

`plan.md` §6, DECIDED 2026-08-14: cadence acceptance is read from a
`NDS_TICK_HUD_DRAW=0` arm. From this capture's own `arm9-profile.regions.csv`,
`round(total_cycles / 1,120,380)` = VBlanks per present, **and no gdb stub was
attached to this run at all**:

```text
NDS_TICK_HUD_DRAW=0   VBI 2:1427  3:162  4:10  6:1     max 6    two-VBlank 89.1%
NDS_TICK_HUD_DRAW=1   VBI 2:1359  3:229  4:9  5:1  6:1  10:1   max 10   two-VBlank 84.9%
```

(region 0 is the pre-window accumulator and is excluded; 1,600 counted each.)
Removing the HUD draw burst is worth **+4.2 cadence points** and takes the max
interval from 10 to 6. **89.1% against a >=95% target — the cadence deficit is
real and is not the instrument.** Caveat carried with the number: this arm has
`NDS_TASK37_PROFILE=1` compiled in (one CP15 region write per frame) and ran on
the attributor build of melonDS. A clean `DRAW=0` non-profile arm was not built;
that is one build and one 2-minute run for the next cycle.

### 4.5 The per-function ranking on the P95 frames

`issue + icache_fill + dcache_fill + write_buffer + interlock + bus_contention`,
ticks/frame, divisor 160, over 1,359 owners. **`issue` is the v3 partition's
residual class and can read negative per PC** — sound in aggregate, unusable
alone.

| tk/fr | share | issue | icache | dcache | wbuf | intlk | bus | owner |
|---:|---:|---:|---:|---:|---:|---:|---:|---|
| 48,731 | 3.6% | 46,803 | 1,929 | 0 | 0 | 0 | 0 | `__aeabi_fadd` |
| 37,492 | 2.8% | 35,282 | 2,209 | 0 | 0 | 0 | 0 | `__mulsf3` (=`__aeabi_fmul`) |
| 36,856 | 2.7% | 15,005 | 314 | 8,946 | 10,533 | 676 | 1,382 | `memcpy` |
| 30,241 | 2.2% | 3,960 | 10,153 | 11,625 | 3,275 | 1,228 | 0 | `ndsFighterMarioFoxDLAllDrawForSlot` |
| 28,942 | 2.1% | -221 | **23,629** | 4,545 | 88 | 901 | 0 | `ndsR2AnimValueQ` |
| 28,303 | 2.1% | 4,721 | 9,554 | 10,823 | 589 | 2,615 | 0 | `gcPlayDObjAnimJoint` |
| 28,127 | 2.1% | 6,297 | 12,844 | 7,352 | 396 | 1,238 | 0 | `ndsR2FtAnimParseDObjFigatree` |
| 27,718 | 2.1% | -1,130 | 15,443 | 8,150 | 477 | 1,587 | 3,192 | `ndsRendererCommitNativeStageSegment` |
| 26,247 | 1.9% | 10,956 | 0 | 3,563 | 142 | 1,299 | **10,287** | `ndsRendererNativeEmitProductionPrimitiveGroups` |
| 24,390 | 1.8% | 11,060 | 499 | 8,694 | 2,449 | 1,636 | 51 | `ndsRendererExecuteNativeFighterOwnerProduction` |
| 21,776 | 1.6% | 5,485 | 90 | 809 | **15,245** | 147 | 0 | `memset` |
| 19,716 | 1.5% | 9,181 | 142 | 5,155 | 1,484 | 1,902 | 1,852 | `ndsRendererNativePrepareProductionRun` |
| 18,761 | 1.4% | -423 | 13,718 | 3,687 | 297 | 1,482 | 0 | `ndsRendererMtxMulAffine20p12` |
| 16,833 | 1.2% | 1,115 | 12 | **13,914** | 1,792 | 0 | 0 | `armCopyMem32` |
| 16,608 | 1.2% | -99 | 11,368 | 3,890 | 22 | 1,428 | 0 | `get_fat.isra.0` |
| 16,456 | 1.2% | -535 | 8,403 | 4,778 | 417 | 1,406 | 1,988 | `ndsRendererNativeStageBeginRun` |
| 15,210 | 1.1% | 2,390 | 4,499 | 6,982 | 413 | 926 | 0 | `ftParamUpdateAnimKeys` |
| 13,539 | 1.0% | 13,363 | 176 | 0 | 0 | 0 | 0 | `__divsf3` (=`__aeabi_fdiv`) |

**No function exceeds 3.6%.** The three-way stall split of §4.3 has a matching
shape at function granularity: there is no single owner to delete.

**Inline attribution (`addr2line -f`) changes the answer in four places**, which
is why both are printed: `ndsRendererCommitNativeStageSegment` 27,718 -> 9,724
with `ndsRendererTask29GXRecord` surfacing at 16,166 (inlined into it);
`ndsFighterMarioFoxDLAllDrawForSlot` dissolves, `ftDisplayMainDrawDefault`
appearing at 9,901; `ndsBaseGcRunAll` surfaces at 9,543;
`ndsRendererNativeStageBeginRun` 16,456 -> 8,710. 44,717 tk/fr (3.3%) falls below
the 30,000-PC attribution cut and is labelled as such rather than distributed.

**The falsifier holds.** Mask B's top rows are the same functions in the same
order within ~2%, and exactly the load-frame items shrink:
`memcpy` 36,856 -> 24,937, `memset` 21,776 -> 16,539, `armCopyMem32` 16,833 ->
12,746, and `get_fat` leaves the top eighteen. The ranking is not an outlier
artefact.

### 4.6 Attribution inside `gcRunAll` — static closure, and what it can and cannot say

The profiler has no call stack, so "inside `gcRunAll`" is answered by
intersecting the ranking with each fighter proc's **static `bl`/`blx` closure**
from the linked ELF, rooted at the `battleship_` symbol each tick-HUD wrapper
forwards to (`reloc_backend_diagnostic_recorders.c:5632-5780`). `gcRunAll`
itself is useless as a root — its own closure is 56 functions, because it
dispatches every proc through a `GObj` function pointer.

**This is not a bound in either direction** and the tool now says so: it misses
indirect calls (a proc's true closure is larger) and it charges a shared leaf
such as `__aeabi_fadd` to every proc that can reach it. Only the exclusive rows
are clean.

P95-set **excess** (mask-A mean minus whole-match mean), ranked six classes:

| closure | whole match | P95 set | **excess** | share |
|---|---:|---:|---:|---:|
| fighter-proc closure only | 214,824 | 354,346 | **+139,522** | 35.8% |
| neither closure | 206,004 | 326,669 | +120,665 | 31.0% |
| both closures | 117,321 | 214,206 | +96,885 | 24.9% |
| **draw closure only** | 423,937 | 456,615 | **+32,678** | **8.4%** |

**Two instruments, two arms, one answer.** The tick-HUD brackets on the `DRAW=1`
arm put the draw side at `+36,028` = 7.1% of the P95 excess; the v3 profile on
the `DRAW=0` arm puts the draw closure at `+32,678` = 8.4%. That agreement is
the strongest thing in this document, because the two measurements share no
instrument, no binary and no match.

Exclusive-owner detail (symbol census, ticks/frame on the 80):
`SINT` 19,243 over 29 functions — `battleship_ftMainProcUpdateInterrupt` 7,945,
`ftComputerCheckFindTarget` 1,460, `ftComputerCheckDetectTarget` 1,395,
`ndsStageMPCeilFloorLoopSweep` 1,203. `SHDT` 5,946 over 32 —
`battleship_ftMainSearchHitFighter` 2,130. `SPRM` 2,044, `SCAT` 700, `SPHD` 560,
`SPHC` 298. The remaining ~42% of the fighter-side cost is in helpers shared by
five or six procs, which no static method can split.

### 4.7 `icache_fill`: where the 40% actually lives — and it is not the demoted lane

`plan.md` §7 demoted the hot-instruction-footprint lane and set one re-admission
condition: *"the gate-arm v3 capture shows `icache_fill` concentrated inside
`gcRunAll` on the P95 frames"*. The capture answers it, and the answer is **no
for that lane's two objects**:

| closure | whole-match icache | P95-set icache | **excess** | share |
|---|---:|---:|---:|---:|
| fighter-proc closure only | 99,371 | 163,918 | **+64,547** | **41.4%** |
| neither closure | 77,130 | 127,575 | +50,445 | 32.4% |
| both closures | 24,550 | 47,410 | +22,860 | 14.7% |
| **draw closure only** | 153,627 | 171,571 | **+17,944** | **11.5%** |

The draw side carries the largest *absolute* fetch (153,627 tk/fr whole match)
and almost none of the *excess* (+17,944). `HOT_FOOTPRINT.md`'s targets —
`scene_backend.o` and `nds_renderer.o` — are that flat 153,627. **So Phase 2
stays demoted on its own condition: it is a P50 lane, now measured as such on the
gate arm.** The three largest fetch owners that *are* inside `gcRunAll` are
`ndsR2AnimValueQ` 23,629, `ndsR2FtAnimParseDObjFigatree` 12,844 and
`gcPlayDObjAnimJoint` 9,554 — 46,027 tk/fr of instruction fetch on the P95
frames, in the animation evaluator.

### 4.8 The soft-float answer (`plan.md` §10's open question)

**On the 80 P95 frames of the gate arm**, `__aeabi_fadd` + `__aeabi_fmul` +
`__aeabi_fdiv` are **99,762 tk/frame**, **95.7% `issue`**, 4.3% `icache_fill`,
**no `dcache` component at all** — the Boundary arm's 82,274 / 97% reproduces in
shape and grows on the gate arm. The whole 14-helper family, attributed to its
callers by `analyze-leaf-helper-attribution.py` over this capture, is
**237,855,196 cycles = 74,283 tk/frame whole match**, 7.65% of non-idle work.

**Do those calls live inside `gcRunAll`? Partly — and that is the answer that
keeps soft float out of Phase 4.** Classifying every caller by static closure:

| caller closure | tk/fr whole match | share |
|---|---:|---:|
| fighter-proc closure only | 28,194 | **38.0%** |
| draw closure only | 23,159 | **31.2%** |
| neither | 15,257 | 20.5% |
| both | 7,673 | 10.3% |

Largest callers inside the fighter procs: `ndsBaseGcPlayMObjMatAnim` 5,402,
`ndsR2FtAnimParseDObjFigatree` 2,550, `ndsStageMPAdjustFloorLoopWallSweep` 2,000,
`ndsBaseGcPlayDObjAnimJoint` 1,999, `mpCollisionGetFCCommonFloor` 1,934,
`syUtilsArcTan` 1,914. Largest on the draw side: `syMatrixLookAtReflectF` 4,726,
`ndsRendererSubmitParticleQuad` 3,300, `guMtxCatF` 2,649, `syMatrixPerspFastF`
2,623.

So: **soft float does NOT promote to a Phase 4 package.** A third of it is draw
side, where the P95 excess is 8.4%; the `gcRunAll` half is real but is spread
over ~150 caller functions whose largest is 5,402 tk/frame. Its lever is
unchanged — *execute fewer of them* — and it remains a finisher that rides a
larger build, exactly as `plan.md` §10 has it. What is new is the number and the
split, so nobody re-sizes it from the 82,274 alone.

---

## 5. What a Phase 4 package should attack, and what it must clear

### 5.1 The requirement, restated on the new bank

```text
the 80th-largest WORK-H frame must fall from 1,184,832 to 1,120,380  =  64,452
   (91,844 was the OLD bank's requirement and is superseded)
net-of-apparatus, the same frame must fall                            =  39,505
```

### 5.2 The three candidate mechanisms, sized on the P95 set and on the falsifier

Ranked six classes, P95-set mean minus whole-match mean, mask A (80 frames) and
mask B (60 frames, the 20 most expensive removed):

| mechanism | mask A total | **mask A excess** | mask B total | **mask B excess** |
|---|---:|---:|---:|---:|
| **in-match asset load / file I/O** | 129,937 | **+93,436** | 87,777 | **+51,276** |
| animation evaluation | 110,564 | +38,465 | 109,104 | +37,006 |
| soft float | 108,572 | +39,843 | 109,847 | +41,118 |
| *(whole P95 set)* | 1,351,835 | +389,749 | 1,248,305 | +286,220 |

- **in-match asset load / file I/O** = `get_fat` +15,058 (10.7x) · `armCopyMem32`
  +15,025 (9.3x) · `f_lseek` +9,509 (10.8x) · `memcpy` +22,531 (2.6x) ·
  `ndsRelocNormalizeFighterAObj16File` +7,369 (**19.2x**) ·
  `ndsRelocAssetIDForToken` +6,012 (6.1x) · `mutexUnlock` +5,567 (11.6x) ·
  `memset` +7,164. Every one is *absent* from an ordinary frame and *present* on
  a P95 frame — the exact shape `MARGINAL_OWNERS.md` §7 law 1 demands.
- **animation evaluation** and **soft float** are almost perfectly invariant
  between the two masks. They are the reliable body of the set; they are not
  where the frames spike.

### 5.3 The named mechanism

**Attack the in-match animation-asset load path: the file I/O, the reloc
normalization and the copies it drives, on the frames where a fighter first
needs an animation asset.** It is **+93,436** on the 80 frames that set P95 and
still **+51,276** after the falsifier removes every load-frame outlier — the only
candidate above the >=30,000 architecture bar on both masks, and the only one
whose cost can be *moved out of the frame* rather than made cheaper, which is
what `PROJECT_GOAL.md`'s "Compute Once, Not Every Frame" asks for. Slice 46
already did exactly this once for the animation cache (warm preload, misses
32 -> 2, P95 1,213,440 -> 1,196,224); this is the same lever on the part of the
working set that preload did not cover.

Two recorded closures this **does not** re-open, and the line that separates
them: `docs/HANDOFF.md` closes "in-match FGM I/O" — that is the **sound-effect**
load, measured and refuted at `…/2026-08-13_c-band-io/` — and records that "the
animation one prices **+0**". That `+0` is a **whole-match mean on the Boundary
arm**. On the gate arm's P95 frames the animation-side symbols
(`ndsRelocNormalizeFighterAObj16File`, `ndsRelocAssetIDForToken`) carry +13,381
of excess at 19.2x and 6.1x presence, and the FAT/copy layer they drive carries
another ~+67,000. A refuted lane is not a refuted bug; this is the lane's
*animation* half, on the *gate* arm, at the *percentile*, and none of those three
were true of the closure.

**The engagement counter that must exist on the gate arm before any code
changes** (`plan.md` §9 law 1 — target the presence, not the mean): a
**per-presented-frame count of in-match asset acquisitions** — calls to
`ndsRelocAssetFindEntry`, `f_lseek`/`disk_read` invocations, and bytes moved by
`armCopyMem32` — sampled with `sample-tick-hud-buckets.ps1 -PerFrameGlobals` on
the `BOTH_CPU=1` arm, so the count can be intersected with the 80 frames that set
P95. If those counters are not concentrated on those 80 frames, this mechanism is
refuted before a build is spent. The prediction to write down first: they should
read ~0 on the 1,359 two-VBlank frames and non-zero on most of the 80.

**It must clear 64,452 at the 80th-largest frame** — not 30,000 on average — and
the whole distribution must be re-ranked afterwards, never the top rows.

> `BLOCKED(decision: are in-match asset-load frames "loading states"?)`
> `docs/P1_EXECUTION_BOARD.md` row 1 records the owner's gate as "loading states
> excluded (owner, 2026-08-05)". Every banked P95 in this campaign, including
> both figures in §2, is computed over all 1,600 samples with no exclusion. If
> these frames are ruled loading states, the gap closes on paper and this
> mechanism is out of scope; if they are ordinary gameplay frames — they occur
> inside the match, triggered by moves — it is the largest thing on the board.
> The options and the price are stated; the call is the owner's.

---

## 6. Corrections this document makes to the record

1. **`docs/HANDOFF.md`'s "THE REALTIME RED IS A GDB-STUB CEILING, NOT A ROM HANG
   — do not chase the ROM" is refuted.** 1,800 s fails identically to 120 s, and
   a larger workload on the same stub finishes in 123 s. §1.
2. **"gdb misreports its file as `nds_renderer.c:15504`" is wrong on this ELF** —
   gdb reports `:15535`, and the breakpoint address is the function's first
   instruction. §1.2.
3. **`MARGINAL_OWNERS.md` §7's bracket ranking is match-specific below `SRC`.**
   Its `SHDT` / `SPHD` / `SPRM` / `SPHC` numbers do not reproduce on a second
   match of the same configuration; `SITR` does. §3.2.
4. **`plan.md` §7's re-admission condition for the hot-footprint lane is
   measured and NOT met**: the draw closure holds 11.5% of the P95 `icache_fill`
   excess. The lane stays demoted, now on gate-arm evidence rather than on
   inference from the bracket table. §4.7.
5. **`plan.md` §10's soft-float question is answered**: 38.0% of the family's
   caller-attributed cost is inside the fighter procs, 31.2% is draw side. It
   does not promote. §4.8.
6. **The gate requirement is 64,452 at the 80th-largest frame, not 91,844.** §5.1.

## 7. What this document does NOT do

- **It does not make Boundary green**, and it does not identify the commit that
  broke it. It measures the failure, refutes the inherited explanation, and
  bounds the bisect window to the five 2026-08-14 code commits (§1.4).
  **§2 and §3 are therefore banked on a tree whose widest verifier is red.**
- **It did not recover the elapsed time at which breakpoint 1 hits**, because
  gdb's buffered stdout was lost when the stub was killed. §1.3 names the fix
  (`set logging enabled on`, verified on this gdb) for whoever runs the bisect.
- **It does not attribute the -26,880 P95 movement to the four fixes.** The
  matches diverge; §2.3 says so and stops.
- **It does not build a clean `NDS_TICK_HUD_DRAW=0` non-profile arm.** The
  cadence figure in §4.4 carries `NDS_TASK37_PROFILE=1`.
- **It does not price a Phase 4 package or write any optimization.** This was an
  instrument cycle; §5 names a target and the counter that must precede it.
- **It does not split the shared-helper ~42%** of the fighter-side cost between
  the six procs. No static method can, and the profile has no call stack.
- **It does not re-open any `plan.md` §3 lane.** §5.3 states precisely which
  closure it borders and why the border is not crossed.

## 8. Reproducing this

```powershell
# gate bank + P95-set decomposition (rows CSV is committed; the run is 123 s)
.\scripts\sample-tick-hud-buckets.ps1 -RunnerSlot 2 -Build build-c158-gate `
    -MakeFlags NDS_R2_BOTH_CPU=1 -Samples 1600 -RingDump -TimeoutSeconds 2400 `
    -RowsCsv artifacts/performance/2026-08-14_runtime2-p95-closure/c158-gate-rows.csv `
    -JsonOut artifacts/performance/2026-08-14_runtime2-p95-closure/c158-gate.json
python scripts/census-tick-hud-p95-set.py `
    --rows artifacts/performance/2026-08-14_runtime2-p95-closure/c158-gate-rows.csv

# v3 gate-arm capture (560 s, writes 3.62 GiB that is NOT committed)
.\scripts\run-task37-profile-census.ps1 -MelonDS emulators\melonds-attributor\melonDS.exe `
    -Build build-c159-profile-bothcpu -StartFrame 438 -Frames 1600 `
    -MakeFlags NDS_R2_BOTH_CPU=1,NDS_TICK_HUD_DRAW=0 -TimeoutSeconds 7200 `
    -OutDir artifacts/performance/2026-08-14_runtime2-p95-closure/v3-gate-arm

# reduce twice -- these two CSVs ARE committed, so no future cycle re-scans 3.6 GB
python scripts/census-marginal-frame-owners.py --reduce `
    --profile artifacts/performance/2026-08-14_runtime2-p95-closure/v3-gate-arm `
    --out artifacts/performance/2026-08-14_runtime2-p95-closure/gate-p95-pc.csv --marginal 80
python scripts/census-marginal-frame-owners.py --reduce `
    --profile artifacts/performance/2026-08-14_runtime2-p95-closure/v3-gate-arm `
    --out artifacts/performance/2026-08-14_runtime2-p95-closure/gate-band-pc.csv `
    --band-min 1174997 --band-max 1436968
# free to re-rank from here on
python scripts/census-marginal-frame-owners.py --report --owner-roots `
    --pc-csv artifacts/performance/2026-08-14_runtime2-p95-closure/gate-p95-pc.csv `
    --build builds/build-c159-profile-bothcpu --top 24
```
