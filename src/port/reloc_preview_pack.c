/* Included by reloc_backend_assets.c: shares the scene-owned file registry and
 * its existing source-format normalizers. Never pages fighter data at runtime. */
#include <nds/nds_preview_pack.h>
#include <nds/nds_scene_manager.h>
#include <stdio.h>

#if NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
typedef struct NDSBattleForeignImageRow {
    u16 asset_id;
    u16 reserved;
    u32 source_offset;
    u32 data_offset;
    u32 data_bytes;
} NDSBattleForeignImageRow;
_Static_assert(sizeof(NDSBattleForeignImageRow) == 16u, "foreign image row ABI");
#endif

typedef struct NDSPreviewResident {
    u32 generation;
    u32 model_source_bytes;
    NDSPreviewPackSection *sections;
    NDSPreviewPackSpan *spans;
#if NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
    NDSBattleForeignImageRow *foreign_images;
    u32 foreign_count;
#endif
} NDSPreviewResident;

static NDSPreviewResident sNdsPreviewResidents[12];
volatile u32 gNdsPreviewPackLoadCount;
volatile u32 gNdsPreviewPackDataBytes;
volatile u32 gNdsPreviewPackFailure;
volatile u32 gNdsPreviewPackFailureKind;
#if NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
volatile u32 gNdsBattleCoreExternPatchCount;
volatile u32 gNdsBattleCoreExternLoadCount;
volatile u32 gNdsBattleCoreExternFailure;
volatile u32 gNdsBattleCoreForeignImageBytes;
volatile u32 gNdsBattleCoreForeignImageRows;
volatile u32 gNdsBattleCoreForeignImageLoadCount;
#define NDS_BATTLE_EXTERN_MAGIC 0x31584542u
#define NDS_BATTLE_EXTERN_VERSION 2u
#define NDS_BATTLE_EXTERN_MAX 24u
typedef struct NDSBattleExternHeader {
    u32 magic;
    u16 version;
    u16 count;
    u32 foreign_count;
    u32 foreign_bytes;
    u32 foreign_hash;
    u32 reserved;
} NDSBattleExternHeader;
typedef struct NDSBattleExternRow {
    u16 slot;
    u16 dep_asset;
    u16 target_offset;
} NDSBattleExternRow;
_Static_assert(sizeof(NDSBattleExternHeader) == 24u, "battle extern header ABI");
_Static_assert(sizeof(NDSBattleExternRow) == 6u, "battle extern row ABI");
#endif

static u32 ndsPreviewHash(const void *data, size_t size, u32 hash)
{
    const u8 *bytes = data;
    while (size-- != 0u)
    {
        hash = (hash ^ *bytes++) * 16777619u;
    }
    return hash;
}

static s32 ndsPreviewRange(u32 offset, u32 bytes, u32 total)
{
    return (offset <= total) && (bytes <= total - offset);
}

static const NDSPreviewPackSection *ndsPreviewSection(
    const NDSRelocLoadedFile *loaded, const NDSPreviewResident **out_resident)
{
    const NDSPreviewResident *resident;
    u32 kind;
    if ((loaded == NULL) || (loaded->reserved[0] == 0u))
    {
        return NULL;
    }
    kind = loaded->reserved[0] - 1u;
    if ((kind >= ARRAY_COUNT(sNdsPreviewResidents)) ||
        (loaded->reserved[1] >= NDS_PREVIEW_PACK_MAX_SECTIONS))
    {
        return NULL;
    }
    resident = &sNdsPreviewResidents[kind];
    if ((resident->generation != sNdsRelocSceneGeneration) ||
        (loaded->owner_generation != sNdsRelocSceneGeneration))
    {
        return NULL;
    }
    if (out_resident != NULL) { *out_resident = resident; }
    return &resident->sections[loaded->reserved[1]];
}

