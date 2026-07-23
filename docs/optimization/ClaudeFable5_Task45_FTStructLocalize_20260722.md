# Task 45 — Localize the Task 37 FTStruct divergence

Standing rules apply (`docs/optimization/TASK_STANDING_RULES.md`).

Branch: `codex/task45-ftstruct-localize`

## Why

Task 37 (ITCM repack) measured a real improvement and was reverted anyway:

| metric | result |
|---|---|
| named work P50 | **−59,328 ticks** |
| ALL P95 | **−559,680** (one whole VBlank) |
| 3-VBlank share | 71.7% → 76.0% |
| 5+ VBlank share | 5.2% → 3.1% |
| owner play test | retail, faster, correct |

The revert was forced by the Task 9 state-hash gate: **692 of 3,892** per-update
records differ, first at **update 1412**, in three bursts separated by runs of
identical records. The region bisect (`ClaudeFable5_Task37_ItcmRepack_20260722.md`)
then proved the core state — RNG seed, scene, battle, camera, ground,
controllers, collision bounds and speeds — **bit-identical for the whole match**,
and localized the difference to `FTStruct`.

There it stopped. `FTStruct` is hashed as one 3,012-byte blob, so region masks
cannot separate a differing position from a differing pointer.

Two features of the failure make the current verdict untrustworthy in *both*
directions, which is why this is worth resolving rather than leaving reverted:

- The **camera is bit-identical on all 3,892 updates**, and the camera tracks
  fighter positions. That is hard to reconcile with fighter positions differing.
- The divergence **heals**. Records re-converge between bursts. A corrupted
  simulation does not recover; a cached or relocation-derived field does.

Until this is settled the gate blocks every other perf item, because the next
wins the Task 37 census points at — texture resolve/bind 11.36%, soft-float
9.49%, `mem*` 7.84% — will each be judged by it.

## Method

The Task 37 doc prescribed an 8-chunk region-mask binary search: ~6 runs to
resolve a 376-byte window, then more to reach members. This captures the raw
bytes instead — **2 runs, exact offsets**.

`NDS_TASK9_FTSTRUCT_SNAPSHOT` copies both fighters' `FTStruct` into a dedicated
buffer on two consecutive updates:

- **slot 0** = update 1411, the last identical record
- **slot 1** = update 1412, the first differing record

Capturing the update *before* the divergence is the point. If slot 0 is
byte-identical across the two builds and slot 1 differs at offset X, the state
entering the divergence was identical and X is the origin rather than a
downstream consequence of something that already drifted.

The capture is a byte loop, not `memcpy`, deliberately: `memcpy` is one of the
three libc leaves Task 37 relocates, so calling it inside the instrument would
make the instrument a function of the change under test.

Arms: baseline `NDS_TASK37_ITCM_LEAVES=0` vs candidate `=1`. Mask 1 is the
minimal reproducer — masks 1, 2 and 4 each produce the identical 692-record
signature — so it gives the tightest hypothesis space.

### Instrument control

The same-ROM-twice determinism control
(`scripts/check-task37-state-hash-determinism.ps1`) was run earlier in this
campaign and came back IDENTICAL, so the gate itself is known reproducible. Slot
0 additionally serves as an embedded control here: if the two builds agree byte
for byte at update 1411, the run is reproducible up to the divergence.

If the reported first-differing record is **not** 1412, the snapshot index is
wrong and the run must be repeated at the reported index — slot 1 would
otherwise be capturing the wrong update.

## Verdict criteria — pre-registered before the data

Recorded ahead of the result so the reading cannot be fitted to it.

**A. Instrument artifact.** The differing word is an address, and the two values
land on opposite sides of the main-RAM / ITCM boundary (one in
`0x02000000`–`0x02400000`, one in `0x01ff8000`–`0x02000000`).

`ndsTask9StateCanonicalWord` collapses main RAM to `0x20000000` precisely so
`.main` relocation does not register as a state change, then gives ITCM its own
class `0x30000000`. A placement task moves code across exactly that boundary by
definition, so every arm that moves anything into ITCM trips it by construction.
That is also the only account anyone has offered for the one fact nothing else
explains: **three disjoint symbol groups producing one byte-identical failure
signature.**

→ Fix the canonicalizer, re-run the full unmasked gate on masks 0/1/2/4/7, and
take Task 37 through the normal KEEP path.

**B. Real defect.** The differing offset is a gameplay member — `status_id` (36),
`physics` (72), `coll_data` (120), `motion_vars` (380), `percent_damage` (44).

→ Task 37 is a real defect and stays reverted permanently. The canonicalizer is
not touched. Record the offset and stop.

**C. Neither.** Offset is a non-pointer, non-gameplay field (render cache,
animation scratch), or no region crossing appears.

→ No shipping decision. Report the member and what it is written by; do not
weaken the gate to accommodate it.

Outcome A is the only branch that changes the published ROM, and it does not do
so on an agent's judgement — it still needs the owner's play test.

## Results

### The divergence is relocation, not gameplay — established

The snapshot diff at updates 1411 and 1412, baseline mask 0 vs candidate mask 1:

