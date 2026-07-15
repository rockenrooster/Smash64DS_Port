# P1 Execution Board

Updated: 2026-07-15 03:52 Central

Deadline: 2026-07-19 23:59 Central

Boundary: `battle_playable_realtime`, mode 163

This is the only dynamic P1 queue. `STATUS.md` summarizes current truth,
`HANDOFF.md` owns exact commands, `optimization/NATIVE_RENDERER_PLAN.md` is the
renderer technical contract, `PERF_LEDGER.md` owns measurements, and
`PORTING.md` remains append-only history.

## Artifact Identity

Current verifier-covered canonical/shipped pair:

```text
smash64ds-battle-playable-hwtri.nds
14,368,768 bytes
SHA-256 F8EFEE10ED15457CD79A9B71B9766B5247BE870C332FB12316431F8301A0A94A
```

Laboratory profile-1 ROMs are evidence only and never replace this filename.

## Phase Evidence

Do not fill this table with figures from a different ROM or phase. Pending is
more accurate than recycling cross-build samples.

| Match phase | Artifact class | ROM SHA-256 | Synchronized window | N | Active median / P95 | State |
|---|---|---|---|---:|---:|---|
| Countdown / GO | canonical-profile-0 | `F8EFEE10ED15457CD79A9B71B9766B5247BE870C332FB12316431F8301A0A94A` | exact completed frames 438/439 | 2 | — / — | GO state + screenshot pass; phase ticks pending |
| Early combat | canonical-profile-0 | same canonical SHA | Pending post-GO window | — | — / — | Pending synchronized canonical baseline |
| Late combat | canonical-profile-0 | same canonical SHA | Pending synchronized late window | — | — / — | Pending synchronized canonical baseline |
| KO / rebirth | canonical-profile-0 | same canonical SHA | Pending natural KO→rebirth window | — | — / — | Pending synchronized canonical baseline |
| Time Up / Results | canonical-profile-0 | same canonical SHA | Pending natural expiry→Results window | — | — / — | Pending one-minute soak |

Profile-1 M2 samples and profile-2 forensic samples stay in `PERF_LEDGER.md`; they are not canonical phase baselines.

## Lane Ownership

| Lane | State | Branch / worktree | Owned surface | Runner |
|---|---|---|---|---|
| Integration/release | Active | live tree | exact Cut G gate, docs, ROM identity; ID626 loop repair/audible retest | direct `4333/4334`; exact capture slot 2 `4463/4464` |
| Renderer implementation | Active | isolated tooling commits | M2 transaction device gate, then M3 owner | no emulator |
| Gameplay + QA | Active | read-only source map | collision graduation and current playtest ordering | no emulator |
| Performance research | Active | isolated research | M3 architecture and M4 RGB256 device-falsifier review | no emulator |

Only `./emulators/melonds/melonDS.exe` (manual) and repo-owned
`./emulators/melonds-runners/slotN/melonDS.exe` copies may launch. Every TOML
uses the 488x675 vertical, equal-size, native-aspect, zero-gap, unswapped,
unfiltered, OSD-off profile. Ports `3333/3334` stay manual-only; slot 0 uses
`4323/4324`, phase FGM slot 1 uses `3343/3344`, and slot 2 uses `4463/4464`.
Scripted launches normalize their selected TOML. DevFast validates the shared
profile and repo-local launcher policy without auditing mutable TOMLs;
`-AuditLocalConfigs` is opt-in and the all-worktree setter is creation/repair only.
Workers do not edit shared files or current-truth docs; they return a
self-contained commit plus exact reproduction evidence.
Keep all three subagent slots occupied while three independent P1 packets are
available, and reassign a slot immediately when its packet completes.

## Acceptance Matrix

