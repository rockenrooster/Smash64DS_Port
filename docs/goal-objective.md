Develop a source-faithful 1:1 Nintendo DS port of Super Smash Bros. 64 from the BattleShip decompilation.

MISSION

Original game source:
".\decomp\BattleShip-main\decomp"

Nintendo DS backend architecture reference:
".\decomp\sm64-nds"

Treat all of ".\decomp\" as read-only. Inspect the relevant BattleShip source before implementing behavior, and inspect sm64-nds before changing DS backend architecture.

This must remain:

Original BattleShip game code + Nintendo DS backend = playable port

Never replace portable original gameplay with handwritten approximations, scripted behavior, or a DS-native Smash rewrite. Import and adapt the original source whenever possible. Keep DS-specific behavior in src/nds or src/port, compatibility declarations in include, and original TU imports in src/import.

Gameplay semantics remain 1:1. Presentation targets roughly 90% overall visual likeness under Nintendo DS performance and memory constraints, not pixel identity. Give cosmetic exactness one measured focused attempt (Native AOT preffered when applicable); if it misses the budget or threatens P1, retain the cheapest recognizable source-derived approximation and document its visible delta, measured reason, and dated screenshot under `artifacts/visibility`. This exception never permits approximating hitboxes, collision, physics, attack timing, gameplay telegraphs, camera meaning, rules, or state transitions.

PRIORITIES

P1 — PLAYABLE DEMO

Complete a verifier-covered, source-faithful battle_playable demo by July 19, 2026, 11:59 PM America/Chicago.

Required match:

- Mario controlled by the user through live DS input.
- Fox controlled by the imported original CPU AI at level 3—not scripted harness input.
- Dream Land/Pupupu stage.
- One-minute timed free-for-all match.
- Items disabled.
- Original defaults for unspecified match settings.
- Direct verified boot into this configured battle is acceptable for P1; complete menu flow belongs to P2.

P1 is complete only when the user-facing ROM naturally supports an entire match from start through the timer expiring and the original match-end/result transition, including:

- Correct assets are used for hit sounds, hit visuals, and all other audio and visual effects
- CPU stalls and game freezes are fixed.
- Live Mario movement, attacks, aerials, specials, shield, grab, throws, damage, knockback, KO, and rebirth.
- Fox level-3 CPU movement, targeting, attacks, defense, recovery, and specials through imported original AI/runtime code.
- Source-backed fighter state, animation, collision, camera, match rules, scoring, timer, and HUD.
- Recognizable and correctly assembled Mario and Fox rendering with required materials and textures.
- Dream Land geometry, background composition, collision, platforms, and battle-relevant stage behavior.
- Battle BGM plus gameplay-critical effects and voices.
- Stable memory use with the project’s required reserve.
- Native DS rendering for 3D and 2D (or most efficient path considering DS contraints, least amount of CPU ticks)
- Real-time playable performance, targeting stable 30 FPS. Do not mark P1 complete below real-time speed without explicit user approval.
- No synthetic proof-only state, harness-controlled combat, crashes, hangs, or known save/data corruption.
- A verifier-covered user-facing ROM and dated capture demonstrating the result.


Use the existing battle_playable scene-level capability as the P1 integration anchor. Do not create new one-bit harness modes, proof masks, seed/restore reruns, or synthetic branch proofs.

P2 — COMPLETE PORT

After every P1 acceptance condition passes and P1 has been documented and snapshotted, continue autonomously toward the complete 1:1 port: original menus, modes, roster, stages, items, AI, audio, presentation, and remaining game systems.

Continue through coherent runtime-first subsystem groups. Import original translation units, wire the narrowest required seams, verify them in natural runtime, and graduate them live. Migrate or delete obsolete bounded proof infrastructure instead of reproducing it.

AUTONOMOUS WORK POLICY

At the beginning of each work cycle:

1. Read AGENTS.md.
2. Run:
   .\scripts\verify-all.ps1 -Profile Boundary -List
