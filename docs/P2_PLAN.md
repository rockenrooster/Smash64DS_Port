# P2 Plan — The Rest of the Game

Owner-approved 2026-08-17. `PROJECT_GOAL.md` owns the P2 contract and gate;
this file owns the execution order, the cross-cutting laws, and the plan tree.
The dynamic queue is `docs/P2_EXECUTION_BOARD.md` — plans here are static
intent, the board is what is actually next.

## Phase order and why

| Phase | Name | Subplan | Depends on | Delivers |
|---|---|---|---|---|
| P2-1 | VS shell | `p2/P2-1-vs-shell.md` | P1 | Title → menus → CSS → SSS → battle → results → loop; match-config seam; bottom-screen HUD groundwork |
| P2-2 | Four-fighter engine | `p2/P2-2-four-fighters.md` | P2-1 | 2–4 fighters, FFA + teams, 4-slot bottom HUD, camera, standing stress gate |
| P2-3 | Fighter production | `p2/P2-3-fighter-production.md` | P2-2 | Generalized fighter pipeline + the remaining 10 fighters |
| P2-4 | Stage production | `p2/P2-4-stage-production.md` | P2-2 | Generalized stage pipeline + the remaining 8 VS stages |
| P2-5 | Items | `p2/P2-5-items.md` | P2-3 partial | Item system, all 20 items + 13 Pokémon, item switch UI |
| P2-6 | 1P Game | `p2/P2-6-one-player.md` | P2-3, P2-4, P2-5 | Full campaign, bosses/variants, 1P-only stages, bonus stages, score, endings, credits |
| P2-7 | Modes & meta | `p2/P2-7-modes-meta.md` | P2-1+ | Training, unlocks, records, options, save data, attract, intro cinematic |
| P3 | Wireless multiplayer | `P3_Multiplayer/Multiplayer.md` | P2 | Out of P2 scope (owner). Multi-card host/join. |

Ordering rationale (owner-ratified 2026-08-17): P2-2 establishes four-fighter
scaling with existing Mario/Fox content. The campaign consumes the roster,
stages, items and ally battles; its dependencies do not imply an owner pause.
Fighter generation includes item-hold/swing/throw states before P2-5 to avoid
retrofits. Proven fighter/stage pipelines may interleave, with fighters first
when serialized; actual shared writers and resource dependencies govern safety.

## Outcome packages

Read `HANDOFF.md` and the board; select the highest-impact ready, unowned
package under its existing phase/unit and ID. Finish a bounded source-defined
feature, including its children and reachable sibling states, or a measured
shared-blocker outcome—not an isolated failing display-list root. Reuse existing
implementations and valid proofs; do not restart P2 or reopen closed work without
contradictory evidence. A resource failure blocks dependent acceptance, not
independent correct work. Avoid new frameworks or queues for work existing
owners can express.

`BUG_FIXING_PROCESS.md` owns source-contract diagnosis and closure;
`VERIFYING.md` owns stable builds, batch verification and reproducible checkpoints.
Current owner directions remain binding: all-ROM native-only rendering,
source-equivalent behavior, 30 Hz menus, active 1P, and the recorded CPU
optimization/raster deferrals. Deferral does not waive final acceptance gates.

## Standing laws (apply to every phase)

1. **Measurement law** is unchanged: `docs/VERIFYING.md` + the board's
   standing-rules section. One-minute gate matches; whole-match instrument;
   cadence read over all presented frames (owner population ruling
   2026-08-17); candidates sized at rank-80 of the 1,600-frame gameplay
   window; cross-build floor ≥14,080.
2. **Stress-config law**: the gate configuration is the measured argmax over
   landed content. Each landed fighter/stage/item gets one measured 4-CPU run
   under the then-current stress config before its row closes; the config is
   re-derived from those per-landing measurements at each phase close.
3. **Budget law** (numbers set in P2-2, enforced per unit thereafter): any 4
   fighters + any stage + items must fit VRAM, main RAM, and sound RAM
   simultaneously. Each fighter/stage/item class gets explicit texture, pack,
   and voice-bank byte budgets. A unit that busts its budget is not landed.
