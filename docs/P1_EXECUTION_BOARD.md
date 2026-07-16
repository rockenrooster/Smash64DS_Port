# P1 Execution Board

Updated: 2026-07-16 14:30 Central

Deadline: 2026-07-19 23:59 Central

Boundary: `battle_playable_realtime`, mode 163

This is the only dynamic P1 queue. `HANDOFF.md` owns the restart surface,
`optimization/NATIVE_RENDERER_PLAN.md` owns the renderer contract,
`PERF_LEDGER.md` owns measurements, and `PORTING.md` is append-only history.

## Artifact Identity

Task-6-qualified user-facing candidate:

```text
smash64ds-battle-playable-hwtri.nds
14,586,880 bytes
SHA-256 7AB28684930899D5A4F5165E1CE85DDA7A93FC7F3CB06D44062283536507BFAD
```

Laboratory profile-1 ROMs are evidence only and never replace this filename.

## Phase Evidence

Do not fill this table with figures from a different ROM or phase. Pending is
more accurate than recycling cross-build samples.

| Match phase | Artifact class | ROM SHA-256 | Synchronized window | N | Active median / P95 | State |
|---|---|---|---|---:|---:|---|
| Countdown / GO | focused profile-1 | `2868DEC6...` | exact completed frames 438–445 | 8 | 1,559,616 / 1,560,448 | Task 6 Cut D sample; stage 488,992/489,344 |
| Early combat | focused profile-1 | `2868DEC6...` | exact completed frames 600–607 | 8 | 1,250,432 / 1,251,264 | Task 6 Cut D sample; draw+flush P50 1,245,664 |
| Mid combat / Whispy change | focused profile-1 | `426B821A...` | exact completed frames 1398–1405 | 8 | 1,616,000 / 1,617,920 | Same-ROM M4 lifecycle sample; worst measured material phase |
| Late combat | focused profile-1 | `426B821A...` | exact completed frames 3300–3307 | 8 | 1,240,832 / 1,414,912 | Same-ROM late sample; no M4 fallback |
| KO / rebirth | canonical-profile-0 | pending candidate republication | Pending natural KO→rebirth window | — | — / — | Pending synchronized canonical baseline |
| Time Up / Results | canonical profile-0 | `7AB28684...` | natural one-minute expiry→Results | 1 lifecycle | n/a / n/a | Pass: 4,084/2,042 exact 2:1, Results 58.2 updates/s, one teardown, zero M4 fence work |

Profile-1 hard-checkpoint windows are O2-equivalent feasibility evidence, not
profile-0 release baselines. M2 detail and profile-2 forensic samples stay in
`PERF_LEDGER.md`.

## Lane Ownership

| Lane | State | Branch / worktree | Owned surface | Runner |
|---|---|---|---|---|
| Integration/release | Task 6 runtime gates pass | live tree | canonical battle ROM, locked-30 smoke, natural one-minute match, two-ROM contract | no runner active |
| Renderer implementation | Task 6 KEEP / STOP | shared live tree / focused lab builds | Phase 0, Cut C first-use preparation, and Cut D valid-color seam retained; forbidden transport/semantic surfaces remain untouched | no runner active |
| Gameplay + QA | Paused | shared live tree / disjoint files | sparse DamageFall runtime gate | no runner active |
| Performance research | Measured cuts accumulate | shared live tree / read-only | Milestone targets no longer discard smaller correct gains; measured regressions and invalid visual packets remain rejected | no runner active |

Runner/window/port policy lives in `VERIFYING.md`; scripted launches normalize
the selected TOML and never touch the user's manual instance.
Workers edit only explicitly assigned disjoint surfaces; the integration lane
owns current-truth docs, shared-file arbitration, commits, and publication.


## Acceptance Matrix

