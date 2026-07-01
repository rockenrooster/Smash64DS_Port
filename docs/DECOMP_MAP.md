# Decomp Reference Map

Everything under `decomp/` is read-only reference material. Those folders are
independent upstream repositories or extracted upstream assets. Do not patch
them to make this port compile; add wrappers in `src/import`, DS backend code in
`src/nds` or `src/port`, and compatibility declarations in `include`.
Lean handoff snapshots include the decomp source/reference and build-critical
top-level O2R context needed for current imports, but exclude upstream decomp
build outputs, baseroms, generated binaries, duplicate nested O2R copies, and
tool caches. Use `scripts/New-Smash64DSSnapshot.ps1 -Mode CodeOnly` when a
handoff intentionally omits all `decomp/` material.

Usefulness labels:

- Critical: primary source for code or backend architecture.
- High: frequent reference for symbols, assets, generated data, or porting
  decisions.
- Medium: useful for audits, tooling, and cross-checks.
- Low: rarely needed for the DS port.
- Avoid: generated, build output, upstream metadata, or not useful for current
  port work.

## BattleShip-Main

`decomp/BattleShip-main` is the broader BattleShip project. It is useful beyond
the nested N64 decomp source because it contains PC-port docs, debug notes,
asset packs, and tools that explain how the original data is interpreted.

| Path | Usefulness | Use For | Notes |
|---|---:|---|---|
| `decomp/BattleShip-main/decomp` | Critical | Original Smash 64 code, headers, symbols, and extracted O2R resources. | Primary source of truth for game logic. Import through `src/import`; never edit in place. |
| `decomp/BattleShip-main/docs` | High | BattleShip architecture notes, render/GBI audits, struct audits, relocation investigations, sprite/texture investigations, bug notes. | Read before guessing at renderer, relocation, sprite, save, title/menu, or data-layout behavior. |
| `decomp/BattleShip-main/BattleShip_o2r` | High | PC-port O2R resource tree and named reloc resource categories. | Cross-check NitroFS packaging and resource naming. |
| `decomp/BattleShip-main/tools` | Medium | Resource analysis scripts, relocation symbol tools, texture probes, struct byte-swap checks. | Use as reference for writing project-owned tools; do not modify upstream scripts. |
| `decomp/BattleShip-main/debug_tools` | Medium | GBI traces, sprite deswizzle tools, reloc extraction/debug aids, screenshot helpers. | Useful when renderer or relocation diagnostics disagree with source expectations. |
| `decomp/BattleShip-main/include` | Medium | PC-port public headers and type definitions. | Reference only. Prefer the N64 decomp headers first for ABI imported into this project. |
| `decomp/BattleShip-main/assets` | Medium | PC-port asset context. | Useful for asset identity and comparisons, not as replacement DS-native art. |
| `decomp/BattleShip-main/port` | Medium | Existing PC-port platform abstractions. | Good comparison point for backend seams, but DS behavior should stay in `src/nds`/`src/port`. |
| `decomp/BattleShip-main/libultraship` | Medium | PC-port engine/platform layer. | Reference for how BattleShip replaced N64 services on PC; do not adopt as DS architecture wholesale. |
| `decomp/BattleShip-main/yamls` | Medium | Asset/resource metadata. | Cross-check resource IDs and extraction assumptions. |
| `decomp/BattleShip-main/cmake` | Low | PC build configuration. | Rarely relevant to devkitPro/libnds. |
| `decomp/BattleShip-main/scripts` | Low | Upstream project automation. | Reference only when investigating BattleShip asset/build flow. |
| `decomp/BattleShip-main/android` | Low | Android port spike/build material. | Not a DS backend reference. |
| `decomp/BattleShip-main/torch` | Low | Upstream project support material. | Not on the current DS-port path. |
| `decomp/BattleShip-main/.git`, `.github` | Avoid | Upstream repository metadata. | Do not edit. |
| Root files (`README.md`, `BUILDING.md`, `CMakeLists.txt`, `config.yml`, `CLAUDE.md`, `gamecontrollerdb.txt`) | Medium | Project orientation, PC build assumptions, controller/backend clues. | Read for context; keep local DS docs authoritative for this repo. |

### BattleShip Decomp Subtree

