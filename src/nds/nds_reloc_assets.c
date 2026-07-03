#include <filesystem.h>
#include <nds.h>
#include <stdio.h>
#include <string.h>

#include <nds/nds_reloc_assets.h>

#define NDS_O2R_RESOURCE_HEADER_SIZE 0x40
#define NDS_O2R_MAGIC_OFFSET 0x04
#define NDS_O2R_MAGIC "OLER"

typedef struct NDSRelocAssetEntry {
    u32 asset_id;
    u32 file_id;
    const char *path;
} NDSRelocAssetEntry;

volatile u32 gNdsRelocAssetInitResult;
volatile u32 gNdsRelocAssetHeaderReadCount;
volatile u32 gNdsRelocAssetPayloadReadCount;
volatile u32 gNdsRelocAssetOpenFailCount;
volatile u32 gNdsRelocAssetFormatFailCount;
volatile u32 gNdsRelocAssetShortReadCount;

static const NDSRelocAssetEntry sNdsRelocAssets[] = {
    { 0, 0, "nitro:/reloc/reloc_menus/MNCommon" },
    { 194, 194, "nitro:/reloc/reloc_misc_named/N64Logo" },
    { 52, 52, "nitro:/reloc/reloc_movies/MVCommon" },
    { 63, 63, "nitro:/reloc/reloc_transitions/MVOpeningRoomTransition" },
    { 56, 56, "nitro:/reloc/reloc_movies/MVOpeningRoomScene1" },
    { 57, 57, "nitro:/reloc/reloc_movies/MVOpeningRoomScene2" },
    { 58, 58, "nitro:/reloc/reloc_movies/MVOpeningRoomScene3" },
    { 59, 59, "nitro:/reloc/reloc_movies/MVOpeningRoomScene4" },
    { 75, 75, "nitro:/reloc/reloc_movies/MVOpeningRunCrash" },
    { 90, 90, "nitro:/reloc/reloc_movies/MVOpeningRoomWallpaper" },
    { 53, 53, "nitro:/reloc/reloc_movies/MVOpeningPortraitsSet1" },
    { 54, 54, "nitro:/reloc/reloc_movies/MVOpeningPortraitsSet2" },
    { 37, 37, "nitro:/reloc/reloc_interface/IFCommonAnnounceCommon" },
    { 65, 65, "nitro:/reloc/reloc_movies/MVOpeningCommon" },
    { 55, 55, "nitro:/reloc/reloc_movies/MVOpeningRun" },
    { 71, 71, "nitro:/reloc/reloc_movies/MVOpeningYamabuki" },
    { 73, 73, "nitro:/reloc/reloc_movies/MVOpeningSector" },
    { 167, 167, "nitro:/reloc/reloc_menus/MNTitle" },
    { 168, 168, "nitro:/reloc/reloc_menus/MNTitleFireAnim" },
    { 0xa6, 0xa6, "nitro:/reloc/reloc_interface/IFCommonPlayer" },
    { 0x52, 0x52, "nitro:/reloc/reloc_interface/IFCommonGameStatus" },
    { 0xa4, 0xa4, "nitro:/reloc/reloc_interface/IFCommonPlayerDamage" },
    { 0xa5, 0xa5, "nitro:/reloc/reloc_interface/IFCommonTimer" },
    { 0x24, 0x24, "nitro:/reloc/reloc_interface/IFCommonDigits" },
    { 0xc5, 0xc5, "nitro:/reloc/reloc_interface/IFCommonBattlePause" },
    { 0x26, 0x26, "nitro:/reloc/reloc_interface/IFCommonPlayerTags" },
    { 0xc7, 0xc7, "nitro:/reloc/reloc_misc_named/SYKseg1Validate" },
    { 6, 6, "nitro:/reloc/reloc_menus/MNVSMode" },
    { 0x11, 0x11, "nitro:/reloc/reloc_menus/MNPlayersCommon" },
    { 0x12, 0x12, "nitro:/reloc/reloc_menus/MNPlayersGameModes" },
    { 0x13, 0x13, "nitro:/reloc/reloc_menus/MNPlayersPortraits" },
    { 0x14, 0x14, "nitro:/reloc/reloc_fighters_common/FTEmblemSprites" },
    { 0x15, 0x15, "nitro:/reloc/reloc_menus/MNSelectCommon" },
    { 0x16, 0x16, "nitro:/reloc/reloc_menus/MNPlayersSpotlight" },
    { 0x1a, 0x1a, "nitro:/reloc/reloc_stages/GRWallpaperTrainingBlack" },
    { 0x1e, 0x1e, "nitro:/reloc/reloc_menus/MNMaps" },
    { 0x21, 0x21, "nitro:/reloc/reloc_menus/MNCommonFonts" },
    { 0xff, 0xff, "nitro:/reloc/reloc_stages/GRPupupuMap" },
    { 0x104, 0x104, "nitro:/reloc/reloc_stages/GRInishieMap" },
    { 0x109, 0x109, "nitro:/reloc/reloc_stages/GRHyruleMap" },
    { 0x1005f, 0x5f, "nitro:/reloc/reloc_stages/StageCastle" },
    { 0x10058, 0x58, "nitro:/reloc/reloc_stages/StageDreamLand" },
    { 0x71, 0x71, "nitro:/reloc/reloc_extern_data/ExternDataBank113" },
    { 0x67, 0x67, "nitro:/reloc/reloc_extern_data/ExternDataBank103" },
    { 0x68, 0x68, "nitro:/reloc/reloc_extern_data/ExternDataBank104" },
    { 0x98, 0x98, "nitro:/reloc/reloc_extern_data/MiscDataBank152" },
    { 0xa3, 0xa3, "nitro:/reloc/reloc_fighters_common/FTManagerCommon" },
    { 0xcb, 0xcb, "nitro:/reloc/reloc_fighters_main/MarioMain" },
    { 0xca, 0xca, "nitro:/reloc/reloc_fighters_main/MarioMainMotion" },
    { 0x128, 0x128, "nitro:/reloc/reloc_fighters_main/MarioModel" },
    { 0x12a, 0x12a, "nitro:/reloc/reloc_fighters_main/MarioShieldPose" },
    { 0xcc, 0xcc, "nitro:/reloc/reloc_fighters_main/MarioSpecial1" },
    { 0x164, 0x164, "nitro:/reloc/reloc_fighters_main/MarioSpecial2" },
    { 0x129, 0x129, "nitro:/reloc/reloc_fighters_main/MarioSpecial3" },
    { 0xd1, 0xd1, "nitro:/reloc/reloc_fighters_main/FoxMain" },
    { 0xd0, 0xd0, "nitro:/reloc/reloc_fighters_main/FoxMainMotion" },
    { 0x139, 0x139, "nitro:/reloc/reloc_fighters_main/FoxModel" },
    { 0x13a, 0x13a, "nitro:/reloc/reloc_fighters_main/FoxShieldPose" },
    { 0xd2, 0xd2, "nitro:/reloc/reloc_fighters_main/FoxSpecial1" },
    { 0x15a, 0x15a, "nitro:/reloc/reloc_fighters_main/FoxSpecial2" },
    { 0xa1, 0xa1, "nitro:/reloc/reloc_fighters_main/FoxSpecial3" },
    { 0x13c, 0x13c, "nitro:/reloc/reloc_fighters_main/FoxSpecial4" },
    { 0xc9, 0xc9, "nitro:/reloc/reloc_extern_data/MiscData201" },
    { 0x12b, 0x12b, "nitro:/reloc/reloc_extern_data/MiscData299" },
    { 0x13b, 0x13b, "nitro:/reloc/reloc_extern_data/MiscData315" },
    { 0x6d, 0x6d, "nitro:/reloc/reloc_extern_data/ExternDataBank109" },
    { 0x1f3, 0x1f3, "nitro:/reloc/reloc_animations/FTMarioAnimWait" },
    { 0x1f4, 0x1f4, "nitro:/reloc/reloc_animations/FTMarioAnim001" },
    { 0x1f5, 0x1f5, "nitro:/reloc/reloc_animations/FTMarioAnim002" },
    { 0x1f6, 0x1f6, "nitro:/reloc/reloc_animations/FTMarioAnim003" },
    { 0x1f7, 0x1f7, "nitro:/reloc/reloc_animations/FTMarioAnim004" },
    { 0x282, 0x282, "nitro:/reloc/reloc_animations/FTFoxAnim000" },
    { 0x283, 0x283, "nitro:/reloc/reloc_animations/FTFoxAnim001" },
    { 0x284, 0x284, "nitro:/reloc/reloc_animations/FTFoxAnim002" },
    { 0x285, 0x285, "nitro:/reloc/reloc_animations/FTFoxAnim003" },
    { 0x286, 0x286, "nitro:/reloc/reloc_animations/FTFoxAnim004" },
};

