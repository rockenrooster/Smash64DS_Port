---
name: smash64ds-arm-codegen-audit
description: Audit one measured Smash64DS ARM9 or ARM7 hot symbol using the actual devkitARM object, ELF, map, and disassembly. Use after profiling identifies a costly function, loop, dispatcher, transform, decoder, mixer, copy, or helper call. Finds ARMv5TE instruction/helper, stack, branch, ARM/Thumb, inlining, and TCM issues, then tests one exact code-generation change. Do not use for speculative compiler sweeps.
---

# Objective

Explain a proven hot path from generated ARM machine code and remove one measured
source of cost without changing source-visible behavior.

# Preconditions

1. A `$smash64ds-perf-experiment` evidence packet names the exclusive phase,
   symbol/call chain, dynamic invocation count or bound, and useful saving
   threshold.
2. Record target/build/profile, effective compile command, object/ELF hashes,
   symbol address/size, section, and current P50/P95.
3. Confirm this is ARM946E-S / ARMv5TE-era code. Do not import AArch64, modern
   out-of-order, desktop cache-counter, or Linux-perf assumptions.

# Inspect the actual artifact

Use the live devkitARM paths and adapt as needed:

```text
arm-none-eabi-nm -S --size-sort <elf>
arm-none-eabi-objdump -drS <object-or-elf>
arm-none-eabi-readelf -S -s <elf>
```

Audit the hot region and its callers for:

- `__aeabi_*div*`, `__aeabi_*float*`, double, 64-bit, or conversion helpers;
- `memcpy`, `memmove`, `memset`, `bzero`, structure copies, and large clears;
- indirect calls, callback dispatch, switch tables, validation, or diagnostics;
- spills, reloads, oversized frames, alias-driven reloads, and address rebuilds;
- loop-invariant matrix/material/texture/state work;
- ARM/Thumb interworking and hot/cold section placement;
- code growth from inlining/O3 that can evict more valuable ITCM code;
- profile-0 volatile telemetry/proof writes;
- generic helpers retained by a supposedly native owner;
- cache maintenance or DMA preparation whose cost is charged to the hot bucket.

Count dynamic frequency. A long sequence called once is not a useful target by
itself.

# Choose one candidate

Prefer, in order:

1. eliminate the call or operation;
2. move invariant work out of the loop/frame;
3. replace generic dispatch with a proven narrow direct path;
4. remove accidental type promotion or unnecessary 64-bit work while preserving
   exact semantics;
5. reduce copying/clearing with an exact lifetime or ownership change;
6. test per-function/object ARM/Thumb, O2/O3, inline/noinline, hot/cold split, or
   TCM placement only when disassembly and call frequency justify it.

Do not globally enable fast-math or single-precision-constant behavior. Do not
replace behavior-sacred gameplay arithmetic with fixed-point hardware
approximations. Do not move code/data into TCM merely because space appears free.

# Prove the change

The after-artifact must show the named helper, call, branch, load/store pattern,
or copy actually disappeared or shrank. Record symbol size, section movement,
call list, stack delta, and ITCM/code-size effects.

Run the same synchronized performance ladder and exact source/renderer/audio
checks as the parent experiment. KEEP only when the measured bucket and relevant
phase P95 improve beyond noise without moving equal or greater cost to another
bucket.

# Output

Return:

```text
HOT SYMBOL / CALLERS:
DYNAMIC FREQUENCY:
BEFORE ADDRESS / SIZE / SECTION / STACK:
BEFORE EXPENSIVE SEQUENCE:
CAUSAL HYPOTHESIS:
ONE CHANGE:
AFTER ADDRESS / SIZE / SECTION / STACK:
AFTER SEQUENCE:
MAP / TCM / ROM DELTA:
P50/P95 RESULT:
EXACTNESS GATES:
KEEP / REWORK / REVERT:
```
