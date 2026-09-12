# 12 — DS cost model

The timing platform, budgets and A/B procedure belong to `PROJECT_GOAL.md` and `docs/VERIFYING.md`. These are implementation tradeoffs, not additional performance gates.

Separate update, pose/collision, native preparation, submission, FIFO/GX wait, upload, storage, audio and idle. Nested/parallel intervals cannot simply be added; missing output is not less work doing the same job. For CSS loading, separate read, decode, fixup and publication cost: an 8 KiB slice is not a time bound.

Remove repeated source-command decode, static material rebuilding, conversions, duplicate poses, allocations, copies/flushes, tiny scattered reads and redundant state changes before optimizing instructions.

Native lists win only when saved interpretation exceeds patching, copying, cache maintenance, DMA setup/waits and command traffic. Small typed bindings can beat a universal patch loop. Share immutable data, not mutable instance state; excessive variants can defeat instruction-cache locality.

Compare precomputation by CPU saved, ROM added, peak RAM and load/stream bandwidth. Linked `const` can consume resident RAM. Sampled animation can enlarge buffers or mishandle fractional playback; palettes can exhaust slots before byte capacity.

ARM/Thumb, TCM, ARM7 offload, layouts, LUTs and hardware math depend on actual access patterns. Desktop time or fewer instructions/polygons alone cannot establish a DS speedup. Preserve the project's permitted behavior and fidelity boundaries.
