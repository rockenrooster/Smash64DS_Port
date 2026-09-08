# Handoff

Current: PAUSED by owner (2026-09-08); resume only on request. Contract: docs/reviews/NATIVE_ONLY_IMPLEMENTATION_GOAL.md.
every new ROM, including diagnostics/P1/profiling, must exclude reference
renderers and software scene compositors. Host reference tools are allowed.
**1P campaign remains paused.** Main Menu/VS/VS Options/Option/Backup Clear accepted.
Latest owner symptoms/order are in docs/BUGS.md; evidence is in docs/p2/BUG_NOTES.md.

## Current checkpoint

Pushed 3c54a018254: native endings/Zebes acid; 31db5819e0d: parallel diagnostics.
Owner shield/KO regressions recorded in 0da265d1add with screenshot hashes.
LoadTile decoder is noinline in main RAM: ITCM dispatcher had grown past 32 KiB.
TIME UP passes and Results now draws both fighters: Mario's Lose pose swaps in
his alternate hand model parts, which the native owner lacked (395547e0c71).
Next Results failure is a sprite with no native program (domain 3, link 27).
Current candidate: builds/build-p2-shell/smash64ds-p2-shell-hwtri.nds.
Full native-only gameplay/visual closure is OPEN; public ROM is unchanged.

Before the migration, Boundary `p2_shell_loop` passed on a5f2223179d (35,604 B
free) with `p2_battle_realtime` at 212 frames. `p2_fourcpu_stress` failed at
frame 45: NULL countdown
GObj in ifCommonEntryAllThread, 12,164 B free. Logs: builds/resume-20260907/boundary.*.
The public ROM is unchanged; the board owns its hash. No full Boundary/P2 closure.

## Active integration

1. Native endings use a separate OBJ bank; GO/sparks/tags stay resident (63,744 B).
   Zebes acid is in the native packet; source Tra-only nodes now admit correctly.
   Stage wave1: six pass; Castle asset86/root0x7558, Saffron160/0x420, Inishie155/0x1c8 fail.
   KO/ReflectBreak models admit; particle env colors/frame animation and visuals remain open.
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

Broad unrelated dirty work (1P integration, tags, pipes, Pakkun, assets, user
P3/P4 docs) must be preserved; do not resume campaign or redo CSS repairs.
Up to 4 Muse + 3 GLM workers; GLM uses swarm-build + model override. No workers remain.
Reports under builds/resume-20260905/native_*. Owner-validate witnesses ship in
every ROM now (2026-09-08); still precheck ELF symbols before a probe.
No new worktrees/snapshots; one build at a time, no -j/MAKEFLAGS override. Parallel
diagnostics now supported per docs/VERIFYING.md; perf/visual acceptance stay solo.
CodeGraph first; restart reads this file plus the board, others lookup-only.
Bank verbose output; bounded UTF-8 log reads (python -X utf8 on Windows).
Start cycle: verify-all.ps1 -Profile Boundary -List and git status --short.
