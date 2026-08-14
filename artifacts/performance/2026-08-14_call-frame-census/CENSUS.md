# The call frame is the largest unexamined lane

**Date:** 2026-08-14
**Instrument:** `scripts/census-visit-counts.py`, `scripts/census-call-sites.py`
**Profile:** `artifacts/performance/2026-08-12_c125-slice48/profile/arm9-profile.csv`
(1,601 regions, whole match, tick-HUD instrumented)

## The question

The 2026-08-14 D-cache census closed the layout lane with the conclusion that
"locality work has no remaining single-structure lever… anything further has to
change how much data is *visited* — node counts, call counts, visit rates."
`HANDOFF.md` says the same for animation: "STRUCTURAL LAYOUT cuts closed
2026-08-13; **call count is the lever**."

This census asks the call-count question directly, and finds that the largest
single answer is not any subsystem's work. It is the cost of *entering* the
functions that do the work.

## The finding

Attributing every `push`/`pop`/`stm`/`ldm` with a register list, plus every
`sp` adjustment, to its enclosing function:

```
TOTAL prologue/epilogue cost = 129,727 cyc/frame (64,863 ticks/frame)
                               across 1,169 functions
```

**5.9% of the whole attributed frame** (2,202,260 cyc/frame) is spent saving and
restoring registers across call boundaries. No prior slice has measured it as a
class. The board found it once, for one function — of `ndsR2AnimValueQ` it says
its "`push {r4-r9,sl,fp,lr}` costs 2,529 cyc/frame … and its `pop` 4,154 —
**6,683 cyc/frame, 16% of the evaluator, to save and restore registers**" — and
then treated it as a property of that function rather than of the program.

It is a property of the program. The twenty-two worst:

| frame cyc/fr | fn cyc/fr | % of fn | calls/fr | cyc/call | function |
|---:|---:|---:|---:|---:|---|
| 7,141 | 18,336 | **38.9** | 246.3 | 74 | `ftGetStruct` |
| 6,919 | 38,574 | 17.9 | 271.2 | 142 | `ndsR2AnimValueQ` |
| 2,470 | 22,604 | 10.9 | 68.1 | 332 | `ndsR2FtAnimParseDObjFigatree` |
| 2,269 | 6,622 | **34.3** | 28.6 | 232 | `ndsRendererAdapterMaterialAnimHash` |
| 2,262 | 10,000 | 22.6 | 54.4 | 184 | `gcRunGObjProcess` |
| 2,214 | 18,443 | 12.0 | 49.6 | 372 | `ftDisplayMainDrawDefault` |
| 2,144 | 24,979 | 8.6 | 103.2 | 242 | `memcpy` |
| 2,090 | 39,260 | 5.3 | 69.8 | 562 | `gcPlayDObjAnimJoint` |
| 2,049 | 27,664 | 7.4 | 54.0 | 513 | `ndsRendererNativeStageBeginRun.part.0` |
| 2,019 | 7,489 | **27.0** | 66.7 | 112 | `gcParseMObjMatAnimJoint` |
| 1,964 | 10,929 | 18.0 | 80.9 | 135 | `ndsRendererNativeStageEmitNoZVertex.isra.0` |
| 1,849 | 29,257 | 6.3 | 81.9 | 357 | `memset` |
| 1,771 | 20,561 | 8.6 | 53.9 | 381 | `ndsRendererAdapterBuildDObjXObjMatrix` |
| 1,736 | 35,182 | 4.9 | 50.4 | 698 | `ndsRendererMtxMulAffine20p12` |
| 1,724 | 34,276 | 5.0 | 64.0 | 536 | `ndsRendererNativePrepareProductionRun` |
| 1,720 | 7,849 | 21.9 | 54.1 | 145 | `ndsFighterDisplayContractCountFlags` |
| 1,695 | 2,869 | **59.1** | 90.3 | 32 | `ndsR2AnimAObjToQ` |
| 1,679 | 11,994 | 14.0 | 102.2 | 117 | `ndsRendererHardwareBindTextureName` |
| 1,674 | 9,004 | 18.6 | 66.7 | 135 | `ndsBaseGcPlayMObjMatAnim` |
| 1,637 | 6,971 | 23.5 | 171.6 | 41 | `cpuGetTiming` |
| 1,607 | 5,777 | **27.8** | 59.8 | 97 | `gcParseDObjAnimJoint` |
| 1,409 | 5,201 | **27.1** | 59.8 | 87 | `ndsBaseGcPlayDObjAnimJoint` |

## Why it is this expensive here, and why the compiler cannot fix it

The functions with the worst *ratio* share one shape: **a hot early-out guarding
a cold body**, where the body's register needs set the frame that the early-out
pays.

