# The gap is +82,065. Two bit-identical codegen changes shipped, measured on a same-binary route that reproduces byte for byte, and they cost −216 bytes.

**Date:** 2026-08-16 · **Branch:** `codex/r2-runtime2` · **base HEAD `efe31fd0abe`**
2 lab builds (`build-c214-hwroute`, `build-c215-hwmath-ship`), 7 whole-match gate runs,
0 gameplay values changed, no default flipped that costs fidelity, no ROM published,
both root ROMs byte-unchanged. Boundary green.
**UNITS: 1 project tick = 1 `cpuGetTiming()` tick = 2 ARM9 cycles.** Every table states
its window.

```text
REQUIREMENT  +94,481 net ticks per presented frame at rank-80.  Basis:
             build-c206-shipgx0, rank-80 1,239,808 raw / 1,214,861 net against
             the 1,120,380 gate; SHIPPING renderer (GX_COMPOSE 0), bore 0, mode
             163 one-minute match, 1,600 samples, frames 439-2038, slips=0
             (../2026-08-16_gap-position/POSITION.md section 1).

BANKED       BOTH ITEMS SHIPPED, BOTH BIT-IDENTICAL BY CONSTRUCTION AND GRADED.

               A  nds_r2_sqrtf.o built -marm, so nds_r2_sqrtf.h:73's 48-bit
                  root*root is one UMULL instead of `bl __aeabi_lmul`
               B  the leading DIVCNT/SQRTCNT busy poll deleted from every site
                  that executes: 16 leading polls before, 2 after, and both
                  survivors are behind NDS_R2_CAMERA_FIXED=0 and execute zero
                  times

             SAME-BINARY ROUTE, four arms, one ROM (sha BEBC5801...):
               rank-80  arm0 1,241,536  ->  arm3 1,229,824   -11,712
               ranks 41-120 band mean                        -14,125
               paired median, whole run                       -5,184  (79.9% win)
             THE SHIPPING BUILD, cross-build against the basis on the same
             window: rank-80 1,239,808 -> 1,227,392, -12,416, band -12,144.
             The two readings agree to 704 ticks.  Sections 3 and 4.

NEW BASIS    build-c215-hwmath-ship, rank-80 1,227,392 raw / 1,202,445 net.
             GAP +94,481 -> +82,065.  Section 4.

LADDER       banked this cycle          12,416   0.131x  (route-only: 0.124x)
             POSITION.md inventory      53,215   0.563x  (unchanged, no overlap)
                                        ------
                                        65,631   0.695x  -> +28,850 unaccounted
             HWMATH.md's 4,300-4,750 ESTIMATE for item A is superseded by
             measurement; the ladder read 0.611x and now reads 0.695x.
             Section 8.

BYTES        NEGATIVE.  .main 931,088 -> 930,872, -216 B.  -marm costs +24 B and
             the deleted polls give back more than that.  Section 5.

METHOD       THE INSTRUMENT IS DETERMINISTIC.  Two repeat runs reproduced their
             1,600-row CSVs BYTE FOR BYTE (identical sha256).  The same-arm
             repeat floor on this instrument is EXACTLY ZERO, so every tick of
             difference between arms is the change.  Section 3.1.

ITEM C       SIZED, NOT BUILT, AND THE FOOTPRINT SAYS WHY.  The C2 chain already
             spends 44.6% of its own cost on instruction fetch: 4,680 B holding
             16,891 tk/fr of icache_fill at 22-93 entries a frame.  THE
             MARGINAL PRICE OF A BYTE IN THAT CHAIN IS 3.61 tk/fr.  Against a
             20,357 tk/fr prize the entire budget is +5,640 B, and both prior
             fixed-point rewrites in this tree exceeded half of it.  Section 7.
```

---

## 1. What changed, exactly

| | |
|---|---|
| `Makefile` | `nds_r2_sqrtf.o: CFLAGS += -marm`; new lab flag `NDS_R2_HWMATH_ROUTE ?= 0` |
| `include/nds/nds_r2_hwmath_unit.h` | `ndsR2HwMathDiv64` / `ndsR2HwMathSqrt64` — libnds's `div64`/`sqrt64` to the register, minus the leading poll — plus the route hook |
| `src/nds/nds_renderer.c` | 8 `div64` + 2 `sqrt64` call sites moved onto them |
| `src/nds/r2/nds_r2_sqrtf.c` | same move for `sqrtf`'s root; body split out so the route can select it |
| `src/nds/r2/nds_r2_sqrtf_arm.c` | **new, lab only** — the ARM-state arm of the route, not in `CFILES` at flag 0 |
| `scripts/check-gbi-decode-fixtures.ps1` | the projected-divider row now accepts either helper name; the invariant it guards (exact signed pre-clamp before the hardware divide) is unchanged |

