# P2-2 — Four-Fighter Engine + Standing Stress Gate

Extends the battle engine from 2 to up-to-4 fighters using only existing
content (Mario/Fox mirrors), moves the HUD to the bottom screen, and stands up
the standing P2 stress gate. Pure engine + performance work; zero new assets.

## Scope

2–4 fighters, any human/CPU mix (single console: 1 human + up to 3 CPUs —
this IS single-console VS completeness; >1 human is P3 wireless). Free-for-all
and team battle (team colors, friendly-fire flag), 4-way results ranking,
4-way Sudden Death and ties.

## Work breakdown

1. **Fighter array generalization.** Kill every hardcoded 2-fighter
   assumption: update loop, engagement pairs (6 pairs at 4 fighters — attack
   hitboxes × hurtboxes, grab, reflectors, projectile owners), hit-resolution
   order, respawn slots, scoring, camera targets. Mirror BattleShip's N-player
   semantics (`ft/ftmanager.c`, `gm/`) for resolution order and ties.
2. **Bottom-screen HUD (owner decision).** Damage meters, stock icons, timer,
   portraits for 4 slots render on the sub screen (2D engine/OAM — near-zero
   ARM9 3D cost). Top screen becomes 100% gameplay. P1's 2-slot main-screen
   HUD retires; tick-HUD instrument stays main-screen (instrument only, never
   shipped).
3. **Camera for 3–4 targets.** BattleShip's multi-target framing/bounds
   imported; verify against 4-fighter spread cases (corner camps, vertical
   splits) on Dream Land.
4. **Memory audit + budget law.** Measure the real 4-instance footprint:
   fighter state, battlepack layout (currently pre-packs 2), VRAM texture
   residency for 4 fighter texture sets + stage, sound RAM for 4 voice banks.
   Output: the per-fighter/per-stage byte budgets that P2-3/P2-4/P2-5 enforce
   (any 4 + any stage must fit). Arena slack is 16K and heap low-water ~52K at
   2 fighters today — expect this to force reclamation work, which is in
   scope here.
5. **Effect/particle pool policy.** 4 sources of VFX into pools sized for 2.
   Global caps + drop-oldest policy so effect submits (the known tail driver)
   stay bounded regardless of fighter count.
6. **Stress gate stand-up.** New verifier arm: 4×level-3-CPU (level re-argmaxed
   later per stress-config law), Dream Land, one-minute match, DLDI on, whole
   match. Reported like every gate: tick P50/P95, cadence histogram
   2/3/4/5+, max interval. This arm becomes the standing stress config and
   joins Boundary at phase close. `NDS_R2_BOTH_CPU` generalizes into it.
7. **First mitigation wave.** The initial 4-CPU number will be deep red —
   expected, that is the point. Census by lane, rank levers, land the cheap
   structural ones (stagger pose updates across frames, anim LOD by on-screen
   size, broadphase AABB prefilter before hitbox pairs). Deep levers (30 Hz
   compensated sim) stay parked pending owner approval per sacrifice order.

## Reference

- `ft/ftmanager.c`, `ft/ftcommon/` for N-fighter iteration and engagement
  order; `gm/` for match rules, teams, results ranking; camera sources for
  multi-target bounds.
- `sm64ds-decomp` for sub-screen 2D HUD layer setup.

## Risks

- Perf: fighter matrix prep and draw roughly double; this phase owns making
  the number *attributed and shrinking*, not green — green is the standing
  gate's job across P2.
- RAM: 4-instance footprint may not fit current arenas without reclamation;
  the audit runs before generalization is declared done.
- Engagement O(n²): 6 pairs vs 1 — broadphase must land here, not later.

## Exit criteria

- [ ] 4-fighter Mario/Fox mirror matches correct: engagement, camera, scoring,
      results, Sudden Death, ties — verified against BattleShip semantics.
- [ ] FFA + team battle rules working; CSS teams toggle live.
- [ ] Bottom-screen HUD shipped for 2–4 slots; owner visual pass.
- [ ] Budget law published (per-fighter/per-stage/per-item byte budgets) from
      the measured 4-way audit.
- [ ] Stress verifier arm running and reporting; first mitigation wave landed;
      remaining gap attributed by lane on the board.
- [ ] 2-fighter Boundary still green (P1 regression guard).
