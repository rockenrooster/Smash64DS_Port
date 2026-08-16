# The bank was measured with `GX_COMPOSE` ON while the ROM ships it OFF — and re-measuring at the shipping default retires the −17,152

**Date:** 2026-08-16 · **Branch:** `codex/r2-runtime2` · **HEAD `b1339828070`**
2 lab builds, 2 whole-match gate runs, 0 source changes.
**UNITS: 2 profile cycles = 1 project tick.** Every table states its window.
**No shipping default was changed. No ROM was published.**

---

## 0. Outcome first

```text
BASIS     THE +85,393 BANK WAS MEASURED WITH GX_COMPOSE ON.  The shipping
          default is OFF.  Proven twice from the build itself, not inferred:
            builds/build-c199-bank0/nds_build_config.h  -> GX_COMPOSE 1
            nm on its linked ELF                        -> 8 gNdsR2GxCompose*
                                                           symbols present
          Those symbols exist only inside `#if NDS_R2_FIGHTER_GX_COMPOSE`, and
          they are absent from every GX=0 ELF.  Makefile:766 is `?= 0` and the
          published block (Makefile:1545) pins 0 UNCONDITIONALLY, so no
          published ROM can carry the slice.  Every bank since c185 is GX=1.

LEVEL     THE SHIPPING LEVEL, MEASURED FRESH AT THIS HEAD:
            build-c206-shipgx0 (GX=0) rank-80 1,239,808 raw / 1,214,861 net
            EXACT NET GAP  +94,481      (banked figure was +85,393)
          The banked configuration reproduces on the same HEAD:
            build-c207-gx1    (GX=1) rank-80 1,232,768 raw / 1,207,821 net
            = 2,048 from c199-bank0's 1,205,773, inside the floor.

PRICE     THE −17,152 IS NOT REPRODUCED AND SHOULD NOT BE SPENT.
          Same-HEAD compile-time pair, ONE config line apart:
            rank-80  A−B = +7,040  (GX on cheaper)  vs a >=14,080 floor
            P50      A−B = −4,288  (GX on DEARER)   vs a ~5,700 floor
          Both are inside their floors and they DISAGREE IN SIGN.  What is
          reproducible is not a saving but a TRANSFER:
            FTR  P50  305,152 (off) -> 296,704 (on)   −8,448
            STG  P50  175,680 (off) -> 182,272 (on)   +6,592
          and the second, independent GX=1 build (c199-bank0, a different
          HEAD) lands within 704 of c207 on STG, so the transfer is a property
          of the flag, not this build's placement.

WHAT IT   The gap the board ranks levers against should be quoted on the ROM's
MEANS     own basis: +94,481.  The 9,088 between the two configurations is
          INSIDE the cross-build floor, so the bank is not PROVABLY optimistic
          — but it is measured on a renderer the user does not run, and that is
          a basis defect whichever way the number falls.

NOT DONE  NO DEFAULT WAS FLIPPED.  BLOCKED(decision: GX_COMPOSE default) is in
          section 5, and it is now a basis-consistency decision, not a −17,152
          performance decision.
```

---

## 1. The load-bearing question, answered from the build

`plan.md` §12f states the c199 bank was taken with "generated header verified
`GX_COMPOSE 1`". That is correct, and it is confirmed here from two independent
modalities rather than re-read from the same document:

| modality | `build-c199-bank0` | `build-c198-bore0` (GX=0 control) |
|---|---|---|
| the build's own `nds_build_config.h` | `#define NDS_R2_FIGHTER_GX_COMPOSE 1` | `... 0` |
| `nm` on the linked ELF, `gNdsR2GxCompose*` | **8 symbols** | **0 symbols** |

Those globals are defined inside `#if NDS_R2_FIGHTER_GX_COMPOSE`
(`src/nds/nds_renderer.c:19307`), so their presence is a property of the linked
binary and cannot be produced by a stale or shared header.

**When the basis changed.** Sweeping every build directory's own config header:
everything up to and including `build-c184-gxc-a` is GX=0 — *including the
`c170` bank*. From `build-c185-gxcompose-bank` onward every banked level is
GX=1: c185, c193-segfix, c197-bank, **c199-bank0**, c200-bank84, c201-camfix,
c202-camfix2, c203-camitcm. The switch happened at c185, on the strength of the
owner's pixel acceptance recorded in `GXCOMPOSE.md` §9, and the default was
never flipped to match.

