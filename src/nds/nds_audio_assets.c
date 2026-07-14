#include <nds/nds_audio_assets.h>

#include <stdio.h>
#include <string.h>

#include <PR/ultratypes.h>
#include <nds/nds_audio_fgm.h>

#define NDS_AUDIO_O2R_RAW_SIZE_OFFSET 0x40L
#define NDS_AUDIO_O2R_DATA_OFFSET 0x44L
#define NDS_AUDIO_MAX_OFFSETS 1024u

#define NDS_AUDIO_MASK_SEQ        (1u << 0)
#define NDS_AUDIO_MASK_BANK1_CTL  (1u << 1)
#define NDS_AUDIO_MASK_BANK1_TBL  (1u << 2)
#define NDS_AUDIO_MASK_BANK2_CTL  (1u << 3)
#define NDS_AUDIO_MASK_BANK2_TBL  (1u << 4)
#define NDS_AUDIO_MASK_FGM_UNK    (1u << 5)
#define NDS_AUDIO_MASK_FGM_TBL    (1u << 6)
#define NDS_AUDIO_MASK_FGM_UCD    (1u << 7)

#define NDS_AUDIO_REV_S1 0x5331u
#define NDS_AUDIO_REV_B1 0x4231u

typedef struct NDSAudioOffsetSet {
    u32 offsets[NDS_AUDIO_MAX_OFFSETS];
    u32 count;
} NDSAudioOffsetSet;

volatile u32 gNdsAudioAssetResult;
volatile u32 gNdsAudioAssetMask;
volatile u32 gNdsAudioAssetOpenCount;
volatile u32 gNdsAudioAssetOpenFailCount;
volatile u32 gNdsAudioAssetFormatFailCount;
volatile u32 gNdsAudioAssetShortReadCount;
volatile u32 gNdsAudioAssetRawBytes;
volatile u32 gNdsAudioAssetResidentBytes;
volatile u32 gNdsAudioAssetScratchMaxBytes;
volatile u32 gNdsAudioAssetSeqCount;
volatile u32 gNdsAudioAssetSeqFirstOffset;
volatile u32 gNdsAudioAssetSeqFirstLength;
volatile u32 gNdsAudioAssetSeqMaxLength;
volatile u32 gNdsAudioAssetBank1BankCount;
volatile u32 gNdsAudioAssetBank1InstrumentCount;
volatile u32 gNdsAudioAssetBank1WaveCount;
volatile u32 gNdsAudioAssetBank1SampleRate;
volatile u32 gNdsAudioAssetBank2BankCount;
volatile u32 gNdsAudioAssetBank2InstrumentCount;
volatile u32 gNdsAudioAssetBank2WaveCount;
volatile u32 gNdsAudioAssetBank2SampleRate;
volatile u32 gNdsAudioAssetFgmUnkCount;
volatile u32 gNdsAudioAssetFgmTableCount;
volatile u32 gNdsAudioAssetFgmUcodeCount;

static u32 sNdsAudioAssetLoaded;

static u16 ndsAudioReadBe16(const u8 *data)
{
    return (u16)(((u16)data[0] << 8) | data[1]);
}

static u32 ndsAudioReadBe32(const u8 *data)
{
    return ((u32)data[0] << 24) | ((u32)data[1] << 16) |
           ((u32)data[2] << 8) | data[3];
}

static u32 ndsAudioReadLe32(const u8 *data)
{
    return ((u32)data[3] << 24) | ((u32)data[2] << 16) |
           ((u32)data[1] << 8) | data[0];
}

static s32 ndsAudioReadExact(FILE *file, void *dst, size_t size)
{
    if (fread(dst, 1, size, file) != size)
    {
        gNdsAudioAssetShortReadCount++;
        return FALSE;
    }
    return TRUE;
}

