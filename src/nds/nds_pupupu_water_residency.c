#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nds/arm9/video.h>
#include <nds/arm9/videoGL.h>
#include <nds/nds_pupupu_water_residency.h>
#include <nds/pupupu_water_tiled_aot.h>

#define NDS_PUPUPU_WATER_PAYLOAD_FNV1A32 0x11bdfe52u

volatile u32 gNdsPupupuWaterResidencyPrepared;
volatile u32 gNdsPupupuWaterResidencyPrepareCount;
volatile u32 gNdsPupupuWaterResidencyPrepareSuccessCount;
volatile u32 gNdsPupupuWaterResidencyPrepareFailCount;
volatile u32 gNdsPupupuWaterResidencyResetCount;
volatile u32 gNdsPupupuWaterResidencyPayloadBytes;
volatile u32 gNdsPupupuWaterResidencyTextureBytes;
volatile u32 gNdsPupupuWaterResidencyPaletteBytes;
volatile u32 gNdsPupupuWaterResidencyTextureCount;
volatile u32 gNdsPupupuWaterResidencyPaletteCount;
volatile u32 gNdsPupupuWaterResidencyScratchBytes;
volatile u32 gNdsPupupuWaterResidencyPrimaryTextureAddress;
volatile u32 gNdsPupupuWaterResidencySecondaryTextureAddress;
volatile u32 gNdsPupupuWaterResidencyPaletteFirstByte;
volatile u32 gNdsPupupuWaterResidencyPaletteEndByte;
volatile u32 gNdsPupupuWaterResidencyPrimaryBindCount;
volatile u32 gNdsPupupuWaterResidencySecondaryBindCount;
volatile u32 gNdsPupupuWaterResidencyBindFailCount;
volatile u32 gNdsPupupuWaterResidencyLastFailure;

#if NDS_RENDERER_M4_WATER_TILED_AOT

#define NDS_PUPUPU_WATER_TEXTURE_VRAM_BASE 0x06800000u
#define NDS_PUPUPU_WATER_TEXTURE_VRAM_END 0x06840000u
#define NDS_PUPUPU_WATER_TEXTURE_BANK_BYTES (128u * 1024u)
#define NDS_PUPUPU_WATER_PRIMARY_TEXTURE 0u
#define NDS_PUPUPU_WATER_SECONDARY_TEXTURE 1u
#define NDS_PUPUPU_WATER_PALETTE_FIRST 2u

/* This libnds helper is used by the inspected sm64-nds texture uploader. It
 * translates a texture-slot address to the physical bank that owns it. */
extern u16 *vramGetBank(u16 *address);

static int sNdsPupupuWaterResourceNames[
    NDS_PUPUPU_WATER_RESIDENCY_RESOURCE_COUNT];
static u32 sNdsPupupuWaterResourceNameCount;

static void ndsPupupuWaterClearResidentState(void)
{
    gNdsPupupuWaterResidencyPrepared = FALSE;
    gNdsPupupuWaterResidencyTextureBytes = 0u;
    gNdsPupupuWaterResidencyPaletteBytes = 0u;
    gNdsPupupuWaterResidencyTextureCount = 0u;
    gNdsPupupuWaterResidencyPaletteCount = 0u;
    gNdsPupupuWaterResidencyPrimaryTextureAddress = 0u;
    gNdsPupupuWaterResidencySecondaryTextureAddress = 0u;
    gNdsPupupuWaterResidencyPaletteFirstByte = 0u;
    gNdsPupupuWaterResidencyPaletteEndByte = 0u;
}

static void ndsPupupuWaterReleaseResources(void)
{
    ndsPupupuWaterClearResidentState();
    if (sNdsPupupuWaterResourceNameCount != 0u)
    {
        glDeleteTextures((int)sNdsPupupuWaterResourceNameCount,
                         sNdsPupupuWaterResourceNames);
        memset(sNdsPupupuWaterResourceNames, 0,
               sizeof(sNdsPupupuWaterResourceNames));
        sNdsPupupuWaterResourceNameCount = 0u;
    }
}

