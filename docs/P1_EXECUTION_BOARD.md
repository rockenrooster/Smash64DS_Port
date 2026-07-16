# P1 Execution Board

Updated: 2026-07-16 07:41 Central

Deadline: 2026-07-19 23:59 Central

Boundary: `battle_playable_realtime`, mode 163

This is the only dynamic P1 queue. `HANDOFF.md` owns the restart surface,
`optimization/NATIVE_RENDERER_PLAN.md` owns the renderer contract,
`PERF_LEDGER.md` owns measurements, and `PORTING.md` is append-only history.

## Artifact Identity

Current user-facing candidate; Boundary republication is pending:

```text
smash64ds-battle-playable-hwtri.nds
14,574,592 bytes
SHA-256 A8371FA93E75338F8BABAC445FA4826663979FE48884E215F27630049B3B6C93
```

Laboratory profile-1 ROMs are evidence only and never replace this filename.

## Phase Evidence

Do not fill this table with figures from a different ROM or phase. Pending is
more accurate than recycling cross-build samples.

| Match phase | Artifact class | ROM SHA-256 | Synchronized window | N | Active median / P95 | State |
|---|---|---|---|---:|---:|---|
| Countdown / GO | focused profile-1 | `426B821A...` | exact completed frames 438–445 | 8 | 1,435,424 / 1,441,088 | Same-ROM hard-checkpoint sample; 19.6 FPS |
| Early combat | focused profile-1 | `426B821A...` | exact completed frames 600–607 | 8 | 1,415,424 / 1,416,064 | Same-ROM hard-checkpoint sample; 19.2 FPS |
| Mid combat / Whispy change | focused profile-1 | `426B821A...` | exact completed frames 1398–1405 | 8 | 1,616,000 / 1,617,920 | Same-ROM M4 lifecycle sample; worst measured material phase |
| Late combat | focused profile-1 | `426B821A...` | exact completed frames 3300–3307 | 8 | 1,240,832 / 1,414,912 | Same-ROM late sample; no M4 fallback |
| KO / rebirth | canonical-profile-0 | pending candidate republication | Pending natural KO→rebirth window | — | — / — | Pending synchronized canonical baseline |
| Time Up / Results | focused profile-0 | `C07617CA94010535A3B260F0B61A62E8E8ED4AFDDCB4C8968DEE9721642390A9` | natural one-minute expiry→Results | 1 lifecycle | state-only | Pass; realtime ticks pending |

Profile-1 hard-checkpoint windows are O2-equivalent feasibility evidence, not
profile-0 release baselines. M2 detail and profile-2 forensic samples stay in
`PERF_LEDGER.md`.

## Lane Ownership

