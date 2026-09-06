#ifndef NDS_PREVIEW_PACK_H
#define NDS_PREVIEW_PACK_H

#include <ssb_types.h>

/* Offline source-data layout for the 1P character-select previews. Animation
 * files and event scripts keep their existing source loaders. All records
 * below are little endian; section bytes retain the O2R big-endian words.
 * File order: header, sections, data, fixups, spans. */
#define NDS_PREVIEW_PACK_MAGIC 0x31435046u /* FPC1 */
#define NDS_PREVIEW_PACK_VERSION 1u
#define NDS_PREVIEW_PACK_MAX_SECTIONS 4u
#define NDS_PREVIEW_PACK_NULL 0xffffffffu

typedef struct NDSPreviewPackHeader {
    u32 magic;
    u32 version;
    u32 file_bytes;
    u32 fkind;
    u32 section_count;
    u32 fixup_count;
    u32 span_count;
    u32 reserved;
    u32 data_bytes;
    u32 data_hash;
    u32 fixup_hash;
    u32 span_hash;
    u32 main_asset_id;
    u32 model_asset_id;
    u32 reserved_tail[2];
} NDSPreviewPackHeader;

/* Section 0 is Main, section 1 Model; remaining sections preserve separately
 * identified tail files (Donkey's stock data). Offsets are relative to data.
 * Model root cells are pairs of BE32 words: ENDDL and original root offset.
 * Native drawing consumes the original identity; it never executes the cell. */
typedef struct NDSPreviewPackSection {
    u32 asset_id;
    u32 data_offset;
    u32 data_bytes;
    u32 source_bytes;
    u32 first_span;
    u32 span_count;
    u32 roots_offset;
    u32 root_count;
} NDSPreviewPackSection;

typedef struct NDSPreviewPackFixup {
    u32 slot_offset;
    u32 target_offset; /* NDS_PREVIEW_PACK_NULL writes a null pointer. */
} NDSPreviewPackFixup;

/* Maps original byte offsets to a section's compact layout. Root identities
 * have their own cells; only retained data spans appear here. */
typedef struct NDSPreviewPackSpan {
    u32 source_offset;
    u32 data_offset; /* Relative to its section, unlike fixup offsets. */
    u32 data_bytes;
} NDSPreviewPackSpan;

_Static_assert(sizeof(NDSPreviewPackHeader) == 64, "preview header ABI");
_Static_assert(sizeof(NDSPreviewPackSection) == 32, "preview section ABI");
_Static_assert(sizeof(NDSPreviewPackFixup) == 8, "preview fixup ABI");
_Static_assert(sizeof(NDSPreviewPackSpan) == 12, "preview span ABI");

#if NDS_P2_1P_GAME
s32 ndsRelocLoadPreviewFighter(s32 fkind);
/* Only native production's original-offset image references use this seam;
 * ordinary relocated MObj pointers already address the compact bytes. */
const void *ndsRelocNativeAssetAddress(const void *base, u32 offset);
#endif

#endif
