# P1 Execution Board

Updated: 2026-07-14 20:10 Central

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
14,362,624 bytes
SHA-256 57B85DDC6B2919D8962589188D6066F6CE6D0FD83B2F729175C9F339C8CCFAFD
```

Laboratory profile-1 ROMs are evidence only and never replace this filename.

## Phase Evidence

Do not fill this table with figures from a different ROM or phase. Pending is
more accurate than recycling cross-build samples.

| Match phase | Artifact class | ROM SHA-256 | Synchronized window | N | Active median / P95 | State |
|---|---|---|---|---:|---:|---|
| Countdown / GO | canonical-profile-0 | `57B85DDC6B2919D8962589188D6066F6CE6D0FD83B2F729175C9F339C8CCFAFD` | exact completed frames 438/439 | 2 | — / — | GO state + screenshot pass; phase ticks pending |
| Early combat | canonical-profile-0 | same canonical SHA | Pending post-GO window | — | — / — | Pending July 14 baseline |
| Late combat | canonical-profile-0 | same canonical SHA | Pending synchronized late window | — | — / — | Pending July 14 baseline |
| KO / rebirth | canonical-profile-0 | same canonical SHA | Pending natural KO→rebirth window | — | — / — | Pending July 14 baseline |
| Time Up / Results | canonical-profile-0 | same canonical SHA | Pending natural expiry→Results window | — | — / — | Pending one-minute soak |

Profile-1 M2 samples and profile-2 forensic samples stay in `PERF_LEDGER.md`; they are not canonical phase baselines.

## Lane Ownership

| Lane | State | Branch / worktree | Owned surface | Runner |
|---|---|---|---|---|
| Integration/release | Active | live tree | exact Cut G gate, docs, ROM identity; audio checkpoint queued | direct `4333/4334`; exact capture slot 2 `4463/4464` |
| Renderer implementation | Active | isolated tooling commits | M2 transaction device gate, then M3 owner | no emulator |
| Gameplay + QA | Active | read-only source map | collision graduation and current playtest ordering | no emulator |
| Performance research | Active | isolated research | M3 architecture and M4 RGB256 device-falsifier review | no emulator |

Only `./emulators/melonds/melonDS.exe` (manual) and repo-owned
`./emulators/melonds-runners/slotN/melonDS.exe` copies may launch. Every TOML
uses the 488x675 vertical, equal-size, native-aspect, zero-gap, unswapped,
unfiltered, OSD-off profile. Ports `3333/3334` stay manual-only; slot 0 uses
`4323/4324`, phase FGM slot 1 uses `3343/3344`, and slot 2 uses `4463/4464`.
Scripted launches normalize their selected TOML and DevFast checks all worktree
emulator TOMLs; the all-worktree setter is creation/repair only.
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
| Fireball spawn/render/damage/destruction | Pass | Gameplay | Source spawn/damage/lifetime and four moving visible textured hardware draws pass with zero rejects | Keep dedicated natural-input gate |
| Fireball trajectory and floor rebound | Confirmed defect (P1) | Gameplay | Live weapon floor-collision seam always misses, so source rebound never gains floor contact | Require natural floor-mask rise, reflected Y at ~0.85 speed, and no premature destruction at pre-impact speed ≥30 |
| Damage/throw map collision | Confirmed defect (P0) | Gameplay | Damage check returns false outside bounded proof; throw default only copies position, omitting source copy/run/reset resolution | Graduate floor primitive then both policies; require zero in-bounds heavy-hit fallthrough violations and one reconciliation per release |
| One-way platform semantics | Confirmed defect (P0) | Gameplay | Upward passage is blocked; bounded down-pass proof does not cover continuous play | Require zero accepted floor hits while ascending regardless of vertex order, one descending floor edge, jump, and intentional down-pass |
| Edge behavior and specials | Retest | Gameplay | Up-B was manually accepted; other edge behavior needs current-ROM qualification | No unrelated behavior change without reproduction |
| Pause-orbit geometry containment | Open (amber) | Renderer + QA | Straight-on Cut G passes; reported off-axis geometry is not covered | Fixed-angle A/B; P1 if normal battle camera fails, else P2 |
| Natural Fox recovery | Coverage debt | Gameplay | Natural Recover objective/offstage return unobserved | No completion claim |
| Cut G M1 affine BG2, 5–35K ticks | Pass | Renderer | 1,856/1,856 ticks; exact frames 438/439 pass and publish | Keep canonical |
| M2 Mario/Fox AOT, 170–250K ticks | Fail / active | Renderer | ~431K; exact 17,704-byte packet is tooling only, not live progress | Run guarded device falsifier; block promotion |
| M3 complete stage AOT, 150–250K ticks | Open | Renderer | Current stage owner ~801K; follows M2 owner decision | Hold, then serialize behind M2 |
| M4 zero gameplay conversion/preparation | Device falsifier eligible | Renderer | Exact RGB256 tooling passes host parity; palette/runtime/preparation open | Run eight-frame device gate; do not claim M4 |
| Lower HUD: FPS, timer, labels, stock, damage | Pass | Integration | User approved; lifecycle and Results clear hook pass | Keep |
| Countdown/3-2-1/GO top presentation | Pass / native OAM | Renderer + QA | Source thread/assets live; 11,584/11,584 ticks, zero gameplay conversion/upload, clean teardown | Keep |
| Dream Land BGM | Partial | Audio | Stream counters pass; enabled DS channel and nonzero PCM peak unproved | Block audio completion |
| Required FGM and Mario/Fox voices | Phase + regular KO pass; voices open | Audio | Mario `439→292→154`, all five KO IDs, recyclable tokens pass; pitch/685/24 calls remain | Keep pack; implement source calls |
| Winner and Results BGM | Pass | Audio | Natural Fox winner 16 → Results 22; errors/overrun/cleanup zero, reserve 171,916 | Keep gate |
| Stable reserve / no corruption | One-match pass | QA | Conservative reserve 171,916 bytes after BGM; stale=0/0 and 17 safety counters zero; repetition pending | Keep gate; repeat for qualification |
| Full Regression | Pending | QA | Required after architecture freeze | Block release candidate |
| Cut G capture / final dated capture / manual retest | Cut G pass / final pending | QA + user | Exact frame438/439 pair passes; final post-change ROM still needs dated capture and user qualification | Block release on final evidence |

## Reconciled Blockers

- Confirmed: damage/throw map collision, one-way platform semantics, Fireball
  rebound, renderer M2–M4, missing pitch/voice playback and 24 unsupported FGM
  calls, incomplete audible Dream Land BGM proof, exact canonical-duration
  qualification, Full Regression, and final exact-ROM qualification evidence.
- Closed: exact US sound IDs, recyclable handles, natural phase/regular-KO FGMs, winner→Results BGM,
  Fireball spawn/render/damage, one-minute natural lifecycle/single-match safety,
  lower HUD pixels, native countdown OAM, exact-frame Cut G M1, and the 10.5K
  CPU-only joint-preorder experiment.
- Retest before unrelated changes: repaired Mario→Fox repeat damage, freeze,
  Up-B, and missing stage/audio reports are not current confirmed failures. Fox
  recovery is coverage debt. Contain pause-orbit geometry against normal camera
  angles before assigning it to P1 or P2.

## Dated Gates

### July 14

- Establish this board, lane ownership, worktrees, build isolation, and ports.
- Close the 10.5K preorder and 36K split-matrix experiments.
- Record countdown, early-combat, late-combat, KO/rebirth, and Results phase
  baselines; repeat canonical one-minute qualification.
- Fireball render/damage and one-minute lifecycle/safety gates now pass; its
  floor-rebound trajectory remains open.
- M2/M3 research ranked whole-owner cuts; countdown software cost is isolated.
- Audio IDs/handles, phase FGMs, Mario regular-KO triplet, and Fox winner 16 →
  Results 22 pass naturally; pitch, voice 685, and other voices remain.
- Exact canonical GO frames 438/439 pass runtime/OAM state and all screenshot
  gates under `artifacts/visibility`; stable `latest.png` is refreshed.

### July 15

- Require a measured renderer decision, not broad work in progress.
- Require either repeatable freeze evidence or repeatable no-freeze soak data.
- Require one collision-graduation design and natural gates for heavy-hit/
  throw landing, one-way platforms, and Fireball floor rebound.
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
