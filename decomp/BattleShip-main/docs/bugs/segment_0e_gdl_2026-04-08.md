# Segment 0x0E G_DL Resolution (2026-04-08) — FIXED

**Symptoms:** Unhandled opcodes (values vary with ASLR: 0x96, 0xC6, 0xE1, etc.) at ~5.5s during mvOpeningRoom scene. Interpreter reads game data structures and RGBA5551 pixel data as GBI commands. Game hangs (timeout kill, exit code 124).

**Root cause:** `portNormalizeDisplayListPointer` had a special exception (`isRuntimeSegRef`) that prevented resolving G_DL commands with segment 0x0E to absolute file addresses. The rationale was that these might reference the runtime graphics heap, but in practice all G_DL segment 0x0E references in resource DLs are intra-file sub-DL branches. The MObj system only changes segment 0x0E for texture references (G_SETTIMG), not display list branches. With the exception active, `0x0E000018` was resolved at runtime via `SegAddr()` using whatever segment 0x0E currently pointed to — often the graphics heap instead of the reloc file — causing the interpreter to branch into random heap data.

**Fix:** Removed the `isRuntimeSegRef` G_DL exception. All segment 0x0E references (including G_DL) in resource DLs are now resolved to `fileBase + offset` at normalization time. Also replaced `syDebugPrintf` + `while(TRUE)` in DL buffer overflow detection with visible `port_log` output under `#ifdef PORT`.

**Files:** `libultraship/src/fast/interpreter.cpp`, `port/bridge/lbreloc_bridge.cpp`, `src/sys/taskman.c`
