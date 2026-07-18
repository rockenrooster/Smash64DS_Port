# P1 Execution Board

Updated: 2026-07-18 13:55 Central

Deadline: 2026-07-19 23:59 Central

Boundary: `battle_playable_realtime`, mode 163

This is the only dynamic P1 queue. `HANDOFF.md` owns the restart surface,
`optimization/NATIVE_RENDERER_PLAN.md` owns the renderer contract,
`PERF_LEDGER.md` owns measurements, and `PORTING.md` is append-only history.

## Artifact Identity

Integrated user-facing candidate:

```text
smash64ds-battle-playable-hwtri.nds
14,669,824 bytes
SHA-256 F3252C5C1A08D89FA040313D5DFEA5F3862A6A80020E8CE3D641E215B3AA1B0B
```

Laboratory profile-1 ROMs are evidence only and never replace this filename.

## Hardware reality (2026-07-18)

A manual retail-DS observation currently puts real-hardware throughput near
`0.75x` the local melonDS run, but that is a starting observation rather than
an admitted uniform multiplier. Task 10 separates CPU/ITCM, CPU/main-RAM, and
GX work so each owner can be projected from its matching on-device bench.

The first same-workload retail baseline is now authoritative: the mode-163 ARM
control photographed `UPD 374,464`, `DRW 1,743,296`, `ACT 1,745,984`,
`LOOP 2,240,384`, `FPS 13.5`, and `SLIP 0`. That is roughly four VBlanks per
update and 13.5-15 FPS in the observed heavy-combat phase. Against its matching
melonDS packet, draw is about x1.51 and update about x1.73 on device. A credible
20 FPS draw budget is below 1,680,570 device ticks, so this sample still needs
roughly another 100K repeatable device draw ticks of margin. The next admitted
levers are the now-proved update locality cut and a measured DTCM audit after
the demo; emulator-only cache projections and GX FIFO DMA stay out of the queue.

The ARM-base Task 19 pair now confirms the update-only placement at the same
`00:58` phase: UPD 366,016→363,456 (-2,560/-0.70%) and DRW
1,699,328→1,696,640 (-2,688/-0.16%) cross LOOP 2,800,832→2,240,448
(five to four VBlanks) and FPS 13.9→14.3, with SLIP 0. ACT rises 15,872
(+0.93%). Keep the threshold win, but do not project the earlier Thumb-base
44,160-tick UPD result onto ARM or call every owner row improved.

| Resource / bench | Fixed work | melonDS 1.1 ticks | Retail DS ticks | HW / melonDS multiplier |
|---|---|---:|---:|---:|
| CPU-ITCM / ALU-ITCM | 1,000,000 dependent chains, 32 adds each | 17,501,568 | 18,002,048 | 1.03 |
| CPU-mainRAM / MEM-THMB | 8,388,608 loads over 256 KiB | 33,557,632 | 50,170,432 | 1.50 |
| CPU-mainRAM / MEM-ARM | identical loads and buffer, ARM state | 37,752,448 | 45,362,752 | 1.20 |
| CPU-cache control / CACHE4K | same load count over 4 KiB | 33,565,696 | 29,676,224 | 0.88 |
| GX / GX-BRST | 10,000 immediate triangles, flush every 2,048 | 2,729,728 | 2,378,432 | 0.87 |
| CARD (optional) | no safe media-independent run admitted | not run | not run | — |

Retail column source: Tyler's device photo
`artifacts/visibility/2026-07-17_233353-8578811_task10-hardware-calibration_real_nds.jpg`
(claude-read 2026-07-18). Two photo digits were disambiguated by physics, not
guessed: ALU-ITCM must exceed the 16,777,216-bus-tick single-issue floor for
33,554,432 dependent adds, fixing the second digit to 8 (18,002,048); MEM-THMB
matches the CACHE4K hardware floor plus ~20 bus ticks per 32-byte line fill
(1,048,576 fills), fixing 50,170,432 over 58,170,432. MEM-ARM mid digits carry
about +-2% photo uncertainty; its ratio is 1.15-1.22 under any plausible read.