**The escape it travels on.** `Makefile:1777` pins the tick-HUD/proof block to
`$(if $(filter 1,$(NDS_R2_FIGHTER_GX_COMPOSE_LAB)),1,0)`. The comment four lines
above it states the instrument "has to stay flag-identical to the published
block". On this flag it has not been since c185.

---

## 2. The arms

Two fresh builds at `b1339828070`, target
`smash64ds-battle-playable-tickhud-hwtri`, flags
`NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1`,
bore 0, DLDI on. B adds `NDS_R2_FIGHTER_GX_COMPOSE_LAB=1` and nothing else.

```text
diff builds/build-c206-shipgx0/nds_build_config.h builds/build-c207-gx1/nds_build_config.h
102c102
< #define NDS_R2_FIGHTER_GX_COMPOSE 0
---
> #define NDS_R2_FIGHTER_GX_COMPOSE 1
```

**One line. That is the whole difference between the arms**, which is the
cleanest compile-time pair this campaign has had on this flag.

| | A `build-c206-shipgx0` | B `build-c207-gx1` |
|---|---|---|
| `NDS_R2_FIGHTER_GX_COMPOSE` | **0 — the shipping renderer** | 1 |
| ROM SHA256 | `ae1eecedd333ea778e46ffa8223597306d7c8a289e8cca96f88d3dcbe148480b` | `2582c7a699150e0b13eeb71ae85f719ed541a79e9d6e44cae033c0ae9adbdb71` |
| `.main` | 931,088 | 933,128 |
| `.itcm` | **32,152 (616 B free)** | 32,188 (580 B free) |

> The camera cycle's "580 bytes free" is the **GX=1** figure. On the shipping
> configuration ITCM has **616 B** free. Neither reaches the 916 B that arm
> overflowed by — see §6.

---

## 3. The measurement

`sample-tick-hud-buckets.ps1 -RingDump -Samples 1600 -StartFrame 438`,
frames 439..2038, `slips=0` on both, 0 `Exception:` in either log.
Series **`WORK-H`**; **rank-80 recomputed from each run's own 1,600 rows**
(this convention reproduces `c199-bank0`'s published 1,230,720 / 1,205,773 /
+85,393 exactly); net = raw − 24,947 apparatus; gate 1,120,380.

| arm | GX | rank-80 raw | net | **gap** | P50 | over-gate |
|---|:--:|---:|---:|---:|---:|---:|
| **A `build-c206-shipgx0`** | **0** | 1,239,808 | 1,214,861 | **+94,481** | 944,448 | 175/1600 |
| B `build-c207-gx1` | 1 | 1,232,768 | 1,207,821 | +87,441 | 948,736 | 184/1600 |
| `build-c199-bank0` (the bank) | 1 | 1,230,720 | 1,205,773 | +85,393 | 942,848 | 170/1600 |

**The same fight, three times.** All seventeen end-of-match counters are
identical across A, B and c199-bank0 — P1Damage 76, sparks 16, shield 480,
AObj32 774, packHits 257, runaway 0, Task36 outcome 2, segment mask 161, all
four `FtAnimParse` counters, arena 1,548,288, heap free-min 53,136, resident
287,904, AllocFail 0, LoadFails 0. No tick delta here is a gameplay delta.

**Engagement, from the run that produced B's ticks:** `Declines=0`,
`Captures = Roots = 62,920`, `Locals = Mults = 109,346`. GX compose is fully
engaged on B and its symbols do not exist on A, which is a stronger negative
control than a zero.

### 3.1 The price, and why it is not −17,152

```text
same-HEAD pair, one config line apart          A (GX off) − B (GX on)
  rank-80                                        +7,040      floor >=14,080
  P50                                            −4,288      floor  ~5,700
  paired per-frame median, 1,600 frames          −4,544      B worse on 1,370
```

