# P1 Execution Board

Updated: 2026-07-17 23:45 Central

Deadline: 2026-07-19 23:59 Central

Boundary: `battle_playable_realtime`, mode 163

This is the only dynamic P1 queue. `HANDOFF.md` owns the restart surface,
`optimization/NATIVE_RENDERER_PLAN.md` owns the renderer contract,
`PERF_LEDGER.md` owns measurements, and `PORTING.md` is append-only history.

## Artifact Identity

Integrated user-facing candidate:

```text
smash64ds-battle-playable-hwtri.nds
14,655,488 bytes
SHA-256 DA8282BBBD9872DC29F7442CC6ED3E0029967A7AB1AA0E94F9EDBED172981F04
```

Laboratory profile-1 ROMs are evidence only and never replace this filename.

## Hardware reality (2026-07-17)

A manual retail-DS observation currently puts real-hardware throughput near
`0.75x` the local melonDS run, but that is a starting observation rather than
an admitted uniform multiplier. Task 10 separates CPU/ITCM, CPU/main-RAM, and
GX work so each owner can be projected from its matching on-device bench.

| Resource / bench | Fixed work | melonDS 1.1 ticks | Retail DS ticks | HW / melonDS multiplier |
|---|---|---:|---:|---:|
| CPU-ITCM / ALU-ITCM | 1,000,000 dependent chains, 32 adds each | 17,501,568 | TBD | TBD |
| CPU-mainRAM / MEM-THMB | 8,388,608 loads over 256 KiB | 33,557,632 | TBD | TBD |
| CPU-mainRAM / MEM-ARM | identical loads and buffer, ARM state | 37,752,448 | TBD | TBD |
| CPU-cache control / CACHE4K | same load count over 4 KiB | 33,565,696 | TBD | TBD |
| GX / GX-BRST | 10,000 immediate triangles, flush every 2,048 | 2,729,728 | TBD | TBD |
| CARD (optional) | no safe media-independent run admitted | not run | TBD | TBD |

Until the paired hardware run exists, use `840,000` melonDS ticks as the
provisional locked-30 hardware budget only for planning. It is not a promotion
or rejection gate and does not authorize changing gameplay semantics.

Hardware operator packet:

1. Copy `builds/build-task10-hardware-calibration/smash64ds-task10-hardware-calibration.nds`
   to the flashcart, boot it on retail hardware, wait for `COMPLETE`, and take
   one legible photo containing the five tick rows and the displayed Git hash.
2. Report the DS model, flashcart, loader/settings, and photographed values.
3. Run the profile-1 battle ROM in the same combat phase and photograph the
   half-second HUD sample (`UPD`, `DRW`, `ACT`, `LOOP`, `SLIP`, `GIT`).

`PRE`/`PRP`/`CMT` rows are populated only by the existing detailed renderer
phase profiler; the low-observer profile-1 ROM intentionally leaves them off.

## Phase Evidence

Do not fill this table with figures from a different ROM or phase. Pending is
more accurate than recycling cross-build samples.

| Match phase | Artifact class | ROM SHA-256 | Synchronized window | N | Active median / P95 | State |
|---|---|---|---|---:|---:|---|
| Countdown / GO | focused profile-1 | `2868DEC6...` | exact completed frames 438–445 | 8 | 1,559,616 / 1,560,448 | Task 6 Cut D sample; stage 488,992/489,344 |
| Early combat | focused profile-1 | `2868DEC6...` | exact completed frames 600–607 | 8 | 1,250,432 / 1,251,264 | Task 6 Cut D sample; draw+flush P50 1,245,664 |
| Mid combat / Whispy change | focused profile-1 | `426B821A...` | exact completed frames 1398–1405 | 8 | 1,616,000 / 1,617,920 | Same-ROM M4 lifecycle sample; worst measured material phase |
| Late combat | focused profile-1 | `426B821A...` | exact completed frames 3300–3307 | 8 | 1,240,832 / 1,414,912 | Same-ROM late sample; no M4 fallback |
| KO / rebirth | focused profile-1 | `32C957AD...` | natural KO frames 708–715; rebirth cross-check 730–737 | 8 + 8 | 1,261,344 / 1,524,864; rebirth 1,110,528 / 1,112,256 | Exact source-event gates; additive 16-triangle effect, stage/M4, and zero-fence contracts pass |
| Time Up / Results | isolated published-equivalent profile-0 | `9C35F4B3...` | natural one-minute expiry→Results | 1 lifecycle | n/a / n/a | Pass: 4,084/2,042 exact 2:1, Results 58.2 updates/s, one teardown, zero M4 fence work |

