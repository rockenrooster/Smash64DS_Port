---
name: n64-to-nds-porting
description: Port, review, and optimize Nintendo 64 source or decompilation code for the original Nintendo DS. Use for libultra/GBI/RSP/RDP migration, display-list compilation, persistent vertex caches and mixed-matrix geometry, N64 binary/relocation conversion, native DS render plans and materials, fixed-point boundaries, animation and collision specialization, scene residency, source tick preservation, and replacing N64 OS/audio tasks. Prioritize removing source-machine work and generating compact DS-native data. Pair with nds-coding-practices for DS hardware, SDK APIs, cache/DMA, GX, ARM7, and code generation.
---

# N64 to NDS Porting

## Mission and boundary

Preserve the required N64 behavior while replacing work and representations that
only make sense on the source machine. Write a DS-native implementation first,
not a general N64 compatibility engine that later needs rescuing.

This is a **companion** to `nds-coding-practices`, not a replacement. That skill
owns DS hardware safety and SDK-version-specific implementation. This skill owns
**what to translate, what to compile away, and which N64 semantics must survive**.
Use the companion when touching actual GX, cache/DMA, VRAM, runtime, or ARM7 APIs.
If it is unavailable, inspect the installed SDK and do not invent those contracts.

Keep the pack game-independent. Read the consuming project's source authority,
rate/precision policy, memory limits, timing authority, and supported content.
Do not import another project's frame gate, reserve, emulator, approximation
permission, expansion requirement, or exact-pixel requirement.

## Default implementation order

1. **Keep semantic code; remove source-machine work.** Preserve state machines,
   collision rules, event order, and required arithmetic. Replace address-domain
   tricks, RSP/RDP task submission, source video/audio plumbing, and asset layout.
2. **Compile immutable interpretation away.** Decode source assets and display
   lists on the host; emit native geometry, material recipes, animation channels,
   direct references, and bounded resource requirements. Keep only genuinely
   dynamic choices at runtime. Do not expand every pose or variant by default.
3. **Prepare once, then bind only changing inputs.** Resolve relocation, static
   topology, asset IDs, dispatch choices, and material classes at build/load/setup.
   Feed live transforms, colors, UVs, and state into a small typed runtime path.
4. **Make the remaining hot path cheap.** Contiguous bounded data, appropriate
   fixed formats, useful GX work, fewer state changes, and little copying. Inspect
   actual ARM output and whole-frame costs before claiming a speedup.

An ordinary feature needs the smallest complete implementation, not a new engine,
manifest framework, audit campaign, or whole-repository scan. Read its source
behavior and the DS owner, select one recipe, implement, and test the affected
path. Architecture changes need proportionate capacity and semantic evidence.

## N64-specific traps to resolve before writing code

- **Source addresses are not target pointers.** Distinguish ROM offsets, CPU
  virtual/physical addresses, segmented RSP addresses, serialized offsets, and
  custom relocation words. Decode byte order and fields explicitly. A cast does
  not port any of these domains. See [binary/ABI](references/01-c-abi-addresses.md).
- **GBI is a stateful program.** Identify the actual microcode/header revision,
  task initialization, calls/branches, and inherited state. Do not assume one
  opcode table, vertex-cache size, or reset at every child list.
- **Vertex state has a history.** A source vertex's transform and relevant
  lighting/UV state are captured when it is loaded or modified, not necessarily
  when its triangle is emitted. Preserve live cache slots across matrix changes
  and nested lists. Mixed-matrix triangles are not a one-matrix mesh. See
  [geometry](references/02-rsp-to-ds-geometry.md) and its primary sources.
- **RDP state is not a DS material.** Classify combiner, alpha/depth behavior,
  tile sampling, texture generation, and framebuffer dependencies. Map only
  proven cases; generate dedicated variants for the rest. Never substitute a
  convenient blend mode silently. See [materials](references/03-rdp-materials-textures.md).
- **Similar fixed types are not interchangeable.** Source integer vertices,
  split-halfword matrices, texture units, angles, and float calculations require
  explicit ranges and rounding. Keep gameplay float semantics where necessary;
  convert at proven boundaries instead of replacing every `float` mechanically.
- **Presentation is not simulation.** Preserve the chosen source tick sequence,
  input edges, RNG calls, spawn/delete order, animation events, and pause behavior.
  Rendering less often does not authorize fewer gameplay updates.
- **Observed use is not complete liveness.** Asset stripping requires typed
  reference coverage, runtime/late-spawn roots, interior references, writable
  state, and identity/alias rules. Unknown reachable data stays resident or blocks
  conversion. A trace alone cannot certify deletion.

## Select the relevant chapter

| Current task | Load |
|---|---|
| Choose a port boundary / first implementation | [00 — decisions](references/00-porting-decisions.md) |
| N64 data, C ABI, pointers, relocation words | [01 — binary and ABI](references/01-c-abi-addresses.md) |
| Display lists, vertex seams, matrices, native geometry | [02 — RSP geometry](references/02-rsp-to-ds-geometry.md) |
| Combiner, texture/TLUT, alpha, sampling, 2D conversion | [03 — RDP materials](references/03-rdp-materials-textures.md) |
| Float/fixed, world scaling, animation, joint attachments | [04 — numbers and animation](references/04-numeric-and-animation.md) |
| Scene packs, live sets, RAM/VRAM/storage admission | [05 — residency](references/05-residency-relocation-storage.md) |
| Simulation, input, collision, objects, scripts | [06 — gameplay](references/06-gameplay-timing-and-objects.md) |
| libultra services, RSP audio, saves, task replacement | [07 — services](references/07-os-audio-and-io.md) |
| Hot paths, specialization, generated code, tradeoffs | [08 — performance](references/08-fast-paths-and-codegen.md) |
| Concrete code-porting recipes | [09 — recipes](references/09-recipes.md) |
| A risky conversion or equivalence claim | [10 — validation](references/10-validation.md) |

Read one task chapter first. Maintenance tests are not mandatory runtime context.
Original reusable helpers and host-tool boundaries are in
[examples/README.md](examples/README.md); provenance is in
[references/SOURCES.md](references/SOURCES.md).

## Required behavior when a fast path cannot express the source

Reject an unsupported asset during conversion, use a complete previously
validated path selected **before any draw-side effects**, or implement the
specific missing behavior. A partial draw followed by replay is not a fallback.
Do not hide unhandled opcodes, missing matrices, stale textures, bad relocations,
or capacity failures behind success returns or invisible geometry.

If a reference interpreter is useful, keep it as a host/debug oracle or bounded
bring-up path. It is not automatically a suitable shipping hot path. Conversely,
do not delete the only complete path until the replacement covers the supported
content. Optimization must not make correctness depend on the common case.

## Finish with evidence, not a performance story

Build the affected real target when available. Test negative/extreme numbers,
inherited display-list state, partial vertex loads, timing/event order, worst
legal content combinations, and repeated load/teardown where relevant. Report
host checks, ARM compilation, SDK linking, emulator/device execution, and timing
as separate evidence. No measured speedup means no claimed measured speedup.
