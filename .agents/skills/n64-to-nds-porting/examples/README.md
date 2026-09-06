# Original examples and integration boundaries

The C headers use only standard integer/boolean/size types. They perform no target
register access, filesystem operations, cache maintenance, allocation, or SDK
calls. Their portability makes host/ARM tests useful, but does not turn them into
a complete DS port or a hardware-tested renderer.

## `n64_data.h`

Includes overflow-safe span validation; unaligned-safe canonical big-endian
reads; a strict normalized segmented-span resolver; standard N64 split-halfword
matrix-element decoding; RGBA5551-to-DS-direct-color bit rearrangement; and
unsigned two-halfword packing.

Validate the containing record once before using unchecked leaf readers. Input
bytes must be normalized to canonical N64 order. The segmented helper deliberately
rejects an upper byte outside 0..15; actual source microcode may mask upper bits,
use different addressing, or support custom commands. Handle that in the
source-aware decoder before calling the normalized helper. Zero-length resolutions
are rejected; no output pointer/value is changed on checked failure.

The color helper returns a **host numeric word**, not a VRAM write, a generic
palette conversion, or an automatically little-endian file buffer. Matrix values
remain in source element order; convention/scaling conversion is separate.

## `n64_numeric.h`

`port_floor_shift32(v, shift)` implements floor division by a power of two for
`shift <= 31`; that precondition belongs to the caller. It avoids negative signed
shifts and negation of the minimum signed value.

`port_q16_to_q12_rne()` uses nearest/ties-to-even conversion. That is one explicit
policy, **not a claim of identical source RSP rounding**.

`port_s16_to_v16()` scales an integer source coordinate by `2^-scale_log2` and packs
it into signed Q4.12 with explicit floor rounding if bits are discarded. It checks
scale 0..27 and range; output stays unchanged on failure. It does not compute a
suitable model origin/scale or update a matrix. Derive those with the geometry's
space convention. All three functions must be used only where their numerical
policy fits the project.

## `tick_ratio.h`

Initialize with source ticks per **caller-defined clock pulse** as a rational
numerator/denominator. `port_tick_pulse()` adds all due work; `debt` stays until
`port_tick_commit_one()` acknowledges an actually completed update. Overflow
leaves the clock unchanged and returns failure. Setup has integer divides; the
steady pulse path does not.

Only mutate initialized clock state under one owner. The helper is not thread/
IRQ-safe by itself and is not a timer API. Feed elapsed pulses even when no frame
was rendered. Input queueing, source rate/region, pause, catch-up budget, overload
handling, and presentation policy remain with the engine.

## Normalized vertex-history compiler

```sh
python3 tools/compile_vertex_plan.py examples/vertex_history.json vertex_plan.json
```

`vertex_history.json` demonstrates a rigid triangle, a child-list partial reload
under another transform, a mixed-transform triangle, and a post-load ST patch.
Expected output: 3 triangles, 5 immutable vertex versions, 2 mixed-position-
transform triangles. The first triangle still references the original version
of the patched vertex.

### Input contract

Top-level keys are `schema: 1`, `cache_slots`, `entry`, and `lists`. Each list has
an explicit `end` or tail `branch`. The cache size is supplied by the already-
normalized profile, not guessed by this tool. Supported operations are:

| Operation | Meaning |
|---|---|
| `state` with `transform`, `vertex_state` | Select immutable semantic IDs for future loads |
| `material` with `id` | Select the effective immutable material ID for future triangles |
| `load` with `first`, `sources` | Overwrite only the specified slots with new immutable versions |
| `patch` with `slot`, `field`, `value` | New version with normalized RGBA (4 bytes) or ST (2 signed halfwords) override |
| `tri` with 3 `slots` | Capture the currently live versions in source order |
| `call` / `branch` with `list` | Share state/cache; call returns, tail branch stops caller |
| `end` | Return/terminate without an implicit state reset |

IDs are semantic identities of immutable data/expressions, not mutable pointers.
The source-aware producer must account for all relevant transform, lighting,
texture-generation, vertex-load, and patch semantics. A `state` operation supplies
both transform and vertex-state identities; it is not a raw `gSPMatrix` opcode.
Triangle face flags and flat-shading selection are not modeled: the producer
must lower them into equivalent semantic data or reject those cases.
ST overrides are retained values; the tool does not implement their sampling
math. It preserves load dependencies rather than computing transformed vertices.

`single_position_transform` classifies only position histories. It does **not**
certify that one current GX lighting/material state can represent the whole
triangle. Output is semantic JSON, not hardware-ready geometry or GX commands.

Unsupported opcodes, screen-space patches, undefined reads/state, missing lists,
cycles, and bounded expansion/depth violations fail. Only the entry's reached
control flow is compiled; this is not an all-assets coverage claim. Dynamic
branches, raw GBI parsing, microcode emulation, clipping, lighting, texture decode,
actual transform evaluation, binary export, and native submission are deliberately
outside this example.

## Conservative whole-object live-set analyzer

```sh
python3 tools/live_set.py examples/live_set.json live_layout.json
```

Input declares an externally justified complete runtime root set and canonical
nonoverlapping ownership objects. Every object has `id`, `size`, power-of-two
`align`, `edge_status` (`complete` or `unknown`), and `edges` to ownership IDs.
Interior pointers, writes, pointer arithmetic, identity, late creation, and
asynchronous retention must already be represented in that graph.

The analyzer traverses cycles, deduplicates shared **identities**, rejects unknown
reachable edges or missing targets, and produces stable whole-object layout
metadata. `roots_complete: true` is a caller assertion, not evidence the tool has
proven completeness. An unreachable opaque object can be excluded only under
that external root/edge contract.

The fixture keeps 5 objects totaling 3,224 object bytes, including the late-created
projectile; a setup-only preview is excluded under the stated graph. Alignment
padding is reported separately. The emitted layout requires the stated base
alignment and excludes any outer allocator/trailing alignment overhead.

This tool does not scan game code, discover roots/edges, shrink individual byte
ranges, perform fixups, write a binary pack, or prove actual DS residency. Use it
as a small replaceable building block, not a certificate that an asset is safe
to delete. Improve unknown metadata or retain a conservative opaque closure.

## Safety of example integration

The CLI tools validate the entire reached plan/closure before writing output.
Malformed input exits nonzero; tests check that such failure does not overwrite
an existing output. Successful writes still use ordinary filesystem semantics,
not a transactional asset-build system. Integrate through the project's build
rules and avoid treating stale output from a failed conversion as a fresh build.
