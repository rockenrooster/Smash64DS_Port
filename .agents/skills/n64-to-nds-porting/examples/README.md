# Examples and exact boundaries

The C headers use standard types only; they do not touch DS registers, allocate resources, or supply an SDK integration. Validate untrusted input before unchecked leaf readers.

| Header | Contract |
|---|---|
| [n64_data.h](n64_data.h) | Checked spans, canonical big-endian reads, normalized segment IDs 0–15, split `Mtx` decoding, RGBA5551 bit rearrangement and unsigned pair packing. Checked failures leave outputs unchanged. The color result is a host numeric word—not a generic palette conversion or VRAM write. |
| [n64_numeric.h](n64_numeric.h) | Floor shift (caller proves shift ≤31); Q16.16→Q20.12 nearest/ties-even; checked source-s16→v16 with scale 0–27 and floor rounding when discarding bits. These are explicit policies, not universal RSP equivalence. |
| [tick_ratio.h](tick_ratio.h) | Rational ticks per caller-defined pulse; debt is removed only after an actual completed update. Overflow is transactional. One owner; source clock, input queue, pause and overload policy remain external. |

## Normalized geometry

```sh
python3 tools/compile_vertex_plan.py examples/vertex_history.json vertex_plan.json
```

Input has `schema: 1`, `cache_slots`, `entry`, and `lists`. IDs represent immutable semantic values/expressions, not mutable pointers. Every reached list has explicit `end` or tail `branch`.

| Operation | Meaning |
|---|---|
| `state`: `transform`, `vertex_state` | Select load-time identities; both are supplied. |
| `material`: `id` | Select draw-time material identity. |
| `load`: `first`, `sources` | Replace only those cache slots with new immutable versions. |
| `patch`: `slot`, `field`, `value` | New RGBA (4 bytes) or ST (2 signed halfwords) version. |
| `tri`: three `slots` | Capture current versions in source order. |
| `call` / `branch`: `list` | Shared inherited state/cache; call returns, branch stops caller. |
| `end` | Return without a state reset. |

Fixture output: 3 triangles, 5 vertex versions, 2 mixed-position triangles. Earlier draws retain pre-patch values. `single_position_transform` does not certify common lighting or material behavior.

This is **not** a raw GBI decoder, RSP emulator or GX exporter. Source normalization owns dialect, matrix/lighting/UV semantics, flat-face selection and dynamic behavior. Unsupported operations, screen patches, undefined reads, missing lists, cycles and expansion/depth overflow fail. Only reached control flow is validated.

## Conditional live sets

```sh
python3 tools/live_set.py examples/live_set.json live_layout.json
```

The caller supplies justified complete roots and canonical nonoverlapping objects with `id`, `size`, power-of-two `align`, `edge_status` and `edges`. Interior pointers, writes, identity, late creation and asynchronous lifetimes must already be represented. `roots_complete: true` is an assertion, not proof.

The analyzer traverses cycles, deduplicates identities, rejects reachable unknown/missing references and emits deterministic whole-object layout. The fixture keeps 5 objects / 3,224 object bytes; alignment padding is separate. It does not discover roots, perform fixups, shrink bytes, emit a game pack or prove DS residency.

## Normalized texture alpha

Import `decode_n64`, `encode_ds`, `decode_ds` and `validate_texture_alpha_draw` from [texture_alpha.py](../tools/texture_alpha.py). Python 3.10+ and the standard library suffice.

`decode_n64(data, format, count, tlut=..., tlut_format=...)` accepts tightly packed logical RGBA16/32, CI4/8, IA4/8/16 or I4/8. It requires exact byte length; CI4 takes the already-selected 16-entry TLUT bank, CI8 all 256 entries. TLUT mode is explicit RGBA16/IA16. No TMEM, row padding, source filter or material equation is guessed.

`encode_ds(rgba, format)` supports direct RGBA/RGB and indexed RGB4/16/256, A3I5/A5I3. RGB reduction takes the top five channel bits; opaque RGB15 colors are deduplicated, not perceptually optimized. Ordinary cutouts reserve zero and remap texels, including opaque black. Capacity overflow, graded→binary conversion and `GL_RGB` cutouts fail. Graded alpha needs exact representability or explicit `allow_alpha_quantization=True`; quantization chooses the nearest actual native alpha level, ties lower. No 4×4 compressor is implemented.

`decode_ds()` independently unpacks that subset for mask/alpha checks. `validate_texture_alpha_draw()` checks only a simple modulation recipe with a zero-threshold global policy. It does not inspect actual registers, palette allocation, source equations, depth/IDs, 2D composition or pixels. Dimension/bank admission and animated remap generations remain caller-owned. Resolve intended source alpha **before** treating encoded output as a material conversion.

## Host CLI I/O

The geometry/live-set CLIs reject duplicate JSON keys, NaN/Infinity/overflowed floats, oversized reads and invalid schemas. Output is serialized to a temporary sibling, flushed, then atomically replaced after success. Failed validation/write/replace retains the existing output. This host replacement is not a DS filesystem durability guarantee; build rules must still reject stale outputs after any failed converter.
