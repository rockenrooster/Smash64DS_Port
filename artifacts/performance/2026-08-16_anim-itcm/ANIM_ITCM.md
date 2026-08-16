# The largest fidelity-neutral item on the board was a placement question, and ITCM had 2,454 dead bytes in it. The kernel is in ITCM, measured on a zero-noise route, and the level is +71,569.

**Date:** 2026-08-16 · **Branch:** `codex/r2-runtime2` · **base HEAD `5ac5ef04f48`**
2 lab builds (`build-c218-animitcm`, `build-c219-animitcm-ship`), 3 whole-match gate
runs, 0 gameplay values changed, no default flipped that costs fidelity, no ROM
published, both root ROMs byte-unchanged. Boundary green, 0 `Exception:`.
**UNITS: 1 project tick = 1 `cpuGetTiming()` tick = 2 ARM9 cycles.** Every table states
its window.

```text
REQUIREMENT  +73,425 net ticks per presented frame at rank-80.  Basis:
             build-c217-tilesync-ship, rank-80 1,218,752 raw / 1,193,805 net
             against the 1,120,380 gate (apparatus 24,947); SHIPPING renderer
             (GX_COMPOSE 0), bore 0, mode 163 one-minute match, NDS_R2_BOTH_CPU=1,
             1,600 samples, frames 439-2038, slips=0
             (../2026-08-16_tilesync-memo/TILESYNC.md section 3.3).

FOUND FIRST  ITCM IS NOT FULL, IT IS 2,454 BYTES DEAD.  The per-PC census that
             priced ndsR2AnimValueQ also enumerates every ITCM resident, and 33
             non-overlapping blocks execute ZERO instructions across the whole
             1,600-frame window.  Two of them are libgcc members the port itself
             replaced; one is a LUT builder that only runs on a cache miss.  No
             build was needed to find this: it is `nm` joined to a CSV already on
             disk.  Section 1.

CORRECTION   "A KERNEL IS FETCHED WHOLE, ON EVERY CALL, BY CONSTRUCTION" IS NOT
             TRUE OF THIS ONE, and the census that the claim was drawn from says
             so per PC: 162 of ndsR2AnimValueQ's 257 instruction slots and 23 of
             its 33 cache lines carry any execution at all.  320 bytes are never
             fetched.  The 21.13 tk/fr per byte it was used to justify is still
             right as a MEASURED price; the mechanism attached to it was not.
             Section 2.

ITEM A       ndsR2AnimValueQ IS IN ITCM AND IT IS MEASURED ON A SAME-BINARY
             ROUTE.  Placement is a link-time property of a symbol, so it cannot
             be routed on one copy -- but it CAN be routed between two identical
             copies at two addresses, which turns a cross-build question with a
             >=14,080 rank-80 floor into a zero-repeat-floor difference.
               the two copies                    257 words, 0 DIFFERING WORDS
               engagement equality  gNdsR2CubicEvals 290,076 on BOTH arms
               paired median, whole run                          -3,840  (85.8%)
               paired median, marginal-80                        -6,432  (73/80)
               rank-80                                          -14,208
               ranks 41-120 band                                -15,572
               complement control     WORK-H -3,840 / WAIT +3,840 / ALL +0
             Every one of 14 sampled ranks improves, rank 1 through rank 1200.
             Sections 3 and 4.

ITEM B       THE BIND PLACEMENT IS NOT BUILT, AND IT HAS TWO SEPARATE BLOCKERS,
             BOTH QUANTIFIED.  It ranks second on the metric the brief asked for
             (14.19 tk/fr per byte against 21.13), it does not FIT -- 268 B
             wanted against 220 B free on the instrument after Item A -- and at a
             3,802 tk/fr port-reachable ceiling it is INSIDE the >=14,080
             cross-build floor, so it needs its own route, not just its own
             build.  Section 6.

NEW BASIS    build-c219-animitcm-ship, rank-80 1,216,896 raw / 1,191,949 net.
             LEVEL +71,569.  The cross-build delta is -1,856 and is NOT banked:
             it is deep inside the >=14,080 floor and the band moved the other
             way (+7,074).  The route is the price; this is the level.
             Section 5.

BANKED       -3,840, the most conservative of five readings of one zero-noise
             instrument.  Section 4.4 states all five and why they differ.

BYTES        .itcm +292 (32,224 -> 32,516 of 32,736; free 512 -> 220 on the
             tick-HUD instrument, 2,572 on the proof ROM).  .main -296.
             Quote the target with the number.  Section 7.

CHECKER      check-task9-float-itcm.ps1 asserted a PLACEMENT POLICY where it
             meant to assert a RENAME, and went red on this change.  It now reads
             each member's placement from the build's own emitted object name.
             It still fails on a mismatch -- proven, section 8 -- and it caught a
             stale-object hazard in the Makefile recipe that a looser guard would
             have shipped.
```