static s32 ndsAudioOpenWrapped(const char *path, u32 expected_raw_size,
                               FILE **out_file, u32 *out_raw_size)
{
    FILE *file;
    u8 size_bytes[4];
    long file_size;
    u32 raw_size;

    file = fopen(path, "rb");
    if (file == NULL)
    {
        gNdsAudioAssetOpenFailCount++;
        return FALSE;
    }
    gNdsAudioAssetOpenCount++;

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        gNdsAudioAssetShortReadCount++;
        return FALSE;
    }
    file_size = ftell(file);
    if (fseek(file, NDS_AUDIO_O2R_RAW_SIZE_OFFSET, SEEK_SET) != 0)
    {
        fclose(file);
        gNdsAudioAssetShortReadCount++;
        return FALSE;
    }
    if (ndsAudioReadExact(file, size_bytes, sizeof(size_bytes)) == FALSE)
    {
        fclose(file);
        return FALSE;
    }
    raw_size = ndsAudioReadLe32(size_bytes);
    if ((raw_size != expected_raw_size) ||
        (file_size != (long)(NDS_AUDIO_O2R_DATA_OFFSET + raw_size)))
    {
        fclose(file);
        gNdsAudioAssetFormatFailCount++;
        return FALSE;
    }
    if (fseek(file, NDS_AUDIO_O2R_DATA_OFFSET, SEEK_SET) != 0)
    {
        fclose(file);
        gNdsAudioAssetShortReadCount++;
        return FALSE;
    }

    gNdsAudioAssetRawBytes += raw_size;
    if (out_raw_size != NULL)
    {
        *out_raw_size = raw_size;
    }
    *out_file = file;
    return TRUE;
}

static s32 ndsAudioReadAt(FILE *file, u32 raw_size, u32 offset, void *dst,
                          size_t size)
{
    if ((offset > raw_size) || (size > (size_t)(raw_size - offset)))
    {
        gNdsAudioAssetFormatFailCount++;
        return FALSE;
    }
    if (fseek(file, NDS_AUDIO_O2R_DATA_OFFSET + (long)offset, SEEK_SET) != 0)
    {
        gNdsAudioAssetShortReadCount++;
        return FALSE;
    }
    if (size > gNdsAudioAssetScratchMaxBytes)
    {
        gNdsAudioAssetScratchMaxBytes = (u32)size;
    }
    return ndsAudioReadExact(file, dst, size);
}

static s32 ndsAudioOffsetSetAdd(NDSAudioOffsetSet *set, u32 offset)
{
    u32 i;

    if (offset == 0u)
    {
        return FALSE;
    }
    for (i = 0u; i < set->count; i++)
    {
        if (set->offsets[i] == offset)
        {
            return FALSE;
        }
    }
    if (set->count >= NDS_AUDIO_MAX_OFFSETS)
    {
        gNdsAudioAssetFormatFailCount++;
        return FALSE;
    }
    set->offsets[set->count++] = offset;
    return TRUE;
}

static s32 ndsAudioParseSound(FILE *file, u32 raw_size, u32 sound_offset,
                              NDSAudioOffsetSet *sounds,
                              NDSAudioOffsetSet *waves)
{
    u8 word[4];
    u32 wave_offset;

    if (sound_offset == 0u)
    {
        return TRUE;
    }
    if ((sound_offset + 12u) > raw_size)
    {
        gNdsAudioAssetFormatFailCount++;
        return FALSE;
    }
    if (ndsAudioOffsetSetAdd(sounds, sound_offset) == FALSE)
    {
        return TRUE;
    }
    if (ndsAudioReadAt(file, raw_size, sound_offset + 8u, word,
                       sizeof(word)) == FALSE)
    {
        return FALSE;
    }
    wave_offset = ndsAudioReadBe32(word);
    if ((wave_offset == 0u) || ((wave_offset + 24u) > raw_size))
    {
        gNdsAudioAssetFormatFailCount++;
        return FALSE;
    }
    ndsAudioOffsetSetAdd(waves, wave_offset);
    return TRUE;
}

static s32 ndsAudioParseInstrument(FILE *file, u32 raw_size,
                                   u32 inst_offset,
                                   NDSAudioOffsetSet *instruments,
                                   NDSAudioOffsetSet *sounds,
                                   NDSAudioOffsetSet *waves)
{
    u8 header[16];
    u8 word[4];
    s16 sound_count;
    u32 i;

    if (inst_offset == 0u)
    {
        return TRUE;
    }
    if ((inst_offset + 16u) > raw_size)
    {
        gNdsAudioAssetFormatFailCount++;
        return FALSE;
    }
    if (ndsAudioOffsetSetAdd(instruments, inst_offset) == FALSE)
    {
        return TRUE;
    }

    if (ndsAudioReadAt(file, raw_size, inst_offset, header,
                       sizeof(header)) == FALSE)
    {
        return FALSE;
    }
    sound_count = (s16)ndsAudioReadBe16(&header[14]);
    if ((sound_count < 0) ||
        ((inst_offset + 16u + ((u32)sound_count * 4u)) > raw_size))
    {
        gNdsAudioAssetFormatFailCount++;
        return FALSE;
    }
    for (i = 0u; i < (u32)sound_count; i++)
    {
        u32 sound_offset;

        if (ndsAudioReadAt(file, raw_size, inst_offset + 16u + (i * 4u),
                           word, sizeof(word)) == FALSE)
        {
            return FALSE;
        }
        sound_offset = ndsAudioReadBe32(word);
        if (ndsAudioParseSound(file, raw_size, sound_offset, sounds, waves) ==
            FALSE)
        {
            return FALSE;
        }
    }
    return TRUE;
}