static s32 ndsPreviewFileOffset(const NDSRelocLoadedFile *loaded,
                               u32 source_offset, u32 size, u32 *out_offset)
{
    const NDSPreviewResident *resident;
    const NDSPreviewPackSection *section = ndsPreviewSection(loaded, &resident);
    u32 i;
    if (section == NULL)
    {
        if (loaded->reserved[0] != 0u) { return FALSE; }
        if (!ndsPreviewRange(source_offset, size, loaded->data_size)) { return FALSE; }
        *out_offset = source_offset;
        return TRUE;
    }
    for (i = 0u; i < section->span_count; i++)
    {
        const NDSPreviewPackSpan *span = &resident->spans[section->first_span + i];
        if ((source_offset >= span->source_offset) &&
            ndsPreviewRange(source_offset - span->source_offset, size, span->data_bytes))
        {
            *out_offset = span->data_offset + source_offset - span->source_offset;
            return TRUE;
        }
    }
    return FALSE;
}

static u32 ndsRelocNativeSourceSize(const NDSRelocLoadedFile *loaded)
{
    const NDSPreviewResident *resident;
    const NDSPreviewPackSection *section = ndsPreviewSection(loaded, &resident);
    if ((section == NULL) && (loaded->reserved[0] != 0u)) { return 0u; }
    if ((section != NULL) && (loaded->reserved[1] == 1u))
    {
        return resident->model_source_bytes;
    }
    return (section != NULL) ? section->source_bytes : loaded->data_size;
}

static u32 ndsRelocNativeRootOffset(const NDSRelocLoadedFile *loaded, const Gfx *dl)
{
    const NDSPreviewPackSection *section = ndsPreviewSection(loaded, NULL);
    u32 offset = (u32)((uintptr_t)dl - (uintptr_t)loaded->data);
    if (section == NULL) { return loaded->reserved[0] ? UINT32_MAX : offset; }
    if ((offset < section->roots_offset) ||
        (((offset - section->roots_offset) & 7u) != 0u) ||
        ((offset - section->roots_offset) / 8u >= section->root_count) ||
        (ndsRelocReadNative32(dl) != 0xdf000000u))
    {
        return UINT32_MAX;
    }
    return ndsRelocReadNative32((const u8 *)dl + 4u);
}

const void *ndsRelocNativeAssetAddress(const void *base, u32 offset)
{
    NDSRelocLoadedFile *loaded;
    u32 mapped;
    if ((gSCManagerSceneData.scene_curr != nSCKind1PGamePlayers) &&
        (gSCManagerSceneData.scene_curr != nSCKindPlayersVS)
#if NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
        && (gNdsSceneManagerCurrIsBattle == 0u)
#endif
       )
    {
        return (const u8 *)base + offset;
    }
    loaded = ndsRelocFindLoadedFileByData((void *)base);
    if ((loaded == NULL) || (loaded->reserved[0] == 0u))
    {
        return (const u8 *)base + offset;
    }
    if (ndsPreviewFileOffset(loaded, offset, 1u, &mapped) == FALSE) { return NULL; }
    return (const u8 *)base + mapped;
}

const void *ndsRelocNativeForeignImageAddress(const void *base, u32 asset_id,
                                             u32 offset)
{
    NDSRelocLoadedFile *owner = ndsRelocFindLoadedFileByData((void *)base);
    NDSRelocLoadedFile *foreign;
    u32 mapped;
    if ((base == NULL) || (owner == NULL) ||
        (owner->owner_generation != sNdsRelocSceneGeneration) ||
        (owner->owner_scene != (u32)gSCManagerSceneData.scene_curr)) { return NULL; }
    if (owner->reserved[0] != 0u)
    {
#if NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
        const NDSPreviewResident *resident;
        u32 i;
        if (ndsPreviewSection(owner, &resident) == NULL) { return NULL; }
        for (i = 0u; i < resident->foreign_count; i++)
        {
            const NDSBattleForeignImageRow *row = &resident->foreign_images[i];
            if ((row->asset_id == asset_id) && (offset >= row->source_offset) &&
                ndsPreviewRange(offset - row->source_offset, 1u, row->data_bytes))
            {
                const u8 *data = (const u8 *)(resident->foreign_images +
                                            resident->foreign_count);
                return data + row->data_offset + offset - row->source_offset;
            }
        }
#endif
        /* Never let a missing private span borrow a different fighter's bank. */
        return NULL;
    }
    foreign = ndsRelocFindLoadedFileByAsset(asset_id);
    if ((foreign == NULL) || (foreign->data == NULL) ||
        (foreign->owner_generation != sNdsRelocSceneGeneration) ||
        (foreign->owner_scene != (u32)gSCManagerSceneData.scene_curr) ||
        !ndsPreviewFileOffset(foreign, offset, 1u, &mapped)) { return NULL; }
    return (const u8 *)foreign->data + mapped;
}

