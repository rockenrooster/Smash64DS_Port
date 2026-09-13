---
name: nds-coding-practices
description: Implement Smash64DS_Port's Nintendo DS backend with correct installed libnds/Calico APIs, ARM9/ARM7 ownership, cache/DMA, GX/BG/OAM transparency, fixed-point arithmetic, storage and bounded resources. Use with n64-to-nds-porting for BattleShip semantics.
---
# Smash64DS — DS implementation

`PROJECT_GOAL.md` owns product limits; `docs/README.md` maps workflow owners. This skill supplies hardware/API recipes, not agent orchestration, build scheduling or acceptance policy. Paths are repository-root-relative.

Start from `src/nds`, `src/port` and `include`, using the installed SDK and existing owners. Pinned upstream sources are a checked baseline, not an upgrade instruction. Substantial DS architecture work also uses read-only `decomp/sm64-nds` and `decomp/sm64ds-decomp`.

## Read the affected route only

| Boundary | Reference |
|---|---|
| Project/SDK map, hardware/build | [00](references/00-source-priority-and-versioning.md), [01](references/01-hardware-model.md), [02](references/02-build-link-and-assets.md) |
| Concrete libnds 2.x / Calico traps | [17](references/17-libnds2-calico-facts.md) |
| C/C++, fixed math and codegen | [03](references/03-c-cpp-and-codegen.md), [10](references/10-fixed-point-and-hardware-math.md) |
| Cache/DMA/shared memory | [04](references/04-cache-dma-and-shared-memory.md) |
| VRAM, textures and transparency | [05](references/05-vram-memory-and-assets.md), then [06](references/06-2d-video-bg-oam.md) for BG/OAM or [07](references/07-3d-gx.md) for GX |
| Input/IRQ/scheduling/audio | [08](references/08-input-irq-timers-frame-loop.md), [09](references/09-arm7-audio-ipc.md) |
| Storage/lifetimes | [11](references/11-storage-filesystems-streaming.md), [14](references/14-porting-design-patterns.md) |
| Cost model / failure signatures / invariants | [12](references/12-performance-practices.md), [13](references/13-debugging-common-failures.md), [15](references/15-review-checklists.md) |

## Hardware invariants

- CPU, DMA, GX and display/audio completion differ. Cache maintenance cannot make TCM DMA-visible; VRAM/palette/OAM need supported access widths.
- Keep formats, palette/remapping, upload and draw state consistent. `GL_RGB` forces direct-color alpha opaque; `POLY_DECAL` is not a cutout recipe; `POLY_ALPHA(0)` is wireframe.
- Keep blocking, filesystem and heavy work out of IRQs. Respect runtime-owned services/channels. Bound work in measured time, not bytes alone.

[Recipes](references/16-fast-implementation-recipes.md) and [examples](examples/README.md) are integration references, not new backend owners. [Sources](references/SOURCES.md) and [tests](tests/README.md) load only when needed.
