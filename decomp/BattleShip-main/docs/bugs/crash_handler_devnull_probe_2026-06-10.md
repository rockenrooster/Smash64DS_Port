# Crash handler double-fault: /dev/null readability probe is a no-op

**Date:** 2026-06-10
**Status:** FIXED
**Corrects:** the "crash-handler FP walk now probes readability via
write-to-/dev/null" item from `stability_audit_sweep_2026-06-09.md` — that
mitigation never worked.

## Symptom

A genuine game crash on the game coroutine (here: the known scene-52
direct-boot junk-fkind SIGSEGV in `sc1PIntroSetupFighterFiles`) produced:

- a macOS crash report (`macos_crash_2026-06-10.txt`, alongside this
  file) blaming **the crash handler
  itself**: `EXC_BAD_ACCESS (SIGBUS) KERN_PROTECTION_FAILURE` in
  `WalkFPChain` (`port_watchdog.cpp:229`), with the VM region map showing
  the fault address `0x11f6b4000` inside a 16K `---/rwx` mapping;
- an `ssb64.log` with the **original fault's backtrace empty** — the
  `---- main-thread backtrace ----` header immediately followed by a second
  `!!!! CRASH SIGBUS` line;
- process exit attributed to SIGBUS instead of the real SIGSEGV, and the
  `Fast::DumpDLDiag` stale-DL dump never running.

Fingerprint: **two** `!!!! CRASH` lines in `ssb64.log`, the second always
`SIGBUS` at a page-aligned address; macOS report top frames =
`WalkFPChain → DumpBacktraceFromContext → CrashSignalHandler → _sigtramp`.

## Root cause

`ProbeReadable()` guarded every frame-pointer dereference by `write()`-ing
the probed bytes to **/dev/null**, expecting `EFAULT` for unreadable
memory. But /dev/null's write handler (XNU `nullwrite`; Linux `write_null`
identically) discards the request **without ever copying from the source
buffer** — the syscall succeeds for any address. The probe always answered
"readable."

The FP chain on a game-coroutine stack walks upward to the stack's base
frame; coroutine stacks are mmapped back-to-back with a `PROT_NONE` guard
page at each mapping's low end (`coroutine_posix.cpp:124`, added in the
2026-06-09 audit). The base frame's saved-FP slot points at/past the top
of the stack — which is the **adjacent coroutine's guard page**. The
alignment/direction/stride sanity checks all pass for that address, the
no-op probe waves it through, `fp_ptr[0]` reads a `---` page → nested
SIGBUS inside the signal handler. The nested handler invocation dumps its
own (useless) context and re-raises, so the OS report records the handler's
SIGBUS and the original backtrace is lost.

## Fix

`port/port_watchdog.cpp`:

- `ProbeReadable` now writes the probed bytes to a **self-pipe**. Pipe
  writes must `copyin` the source into the kernel pipe buffer, so they
  genuinely fail with `EFAULT` on unreadable memory. After a successful
  probe the bytes are immediately `read()` back so the pipe never fills
  (with an `EAGAIN` drain-and-retry fallback). `write`/`read` are
  async-signal-safe.
- The pipe fds are created in `port_watchdog_init()` (with `O_NONBLOCK` +
  `FD_CLOEXEC`) because `pipe(2)` is not async-signal-safe; if creation
  failed, the probe reports unreadable and `WalkFPChain` falls back to
  `backtrace()`.

## Verification (2026-06-10)

Re-triggered the same crash (`SSB64_START_SCENE=52 SSB64_SPGAME_STAGE=0`
without `SSB64_SPGAME_FKIND`):

- exactly **one** `!!!! CRASH SIGSEGV fault_addr=0x223` line;
- full 13-frame backtrace, frame 0 = the true faulting function
  (`ftManagerSetupFilesAllKind + 40` — previously visible only as raw `pc`
  in the register dump), walking the whole coroutine stack down to
  `ucontext_entry` / `_ctx_start`;
- process exits 139 (SIGSEGV — the original signal, properly re-raised);
- clean runs (900-frame co-op direct boot) unaffected.

Known cosmetic leftover: frames 1/2 can duplicate (`lr` seed == first
walked return address when the fault pc is in a function whose frame
record is already pushed). Left as-is — deduping would silently drop a
genuine frame in the 1-deep-recursion case, and a visible duplicate is
self-explanatory.

## Audit hook

Any "is this memory readable?" probe built on `write()` must target a
**pipe or socket**, never /dev/null (or any device whose driver doesn't
copy from the user buffer). Grep for `write(` near `/dev/null` when
reviewing signal handlers, memory walkers, or save-state validators.
