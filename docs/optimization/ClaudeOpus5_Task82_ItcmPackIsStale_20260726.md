# Task 82 E0 — The ITCM pack has gone stale: 5,040 idle bytes, 67,045 ticks in reach

**Date:** 2026-07-26
**Status:** E0 complete, **GO**. Sized by the census tooling that already exists.
No runtime change.
**Input:** `artifacts/task78-anim-census/census.txt`, sections B/C/D — 128 settled
frames (439–567).

## 1. Where this came from

Tasks 78–81 closed the animation and texture directions and left the ranked
families — soft-float 161,187, matrix 158,500, `mem*` 137,193 — unsized by call
count. Before sizing another family, the stall table at the top of the same
census answers a different and larger question.

**Total stall, excluding idle: 944,626 ticks/frame — 62.3% of the 1,515,768
ticks of work.** That independently reproduces Task 65's 62% figure from a
different instrument. Stall, not instruction count, is the frame.

And one row of it is directly actionable today.

## 2. The pack is stale

`.itcm` holds 32,596 bytes across 71 residents. **27 of them never execute in
the battle window, holding 5,040 idle bytes.**

That is not a design error, it is drift. Task 37 packed ITCM against a 2026-07-22
measurement. Tasks 44, 53, 54, 55, 56, 72 and 76 have changed what the frame
does since. Nothing re-ran the pack, so the residents are optimal for a frame the
ROM no longer executes.

The census script sizes the recovery itself:

```
zero-eviction pack: 136 B, 518,110 non-mem stall cycles in reach
with eviction:      5,178 B, 17,163,581 non-mem stall cycles in reach
```

**17,163,581 cycles is 67,045 ticks/frame** — 8× the ±8,000 placement variance,
and it is the largest single lever any task in this session has identified.

## 3. Ranked admissions (census section D)

| stall/byte | non-mem stall | bytes | current tier | symbol |
|---|---|---|---|---|
| 8,535 | 4,062,873 | 476 | `.text.hot.draw` | `ndsRendererTask36ReplayRun` |
| 5,276 | 1,645,977 | 312 | `.text.hot.draw` | `ndsRendererMtxMul20p12` |
| 4,594 | 2,296,779 | 500 | `.text.hot` | `gcPlayDObjAnimJoint` |
| 3,686 | 442,320 | 120 | `.text.hot` | `gcRunAll` |
| 2,964 | 924,745 | 312 | `.main` | `_ntrcardRomReadSector` |
| 2,957 | 757,017 | 256 | `.text.hot.draw` | `ndsRendererMtxLoadN64ToDS20p12` |
| 2,511 | 1,004,558 | 400 | `.main` | `ndsRendererHardwareGetLightShadeLut` |
| 2,165 | 1,342,337 | 620 | `.text.hot.draw` | `ndsRendererNativeApplyStateDelta` |

Plus a tail of very high density, very small entries that fit the 140 bytes free
*today* with no eviction at all: `cpuGetTiming` (24 B, 128,754), `DynamicArrayGet`
(22 B, 104,713), `lbCommonCos` (8 B, 49,062), `lbCommonSin` (8 B),
`ftCommonDeadCheckInterruptCommon` (8 B, 32,072),
`mpCommonUpdateFighterSlopeContour` (2 B, 15,687 — 31.64 cycles per instruction).

The zero-eviction subset is worth ~2,024 ticks/frame and needs no eviction
decision at all. That is below the noise floor on its own and should be taken as
part of the larger repack, not alone.

## 4. What is safely evictable — less than the headline suggests

The 5,040 idle bytes are not all free to take. Reading the membership rather
than the total, per Task 78 §7:

**Cleanly evictable (port code, placement is ours):** ~3,592 B —
`ndsRendererHardwareConvertTexel01Ci4Direct` (2,600), `ndsRendererNativeEmitDenseRawRun`
(532), `ndsRendererHardwareLitShadeColorPrepared` (460).

**Not to be touched:** `__arm_excpt_*` vectors, `armDCacheFlushAll`,
`armICacheInvalidateAll`, `armContextLoad*`, `threadUnblock*`. These are idle in
the battle window because battle does not fault or switch scenes, which is
exactly why they must stay resident.

**Cannot be evicted individually:** `__addsf3`, `__aeabi_frsub`, `__cmpsf2`,
`__gesf2`, `__lesf2`, `__aeabi_ul2f`, and the `__nds_task*_golden` reference
stubs. These arrive as whole libgcc archive members through the Makefile's
`objcopy --rename-section`, so evicting one evicts its live siblings too.

So the realistic budget is **~3,732 bytes** (3,592 evicted + 140 already free),
not 5,178. Against the section-D ranking that still admits the top four or five
entries and a long tail of small ones.

## 5. The instrument is already specified, and it matters which one

`include/nds/nds_task37_itcm.h` defines **`NDS_TASK37_ITCM_CODE` as placement
only** — `section(".itcm")` and nothing else — and its comment states why:

> a task that moved code and recompiled it differently at the same time could
> not attribute its own result

The other macro, `NDS_RENDERER_HOT_CODE`, additionally applies `hot`, `O3` and
`target("arm")`. Using it for a repack would change placement, ISA and
optimization level in one step, roughly double the byte cost of every admission,
and make the A/B uninterpretable. **The repack uses `NDS_TASK37_ITCM_CODE`.**

## 6. The estimate is an upper bound, and it is stated as one

"Non-mem stall in reach" counts *all* non-memory stall in the admitted symbols.
ITCM residency removes instruction-fetch stall; it does not remove pipeline
hazards or branch mispredicts, which are also non-memory stall. The realised
fraction is unknown and this task will not know it until it measures.

Task 37's own precedent is the best available prior: it measured named work P50
−59,328 and `ALL` P95 −559,680 from a comparable repack. That is the right order
of magnitude to expect, and it is why this is worth doing — not the 67,045
headline.

## 7. One stale comment to fix while here

`nds_task37_itcm.h` still reads *"nothing here is trusted until the responsible
group is identified"*, and `NDS_TASK37_ITCM_PORT` still defaults to 0 on that
basis. Task 45 identified it: the divergence was a state-hash canonicalisation
artifact, not a code defect, and Task 37 shipped. The comment predates that
resolution and now argues against a conclusion the campaign has already reached.

## 8. E1 plan

1. Evict the three dead port functions by dropping their ITCM placement.
2. Admit the section-D ranking greedily by stall/byte until the ~3,732 bytes are
   spent, using `NDS_TASK37_ITCM_CODE` only.
3. A/B from one tree, flag off versus on, per the Task 79 rule — vary the build,
   never the run.
4. Gate on `WORK-H` P95 plus Boundary. No fidelity budget applies: placement
   cannot change output.
5. Re-run the census afterwards. A repack invalidates the ranking that chose it,
   which is the same reason the pack went stale in the first place — and this
   time the follow-up census should be part of the task rather than a later
   discovery.
