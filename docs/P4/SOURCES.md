# P4 source and evidence ledger — revision 2

## Exact review pins

| Component | Repository | Commit |
|---|---|---|
| Project master | `rockenrooster/Smash64DS_Port` | `70e45e28d8e7b09545b1f772cf89f894452e9c5f` |
| Direct Remix submodule | `JSsixtyfour/smashremix` | `5e04fe7fcd023cd43c71f25f89bb6e810d254d55` |
| EXTRA submodule | `joaorb64/smashremix-plus-extra` | `96621afea26a83305abaf81add07dcf5a9c5fe3e` |
| EXTRA's nested Remix | `JSsixtyfour/smashremix` | `5e04fe7fcd023cd43c71f25f89bb6e810d254d55` |

The GitHub branch and directory/gitlink responses were checked, not inferred from directory names. The older `codex/r2-runtime2` and `p2-pikachu` branches were not selected as the updated project basis. Submodule content was followed through its declared repository and exact gitlink.

The current master still contains the original New_Characters.md content blob `a12220f273569e326af65e78d8cfe82cd15cc2cd`; this package proposes its replacement. PROJECT_GOAL.md content blob remains `5ce22392b74f4d6449b6da2c730c557f51d8c03b`. No repository write was performed.

## Shared sources read at the locked revisions

