# 02 — Compile geometry semantics, not command spelling

For static content: `GBI + known entry state -> dialect-aware host decode -> versioned vertices + ordered material draws -> native geometry + live bindings`. For runtime producers, emit native data directly where possible. Preserve source control/state behavior without a target-frame N64 interpreter.

## State and vertex history

Use the actual microcode and GBI headers: opcode encoding, cache capacity, matrix rules and extensions vary. Bound calls, tail branches, recursion/steps and termination. Child lists inherit and can modify vertex, segment and material state; a filename is not a reset boundary. Include dynamically patched/generated lists in support coverage.

A loaded vertex carries its **load-time** transform and relevant lighting/UV-generation state. Partial loads leave other cache slots live. Later matrix changes do not retroactively retransform them. Track immutable identities:

`source data/version + index + load transform expression/version + load attribute state + post-load edit version`

A matrix pointer is not a version: source scratch storage may be overwritten. Retain values or a stable expression of the actual live inputs, evaluated for the required frame/pose.

For `matrix A; load a,b,c into 0,1,2; matrix B; load d into 3; tri(0,1,3)`, the triangle uses **A(a), A(b), B(d)**. Assigning B to the whole triangle, clearing the cache, or splitting the triangle into different matrix batches changes geometry.

| Case | Native lowering |
|---|---|
| Common rigid transform | Local geometry plus one live transform; also preserve load-time lighting/UV semantics. |
| Mixed rigid transforms in one triangle | Compute needed vertices in a common space under their own histories, or a proven cross-joint binding plan. Cache shared results by pose/version. Do not invent skin weights. |
| Screen-space edits/special microcode | Dedicated viewport/depth-aware native path, or explicit unsupported conversion. |
| Dynamic generated geometry | Bounded native producer with matching semantic state and resource budget. |

CPU-transform only the cases needing it; avoid matrix readback per vertex. `gSPModifyVertex` color/ST edits create a **new version** for future uses, never mutate already-emitted triangles. Screen-coordinate edits are not ordinary local-space edits.

## Draw state and lowering

Capture effective draw-time material and triangle face-vertex selection for flat shading. Rotating indices or batching triangles must not change the selected color/normal or equal-depth/alpha order. Preserve surrounding state/events even when geometric emission is removed for a proven reason.

Emit static payloads, dynamic subsets, material runs, transform bindings and actual resource requirements. Native command prepacking is useful only when interpretation saved exceeds copying, patching, flushes and waits. Budget expanded vertices, command/matrix traffic, clipping, alpha overdraw and DS capacities—not only the source indexed count.

Cache keys include asset, pose/blend, root/attachment, camera when view-dependent, material/palette, UV animation and mutable-geometry generations. “Same animation frame” is insufficient after a camera or blend change. Use semantic versions rather than hashing entire arrays each frame.

The [plan compiler](../tools/compile_vertex_plan.py) is a normalized history model, **not** a raw GBI decoder, complete RSP emulator or GX exporter. Callers must supply immutable state IDs and lower unsupported face/screen behavior explicitly. Validate ordered semantic draws against a host/source oracle, then validate actual target pixels. Reject unsupported cases before any GX submission; alternate rendering must obey the native-only contract. [Sources](SOURCES.md).
