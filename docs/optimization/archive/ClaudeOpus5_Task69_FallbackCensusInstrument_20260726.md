# Task 69 — Repairing the instrument, and what the fallback census actually needed

**Date:** 2026-07-26
**Status:** Instrument repaired; census taken. One cause, named.
**Inputs:** `artifacts/task69-rebaseline-task66rom.json`,
`artifacts/task69-ring-task66rom.json`, `artifacts/task69-ring-fallback.json`,
`artifacts/task69-census.json`.

Task 68 left native-owner fallback counters committed but unvalidated: every
attempt to sample them tripped the sampler's own guard,
*"Tick-HUD samples repeated a presented frame"*. That was recorded as probable
emulator state drift. It was not.

## 1. The state-drift hypothesis was wrong

Re-running the known-good ROM (`159cafa3`, `build-task66-tickhud`) with the
current sampler reproduces the earlier capture **exactly**:

| bucket | Task 66 capture | Task 69 re-baseline |
|---|---|---|
| `ALL` P50 / P95 | 1,680,064 / 2,240,512 | 1,680,064 / 2,240,512 |
| `WORK-H` P50 / P95 | 1,371,776 / 1,985,024 | 1,371,776 / 1,985,024 |

Nothing had drifted. `emulators/melonds/rtc.bin` had not even been rewritten
since the clean run — melonDS is killed rather than exited, so it never
persisted the RTC the hypothesis blamed.

A byte-diff of the two ROMs involved settles the other half. `build-task66-tickhud`
and `build-task68-default` differ in **47 bytes**, in three clusters: a 7-character
string at `0xbd104` and the two header/secure-area checksums that follow from it.
The string is `NDS_TASK10_GIT_SHORT`, the commit hash the tick HUD prints. The two
ROMs are the same program. **A different build is not a different ROM for pacing
purposes just because it was built at a different commit.**

## 2. What the guard was catching

Extending the guard to print the offending rows shows the duplicates carry
*different* bucket payloads — frame 442 appears twice with `SRC` 304,640 then
328,896, `WORK` 1,337,664 then 1,331,584. So it is not one stale read of a whole
sample. The presented-frame counter and the frame-complete marker disagree, in
runs, on some ROMs and not others.

That question is now moot rather than answered, because the sampler no longer
depends on the counter — see below. It is recorded here so nobody re-derives it.

## 3. The ROM was already keeping the data

`ndsPlatformTickHudSample` (`src/nds/nds_platform.c`) writes every bucket of
every presented iteration into `sBattleTickHudRing`, an 11 x 128 ring the HUD
uses for its own percentiles. The sampler had been stopping GDB 128 times to
re-collect data the ROM was recording anyway.

`sample-tick-hud-buckets.ps1 -RingDump` now takes **one** stop and reads the ring
with `dump binary memory`. One stop cannot perturb pacing 128 times, and ring
order advances exactly once per finalized iteration, so it cannot express the
repeated-frame failure at all.

Validated against the method it replaces, same ROM, same window:

| | per-frame | ring dump |
|---|---|---|
| `ALL` P50 / P95 | 1,680,064 / 2,240,512 | 1,680,064 / 2,240,512 |
| `WORK-H` P95 | 1,985,024 | 1,985,024 |
| `SRC` P95 | 948,800 | 948,800 |

(The one-frame window shift, 438..565 against 439..566, accounts for the small
differences in the other P50s.)

Two implementation notes, each of which cost a run: GDB's `dump binary memory`
splits its arguments on whitespace, so the bounds must be single tokens — a cast
like `(char *)&ring` parses as two arguments; and a null element in the
PowerShell array of script lines writes a blank line, which GDB executes as
"repeat the previous command".

## 4. The counters read zero, and zero meant nothing

With the instrument working, the fallback census on `build-task68-fallback`
returned **0** for all four reasons — not merely zero in the window, zero
cumulative since reset.