static u16 ndsReadLe16(const u8 *bytes)
{
    return (u16)bytes[0] | ((u16)bytes[1] << 8);
}

static u32 ndsReadLe32(const u8 *bytes)
{
    return (u32)bytes[0] |
           ((u32)bytes[1] << 8) |
           ((u32)bytes[2] << 16) |
           ((u32)bytes[3] << 24);
}

static const NDSRelocAssetEntry *ndsRelocAssetFindEntry(u32 asset_id)
{
    size_t i;

    for (i = 0; i < (sizeof(sNdsRelocAssets) / sizeof(sNdsRelocAssets[0])); i++)
    {
        if (sNdsRelocAssets[i].asset_id == asset_id)
        {
            return &sNdsRelocAssets[i];
        }
    }
    return NULL;
}

const char *ndsRelocAssetGetPath(u32 asset_id)
{
    const NDSRelocAssetEntry *entry = ndsRelocAssetFindEntry(asset_id);

    return (entry != NULL) ? entry->path : NULL;
}

void ndsRelocAssetsInit(void)
{
    if (nitroFSInit(NULL))
    {
        gNdsRelocAssetInitResult = NDS_RELOC_ASSET_INIT_PASS;
        iprintf("NitroFS: PASS\n");
    }
    else
    {
        gNdsRelocAssetInitResult = 0;
        iprintf("NitroFS: FAIL\n");
    }
}

