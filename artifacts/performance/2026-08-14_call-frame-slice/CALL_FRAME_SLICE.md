# Call-frame slice — STOP. No bankable package exists under the 64.9K ceiling.

**Date:** 2026-08-14
**Gate:** ≥32,000 profile cycles/frame = ≥16,000 ticks/frame, realistically
removable, proven before writing production code.
**Result:** the largest correctness-safe package that survives inspection is
**10,544 cycles/frame = 5,272 ticks/frame — 3.0x short of the gate.**
**No production code was written. No ROM was built.**

**UNITS: 2 profile cycles = 1 project tick.** Every figure below carries both.

---

## Phase 0 — three prior errors corrected

### 0.1 The classifier failed open

`scripts/census-dcache-working-set.py` mapped a load whose base register the
backward walk could not resolve to **`cacheable`** — the one class a layout change
is allowed to act on, and the class whose size decides whether that lane is worth
opening. Every unresolvable site was therefore inflating the case for opening it.
Now `unknown`, counted in neither direction.

### 0.2 Profile cycles were compared against tick floors

`artifacts/performance/2026-08-14_dcache-working-set/CENSUS.md` compared
cycle figures directly against the campaign's tick floors, overstating every
candidate by exactly 2x. The DMA busy-wait was the visible casualty:

| | as published | corrected |
|---|---|---|
| `ndsRendererTask36ReplayRun` DMA0CNT poll | "16,629 … at the bankable bar" | 16,629 cyc/frame = **8,315 ticks/frame** |
| verdict | the only single site at the bar | **under the ±8,544 placement floor** — a perfect deletion would not be measurable alone |

Best layout candidate likewise falls from "~3,000" to **~1,500 ticks**. Both
documents now carry an explicit units header.

### 0.3 `ftGetStruct` did not become frameless

This is the correction that reshapes the lane. Fitting **every** frame instruction
in the build with ≥2,000 executions against its register count:

| form | measured cost |
|---|---|
| `push` of N registers | ≈ **1.6 + 1.2·N** cycles (N=1 → 2.86, N=6 → 9.66) |
| `pop`+pc of N registers | ≈ **5.0 + 1.6·N** cycles (N=1 → 6.45, N=6 → 16.75) |

A function returning through `pop {…, pc}` keeps **~9.3 cycles a call** however
few registers it saves — the `pc` load's pipeline flush is the floor, not the
register list. So:

> **A cold-tail split recovers only `(N_old − N_new) × ~2.8` cycles per call.
> The whole frame is recoverable only if the function becomes genuinely frameless,
> or if the call disappears entirely.**

`ftGetStruct` corrected: `push{r3-r7,lr}` 9.66 + `pop{r3-r7,pc}` 16.75 = 26.4
cyc/call → `push{r4,lr}` 4.05 + `pop{r4,pc}` 8.42 = 12.5. Its measured 7,141
cyc/frame falls to ≈3,371, so the saving is **≈3,770 cyc/frame ≈1,885 ticks —
not the 7,141 cycles previously credited.** The prior cycle's two cuts total
**≈6,270 cyc/frame ≈3,135 ticks**, not 9,600/4,800.

---

## Phase 1 — top-50 classification

Instrument: `scripts/census-frame-candidates.py` (new; models SPLIT / LEAF /
DELETE per function off the fitted cost model above, no build).

Classes: **A** removable cold-tail · **B** removable wrapper · **C** one-call-site
inline · **D** diagnostic-only · **E** frame required by hot body · **F** compiler
artifact, unsafe/unprofitable · **G** instrumentation-only · **H** already
optimized/refuted.

