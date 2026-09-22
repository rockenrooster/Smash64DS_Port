#ifndef NDS_PREVIEW_PACK_H
#define NDS_PREVIEW_PACK_H

#include <ssb_types.h>

/* Offline source-data layout for the 1P character-select previews. Animation
 * files and event scripts keep their existing source loaders. All records
 * below are little endian; section bytes retain the O2R big-endian words.
 * File order: header, sections, data, fixups, spans. */
#define NDS_PREVIEW_PACK_MAGIC 0x31435046u /* FPC1 */
#define NDS_PREVIEW_PACK_VERSION 2u
#define NDS_PREVIEW_PACK_MAX_SECTIONS 4u
#define NDS_PREVIEW_PACK_NULL 0xffffffffu

/* Shared VS/1P CSS preview-arena policy. Compact native packs include their
 * owner images; source/oracle profiles retain the complete reloc closure. */
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
#define NDS_PLAYERS_VS_SLOT_RESIDENT_BYTES (80u * 1024u)
#else
#define NDS_PLAYERS_VS_SLOT_RESIDENT_BYTES (156u * 1024u)
#endif

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
    /* Runtime owner validation needs the source Model file's raw byte size.
     * source_bytes in section 1 remains the span extent, which can be larger
     * when the offline compiler appends synthetic welded display lists. */
    u32 model_source_bytes;
    u32 reserved_tail;
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

#if NDS_P2_1P_GAME || NDS_P2_MENU_SHELL || NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
s32 ndsRelocLoadPreviewFighter(s32 fkind);

/* Resumable form of the same compact load, for the character-select preview
 * transaction. M03: the one-call form moved the whole pack (up to 32,032 data
 * bytes plus 385 fixup records) between a BGM suspend and its resume, so every
 * hover that missed the four-block cache silenced the menu track for the whole
 * closure. Begin/Step/Cancel run the identical phases under a caller-supplied
 * per-step span budget, so the CSS can advance one bounded unit per update and
 * the stream keeps being serviced by the shell loop's own ndsAudioBgmUpdate.
 *
 * Nothing is published to FTData until the owner boundary: a completed
 * transaction is STAGED, and the next ftManagerSetupFilesAllKind ->
 * ndsRelocLoadPreviewFighter for that kind consumes it in O(1) and returns 2
 * exactly as the synchronous path does. A cancelled transaction closes its file
 * handle and drops its staging without ever touching p_file_main.
 *
 * The handle is allocated from the malloc region current at Begin, which is the
 * caller's resettable block arena; Cancel releases the file handle and the
 * staging, and the caller's syMallocReset reclaims the bytes. */
enum {
    NDS_PREVIEW_PACK_STEP_FAIL = 0,
    NDS_PREVIEW_PACK_STEP_IN_PROGRESS = 1,
    NDS_PREVIEW_PACK_STEP_DONE = 2
};
void *ndsRelocPreviewFighterLoadBegin(s32 fkind);
s32 ndsRelocPreviewFighterLoadStep(void *handle, u32 byte_budget,
                                   u32 *out_bytes);
/* `fkind` is checked against the handle so a stale pointer to a pool row that
 * has since been recycled cannot retire another kind's transaction. */
void ndsRelocPreviewFighterLoadCancel(void *handle, s32 fkind);
/* TRUE once Step has returned DONE and the kind's pack is waiting for its
 * owner-boundary publication. */
s32 ndsRelocPreviewFighterLoadIsStaged(s32 fkind);
extern volatile u32 gNdsPreviewPackStepCount;
extern volatile u32 gNdsPreviewPackStepByteMax;
extern volatile u32 gNdsPreviewPackStageCommitCount;
extern volatile u32 gNdsPreviewPackStageCancelCount;
/* Results uses the same full-motion fighter data, not the idle-preview pack. */
u32 ndsRelocUseBattleCoreFighterData(void);
/* Only native production's original-offset image references use this seam;
 * ordinary relocated MObj pointers already address the compact bytes. */
const void *ndsRelocNativeAssetAddress(const void *base, u32 offset);
#endif
/* Model display-list roots are not ordinary retained spans in FPC2. They live
 * in the section's identity-cell tail as { ENDDL, source_root_offset }. Resolve
 * that cell explicitly; raw/non-compact files preserve base + root_offset. */
const void *ndsRelocNativeRootAddress(const void *base, u32 root_offset);
/* Retire the compact pack records owned by one fighter before its resettable
 * CSS arena is reused. This is symmetric with ndsRelocLoadPreviewFighter and
 * does not depend on the arena pointer still being discoverable afterward.
 * Declared for every configuration: the character-select retire paths call it
 * unconditionally, and a configuration without compact packs links the empty
 * definition beside ndsRelocNativeForeignImageAddress in reloc_backend_assets.c. */
void ndsRelocReleasePreviewFighter(s32 fkind);
void ndsMNPlayersClearPreviewFighterFiles(s32 fkind);

/* Foreign IMAGE/TLUT identity is source-qualified. Compact owners consult
 * their private scene bank; raw owners consult the actual loaded asset. */
const void *ndsRelocNativeForeignImageAddress(const void *base, u32 asset_id,
                                             u32 offset);

#if NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
/* FPC1 VSBattle packs leave non-Model Main externs NULL until the source
 * status/special closure has been made resident.  Restore those generated
 * source slots before fighter construction. */
s32 ndsRelocPatchCompactBattleMainExterns(s32 fkind);
extern volatile u32 gNdsBattleCoreExternPatchCount;
extern volatile u32 gNdsBattleCoreExternLoadCount;
extern volatile u32 gNdsBattleCoreExternFailure;
extern volatile u32 gNdsBattleCoreForeignImageBytes;
extern volatile u32 gNdsBattleCoreForeignImageRows;
extern volatile u32 gNdsBattleCoreForeignImageLoadCount;
#endif

#endif
