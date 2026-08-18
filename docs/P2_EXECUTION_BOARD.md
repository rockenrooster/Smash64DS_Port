# P2 Execution Board

Created: 2026-08-17. **The only dynamic queue.** Plans live in
`docs/P2_PLAN.md` + `docs/p2/`; this board is what is actually next. Closed
work goes to `docs/archive/` (dated section), not back onto this board. The
P1 board is archived at `docs/archive/P1_EXECUTION_BOARD.md`.

## Standing rules (carried from P1 — still law)

1. **Measurement law** lives in `docs/VERIFYING.md` + this section. Boundary
   today is still `battle_playable_realtime`, mode `163` (Mario vs level-3
   CPU Fox, Dream Land, one-minute Time match, items off); it evolves per
   `P2_PLAN.md` law 4, by board row, at phase closes.
2. **Match-length rule (owner, 2026-08-05)**: gate arms run the one-minute
   match. The soak's long match is its own flag (`NDS_R2_SOAK_MATCH_MINUTES`)
   and never rides a gate seed.
3. **Cadence population (owner, 2026-08-17)**: cadence verdicts read over ALL
   presented frames. The 1,600-frame gameplay-window rank-80 stays the
   candidate-sizing basis. Label which of the two any figure is.
4. **Whole-match instrument only**; cross-build placement floor ≥14,080 at
   rank-80; size levers against the current requirement, never a stale basis;
   measure the configuration that ships (`nds_build_config.h` is the truth).
5. **Publish law**: P2 publishes `smash64ds.nds` from verifier-covered
   configurations only. `smash64ds-battle-playable-hwtri.nds` is the frozen
   P1 artifact (12,530,688 B, SHA-256 `2F47C8AC…CB2F`, commit `843fe40f4d2`;
   root-pin rebuild pair per `docs/archive/` HANDOFF record).
6. Evidence: performance/visibility artifacts are permanent
   (`artifacts/performance`, `artifacts/visibility`); board rows close with
   links, and Device A/B reports show the 2/3/4/5+ histogram, max interval,
   P50/P95.

## Phase status

| Phase | State | Gate summary |
|---|---|---|
| P2-1 VS shell | **OPEN — active phase** | Loop soak green, menus hold cadence, Boundary re-defined |
| P2-2 Four-fighter engine | queued | 4-CPU stress arm stands up; budgets published |
| P2-3 Fighter production | queued | 10 fighters, pipeline reproducible |
| P2-4 Stage production | queued | 8 VS stages |
| P2-5 Items | queued | System + 20 items + 13 Pokémon; stress = items ON |
| P2-6 1P Game | queued | Campaign start-to-credits |
| P2-7 Modes & meta | queued | Fresh-cart parity; P2 close gate |

## Queue — P2-1 VS shell (all unowned; 1a/1b/1b-1 green, 1c-1g red)