| function | frame cyc/fr | total cyc/fr | calls/fr | push→ | SPLIT cyc | class | why |
|---|---:|---:|---:|---|---:|:--:|---|
| `ftGetStruct` | 7,141 | 18,336 | 246.3 | 6→2 | 3,448 | **A** | done last cycle; hot route still framed |
| `ndsR2AnimValueQ` | 6,919 | 38,574 | 271.2 | 9 | 6,074 | **H** | see §Blocked — attributes are measured |
| `ndsR2FtAnimParseDObjFigatree` | 2,470 | 22,604 | 68.1 | 5 | 763 | E | body runs on the taken path |
| `ndsRendererAdapterMaterialAnimHash` | 2,269 | 6,622 | 28.6 | 2 | **80** | **H** | live memo key, already 2-reg and hand-tuned to 2 cache lines |
| `gcRunGObjProcess` | 2,262 | 10,000 | 54.4 | 5 | 609 | A | dispatcher, cold arms available |
| `ftDisplayMainDrawDefault` | 2,214 | 18,443 | 49.6 | 5 | 556 | E | draws every call |
| `memcpy` | 2,144 | 24,979 | 103.2 | 5 | 1,156 | **F** | libgcc; not ours to reshape |
| `gcPlayDObjAnimJoint` | 2,090 | 39,260 | 69.8 | 5 | 782 | E | evaluator body is the work |
| `ndsRendererNativeStageBeginRun.part.0` | 2,049 | 27,664 | 54.0 | 9 | 1,209 | E | emits GX every call |
| `gcParseMObjMatAnimJoint` | 2,019 | 7,489 | 66.7 | 5 | 747 | **A** | whole body under `anim_wait != NULL` |
| `ndsRendererNativeStageEmitNoZVertex.isra.0` | 1,964 | 10,929 | 80.9 | 7 | 1,360 | E | vertex emit |
| `memset` | 1,849 | 29,257 | 81.9 | 3 | 459 | **F** | libgcc |
| `ndsRendererAdapterBuildDObjXObjMatrix` | 1,771 | 20,561 | 53.9 | 5 | 604 | E | builds every call |
| `ndsRendererMtxMulAffine20p12` | 1,736 | 35,182 | 50.4 | 9 | 1,129 | E | 20.12 kernel, slice 42 closed |
| `ndsRendererNativePrepareProductionRun` | 1,724 | 34,276 | 64.0 | 9 | 1,433 | **H** | per-run AOT refuted 2026-08-14 |
| `ndsFighterDisplayContractCountFlags` | 1,720 | **7,849** | 54.1 | 6 | 758 | **D** | **only confirmed deletable entry** |
| `ndsR2AnimAObjToQ` | 1,695 | 2,869 | 90.3 | 4 | 759 | **A** | done last cycle (call now gone) |
| `ndsRendererHardwareBindTextureName` | 1,679 | 11,994 | 102.2 | 3 | 572 | E | binds every call |
| `ndsBaseGcPlayMObjMatAnim` | 1,674 | 9,004 | 66.7 | 5 | 747 | E | 5 live tracks; board forbids blanket convert |
| `cpuGetTiming` | 1,637 | 6,971 | 171.6 | 2 | 480 | **G** | the tick-HUD instrument itself |
| `gcParseDObjAnimJoint` | 1,607 | 5,777 | 59.8 | 5 | 670 | **A** | whole body under `anim_wait != NULL` |
| `ndsBaseGcPlayDObjAnimJoint` | 1,409 | 5,201 | 59.8 | 5 | 669 | **A** | whole body under `anim_wait != NULL` |
| `ndsRendererNativeApplyStateDelta` | 1,400 | 13,558 | 189.7 | 3 | 1,062 | E | applies deltas |
| `ndsRendererNativeEmitProductionPrimitiveGroups` | 1,336 | 50,052 | 51.9 | 9 | 1,162 | E | the emit loop |
| `glBindTexture` | 1,332 | 10,149 | 55.6 | 4 | 467 | F | libnds |
| `__aeabi_lmul` | 1,319 | 4,811 | 42.4 | 5 | 474 | F | libgcc |
| `tickGetCount` | 1,316 | 16,475 | 171.6 | 2 | 480 | **G** | instrument |
| `ndsRendererAdapterBuildDObjLocalMatrix` | 1,315 | 9,399 | 53.9 | 5 | 604 | **H** | local-matrix memo "dead twice" |
| `ndsRendererHardwareApplyTextureParams` | 1,170 | 5,624 | 58.0 | 3 | 325 | E | |
| `ndsRendererNativeShadeProductionActions` | 1,112 | 13,032 | 47.2 | 6 | 660 | E | |
| rows 31–50 (`…BuildFighterTraRotRpyDirect`, `…StageEmitNoZTriangle`, `ndsRendererTask36ReplayRun`, `…R2MaterialColor15`, `gcCaptureCameraGObj`, `ndsMPFindLineEndpoints`, `ndsStageGCDrawAllLoopRecordCapturedDisplay`, `…MarkDisplayProcHeads`, `…CommitNativeStageDisplay`, `…LoadHardwareMatrixPair`, `…StageLoadNoZMatrix`, `sqrtf`, `battleship_ftMainProcUpdateInterrupt`, `…BuildPersistentStageWorld`, `glGetTexParameter`, `ndsMPLineExtentSweepRejects`, `glTexParameter`, `ftDisplayMainDecideFogDraw`, `ndsRendererCommitNativeStageSegment`) | 14,470 | — | — | — | 5,585 | E/F/H | each does real work on the taken path or is library code |

