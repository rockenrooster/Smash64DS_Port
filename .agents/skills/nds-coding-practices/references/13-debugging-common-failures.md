# 13 — Symptom-directed diagnosis

These are hypotheses, not diagnoses. Attempted, accepted, emitted and visible are different observations. Keep heavy logging outside hot loops/IRQs; debugger reads need cache visibility and optimized arguments need confirmation from compiled code.

| Symptom | Check before rewriting |
|---|---|
| Texture/sprite is an opaque rectangle | Decoded alpha, effective source alpha equation, native format/mask, `GL_RGB`, indexed remap + color-zero flag, `POLY_DECAL`, polygon alpha, palette/material generation; then [07](07-3d-gx.md). |
| Holes correct but faded edges wrong | Graded-alpha quantization, global blend/test settings, depth writes, overlap IDs/order and 2D destination layers. |
| Colors wrong | Endian/channel bits, palette mode/generation/bank, vertex tint/lighting, stale binds and native nibble order. |
| Sprite invisible or clipped | Engine/bank/mapping, OAM hidden flag, tiled/bitmap format, bitmap alpha, dimensions, priority, affine double-size and scanline OBJ limits. |
| Geometry missing/striped | Vertex-cache lowering, count/arity, matrix space/range, culling/winding, clipping, depth, resource lifetime, polygon/vertex counters and overflow flags. |
| Heavy rows or tearing | Measured raster overrun, unsafe video writes, excessive translucent overdraw, ordering or resource reuse; do not assert capacity overflow without evidence. |
| Works on host but not DS | Alignment/aliasing, endian or pointer ABI, signed UB, TCM visibility, byte writes, stale cache, missing service initialization and installed API differences. |
| Works only with logs/debug | Lifetime/race, uninitialized state, stack use, changed scheduling or compiler-sensitive UB; logging is not the fix. |
| ARM7 job never finishes | Channel/peer readiness, service image, blocked sender, queue overflow, priority starvation, IRQ blocking and missing acknowledgement. |
| Loading hangs/stutters | Return values, supported filesystem stack, tiny/random reads, storage latency, memory corruption, stale completion and resource capacity. |
| FPS drops after content grows | Actual active/resident set, hidden per-object work, source fallback, cache/VRAM/table pressure, transition overlap and repeated conversion. |

Use runtime-supported diagnostics (exception reporting, emulator logging, guarded counters) in safe context. Check their availability in the installed SDK. Stack failures need separate main/worker/IRQ/exception budgets; IRQ nesting is runtime-dependent and is not a Calico assumption. Preserve sticky hardware fault evidence before register helpers clear it.

Do not “fix” by globally resetting live caches, dropping required draws, disabling alpha, hiding the failing asset, switching to a prohibited renderer, or introducing an unbounded retry. Fail locally with resource/material provenance when recovery is not safe.
