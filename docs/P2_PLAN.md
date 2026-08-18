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

Ordering rationale (owner-ratified 2026-08-17):

- **1P Game moved after fighters/stages** (owner's draft had it 2nd): the
  campaign consumes nearly the whole roster as opponents plus four exclusive
  stages and 4-fighter ally battles; building it second would mean building
  most of P2-3/P2-4 inside it, unsequenced.
- **P2-2 exists and comes early**: the end-goal stress test is the project's
  largest structural risk (P1 shipped with a small gate margin on a 2-fighter
  match; 4 fighters roughly doubles fighter update/draw cost). P2-2 retires the
  scaling axis using only existing content (Mario/Fox mirror matches) — zero
  new assets — and stands up the standing gate so perf debt is discovered per
  landing, never at the end.
- **P2-3 before P2-5, but the fighter pipeline bakes item-hold states from day
  one**: item pickup/swing/throw animations and states are per-fighter. Baking
  them into the pipeline output as each fighter lands avoids retrofitting 12
  fighters when items arrive.
- **P2-3 and P2-4 may interleave** once both pipelines are proven; they share
  no runtime seam. Fighters are the schedule risk, so fighters get priority
  when serialized.

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
   visibility, and the 2-fighter P1 configuration stays green throughout
   (regression guard for shipped behavior).
5. **Publish law**: P2 publishes `smash64ds.nds` only from verifier-covered
   configurations. `smash64ds-battle-playable-hwtri.nds` stays frozen as the
   P1 artifact.
6. **Unit definition of done** (every fighter/stage/item/screen): mechanically
   equivalent per the relevant BattleShip source (inspected, not remembered);
   complete assets, VFX, SFX, voice/announcer where applicable; within its
   budgets; measured under the current stress config; visual deltas recorded
   per the DS Visual Fidelity rules with an `artifacts/visibility` screenshot;
   its board row closed with evidence links.
7. **Reference-first**: before implementing any subsystem, read the named
   BattleShip directory in its subplan; before DS architecture choices, check
   `sm64-nds`/`sm64ds-decomp` per `AGENTS.md`.

## Owner decisions log (2026-08-17)

- 1P campaign runs after fighters+stages. Engineering fighter order and
  hazard-ascending stage order accepted (tables in P2-3/P2-4).
- **Bottom screen shows in-battle info** (damage/stocks/timer/portraits);
  top screen is gameplay. HUD migration lands with P2-2's 4-slot HUD rework.
  Menus render on the top screen; bottom screen static outside battle until
  revisited.
- Intro cinematic deferred to P2-7; P2-1 boots splash → title → menus.
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

## Top risks

1. **4-fighter frame cost** — P1 margin was thin at 2 fighters. Mitigation
   runway (in sacrifice-order-legal order): per-fighter LOD, staggered pose
   updates, effect pool caps, engagement broadphase, and — only with owner
   approval — compensated 30 Hz simulation.
2. **RAM/arena headroom** — general heap low-water ~52K against a 32K floor
   and 16K arena slack *today*, at 2 fighters. P2-2 includes a 4-way memory
   audit before any budget is promised.
3. **Kirby copy ability** — needs every other fighter's neutral-B; Kirby is
   scheduled last and copy is its own slice.
4. **Pipeline generalization stalling on the first new fighter** — Luigi
   (Mario variant) is deliberately first to prove the variant path cheaply
   before the harder archetypes.
5. **Menu/2D engine scope creep in P2-1** — the shell ships with recognizable
   approximations per the visual doctrine; cosmetic exactness is timeboxed.

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
