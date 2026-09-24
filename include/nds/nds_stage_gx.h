#ifndef NDS_STAGE_GX_H
#define NDS_STAGE_GX_H

#include <stdint.h>

enum { NDS_STAGE_GX_VIEW = 1, NDS_STAGE_GX_WORLD, NDS_STAGE_GX_NOZ,
       NDS_STAGE_GX_COLOR, NDS_STAGE_GX_UV, NDS_STAGE_GX_PROJECTION,
       NDS_STAGE_GX_COMPOSED_NOZ, NDS_STAGE_GX_CORNER_NOZ,
       NDS_STAGE_GX_MATERIAL };
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

#endif
