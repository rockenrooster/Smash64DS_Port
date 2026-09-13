# 08 — High performance without missing work

Optimize the boundary first: remove repeated GBI production/interpretation, animation/script decode, source-pointer resolution, static material conversion, duplicate joint evaluation and texture conversion. Then reduce copying/state traffic, select native operations and inspect arithmetic/placement. A native specialization is a design candidate, not a measured speedup.

Bind stable facts at build/load/action transitions: rigid geometry plus transform; mixed-history geometry plus small cross-joint bindings; UV/color animation plus live parameters; procedural geometry plus bounded producer. Keep variant count proportional to actual differences. Code/metadata explosion can defeat instruction-cache savings.

Invalidate on every true input: source/material/palette generation, blend/pose, root/attachment, camera, UVs and script patches. A prebuilt buffer is useful only after accounting for patch scans, copies, flushes, DMA setup/waits and command traffic. Prefer small typed bindings or direct stores over a universal byte-offset patch interpreter when sufficient.

Split hot state from cold source metadata according to access patterns. Compact IDs save space only if repeated lookups do not cost more; hoist validated resolution. Keep live CPU fields aligned. SoA and AoS are workload choices, not universal rules.

Inspect the target function and callers for soft-float/conversions, variable divides, spills, indirect calls, lookup repetition and data movement. Wide multiply may be efficient and required; setup-time helpers may be harmless. Do not globally enable fast-math, narrow types, or change alias/overflow rules. ARM/Thumb, TCM and hot/cold placement require final map and actual compiler evidence.

Separate source update, pose/collision, native prep, submission, stalls, storage/audio and idle without double-counting. `docs/VERIFYING.md` owns the matched-workload measurement procedure.

For each precompute compare **CPU saved, ROM added, peak RAM change, load/stream bandwidth**. Sampled poses may enlarge working sets; material variants consume palettes; static lists may still be recopied; collision cells may duplicate geometry and require order restoration. Reduced cadence, quality or content needs project approval, not a speed label. Freestanding ARM output is not target timing. [Validation](10-validation.md).
