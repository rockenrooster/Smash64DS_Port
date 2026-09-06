# 01 — N64 C, binary data, and address domains

## Recompiling C does not convert serialized data

Use the game's actual headers and exporter for its layout. Check fixed-width
fields, unions, pointer tagging, bitfields, signed `char`, enum size, `long`,
alignment, float bit patterns, and structs shared with tools. A 64-bit host and
an ARM target need not reproduce the original ABI. Do not read file records with
`fread(&native_struct, sizeof native_struct, 1, f)` unless the wire format was
explicitly designed and verified for that exact layout.

For binary assets: normalize the container's byte order once, then decode typed
records into a defined target format. Do not apply a whole-ROM byte swap to a
mixture of source data, already-converted little-endian fields, and byte streams.
Use unsigned masks/shifts for GBI words, not C bitfield overlays.

[Original helpers](../examples/n64_data.h) demonstrate checked spans, big-endian
reads, strict normalized segmented addressing, split-matrix decoding, color
bit-position conversion, and unsigned halfword packing. They deliberately do
not pretend to parse a game ROM or guess its relocation format.

```c
/* Wrong: alignment, byte order, aliasing, and address domain are all guessed. */
/* Vtx *v = (Vtx *)(uintptr_t)(*(uint32_t *)(blob + off)); */

/* Appropriate boundary: validate the enclosing record once, decode, then use
 * a resolver for the SPECIFIC address kind. */
if (!port_span_ok(blob_size, off, 4u))
    return false;
uint32_t word = port_be32(blob + off);
const uint8_t *source_vertices;
if (!port_resolve_segment(segments, word, vertex_bytes, &source_vertices))
    return false;
/* Decode the source record; do not cast source_vertices to a target struct. */
```

The fragment is an integration illustration, not a complete function. The
resolver assumes a normalized segmented word, not arbitrary source CPU addresses.

## Address kinds must be explicit

| Kind | Port treatment |
|---|---|
| Cartridge/ROM byte offset | Resolve against the normalized input file; bounds-check and translate to an asset ID |
| N64 CPU virtual/physical address | Resolve through the known source image/overlay map; do not turn it into an ARM pointer |
| RSP segmented reference | Resolve with the correct microcode's segment table at that command |
| Serialized file-relative offset | Resolve within that declared file/record, including relocation state |
| Custom external relocation | Decode the game's actual link format and follow cross-file references |
| Runtime code/data pointer | Replace with a valid target symbol, callback index, or owned target reference |

Standard documented segmented addressing uses a table selected by bits 24–27 and
a low-24-bit offset; that documentation masks the upper four bits. This is not a
license to apply the same mask to every address word. The helper in this pack
accepts only **canonical normalized** segment IDs 0–15, intentionally rejecting
other upper bytes. A dialect-aware decoder must perform any source masking or
special interpretation first. [N64 address map][address] [Segment macro][segment]

A segment register can be changed mid-stream. Cache resolved references only
when that segment's generation is part of the cache key, or resolve and freeze
them while compiling the correctly ordered command stream.

## Matrix memory is especially misleading

The standard N64 `Mtx` stores the integer halves of all 16 elements first, then
the fractional halves. It is not a contiguous array of native signed 32-bit
Q16.16 elements. Decode each logical element from the two halfword blocks before
changing precision or matrix convention. [N64 matrix definition][matrix]

The helper returns the source element order; it does **not** decide whether to
transpose, reverse concatenation, change handedness, or compensate vertex scale.
Use basis-vector and noncommuting transform tests for those decisions. A matrix
of plausible numbers and a symmetric model are weak tests.

## Arithmetic and C semantics are part of the port

Use explicit widths for source arithmetic. Reproduce intended wrap with unsigned
operations and deliberate conversion, not signed overflow. Check integer
promotion in masks, packed color/vertex words, negative shifts, and overflow
before narrowing. Avoid unaligned typed loads even if the host tolerates them.
For float reinterpretation use a size-checked `memcpy` or exporter bit decoding,
not pointer punning. Preserve source rounding where it changes decisions.

Do not replace `double` with `float`, `float` with fixed, or a lookup table with
host `sin()` just because output is eventually quantized. Intermediate values can
change branches, animation endpoints, and collision. Keep the reference numeric
path where needed until a narrower boundary is proven.

## Relocation and identity

Represent a source reference in tools as `(object/file ID, byte offset, kind)`.
Map it to an owned target object or span, validating required size and alignment.
Keep interior pointers attached to their base object. Do not merge equal bytes
if pointer identity, mutation, arithmetic across a span, or sentinel adjacency
is observable. A null, absent optional resource, and malformed relocation are
three different outcomes.

Swizzling a block once during load can be efficient when raw pointers remain
necessary. Generated IDs/offsets can reduce retained metadata when consumers can
be converted. Choose one boundary deliberately; repeatedly unswizzling and
re-resolving it in gameplay is not inherently safer.

[address]: https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro03/03-06.html
[segment]: https://ultra64.ca/files/documentation/online-manuals/functions_reference_manual_2.0i/gsp/gSPSegment.html
[matrix]: https://ultra64.ca/files/documentation/online-manuals/functions_reference_manual_2.0i/gsp/gSPMatrix.html