**libnds is not touched.** `div64`/`sqrt64` are `static inline` in `nds/arm9/math.h`, so
their leading poll was compiled into *our* objects. This is a port-side change end to end.

**`src/import/battleship_gmcamera.c` is deliberately not touched.** Its
`ndsR2CamDiv64`/`ndsR2CamSqrt64` carry the same poll, but `NDS_R2_CAMERA_FIXED` is `0` in
every shipping basis, so they execute **zero times** — and editing them would only stale
the `−4,736 tk/fr` the owner's pending draw-side-precision decision is priced on.

---

## 2. The leading poll, priced before a byte was written

`[[entry-pc-gives-exact-call-counts]]`. The per-PC execution profile already on disk
(`../2026-08-15_ftanim-dispatch-attribution/c200-off-pc.csv`, `build-c200-trackprof-off`,
`regions=1601`, `marginal_frames=80`, `cycles_per_tick=2`) gives the poll loops' **exact
execution counts and cycles**, so "41.0 tk × an unknown rate" never had to be guessed.
Full table in `poll-census.txt`.

| site | leading-poll cycles (80 frames) | **tk/fr** |
|---|---:|---:|
| `sqrtf` | 295,320 | **1,845.8** |
| `ndsRendererR2WriteLightVector` (1 root + 3 divides) | 46,191 | 288.7 |
| `ndsRendererHardwareSubmitVertex` (2 divides) | 32,016 | 200.1 |
| `ndsRendererSubmitNativeImpactWave` (2 divides) | 25,088 | 156.8 |
| **total** | **398,615** | **2,491.3** |

Two independent cross-checks on the microbenchmark that motivated the item:

- `sqrtf`'s leading poll runs **3.00 iterations per call** and costs **46.0 cycles = 23.0
  tk/call**, against `HWMATH.md` §3.1's bench figure of **20.0 tk** for the root's leading
  poll.
- `ndsRendererHardwareSubmitVertex`'s runs **6.00 iterations** and costs **87.0 cycles =
  43.5 tk/call**, against the bench's **41.0 tk** for the divide's.

**`div64`, the outlined libnds copy, executes zero times in this match** — `all_instructions`
is 0 at its entry PC — so the "libnds is hot" branch of the brief's question never arises.
Every executing poll was inlined into our own functions.

**The call rate.** `sqrtf`'s entry PC reads **6,421 calls over the 80 marginal frames =
80.26 calls/frame**, and **73,191 over all 1,601 = 45.72 calls/frame**: the lane is
**1.76× denser on the population that sets rank-80**, which is why the measured win below
concentrates in the tail.

---

## 3. The route, and why the measurement is decisive

Both changes are codegen, and both are far under the **≥14,080 rank-80 cross-build floor**,
so a two-build A/B cannot decide either. `build-c214-hwroute` therefore holds **both arms of
both items in one binary**, selected by one `.data` word:

```text
gNdsR2HwMathRoute   bit 0  sqrtf body from the -marm object (nds_r2_sqrtf_arm.o)
                    bit 1  leading DIVCNT/SQRTCNT poll skipped
arm 0 = exactly what shipped before this cycle
arm 3 = exactly what ships after it
```

| | |
|---|---|
| target | `smash64ds-battle-playable-tickhud-hwtri` |
| config | `NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1 NDS_R2_HWMATH_ROUTE=1`, GX_COMPOSE 0, bore 0, DLDI on |
| window | 1,600 samples, frames 439–2038, `-RingDump`, `slips=0`, `cadenceViolations=0` on every arm |
| provenance | `romSha256` **`BEBC58017E1133D4…` identical on all four arms**; `readback == requested` and `stuck: true` on each |
| level check | arm 0's rank-80 is **1,241,536** against `build-c206-shipgx0`'s **1,239,808** — **1,728 apart**, far inside the cross-build floor, so the instrument sits at the banked shipping level |
| raw | `r0.json`…`r3.json`, `r{0,3}b.json`, `*-rows.csv`, `route-arms.txt` |

**The route word is in `.data`, not `.bss`** — `nm` reads `020eb148 D gNdsR2HwMathRoute` —
because a zero-initialised route word without an explicit section attribute drags a
~10,000 tk/fr placement floor.

**Disassembled before measuring**, because kernels have silently compiled to Thumb here
twice: `ndsR2SqrtfArmBody` contains `umull` and no `__aeabi_lmul`; `ndsR2SqrtfBody`
contains `bl __aeabi_lmul` and no `umull`; `sqrtf` is a Thumb dispatcher that pays exactly
one call on **both** arms, so the delta is the instruction selection and not the call.