---

## 1. The discriminating read cost one `nm` and one CSV that was already on disk

`TILESYNC.md` §5.3 handed this cycle a number (21,719 tk/fr of `icache_fill`) and a
constraint (512 B free ITCM against a 1,028 B kernel). The constraint is what makes the
item hard, so the first question was not "how do I shrink the kernel" but **"is ITCM
actually full?"**

`c200-off-pc.csv` is per-PC. Joined against the linked ELF's symbol table it prices every
ITCM resident, not just the ones someone thought to ask about. `itcm-census.txt`:

| what | bytes | why it is dead |
|---|---:|---|
| `_arm_cmpsf2.o` (`__gesf2`/`__lesf2`/`__cmpsf2`/`__aeabi_cfrcmple`/`__aeabi_cfcmpeq` + five `fcmp` goldens) | 276 | the port defines `__aeabi_fcmpeq/lt/le/ge/gt` itself; the Task 9/16 `--redefine-sym` filters rename libgcc's copies to `*_golden`, which **by construction have no caller** |
| `_arm_unordsf2.o` (`__unordsf2` / `fcmpun_golden`) | 56 | same |
| `ndsRendererHardwareGetLightShadeLut` | 404 | the LUT **builder**; `…FindLightShadeLut` is the lookup. The shade-LUT set stabilises before the window |
| exception vectors + handlers, `armDCacheFlushAll`, `armICacheInvalidateAll`, `threadUnblockAllByValue`, the two raw-run emitters, `ndsRendererNativeApplyStateSpan`, `ndsFTParamsInvalidateFighterParts`, … | 1,718 | also zero, **not taken** — §6 |
| **total cold** | **2,454** | |

**736 B taken. The rest was left**, because a kernel path's silence during one match is
not proof it is unreachable, and this cycle did not need it.

> **The first version of this table was wrong and the error is worth recording.** `nm`
> reports `__aeabi_frsub` at 456 B, `__subsf3` at 448 and `__addsf3` at 444 — three
> *overlapping aliases of one blob*. Summing them gave 33,486 B of ITCM inside a 32,736 B
> region, i.e. a total that could not exist. Collapsing to non-overlapping blocks first is
> what makes the 2,454 real. **`[[read-arrays-as-arrays]]`, applied to a symbol table.**

The two float members are also the reason the golden bodies exist at all: `Makefile:3845`
renames libgcc's `__aeabi_fcmpeq` out of the call graph so the port's replacement links.
The rename is exactly what makes the body unreachable, so the census and the build
mechanism agree from two independent directions.

---

## 2. The correction: this kernel is not fetched whole

`TILESYNC.md` §5.2 justified 21.13 tk/fr per byte with "a kernel is fetched **whole**, on
every call, **by construction**". The per-PC census it was computed from refutes the
mechanism while confirming the price:

```text
ndsR2AnimValueQ  0x02068c74  0x404 = 1,028 B   370.6 entries/frame (entry-PC count)
  instruction slots with any execution     162 of 257
  cache lines with any execution            23 of 33
  never-fetched tail                        10 lines = 320 bytes
  marginal-80 total                     26,664 tk/fr
  marginal-80 icache_fill               21,719 tk/fr   = 81.4% of it
```

It is a three-arm switch (Step / Linear / Cubic), and the Cubic arm's straight-line block
reads a uniform 1,540.7 instructions a frame across twelve consecutive lines — 192.6
passes, **52.0% of the 370.6 entries**. The other arms are short.

This matters for the decision it drives: the whole 1,028 B was moved rather than a
hand-split hot half, **because the cold 320 bytes cost nothing to leave in place** — they
are already free of fills, so splitting them out would have bought ITCM space at the price
of a source change with no fetch saving attached to it.

The price itself stands: 21,719 tk/fr over 370.6 entries is 117 cycles of fetch per entry,
about twelve line fills of a twenty-three-line working set, i.e. the lines genuinely do not
survive between entries.