Calibrated findings (2026-07-18):

1. melonDS 1.1 does not model the ARM9 dcache: it charges the 4 KiB and
   256 KiB load loops identically (0.02% apart) while hardware runs the
   cache-resident loop 41% faster. Cache-locality wins are invisible in
   melonDS and must be measured on hardware (TASK 12 hardware-primary rule).
2. The hardware tax is streaming/miss-heavy main-RAM traffic: x1.50. This is
   the bucket most of our draw-prep and update CPU work lives in.
3. The ARM/Thumb ranking inverts between emulator and hardware: melonDS
   penalizes ARM code in main RAM continuously (no icache model); hardware
   runs the icache-resident ARM loop faster than the Thumb one. Hot,
   cache-resident functions prefer ARM; only the cold bulk favors Thumb.
   Neither bench measures large-footprint code streaming, so TASK 12's
   on-workload hardware A/B remains the decisive test.
4. GX transport is cheaper on hardware (x0.87): moving work onto the GX
   (TASK 8 CUT A style) is worth more on-device than melonDS credits, and
   GX FIFO DMA feeding is deprioritized (no hidden stall tax observed).

No emulator-to-device formula or universal multiplier is admissible. The rows
above describe only their exact microbench workloads; they may rank a matching
diagnostic, but they cannot synthesize a whole-frame device result or promote a
candidate. Emulator evidence remains authoritative for deterministic state,
semantic/GX traces, arithmetic, and pixels. Retail A/B is authoritative for
DTCM, ARM/Thumb, code/data/cache layout, generated-program footprint, direct
VRAM stores, DMA, GX FIFO behavior, and final pacing. The former `840,000`
melonDS planning figure is retired; stable 30 is decided by the device's actual
two-VBlank interval distribution and zero-slip lifecycle.

Hardware operator packet:

Lab ROM: `builds/build-task10-hardware-calibration/smash64ds-task10-hardware-calibration.nds`
(`04F32CA76045B821A5404D55F740808DF13B3A0BA2261CC8D7AD5DB3459D263C`).
Profile-1 HUD ROM: `builds/task10-hardware-packet/smash64ds-task10-phase-hud-profile1.nds`
(`37AD2C8A4C388F3969A52E048D4921B524F8F579516A6973944CE5A7D9E81B60`).

1. Copy `builds/build-task10-hardware-calibration/smash64ds-task10-hardware-calibration.nds`
   to the flashcart, boot it on retail hardware, wait for `COMPLETE`, and take
   one legible photo containing the five tick rows and the displayed Git hash.
2. Report the DS model, flashcart, loader/settings, and photographed values.
3. Run the profile-1 HUD ROM above in the same combat phase and photograph the
   half-second HUD sample (`UPD`, `DRW`, `ACT`, `LOOP`, `SLIP`, `GIT`).

`PRE`/`PRP`/`CMT` rows are populated only by the existing detailed renderer
phase profiler; the low-observer profile-1 ROM intentionally leaves them off.

## Tasks 20R-25R atomic reconciliation checkpoint

Live-unit capture began on branch `master`, worktree
`D:\Stuff\DevFolder\Smash64DS_Port`, HEAD
`458191bef147f1c6963b2f533c601f9f68fc7730`. The tree was intentionally dirty:
the user's 344-line
`docs/optimization/ClaudeFable5_JumpABC_Tasks_20260715_2326.md` change is
preserved, unedited, and outside this checkpoint; the user-supplied
`docs/optimization/tasks.md` is the new authoritative queue.