static __attribute__((noinline, noreturn)) void ndsPreviewPackLoadHalt(u32 reason, u32 kind)
{
    gNdsPreviewPackFailure = reason;
    gNdsPreviewPackFailureKind = kind;
    gNdsRelocAssetFormatFailCount++;
    for (;;) { __asm__ volatile("nop"); }
}

/* Require complete, disjoint sections and bounded remaps before publishing
 * anything to FTData. Hashes catch truncated/corrupt generated payloads. */
static s32 ndsPreviewValidateSections(const NDSPreviewPackHeader *header,
                                      const NDSPreviewPackSection *sections)
{
    u32 i;
    u32 end = 0u;
    for (i = 0u; i < header->section_count; i++)
    {
        const NDSPreviewPackSection *s = &sections[i];
        u32 j;
        if ((s->data_bytes == 0u) || ((s->data_offset | s->data_bytes) & 3u) ||
            ((s->data_offset & 15u) != 0u) || (s->data_offset < end) ||
            !ndsPreviewRange(s->data_offset, s->data_bytes, header->data_bytes) ||
            !ndsPreviewRange(s->first_span, s->span_count, header->span_count) ||
            (s->source_bytes == 0u) || (s->root_count > s->data_bytes / 8u) ||
            !ndsPreviewRange(s->roots_offset, s->root_count * 8u, s->data_bytes) ||
            ((i != 1u) && (s->root_count != 0u || s->roots_offset != 0u)) ||
            ((i == 1u) && (s->root_count == 0u ||
                s->roots_offset + s->root_count * 8u != s->data_bytes)))
        {
            return FALSE;
        }
        for (j = 0u; j < i; j++)
        {
            if (sections[j].asset_id == s->asset_id) { return FALSE; }
        }
        end = s->data_offset + s->data_bytes;
    }
    return (sections[0].asset_id == header->main_asset_id) &&
           (sections[1].asset_id == header->model_asset_id) &&
           (end == header->data_bytes);
}

static s32 ndsPreviewSectionContains(const NDSPreviewPackSection *sections,
                                     u32 count, u32 offset, u32 size)
{
    u32 i;
    for (i = 0u; i < count; i++)
    {
        if ((offset >= sections[i].data_offset) &&
            ndsPreviewRange(offset - sections[i].data_offset, size, sections[i].data_bytes))
        {
            return TRUE;
        }
    }
    return FALSE;
}

void ndsFsLock(void);
void ndsFsUnlock(void);

