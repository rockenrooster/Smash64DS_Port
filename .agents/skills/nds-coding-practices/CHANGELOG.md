# Final Revision — 2026-09-06

Supersedes `nds-coding-practices-2026-09-06-revised.zip`. The original pack and
prior review artifacts are unchanged. This remains project-independent.

## Entry point and structure

- Rewrote the entry point around an efficient first implementation, separating
  hardware requirements, sensible defaults, and measured optimization experiments.
- Kept the fast-implementation recipes in chapter 16 and added chapter 17 for
  pinned libnds 2.x / Calico facts. Reduced repeated process instructions,
  especially in the BG and IRQ/frame-loop chapters.
- Clarified that existing SDKs are not migrated automatically and project-specific
  targets/testing policy belong in the optional project context, not this skill.

## Incorporated technical corrections

- Retained the prior zero/count/alignment DMA guards, cache-line ownership,
  TCM addressability, `const`/RAM clarification, removed uncached-mirror assumptions,
  correct GX overflow register, END semantics, and optional capture guidance.
- Added the stock Calico ARM9 section limit, actual DTCM main-stack behavior,
  conditional stack relocation, and explicit main-RAM DMA-buffer default.
- Added native PXI names, 26-bit/extended-message units, IRQ-to-worker APIs,
  priority/no-timeslicing semantics, `threadYield` limitation, worker TLS, and
  runtime timer reservations.
- Identified the PXI mailbox adapter's ignored full-queue failure; documented
  loss/backpressure implications and used one outstanding request in the example.
- Added precise native fixed formats, normal scale 512, math-helper rounding and
  divider distinctions, API transfer/remap effects, matrix limits, build flags,
  and concrete debugger/code-inspection commands.

## Examples and maintenance checks

- Replaced the default sequence-lock example with a tiny protected same-CPU copy.
- Added a Calico IRQ worker with coalescing, TLS, and stop/join; added matching
  PXI ARM9/ARM7 components without replacing the ARM7 core or its services.
- Made small demo video writes explicitly volatile halfword operations and kept
  BG scrolling bounded. Portable math remains a reference, not a mandatory
  software-division hot path.
- Added nonconstant exported helper probes, a real-SDK compile/inspection script,
  an illustrative Clang ARM probe, archive/link checks, and additional proposed
  agent-evaluation fixtures. Maintainer material stays outside ordinary routing.

## Validation boundary

The host tests and illustrative ARM codegen were executed; see
[`tests/REVIEW_RESULTS.md`](tests/REVIEW_RESULTS.md). A real installed devkitARM
SDK was unavailable, and the target-check script returned SKIP with exit 2.
No target link, emulator/device execution, DMA/cache/scheduler/PXI hardware test,
visual approval, speedup, or model-effectiveness comparison is claimed.
