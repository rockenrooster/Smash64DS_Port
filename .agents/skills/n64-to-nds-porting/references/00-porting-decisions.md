# 00 — Smash64DS translation map

All paths are repository-root-relative. These are entry points, not a frozen inventory; follow current callers and build inputs when files move.

| Meaning | Entry point |
|---|---|
| Original behavior, constructor roots, asset schemas | Read-only `decomp/BattleShip-main/decomp`; `docs/DECOMP_MAP.md` |
| Imported source and ABI adapters | `src/import`, `include`; `docs/FTSTRUCT_PARITY.md` |
| Platform/relocation integration | `src/port` |
| Renderer composition and native consumers | `src/nds/nds_renderer.c`; follow its includes to the affected owner |
| Texture/sampling/native material consumers | `src/nds/nds_renderer_textures_effects.c`, `nds_renderer_preamble.c` in the same directory |
| Content conversion and host fixtures | `scripts/fighters`, `scripts/stages`, `scripts/2d_vfx`, `scripts/menus`; layout in `scripts/README.md` |
| Comparable DS implementations | Read-only `decomp/sm64-nds`, `decomp/sm64ds-decomp` |

Prefer `source semantics/assets → host lowering → native data + small live bindings → GX/BG/OAM/native update`. Keep competitive original gameplay code; specialize when it removes work without changing required behavior. A source sprite need not become OAM, and a source file need not become a resident closure.

Identify the actual GBI dialect, source byte/container layout and constructor-to-consumer relationship. A renderer filename or existing symbol does not authorize a legacy path. Complete native owners can share CPU transforms, typed binding loops and hardware kernels; native-only does not mean GX must perform every calculation.

Use `PROJECT_GOAL.md` for permitted adaptations and `docs/README.md` for operational ownership. Do not copy current phase status, gates or historical diagnoses into this map.