static s32 ndsRelocLoadPreviewFighterUnlocked(s32 fkind)
{
    NDSPreviewPackHeader header;
    NDSPreviewPackSection sections[NDS_PREVIEW_PACK_MAX_SECTIONS];
    NDSRelocLoadedFile *records[NDS_PREVIEW_PACK_MAX_SECTIONS];
    NDSPreviewResident *resident;
    NDSPreviewPackFixup pair;
    NDSPreviewPackSpan *spans;
    FTData *fighter;
    NDSRelocAssetHeader reloc_header;
    FILE *file;
    char preview_path[] = "nitro:/fighters/preview/00.fpc";
#if NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
    char battle_path[] = "nitro:/fighters/battle/00.fpc";
#endif
    char *path = preview_path;
    u32 digit_at = sizeof("nitro:/fighters/preview/") - 1u;
    s32 is_battle_pack = FALSE;
    u8 *data;
    u32 i;
    u32 hash;
    u32 allocation;
    long file_size;
    u64 expected_size;

#if !NDS_RENDERER_HW_TRIANGLES || (NDS_RENDERER_PROFILE_LEVEL >= 2)
    /* The source/oracle renderer still consumes full Gfx/Vtx programs. */
    return FALSE;
#endif
    if (((gSCManagerSceneData.scene_curr != nSCKind1PGamePlayers) &&
         (gSCManagerSceneData.scene_curr != nSCKindPlayersVS)
#if NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
         && (gNdsSceneManagerCurrIsBattle == 0u)
#endif
        ) ||
        ((u32)fkind >= ARRAY_COUNT(sNdsPreviewResidents))) { return FALSE; }
    fighter = dFTManagerDataFiles[fkind];
    if ((fighter == NULL) || (fighter->p_file_main == NULL) ||
        (fighter->p_file_model == NULL)) { ndsPreviewPackLoadHalt(1u, fkind); }
    if (*fighter->p_file_main != NULL) { return TRUE; }

    /* The roster index is two decimal digits. Pulling in snprintf here
     * retained newlib's floating-point formatter for this integer-only path. */
#if NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
    if (gNdsSceneManagerCurrIsBattle != 0u)
    {
        path = battle_path;
        digit_at = sizeof("nitro:/fighters/battle/") - 1u;
        is_battle_pack = TRUE;
    }
#endif
    path[digit_at] = '0' + (u32)fkind / 10u;
    path[digit_at + 1u] = '0' + (u32)fkind % 10u;
    file = fopen(path, "rb");
    if (file == NULL) { ndsPreviewPackLoadHalt(2u, fkind); }
    if (fseek(file, 0, SEEK_END) != 0) { ndsPreviewPackLoadHalt(3u, fkind); }
    file_size = ftell(file);
    if ((file_size < (long)sizeof(header)) || (fseek(file, 0, SEEK_SET) != 0) ||
        (fread(&header, 1u, sizeof(header), file) != sizeof(header)))
    {
        ndsPreviewPackLoadHalt(3u, fkind);
    }
    expected_size = sizeof(header) + (u64)header.section_count * sizeof(sections[0]) +
        header.data_bytes + (u64)header.fixup_count * sizeof(pair) +
        (u64)header.span_count * sizeof(*spans);
    if ((header.magic != NDS_PREVIEW_PACK_MAGIC) ||
        (header.version != NDS_PREVIEW_PACK_VERSION) || (header.fkind != (u32)fkind) ||
        (header.section_count < 2u) || (header.section_count > ARRAY_COUNT(sections)) ||
        (header.main_asset_id != ndsRelocAssetIDForToken((u32)fighter->file_main_id)) ||
        (header.model_asset_id != ndsRelocAssetIDForToken((u32)fighter->file_model_id)) ||
        (header.file_bytes != (u32)file_size) || (expected_size != (u32)file_size) ||
        (header.model_source_bytes == 0u) || (header.reserved | header.reserved_tail) ||
        (header.data_bytes & 3u) ||
        (sNdsRelocLoadedFileCount + header.section_count > NDS_RELOC_LOADED_FILE_CAPACITY) ||
        (fread(sections, sizeof(sections[0]), header.section_count, file) != header.section_count) ||
        !ndsPreviewValidateSections(&header, sections))
    {
        ndsPreviewPackLoadHalt(4u, fkind);
    }
    if (header.model_source_bytes > sections[1].source_bytes)
    {
        ndsPreviewPackLoadHalt(4u, fkind);
    }
    /* Metadata is smaller than its bytes on disk; the file-size equality above
     * proves this addition cannot overflow the positive signed file length. */
    allocation = header.data_bytes + header.section_count * sizeof(sections[0]) +
        header.span_count * sizeof(*spans);
    data = syTaskmanMalloc(allocation, 16u);
    if (fread(data, 1u, header.data_bytes, file) != header.data_bytes ||
        ndsPreviewHash(data, header.data_bytes, 2166136261u) != header.data_hash)
    {
        ndsPreviewPackLoadHalt(5u, fkind);
    }
    for (i = 0u; i < header.data_bytes; i += 4u)
    {
        ndsRelocWriteNative32(data + i, ndsRelocReadBe32(data + i));
    }
    hash = 2166136261u;
    for (i = 0u; i < header.fixup_count; i++)
    {
        if (fread(&pair, 1u, sizeof(pair), file) != sizeof(pair) ||
            (pair.slot_offset & 3u) ||
            !ndsPreviewSectionContains(sections, header.section_count, pair.slot_offset, 4u) ||
            ((pair.target_offset != NDS_PREVIEW_PACK_NULL) &&
             !ndsPreviewSectionContains(sections, header.section_count, pair.target_offset, 1u)))
        {
            ndsPreviewPackLoadHalt(6u, fkind);
        }
        hash = ndsPreviewHash(&pair, sizeof(pair), hash);
        ndsRelocWriteNativePointer(data + pair.slot_offset,
            (pair.target_offset == NDS_PREVIEW_PACK_NULL) ? NULL : data + pair.target_offset);
    }
    if (hash != header.fixup_hash) { ndsPreviewPackLoadHalt(6u, fkind); }
    resident = &sNdsPreviewResidents[fkind];
    resident->model_source_bytes = header.model_source_bytes;
#if NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
    resident->foreign_images = NULL;
    resident->foreign_count = 0u;
#endif
    resident->sections = (NDSPreviewPackSection *)(data + header.data_bytes);
    memcpy(resident->sections, sections, header.section_count * sizeof(sections[0]));
    spans = (NDSPreviewPackSpan *)(resident->sections + header.section_count);
    if ((fread(spans, sizeof(*spans), header.span_count, file) != header.span_count) ||
        (ndsPreviewHash(spans, header.span_count * sizeof(*spans), 2166136261u) != header.span_hash))
    {
        ndsPreviewPackLoadHalt(7u, fkind);
    }
    fclose(file);
    resident->spans = spans;
    resident->generation = sNdsRelocSceneGeneration;
    for (i = 0u; i < header.section_count; i++)
    {
        const NDSPreviewPackSection *s = &sections[i];
        u32 j;
        for (j = 0u; j < s->span_count; j++)
        {
            const NDSPreviewPackSpan *span = &spans[s->first_span + j];
            if (!ndsPreviewRange(span->source_offset, span->data_bytes, s->source_bytes) ||
                !ndsPreviewRange(span->data_offset, span->data_bytes, s->data_bytes))
            {
                ndsPreviewPackLoadHalt(8u, fkind);
            }
        }
        if (ndsRelocFindLoadedFileByAsset(s->asset_id) != NULL)
        {
            ndsPreviewPackLoadHalt(9u, fkind);
        }
        memset(&reloc_header, 0, sizeof(reloc_header));
        reloc_header.file_id = s->asset_id;
        reloc_header.data_size = s->data_bytes;
        reloc_header.reloc_intern_offset = reloc_header.reloc_extern_offset = 0xffffu;
        records[i] = ndsRelocRegisterLoadedFile(s->asset_id, 0u, data + s->data_offset, &reloc_header);
        if (records[i] == NULL) { ndsPreviewPackLoadHalt(10u, fkind); }
        records[i]->internal_fixups_applied = records[i]->external_fixups_applied = TRUE;
        records[i]->reserved[0] = (u8)(fkind + 1);
        records[i]->reserved[1] = (u8)i;
    }
    if (ndsRelocNormalizeFighterAttributesFile(records[0]) == FALSE)
    {
        ndsPreviewPackLoadHalt(11u, fkind);
    }
    for (i = 0u; i < header.section_count; i++)
    {
        if (ndsRelocNormalizeBattleInterfaceSprites(records[i]) == FALSE)
        {
            ndsPreviewPackLoadHalt(12u, fkind);
        }
    }
    /* Main is byte-for-byte source-sized in FPC1, but generic preview packing
     * deliberately NULLs dependencies outside the compact sections.  For the
     * migrated P2-2 fighters those nine ShieldPose externs are not optional:
     * the native guard package replaces them.  Repoint the generated source
     * slots only after all generic normalization is complete so no later
     * byte-lane pass can reinterpret a native pointer. */
#if NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
    if (is_battle_pack &&
        (ndsShieldPosePatchCompactMain(
            fkind, records[0]->data, records[0]->data_size) < 0))
    {
        ndsPreviewPackLoadHalt(13u, fkind);
    }
#endif
    *fighter->p_file_main = records[0]->data;
    *fighter->p_file_model = records[1]->data;
    gNdsPreviewPackLoadCount++;
    gNdsPreviewPackDataBytes += allocation;
    return 2; /* Newly loaded, so the source particle bank must be initialized. */
}

