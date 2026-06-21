# Roadmap

This roadmap tracks milestone status. Keep detailed proof in `docs/STATUS.md`,
`docs/HANDOFF.md`, `docs/DIAGNOSTIC_REFERENCE.md`, and append-only history in
`docs/PORTING.md`.

Status labels:

- Done: verified in current ROM/runtime.
- In Progress: imported or started, but not enough for the next real
  gameplay/menu behavior.
- Deferred: known work, not started.

## Milestones

| # | Milestone | Status | Next Gate |
|---|---|---:|---|
| 1 | Clean devkitPro/libnds NDS project skeleton | Done | Keep `make -j4` green. |
| 2 | Add BattleShip as imported/reference source | Done | Keep `decomp/` read-only; import through `src/import`. |
| 3 | Identify original N64 entry/game-loop path | Done | Maintain `syMainLoop -> syMainThread5 -> scManagerRunLoop` evidence in docs. |
| 4 | DS platform layer like `sm64-nds/src/nds` | In Progress | Continue isolating DS behavior in `src/nds` and `src/port`. |
| 5 | Stub enough libultra/N64 platform functions | In Progress | Replace broad stubs only when the next original boundary needs them. |
| 6 | Boot an `.nds` ROM | Done | Keep melonDS/no$gba smoke paths working. |
| 7 | Run minimal original game-state/update loop | In Progress | Current loop reaches bounded Title setup, bounded VS setup, imported PlayersVS setup, imported Maps setup, and a guarded VS Mode -> PlayersVS -> Maps -> VSBattle boundary proof. |
| 8 | Render a simple placeholder frame | Done | Placeholder remains disabled behind original-preview paths. |
| 8a | Render one original Startup asset | Done | Preserve original `N64Logo` bounded SObj preview. |
| 8b | Render one original Opening Room DObj slice | Done | Preserve bounded Opening Room DObj preview and renderer diagnostics. |
| 8c | Render first post-Opening Room movie scene | Done | Preserve Opening Portraits SObj preview and handoff. |
| 8d | Reach natural opening movie Title boundary | Done | Preserve `verify-title-boundary.ps1` and current Title preview proof. |
| 9 | Replace placeholder rendering with N64 DL-to-DS rendering | In Progress | Continue from the selected bounded Opening Room material/DObj path only when a source boundary needs renderer work. |
| 10 | Load one stage and one fighter | Deferred | Requires asset/reloc, taskman, object manager, renderer, and game scene readiness. |
| 11 | Mario on Hyrule Castle moving using original logic | Deferred | Requires original fighter/stage/gameplay source path with adequate DS backend. |
| 12 | Expand menus, fighters, items, audio, full gameplay | Deferred | Start after first playable source-driven slice. |
| 13 | 1:1 full game port | Deferred | Requires broad compatibility, renderer/audio/save/overlay maturity, and behavioral verification. |

## Active Focus

Current source boundary: bounded imported Title setup after the natural opening
movie path, plus guarded dev/test harnesses that can start directly at bounded
original VS Mode, PlayersVS, or Maps setup and prove the VS Mode -> PlayersVS
-> Maps -> VSBattle boundary chain. See `docs/STATUS.md` for the exact current
state.

Recommended next milestone:

1. Use the VSBattle boundary proof to pick the smallest original
   `scvsbattle.c` setup boundary.
2. Inspect BattleShip source and docs before changing code.
3. Use `decomp/sm64-nds` only for DS backend architecture comparison.
4. Add minimal compatibility shims in project-owned code.
5. Prove the new boundary with the narrowest verifier.

Avoid for the next milestone:

- broad fighter/stage/action-scene imports
- full title/menu rewrite
- full audio import
- full renderer rewrite
- visual polish unless it blocks the selected original source boundary

## Renderer Track

The renderer remains in progress. The current proven path is a bounded Opening
Room material/DObj preview through the DS renderer adapter, not a general
hardware renderer. Continue renderer work only from a selected original asset or
source boundary, and keep diagnostics in `docs/DIAGNOSTIC_REFERENCE.md`.

Do not import all of `sys/objdisplay.c` as one step. That crosses matrix,
camera, framebuffer/depth, sprite, and broad GBI contracts before the DS
translator is ready.

## Mid-Term

1. Expand original Title/menu flow behind DS shims.
2. Import enough task/object/display contracts for one gameplay scene boundary.
3. Implement DS-compatible relocation, overlay, and asset streaming strategy.
4. Bring up one stage and one fighter from original data.
5. Replace audio stubs with a DS sequence/sample backend.
6. Add persistent save-data support.

## Long-Term

1. Expand fighters, stages, items, effects, menus, and battle modes.
2. Improve renderer fidelity and performance.
3. Add asset conversion/cache tooling where runtime conversion is too expensive.
4. Verify behavior against original formulas and state transitions.
5. Optimize DS memory layout, overlays, and data streaming after original paths
   are proven.

## Non-Goals

- Do not hand-author Smash-like physics or attacks.
- Do not approximate moves, hitboxes, hurtboxes, or knockback when original code
  exists.
- Do not replace menus with DS-native rewrites because they are easier.
- Do not edit generated build output directories.
- Do not optimize around temporary probes before proving original code paths.
