#include <math.h>

#include <PR/os.h>
#include <PR/ucode.h>
#include <nds/timers.h>
#include <nds/nds_platform.h>

OSTime osGetTime(void)
{
    return (OSTime)cpuGetTiming();
}

f32 __sinf(f32 value)
{
    return sinf(value);
}

f32 __cosf(f32 value)
{
    return cosf(value);
}

/* N64 RSP microcode placeholder storage. On the N64 these are real F3DEX2
 * microcode blobs linked into the ROM; the imported sys/taskman.c references
 * them when building its task ucode table. The DS has no RSP and the bounded
 * taskman seam returns before task_draw, so these are zero-sized placeholders
 * that satisfy the link without occupying significant DS memory. The Start !=
 * End pairs are distinct symbols so the original size arithmetic is defined. */
long long int gspF3DEX2_fifoTextStart[1];
long long int gspF3DEX2_fifoTextEnd[1];
long long int gspF3DEX2_fifoDataStart[1];
long long int gspF3DEX2_fifoDataEnd[1];
