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

## Queue — P2-1 VS shell (all unowned, all red)

| ID | Slice | Status | Notes |
|---|---|---|---|
| P2-1a | Match-config seam: descriptor struct (4 slots), mode 163 becomes a preset; battle consumes descriptor only | **green** | `NdsMatchConfig` in `include/nds/nds_match_config.h`; preset + apply in `src/port/nds_match_config.c`. Mode 163's seeder writes zero battle-state fields now. Boundary green; `CPU_CONFIG=0,1,1,1,1,1,0,0,1` is the descriptor read back out of the live battle state. P2-1e replaces the preset with CSS output |
| P2-1b | Scene manager: menu scenes + transitions + per-scene arena reset discipline | red — **manager landed and instrumented; the N-loop proof is blocked on a named defect** | `src/port/nds_scene_manager.c` + `include/nds/nds_scene_manager.h`: scene registry (the six kinds this build has), `ndsSceneManagerRequest` as the only port-owned writer of `scene_curr`/`scene_prev` (fail-closed on an unregistered kind), and `ndsSceneManagerEnter`/`Exit` hooked around `syTaskmanStartTask` — the real per-scene arena rewind (decomp `sys/taskman.c:1227`→`:258`) — recording each entry's arena high-water. Boundary green; root ROMs byte-identical. **Measured (`smash64ds-p2-1b-scene-walk-hwtri`, `NDS_R2_SCENE_LOOP_WALK=3`, 2026-08-17):** one 1,548,288 B arena at `0x022698b0` shared by every scene (`ArenaMismatch=0`), per-entry high-water VSBattle 1,481,116 / VSResults 1,322,828 / VSMode 392,536 — the rewind is real across kinds, no carry-over. **BLOCKER:** the *second* VSBattle entry (the menu→battle leg) dies `SIGILL` in `ndsBaseLbParticleDrawTextures` (`decomp .../lb/lbparticle.c:1942`), so there is no second same-kind high-water to compare and **the loop is not proven leak-free**. See P2-1b-1 |
| P2-1b-1 | Menu→battle re-entry `SIGILL` in the particle draw | red | Opened by P2-1b's own instrument, reproducible in ~90 s: `scripts/probe-scene-loop-walk.ps1 -Build build-p2-1b-walk -Target smash64ds-p2-1b-scene-walk-hwtri`. Sequence is VSBattle→VSResults→VSMode→VSBattle; the fourth entry faults. **The cheapest discriminator has not been run:** rebuild the walk hopping VSResults→VSBattle with **no** menu scene between. P1's START rematch already does exactly that and is green, so if the no-menu walk is also green the defect is *a menu scene between two battles* (particle/DL state that survives one scene but not two), not battle re-entry. Do that before theorising. Note also that `gNdsSceneWalkHopsRemaining` read 5 where 4 was due at that entry — one static's worth of state disagreeing, in the same window as the fault |
| P2-1c | 2D UI kit: SSB64 font/text, cursors, menu SFX, portrait conversion (Mario/Fox), dual-screen aware | red | Groundwork for P2-2 bottom-screen HUD |
| P2-1d | Title screen + main menu + VS menu + rules screen (greyed stubs for unbuilt modes) | red | After 1b/1c |
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
