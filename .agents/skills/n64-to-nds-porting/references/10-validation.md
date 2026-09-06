# 10 — Validate the changed semantic boundary

Do not require a full audit for every edit. Use tests that challenge the actual
conversion and report exactly which evidence exists. Keep the source/debug path
available long enough to distinguish a source behavior from a target bug.

## Use the right oracle

The source/decompilation defines intended behavior subject to the project's
revision and modifications. Its host build is a useful oracle only after
accounting for numeric/ABI differences: endian data, float evaluation, rounding,
undefined source C, compiler options, host pointer widths, and execution order.
A matching N64 execution trace can resolve behavior a host reconstruction does
not faithfully reproduce.

Separate comparisons:

| Boundary | Useful comparison |
|---|---|
| Binary decode/relocation | Typed values, bounds, target mapping, null/interior references |
| Source display-list lowering | Ordered semantic triangles, vertex history, materials, state dependencies |
| Native numeric packing | Range, rounding, boundary values, justified error or exactness |
| Gameplay | Canonical state and event stream at each source tick |
| Pose/attachments | Required transforms/positions at the consuming tick and coordinate space |
| Rendering | Native captures against the project's visual acceptance contract |
| Audio | Event/voice timing, pitch/envelopes, loop boundaries and repeated transitions |
| Residency | Worst legal selection, peak overlap, table/byte capacities, repeated teardown |
| Performance | Same workload/configuration, actual path use, end-to-end service costs |

Compare canonical serialized state rather than raw `memcmp` on structs with
padding, uninitialized bytes, pointers, or different target layouts. Use stable
asset/object IDs for pointer identity. A checksum can find divergence; a detailed
first-divergence record explains it.

## Adversarial cases with high return

**GBI/geometry:** child list inherits and changes state; tail branch stops the
caller; partial reload preserves older slots; a triangle crosses transform
histories; a color/ST patch must not change an earlier triangle; source matrix
scratch is overwritten; lighting changes between loads; unsupported screen-space
edits; dynamic source-list patch; clipping/near-plane/winding edge cases.

**Materials:** same pixels with another palette generation; negative/edge UVs;
shifted tile and different wrap mask; padded image; animated alpha; overlapping
translucent geometry; equal-depth surfaces; source rectangle endpoint conventions;
material swaps under the same object ID.

**Numbers/animation:** negative halves/ties; minimum signed value; largest model
extent; large translation plus small scale; noncommuting transforms; nonuniform
scale; fractional/reverse animation; loop boundary; blend transition; joint effect
sampled before versus after pose update; paused camera movement.

**Gameplay:** multiple source ticks per presentation; no rendered frame; queued
press then release; actor spawn/delete during traversal; equal-priority collisions;
RNG consumption across paths; crossed animation events; pause/unpause; reset and
rematch. Never replace these with a single idle screenshot.

**Residency:** distinct heavy actors versus repeated instances; late-spawn action;
opaque unknown references; interior pointer; shared mutable data; old/new overlap;
texture table full despite free VRAM; failed read/decode; stale preview completion;
repeated entry/exit until leaks or generation errors become visible.

## Source traces do not prove completeness

Traces are strong counterexample finders but only cover executed paths. A trace
that never observes a rare weapon does not authorize deleting its assets. Combine
typed schema/reference coverage and supported-state reasoning with dynamic tests.
For unknown content, keep a conservative complete path or fail explicitly.

A plan compiler's semantic comparison is also not a pixel proof. DS rasterization,
clipping, depth, material precision, and resource behavior require the appropriate
integration checks. Conversely, a visually similar scene does not prove that its
hitboxes, RNG, or event sequence is preserved.

## Performance claims require the right execution environment

Use the consuming project's declared timing authority: target hardware or a
validated model as specified there. Do not import another project's authority,
frame gate, or emulator settings. Ordinary emulator execution can check behavior
without proving timing, cache, transfer, or storage performance.

Host tests prove the tested portable algorithms on that host. Cross-compilation
proves only what was actually compiled. A freestanding ARM object is not a linked
`.nds` ROM, and a linked ROM is not a successful run. Keep these labels distinct.
Verify that optimizations have not reduced geometry, source updates, event counts,
or required work; a faster wrong path is not a win.

## Practical completion statement

For a normal code task, give the concrete change, why its semantic boundary is
safe, tests/builds actually executed, resource/performance effects actually
measured, and remaining integration limits. Do not create new progress rituals.
A short truthful result is better than an elaborate unsupported certification.

Pack maintenance tests and their actual run results live in
[tests/README.md](../tests/README.md) and
[tests/REVIEW_RESULTS.md](../tests/REVIEW_RESULTS.md).
