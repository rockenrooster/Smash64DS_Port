# 08 — High-performance paths that keep the required work

## Optimize the port boundary before optimizing instructions

Highest-leverage candidates are often source-only repeated work: producing then
interpreting GBI, decoding animation commands, resolving source addresses,
rebuilding static material state, transforming the same joint repeatedly, or
converting the same texture at every bind. Remove these before inventing assembly,
ARM7 offload, or a universal new renderer.

A practical order is: eliminate unnecessary work; compile static structure;
prepare invariant setup data; reuse live results; reduce copying/state traffic;
choose appropriate native operations; then inspect arithmetic, placement, and
instruction selection. This is a prioritization strategy, not proof that any
specific change is faster.

## Specialize by stable facts

Select a concrete path at build/load/action-transition time when possible:

```text
rigid opaque geometry -> native vertex block + matrix binding + material ID
mixed-transform geometry -> small cross-transform binding plan
UV animation -> static geometry + live UV parameters
color animation -> static geometry + live material colors
procedural geometry -> bounded native vertex producer
unsupported semantics -> explicit dedicated path or conversion failure
```

Keep variant count proportional to real differences. An enormous cross product
of all flags can inflate linked code, cold metadata, converter complexity, and
instruction-cache pressure. Share a cold dispatcher or a common leaf kernel when
that is a better trade than duplicating everything.

Do not compile away a branch whose input can change without rebuilding the plan.
Version or invalidate at the true owner: action transition, palette update,
attachment change, animation blend, camera update, resource generation, or source
script patch.

## Dynamic patching is not automatically cheap

A prebuilt list helps most when large immutable portions can be reused without
copying and the live portion is small. Count copies, patch scans, flushes, DMA
setup, waits, and command traffic. Recopying a mostly static list into a new
buffer every frame can erase the saved interpretation.

Prefer a small typed binding array for dynamic inputs over an untyped list of
“patch this byte offset” records when the latter needs checks and interpretation
on every draw. But do not invent a patch manager if two direct stores under the
existing owner solve the task.

Immutable static blocks still have a target placement and lifetime. `const` in
linked code is not proof of cartridge residency. The companion owns native list
format, cache handling, DMA-visible placement, and submission APIs.

## Data layout: fewer bytes accessed, not merely fewer bytes stored

Split hot fields from source-only metadata when access patterns justify it.
A dense actor update may need flags, position, velocity, and action state, not a
large string/debug/render/relocation record. Compact indexes can reduce retained
pointer graphs, but repeated lookup of several tables can outweigh the savings.
Hoist validated lookups outside inner loops and bind once for the appropriate
lifetime.

Do not overpack live CPU structs into unaligned fields. Keep wire formats compact
and explicit; decode hot target state into suitable alignment. Structure-of-arrays
can help a uniform sweep, while a small array-of-structs can be better when each
iteration consumes all fields. Measure the actual loop, not a desktop slogan.

## Codegen inspection that answers a porting question

Inspect the affected target object/function for unintended software-float calls,
float/double conversion, variable divides, excessive spills, indirect calls,
repeated pointer resolution, and constant data movement. A helper name is a
clue, not a verdict: setup-time helpers can be acceptable, and a required wide
multiply may be efficient. Count dynamic frequency and caller context.

Use the project's real compiler/flags and inspect the final linked layout for
placement claims. Freestanding Clang ARM output can reveal an obvious issue but
is not a substitute for devkitARM/SDK linkage or timing. The included tests make
that boundary explicit.

Do not enable broad `-ffast-math`, narrow types globally, or change alias/overflow
assumptions to make a benchmark win without checking source semantics. ARM versus
Thumb, inlining, code size, TCM placement, and hot/cold splitting are workload-
and runtime-dependent. Use `nds-coding-practices` for the target details.

## Main-frame economics

Measure source update, pose/collision, native preparation, command submission,
GPU/FIFO waiting, uploads, storage, audio service, and idle wait as appropriate.
Avoid double-counting nested intervals or adding parallel durations as though
they were sequential. End-to-end service time and over-budget frames decide
whether the optimization reaches the goal.

Compare the same content, inputs, build mode, cache/timing configuration, and
sampling window. Report representative central/tail costs and the over-budget
fraction rather than only a best case. Confirm the intended fast path actually
runs and content/geometry/event counts did not silently fall.

For a rough planning bound, if a fraction `p` of runtime becomes `s` times faster,
Amdahl's idealized total speedup is `1 / ((1-p) + p/s)`. Overhead, stalls, and new
memory traffic can make the real result worse. Use that arithmetic to avoid
spending days on a tiny slice, not as a measured speed claim.

## Precompute decisions need four columns

For each candidate, compare CPU saved, ROM added, peak RAM added/removed, and
load/stream bandwidth. Examples:

| Candidate | Likely benefit to test | Common hidden cost |
|---|---|---|
| Native static mesh/command data | No per-frame source decode | Expanded payload, per-instance copies, submission overhead |
| Sampled animation | Less evaluator work | Active pose working set, random transition decode, fractional playback |
| Prebaked material variants | Simpler runtime equation | Variant explosion, extra palettes/textures, residency pressure |
| Precomputed collision cells | Fewer candidate tests | Duplicated boundary geometry, ordering restoration, dynamic movement |
| Cached pose results | Shared joint/attachment work | Dependency tracking, retained matrices, stale generation bugs |

Favor transformations that remove repeated source work without changing behavior.
Only consider reduced cadence/content/precision as an explicitly authorized
adaptation, with a named acceptance boundary. Never report a speedup caused by
missing work as a successful optimization.
