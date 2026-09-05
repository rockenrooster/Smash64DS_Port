# P2 Execution Board

Created: 2026-08-17.
Updated: 2026-09-02 (token-efficiency cleanup; active queue only, historical detail stays with archive/evidence owners. Pikachu and Yoshi landed opt-in -- rows P2-3f34..f45 in the archive; P2-3f46 open; P2-3f47 roster close in progress.)

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

SHA-256 0636B28D063ADA82F1FBE24F4EABA379469B696ACAA1FB725A2754E5F501CF66

5. Performance/visibility evidence is permanent under `artifacts/performance`
   and `artifacts/visibility`. Device A/B reports include 2/3/4/5+ VBlank
   histogram, max interval, and P50/P95.

## Phase status

| Phase | State | Gate summary |
|---|---|---|
| P2-1 VS shell | **green; owner acceptance pending** | Closed; history archived. |
| P2-2 Four-fighter engine | **NOT green — 2.4x over on WORK; arm is live, not parked** | The 2,238,464 figure was `ALL` (VBlank-quantised: exactly 4x559,616) on mirrors — never a cost readout. Last real run (`artifacts/performance/2026-09-01_bug-fourcpu-relative-fastpath-full/`, Samus/Fox/Captain/Donkey, whole match): **WORK P50 1,560,672 / P95 2,697,209**, cadence 3.29% two-VBlank. P50 is already 1.39x over: not a tail problem. Four distinct kinds DO fit (whole 60 s matches, heap floor met). "Parked" is a label only: `p2_fourcpu_stress` is in both registry profiles and its ROM is on disk. Arm defect: `Makefile:2526` forces `NDS_DEV_LIVE_INPUT_PREVIEW`, which zeroes items — the gate says items ON. Both external reviews cite the stale 08-27 artifact; their two named blockers closed 09-01 (STG P95 4,098,368 -> 167,808). Pack estimator: typed object index landed (`estimate_fighter_pack.py`, `82850745ad2`); the capacity bound is not. |
| P2-3 Fighter production | **IN PROGRESS — ten of twelve ship; Ness and Kirby held** | Rung 8 (2026-09-04) adds Jigglypuff on P2-3f50/f51 closing. Held: Ness (EF block landed, `efmanager.c:1529`; smoke owed) and Kirby (heap, P2-3f47). Link PARTIAL. Owner-open: Pikachu ears — geometry closure CLEAN at both details, per-root matrix probe out. Yoshi's source-pair resolver is corrected; all six geometry closures now pass both details with negative controls (`fighters/yoshi.md`). CSS/stress acceptance remains open. Pose clock: see P2-3c1. |
| P2-4 Stage production | **all 8 selectable; four layer packets wired in source** | Dream Land, Yoster, Castle and Jungle pass packet/source checks. Matrix kind 48 now uses source Mod1/MVP behavior; camera bindings bypass affine caching/replay. Runtime remains unbuilt. The five remaining stages need DLLinks/head-order support. Native actors and acceptance remain open. Detail: `docs/p2/P2-4-stage-production.md`. |
| P2-5 Items | **45 of 45 kinds in code (Target behind the 1P flag); Item Switch built** | 20 common, 13 Pokemon, 7 of 8 stage-spawned. Item Switch has its TU (`nds_menu_shell_items.c`, 234 lines) and 79 baked `ITEM_SWITCH` surfaces. A 2026-09-04 row calling it an empty backdrop was WRONG (guessed filename). It has no ScreenSpec, so its art is unaudited like VS Options was. Stress arm must be items ON (gate); it is items OFF as built — see P2-2. |
| P2-6 1P Game | **~15% — steps 1 and 5 imported behind the flag (unbuilt)** | `NDS_P2_1P_GAME ?= 0`, no target sets it. Step 1 driver (manager loop, spawn/team-next, Link/Hyrule bridge) and step 5 bonus stages landed; the other scenes stay stubs. Blockers are in the commits, plus: the runtime TU's `ndsExcluded*` table/bonus-counter copies must be unified with the real ones. |
| P2-7 Modes & meta | **~10% — save data landed (unbuilt)** | Item 1: `nds_backup.c` FAT-backed SRAM image + transcribed `lbbackup.c`; loads at scmanager start, atomic file replace, counters. Masks still forced open in the harness (item 9). Eleven scenes stubbed at `title_backend.c:412-447`; every other row of `docs/p2/P2-7-modes-meta.md:88-153` reads NOT PRESENT. |

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
| P2-3f47 | Roster close: Ness, Jigglypuff and Kirby admitted opt-in | **IN PROGRESS** | Kirby both-CPU smoke RED 2026-09-04, **not** Jigglypuff's fixup defect: `ndsSyMallocOverflowHalt` on Fox's 115,440 after Kirby's own setup succeeds. Not the arena: a same-tree Mario/Fox control plays at 1,319,008 vs 1,318,912. No room is left for a second fighter. Remaining: his heap cost, Jigglypuff's fixup, Ness smoke, CSS capture, Boundary, stress arm with P2-3f46. Detail: `docs/p2/fighters/{ness,jigglypuff,kirby}.md`. |
| P2-3f48 | ITCommonData (0xfb) residency | **LANDED (`45d5fead788`); runtime unverified** | `gITManagerCommonData` loads in `itManagerInitItems`; both asset rows and the address-shaped token row are in. **To close:** read `gNdsITCommonDataBytes` on a booted ROM and confirm 82,976, not 68. |
| P2-3c1 | Pose clock: Q12 shifted landing-lag End ticks ±1 on shipped fighters | **WIRED EXACT (unbuilt); cost at the final pass** | Differential (`test_pose_clock_differential.py`) showed Mario/Fox 635 live mismatches (Fox dair/uair End 40→41, Mario SJP landing 26→25) and no Q closes it. Fix landed `f6f65a…`+: the clock is binary32 bits stepped by `nds_f32_exact.h` (proven 0/1.14G ops vs the host IEEE adder), ~20 sites in `nds_ft_pose.c`, pose values stay Q12. Revert is one commit. Final pass: re-run the differential's live set through the ROM oracle, measure the ~4K tk/fr estimate. |

