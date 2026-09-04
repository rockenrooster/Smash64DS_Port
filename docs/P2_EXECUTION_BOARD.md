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
| P2-2 Four-fighter engine | **automated green; blocked on RAM for four DISTINCT kinds** | Semantics, HUD, camera, Results, Sudden Death landed. Four-kind resident cost is the open problem; architecture settled in `docs/p2/P2-2-pack-estimator.md`. Next step is the census estimator, not runtime code. |
| P2-3 Fighter production | **IN PROGRESS — ten of twelve ship; Ness and Kirby held** | Rung 8 (2026-09-04) adds Jigglypuff on P2-3f50/f51 closing. Held: Ness (no EF roster desc block, `battleship_efmanager.c:1528-1536`) and Kirby (heap, P2-3f47). Link PARTIAL. Owner-open: Pikachu ears, Yoshi CSS preview, both draw-time; cross-palette-slot width is REFUTED at high confidence, see `docs/p2/fighters/pikachu.md`. |
| P2-4 Stage production | **all 8 ship, but ONLY DREAM LAND HAS NATIVE GEOMETRY** | One cause behind the owner's missing-geometry reports, verified 2026-09-04: `scripts/stages/native_stage_descriptors/` holds only `dreamland.py` and `yoster.py`, and `renderer_adapter_matrix.c:514-525` binds all eight gkind arms to the Dream Land descriptor, so every other stage mismatches its asset ids, takes reject reason 3 and draws zero native triangles. Sector Z is worst because it is largest. THE WORK IS PER-STAGE DESCRIPTORS. Also: Sector Z and Congo have no `nds_audio_bgm.c` row. |
| P2-5 Items | **IN PROGRESS — 44 of 45 kinds; Item Switch IS built** | 20 common, 13 Pokemon, 7 of 8 stage-spawned. Item Switch has its TU (`nds_menu_shell_items.c`, 234 lines) and 79 baked `ITEM_SWITCH` surfaces. A 2026-09-04 row calling it an empty backdrop was WRONG (guessed filename). It has no ScreenSpec, so its art is unaudited like VS Options was. Stress = items ON. |
| P2-6 1P Game | queued | Campaign start-to-credits. |
| P2-7 Modes & meta | queued | Fresh-cart parity and P2 close gate. |

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

## Queue — P2-4 engineering

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-4s1 | Yoshi's Island (Yoster), stage 1 | **BOOTS AND PLAYS - full scripted lap, stage identity asserted** | Swept on the all-stages ROM with `-TargetGkind`, which fails the run unless the battle loaded the requested stage. Remaining: native packet (P2-4n1). |
| P2-4s2 | Peach's Castle (Castle), stage 2 | **BOOTS AND PLAYS - full scripted lap, stage identity asserted** | Swept on the all-stages ROM with `-TargetGkind`, which fails the run unless the battle loaded the requested stage. Remaining: native packet (P2-4n1). |

| P2-4s3 | Congo Jungle (Jungle), stage 3 | **BOOTS AND PLAYS - full scripted lap, stage identity asserted** | Swept on the all-stages ROM with `-TargetGkind`, which fails the run unless the battle loaded the requested stage. Remaining: native packet (P2-4n1). |
| P2-4n1 | Native stage packet, all stages | **OPEN - generator descriptor threading in progress** | The generator, the runtime adapter and the checker are all pinned to Dream Land's counts. Step 1 (thread a stage descriptor through the generator, Dream Land output byte-identical) is delegated and `scripts/stages/native_stage_descriptors/` exists. Detail: `docs/p2/P2-4-stage-production.md:297`. |


| P2-4s4 | Planet Zebes (Zebes), stage 4 | **BOOTS AND PLAYS - full scripted lap, stage identity asserted** | Swept on the all-stages ROM with `-TargetGkind`, which fails the run unless the battle loaded the requested stage. Remaining: native packet (P2-4n1). |
| P2-4s5 | Hyrule Castle (Hyrule), stage 5 | **BOOTS AND PLAYS - full scripted lap, stage identity asserted** | Swept on the all-stages ROM with `-TargetGkind`, which fails the run unless the battle loaded the requested stage. Remaining: native packet (P2-4n1). |
| P2-4s6 | Saffron City (Yamabuki), stage 6 | **BOOTS AND PLAYS - full scripted lap, stage identity asserted** | Swept on the all-stages ROM with `-TargetGkind`, which fails the run unless the battle loaded the requested stage. Remaining: native packet (P2-4n1). |
| P2-4s7 | Mushroom Kingdom (Inishie), stage 7 | **BOOTS AND PLAYS - full scripted lap, stage identity asserted** | Swept on the all-stages ROM with `-TargetGkind`, which fails the run unless the battle loaded the requested stage. Remaining: native packet (P2-4n1). |
| P2-4s8 | Sector Z (Sector), stage 8 | **BOOTS AND PLAYS - full scripted lap, stage identity asserted** | Swept on the all-stages ROM with `-TargetGkind`, which fails the run unless the battle loaded the requested stage. Remaining: native packet (P2-4n1). |