Profile-1 hard-checkpoint windows are O2-equivalent feasibility evidence, not
profile-0 release baselines. M2 detail and profile-2 forensic samples stay in
`PERF_LEDGER.md`.

## Lane Ownership

| Lane | State | Branch / worktree | Owned surface | Runner |
|---|---|---|---|---|
| Integration/release | July 17 Latest + one-minute pass | live tree | integrated battle ROM, countdown, effects, audio, two-ROM contract, and current terminal lifecycle | no runner active |
| Renderer implementation | Tasks 8-10 banked; Task 11 owns the renderer TU | isolated renderer worktree plus shared integration tree | Task 8 CUT A is the retained constant-depth painter path, Task 9 keeps stock-libgcc ITCM plus exact `fcmpeq`, and Task 10 supplies the hardware lab/HUD. Task 11 is the sole current renderer writer; Task 12 waits for that lane to clear | renderer-economy runner only |
| Gameplay + QA | Playtest review fixed / manual candidate retest pending | shared live tree / disjoint files | Down+A is source-fixed at the shared ClearAll seam. Human-P2 Fox completes all nine Down-Air callbacks and exits naturally; Mario completes the same focused route with eight live imported CPU updates. Latest then passes canonical mode 163, two-ROM publication, runtime, registry, renderer/ITCM, and visual gates. Countdown also remains fixed | no runner active |
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
| Locked-30 scheduler | Pass / slowdown published | Integration | Task 9 natural match: 4,084 updates / 2,042 presents, exact fixed-two pacing; phase rates 39.9/37.4/39.3/n.a./58.2 updates/s and slips 196/1088/946/0/3. Latest profile-0 smoke is 19.7 FPS. Focused natural KO/rebirth active P95 is 1,524,864/1,112,256 ticks | Keep exact 2:1 and conditional 59.0–61.0 gates; full-speed locked 30 remains unmet |
| Mario can damage Fox | Pass / continuous gate | Gameplay | User confirmed the repair manually. The focused current-ROM route now damages Fox 0→59, lets imported level-3 Recover return naturally to line 3, then damages Fox again 59→72 within 78 frames while all 11 damage colliders and global/special/star hit statuses remain normal | Keep the post-Recover assertion in `verify-battle-playable-fox-recovery.ps1` |
| Fireball spawn/render/damage/destruction | Pass | Gameplay | Source spawn/damage/lifetime and 40 moving visible textured hardware draws pass with zero rejects | Keep dedicated natural-input gate |
| Common visual effects | Natural common-hit pass / broader attack-hit audit open | Renderer + Gameplay | A natural mode-163 Mario contact reached BattleShip kick ID 32 at frame 125. Its following source hitlog selected DS `HitNormal` kind 1, created the normal/orb/spark trio with zero drops, and submitted 3/3 effects / 24 triangles with zero texture or draw rejects. Dated Dream Land capture `2026-07-17_220144-9409465_fox-recovery.png` passes nonblank green/detail/single-color gates. Fireball's separate focused route remains green | Keep the exact-event counter and image-content gate. Continue tracing the remaining reported attack visuals; common particle banks remain unresolved and this row is not globally closed |
| Fireball trajectory, floor rebound, and terminal | Pass | Gameplay | Current custom `0x47` submits 40/40 and first rebound is 55→46.75 at lifetime 122. The exact ELF destroy callsite then records the same weapon crossing Pupupu bottom −3500 at x/y/z −4650.5/−3504.8/−66.9 with lifetime 10; it is absent next frame. The deterministic source-MVP ROI changes 50→0 orange pixels | Keep the focused countdown/Fox-off gate and both `20260716_fireball-source-mvp-long-travel*.png` captures; the earlier far-left theory was false |
| Damage/throw map collision | P1 Dream Land boundary pass | Gameplay | Five source up-smashes proved DamageFall recovery. A separate external-input route then used nine short source Walk/Dash/Run steps for one Mario catch/forward throw: Fox took 0→12%, released status 169→186, swept/clamped to line 3, DownBounced once, cleared every catch link, and retained 202,256 bytes. Source Dream Land contains one static collision group: 4 floors, 1 ceiling, 1 right wall, and 1 left wall | Keep both sparse gates; throw evidence is `2026-07-16_063512-7696185_throw-release-recovery-p21764.png`. Moving-stage/project providers are P2 and must not delay this static-stage P1 |
| One-way platform semantics | Pass / hardened natural gate | Gameplay | Current-ROM mode 163 completed 715 natural frames: six ordered continued-ascent/strict-descent/downward-crossing flights (`0x3f`), all three platform masks `0x7`, two side cycles, three exact ignore-line Pass crossings, nine landings, and 214,544-byte reserve | Keep the focused gate; screenshot `2026-07-16_052652-7356809_platform-semantics-p984.png` |
| Edge behavior and specials | Retest | Gameplay | Up-B was manually accepted; other edge behavior needs current-ROM qualification | No unrelated behavior change without reproduction |
| Down+A CPU-stall report | FIXED / source-exact shared clear + three runtime gates | Gameplay + QA | BattleShip ClearAll preserves reusable collider payload; the port shim incorrectly erased it before Fox's scripted one-tick-later Refresh. The shim now clears only `attack_state` plus `is_attack_active`, and the existing damage-common probe requires seeded damage 7 to survive. Human-P2 Fox completes 9 callbacks and exits to status 31 with logic/cpu/reads 8/0/12, 116,992 update ticks, and 205,744-byte reserve. Mario exits to status 26 with 8/8/12, 134,784 ticks, and 203,536-byte reserve. Both routes load their exact `0x303`/`0x272` source assets. CPU-on Current passes in 661.5 seconds with the two-ROM contract and mode-163 CPU setup/proc/target 1/33/33 | Keep `verify-battle-playable-down-air-stall.ps1` for both actors and the corrected payload-preservation assertion. Do not restore the compatibility shim's non-source field clearing |
| Normal-play stage painter/depth order | FIXED / pixel + profile-2 pass | Renderer + QA | BattleShip layer modes classify 66 source-Z and 126 no-Z triangles; one full v16 step per no-Z triangle removes the grass/bush overlap, preserves 202 triangles, and reserves disjoint endpoint bands | Keep as correctness fix; final frame 438/501 captures and zero-collision trace are authoritative |
| Pause-orbit geometry containment | FIXED / user confirmed | Renderer + QA | Clip-space near-plane containment removes screen-blocking triangles at the breaking orbit angles without changing normal-play profile-2 output | Keep focused angle gate; paused −33.6° is also the strongest Mario underside view |
| Mario pant/underside visual | FIXED / user confirmed | Renderer + QA | Source root light preambles were missing; replay restores blue right pant and closed underside with unchanged 320-triangle census | Tyler accepted `20260716-034036_slot3_p10612_mode163_camera_pause_minus33p6.png` on 2026-07-16 |
| Natural Fox recovery | Pass | Gameplay | Current-ROM mode 163 used only external Mario input: Fox took 0→59 damage, selected BattleShip Recover for 40 frames at offstage x=2379.905, grounded on line 3 at x=1336.084, and took a later hit to 72 without KO/rebirth in 897 frames | Keep focused gate; reserve 202,256 and screenshot `20260716_fox-recovery-post-hit.png` |
| Cut G M1 affine BG2, 5–35K ticks | Pass | Renderer | 1,856/1,856 ticks; exact frames 438/439 pass and publish | Keep canonical |
| M2 Mario/Fox AOT, 170–250K ticks | Incremental compute KEEP at 385.3K | Renderer | Current ledger-off Mode 8 is 385,312/388,480 after exact light-state restoration. The raw texture-class split moves the synchronized current-build combined fighter 386,624/389,824 → 385,312/388,480, draw 1,011,648/1,014,976 → 1,009,824/1,013,120, and active 1,015,680/1,018,880 → 1,014,048/1,017,088. The corpus proves 43 untextured and 11 textured raw calls, so the untextured callee removes 172 main-RAM stack word transfers per frame. Exact 70/686, 60/320/306/29/0/0, all sampled non-timing fields, and 0/49,152 changed pixels remain | Keep the scalar reset, raw-corner representation, and two raw texture-class callees. ITCM is 28,052/32,768; full inlining, tail dispatch, and shared camera hoist remain rejected. The 170–250K milestone is still red and directional |
| Mode-8 generic/fast forensic parity | PASS / source-exact light state | Renderer | F3DEX2 `G_MOVEWORD` decodes/emits index bits 16..23 and offset bits 0..15. The exact O2R census is 148 fighter `G_MW_LIGHTCOL` commands: 120 compact root preambles plus 28 intra-root changes carried by epoch state spans. Current profile-2 ROM `75A731E1...` frames 180–187 has 0 semantic, owner, or geometry mismatches and retains 686 triangles in both arms | Keep the exact 120/28 representation and validator gate. Evidence: `2026-07-17_m2-raw-texture-split-{generic,fast}-profile2.json` |
| M3 complete stage AOT, 150–250K ticks | Task 6 accumulated KEEP at 489K / STOP | Renderer | R0→Cut D moves combat stage 539,616/539,904 → 489,184/489,536 and draw+flush P50 1,297,056 → 1,245,664. Cut C and Cut D each preserve exact 121/828, 8/255/57/42/54/202/49/4, cross 5/10/15, zero fallback/fence/conservation, and 0/49,152 native pixels | Bank both gains. The remaining 455,664-tick gap to ~790K is required work or lies behind forbidden packet/order/poly/translucency boundaries; do not manufacture another cut from nested profiler noise |
| Task 8 CUT A constant-depth GX painter | PASS / already canonical | Renderer | The requested mechanism had already landed at the pause/depth root-cause fix: each no-Z band loads a matrix whose Z row is the exact band constant times W, so the GX divider produces constant post-divide depth without the retired CPU `div64` projector. The subsequent exact AOT-shift and zero-shift specializations saved another 22,016/22,208 and 14,304/14,080 stage ticks | Reconcile as completed; do not add a second constant-depth path. Retain the depth trace, 121/828 census, zero fallback/fence/conservation, and pixel gates |
| Task 9 bit-exact soft-float | PASS / Phase 1 16.54%, Phase 2 8.86% owner gains | Renderer + Gameplay | Phase 1 keeps six byte-identical GCC 15.2.0 objects (1,952 ITCM bytes). Phase 2 replaces only the 556.25-calls/update `__aeabi_fcmpeq` with a 36-byte, nine-instruction ARM leaf: source-update 236,640/238,016 → 215,680/217,024 P50/P95. Host proof covers 16,777,216 directed plus 100,000,000 deterministic random pairs; the standalone ARM9 ROM covers 2,916 directed pairs; 3,892 full-state rows match exactly | Keep both phases. ITCM is 28,088/32,768 with 4,680 bytes free. The renamed stock helper remains the linked golden only in Phase 2; Phase 1 proves it absent. Main-RAM coroutine stack finding remains report-only |
| M4 zero gameplay conversion/preparation | Latest startup + lifecycle pass | Renderer | Latest canonical frame 212 retains 22 textures / 131,072 bytes, records 646 cache hits, zero hot conversion/upload, and all fence counts zero. The Task 9 one-minute proof reaches Results with one normal teardown and zero post-GO fence work | Keep the 57,344-byte total overlay texture layout |
| Lower HUD: FPS, timer, labels, stock, damage | Pass | Integration | User approved; lifecycle and Results clear hook pass | Keep |
| Countdown/3-2-1/GO top presentation | FIXED / full runtime gate pass | Renderer + QA | The source-backed point sample changes 49 atlas pixels only inside the 12x9 `ShadowGo`; all five GO frames retain its 70-pixel count plus 10/10/10 draws/queued/emitted, 57,344 texture bytes, 608 palette bytes, and zero hot conversion/upload. The large-GO mismatch was a stale crop lock after the source-light repair: only 125/26,400 pixels changed, all inside Mario's 22x14 area, while the GO RGB555 payload remained `05330f47...`. The rebuilt full verifier passes with crop `d968b0cc...`, GO `3 OBJ + 10 quads`, and 31,168 OBJ bytes | Keep both accepted crop locks and the source-derived DS assets; no GO source change was warranted |
| Dream Land BGM | Pass | Audio | Tyler reports the stage theme sounds normal. The exact source-derived initial 65,536-byte DS ring has peak 9,928 / RMS 2,283.623; the natural public-ROM recovery route observes the live BGM channel bit in Calico's ARM7-shared mask with clean 44.1 KB/s streaming and zero I/O/unsafe/overrun faults | Keep; repeat only in final lifecycle qualification |
| Required FGM, attack/hit sounds, and Mario/Fox voices | Six common channel starts restored / acoustic and broader audit open | Audio + Gameplay | The source table `dFTMainHitCollisionFGMs` exposed an intentional DS exclusion: natural kick ID 32 produced unsupported delta 1 and no channel. The 121,720-byte pack now maps 18 exact IDs plus punch/kick IDs 40/38/37/34/32/31 from their two exact primary BattleShip samples and source frequency/volume envelopes; omitted composite forks/custom FX remain explicit fidelity debt. The repeated natural ID-32 route records pan 80, supported/unsupported/acquire `1/0/1`, a live channel mask, zero playback faults, and 187,152-byte reserve. Automated channel ACK is not an ear check; five special/projectile contacts remain fail-closed | Keep the six common mappings and natural channel/effect gate. Tyler's exact-ROM ear retest remains required. Next resolve the naturally observed unsupported Escape 11, Grind4 85, MarioFoot 110, MarioDash 121, swing 41/42, GuardOn 13, Fox special 186/189, MarioSmash3 431, and FoxDamage 375 cues; requalify the one-minute reserve before adding more resident audio |
| Winner and Results BGM | Pass | Audio | Natural Fox winner 16 → Results 22; errors/overrun/cleanup zero, reserve 172,024 | Keep gate |
| Stable reserve / no corruption | Focused pass / one-minute post-pack recheck pending | QA | The natural attack/hit route retains 187,152 bytes after loading the expanded 121,720-byte FGM pack, 56,080 above the 128 KiB floor. The prior full expiry→Results reserve belongs to the smaller 107,536-byte pack and cannot close the new candidate | Run the CPU-on one-minute lifecycle/reserve gate before further pack growth or release; do not infer the full-match floor from the focused route |
| Focused/checkpoint verification | Latest PASS / natural attack-hit channel/effect PASS | QA | Published battle candidate `1409F7CB...` passes the 22:04 Latest checkpoint in 194 seconds with normal runtime, mode-163 CPU setup/proc/target 1/33/33, GBI fixtures, two-ROM publication, 28,052-byte ITCM placement, zero renderer rejects, and dated visual analysis. The repeated focused route proves exact natural ID-32 channel/effect output and 187,152-byte reserve | Keep this development checkpoint. Full one-minute post-pack reserve, exact-ROM ear check, and the remaining cue audit are the restart packet |
| Cut G capture / final dated capture / manual retest | Automated exactness pass / manual current-ROM retest pending | QA + user | Latest capture is `2026-07-17_canonical_fast_220437-8681451-p13676.png`; its paired frame has 788/49,152 meaningful changes, 100% overlap, and all named-region/detail gates pass. Task 6 C/D, Task 8 G2, reserve repair, Task 9 state identity, source-light parity, both emitter splits, raw-corner cut, Down+A, and the common-contact checkpoint remain retained | Manually retest exact ROM `1409F7CB...`; automated common-contact A/V closure is focused, not global |

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
one-minute run closes the Results lifecycle gap. Its older pacing classifier
watched proof-route counters and therefore reported zero KO/rebirth presents
despite a real KO FGM triplet. The focused profile-1 sampler now gates on
BattleShip's natural KO trace and `ftCommonRebirthDownSetStatus`: KO frames
708–715 are the worse 1,261,344/1,524,864-tick active window, while rebirth
frames 730–737 measure 1,110,528/1,112,256. This closes the missing phase
baseline, not the full-speed requirement.

### July 17

- Architecture and integration freeze. Merge only measured candidates and
  confirmed blocker fixes. Run Latest instead of Boundary when shared runtime
  or normal launch changes.

### July 18

- Release qualification: one-minute soak, focused checks, one Boundary or
  Latest run, clean canonical rebuild/parity, exact-ROM manual retest, and
  dated capture set.
- Release-candidate fixes and verification only. No new architecture and no P2
  work. Report every remaining red acceptance row honestly.

### July 19
- Get ready for public Github release
- Public repo must be able to build current nds rom with legal user supplied n64 ROM with one .ps1 script.


## Integration Rule

Performance iteration is eight synchronized A frames and eight B frames with
ticks, FPS, screenshots/image analysis, and cheap correctness counters. Add A2
only when inconclusive. Compilation alone never closes a row. P2 begins only
after every required P1 row is green, documented, and snapshotted.