static s32 ndsPupupuWaterTextureRangeValid(const void *address, u32 bytes)
{
    uintptr_t first = (uintptr_t)address;

    return ((first >= NDS_PUPUPU_WATER_TEXTURE_VRAM_BASE) &&
            (first <= NDS_PUPUPU_WATER_TEXTURE_VRAM_END) &&
            (bytes <= (NDS_PUPUPU_WATER_TEXTURE_VRAM_END - first))) ?
        TRUE : FALSE;
}

static s32 ndsPupupuWaterUpload(void *destination, const u8 *source,
                                u32 bytes)
{
    uintptr_t target = (uintptr_t)destination;

    while (bytes != 0u)
    {
        u16 *bank = vramGetBank((u16 *)target);
        u32 bank_offset = (u32)(target &
            (NDS_PUPUPU_WATER_TEXTURE_BANK_BYTES - 1u));
        u32 copy_bytes = NDS_PUPUPU_WATER_TEXTURE_BANK_BYTES - bank_offset;
        u32 saved_banks;

        if (copy_bytes > bytes)
        {
            copy_bytes = bytes;
        }
        saved_banks = VRAM_CR;
        if (bank == VRAM_A)
        {
            vramSetBankA(VRAM_A_LCD);
        }
        else if (bank == VRAM_B)
        {
            vramSetBankB(VRAM_B_LCD);
        }
        else
        {
            return FALSE;
        }
        memcpy((u8 *)bank + bank_offset, source, copy_bytes);
        vramRestorePrimaryBanks(saved_banks);
        target += copy_bytes;
        source += copy_bytes;
        bytes -= copy_bytes;
    }
    return TRUE;
}

static u32 ndsPupupuWaterRead(FILE *file, u8 *scratch, u32 bytes,
                              void *texture_destination, u32 *hash)
{
    u8 *destination = (u8 *)texture_destination;

    while (bytes != 0u)
    {
        u32 chunk = (bytes < NDS_PUPUPU_WATER_RESIDENCY_SCRATCH_BYTES) ?
            bytes : NDS_PUPUPU_WATER_RESIDENCY_SCRATCH_BYTES;
        u32 index;

        if (fread(scratch, 1u, chunk, file) != chunk)
        {
            return nNDSPupupuWaterResidencyFailureRead;
        }
        for (index = 0u; index < chunk; index++)
        {
            *hash = (*hash ^ scratch[index]) * 0x01000193u;
        }
        gNdsPupupuWaterResidencyPayloadBytes += chunk;
        if ((destination != NULL) &&
            (ndsPupupuWaterUpload(destination, scratch, chunk) == FALSE))
        {
            return nNDSPupupuWaterResidencyFailureTextureBank;
        }
        if (destination != NULL)
        {
            destination += chunk;
        }
        bytes -= chunk;
    }
    return nNDSPupupuWaterResidencyFailureNone;
}

static void *ndsPupupuWaterAllocateTexture(int name,
                                            GL_TEXTURE_TYPE_ENUM type,
                                            int size_x, int size_y,
                                            int parameters, u32 bytes)
{
    void *address;

    glBindTexture(GL_TEXTURE_2D, name);
    if (glTexImage2D(GL_TEXTURE_2D, 0, type, size_x, size_y, 0,
                     parameters, NULL) == 0)
    {
        return NULL;
    }
    address = glGetTexturePointer(name);
    return (ndsPupupuWaterTextureRangeValid(address, bytes) != FALSE) ?
        address : NULL;
}