> **Premise flagged, not re-measured.** That census is `build-c200-trackprof-off`, which
> differs from the shipping configuration in `NDS_R2_FIGHTER_GX_COMPOSE` (1 vs 0) and
> `NDS_R2_FTANIM_TRACK`/`_DISPATCH` (1/0 vs 0/1). The 21,719 is therefore a **size**, not a
> prediction for the shipped ROM, and §4 measures the shipped configuration directly rather
> than inheriting it. The two evictions were checked against this: the raw-run emitters are
> gated by the **strip route** (default 2, shipped), not by GX_COMPOSE.

---

## 3. What was built

Three edits, all placement:

| | |
|---|---|
| `Makefile` | `NDS_TASK9_FLOAT_MAIN_MEMBERS := _arm_cmpsf2.o _arm_unordsf2.o` — still extracted, still `--redefine-sym`'d, still in `$(OFILES)`; only `--rename-section` is skipped |
| `src/nds/nds_renderer.c` | `ndsRendererHardwareGetLightShadeLut`'s declaration loses `NDS_TASK82_ITCM_CODE` |
| `src/import/battleship_sys_objanim.c` | the kernel body becomes an `always_inline` impl; `NDS_R2_ANIM_Q_ITCM_ON` (default 1) puts the out-of-line wrapper in `.itcm` |

**The suffix is the load-bearing part of the first one, and dropping `--rename-section`
alone freed nothing.** `linker/nds_hot_text.ld:113` reads

```ld
*.itcm.* (.text .stub .text.* .gnu.linkonce.t.*)
```

— a **filename** rule. A file still named `_arm_cmpsf2.itcm.o` has its `.text` placed in
ITCM whatever the section is called, which is why the first link overflowed by exactly the
332 bytes that were supposed to have moved. The member now leaves the pattern as
`_arm_cmpsf2.mainram.o`.

### 3.1 The route, and why a placement change can have one

Placement is a property of a symbol's address, so it cannot be toggled on one copy. It can
be toggled **between two copies of one body at two addresses**, and that is the whole
design: `ndsR2AnimValueQItcm` in `.itcm`, `ndsR2AnimValueQMain` in `.main`, one `.data`
word choosing the `bl`. Both copies are emitted from one `always_inline` impl so they
cannot drift.

```text
nm build-c218-animitcm:
  01ffeee4 00000404 t ndsR2AnimValueQItcm      <- ITCM  (0x01ff8020..0x02000000)
  02068a88 00000404 t ndsR2AnimValueQMain      <- .main
  020ea228 00000004 D gNdsR2AnimItcmRoute      <- .data, NOT .bss
```

`.data` and not `.bss` for the reason `nds_r2_sqrtf.c` states: a zero-initialised route
word with no explicit section lands in `.bss` and drags a ~10,000 tk/fr placement floor.

**Disassembled before measuring.** The two copies are **257 instruction words each with
ZERO differing words** — same encodings, same order, different addresses. There is no
codegen difference for the measurement to be confounded by; the arms differ in one thing.

---

## 4. The measurement

| | |
|---|---|
| target | `smash64ds-battle-playable-tickhud-hwtri` |
| config | `NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1 NDS_R2_ANIM_ITCM_ROUTE=1`, GX_COMPOSE 0, bore 0, DLDI on |
| window | 1,600 samples, frames 439–2038, `-RingDump`, `slips=0` on both arms |
| raw | `a0.json`, `a1.json`, `a*-rows.csv`, `route-arms.txt` |

### 4.1 Engagement, and the control that could have failed

```text
gNdsR2CubicEvals = 290,076      arm 0 (.main)
gNdsR2CubicEvals = 290,076      arm 1 (.itcm)
```

**Bit-identical**, as a pure placement change requires: both arms run the same
instructions on the same inputs and must reach the Cubic arm the same number of times. A
change that perturbed the animation would move this counter, and it did not.

### 4.2 The result

| statistic | arm 0 → arm 1 |
|---|---:|
| paired median, whole run | **−3,840** (85.8% of 1,600 frames improve) |
| paired median, marginal-80 | **−6,432** (73 of 80 improve) |
| rank-80 | **−14,208** |
| ranks 41–120 band mean | **−15,572** |
| P50 | 940,192 → 936,640 (**−3,552**) |
| complement control | `WORK-H −3,840` / `WAIT +3,840` / `ALL +0` |

