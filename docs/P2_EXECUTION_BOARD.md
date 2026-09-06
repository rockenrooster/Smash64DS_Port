# P2 Execution Board

Created: 2026-08-17.
Updated: 2026-09-06 (real-items four-fighter window clears RAM; cache/performance remain red).

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
| P2-1 VS shell | **shell/battle runtime GREEN; test ROM published** | Battle arm passes. Fixed shell lap/rematch: 10 entries, 76,580 B floor, zero faults. Results cache waits for setup; recovery 258,048 B (`9eca1e7054a`). Evidence: `artifacts/performance/2026-09-06_shell-results-floor/`. Root ROM passes startup/Options route; four-CPU cache/performance remains red. |
| P2-2 Four-fighter engine | **real-items execution baseline and RAM floor verified; cache/performance RED** | Two items spawn across 1,972 samples / 59 s, all four native draw slots active; no allocator, panic or normalization failures. Floor 31,988 B >25,600; graphics peak 56/4,096 B. Cache hits/fills/wraps remain zero; WORK-H P95 2,808,768 exceeds target. Evidence: `artifacts/performance/2026-09-06_fourcpu-real-items-memory/`. Ending/Results and final acceptance remain open. |
| P2-3 Fighter production | **IN PROGRESS — nine enabled in the current public ROM** | The compiled config has Ness/Purin/Kirby=0; Purin has opt-in smoke evidence but is not enabled by default. Ness smoke, Kirby heap, Link integration, Pikachu ears and roster acceptance remain. Details: `docs/p2/fighters/`; pose clock: P2-3c1. |
| P2-4 Stage production | **40 native packets (8 VS, 5 arenas, 25 boards), blob-resident since `626f30b0c83`; background actors in (unbuilt)** | All 40 pass the checker; every stage but Dream Land loads its packet from NitroFS at stage start (`nds_native_stage_blob.c`). `efground.c` is in whole (Lakitu, Dedede, Ridley, birds, ships) on link 4. Boards registered and admitted (`e2e5c8bef5b`); the barrel cannon has its actor slot, six stage actors follow it. |
| P2-5 Items | **45 of 45 kinds in code (Target behind the 1P flag); runtime acceptance open** | Item Switch and VS Options have source asset coverage. All 22 imported screens have sprite geometry rows (`bfb35a3b6a7`). This proves staging/normalization metadata, not blitter admission, layout or rendered pixels. |
| P2-6 1P Game | **CSS native rendering verified; battle RAM RED** | Source generation pushed (`87c6be2549b`); local CSS renders selected Link with 12 packs (198,980 B), 66,972 B free, zero pack/GX overflow faults. Evidence: `artifacts/performance/2026-09-06_1p-css-native/`. Battle needs 105,152 B with 98,100 B free. Intro actors/full roster tour remain. Shipping `NDS_P2_1P_GAME=0`. |
| P2-7 Modes & meta | **Options route verified/published** | Root ROM enters Options, toggles/restores sound, visits Screen Adjust and Backup Clear, returns to Main Menu. Source fill/IA4 repairs committed (`68c0e522d3c`, `f33c5aa039f`). Sound Test links but Data routing remains gated. Intro cinematic owner-deferred. |

## Current integration checkpoint

**Critical path:** offline match-resident packing must restore animation-cache
engagement and performance while preserving real items and the RAM floor.
Runtime paging and RAM expansion remain excluded. Up to eight useful helpers;
at most two substantial slices awaiting integration (owner objective 2026-09-06).

| Unit | SOURCE PRESENT | COMPILED/LINKED | RUNTIME VERIFIED | ACCEPTED |
|---|---|---|---|---|
| Four distinct fighters + real items | `e99db8cf004`, `e5ac85862f8`, `46a5aa33c52`, `21420ebd843` | Strong providers, ITEM_CORE=1, ROM `43829577…` | Standing window completes; 2 items, 31,988 B floor, all four draw slots | No: cache engagement, performance, ending/Results and visual acceptance owed |
| Shell memory/pacing evidence | `7244f63a95a` | Shell ROM hash above; embedded revision c3c37f7 plus integration dirty work | One-minute lifecycle passes; published snapshot flush fixes stale debugger reads | Instrument correction verified; full P2 acceptance open |
| Options source-menu handoff/admission | `68c0e522d3c`, `f33c5aa039f` | Root human-input ROM, strong providers | Startup/nine-entry route, 23 host cases, shell/battle checks pass | Test-ready; remaining modes/visual acceptance open |
| Compact 1P previews | Producer `87c6be2549b`; local loader/bridge | Twelve FPC packs + native root/image remapping compiled; source and actual-C tests pass | Link selected preview and menu render, 66,972 B free; onward battle OOM | No: full roster/costume tour, Intro rendering and campaign gates remain |

Main owns live integration and serialized builds. Preview artifacts/review:
`builds/resume-20260905/{preview-compact,preview-binding}`. The prior zero-filled
Selected section was a prototype defect; repaired bytes do not make the bins runtime-ready.
**Next:** integrate compact preview loader/root identity without losing geometry,
costumes or motion-table/particle setup; then continue battle packing.

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
| P2-4s1..s8 | All eight VS stages | **BOOT AND PLAY — full scripted lap each** | Stage identity asserted by `-TargetGkind`; native actor/visual acceptance remains under P2-4n1. |
| P2-4n1 | Native stage packet and actors | **40 packets connected; actor/runtime acceptance open** | Host loader/actor tests pass. Yoster vapor C initializers discarded most bytecode despite textual checks; fixed in `ad39124faa4`, with compiled-byte source comparison and negative control. Barrel native draw and Hyrule's full bank are committed; Yoster clouds/Lakitu/Bronto integration needs visual/VRAM acceptance. |

## Queue — P2-5 items

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-5i1 | Item manager and twenty common items | **ALL 20 IN THE ROM** | Runtime acceptance remains. Fidelity/audio checkers and attack-event repair: `docs/p2/P2-5-items.md`. |
| P2-5i2 | The 13 Poke Ball Pokemon | **ALL 13 IN THE ROM** | Dispatch proved by `gNdsItMonsterMakerMask` = `1fff`, read off the table rather than from a roll: a ball opens only when thrown or hit, so a 60 s CPU match can spawn five and open none. Item particle effects are invisible (`gITManagerParticleBankID` has no pack) -- presentation, not gameplay. |
| P2-5i3 | Stage-spawned kinds | **7 OF 8 IN THE ROM; the 8th linked behind the 1P flag** | POW block, Piranha, Saffron's five Pokemon ship. The bonus-stage Target now links behind `NDS_P2_1P_GAME` (its providers landed with P2-6 step 5, 2026-09-04); it reaches the ROM when that flag does. |
| P2-5i4 | Pick up, throw, shoot and swing | **LANDED; acceptance open** | Live search/pickup/hold proved; source fixes and memory evidence: `docs/p2/P2-5-items.md`. |
| P2-5u1 | Item Switch and VS Options screens | **SOURCE ASSET AUDIT GREEN — runtime lap owed** | Both ScreenSpecs and their native surface coverage pass. A scripted lap through VS Options to Item Switch and source-art visual comparison remain. |
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
