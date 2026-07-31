---
name: smash64ds-route-ds-subsystems
description: Route Nintendo DS low-level engineering tasks in Smash64DS_Port to the correct specialized skill or ordered skill set. Use when a task spans multiple DS subsystems, the owning hardware cost is unclear, a performance report needs triage, or an agent must decide whether the problem belongs to ARM9/cache/TCM, RAM/VRAM/assets, GX 3D, 2D/OAM/display, DMA/card I/O, ARM7/audio/IPC, fixed-point math, timers/VBlank/profiling, or platform/input/interrupts.
---

# Route DS Subsystem Work

## Purpose

Classify the task before touching code. Load the smallest relevant skill set and preserve the repository's existing operating model.

This is a router, not a substitute for the domain skills.

## Mandatory project context

Read these first:

1. `AGENTS.md`
2. `PROJECT_GOAL.md`
3. `docs/P1_EXECUTION_BOARD.md`
4. `docs/HANDOFF.md`
5. For optimization work, `docs/optimization/TASK_STANDING_RULES.md`
6. For verifier choice, `docs/VERIFYING.md`

Because `.codegraph/` exists, use CodeGraph before grep/find or broad file reading when locating symbols and call paths.

Never edit `decomp/`. It is read-only evidence. Inspect the relevant BattleShip, `sm64-nds`, and `sm64ds-decomp` code before substantial gameplay, renderer, memory, asset, or backend changes.

## Classification table

Load only the skills that match the live mechanism:

| Symptom or task | Primary skill | Usually pair with |
|---|---|---|
| Hot ARM9 functions, soft-float, cache misses, section layout, alignment, ITCM/DTCM | `smash64ds-optimize-arm9-memory` | timing skill |
| Heap/taskman pressure, resident assets, texture cache, VRAM bank plan, OOM | `smash64ds-budget-ram-vram-assets` | GX or 2D skill |
| Stage/fighter draw, command streams, matrices, vertices, polygons, culling, GX stalls | `smash64ds-optimize-gx-3d` | RAM/VRAM and timing |
| Wallpaper, HUD, sprites, OAM, BG layers, affine/bitmap modes, screen routing | `smash64ds-optimize-2d-oam-display` | RAM/VRAM and timing |
| Copies, FIFO DMA, cache coherency, NitroFS/card reads, streaming or load spikes | `smash64ds-optimize-dma-card-io` | RAM/VRAM and timing |
| BGM/FGM, mixer channels, ARM7 acknowledgements, audio streaming, IPC stalls | `smash64ds-optimize-arm7-audio-ipc` | DMA/card I/O |
| Float helpers, divide/sqrt, fixed-point conversion, LUTs, quantization | `smash64ds-optimize-fixed-math` | ARM9 and timing |
| FPS/ticks, VBlank slips, timers, instrumentation, P50/P95 attribution | `smash64ds-measure-frame-pacing` | whichever owns the work |
| Input polling, IRQ/shared state, startup, BIOS/power/lid, platform glue | `smash64ds-harden-platform-input-irqs` | timing only when measured |

Wi-Fi, RTC, Slot-2, microphone, and power-management work stays in the platform skill until one becomes a real subsystem with its own repeated workflow.

## Deep-audit and asset specialists

Four narrower skills exist alongside the domain set. Prefer one when the task is exactly its shape:

| Task shape | Skill |
|---|---|
| One measured hot symbol needs a disassembly-level codegen audit | `smash64ds-arm-codegen-audit` |
| One measured memory/VRAM/transfer blocker needs a byte/lifetime audit | `smash64ds-memory-vram-audit` |
| One P1 audio path needs source-to-speaker qualification | `smash64ds-audio-qualification` |
| BattleShip sprite art needs extraction or DS-format baking | `smash64ds-extract-n64-sprites` |

## Triage workflow

1. **State the observed failure.** Use a concrete measurement or reproducible visual/behavioral defect.
2. **Name the boundary.** Default to canonical `battle_playable_realtime`, mode `163`, unless the task explicitly owns another scene-level capability.
3. **Locate the owner.** Use CodeGraph to identify the producer, consumer, state owner, and frame-loop call path.
4. **Classify the cost or defect.**
   - CPU instructions or memory locality
   - memory capacity/residency
   - geometry submission/backpressure
   - raster/display composition
   - transfer/card I/O
   - ARM7/audio/IPC
   - arithmetic
   - pacing/measurement
   - platform/interrupt/input
5. **Choose no more than three skills.** Load them in mechanism order, not subsystem-list order.
6. **Write one falsifiable hypothesis.** Include the counter, trace, screenshot, or verifier result that would refute it.
7. **Use a controlled build A/B.** Same tree, same boundary, one feature flag or one isolated change.
8. **Report an evidence-backed result.** Do not turn a subsystem guess into an implementation plan without proving ownership.

## Multi-skill ordering

Use these common sequences:

- Stage/fighter draw cost: timing → GX → ARM9 or RAM/VRAM.
- Missing/wrong geometry: GX → RAM/VRAM; use timing only after correctness.
- Wallpaper/HUD cost: 2D/display → RAM/VRAM → timing.
- Audio hitch: audio/IPC → DMA/card I/O → timing.
- Load spike: DMA/card I/O → RAM/assets → timing.
- Soft-float hotspot: fixed math → ARM9 → timing.
- Freeze/nondeterminism: platform/IRQs → owning subsystem → timing.
- OOM while adding an optimization: RAM/VRAM → owning subsystem; capacity is a design constraint, not permission to remove content.

## Global stop rules

Do not:

- repeat broad audits when a current census already identifies the owner;
- use `ALL` flatness to claim no work was removed;
- use min FPS as the pacing verdict;
- self-approve visuals or audio;
- silently remove battle-reachable content;
- propose another code-placement experiment without a new mechanism;
- keep a generic runtime abstraction when a faster mechanically equivalent DS-native path is available;
- stack verifiers that cover the same runtime.

## Required routing output

Before implementation, state:

- selected skill(s), in order;
- owning code seam and reference seam;
- measured symptom;
- falsifiable mechanism;
- smallest experiment;
- correctness/fidelity gate;
- performance metric;
- device classification: melonDS-sufficient or device-only.

Then proceed without asking the owner to choose among equivalent technical routes.