**The complement is the control and it is exact to the tick.** `ALL` is VBlank-quantised
(`[[all-is-a-quantized-gate]]`), so at a fixed presented cadence a genuine work deletion
must show as work down and idle up by the same amount. VBI histograms: arm 0
`2:1719 3:292 4:19 5+:8 max:19`, arm 1 `2:1720 3:292 4:18 5+:8 max:19`.

Per-bucket marginal-80 paired medians land where the mechanism says they must:
`SRC −5,760`, `GCRA −5,728`, `SINT −4,832`, `FTR −128`, `STG −64`. The animation chain
moved; the stage and fighter draw did not.

### 4.3 The rank curve improves everywhere

Unlike last cycle, rank-80 is **not** a local kink here — every sampled rank improves:

```text
rank    1  -125,696     rank   60   -33,408     rank  240   -4,736
rank    5   -13,376     rank   80   -14,208     rank  400   -4,160
rank   10   -18,816     rank  100   -18,304     rank  800   -3,584
rank   20   -22,912     rank  120   -17,216     rank 1200   -2,432
rank   40    -8,896     rank  160    -5,376
```

**The ±4.2M frames are pairing artifacts, not results.** Three frames (766, 1608, 990)
improve by ~4,197,000 and two (1529, 1902) regress by the same — cartridge frames landing
one presented frame apart between arms. They are why the marginal-80 **mean** reads
−172,098 against a median of −6,432. `[[the-obvious-statistics-lie-on-this-driver]]`:
quote the median. The trimmed (4/4) marginal-80 mean is −17,546, between the two.

### 4.4 What is honestly bankable, and why five readings differ

```text
whole-run paired median      -3,840     most conservative; BANKED
marginal-80 paired median    -6,432     the population the gate lives in
P50                          -3,552     same-binary, so no ~5,700 floor applies
rank-80                     -14,208     the gate metric itself
ranks 41-120 band           -15,572     the band, agreeing with rank-80
```

All five come from **one zero-repeat-floor instrument** — same ROM, same window, one
poked `.data` bit — so none of the spread is measurement noise; it is all real structure.
The percentile readings are 2.2–2.4× the paired median because **the saving clusters on
expensive frames**: a heavy frame plays more Q AObj nodes, so it takes more of the 370.6
entries and pays more of the fetch. That is `[[mean-self-time-predicts-p50-not-p95]]`
running in the favourable direction, and it is the same reason the marginal-80 median
(−6,432) exceeds the whole-run median (−3,840).

**−3,840 is banked** because it is the reading no interpretation can inflate. The
percentile-side numbers are reported, not banked, and the level in §5 is what the shipping
configuration actually reads.

