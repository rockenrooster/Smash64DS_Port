# Roadmap

This is the stable milestone map, not an active queue. The only dynamic P1
queue is `P1_EXECUTION_BOARD.md`; exact commands live in `HANDOFF.md`, concise
current truth in `STATUS.md`, measured history in `PERF_LEDGER.md`, and
append-only history in `PORTING.md`.

## P1 — Battle-Playable Release

Deadline: **2026-07-19 23:59 America/Chicago**

Active boundary: canonical `battle_playable_realtime`, mode `163`

P1 is the natural one-minute Mario-human versus original level-3 Fox CPU match
on Dream Land/Pupupu, items disabled, through Time Up and original Results. It
requires source-backed gameplay, recognizable presentation, gameplay-critical
audio, stable memory reserve, real-time performance targeting 60 FPS, verifier
coverage, and a dated exact-ROM capture.

| Program | State | Completion gate |
|---|---|---|
| Source battle runtime | Integrated | Full natural match, no synthetic combat/state restore, required Mario/Fox behavior accepted |
| Renderer M1 — affine BG2 | Complete | Cut G screenshot plus 5–35K-tick affine update |
| Renderer M2 — Mario/Fox | Active | AOT DS-native owner at 170–250K combined with semantic and screenshot parity |
| Renderer M3 — complete stage | Open | AOT DS-native stage at 150–250K with source/collision/pixel parity |
| Renderer M4 — conversion | Open | Zero texture conversion or preparation during gameplay |
| Weapons/gameplay gaps | Active | Fireball passes; natural recovery and current-ROM acceptance remain |
| Audio | Active | Five natural phase FGMs pass; voices plus winner/Results BGM remain |
| Stability and memory | Open | Repeated one-minute soaks, no crash/corruption, reserve gate maintained |
| Release evidence | Open | Full Regression, canonical parity, dated capture, exact-ROM user retest |

The July 16 feasibility checkpoint uses phase-specific P95 active ticks for
countdown, early/late combat, KO/rebirth, and Results. A credible 60 FPS path
must approach one VBlank (about 560K ticks) in every material phase. Any lower
presentation target requires explicit user approval; P1 is never redefined
silently. The detailed dated gates and integration decisions remain on the
execution board so this file cannot become a second queue.

## P2 — Complete 1:1 Port

P2 begins only after every required P1 row is green, P1 is documented, and the
Lean snapshot is complete. It proceeds as a dependency graph with independent
programs that integrate through the same source-faithful backend boundary:

| Program | Scope |
|---|---|
| Scenes and modes | Startup, menus, single-player/multiplayer modes, transitions, Results breadth |
| Fighters | Complete roster, source status/animation/assets, AI, effects, projectiles |
| Stages and items | Remaining stages, hazards, items, collision, presentation |
| Renderer | Generalized 3D/2D owners, material/combiner fidelity, streaming |
| Audio | Complete BGM/FGM/voice engine and scene coverage |
| Platform | Save/backup, overlays, memory streaming, hardware compatibility |
| Release engineering | Continuous compatibility, regressions, captures, packaging |

Each program imports coherent BattleShip subsystems, keeps DS behavior in the
backend, defines measurable stop gates before implementation, and graduates
only after natural-runtime verification. P2 work does not run in parallel with
an unresolved P1 release.