`decomp/BattleShip-main/decomp` is the original N64 Smash 64 decompilation and
the port's game-code source of truth.

| Path | Usefulness | Use For | Notes |
|---|---:|---|---|
| `decomp/BattleShip-main/decomp/src` | Critical | Original translation units for systems, scenes, menus, fighters, stages, items, effects, and libraries. | Import narrow slices through wrappers in `src/import`. |
| `decomp/BattleShip-main/decomp/include` | Critical | Original ABI, structs, enums, libultra declarations, and reloc symbol declarations. | Inspect before adding project-owned shadow declarations. Do not add globally without checking conflicts. |
| `decomp/BattleShip-main/decomp/BattleShip_o2r` | Avoid | Duplicate nested copy of extracted O2R resources. | The DS Makefile uses `decomp/BattleShip-main/BattleShip_o2r`; Lean snapshots exclude this nested duplicate to keep handoffs smaller. |
| `decomp/BattleShip-main/decomp/symbols` | High | Symbol maps for source/data offset resolution. | Use when adding relocation symbol coverage. |
| `decomp/BattleShip-main/decomp/assets` | High | Extracted source assets and build metadata. | Reference for asset identity and dimensions. |
| `decomp/BattleShip-main/decomp/docs` | High | Original decomp documentation index. | Start here when source comments are sparse. |
| `decomp/BattleShip-main/decomp/tools` | High | Decomp extraction, relocation, texture, audio, and symbol tooling. | Reference implementations for project-owned verifiers/converters. |
| `decomp/BattleShip-main/decomp/asm` | Medium | Assembly fallback and unmatched references. | Useful when C source is incomplete or ABI details are unclear. |
| `decomp/BattleShip-main/decomp/.splat`, `*.yaml`, `decomp.yaml` | Medium | ROM segmentation and extraction configuration. | Use for segment/resource layout checks. |
| `decomp/BattleShip-main/decomp/audio.md`, `MUSIC_AND_SFX_DISCOVERIES.md`, `PARTICLE_BANK_DISCOVERIES.md`, `relocData.md` | Medium | Focused subsystem notes. | Useful before audio, particle, or relocation work. |
| `decomp/BattleShip-main/decomp/build` | Avoid | Upstream generated build output. | Do not read as authoritative source and do not edit. |
| `decomp/BattleShip-main/decomp/.github`, `.git` | Avoid | Upstream repository metadata. | Do not edit. |
| ROM/report/root build files (`baserom.us.z64`, `Makefile`, `*_report.json`, diff/permuter scripts) | Low/Medium | Matching/decomp workflow context. | Useful for source provenance, not DS build integration. |

Important `src` folders:

| Path | Usefulness | Use For |
|---|---:|---|
| `src/sys` | Critical | Scheduler, taskman, object manager, controller, video, malloc, display/object helpers. |
| `src/sc` | Critical | Scene manager and scene-subsystem controller flow. |
| `src/mn` | Critical | Startup, Title, menus, menu-state flow. |
| `src/mv` | Critical | Opening movie and cinematic scene flow. |
| `src/ft`, `src/gm`, `src/gr`, `src/it`, `src/wp` | Critical | Fighter, game, stage, item, and weapon systems for later gameplay milestones. |
| `src/lb` | High | Common helpers for sprites, relocation, math, particles, and display helpers. |
| `src/ef`, `src/particles` | High | Effect/particle systems needed after scene boundaries. |
| `src/libultra` | High | Original N64 platform contracts to shim on DS. |
| `src/audio` | Medium | Original audio flow for later backend replacement. |
| `src/relocData` | High | Relocation data declarations and generated references. |
| `src/db`, `src/ovl8`, `src/credits`, `src/mp`, `src/if` | Medium | Menus/interface/debug/overlay subsystems as milestones reach them. |

## sm64-nds

`decomp/sm64-nds` is the main architecture reference for hosting an N64
decompilation on Nintendo DS. It is not a source of Smash gameplay and should
not be copied as a replacement engine. Use it to answer "how did a working N64
decomp port structure this DS backend problem?"