Against the 21,719 tk/fr size in §2, the measurement recovers 18% (paired median) to 65%
(rank-80). The gap is not explained here and should not be: that 21,719 was measured in a
different configuration (§2's flagged premise), and this cycle bought a measurement of the
shipped one instead of an explanation of an inherited number.

---

## 5. The shipping build, and why its cross-build delta is not a price

`build-c219-animitcm-ship` is the same source with `NDS_R2_ANIM_ITCM_ROUTE` at its default
0: one copy, in ITCM, no route word, `nm` finds no `gNdsR2AnimItcmRoute` at all.

| | rank-80 raw | net | level | ranks 41–120 | P50 |
|---|---:|---:|---:|---:|---:|
| `c217-tilesync-ship` (basis) | 1,218,752 | 1,193,805 | +73,425 | 1,222,048 | 934,944 |
| **`c219-animitcm-ship`** | **1,216,896** | **1,191,949** | **+71,569** | 1,229,122 | 933,696 |
| cross-build Δ | −1,856 | | | **+7,074** | −1,248 |

**Do not read anything into −1,856.** The cross-build rank-80 floor is ≥14,080 and the band
moved the *other way*; the P50 −1,248 is inside its ~5,700 floor. This pair is a level, not
a price, and the route in §4 is why the cycle has a price at all.

The floor is visible directly in this cycle's own data: the route build's **arm 0** —
kernel in `.main`, i.e. the pre-move behaviour — reads rank-80 1,231,616 against the c217
basis's 1,218,752. **+12,864 for a build that changed nothing about the kernel's
placement.** That is the floor, measured, on the same day and window.

**The two builds agree where they should.** Route arm 1 (kernel in ITCM) reads rank-80
1,217,408; the ship build reads 1,216,896 — **512 apart**, two independently linked
binaries carrying the kernel at the same kind of address.

### 5.1 The shipped kernel against the basis kernel

253 of 257 words are byte-identical. The four that differ:

```text
slot 169   c217  cmp   ip, lr        c219  cmp   lr, ip
slot 170   c217  ldrle r3, [r0,#20]  c219  ldrge r3, [r0,#20]
slot 252   c217  .word 0x0223fa48    c219  .word 0x0223f928
slot 253   c217  .word 0x0223fa4c    c219  .word 0x0223f92c
```

Slots 252–253 are the literal pool holding `&gNdsR2CubicSaturations` and
`&gNdsR2CubicEvals`, which moved with `.bss`. Slots 169–170 are the **Step** arm's
`(inv <= len)`: `ip <= lr` and `lr >= ip` are the same predicate for every signed input
including equality, and GCC picked the commuted form after the `always_inline`
restructure. **Behaviour is identical; the encoding is not bit-identical and this document
does not claim it is.**

### 5.2 Boundary is a second, independent equality control

Boundary builds its own `smash64ds-battle-playable-proof-hwtri`. Its realtime pacing smoke
over the same 212 frames, against the last three Boundary runs on this tree:

| | `boundary-c206` / `-c209b` / `-c217` | this cycle | Δ |
|---|---|---|---|
| `binds` / `vtx` / `tri` | `54 / 2484 / 828` | `54 / 2484 / 828` | **identical** |
| `ftrTri` | `132712/p067840/p164872/own424` | `132712/p067840/p164872/own424` | **identical** |
| `frames` / `fps` / `rprof` | `212 / 241/480 / 0` | `212 / 241/480 / 0` | identical |
| `ticks` | 294,363,520 (c217) | **294,353,408** | **−10,112 = −47.7 tk/fr** |

**Same geometry, same binds, same triangles, measurably less time.** The counters that
would move if the relocated kernel computed a different pose are the ones that did not
move at all. (This is the proof ROM's boot/Pupupu scene, not the gate window: a direction
check, not a second price.)

---

## 6. Item B — ranked second, and blocked twice

Ranked as the brief asked, by `icache_fill` tk/fr per byte on the marginal-80:

| candidate | bytes | `icache_fill` tk/fr | **tk/fr per byte** | outcome |
|---|---:|---:|---:|---|
| `ndsR2AnimValueQ` | 1,028 | 21,719 | **21.13** | shipped, §4 |
| `glBindTexture` | 92 | 1,952 | 21.22 | libnds; needs the `--rename-section` route `BASIS.md` §6 found traps live neighbours |
| `ndsRendererHardwareBindTextureName` | 268 | 3,802 | **14.19** | **not built** |

Two independent blockers, both quantified:

1. **It does not fit.** After Item A the tick-HUD instrument has **220 B** free (32,516 of
   the linker's 32,736 B region). The candidate wants 268. Freeing it means taking more of
   §1's remaining 1,718 cold bytes — the two raw-run emitters are 240 B and are the
   cheapest honest next slice.
2. **A build could not decide it anyway.** Its port-reachable ceiling is 3,802 tk/fr, which
   is inside the ≥14,080 cross-build rank-80 floor — a floor this cycle re-measured at
   +12,864 on its own arm-0-vs-basis pair. It needs a **same-binary route** of the kind §3.1
   builds, not simply its own build. It is `static` in `nds_renderer.c`, so the same
   two-copy pattern applies directly.

Its 3,802 also carries §2's configuration caveat and is more exposed to it than Item A's:
it is a renderer figure from a `GX_COMPOSE=1` census against a `GX_COMPOSE=0` ship.

---

## 7. Bytes

```text
                        .itcm      .main
c217-tilesync-ship     32,224    931,256
c219-animitcm-ship     32,516    930,960
                         +292       -296

  ndsR2AnimValueQ                    -> .itcm   +1,028
  _arm_cmpsf2.o + _arm_unordsf2.o    -> .main     -332
  ndsRendererHardwareGetLightShadeLut-> .main     -404
```

**ITCM headroom is per target and the two figures are far apart. State which one.**

```text
region 0x7fe0 = 32,736 B (linker/nds_hot_text.ld:18, ITCM minus the vectors)

smash64ds-battle-playable-tickhud-hwtri   (the measurement instrument)
   build-c217-tilesync-ship    32,224      512 free
   build-c219-animitcm-ship    32,516      220 free    <- Item B needs 268

smash64ds-battle-playable-proof-hwtri     (what Boundary builds and grades)
   this cycle                  30,164    2,572 free
```

`check-task9-float-itcm.ps1` prints `free=` against the gross 32,768 rather than the
linker's 32,736 region, so its line reads 252 and 2,604 for the same two builds. The
region figure is the one a placement decision must use.

---

## 8. The checker was asserting the wrong invariant, and it still fails when it should

`check-task9-float-itcm.ps1` went red on this change:

```text
Task 9 Phase 2 ELF omitted its selected-libgcc fcmpeq golden.
```

The invariant it exists to prove is that Phase 2's `--redefine-sym` **fired** — that the
stock helper was renamed out of the call graph so the port's replacement is the one
linked. It was asserting that *plus* a section, and the section was only ever `.itcm`
because the whole member happened to be placed there. Five assertions carried the same
hardcoded `.itcm`, plus three private-copy paths named `<stem>.itcm.o` directly.

It now reads each member's placement from **the build's own emitted object name** — an
artifact independent of the ELF under test — and maps input `.text` to the linked output
section `.main`. **It was not loosened:**

| case | result |
|---|---|
| c219 ELF + c219 build dir (new layout) | **passes**, `itcm=32516/32768 free=252` |
| c217 ELF + c217 build dir (old layout) | **passes**, `itcm=32224/32768 free=544` |
| c219 ELF + c217 build dir (declared ITCM, golden in `.main`) | **throws** — `…omitted its selected-libgcc fcmpeq golden from ITCM.` |

The byte-for-byte equivalence proof it performs is what makes the eviction *placement
only*, and it reports the same hashes on both layouts:
`_arm_cmpsf2.o=276/2B656E12…`, `_arm_unordsf2.o=56/7B4D5CFB…` — identical in c217 and
c219. Same code, different address.

**And the new guard caught something the old one could not.** A build directory that
previously held `_arm_cmpsf2.itcm.o` keeps that file when the member moves to
`_arm_cmpsf2.mainram.o`, and the two together look like one member claiming two
placements. Boundary failed closed on exactly that; the Makefile recipe now removes both
spellings for every member. A guard written only to let this change through would have
shipped a stale object in every pre-existing build directory.

---

## 9. What this cycle did NOT do

- **Item B was not built.** §6 gives its ceiling (3,802 tk/fr port-reachable), its cost
  (268 B against 220 free), and the reason a plain build cannot price it.
- **The remaining 1,718 cold ITCM bytes were not taken.** §1 lists them; the exception
  vectors, cache-maintenance and thread paths in that list are deliberately left alone.
- **The 320 never-fetched bytes inside the kernel were not split out.** §2 explains why
  that would buy ITCM space and no fetch.
- **The 21,719 was not re-measured in the shipped configuration.** §2 flags the premise;
  §4 measures the shipped configuration's outcome instead.
- **No draw reordering.** The 2,484 tk/fr bind-revisit ceiling still stands unclaimed.
- **No fidelity decision was taken or asked for.** Placement cannot change behaviour, and
  §4.1 / §5.2 are the two controls that say it did not.
- **No pixel capture** beyond Boundary's own, no ROM published, both root ROMs
  byte-unchanged: `smash64ds.nds` `54c07fac…`, `smash64ds-battle-playable-hwtri.nds`
  `6c939434…` (the bore-84 link, which must not be published), before and after.
- **`build-c205-camtoggle` was not rebuilt.**

---

## 10. Reproduction

```powershell
make TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-c218-animitcm `
    NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1 `
    NDS_R2_ANIM_ITCM_ROUTE=1
foreach ($arm in 0,1) {
  pwsh -File scripts\sample-tick-hud-buckets.ps1 -Build build-c218-animitcm -NoBuild `
      -RingDump -Samples 1600 -StartFrame 438 -TimeoutSeconds 3600 `
      -SetGlobals "gNdsR2AnimItcmRoute=$arm" -ExtraGlobals gNdsR2CubicEvals `
      -RowsCsv ...\a$arm-rows.csv -JsonOut ...\a$arm.json
}
make TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-c219-animitcm-ship `
    NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1
```

`analyze.py a0 a1` is §4; `analyze.py c217 ship` is §5 (copy
`../2026-08-16_tilesync-memo/ship-rows.csv` in as `c217-rows.csv` first). §1 and §2 need
**no build at all** — `itcm-census.txt` is `nm` over `builds/build-c200-trackprof-off`'s
ELF joined to `../2026-08-15_ftanim-dispatch-attribution/c200-off-pc.csv`, both already on
disk.
