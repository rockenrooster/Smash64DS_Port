# P2-6 — Metal Mario, Giant DK and Polygon Variant Contracts

Use the existing variant admissions, source overlays and native exports. Stat/material/scale sharing is an implementation option; each variant still needs an explicit source capability and lifetime contract. “Low-poly” or “data-only” is not a measured capacity or acceptance verdict.

## Metal Mario package

**Reuse:** Existing `ftmmario` data, Mario-family behavior and native export; Meta Crystal stage profile.

**Outcome:** Source attributes, passive resistance and collision/knockback behavior, material animation, applicable voices/SFX and camera all work in the natural campaign fight. Preserve exact source field values/selection rules; do not assume every Mario parameter is identical or that no voice exists.

**Proof:** Source table comparison for differing and inherited attributes, representative damage/launch/movement and attacks, visible metal material under relevant poses, source audio, KO/defeat and next-stage flow. Resource accounting charges shared immutable atoms once and actual mutable state/variant capability separately. A shiny screenshot is not mechanics proof.

## Giant DK package

**Reuse:** Ordinary DK source gameplay and qualified cargo, `ftgdonkey` attribute/scale data, native variant export and Jungle.

**Outcome:** The source player+two-allies battle versus Giant DK has correct traits/team setup, scale, resistance, collision/hit/attachment ranges and camera. Do not propagate the giant's passive resistance into ordinary DK.

**Proof:** Source-generated setup across applicable player/ally choices; actual four-instance fight, attacks/damage/capture cases, camera extreme, barrel/platform interactions where reached, native material/body/audio and final transition. A model scale alone does not prove source gameplay scale. Measure actual peak, not “same model therefore free.”

## Polygon kinds and waves package

**Reuse:** Existing `battleship_ftn_polygons.c`, twelve source `ftn*` data kinds and generated owners/admission. Some source model sharing is intentional; e.g. a shared model does not imply shared instance pose or copied player stats.

**Outcome:** Each source kind selects its declared attributes, motion/status capability, model/material and source CPU trait. Do not impose an assumed universal no-specials/reduced-kit rule without its source consumers. MainMotion/ShieldPose/native closure remains required even when a model export already exists.

**Proof:** Per-kind source/asset/admission checks and materially distinct active output, then the natural full Polygon Team on Duel Zone and Race source instances where used. Test replacement order/selection, concurrency, entry/KO, source total-opponent completion and exactly-once final tally. Corresponding live-instance/heap/owner counts must stabilize across all waves; replacement must not leave stale data or leak each defeated kind.

## Common dependencies and exit

P2-6 owns route, ally/wave spawning, tally and progression; this file owns variant behavior/capabilities. P2-2 owns capacity and per-instance lifecycle; P2-4 owns venues; P2-5 owns any required item/weapon/effect children. Native child coverage is not inherited merely because the base fighter is accepted.

- [ ] Source deltas and inherited behavior are pinned per variant/kind without guessed omissions.
- [ ] Native geometry/material/animation/audio and source hit/scale behavior are qualified.
- [ ] Giant ally, Polygon full-team and relevant Race populations pass natural lifecycle tests.
- [ ] Resident/transient resource profiles, cadence and required owner review pass.

## Source and retained evidence

Repository/source baseline: `907c46daffbec55477459cc56e83dfc9a417dabb` (September 10, 2026). This revision defines work and acceptance; it does not claim a new build or runtime pass. Current state belongs to `docs/P2_EXECUTION_BOARD.md`; owner symptoms belong to `docs/BUGS.md`.

- `src/import/battleship_ftn_polygons.c`.
- `decomp/BattleShip-main/decomp/src/ft/ftchar/ftmmario`.
- `decomp/BattleShip-main/decomp/src/ft/ftchar/ftgdonkey`.
- `decomp/BattleShip-main/decomp/src/ft/ftmanager.c`.
- `decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pgame.c`.
- `docs/p2/P2-6-one-player.md`.

[Pre-revision document and its source pins](https://github.com/rockenrooster/Smash64DS_Port/blob/907c46daffbec55477459cc56e83dfc9a417dabb/docs/p2/fighters/variants.md). The bundle installer preserves that document verbatim under `docs/archive/P2_PLAN_BASELINE_2026-09-10/p2/fighters/variants.md`. Use retained investigations only when relevant; superseded diagnoses are not new implementation instructions.