- `ftGetStruct` returns a pointer in three loads on every one of its 246.3 calls
  a frame; the `bzero`-and-populate stub builder after those returns is what
  needs `r3`–`r7`. The whole-match profile shows the stub path executing **zero
  times** and the frame costing **7,141 cyc/frame**.
- `ndsR2AnimAObjToQ` is one byte compare in the steady state — the board already
  records that "once migrated it early-outs in ~5 instructions" — and costs 32
  cycles a call, **19 of them frame**.
- `gcParseDObjAnimJoint`, `gcParseMObjMatAnimJoint` and `ndsBaseGcPlayDObjAnimJoint`
  each wrap their entire body in `anim_wait != AOBJ_ANIM_NULL`
  (`scripts/check_anim_null_guard.py` asserts this), and each pays ~27% frame to
  discover the guard is false.

GCC's `-fshrink-wrap` is on at `-O2` and is exactly the transform that would sink
these prologues past the early return. It cannot do it here: **ARMv5TE Thumb-1
has no conditional execution**, so an early exit cannot be predicated and the
prologue cannot be duplicated cheaply onto the cold path only. The build is
`-mthumb` (`Makefile:2139`) apart from three objects given `-marm` for SMULL
reasons.

So the fix is structural, per-function, and free: move the cold tail into a
`noinline` function of its own. The code is the same code, one branch further
away, on a path the profile never executed.

## Implemented this cycle

Two, chosen because their equivalence is provable by inspection and neither adds
a byte of hot code:

1. **`ftGetStruct`** (`src/port/reloc_backend_compat_shims.c`) — stub builder
   split to `ftGetStructBuildStub`. The three fast returns are unchanged and in
   the same order, so every caller receives the same pointer on every path.
   Predicted **−7,141 cyc/frame (−3,570 ticks)**.
2. **`ndsR2AnimAObjToQ`** (`src/import/battleship_ftanim.c`) — the
   already-converted test is now `static inline` at its three call sites, and the
   conversion is `ndsR2AnimAObjToQConvert`, `noinline`. Predicted
   **−2,500 cyc/frame (−1,250 ticks)**.

Combined prediction **≈ −9,600 cyc/frame ≈ −4,800 ticks/frame**, which is
**below** the ±8,544 cross-build placement floor. These two are banked for
correctness and static evidence (the `push`/`pop` pairs leave the hot path in the
disassembly); they are not, on their own, a measurable A/B.

## Ranked next targets, and one that is not a codegen problem at all

The class holds 64,863 ticks/frame and the top twenty-two hold roughly 25,000
ticks of it. The same split applied to `ndsRendererAdapterMaterialAnimHash`
(34.3%), `gcRunGObjProcess` (22.6%), `gcParseDObjAnimJoint` (27.8%),
`gcParseMObjMatAnimJoint` (27.0%) and `ndsBaseGcPlayDObjAnimJoint` (27.1%) is
worth roughly a further 9,600 cyc/frame and should be taken as **one slice**, so
that a single A/B measures the class rather than any one member.

`ndsR2AnimValueQ` (6,919 cyc/frame, the single largest absolute entry) is the
board's own open item and it says what is required: inlining into its **single**
call site. It is `noinline` + `target("arm")` deliberately; changing that needs
the reason for the attribute checked first, not assumed stale.

**`ndsFighterDisplayContractCountFlags` is a different finding and belongs to
Task 5's brief, not to this class.** It is a recursive walk of the whole fighter
DObj tree, twice per presented frame, costing **7,849 cyc/frame (3,925 ticks)**,
and it computes nothing the renderer reads: `gNdsFighterDisplayContractHiddenCount`
and `…NoTextureCount` are reset in `taskman_seam.c:3147` and read only by
`scripts/probe-ko-vfx.ps1` and
`scripts/verify-battle-mariofox-gcrunall-loop-harness.ps1`. That is a whole
per-frame tree traversal of pure instrumentation in the shipped battle ROM.
Deleting it from the shipped configuration is worth more than either cut above —
but the globals must stay `__attribute__((used))` and the harness build must keep
computing them, because `--gc-sections` dropping a diagnostic global is exactly
what turned Boundary RED on 2026-08-11.

## What this does not claim

The profile is tick-HUD instrumented, so placement differs from the shipped ELF
and `cpuGetTiming`'s own 1,637 cyc/frame of frame cost is the instrument, not the
game. The class total is a *ceiling* on what perfect frame elimination would pay;
a leaf that genuinely needs its registers keeps them.

## Reproduce

```bash
arm-none-eabi-objdump -d builds/build-c125-profile/smash64ds-battle-playable-tickhud-hwtri.elf > c125.dis
python scripts/census-call-frames.py \
  artifacts/performance/2026-08-12_c125-slice48/profile/arm9-profile.csv \
  --dis c125.dis --regions 1601
```
