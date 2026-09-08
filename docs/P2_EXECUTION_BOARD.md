# P2 Execution Board

Created: 2026-08-17.
Updated: 2026-09-06 (owner gameplay review reopens menus, CSS and all eight non-Dream-Land stages).

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
4. The canonical current P2 ROM hash appears on exactly this line:

SHA-256 271DD41BD81CB231065B909FFC66456663339D747346521033DE00F53084D855

5. Performance/visibility evidence is permanent under `artifacts/performance`
   and `artifacts/visibility`. Device A/B reports include 2/3/4/5+ VBlank
   histogram, max interval, and P50/P95.

## Phase status

| Phase | State | Gate summary |
|---|---|---|
| P2-1 VS shell | **VS Mode reference; VS Options visuals accepted** | `eafdf226c52` connects native VS Options/Item Switch entries and fixes row budgets/retry. Owner accepts VS Options visually; natural round trip and 9 bundled host cases pass. Wider regression/cadence acceptance remains. Evidence: `artifacts/performance/2026-09-06_vs-options-bundle/`. |
| P2-2 Four-fighter engine | **RAM cliff and performance RED** | Two items spawn across 1,972 samples / 59 s, all four native draw slots active. Heap floor fell from 31,988 B (09-06) to 15,640 B (09-07 morning) and the 09-07 afternoon run wander-crashed after frame 256; WORK-H P95 2,808,768 exceeds target. Evidence: `artifacts/performance/2026-09-06_fourcpu-real-items-memory/`. Ending/Results and final acceptance remain open. |
| P2-3 Fighter production | **IN PROGRESS — nine enabled in the current public ROM** | Compiled config has Ness/Purin/Kirby=0. Yoshi CSS and Pikachu ears are native again (`docs/BUGS.md`); their battle acceptance, Ness smoke, Kirby heap, Link integration and roster acceptance remain. Details: `docs/p2/fighters/`; pose clock: P2-3c1. |
| P2-4 Stage production | **REOPENED — admission repaired; Zebes crash, acid depth, Hyrule tornado, Inishie music fixed** | All eight admit; ledger 5,120 x 5 B. Open per `docs/BUGS.md`: barrel, Castle roof (range near-plane), Yoster floor/cloud alpha (upload unreached), Inishie platforms/20 FPS, Sector Arwing, Saffron door. Saffron wall is source behaviour. |
| P2-5 Items | **45 of 45 kinds in code (Target behind the 1P flag); runtime acceptance open** | Item Switch and VS Options have source asset coverage. All 22 imported screens have sprite geometry rows (`bfb35a3b6a7`). This proves staging/normalization metadata, not blitter admission, layout or rendered pixels. |
| P2-6 1P Game | **PAUSED BY OWNER** | CSS pushed (`d9161127d46`). Local integration reaches Intro and Link/Hyrule play after GO, 638 updates, 8,356 B free; memory margin/full campaign acceptance remain red. Captures: `builds/resume-20260905/preview-runtime/first-campaign-combat*`. Partial staffroll work preserved with helper stopped. Shipping `NDS_P2_1P_GAME=0`; resume only on owner request. |
| P2-7 Modes & meta | **Options/Backup Clear visuals accepted; validation open; Data inaccessible** | Owner (2026-09-06): VS Options, Option and Backup Clear look good. Native Options/Backup Clear route and cancellation pass; host tests execute native confirmation/clear logic. Cadence and disposable-save persistence still need verification. |

## Current integration checkpoint

**Critical path:** enforce the adopted all-ROM native-only boundary and complete
its live callers, then match repairs in owner order; DATA children last. Owner reports
are authoritative symptoms in `docs/BUGS.md`; generic-fallback use needs measured
attribution and must not silently pass match verification. Main Menu/VS Mode are
the accepted menu references. 1P remains paused. Runtime paging/expansion excluded.
Shield texture and KO pillar regressions added 09-08.