static s32 ndsAudioParseCtl(const char *path, u32 raw_expected,
                            volatile u32 *out_bank_count,
                            volatile u32 *out_instrument_count,
                            volatile u32 *out_wave_count,
                            volatile u32 *out_sample_rate)
{
    FILE *file;
    u8 header[12];
    u8 word[4];
    u32 raw_size;
    u16 revision;
    u16 bank_count;
    u32 i;
    NDSAudioOffsetSet instruments = { { 0 }, 0u };
    NDSAudioOffsetSet sounds = { { 0 }, 0u };
    NDSAudioOffsetSet waves = { { 0 }, 0u };

    if (ndsAudioOpenWrapped(path, raw_expected, &file, &raw_size) == FALSE)
    {
        return FALSE;
    }
    if (raw_size < 8u)
    {
        gNdsAudioAssetFormatFailCount++;
        goto fail;
    }
    if (ndsAudioReadAt(file, raw_size, 0u, header, 4u) == FALSE)
    {
        goto fail;
    }
    revision = ndsAudioReadBe16(header);
    bank_count = ndsAudioReadBe16(&header[2]);
    if ((revision != NDS_AUDIO_REV_B1) || (bank_count == 0u) ||
        ((4u + ((u32)bank_count * 4u)) > raw_size))
    {
        gNdsAudioAssetFormatFailCount++;
        goto fail;
    }

    for (i = 0u; i < bank_count; i++)
    {
        u32 bank_offset;
        s16 inst_count;
        u32 sample_rate;
        u32 percussion;
        u32 j;

        if (ndsAudioReadAt(file, raw_size, 4u + (i * 4u), word,
                           sizeof(word)) == FALSE)
        {
            goto fail;
        }
        bank_offset = ndsAudioReadBe32(word);
        if ((bank_offset + 12u) > raw_size)
        {
            gNdsAudioAssetFormatFailCount++;
            goto fail;
        }
        if (ndsAudioReadAt(file, raw_size, bank_offset, header,
                           sizeof(header)) == FALSE)
        {
            goto fail;
        }
        inst_count = (s16)ndsAudioReadBe16(header);
        if (inst_count < 0)
        {
            gNdsAudioAssetFormatFailCount++;
            goto fail;
        }
        sample_rate = ndsAudioReadBe32(&header[4]);
        percussion = ndsAudioReadBe32(&header[8]);
        if (i == 0u)
        {
            *out_sample_rate = sample_rate;
        }
        if ((bank_offset + 12u + ((u32)inst_count * 4u)) > raw_size)
        {
            gNdsAudioAssetFormatFailCount++;
            goto fail;
        }
        if (ndsAudioParseInstrument(file, raw_size, percussion, &instruments,
                                    &sounds, &waves) == FALSE)
        {
            goto fail;
        }
        for (j = 0u; j < (u32)inst_count; j++)
        {
            u32 inst_offset;

            if (ndsAudioReadAt(file, raw_size,
                               bank_offset + 12u + (j * 4u), word,
                               sizeof(word)) == FALSE)
            {
                goto fail;
            }
            inst_offset = ndsAudioReadBe32(word);
            if (ndsAudioParseInstrument(file, raw_size, inst_offset,
                                        &instruments, &sounds, &waves) ==
                FALSE)
            {
                goto fail;
            }
        }
    }

    *out_bank_count = bank_count;
    *out_instrument_count = instruments.count;
    *out_wave_count = waves.count;
    fclose(file);
    return TRUE;

fail:
    fclose(file);
    return FALSE;
}

