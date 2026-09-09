# P2 Execution Board

Created: 2026-08-17.
Updated: 2026-09-09 (ten native owners landed; **9 of 9 stages AND 9 of 9
fighters read ZERO native failures**, from ~2,629 that morning, both waves on
one ROM. Causes and contracts are in docs/p2/BUG_NOTES.md).

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

SHA-256 2CB6B86242F9BF2B0CF8D99FF0405C1C4F87DE38F1A03AA51D3514BED421DF99

5. Performance/visibility evidence is permanent under `artifacts/performance`
   and `artifacts/visibility`. Device A/B reports include 2/3/4/5+ VBlank
   histogram, max interval, and P50/P95.

## Phase status

| Phase | State | Gate summary |
|---|---|---|
| P2-1 VS shell | **VS Mode reference; VS Options visuals accepted** | `eafdf226c52` connects native VS Options/Item Switch entries; owner accepts them visually, round trip and 9 host cases pass. 09-08: `p2_shell_loop`'s sprite layer is clean (11 failures to 0), the arm red only on a Dream Land MObjSub attachment decline. Evidence: `artifacts/performance/2026-09-06_vs-options-bundle/`. |
| P2-2 Four-fighter engine | **RAM cliff and performance RED** | **"Wander-crashed after frame 256" is RETRACTED -- no such log exists.** The one captured four-fighter crash is at **frame 45** (`builds/resume-20260907/boundary.err.txt:3414-3433`): a NULL store in `ifCommonEntryAllThread` with `r4 = 0x0`, alongside `TICKFAULT_GOBJ active=58 max=59 free=12164` -- i.e. the GObj cap latch has fired (`ifcommon.c:3156-3163` latches below 25,600 B free, `:2299-2305` derefs NULL). The 16,348 B floor regression: `632813ab3d6` raised `NDS_AOBJ_EVENT32_NORMALIZED_MAX` 3072 to 4096. WORK-H P95 2,808,768 exceeds target. Ending/Results and final acceptance remain open. |
| P2-3 Fighter production | **ALL NINE FIGHTERS ZERO NATIVE FAILURES (09-09)** | 09-09, 1,200 presents: `native failures=0` across all nine. Battle acceptance, Ness smoke, Kirby heap and roster acceptance remain. Details: `docs/p2/fighters/`; pose clock: P2-3c1. |
| P2-4 Stage production | **ALL NINE STAGES ZERO NATIVE FAILURES (09-09); owner visual rows open** | 09-09: `native failures=0` across all nine. Three same-day RETRACTIONS, all `DIAG_OWNERTRI` misreadings or scope slips. **SUBMITTED-BUT-INVISIBLE is now a five-surface family**: Castle roof, Yoster, Mushroom Kingdom bricks, Zebes shafts and Congo's barrel (2 tris/frame, 0 rejects, never on screen). A cross-stage comparison of 41 losing vs 6 drawing runs found NO discriminating field and both follow-up candidates then died -- `othermode_l == 0` evaluates identically to `G_RM_ZB_OPA_SURF` in every consumer (148/533 runs carry zero, including drawing ones), and the Castle binding record carries no render state. Mushroom Kingdom studied alone (HIGH): g25-g27 are the right pipe's brick pedestal, correctly placed, unconditional static layer-1, **lost in or after commit**. The per-run `Emitted`-vs-`Given` witness now exists and is the next reading. |
| P2-5 Items | **DRAW is the blocker: 11 of 45 kinds have owners, ~30 remain** | Spawn law, switch mask, frequency and the mball chain are live and source-faithful, 45/45 makers registered; an ownerless item spawns and records NO_PROGRAM instead of appearing. Pickup FileIDs (were all `0u`) are resolved. A pipeline study (HIGH) prices the remaining ~30: **20-25 fit existing generator templates as table rows unchanged, ~5 need a shape parameter, 0-2 bespoke** -- so the backlog is mechanical, not thirty investigations. Two batches shipped with no Makefile rules or object prerequisites and a clean-checkout build had to find it. |
| P2-6 1P Game | **PAUSED BY OWNER** | CSS pushed (`d9161127d46`). Local integration reaches Intro and Link/Hyrule play after GO, 8,356 B free; memory margin and campaign acceptance remain red. Shipping `NDS_P2_1P_GAME=0`; resume only on owner request. |
| P2-7 Modes & meta | **Options/Backup Clear visuals accepted; validation open; Data inaccessible** | Owner (09-06): VS Options, Option and Backup Clear look good; native route, cancellation and host confirmation/clear tests pass. Cadence and disposable-save persistence need verification. |

## Current integration checkpoint

