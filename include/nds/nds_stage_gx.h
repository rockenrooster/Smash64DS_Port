#ifndef NDS_STAGE_GX_H
#define NDS_STAGE_GX_H

#include <stdint.h>

enum { NDS_STAGE_GX_VIEW = 1, NDS_STAGE_GX_WORLD, NDS_STAGE_GX_NOZ,
       NDS_STAGE_GX_COLOR, NDS_STAGE_GX_UV, NDS_STAGE_GX_PROJECTION,
       NDS_STAGE_GX_COMPOSED_NOZ, NDS_STAGE_GX_CORNER_NOZ,
       NDS_STAGE_GX_MATERIAL, NDS_STAGE_GX_COMPOSED,
       NDS_STAGE_GX_CORNER_SOURCE,
       /* Version 6: the translated painter -- one matrix a no-Z run, the
        * depth stepped by MTX_TRANS (compile_nds_stage_gx.py). Keep these
        * two last: the executors test kind >= NOZ_VIEW_RUN. */
       NDS_STAGE_GX_NOZ_VIEW_RUN, NDS_STAGE_GX_NOZ_COMPOSED_RUN };
typedef struct NDSStageGxHeader {
    uint32_t magic, version, gkind, run_count, word_count, patch_count;
    uint32_t segment_mask, source_hash, body_bytes, body_hash, static_world_lo, static_world_hi;
} NDSStageGxHeader;
typedef struct NDSStageGxRun {
    uint16_t first_word, word_count, first_patch, patch_count, triangles, segment;
    int16_t minimum[3], maximum[3];
    uint32_t binding_lo, binding_hi;
} NDSStageGxRun;
typedef struct NDSStageGxPatch {
    uint16_t word, kind, index, aux;
} NDSStageGxPatch;
/* Version 9's trailer (after the body: a u32 count, then one row a packet
 * segment): the segment's slices of the run, patch and word tables, which
 * are consecutive from one segment to the next, and one FNV-1a over the
 * three slices in that order. */
typedef struct NDSStageGxSegment {
    uint16_t first_run, run_count, first_patch, patch_count, first_word, word_count;
    uint32_t hash;
} NDSStageGxSegment;

#endif