Both whole-frame statistics sit inside their cross-build floors
(`VERIFYING.md`, cycle-100 calibration) **and they disagree in sign**. The
`−17,152 at rank-80` in `GXCOMPOSE.md` was taken on `build-c184-gxc-a/-b` at a
level of 1,189,312 → 1,172,160, i.e. **before the segment-phase repair
`64c41c361a7`** which moved the whole level to 1,230,720. It is a pre-repair
figure in exactly the category `MENU.md`'s 94,602 turned out to be, and this
cycle's same-HEAD pair does not reproduce it.

### 3.2 What IS reproducible: a transfer, not a saving

| bucket | A GX=0 P50 | B GX=1 P50 | `c199-bank0` GX=1 P50 | A→GX-on |
|---|---:|---:|---:|---:|
| **FTR** | 305,152 | 296,704 | 293,376 | **−8,448 / −11,776** |
| **STG** | 175,680 | 182,272 | 182,976 | **+6,592 / +7,296** |
| WORK-H | 944,448 | 948,736 | 942,848 | +4,288 / −1,600 |

The two GX=1 arms are **separate builds at different HEADs** and they agree with
each other to **704 on STG** and **3,328 on FTR**, while both differ from GX=0 by
~6,600–7,300 and ~8,400–11,800 respectively. A placement artefact would not
reproduce across two independent links; **the FTR→STG transfer is a property of
the flag**. Paired per-frame it is near-total: FTR improves on **1,599/1,600**
frames, STG regresses on **1,599/1,600**.

**Not diagnosed here.** Whether the stage genuinely pays more matrix work when
the fighter path leaves the hardware in GX-composed state, or whether the ticks
merely cross a tick-HUD bucket boundary, is one counter's worth of work and was
not taken. Stated as an observation, not a mechanism.

---

## 4. What this does to the campaign's central number

- The shipping configuration reads **+94,481**. The banked configuration reads
  **+85,393**. The difference, **9,088, is inside the >=14,080 cross-build
  floor**, so it is *not* proof that the bank is optimistic.
- It is, however, proof that **the bank is not on the ROM's basis**. Every "x of
  the gap" ratio in `LADDER.md`, `MENU.md` and `CAMERA_Q20_12.md` divides by a
  denominator measured on a renderer the user does not run.
- The cheapest repair needs no decision at all: **quote +94,481 and cite
  `build-c206-shipgx0`.** Ratios move by 85,393/94,481 = **0.904x** — the camera
  chain's 5.5% becomes 5.0%, the ladder's rungs all shrink by ~10%, and no
  conclusion in either document changes sign.

---

## 5. BLOCKED(decision: `GX_COMPOSE` default) — the package

**The decision is no longer a performance decision.** It is whether the shipped
renderer should equal the measured renderer, and if so which one moves.

**What it costs in pixels** (`GXCOMPOSE.md` §8, `build-c184-cap-a/-cap-b`,
`-ExactTimeRemain` simulation-clock locks, battle screen, 120,000 px):

| lock | differing pixels | max channel delta | same-build adjacent floor |
|---|---:|---:|---:|
| 48 | 43 (**0.0358%**) | 251 | — |
| 50 | 83 (**0.0692%**) | 251 | A 47.1117% / B 47.1133% |
| 1692 | 209 (**0.1742%**) | 251 | — |
| 1694 | 148 (**0.1233%**) | 251 | A 67.4892% / B 67.4842% |

Images: `artifacts/visibility/2026-08-15_gxcompose-bank/`, including
`gxcompose-cross-t1694-battle-diff.png` and `gxcompose-cross-t1692-battle-diff.png`.
**The criterion these were judged against was `PREDICTION.md` §5's
pre-registered *identity* — "any pixel difference" — not a numeric budget.**
The owner reviewed those masks and accepted the delta (`GXCOMPOSE.md` §9).

**What it costs in correctness:** nothing measured. `gNdsHardwareRendererStatus`
= `0x06000000` on 128/128 samples on both arms; accepted polygon RAM
432/463.5/510 with 0/128 under 350 (the blink signature was a 106..306 collapse
and is absent); seventeen invariants identical here as well.

**What it buys in ticks:** on the current tree, **nothing resolvable** — §3.1.
The −17,152 is retired.

**The options, priced, with no recommendation:**