#if NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
static s32 ndsBattleForeignImagesValid(const NDSBattleForeignImageRow *rows,
                                       u32 count, u32 bytes)
{
    u32 i;
    if ((count == 0u) != (bytes == 0u)) { return FALSE; }
    for (i = 0u; i < count; i++)
    {
        const NDSBattleForeignImageRow *row = &rows[i];
        u32 j;
        if ((row->reserved != 0u) || (row->data_bytes == 0u) ||
            ((row->source_offset | row->data_offset | row->data_bytes) & 3u) ||
            !ndsPreviewRange(row->data_offset, row->data_bytes, bytes) ||
            (row->data_bytes > UINT32_MAX - row->source_offset)) { return FALSE; }
        for (j = 0u; j < i; j++)
        {
            const NDSBattleForeignImageRow *other = &rows[j];
            if ((row->asset_id == other->asset_id) &&
                (row->source_offset < other->source_offset + other->data_bytes) &&
                (other->source_offset < row->source_offset + row->data_bytes) &&
                ((row->source_offset != other->source_offset) ||
                 (row->data_offset != other->data_offset) ||
                 (row->data_bytes != other->data_bytes))) { return FALSE; }
            if ((row->data_offset < other->data_offset + other->data_bytes) &&
                (other->data_offset < row->data_offset + row->data_bytes) &&
                ((row->data_offset != other->data_offset) ||
                 (row->data_bytes != other->data_bytes))) { return FALSE; }
        }
    }
    return TRUE;
}

