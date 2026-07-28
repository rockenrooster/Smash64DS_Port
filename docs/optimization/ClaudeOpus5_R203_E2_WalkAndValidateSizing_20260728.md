# R2-03 E2 — sizing the fighter walk and revalidation

**Date:** 2026-07-28
**Phase:** R2-03 sizing, second candidate from
`ClaudeOpus5_R203_E0_FrameRebaseline_20260728.md` §4. No runtime change.
**Instrument:** `NDS_TASK91_DRAW_PHASE_CENSUS` (existing, lab, default `0`),
built on the post-R2-02 configuration.
**Result: 13,670 ticks/frame, three quarters of it in the revalidation.** Real,
above the noise floor, and smaller than the same function's unattributed
remainder.

---

## 1. The numbers

`ndsFighterMarioFoxDLAllDrawForSlot` walks the generic DObj tree, revalidates
every collected display list against the loaded asset and walks each MObj chain,
and only then hands over to the native owner. The E0 census puts the whole
function at **37,206 ticks/frame at 5.55 cycles per instruction** — the highest
stall ratio of any large function in the frame, which is what pointer-chasing
looks like.

Two independent windows, agreeing to 0.5%:

| | walk | validate | draw calls |
|---|---:|---:|---:|
| 2-frame delta, frames 439–441 | 3,296/frame | 10,432/frame | 2.0/frame |
| cumulative to frame 1,828 | 3,289/frame | 10,381/frame | 1.94/frame |

Per call: walk **1,644**, validate **5,190**. Both are well clear of the
~1,000-tick figure below which the standing rules say to re-derive an E-series
span a second way — and they were re-derived a second way regardless.

`gNdsTask91NativeEligible` is **3,493 of 3,553 calls (98.3%)**. The revalidation
passes essentially always; the 1.7% is the frames before both fighters exist.
Consistent with Task 70's finding that native-owner fallback is 0.44% of the
P95.

## 2. What that means for the cut

**Available: ~13,300.** The revalidation is the three-quarters worth having.
Its shape is E1a's: it proves per frame something whose inputs are the loaded
asset bases and the DObj tree shape, and those change at match load, not at 60
Hz. A generation stamp over those inputs turns 10,381 ticks/frame into a
comparison, and the same stamp lets the collected DObj list be cached, taking
the 3,289-tick walk with it.

Before writing that, run the E3 falsifier pattern on it: hash the validation's
inputs per frame and count the frames they change on. If the answer is zero, the
stamp is sound and the fail-closed behaviour is preserved by invalidating on the
stamp rather than by re-proving. If it is not zero, the stamp needs to be the
thing that actually moves.

**Do not start there.** The same function has **~24,000 ticks/frame that
nothing has attributed** — 37,206 total against 13,670 of walk and validate, and
the walk lives in `ndsFighterCollectAllDObjsWithDL`, a separate symbol, so the
remainder is `DLAllDrawForSlot`'s own body plus whatever inlined into it. That
is the larger half and it is unmeasured. The E3 lesson was that the aggregate
hid the answer and the per-part split found it; the same split has not been done
here.

Ranked, after this measurement:

1. **Split `DLAllDrawForSlot`'s remaining ~24,000.** One more counter pair in an
   instrument that already exists. Cheapest information left in the fighter.
2. The revalidation stamp, ~10,400.
3. The cached collection, ~3,300 (do it with 2 — same stamp).
4. The adapter matrix rebuild, 56,879 (E0 §4).

## 3. Instrument defect found on the way

`scripts/census-fighter-draw-phases.ps1` accepts `-WindowFrames` up to 600 and
**silently measures two frames whatever you pass it.** Asked for 128, it
reported "over 2 presented frames (frames 439 .. 441)". The cause is already a
standing rule — *GDB `if` at top level resumes exactly once* (Task 96) — so the
second stop fires at the next breakpoint hit rather than at
`StartFrame + WindowFrames`.

The numbers it printed are not wrong; they are a correct 2-frame delta. But a
2-frame window is a thin sample and the script does not say so. Until it is
fixed, size these counters the way this task did: they accumulate from boot, so
one `sample-tick-hud-buckets.ps1 -ExtraGlobals` stop at a late frame divided by
`gNdsTask91DrawCalls` gives a per-call figure over the whole match, with no
window logic to get wrong.

## 4. Evidence

| SHA-256 (first 16) | file |
|---|---|
| `8602FD6BDCDF8B10` | `artifacts/performance/r2-03-e2-walk-validate-1700.json` |

ROM `builds/build-r2-03-e2-phases`, `NDS_R2_STAGE_DIRECT=1 NDS_R2_STAGE_DMA=1
NDS_R2_STAGE_ACTORS=1 NDS_TASK91_DRAW_PHASE_CENSUS=1`.