**Critical path:** the native-only boundary now reads zero failures on every
stage and fighter, so the path is match repairs in owner order; DATA children
last. Owner reports in `docs/BUGS.md` are the authoritative symptoms. Main
Menu/VS Mode are the accepted menu references; 1P stays paused.

| Unit | SOURCE PRESENT | COMPILED/LINKED | RUNTIME VERIFIED | ACCEPTED |
|---|---|---|---|---|
| Four distinct fighters + real items | `e99db8cf004`, `e5ac85862f8`, `46a5aa33c52`, `21420ebd843` | Strong providers, ITEM_CORE=1 | 2 items, 31,988 B floor, all four draw slots | No: cache engagement, performance, ending/Results owed |
| Shell memory/pacing evidence | `7244f63a95a` | Shell ROM hash above | One-minute lifecycle passes; snapshot flush fixes stale debugger reads | Instrument correction verified; P2 acceptance open |
| Options source-menu handoff/admission | `68c0e522d3c`, `f33c5aa039f` | Root human-input ROM | Startup/nine-entry route, 23 host cases, shell/battle checks pass | Test-ready; remaining modes/visual acceptance open |
| Compact 1P previews | Producer `87c6be2549b`; local loader/bridge | Twelve FPC packs compiled; source and actual-C tests pass | Link preview and menu render, 66,972 B free; onward battle OOM | No: roster tour, Intro rendering and campaign gates remain |

Main owns live integration and serialized builds. Preview artifacts:
`builds/resume-20260905/{preview-compact,preview-binding}`; repaired bytes in
the once zero-filled Selected section do not make those bins runtime-ready.
**1P development is paused by the owner**; preserve local campaign integration.

Root `smash64ds.nds` republished 2026-09-09 (51,454,976 B), hash above re-pinned
to it, carrying the angle range reduction and all ten 09-09 native owners.

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
| P2-4n1 | Native stage packet and actors | **38 blob packets plus Dream Land linked; acceptance open** | Host tests pass. Actor arms run since the ground-vars union fix; acid keeps G_ZBUFFER. Barrel submits but is unproved on screen. Lakitu/Bronto and visual acceptance open (`docs/p2/BUG_NOTES.md`). |

## Queue — P2-5 items

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-5i1 | Item manager and twenty common items | **ALL 20 IN THE ROM** | Runtime acceptance remains. |
| P2-5i2 | The 13 Poke Ball Pokemon | **ALL 13 IN THE ROM; draw owners missing** | Dispatch proved by `gNdsItMonsterMakerMask` = `1fff`, read off the table: a ball opens only when thrown or hit, so a 60 s CPU match can spawn five and open none. |
| P2-5i3 | Stage-spawned kinds | **8 OF 10 IN THE ROM; two behind the 1P flag** | POW, Piranha and Saffron's five ship; Target and TaruBomb behind `NDS_P2_1P_GAME`. Native owners exist for 1 of 42 distinct item shapes. |
| P2-5i4 | Pick up, throw, shoot and swing | **LANDED; acceptance open** | Live search/pickup/hold proved; source fixes and memory evidence: `docs/p2/P2-5-items.md`. |
| P2-5u1 | Item Switch and VS Options screens | **Native entry/row repair committed; acceptance open** | `eafdf226c52`: VS Mode → VS Options → Item Switch → VS Options → VS Mode passes. Row budgets and failed-blit retries pass actual-C tests. Cadence, settings coverage and wider regression remain. |
| P2-5x1 | Audio cue coverage | **SOURCE WIRED — ROM acceptance pending** | FGM header pins 573 entries / 6,874,344 bytes; the census covers all 47 BGM tracks with no missing cues. Hammer/Star playback matches BattleShip across 162,732 host cases; 17 tests pass. Samus 246 is source-unreachable. ROM playback acceptance remains. |
| P2-5a1 | Item TU fidelity audit | **CLEAN** | All 21 item TUs compared line by line against their decomp originals 2026-09-03/04: constants, operators, branch structure, status tables, loop bounds, call targets. No in-scope defect. |

## Queue — P2-2 performance debt

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-2p8 | Four-CPU renderer/performance, target `<1.12m` ticks | **ACTIVE; cache/performance red** | Current measured baseline above supersedes the old parked instrument. Preserve RAM floor while restoring cache engagement. Texture investigation: `docs/p2/P2-texture-residency.md`; older hypotheses require runtime confirmation. |

## Queue discipline

- Keep only red/current/deferred/owner-acceptance summaries here; move closed row detail out immediately.
- Keep each active row short enough to decide the next action without loading its historical investigation.
- Search owner docs/evidence for detail instead of expanding this board.
- After verified progress, update this queue and the owning evidence doc; do not duplicate the same result across restart surfaces.
