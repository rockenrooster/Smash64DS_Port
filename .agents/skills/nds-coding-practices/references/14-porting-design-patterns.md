# 14 — Native representations and lifetimes

Port the source behavior, not its CPU/OS machinery. Compile static asset structure on the host, resolve stable references at load, bind action/material variants at transitions, and leave only genuinely dynamic inputs in the frame loop. The N64 companion owns display-list, TLUT, animation and gameplay semantics.

| Pattern | Contract |
|---|---|
| Scene-owned native pack | One validated admission boundary; complete live roots; shared immutable assets plus per-instance mutable state; peak transition/scratch budget. |
| Compact preview bank | Selection UI loads its actual icons/poses, not every gameplay closure. Generations reject stale selection completions. |
| Stable handles | Index + generation, validated by every asynchronous consumer; never reuse until old work cannot write/read the object. |
| Small event/state machine | Preserve ordering and completion without rebuilding an entire source scheduler. Bound queues and define overflow. |
| Native draw bindings | Immutable geometry plus typed live matrices/material values. Cache keys include all changing dependencies. |
| Load/decode/upload stages | Each buffer has an owner and release event; no first-use synchronous read hidden in active rendering. |

A resource transition is a protocol: stop new requests, invalidate/advance generation, cancel or drain outstanding work, detach render/audio references, reach hardware-safe release, then reclaim. Changing a generation does not itself cancel an external writer. Failure must preserve the old valid state or stop explicitly, not install a partial new one.

Smash64DS uses native content rendering; source-renderer oracles stay host-side. Unsupported materials need a native recipe or a provenance-bearing preflight error before submission, not a partial draw followed by replay. Native CPU transforms and shared GX/BG/OAM kernels remain valid.

Use existing owners and direct bindings when sufficient. `PROJECT_GOAL.md` owns adaptations; `docs/README.md` maps operational policy.
