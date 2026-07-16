# P1 Execution Board

Updated: 2026-07-16 05:08 Central

Deadline: 2026-07-19 23:59 Central

Boundary: `battle_playable_realtime`, mode 163

This is the only dynamic P1 queue. `HANDOFF.md` owns the restart surface,
`optimization/NATIVE_RENDERER_PLAN.md` owns the renderer contract,
`PERF_LEDGER.md` owns measurements, and `PORTING.md` is append-only history.

## Artifact Identity

Current user-facing candidate; Boundary republication is pending:

```text
smash64ds-battle-playable-hwtri.nds
14,564,352 bytes
SHA-256 6265772AB02446A1247DB8444129A3040835BDDDBC968A090DA2AA289423ED24
```

Laboratory profile-1 ROMs are evidence only and never replace this filename.

## Phase Evidence

Do not fill this table with figures from a different ROM or phase. Pending is
more accurate than recycling cross-build samples.

| Match phase | Artifact class | ROM SHA-256 | Synchronized window | N | Active median / P95 | State |
|---|---|---|---|---:|---:|---|
| Countdown / GO | canonical-profile-0 | pending candidate republication | exact completed frames 438/439 | 2 | — / — | Prior gate passed; refresh on final candidate only |
| Early combat | canonical-profile-0 | pending candidate republication | Pending post-GO window | — | — / — | Pending synchronized canonical baseline |
| Late combat | canonical-profile-0 | pending candidate republication | Pending synchronized late window | — | — / — | Pending synchronized canonical baseline |
| KO / rebirth | canonical-profile-0 | pending candidate republication | Pending natural KO→rebirth window | — | — / — | Pending synchronized canonical baseline |
| Time Up / Results | focused profile-0 | `C07617CA94010535A3B260F0B61A62E8E8ED4AFDDCB4C8968DEE9721642390A9` | natural one-minute expiry→Results | 1 lifecycle | state-only | Pass; realtime ticks pending |

Profile-1 M2 samples and profile-2 forensic samples stay in `PERF_LEDGER.md`; they are not canonical phase baselines.

## Lane Ownership

| Lane | State | Branch / worktree | Owned surface | Runner |
|---|---|---|---|---|
| Integration/release | CPU lifecycle pass / checkpoint | live tree | source-ordered LoadScene break, exact VSBattle ledger sample, commit | runner 2 |
| Renderer implementation | M2/M3 feasibility cuts exhausted; exact guards retained | shared live tree / focused lab builds | M2 Jump C stopped 8,096 ticks short before code; M3 keeps only the 12.99K no-Z codegen gain | no runner active |
| Gameplay + QA | Paused | shared live tree / disjoint files | sparse DamageFall runtime gate | no runner active |
| Performance research | Paused at measured boundary | shared live tree / read-only | Do not reopen rejected M2 ITCM or M3 dense/incremental-matrix cuts without a new attributable bound | no runner active |

Runner/window/port policy lives in `VERIFYING.md`; scripted launches normalize
the selected TOML and never touch the user's manual instance.
Workers edit only explicitly assigned disjoint surfaces; the integration lane
owns current-truth docs, shared-file arbitration, commits, and publication.


## Acceptance Matrix

