# Unk80045268 ContMotorEvt LP64 Struct Pun (2026-04-11) — FIXED

**Symptoms:** Intermittent crashes on macOS/arm64 with `EXC_BAD_ACCESS: KERN_INVALID_ADDRESS at 0x0000000000000011`. Backtrace:
```
osSendMesg (n64_stubs.c:225)      // reads mq->validCount → faults at 0x11
syControllerParseEvent (controller.c, inside CONT_EVENT_MOTOR case)
syControllerThreadMain
ucontext_entry
_ctx_start
```
The crash reproduced ~1 in ~30 runs of longer sessions, seemingly randomly — "shutdown / end-of-runtime crash" from the user's perspective because it most often fired when the game had been running long enough to reach a rumble-emitting scene. Survived the `016dea1`/`e393d0b` AudioBlob-shutdown fix because it's an unrelated runtime bug that happens to live in the same "end of session" window.

**Root cause:** `src/sys/controller.h`'s `Unk80045268` struct was designed so `&D_80045268[i].unk04` could be **pointer-punned as a `ContMotorEvt *`**. On N64 (32-bit pointers) that works exactly:
```
Unk80045268 (N64, 0x18 bytes):      ContMotorEvt (N64, 0x14 bytes) starting at &unk04:
  0x00: unk00 (busy)                  0x00: type
  0x04: unk04 (type = 5)              0x04: mesg
  0x08: unk08 (mesg = i)              0x08: cbQueue
  0x0C: unk0C (cbQueue*)              0x0C: contID (= unk10)
  0x10: unk10 (port)                  0x10: cmd    (= unk14)
  0x14: unk14 (ev_kind)
```
Every field lines up. On **LP64** (clang/gcc, 64-bit pointers) the ControllerEvent grows: `OSMesg` (void*) and `OSMesgQueue*` are 8 bytes each with 8-byte alignment, so `ControllerEvent` becomes 24 bytes (`type` @0, padding, `mesg` @8, `cbQueue` @16). `Unk80045268` also grows (pointer moves, adds padding). The two layouts no longer match, and `(ContMotorEvt*)&unk04` reads fields at the wrong offsets:
- `evt->type` reads `unk04` → still 5 (CONT_EVENT_MOTOR), so the motor case is taken.
- `evt->cbQueue` reads 8 bytes starting at `(base+4)+16 = base+20`. In the LP64 layout, that covers the upper 4 bytes of `unk0C` and the low 4 bytes of `unk10`. `unk10 = port = 0` in the common case, `unk0C` is a .bss pointer on macOS with upper dword `0x00000001` → `evt->cbQueue` decodes as `0x0000000000000001`.
- `osSendMesg((OSMesgQueue*)0x1, …)` then reads `mq->validCount` at offset `0x10` → faults at `0x11`.

The crash was flaky because whether `evt->cbQueue` decoded to `0x1` vs some random value depended on ASLR and what `unk10` happened to hold at the instant the event was dispatched. When it decoded to 0 the NULL check masked the bug; when it decoded to a valid-looking but wrong address it faulted elsewhere; when it decoded to `0x1` it faulted at `0x11` on the queue read.

**Fix:** Add a dedicated, compiler-laid-out `ContMotorEvt port_motor_evt` field to `Unk80045268` under `#ifdef PORT`, populate it from `unk04`/`unk08`/`unk0C`/`unk10`/`unk14` each time `syControllerUpdateRumbleEvent` dispatches, and pass `&port_motor_evt` to `osSendMesg` instead of `&unk04`. The receiver's existing `(ContMotorEvt*)evt` cast then reads a naturally-aligned struct on both ABIs. N64 builds are untouched — the `#ifdef PORT` gate keeps the original pointer pun.

**Files:**
- `src/sys/controller.h` — add `ContMotorEvt port_motor_evt` field inside `Unk80045268` under `#ifdef PORT`.
- `src/sys/controller.c` — rewrite the `osSendMesg` call in `syControllerUpdateRumbleEvent` under `#ifdef PORT` to populate and dispatch `port_motor_evt`.
- `port/port.cpp` — drive-by: add `SSB64_MAX_FRAMES=N` env-var hook that drives `Window::Close()` after N frames, for automated clean-shutdown testing without needing a human to click the close button.

**Class-of-bug lesson:** Any decomp struct whose fields include `void*`, `OSMesg`, or other pointer-width types can silently drift on LP64 vs the N64 ABI that the layout was originally designed around. Field-access-by-name is still safe (the compiler picks the right offset), but **type-punning a substruct via pointer cast is not** — the substructs have to agree on offsets on both ABIs, and with 8-byte pointers they usually don't. When you see a decomp function pass `&some_struct.sub_field` to a callee that casts it to a different type, audit whether every field offset still matches. This is a sibling class to the "OSMesg Union/Pointer Type Split" bug — same root (C ABI differences), different manifestation.
