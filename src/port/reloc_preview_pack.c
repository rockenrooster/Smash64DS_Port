/* Included by reloc_backend_assets.c: shares the scene-owned file registry and
 * its existing source-format normalizers. Never pages fighter data at runtime. */
#include <nds/nds_preview_pack.h>
#include <nds/nds_scene_manager.h>
#include <stdio.h>

u32 ndsRelocUseBattleCoreFighterData(void)
{
#if NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
    return (gNdsSceneManagerCurrIsBattle != 0u) ||
           (gSCManagerSceneData.scene_curr == nSCKindVSResults);
#else
    return FALSE;
#endif
}

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

const void *ndsRelocNativeRootAddress(const void *base, u32 root_offset)
{
    NDSRelocLoadedFile *loaded;
    const NDSPreviewPackSection *section;
    u32 i;

    if (base == NULL)
    {
        return NULL;
    }
    loaded = ndsRelocFindLoadedFileByData((void *)base);
    if ((loaded == NULL) || (loaded->reserved[0] == 0u))
    {
        return (const u8 *)base + root_offset;
    }
    section = ndsPreviewSection(loaded, NULL);
    if (section == NULL)
    {
        return NULL;
    }
    for (i = 0u; i < section->root_count; i++)
    {
        const u8 *cell = (const u8 *)base + section->roots_offset + (i * 8u);

        if ((ndsRelocReadNative32(cell) == 0xdf000000u) &&
            (ndsRelocReadNative32(cell + 4u) == root_offset))
        {
            return cell;
        }
    }
    return NULL;
}