1. **Flip the default to 1.** The ROM gains the accepted 0.0358–0.1742% pixel
   delta and no measurable tick change; the bank becomes correct as it stands.
2. **Leave the default at 0 and re-bank at GX=0.** Costs nothing, needs no
   owner decision, and this cycle has already produced the number: **+94,481**.
3. **Leave both as they are.** Every ratio in the ranking documents keeps a
   denominator from a renderer the user does not run.

**This is the owner's call and it is not made here.**

---

## 6. Falsifier — the ITCM golden reclaim, closed at 632 B and mostly untakeable

**The literal-pool modality has no discriminating power here and that is the
answer to the open step.** Scanning every LOAD section of the linked ELF for
4-byte words equal to a float helper's address returns **0 for every target —
including `__aeabi_fadd`, `__mulsf3` and `__divsf3`, which are entered 1,728 /
2,122 / 431 times.** The controls cannot fail, so a zero for the goldens proves
nothing by this route. Soft-float helpers are reached only by direct `bl`.

The modality that does work is branch reachability, counting only edges whose
**source is outside** the target range:

| ITCM range | bytes | external entries |
|---|---:|---:|
| `__aeabi_frsub` / `__subsf3` / `__addsf3` (= task16 fadd/fsub goldens) | 456 | **0** (entered only from `__aeabi_frsub`, itself 0) |
| task9 `fcmpeq` + task16 `fcmplt`/`fcmple`/`fcmpge`/`fcmpgt` goldens | 120 | **0** |
| `__unordsf2` / task16 `fcmpun` golden | 56 | **0** |
| *controls* `__mulsf3` / `__aeabi_fadd` / `__divsf3` / `__aeabi_f2iz` | — | 2,122 / 1,728 / 431 / 324 |

**632 B of ITCM is unreachable code, and the lead's "~600 B" was right.**
Independently confirmed by the v3 capture already on disk
(`../2026-08-15_ftanim-dispatch-attribution/c200-off-pc.csv`, 1,601 regions,
54,959,909 sampled PCs): **zero instructions at any PC in
`0x01ff802c..0x01ff81e8` across the whole match**, against `__aeabi_fadd`'s
89,131,406.

**What these bytes actually are — read from the link map, not assumed.** The
routines that serve the real calls are **repo-authored**, not a second libgcc
copy:

```text
.itcm.task16_float_compare  0x01fff1a0  0xec  nds_task16_float_compare.o
.itcm.task16_float_addsub   0x01fff2e8 0x194  nds_task16_float_addsub.o
                                              -> __aeabi_fsub, __aeabi_fadd
```

Task 16 replaced libgcc's soft-float add/sub and compares with its own ARM
implementations; `--redefine-sym` renames libgcc's originals to
`__nds_task*_libgcc_*_golden` and `-Wl,--undefined=` (`Makefile:2477-2484`)
keeps those originals in the image **as reference bodies for the comparison lab
`builds/build-task16-compare-arm9-lab/`**. So the 632 B is retained-on-purpose
reference code.

**Setting `NDS_TASK16_FLOAT_*` to 0 is therefore NOT a reclaim — it reverts the
repo's own float implementation to stock libgcc.** Nobody should do it for
space.

**And the reference bodies mostly cannot be dropped either**, because the
extraction renames each libgcc member's `.text` whole (`Makefile:3767`,
`--rename-section .text=.itcm`), so `--gc-sections` cannot split them:

```text
.itcm  0x01ff8020  0x2ac  _arm_addsubsf3.itcm.o   dead 456 B, but also defines the
                                                  LIVE __aeabi_ui2f (99 entries),
                                                  __aeabi_l2f (5), __aeabi_ul2f (4)
.itcm  0x01ff85c4  0x114  _arm_cmpsf2.itcm.o      dead 120 B, but also defines the
                                                  live __gesf2/__lesf2/__cmpsf2
.itcm  0x01ff86d8   0x38  _arm_unordsf2.itcm.o    WHOLLY dead, 56 B
```

