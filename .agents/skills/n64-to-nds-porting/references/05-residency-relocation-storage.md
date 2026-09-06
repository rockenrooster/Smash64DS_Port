# 05 — Source closures, native scene packs, and bounded residency

## Port the live representation, not the source file boundary

N64 files/overlays can group setup data, runtime animation, display lists,
relocation tables, previews, and shared references into one source unit. Loading
that unit unchanged may retain much more than the DS runtime needs. Repacking
can be valuable, but deletion must follow the runtime's actual consumers.

Prefer native scene/match residency when content choices are known at a loading
boundary. Do not introduce gameplay paging just because a source closure does
not fit. First eliminate source-only retained representations and repeated
conversion. Streaming remains appropriate for genuinely sequential or bounded
content when its worst-case service requirements are proven.

## Safe live-set construction

Define the post-setup runtime roots from code and asset schemas, not only from a
successful trace. Include selected actors, shared systems, all reachable actions,
late-created weapons/effects/items, callback/script tables, animation/material
channels, collision data, alternate forms, audio, and any asynchronous consumer.
Those categories are examples; the game's actual code decides the roots.

Extract **typed references** from known structures and custom relocation formats.
Edges must represent reads, writes, address identity, and retained pointer ranges.
For `(base object, interior offset)` retain the owning object or a proven safe
subobject with all references rewritten. Array stride/pointer arithmetic can
require more than the currently addressed element. Aliasing can make two equal
byte ranges non-interchangeable.

Compute the transitive closure over these ownership units. Traverse cycles and
cross-file references; account shared objects once without merging unrelated
identities. A second actor can reuse an immutable asset while still requiring its
own mutable instance state.

Unknown reachable reference semantics require one of two honest results: retain
the entire opaque object and a conservative externally established closure, or
stop the shrink/conversion until its schema is understood. “No pointer-like words
found” and “not accessed in our tests” do not establish that an object is dead.

The included [live-set tool](../tools/live_set.py) computes a deterministic
**whole-object** closure/layout from supplied metadata. It rejects reachable
unknown edges and missing roots. It does not extract or prove that metadata,
perform relocations, or authorize removal of individual bytes. Its result is
conditional on the supplied graph. See [fixture](../examples/live_set.json).

## Repack with a reversible map

Emit a mapping from each supported source `(object ID, offset)` to a target object
or target span, plus source provenance. Validate every fixup's kind, extent,
alignment, null policy, and lifetime. Write the final target field only after
resolving the complete dependency. Fail conversion on unknown fixups rather than
copying the original N64 address into a target pointer.

Keep original source data and mapping on the host for comparison. On DS, retain
only metadata needed by live consumers. For migrated render paths, a source
geometry blob can become unnecessary once all references use a native plan; for
unmigrated consumers, it is still live regardless of the new renderer's needs.

Byte-level compaction is a later, stricter optimization. It needs exact typed
layouts, subobject extents, all address arithmetic, relocation rewriting, and
identity rules. Whole-object closure is deliberately more conservative.

## Admission uses sets and peak lifetimes

For a scene selection `S`, a useful main-RAM model is:

```text
peak(S) = permanent code/runtime/state
        + union of selected immutable runtime closures
        + sum of mutable per-instance state
        + active geometry/pose/audio caches
        + maximum simultaneous transition/decode/transfer scratch
        + stacks and required reserve
```

Do not add full per-actor closures if they share objects; do not deduplicate
mutable state because the actors use the same source character. Check every
supported legal combination when the roster/set space is small enough, or use a
conservative mechanically justified bound when it is not.

RAM, VRAM banks, texture data intervals, palette intervals, handle/table slots,
geometry capacity, transfer bandwidth, and CPU cost are separate constraints.
Passing one byte total does not imply the content fits. Generate material/texture
requirements with the native draw plans so admission sees actual demand rather
than source filenames.

Stage the next scene deliberately. The maximum footprint may occur while old and
new assets overlap, while a decompressor has both compressed and decoded buffers,
or while a DMA/audio/GX consumer still holds the old data. A lower steady-state
heap cost does not prove a safer transition.

## Menus and previews are not gameplay packs

A selection screen usually needs icons, names, portraits, palettes, and a bounded
preview, not every selectable actor's gameplay closure. Give previews a separate
native representation. On selection changes, reject stale completion by request
or scene generation. Do not let an old asynchronous read install the previous
selection's texture into the current preview.

Frequent short reads can make an otherwise small lazy UI slow. Pack related
metadata/previews into an indexed sequential-friendly layout and use bounded
read-ahead or caching according to the actual DS storage path. Benchmark the
selected filesystem/device or accepted timing model; host filesystem latency is
not evidence about that path.

## Decompress once into the right lifetime

Prefer independently loadable chunks sized for useful access, not one huge
compressed archive requiring full decode to reach any actor. Where practical,
read/decompress directly into a final owned native buffer, with bounded staging
only when alignment, decoder, transfer, or layout requires it. Do not retain both
formats after all source consumers have been retired.

No synchronous texture decode/read is an acceptable hidden default in the active
frame. A new effect can use preloaded data, a bounded resident bank, or an explicitly
admitted streaming scheme. For actual streaming, specify demand bound, buffer
capacity, completion deadline, consumer ownership, and underrun behavior. Moving
a blocking read to another CPU is not a proof that its deadline becomes safe.

## Local, explicit failure

Admission failure before scene start should identify the limiting resource and
selection. Capacity exhaustion must not globally reset a texture table, return
someone else's handle, or overwrite a still-live arena. Optional degradation is
allowed only under the project's explicit policy and must be local to that
content; required behavior cannot quietly disappear.

Use `nds-coding-practices` for actual allocator, cache/DMA, VRAM, storage API,
async teardown, and SDK memory-layout contracts.