static s32 ndsAudioParseSeq(void)
{
    FILE *file;
    u8 header[4];
    u32 raw_size;
    u16 revision;
    u16 seq_count;
    u32 i;
    u32 max_len = 0u;

    if (ndsAudioOpenWrapped("nitro:/audio/S1_music_sbk", 159248u, &file,
                            &raw_size) == FALSE)
    {
        return FALSE;
    }
    if (ndsAudioReadExact(file, header, sizeof(header)) == FALSE)
    {
        fclose(file);
        return FALSE;
    }
    revision = ndsAudioReadBe16(header);
    seq_count = ndsAudioReadBe16(&header[2]);
    if ((revision != NDS_AUDIO_REV_S1) ||
        ((4u + ((u32)seq_count * 8u)) > raw_size))
    {
        fclose(file);
        gNdsAudioAssetFormatFailCount++;
        return FALSE;
    }
    gNdsAudioAssetSeqCount = seq_count;

    for (i = 0u; i < seq_count; i++)
    {
        u8 entry[8];
        u32 offset;
        u32 len;

        if (ndsAudioReadExact(file, entry, sizeof(entry)) == FALSE)
        {
            fclose(file);
            return FALSE;
        }
        offset = ndsAudioReadBe32(entry);
        len = ndsAudioReadBe32(&entry[4]);
        if ((offset >= raw_size) || (len > raw_size) ||
            ((offset + len) > raw_size))
        {
            fclose(file);
            gNdsAudioAssetFormatFailCount++;
            return FALSE;
        }
        if (i == 0u)
        {
            gNdsAudioAssetSeqFirstOffset = offset;
            gNdsAudioAssetSeqFirstLength = len;
        }
        if (len > max_len)
        {
            max_len = len;
        }
    }
    fclose(file);
    gNdsAudioAssetSeqMaxLength = max_len;
    gNdsAudioAssetMask |= NDS_AUDIO_MASK_SEQ;
    return TRUE;
}

static s32 ndsAudioParseCtlAssets(void)
{
    FILE *file;
    u32 raw_size;

    if (ndsAudioParseCtl("nitro:/audio/B1_sounds1_ctl", 26400u,
                         &gNdsAudioAssetBank1BankCount,
                         &gNdsAudioAssetBank1InstrumentCount,
                         &gNdsAudioAssetBank1WaveCount,
                         &gNdsAudioAssetBank1SampleRate) == FALSE)
    {
        return FALSE;
    }
    gNdsAudioAssetMask |= NDS_AUDIO_MASK_BANK1_CTL;

    if (ndsAudioOpenWrapped("nitro:/audio/B1_sounds1_tbl", 1141104u, &file,
                            &raw_size) == FALSE)
    {
        return FALSE;
    }
    fclose(file);
    gNdsAudioAssetMask |= NDS_AUDIO_MASK_BANK1_TBL;

    if (ndsAudioParseCtl("nitro:/audio/B1_sounds2_ctl", 64416u,
                         &gNdsAudioAssetBank2BankCount,
                         &gNdsAudioAssetBank2InstrumentCount,
                         &gNdsAudioAssetBank2WaveCount,
                         &gNdsAudioAssetBank2SampleRate) == FALSE)
    {
        return FALSE;
    }
    gNdsAudioAssetMask |= NDS_AUDIO_MASK_BANK2_CTL;

    if (ndsAudioOpenWrapped("nitro:/audio/B1_sounds2_tbl", 2998752u, &file,
                            &raw_size) == FALSE)
    {
        return FALSE;
    }
    fclose(file);
    gNdsAudioAssetMask |= NDS_AUDIO_MASK_BANK2_TBL;
    return TRUE;
}

static s32 ndsAudioParseFgmUnk(void)
{
    FILE *file;
    u8 bytes[4];
    u32 raw_size;

    if (ndsAudioOpenWrapped("nitro:/audio/fgm_unk", 2080u, &file,
                            &raw_size) == FALSE)
    {
        return FALSE;
    }
    if (ndsAudioReadExact(file, bytes, sizeof(bytes)) == FALSE)
    {
        fclose(file);
        return FALSE;
    }
    fclose(file);
    gNdsAudioAssetFgmUnkCount = ndsAudioReadBe32(bytes);
    gNdsAudioAssetMask |= NDS_AUDIO_MASK_FGM_UNK;
    return TRUE;
}

