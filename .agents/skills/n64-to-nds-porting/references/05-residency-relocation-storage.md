# 05 — Native live sets and bounded residency

Load what target consumers need, not entire source files/overlays. Source units may combine previews, setup, relocation, gameplay and late effects. Prefer complete native scene/match packs before inventing gameplay paging; stream only with a proven demand/deadline/buffer contract.

Define runtime roots from actual code and typed schemas, including reachable actions, late spawns, callbacks/scripts, alternate forms, material/animation tracks, collision, audio and asynchronous references. A successful trace does not prove absent content is dead. Extract typed reads, writes, identity and pointer ranges. Retain complete consumer spans: interior references, pointer arithmetic, adjacent handle pairs, split arrays and trailing NULL sentinels. C declaration boundaries are not necessarily runtime read boundaries; independently walk the compact result with the source consumer's termination rules.

Compute a cross-file transitive closure with cycles. Share immutable objects once, but retain each instance's mutable state. Unknown reachable semantics require conservative externally justified retention or a stop until understood—not scanning for pointer-looking words and discarding the rest. The [live-set tool](../tools/live_set.py) computes a whole-object closure from supplied metadata; it neither proves the roots/edges nor performs relocation or byte-level shrinking.

Emit reversible source `(object,offset,kind)`→target mappings and validate every fixup's null policy, span, alignment and lifetime. Install fields only after dependencies resolve. Source blobs remain live while any unmigrated consumer uses them, even if the renderer no longer does. Byte-level compaction additionally needs complete typed subobject extents and arithmetic/identity rules.

## Admission and transitions

`peak = permanent runtime + union(immutable closures) + sum(instance state) + active caches + maximum overlapping transition/decode/transfer scratch + stacks/reserve`

Check worst legal selections or a mechanically justified conservative bound. Give every byte one accounting owner; do not charge raw source again when an existing native replacement is already in the baseline. Derive credits from complete reader coverage, never a target number. Main RAM, texture/palette intervals, VRAM roles, handle/table slots, geometry capacity, bandwidth and CPU are separate constraints. Generate requirements from actual native plans. Old/new scene overlap or active DMA/GX/audio consumers can determine the peak rather than steady-state allocation.

Selection screens usually need compact icons/names/portraits/preview poses, not every gameplay closure. Use a dedicated native preview bank and indexed sequential-friendly storage. Version selection requests so stale completion cannot replace the current preview. Frequent tiny “lazy” reads can still be slower than bounded preloading.

Decode independently loadable chunks into their final owned lifetime when possible. Release redundant source/converted forms only after migrating all consumers. No hidden first-use synchronous decode/read in active rendering; require resident banks or an explicitly admitted streaming scheme. Another CPU does not remove storage latency.

Fail admission locally with the limiting resource and selection. Never globally reset live caches, overwrite arenas, alias somebody else's handle, or hide required content. Teardown must drain/cancel and release every external consumer before reuse; generation changes alone do not stop a writer. DS allocation/cache/VRAM/stream APIs belong to the companion.