4. **Boundary evolution**: the registry stays Latest + Boundary only. The
   Boundary *definition* upgrades at each phase close (P2-1: full-loop soak;
   P2-2: 4-CPU stress arm; …). Each upgrade is a board row with owner
   visibility, and the 2-fighter regression battle stays green throughout
   (regression guard for shipped behavior) — carried since P2-1M (owner,
   2026-08-19) by the `p2_battle_realtime` arm, which runs that same match
   (Mario human vs level-3 CPU Fox, Dream Land, one-minute Time) **through the
   P2 shell** on a P2-named lab artifact rather than out of a P1-named
   boot-into-battle proof ROM.
5. **Publish law**: P2 publishes `smash64ds.nds` only from verifier-covered
   configurations. `smash64ds-battle-playable-hwtri.nds` stays frozen as the
   P1 artifact.
6. **Unit definition of done** (every fighter/stage/item/screen): mechanically
   equivalent per the relevant BattleShip source (inspected, not remembered);
   complete assets, VFX, SFX, voice/announcer where applicable; within its
   budgets; measured under the current stress config; visual deltas recorded
   per the DS Visual Fidelity rules with an `artifacts/visibility` screenshot;
   **visual acceptance ships a side-by-side against the extracted source
   assets** (asset-dump comparison — owner, 2026-08-18): converted art,
   layout, and animation inventory checked against the source dump *before*
   the owner's eye, so deltas are caught by the implementer, not the owner;
   its board row closed with evidence links.
7. **Reference-first, assets included**: before implementing any subsystem,
   read the named BattleShip directory in its subplan; before DS architecture
   choices, check `sm64-nds`/`sm64ds-decomp` per `AGENTS.md`. **Before
   implementing any screen, effect, or UI element, enumerate the original's
   assets** — art, layouts, fonts, cursor placements/states, animations —
   from the source asset dumps into the unit's inventory (owner, 2026-08-18:
   "inspect original assets before DS implementation"). Implementation starts
   from converted source assets, never invented stand-ins.
8. **Native-renderer law: every built ROM is native-only**, including debug,
   bring-up and profiling. Exclude generic renderers/software scene compositors
   from build inputs and linked binaries; reference rendering stays host-side.
   There are no fallback switches or target exceptions for game content.
   Native rejection, skipped required output and claiming no-op owners fail;
   halting instead of falling back is containment, not completion. Shared native
   GX/BG/OAM routines and CPU-side native transforms remain allowed. This replaces
   the older bring-up/debug exception, including examples that still cite it.

## Owner decisions log (2026-08-17)

- 1P campaign runs after fighters+stages. Engineering fighter order and
  hazard-ascending stage order accepted (tables in P2-3/P2-4).
- **Bottom screen shows in-battle info** (damage/stocks/timer/portraits);
  top screen is gameplay. HUD migration lands with P2-2's 4-slot HUD rework.
  Menus render on the top screen; bottom screen static outside battle until
  revisited.
- Intro cinematic deferred to P2-7; P2-1 boots straight to the title (row
  P2-1h deleted the interim splash), then menus.
- Wireless multiplayer is P3 (multi-card host/join, design already in
  `docs/P3_Multiplayer/Multiplayer.md`). Determinism discipline (replay
  verifier stays green) is maintained through P2 so lockstep stays cheap.
- Standing permission to keep `CLAUDE.md`/`AGENTS.md` board and Boundary
  pointers current as phases land (content rules untouched).

## Owner decisions log (2026-08-18)

- **All first-party branding and original presentation assets ship** —
  logos, title/boot screens, menu artwork, copyright text — converted from
  source like every other asset; no invented "identity-safe" placeholders.
  (Recorded contract-side in `PROJECT_GOAL.md` Visual Requirements.)
- Boot goes **straight to the original title screen** (no invented splash);
  the intro cinematic precedes it when P2-7 lands, matching the N64 flow.