### 3.1 The same-arm repeat floor on this instrument is exactly zero

`r0b` and `r3b` re-ran arms 0 and 3 from scratch. **Both reproduced their 1,600-row CSVs
byte for byte** — `sha256(r0-rows.csv) == sha256(r0b-rows.csv)` and likewise for r3 — down
to the VBlank-interval histogram and the `named` total.

This is a first-class methodological result and it should be used: **on a same-binary
route with a fixed input script, this instrument has no session noise at all.** Every tick
of difference between arms below is the change. It also means a repeat run of an
unchanged arm buys nothing and should not be spent.

---

## 4. The result

`route-arms.txt` carries every table. WORK-H, same window on every row.

| build / arm | rank-80 raw | net | gap | P50 | ranks 41–120 |
|---|---:|---:|---:|---:|---:|
| `c206-shipgx0` (basis, BEFORE) | 1,239,808 | 1,214,861 | **+94,481** | 944,480 | 1,240,849 |
| c214 arm 0 (route control) | 1,241,536 | 1,216,589 | +96,209 | 958,240 | 1,251,813 |
| c214 arm 1 (A only) | 1,240,064 | 1,215,117 | +94,737 | 953,056 | 1,245,010 |
| c214 arm 2 (B only) | 1,244,608 | 1,219,661 | +99,281 | 957,120 | 1,250,242 |
| c214 arm 3 (A+B) | 1,229,824 | 1,204,877 | +84,497 | 952,160 | 1,237,688 |
| **`c215-hwmath-ship` (AFTER, no route)** | **1,227,392** | **1,202,445** | **+82,065** | 944,864 | 1,228,705 |

**Same-binary, arm 0 → arm 3:**

| statistic | Δ | what it is |
|---|---:|---|
| rank-80 | **−11,712** | the quantity the requirement is stated in |
| ranks 41–120 band mean | **−14,125** | reorder-robust tail, where the gate lives |
| ranks 81–240 band mean | −6,439 | just below the gate |
| paired median, whole run | **−5,184** | the median frame; **79.9%** of 1,581 paired frames improve |
| paired median, marginal-80 | −11,584 | the population that sets rank-80 |

**Read the spread honestly.** The change removes about **5,200 tk/fr from the median frame
and about 13,000–14,000 from the gate population** — a 2.5× concentration, because the
sqrtf lane is 1.76× denser there and the divide sites more so. rank-80 itself sits on a
steep local slope (rank 60 −26,880, rank 80 −11,712, rank 100 −5,824), so **−11,712 is one
order statistic, not a plateau**; the 41–120 band mean of −14,125 is the robust tail figure
and it is larger, not smaller.

**Two independent readings agree.** The route says −11,712 at rank-80; the actual shipping
build says **−12,416** against the basis on the same window, with the 41–120 band at
−12,144. The shipping build reads slightly *better* than arm 3 because it does not carry
the route instrument (§5), so **−11,712 is a lower bound on what ships.**

### 4.1 The split, and the control that proves the arms are doing work

| | paired median (whole run) | share |
|---|---:|---:|
| A alone, arm 0→1 | −3,840 | 74% |
| B alone, arm 0→2 | −768 | 15% |
| B on top of A, arm 1→3 | −1,344 | |
| A on top of B, arm 2→3 | −4,416 | |
| **A+B, arm 0→3** | **−5,184** | |

Additive to within one sampling quantum: `−3,840 + −768 = −4,608` against `−5,184`.
Predictions land inside the measured bracket — item A was predicted 4,300–4,750 tk/fr at
marginal-80 and reads −3,840 whole-run / −8,960 marginal-80; item B was predicted 2,491 and
reads −768 / −4,512.

**The complement is the control.** On arm 3 the per-bucket paired medians are
`WORK-H −5,184` and `WAIT +5,312`, with `ALL` at `+0`. `ALL` is VBlank-quantised
(`[[all-is-a-quantized-gate]]`), so at a fixed presented cadence a genuine work deletion
must appear as work down and idle up **by the same amount** — and it does, to 128 ticks.
The deletion lands where the code is: `FTR −1,984`, `SRC −2,176` (all of it inside
`GCRA`), `MISC −832`, `SINT −960`, `STG −256`.

**Do not read arm 2's rank-80 as a regression.** It reads **+3,072** while its paired
median is −768 and its 41–120 band is −1,571. rank-80 is an order statistic on a steep
slope and the frames reorder between arms; with zero session noise that +3,072 is real and
it is still not evidence that removing the poll costs anything.

