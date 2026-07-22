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

<!-- filled in when the runs land -->

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