static s32 ndsPupupuWaterCreatePalette(int name, const u16 *palette,
                                        u32 palette_index)
{
    int width = -1;
    int address = -1;

    glBindTexture(GL_TEXTURE_2D, name);
    glColorTableEXT(GL_TEXTURE_2D, 0,
                    NDS_PUPUPU_WATER_TILED_PALETTE_ENTRIES,
                    0, 0, palette);
    glGetColorTableParameterEXT(GL_TEXTURE_2D,
                                GL_COLOR_TABLE_WIDTH_EXT, &width);
    glGetColorTableParameterEXT(GL_TEXTURE_2D,
                                GL_COLOR_TABLE_FORMAT_EXT, &address);
    return ((width == NDS_PUPUPU_WATER_TILED_PALETTE_ENTRIES) &&
            (address >= 0) &&
            ((u32)address * 16u == palette_index * 512u)) ? TRUE : FALSE;
}

static s32 ndsPupupuWaterGenerateResourceNames(void)
{
    u32 index;

    for (index = 0u; index < NDS_PUPUPU_WATER_RESIDENCY_RESOURCE_COUNT;
         index++)
    {
        if ((glGenTextures(1, &sNdsPupupuWaterResourceNames[index]) == 0) ||
            (sNdsPupupuWaterResourceNames[index] == 0))
        {
            return FALSE;
        }
        sNdsPupupuWaterResourceNameCount++;
    }
    return TRUE;
}

#endif

