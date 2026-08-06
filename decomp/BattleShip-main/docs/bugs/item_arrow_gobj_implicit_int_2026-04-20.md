# Dropped-item despawn segfault: LP64 pointer truncation via implicit int — Resolved (2026-04-20)

## Symptom

In a battle with items, every dropped stage item that timed out (pickup_wait → 0) caused a hard segfault inside `itMainDestroyItem`. Fault address was always sub-4GiB and always offset `0x1B` into the bogus base, which is `offsetof(GObj, obj_kind)` on LP64 — so the crash was the very first field load of `gcEjectGObj(ip->arrow_gobj)`.

Diagnostic log captured it clearly:

```
SSB64: itMainDestroyItem ENTER gobj=0x100cc7960 id=1013 kind=1 ip=0x100c4c888 ip->kind=0 arrow=0xcc9608
SSB64: itMainDestroyItem TRUNC arrow_gobj=0xcc9608 raw_bytes=[08 96 cc 00 00 00 00 00] — skipping eject (leak)
```

Low 32 bits were a real pointer (`0xcc9608`), high 32 bits were a clean `00 00 00 00` — not random garbage. That pattern rules out a bitfield RMW or memory stomp; it pointed squarely at a 64→32 truncation at the moment of assignment.

## Root cause

Every item source file (`src/it/itcommon/*.c`) writes `ip->arrow_gobj = ifCommonItemArrowMakeInterface(ip);` but **none of them included `if/ifcommon.h`**, and neither did the umbrella `src/it/item.h` that they all include.

With no prototype in scope, C falls back to the **implicit-function-declaration** rule: the function is assumed to return `int`. On LP64, clang compiles the call as:

1. Call the function (it actually returns `GObj*` — 64-bit — in `x0`)
2. Treat the return as 32-bit `int` (`w0`)
3. Sign-extend to 64 bits and store into the pointer slot

Because user-space pointers have bit 31 = 0, sign-extension is a zero-extension — which gives you the `[low32 | 0x00000000]` pattern we saw. Every write of `arrow_gobj` lost its upper 32 bits.

The warning that would have caught this (`-Wimplicit-function-declaration`) is suppressed project-wide in `CMakeLists.txt:200`:

```cmake
-Wno-implicit-function-declaration
-Wno-implicit-int
```

Those suppressions exist for decomp compatibility — IDO-style decomp code contains many implicit declarations — but they also silence the diagnostic that would have caught this 64-bit-safety bug on day one.

## Fix

Add `#include <if/ifcommon.h>` to `src/it/item.h` (the umbrella header every item file includes). That brings the correct prototype into scope for every caller at once, so the `GObj*` return isn't truncated.

```c
/* src/it/item.h */
#include "ittypes.h"
#include "itfunctions.h"
#include <if/ifcommon.h>   // ← adds correct prototype for
                           //   ifCommonItemArrowMakeInterface etc.
```

## Class of bug / where to look next

This class of bug is latent anywhere a function returning a **wider-than-int** type (pointer, `s64`, `u64`, `f64`) is called without a prototype in scope. The suppressed warnings make it invisible at compile time.

To audit systematically:
1. Temporarily flip `-Wno-implicit-function-declaration` → `-Werror=implicit-function-declaration` in CMakeLists and do a full rebuild. Every hit is a candidate for the same LP64 trap (though only pointer/wide-type returns actually corrupt data).
2. For each hit, check the return type in the canonical header. If it's `void`/`int`/`s32`, it's safe. If it's `GObj*`/`ITStruct*`/any wide type, it's a silent-corruption bug.

Related landmines noted in memory:
- `docs/bugs/controller_motorevt_lp64_2026-04-11.md` — LP64 struct-pun truncation
- `docs/bugs/osmesg_union_2026-04-11.md` — uninitialized upper bytes via union alias

## Diagnostic scaffolding left in place (can be removed in a cleanup commit)

- `port/port_watchdog.cpp` — SIGSEGV/SIGBUS/SIGILL/SIGFPE/SIGABRT handler dumps `fault_addr` and a backtrace to `ssb64.log`. Generally useful; keep.
- `src/sys/objman.c` `gcEjectGObj` — double-eject sentinel via `obj_kind = 0xFE`. Cheap, catches a different class of GObj-lifecycle bug. Keep.
- `src/sys/objhelper.c` `gcEndProcessAll` — "SUSPECT proc=..." warning for low/unaligned pointers. Keep.
- `src/it/itmain.c` `itMainDestroyItem` — entry log + `TRUNC` defensive skip. Can be removed now that the root cause is fixed (should never fire again).