Modelled column totals across all 50, **before** class filtering:

```
SPLIT  (cold tail out, 1-reg hot frame)   37,018 cyc/frame =  18,509 ticks/frame
LEAF   (frameless)                        68,011 cyc/frame =  34,006 ticks/frame
DELETE (call and body gone)              792,974 cyc/frame = 396,487 ticks/frame
```

**Those three numbers are the trap this document exists to defuse.** They assume
every function has a movable cold tail, can be made frameless, or can be deleted.
Applying the classes above, almost none can.

---

## Phase 2 — the package, and why it does not reach the gate

### Confirmed removable

| item | cyc/frame | ticks/frame | evidence |
|---|---:|---:|---|
| `ndsFighterDisplayContractCountFlags` — whole call | 7,849 | 3,925 | `gNdsFighterDisplayContract{Hidden,NoTexture}Count` have **no runtime reader**: written at `reloc_backend_renderer_dl.c:13495/13499`, reset at `taskman_seam.c:3147`, declared in `diagnostics.c`/`nds_startup.h`, and read only by `probe-ko-vfx.ps1` and `verify-battle-mariofox-gcrunall-loop-harness.ps1` |
| `gcParseDObjAnimJoint` split | 670 | 335 | body entirely under `anim_wait != AOBJ_ANIM_NULL`, asserted by `check_anim_null_guard.py` |
| `gcParseMObjMatAnimJoint` split | 747 | 374 | same guard |
| `ndsBaseGcPlayDObjAnimJoint` split | 669 | 335 | same guard |
| `gcRunGObjProcess` split | 609 | 305 | dispatcher with cold arms |
| **TOTAL** | **10,544** | **5,272** | |

**Gate is 32,000 cycles / 16,000 ticks. The package reaches 33% of it.**

### Blocked, with the reason

**`ndsR2AnimValueQ` — 6,919 cyc/frame, the single largest frame entry — must not
be touched.** Its `noinline, target("arm")` is not a stale attribute; it is a
measured result recorded at `battleship_sys_objanim.c:311-323`:

- Q16 basis needs 64-bit squares for t² and t³. ARMv5TE **Thumb has no SMULL**, so
  GCC emitted `bl __aeabi_lmul` at eleven sites in `gcPlayDObjAnimJoint`, eight on
  the executed path. The Thumb arm measured **SRC P50 +17,728 / WORK-H P50
  +25,472**.
- `noinline` keeps six inlined float→fixed conversions to **one** copy rather than
  one per call site, because this lands in `.text.hot`'s curated 8 KiB.

Its 9-register push is ARM-mode allocation for the SMULL/SMLAL chains — i.e. **the
frame cost is the price of a change that already paid −25,472 WORK-H P50.**
Reducing the saved set means changing the arithmetic. Not in scope, and the sign
is known.

### Candidates that looked deletable and are not

A name-shaped sweep found 202 diagnostic-sounding functions worth 98,871
cyc/frame (49,436 ticks) — and **all but one are load-bearing.** Each was checked
against the *published* ELF (`smash64ds-battle-playable-hwtri.elf`, 2026-08-14
12:58), not against source:

