# P2 Execution Board

Created: 2026-08-17. Updated: 2026-09-11 against `35ab1a83dfef`.
This documentation refresh records existing evidence; it ran no ROM or new
acceptance test. Local integrated results are not automatically reproducible
from the pushed source. See **Current integration checkpoint** for scope.

**Native-output acceptance remains RED.** The latest reported four-CPU first
failure is Samus Catch (`320:0x57D8`, status `0xA6`, `REJECTED_PROGRAM`). Kirby
CopyLink remains independently open. Capacity is recovered for the tested
Donkey/Samus/Link/Kirby window, not qualified globally. Do not restart the old
raw-pack investigation merely because broader acceptance is still red.

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
| P2-1 VS shell | **VS Mode reference; VS Options visuals accepted** | `eafdf226c52` connects native VS Options/Item Switch entries, accepted visually. `p2_shell_loop` is red on **its own free-floor assertion** -- 1,968 B against a 32,768 minimum. **CSS cadence remains RED after the 8 KiB/4-node load slice:** the atomic load+finish tic is gone, but stop 5 is still 4,409,600 ticks and `MSVB3 1482/110/24/35 max=8`; 7 loads finish with zero failures while retry rises to 84. Do not tune K before pricing read vs publish/finalize inside one continuation (BUG_NOTES). FPS HUD `b242a60acaa`, latch `705e39b4be0`. |
| P2-2 Four-fighter engine | **Tested-roster capacity recovered locally; global capacity/performance OPEN** | Four compact cores and ShieldPose sets stay live through frame 1,973, clock 60→1; free-min 94,076 B against this arm's 25,600 B floor. Other rosters, shipping-shell overlap, late states/Results/rematch and final performance are not closed. Reproducible integration and evidence links below. |
| P2-3 Fighter production | **Native feature coverage progressing; acceptance OPEN** | Donkey low-detail Up Smash and Kirby Final Cutter effects/weapon pass their recorded natural windows. Next observed failure: Samus Catch; Kirby CopyLink, Ness/Purin/CSS and broader state coverage remain. Zero declines or a four-slot draw mask is not complete native pixels/behavior. See checkpoint and `p2/fighters/`. |
| P2-4 Stage production | **Source/collision checks reported; visual acceptance OPEN** | Castle roof alpha repair recorded 09-09; source collision comparison passes nine stages. Yoster platforms, Inishie bricks and Congo barrel remain unproved on screen; Zebes shafts submit but appearance remains open. Keep current-corpus captures and owner symptoms in `BUGS.md` / `p2/BUG_NOTES.md`; no new stage acceptance here. |
| P2-5 Items | **Native coverage incomplete; Sword lifetime repaired in tested battle** | Source makers/interaction paths exist; registration is not native-state coverage. Sword blade/hilt now draw, with a shared untextured-run fix and 18,528 B entry-only texture retirement. Prior owner counts are not re-censused here. Required atlas membership, other kinds/children and interactions remain open; see checkpoint evidence. |
| P2-6 1P Game | **UNPAUSED by owner 09-10** | 13 plan items now need owners and none has a study. CSS pushed (`d9161127d46`); local integration reaches Intro and Link/Hyrule play after GO at 8,356 B free. Memory margin and campaign acceptance red. Ships `NDS_P2_1P_GAME=0` until a flag flip is verified. |
| P2-7 Modes & meta | **Options/Backup Clear visuals accepted; validation open; Data inaccessible** | Owner (09-06): VS Options, Option and Backup Clear look good; native route, cancellation and host confirmation/clear tests pass. Cadence and disposable-save persistence need verification. |

## Current integration checkpoint

**Next integration outcome:** preserve and land the already-measured capacity
implementation and its generator/probe dependencies; do not re-import it. Then
complete source-defined native features under the existing P2-3/P2-5 owners:
Samus Catch (`p2/fighters/samus.md`) and P2-3f47 CopyLink are known open cases.
Use source-derived sibling/binding checks and bounded distinct-failure collection
where needed. The current recorder exposes a first cause, not all failures;
this doc refresh implements neither a new collector nor a new checker.

Main owns shared outputs and the serialized build. Keep unrelated dirty changes
and local 1P/CSS integration. Independent CSS, item, stage and campaign packages
may advance without waiting for another package's acceptance. Current owner
settings: 30 Hz menus, 1P active, P2-2p8 optimization and P2-3r17 raster repair
deferred. Required final gates remain unchanged.

