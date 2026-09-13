# 01 — ABI, bytes, and address domains

Recompiling C does not convert serialized data. Normalize the source container's byte order once, then decode typed records using actual game schemas. Check widths, enum/long/pointer size, alignment, unions, bitfields, signed char and float bits. Avoid `fread()` into native structs, C bitfield overlays for GBI, unaligned typed loads and pointer punning. Mixed source and already-converted streams cannot share a blind byte-swap pass.

| Source reference | Required treatment |
|---|---|
| ROM/file offset | Bounds-check against that container; not a RAM pointer. |
| Normalized segmented address | Segment ID + offset under the current segment mapping/generation. |
| CPU virtual/KSEG address | Resolve using the actual source memory map and object identity. |
| Physical address | Resolve against the source address domain, not host or DS virtual memory. |
| Relocation index/tagged word | Decode its schema before interpreting payload bits. |
| Interior pointer | Retain the owning object's lifetime and validate offset, extent and alignment. |

Do not fix arbitrary address words by masking high bits. Source dialects may mask segment fields differently; the [helper](../examples/n64_data.h) accepts only **canonical normalized** segment IDs 0–15 and rejects other upper bytes. A dialect decoder must normalize first. Mid-stream segment changes invalidate cached resolutions unless the mapping generation is included.

The standard N64 `Mtx` stores 16 integer halfwords followed by 16 fractional halfwords, not an array of contiguous native Q16.16 words. Decode logical elements, then deliberately convert precision, order, handedness and multiplication convention. Test basis vectors and noncommuting transforms; a symmetric model can hide a transpose error.

Use explicit-width unsigned operations for intended bit packing/wrap, checked products and subtraction-based span bounds. Preserve source rounding where branches or endpoints depend on it. Do not replace floating expressions, lookup tables or signed arithmetic globally merely because the final output is fixed-point.

Represent tool references as `(object ID, offset, kind)`. Validate each fixup's null policy, required span, alignment and lifetime before installing target fields. Distinguish absent optional data from malformed references. Equal bytes cannot be merged when mutation, address identity, adjacent sentinels or pointer arithmetic is observable.

Keep fixed semantic IDs separate from compact build-specific enums/counts. Prove every enabled ID indexes the full native table domain, including sparse configurations and the highest slot. Do not enable unrelated content to fill enum holes. Swizzle once at load or use native IDs/offsets; repeated hot-loop resolution is not inherently safer. [n64_data.h](../examples/n64_data.h) implements checked byte/span/matrix/color boundaries, not a ROM parser or relocation extractor. [Sources](SOURCES.md).
