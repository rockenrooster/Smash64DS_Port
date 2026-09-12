# P2 Execution Board

Created: 2026-08-17.
Updated: 2026-09-12 after reproducible compact capacity and isolated checks.

**Native-output acceptance remains RED; scoped compact capacity passes.**
Isolated source-export stress with default items and both details reaches clock
1 with 48,868 B free. First native failure is Samus morph `0x9D:0x8158`.
Earlier capsule-diagnostic/shared-disk runs are not standing-gate evidence.

**The only dynamic queue.** Normal restart reads `docs/HANDOFF.md` + this file.
Plans live in `docs/P2_PLAN.md` + `docs/p2/`. Closed row history lives in
`docs/archive/P2_CLOSED_ROWS.md`; measurements in `PERF_LEDGER.md`; chronology in
`PORTING.md`. Those large documents are lookup-only during ordinary work.

## Standing rules

1. **Measurement law:** `docs/VERIFYING.md` owns procedure. Boundary is
   `p2_shell_loop`, `p2_battle_realtime`, and `p2_fourcpu_stress`.
   `p2_battle_realtime` is mode 163: shell-driven Mario human vs level-3 Fox,
   Dream Land, one-minute Time, items off. Gate arms use the one-minute match;
   long soak length is a separate flag.
2. Cadence verdicts use all presented frames; the 1,600-frame gameplay rank-80
   remains candidate-sizing evidence. Measure the configuration that actually
   ships (`nds_build_config.h` is truth).
3. **Publish law:** P2 publishes only verifier-covered `smash64ds.nds`, from the
   shipping VS shell with human input, no scripted walk, and no fast logic.
   Rebuild it after each verified fix batch. The frozen P1 artifact is not
   rebuilt routinely.
4. Last recorded published P2 ROM hash (carried forward, not reverified here):

SHA-256 2CB6B86242F9BF2B0CF8D99FF0405C1C4F87DE38F1A03AA51D3514BED421DF99

5. Performance/visibility evidence is permanent under `artifacts/performance`
   and `artifacts/visibility`. Device A/B reports include 2/3/4/5+ VBlank
   histogram, max interval, and P50/P95.

## Phase status

| Phase | State | Gate summary |
|---|---|---|
| P2-1 VS shell | **Shell regression RED before battle** | Isolated Mario/Fox guard times out at 600 s before VSBattle start. Localize the shell stop; native CSS uses 64 KiB compact slots, so old raw-slice timings do not apply. Startup passes. No new publication. |
| P2-2 Four-fighter engine | **Scoped capacity PASS; final performance RED** | Source-export one-minute run, both details: 48,868 B free, 23,268 B margin. Other rosters, shell overlap, Results/rematch and final cadence remain open. |
| P2-3 Fighter production | **Acceptance OPEN** | Link Neutral-B/Spin have diagnostic output; source-default proof remains. Samus morph (roll/cliff-escape/Bomb) is independently ready. Preserve prior scoped proofs unless contradicted. |
| P2-4 Stage production | **Visual acceptance OPEN** | Nine-stage source collision comparison passes; Castle alpha repair recorded. Yoster/Inishie/Congo actors and Zebes appearance remain unproved. Source symptoms/captures: `BUGS.md` / `p2/BUG_NOTES.md`. |
| P2-5 Items | **Native coverage incomplete** | Sword lifetime repair recorded. Registration is not state coverage; atlas membership, other kinds/children and interactions remain open. |
| P2-6 1P Game | **UNPAUSED 09-10** | CSS `d9161127d46`; local Intro→Link/Hyrule reaches GO with 8,356 B free. Thirteen plan items, memory and campaign acceptance remain open. Shipping flag stays 0 until verified. |
| P2-7 Modes & meta | **Options/Backup Clear visuals accepted; validation open; Data inaccessible** | Owner (09-06): VS Options, Option and Backup Clear look good; native route, cancellation and host confirmation/clear tests pass. Cadence and disposable-save persistence need verification. |

## Current integration checkpoint