| Unit | SOURCE / REPRODUCIBILITY | RECORDED RUNTIME EVIDENCE | ACCEPTANCE |
|---|---|---|---|
| P2-2 compact capacity | Recovery implementation was local and mixed with other edits; `c25bc157fb5` records evidence, not its full implementation. Preserve it and commit reproducible dependencies. | Donkey/Samus/Link/Kirby: 4/4 cores, 92,200 B; 4/4 ShieldPose, 11,799 B; heap minimum 94,076 B, margin 68,476 B over this arm's floor; frames 1–1,973, clock 60→1. | Capacity proven only for that local candidate/window. No all-roster, shell, Results/rematch or performance acceptance. |
| P2-3/P2-5 native output | DamageSlash `f36feff7e21`; Sword `2b60863c492`; Cutter effects/weapon `c3f79cf2801`, `be0bfcd4e50`; Donkey binding `35ab1a83dfe`. | Reports show engaged native paths and retained capacity. Donkey passes beyond its frame-416 failure; first wide failure advances to Samus Catch. | Feature-specific proof, not global closure. CopyLink and remaining natural-state/pixel/audio coverage stay open. |
| Shell memory/pacing | `7244f63a95a`; earlier configuration-specific checkpoint. | Snapshot-publication repair and lifecycle evidence recorded; CSS slice still reports 4,409,600 worst ticks. | Shell free-floor/cadence and shipping acceptance remain open; do not mix these figures with the compact battle arm. |
| Options/menu handoff | `68c0e522d3c`, `f33c5aa039f`; earlier root-input configuration. | Startup/nine-entry route, 23 host cases and shell/battle checks reported passing. | Remaining mode/cadence/persistence and required owner acceptance open. |
| Compact 1P previews | Producer `87c6be2549b`; local loader/bridge. | Twelve FPC packs; Link preview reported 66,972 B free, onward battle OOM. | Preserve active 1P work; roster, Intro, memory and campaign gates remain open. |

Permanent reports (each pins its own ROM/configuration and proof scope):
[capacity](../artifacts/performance/2026-09-10_pack-skeleton-ceiling/BATTLE_CORE_RECOVERY.md),
[Kirby hidden-part program](../artifacts/visibility/2026-09-11_kirby-root-program-recovery.md),
[DamageSlash](../artifacts/visibility/2026-09-11_damage-slash-native.md),
[Sword](../artifacts/visibility/2026-09-11_sword-native-vram-lifetime.md),
[Final Cutter](../artifacts/visibility/2026-09-11_kirby-cutter-native.md),
[Donkey binding / next Samus failure](../artifacts/visibility/2026-09-11_donkey-low-modelpart-binding.md).
The September 10 [raw-pack ceiling](../artifacts/performance/2026-09-10_pack-skeleton-ceiling/CEILING.md)
remains historical evidence for its configuration, not a refutation of the later
compact-core result or a current all-roster bound. High detail remains reachable;
low-only stripping is not authorized by the capacity recovery.

**Reproducibility still needs integration.** At this pinned source checkpoint,
`generate_battle_core_packs.py` and the reported `-FirstDonkeyReject` probe
parameter are not present in the pushed paths. Recover existing local work and
validate source/build/probe identity before replay; do not invent equivalent
switches or claim the reports are reproducible solely from HEAD. The normal ROM
reported in the Kirby recovery note is build-health evidence, not a newly
accepted canonical ROM; the published hash above is intentionally unchanged.

## Queue — acceptance only

Request subjective owner checks only after required measurable proof. Missing
pixels/audio or unexercised states remain engineering work, not feel-only review.

- P2-1 shell presentation; P2-2 four-way camera, lower HUD, Team feel, Results and Sudden Death.
- P2-3: Mario/Luigi pipe (`P2-3r1`), Luigi animation (`P2-3r2`), intro visibility (`P2-3r5`), CSS preview rebuild (`P2-3r7`), Falcon/Samus feel.

## Queue — P2-3 engineering

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-3r17 | Intermittent fighter seams/holes around DK and Mario cap | **DEFERRED BY OWNER** | Root-caused as an N64-to-DS raster coverage mismatch, not missing geometry; the production fix is a bounded AOT guard band in the owner generator. Full analysis and acceptance: `docs/BUGS.md`. |
| P2-3f33 | Link entry wave/beam native graduation + integrated specials acceptance | **PARTIAL — static/native checks green; runtime acceptance owed** | Detail: `docs/p2/fighters/link.md`. |
| P2-3f46 | Yoshi stress arm: the landed argmax moves and the roster arm halts before its first sample | **BLOCKED behind P2-2p8** | Same tick-HUD ceiling as the four-CPU arm; resume with it. |
| P2-3f47 | Roster close: Ness, Jigglypuff and Kirby | **IN PROGRESS; bounded Kirby features verified locally** | Hidden-part status `0x116` and Final Cutter have scoped reports above; CopyLink `0x122` remains open. Preserve Purin fixup; qualify Ness/Purin/Kirby CSS, remaining states, residency and stress. Do not equate one roster/window with complete copy-power coverage. |
| P2-3f48 | ITCommonData (0xfb) residency | **LANDED (`45d5fead788`); runtime unverified** | `gITManagerCommonData` loads in `itManagerInitItems`; both asset rows and the address-shaped token row are in. **To close:** read `gNdsITCommonDataBytes` on a booted ROM and confirm 82,976, not 68. |
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