| ID | Slice | Status | Notes |
|---|---|---|---|
| P2-1a | Match-config seam: descriptor struct (4 slots), mode 163 becomes a preset; battle consumes descriptor only | **green** | `NdsMatchConfig` in `include/nds/nds_match_config.h`; preset + apply in `src/port/nds_match_config.c`. Mode 163's seeder writes zero battle-state fields now. Boundary green; `CPU_CONFIG=0,1,1,1,1,1,0,0,1` is the descriptor read back out of the live battle state. P2-1e replaces the preset with CSS output |
| P2-1b | Scene manager: menu scenes + transitions + per-scene arena reset discipline | **green** | `src/port/nds_scene_manager.c` + `include/nds/nds_scene_manager.h`: scene registry (the six kinds this build has), `ndsSceneManagerRequest` as the only port-owned writer of `scene_curr`/`scene_prev` (fail-closed on an unregistered kind), and `ndsSceneManagerEnter`/`Exit` hooked around `syTaskmanStartTask` — the real per-scene arena rewind (decomp `sys/taskman.c:1227`→`:258`) — recording each entry's arena high-water. Boundary green; root ROMs byte-identical. **N-LOOP PROOF (`smash64ds-p2-1b-scene-walk-hwtri`, `NDS_R2_SCENE_LOOP_WALK=3`, 2026-08-17, `artifacts/verification/2026-08-17_scene-loop-walk-3loops.txt`):** ten entries, ten exits, `loops=3`, `rej=0 unreg=0 mism=0`, transition ring `{0x1809,0x0916}` ×3 — Results→VSMode→VSBattle walked three times. One 1,548,288 B arena at `0x02269910` shared by every entry (the base moved 96 B from the pre-fix `0x022698b0` because the binary grew; `gNdsTaskmanArenaChosenSize` is unchanged, so the arena did not shrink). Per-entry high-water by kind — VSBattle 1,481,116 / 1,480,936 / 1,481,504 / 1,480,592 (spread 912 B, **non-monotonic**: the max is the third entry and the min is the fourth, so the loop does not grow); VSResults 1,322,828 / 1,322,828 / 1,322,828 and VSMode 392,536 / 392,536 / 392,536, both **exactly flat over three entries**. Arena free at exit never below **66,784 B** against the 32 K floor. Zero exceptions and zero faults: `lr_abt`/`sp_abt`/`spsr_abt` read `0`/`0`/`0x10` at the end of the run, untouched since reset. **Measured on the SHIPPING configuration** (`build-battle-playable-proof-hwtri-harness`, walk off, `artifacts/verification/2026-08-17_scene-manager-boundary-config.txt`): the two VSBattle entries a one-minute match makes — the battle and its Sudden Death, `scvsbattle.c:528`/`:548` — reach 1,494,260 then **1,490,888**. A same-kind re-entry does **not** grow the high-water; it is 3,372 B *lower*. Arena free never fell below 54,028 B, well above the 32 K floor. This also re-measures the archived "Sudden Death takes ~119 KB more arena" premise, which stays withdrawn. The blocker that held this row (the menu→battle `SIGILL`) is closed by P2-1b-1 |
| P2-1b-1 | Menu→battle re-entry data abort: the thread registry outlived the arena rewind | **green** | **Root cause:** `sThreads[]` (`src/port/libultra_os.c`) is a port-private thread registry the source does not have, and nothing tied it to the taskman arena's lifetime. A GObj thread's `OSThread` and its coroutine block are drawn from that arena (objman.c:810-818 via `syTaskmanMalloc`), which `syTaskmanStartTask`→`syTaskmanInitGeneralHeap` rewinds on every scene entry (taskman.c:1227→:258). BattleShip's own contract is `osDestroyThread` before `gcEjectGObjStack` (objman.c:918); the port's *bounded* VS Mode branch never runs that teardown, so the VS Mode scene left two GObj threads registered, the next VSBattle entry rewound over them, and `ndsOsRunThreads` (`libultra_os.c:345`) then read `0xFFFFFFFF` out of the reused memory as their coroutine. **The reported fault site was false:** `ndsBaseLbParticleDrawTextures` is not in the walk ELF at all — `--gc-sections` deletes it exactly as `battleship_lbparticle.c:27` says, so gdb resolved the PC through stale DWARF relocated to 0 (`pc=0x00000910`, `cpsr=0x400000b7` = ABT mode = a *nested* abort inside calico `__excpt_entry`). Measured instead by breaking on `__excpt_entry`, whose FIRST hit in the whole run is this one: `lr_abt=0x02030b1c` = `portCoroutineIsFinished+4`, instruction `ldr r0,[r0,#96]`, `r0=0xFFFFFFFF`, `lr_usr=0x02031043` = `ndsOsRunThreads+54`. Existence chain from a registry snapshot at every `ndsSceneManagerEnter`: `sThreads[5]/[6]` are NULL at entries 1/2/3 and hold `0x022c6868`/`0x022c7f20` at entry 4 — both inside the one arena `[0x022698b0,0x023e32b0)` (`mism=0`) — and at the fault that memory holds floats and texture bytes, not an `OSThread`. **Fix:** `ndsOsForgetThreadsInArena()` (`libultra_os.c`, declared `include/nds/nds_os.h`), called from `ndsSceneManagerEnter`'s `NDS_SCENE_FLAG_ARENA_RESET` branch — *before* the rewind, so the structs are intact — clearing registry slots only and never writing through the doomed pointer. **Engagement:** `gNdsOsArenaThreadsDropped` 0→2→4→6, `gNdsOsArenaThreadDropEntries` 0→1→2→3, `gNdsOsArenaThreadDropLastId` 10000006/10000013/10000020 (BattleShip `dGCProcessThreadID` range, so GObj threads and not service threads), landing on entries 4/7/10 — every menu→battle re-entry, the one that used to fault. **Negative control:** zero drops through entries 1-3 and at every Results/VSMode entry, and the run ends with `sThreads` holding only the five static service threads (`0x021b4828`…`0x021b0228`, all below the arena base). Artifacts: `2026-08-17_scene-walk-fault-pc.txt`, `_scene-walk-fault-excpt.txt`, `_scene-walk-stale-thread-registry.txt`, `_scene-loop-walk-3loops.txt`. **Second tell resolved:** `gNdsSceneWalkHopsRemaining` now reads 4 (not 5) at entry 4 and 6/5/4/3/2/1 at the six `ndsSceneWalkAdvance` calls, with `sNdsSceneWalkArmed` latching 1 once and never re-arming — same window, same fix; the pre-fix micro-mechanism was not isolated and the pre-fix ROM has been rebuilt over |
| P2-1c | 2D UI kit: SSB64 font/text, cursors, menu SFX, portrait conversion (Mario/Fox), dual-screen aware | red | Groundwork for P2-2 bottom-screen HUD |
| P2-1d | Title screen + main menu + VS menu + rules screen (greyed stubs for unbuilt modes) | red | After 1b/1c. Inherited from P2-1b-1: the bounded VS Mode branch this row replaces (`taskman_seam.c:7600`) never runs the source's GObj-process teardown, so it leaves GObj threads registered; the real scene must run it. The registry drop at the arena rewind makes that harmless either way |
| P2-1e | Character select: hand cursor, tokens, CPU toggle/level, 12-slot layout (10 locked) | red | Reference `mn/mnplayers` |
| P2-1f | Stage select (Dream Land + locked slots + random) → load → battle → results → CSS loop | red | Reference `mn/mnmaps` |
| P2-1g | Loop verifier: scripted full-loop walk ×N, heap watermarks, cadence, Boundary equivalence; becomes new Boundary at phase close | red | Closes the phase; owner visual pass rides here |