3. Read docs/P1_EXECUTION_BOARD.md and docs/HANDOFF.md.
4. Read the relevant architecture, verification, known-issue, diagnostic, and decomp-map documentation.
5. Inspect repository status and preserve all pre-existing user changes.
6. Select the highest-impact unowned red P1 packet that is not blocked.

Work continuously through investigation, implementation, building, captures, verification, and root-cause fixes. Make reasonable in-scope decisions without asking for routine guidance. Provide compact checkpoint updates naming the current task, verified evidence, remaining work, and blockers.

Treat every new finding, mistake, or inefficiency as feedback that must improve
the next cycle. When safe and P1-relevant, fix the root cause and improve the
existing shared code, helper, checker, or owning doc in the same checkpoint so
later work does not repeat the waste. Otherwise record one concise actionable
item in the owning doc and continue the highest-priority work.

Do not spend time on unrelated cleanup, speculative abstractions, or tooling without a measured benefit to the P1 critical path.

Parallel lanes must own disjoint files, builds, and runner slots. Do not start P2
while any required P1 row is red.

Prefer incremental builds, existing helpers, and less code. Do not add tooling,
caches, selectors, or abstractions without a measured P1 benefit.

Performance iteration uses synchronized eight-frame A/B ticks, FPS, screenshots,
and screenshot analysis. Run a third A only when A/B is inconclusive. Use the
smallest focused checker while editing and one widest relevant verifier at the
checkpoint; Current replaces Boundary when normal/shared startup is affected.
Only Latest and Boundary remain in the executable registry.

BLOCKER POLICY

After approximately 60 minutes of active investigation on the same bug without meaningful progress:

1. Preserve the exact failing command, output, suspected cause, attempted fixes, and best next experiment.
2. Record the blocker concisely in the appropriate active documentation.
3. If the changes are isolated and contain no pre-existing user work, create a local branch named "codex/wip-<topic>" and commit only the agent-owned WIP.
4. Never include unrelated user changes, push the branch, or open a pull request unless explicitly requested.
5. Move only to an independent queued P1 task that does not depend on the blocker.
6. Return to blocked P1 work after making useful independent progress. Do not switch to P2 while any P1 acceptance condition remains unmet.

DOCUMENTATION

Keep documentation current but lean:

- docs/P1_EXECUTION_BOARD.md: the only dynamic queue, acceptance matrix, lane ownership, dated gates, and evidence decisions.
- docs/HANDOFF.md: exact restart command and next packet, not another queue.
- docs/VERIFYING.md: the only verification and A/B workflow.
- docs/optimization/NATIVE_RENDERER_PLAN.md: current renderer contract.
- docs/PERF_LEDGER.md: measured and rejected renderer experiments.
- docs/PORTING.md: append-only progress history.
- docs/DIAGNOSTIC_REFERENCE.md: complete marker strings and diagnostics.
- docs/KNOWN_ISSUES.md: confirmed blockers and deferred gaps.

Keep HANDOFF.md short. Do not duplicate hashes, command stacks, or historical
marker records across active documents.

DEADLINE POLICY

The July 19 deadline controls prioritization; it does not permit rewritten gameplay, fake proofs, disabled verification, or false completion claims.

If P1 is incomplete at the deadline:

- Keep the repository buildable.
- Produce the best verifier-covered P1 ROM currently possible.
- Record exactly which acceptance conditions remain unmet and why.
- Do not label P1 complete.
- Continue working on P1 unless the user pauses or redirects the goal.

CHECKPOINT AND COMPLETION POLICY

After successful verified progress:

1. Remove temporary probes and artifacts.
2. Finish documentation and all applicable static checks.
3. Run the required verifiers.
4. Inspect final repository status.
5. Run:
   .\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
   as the final project action.
6. Run no commands after the snapshot in that work cycle.

Never claim completion from compilation alone. Report source imports, live behavior, ROM/capture evidence, verification results, performance, remaining gaps, and any WIP branches.