static __attribute__((noinline, noreturn)) void ndsBattleCoreExternHalt(s32 fkind)
{
    gNdsBattleCoreExternFailure++;
    ndsPreviewPackLoadHalt(14u, (u32)fkind);
}

s32 ndsRelocPatchCompactBattleMainExterns(s32 fkind)
{
    NDSBattleExternHeader header;
    NDSBattleExternRow rows[NDS_BATTLE_EXTERN_MAX];
    NDSRelocLoadedFile *main_loaded;
    NDSPreviewResident *resident;
    NDSBattleForeignImageRow *foreign_images = NULL;
    FILE *file;
    long file_bytes;
    u64 bank_bytes;
    u64 expected_bytes;
    u32 allocation;
    FTData *fighter;
    char path[] = "nitro:/fighters/battle/00.ext";
    const u32 digit_at = sizeof("nitro:/fighters/battle/") - 1u;
    u32 i;

    if (((u32)fkind >= ARRAY_COUNT(sNdsPreviewResidents)) ||
        (gNdsSceneManagerCurrIsBattle == 0u))
    {
        return FALSE;
    }
    fighter = dFTManagerDataFiles[fkind];
    if ((fighter == NULL) || (fighter->p_file_main == NULL) ||
        (*fighter->p_file_main == NULL))
    {
        ndsBattleCoreExternHalt(fkind);
    }
    main_loaded = ndsRelocFindLoadedFileByData(*fighter->p_file_main);
    if ((main_loaded == NULL) || (main_loaded->reserved[0] != (u8)(fkind + 1u)))
    {
        ndsBattleCoreExternHalt(fkind);
    }

    path[digit_at] = (char)('0' + ((u32)fkind / 10u));
    path[digit_at + 1u] = (char)('0' + ((u32)fkind % 10u));
    ndsFsLock();
    file = fopen(path, "rb");
    if ((file == NULL) || (fseek(file, 0, SEEK_END) != 0) ||
        ((file_bytes = ftell(file)) < 0) || (fseek(file, 0, SEEK_SET) != 0) ||
        (fread(&header, 1u, sizeof(header), file) != sizeof(header)) ||
        (header.magic != NDS_BATTLE_EXTERN_MAGIC) ||
        (header.version != NDS_BATTLE_EXTERN_VERSION) ||
        (header.count > NDS_BATTLE_EXTERN_MAX) || (header.reserved != 0u) ||
        ((header.foreign_bytes & 3u) != 0u))
    {
        ndsBattleCoreExternHalt(fkind);
    }
    bank_bytes = (u64)header.foreign_count * sizeof(*foreign_images) +
        header.foreign_bytes;
    expected_bytes = sizeof(header) + (u64)header.count * sizeof(rows[0]) + bank_bytes;
    if ((expected_bytes != (u64)file_bytes) || (bank_bytes > UINT32_MAX - 15u) ||
        (fread(rows, sizeof(rows[0]), header.count, file) != header.count))
    {
        ndsBattleCoreExternHalt(fkind);
    }
    allocation = ((u32)bank_bytes + 15u) & ~15u;
    if (allocation != 0u)
    {
        foreign_images = syTaskmanMalloc(allocation, 16u);
        if ((foreign_images == NULL) ||
            (fread(foreign_images, 1u, (u32)bank_bytes, file) != (u32)bank_bytes))
        {
            ndsBattleCoreExternHalt(fkind);
        }
    }
    if ((ndsPreviewHash(foreign_images, (u32)bank_bytes, 2166136261u) !=
         header.foreign_hash) ||
        !ndsBattleForeignImagesValid(foreign_images, header.foreign_count,
                                    header.foreign_bytes))
    {
        ndsBattleCoreExternHalt(fkind);
    }
    fclose(file);
    ndsFsUnlock();
    if (header.foreign_count != 0u)
    {
        u8 *foreign_data = (u8 *)(foreign_images + header.foreign_count);
        /* Match ordinary O2R/FPC word normalization AFTER checking source-byte
         * integrity. Texture readers consume this representation, not raw BE. */
        for (i = 0u; i < header.foreign_bytes; i += 4u)
        {
            ndsRelocWriteNative32(foreign_data + i, ndsRelocReadBe32(foreign_data + i));
        }
    }
    resident = &sNdsPreviewResidents[fkind];
    resident->foreign_images = foreign_images;
    resident->foreign_count = header.foreign_count;
    gNdsBattleCoreForeignImageBytes += allocation;
    gNdsBattleCoreForeignImageRows += header.foreign_count;
    if (header.foreign_count != 0u) { gNdsBattleCoreForeignImageLoadCount++; }

    for (i = 0u; i < header.count; i++)
    {
        NDSRelocLoadedFile *dep;
        s32 was_loaded;
        u32 main_offset;
        u32 dep_offset;

        if (ndsPreviewFileOffset(main_loaded, rows[i].slot, sizeof(void *),
                                 &main_offset) == FALSE)
        {
            ndsBattleCoreExternHalt(fkind);
        }
        dep = ndsRelocFindLoadedFileByAsset(rows[i].dep_asset);
        was_loaded = (dep != NULL) ? TRUE : FALSE;
        dep = ndsRelocEnsureLoadedAsset(rows[i].dep_asset);
        if (dep != NULL)
        {
            /* BattleShip's Main extern closure populates the STATUS buffer;
             * later ftmanager publication is deliberately a lookup-only
             * lbRelocGetStatusBufferFile().  EnsureLoadedAsset owns the actual
             * DS load/finalize but does not create that source residency alias,
             * so publish the same numeric asset key after a successful load. */
            ndsRelocAddStatusBufferFile(rows[i].dep_asset, dep->data);
            if (was_loaded == FALSE)
            {
                gNdsBattleCoreExternLoadCount++;
            }
        }
        if ((dep == NULL) ||
            (ndsRelocFindStatusNode(sNdsRelocStatusBuffer,
                                    sNdsRelocStatusBufferCount,
                                    rows[i].dep_asset) != dep->data) ||
            (ndsPreviewFileOffset(dep, rows[i].target_offset, 1u,
                                  &dep_offset) == FALSE))
        {
            ndsBattleCoreExternHalt(fkind);
        }
        ndsRelocWriteNativePointer((u8 *)main_loaded->data + main_offset,
                                   (u8 *)dep->data + dep_offset);
        gNdsBattleCoreExternPatchCount++;
    }
    return TRUE;
}
#endif