static s32 ndsAudioParsePackage(const char *path, u32 raw_expected,
                                u32 expected_count,
                                volatile u32 *out_count,
                                u32 mask)
{
    FILE *file;
    u8 bytes[4];
    u32 raw_size;
    u32 count;
    u32 i;
    u32 previous = 0u;

    if (ndsAudioOpenWrapped(path, raw_expected, &file, &raw_size) == FALSE)
    {
        return FALSE;
    }
    if (ndsAudioReadExact(file, bytes, sizeof(bytes)) == FALSE)
    {
        fclose(file);
        return FALSE;
    }
    count = ndsAudioReadBe32(bytes);
    if ((count != expected_count) ||
        ((4u + (count * 4u)) > raw_size))
    {
        fclose(file);
        gNdsAudioAssetFormatFailCount++;
        return FALSE;
    }
    for (i = 0u; i < count; i++)
    {
        u32 offset;

        if (ndsAudioReadExact(file, bytes, sizeof(bytes)) == FALSE)
        {
            fclose(file);
            return FALSE;
        }
        offset = ndsAudioReadBe32(bytes);
        if ((offset < (4u + (count * 4u))) || (offset >= raw_size) ||
            ((i != 0u) && (offset < previous)))
        {
            fclose(file);
            gNdsAudioAssetFormatFailCount++;
            return FALSE;
        }
        previous = offset;
    }
    fclose(file);
    *out_count = count;
    gNdsAudioAssetMask |= mask;
    return TRUE;
}

void ndsAudioAssetDiagnosticsReset(void)
{
    ndsAudioFgmDiagnosticsReset();
    sNdsAudioAssetLoaded = 0u;
    gNdsAudioAssetResult = 0u;
    gNdsAudioAssetMask = 0u;
    gNdsAudioAssetOpenCount = 0u;
    gNdsAudioAssetOpenFailCount = 0u;
    gNdsAudioAssetFormatFailCount = 0u;
    gNdsAudioAssetShortReadCount = 0u;
    gNdsAudioAssetRawBytes = 0u;
    gNdsAudioAssetResidentBytes = 0u;
    gNdsAudioAssetScratchMaxBytes = 0u;
    gNdsAudioAssetSeqCount = 0u;
    gNdsAudioAssetSeqFirstOffset = 0u;
    gNdsAudioAssetSeqFirstLength = 0u;
    gNdsAudioAssetSeqMaxLength = 0u;
    gNdsAudioAssetBank1BankCount = 0u;
    gNdsAudioAssetBank1InstrumentCount = 0u;
    gNdsAudioAssetBank1WaveCount = 0u;
    gNdsAudioAssetBank1SampleRate = 0u;
    gNdsAudioAssetBank2BankCount = 0u;
    gNdsAudioAssetBank2InstrumentCount = 0u;
    gNdsAudioAssetBank2WaveCount = 0u;
    gNdsAudioAssetBank2SampleRate = 0u;
    gNdsAudioAssetFgmUnkCount = 0u;
    gNdsAudioAssetFgmTableCount = 0u;
    gNdsAudioAssetFgmUcodeCount = 0u;
}

void ndsAudioAssetLoadFenced(void)
{
    if (sNdsAudioAssetLoaded != 0u)
    {
        return;
    }
    sNdsAudioAssetLoaded = 1u;
    ndsAudioFgmLoadFenced();

    if ((ndsAudioParseSeq() != FALSE) &&
        (ndsAudioParseCtlAssets() != FALSE) &&
        (ndsAudioParseFgmUnk() != FALSE) &&
        (ndsAudioParsePackage("nitro:/audio/fgm_tbl", 11728u, 464u,
                              &gNdsAudioAssetFgmTableCount,
                              NDS_AUDIO_MASK_FGM_TBL) != FALSE) &&
        (ndsAudioParsePackage("nitro:/audio/fgm_ucd", 19232u, 695u,
                              &gNdsAudioAssetFgmUcodeCount,
                              NDS_AUDIO_MASK_FGM_UCD) != FALSE) &&
        (gNdsAudioAssetMask == NDS_AUDIO_ASSET_EXPECTED_MASK))
    {
        gNdsAudioAssetResult = NDS_AUDIO_ASSET_PASS;
    }
}