## Inherited reds — hand-run checkers, none on the Boundary path

Found while landing P2-1a, all pre-existing at `82f7f2fd54a` and all unowned.
None is P2-1 work; each is a one-line fix in the checker that already exists.

- `check-docs.ps1` and `check-architecture.ps1` still require docs the P2
  restructure archived (`P1_EXECUTION_BOARD.md`,
  `Smash64DS_Runtime2_SwitchPlan.md`) and fail on tracked `artifacts/bugs/**`.
- `check-one-minute-match-verifier.ps1` (DevFast) pins
  `$bp[2] -eq (2 * $bp[3])` in `verify-battle-mariofox-gcrunall-loop-harness.ps1`;
  that spelling does not exist there and did not at HEAD.
- `Makefile:3748` holds a literal `\n` instead of a recipe continuation, so
  `#define NDS_R2_COLLISION_L7_ORACLE` is never written to
  `nds_build_config.h`. Harmless today (default 0, undefined reads as 0 in
  `#if`) but `make NDS_R2_COLLISION_L7_ORACLE=1` silently does nothing. Repair
  with Read/Edit only.

## Decisions pending

- **`smash64ds_P1.nds` at repo root** (untracked, gitignored, byte-identical
  to the frozen P1 artifact `576F51ED…E723`, created 2026-08-17 19:47,
  referenced by no doc or script): `check-published-roms.ps1` rejects any
  third ROM at root, turning every Boundary run red. Owner chooses: (a)
  allowlist the name in the checker, or (b) keep it under `builds/`. Until
  ruled, Boundary runs relocate it to `builds/` for the run and restore it
  hash-verified (try/finally pattern from the P2-1a landing).

## Closed

*(empty)*
