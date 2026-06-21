#ifndef SSB64_NDS_SPTASK_H
#define SSB64_NDS_SPTASK_H

#include <PR/ultratypes.h>

typedef struct {
    u32 type;
    u32 flags;
    u64 *ucode_boot;
    u32 ucode_boot_size;
    u64 *ucode;
    u32 ucode_size;
    u64 *ucode_data;
    u32 ucode_data_size;
    u64 *dram_stack;
    u32 dram_stack_size;
    u64 *output_buffer;
    u64 *output_buffer_size;
    u64 *data_ptr;
    u32 data_size;
    u64 *yield_data_ptr;
    u32 yield_data_size;
} OSTask_t;

typedef union {
    OSTask_t t;
    long long force_structure_alignment;
} OSTask;

typedef u32 OSYieldResult;

#define OS_TASK_YIELDED 0x0001u
#define OS_TASK_DP_WAIT 0x0002u
#define OS_TASK_LOADABLE 0x0004u

/* Yield data buffer size. The N64 RSP yield buffer is this many bytes; the DS
 * has no RSP but the imported task manager sizes its yield-data allocation. */
#define OS_YIELD_DATA_SIZE 0xC00

void osSpTaskLoad(OSTask *task);
void osSpTaskStartGo(OSTask *task);
void osSpTaskYield(void);
OSYieldResult osSpTaskYielded(OSTask *task);

#define osSpTaskStart(task) do { \
    osSpTaskLoad((task)); \
    osSpTaskStartGo((task)); \
} while (0)

#endif