s32 ndsRelocAssetReadHeaderFromFile(FILE *file, u32 expected_file_id,
                                            NDSRelocAssetHeader *out_header,
                                            long *out_data_offset)
{
    u8 header[NDS_O2R_RESOURCE_HEADER_SIZE + 16];
    u32 extern_count;
    long data_size_offset;
    long data_offset;

    if (fseek(file, 0, SEEK_SET) != 0)
    {
        gNdsRelocAssetShortReadCount++;
        return FALSE;
    }
    if (fread(header, 1, sizeof(header), file) != sizeof(header))
    {
        gNdsRelocAssetShortReadCount++;
        return FALSE;
    }
    if (memcmp(&header[NDS_O2R_MAGIC_OFFSET], NDS_O2R_MAGIC, 4) != 0)
    {
        gNdsRelocAssetFormatFailCount++;
        return FALSE;
    }

    out_header->file_id = ndsReadLe32(&header[NDS_O2R_RESOURCE_HEADER_SIZE]);
    out_header->reloc_intern_offset =
        ndsReadLe16(&header[NDS_O2R_RESOURCE_HEADER_SIZE + 4]);
    out_header->reloc_extern_offset =
        ndsReadLe16(&header[NDS_O2R_RESOURCE_HEADER_SIZE + 6]);
    extern_count = ndsReadLe32(&header[NDS_O2R_RESOURCE_HEADER_SIZE + 8]);
    out_header->extern_file_ids_num = extern_count;

    if (out_header->file_id != expected_file_id)
    {
        gNdsRelocAssetFormatFailCount++;
        return FALSE;
    }

    data_size_offset = NDS_O2R_RESOURCE_HEADER_SIZE + 12 + ((long)extern_count * 2);
    if (fseek(file, data_size_offset, SEEK_SET) != 0)
    {
        gNdsRelocAssetShortReadCount++;
        return FALSE;
    }
    if (fread(header, 1, 4, file) != 4)
    {
        gNdsRelocAssetShortReadCount++;
        return FALSE;
    }
    out_header->data_size = ndsReadLe32(header);

    data_offset = data_size_offset + 4;
    if (out_data_offset != NULL)
    {
        *out_data_offset = data_offset;
    }
    return TRUE;
}