| Acceptance condition | State | Lane | Blocker / evidence | Integration decision |
|---|---|---|---|---|
| Mario human versus original level-3 Fox CPU, Dream Land, one-minute Time match, items off | State/lifecycle pass | Gameplay + QA | Exact 3600→0, active CPU, Time Up, 22→24, Results120 pass; exact canonical-duration qualification remains | Keep gate; block release qualification |
| Original Wait → countdown → GO control/timer gate | Pass | Integration | Automated synchronized source-state proof passes | Keep |
| Mario can damage Fox | Manual pass / continuous gate open | Gameplay | User confirmed damage works after the exact-ROM Fox trace restored 11/11 colliders, zero mismatch, and flag clear | Keep repair; add continuous natural-hit coverage before release |
| Fireball spawn/render/damage/destruction | Pass | Gameplay | Source spawn/damage/lifetime and 40 moving visible textured hardware draws pass with zero rejects | Keep dedicated natural-input gate |
| Fireball trajectory and floor rebound | Automated source-MVP/rebound pass / manual travel OPEN | Gameplay | BattleShip custom `0x47` applied 40/40 with zero mismatch/reject/drift; natural travel=1,757, rebound=55→46.75, reserve=222,736; dated capture exists | Keep gate; close only after exact-ROM user retest |
| Damage/throw map collision | Confirmed manual defect (P0) / gates open | Gameplay | Live damage/default policies are floor-only; BattleShip requires wall/ceiling Run state for connected-floor recovery. DamageFall timed out and throw remains candidate-only | Graduate coherent `mpprocess` + damage/default `mpcommon`; require zero in-bounds fallthrough and one reconciliation per release |
| One-way platform semantics | Natural all-three-line pass / manual OPEN | Gameplay | 715-frame gate: exact live geometry; Mario-only mask `0x7`, 3 upward passes/0 accepts, 9 reverse hits/landings, 2 side cycles, 3 Pass rejections, reserve 222,736 | Keep gate and require user retest of the original symptom |
| Edge behavior and specials | Retest | Gameplay | Up-B was manually accepted; other edge behavior needs current-ROM qualification | No unrelated behavior change without reproduction |
| Pause-orbit geometry containment | Reproduced wide-view symptom / P2 parity open | Renderer + QA | Normal source-envelope and front/±16.8° frames are clean; synchronized ±33.6° frames show foreground occlusion. Plus view has 15.200% one-color concentration; minus retains only 0.602% green | Keep the deterministic symptom gate; compare identical BattleShip/N64 view before changing renderer |
| Natural Fox recovery | Coverage debt | Gameplay | Natural Recover objective/offstage return unobserved | No completion claim |
| Cut G M1 affine BG2, 5–35K ticks | Pass | Renderer | 1,856/1,856 ticks; exact frames 438/439 pass and publish | Keep canonical |
| M2 Mario/Fox AOT, 170–250K ticks | Fail / active | Renderer | ~431K; 50–75K hierarchy evidence; Mario/Fox tables have no animlock motions, but the generic active branch is incomplete; GX slots 16–23/16–17 | Require natural animlock census zero/reject-before-GX, exact active fixture for promotion, and ≥80K saving / ≤351K first window |
| M3 complete stage AOT, 150–250K ticks | Strict falsifier specified | Renderer | One preflight/session; callback GX segments preserve links 4/6/13/16/17 around fighters/weapons; exact 42/886/302/54/202, ≤16 KiB, no BSS/heap | Keep only ≥300K paired saving and ≤500K; otherwise remove |
| M4 zero gameplay conversion/preparation | Static manifest specified / full-water representation blocked | Renderer | ~69 static keys / 179,328 bytes fit; +2 water owners estimate 71 / 216,192, but exact water needs 903,168 bytes versus 524,288 total texture VRAM | Generate exact static census/prewarm; require a new 216-state water representation and zero post-GO violations |
| Lower HUD: FPS, timer, labels, stock, damage | Pass | Integration | User approved; lifecycle and Results clear hook pass | Keep |
| Countdown/3-2-1/GO top presentation | Pass / native OAM | Renderer + QA | Source thread/assets live; 11,584/11,584 ticks, zero gameplay conversion/upload, clean teardown | Keep |
| Dream Land BGM | Partial | Audio | User reports the stage theme sounds normal; stream counters pass, but enabled DS channel and nonzero PCM peak remain unproved | Block audio completion |
| Required FGM and Mario/Fox voices | Production phase/KO + isolated crowd-command/loop-pack pass; audible pending | Audio | User ROM has no blocking ACK trace; PNT=1/LEN=3527 state model rejects missing restore/wrong PNT/LEN; cycle has 28,214 source + 2 guard samples | Exact-ROM audible retest; voices remain open |
| Winner and Results BGM | Pass | Audio | Natural Fox winner 16 → Results 22; errors/overrun/cleanup zero, reserve 172,024 | Keep gate |
| Stable reserve / no corruption | One-match pass | QA | Conservative reserve 172,024 bytes after BGM; stale=0/0 and 17 safety counters zero; repetition pending | Keep gate; repeat for qualification |
| Full Regression | Pending | QA | Required after architecture freeze | Block release candidate |
| Cut G capture / final dated capture / manual retest | Cut G pass / final pending | QA + user | Exact frame438/439 pair passes; final post-change ROM still needs dated capture and user qualification | Block release on final evidence |

