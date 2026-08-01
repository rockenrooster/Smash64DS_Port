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

# Current Milestone

The immediate target is a complete vertical slice (P1):

# Mario vs. Level-3 CPU Fox

# Dream Land

# One-minute timed match

# Items Off

# Results

# Sudden Death

The ROM may boot directly into the match.

No menus are required for this milestone.

---

## Gameplay Requirements

Mario and Fox must have their complete original movesets and applicable gameplay behavior, including:

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
* pressing start at results screen restarts the P1 match, up to infinite successive matches

Fox must use behavior equivalent to the original Level-3 CPU.

---

## Dream Land Requirements

Dream Land must include:

* correct gameplay collision
* three pass-through platforms
* correct blast zones
* Whispy wind gameplay
* tree-face animation
* flower animation
* correct camera behavior/bounds
* moving background
* recognizable lighting/presentation
* stage music

Its visual implementation may differ from the N64 version.

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

---

## Audio Requirements

The milestone should include the applicable complete match audio experience:

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

## Current Performance Gate

The Mario/Fox/Dream Land milestone succeeds when representative gameplay achieves approximately:

**P95 ≤ 1.12M ARM9 ticks per presented frame**

and maintains a stable perceived 30 FPS.

Rare overruns are acceptable.

The stability requirement applies to every screen in the milestone, not only the battle: GAME SET, the Results screen, and Sudden Death must hold their presented cadence without perceptible hitching. The tick budget is per presented frame **at that screen's cadence** — a screen presented at 60 Hz budgets approximately 560K ARM9 ticks per presented frame; one presented at 30 Hz budgets approximately 1.12M.

Any implementation that exceeds the gate is an intermediate implementation, not an acceptable endpoint.

---

## Explicitly Out of Scope for This Milestone

The current milestone does not require:

* items
* additional fighters
* additional stages
* local multiplayer
* single-player modes
* training
* character select
* stage select
* main menus
* unlockables
* intro
* credits
* save data
* full rematch flow (character/stage re-select; START-restart from the Results screen **is** in scope)
* exact graphical fidelity
* native 60 FPS rendering
* generic interpreter compatibility
* generic support for arbitrary SSB64 content

These remain part of the ultimate full-game goal.

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