**Next shared fix:** localize the isolated shell guard's pre-battle timeout.
**Independent next feature:** Samus morph-ball under the existing Samus owner. Source
`216_SamusMainMotion.c` hides all parts, then draws joint 6 alone using roots
`0x8158` / `0x8708`, then restores the body. Existing geometry is reusable;
add complete one-root programs for roll, cliff-escape and ground/air Bomb.
This is independent of closed Catch. Capacity is a separate shared blocker.

Main owns shared outputs and the serialized build. Keep unrelated dirty changes
and local 1P/CSS integration. Independent CSS, item, stage and campaign packages
may advance without waiting for another package's acceptance. Current owner
settings: 30 Hz menus, 1P active, P2-2p8 optimization and P2-3r17 raster repair
deferred. Required final gates remain unchanged.

| Unit | Current evidence / boundary |
|---|---|
| Compact capacity + Link | [Current report](../artifacts/visibility/2026-09-12_link-native-integration.md) pins exported source, isolated whole-match capacity and Samus rejection. Diagnostic captures are labeled; shared alpha-key/hat-binding producers are included. |
| Retained native proofs | DamageSlash `f36feff7e21`; Sword `2b60863c492`; Cutter `c3f79cf2801` / `be0bfcd4e50`; Donkey `35ab1a83dfe`; Samus Catch `5389765200f`; Link Catch `f4437339d28`; CopyLink `5e09e477b29`. Keep their configuration limits. |
| Options / 1P | Options `68c0e522d3c` / `f33c5aa039f`; compact preview producer `87c6be2549b`. Preserve local loaders/bridge and active campaign work. Prior route/preview checks do not close cadence, persistence, memory or campaign acceptance. |

Permanent reports (each pins its own ROM/configuration and proof scope):
[capacity](../artifacts/performance/2026-09-10_pack-skeleton-ceiling/BATTLE_CORE_RECOVERY.md),
[Kirby hidden-part program](../artifacts/visibility/2026-09-11_kirby-root-program-recovery.md),
[DamageSlash](../artifacts/visibility/2026-09-11_damage-slash-native.md),
[Sword](../artifacts/visibility/2026-09-11_sword-native-vram-lifetime.md),
[Final Cutter](../artifacts/visibility/2026-09-11_kirby-cutter-native.md),
[Donkey binding / next Samus failure](../artifacts/visibility/2026-09-11_donkey-low-modelpart-binding.md),
[Samus Catch](../artifacts/visibility/2026-09-11_samus-catch-native.md),
[Link Catch](../artifacts/visibility/2026-09-11_link-catch-native.md), and
[Kirby CopyLink](../artifacts/visibility/2026-09-12_kirby-copylink-native.md).
The older [raw-pack ceiling](../artifacts/performance/2026-09-10_pack-skeleton-ceiling/CEILING.md)
and LOW-only capacity reports retain their historical scope. HIGH remains
reachable; stripping it is not authorized. Current builds do not replace the
published P2 hash above until required gates pass.

## Queue — acceptance only

Request subjective owner checks only after required measurable proof. Missing
pixels/audio or unexercised states remain engineering work, not feel-only review.

- P2-1 shell presentation; P2-2 four-way camera, lower HUD, Team feel, Results and Sudden Death.
- P2-3: Mario/Luigi pipe (`P2-3r1`), Luigi animation (`P2-3r2`), intro visibility (`P2-3r5`), CSS preview rebuild (`P2-3r7`), Falcon/Samus feel.

## Queue — P2-3 engineering

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-3r17 | Intermittent fighter seams/holes around DK and Mario cap | **DEFERRED BY OWNER** | Root-caused as an N64-to-DS raster coverage mismatch, not missing geometry; the production fix is a bounded AOT guard band in the owner generator. Full analysis and acceptance: `docs/BUGS.md`. |
| P2-3f33 | Link entry wave/beam + specials | **PARTIAL — source programs implemented** | Topology and compact-file admission repaired; retain Catch proof. Neutral-B/Spin diagnostic output needs isolated source-default requalification; sibling/lifetime, visual/audio acceptance remain. |
| P2-3 Samus | Morph-ball source program closure | **READY — first native failure `0x9D`** | Reuse appendix roots `0x8158/0x8708` as complete one-root programs. Cover rolls, cliff escapes and Bomb restoration; preserve Catch program 1. |
| P2-3f46 | Yoshi stress arm: the landed argmax moves and the roster arm halts before its first sample | **BLOCKED behind P2-2p8** | Same tick-HUD ceiling as the four-CPU arm; resume with it. |
| P2-3f47 | Roster close: Ness, Jigglypuff and Kirby | **IN PROGRESS; CopyLink CLOSED for measured natural path** | Hidden-part status `0x116`, Final Cutter and CopyLink `0x122..0x127` have scoped reports above. Preserve Purin fixup; qualify Ness/Purin/Kirby CSS, remaining copy powers/states, residency and stress. Do not equate one roster/window with complete copy-power coverage. |
| P2-3c1 | Exact pose clock | **WIRED; runtime differential/cost owed** | Binary32 clock replaces Q12 timing (`f6f65a…`, `nds_f32_exact.h`); pose values stay Q12. Run `test_pose_clock_differential.py` live set through ROM oracle and measure cost. |