## Reconciled Blockers

- Confirmed/open: damage/throw map collision, renderer M2–M4, missing
  pitch/voice playback and 24 unsupported FGM calls, incomplete audible Dream
  Land BGM proof, exact canonical-duration qualification, Full Regression, and
  final exact-ROM qualification evidence.
- Automated pass, manual open: one-way platform semantics and Fireball 40-draw
  source-MVP/rebound now pass focused gates; the original platform and long-distance
  Fireball reports still require exact-ROM user retest.
- Closed: exact US sound IDs, recyclable handles, natural phase/regular-KO FGMs, winner→Results BGM,
  Fireball spawn/render/damage, one-minute natural lifecycle/single-match safety,
  lower HUD pixels, native countdown OAM, exact-frame Cut G M1, and the 10.5K
  CPU-only joint-preorder experiment.
- Manual findings still open: pause-orbit geometry, heavy-hit/throw floor
  recovery, long-distance Fireball travel, platform feel, and the opening crowd
  overlap/loop report. Mario→Fox damage is user-confirmed but still needs a
  continuous gate; Fox recovery remains coverage debt.

## Dated Gates

### July 14

- Establish this board, lane ownership, worktrees, build isolation, and ports.
- Close the 10.5K preorder and 36K split-matrix experiments.
- Record countdown, early-combat, late-combat, KO/rebirth, and Results phase
  baselines; repeat canonical one-minute qualification.
- Fireball source-MVP/render/damage/rebound and one-minute lifecycle/safety gates pass;
  platform semantics also pass the 715-frame all-three-line gate. Both reported visual/
  feel symptoms remain open for user retest.
- M2 evidence supports about 50–75K from the first hierarchy cut; its stricter
  ≥80K keep gate remains. M3/M4 retain their strict keep/remove contracts.
- Audio IDs/handles, phase FGMs, Mario regular-KO triplet, and Fox winner 16 →
  Results 22 pass naturally; pitch, voice 685, and other voices remain.
- Exact canonical GO frames 438/439 pass runtime/OAM state and all screenshot
  gates under `artifacts/visibility`; stable `latest.png` is refreshed.

### July 15

- Require a measured renderer decision, not broad work in progress.
- Require either repeatable freeze evidence or repeatable no-freeze soak data.
- Require runtime heavy-hit/throw recovery gates plus exact-ROM manual retests
  for platforms and long-distance Fireball travel.
- Keep pause-angle symptom gate; isolated crowd command ACKs pass, but require the
  exact-ROM audible retest before closing the opening-crowd report.
- Require one naturally triggered real FGM and one required voice event.

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
  confirmed blocker fixes. Run shared regression after ABI/shared-TU changes.

### July 18

- Release qualification: repeated one-minute soak, Full Regression, clean
  canonical rebuild/parity, exact-ROM manual retest, and dated capture set.

### July 19

- Release-candidate fixes and verification only. No new architecture and no P2
  work. Report every remaining red acceptance row honestly.

## Integration Rule

Keep a change only when identical-configuration counters, synchronized phase
windows, semantic traces, runtime state, memory, audio state, and screenshots
agree. Compilation alone never closes a row. P2 begins only after every P1 row
required by the goal is green, documented, and snapshotted.