That is not the finding it looks like. `native_owner_enabled = FALSE` is assigned
at **twelve** points in `ndsFighterMarioFoxDLAllDrawForSlot`
(`src/port/reloc_backend_renderer_dl.c`). Task 68 instrumented the last four.
The eight earlier ones — animation locks, selected-count bounds, display-list
residency, material count, cached-owner validation, matrix preparation, material
preparation — all leave `native_owner_enabled` FALSE **before** the block holding
those four counters, so that block is skipped and nothing is counted. The
generic display-list interpreter still runs.

A give-up census has to cover every give-up, or its zero means nothing. The
counters now cover all twelve, plus two denominators (`calls`, `eligible`)
without which "no rejections" cannot be distinguished from "this path was never
taken" — which is exactly the ambiguity that made the first zero unreadable.

The rejection at `reloc_backend_renderer_dl.c:11582` is the one to watch:

```c
((fp->is_use_animlocks != FALSE) || (fp->shuffle_tics != 0u))
```

That is per-fighter animation state, which would hold for a few consecutive
frames at a time — matching the runs of about five that Task 67 measured, and
consistent with `gcPlayDObjAnimJoint` collapsing from 33,876 to 1,493 and
`battleship_ftAnimParseDObjFigatree` falling to zero on those frames. It is a
hypothesis with a mechanism, not a conclusion; the census decides it.

## 5. The census: one cause, and only one

`build-task69-census`, 128 presented frames, all twelve rejection points live:

| counter | window |
|---|---|
| `calls` | **256** |
| `eligible` | **256** |
| `animLock` | **10** |
| `selected`, `displayList`, `materialCount`, `validate` | 0 |
| `matrices`, `materialPrep` | 0 |
| `inputs`, `contract`, `postGx`, `begin` | 0 |

Read the denominators first. 256 draws over 128 frames is exactly two per frame,
one per fighter, so no draw is being missed. `eligible` equal to `calls` means the
fast-run mode selects the native production owner on **every** draw — the owner is
always attempted, which is what the first zero could not establish and what makes
the rest of the table meaningful.

**10 of 256 draws fall back, 3.9%, and every one of them is the animation-lock
rejection.** The other eleven give-up points never fire once. The eight that Task
68 could not see are, with one exception, genuinely zero — but that exception is
the whole effect, which is why a census missing them was worthless rather than
merely incomplete.

The count is the right size for the symptom. `WORK-H` P95 over 128 samples is
about the eighth-worst frame; 10 fallen-back draws land on at most 10 frames.
Task 67's independent estimate from the per-PC profiler was ~1 frame in 20 in
runs of about five, which over 128 frames is ~6 frames. Three independent
routes — profiler attribution, per-frame bucket series, and now a direct
counter — agree on the same handful of frames.

Task 67's conclusion therefore survives, with its mechanism corrected. The
expensive frames are the fighter draw abandoning the native production owner, as
stated. The reason is not texture residency, a run-cache generation miss, or
Task 44 admission, which were the four candidates offered; it is that the native
owner refuses to draw a fighter whose animation is under an animlock or inside a
shuffle window.

## 6. What this makes available

`is_use_animlocks` and `shuffle_tics` describe how the *animation* was computed,
not what geometry comes out of it. The native owner rejects on them
conservatively, up front, before looking at whether the resulting display lists
are anything it could not handle — and Task 67 measured the content on those
frames as unchanged. If the rejection can be narrowed to the cases that actually
need the generic path, the P95 loses its driver at no visual cost. Sizing that,
and confirming the fallback frames are the `WORK-H` spike frames rather than
merely as numerous, is the next task.

The confirmation needs one thing the ring cannot give: the ring holds buckets
only, so `-RingDump` reports fallbacks per run, not per frame. A parallel
128-entry ring for the fallback delta, under the same lab flag, would pin each
fallback to the frame whose `WORK-H` spiked.

## 7. Standing consequences

- Use `-RingDump` for every tick-HUD bucket measurement from here.
- Do not read a zero from a partial census as evidence of absence. Instrument the
  denominator in the same change that instruments the numerator.
