# P1 Execution Board

Updated: 2026-07-15 20:04 Central

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
SHA-256 3F3AC2E1A20F7D93B0E92419BA642FD5D97A275454ABEC0D1C96EF7742E6BB38
```

Laboratory profile-1 ROMs are evidence only and never replace this filename.

## Phase Evidence

Do not fill this table with figures from a different ROM or phase. Pending is
more accurate than recycling cross-build samples.

| Match phase | Artifact class | ROM SHA-256 | Synchronized window | N | Active median / P95 | State |
|---|---|---|---|---:|---:|---|
| Countdown / GO | canonical-profile-0 | `3F3AC2E1A20F7D93B0E92419BA642FD5D97A275454ABEC0D1C96EF7742E6BB38` | exact completed frames 438/439 | 2 | — / — | GO state + screenshot pass; Boundary sampled frame 46 before M4 arm; phase ticks pending |
| Early combat | canonical-profile-0 | same canonical SHA | Pending post-GO window | — | — / — | Pending synchronized canonical baseline |
| Late combat | canonical-profile-0 | same canonical SHA | Pending synchronized late window | — | — / — | Pending synchronized canonical baseline |
| KO / rebirth | canonical-profile-0 | same canonical SHA | Pending natural KO→rebirth window | — | — / — | Pending synchronized canonical baseline |
| Time Up / Results | canonical-profile-0 | same canonical SHA | Pending natural expiry→Results window | — | — / — | Pending one-minute soak |

Profile-1 M2 samples and profile-2 forensic samples stay in `PERF_LEDGER.md`; they are not canonical phase baselines.

## Lane Ownership

| Lane | State | Branch / worktree | Owned surface | Runner |
|---|---|---|---|---|
| Integration/release | Active checkpoint | live tree | consolidated docs, four-record verifier fleet, published identity, commit | runner 2 |
| Renderer implementation | REWORK / bounded dense-prepare cut | shared live tree / `builds/build-m3-stage-owner-lab` | M3 stage owner measured 664.5K; de-duplicate the attributable prepare bucket by dense index | runner 3 |
| Gameplay + QA | Paused | shared live tree / disjoint files | sparse DamageFall runtime gate | no runner active |
| Performance research | Routed | shared live tree / read-only | M4 one-minute GO-to-teardown fence/reserve qualification | no runner active |

Runner/window/port policy lives in `VERIFYING.md`; scripted launches normalize
the selected TOML and never touch the user's manual instance.
Workers edit only explicitly assigned disjoint surfaces; the integration lane
owns current-truth docs, shared-file arbitration, commits, and publication.


## Acceptance Matrix

| Acceptance condition | State | Lane | Blocker / evidence | Integration decision |
|---|---|---|---|---|
| Mario human versus original level-3 Fox CPU, Dream Land, one-minute Time match, items off | CPU-on lifecycle pass / temporary default pause | Gameplay + QA | Fox stays classified level-3 CPU; public/manual default pauses only decisions/inputs, while CPU/lifecycle proofs explicitly enable them | Wait for Tyler's re-enable request, then require final CPU-on canonical qualification |
| Original Wait → countdown → GO control/timer gate | Pass | Integration | Automated synchronized source-state proof passes | Keep |
| Mario can damage Fox | Manual pass / continuous gate open | Gameplay | User confirmed damage works after the exact-ROM Fox trace restored 11/11 colliders, zero mismatch, and flag clear | Keep repair; add continuous natural-hit coverage before release |
| Fireball spawn/render/damage/destruction | Pass | Gameplay | Source spawn/damage/lifetime and 40 moving visible textured hardware draws pass with zero rejects | Keep dedicated natural-input gate |
| Fireball trajectory and floor rebound | Early submission/rebound pass / full-lifetime visual OPEN | Gameplay | Current custom `0x47` submits 40/40 and first rebound is 55→46.75, but the check is not an independent source matrix oracle and the screenshot does not ROI-gate the projectile | Trace all 140 ticks; synchronized near/mid/far/event captures; exact-ROM retest |
| Damage/throw map collision | Confirmed manual defect (P0) / isolated LIVE static gate passes | Gameplay | Exact LIVE closure is exclusive after endpoint-world/common-local repair; first natural run stalled before attack, and moving-wall/project-floor plus coherent `mpcommon` remain open | Run sparse natural gate; repair remaining providers before any default-live graduation; require zero fallthrough |
| One-way platform semantics | Reopened / automation insufficient | Gameplay | Crossing frame rejects correctly, but the 715-frame route can accept a wrong next-frame landing without continued ascent or a descending crossing | Extend the same mode-163 gate; keep Tyler's report open |
| Edge behavior and specials | Retest | Gameplay | Up-B was manually accepted; other edge behavior needs current-ROM qualification | No unrelated behavior change without reproduction |
| Pause-orbit geometry containment | Reproduced wide-view symptom / P2 parity open | Renderer + QA | Normal source-envelope and front/±16.8° frames are clean; synchronized ±33.6° frames show foreground occlusion. Plus view has 15.200% one-color concentration; minus retains only 0.602% green | Keep the deterministic symptom gate; compare identical BattleShip/N64 view before changing renderer |
| Natural Fox recovery | Coverage debt | Gameplay | Natural Recover objective/offstage return unobserved | No completion claim |
| Cut G M1 affine BG2, 5–35K ticks | Pass | Renderer | 1,856/1,856 ticks; exact frames 438/439 pass and publish | Keep canonical |
| M2 Mario/Fox AOT, 170–250K ticks | Fail / Mode 8 correct, Mode 7 rejected | Renderer | Latest detailed A0/A1 477,152/477,376; Mode 7 518,336/518,784 and blank fighters; direct-contract estimate 62–75K is unimplemented | Remove Mode 7; implement only the exact bounded direct-contract path, then require ≥80K saving and ≤337,472 first window |
| M3 complete stage AOT, 150–250K ticks | Semantic pass / performance REWORK | Renderer | Frames 438–445 retain exact 8/57/42/54/202 ownership and zero fence/fallback, but stage-exclusive P50/P95 is 664,544/664,640: ~140K saved vs ~805K, not ≥300K, and 164,544 above the ≤500K gate | Run the Native Plan dense-prepare cut in `nds_renderer.c` + `check_nds_native_stage.py`; require ≥164,544 saved and ≤500K |
| M4 zero gameplay conversion/preparation | Device window pass / Boundary timing gate + full-minute gate pending | Renderer | Frozen frame 0/fraction 114; post-GO frames 438-445 previously prove 22 keys, 21 outputs, 131,072 prepared bytes and zero fence work. Current Boundary attached at frame 46 with all 22 prepared but arm still zero | Align Boundary with the natural post-GO arm, then prove the same zero-work fence and ≥128 KiB reserve through one-minute teardown |
| Lower HUD: FPS, timer, labels, stock, damage | Pass | Integration | User approved; lifecycle and Results clear hook pass | Keep |
| Countdown/3-2-1/GO top presentation | Pass / native OAM | Renderer + QA | Source thread/assets live; 11,584/11,584 ticks, zero gameplay conversion/upload, clean teardown | Keep |
| Dream Land BGM | Partial | Audio | User reports the stage theme sounds normal; stream counters pass, but enabled DS channel and nonzero PCM peak remain unproved | Block audio completion |
| Required FGM and Mario/Fox voices | Production phase/KO + isolated crowd-command/loop-pack pass; audible pending | Audio | Source calls ID626 once for 6.9 s; its 1.868 s DS loop intentionally overlaps countdown/GO, but exact channel-mask/acoustic proof is absent | Add host acoustic oracle and tighten existing ACK masks; exact-ROM audible retest; voices remain open |
| Winner and Results BGM | Pass | Audio | Natural Fox winner 16 → Results 22; errors/overrun/cleanup zero, reserve 172,024 | Keep gate |
| Stable reserve / no corruption | Prior-ROM one-match pass / current gate pending | QA | Pre-M3/M4 baseline reserve was 172,024 bytes with stale=0/0 and 17 safety counters zero; current `3F3AC…` full-minute result is unmeasured | Repeat current published-equivalent one-minute gate |
| Focused/checkpoint verification | Static checks + synchronized capture pass / Boundary open | QA | Registry/docs/architecture/GBI checks pass. Boundary rebuilt successfully but sampled frame 46 before M4 arm and failed closed | Align the post-GO sample; do not weaken the gate. Only Latest and Boundary remain |
| Cut G capture / final dated capture / manual retest | Current Cut G pass / final P1 pending | QA + user | Rebuilt exact frames 438/439 pass on 2026-07-15 under `artifacts/visibility`; top coverage 100%, green 42.495%, detail 52.675%, meaningful delta 0.142% | Block release on final complete-match evidence and user qualification |

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
