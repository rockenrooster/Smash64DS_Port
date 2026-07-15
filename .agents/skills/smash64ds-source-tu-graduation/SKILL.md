---
name: smash64ds-source-tu-graduation
description: Import and graduate a coherent original BattleShip subsystem into the live Smash64DS mode-163 runtime. Use for gameplay, collision, fighter state, weapons, effects, audio calls, map logic, or shared source translation units that are still stubbed, isolated, or proof-only. Drives source/ABI closure, narrow DS seams, natural runtime evidence, default-live graduation, and migrate-or-delete cleanup. Do not use for handwritten behavior clones or isolated symbol stubs presented as gameplay progress.
---

# Objective

Move one coherent original-source subsystem from missing/isolated/proof-only to
natural canonical runtime without changing BattleShip behavior or creating a new
permanent harness.

# 1. Establish source ownership

1. Read `AGENTS.md`, the selected P1 board row, and the current focused status.
2. Identify the authoritative BattleShip translation unit group, call graph,
   state structs, update order, data/reloc dependencies, and observable behavior.
3. Read bundled `decomp/sm64-nds` only for DS backend architecture patterns, not
   Smash behavior.
4. Treat all of `decomp/` as read-only.
5. Search the live tree for existing imports, partial stubs, seams, duplicate
   implementations, bounded modes, and verifier markers.
6. Inspect current ownership before touching shared headers, task manager,
   relocation, map, collision, or renderer registration files.

Do not import one function when its semantics depend on a coherent TU group that
can be brought over safely as a unit.

# 2. Close compile and ABI dependencies first

Build a dependency table:

- source TUs and required global data;
- public and private symbols;
- struct/enum/bitfield layout assumptions;
- function signatures and calling conventions;
- reloc/file/data IDs;
- task/GObj/DObj/MObj ownership;
- map/collision/audio/render seams;
- unsupported platform services;
- existing exact adapters that should be reused.

Prefer original TUs plus narrow platform seams. Keep port shims small,
mechanical, and source-traceable. Add host/static checks for symbol closure,
layout, constants, coordinate domains, and provider selection before runtime.

Forbidden shortcuts:

- handwritten gameplay clones;
- synthetic branch reruns or proof-only state restore;
- new one-bit “proved” masks in place of live state;
- force-setting outcomes that should occur naturally;
- broad fallback that silently keeps the old implementation active;
- creating a new harness mode for a subsystem that mode 163 can exercise.

# 3. Activate in the natural runtime

Use the existing mode-163 scene and the narrowest current focused verifier.
Activation must preserve:

- source update order and ownership;
- input/control gates and timer lifecycle;
- object creation/destruction and state transitions;
- collision/map coordinate contracts;
- fighter/weapon/effect interactions;
- renderer/audio calls and ordering;
- pause, KO, rebirth, Time Up, and Results behavior when applicable;
- deterministic fallback/reject counters.

A temporary runtime provider selector is allowed only at a subsystem boundary
for attribution. It must not branch in a hot inner loop or remain as an
uncontrolled production mode.

# 4. Prove behavior in layers

1. Static/host source and ABI checks.
2. Isolated LIVE provider check with zero missing symbols/fallthrough.
3. Sparse natural driver that reaches the source state without force-setting the
   outcome.
4. Continuous natural event evidence, not just one marker frame.
5. Mode-163 capture/state/semantic proof for the user-visible behavior.
6. Focused verifier, then DevFast/Boundary/Current according to
   `$smash64ds-verifier-router`.

Compilation, symbol presence, or an isolated fixture does not close a gameplay
row. Record first failure and every fallback provider.

# 5. Graduate and clean up

Default-live graduation requires:

- exact source behavior over the required natural window;
- zero unexpected fallback/fallthrough;
- no duplicate update or rendering/audio side effect;
- stable lifecycle and memory;
- all applicable current gates green;
- integration approval for shared seams.

After graduation, migrate or delete superseded bounded modes, proof-only masks,
stale stubs, duplicate providers, and obsolete docs. Preserve a focused verifier
only when it proves a continuing source contract.

# 6. Return packet

Use `references/source-graduation-evidence.md`. Workers return a coherent commit
or patch and evidence to integration; only integration edits central truth docs
and publishes artifacts.