---

## 5. Bytes: the change is negative

```text
.main   build-c206-shipgx0     931,088 B      (before)
        build-c215-hwmath-ship 930,872 B      (after)      -216 B
        build-c214-hwroute     931,784 B      (+696 B of route instrument)

nds_r2_sqrtf.o text   220 B before (Thumb, with the leading poll)
                      220 B after  (ARM,    without it)
                      -marm costs +24 B; the deleted poll gives back 24 B
```

The campaign's standing failure mode — added bytes inverting a win — does not apply: this
change deletes work *and* text. The renderer's nine deleted poll loops are the rest.

**Structural proof that the polls are gone**, from the two linked ELFs rather than from
grep:

```text
build-c200-trackprof-off   LEADING polls 15 detected + sqrtf's (Thumb form) = 16
build-c215-hwmath-ship     LEADING polls  2, both ndsR2CamDiv64 / ndsR2CamSqrt64
                                            (NDS_R2_CAMERA_FIXED=0, zero entries)
```

---

## 6. Equivalence: nothing here is a judgement call

Both items are **bit-identical, not merely equivalent**, and both proofs already existed:

- **A.** `build-c213-hwmath4`'s `gNdsR2HwMathBenchSqrtfMismatch` is **0 over 65,536
  inputs**, comparing an ARM build of `include/nds/nds_r2_sqrtf.h` against the shipped
  Thumb `sqrtf` (`HWMATH.md` §6.3). The header itself is unchanged; only its object's
  instruction-set flag moved. `scripts/check-r2-fixed-sqrt.ps1` still grades the algorithm
  exhaustively on the host.
- **B.** GBATEK: writing `DIVCNT`/`DIV_NUMER`/`DIV_DENOM` restarts the division and only
  the last write matters; the same holds for the root. `DivMismatch`, `DivFastMismatch`,
  `SqrtMismatch`, `SqrtFastMismatch`, `QuotMismatch`, `QuotLeadMismatch`, `RemMismatch`
  and `RemLeadMismatch` are all **0 over 65,536 operands each on four builds**, with
  **32,914 negative denominators** and **223 rounding half-cases** as live controls
  (`HWMATH.md` §3.1). `decomp/sm64ds-decomp`'s `cstd::div` and `cstd::sqrt` ship without
  the leading poll.

No fidelity decision was taken or needed. Boundary is green, including
`check-gbi-decode-fixtures`, `check-r2-fixed-sqrt`, `check-decomp-pristine` and the
`battle_playable` realtime pacing smoke (`frames=212 ... rprof=0`).

---

## 7. Item C — sized, not built, and the footprint is the answer

`c2-footprint.txt`. The four C2 members, from the same ELF and the same per-PC profile:

| member | bytes | lines | entries/fr | issue | **icache_fill** | dcache_fill | total tk/fr |
|---|---:|---:|---:|---:|---:|---:|---:|
| `ndsR2FtAnimParseDObjFigatree` | 3,016 | 95 | 93.4 | 5,851 | 11,546 | 6,807 | 25,648 |
| `ndsBaseGcPlayDObjAnimJoint` | 500 | 16 | 62.1 | 626 | 1,371 | 1,104 | 3,318 |
| `ndsBaseGcPlayMObjMatAnim` | 732 | 23 | 85.3 | 871 | 2,401 | 1,818 | 5,416 |
| `gmCollisionTransformMatrixAll` | 432 | 14 | 22.5 | 576 | 1,574 | 1,089 | 3,472 |
| **total** | **4,680** | **147** | | **7,924** | **16,891** | **10,817** | **37,854** |

**The prize is exactly the 20,357 the brief quotes**, and this cycle confirmed what it is:
the **soft-float gross charged to these four callers**, 1,446.2 helper calls a frame
(`__aeabi_fmul` 44.8k, `__aeabi_fadd` 26.8k, `__aeabi_fsub` 21.3k, `__aeabi_fcmpeq` 21.5k
calls over the 80 marginal frames). It is disjoint from the 37,854 the bodies themselves
cost.

**The footprint question, answered with a number instead of an analogy:**

```text
instruction fetch is 44.6% of the C2 chain's own cost, TODAY, at 22-93 entries/frame
MARGINAL PRICE OF A BYTE IN THIS CHAIN = 3.61 tk/fr   (115 tk/fr per 32 B cache line)

  the whole 20,357 prize is spent by   +5,640 bytes
  half of it is spent by               +2,820 bytes
  the collision ring's 3,228 B would cost 11,650 tk/fr of fetch alone
```