| Path | Usefulness | Use For | Notes |
|---|---:|---|---|
| `decomp/sm64-nds/src/nds` | Critical | DS entry point, renderer adapter, input, overlays, sample cache, menu/netplay seams, ARM7 support. | Primary DS architecture reference. Compare before adding DS backend systems. |
| `decomp/sm64-nds/src/engine` | High | Display-list-facing engine helpers, graph nodes, level scripts, math, surface loading/collision. | Use for backend patterns and separation of original logic from DS services. |
| `decomp/sm64-nds/src/game` | High | Game loop, memory, object processing, rendering graph, save, camera, level update integration. | Reference for porting N64-era game systems without rewriting behavior. |
| `decomp/sm64-nds/include` | High | Port compatibility headers, `PR` shim headers, segments, types, macros. | Reference for narrow ABI design; do not copy broad headers blindly. |
| `decomp/sm64-nds/tools` | High | DS overlay scripts, segment stub generation, asset converters, audio tools. | Reference for project-owned tooling and linker/overlay ideas. |
| `decomp/sm64-nds/levels`, `actors`, `textures`, `sound`, `data` | Medium | Asset organization and extraction/build flow. | Useful architecture comparison only; not Smash assets. |
| `decomp/sm64-nds/rsp` | Medium | RSP/Fast3D-related reference material. | Useful when display-list command semantics are unclear. |
| `decomp/sm64-nds/src/audio` | Medium | N64 audio system hosted on DS. | Use later for audio backend architecture. |
| `decomp/sm64-nds/src/buffers` | Medium | Buffer ownership and memory-layout examples. | Useful for DS memory pressure planning. |
| `decomp/sm64-nds/src/menu` | Medium | Menu integration patterns. | Architecture reference only, not Smash menu source. |
| `decomp/sm64-nds/src/goddard` | Low | SM64-specific face/goddard subsystem. | Usually irrelevant to Smash. |
| `decomp/sm64-nds/lib` | Medium | Third-party/support libraries used by that port. | Reference for dependencies, not automatic imports. |
| `decomp/sm64-nds/bin`, `build` | Avoid | Generated/staged output. | Do not edit or treat as source. Lean snapshots exclude generated `build/` payloads. |
| `decomp/sm64-nds/asm` | Medium | Assembly fallback/source matching context. | Use only for ABI or behavior questions. |
| Root build files (`Makefile`, `Makefile.split`, `sm64.ld`, `util.mk`, `assets.json`) | High | devkitPro/libnds build, linker, asset, and segment organization. | Compare before changing this project's Makefile/linker/source list. |
| ROM/hash/root extraction files (`baserom.us.z64`, `*.sha1`, `extract_assets.py`) | Low | SM64 extraction context. | Not Smash data. |
| `.git`, metadata files | Avoid | Upstream metadata. | Do not edit. |

Important `src/nds` files:

| Path | Usefulness | Use For |
|---|---:|---|
| `src/nds/main.c` | Critical | DS boot, hardware init, loop handoff shape. |
| `src/nds/nds_renderer.c`, `.h` | Critical | N64 display-list to DS renderer architecture reference. |
| `src/nds/ultra_reimplementation.c` | High | libultra compatibility patterns on DS. |
| `src/nds/nds_controller.c` | High | DS input mapped to original controller state. |
| `src/nds/nds_overlay.c`, `.h` and `tools/nds_*overlay*` | High | Overlay/linker strategy reference. |
| `src/nds/nds_sample_cache.c`, `.h` and `src/nds/arm7` | Medium | Audio/sample handling architecture. |
| `src/nds/gfx` | High | DS graphics helpers/assets used by the reference backend. |
| `src/nds/nds_menu*`, `nds_net*`, `nds_netplay*` | Low/Medium | UI/network examples; generally not needed for the Smash port core. |

## How To Use This Map

Before adding a subsystem:

1. Read the relevant BattleShip source/header under `decomp/BattleShip-main`.
2. Check BattleShip docs/tools for data-layout, relocation, renderer, or asset
   notes.
3. Check `decomp/sm64-nds/src/nds` and neighboring engine/build files for the
   DS backend architecture pattern.
4. Add or adjust only project-owned code.
5. Update `docs/PORTING.md`, `docs/HANDOFF.md`, and any verifier that proves
   the new boundary.
