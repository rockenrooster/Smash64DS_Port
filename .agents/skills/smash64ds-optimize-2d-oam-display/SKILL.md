---
name: smash64ds-optimize-2d-oam-display
description: Analyze, optimize, or debug Nintendo DS 2D engines, BG layers, affine or bitmap backgrounds, sprites/OAM, HUD and effects composition, wallpaper, main/sub display routing, display capture, VBlank commits, blending, windows, or layer ordering in Smash64DS_Port. Use for background/HUD/sprite performance, hybrid OAM work, screen corruption, VRAM bank conflicts, or replacing 3D presentation with cheaper 2D hardware.
---

# Optimize 2D, OAM, and Display

## Mission

Use the DS 2D engines where they remove measurable ARM9/GX work, while preserving readable SSB64 presentation and stable screen composition.

## Owning seams

Use CodeGraph, then inspect:

- `src/nds/nds_platform.c`;
- `src/nds/nds_ifcommon_oam.c`;
- `include/nds/nds_ifcommon_oam.h`;
- `src/port/sprite_preview_backend.c`;
- wallpaper, HUD, results, and effect paths reached from platform/video code;
- `scripts/check_ifcommon_hybrid_oam.py`;
- VRAM setup and Makefile flags;
- `sm64-nds` and `sm64ds-decomp` BG/OAM patterns.

## Workflow

1. **Map the complete display composition.**
   Record for both engines:
   - display mode;
   - BG0-BG3 type, priority, size, base, and role;
   - OBJ mapping and OAM owner;
   - VRAM banks;
   - blending/window/mosaic state;
   - main/sub LCD routing;
   - which data changes per frame and when it commits.

2. **Classify the cost.**
   - CPU pixel conversion or clear/copy;
   - cache flush;
   - VRAM upload;
   - affine calculation;
   - OAM rebuild;
   - layer clear/redraw;
   - VBlank synchronization;
   - 3D-to-2D handoff;
   - avoidable redraw of unchanged presentation.

3. **Prefer stable hardware state.**
   - Configure modes and banks at scene setup.
   - Update only affine registers, OAM entries, palette words, or dirty tiles that changed.
   - Keep static wallpaper/HUD assets resident.
   - Use generation/dirty tracking rather than full clear/copy.
   - Batch OAM writes and commit at a defined VBlank seam.
   - Avoid active-game VRAM remaps unless measured and proven safe.

4. **Choose representation by update shape.**
   - Static or slowly moving background: resident bitmap/tile/affine BG.
   - Small independent effects: OBJ/OAM.
   - Text/HUD: tile or bitmap layer with dirty regions.
   - Large source-derived sprite composition: preconverted DS-native data.
   - 3D impostor or captured layer only when capture/upload cost is lower than the replaced path.

5. **Prove layer correctness.**
   Test:
   - foreground/background priority;
   - transparency color and alpha/blend behavior;
   - top/sub screen routing;
   - camera scale and origin;
   - clipping at all screen edges;
   - pause/results/transitions;
   - no one-frame stale or clear-color flash;
   - no OAM index collision;
   - no bank overlap with 3D textures/palettes.

6. **Measure a controlled A/B.**
   - Count bytes cleared, copied, flushed, and uploaded.
   - Count OAM entries and dirty regions.
   - Measure typed ticks and VBlank histogram.
   - Use synchronized screenshots for any presentation change.

## Wallpaper rule

Dream Land water is frozen at source frame 0 by project contract. Background motion and visual fidelity may be approximated, but the owner remains the approval oracle.

A hardware-affine path is not automatically cheaper. Measure its CPU setup, VRAM footprint, arena interaction, and device pacing. Existing profile configurations can have different memory behavior; never compare an affine shipping arm against an instrumented arm that OOMs or silently falls back.

## Hybrid 2D/3D checklist

When moving effects or UI from 3D to 2D:

- preserve gameplay telegraphs and timing;
- preserve screen-space scale/position semantics;
- define which camera transforms still apply;
- maintain deterministic draw order;
- prevent double-rendering during migration;
- retain a correctness/oracle path only when it remains useful and bounded;
- delete obsolete proof-only reruns after natural runtime graduates.

## Device classification

VRAM upload, DMA timing, and pacing near a VBlank edge can be device-only. Pure CPU work removal with identical display behavior can be melonDS-sufficient when engagement is proven.

## Required result

Report:

- two-engine layer/VRAM map;
- bytes and entries updated per frame before/after;
- dirty-state mechanism;
- `WORK-H` P50/P95 and VBlank histogram;
- screenshot delta and owner-check requirement;
- transition/restart checks;
- device classification;
- KEEP, REVERT, STOP, or WIP.