## Queue — P2-4 engineering

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-4s1..s8 | All eight VS stages | **REOPENED — owner rejects current result** | Missing backgrounds on all eight; geometry, moving platforms/collision, hazards, pipes and audio failures vary by stage. See verbatim `docs/BUGS.md`; verify source behavior and native routing before closing. |
| P2-4n1 | Native stage packet and actors | **38 blob packets plus Dream Land linked; acceptance open** | Host tests pass. Actor arms run since the ground-vars union fix; acid keeps G_ZBUFFER. Barrel submits but is unproved on screen. Lakitu/Bronto and visual acceptance open (`docs/p2/BUG_NOTES.md`). |

## Queue — P2-5 items

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-5i1 | Item manager and twenty common items | **SOURCE PRESENT; Sword tested-lifetime repair recorded** | Blade/hilt and entry-texture lifetime proof linked above. Remaining kinds, children, states, interactions and full natural-path acceptance stay open. |
| P2-5i2 | The 13 Poke Ball Pokemon | **ALL 13 IN THE ROM; draw owners missing** | Dispatch proved by `gNdsItMonsterMakerMask` = `1fff`, read off the table: a ball opens only when thrown or hit, so a 60 s CPU match can spawn five and open none. |
| P2-5i3 | Stage-spawned kinds | **8 OF 10 IN THE ROM; two behind the 1P flag** | POW, Piranha and Saffron's five ship; Target and TaruBomb behind `NDS_P2_1P_GAME`. Native owners exist for 1 of 42 distinct item shapes. |
| P2-5i4 | Pick up, throw, shoot and swing | **LANDED; acceptance open** | Pickup animation FileIDs (all `0u`) resolved 09-09. |
| P2-5u1 | Item Switch and VS Options screens | **Entry/row repair committed; acceptance open** | `eafdf226c52`. Switch mask is honoured by the spawn law; the UI half is uncensused. |
| P2-5x1 | Audio cue coverage | **SOURCE WIRED; ROM acceptance pending** | FGM header pins 573 entries / 6,874,344 B; census covers all 47 banks. |
| P2-5a1 | Item TU fidelity audit | **CLEAN** | All 21 item TUs line-by-line against decomp, 09-03/04. |

## Queue — P2-2 performance debt

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-2p8 | Four-CPU renderer/performance, target `<1.12m` ticks | **DEFERRED BY OWNER; final performance RED** | Preserve measurements; no CPU optimization until authorized. Capacity/native correctness progress does not establish final P95/cadence acceptance. |

## Queue discipline

- Keep only red/current/deferred/owner-acceptance summaries; move closed detail out at once, and keep each row short enough to decide the next action without loading its history.
- After verified progress, update the existing row and permanent evidence; handoff points here. Distinguish local candidates, reproducible commits and accepted scope. Agent scratch under `builds/` is gitignored; record its durable finding before handoff. Keep original owner reports in `BUGS.md` unchanged.
- Worktree audit: 20 auxiliaries outside `.worktrees/`; 17 dirty/ambiguous, including a locked incomplete checkout. Create none. Separate cleanup cycle must recheck idle/status, hash-migrate evidence and reconcile preserved work; three clean ancestor-contained candidates are `builds/p2-v4-index-worktree`, `.codex-worktrees/startup-oom-run`, and sibling `_s64_itcm_measure`.