| Unit | Class / atomic sub-cut | Owned files or hunks | Checkpoint disposition |
|---|---|---|---|
| Task 20R | **B** — stack high-water and DTCM capacity census only | `Makefile`; `include/nds/nds_os.h`; `include/port/coroutine.h`; `src/port/coroutine.c`; `src/port/coroutine_arm.s`; `src/port/libultra_os.c`; Task-20 verifier/checker hunks | Preserve default-off measurement support. Earlier console output suggested gameplay/main HWM 13,044/3,700 and `NO_FIT`, but the legacy exports omitted every Task-20 row, so that deeper result is provisional rather than promoted evidence. The corrected startup-only recheck (`CA743BF9...` ROM, `9DE93ADA...` ELF, JSON `6173F456...`) exports the enabled flag, eight raw records, sampleCount 1, exact capacity/alignment/guards, and its shallow 252/2,832 HWM. It proves instrumentation/export safety, not gameplay-phase fit. Profile 0 exposes zero Task-20 symbols. No scratch or stack move exists; complete lifecycle/IRQ/escape proof remains for Task 20R after Task 25R. |
| Task 21R | **B** — 21A shade/color key census only | `include/nds/nds_renderer.h`; `src/nds/nds_renderer.c`; `src/port/reloc_backend_renderer_dl.c`; M2 verifier hunk | Preserve compile-gated production-loop census. Frames 438–445 found 16 exact resident hits / 392 epochs (4.08%; Mario 11, Fox 5), zero collisions, below the 20% cache gate. No cache, 21B, 21C, or 23R work began. 21C remains only Task 27's foundation. |
| Task 22R | **A** — one threshold-4 packed dirty-span writer | Task-22 hunks in `Makefile`, renderer/startup headers, benchmark/verifier/checker, diagnostics, `src/port/sprite_preview_backend.c`, and `src/port/taskman_seam.c` | **REVERT.** Early/Countdown/Whispy writer P50 improved, but natural-KO wallpaper P95 regressed 366,784 -> 370,944 (+4,160), over the +2,000 gate. Profile 2 recorded 136,192 map and 14,942,208 pixel checks with zero mismatches. Candidate and selector are removed; only neutral default-off span/store census remains. |
| Task 24 | **C** — the already evidence-cleared worktree batch | committed manifest/evidence, worktree administration, `AGENTS.md`, ledger/board | Manifest SHA-256 `62585F0AE38C8B2ED4E0DC855DE56102B046585C69A208A6624EC0C16A8AF1BB`; 225 unique files / 20,501,076 bytes copied and 292 destinations rehashed with zero failures. Exactly 17 cleared worktrees were removed; all 15 held/dirty worktrees, every branch, builds, logs, and telemetry remain. No more cleanup until a quiet slot. |
| Old Task 25 | Not live — completed immediately before reconciliation | historical phase-matrix evidence/tooling | Preserve as historical input only. It is not Task 25R and cannot set current priority. |

During the only renderer edit, `/root/task21_23_renderer` was the sole writer of
`src/nds/nds_renderer.c`; that lane is stopped. Focused PowerShell parsing, GBI
fixtures, the Task 20 startup-sample recheck, Task 21 census, Task 22 oracle,
post-revert census, and the final profile-0 Boundary gate are green. `DevFast`
is retired and is not a registry profile; no obsolete gate was revived. This
closes the Task 24 safety gate for the evidence-cleared batch.

## Stable-30 retail contract

The final profile-0 device gate requires exactly two unchanged source updates
per presentation, with no debt, catch-up, skipped logic, interpolation, or
speculative state. Across the complete canonical lifecycle it permits zero
presentation intervals of three or more VBlanks and zero pacing-slip events.
State, audio, lifecycle, geometry, material, texture, depth, and ownership must
remain exact; synchronized native top-screen delta must be `0/49,152`; net
reserve must be at least 128 KiB. Each phase publishes P50, P95, maximum, N,
the interval histogram, presentations/s, and source updates/s.

## Historical pre-Task-25R phase evidence

Every timed row below came from one profile-1 ROM with live Fox, production
incremental wallpaper, and eight samples. It repaired the older mixed-ROM
comparison, but it is not Task 25R: it lacks the matching profile-0 sibling,
maximums, complete owner rows, interval histogram, stable-30 deficit, and
artifact/code-layout pack. Values are retained as historical P50/P95 ARM9
ticks; `UPD` is the source-update owner, and residuals are draw/present/loop.

