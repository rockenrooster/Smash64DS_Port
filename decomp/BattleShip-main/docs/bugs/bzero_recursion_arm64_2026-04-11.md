# bzero Infinite Recursion on macOS/arm64 (2026-04-11) — FIXED

**Symptoms:** On macOS Apple Silicon the port segfaulted inside `std::make_shared<Fast::Fast3dWindow>()` → `Interpreter::Interpreter()` → first `new RSP()`. The crash report showed dozens of stack frames of `bzero → bzero → bzero → …` and terminated with `EXC_BAD_ACCESS: Could not determine thread index for stack guard region` (i.e. thread stack overflow).

**Root cause:** `port/stubs/libc_compat.c` previously provided `void bzero(void *p, unsigned int n) { memset(p, 0, n); }`. Two problems:
1. The final macOS binary exported `_bzero`, shadowing libSystem's `bzero` for every module that dynamically resolved it.
2. Clang lowers `memset(ptr, 0, len)` back to a `bzero()` call as a peephole optimization. With the shadowing stub in place, `bzero → memset → bzero → memset → …` recursed forever. The value-initialized `new RSP()` (which the compiler implements via zero-fill memset) was the first caller large enough to trip the stack guard.

**Fix:** Delete the bzero stub entirely — every libc we target (glibc, musl, libSystem, MSVC's BSD compatibility shims) already provides `bzero`, so there is nothing to emulate. Decomp call sites that use `bzero` resolve to the platform version via `<PR/os.h>` as they did before.

**Files:** `port/stubs/libc_compat.c`.
