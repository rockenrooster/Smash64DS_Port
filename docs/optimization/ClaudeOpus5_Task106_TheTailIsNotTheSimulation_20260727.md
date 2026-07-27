# Task 106/107 E0 — Halving the simulation moves the median across the gate and leaves the tail

**Date:** 2026-07-27
**Status:** E0 complete, both parts. **No runtime change shipped**
(`NDS_TASK106_UPDATES_PER_PRESENT` defaults to 2). The 30 Hz simulation is
sized; the P95 tail is attributed and is a different quantity.
**Inputs:** `artifacts/task106-ctrl.json`, `artifacts/task106-cand.json`,
`artifacts/task106-cand-matched.json`, `artifacts/task107-ctrl-shift.json`.
**Follows:** Task 105's closure of the memory-traffic axis, which left the
30 Hz simulation as the only untested lever above 100,000.

## 1. `SRC` is the update phase, and the sizing needed no instrument

`ndsRunMarioFoxProofUpdate` (`taskman_seam.c:4348`) is the **only** writer of
`gNdsTickHudSourceTicks`, and it brackets `ndsTask39EffectsUpdate()` plus
`scVSBattleFuncUpdate()`. The realtime loop runs it twice per presented frame —
`update_in_iteration < 2u`, with `profile_source_update_by_index[]` recording
each separately.

So `SRC` has been the cost of both logical updates in every measurement this
campaign has taken. `NDS_TASK106_UPDATES_PER_PRESENT` makes it a flag; building
with 1 prices the lever exactly.

`PROJECT_GOAL.md` ranks this **third** in the sacrifice order — above gameplay
fidelity and above stable 30 FPS — and line 211 states the 60 Hz simulation is
"desirable but not sacred".

## 2. The measurement, matched at equal logic ticks

The first A/B was confounded: at 1:1 the match advances one logic tick per
present, so at presented frame 439 the candidate sat at logic tick 439 against
the control's 878, and the two arms sampled different spans of match time. That
reading claimed `WORK-H` P95 −361,472 and it is not usable.

Re-run with the candidate started at presented frame 878, putting both arms at
**logic tick 878**:

| bucket | control (2:1) | candidate (1:1) | Δ |
|---|---|---|---|
| `SRC` P50 | 316,032 | 155,264 | **−160,768 (49.1%)** |
| **`WORK-H` P50** | 1,278,208 | **1,119,616** | **−158,592** |
| **`WORK-H` P95** | 1,706,880 | 1,587,136 | **−119,744** |
| `FTR` P50 | 544,064 | 546,880 | +2,816 |
| `STG` P50 | 347,200 | 350,208 | +3,008 |

`SRC` halves to 49.1% — structurally exactly what removing one of two updates
should do, which is what makes the matched window the correct comparison.

**`WORK-H` P50 lands at 1,119,616, which is 384 ticks under the 1,120,000
gate.** The median frame would pass.

**The gate is P95, and P95 falls only to 1,587,136 — still 467,136 over.**

## 3. Why the tail does not follow the median

Per-frame ranking of both arms, from the ring dumps:

| | control | candidate |
|---|---|---|
| `SRC` median | 316,032 | 155,744 |
| `SRC` max | 834,048 | 678,464 |
| **excursion above median** | **+518,016** | **+522,720** |

**The excursion is unchanged.** Halving the update rate halves the median `SRC`
and leaves its tail exactly where it was, because the excursion is asset loading
driven by animation *events* — and running half as many update ticks does not
reduce the number of distinct animations a match needs to load. It only spreads
them over fewer frames.

That is the finding: **the P95 is not the simulation.** The largest lever the
campaign has available, spent in full, moves the median across the line and
leaves the tail two thirds of the way out.

## 4. A hypothesis tested and refuted

Both arms showed an `FTR` spike of ~980,000 against a ~545,000 median on their
first sampled frame, and both windows began at logic tick 878. That is the
signature of a sampling artifact, and at 32 samples the P95 index is the third
highest value, so an artifact there would have inflated **every P95 quoted in
this session**.

Re-sampling the control from presented frame 500 (logic tick 1000) settles it:

```
FTR   p50 543,040   p95 545,920   max 548,288   spread 1.01
```

No spike. The excursion is a **real game event at logic tick 878–879**, visible
in both arms because both were sampled at the same match moment, not an artifact
of where sampling began. The P95 figures stand.

This is recorded because the artifact hypothesis was mine and it was wrong; the
test cost one run and would have cost the campaign a retraction of a day's
numbers if it had gone the other way.

## 5. Tail attribution across two independent windows

| window | `SRC` excursion | other excursions |
|---|---|---|
| logic 878–940 | **+518,016** | `FTR` +439,584 (one real event), `MISC` +38,880 |
| logic 1000–1062 | **+385,600** | `MISC` +113,024, `FTR` +5,248 |

`SRC` excursion is the persistent tail component in both. `FTR` and `MISC`
spikes are episodic and window-dependent. `STG` is flat in both (+7,360,
+7,232) — Task 104 left it well behaved.

## 6. What this authorizes and what it does not

**Does not authorize building the compensated 30 Hz simulation on performance
grounds alone.** It does not close the gate. Compensation — advancing timers,
physics integration and animation by two frames per tick — is a large change
that turns the Task 9 state hash red by construction, and its acceptance
criterion is `PROJECT_GOAL.md`'s "substantially the same gameplay experience",
which is the owner's judgement on a ROM they play, not a verifier's.

A build with `NDS_TASK106_UPDATES_PER_PRESENT=1` is **not a candidate ROM**. It
is uncompensated and plays at half speed. It is a sizing arm only.

**Does authorize** the next task being the `SRC` excursion rather than any
further work on median cost. The gate is a tail statistic and the tail has one
dominant owner. Tasks 72 and 76 both reduced it (one open per animation load;
the sizing open removed, `SRC` P95 −115,712) and it remains the largest single
component of the frame that exceeds budget.

## 7. Cost

Two builds, four sampling runs, one refuted hypothesis. The 30 Hz lever is now
sized to within the noise floor and does not need revisiting.