s32 ndsPupupuWaterResidencyPrepare(void)
{
#if NDS_RENDERER_M4_WATER_TILED_AOT
    FILE *file = NULL;
    u8 *scratch = NULL;
    void *primary_texture;
    void *secondary_texture;
    u32 hash = 0x811c9dc5u;
    u32 palette_index;
    u32 failure = nNDSPupupuWaterResidencyFailureNone;

    if (gNdsPupupuWaterResidencyPrepared != FALSE)
    {
        return TRUE;
    }
    gNdsPupupuWaterResidencyPrepareCount++;
    gNdsPupupuWaterResidencyPayloadBytes = 0u;
    gNdsPupupuWaterResidencyScratchBytes = 0u;
    gNdsPupupuWaterResidencyLastFailure =
        nNDSPupupuWaterResidencyFailureNone;
    ndsPupupuWaterReleaseResources();

    file = fopen(NDS_PUPUPU_WATER_TILED_RESIDENCY_PAYLOAD_PATH, "rb");
    if (file == NULL)
    {
        failure = nNDSPupupuWaterResidencyFailureOpen;
        goto fail;
    }
    scratch = malloc(NDS_PUPUPU_WATER_RESIDENCY_SCRATCH_BYTES);
    if (scratch == NULL)
    {
        failure = nNDSPupupuWaterResidencyFailureScratch;
        goto fail;
    }
    gNdsPupupuWaterResidencyScratchBytes =
        NDS_PUPUPU_WATER_RESIDENCY_SCRATCH_BYTES;
    if (ndsPupupuWaterGenerateResourceNames() == FALSE)
    {
        failure = nNDSPupupuWaterResidencyFailureResource;
        goto fail;
    }

    /* Visibility is baked into index zero in all 572 exact masked tiles.
     * Color-zero transparency therefore matches the source alpha bit in one
     * semantic pass and also makes the four unused secondary slots fail closed. */
    primary_texture = ndsPupupuWaterAllocateTexture(
        sNdsPupupuWaterResourceNames[NDS_PUPUPU_WATER_PRIMARY_TEXTURE],
        GL_RGB256, TEXTURE_SIZE_512, TEXTURE_SIZE_256,
        TEXGEN_OFF | GL_TEXTURE_COLOR0_TRANSPARENT,
        NDS_PUPUPU_WATER_TILED_PRIMARY_ATLAS_BYTES);
    secondary_texture = ndsPupupuWaterAllocateTexture(
        sNdsPupupuWaterResourceNames[NDS_PUPUPU_WATER_SECONDARY_TEXTURE],
        GL_RGB256, TEXTURE_SIZE_256, TEXTURE_SIZE_64,
        TEXGEN_OFF | GL_TEXTURE_COLOR0_TRANSPARENT,
        NDS_PUPUPU_WATER_TILED_SECONDARY_ATLAS_BYTES);
    if ((primary_texture == NULL) || (secondary_texture == NULL))
    {
        failure = nNDSPupupuWaterResidencyFailureTexture;
        goto fail;
    }
    /* A 128 KiB primary atlas cannot survive static-first fragmentation. The
     * integration owner must reset once, prepare this atlas at A:0, then let
     * the 94,208-byte static set consume B after the secondary atlas. */
    if (((uintptr_t)primary_texture !=
         NDS_PUPUPU_WATER_TEXTURE_VRAM_BASE) ||
        ((uintptr_t)secondary_texture !=
         (NDS_PUPUPU_WATER_TEXTURE_VRAM_BASE +
          NDS_PUPUPU_WATER_TILED_PRIMARY_ATLAS_BYTES)))
    {
        failure = nNDSPupupuWaterResidencyFailureTextureBank;
        goto fail;
    }
    gNdsPupupuWaterResidencyPrimaryTextureAddress =
        (u32)(uintptr_t)primary_texture;
    gNdsPupupuWaterResidencySecondaryTextureAddress =
        (u32)(uintptr_t)secondary_texture;

    failure = ndsPupupuWaterRead(
        file, scratch, NDS_PUPUPU_WATER_TILED_PRIMARY_ATLAS_BYTES,
        primary_texture, &hash);
    if (failure != nNDSPupupuWaterResidencyFailureNone)
    {
        goto fail;
    }
    gNdsPupupuWaterResidencyTextureBytes +=
        NDS_PUPUPU_WATER_TILED_PRIMARY_ATLAS_BYTES;
    gNdsPupupuWaterResidencyTextureCount++;
    failure = ndsPupupuWaterRead(
        file, scratch, NDS_PUPUPU_WATER_TILED_SECONDARY_ATLAS_BYTES,
        secondary_texture, &hash);
    if (failure != nNDSPupupuWaterResidencyFailureNone)
    {
        goto fail;
    }
    gNdsPupupuWaterResidencyTextureBytes +=
        NDS_PUPUPU_WATER_TILED_SECONDARY_ATLAS_BYTES;
    gNdsPupupuWaterResidencyTextureCount++;

    for (palette_index = 0u;
         palette_index < NDS_PUPUPU_WATER_RESIDENCY_PALETTE_COUNT;
         palette_index++)
    {
        failure = ndsPupupuWaterRead(
            file, scratch, 512u, NULL, &hash);
        if (failure != nNDSPupupuWaterResidencyFailureNone)
        {
            goto fail;
        }
        if (ndsPupupuWaterCreatePalette(
                sNdsPupupuWaterResourceNames[
                    NDS_PUPUPU_WATER_PALETTE_FIRST + palette_index],
                (const u16 *)scratch, palette_index) == FALSE)
        {
            failure = nNDSPupupuWaterResidencyFailurePalette;
            goto fail;
        }
        gNdsPupupuWaterResidencyPaletteBytes += 512u;
        gNdsPupupuWaterResidencyPaletteCount++;
    }
    if ((fgetc(file) != EOF) || (ferror(file) != 0))
    {
        failure = nNDSPupupuWaterResidencyFailureRead;
        goto fail;
    }
    if ((gNdsPupupuWaterResidencyPayloadBytes !=
         NDS_PUPUPU_WATER_TILED_RESIDENCY_PAYLOAD_BYTES) ||
        (hash != NDS_PUPUPU_WATER_PAYLOAD_FNV1A32))
    {
        failure = nNDSPupupuWaterResidencyFailurePayloadHash;
        goto fail;
    }
    gNdsPupupuWaterResidencyPaletteFirstByte = 0u;
    gNdsPupupuWaterResidencyPaletteEndByte =
        NDS_PUPUPU_WATER_RESIDENCY_PALETTE_BYTES;
    if ((sNdsPupupuWaterResourceNameCount !=
         NDS_PUPUPU_WATER_RESIDENCY_RESOURCE_COUNT) ||
        (gNdsPupupuWaterResidencyTextureBytes !=
         NDS_PUPUPU_WATER_RESIDENCY_TEXTURE_BYTES) ||
        (gNdsPupupuWaterResidencyPaletteBytes !=
         NDS_PUPUPU_WATER_RESIDENCY_PALETTE_BYTES) ||
        (gNdsPupupuWaterResidencyTextureCount !=
         NDS_PUPUPU_WATER_RESIDENCY_TEXTURE_COUNT) ||
        (gNdsPupupuWaterResidencyPaletteCount !=
         NDS_PUPUPU_WATER_RESIDENCY_PALETTE_COUNT))
    {
        failure = nNDSPupupuWaterResidencyFailureResource;
        goto fail;
    }

    if (fclose(file) != 0)
    {
        file = NULL;
        failure = nNDSPupupuWaterResidencyFailureRead;
        goto fail;
    }
    file = NULL;
    free(scratch);
    scratch = NULL;
    gNdsPupupuWaterResidencyPrepared = TRUE;
    gNdsPupupuWaterResidencyPrepareSuccessCount++;
    return TRUE;

fail:
    if (file != NULL)
    {
        fclose(file);
    }
    free(scratch);
    ndsPupupuWaterReleaseResources();
    gNdsPupupuWaterResidencyPrepareFailCount++;
    gNdsPupupuWaterResidencyLastFailure = failure;
    return FALSE;
#else
    gNdsPupupuWaterResidencyPrepareCount++;
    gNdsPupupuWaterResidencyPrepareFailCount++;
    gNdsPupupuWaterResidencyLastFailure =
        nNDSPupupuWaterResidencyFailureDisabled;
    return FALSE;
#endif
}

