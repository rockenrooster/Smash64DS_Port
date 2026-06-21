#ifndef SSB64_NDS_PR_UCODE_H
#define SSB64_NDS_PR_UCODE_H

/* N64 RSP microcode linker-symbol declarations (narrow PR/ucode.h shim).
 *
 * The imported sys/taskman.c references the F3DEX2 FIFO microcode symbols
 * (gspF3DEX2_fifoTextStart/DataStart) when building its task ucode table. On
 * the N64 these resolve to RSP microcode blobs linked into the ROM; on the DS
 * there is no RSP, so the symbols are backed by DS-owned placeholder storage
 * (defined in src/port) and the task ucode path is never exercised for real
 * rendering in this increment (the DS seam returns before task_draw).
 *
 * Only the symbols the imported task manager actually references are declared
 * here, per the project's narrow-ABI policy. */
#include <PR/ultratypes.h>

/* N64 SP microcode size constants. The imported task manager sizes its SP
 * DRAM-stack, ucode, and ucode-data allocations from these; the DS has no SP,
 * so they only need to be defined for the allocation arithmetic to compile. */
#define SP_DRAM_STACK_SIZE8 1024
#define SP_DRAM_STACK_SIZE64 (SP_DRAM_STACK_SIZE8 >> 3)
#define SP_UCODE_SIZE 4096
#define SP_UCODE_DATA_SIZE 2048

extern long long int gspF3DEX2_fifoTextStart[], gspF3DEX2_fifoTextEnd[];
extern long long int gspF3DEX2_fifoDataStart[], gspF3DEX2_fifoDataEnd[];

#endif /* SSB64_NDS_PR_UCODE_H */