const void *ndsRelocNativeAssetAddress(const void *base, u32 offset)
{
    NDSRelocLoadedFile *loaded;
    u32 mapped;
    if ((gSCManagerSceneData.scene_curr != nSCKind1PGamePlayers) &&
        (gSCManagerSceneData.scene_curr != nSCKindPlayersVS)
#if NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
        && (ndsRelocUseBattleCoreFighterData() == FALSE)
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

/* THE RENDERER'S DECLINE WITNESS, REPUBLISHED WHERE IT SURVIVES THE LINK.
 *
 * Reason 20 is "a packed preview reached the draw with no native owner", so
 * the renderer's own decline words are the entire explanation -- and they are
 * write-only diagnostics, which `--gc-sections` discards even with
 * `__attribute__((used))`: `used` keeps a symbol inside its object, not its
 * section through the link. On 2026-09-21 that made gdb resolve four freshly
 * added witness names to one stale word. Reading them here is what keeps them:
 * the read comes from surviving code, so the linker keeps the sections, and
 * the copy lands in an array this terminal function writes. */
extern volatile u32 gNdsFtrDeclineStage;
extern volatile u32 gNdsFtrDeclineDisplayListClause;
extern volatile u32 gNdsFtrDeclineDisplayListAsset;
extern volatile u32 gNdsFtrDeclineDisplayListExpected;
extern volatile u32 gNdsFtrDeclineDisplayListOwnerAsset;
__attribute__((used)) volatile u32 gNdsPreviewPackHaltDecline[5];

static __attribute__((noinline, noreturn)) void ndsPreviewPackLoadHalt(u32 reason, u32 kind)
{
    gNdsPreviewPackFailure = reason;
    gNdsPreviewPackFailureKind = kind;
    gNdsRelocAssetFormatFailCount++;
    gNdsPreviewPackHaltDecline[0] = gNdsFtrDeclineStage;
    gNdsPreviewPackHaltDecline[1] = gNdsFtrDeclineDisplayListClause;
    gNdsPreviewPackHaltDecline[2] = gNdsFtrDeclineDisplayListAsset;
    gNdsPreviewPackHaltDecline[3] = gNdsFtrDeclineDisplayListExpected;
    gNdsPreviewPackHaltDecline[4] = gNdsFtrDeclineDisplayListOwnerAsset;
    /* Everything a debugger wants at this halt was written in the frames just
     * before it and is still sitting dirty in the data cache; the spin below
     * never writes again, so those lines are never evicted and every witness
     * reads back as whatever was last in that line. That is how the renderer's
     * stage-2 decline witness came back as four copies of one stale word
     * (2026-09-21). The halt is terminal -- flushing once here costs nothing
     * and makes every counter that explains it readable. */
    DC_FlushAll();
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

/* --- M03: one compact loader, driven either to completion or one span at a
 * time -------------------------------------------------------------------
 *
 * The phases below are exactly the phases the single-call loader always ran;
 * only their driver changed. READ now hashes and byte-lane-normalizes each
 * chunk as it arrives instead of making three separate passes over the whole
 * payload, and FIXUP reads its 8-byte records in batches instead of one fread
 * per record (up to 385 of them for Captain).
 *
 * Nothing here publishes: a completed transaction sits in NDS_FPC_STATE_STAGED
 * until ndsRelocLoadPreviewFighter is called for that kind at the source's own
 * ftManagerSetupFilesAllKind boundary. */
enum {
    NDS_FPC_STATE_FREE = 0,
    NDS_FPC_STATE_READ,
    NDS_FPC_STATE_FIXUP,
    NDS_FPC_STATE_SPANS,
    NDS_FPC_STATE_REGISTER,
    NDS_FPC_STATE_STAGED
};

/* Four character-select blocks can each hold one in-flight transaction; the
 * fifth covers a synchronous 1P/battle load overlapping them. Static so a
 * caller's syMallocReset can never leave a live handle dangling in a reset
 * arena, and small enough (five 64-byte rows) not to matter against the
 * scene heap floor. */
#define NDS_FPC_LOAD_SLOTS 5u

typedef struct NDSPreviewPackLoad {
    FILE *file;
    u8 *data;
    NDSPreviewPackSection *sections;
    NDSPreviewPackSpan *spans;
    u32 allocation;
    u32 data_bytes;
    u32 fixup_count;
    u32 span_count;
    u32 data_hash;
    u32 fixup_hash;
    u32 span_hash;
    u32 model_source_bytes;
    u32 cursor;
    u32 hash;
    u32 generation;
    s16 fkind;
    u8 section_count;
    u8 is_battle_pack;
    u8 state;
} NDSPreviewPackLoad;

static NDSPreviewPackLoad sNdsPreviewPackLoads[NDS_FPC_LOAD_SLOTS];
volatile u32 gNdsPreviewPackStepCount;
volatile u32 gNdsPreviewPackStepByteMax;
volatile u32 gNdsPreviewPackStageCommitCount;
volatile u32 gNdsPreviewPackStageCancelCount;

static void ndsPreviewPackLoadRelease(NDSPreviewPackLoad *load)
{
    if (load->file != NULL)
    {
        ndsFsLock();
        fclose(load->file);
        ndsFsUnlock();
        load->file = NULL;
    }
}

static void ndsPreviewPackLoadFree(NDSPreviewPackLoad *load)
{
    ndsPreviewPackLoadRelease(load);
    load->state = NDS_FPC_STATE_FREE;
    load->data = NULL;
    load->fkind = -1;
}

void *ndsRelocPreviewFighterLoadBegin(s32 fkind)
{
    NDSPreviewPackLoad *load = NULL;
    NDSPreviewPackHeader header;
    NDSPreviewPackSection sections[NDS_PREVIEW_PACK_MAX_SECTIONS];
    FTData *fighter;
    FILE *file;
    char preview_path[] = "nitro:/fighters/preview/00.fpc";
#if NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
    char battle_path[] = "nitro:/fighters/battle/00.fpc";
#endif
    char *path = preview_path;
    u32 digit_at = sizeof("nitro:/fighters/preview/") - 1u;
    s32 is_battle_pack = FALSE;
    long file_size;
    u64 expected_size;
    u32 i;

#if !NDS_RENDERER_HW_TRIANGLES || (NDS_RENDERER_PROFILE_LEVEL >= 2)
    /* The source/oracle renderer still consumes full Gfx/Vtx programs. */
    return NULL;
#endif
    if (((gSCManagerSceneData.scene_curr != nSCKind1PGamePlayers) &&
         (gSCManagerSceneData.scene_curr != nSCKindPlayersVS)
#if NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
         && (ndsRelocUseBattleCoreFighterData() == FALSE)
#endif
        ) ||
        ((u32)fkind >= ARRAY_COUNT(sNdsPreviewResidents))) { return NULL; }
    fighter = dFTManagerDataFiles[fkind];
    if ((fighter == NULL) || (fighter->p_file_main == NULL) ||
        (fighter->p_file_model == NULL)) { ndsPreviewPackLoadHalt(1u, fkind); }

    /* One kind is loaded once. A second request for a kind already in flight
     * resumes that transaction rather than opening a rival one against the
     * same asset ids -- which REGISTER would halt on (reason 9). Scan for that
     * before claiming a free row; a stale-generation row is not a match. */
    for (i = 0u; i < NDS_FPC_LOAD_SLOTS; i++)
    {
        if ((sNdsPreviewPackLoads[i].state != NDS_FPC_STATE_FREE) &&
            (sNdsPreviewPackLoads[i].fkind == (s16)fkind))
        {
            if (sNdsPreviewPackLoads[i].generation == sNdsRelocSceneGeneration)
            {
                return &sNdsPreviewPackLoads[i];
            }
            ndsPreviewPackLoadFree(&sNdsPreviewPackLoads[i]);
        }
    }
    for (i = 0u; i < NDS_FPC_LOAD_SLOTS; i++)
    {
        if (sNdsPreviewPackLoads[i].state == NDS_FPC_STATE_FREE)
        {
            load = &sNdsPreviewPackLoads[i];
            break;
        }
    }
    if (load == NULL) { return NULL; }
    memset(load, 0, sizeof(*load));
    load->fkind = (s16)fkind;
    load->generation = sNdsRelocSceneGeneration;
    if (*fighter->p_file_main != NULL)
    {
        /* Already resident. Stage an empty transaction so the owner boundary
         * still consumes exactly one handle and answers 1, as before. */
        load->state = NDS_FPC_STATE_STAGED;
        return load;
    }

    /* The roster index is two decimal digits. Pulling in snprintf here
     * retained newlib's floating-point formatter for this integer-only path. */
#if NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
    if (ndsRelocUseBattleCoreFighterData() != FALSE)
    {
        path = battle_path;
        digit_at = sizeof("nitro:/fighters/battle/") - 1u;
        is_battle_pack = TRUE;
    }
#endif
    path[digit_at] = '0' + (u32)fkind / 10u;
    path[digit_at + 1u] = '0' + (u32)fkind % 10u;
    ndsFsLock();
    file = fopen(path, "rb");
    if (file == NULL) { ndsFsUnlock(); ndsPreviewPackLoadHalt(2u, fkind); }
    if (fseek(file, 0, SEEK_END) != 0) { ndsPreviewPackLoadHalt(3u, fkind); }
    file_size = ftell(file);
    if ((file_size < (long)sizeof(header)) || (fseek(file, 0, SEEK_SET) != 0) ||
        (fread(&header, 1u, sizeof(header), file) != sizeof(header)))
    {
        ndsPreviewPackLoadHalt(3u, fkind);
    }
    expected_size = sizeof(header) + (u64)header.section_count * sizeof(sections[0]) +
        header.data_bytes + (u64)header.fixup_count * sizeof(NDSPreviewPackFixup) +
        (u64)header.span_count * sizeof(NDSPreviewPackSpan);
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
    load->allocation = header.data_bytes +
        header.section_count * sizeof(sections[0]) +
        header.span_count * sizeof(NDSPreviewPackSpan);
    load->data = syTaskmanMalloc(load->allocation, 16u);
    if (load->data == NULL) { ndsPreviewPackLoadHalt(5u, fkind); }
    ndsFsUnlock();
    load->file = file;
    load->data_bytes = header.data_bytes;
    load->fixup_count = header.fixup_count;
    load->span_count = header.span_count;
    load->data_hash = header.data_hash;
    load->fixup_hash = header.fixup_hash;
    load->span_hash = header.span_hash;
    load->model_source_bytes = header.model_source_bytes;
    load->section_count = (u8)header.section_count;
    load->is_battle_pack = (u8)((is_battle_pack != FALSE) ? 1u : 0u);
    load->sections = (NDSPreviewPackSection *)(load->data + header.data_bytes);
    memcpy(load->sections, sections, header.section_count * sizeof(sections[0]));
    load->spans = (NDSPreviewPackSpan *)(load->sections + header.section_count);
    load->cursor = 0u;
    load->hash = 2166136261u;
    load->state = NDS_FPC_STATE_READ;
    return load;
}

void ndsRelocPreviewFighterLoadCancel(void *handle, s32 fkind)
{
    NDSPreviewPackLoad *load = handle;

    if ((load == NULL) || (load->state == NDS_FPC_STATE_FREE) ||
        (load->fkind != (s16)fkind))
    {
        return;
    }
    gNdsPreviewPackStageCancelCount++;
    /* The data allocation belongs to the caller's resettable arena, and the
     * loaded-file records (if REGISTER already ran) belong to the caller's
     * ndsRelocReleasePreviewFighter. Only the file handle and the staging are
     * ours to drop. */
    ndsPreviewPackLoadFree(load);
}

s32 ndsRelocPreviewFighterLoadIsStaged(s32 fkind)
{
    u32 i;

    for (i = 0u; i < NDS_FPC_LOAD_SLOTS; i++)
    {
        if ((sNdsPreviewPackLoads[i].state == NDS_FPC_STATE_STAGED) &&
            (sNdsPreviewPackLoads[i].fkind == (s16)fkind) &&
            (sNdsPreviewPackLoads[i].generation == sNdsRelocSceneGeneration))
        {
            return TRUE;
        }
    }
    return FALSE;
}

/* One bounded unit of the transaction. `byte_budget` is the largest read/decode
 * span this call may move; 0 means "no bound", which is what the synchronous
 * 1P/battle entry point uses. */
s32 ndsRelocPreviewFighterLoadStep(void *handle, u32 byte_budget, u32 *out_bytes)
{
    NDSPreviewPackLoad *load = handle;
    NDSPreviewPackFixup batch[32];
    NDSPreviewResident *resident;
    NDSRelocAssetHeader reloc_header;
    NDSRelocLoadedFile *records[NDS_PREVIEW_PACK_MAX_SECTIONS];
    s32 fkind;
    u32 moved = 0u;
    u32 chunk;
    u32 i;

    if (out_bytes != NULL) { *out_bytes = 0u; }
    if (load == NULL) { return NDS_PREVIEW_PACK_STEP_FAIL; }
    fkind = load->fkind;
    if (load->state == NDS_FPC_STATE_STAGED)
    {
        return NDS_PREVIEW_PACK_STEP_DONE;
    }
    if ((load->state == NDS_FPC_STATE_FREE) ||
        (load->generation != sNdsRelocSceneGeneration))
    {
        return NDS_PREVIEW_PACK_STEP_FAIL;
    }
    gNdsPreviewPackStepCount++;

    switch (load->state)
    {
    case NDS_FPC_STATE_READ:
        /* Read, hash and byte-lane normalize one chunk. The hash must see the
         * raw big-endian bytes, so it runs before the swap on the same chunk
         * rather than as a second pass over the whole payload. */
        chunk = load->data_bytes - load->cursor;
        if ((byte_budget != 0u) && (chunk > byte_budget))
        {
            chunk = byte_budget & ~3u;
            if (chunk == 0u) { chunk = 4u; }
        }
        ndsFsLock();
        if (fread(load->data + load->cursor, 1u, chunk, load->file) != chunk)
        {
            ndsFsUnlock();
            ndsPreviewPackLoadHalt(5u, (u32)fkind);
        }
        ndsFsUnlock();
        load->hash = ndsPreviewHash(load->data + load->cursor, chunk, load->hash);
        for (i = 0u; i < chunk; i += 4u)
        {
            u8 *word = load->data + load->cursor + i;

            ndsRelocWriteNative32(word, ndsRelocReadBe32(word));
        }
        load->cursor += chunk;
        moved = chunk;
        if (load->cursor == load->data_bytes)
        {
            if (load->hash != load->data_hash)
            {
                ndsPreviewPackLoadHalt(5u, (u32)fkind);
            }
            load->cursor = 0u;
            load->hash = 2166136261u;
            load->state = NDS_FPC_STATE_FIXUP;
        }
        break;

    case NDS_FPC_STATE_FIXUP:
        chunk = load->fixup_count - load->cursor;
        if (chunk > ARRAY_COUNT(batch)) { chunk = ARRAY_COUNT(batch); }
        if ((byte_budget != 0u) && (chunk * sizeof(batch[0]) > byte_budget))
        {
            chunk = byte_budget / sizeof(batch[0]);
            if (chunk == 0u) { chunk = 1u; }
        }
        ndsFsLock();
        if (fread(batch, sizeof(batch[0]), chunk, load->file) != chunk)
        {
            ndsFsUnlock();
            ndsPreviewPackLoadHalt(6u, (u32)fkind);
        }
        ndsFsUnlock();
        for (i = 0u; i < chunk; i++)
        {
            const NDSPreviewPackFixup *pair = &batch[i];

            if ((pair->slot_offset & 3u) ||
                !ndsPreviewSectionContains(load->sections, load->section_count,
                                           pair->slot_offset, 4u) ||
                ((pair->target_offset != NDS_PREVIEW_PACK_NULL) &&
                 !ndsPreviewSectionContains(load->sections, load->section_count,
                                            pair->target_offset, 1u)))
            {
                ndsPreviewPackLoadHalt(6u, (u32)fkind);
            }
            load->hash = ndsPreviewHash(pair, sizeof(*pair), load->hash);
            ndsRelocWriteNativePointer(load->data + pair->slot_offset,
                (pair->target_offset == NDS_PREVIEW_PACK_NULL) ?
                    NULL : load->data + pair->target_offset);
        }
        load->cursor += chunk;
        moved = chunk * sizeof(batch[0]);
        if (load->cursor == load->fixup_count)
        {
            if (load->hash != load->fixup_hash)
            {
                ndsPreviewPackLoadHalt(6u, (u32)fkind);
            }
            load->state = NDS_FPC_STATE_SPANS;
        }
        break;

    case NDS_FPC_STATE_SPANS:
        /* At most 27 rows in the shipped packs: one unbounded unit of 324 B. */
        ndsFsLock();
        if (fread(load->spans, sizeof(load->spans[0]), load->span_count,
                  load->file) != load->span_count)
        {
            ndsFsUnlock();
            ndsPreviewPackLoadHalt(7u, (u32)fkind);
        }
        ndsFsUnlock();
        if (ndsPreviewHash(load->spans,
                           load->span_count * sizeof(load->spans[0]),
                           2166136261u) != load->span_hash)
        {
            ndsPreviewPackLoadHalt(7u, (u32)fkind);
        }
        ndsPreviewPackLoadRelease(load);
        moved = load->span_count * sizeof(load->spans[0]);
        load->state = NDS_FPC_STATE_REGISTER;
        break;

    case NDS_FPC_STATE_REGISTER:
        /* Publishing the resident record is what makes the compact span map
         * readable, so it happens here and not one step earlier: a transaction
         * cancelled before REGISTER leaves no resident and no loaded-file row
         * behind at all. */
        resident = &sNdsPreviewResidents[fkind];
        resident->model_source_bytes = load->model_source_bytes;
#if NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
        resident->foreign_images = NULL;
        resident->foreign_count = 0u;
#endif
        resident->sections = load->sections;
        resident->spans = load->spans;
        resident->generation = sNdsRelocSceneGeneration;
        for (i = 0u; i < load->section_count; i++)
        {
            const NDSPreviewPackSection *s = &load->sections[i];
            u32 j;

            for (j = 0u; j < s->span_count; j++)
            {
                const NDSPreviewPackSpan *span = &load->spans[s->first_span + j];

                if (!ndsPreviewRange(span->source_offset, span->data_bytes,
                                     s->source_bytes) ||
                    !ndsPreviewRange(span->data_offset, span->data_bytes,
                                     s->data_bytes))
                {
                    ndsPreviewPackLoadHalt(8u, (u32)fkind);
                }
            }
            if (ndsRelocFindLoadedFileByAsset(s->asset_id) != NULL)
            {
                ndsPreviewPackLoadHalt(9u, (u32)fkind);
            }
            memset(&reloc_header, 0, sizeof(reloc_header));
            reloc_header.file_id = s->asset_id;
            reloc_header.data_size = s->data_bytes;
            reloc_header.reloc_intern_offset = reloc_header.reloc_extern_offset = 0xffffu;
            records[i] = ndsRelocRegisterLoadedFile(s->asset_id, 0u,
                                                    load->data + s->data_offset,
                                                    &reloc_header);
            if (records[i] == NULL) { ndsPreviewPackLoadHalt(10u, (u32)fkind); }
            records[i]->internal_fixups_applied = TRUE;
            records[i]->external_fixups_applied = TRUE;
            records[i]->reserved[0] = (u8)(fkind + 1);
            records[i]->reserved[1] = (u8)i;
        }
        if (ndsRelocNormalizeFighterAttributesFile(records[0]) == FALSE)
        {
            ndsPreviewPackLoadHalt(11u, (u32)fkind);
        }
        for (i = 0u; i < load->section_count; i++)
        {
            if (ndsRelocNormalizeBattleInterfaceSprites(records[i]) == FALSE)
            {
                ndsPreviewPackLoadHalt(12u, (u32)fkind);
            }
        }
        /* Main is byte-for-byte source-sized in FPC1, but generic preview
         * packing deliberately NULLs dependencies outside the compact sections.
         * For the migrated P2-2 fighters those nine ShieldPose externs are not
         * optional: the native guard package replaces them. Repoint the
         * generated source slots only after all generic normalization is
         * complete so no later byte-lane pass can reinterpret a native
         * pointer. */
#if NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
        if ((load->is_battle_pack != 0u) &&
            (ndsShieldPosePatchCompactMain(
                fkind, records[0]->data, records[0]->data_size) < 0))
        {
            ndsPreviewPackLoadHalt(13u, (u32)fkind);
        }
#endif
        load->state = NDS_FPC_STATE_STAGED;
        break;

    default:
        return NDS_PREVIEW_PACK_STEP_FAIL;
    }

    if (out_bytes != NULL) { *out_bytes = moved; }
    if (moved > gNdsPreviewPackStepByteMax)
    {
        gNdsPreviewPackStepByteMax = moved;
    }
    return (load->state == NDS_FPC_STATE_STAGED) ?
        NDS_PREVIEW_PACK_STEP_DONE : NDS_PREVIEW_PACK_STEP_IN_PROGRESS;
}

/* The owner boundary. `*p_file_main` becomes non-NULL exactly here, with the
 * whole closure already relocated, registered and normalized. */
static s32 ndsPreviewPackLoadPublish(NDSPreviewPackLoad *load)
{
    FTData *fighter = dFTManagerDataFiles[load->fkind];
    s32 fkind = load->fkind;
    u32 allocation = load->allocation;
    u8 *data = load->data;

    if (data == NULL)
    {
        /* The "already resident" staging. */
        ndsPreviewPackLoadFree(load);
        return TRUE;
    }
    if ((fighter == NULL) || (fighter->p_file_main == NULL) ||
        (fighter->p_file_model == NULL))
    {
        ndsPreviewPackLoadHalt(1u, (u32)fkind);
    }
    *fighter->p_file_main =
        (u8 *)data + load->sections[0].data_offset;
    *fighter->p_file_model =
        (u8 *)data + load->sections[1].data_offset;
    ndsPreviewPackLoadFree(load);
    gNdsPreviewPackStageCommitCount++;
    gNdsPreviewPackLoadCount++;
    gNdsPreviewPackDataBytes += allocation;
    return 2; /* Newly loaded, so the source particle bank must be initialized. */
}

static s32 ndsRelocLoadPreviewFighterUnlocked(s32 fkind)
{
    NDSPreviewPackLoad *load;
    s32 step;
    u32 i;

    /* A character-select transaction already advanced this kind one bounded
     * span per update; consume it instead of reloading the pack. A staging
     * from a previous scene generation is not a completion for this one: drop
     * it and load normally rather than publishing into a rewound arena. */
    for (i = 0u; i < NDS_FPC_LOAD_SLOTS; i++)
    {
        if ((sNdsPreviewPackLoads[i].state == NDS_FPC_STATE_STAGED) &&
            (sNdsPreviewPackLoads[i].fkind == (s16)fkind))
        {
            if (sNdsPreviewPackLoads[i].generation == sNdsRelocSceneGeneration)
            {
                return ndsPreviewPackLoadPublish(&sNdsPreviewPackLoads[i]);
            }
            ndsPreviewPackLoadFree(&sNdsPreviewPackLoads[i]);
        }
    }
    load = ndsRelocPreviewFighterLoadBegin(fkind);
    if (load == NULL) { return FALSE; }
    do
    {
        step = ndsRelocPreviewFighterLoadStep(load, 0u, NULL);
    } while (step == NDS_PREVIEW_PACK_STEP_IN_PROGRESS);
    if (step != NDS_PREVIEW_PACK_STEP_DONE)
    {
        ndsRelocPreviewFighterLoadCancel(load, fkind);
        return FALSE;
    }
    return ndsPreviewPackLoadPublish(load);
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
        (ndsRelocUseBattleCoreFighterData() == FALSE))
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
    /* Every retire and cancel path in the character-select transaction reaches
     * here, so this is the one place that cannot be skipped: a handle whose
     * data allocation is about to be reset must not stay staged, or the next
     * acquire of this kind would publish into a freed arena. */
    for (i = 0u; i < NDS_FPC_LOAD_SLOTS; i++)
    {
        if ((sNdsPreviewPackLoads[i].state != NDS_FPC_STATE_FREE) &&
            (sNdsPreviewPackLoads[i].fkind == (s16)fkind))
        {
            ndsPreviewPackLoadFree(&sNdsPreviewPackLoads[i]);
        }
    }
    i = 0u;
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