## Queue — P2-5 items

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-5i1 | Item manager core, and the twenty common items | **ALL 20 IN THE ROM** | `ndsItGetAttackEvent` was Link's-bomb-only and returned NULL for every other kind, so all four containers dereferenced it and the first detonation aborted the ARM9; it now decodes any kind against a decomp oracle. Checkers: `scripts/items/check-item-import-fidelity.py` and `scripts/check-audio-ordinals.py` (522). Detail and traps: `docs/p2/P2-5-items.md`. |
| P2-5i2 | The 13 Poke Ball Pokemon | **ALL 13 IN THE ROM** | Dispatch proved by `gNdsItMonsterMakerMask` = `1fff`, read off the table rather than from a roll: a ball opens only when thrown or hit, so a 60 s CPU match can spawn five and open none. Item particle effects are invisible (`gITManagerParticleBankID` has no pack) -- presentation, not gameplay. |
| P2-5i3 | Stage-spawned kinds | **7 OF 8 IN THE ROM** | POW block, Piranha, and Saffron City's five Pokemon. The bonus-stage Target compiles but is not linked: it needs `sc1PBonusStageUpdateTargetCount` and `gSC1PBonusStageItemFile`, which are P2-6 scene state. Phase boundary, not a gap. |
| P2-5i4 | The fighter half of items | **LANDED** | Pick up, throw, shoot and swing -- four features that existed in the port and had no route to them (a weak stub, an empty shim, a `#if` on the wrong flag). Measured search=7 found=4 status=4 hold=4. A picked-up barrel never explodes, so the battle arena high-water fell 194,440 B and every pre-pickup arena figure is pessimistic. Shape and how to find the next one: `docs/p2/P2-5-items.md`. |
| P2-5u1 | Item Switch and VS Options screens | **AMBER -- Item Switch built, not yet reachable** | Screen module `src/nds/nds_menu_shell_items.c` landed with its 37 surfaces, screen ids 5/6 reserved, `nSCKindVSItemSwitch` wired (it was an `NDS_SCENE_STUB` that parked the thread), and the commit rule and `damage_ratio` in place. TWO things remain: the cursor, 146x10 after the 4/5 scale and so wider than a DS OBJ cell, being split into three abutting cells; and the VS OPTIONS screen between, whose art and module are outstanding -- its OPTIONS row still refuses (`nds_menu_shell_mode_vs.c:612`), so nothing reaches Item Switch yet. Verified as the no-op it should be: e5=0 e6=0, existing screen counters unchanged. |
| P2-5a1 | Item TU fidelity audit | **CLEAN** | All 21 item TUs landed 2026-09-03/04 compared against their decomp originals line by line -- constants, operators, branch structure, status tables, loop bounds, call targets. No in-scope defect in any of them. |

## Queue — P2-2 performance debt

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-2p8 | Four-CPU renderer/performance repair, target `<1.12m` ticks | **PARKED BY OWNER; reviewed and probed 2026-09-04** | `docs/reviews/4Fighter_optimization.md`, but verify first: two of its three broken routes do not hold. `battlePackResidentBytes=0` is by design (`reloc_backend_assets.c:9133` needs Fox AND `distinct<=2`); the 163,840 B cache is the behind-pack constant and `:9236` defaults to 258,048 on decline. **`reason 6` names nothing** — it is the outer code at `renderer_adapter_stage.c:3218` for 'inner owner returned FALSE'; the inner reason needs `NDS_TASK36_REJECT_TRACE`. Probe chain: texture resolve in `native_owners.c:1043-1055` emits PrepareRun 2, bubbling as `300+run`; `textures_effects.c:8224-8226` names `342 -> PrepareRun 2 -> resolve refusing`. Theory: four fighters exhaust 79 dynamic texture slots so the STAGE loses its native path. See `docs/reviews/Ask_ds_texture_residency.md`. |

## Queue discipline

- Keep only red/current/deferred/owner-acceptance summaries here; move closed row detail out immediately.
- Keep each active row short enough to decide the next action without loading its historical investigation.
- Search owner docs/evidence for detail instead of expanding this board.
- After verified progress, update this queue and the owning evidence doc; do not duplicate the same result across restart surfaces.