| candidate | cyc/frame | verdict |
|---|---:|---|
| `ndsStageGCDrawAllLoopRecordCapturedDisplay` | 16,914 | **E.** Its counters are diagnostic, but it also sets `sNdsStageGCDrawAllLoopCurrentCamera/Display/LinkID`, calls `MarkDisplayProcHeads`, `RecordWeaponCapture`, `RecordEffectCapture`, and **dispatches `ndsRendererAdapterCommitNativeStageDisplay`** — it is the stage render hook |
| `tickGetCount` | 16,475 | **G.** The instrument. 171.6 calls/frame is tick-HUD sampling; not shipped cost and cannot be sized from this profile |
| `ndsRendererAdapterMaterialAnimHash` | 6,622 | **H.** A live FNV memo key over `MObj` material fields, already hand-packed to two cache lines / nine multiply-accumulates |
| `ndsIFCommonRecordHUDState` (+`RecordStockState`, `RecordDamageState`) | 4,871 | **E.** `nds_platform.c:2616,2639,2648,2665,2725,2747,2760` reads `gNdsIFCommonHUD*` to drive the on-screen damage/timer/stock HUD. Deleting it blanks the HUD |
| `ndsRendererRecordSetTile` / `RecordOtherMode` / `RecordTextureState` | 5,734 | **E.** These write `NDSRendererStats` — the N64 RDP state machine the native renderer reads. The name misleads; this is replay, not instrumentation |
| `ndsRendererHardwareRecordBattleStaticTextureHit` | 4,367 | **E.** Feeds the static-texture cache |
| `syTaskmanCheckBufferLengths` | 3,129 | **E.** Display-list overflow guard. `decomp` asserts here; the port records instead because "a devkit assert becomes a dead handheld" |

One methodological note worth keeping: `codegraph_explore` returned the
`#else` (non-hwtri) bodies of `ndsRendererAdapterCommitNativeStageDisplay` and
`ndsRendererAdapterMarkDisplayProcHeads`, which are `return FALSE;` and `{}`.
`nm` on the shipped ELF shows 180 and 108 bytes. **HEAD carries two definitions of
each under `#if NDS_RENDERER_HW_TRIANGLES`, and the shipped ROM compiles the real
one.** A source read alone would have booked ~14,800 cycles of "calling nothing"
that does not exist.

---

## Verdict

The task's own stop condition applies:

> *If no correctness-safe package reaches that after reviewing the top 50: STOP.
> Record that call-frame optimization has no bankable package despite its 64.9K
> theoretical ceiling. Do not land another 3K–8K tick collection.*

**10,544 cycles / 5,272 ticks is a 3K–8K tick collection. Nothing was
implemented.** The shipped ROM is untouched.

### Why the 64,863-tick ceiling does not convert

Three independent reasons, each measured here rather than assumed:

1. **The `pop {…, pc}` flush is a floor.** ~9.3 cycles a call survive any split.
   Only framelessness or call deletion recovers the whole frame, and neither is
   available to a function that does real work on its taken path.
2. **The frame is spread the way the D-cache census found the loads spread.**
   1,169 functions, largest single entry 6,919 cycles — and that one is protected
   by a −25,472 measurement.
3. **The diagnostic reservoir is nearly empty.** 98,871 cycles of
   diagnostic-*shaped* names contain 7,849 cycles of actual diagnostic. This repo
   has already graduated its proof modes, exactly as `AGENTS.md` requires.

### What remains worth doing, and at what size

`ndsFighterDisplayContractCountFlags` is a real 3,925-ticks/frame deletion and
should be taken **when something else is being built anyway** — never on its own,
since it cannot clear the ±8,544 placement floor. When it is taken:

- gate the traversal, not the globals;
- keep `gNdsFighterDisplayContract{Hidden,NoTexture}Count`
  `__attribute__((used))` — `--gc-sections` dropping a diagnostic global is what
  turned Boundary RED on 2026-08-11;
- keep the harness configuration computing them, because
  `verify-battle-mariofox-gcrunall-loop-harness.ps1` prints both.

## Reproduce

```bash
arm-none-eabi-objdump -d builds/build-c125-profile/smash64ds-battle-playable-tickhud-hwtri.elf > c125.dis
python scripts/census-frame-candidates.py \
  artifacts/performance/2026-08-12_c125-slice48/profile/arm9-profile.csv \
  --dis c125.dis --regions 1601 --top 50
```
