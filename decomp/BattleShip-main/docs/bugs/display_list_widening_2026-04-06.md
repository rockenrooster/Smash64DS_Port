# Display List Widening (2026-04-06) — FIXED

**Symptoms:** Unhandled opcodes (0xEC, 0x84, 0x94, 0xCD), crashes in renderer.

**Root cause:** Resource DLs in reloc files are 8 bytes/entry (N64 format) but PC interpreter expects 16 bytes/entry. Segment 0x0E addresses within packed DLs resolved against wrong base.

**Fix:** `portNormalizeDisplayListPointer` widens packed DLs to native format with segment 0x0E rewriting, opcode validation, and G_ENDDL guarantee.

**Files:** `libultraship/src/fast/interpreter.cpp`