| Lane | State | Branch / worktree | Owned surface | Runner |
|---|---|---|---|---|
| Integration/release | CPU lifecycle pass / checkpoint | live tree | source-ordered LoadScene break, exact VSBattle ledger sample, commit | runner 2 |
| Renderer implementation | M2/M3 feasibility cuts exhausted; exact guards retained | shared live tree / focused lab builds | M2 Jump C stopped 8,096 ticks short before code; M3 keeps only the 12.99K no-Z codegen gain | no runner active |
| Gameplay + QA | Paused | shared live tree / disjoint files | sparse DamageFall runtime gate | no runner active |
| Performance research | Paused at measured boundary | shared live tree / read-only | Do not reopen rejected M2 ITCM or M3 dense/incremental-matrix/signed-16-rounding cuts without a new attributable bound | no runner active |

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
| Fireball trajectory and floor rebound | Early submission/rebound pass / natural terminal visual OPEN | Gameplay | Current custom `0x47` submits 40/40 and first rebound is 55→46.75 at lifetime 122. The focused exact-ROM gate skips countdown/Fox and finishes in 9.5 s. The shot then leaves Pupupu's map bounds, so BattleShip `wpprocess.c:167-180` destroys it before either lifetime-zero or low-speed-map callback; probes waiting on those two wrong branches timed out and were reverted | Observe the source out-of-bounds destroy line once, then capture the next completed frame and add an independent source-matrix/ROI gate. Do not poll frames or require the unused 140-tick maximum |
| Damage/throw map collision | Natural floor + throw recovery pass / non-floor providers open | Gameplay | Five source up-smashes proved DamageFall recovery. A separate external-input route then used nine short source Walk/Dash/Run steps for one Mario catch/forward throw: Fox took 0→12%, released status 169→186, swept/clamped to line 3, DownBounced once, cleared every catch link, and retained 202,256 bytes | Keep both sparse gates; throw evidence is `2026-07-16_063512-7696185_throw-release-recovery-p21764.png`; repair moving-wall/project-floor and coherent `mpcommon` before default-live graduation |
| One-way platform semantics | Pass / hardened natural gate | Gameplay | Current-ROM mode 163 completed 715 natural frames: six ordered continued-ascent/strict-descent/downward-crossing flights (`0x3f`), all three platform masks `0x7`, two side cycles, three exact ignore-line Pass crossings, nine landings, and 214,544-byte reserve | Keep the focused gate; screenshot `2026-07-16_052652-7356809_platform-semantics-p984.png` |
| Edge behavior and specials | Retest | Gameplay | Up-B was manually accepted; other edge behavior needs current-ROM qualification | No unrelated behavior change without reproduction |
| Normal-play stage painter/depth order | FIXED / pixel + profile-2 pass | Renderer + QA | BattleShip layer modes classify 66 source-Z and 126 no-Z triangles; one full v16 step per no-Z triangle removes the grass/bush overlap, preserves 202 triangles, and reserves disjoint endpoint bands | Keep as correctness fix; final frame 438/501 captures and zero-collision trace are authoritative |
| Pause-orbit geometry containment | FIXED / user confirmed | Renderer + QA | Clip-space near-plane containment removes screen-blocking triangles at the breaking orbit angles without changing normal-play profile-2 output | Keep focused angle gate; paused −33.6° is also the strongest Mario underside view |
| Mario pant/underside visual | Candidate KEEP / user visual pending | Renderer + QA | Source root light preambles were missing; replay restores blue right pant and closed underside with unchanged 320-triangle census | Tyler eyeballs `20260716-034036_slot3_p10612_mode163_camera_pause_minus33p6.png` before FIXED |
| Natural Fox recovery | Pass | Gameplay | Current-ROM mode 163 used only external Mario input: Fox took 0→59 damage, selected BattleShip Recover for 40 frames at offstage x=2379.905, returned without KO/rebirth, and grounded on line 3 at x=1336.084 after 820 frames | Keep `verify-battle-playable-fox-recovery.ps1`; current reserve 202,256 and screenshot `2026-07-16_055015-5167574_fox-recovery.png` |
| Cut G M1 affine BG2, 5–35K ticks | Pass | Renderer | 1,856/1,856 ticks; exact frames 438/439 pass and publish | Keep canonical |
| M2 Mario/Fox AOT, 170–250K ticks | Fail / Jump C stopped before code | Renderer | Current local builder is 53,824 ticks and lighting is already prepared-direction + exact shade LUT. Local + rejected ITCM gain bounds at 71,904, 8,096 short of the required first cut | Do not manufacture another lighting/cache path; require a new source-backed bound before coding |
| M3 complete stage AOT, 150–250K ticks | Semantic pass / performance REWORK | Renderer | Removing cold/Os from the 126-triangle no-Z emitter keeps exact pixels and improves stage 624,384 → 611,392. Dense reuse reached 563,296 but remained above 500K; incremental matrix transport regressed; signed-16 rounding saved only 2,048 ticks and failed its visual packet. All three were reverted | Keep only codegen commit `bbe8d3eee2`; require a new attributable ≥111,392-tick cut |
| M4 zero gameplay conversion/preparation | Focused lifecycle repair pass / final teardown refresh pending | Renderer | Exact CPU-on phase sampling exposed a missing Whispy mouth image at frame 1398: run 28 rejected, the owner aborted, and generic fallback converted 40 textures. The native owner now reuses the pre-GO resident first source image only when every other word of the 59-word key matches. Frames 1398–1405 and 3300–3307 retain 202 stage triangles, zero post-arm fallback, zero ten-class fence work, and zero premature teardown on exact ROM `426B821A...` | Keep the documented cosmetic source-frame approximation; repeat only the final natural teardown in release qualification |
| Lower HUD: FPS, timer, labels, stock, damage | Pass | Integration | User approved; lifecycle and Results clear hook pass | Keep |
| Countdown/3-2-1/GO top presentation | FIXED / bitmap OAM | Renderer + QA | Hybrid runtime submission was invisible despite valid counters; restored proven all-bitmap OBJ ownership shows the traffic light with 93,824 bytes prepared pregame and zero gameplay conversion/upload | Keep published bitmap path; hybrid remains lab-only |
| Dream Land BGM | Pass | Audio | Tyler reports the stage theme sounds normal. The exact source-derived initial 65,536-byte DS ring has peak 9,928 / RMS 2,283.623; the natural public-ROM recovery route observes the live BGM channel bit in Calico's ARM7-shared mask with clean 44.1 KB/s streaming and zero I/O/unsafe/overrun faults | Keep; repeat only in final lifecycle qualification |
| Required FGM and Mario/Fox voices | Crowd FIXED / one natural voice per fighter PASS | Audio | The 102,196-byte AOT pack adds source FoxSmash1 ID372 and MarioSmash2 ID430. The natural 820-frame recovery route triggered both, acquired/released two clean DS handles, reported zero play/load/stale/generation faults, and retained 202,256 bytes; ID626 remains user-confirmed | Keep the two representative cues; Tyler ear-checks them, while remaining variants and exact pitch automation stay open |
| Winner and Results BGM | Pass | Audio | Natural Fox winner 16 → Results 22; errors/overrun/cleanup zero, reserve 172,024 | Keep gate |
| Stable reserve / no corruption | Pass on current candidate | QA | Battle-only opening/static store is exactly bounded at 185,696 bytes; after adding two fighter cues, the natural CPU-on recovery route retains 202,256 bytes, still 71,184 above the 128 KiB floor | Keep exact harness lifetime bound; repeat only in final CPU-on lifecycle qualification |
| Focused/checkpoint verification | Focused gates pass / Boundary refresh pending | QA | Current candidate has exact profile-2 fighter/stage census, pause-angle containment, source-derived audio asset checks, a natural two-fighter-voice channel gate, and synchronized M2/M3 measurements; no stacked full regression was run | Run one Boundary only at the next release checkpoint; Current only if original launch changes |
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

Checkpoint verdict: the synchronized CPU-on ROM measures 1.415–1.618M active
P95 ticks across four material combat phases, 2.53–2.89 times the one-VBlank
budget. The exhausted M2/M3 bounds do not supply the missing 855K–1.058M ticks,
so a credible 60 FPS path by July 19 is not established. Approval is requested
for a stable 20 FPS presentation target; until then 60 FPS remains an explicit
unmet P1 target. KO/rebirth and Results realtime samples remain evidence gaps,
not reasons to delay this already-decisive feasibility verdict.

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