s32 ndsRelocAssetReadHeader(u32 asset_id, NDSRelocAssetHeader *out_header)
{
    const NDSRelocAssetEntry *entry;
    FILE *file;
    long data_offset;
    s32 ok;

    if (out_header == NULL)
    {
        return FALSE;
    }

    entry = ndsRelocAssetFindEntry(asset_id);
    if (entry == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }

    file = fopen(entry->path, "rb");
    if (file == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }

    ok = ndsRelocAssetReadHeaderFromFile(file, entry->file_id, out_header, &data_offset);
    fclose(file);

    if (ok != FALSE)
    {
        gNdsRelocAssetHeaderReadCount++;
    }
    return ok;
}

s32 ndsRelocAssetReadExternFileIDs(u32 asset_id, u32 *out_file_ids,
                                   u32 capacity, u32 *out_count)
{
    const NDSRelocAssetEntry *entry;
    FILE *file;
    NDSRelocAssetHeader header;
    long data_offset;
    u32 i;

    if (out_count != NULL)
    {
        *out_count = 0;
    }
    if ((out_file_ids == NULL) && (capacity != 0))
    {
        return FALSE;
    }

    entry = ndsRelocAssetFindEntry(asset_id);
    if (entry == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }

    file = fopen(entry->path, "rb");
    if (file == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }

    if (ndsRelocAssetReadHeaderFromFile(file, entry->file_id, &header,
                                        &data_offset) == FALSE)
    {
        fclose(file);
        return FALSE;
    }
    if (header.extern_file_ids_num > capacity)
    {
        fclose(file);
        gNdsRelocAssetFormatFailCount++;
        return FALSE;
    }
    if (fseek(file, NDS_O2R_RESOURCE_HEADER_SIZE + 12, SEEK_SET) != 0)
    {
        fclose(file);
        gNdsRelocAssetShortReadCount++;
        return FALSE;
    }

    for (i = 0; i < header.extern_file_ids_num; i++)
    {
        u8 id_bytes[2];

        if (fread(id_bytes, 1, sizeof(id_bytes), file) != sizeof(id_bytes))
        {
            fclose(file);
            gNdsRelocAssetShortReadCount++;
            return FALSE;
        }
        out_file_ids[i] = ndsReadLe16(id_bytes);
    }
    fclose(file);

    if (out_count != NULL)
    {
        *out_count = header.extern_file_ids_num;
    }
    return TRUE;
}

s32 ndsRelocAssetLoadData(u32 asset_id, void *dst, size_t dst_capacity,
                           NDSRelocAssetHeader *out_header)
{
    const NDSRelocAssetEntry *entry;
    FILE *file;
    NDSRelocAssetHeader header;
    long data_offset;
    s32 ok;

    if (dst == NULL)
    {
        return FALSE;
    }

    entry = ndsRelocAssetFindEntry(asset_id);
    if (entry == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }

    file = fopen(entry->path, "rb");
    if (file == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }

    ok = ndsRelocAssetReadHeaderFromFile(file, entry->file_id, &header, &data_offset);
    if (ok == FALSE)
    {
        fclose(file);
        return FALSE;
    }
    if ((size_t)header.data_size > dst_capacity)
    {
        gNdsRelocAssetShortReadCount++;
        fclose(file);
        return FALSE;
    }
    if (fseek(file, data_offset, SEEK_SET) != 0)
    {
        gNdsRelocAssetShortReadCount++;
        fclose(file);
        return FALSE;
    }
    if (fread(dst, 1, header.data_size, file) != header.data_size)
    {
        gNdsRelocAssetShortReadCount++;
        fclose(file);
        return FALSE;
    }
    fclose(file);

    if (out_header != NULL)
    {
        *out_header = header;
    }
    gNdsRelocAssetPayloadReadCount++;
    return TRUE;
}
