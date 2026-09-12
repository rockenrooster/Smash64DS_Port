---
name: n64-to-nds-porting
description: Port Smash64DS_Port's BattleShip source into fast DS-native gameplay, geometry, materials, textures, animation and resident data. Use for GBI/RDP, transparency, source ABI/relocation or removing N64-machine work. Pair with nds-coding-practices for hardware/APIs.
---
# Smash64DS — N64 → DS translation

`PROJECT_GOAL.md` owns behavior, fidelity and budgets; `docs/README.md` maps workflow owners. This skill supplies technical translation rules, not task ordering, verification schedules or closure policy. Paths below are repository-root-relative.

Use read-only `decomp/BattleShip-main/decomp` for source semantics. Implement the fastest mechanically equivalent DS representation; do not require universal bit/pixel identity. CPU-side native transforms and shared native kernels are valid. Source graphics interpreters are host references, not target fallback.

## Read the affected route only

| Boundary | Reference |
|---|---|
| Project source/native map | [00](references/00-porting-decisions.md) |
| ABI, addresses, stable IDs | [01](references/01-c-abi-addresses.md) |
| GBI, vertex history, matrices | [02](references/02-rsp-to-ds-geometry.md) |
| Texture/sprite alpha, TLUT, sampling | **[03](references/03-rdp-materials-textures.md)**; companion DS 05–07 |
| Numeric/animation/joint bindings | [04](references/04-numeric-and-animation.md) |
| Residency, compaction, CSS previews | [05](references/05-residency-relocation-storage.md) |
| Gameplay clocks, collision/events | [06](references/06-gameplay-timing-and-objects.md) |
| OS/audio/save translation | [07](references/07-os-audio-and-io.md) |
| Specialization/precompute | [08](references/08-fast-paths-and-codegen.md) |
| Failure signatures / semantic fixtures | [09](references/09-recipes.md), [10](references/10-validation.md) |

## Translation invariants

- Resolve asset roots, joint bindings, inherited state and consumer spans from source producers **and** consumers, not names or generator constants.
- Compile immutable interpretation on the host; prepare stable bindings at transitions; update live inputs only. Preserve load-time vertex history, observable event order and complete reachable data.
- Preserve **effective source alpha**, not raw texture alpha indiscriminately: source equation → native bytes → upload → material/layer state. Keep offline/runtime material identity consistent.
- Keep stable semantic IDs separate from enabled counts. Compaction preserves aliasing, adjacent records and sentinels; memory accounting includes actual consumer lifetimes.

[Examples](examples/README.md) are bounded helpers, not replacement repository compilers. [Sources](references/SOURCES.md) and [tests](tests/README.md) are on-demand maintenance material.
