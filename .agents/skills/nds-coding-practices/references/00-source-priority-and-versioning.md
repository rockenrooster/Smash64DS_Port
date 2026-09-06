# Source Priority and API Versioning

## The central rule

Nintendo DS tutorials span roughly two decades. Many contain sound hardware
ideas but obsolete library setup, ARM7 templates, FIFO APIs, Makefiles, or
initialization assumptions. Never combine snippets from different toolchain
generations without checking the installed headers.

## Establish the project generation

Inspect:

- `DEVKITARM`, package manifests, CI images, or devkitPro pacman packages;
- included headers and linked libraries;
- whether libnds headers include Calico facilities;
- ARM7 build ownership: stock/default core, Maxmod template, or custom ARM7;
- whether the project uses libnds, BlocksDS, NitroSDK-derived code, or direct
  registers;
- Makefile templates and linker scripts.

Current devkitPro libnds uses Calico as its low-level foundation. Official
current examples commonly use `pmMainLoop()` as the application loop guard.
Legacy projects often use older FIFO/service initialization and may need a
coherent migration rather than isolated API substitutions.

## Source-of-truth order

1. **Installed/project headers** — exact ABI and signatures.
2. **Project wrappers** — ownership and compatibility contract.
3. **Upstream source at the project's version** — implementation semantics.
4. **Official examples at a compatible version** — intended composition.
5. **GBATEK** — hardware register and timing behavior.
6. **Community code** — useful patterns, never automatic API authority.

Hardware documentation governs hardware behavior; project policy does not
override access-width, addressability, or cache rules. Installed source governs
what a library actually does, even when an API comment is misleading.

## Runtime differences that change implementation choices

| Area | Calico-based devkitPro libnds 2.x | Legacy libnds / other SDKs |
|---|---|---|
| Timers | Calico owns timers 2 and 3. Use the tick API; reserve 0/1 only when needed. | Inspect actual runtime reservations; do not assume the modern assignment. |
| Scheduling | Priority-preemptive threads without timeslicing. Workers must block appropriately. | Use that SDK's supported scheduler or a bounded main-loop state machine. |
| Callbacks | Tick-task and PXI callbacks execute in IRQ context. | Inspect each callback's context, not just its name. |
| IPC/services | Calico/PXI services; compatible ARM7 runtime required. | Preserve the coherent legacy protocol unless migration is requested. |
| Filesystems | libdvm replaces libfat/libfilesystem, with compatibility interfaces. | Existing libfat/other filesystem stack may remain appropriate. |
| Cached aliases | Old cached/uncached main-RAM aliases were removed; use supported shared allocations and cache APIs. | Check the actual MPU/linker mapping before using an alias. |

This is a generation-selection aid, not a mandate to upgrade a working project.
Do not transplant Calico-only calls into BlocksDS or a frozen older SDK.

## Concrete current-runtime facts

Use `17-libnds2-calico-facts.md` for named replacement APIs, default memory/stack
layout, format scales, and API side effects. Keep those facts version-scoped:
Calico's memory split, PXI, and scheduler are not universal DS hardware rules.

## Rules for examples in this pack

- Examples use current libnds-style names where practical.
- They are patterns, not a substitute for checking the installed API.
- Asset symbols and project callbacks are intentionally passed as parameters.
- Direct-register examples describe hardware behavior and can coexist with an
  API only when the API does not maintain conflicting shadow state.

## Modernization rule

When updating old code, migrate one ownership seam completely:

- initialization;
- callback or message registration;
- producer and consumer;
- shutdown/reset;
- associated ARM7 side if any.

Do not leave both old and new service paths active. Do not copy an old custom
ARM7 binary into a modern runtime without confirming startup, PXI/FIFO, sound,
touch, power-management, and storage expectations.

## Compatibility comments

When code intentionally supports multiple libnds generations, isolate the
compatibility layer in one header/source pair. Avoid version checks scattered
through rendering or gameplay code. Prefer feature detection from headers or
build configuration over guessed version numbers.

## Pinned research baseline

This pack was prepared on 2026-08-03 against:

- devkitPro/libnds at commit `84e6082ce27c87ed218fb369a9944644aa2243a6`;
- devkitPro/nds-examples at commit `f1ba715a451c6407f8b0f805999d0153062ff552`;
- devkitPro/calico at commit `81b75e314d57ed1784545e28554e567f26f572f1`.

These pins document the research baseline only. A project-pinned dependency
always wins.
