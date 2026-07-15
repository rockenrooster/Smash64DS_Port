# P1 Execution Board

Updated: 2026-07-15 12:20 Central

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
SHA-256 E08C6C9EA29F671EE5AA9D9D6491B1B12E80A1DBC348AF99468CA72BE072425F
```

Laboratory profile-1 ROMs are evidence only and never replace this filename.

## Phase Evidence

Do not fill this table with figures from a different ROM or phase. Pending is
more accurate than recycling cross-build samples.

| Match phase | Artifact class | ROM SHA-256 | Synchronized window | N | Active median / P95 | State |
|---|---|---|---|---:|---:|---|
| Countdown / GO | canonical-profile-0 | `E08C6C9EA29F671EE5AA9D9D6491B1B12E80A1DBC348AF99468CA72BE072425F` | exact completed frames 438/439 | 2 | — / — | GO state + screenshot pass; phase ticks pending |
| Early combat | canonical-profile-0 | same canonical SHA | Pending post-GO window | — | — / — | Pending synchronized canonical baseline |
| Late combat | canonical-profile-0 | same canonical SHA | Pending synchronized late window | — | — / — | Pending synchronized canonical baseline |
| KO / rebirth | canonical-profile-0 | same canonical SHA | Pending natural KO→rebirth window | — | — / — | Pending synchronized canonical baseline |
| Time Up / Results | canonical-profile-0 | same canonical SHA | Pending natural expiry→Results window | — | — / — | Pending one-minute soak |

Profile-1 M2 samples and profile-2 forensic samples stay in `PERF_LEDGER.md`; they are not canonical phase baselines.

## Lane Ownership

| Lane | State | Branch / worktree | Owned surface | Runner |
|---|---|---|---|---|
| Integration/release | Active | live tree | exact Cut G gate, docs, ROM identity; ID626 loop repair/audible retest | direct `4333/4334`; exact capture slot 2 `4463/4464` |
| Renderer implementation | Active | shared live tree / disjoint files | M2 post-packet redesign and exact host fixtures | focused runner only after bounded host gate |
| Gameplay + QA | Active | shared live tree / disjoint files | sparse DamageFall runtime gate plus M3 dynamic-invalidation contract | focused runner after static gate |
| Performance research | Active | shared live tree / disjoint files | M4 pre-GO residency contract and exact size proof | no emulator |

Only `./emulators/melonds/melonDS.exe` (manual) and repo-owned
`./emulators/melonds-runners/slotN/melonDS.exe` copies may launch. Every TOML
uses the 488x675 vertical, equal-size, native-aspect, zero-gap, unswapped,
unfiltered, OSD-off profile. Ports `3333/3334` stay manual-only; slot 0 uses
`4323/4324`, phase FGM slot 1 uses `3343/3344`, and slot 2 uses `4463/4464`.
Scripted launches normalize their selected TOML. DevFast validates the shared
profile and repo-local launcher policy without auditing mutable TOMLs;
`-AuditLocalConfigs` is opt-in and the all-worktree setter is creation/repair only.
Workers edit only explicitly assigned disjoint surfaces; the integration lane
owns current-truth docs, shared-file arbitration, commits, and publication.
Keep all three subagent slots occupied while three independent P1 packets are
available, and reassign a slot immediately when its packet completes.

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
| M2 Mario/Fox AOT, 170–250K ticks | Fail / FIFO packet rejected | Renderer | Clean A0=A1 413,504; packet B 537,792 (+124,288), exact semantics, runtime path removed | Select a no-whole-copy design; require bounded cost, ≥80K saving, and ≤337,472 first window |
| M3 complete stage AOT, 150–250K ticks | Exact host packet / production absent | Renderer | 10,076 bytes / 8 callbacks / 57 DObjs / 42 bindings / 54 runs / 202 triangles; production_linked=0 | Close dynamic invalidation, then keep only ≥300K saving and ≤500K |
| M4 zero gameplay conversion/preparation | Exact host candidates / production absent | Renderer | Water fixture 181,408 bytes; proposed pre-GO payload 258,048; reserve is estimated only; production_linked=0 | Measure reserve/banks, preload before GO, then require zero post-GO work |
| Lower HUD: FPS, timer, labels, stock, damage | Pass | Integration | User approved; lifecycle and Results clear hook pass | Keep |
| Countdown/3-2-1/GO top presentation | Pass / native OAM | Renderer + QA | Source thread/assets live; 11,584/11,584 ticks, zero gameplay conversion/upload, clean teardown | Keep |
| Dream Land BGM | Partial | Audio | User reports the stage theme sounds normal; stream counters pass, but enabled DS channel and nonzero PCM peak remain unproved | Block audio completion |
| Required FGM and Mario/Fox voices | Production phase/KO + isolated crowd-command/loop-pack pass; audible pending | Audio | Source calls ID626 once for 6.9 s; its 1.868 s DS loop intentionally overlaps countdown/GO, but exact channel-mask/acoustic proof is absent | Add host acoustic oracle and tighten existing ACK masks; exact-ROM audible retest; voices remain open |
| Winner and Results BGM | Pass | Audio | Natural Fox winner 16 → Results 22; errors/overrun/cleanup zero, reserve 172,024 | Keep gate |
| Stable reserve / no corruption | One-match pass | QA | Conservative reserve 172,024 bytes after BGM; stale=0/0 and 17 safety counters zero; repetition pending | Keep gate; repeat for qualification |
| Shared-TU Regression checkpoint | Running | QA | Forced Regression prebuild/stamp plus four isolated NoBuild shards | Block this checkpoint until pass |
| Full profile / release regression | Pending | QA | 60 Full-only records remain outside Regression | Block P1 release |
| Cut G capture / final dated capture / manual retest | Cut G pass / final pending | QA + user | Exact frame438/439 pair passes; final post-change ROM still needs dated capture and user qualification | Block release on final evidence |

## Reconciled Blockers

- Confirmed/open: damage/throw map collision, renderer M2–M4, missing
  pitch/voice playback and 24 unsupported FGM calls, incomplete audible Dream
  Land BGM proof, final CPU-on canonical qualification after Tyler re-enables
  Fox decisions, and final exact-ROM qualification evidence.
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
- Fireball early submit/damage/rebound and one-minute lifecycle/safety gates
  pass; full-lifetime visual/source-matrix parity remains open. The platform
  route has a next-frame landing blind spot and no longer
  counts as semantic closure; both manual symptoms remain open.
- No retained M2 treatment passes the ≥80K / ≤337,472 first-window gate. M3's
  slab and M4's tiled water remain host candidates, not device/runtime progress;
  keep their falsification gates in `OPTIMIZATION_ROADMAP.md`.
- Audio IDs/handles, phase FGMs, Mario regular-KO triplet, and Fox winner 16 →
  Results 22 pass naturally; pitch, voice 685, and other voices remain.
- Exact canonical GO frames 438/439 pass runtime/OAM state and all screenshot
  gates under `artifacts/visibility`; stable `latest.png` is refreshed.

### July 15

- Require a measured renderer decision, not broad work in progress.
- Require either repeatable freeze evidence or repeatable no-freeze soak data.
- Require runtime heavy-hit/throw recovery gates plus exact-ROM manual retests
  for platforms and long-distance Fireball travel.
- Isolated LIVE `mpprocess` symbol closure passes after the endpoint/common
  coordinate repair, but the first natural verifier stalled before attack.
  Run the sparse driver and repair moving-wall/project-floor providers plus
  coherent `mpcommon` before default-live graduation.
- Reject the whole-owner M2 FIFO packet: clean A0=A1 is 413,504 ticks and B is
  537,792. Remove its runtime selector/link, retain exact host fixtures and the
  dated negative ledger, and require a bounded no-whole-copy design next.
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
