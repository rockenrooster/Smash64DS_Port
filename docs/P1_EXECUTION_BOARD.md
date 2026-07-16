# P1 Execution Board

Updated: 2026-07-15 21:13 Central

Deadline: 2026-07-19 23:59 Central

Boundary: `battle_playable_realtime`, mode 163

This is the only dynamic P1 queue. `HANDOFF.md` owns the restart surface,
`optimization/NATIVE_RENDERER_PLAN.md` owns the renderer contract,
`PERF_LEDGER.md` owns measurements, and `PORTING.md` is append-only history.

## Artifact Identity

Current verifier-covered canonical/shipped pair:

```text
smash64ds-battle-playable-hwtri.nds
14,534,656 bytes
SHA-256 CE922B60EFFE16D3A05A18ED3B0FD54F0A73A70C8CE9076AF85A5A59D5B96478
```

Laboratory profile-1 ROMs are evidence only and never replace this filename.

## Phase Evidence

Do not fill this table with figures from a different ROM or phase. Pending is
more accurate than recycling cross-build samples.

| Match phase | Artifact class | ROM SHA-256 | Synchronized window | N | Active median / P95 | State |
|---|---|---|---|---:|---:|---|
| Countdown / GO | canonical-profile-0 | `CE922B60EFFE16D3A05A18ED3B0FD54F0A73A70C8CE9076AF85A5A59D5B96478` | exact completed frames 438/439 | 2 | — / — | GO state + screenshot + post-arm Boundary pass; phase ticks pending |
| Early combat | canonical-profile-0 | same canonical SHA | Pending post-GO window | — | — / — | Pending synchronized canonical baseline |
| Late combat | canonical-profile-0 | same canonical SHA | Pending synchronized late window | — | — / — | Pending synchronized canonical baseline |
| KO / rebirth | canonical-profile-0 | same canonical SHA | Pending natural KO→rebirth window | — | — / — | Pending synchronized canonical baseline |
| Time Up / Results | focused profile-0 | `C07617CA94010535A3B260F0B61A62E8E8ED4AFDDCB4C8968DEE9721642390A9` | natural one-minute expiry→Results | 1 lifecycle | state-only | Pass; realtime ticks pending |

Profile-1 M2 samples and profile-2 forensic samples stay in `PERF_LEDGER.md`; they are not canonical phase baselines.

## Lane Ownership

| Lane | State | Branch / worktree | Owned surface | Runner |
|---|---|---|---|---|
| Integration/release | CPU lifecycle pass / checkpoint | live tree | source-ordered LoadScene break, exact VSBattle ledger sample, commit | runner 2 |
| Renderer implementation | M2 ITCM cut rejected; guard retained | shared live tree / focused lab builds | no-copy direct-contract candidate may combine with the measured 18K ITCM placement only if its pre-code bound reaches the 80K gate | no runner active |
| Gameplay + QA | Paused | shared live tree / disjoint files | sparse DamageFall runtime gate | no runner active |
| Performance research | M2 contract reconciled | shared live tree / read-only | no-copy Mode 8 buckets, gates, source stop rules, and rejected paths confirmed | no runner active |

Runner/window/port policy lives in `VERIFYING.md`; scripted launches normalize
the selected TOML and never touch the user's manual instance.
Workers edit only explicitly assigned disjoint surfaces; the integration lane
owns current-truth docs, shared-file arbitration, commits, and publication.


## Acceptance Matrix

