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

- devkitPro/libnds master near commit `84e6082ce27c87ed218fb369a9944644aa2243a6`;
- devkitPro/nds-examples master near commit `f1ba715a451c6407f8b0f805999d0153062ff552`;
- devkitPro/calico master near commit `81b75e314d57ed1784545e28554e567f26f572f1`.

These pins document the research baseline only. A project-pinned dependency
always wins.
