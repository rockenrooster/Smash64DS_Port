# Inspection sources and confidence

Prepared 2026-09-14; repository snapshot `a5c5bc08d8e8661658865216798d600462db948e` on `master`.

The latest user message is the symptom authority. Repository sources are evidence
and implementation anchors, not a substitute for reproducing those symptoms.
This retained S-series index describes the first inspection. For expanded reads,
actual candidate source changes and host-test scope, use
[research/SOURCE_LEDGER.md](research/SOURCE_LEDGER.md) and
[research/VALIDATION.md](research/VALIDATION.md). No ARM ROM build/run or remote
commit was performed for this revision.

## Evidence labels

- **STATIC CONFIRMED**: code or a source declaration was directly read here. This does not prove a live route engaged.
- **REPOSITORY RECORD**: the board/artifact reports earlier implementation or tests. Its own scope/identity applies.
- **OWNER REPORT / USER-SUPPLIED DIAGNOSIS**: supplied by the user; preserved but not independently reproduced here.
- **Hypothesis / search target**: a falsifiable investigation direction, not an established cause or guaranteed file layout.

File references below are pinned. Search by symbol in a changed local revision.

## S01 — Repository operating rules

`AGENTS.md`

Directly read. Native-only builds, read-only reference trees, shared-build serialization, verification and hygiene.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/AGENTS.md)

## S02 — Current execution board

`docs/P2_EXECUTION_BOARD.md`

Directly read lines 1-125. Status reported by repository authors as of September 14; no runtime claims independently rerun here.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/docs/P2_EXECUTION_BOARD.md)

## S03 — Bug-fixing and closure policy

`docs/BUG_FIXING_PROCESS.md`

Directly read. Observable source contract, first divergence, natural-path proof, closure and annotations.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/docs/BUG_FIXING_PROCESS.md)

## S04 — Build and verification procedure

`docs/VERIFYING.md`

Directly read lines 1-170. Target identities, current profile coverage and timing/evidence limitations.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/docs/VERIFYING.md)

## S05 — Native impact wave implementation

`src/nds/nds_renderer_native_common.c`

Directly read lines 1030-1375. Transformed X/Y reused; painter-depth counter used per triangle; non-Z comment.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/src/nds/nds_renderer_native_common.c)

## S06 — Stage/effect initial geometry state

`src/port/reloc_backend_movement.c`

Directly read lines 11800-11885. Current actor-preserves-Z exception and effect admission context; downstream paths are follow-up search targets.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/src/port/reloc_backend_movement.c)

## S07 — Original impact wave display and update

`decomp/BattleShip-main/decomp/src/ef/efmanager.c`

Directly read lines 3270-3328. Display selects G_RM_AA_ZB_XLU_SURF and sets dynamic primitive alpha. Asset/camera decode in the supplied diagnosis was not independently repeated.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/decomp/BattleShip-main/decomp/src/ef/efmanager.c)

## S08 — Native DATA owner

`src/nds/nds_menu_shell_data.c`

Search excerpts inspected. Current board provides the retired-slab diagnosis; trace actual route in the working revision.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/src/nds/nds_menu_shell_data.c)

## S09 — Existing owner bug list

`docs/BUGS.md`

Directly read. Older report list differs from the latest user request; no extra old defects silently added. Falcon Punch/Kick are labeled repository-only leads.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/docs/BUGS.md)

## S10 — CSS residency repair and proof history

`artifacts/visibility/2026-09-13_css-residency-loop.md`

Directly read lines 1-290. Raw 0x152 pin, owner image/arena and particle re-entry repairs, three-lap proof and residuals. Historical proof is not current symptom closure.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/artifacts/visibility/2026-09-13_css-residency-loop.md)

## S11 — Samus morph source/owner tests

`scripts/fighters/test_native_samus_morph.py`

Directly read. MorphUnfold/MorphBall source roots and program IDs; roll/Bomb/restore source assertions. Test presence is not natural runtime proof.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/scripts/fighters/test_native_samus_morph.py)

## S12 — Stage setup repair and bounded proof

`artifacts/visibility/2026-09-14_stage-hazard-guards.md`

Directly read. Hyrule/Inishie setup guards, natural entry and native output. Does not prove Peach's Castle roof texturing or all other stage visuals.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/artifacts/visibility/2026-09-14_stage-hazard-guards.md)

## S13 — Saffron gate source and native anchors

`scripts/stages/native_stage_descriptors/yamabuki.py`

Search excerpts inspected together with src/import/battleship_gryamabuki_ground.c and the original gryamabuki.c. Open/close animation assets and source state/proximity/timer routing.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/scripts/stages/native_stage_descriptors/yamabuki.py)

## S16 — Product and performance contract

`PROJECT_GOAL.md`

Directly read. Native-only scope, 30 FPS target, 30 Hz menus, acceptance and custom accuracy-focused melonDS reference.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/PROJECT_GOAL.md)

## S17 — Native entry/effect generator

`scripts/3d_vfx/generate_nds_entry_effects.py`

Search excerpts inspected. Confirmed producer path; per-effect details must be traced on the actual working tree.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/scripts/3d_vfx/generate_nds_entry_effects.py)

## S14 — DS texture formats

Martin Korth / GBATEK, DS 3D Texture Formats. Read the A3I5/A5I3, direct-color and palette-format descriptions; these are hardware representation constraints, not diagnoses of any specific port asset.

[GBATEK texture formats](https://problemkaputt.de/gbatek-ds-3d-texture-formats.htm)

## S15 — DS texture/vertex blending

Martin Korth / GBATEK, DS 3D Texture Blending. Read per-polygon versus per-vertex alpha and modulation/decal behavior before choosing a gradient representation.

[GBATEK texture blending](https://problemkaputt.de/gbatek-ds-3d-texture-blending.htm)

## Supplied impact-wave analysis

The user's diagnosis additionally identifies packed EFCommonEffects1 DL 0x7C28,
command 18 `d9ddfbff 00000000`, inherited G_ZBUFFER, shared-camera link ordering,
item/effect initial-state differences, the generic-control shared bug, projected
depth-counter behavior and the rebirth-halo follow-up. These details are useful
leads but were not all independently inspected or re-decoded for this package.
The live code read here also contains the stage-actor Z-preservation exception.
Reconcile the actual working tree and source asset before making a broader fix.