| Acceptance condition | State | Lane | Blocker / evidence | Integration decision |
|---|---|---|---|---|
| Mario human versus original level-3 Fox CPU, Dream Land, one-minute Time match, items off | CPU-on public default / visible fast-iteration override | Gameplay + QA | The published ROM and source runtime gates retain flag `1`; the separate `-FastIteration` screenshot launch selects flag `0` before battle, skipping countdown/Fox and freezing `1:00` | Keep lifecycle gates on `1`; use the focused flag-`0` capture during routine visual iteration |
| Original Wait → countdown → GO control/timer gate | Pass | Integration | Automated synchronized source-state proof passes | Keep |
| Mario can damage Fox | Manual pass / continuous gate open | Gameplay | User confirmed damage works after the exact-ROM Fox trace restored 11/11 colliders, zero mismatch, and flag clear | Keep repair; add continuous natural-hit coverage before release |
| Fireball spawn/render/damage/destruction | Pass | Gameplay | Source spawn/damage/lifetime and 40 moving visible textured hardware draws pass with zero rejects | Keep dedicated natural-input gate |
| Fireball trajectory and floor rebound | Early submission/rebound pass / full-lifetime visual OPEN | Gameplay | Current custom `0x47` submits 40/40 and first rebound is 55→46.75, but the check is not an independent source matrix oracle and the screenshot does not ROI-gate the projectile | Trace all 140 ticks; synchronized near/mid/far/event captures; exact-ROM retest |
| Damage/throw map collision | Natural floor recovery pass / non-floor providers open | Gameplay | Five source up-smashes produced two damage events; Fox entered status 54, crossed line 3, and completed one floor recovery with direct result `1/0` and zero invalid results | Keep sparse gate; repair moving-wall/project-floor and coherent `mpcommon` before default-live graduation |
| One-way platform semantics | Reopened / automation insufficient | Gameplay | Crossing frame rejects correctly, but the 715-frame route can accept a wrong next-frame landing without continued ascent or a descending crossing | Extend the same mode-163 gate; keep Tyler's report open |
| Edge behavior and specials | Retest | Gameplay | Up-B was manually accepted; other edge behavior needs current-ROM qualification | No unrelated behavior change without reproduction |
| Normal-play stage painter/depth order | FIXED / pixel + profile-2 pass | Renderer + QA | BattleShip layer modes classify 66 source-Z and 126 no-Z triangles; one full v16 step per no-Z triangle removes the grass/bush overlap, preserves 202 triangles, and reserves disjoint endpoint bands | Keep as correctness fix; final frame 438/501 captures and zero-collision trace are authoritative |
| Pause-orbit geometry containment | FIXED / user confirmed | Renderer + QA | Clip-space near-plane containment removes screen-blocking triangles at the breaking orbit angles without changing normal-play profile-2 output | Keep focused angle gate; paused −33.6° is also the strongest Mario underside view |
| Mario pant/underside visual | Candidate KEEP / user visual pending | Renderer + QA | Source root light preambles were missing; replay restores blue right pant and closed underside with unchanged 320-triangle census | Tyler eyeballs `20260716-034036_slot3_p10612_mode163_camera_pause_minus33p6.png` before FIXED |
| Natural Fox recovery | Coverage debt | Gameplay | Natural Recover objective/offstage return unobserved | No completion claim |
| Cut G M1 affine BG2, 5–35K ticks | Pass | Renderer | 1,856/1,856 ticks; exact frames 438/439 pass and publish | Keep canonical |
| M2 Mario/Fox AOT, 170–250K ticks | Fail / Jump C stopped before code | Renderer | Current local builder is 53,824 ticks and lighting is already prepared-direction + exact shade LUT. Local + rejected ITCM gain bounds at 71,904, 8,096 short of the required first cut | Do not manufacture another lighting/cache path; require a new source-backed bound before coding |
| M3 complete stage AOT, 150–250K ticks | Semantic pass / performance REWORK | Renderer | Removing cold/Os from the 126-triangle no-Z emitter keeps exact pixels and improves stage 624,384 → 611,392. Dense reuse reached 563,296 but remained above 500K and was reverted; incremental matrix transport regressed | Keep only codegen commit `bbe8d3eee2`; require a new attributable ≥111,392-tick cut |
| M4 zero gameplay conversion/preparation | Pass for current one-minute gate | Renderer | Natural CPU-on expiry proves prepare/arm/teardown `1/1/1`, 22 keys/131,072 bytes, zero ten-class post-GO fence work, and 163,312-byte audio-adjusted reserve | Keep; repeat in final published CPU-on qualification |
| Lower HUD: FPS, timer, labels, stock, damage | Pass | Integration | User approved; lifecycle and Results clear hook pass | Keep |
| Countdown/3-2-1/GO top presentation | FIXED / bitmap OAM | Renderer + QA | Hybrid runtime submission was invisible despite valid counters; restored proven all-bitmap OBJ ownership shows the traffic light with 93,824 bytes prepared pregame and zero gameplay conversion/upload | Keep published bitmap path; hybrid remains lab-only |
| Dream Land BGM | Partial | Audio | User reports the stage theme sounds normal; stream counters pass, but enabled DS channel and nonzero PCM peak remain unproved | Block audio completion |
| Required FGM and Mario/Fox voices | Crowd FIXED / fighter voices open | Audio | ID626 is one finite 104,204-sample source-loop-ordered AOT cue with quadratic source ramps and no DS hardware loop/runtime envelope; Tyler confirmed the audible fix | Keep crowd cue; qualify remaining natural Mario/Fox voice IDs and pitch behavior |
| Winner and Results BGM | Pass | Audio | Natural Fox winner 16 → Results 22; errors/overrun/cleanup zero, reserve 172,024 | Keep gate |
| Stable reserve / no corruption | FAIL on current candidate | QA | Natural DamageFall run reports 128,528 bytes, 2,544 below the 131,072-byte floor; the older 163,312-byte result predates the current AOT/audio candidate | Recover at least 2,544 measured bytes without changing behavior, then rerun only the focused reserve-bearing gate |
| Focused/checkpoint verification | Focused gates pass / Boundary refresh pending | QA | Current candidate has exact profile-2 fighter/stage census, pause-angle containment, audio cue checks, and synchronized M2/M3 measurements; no stacked full regression was run | Run one Boundary only at the next release checkpoint; Current only if original launch changes |
| Cut G capture / final dated capture / manual retest | Current Cut G pass / final P1 pending | QA + user | Exact frames 438/439 pass on 2026-07-15 under `artifacts/visibility`; top coverage 100%, green 42.495%, detail 52.675%, meaningful delta 0.142% | Block release on final complete-match evidence and user qualification |

## Dated Gates

### July 16 — Hard Feasibility Checkpoint

- Publish phase-specific P95 active ticks for countdown, early/late combat,
  KO/rebirth, and Results.
- A credible 60 FPS path must converge near one VBlank (~560K ticks, preferably
  ≤520K with margin) in every material phase, including interface work.
- If evidence does not support that path, present an explicit decision: keep
  pursuing 60 FPS without claiming P1 complete; request approval for a lower
  stable presentation target; or deliver the best verifier-covered incomplete
  candidate. Do not silently redefine P1.

### July 17

- Architecture and integration freeze. Merge only measured candidates and
  confirmed blocker fixes. Run Current instead of Boundary when shared runtime
  or normal launch changes.

### July 18

- Release qualification: one-minute soak, focused checks, one Boundary or
  Current run, clean canonical rebuild/parity, exact-ROM manual retest, and
  dated capture set.

### July 19

- Release-candidate fixes and verification only. No new architecture and no P2
  work. Report every remaining red acceptance row honestly.

## Integration Rule

Performance iteration is eight synchronized A frames and eight B frames with
ticks, FPS, screenshots/image analysis, and cheap correctness counters. Add A2
only when inconclusive. Compilation alone never closes a row. P2 begins only
after every required P1 row is green, documented, and snapshotted.