void ndsRelocReleasePreviewFighter(s32 fkind)
{
    u32 i = 0u;
    u8 owner;

    if ((fkind < 0) || ((u32)fkind >= ARRAY_COUNT(sNdsPreviewResidents)))
    {
        return;
    }
    owner = (u8)(fkind + 1);
    while (i < sNdsRelocLoadedFileCount)
    {
        NDSRelocLoadedFile *loaded = &sNdsRelocLoadedFiles[i];

        if (loaded->reserved[0] == owner)
        {
            void *data = loaded->data;
            u32 remaining;
            s32 status_i;

            ndsRelocForgetNormalizedWeaponAttrs(loaded->asset_id);
            status_i = 0;
            while (status_i < sNdsRelocStatusBufferCount)
            {
                if (sNdsRelocStatusBuffer[status_i].addr == data)
                {
                    ndsRelocRemoveStatusNodeAt(sNdsRelocStatusBuffer,
                                               &sNdsRelocStatusBufferCount,
                                               status_i);
                    continue;
                }
                status_i++;
            }
            status_i = 0;
            while (status_i < sNdsRelocForceStatusBufferCount)
            {
                if (sNdsRelocForceStatusBuffer[status_i].addr == data)
                {
                    ndsRelocRemoveStatusNodeAt(sNdsRelocForceStatusBuffer,
                                               &sNdsRelocForceStatusBufferCount,
                                               status_i);
                    continue;
                }
                status_i++;
            }
            remaining = (sNdsRelocLoadedFileCount - i) - 1u;
            if (remaining != 0u)
            {
                memmove(&sNdsRelocLoadedFiles[i],
                        &sNdsRelocLoadedFiles[i + 1u],
                        (size_t)remaining * sizeof(sNdsRelocLoadedFiles[0]));
            }
            sNdsRelocLoadedFileCount--;
            continue;
        }
        i++;
    }
    memset(&sNdsPreviewResidents[fkind], 0,
           sizeof(sNdsPreviewResidents[fkind]));
    sNdsRelocRelativeOffsetsMemo = NULL;
    sNdsRelocRelativeOffsetsMemoBase = NULL;
}

s32 ndsRelocLoadPreviewFighter(s32 fkind)
{
    s32 result;

    ndsFsLock();
    result = ndsRelocLoadPreviewFighterUnlocked(fkind);
    ndsFsUnlock();
    return result;
}
