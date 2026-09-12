# Shared plan 01 — Resolve donor sources into the existing DS pipeline

**Status:** implementation plan, not an implemented importer. Parent: [master](../New_Characters.md).

## Decision: two input adapters, one output contract

The current production manifest reads BattleShip FTData/FTMotionDesc, generated C-symbol bindings and O2R headers/externs. A Remix directory does not satisfy that input contract. Add a source-adapter seam; retain the downstream native model, animation, audio, residency and verification machinery. Do not patch read-only BattleShip source merely to make the P2 parser see new names. [Evidence: D3 in [sources](../SOURCES.md).]

Prefer the donor assembler and EXTRA appender to resolve inheritance and symbols. Do not first write a general Bass interpreter, an automatic MIPS-to-C compiler or a DS runtime mod loader. Hand-port the required native callback semantics, with reusable generation for data and repetitive glue.

## 1. Freeze the exact inputs

The reviewed pins are in [source-lock.json](../source-lock.json): main Remix and EXTRA's nested Remix both use `5e04fe7fcd023cd43c71f25f89bb6e810d254d55`. EXTRA uses `96621afea26a83305abaf81add07dcf5a9c5fe3e`. These are Git submodules, not missing ordinary folders.

Initialize the pinned tree with `git submodule update --init --recursive`, then verify the three actual HEADs and clean state. Never use `--remote` for a reproducible import. Record supported source-ROM revision/byte order, input hash, patch hash, toolchain versions, profile options and outputs. The current review lock is provenance only; build-tool enforcement is a proposed deliverable.

Read-only references stay read-only. The donor build/appender writes files, so use a disposable staging copy/worktree outside `decomp/`. Resolve dependencies from a pinned environment instead of blindly retaining build.bat's automatic pipenv upgrade. Preserve the original scripts as reference; document any staging-only adaptation and its hash. Input ROMs and extracted protected payloads are not part of this plan package or a source commit. Preserve credits and review redistribution permissions separately.

Filename case must be handled deliberately: actual files include `src/moveset.asm` and `src/Bowser/bowser.asm`; the donor main include spells Bowser differently. Audit portability in staging, without silently editing the pinned source. [R2; Bowser card; E2/E3.]

## 2. Export the linked result, not merely loose .bin files

Main Remix's patched asset ROM and its final assembled action/script image have different roles. Character macros read base-ROM tables; action edits retain or override fields; assembly inserts binaries, appends commands and emits pointers. EXTRA adds configuration, request-list fixups, merges, append-files and sound substitutions before final patch/assembly. [R1/R2/E3.]

Preferred approach: augment the disposable donor build with an export table of character descriptors, effective actions, symbols and source addresses. Parse those resolved values together with the appropriate resource-file table. Verify the assembler's available export mechanism during the first experiment; this plan does not assume an undocumented command-line symbol-dump flag. A generated review-only table appended to an output, or an equivalent assembler-supported export, is sufficient. Keep the reference execution's gameplay payload unchanged by the export instrumentation.

Give each object an identity `(donor revision, file/symbol, offset, type)`. Map N64 addresses to known sections/objects before replacing them with DS IDs/handles. Verify endianness per typed field, alignment, length, bounds, null/sentinel meanings and ownership. Never blanket-swap mixed-format data or leave a donor function pointer in DS output.

## 3. Four dependency views

| View | Required result | Source example |
|---|---|---|
| Setup/actions | Fully resolved parent defaults, overrides, disabled entries and callback slots | Roy uses Captain as setup parent. |
| Resources | Model, skeleton, motion, texture, material, sound and UI donors | Roy references Marth animation assets. |
| Shared behavior | Required patched engine functions and indirect callback targets | Ganondorf reaches Captain shared code; Crash reaches a size/effect render helper. |
| Copy abilities | Exact copy policy, callbacks and reachable resources | Snake's grenade files are separated for Kirby. |

Root every supported gameplay, item, entry, result, preview and copied-ability action. Follow resets and late constructors, not just setup. Where arbitrary native code or computed indices prevent automatic classification, require a reviewed annotation with target domain, lifetime and source witness. Unknowns block that candidate's admission; they do not block unrelated source census work. Trace coverage can confirm a path, never prove an unvisited path dead.

Classify global patches as already equivalent, required narrow extension, deliberately excluded optional feature, or unresolved/conflicting. Preserve vanilla common rules unless the selected donor mechanic requires an explicit extension. Do not import all Remix toggles, Super Sonic or EXTRA variants because a shared function mentions them.

## 4. Typed semantic metadata is a real deliverable

The P2 pack-estimator design relies on typed BattleShip objects and relocation semantics. Remix binary assets do not automatically come with that typed C index. Emit equivalent object/consumer metadata from validated schemas and resolved tables. Container-format compatibility is not evidence that semantic liveness is already known.

The adapter must identify default hurtboxes, modelpart domains, hierarchy, material alternatives, texture/palette binding, callbacks, reconstruction inputs and gameplay joint consumers. Preserve reset defaults and effective low-detail fallbacks. An unclassified atom is not removable padding. Reuse the existing pack disposition rules where their input contracts truly match, and fail closed otherwise.

## 5. Lower events and native callbacks

Build a typed event/control-flow graph from valid roots and instruction lengths. Preserve assembly-built streams, fall-through, loops, subroutine returns, concurrent streams, throw-data targets and state exits. Reject invalid targets, unknown opcodes, impossible stack depth and zero-time cycles. Decode custom command families using their actual format, not a byte-pattern or vanilla-opcode scan.

Keep animation phase/speed, event execution time and presented-frame time distinct. Ganondorf's idle rate command, Falco's fall-through, Wario's concurrent trail and Dedede's one-source-frame input buffer are initial test fixtures. Adapt simulation timing only under the project contract, with compensated behavioral tests; do not scale every constant by render FPS.

Port callback semantics to native C at existing DS seams. Audit delay slots, branch-likely behavior, signed loads, arithmetic shifts, unions/aliasing, float bit patterns, callback argument conventions and RNG calls. Lanky's `0x43960000` comment mismatch is a numeric-decoder fixture, not a license to assume comments are always wrong. Use the fastest equivalent implementation, not instruction-for-instruction MIPS emulation.

Prefer the existing competitive event runtime or compact specialized output. Fully unrolling every script into C can inflate `.text`; compare output size and frame cost. No universal new VM is required by this plan.

## Deliverables and exit

Produce a reproducible source export, typed dependency report, action/event/callback inventory, unsupported-feature list, source-to-DS identity maps and native production input. Proposed schemas/commands must be implemented and named in the existing tooling, not asserted to exist because this document names them.

Falco exits source admission when its reachable dependencies are classified, round-trip/reference comparisons and negative controls pass, and generated input is reproducible. Bowser topology and Meta Knight EXTRA canaries then check that the seam generalizes. This is still not a DS runtime or performance pass.