| Acceptance condition | State | Lane | Blocker / evidence | Integration decision |
|---|---|---|---|---|
| Mario human versus original level-3 Fox CPU, Dream Land, one-minute Time match, items off | CPU-on public default / visible fast-iteration override | Gameplay + QA | The published ROM and source runtime gates retain flag `1`; the separate `-FastIteration` screenshot launch selects flag `0` before battle, skipping countdown/Fox and freezing `1:00` | Keep lifecycle gates on `1`; use the focused flag-`0` capture during routine visual iteration |
| Original Wait → countdown → GO control/timer gate | Pass | Integration | Automated synchronized source-state proof passes | Keep |
| Locked-30 scheduler | Pass / slowdown published | Integration | Canonical Task 6 match: 4,084 updates / 2,042 presents, exact fixed-two pacing; phase rates 39.9/38.1/39.6/n.a./58.2 updates/s and slips 196/1036/925/0/3. The smoke reports 19.5 presents/s and 38.9 updates/s | Keep exact 2:1 and conditional 59.0–61.0 gates; full-speed locked 30 remains unmet |
| Mario can damage Fox | Pass / continuous gate | Gameplay | User confirmed the repair manually. The focused current-ROM route now damages Fox 0→59, lets imported level-3 Recover return naturally to line 3, then damages Fox again 59→72 within 78 frames while all 11 damage colliders and global/special/star hit statuses remain normal | Keep the post-Recover assertion in `verify-battle-playable-fox-recovery.ps1` |
| Fireball spawn/render/damage/destruction | Pass | Gameplay | Source spawn/damage/lifetime and 40 moving visible textured hardware draws pass with zero rejects | Keep dedicated natural-input gate |
| Fireball trajectory, floor rebound, and terminal | Pass | Gameplay | Current custom `0x47` submits 40/40 and first rebound is 55→46.75 at lifetime 122. The exact ELF destroy callsite then records the same weapon crossing Pupupu bottom −3500 at x/y/z −4650.5/−3504.8/−66.9 with lifetime 10; it is absent next frame. The deterministic source-MVP ROI changes 50→0 orange pixels | Keep the focused countdown/Fox-off gate and both `20260716_fireball-source-mvp-long-travel*.png` captures; the earlier far-left theory was false |
| Damage/throw map collision | P1 Dream Land boundary pass | Gameplay | Five source up-smashes proved DamageFall recovery. A separate external-input route then used nine short source Walk/Dash/Run steps for one Mario catch/forward throw: Fox took 0→12%, released status 169→186, swept/clamped to line 3, DownBounced once, cleared every catch link, and retained 202,256 bytes. Source Dream Land contains one static collision group: 4 floors, 1 ceiling, 1 right wall, and 1 left wall | Keep both sparse gates; throw evidence is `2026-07-16_063512-7696185_throw-release-recovery-p21764.png`. Moving-stage/project providers are P2 and must not delay this static-stage P1 |
| One-way platform semantics | Pass / hardened natural gate | Gameplay | Current-ROM mode 163 completed 715 natural frames: six ordered continued-ascent/strict-descent/downward-crossing flights (`0x3f`), all three platform masks `0x7`, two side cycles, three exact ignore-line Pass crossings, nine landings, and 214,544-byte reserve | Keep the focused gate; screenshot `2026-07-16_052652-7356809_platform-semantics-p984.png` |
| Edge behavior and specials | Retest | Gameplay | Up-B was manually accepted; other edge behavior needs current-ROM qualification | No unrelated behavior change without reproduction |
| Normal-play stage painter/depth order | FIXED / pixel + profile-2 pass | Renderer + QA | BattleShip layer modes classify 66 source-Z and 126 no-Z triangles; one full v16 step per no-Z triangle removes the grass/bush overlap, preserves 202 triangles, and reserves disjoint endpoint bands | Keep as correctness fix; final frame 438/501 captures and zero-collision trace are authoritative |
| Pause-orbit geometry containment | FIXED / user confirmed | Renderer + QA | Clip-space near-plane containment removes screen-blocking triangles at the breaking orbit angles without changing normal-play profile-2 output | Keep focused angle gate; paused −33.6° is also the strongest Mario underside view |
| Mario pant/underside visual | FIXED / user confirmed | Renderer + QA | Source root light preambles were missing; replay restores blue right pant and closed underside with unchanged 320-triangle census | Tyler accepted `20260716-034036_slot3_p10612_mode163_camera_pause_minus33p6.png` on 2026-07-16 |
| Natural Fox recovery | Pass | Gameplay | Current-ROM mode 163 used only external Mario input: Fox took 0→59 damage, selected BattleShip Recover for 40 frames at offstage x=2379.905, grounded on line 3 at x=1336.084, and took a later hit to 72 without KO/rebirth in 897 frames | Keep focused gate; reserve 202,256 and screenshot `20260716_fox-recovery-post-hit.png` |
| Cut G M1 affine BG2, 5–35K ticks | Pass | Renderer | 1,856/1,856 ticks; exact frames 438/439 pass and publish | Keep canonical |
| M2 Mario/Fox AOT, 170–250K ticks | Incremental compute KEEP at 384.0K | Renderer | Current Mode-8 A/B moves combined fighter P50/P95 386,880/386,944 → 384,000/384,000 by co-locating the prepared color/UV and immutable AOT GX words in one 16-byte output record. Exact 70/686 and 60/320/306/29/0/0 remain, conservation is zero, and the native top-screen image is byte-identical | Keep ITCM, exact quantization, raw-loop specialization, batched accounting, AOT GX coordinates, and the output-local record; lighting is already a bare exact LUT path. Continue the measured production emit path. The 170–250K milestone is directional, not an intermediate discard gate |
| M3 complete stage AOT, 150–250K ticks | Task 6 accumulated KEEP at 489K / STOP | Renderer | R0→Cut D moves combat stage 539,616/539,904 → 489,184/489,536 and draw+flush P50 1,297,056 → 1,245,664. Cut C and Cut D each preserve exact 121/828, 8/255/57/42/54/202/49/4, cross 5/10/15, zero fallback/fence/conservation, and 0/49,152 native pixels | Bank both gains. The remaining 455,664-tick gap to ~790K is required work or lies behind forbidden packet/order/poly/translucency boundaries; do not manufacture another cut from nested profiler noise |
| M4 zero gameplay conversion/preparation | Lifecycle pass / teardown refreshed | Renderer | Exact CPU-on phase sampling exposed and repaired the Whispy mouth-image fallback. The canonical Task 6 one-minute match retains 22 textures / 131,072 bytes, reports exactly one normal teardown, and keeps all ten post-GO fence counts zero | Keep the documented cosmetic source-frame approximation and terminal lifecycle gate |
| Lower HUD: FPS, timer, labels, stock, damage | Pass | Integration | User approved; lifecycle and Results clear hook pass | Keep |
| Countdown/3-2-1/GO top presentation | FIXED / bitmap OAM | Renderer + QA | Hybrid runtime submission was invisible despite valid counters; restored proven all-bitmap OBJ ownership shows the traffic light with 93,824 bytes prepared pregame and zero gameplay conversion/upload | Keep published bitmap path; hybrid remains lab-only |
| Dream Land BGM | Pass | Audio | Tyler reports the stage theme sounds normal. The exact source-derived initial 65,536-byte DS ring has peak 9,928 / RMS 2,283.623; the natural public-ROM recovery route observes the live BGM channel bit in Calico's ARM7-shared mask with clean 44.1 KB/s streaming and zero I/O/unsafe/overrun faults | Keep; repeat only in final lifecycle qualification |
| Required FGM and Mario/Fox voices | Crowd FIXED / one natural voice per fighter PASS | Audio | The 102,196-byte AOT pack adds source FoxSmash1 ID372 and MarioSmash2 ID430. The natural 820-frame recovery route triggered both, acquired/released two clean DS handles, reported zero play/load/stale/generation faults, and retained 202,256 bytes; ID626 remains user-confirmed | Keep the two representative cues; Tyler ear-checks them, while remaining variants and exact pitch automation stay open |
| Winner and Results BGM | Pass | Audio | Natural Fox winner 16 → Results 22; errors/overrun/cleanup zero, reserve 172,024 | Keep gate |
| Stable reserve / no corruption | Pass on current candidate | QA | Canonical Task 6 CPU-on expiry→Results retains 206,352 bytes before the 65,536-byte resident BGM ring, leaving 140,816 bytes after BGM and 9,744 bytes above the 128 KiB floor | Keep exact harness lifetime bound and one-minute reserve gate |
| Focused/checkpoint verification | Task 6 gates PASS / Boundary refresh pending | QA | Published ROM `7AB28684...` passes ITCM 26,104/32,768, locked-30 smoke, natural one-minute lifecycle, and the two-ROM contract. The generic ROM remains exact SHA `009211A8...` | A later release-wide Boundary/Current run remains outside Task 6; do not claim it from narrower gates |
| Cut G capture / final dated capture / manual retest | Automated exactness pass / manual current-ROM retest pending | QA + user | Task 6 Cut C and Cut D each pass deterministic native 256x192 comparison at 0/49,152 changed pixels; canonical lifecycle is complete | Tyler still owes the requested 30-second eyeball of exact ROM `7AB28684...` |

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

Checkpoint decision: presentation targets locked 30; 60 FPS is not claimed.
The scheduler uses exactly two source updates per present and never catches up
after a slip. This matches Smash 64's uniform slowdown under load, preserves
real-time hardware-paced audio, avoids 2/3-tick motion judder, and self-recovers
after one slow frame. Old one-update-per-present and debt/cap-4 samples are not
comparable; all phase baselines require fixed-two resampling. The natural
one-minute run closes the Results lifecycle gap; its KO/rebirth bucket had zero
presents, so a focused synchronized KO/rebirth phase sample remains pending.

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