**Verdict: C2 converts if and only if the rewrite is byte-neutral or byte-negative, and no
fixed-point rewrite in this tree has been.** The ring added 3,228 B; the camera inlining
added 3,032 B and inverted a −4,736 win into +1,600. Both exceed the half-prize budget
here. The entry rate is 23–96× C1's, which is what made C2 the candidate — but the
mechanism that killed C1 is *not* singleton entry, it is compulsory fetch, and this chain
already pays it on **every** entry: 3,016 B of `ndsR2FtAnimParseDObjFigatree` at 93.4
entries a frame still spends 45% of its cycles in `icache_fill`, i.e. its lines do not
survive between entries.

So a C2 cycle is a **byte-budgeted rewrite**, and its first deliverable is a byte ledger,
not an arithmetic argument. Two further facts it should carry:

- The chain's own **issue** cost is only **7,924 tk/fr of 37,854 (20.9%)**. Converting the
  bodies cannot touch the other 79%. The addressable ceiling is the 20,357 prize plus at
  most that 7,924 — about **24,000 tk/fr = 0.29× of the new +82,065 gap.**
- **`dcache_fill` (10,817) is larger than `issue` (7,924).** The chain is memory-bound on
  both sides, and a Q26 joint matrix is the same 32 bits per element as the `f32` it
  replaces, so a conversion does not shrink the data.

---

## 8. The ladder

```text
against +94,481 (build-c206-shipgx0)

  BANKED this cycle, measured and shipped      12,416   0.131x   (route-only 11,712 = 0.124x)
  POSITION.md fidelity-neutral inventory       53,215   0.563x   (no overlap; none of its
                                               ------             six items is sqrtf or a poll)
                                               65,631   0.695x  ->  +28,850 unaccounted

  superseded: HWMATH.md section 6.4's "+ sqrtf in ARM state ~4,500 = 0.048x, ladder 0.611x".
              The item is the same; the estimate is replaced by a measurement 2.8x larger,
              because it was priced on the whole-match sqrtf rate and the gate population
              carries 1.76x that rate, and because item B rode with it.

new basis for the next cycle
  build-c215-hwmath-ship   rank-80 1,227,392 raw / 1,202,445 net   GAP +82,065
  same target, same config, same window as the c206 basis it replaces.
```

---

## 9. What this cycle did NOT do

- **`battleship_gmcamera.c`'s two leading polls were not removed.** They are the only ones
  left in the binary and they execute **zero** times at `NDS_R2_CAMERA_FIXED=0`; removing
  them would change the arm the owner's pending draw-side-precision decision is priced on
  and buy nothing today.
- **`sqrtf`'s IME mask was not removed.** `nds_r2_hwmath_unit.h`'s ELF survey shows the
  binary has no interrupt-context user of either unit, so the reachability it guards
  against does not currently exist, and it is priced: **698 tk/fr** for its three I/O
  accesses, **1,294 tk/fr** if the register setup around them is charged too
  (`poll-census.txt`). Removing a safety property is an owner call.
  **BLOCKED(decision: sqrtf IME mask).**
- **C2 was not built.** §7 gives its byte budget and the condition under which it converts.
- **No new counter was written for `POSITION.md`'s two counter-gated items**
  (`ndsRendererSyncTextureTile`, texture-bind collapse). They remain 0.241× of *unsized*
  volume.
- **No fidelity decision was taken or asked for.** Both items are bit-identical and graded.
- **No pixel capture** beyond Boundary's own, no ROM published, both root ROMs
  byte-unchanged: `smash64ds.nds` `54c07fac…`, `smash64ds-battle-playable-hwtri.nds`
  `6c939434…` (the bore-84 link, which must not be published), before and after.
- **`build-c205-camtoggle` was not rebuilt.**

---

## 10. Reproduction

```powershell
# the route instrument, four arms, one binary
make TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-c214-hwroute `
    NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1 `
    NDS_R2_HWMATH_ROUTE=1
foreach ($arm in 0,1,2,3) {
  pwsh -File scripts\sample-tick-hud-buckets.ps1 -Build build-c214-hwroute -NoBuild `
      -RingDump -Samples 1600 -StartFrame 438 -TimeoutSeconds 3600 `
      -SetGlobals "gNdsR2HwMathRoute=$arm" `
      -RowsCsv ...\r$arm-rows.csv -JsonOut ...\r$arm.json
}

# the shipping configuration
make TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-c215-hwmath-ship `
    NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1
```

§2 and §7 need **no build at all** — they are exact arithmetic over
`../2026-08-15_ftanim-dispatch-attribution/c200-off-pc.csv` and two linked ELFs, both
already on disk.
