# Handoff

Current: resumed from Claude's a5f2223179d; owner adopted docs/reviews/NATIVE_ONLY_IMPLEMENTATION_GOAL.md:
every new ROM, including diagnostics/P1/profiling, must exclude reference
renderers and software scene compositors. Host reference tools are allowed.
**1P campaign remains paused.** Main Menu/VS/VS Options/Option/Backup Clear accepted.
Latest owner symptoms/order are in docs/BUGS.md; evidence is in docs/p2/BUG_NOTES.md.

## Current checkpoint

Pushed ff7a1ac192f / 868306a54a0: converted wallpaper assets and native BG2 owner.
Owner shield/KO regressions recorded in 0da265d1add with screenshot hashes.
Native-only candidate now builds: all 260 actual link inputs pass the gate.
LoadTile decoder is noinline in main RAM: ITCM dispatcher had grown past 32 KiB.
Wallpaper alias fix is visible: 1 load/15 reuses, zero native failures at entry.
CatchSwirl and KO roots now pass native admission; the next failure is a
domain 3, scene 22, Interface GObj 1016/link 23 RGBA32 sprite (bitmap 0x022cf2c8).
Current candidate: builds/build-p2-shell/smash64ds-p2-shell-hwtri.nds.
Latest log: builds/resume-20260907/native-ko-first-realtime.txt.
Full native-only gameplay/visual closure is OPEN; public ROM is unchanged.

Before the migration, Boundary on a5f2223179d passed shell loop (35,604 B free)
and Mario/Fox realtime (212 frames). Four-CPU failed at frame 45: NULL countdown
GObj in ifCommonEntryAllThread, 12,164 B free. Logs: builds/resume-20260907/boundary.*.
The public ROM is unchanged; the board owns its hash. No full Boundary/P2 closure.

## Active integration

1. CatchSwirl's four roots are native, with white RGB/graded-alpha I4 conversion.
   lbCommonAddMObjForTreeDObjs now normalizes mixed fields via the shared helper;
   its bypass changed source PRIM flags 0x0200 into live ENV flags 0x0400.
   KO/ReflectBreak packets and fixed source palettes pass host checks; KO visuals remain open.
2. Animlock worker edits only renderer_adapter_matrix.c + test_native_animlock_matrices.py.
   Main fixed test duplicates/C syntax and cached-scale publication; 15 host tests pass,
   including actual C. Production now routes locks through source CPU composition;
   legacy hierarchy still declines. ARM/scene compile passes; lock-state runtime owed.
3. Haze generator retains all 17 bindings/19 DObjs, omits only four panel triangles.
   Pushed c829d677e05: regeneration/hash re-pin and six host tests pass.
   Native-ROM visual acceptance remains; no blanket white-pixel removal.
   Main took over malformed sprite edits; ten wallpaper assets have native loaders.
   Alias/CLI tests pass; native-wallpaper-probe.ps1 in resume-20260907 is current.
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
Up to 4 Muse + 3 GLM workers; GLM uses swarm-build + model override. Read-only worker
native_interface23_inventory (PID 39900) logs under builds/resume-20260905/; inspect before duplicating.
No new worktrees or snapshots. One build/emulator at a time; no manual -j or
MAKEFLAGS override. Freeze source/generated inputs during builds and verifiers.
CodeGraph first. Bounded UTF-8 reads for live logs; use python -X utf8 on Windows.
Start cycle: verify-all.ps1 -Profile Boundary -List and git status --short.
