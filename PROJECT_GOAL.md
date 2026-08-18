# Smash64DS — Project Goal

This file is the authoritative product, fidelity, milestone, and definition-of-done
contract. Operational documents may define measurable gates, but may not silently
tighten this contract.

## Ultimate Goal
 
Recreate the complete Super Smash Bros. 64 game for the Nintendo DS.

The finished project should include essentially 100% of the original game's content and functionality: all fighters, stages, items, modes, menus, single-player content, multiplayer, training, bonus stages, progression, unlocks, records, audio, effects, and other major game systems.

The Nintendo 64 implementation itself is **not** sacred.

The goal is to preserve SSB64's gameplay, mechanics, timing, behavior, content, identity, and overall feel while rebuilding the game around what works best on Nintendo DS hardware.

Think:

**SSB64 → Smash64DS**

in the same broad spirit as:

**SM64 → SM64DS**

It should unmistakably feel and play like SSB64, but its internals and visual implementation may differ radically.

---

## Local Reference Repositories

These repositories exist locally under `.\decomp\` and are intentionally ignored by Git:

Original SSB64 behavioral reference:
- `.\decomp\BattleShip-main\decomp`

Nintendo DS implementation/architecture references:
- `.\decomp\sm64-nds`
- `.\decomp\sm64ds-decomp`

Treat all of `.\decomp\` as strictly read-only reference material.

Before implementing or changing SSB64 gameplay behavior, inspect the relevant BattleShip source. Do not guess, approximate from memory, or treat the current Smash64DS implementation as authoritative when the original behavior can be determined from BattleShip.

Before making substantial DS renderer, memory, asset, hardware, or backend architecture decisions, inspect how `sm64-nds` and `sm64ds-decomp` solve comparable problems.

Reference repositories are evidence and design input, not architectures that must be copied. The fastest correct Smash64DS implementation still wins.

## Source of Truth

Original SSB64 behavior is the specification.

The original source/decompilation should be used to determine:

* gameplay rules
* move behavior
* frame timing
* physics
* collision
* hitboxes
* knockback
* CPU behavior
* camera behavior
* stage behavior
* game-state behavior
* audio/event timing
* content

It does **not** prescribe how those systems must be implemented on the DS.

Mechanical equivalence is required.

Bit-exact or numerically identical execution is not.

Small numerical differences are acceptable when they do not materially alter gameplay or game feel.

---

## Core Engineering Rule

# The fastest correct DS implementation wins.

Performance takes priority over:

* architectural similarity to SSB64
* runtime generality
* elegant abstraction
* interpreter compatibility
* shared generic systems
* graphical exactness
* short loading times
* low ROM usage
* unused RAM

Never retain an expensive abstraction merely because the original game used it.

---

## Native Specialization Is Explicitly Encouraged

Content may be compiled, generated, specialized, precomputed, manually rewritten, or otherwise transformed into the fastest representation for Nintendo DS.

It is completely acceptable to have systems such as:

* `Mario_Update()`
* `Mario_Draw()`
* `Fox_Update()`
* `Fox_Draw()`
* `DreamLand_Update()`
* `DreamLand_Draw()`

instead of generic fighter, animation, object, scene-graph, or stage interpreters.

If all twelve fighters require twelve highly specialized native implementations, that is acceptable.

If every stage needs a completely different renderer, that is acceptable.

The preferred solution is to make **build tooling generic while allowing runtime code to remain highly specialized**.

---

## Allowed Optimization Techniques

Anything that preserves an acceptable gameplay result is allowed, including:

* generated C/C++
* generated ARM assembly
* fixed-point replacements
* fighter-specific code
* stage-specific code
* move-specific code
* precompiled GX command/data streams
* precomputed animation data
* precomputed matrices
* quantized animation poses
* pretransformed geometry
* precomputed hitbox trajectories
* precomputed camera behavior
* large lookup tables
* compile-time asset conversion
* aggressive baking
* DS-specific replacement systems
* 2D hardware in place of 3D where useful
* sprite-based effects
* static substitutes for expensive animation
* reduced visual update rates
* reduced simulation rates where behavior can be compensated
* heavy loading-time preparation

Runtime interpreters should exist only where they are actually competitive with specialized native code, or for dev only builds for visual comparisions.

---

## Compute Once, Not Every Frame

Loading time is cheap.

Gameplay CPU time is precious.

Expensive work should therefore be moved out of active gameplay wherever practical.

A match may spend several seconds preparing:

* fighter data
* stage data
* animation programs
* geometry
* textures
* lookup tables
* render programs
* audio
* effects

if doing so substantially reduces active-match CPU cost.

The ideal match runtime performs only work that genuinely changes.

---

## ROM and RAM Philosophy

ROM size may be traded aggressively for runtime speed.

Large quantities of precomputed or specialized data are acceptable, including ROM sizes in the tens or even hundreds of megabytes if necessary.

RAM should likewise be treated primarily as a performance resource.

A solution using nearly all available RAM at 900K ticks is preferable to one using little RAM at 1.15M ticks, provided sufficient reserve remains for reliable operation.

Stability is mandatory.

Unused resources have no intrinsic value.

RAM resources may HAVE to be reclaimed or shuffled to make room for other performance improvements.

---

## Independent Update Rates

Different systems do not need to execute at the same rate.

For example, it is acceptable for:

* gameplay logic to run at 60 Hz
* rendering to run at 30 Hz
* skeletal poses to update at 30 Hz
* particles to update at 15 Hz
* backgrounds to update at 15 Hz
* lighting to update only when required
* audio to operate primarily through events

These rates may differ further where necessary.

The original 60 Hz gameplay simulation is desirable but is **not sacred**.

If a compensated 30 Hz implementation produces substantially the same gameplay experience while enabling stable 30 FPS rendering, it is acceptable.

---

## Performance Target

The primary performance objective is:

# Stable 30 FPS

The nominal budget is approximately:

# 1.12 million ARM9 ticks per presented frame

The principal performance metric is approximately:

# P95 ≤ 1.12M ticks

during representative active gameplay.

Rare exceptional frames may exceed the budget.

For example:

* P50: 850K
* P95: 1.08M
* P99: 1.70M

may still constitute a successful 30 FPS implementation if those expensive frames are genuinely rare and gameplay remains stable.

Optimization should therefore prioritize large improvements to representative P50/P95 cost rather than sacrificing major wins merely to eliminate pathological outliers.

60 FPS rendering is aspirational.

30 FPS is the required target.

---

## Canonical Performance Platform

The project's custom accuracy-focused melonDS build is the primary development and performance reference.

It has been validated to remain approximately within 5% of physical Nintendo DS performance and is therefore sufficiently representative for normal optimization work.

Physical hardware remains useful for validation, but ordinary performance development does not need to block on repeated hardware testing.

---

## Sacrifice Order

When the Nintendo DS cannot afford everything, compromise in this order:

1. Audio fidelity
2. Visual fidelity
3. Gameplay fidelity
4. Original 60 Hz simulation implementation
5. Stable 30 FPS

Stable 30 FPS is the most protected requirement.

Feel free to test any of the top 3 but only after you completely exhaust cheaper ways of reproducing equivalent results.
Permanent implementation REQUIRES owner approval.

Before changing gameplay behavior, completely exhaust cheaper ways of reproducing equivalent results through:

* specialization
* approximation
* precomputation
* lower-frequency processing
* interpolation
* event-driven updates
* simplified representations
* DS-specific implementations



---

# Milestone P1 — COMPLETE (2026-08-17)

The vertical slice shipped: Mario vs. level-3 CPU Fox on Dream Land, one-minute
timed match, items off, with GAME SET, Results, Sudden Death, the START-restart
loop, and the complete match audio experience, at the performance gate
(owner-ruled cadence population: ≥95% two-VBlank over all presented frames on
the stress arm). Its requirements below remain binding product behavior,
regression-guarded by the Boundary verifier. The P1 ROM
`smash64ds-battle-playable-hwtri.nds` remains the published P1 artifact.

---

# Current Milestone — P2: The Rest of the Game

P2 turns the vertical slice into the complete game described by the Ultimate
Goal. Execution order (operational detail lives in `docs/P2_PLAN.md`):

1. **VS shell** — the full flow Title → main menu → character select
   (Mario/Fox) → stage select (Dream Land) → battle → results → back to
   character select, replacing the boot-into-match demo. The battle HUD moves
   to the bottom screen (owner decision, 2026-08-17).
2. **Four-fighter engine** — up to 4 fighters in a match (on one console:
   1 human + up to 3 CPUs), 4-slot HUD, camera, engagement, teams,
   free-for-all; stands up the standing P2 stress gate below.
3. **All fighters** — the remaining 10, pipeline-produced, each with complete
   moveset, assets, VFX, SFX, voice, and announcer audio.
4. **All stages** — the remaining 8 VS stages with full hazards, effects, and
   music.
5. **All items** — all items and Poké Ball Pokémon, with the spawn/carry/throw
   system and item switch UI.
6. **1P Game** — the full campaign (Mario first), bosses and variants (Master
   Hand, Metal Mario, Giant DK, Fighting Polygon Team), 1P-only stages, bonus
   stages, scoring, continues, endings, credits.
7. **Modes & meta** — Training, unlock flow, records, options, save data,
   attract demos, and the intro cinematic (deferred here by owner decision).

Every step includes its applicable menu/UI work. Wireless multiplayer is P3
(`docs/P3_Multiplayer/Multiplayer.md`); single-console VS play is 1 human plus
CPUs and is delivered by step 2. P2 publishes `smash64ds.nds`.

---

## Fighter Completeness Standard

Every fighter must have its complete original moveset and applicable gameplay
behavior, including:

* walking
* running
* jumping
* aerial movement
* crouching
* shielding
* dodging
* grabbing
* throws
* normal attacks
* aerial attacks
* smash attacks
* special moves
* recovery
* damage
* hitboxes
* knockback
* hitstun
* invulnerability
* stale moves
* platform interaction
* ledges
* KO boundaries
* death
* respawn
* respawn invulnerability
* pause
* timer
* scoring
* ties
* sudden death
* results screen

CPU fighters must use behavior equivalent to the original CPU at the selected
level. Once items exist, every fighter also carries its item pickup, hold,
swing, and throw behavior.

---

## Stage Completeness Standard

Every stage must include (Dream Land, completed in P1, is the exemplar —
collision, three pass-through platforms, blast zones, Whispy wind, tree-face
and flower animation, camera bounds, moving background, lighting, music):

* correct gameplay collision and pass-through platforms
* correct blast zones
* its hazards and interactive elements, mechanically equivalent
* its animated set pieces
* correct camera behavior/bounds
* its background treatment, moving where the original moved
* recognizable lighting/presentation
* stage music

A stage's visual implementation may differ from the N64 version.

---

## Visual Requirements

Visual accuracy is secondary to gameplay and performance.

The following compromises are explicitly allowed:

* lower-poly fighters
* simplified geometry
* fewer transformed body parts
* reduced animation interpolation
* lower animation update rates
* simplified lighting
* baked lighting
* baked vertex colors
* reduced texture resolution
* static decorations
* reduced background animation
* sprite effects
* fewer particles
* removed shadows
* effects updated every other frame
* highly stage-specific rendering tricks

The result only needs to remain recognizable, readable during gameplay, and consistent with the identity of SSB64.

Presentation identity includes the original's own presentation assets: title
and boot screens, logos and first-party branding, menu artwork, fonts, and
copyright text are content to be converted like any other asset (owner,
2026-08-18). Do not substitute invented or "identity-safe" placeholder
branding. The compromises above govern **how** such assets are realized
within DS budgets, never **whether** they appear.

---

## Audio Requirements

Every playable configuration should include the applicable complete audio
experience:

* stage music
* character voices
* movement sounds
* attack sounds
* hit sounds
* shield sounds
* KO sounds
* announcer audio
* crowd/gameplay sounds
* overlapping music and effects

Exact audio fidelity may be reduced before visual/gameplay/performance requirements are sacrificed.

---

## Current Performance Gate (P2)

The standing P2 gate is the strictest configuration the shipped content
supports, re-derived as content lands — "hardest" is the measured argmax over
landed content, never a guess:

**A 4-CPU stress battle on the measured hardest stage with the measured
hardest fighter set, all items on, holding P95 ≤ 1.12M ARM9 ticks per
presented frame and ≥95% two-VBlank cadence over all presented frames, at a
stable perceived 30 FPS.**

Every landed fighter, stage, and item is measured under the then-current
stress configuration before its work closes. Rare overruns remain acceptable.

The stability requirement applies to every screen, not only battle: menus,
character/stage select, GAME SET, Results, Sudden Death, campaign
interstitials, and bonus stages must hold their presented cadence without
perceptible hitching. The tick budget is per presented frame **at that
screen's cadence** — a screen presented at 60 Hz budgets approximately 560K
ARM9 ticks per presented frame; one presented at 30 Hz budgets approximately
1.12M.

Any implementation that exceeds the gate is an intermediate implementation, not an acceptable endpoint.

---

## Explicitly Out of Scope for P2

P2 does not require:

* wireless multiplayer (P3 — design parked in `docs/P3_Multiplayer/`)
* localization beyond the original US English content
* content the original game does not have
* exact graphical fidelity
* native 60 FPS rendering
* generic interpreter compatibility

Everything else in the Ultimate Goal is P2 scope.

---

## After the Vertical Slice

After Mario, Fox, and Dream Land are complete and performant, generalize the **generation and development pipeline**, not necessarily the runtime architecture.

Do not replace fast specialized systems with slower generic systems simply to make future content easier to add.

Instead, automate production of additional specialized native fighters, stages, effects, and other content.

---

# Definition of Done

Smash64DS is complete when essentially the full original Super Smash Bros. 64 game and its content are playable through the Nintendo DS implementation, the game retains the mechanics, identity, and overall feel of SSB64, and the result operates at a stable target performance appropriate for Nintendo DS hardware.

The implementation does not need to resemble the N64 engine.

It needs to resemble **SSB64 when you play it**.
