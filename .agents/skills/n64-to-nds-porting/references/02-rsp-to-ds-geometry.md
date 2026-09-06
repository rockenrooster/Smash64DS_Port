# 02 — Compile RSP geometry semantics, not GBI syntax

## The high-value boundary

For immutable source lists, aim for:

```text
source GBI + assets + known entry state
    -> dialect-aware host decoding
    -> versioned vertices + ordered material/draw records
    -> DS-native static geometry + small dynamic binding records
    -> direct runtime submission
```

For code-generated geometry, often port the producer directly instead: the code
already knows which vertices/materials it intends. Do not generate an N64 list,
interpret it on ARM9, unpack its fields, convert its vertices, then issue GX every
frame when a typed DS draw can express the same result.

A host reference interpreter is useful for conversion/debugging. It need not be
a production runtime dependency.

## Identify the actual source dialect

Use the source microcode and matching GBI headers, not a generic opcode list.
Command encoding, cache capacity, matrix behavior, and extensions vary. Record
supported operations and fail on unknown reached operations. Expand call/branch
control flow with correct entry state, termination, stack bounds, and cycle/error
handling. A child list does not implicitly reset vertex, segment, or material
state. Task boundaries may establish a new state contract; infer it from the
source, not from the filename of a list.

Include data generated or patched at runtime in the support inventory. Static
asset coverage alone does not prove that combat, cutscenes, menus, or effects are
covered. A pointer-stable list whose words change is not immutable.

## Most important rule: version the loaded vertex

The N64 vertex command loads selected cache slots and transforms coordinates
using the then-current matrices. Later matrix loads do not retroactively replace
those coordinates. Partial vertex loads leave other slots live. Lighting and
texture-coordinate processing also have state-dependent load behavior.
[N64 vertex command][vertex] [Lighting state][light]

For each loaded vertex, capture an immutable semantic identity containing enough
to reconstruct its source result:

```text
source data identity/version + vertex index
transform expression/version at LOAD time
relevant lighting/geometry/UV-generation state at LOAD time
post-load edits/version
```

The source transform's pointer is not a version: it can point to a scratch matrix
that is rewritten before the later draw. Save values or a stable expression of
the required runtime inputs. Re-evaluate that expression for the correct frame;
do not permanently bake last frame's matrix into a supposedly static plan.

Example, where `A` and `B` are different transform states:

```text
matrix A; load vertices [a,b,c] into slots [0,1,2]
matrix B; load vertex d into slot 3
triangle(0,1,3)
```

The triangle uses `A(a), A(b), B(d)`. Assigning the whole triangle matrix B moves
two vertices incorrectly. Clearing the cache when loading B loses two required
vertices. Splitting the triangle into separate matrix batches cannot preserve
its topology. These bugs can appear as holes or seams far from the changed code.

## Handling mixed transforms without inventing skin weights

Classify by the vertex transform histories, not by the name of the object:

**One common transform.** Store local vertices and one live transform binding.
GX is a natural candidate for transform work only the renderer needs. Preserve
load-time lighting/UV semantics too; a common position transform does not prove
that applying one current lighting state is equivalent.

**Different rigid transforms within one triangle.** Transform the required
vertices into one common object/view space using their own source histories,
then submit the triangle under the remaining common transform. Cache shared
results once per required pose/version. Alternatively, compile a proven
cross-joint binding plan. This is not automatically weighted skeletal skinning;
using arbitrary weights changes the source.

**Source screen-space edits, special microcode, or generated clip-space data.**
Give them a dedicated path with explicit depth/viewport behavior or reject the
conversion until implemented. Do not run them through an ordinary local-space
mesh merely because they still reference three vertex slots.

Performing CPU transforms for every mesh is not the default solution to the few
mixed cases. Likewise, matrix readback per vertex can destroy useful overlap.
Keep the exceptional set small and visible in conversion reports.

## Post-load modifications require new versions

`gSPModifyVertex` can alter already-loaded color, ST, or screen-space fields
without performing an ordinary vertex reload. A previously emitted triangle must
keep its previous values. Update the cache slot to a new version; do not mutate
all earlier references to the same emitted vertex. Screen-coordinate edits need
separate treatment from color/ST edits. [Modify-vertex contract][modify]

The included [normalized plan compiler](../tools/compile_vertex_plan.py) models
this persistence and immutable versioning. It is an executable teaching/tooling
seed, **not** a raw Fast3D/F3DEX decoder, full RSP emulator, or GX exporter. Its
input state IDs must already represent immutable semantic versions. See the
[example contract](../examples/README.md) before using its output.

## Lower the semantic plan into useful target data

Build separate outputs for static vertex payloads, dynamic vertex subsets,
material runs, transform bindings, and resource requirements. Select direct
specialized paths for common geometry/material classes. Prepack commands only
when that removes interpretation without excessive copies or dynamic patching.
A giant command buffer rebuilt and flushed every frame can be worse than a small
typed loop over changing fields.

Use stable order by default. Reordering for material batching requires proof
about opacity, depth, equal-depth ties, blend effects, and any relevant source
ordering. Validate before stripping degenerate triangles: the source pipeline may
still carry state/events in the surrounding stream, and a geometric operation
can interact with quantization or winding differently on the target.

Partition target batches for actual DS limits and state ownership through
`nds-coding-practices`. Account for expanded emitted vertices, matrix/command
traffic, and transparent overdraw, not just original indexed vertex count. DS
native command reuse does not make source RSP cache reuse free.

## Preserve triangle-time state too

Load-time vertex history is only part of the pipeline. A source triangle also
uses effective draw-time rendering state, and the standard triangle command's
face flag selects the vertex supplying a flat-shaded face's color or normal.
Do not lose that selection when rotating indices or reordering triangles.
Represent it explicitly, lower it into equivalent per-corner data, or reject the
unsupported case. [Triangle command][triangle]

## Cache keys and invalidation

A cache of prepared draw data must include every input it depends on: source
asset generation, pose/blend generation, attachment/root transform, camera when
view-dependent, material/texture/palette version, UV animation, and mutable
geometry. Use cheap semantic dirty/version inputs rather than hashing huge arrays
every frame. Do not reuse on “same animation frame” if a blend or camera changed.

Reserve/validate resources before submission. If the selected path is unsupported,
choose a complete fallback before writing any GX state. A fallback after partial
submission can duplicate geometry, mix state, or exceed the resource budget.

[vertex]: https://ultra64.ca/files/documentation/online-manuals/functions_reference_manual_2.0i/gsp/gSPVertex.html
[light]: https://ultra64.ca/files/documentation/online-manuals/functions_reference_manual_2.0i/gsp/gSPLight.html
[modify]: https://ultra64.ca/files/documentation/online-manuals/functions_reference_manual_2.0i/gsp/gSPModifyVertex.html

[triangle]: https://ultra64.ca/files/documentation/online-manuals/man/n64man/gsp/gSP1Triangle.html