| ID | Source | Evidence boundary |
|---|---|---|
| D1 | [.gitmodules](https://github.com/rockenrooster/Smash64DS_Port/blob/70e45e28d8e7b09545b1f772cf89f894452e9c5f/.gitmodules) | Parent submodule URLs; exact gitlinks additionally verified through GitHub contents. |
| D2 | [PROJECT_GOAL.md](https://github.com/rockenrooster/Smash64DS_Port/blob/70e45e28d8e7b09545b1f772cf89f894452e9c5f/PROJECT_GOAL.md#L208-L285) | Current performance, rate and approval contract; full contract also reviewed on the prior baseline. |
| D3 | [scripts/fighters/generate_fighter_production_manifest.py](https://github.com/rockenrooster/Smash64DS_Port/blob/70e45e28d8e7b09545b1f772cf89f894452e9c5f/scripts/fighters/generate_fighter_production_manifest.py#L1-L130) | BattleShip/O2R source-reader contract and nonselectable MMario roots. |
| R1 | [src/Character.asm](https://github.com/JSsixtyfour/smashremix/blob/5e04fe7fcd023cd43c71f25f89bb6e810d254d55/src/Character.asm#L1-L140) | Base-ROM reads and original-cast-only parent macro. |
| R2 | [src/moveset.asm](https://github.com/JSsixtyfour/smashremix/blob/5e04fe7fcd023cd43c71f25f89bb6e810d254d55/src/moveset.asm#L1-L330) | Pointer semantics, control flow and shared event macros. |
| E1 | [.gitmodules](https://github.com/joaorb64/smashremix-plus-extra/blob/96621afea26a83305abaf81add07dcf5a9c5fe3e/.gitmodules) | Nested submodule URL; exact nested gitlink checked separately and matches the main Remix pin. |
| E2 | [build_single_character.bat](https://github.com/joaorb64/smashremix-plus-extra/blob/96621afea26a83305abaf81add07dcf5a9c5fe3e/build_single_character.bat) | Single-character front end delegates to build.bat. |
| E3 | [build.bat](https://github.com/joaorb64/smashremix-plus-extra/blob/96621afea26a83305abaf81add07dcf5a9c5fe3e/build.bat) | Python appender, patch stage, dependency installation and generated outputs. |

All character-specific pinned links and read ranges are in [SOURCE_AUDIT.md](SOURCE_AUDIT.md) and the individual cards. A located cross-file dependency is labeled separately from a read slice. No search hit on a different EXTRA revision is used to establish a fact about the pinned source.

Important literal paths: `src/moveset.asm`, `src/Bowser/bowser.asm`, `extra_characters/Snake/config.yaml`, `extra_characters/MetaKnight/config.yaml`, `extra_characters/MRGAW/config.yaml`. The primary donor contains case-inconsistent include spellings; a staging portability audit is planned, not a claim of a clean Linux build.

## Prior project baseline reused as design context

The first pass inspected these paths at `806c007b88b3f753642925500d73314ed905e460`. They remain architecture/contract context, not evidence that a recommended mechanism is already landed or that old byte budgets remain valid. Implementation must reread the affected current sources and measure the current configuration.

| Prior source | What was used |
|---|---|
| [include/ft/fighter.h](https://github.com/rockenrooster/Smash64DS_Port/blob/806c007b88b3f753642925500d73314ed905e460/include/ft/fighter.h) | Mirrored legacy IDs and kind-dependent structures; verify all affected consumers on implementation. |
| [docs/P2_PLAN.md](https://github.com/rockenrooster/Smash64DS_Port/blob/806c007b88b3f753642925500d73314ed905e460/docs/P2_PLAN.md) | Standing workflow, native-renderer and measurement laws. |
| [docs/p2/P2-3-fighter-production.md](https://github.com/rockenrooster/Smash64DS_Port/blob/806c007b88b3f753642925500d73314ed905e460/docs/p2/P2-3-fighter-production.md) | Existing source-driven fighter production and original-cast completeness. |
| [docs/p2/P2-2-pack-estimator.md](https://github.com/rockenrooster/Smash64DS_Port/blob/806c007b88b3f753642925500d73314ed905e460/docs/p2/P2-2-pack-estimator.md) | Typed object/consumer packing design, reset defaults and effective detail fallbacks. |
| [docs/reviews/Review_DS_Texture_VRAM_Residency.md](https://github.com/rockenrooster/Smash64DS_Port/blob/806c007b88b3f753642925500d73314ed905e460/docs/reviews/Review_DS_Texture_VRAM_Residency.md) | Recommended pre-GO placement; not proof the architecture is fully landed. |
| [docs/P3_Multiplayer/Multiplayer.md](https://github.com/rockenrooster/Smash64DS_Port/blob/806c007b88b3f753642925500d73314ed905e460/docs/P3_Multiplayer/Multiplayer.md) | Multi-card scope; does not select a synchronization algorithm. |
| [decomp/sm64-nds/src/nds/nds_overlay.c](https://github.com/rockenrooster/Smash64DS_Port/blob/806c007b88b3f753642925500d73314ed905e460/decomp/sm64-nds/src/nds/nds_overlay.c) | Boundary-loaded overlay reference; not a P4 executable-loader implementation. |
| [decomp/sm64ds-decomp/src/LoadOverlay.c](https://github.com/rockenrooster/Smash64DS_Port/blob/806c007b88b3f753642925500d73314ed905e460/decomp/sm64ds-decomp/src/LoadOverlay.c) | Residency/overlap reference for conditional overlay work. |

A prior search also identified a Captain victim-kind offset table in `src/import/battleship_ftcommon_capturecaptain.c`. It motivates the table census; it is not an exhaustive audit of capture consumers.

## External hardware source

[BlocksDS: 3D graphics](https://blocksds.skylyrac.net/tutorial/intermediate/3d_graphics/), consulted for the narrow independent polygon/vertex-capacity point. The plan does not infer any candidate's DS geometry size, rendering cost or achievable detail from that documentation.

## What the review did and did not establish

Established: current source pins and source access; selected source code/config facts for all eighteen candidates; concrete importer, inheritance, timing, copy, transform and resource-lifecycle risks; the existing production input-contract mismatch.

Not established: a complete callback/patch audit, reproducible donor build, original.xdelta application, decoded model inventory, semantic pack closure, loaded DS bytes, article live maxima, complete native conversion, gameplay equivalence, stable 30 FPS or network determinism. No local source corpus bulk download succeeded, and no extraction/build/performance tool was run. GitHub source slices were inspected directly instead.

All new manifest/schema/report names are proposed unless explicitly identified as existing project paths. `source-lock.json` is a review artifact; it does not become an enforced production lock until the build integrates it. Static observations and recommendations are deliberately separate from acceptance results.