Same artifact: profile-1 ROM
`FB0704BA5E23782903A28429D58DD89C29DCF7E23131EBE270105ECCC978C144`,
ELF `A1811E43CFF97DC57EC1DB19DCE89CE4E0E36C50900E83857A371B0340F2CF68`.

| Match phase | ROM SHA-256 | Profile frames / source ticks | N | UPD | Draw | Stage | Mario | Fox | Wallpaper | Active | Loop | Residual D/P/L | State |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| Countdown / GO | `FB0704BA5E23782903A28429D58DD89C29DCF7E23131EBE270105ECCC978C144` | 438–445 / 484–498 | 8 | 199,776/200,896 | 1,148,416/1,216,576 | 465,984/466,112 | 169,440/171,200 | 208,960/209,344 | 286,112/351,296 | 1,153,248/1,221,376 | 1,680,448/2,240,640 | 19,616/19,968; 1,408/1,536; 1,600/1,728 | Exact natural countdown window |
| Early combat | `FB0704BA5E23782903A28429D58DD89C29DCF7E23131EBE270105ECCC978C144` | 600–607 / 808–822 | 8 | 144,480/145,920 | 1,063,232/1,085,824 | 466,112/466,240 | 9,920/9,984 | 196,800/196,928 | 340,544/362,880 | 1,068,032/1,090,624 | 1,680,448/1,680,512 | 50,048/50,304; 1,376/1,536; 1,600/1,728 | Mario naturally off-screen; fast owners remain exact |
| Mid combat / Whispy | `FB0704BA5E23782903A28429D58DD89C29DCF7E23131EBE270105ECCC978C144` | 1398–1405 / 2404–2418 | 8 | 214,944/814,080 | 1,183,328/1,221,312 | 466,016/466,048 | 169,504/171,264 | 209,184/209,728 | 320,416/358,592 | 1,188,160/1,226,112 | 1,680,448/2,240,640 | 19,520/19,776; 1,376/1,536; 1,536/1,664 | One live texture conversion/decode/alloc/create/upload/fallback reopens the lifecycle fence |
| Natural KO | `FB0704BA5E23782903A28429D58DD89C29DCF7E23131EBE270105ECCC978C144` | 566–573 / 740–754 | 8 | 194,336/222,912 | 799,872/1,053,248 | 470,016/470,144 | 7,584/7,616 | 197,568/197,632 | 73,888/327,360 | 804,672/1,058,048 | 1,120,256/1,680,512 | 50,720/50,880; 1,408/1,408; 1,568/1,664 | Exact source-event gate |
| Natural rebirth | `FB0704BA5E23782903A28429D58DD89C29DCF7E23131EBE270105ECCC978C144` | 589–596 / 786–800 | 8 | 149,120/824,640 | 1,042,880/1,046,592 | 465,504/466,112 | 9,920/9,984 | 197,504/197,632 | 319,904/323,968 | 1,048,864/1,303,360 | 1,680,448/2,240,640 | 49,920/50,048; 1,408/1,472; 1,600/1,728 | Exact source-event gate; update P95 spike retained |
| Late combat | `FB0704BA5E23782903A28429D58DD89C29DCF7E23131EBE270105ECCC978C144` | 1846–1853 / 3300–3314 | 8 | 197,696/209,728 | 864,864/867,328 | 466,048/466,176 | 169,568/171,328 | 208,384/208,768 | 2,400/2,432 | 869,696/872,192 | 1,120,256/1,680,448 | 19,520/20,736; 1,408/1,536; 1,600/1,600 | Source-tick gate; no stale presentation-frame assumption |
| Time-Up boundary | `FB0704BA5E23782903A28429D58DD89C29DCF7E23131EBE270105ECCC978C144` | 1988–1995 / 3584–3598 | 8 | 197,568/617,920 | 865,728/896,064 | 465,984/466,112 | 169,504/171,200 | 208,096/210,112 | 2,400/2,432 | 870,528/900,864 | 1,120,256/1,680,448 | 19,584/49,600; 1,408/1,472; 1,600/1,600 | Final battle owner window; Results exposes no equivalent owner timers |
| Expiry → Results | `FB0704BA5E23782903A28429D58DD89C29DCF7E23131EBE270105ECCC978C144` | full one-minute lifecycle | 1 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | Transition/teardown and 4,084/2,042 pacing pass; reserve gate fails at 117,520 net bytes versus 131,072 |

