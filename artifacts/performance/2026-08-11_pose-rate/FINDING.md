# 30 Hz pose evaluation — REJECTED, and the A/B that priced it was invalid

Cycle 119. Same-binary route A/B on `builds/build-c119-pose-route`
(`smash64ds-battle-playable-tickhud-hwtri`, `NDS_R2_BOTH_CPU=1`,
`NDS_R2_POSE_RATE_ROUTE=1`), 1,600 presented frames from 438, stride 96,
DLDI ON. Arm A `gNdsR2PoseRateRoute=0`, arm B `=1`, one ELF, one poke.

## Verdict

**REJECT.** Not because it saved too little — because it changes how the
level-3 CPU plays, which `PROJECT_GOAL.md` names as a requirement
("Fox must use behavior equivalent to the original Level-3 CPU"), and because
the instrument cannot price a gameplay-affecting change over a whole match.

| bucket | arm A (60 Hz) | arm B (30 Hz) | delta |
|---|---:|---:|---:|
| `WORK-H` P50 | 959,360 | 966,144 | **+6,784** |
| `WORK-H` P95 | 1,255,104 | 1,262,144 | **+7,040** |
| `SCPU` P50 (CPU decision proc) | 41,344 | 33,088 | −8,256 (−20.0%) |
| `SINT` P50 (interrupt proc) | 151,040 | 120,704 | −30,336 |
| `SPHD` P50 (physics/map arm) | 59,136 | 64,704 | **+5,568** |
| `SCAT` max | 1,600 | 910,400 | **×569** |

Arm A reproduces the banked gate (1,258,112) to within 3,008 — a third of the
±8,544 cross-build floor — so the control is sound and the instrument is not
the problem.

## Engagement is real; that is not the issue

`gNdsR2PoseSkippedDObjs = 149,323` (93.3 per presented frame) and
`gNdsR2PoseSkippedNodes = 586,162` (366 per presented frame) against 0/0 on
arm A. E61 already established `GOBJ_FLAG_NOANIM` skips are **0**, so every
pose computed is a pose used and the route removes exactly half of them.
The lever fired at full strength and still did not move the gate.

## Divergence is proven, not inferred

A repeat of both arms carrying end-of-match gameplay counters
(`armA2*` / `armB2*`, identical buckets to the first pair):

| counter | arm A (60 Hz) | arm B (30 Hz) |
|---|---:|---:|
| `gNdsBattleTextHudP0Damage` | **130** | **33** |
| `gNdsBattleTextHudP1Damage` | **51** | **65** |
| `gNdsDamageSparkScaleCount` | 13 | 6 |
| `gNdsR2CubicEvals` | 292,679 | 225 |
| `gNdsR2PoseSkippedDObjs` | 0 | 149,323 |
| `gNdsR2PoseSkippedNodes` | 0 | 586,162 |

Same ELF, same seed, one poked bit, and the two matches end with different
damage on both fighters. That is the whole argument: these are not two
measurements of one match.

`gNdsR2CubicEvals` collapsing to 225 rather than halving is not explained by
the route -- `gNdsR2PoseSkippedNodes` shows exactly half the nodes skipped, as
designed. The likeliest reading is that arm B's diverged match reached a scene
transition that re-initialised the counter before the end-of-run read; it is
recorded here as a further inconsistency between the arms, and the verdict does
not rest on it.

## Why the number is not a cost

`SCPU` is the per-fighter CPU decision proc. It does not evaluate poses. It
moved **−8,256 (−20%)**. `SPHD` moved the other way. `SCAT`'s max went from
1,600 to 910,400. Per-frame `WORK-H` correlation between the arms is **+0.062**
and no frame in 1,600 matches; the windowed delta swings from −24,896 to
**+95,455**. Those are not the signatures of a cost change, they are two
different matches: the evaluated pose lands in `dobj->rotate/translate/scale`,
collision reads it, the fight diverges within the sampled window, and after
53 s the arms share only their seed.

**Standing rule this establishes: a one-binary route A/B is valid only for a
change that cannot alter gameplay state.** The route form removes the
cross-build placement floor, which is why the board prefers it — but it assumes
both arms walk the same trajectory. A change that feeds collision breaks that
assumption, and the resulting delta prices the difference between two matches,
not the difference between two implementations. Such a change needs a fixed
input replay, or must be priced by its own subsystem cost rather than by frame
cost.

## What was learned that keeps

- **Parity must come from the LOGIC tick, not `gNdsFrameCounter`.** That
  counter advances once per PRESENTED frame and this configuration runs
  `NDS_TASK106_UPDATES_PER_PRESENT == 2` logic updates inside each one, so its
  parity is constant across both. The first build of this route used it and
  would have evaluated twice on even frames and zero times on odd — the same
  mean, the surviving work clustered onto alternate frames (the wrong shape for
  a percentile), and a 15 Hz pose rather than 30. Logic-tick parity is exactly
  `update_index & 1`; `ndsR2HostBattleUpdateOnce` publishes it.
- **Skip update 0, evaluate update 1.** The presented frame is drawn after the
  last update, so evaluating on the last one hands the renderer a fully current
  pose every presented frame. The change is invisible at 30 Hz presentation;
  the entire cost is paid by gameplay reads during update 0. That is what makes
  it a gameplay trade rather than a visual one, and why the sacrifice order in
  `PROJECT_GOAL.md` — visual fidelity before gameplay fidelity — argues against
  it before any tick is measured.
- **No speed doubling.** The loop already separates advancing animation TIME
  (`play & 1`) from EVALUATING the pose (`play & 2`), so `length` keeps
  advancing at 60 Hz and timing stays bit-identical. Doubling the step would
  have changed WHICH keyframe is selected, not just how often it is sampled.

## Harness defect found and fixed on the way

Both arms first failed `sample-tick-hud-buckets.ps1`'s repeated-presented-frame
guard, twice, costing two whole-match runs. All five duplicates sat at exact
multiples of `-RingStopStride`, and they reproduced with the route **off**.

Proven, not inferred: the run's own `ringStopSkews` records exactly five stops
with `skew == -1` (stops 2, 4, 9, 11, 15) and there are exactly five duplicates,
at exactly those stops' first rows. The stitcher labels rows by counting
backward from the presented-frame counter while the same function documents that
ring slots need not equal presented frames — so `skew == -1` makes a stop's
first label collide with the previous stop's last by construction.

`sample-tick-hud-buckets.ps1` now classifies a duplicate that a stop's recorded
negative skew explains, warns precisely, and continues without an override. It
does **not** soften the guard: an identical payload still fails unconditionally,
and so does any duplicate away from a seam — which is what caught the cycle-118
vertex-memo defect, whose four duplicates included one that was not at a stop
boundary.

## Files

`armA.json` / `armA-rows.csv` — 60 Hz control.
`armB.json` / `armB-rows.csv` — 30 Hz candidate.
`armA2*` / `armB2*` — repeat carrying end-of-match gameplay counters.
`armA-precheck-seamfail.log` — the run that exposed the harness seam defect.