- DS system-menu banner (icon + title text) carries **original branding**.
- The menu collage (`llMNCommonSmashBrosCollageSprite`) is required content
  and lands **now**, ahead of P2-2 — board row P2-1h, via the cheapest
  `docs/p2/P2-1c-vram-map.md` option that holds 60 Hz menus.
- **Round-1 visual pass on the shell: FAIL, six findings** (see risk 5) —
  P2-1h's "landed" state did not meet the ruling. Doctrine sharpened at both
  roots: **target = the original's own art/layout/animation; approximation is
  a measured-fallback only; sacrifice order applies only to measured
  conflicts**. Asset inspection before implementation and side-by-side
  **asset-dump comparison** are now unit-DoD requirements (laws 6/7). Rework
  row P2-1i re-opens the phase.

## Owner decisions log (2026-08-19)

- **The Boundary battle arm rebases onto the shell.** The regression battle
  (law 4) runs through the P2 shell configuration and builds P2-named lab
  artifacts; the P1-named proof target `smash64ds-battle-playable-proof-hwtri`
  leaves the routine gate. It stays in the Makefile for the dozen specialized
  probes that legitimately still boot straight into a battle. The frozen P1
  artifact `smash64ds-battle-playable-hwtri.nds` is untouched.
- **`smash64ds` is the base ROM now.** P2 publishes it from the shipping shell
  configuration and the owner plays it; the free-play lab name retires into it,
  so "what the owner plays" and "what the gate publishes" are one flag set.
- **Workflow**: the free-play ROM is rebuilt and delivered after each fix
  batch, and owner questions are batched *before* any ROM-affecting build.
  Recorded in `docs/VERIFYING.md` ("How A P2 Row Runs") rather than in a new
  document, per `docs/README.md`'s no-new-workflow-doc rule.
- Board row P2-1M carries all of the above, plus the docs modernization and the
  four inherited checker reds.

## Top risks

1. **4-fighter frame cost** — P1 margin was thin at 2 fighters. Mitigation
   runway (in sacrifice-order-legal order): per-fighter LOD, staggered pose
   updates, effect pool caps, engagement broadphase, and — only with owner
   approval — compensated 30 Hz simulation.
2. **RAM/arena headroom** — P2-2 must qualify four-way capacity, transition
   peaks and lifetimes before promising budgets. Current configuration-specific
   measurements and unresolved combinations belong on the board, not here.
3. **Kirby copy ability** — needs every other fighter's neutral-B; Kirby is
   scheduled last and copy is its own slice.
4. **Pipeline generalization stalling on the first new fighter** — Luigi
   (Mario variant) is deliberately first to prove the variant path cheaply
   before the harder archetypes.
5. **Presentation drift** — P2-1's round-1 visual pass FAILED on six findings
   (placeholder CSS/SSS backgrounds, invented button art, hand cursor missing
   from screens the source gives it, missing title background and title
   fire/label animation) because "recognizable" was read as the target and
   "sacrificable" as a skip-license without measurement. The doctrine now
   pins target = source at both roots (`PROJECT_GOAL.md` Visual Requirements,
   `AGENTS.md` DS Visual Fidelity); laws 6/7 make asset inspection and
   side-by-side comparison mandatory. Rework is board row P2-1i.

## Plan tree

```
docs/P2_PLAN.md                  ← this file
docs/P2_EXECUTION_BOARD.md       ← the only dynamic queue
docs/p2/P2-1-vs-shell.md
docs/p2/P2-2-four-fighters.md
docs/p2/P2-3-fighter-production.md   + p2/fighters/<name>.md (10 + variants + master-hand)
docs/p2/P2-4-stage-production.md     + p2/stages/<name>.md   (8 VS + 4 1P-only + bonus-stages)
docs/p2/P2-5-items.md                + p2/items/<class>.md   (6 classes)
docs/p2/P2-6-one-player.md
docs/p2/P2-7-modes-meta.md
```

Unit files are seeded now with content inventory, source pointers, risks, and
acceptance checklists; they get refined (numbers, exact frame data locations)
when the unit enters work. Keep them lean — current truth only.