| Acceptance condition | State | Lane | Blocker / evidence | Integration decision |
|---|---|---|---|---|
| Mario human versus original level-3 Fox CPU, Dream Land, one-minute Time match, items off | CPU-on lifecycle pass / temporary fast-iteration default | Gameplay + QA | Flag `0` skips CPU/countdown and freezes the timer; source gates explicitly select flag `1` and retain original Wait → GO timing | Wait for Tyler's re-enable request, then require final CPU-on canonical qualification |
| Original Wait → countdown → GO control/timer gate | Pass | Integration | Automated synchronized source-state proof passes | Keep |
| Mario can damage Fox | Manual pass / continuous gate open | Gameplay | User confirmed damage works after the exact-ROM Fox trace restored 11/11 colliders, zero mismatch, and flag clear | Keep repair; add continuous natural-hit coverage before release |
| Fireball spawn/render/damage/destruction | Pass | Gameplay | Source spawn/damage/lifetime and 40 moving visible textured hardware draws pass with zero rejects | Keep dedicated natural-input gate |
| Fireball trajectory and floor rebound | Early submission/rebound pass / full-lifetime visual OPEN | Gameplay | Current custom `0x47` submits 40/40 and first rebound is 55→46.75, but the check is not an independent source matrix oracle and the screenshot does not ROI-gate the projectile | Trace all 140 ticks; synchronized near/mid/far/event captures; exact-ROM retest |
| Damage/throw map collision | Confirmed manual defect (P0) / isolated LIVE static gate passes | Gameplay | Exact LIVE closure is exclusive after endpoint-world/common-local repair; first natural run stalled before attack, and moving-wall/project-floor plus coherent `mpcommon` remain open | Run sparse natural gate; repair remaining providers before any default-live graduation; require zero fallthrough |
| One-way platform semantics | Reopened / automation insufficient | Gameplay | Crossing frame rejects correctly, but the 715-frame route can accept a wrong next-frame landing without continued ascent or a descending crossing | Extend the same mode-163 gate; keep Tyler's report open |
| Edge behavior and specials | Retest | Gameplay | Up-B was manually accepted; other edge behavior needs current-ROM qualification | No unrelated behavior change without reproduction |
| Normal-play stage painter/depth order | FIXED / pixel + profile-2 pass | Renderer + QA | BattleShip layer modes classify 66 source-Z and 126 no-Z triangles; one full v16 step per no-Z triangle removes the grass/bush overlap, preserves 202 triangles, and reserves disjoint endpoint bands | Keep as correctness fix; final frame 438/501 captures and zero-collision trace are authoritative |
| Pause-orbit geometry containment | REOPENED / user-confirmed defect | Renderer + QA | The scaled-raw repair covered ten range triangles, but Tyler still sees screen-blocking geometry at extreme pause angles; CPU-projected near-plane crossings remain uncontained | Add clip-space near-plane containment and rerun normal/profile-2/+33.6° gates |
| Natural Fox recovery | Coverage debt | Gameplay | Natural Recover objective/offstage return unobserved | No completion claim |
| Cut G M1 affine BG2, 5–35K ticks | Pass | Renderer | 1,856/1,856 ticks; exact frames 438/439 pass and publish | Keep canonical |
| M2 Mario/Fox AOT, 170–250K ticks | Fail / Mode 8 correct; ITCM-only cut rejected | Renderer | Ledger-off A/B at frames 600–607: 416,576/416,704 → 398,496/398,592, only 18,080 saved; exact stage/Mario/Fox 60/320/306 and screenshot stage pixels match | Keep the pre-GX animlock/shuffle guard; do not retry placement alone. Bound the no-copy direct-contract plus placement combination before coding and require ≥80K saved and ≤336,576 |
| M3 complete stage AOT, 150–250K ticks | Semantic pass / performance REWORK | Renderer | Scaled raw clipping fixes the pause-orbit defect and improves the retained stage path 664,544/664,640 → 645,248/645,440, but it remains 145,248 above the first 500K gate. Exact ownership stays 121/828 | Keep clipping fix; do not retry rejected dense-only reuse. Require a different attributable ≥145,248-tick cut before widening |
| M4 zero gameplay conversion/preparation | Pass for current one-minute gate | Renderer | Natural CPU-on expiry proves prepare/arm/teardown `1/1/1`, 22 keys/131,072 bytes, zero ten-class post-GO fence work, and 163,312-byte audio-adjusted reserve | Keep; repeat in final published CPU-on qualification |
| Lower HUD: FPS, timer, labels, stock, damage | Pass | Integration | User approved; lifecycle and Results clear hook pass | Keep |
| Countdown/3-2-1/GO top presentation | FIXED / bitmap OAM | Renderer + QA | Hybrid runtime submission was invisible despite valid counters; restored proven all-bitmap OBJ ownership shows the traffic light with 93,824 bytes prepared pregame and zero gameplay conversion/upload | Keep published bitmap path; hybrid remains lab-only |
| Dream Land BGM | Partial | Audio | User reports the stage theme sounds normal; stream counters pass, but enabled DS channel and nonzero PCM peak remain unproved | Block audio completion |
| Required FGM and Mario/Fox voices | Production phase/KO + isolated crowd-command/loop-pack pass; audible pending | Audio | Source calls ID626 once for 6.9 s; its 1.868 s DS loop intentionally overlaps countdown/GO, but exact channel-mask/acoustic proof is absent | Add host acoustic oracle and tighten existing ACK masks; exact-ROM audible retest; voices remain open |
| Winner and Results BGM | Pass | Audio | Natural Fox winner 16 → Results 22; errors/overrun/cleanup zero, reserve 172,024 | Keep gate |
| Stable reserve / no corruption | Focused one-minute pass | QA | Current source tree reaches Results with 163,312 audio-adjusted bytes, stale=0/0, and 17 safety counters zero | Repeat on final published CPU-on ROM |
| Focused/checkpoint verification | Boundary pass | QA | Current ROM 19C6CD… passes registry/GBI/ITCM, exact 658/44/126/0 classes, M3/M4 state, published pair, and visible exact-frame Cut G capture | Keep only Latest and Boundary; run Current only when the original launch path changes |
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
