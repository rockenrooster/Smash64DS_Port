/*
 * Task 49 -- GX equivalence differ capture recorder (Part 2 instrument).
 *
 * Mirrors the Task 34 stage-stream recorder shape
 * (src/nds/nds_renderer.c:460-580): a cold, noinline record function with
 * overflow and fault counters, and a per-frame reset keyed on
 * gNdsRendererProfileFrameCount. The difference is bracketing: this recorder
 * groups per OWNER (not per stage segment) and advances a binding index on
 * every MATRIX_LOAD4X4, so a host analyzer can recompose each binding's
 * composed clip matrix separately for the Tier 2 effective-transform compare.
 *
 * Default off (NDS_TASK49_GX_DIFFER ?= 0). Ships no runtime behavior at the
 * default; the entire TU is compiled out unless the flag is on. The single
 * nds_renderer.c writer hook is the NDS_TASK49_GX_DIFFER_RECORD call inside
 * the lean ndsRendererTask29GXRecord funnel.
 */

#include <nds/nds_startup.h>
#include <nds/nds_task49_gx_differ.h>

#if NDS_TASK49_GX_DIFFER

#define NDS_TASK49_GX_DIFFER_CODE \
    __attribute__((noinline, noclone, cold, optimize("Os")))

/* The largest GX command word count (a 4x4 matrix load = 16). Mirrors
 * NDS_TASK29_GX_MAX_WORDS in nds_renderer.c; defined locally so this TU does
 * not depend on a renderer-private macro. */
#define NDS_TASK49_GX_DIFFER_MAX_WORDS 16u

volatile u32 gNdsTask49GxDifferFrame;
volatile u32 gNdsTask49GxDifferCaptureEnabled;
volatile u32 gNdsTask49GxDifferSelectedOwner = NDS_TASK49_GX_DIFFER_OWNER_NONE;
volatile u32 gNdsTask49GxDifferEntryCount;
volatile u32 gNdsTask49GxDifferWordCount;
volatile u32 gNdsTask49GxDifferOverflowCount;
volatile u32 gNdsTask49GxDifferFaultCount;
volatile u32 gNdsTask49GxDifferBindingCount;
volatile NDSRendererTask49GxDifferEntry
    gNdsTask49GxDifferEntries[NDS_TASK49_GX_DIFFER_ENTRY_CAPACITY];
volatile u32
    gNdsTask49GxDifferWords[NDS_TASK49_GX_DIFFER_WORD_CAPACITY];

static u32 sNdsTask49GxDifferBinding;

/* A binding begins at every composed clip-matrix load. Profile 1 emits
 * MATRIX_LOAD4X4 of the CPU-composed projection*view*model product
 * (src/nds/nds_renderer.c:12849-12868); profile 0 will emit MTX_MULT4x3 of a
 * constant model under a once-loaded view. Either way the load is the
 * boundary between one binding's geometry and the next. */
static u32 ndsRendererTask49GxDifferIsBindingStart(u32 command_class)
{
    return (command_class == (u32)NDS_TASK29_GX_MATRIX_LOAD4X4);
}

void NDS_TASK49_GX_DIFFER_CODE
ndsRendererTask49GxDifferRecord(
    u32 owner, u32 command_class, const u32 *words, u32 word_count)
{
    u32 entry_index;
    u32 first_word;
    u32 word_index;
    volatile NDSRendererTask49GxDifferEntry *entry;

    if (gNdsTask49GxDifferCaptureEnabled == FALSE)
    {
        return;
    }
    /* Per-frame reset, keyed on the shared frame counter (Task 34 idiom at
     * nds_renderer.c:531). The first record of a new frame clears the
     * buffers and restarts binding numbering. */
    if (gNdsTask49GxDifferFrame != gNdsRendererProfileFrameCount)
    {
        gNdsTask49GxDifferFrame = gNdsRendererProfileFrameCount;
        gNdsTask49GxDifferEntryCount = 0u;
        gNdsTask49GxDifferWordCount = 0u;
        gNdsTask49GxDifferOverflowCount = 0u;
        gNdsTask49GxDifferFaultCount = 0u;
        gNdsTask49GxDifferBindingCount = 0u;
        sNdsTask49GxDifferBinding = 0u;
    }
    /* Capture one owner per run. The selected owner is armed by the capture
     * harness; records from any other owner are dropped so a whole-frame
     * capture (which does not fit, see the header) is never attempted. */
    if ((gNdsTask49GxDifferSelectedOwner == NDS_TASK49_GX_DIFFER_OWNER_NONE) ||
        (owner != gNdsTask49GxDifferSelectedOwner))
    {
        return;
    }
    if (((command_class >= (u32)NDS_TASK29_GX_CLASS_COUNT) &&
         (command_class != (u32)NDS_TASK29_GX_FLUSH)) ||
        (word_count > NDS_TASK49_GX_DIFFER_MAX_WORDS) ||
        ((word_count != 0u) && (words == NULL)))
    {
        gNdsTask49GxDifferFaultCount++;
        return;
    }
    entry_index = gNdsTask49GxDifferEntryCount;
    first_word = gNdsTask49GxDifferWordCount;
    if ((entry_index >= NDS_TASK49_GX_DIFFER_ENTRY_CAPACITY) ||
        (first_word + word_count > NDS_TASK49_GX_DIFFER_WORD_CAPACITY))
    {
        gNdsTask49GxDifferOverflowCount++;
        return;
    }
    if (ndsRendererTask49GxDifferIsBindingStart(command_class) != FALSE)
    {
        sNdsTask49GxDifferBinding = gNdsTask49GxDifferBindingCount;
        gNdsTask49GxDifferBindingCount++;
    }
    entry = &gNdsTask49GxDifferEntries[entry_index];
    entry->word_offset = (u16)first_word;
    entry->command_class = (u8)command_class;
    entry->word_count = (u8)word_count;
    entry->binding_index = (u16)sNdsTask49GxDifferBinding;
    entry->owner = (u8)owner;
    entry->reserved = 0u;
    for (word_index = 0u; word_index < word_count; word_index++)
    {
        gNdsTask49GxDifferWords[first_word + word_index] = words[word_index];
    }
    gNdsTask49GxDifferEntryCount++;
    gNdsTask49GxDifferWordCount += word_count;
}

#endif /* NDS_TASK49_GX_DIFFER */
