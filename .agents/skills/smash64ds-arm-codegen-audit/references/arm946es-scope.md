# ARM946E-S scope guard

Use actual devkitARM output and Nintendo DS measurements. Generic low-level
advice is inapplicable when it assumes:

- AArch64 registers/instructions;
- speculative out-of-order execution or modern branch predictors;
- desktop/Linux hardware performance counters;
- coherent caches across DMA without explicit maintenance;
- SIMD/NEON;
- virtual-memory or OS scheduler behavior absent from the DS runtime.

Relevant questions include ARM versus Thumb state, ARMv5TE instruction
selection, software EABI helpers, stack/register pressure, instruction/data
cache locality, ITCM/DTCM placement, direct MMIO emission, DMA/cache maintenance,
and exact fixed-point/range behavior.
