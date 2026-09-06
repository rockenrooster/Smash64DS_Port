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

SHA-256 0636B28D063ADA82F1FBE24F4EABA379469B696ACAA1FB725A2754E5F501CF66

5. Performance/visibility evidence is permanent under `artifacts/performance`
   and `artifacts/visibility`. Device A/B reports include 2/3/4/5+ VBlank
   histogram, max interval, and P50/P95.

## Phase status

| Phase | State | Gate summary |
|---|---|---|
| P2-1 VS shell | **full-minute integration runtime GREEN; publication pending** | The serial Mario/Fox match completes 3,600 timer ticks and Results, reserve 140,536 B, safety/stale/FGM misses zero, post-GO texture fence zero. `artifacts/verification/p2-shell-one-minute-coherent-ledger-20260906.txt`; ROM `792F7F0B2D818D04FB25FF5CF25299461647B7D01BA0EC3F549455A34A1E013D`. This predates the unbuilt Options admission. Prior shell lap/rematch passes with 34,500 B floor. Four-CPU acceptance remains red. |
| P2-2 Four-fighter engine | **real-items execution baseline and RAM floor verified; cache/performance RED** | Two items spawn across 1,972 samples / 59 s, all four native draw slots active; no allocator, panic or normalization failures. Floor 31,988 B >25,600; graphics peak 56/4,096 B. Cache hits/fills/wraps remain zero; WORK-H P95 2,808,768 exceeds target. Evidence: `artifacts/performance/2026-09-06_fourcpu-real-items-memory/`. Ending/Results and final acceptance remain open. |
| P2-3 Fighter production | **IN PROGRESS — ten of twelve ship; Ness and Kirby held** | Rung 8 (2026-09-04) adds Jigglypuff on P2-3f50/f51 closing. Held: Ness (EF block landed, `efmanager.c:1529`; smoke owed) and Kirby (heap, P2-3f47). Link PARTIAL. Owner-open: Pikachu ears — geometry closure CLEAN at both details, per-root matrix probe out. Yoshi's source-pair resolver is corrected; all six geometry closures now pass both details with negative controls (`fighters/yoshi.md`). CSS/stress acceptance remains open. Pose clock: see P2-3c1. |
| P2-4 Stage production | **40 native packets (8 VS, 5 arenas, 25 boards), blob-resident since `626f30b0c83`; background actors in (unbuilt)** | All 40 pass the checker; every stage but Dream Land loads its packet from NitroFS at stage start (`nds_native_stage_blob.c`). `efground.c` is in whole (Lakitu, Dedede, Ridley, birds, ships) on link 4. Boards registered and admitted (`e2e5c8bef5b`); the barrel cannon has its actor slot, six stage actors follow it. |
| P2-5 Items | **45 of 45 kinds in code (Target behind the 1P flag); runtime acceptance open** | Item Switch and VS Options have source asset coverage. The 22 imported screens now have no missing sprite geometry; 155 descriptors were added in `bfb35a3b6a7`. This proves drawable source assets, not final screen layout or native-renderer acceptance. |
| P2-6 1P Game | **campaign build/source-menu route live; CSS RAM blocks first battle** | All base/Polygon donors and Master Hand link. New lab `builds/resume-20260905/campaign-walk/smash64ds.nds` reaches Startup → Title → Main Menu → 1P Mode → source 1P CSS, then OOM while preloading twelve fighters; arena 782,336 B. Capture: `artifacts/verification/2026-09-06_p2-campaign.txt`. Earlier campaign startup passed; that did not prove gameplay. Shipping default remains `NDS_P2_1P_GAME=0`. |
| P2-7 Modes & meta | **source imports link in campaign; menu admission unbuilt** | Options/Screen Adjust/Backup Clear/Sound Test are now locally gated by shell OR campaign, preserving the remaining campaign gates. Source-menu entry clears stale BG layers once; extracted runtime tests and visual acceptance are in progress. No new public ROM. Intro cinematic remains owner-deferred. |

## Current integration checkpoint

The 2026-09-06 owner objective replaces continuous worker utilization with up to
eight useful helpers and at most two substantial slices awaiting integration.
**Critical path:** use the accepted offline match-resident packing approach to
restore animation-cache engagement and performance while retaining real items
and the now-green RAM floor. The standing 59-second window clears the mandatory
floor after duplicate metadata removal, compact renderer storage and the measured
graphics-scratch cut. Do not mistake this for ending/Results or final performance
acceptance. Runtime paging and RAM expansion remain excluded.