void ndsPupupuWaterResidencyReset(void)
{
    gNdsPupupuWaterResidencyPrepared = FALSE;
#if NDS_RENDERER_M4_WATER_TILED_AOT
    ndsPupupuWaterReleaseResources();
#endif
    gNdsPupupuWaterResidencyResetCount++;
}

s32 ndsPupupuWaterResidencyBindPrimary(u32 palette_index)
{
#if NDS_RENDERER_M4_WATER_TILED_AOT
    if ((gNdsPupupuWaterResidencyPrepared == FALSE) ||
        (palette_index >=
         NDS_PUPUPU_WATER_RESIDENCY_PALETTE_COUNT))
    {
        gNdsPupupuWaterResidencyBindFailCount++;
        return FALSE;
    }
    glBindTexture(GL_TEXTURE_2D,
                  sNdsPupupuWaterResourceNames[
                      NDS_PUPUPU_WATER_PRIMARY_TEXTURE]);
    glAssignColorTable(
        GL_TEXTURE_2D,
        sNdsPupupuWaterResourceNames[
            NDS_PUPUPU_WATER_PALETTE_FIRST + palette_index]);
    gNdsPupupuWaterResidencyPrimaryBindCount++;
    return TRUE;
#else
    (void)palette_index;
    gNdsPupupuWaterResidencyBindFailCount++;
    return FALSE;
#endif
}

s32 ndsPupupuWaterResidencyBindSecondary(u32 palette_index)
{
#if NDS_RENDERER_M4_WATER_TILED_AOT
    if ((gNdsPupupuWaterResidencyPrepared == FALSE) ||
        (palette_index >= NDS_PUPUPU_WATER_RESIDENCY_PALETTE_COUNT))
    {
        gNdsPupupuWaterResidencyBindFailCount++;
        return FALSE;
    }
    glBindTexture(GL_TEXTURE_2D,
                  sNdsPupupuWaterResourceNames[
                      NDS_PUPUPU_WATER_SECONDARY_TEXTURE]);
    glAssignColorTable(
        GL_TEXTURE_2D,
        sNdsPupupuWaterResourceNames[
            NDS_PUPUPU_WATER_PALETTE_FIRST + palette_index]);
    gNdsPupupuWaterResidencySecondaryBindCount++;
    return TRUE;
#else
    (void)palette_index;
    gNdsPupupuWaterResidencyBindFailCount++;
    return FALSE;
#endif
}