- **56 B is cleanly droppable** — `_arm_unordsf2.itcm.o` has no live symbol, and
  the repo's own `__aeabi_fcmpun` at `0x01fff270` also has **0 external
  entries**, so unordered compare is not used anywhere. It survives only because
  `NDS_TASK16_FLOAT_COMPARE=1` force-links `__nds_task16_libgcc_fcmpun_golden`.
  Dropping just that `--undefined` without disabling the `--redefine-sym` needs
  the flag split in two — small, but it is a Makefile change, not a flip.
- **The other 576 B needs the extraction to emit per-function sections.**

**Consequence for the camera v3 arm:** 616 B free (shipping configuration) + 56 B
= **672 B against a 916 B overflow**. **The ITCM route stays closed** without
that tooling change.

---

## 7. Falsifier — the float-add ladder is flat, and 444 B is the wrong copy

`__addsf3` at `0x01ff802c` (444 B) is libgcc's original, proven above to be
**never entered and never executed**. The ladder that actually runs is the
repo's own **`__aeabi_fadd` from `nds_task16_float_addsub.o` at `0x01fff2ec`,
400 B / 100 instructions**. Everything below is that one.

From `c200-off-pc.csv` (`build-c200-trackprof-off`, v3, 1,601 regions,
marginal-80 mask at `total_cycles − halt_wait >= 1,224,970`):

```text
whole match          33,106 tk/fr        marginal-80        74,380 tk/fr
entry-PC call count  2,890,928 whole = 1,805.7/frame ; 312,312 marginal = 3,903.9/frame
cost per call        18.33 tk whole     19.05 tk marginal
stall composition (marginal-80, % of the function's own cycles)
   issue        96.22%      icache_fill 3.78%      everything else 0.00%
```

**PC histogram — 81 distinct PCs, top of the distribution:**

| pc | % of the function | pc | % |
|---|---:|---|---:|
| `0x01fff324` | **4.63%** | `0x01fff3a0` | 3.08% |
| `0x01fff388` | 3.71% | `0x01fff310`–`0x320` (5 PCs, prologue) | 2.62% each |
| `0x01fff3c8` | 3.34% | `0x01fff398` | 2.46% |
| `0x01fff378` | 3.20% | `0x01fff430` | 1.72% |
| `0x01fff380` | 3.20% | `0x01fff348` | 1.65% |

Top PC **4.63%**, and the distribution is flat from there down across 81 PCs.
Per `[[a-flat-function-only-lever-is-not-entering-it]]` **there is no
instruction to delete; the only lever is the call count** — 1,805.7/frame whole
match, 3,903.9/frame on the frames that set the percentile. It is 96% issue
stall: this is a long dependent integer chain, not a memory problem, so it will
not respond to placement or to ITCM residency either (it is already in ITCM).

**Not priced as instructions × calls, and the residual is not divided by the
count**: 18.33 tk/call comes from the entry PC's own instruction count against
the function's own cycles, both measured on the same capture.

---

## 8. What this cycle did NOT do

- **No default was flipped and no ROM was published.** Root ROMs
  `54c07fac…` (`smash64ds.nds`) and `6c939434…`
  (`smash64ds-battle-playable-hwtri.nds`) are byte-unchanged;
  `root-roms-before.txt` / `root-roms-after.txt` in this directory.
- **The FTR→STG transfer was not diagnosed.** No counter was added.
- **No pixel capture was taken this cycle.** §5 re-quotes `GXCOMPOSE.md` §8.
- **No `NDS_TASK16_FLOAT_*` flag was changed**, and none should be until the
  body-swap in §6 is measured.
- **`LADDER.md` / `MENU.md` were not re-divided by +94,481.** One line was added
  to the board instead; re-quoting a dozen ratios is its own task.

## 9. Reproduction

```powershell
make TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-c206-shipgx0 `
    NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1
make TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-c207-gx1 `
    NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1 `
    NDS_R2_FIGHTER_GX_COMPOSE_LAB=1
# pwsh 7 only; through cmd use cmd's own `> log 2>&1`
pwsh -File scripts\sample-tick-hud-buckets.ps1 -Build build-c206-shipgx0 -NoBuild `
    -RingDump -Samples 1600 -StartFrame 438 -TimeoutSeconds 3600 -ExtraGlobals <17 names> `
    -RowsCsv ...\c206-shipgx0-rows.csv -Json ...\c206-shipgx0.json
```