## Queue — P2-4 engineering

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-4s1..s8 | All eight VS stages (Yoster, Castle, Jungle, Zebes, Hyrule, Yamabuki, Inishie, Sector) | **BOOT AND PLAY — full scripted lap each, stage identity asserted** | Swept on the all-stages ROM with `-TargetGkind`, which fails the run unless the battle loaded the requested stage. Remaining for every one: native packet (P2-4n1). |
| P2-4n1 | Native stage packet, all stages | **7/9 runtime-wired, unbuilt; Zebes/Yamabuki in flight** | Sector/Hyrule DLLink packets registered, captured per link+head with packet identity checks, executed head-ordered 0,2,1,3 per segment (step 7, `docs/p2/P2-4-stage-production.md`). Accepted delta: cross-owner head interleave within a camera group. Inishie landed (`7b04b5429fb`; gkind 8, tables now 9); Zebes (GLM: material decoding) and Yamabuki are agent tasks. Final pass: compile + admission on all six. |

## Queue — P2-5 items

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-5i1 | Item manager core, and the twenty common items | **ALL 20 IN THE ROM** | `ndsItGetAttackEvent` was Link's-bomb-only and returned NULL for every other kind, so all four containers dereferenced it and the first detonation aborted the ARM9; it now decodes any kind against a decomp oracle. Checkers: `scripts/items/check-item-import-fidelity.py` and `scripts/check-audio-ordinals.py` (522). Detail and traps: `docs/p2/P2-5-items.md`. |
| P2-5i2 | The 13 Poke Ball Pokemon | **ALL 13 IN THE ROM** | Dispatch proved by `gNdsItMonsterMakerMask` = `1fff`, read off the table rather than from a roll: a ball opens only when thrown or hit, so a 60 s CPU match can spawn five and open none. Item particle effects are invisible (`gITManagerParticleBankID` has no pack) -- presentation, not gameplay. |
| P2-5i3 | Stage-spawned kinds | **7 OF 8 IN THE ROM; the 8th linked behind the 1P flag** | POW block, Piranha, Saffron's five Pokemon ship. The bonus-stage Target now links behind `NDS_P2_1P_GAME` (its providers landed with P2-6 step 5, 2026-09-04); it reaches the ROM when that flag does. |
| P2-5i4 | The fighter half of items | **LANDED** | Pick up, throw, shoot and swing -- four features that existed in the port and had no route to them (a weak stub, an empty shim, a `#if` on the wrong flag). Measured search=7 found=4 status=4 hold=4. A picked-up barrel never explodes, so the battle arena high-water fell 194,440 B and every pre-pickup arena figure is pessimistic. Shape and how to find the next one: `docs/p2/P2-5-items.md`. |
| P2-5u1 | Item Switch and VS Options screens | **LANDED — lap and art audit owed** | VS Options (`7e69c9a66d1`, opened from `mode_vs.c:617`) and the three-cell Item Switch cursor (`936d37a0fd0`) are linked in the shell ELF; scene registered `nds_scene_manager.c:72`. Owed: a scripted lap through VS Options to Item Switch; `item_switch` ScreenSpec audit (agent out). |
| P2-5x1 | FGM cue coverage | **SOURCE WIRED — final render pending** | 86 additional cues, 495 total; selector factories and full-program dispatch are wired. Seven source regression controls pass; old bank hashes unchanged. Samus 246 is an unreachable held-loop entry. **Final render must repin `nds_audio_fgm.h` count 495, bytes and hashes.** SpearSwarm's required 16 kHz output awaits acoustic acceptance. Detail: `docs/p2/P2-5-items.md`. |
| P2-5a1 | Item TU fidelity audit | **CLEAN** | All 21 item TUs landed 2026-09-03/04 compared against their decomp originals line by line -- constants, operators, branch structure, status tables, loop bounds, call targets. No in-scope defect in any of them. |

## Queue — P2-2 performance debt

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-2p8 | Four-CPU renderer/performance repair, target `<1.12m` ticks | **PARKED BY OWNER; this is the blocked measurement instrument** | `docs/reviews/4Fighter_optimization.md`, but two of its three broken routes do not hold — verify before acting. `reason 6` names nothing: it is the outer code at `renderer_adapter_stage.c:3218` for 'inner owner returned FALSE'. Probe theory: four fighters exhaust the 79 dynamic texture slots so the STAGE loses its native path. See `docs/p2/P2-texture-residency.md`. |

## Queue discipline

- Keep only red/current/deferred/owner-acceptance summaries here; move closed row detail out immediately.
- Keep each active row short enough to decide the next action without loading its historical investigation.
- Search owner docs/evidence for detail instead of expanding this board.
- After verified progress, update this queue and the owning evidence doc; do not duplicate the same result across restart surfaces.