```
215 differing words across both fighters, both updates
    ALL 215 are main-RAM heap pointers
    ALL 215 differ by exactly +0x180 (384 bytes)
    ZERO non-pointer differences
```

Delta histogram — one bucket, no spread:

| delta | count |
|---|---|
| +0x180 (384) | 215 |

The candidate moves three libc leaves (`memset`, `memcpy`, `memcmp`) out of
`.main` into ITCM. The main-RAM image shrinks by 384 bytes and every heap object
below it relocates by exactly that. The differing members are `next`,
`fighter_gobj`, `coll_data`, `input`, all ten `damage_colls`, all twenty-five
`joints`, `motion_scripts`, `computer`, `attack_colls` — every one a pointer.
**No position, velocity, status, damage or any other gameplay value differs
anywhere in either fighter.**

This settles criterion **B**: the fighters are in bit-identical logical state, so
Task 37 is not a real defect. Combined with the earlier region bisect (core state
bit-identical for the whole match, camera identical on all 3,892 updates) and the
owner's retail play test, the simulation is not diverging.

### The leak mechanism is NOT identified — two hypotheses falsified

Criterion **A** as pre-registered is also falsified: there is **no ITCM boundary
crossing**. Every value stays inside main RAM. The mechanism I predicted does not
exist, and the fix the plan named (merging the ITCM class into `0x20000000`)
would have been the wrong edit.

A second hypothesis — that a pointer's canonical *class* flips because
general-heap membership is bounded on `.ptr`, the live allocation high-water —
was tested by bounding on `.end` instead. **It changed nothing at all:** masks
1/2/4/7 each still failed with exactly 692 of 3,892, first at update 1412, an
identical signature. That change was reverted (`60ce8eb`), because its entire
justification was the hypothesis the data killed and an unjustified widening of
what counts as a heap pointer is a gate change with no evidence behind it.

A third hypothesis — that the FTStruct fell outside the heap bound in one build
and was skipped — is excluded for free from the existing exports:

```
of the 692 differing records:
  bytes differ    : 0     (57,168 in both)
  records differ  : 0     (646 in both)
  overflow differ : 0     (zero in both)
  hash-only differ: 692
```

Identical traversal, identical byte count, no overflow. Only canonicalized
content differs. This also explains why `.ptr` → `.end` was inert: the pointers
were already below the high-water in both builds, so widening the range moved
nothing.

### What remains open

Why exactly 692 of 3,892 records, first at 1412, in bursts that heal — when the
underlying pointer delta is a single constant present at *both* sampled updates.
A constant delta on constant objects should mismatch every record or none. That
it does neither means something changes at 1412 that the two dumps do not
capture; the likeliest remaining candidate is a reallocation moving an object to
an address where the two builds' canonical forms stop coinciding.

**Stopped here, at the time-box.** This task has cost roughly eleven emulator
runs against a ~6-run budget, and I have now been wrong twice about the
mechanism. Continuing to guess is the failure mode this campaign already paid
for once.

### Next step, specified so it is not a fourth guess

Dump `gSYTaskmanGeneralHeap.start`, `.ptr` and `.end` alongside the FTStruct
snapshot, at the same two updates, in both builds. With `start` known, the
canonical value of every captured word can be **computed offline** from bytes
already in hand and compared directly — which branch each pointer takes and what
it produces stops being an inference and becomes arithmetic. One run pair.

The single most useful question it answers: **does `start` shift by the same
+0x180 as the pointers?** If yes, the offset form absorbs and the leak is
elsewhere. If no, the offset form is the leak and the canonicalizer needs a
relocation-invariant base.

### Verdict

| question | answer |
|---|---|
| Is Task 37 a real gameplay defect? | **No** — 215/215 differing words are relocated pointers, constant delta, zero gameplay differences |
| Is the gate repaired? | **No** — mechanism unidentified, speculative fix reverted |
| Does Task 37 ship on this? | **Not on my judgement.** The evidence for artifact is now strong, but the gate is still red and the shipping call is the owner's |

Task 37 stays reverted for now. Not because it is believed defective — the
evidence says it is not — but because "the gate is red and I could not repair
it" is not the same as "the gate is green."

## Files

| File | Change |
|---|---|
| `src/nds/nds_task9_state_hash.c` | snapshot buffer + capture in the fighter case |
| `Makefile` | `NDS_TASK9_FTSTRUCT_SNAPSHOT`, `_SNAPSHOT_UPDATE` (both default 0) |
| `scripts/verify-battle-mariofox-gcrunall-loop-harness.ps1` | `-Task9FTStructSnapshotPath`, reusing the Task 34 `dump binary memory` form |
| `scripts/verify-task37-itcm-state-hash-ab.ps1` | `-FTStructSnapshotUpdate`, runs the comparator before the gate throw |
| `scripts/compare-task37-ftstruct-snapshot.ps1` | new — byte diff, offset→member mapping, region classification |

All new flags are lab-only and default to 0; the disabled path compiles to
nothing and both paths build clean under `-Wall -Wextra`.
