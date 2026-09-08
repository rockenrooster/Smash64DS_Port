# Handoff

Current: resumed from Claude's a5f2223179d; owner adopted docs/reviews/NATIVE_ONLY_IMPLEMENTATION_GOAL.md:
every new ROM, including diagnostics/P1/profiling, must exclude reference
renderers and software scene compositors. Host reference tools are allowed.
**1P campaign remains paused.** Main Menu/VS/VS Options/Option/Backup Clear accepted.
Latest owner symptoms/order are in docs/BUGS.md; evidence is in docs/p2/BUG_NOTES.md.

## Current checkpoint

Pushed 02d5a9cdaa8 / 2287834cd96 / 757ff7a494c: mandatory input/object/ELF gate,
13 controls pass; interpreter/command helpers and their cache moved host-only.
ARM9 APIs poisoned, scanner-presence rule reversed, legacy renderer modes rejected.
This is containment and separation, NOT completed native-only gameplay.
No new ROM has been built since the owner adopted the all-ROM requirement.
Current compiler worklist: builds/resume-20260907/nativeonly-caller-inventory.txt.
The standalone opening_movie_backend.o errors are irrelevant: it is a textual
include of scene_backend.c, not an independent translation unit.
The newer core renderer object compiles and passes symbol/input exclusion checks
(native-renderer-object-check.txt). Whole scene/backend compilation remains open.

Before the migration, Boundary on a5f2223179d passed shell loop (35,604 B free)
and Mario/Fox realtime (212 frames). Four-CPU failed at frame 45: NULL countdown
GObj in ifCommonEntryAllThread, 12,164 B free. Logs: builds/resume-20260907/boundary.*.
The public ROM is unchanged; the board owns its hash. No full Boundary/P2 closure.

## Active integration

1. Failure record and verifier readers are committed and host-tested. Fighter/stage
   fallthrough replacements and CPU fighter-raster removal remain uncommitted;
   scene compilation exposes retired opening/title callers. Core renderer compiles
   against forbidden-symbol/input checks. This is not complete native gameplay.
2. Animlock worker edits only renderer_adapter_matrix.c + test_native_animlock_matrices.py.
   Main fixed test duplicates/C syntax and cached-scale publication; 15 host tests pass,
   including actual C. Production now routes locks through source CPU composition;
   legacy hierarchy still declines. ARM/scene compile and runtime acceptance remain.
3. Haze generator retains all 17 bindings/19 DObjs, omits only four panel triangles.
   Pushed c829d677e05: regeneration/hash re-pin and six host tests pass.
   Native-ROM visual acceptance remains; no blanket white-pixel removal.
   Main took over malformed sprite edits; ten wallpaper assets pass six host tests.
   Wallpaper/platform objects compile; 23 host tests pass. Scene/visual checks remain.
4. Barrel projection constant-row scale has a local correction and passing host math
   test, but visible/capture/launch closure remains OPEN. Builds/resume-20260907/barrel-*.
   On-screen diagnostic global-status pokes crashed melonDS; do not repeat those writes.
5. Current Arwings ARE VISIBLE per owner. Check rideable-state gates and 2D/3D laser
   muzzles; the older no-spawn probe is not current owner evidence. DATA children last.
6. Packet cost is NOT eliminated as the 20 FPS cause: small costs can cross a VBlank
   deadline. Measure actual start/end, waits, present phase and remaining margin.

## Preserved work and operating rules

Broad unrelated dirty work includes prior 1P integration, tags, pipes, Pakkun,
assets and user P3/P4 documents. Preserve it. Campaign lab breadcrumbs remain at
builds/resume-20260905/preview-runtime/; do not resume campaign or redo CSS repairs.
Up to 4 Muse + 3 GLM workers; GLM uses swarm-build + model override. Logs remain under
builds/resume-20260905/nativeonly_*; current summaries under builds/resume-20260907/.
No new worktrees or snapshots. One build/emulator at a time; no manual -j or
MAKEFLAGS override. Freeze source/generated inputs during builds and verifiers.
CodeGraph first. Bounded UTF-8 reads for live logs; use python -X utf8 on Windows.
Start cycle: verify-all.ps1 -Profile Boundary -List and git status --short.