| Unit | SOURCE PRESENT | COMPILED/LINKED | RUNTIME VERIFIED | ACCEPTED |
|---|---|---|---|---|
| Four distinct fighters + real items | `e99db8cf004`, `e5ac85862f8`, `46a5aa33c52`, `21420ebd843` | Strong providers, ITEM_CORE=1, ROM `43829577…` | Standing window completes; 2 items, 31,988 B floor, all four draw slots | No: cache engagement, performance, ending/Results and visual acceptance owed |
| Shell memory/pacing evidence | `7244f63a95a` | Shell ROM hash above; embedded revision c3c37f7 plus integration dirty work | One-minute lifecycle passes; published snapshot flush fixes stale debugger reads | Instrument correction verified; full P2 acceptance open |
| Options source-menu handoff/admission | `68c0e522d3c`, `f33c5aa039f` | Root human-input ROM, strong providers | Startup/nine-entry route, 23 host cases, shell/battle checks pass | Test-ready; remaining modes/visual acceptance open |
| Compact 1P previews | Producer `87c6be2549b`; local loader/bridge | Twelve FPC packs + native root/image remapping compiled; source and actual-C tests pass | Link selected preview and menu render, 66,972 B free; onward battle OOM | No: full roster/costume tour, Intro rendering and campaign gates remain |

Main owns live integration and serialized builds. Preview artifacts/review:
`builds/resume-20260905/{preview-compact,preview-binding}`. The prior zero-filled
Selected section was a prototype defect; repaired bytes do not make the bins runtime-ready.
**1P development is paused by the owner.** Preserve local campaign integration;
the remaining active P2 queues below govern non-campaign work.

Root `smash64ds.nds` rebuilt 2026-09-06: 46,152,704 bytes, canonical hash above.
Startup/Options route pass; menu walk=0, fast logic=0, campaign=0. Stable compiled
inputs and ROM/ELF/config hashes: `builds/resume-20260905/menu-checkpoint/publish-*`.
Screenshots: `artifacts/visibility/2026-09-06_published-menu-*.png`.

## Queue — acceptance only

Owner checks, not implementation work unless a reproduction fails.

- P2-1 shell presentation; P2-2 four-way camera, lower HUD, Team feel, Results and Sudden Death.
- P2-3: Mario/Luigi pipe (`P2-3r1`), Luigi animation (`P2-3r2`), intro visibility (`P2-3r5`), CSS preview rebuild (`P2-3r7`), Falcon/Samus feel.

## Queue — P2-3 engineering

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-3r17 | Intermittent fighter seams/holes around DK and Mario cap | **DEFERRED BY OWNER** | Root-caused as an N64-to-DS raster coverage mismatch, not missing geometry; the production fix is a bounded AOT guard band in the owner generator. Full analysis and acceptance: `docs/BUGS.md`. |
| P2-3f33 | Link entry wave/beam native graduation + integrated specials acceptance | **PARTIAL — static/native checks green; runtime acceptance owed** | Detail: `docs/p2/fighters/link.md`. |
| P2-3f46 | Yoshi stress arm: the landed argmax moves and the roster arm halts before its first sample | **BLOCKED behind P2-2p8** | Same tick-HUD ceiling as the four-CPU arm; resume with it. |
| P2-3f47 | Roster close: Ness, Jigglypuff and Kirby | **IN PROGRESS** | Kirby/Fox OOM on Fox's 115,440 B allocation; fix residency. Jigglypuff fixup closed; Ness smoke, CSS capture and stress remain. Detail: `docs/p2/fighters/{ness,jigglypuff,kirby}.md`. |
| P2-3f48 | ITCommonData (0xfb) residency | **LANDED (`45d5fead788`); runtime unverified** | `gITManagerCommonData` loads in `itManagerInitItems`; both asset rows and the address-shaped token row are in. **To close:** read `gNdsITCommonDataBytes` on a booted ROM and confirm 82,976, not 68. |
| P2-3c1 | Exact pose clock | **WIRED; runtime differential/cost owed** | Binary32 clock replaces Q12 timing (`f6f65a…`, `nds_f32_exact.h`); pose values stay Q12. Run `test_pose_clock_differential.py` live set through ROM oracle and measure cost. |

