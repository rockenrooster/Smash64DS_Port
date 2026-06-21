#include <filesystem.h>
#include <nds.h>
#include <stdio.h>
#include <string.h>

#include <nds/nds_reloc_assets.h>

#define NDS_O2R_RESOURCE_HEADER_SIZE 0x40
#define NDS_O2R_MAGIC_OFFSET 0x04
#define NDS_O2R_MAGIC "OLER"

typedef struct NDSRelocAssetEntry {
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
    { 194, "nitro:/reloc/reloc_misc_named/N64Logo" },
    { 52, "nitro:/reloc/reloc_movies/MVCommon" },
    { 63, "nitro:/reloc/reloc_transitions/MVOpeningRoomTransition" },
    { 56, "nitro:/reloc/reloc_movies/MVOpeningRoomScene1" },
    { 57, "nitro:/reloc/reloc_movies/MVOpeningRoomScene2" },
    { 58, "nitro:/reloc/reloc_movies/MVOpeningRoomScene3" },
    { 59, "nitro:/reloc/reloc_movies/MVOpeningRoomScene4" },
    { 75, "nitro:/reloc/reloc_movies/MVOpeningRunCrash" },
    { 90, "nitro:/reloc/reloc_movies/MVOpeningRoomWallpaper" },
    { 53, "nitro:/reloc/reloc_movies/MVOpeningPortraitsSet1" },
    { 54, "nitro:/reloc/reloc_movies/MVOpeningPortraitsSet2" },
    { 37, "nitro:/reloc/reloc_interface/IFCommonAnnounceCommon" },
    { 65, "nitro:/reloc/reloc_movies/MVOpeningCommon" },
    { 55, "nitro:/reloc/reloc_movies/MVOpeningRun" },
    { 71, "nitro:/reloc/reloc_movies/MVOpeningYamabuki" },
    { 73, "nitro:/reloc/reloc_movies/MVOpeningSector" },
    { 167, "nitro:/reloc/reloc_menus/MNTitle" },
    { 168, "nitro:/reloc/reloc_menus/MNTitleFireAnim" },
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

const char *ndsRelocAssetGetPath(u32 file_id)
{
    size_t i;

    for (i = 0; i < (sizeof(sNdsRelocAssets) / sizeof(sNdsRelocAssets[0])); i++)
    {
        if (sNdsRelocAssets[i].file_id == file_id)
        {
            return sNdsRelocAssets[i].path;
        }
    }
    return NULL;
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

s32 ndsRelocAssetReadHeader(u32 file_id, NDSRelocAssetHeader *out_header)
{
    const char *path;
    FILE *file;
    long data_offset;
    s32 ok;

    if (out_header == NULL)
    {
        return FALSE;
    }

    path = ndsRelocAssetGetPath(file_id);
    if (path == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }

    file = fopen(path, "rb");
    if (file == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }

    ok = ndsRelocAssetReadHeaderFromFile(file, file_id, out_header, &data_offset);
    fclose(file);

    if (ok != FALSE)
    {
        gNdsRelocAssetHeaderReadCount++;
    }
    return ok;
}

s32 ndsRelocAssetLoadData(u32 file_id, void *dst, size_t dst_capacity,
                           NDSRelocAssetHeader *out_header)
{
    const char *path;
    FILE *file;
    NDSRelocAssetHeader header;
    long data_offset;
    s32 ok;

    if (dst == NULL)
    {
        return FALSE;
    }

    path = ndsRelocAssetGetPath(file_id);
    if (path == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }

    file = fopen(path, "rb");
    if (file == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }

    ok = ndsRelocAssetReadHeaderFromFile(file, file_id, &header, &data_offset);
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
