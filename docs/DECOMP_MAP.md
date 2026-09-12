# Decomp Reference Map

[PROJECT_GOAL.md](../PROJECT_GOAL.md) owns behavior/fidelity;
[ARCHITECTURE.md](ARCHITECTURE.md) owns implementation boundaries. This file
locates evidence, not tasks. BattleShip specifies SSB64 behavior; DS reference
implementations inform, but do not dictate, the fastest correct port.

## Ownership and availability

Treat all of `decomp/` as read-only. Project imports belong in `src/import/`,
DS implementations in `src/nds/` or `src/port/`, and compatibility APIs in
`include/`. Change derived content through project-owned producers.

Read-only does not mean ignored. Git carries reference sources; explicitly ignored
O2R, extracted assets and build outputs may be missing from a fresh checkout.
`DECOMP_PIN.txt`, the build scripts and the actual checkout establish provenance.
Use `git ls-files -- <path>` and the applicable ignore rules before cleanup or
junction changes; never overlay/delete the whole reference tree.

[VERIFYING.md](VERIFYING.md) owns acquisition/build prerequisites. A missing
extracted asset is an input problem, not evidence that the original lacks it.
An archived handoff bundle is not necessarily a self-contained build environment;
inspect its source and derived-input provenance before using it as evidence.

## BattleShip: original behavior and source data

Primary root: `decomp/BattleShip-main/decomp/`.

| Relative path | First use |
|---|---|
| `src/sys/` | Taskman/scheduler, objects, input, memory and video/platform contracts. |
| `src/sc/` | Scene dispatch, battle setup, campaign, Results and transitions. |
| `src/mn/`, `src/mv/` | Menus/title and opening/cinematic state flow. |
| `src/ft/`, `src/gm/` | Fighter moves/statuses, CPU behavior and game systems. |
| `src/gr/`, `src/mp/` | Stage/hazard behavior and map/collision geometry respectively. |
| `src/it/`, `src/wp/` | Items, Pokémon, projectiles and weapon lifetimes. |
| `src/ef/`, `src/lb/` | Effects and shared animation, sprite, relocation, math/display helpers. |
| `src/if/` | Gameplay interface and HUD behavior. |
| `include/`, headers beside `src/` modules | ABI, enums, structures and declarations; follow actual include paths. |
| `symbols/`, `tools/`, `docs/` | Symbol resolution, extraction/data tooling and supporting notes. |

Follow state tables, callers, flags and consumers, not just the function named
by a symptom. Model roots can change with hidden parts, copy powers, status and
detail level. Visible stage geometry and collision geometry are separate inventories.

The broader `decomp/BattleShip-main/` tree supplies PC-port/data context:

| Relative path | Use |
|---|---|
| `docs/` | Renderer/GBI, ABI, sprite, relocation and platform investigations. |
| `BattleShip_o2r/` | Named extracted resources consumed by the DS asset pipeline. |
| `tools/`, `debug_tools/`, `yamls/` | Resource identity, extraction and texture/relocation cross-checks. |
| `port/`, `include/`, `libultraship/` | PC-platform replacement seams, not a DS engine or gameplay authority. |

Cross-check notes and PC adaptations against original source when they disagree.
The nested `decomp/assets/us/relocData/` is a generated extraction input for the
DS build. Nested O2R and upstream `build/` outputs may be inputs to particular
tools: inspect the current consumer before calling a duplicate disposable.
Never treat generated output as authoritative source or patch it by hand.

## sm64-nds: N64-to-DS port patterns

Root: `decomp/sm64-nds/`. This is an N64-decomp-to-DS port reference, not Smash
behavior or a replacement engine. Compare its ownership mechanisms and hardware
seams; its graphics interpreter is not permission to include one in this ROM.

| Relative path | Use |
|---|---|
| `src/nds/` | Entry point, renderer, controller, overlays, sample cache and ARM7 integration. |
| `src/engine/`, `src/game/` | Original-engine/platform seams, memory and rendering integration. |
| `include/`, `tools/` | Compatibility declarations, overlay/linker support and conversion tools. |
| `src/buffers/`, `src/audio/` | Buffer lifetimes and audio implementation examples. |
| `Makefile`, `sm64.ld` | Source selection, section placement and resource organization. |

Start with the corresponding `src/nds/` subsystem, then trace its callers and
build placement. Do not copy capacities, addresses or cache settings without the
associated lifetime and hardware contract.

## sm64ds-decomp: retail DS implementation evidence

Root: `decomp/sm64ds-decomp/`. This reconstructs the retail Super Mario 64 DS
implementation; it is neither Nintendo-published source nor the `sm64-nds` port.
Use it for comparable rendering, fixed-point, memory, overlays and hardware work.
It does not specify SSB64 gameplay.

Start with `symbols/` for a routine/overlay, then its function-level implementation
in `src/`. `notes/` and `tools/` supply matching/reverse-engineering context.
Check the particular routine's provenance; `nearmiss/` candidates are not verified
implementations. Upstream matching/contribution instructions are not this project's
workflow.

## Use only the relevant evidence

For behavior changes, establish the BattleShip rule and its asset/state inputs.
For substantial DS backend decisions, inspect comparable mechanisms in both DS
references. Use CodeGraph first where available under the repository's tooling
rules, then verify the source/header details. Record the relevant source path and
symbol with the implementation or evidence; do not reread whole reference trees.
A missing reference should be reported as missing, not replaced by a guess.