Profile-1 hard-checkpoint windows are O2-equivalent feasibility evidence, not
profile-0 release baselines. M2 detail and profile-2 forensic samples stay in
`PERF_LEDGER.md`.

## Reconciled stable-30 queue

The old Task 25 matrix above is historical. After the preserved atomic units
close, Task 25R is the only next task. It establishes the current profile-1 and
profile-0 artifact pair, complete phase rows, maximums, interval histogram,
stable-30 deficit, and M3-versus-M2 priority.

Task 25R then selects the implementation order:

1. Default M3 lane: Task 23R Phase 0 -> Task 26 -> Task 23R residual Phase 1.
2. M2 lane: Task 21R -> Task 27. Task 21C is only the compact-table
   foundation for Task 27, not the completed generated fighter architecture.
3. Disjoint lane: Task 20R -> Task 22R, with retail hardware deciding DTCM and
   direct-VRAM/DMA performance.
4. After representations stabilize: Task 28 -> Task 29.
5. Task 24 only in a quiet slot; Task 30 is the final stable-30 gate.

`src/nds/nds_renderer.c` is a mandatory one-writer surface throughout.

## Lane Ownership

| Lane | State | Branch / worktree | Owned surface | Runner |
|---|---|---|---|---|
| Integration/release | July 18 Boundary pass (`DevFast` retired) | live tree | integrated battle ROM, countdown, effects, audio, two-ROM contract, and current terminal lifecycle | no runner active |
| Renderer implementation | ARM restored; Task 12 closed REVERT; Task 14 KEEP | shared integration tree | Retail hardware rejects blanket Thumb by +594,816 DRW ticks (+34.1%); its renderer hot group is device noise and reverts with that base. The renderer TU is ARM again with Task 14 intact | no runner active |
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
| Task 12 renderer code placement | CLOSED / Phase A REVERT; Phase B REVERT WITH BASE | Renderer | Device ARM→Thumb moves DRW 1,743,296→2,338,112 (+594,816/+34.1%), LOOP 2,240,384→2,800,640, and FPS 13.5→10.6. The Thumb hot group then changes DRW only -5,440 (-0.23%) while ACT/LOOP worsen slightly: noise | Keep mode-163 renderer ARM. Retain the safe placement mechanism, not either rejected Task 12 candidate; melonDS cannot referee cache placement |
| Task 13 fighter decimation pack | CLOSED / REVERT | Renderer + Assets | The deterministic 17/32-part pack reduced natural Mario/Fox fighter triangles 626→402. Even mask 0 paid +5,120 draw ticks; the best mask `0x89` removed 64 triangles but regressed draw +3,168 and active +3,328, and the final cold-placement/Fox-skip salvage regressed every paired frame by 3,136–3,264 ticks. Runtime, tools, and derived assets were removed | Do not revive this hash lookup/ITCM design. The performance gate was decisive before visual sign-off or any default-on discussion |
| Task 14 dense-preparation reuse | KEEP / exact | Renderer | A 312-entry generation-gated first-visit plan removes the per-frame 606-corner mask walk while leaving matrices, materials, texture selection, alpha, color, UV, and near-plane work live. Stage is 904,928/905,088 → 895,872/896,000 (-9,056/-9,088); draw is -9,280/-8,704 and active is -9,248/-8,704. All eight paired stage frames improve, native pixels are 0/49,152, production is full1/hit437→444 with zero mismatch/inject/revalidate, and the fault lab is full2/hit436→443/mismatch1/inject1/revalidate1 | Bank the 736-byte BSS / zero-ITCM cut. Keep the exact 55-offset/312-permutation/uniform-alpha host contract and fail-closed publication checker; all live prepared attributes remain per-frame |
| Task 16 extended bit-exact soft-float | PASS / compare, i2f, add/sub shipped; fmul REVERT | Gameplay + Toolchain | The three exact candidates coexist in one link and move source-update 215,104/216,512 → 200,000/201,088 P50/P95 (-15,104/-15,424; -7.02% median). Combined ITCM is 28,820/32,768 with 3,948 bytes free and zero fill; all 3,892/3,892 six-field state rows match with zero overflow. The exact-zero fmul wrapper passed 100M host and 1,050,880 literal ARM9 pairs but regressed its representative microbench 5.34% and natural source-update 1.64%, so all fmul code/tools/link changes were removed. Canonical Boundary passed with published Task16 1/1/1 and ROM `DADB7C96...` | Keep global selectors default OFF for exact lab controls and the published/freeze-equivalent overrides at 1/1/1. Keep stock 408-byte `__aeabi_fmul`; the authoritative combined verifier always rebuilds both sides and binds build, ROM, ELF, and all six row fields |
| Task 17 update hot-text round 2 | KEEP CONFIRMED / update-only on ARM | Gameplay + Toolchain | The original Thumb-base device pair moves UPD 386,240→342,080 (-44,160/-11.4%). The decisive ARM-base pair narrows that claim: UPD 366,016→363,456 (-2,560/-0.70%) and DRW -2,688 cross LOOP 2,800,832→2,240,448 (five→four VBlanks) and FPS 13.9→14.3, while ACT rises 15,872. Exact retained surface remains 11 functions / 5,016 bytes under an 8 KiB ASSERT; Task 14 and Task 16 1/1/1 remain intact | Keep the threshold win and both device photos. Do not claim -44K on ARM or infer cache placement from melonDS; continue phase-wide performance qualification normally |
| Task 18 KO wallpaper spike | CLOSED / bad baseline | Renderer + Platform | Production incremental mode measures KO wallpaper at 302,880/357,824 versus steady 292,224/360,000: +10,656/-2,176, not the cited 547,584/547,648 full-raster control. No runtime change is justified | Keep incremental mode and the affine lab retired. The Task 18 ledger row is the durable correction |
| Pre-25R same-artifact phase matrix | HISTORICAL / superseded contract | Integration | Profile-1 ROM `FB0704BA...` supplies seven eight-frame windows plus Results and remains valid historical evidence. It does not contain Task 25R's profile-0 sibling, maximums, full owner rows, interval histogram, or stable-30 deficit | Preserve the evidence; Task 25R must replace it before any remaining estimate or lane-priority claim |
| M4 zero gameplay conversion/preparation | Startup pass / Whispy lifecycle REOPENED | Renderer | Latest canonical startup frame 212 retains 22 textures / 131,072 bytes, records 646 cache hits, and zero hot conversion/upload. The historical pre-25R Whispy window instead records one conversion, decode, allocation, GL create, upload, and fallback from a live weapon texture | Keep the 57,344-byte overlay layout, but trace and pre-reside the exact live weapon asset before restoring the zero-fence lifecycle claim |
| Lower HUD: FPS, timer, labels, stock, damage | Pass | Integration | User approved; lifecycle and Results clear hook pass | Keep |
| Countdown/3-2-1/GO top presentation | FIXED / full runtime gate pass | Renderer + QA | The source-backed point sample changes 49 atlas pixels only inside the 12x9 `ShadowGo`; all five GO frames retain its 70-pixel count plus 10/10/10 draws/queued/emitted, 57,344 texture bytes, 608 palette bytes, and zero hot conversion/upload. The large-GO mismatch was a stale crop lock after the source-light repair: only 125/26,400 pixels changed, all inside Mario's 22x14 area, while the GO RGB555 payload remained `05330f47...`. The rebuilt full verifier passes with crop `d968b0cc...`, GO `3 OBJ + 10 quads`, and 31,168 OBJ bytes | Keep both accepted crop locks and the source-derived DS assets; no GO source change was warranted |
| Dream Land BGM | Pass | Audio | Tyler reports the stage theme sounds normal. The exact source-derived initial 65,536-byte DS ring has peak 9,928 / RMS 2,283.623; the natural public-ROM recovery route observes the live BGM channel bit in Calico's ARM7-shared mask with clean 44.1 KB/s streaming and zero I/O/unsafe/overrun faults | Keep; repeat only in final lifecycle qualification |
| Required FGM, attack/hit sounds, and Mario/Fox voices | Six common channel starts restored / acoustic and broader audit open | Audio + Gameplay | The source table `dFTMainHitCollisionFGMs` exposed an intentional DS exclusion: natural kick ID 32 produced unsupported delta 1 and no channel. The 121,720-byte pack now maps 18 exact IDs plus punch/kick IDs 40/38/37/34/32/31 from their two exact primary BattleShip samples and source frequency/volume envelopes; omitted composite forks/custom FX remain explicit fidelity debt. The repeated natural ID-32 route records pan 80, supported/unsupported/acquire `1/0/1`, a live channel mask, zero playback faults, and 187,152-byte reserve. Automated channel ACK is not an ear check; five special/projectile contacts remain fail-closed | Keep the six common mappings and natural channel/effect gate. Tyler's exact-ROM ear retest remains required. Next resolve the naturally observed unsupported Escape 11, Grind4 85, MarioFoot 110, MarioDash 121, swing 41/42, GuardOn 13, Fox special 186/189, MarioSmash3 431, and FoxDamage 375 cues; requalify the one-minute reserve before adding more resident audio |
| Winner and Results BGM | Pass | Audio | Natural Fox winner 16 → Results 22; errors/overrun/cleanup zero, reserve 172,024 | Keep gate |
| Stable reserve / no corruption | RED / historical proof is 13,552 bytes short | QA | The pre-25R CPU-on lifecycle reaches Results and reports 183,056 arena headroom with 65,536 resident audio bytes: 117,520 net versus the 131,072-byte floor. Transition, teardown, audio markers, and exact 4,084/2,042 pacing pass; reserve fails | Do not add resident assets or relax the floor. Re-measure the current artifact in Task 25R, then follow its ranked ownership result |
| Focused/checkpoint verification | DevFast + Boundary PASS / ARM-base device confirmation PASS | QA | Published battle candidate `DADB7C96...` passes the 10:17 Boundary checkpoint in 58.8 seconds with normal runtime, mode-163 CPU setup/proc/target 1/33/33, GBI fixtures, two-ROM publication, Task16 1/1/1 at 28,820-byte ITCM, zero renderer rejects, and dated visual analysis. DevFast also passes in 206.4 seconds. The same-HUD retail pair then confirms Task 17's five→four-VBlank threshold win | Keep this development checkpoint. Resume the full one-minute post-pack reserve, exact-ROM ear check, and remaining attack/hit audio-visual cue audit |
| Cut G capture / final dated capture / manual retest | Automated exactness pass / manual current-ROM retest pending | QA + user | Latest capture is `2026-07-18_canonical_fast_101715-1383371-p57268.png`; its paired frame has 747/49,152 meaningful changes, 100% overlap, and all named-region/detail gates pass. Task 6 C/D, Task 8 G2, reserve repair, Task 9/16 state identity, source-light parity, both emitter splits, raw-corner cut, Down+A, and the common-contact checkpoint remain retained | Manually retest exact ROM `DADB7C96...`; automated common-contact A/V closure is focused, not global |

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