| Unit | SOURCE PRESENT | COMPILED/LINKED | RUNTIME VERIFIED | ACCEPTED |
|---|---|---|---|---|
| Four distinct fighters + real items | `e99db8cf004`, `e5ac85862f8`, `46a5aa33c52`, `21420ebd843` | Strong providers, ITEM_CORE=1, ROM `43829577…` | Standing window completes; 2 items, 31,988 B floor, all four draw slots | No: cache engagement, performance, ending/Results and visual acceptance owed |
| Shell memory/pacing evidence | `7244f63a95a` | Shell ROM hash above; embedded revision c3c37f7 plus integration dirty work | One-minute lifecycle passes; published snapshot flush fixes stale debugger reads | Instrument correction verified; full P2 acceptance open |
| Options source-menu handoff/admission | Local working tree | Human-input candidate built, real menu providers linked | Startup and 15 host menu tests pass; corrected screens not photographed | No |
| Compact 1P previews | Scratch builder + 12 binary prototypes | Source-derived closure self-checks pass; no runtime integration | Campaign CSS still OOM on prior lab | No: native root identity and loader integration owed |

Main owns live C/headers/Makefile integration and serialized stable-input builds.
Helper reviews are complete; capacity may stay idle. The item-padding investigation
was stopped with its results preserved (712 B candidate, not worth integration).
All twelve preview prototypes and maps are in `builds/resume-20260905/preview-compact`;
their original native root identities still need a runtime binding contract.
GLM resumed successfully for the focused graphics-reserve review.
**Next runnable checkpoint:** verify the candidate's source-menu route, then
integrate the compact preview loader without dropping native geometry or costumes;
continue the battle packing work against the recorded real-items baseline.

Human-input candidate: `builds/resume-20260905/playable-candidate-20260906/smash64ds.nds`,
SHA-256 `92FFD9A5E281D43C0EF207BB23C4629279F1DF6093EE36357E6BAADD0CBD4DB3`.
Title startup passed; menu walk=0, fast logic=0, campaign=0. The root ROM remains
unchanged. This candidate has not passed the widest profile or visual acceptance.
Failed/redundant runs are preserved beside the owning evidence: low-memory object
cap, the frame-365 unresolved Samus effect descriptor, and the relative-ROM-path
runner failure (fixed). One intermediate world-cache build was superseded before
running when the generation-domain defect was found.

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
| P2-4n1 | Native stage packet and actors | **40 packets connected; actor/runtime acceptance open** | Host loader/actor tests pass. Yoster vapor C initializers discarded most bytecode despite textual checks; fixed in `ad39124faa4`, with compiled-byte source comparison and negative control. Barrel native draw and Hyrule's full bank are committed; Yoster clouds/Lakitu/Bronto integration needs visual/VRAM acceptance. |

## Queue — P2-5 items

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-5i1 | Item manager core, and the twenty common items | **ALL 20 IN THE ROM** | `ndsItGetAttackEvent` was Link's-bomb-only and returned NULL for every other kind, so all four containers dereferenced it and the first detonation aborted the ARM9; it now decodes any kind against a decomp oracle. Checkers: `scripts/items/check-item-import-fidelity.py` and `scripts/check-audio-ordinals.py` (522). Detail and traps: `docs/p2/P2-5-items.md`. |
| P2-5i2 | The 13 Poke Ball Pokemon | **ALL 13 IN THE ROM** | Dispatch proved by `gNdsItMonsterMakerMask` = `1fff`, read off the table rather than from a roll: a ball opens only when thrown or hit, so a 60 s CPU match can spawn five and open none. Item particle effects are invisible (`gITManagerParticleBankID` has no pack) -- presentation, not gameplay. |
| P2-5i3 | Stage-spawned kinds | **7 OF 8 IN THE ROM; the 8th linked behind the 1P flag** | POW block, Piranha, Saffron's five Pokemon ship. The bonus-stage Target now links behind `NDS_P2_1P_GAME` (its providers landed with P2-6 step 5, 2026-09-04); it reaches the ROM when that flag does. |
| P2-5i4 | The fighter half of items | **LANDED** | Pick up, throw, shoot and swing -- four features that existed in the port and had no route to them (a weak stub, an empty shim, a `#if` on the wrong flag). Measured search=7 found=4 status=4 hold=4. A picked-up barrel never explodes, so the battle arena high-water fell 194,440 B and every pre-pickup arena figure is pessimistic. Shape and how to find the next one: `docs/p2/P2-5-items.md`. |
| P2-5u1 | Item Switch and VS Options screens | **SOURCE ASSET AUDIT GREEN — runtime lap owed** | Both ScreenSpecs and their native surface coverage pass. A scripted lap through VS Options to Item Switch and source-art visual comparison remain. |
| P2-5x1 | Audio cue coverage | **SOURCE WIRED — ROM acceptance pending** | Current FGM header pins 573 entries / 6,874,344 bytes; the census covers all 47 BGM tracks and reports no missing cues. Hammer/Star playback and restoration now match BattleShip across 162,732 host cases; 17 census tests pass. Samus 246 remains source-unreachable. ROM playback and acoustic acceptance remain. Detail: `docs/p2/P2-5-items.md`. |
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
