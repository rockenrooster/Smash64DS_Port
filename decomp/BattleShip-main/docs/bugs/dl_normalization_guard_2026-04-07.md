# DL Normalization Guard False Positive (2026-04-07) — FIXED

**Symptoms:** Game hangs at first frame of mvOpeningRoom scene (frame 55). DX11 shader compile error `variable 'texel' used without having been completely initialized`. Appears as a hang because DX11 error path had a blocking `MessageBoxA`.

**Root cause:** `portNormalizeDisplayListPointer` had a guard (`probe[1] == 0`) to detect native 16-byte display lists within reloc file address ranges. Packed 8-byte DLs commonly start with sync commands (`gDPPipeSync`, `gDPSetFogColor(0)`, `gSPGeometryMode` with no set bits) which all have `w1=0`, causing false positives. When normalization was skipped, the interpreter read packed data at wrong stride — garbled combiner state, half of commands skipped.

**Fix:** Disabled the heuristic guard entirely (data inspection can't reliably distinguish packed vs native when many GBI commands have w1=0). Added shader compile failure resilience: `sFailedShaderIds` cache prevents infinite retry, null-program check skips broken draws, `MessageBoxA` calls removed from error paths.

**Files:** `libultraship/src/fast/interpreter.cpp`, `libultraship/src/fast/backends/gfx_direct3d11.cpp`, `libultraship/src/ship/debug/CrashHandler.cpp`, `port/gameloop.cpp`
