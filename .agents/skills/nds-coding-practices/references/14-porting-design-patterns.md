# Porting Design Patterns for Nintendo DS

## Port semantics, not desktop architecture

A successful DS port preserves the behavior that matters while replacing
architectural assumptions that do not fit:

- large heaps and pointer-rich object graphs;
- floating-point transforms everywhere;
- immediate-mode generic renderers;
- runtime image/audio conversion;
- synchronous filesystem access on demand;
- many threads and locks;
- unbounded containers;
- shader-era blending/texture assumptions;
- per-object virtual dispatch in hot loops.

Do not translate line-for-line when the source platform's ownership model is
incompatible with DS hardware.

## Establish behavioral authority

For each subsystem, identify:

- source behavior/oracle;
- acceptable approximation policy;
- deterministic test cases;
- DS owner and representation;
- setup/build-time work versus active-frame work;
- memory and frame budget;
- fallback or unsupported behavior.

The original implementation is authoritative for behavior. Proven DS software
can inform hardware architecture, but it does not redefine source behavior.

## Build-time specialization

Move stable work into host tools:

- texture quantization/swizzling;
- palette assignment;
- tile/map generation;
- mesh packing and strip conversion;
- fixed-point conversion;
- animation/keyframe simplification;
- lookup tables;
- script/data compilation;
- compression and pack-file indexes;
- command templates for immutable draw data.

Converters should be deterministic, validated, versioned, and able to emit
human-readable manifests/previews.

## Runtime data transformation

Replace source-format objects with DS-runtime forms:

```text
Source asset -> host converter -> compact DS asset -> scene preparation ->
hardware-resident handle
```

Avoid keeping the source object as the runtime API when it forces repeated
interpretation. A runtime handle should be compact, validated, and tied to a
resource generation.

## Fixed-capacity systems

The DS benefits from explicit maxima:

- actors;
- projectiles/particles;
- draw items;
- matrix palette entries;
- sprites/OAM slots;
- audio voices;
- messages;
- stream buffers;
- collision contacts.

When capacity is exceeded, use an intentional policy: reject, prioritize,
coalesce, evict, or degrade. Silent heap growth is not a policy.

## Scene arenas and generations

Allocate scene-owned objects from an arena and reset them together, but pair the
arena with a generation. Asynchronous handles/messages carry that generation.
Before reset:

1. stop producing new work;
2. drain/cancel DMA, GX staging, audio, ARM7, and storage users;
3. invalidate handles;
4. reset the arena;
5. increment generation;
6. initialize the next scene completely.

This prevents late completions from writing into reused memory.

## Renderer ownership

Convert a generic source renderer into explicit DS owners:

- visibility/list builder;
- matrix/animation preparation;
- main 3D submitter;
- main/sub 2D/OAM owners;
- VRAM/texture allocator;
- frame finalizer.

Each draw batch receives prepared DS-native data and sets its own persistent
state. Avoid a per-command interpreter unless dynamic behavior genuinely
requires it.

## Approximation hierarchy

Only when the project explicitly permits approximation, prefer controlled
changes with a stated error or visual/behavioral acceptance criterion:

1. preserve gameplay/collision timing;
2. preserve silhouettes, readability, and key animation poses;
3. reduce update frequency with interpolation;
4. precompute or simplify fixed assets;
5. reduce particles, bones, materials, lights, or background motion;
6. replace expensive effects with 2D/hardware alternatives;
7. drop low-value detail explicitly.

Tie approximations to acceptance tests rather than vague “looks close” claims.

## Choose a runtime-supported concurrency model

Calico supports native and standard threads. Keep useful worker threads when
the runtime, priority/blocking behavior, and memory budget support them; do not
rewrite every thread as a state machine just because the target is DS.
Depending on the workload, desktop worker jobs can become:

- build-time tools;
- incremental main-loop state machines;
- supported ARM9 worker threads when the selected runtime provides them;
  Calico has priority scheduling without timeslicing, so workers must block
  appropriately and shared data still needs synchronization;
- bounded DMA overlaps;
- ARM7 services only when hardware ownership fits;
- double-buffered producer/consumer stages.

Do not recreate a desktop threading model with cross-CPU busy waits.

## Filesystem conversion

Replace arbitrary runtime path lookup with:

- generated asset IDs;
- pack-file table of contents;
- bounded scene preload lists;
- sequential streaming queues;
- explicit save-data layer.

Keep host paths and source filenames out of hot target code where possible.

## Timing conversion

Map source timing deliberately:

- source tick rate;
- DS logical tick rate;
- presentation rate;
- animation sampling rate;
- audio/event timing;
- pause and catch-up behavior;
- integer/fixed timer units.

Do not bind gameplay speed to achieved frame rate unless the source did so and
the behavior is intentionally preserved.

## Input conversion

Normalize DS buttons/touch into source actions once per update. Preserve edge,
hold, repeat, and chord semantics. Define touch behavior where the source had no
analogue rather than letting UI and gameplay independently interpret raw input.

## Audio conversion

Map source voices/events into a bounded DS voice allocator with priority,
stealing, loop, pitch, pan, and lifetime rules. Convert samples offline and
separate music streaming from short resident SFX.

## Validation layers

Use multiple oracles:

- host tests for parsers, converters, fixed math, and behavior logic;
- generated asset previews/manifests;
- emulator for fast integration and deterministic captures;
- hardware or validated timing model for cache, storage, audio, and performance;
- scene-specific visual/gameplay comparisons.

Do not require exact binary identity when semantic equivalence is the goal, and
do not call a visual match behaviorally correct without gameplay tests.

## Anti-patterns

- “Make all code fixed point” without format/range design.
- “Use DMA everywhere.”
- “Move it to ARM7.”
- “Put every hot symbol in ITCM.”
- “Reduce triangles” without identifying command/fill/state cost.
- “Keep the original generic renderer for fidelity” when it repeatedly
  interprets static data.
- “Load on first use” in visible gameplay.
- “Use raw pointers in IPC because both CPUs share RAM.”

## Review checklist

- [ ] Source behavior and accepted approximations are explicit.
- [ ] Runtime data is DS-native rather than source-format interpretation.
- [ ] Stable conversion/preparation moved to build/setup time.
- [ ] Capacities and overflow policies are bounded.
- [ ] Scene teardown drains all asynchronous consumers.
- [ ] Logic, rendering, animation, and presentation rates are deliberate.
- [ ] Validation includes both semantic and DS-hardware evidence.
