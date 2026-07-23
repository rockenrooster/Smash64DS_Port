#ifndef SSB64_NDS_TASK49_GX_DIFFER_H
#define SSB64_NDS_TASK49_GX_DIFFER_H

/*
 * Task 49 -- GX equivalence differ capture (Part 2 instrument).
 *
 * Records the per-owner GX command stream word-for-word so a host analyzer can
 * diff a profile-0 stream against the profile-1 stream for the same frame.
 * The capture shape follows the Task 34 stage-stream recorder
 * (src/nds/nds_renderer.c:460-580) but brackets per OWNER rather than per stage
 * segment: a binding = one owner + the sequence of commands following a
 * MATRIX_LOAD4X4 (the composed clip matrix that starts each binding's run).
 *
 * Capacity is sized from Task 34's measured count, not a guess: the stage alone
 * uses 2,557 entries / 6,894 words (PERF_LEDGER.md:159-170), so the whole-frame
 * capture does not fit in Task 34's buffer. Capture one owner per run, selected
 * by the build flag, and report overflow/fault counters in every certificate --
 * as Task 34 and Task 48 both do.
 *
 * Default off. Ships no runtime behavior; this is a measurement instrument.
 */

/* Task 46 header idiom: this header does NOT #include nds_build_config.h (it is
 * force-included via CFLAGS -include at Makefile:535). If the flag is undefined
 * the include order is broken and the build must fail loudly rather than silently
 * compile the recorder out -- this project has paid for silent no-ops. */
#ifndef NDS_TASK49_GX_DIFFER
#error "nds_task49_gx_differ.h needs nds_build_config.h (CFLAGS -include)"
#endif

#include <nds/nds_renderer.h>

/* One owner's full frame fits within Task 34's stage-only measured count
 * (2,557 entries / 6,894 words); owner capture is comfortably under this. */
#define NDS_TASK49_GX_DIFFER_ENTRY_CAPACITY 2800u
#define NDS_TASK49_GX_DIFFER_WORD_CAPACITY 7000u
#define NDS_TASK49_GX_DIFFER_OWNER_NONE 0xffu

typedef struct NDSRendererTask49GxDifferEntry
{
    u16 word_offset;
    u8 command_class;
    u8 word_count;
    u16 binding_index;
    u8 owner;
    u8 reserved;
} NDSRendererTask49GxDifferEntry;

#if NDS_TASK49_GX_DIFFER
extern volatile u32 gNdsTask49GxDifferFrame;
extern volatile u32 gNdsTask49GxDifferCaptureEnabled;
extern volatile u32 gNdsTask49GxDifferSelectedOwner;
extern volatile u32 gNdsTask49GxDifferEntryCount;
extern volatile u32 gNdsTask49GxDifferWordCount;
extern volatile u32 gNdsTask49GxDifferOverflowCount;
extern volatile u32 gNdsTask49GxDifferFaultCount;
extern volatile u32 gNdsTask49GxDifferBindingCount;
extern volatile NDSRendererTask49GxDifferEntry
    gNdsTask49GxDifferEntries[NDS_TASK49_GX_DIFFER_ENTRY_CAPACITY];
extern volatile u32
    gNdsTask49GxDifferWords[NDS_TASK49_GX_DIFFER_WORD_CAPACITY];

/* Called from the single funnel ndsRendererTask29GXRecord hook site
 * (src/nds/nds_renderer.c). The owner is passed in because the runtime owner
 * is a file-static of nds_renderer.c that this TU cannot name. */
void ndsRendererTask49GxDifferRecord(
    u32 owner, u32 command_class, const u32 *words, u32 word_count);
#else
#define NDS_TASK49_GX_DIFFER_RECORD(owner, command_class, words, word_count) \
    ((void)0)
#endif

#endif
