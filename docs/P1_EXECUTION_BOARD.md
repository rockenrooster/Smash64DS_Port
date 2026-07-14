# P1 Execution Board

Updated: 2026-07-14 19:00 Central

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
12,043,264 bytes
SHA-256 385B9F051C5CBB801089C69E13D49F9E0D19C07F1E4DA19DA943772B5553FC21
```

Laboratory profile-1 ROMs are evidence only and never replace this filename.

## Phase Evidence

Do not fill this table with figures from a different ROM or phase. Pending is
more accurate than recycling cross-build samples.

| Match phase | Artifact class | ROM SHA-256 | Synchronized window | N | Active median / P95 | State |
|---|---|---|---|---:|---:|---|
| Countdown / GO | canonical-profile-0 | `385B9F051C5CBB801089C69E13D49F9E0D19C07F1E4DA19DA943772B5553FC21` | Pending exact pre-GO→GO window | — | — / — | Pending canonical phase baseline |
| Early combat | canonical-profile-0 | same canonical SHA | Pending post-GO window | — | — / — | Pending July 14 baseline |
| Late combat | canonical-profile-0 | same canonical SHA | Pending synchronized late window | — | — / — | Pending July 14 baseline |
| KO / rebirth | canonical-profile-0 | same canonical SHA | Pending natural KO→rebirth window | — | — / — | Pending July 14 baseline |
| Time Up / Results | canonical-profile-0 | same canonical SHA | Pending natural expiry→Results window | — | — / — | Pending one-minute soak |

Profile-1 M2 samples and profile-2 forensic samples stay in `PERF_LEDGER.md`; they are not canonical phase baselines.

## Lane Ownership

| Lane | State | Branch / worktree | Owned surface | Runner |
|---|---|---|---|---|
| Integration/release | Active | live tree | shared gates, one-minute lifecycle integration, current docs, ROM identity | direct `4333/4334`; capture slot 3 `3363/3364` |
| Renderer implementation | Active | isolated M4 corpus lane | zero-I/O pair-index/palette feasibility | no emulator |
| Gameplay + QA | Active | `codex/renderer-parity-contract` | M2/M3 RGB15/t16 owner-parity microbenchmark | no emulator |
| Performance research | Active | isolated audio worktree | source-backed regular KO FGM slice | no emulator |

Only `./emulators/melonds/melonDS.exe` (manual) and repo-owned
`./emulators/melonds-runners/slotN/melonDS.exe` copies may launch. Every TOML
uses the 488x675 vertical, equal-size, native-aspect, zero-gap, unswapped,
unfiltered, OSD-off profile. Ports `3333/3334` stay manual-only; slot 0 uses
`4323/4324`, phase FGM slot 1 uses `3343/3344`, and slot 2 uses `4463/4464`.
Run `Set-MelonDSWindowConfig.ps1 -AllWorktrees` after creating runner slots.
Workers do not edit shared files or current-truth docs; they return a
self-contained commit plus exact reproduction evidence.
Keep all three subagent slots occupied while three independent P1 packets are
available, and reassign a slot immediately when its packet completes.

## Acceptance Matrix

| Acceptance condition | State | Lane | Blocker / evidence | Integration decision |
|---|---|---|---|---|
| Mario human versus original level-3 Fox CPU, Dream Land, one-minute Time match, items off | State/lifecycle pass | Gameplay + QA | Exact 3600→0, active CPU, Time Up, 22→24, Results120 pass; exact canonical-duration qualification remains | Keep gate; block release qualification |
| Original Wait → countdown → GO control/timer gate | Pass | Integration | Automated synchronized source-state proof passes | Keep |
| Mario can damage Fox | Trigger pass / manual retest | Gameplay | Exact-ROM natural Fox up-smash trace restored 11/11 colliders, zero mismatch, flag clear; repeat Mario→Fox contacts pending | Keep repair; add continuous counter after higher release blockers |
| Fireball visible, moving, damaging, and destructible | Pass | Gameplay | Source spawn/damage/lifetime and four moving visible textured hardware draws pass with zero rejects | Keep dedicated natural-input gate |
| Platform/edge and specials | Retest | Gameplay | Reports predate current exact ROM; Up-B was manually accepted | No behavior change without current reproduction |
| Natural Fox recovery | Coverage debt | Gameplay | Natural Recover objective/offstage return unobserved | No completion claim |
| Cut G M1 affine BG2, 5–35K ticks | Pass | Renderer | 1,856/1,856 ticks and accepted canonical screenshot | Keep canonical |
| M2 Mario/Fox AOT, 170–250K ticks | Fail / active | Renderer | Current ~431K; no-submit floor 331K | Instrument whole-owner wall; block promotion |
| M3 complete stage AOT, 150–250K ticks | Not started | Renderer | Shared renderer owner follows M2 decision | Hold, then serialize behind M2 |
| M4 zero gameplay conversion/preparation | Streaming rejected / active | Renderer | Exact corpus passes; direct path made 12 NitroFS reads/32 frames and did not improve inclusive timing | Require complete pre-GO zero-I/O residency |
| Lower HUD: FPS, timer, labels, stock, damage | Pass | Integration | User approved; lifecycle and Results clear hook pass | Keep |
| Countdown/3-2-1/GO top presentation | Pass / native OAM | Renderer + QA | Source thread/assets live; 11,584/11,584 ticks, zero gameplay conversion/upload, clean teardown | Keep |
| Dream Land BGM | Partial | Audio | Stream counters pass; enabled DS channel and nonzero PCM peak unproved | Block audio completion |
| Required FGM and Mario/Fox voices | Phase FGM/handles pass; voices open | Audio | Five natural phase IDs pass; recyclable tokens pass; voices and 25 observed unsupported calls remain | Keep pack; implement source calls |
| Winner and Results BGM | Backend integrated / gate open | Audio | Source tracks Mario 12 / Fox 16 / Results 22 stream; natural transition proof pending | Block full Results acceptance |
| Stable reserve / no corruption | One-match pass | QA | Conservative reserve 171,916 bytes after BGM; stale=0/0 and 17 safety counters zero; repetition pending | Keep gate; repeat for qualification |
| Full Regression | Pending | QA | Required after architecture freeze | Block release candidate |
| Dated canonical capture and exact-ROM manual retest | Pending | QA + user | July 18 qualification artifact absent | Block release |

## Reconciled Blockers

- Confirmed: renderer M2–M4, missing voice playback and 25 observed unsupported
  FGM calls, incomplete natural winner/Results transition and audible-channel
  proof for Dream Land BGM, exact canonical-duration qualification, Full
  Regression, and final dated capture/manual exact-ROM qualification.
- Closed: exact US sound-ID drift, recyclable FGM handles, five natural countdown phase FGMs,
  visible/damaging Fireball, one-minute natural lifecycle/single-match safety,
  lower HUD pixels, native countdown OAM, Cut G M1, and the 10.5K
  CPU-only joint-preorder experiment.
- Retest before changes: repaired Mario→Fox repeat damage, freeze,
  platform/edge behavior, Up-B, and missing
  stage/audio reports came from older artifacts or are not currently
  reproducible. Fox recovery is coverage debt, not a confirmed bug.

## Dated Gates

### July 14

- Establish this board, lane ownership, worktrees, build isolation, and ports.
- Close the 10.5K preorder and 36K split-matrix experiments.
- Record countdown, early-combat, late-combat, KO/rebirth, and Results phase
  baselines; repeat canonical one-minute qualification.
- Fireball render/damage and one-minute lifecycle/safety gates now pass.
- M2/M3 research ranked whole-owner cuts; countdown software cost is isolated.
- Audio IDs/handles and AOT PublicExcited → 3 → 2 → 1 → GO pass; winner/
  Results streams are integrated, while voices and their natural gates remain.

### July 15

- Require a measured renderer decision, not broad work in progress.
- Require either repeatable freeze evidence or repeatable no-freeze soak data.
- Require visible/damaging Fireball progress behind a distinct weapon owner.
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
