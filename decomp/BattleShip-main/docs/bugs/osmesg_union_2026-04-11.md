# OSMesg Union/Pointer Type Split (2026-04-11) — FIXED

**Symptoms:** On macOS/arm64, the scheduler crashed in `sySchedulerExecuteTasksAll + 0x200` with SIGBUS: `blr x8` where `x8 = curr->fnCheck` pointed to an invalid code address. `curr` itself was garbage — not a heap pointer but a stack-range address with low bits looking like `0x16f...01`. This surfaced immediately after "Boot sequence yielded — entering frame loop" as the first task dispatch. The same build worked on Windows because MSVC-x64 happened to leave the relevant stack slots zero.

**Root cause:** `OSMesg` has **two conflicting definitions** visible in the same program:
- `include/PR/os.h` (decomp): `typedef void* OSMesg;`
- `libultraship/include/libultraship/libultra/message.h`: `typedef union { u8 data8; u16 data16; u32 data32; void* ptr; } OSMesg;`

Both are 8 bytes with 8-byte alignment, so the ABI matches across translation units. But the semantics of a **C-style cast from integer to OSMesg** differ by a mile:
- In C TUs that see `void*`: `(OSMesg)INTR_VRETRACE` → well-defined int-to-pointer conversion, always zero-extends.
- In C++ TUs that see the union (e.g. `port/gameloop.cpp` via the LUS header chain): clang treats `(OSMesg)1` as a brace-init-like conversion that writes the low union member (`data8`/`data32`) and leaves the remaining bytes **uninitialised** — i.e., whatever the register/stack happened to hold. On MSVC/x64 those bytes were zero by luck; on macOS/arm64 they were a fresh stack pointer.

So `PortPushFrame()` posted `{data32 = 1, upper_bytes = <stack garbage>}` to `gSYSchedulerTaskMesgQueue`. The scheduler read it back, saw a value that was neither `INTR_VRETRACE (1)` nor any other interrupt code, and fell through to `default:` where it casts to `SYTaskInfo*`. The "task pointer" pointed into the stack, `curr->type`/`priority`/`fnCheck` were all garbage, and the first `curr->fnCheck(curr)` call jumped to an invalid address.

**Fix:** `port/gameloop.cpp` grew a `port_make_os_mesg_int(uint32_t code)` helper that `OSMesg{}`-zero-initialises a fresh union and then sets `data32`. Every integer-to-OSMesg send in the port (C++) layer should go through that helper so all 8 bytes are well-defined on every platform. C callers of `osSendMesg` keep using `(OSMesg)(intptr_t)code` because they see the `void*` typedef.

**Why it also matters on Windows:** It doesn't crash today but the same UB is present — any future code change that shifts the stack layout could unmask it. The helper makes it deterministic.

**Files:**
- `port/gameloop.cpp` — `port_make_os_mesg_int()` + call site in `PortPushFrame()`.

**Diagnostic that cracked it:** `port_log` inside the scheduler's task loop printed `task=0x16f...2d01 type=24103981 fnCheck=0x10000000...` — obvious "stack pointer OR'd with an int". Then logging `osSendMesg`/`osRecvMesg` on the task queue showed the queue was round-tripping that same value, meaning the sender was writing it. Adding a dedicated send debug print at `PortPushFrame` confirmed the C-style cast was producing garbage upper bytes.
