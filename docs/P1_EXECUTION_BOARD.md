# P1 Execution Board

Updated: 2026-07-17 01:16 Central

Deadline: 2026-07-19 23:59 Central

Boundary: `battle_playable_realtime`, mode 163

This is the only dynamic P1 queue. `HANDOFF.md` owns the restart surface,
`optimization/NATIVE_RENDERER_PLAN.md` owns the renderer contract,
`PERF_LEDGER.md` owns measurements, and `PORTING.md` is append-only history.

## Artifact Identity

Integrated user-facing candidate:

```text
smash64ds-battle-playable-hwtri.nds
14,612,480 bytes
SHA-256 83E41C1BB8E04155D71F7AB9BF849AB3DC58618F16CE52BEC5F39813EC680EEA
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
| Integration/release | July 17 Boundary + one-minute pass | live tree | integrated battle ROM, countdown, effects, audio, two-ROM contract, and current terminal lifecycle | no runner active |
| Renderer implementation | Task 8 complete; Task 9 next | shared live tree / focused lab builds | E/F/G2 are banked; H was not measured, and the update-pair draw audit found no duplicate display work | no runner active |
| Gameplay + QA | Integrated / manual review pending | shared live tree / disjoint files | source effects and audio mappings verified; exact-ROM visual/acoustic eyeball remains | no runner active |
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
| Common visual effects | Focused pass | Renderer + Gameplay | Natural Fireball rebound created 3 bounded source effects; 13 hardware submissions emitted 96 triangles with kind mask `0x45`, zero texture/reject faults, and 176,464-byte reserve | Keep the imported effect manager and focused `-VisualEffectsOnly` gate |
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
| M4 zero gameplay conversion/preparation | Current startup pass / lifecycle refresh pending | Renderer | Compacting the two source countdown atlases from 65,536 to 49,152 bytes restores pre-GO source-frame residency without transition deletion. Current canonical frame 212 retains 22 textures / 131,072 bytes, records 646 cache hits, zero hot conversion/upload, and all fence counts zero | Keep the 57,344-byte total overlay texture layout; rerun terminal lifecycle with the release gate |
| Lower HUD: FPS, timer, labels, stock, damage | Pass | Integration | User approved; lifecycle and Results clear hook pass | Keep |
| Countdown/3-2-1/GO top presentation | FIXED / source-derived hybrid | Renderer + QA | O2R decode reverses the original odd-row texture interleave. Big GO is prepare-time RGB555+A1 OAM; the opaque shaded traffic box is A3I5; only its foreground flare is A5I3. Runtime proof prepares 31,168 OBJ + 57,344 texture + 608 palette bytes, emits GO as 3 OBJ + 10 quads, and performs zero hot conversion/upload | Keep the prepare-once path and source pixels; no runtime antialiasing |
| Dream Land BGM | Pass | Audio | Tyler reports the stage theme sounds normal. The exact source-derived initial 65,536-byte DS ring has peak 9,928 / RMS 2,283.623; the natural public-ROM recovery route observes the live BGM channel bit in Calico's ARM7-shared mask with clean 44.1 KB/s streaming and zero I/O/unsafe/overrun faults | Keep; repeat only in final lifecycle qualification |
| Required FGM, attack/hit sounds, and Mario/Fox voices | Focused source-behavior pass | Audio | The 107,536-byte AOT pack maps 18 exact source IDs / 16 unique samples plus 11 collision cues. Natural qualification observed 14 plays, 21 envelope steps, max 3 live handles, and 174,864-byte headroom; ID626 remains user-confirmed. Continuous-pitch IDs 429/435 and fork voice 685 remain explicit debt | Keep exact event mapping/behavior fixtures and natural gates; finish acoustic ear checks |
| Winner and Results BGM | Pass | Audio | Natural Fox winner 16 → Results 22; errors/overrun/cleanup zero, reserve 172,024 | Keep gate |
| Stable reserve / no corruption | Pass on current candidate | QA | Current CPU-on expiry→Results retains 203,536 bytes before the 65,536-byte BGM ring, leaving 138,000 bytes after BGM and 6,928 bytes above the 128 KiB floor | Keep the serial display-scratch lifetime bound and one-minute reserve gate |
| Focused/checkpoint verification | Integrated Boundary + focused + lifecycle PASS | QA | Published candidate `162212F4...` passes Boundary at frame 212, source countdown, visual-effects, full FGM phase-pack, and current CPU-on expiry→Results; ITCM is 26,176/32,768 | Tyler's exact-ROM visual/acoustic eyeball remains the release gate |
| Cut G capture / final dated capture / manual retest | Automated exactness pass / manual current-ROM retest pending | QA + user | Task 6 C/D, Task 8 G2, and the reserve repair preserve deterministic native output at 0/49,152 changed pixels; current Boundary capture is dated | Tyler still owes the requested 30-second eyeball of exact ROM `162212F4...` |

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