## Queue — P2-4 engineering

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-4s1..s8 | All eight VS stages | **REOPENED — owner rejects current result** | Missing backgrounds on all eight; geometry, moving platforms/collision, hazards, pipes and audio failures vary by stage. See verbatim `docs/BUGS.md`; verify source behavior and native routing before closing. |
| P2-4n1 | Native stage packet and actors | **40 packets connected; actor arms live; acceptance open** | Host tests pass. Actor arms run since the ground-vars union fix; acid keeps G_ZBUFFER (was painted in the foreground band). Barrel: native quad invisible, platform barrel is another drawer. Lakitu/Bronto and visual acceptance open (`docs/p2/BUG_NOTES.md`). |

## Queue — P2-5 items

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-5i1 | Item manager and twenty common items | **ALL 20 IN THE ROM** | Runtime acceptance remains. Fidelity/audio checkers and attack-event repair: `docs/p2/P2-5-items.md`. |
| P2-5i2 | The 13 Poke Ball Pokemon | **ALL 13 IN THE ROM** | Dispatch proved by `gNdsItMonsterMakerMask` = `1fff`, read off the table rather than from a roll: a ball opens only when thrown or hit, so a 60 s CPU match can spawn five and open none. Item particle effects are invisible (`gITManagerParticleBankID` has no pack) -- presentation, not gameplay. |
| P2-5i3 | Stage-spawned kinds | **7 OF 8 IN THE ROM; the 8th linked behind the 1P flag** | POW block, Piranha, Saffron's five Pokemon ship. The bonus-stage Target now links behind `NDS_P2_1P_GAME` (its providers landed with P2-6 step 5, 2026-09-04); it reaches the ROM when that flag does. |
| P2-5i4 | Pick up, throw, shoot and swing | **LANDED; acceptance open** | Live search/pickup/hold proved; source fixes and memory evidence: `docs/p2/P2-5-items.md`. |
| P2-5u1 | Item Switch and VS Options screens | **Native entry/row repair committed; acceptance open** | `eafdf226c52`: VS Mode → VS Options → Item Switch → VS Options → VS Mode passes. Row budgets and failed-blit retries pass actual-C tests. Cadence, settings coverage and wider regression remain. |
| P2-5x1 | Audio cue coverage | **SOURCE WIRED — ROM acceptance pending** | Current FGM header pins 573 entries / 6,874,344 bytes; the census covers all 47 BGM tracks and reports no missing cues. Hammer/Star playback and restoration now match BattleShip across 162,732 host cases; 17 census tests pass. Samus 246 remains source-unreachable. ROM playback and acoustic acceptance remain. Detail: `docs/p2/P2-5-items.md`. |
| P2-5a1 | Item TU fidelity audit | **CLEAN** | All 21 item TUs landed 2026-09-03/04 compared against their decomp originals line by line -- constants, operators, branch structure, status tables, loop bounds, call targets. No in-scope defect in any of them. |

## Queue — P2-2 performance debt

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-2p8 | Four-CPU renderer/performance, target `<1.12m` ticks | **ACTIVE; cache/performance red** | Current measured baseline above supersedes the old parked instrument. Preserve RAM floor while restoring cache engagement. Texture investigation: `docs/p2/P2-texture-residency.md`; older hypotheses require runtime confirmation. |

## Queue discipline

- Keep only red/current/deferred/owner-acceptance summaries here; move closed row detail out immediately.
- Keep each active row short enough to decide the next action without loading its historical investigation.
- Search owner docs/evidence for detail instead of expanding this board.
- After verified progress, update this queue and the owning evidence doc; do not duplicate the same result across restart surfaces.
