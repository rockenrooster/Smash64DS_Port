# Task 105 E0 — The memory-traffic axis is harvested, and the residue is the type

**Date:** 2026-07-27
**Status:** **STOP at E0. No build spent, no runtime change.** Every remaining
`mem*` lever is under the noise floor once Task 104's realisation rule is applied.
**Input:** `artifacts/task105-memcpy-callers.json` — 33,618 `memcpy` samples from
frame 439, exact-entry-address method, current master (post-85, post-104).
**Follows:** Task 104's win, Task 85 §6's unowned list, Task 84's `memset` table.

## 1. Why this was opened

Task 104 removed a 1,292-byte clear plus a 1,292-byte copy that carried four live
bytes, for `STG` −22,016. The obvious question is whether the same shape exists
elsewhere. Task 84's `memset` table and Task 85 §6's `memcpy` remainder both name
candidates, and neither had been audited for liveness.

## 2. `memset` closes by arithmetic, without an instrument

Task 84 E1.3 established that `ndsRendererInitStats` — 39.8% of `memset` bytes —
accounted for **~72% of `memset` time**, because a 1,292-byte clear spans ~41 cold
lines at 2.74 ticks/byte while the 101-byte average clear writes resident lines at
1.51.

That prices the entire remainder: 28% × 57,206 = **~16,018 ticks/frame across all
five other callers**, so the largest is ~4,000. Every one is under the 5,000–7,000
floor before any change is designed.

`ndsRendererAdapterMtxIdentity20p12` is the most Task-104-shaped of them —

```c
memset(out, 0, sizeof(*out));          /* 64 bytes */
for (i = 0; i < 4u; i++)
    out->m[i][i] = 1 << NDS_RENDERER_ADAPTER_MTX_FRAC_BITS;
```

a 64-byte clear to write four values, 8,916 sampled calls. Its share is ~4,400
ticks/frame. Correct shape, wrong size.

## 3. `memcpy`, re-attributed on current master

33,618 samples. Dated against the post-Task-85 rate of ~412 calls/frame, the
sample spans ~81.6 frames.

| caller | samples | share | calls/frame | bytes/frame |
|---|---|---|---|---|
| `ndsRendererAdapterBuildDObjLocalMatrix` | 6,064 | 18.0% | ~74 | 4,736 |
| `ndsRendererAdapterDObjWorldIndexHash` | 5,900 | 17.6% | ~72 | 4,608 |
| `ndsRendererAdapterBuildPersistentStageWorldMatrix` (2 sites) | 6,276 | 18.7% | ~77 | 4,928 |
| `ndsFighterDisplayContractSelectDL` | 3,776 | 11.2% | ~46 | — |
| `ndsRendererAdapterPrepareNativeOwnerMatrices` | 3,862 | 11.5% | ~47 | 3,008 |
| `ntrcardRomRead` | 2,694 | 8.0% | ~33 | — |
| `ndsRendererAdapterPrepareInitialMatrices` | 1,991 | 5.9% | ~24 | 1,536 |

**Two rows are attribution artifacts and must not be used.**
`ndsRendererAdapterDObjWorldIndexHash` contains no `memcpy` — it is four integer
operations on a pointer. `ndsFighterDisplayContractSelectDL` contains no `memcpy`
and no matrix; it writes eleven scalar fields into an event slot. Both are
inlined-range resolutions. The hash row is recoverable by inspection — it is
`ndsRendererAdapterBuildDObjWorldMatrix`'s cache-hit copy, `*out = *cached`, with
`FindDObjWorldMatrix` inlined into it. The `SelectDL` row is not attributable
without a further run and is excluded rather than guessed at.

Matrix copies are therefore **~294 calls/frame × 64 B = 18,816 bytes/frame**,
about 60% of `memcpy` calls, worth 20,000–40,000 ticks/frame in aggregate.

## 4. Why the aggregate does not survive contact with Task 104's rule

Task 104 established that removing one of two accesses to the same cache lines
relocates the misses rather than eliminating them, and realises roughly 28%.

Every matrix copy site here has that shape. The destination is a caller-owned
`NDSRendererMatrix20p12` — usually a stack local — that the caller then reads.
Passing by reference removes the write but the reader still touches 64 bytes
somewhere; only the destination write and one of the two reads actually go cold.

Per site, after the discount:

| site | nominal | realised at ~28% |
|---|---|---|
| `BuildDObjLocalMatrix` | ~10,200 | ~2,900 |
| `BuildDObjWorldMatrix` hit path | ~10,000 | ~2,800 |
| `BuildPersistentStageWorldMatrix` ×2 | ~10,600 | ~3,000 |
| `PrepareNativeOwnerMatrices` | ~6,500 | ~1,800 |
| `PrepareInitialMatrices` | ~3,300 | ~900 |

Nothing here is resolvable by an A/B. Taking all five would be five separate
correctness arguments about pointer lifetime into a per-frame cache that
`ndsRendererAdapterStoreDObjWorldMatrix` mutates during the same traversal — the
aliasing hazard Task 104 did not have, because its copy was provably dead.

**STOP.** This is the rule from Task 100 applied in advance rather than after a
flat reading: a lever predicted under ~7,000 cannot be measured, so it must not be
built to find out.

## 5. The residue is the type, not the call sites

`NDSRendererMatrix20p12` is 4×4 s20.12 — 64 bytes. Every one of these transforms
is affine; the codebase already knows this (`ndsRendererMtxMulAffine20p12`), and
the DS hardware loads 4×3 natively. Row 3 is invariably (0,0,0,1) and carries no
information.

A 4×3 hot representation would take 25% off all 294 copies per frame, off every
`MtxIdentity20p12` clear, and off the DObj world cache's footprint — together, not
one site at a time, which is the only form in which this residue is above the
floor. It is still only ~10,000 ticks/frame, against a refactor touching every
renderer file with no single verifiable seam and no way to A/B it incrementally.

**Not recommended for Runtime 1.** Recorded because it is the correct answer to
"what would it take", and because it is exactly the decision a new runtime gets to
make for free at declaration time.

## 6. What this means for the Runtime 2 question

This result runs against the recommendation that opened it, and should be read
that way.

The case for staying in Runtime 1 rested partly on Task 104 proving there was more
recoverable memory traffic to find. **There is not much.** Task 85 removed 51% of
`memcpy` calls, Task 104 removed the clear-plus-copy that was 72% of `memset`
time, and what remains is six sites of 3,000–10,000 nominal that discount to
1,000–3,000 each. The axis is harvested.

That strengthens the memory-traffic argument in
`Smash64DS_Runtime2_Revised_After_Task104.md` §5–§8 rather than weakening it: the
remaining traffic is structural, fixed by a type declaration made before any of
this code existed, and unreachable by the incremental method that has taken every
other win in this campaign.

It does **not** settle the question on its own. The larger untested Runtime 1
lever is the 30 Hz visual pose (§9/§16 of that document, and pre-authorised by
`PROJECT_GOAL.md`'s sacrifice order): animation is 106,700 ticks/frame, renderer
matrix 107,810, fighter native production 255,061, and halving the visual half of
those is worth more than everything Task 105 examined. That is the test that
should decide it.

## 7. Cost of this task

One 90-second census run. No builds, no A/B, no verifier. The alternative was
five builds and five flat readings.
