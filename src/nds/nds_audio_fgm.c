#include <nds.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <gm/gmsound.h>
#include <nds/nds_audio_fgm.h>

#define NDS_AUDIO_FGM_PATH "nitro:/audio/fgm_phase_pack_ima.bin"
#define NDS_AUDIO_FGM_PACK_HEADER_BYTES 16u
#define NDS_AUDIO_FGM_PACK_ENTRY_BYTES 32u
#define NDS_AUDIO_FGM_ENVELOPE_POINT_BYTES 4u
#define NDS_AUDIO_FGM_PACK_DATA_OFFSET \
    (NDS_AUDIO_FGM_PACK_HEADER_BYTES + \
     (NDS_AUDIO_FGM_PHASE_COUNT * NDS_AUDIO_FGM_PACK_ENTRY_BYTES))
#define NDS_AUDIO_FGM_HANDLE_COUNT NDS_AUDIO_FGM_NONREUSE_HANDLE_CAPACITY
#define NDS_AUDIO_FGM_CHANNEL_COUNT 16u
#define NDS_AUDIO_FGM_TIMER_MICROSECONDS 5750u

#define NDS_AUDIO_FGM_MASK_PACK_LOADED (1u << 0)
#define NDS_AUDIO_FGM_MASK_SUPPORTED_PLAY (1u << 1)
#define NDS_AUDIO_FGM_MASK_LOOP_PLAY (1u << 2)
#define NDS_AUDIO_FGM_MASK_PHASE_COMPLETE (1u << 3)

typedef struct NDSAudioFgmPackEntry {
    u16 id;
    u16 flags;
    u32 data_offset;
    u32 data_bytes;
    u32 sample_count;
    u16 frequency;
    u16 duration_ticks;
    u8 volume;
    u8 pan;
    u16 source_sound_index;
    u32 envelope_offset;
    u16 envelope_count;
    u16 reserved;
} NDSAudioFgmPackEntry;

typedef struct NDSAudioFgmHandle {
    alSoundEffect effect;
    u32 generation;
    u32 start_tick;
    u32 end_tick;
    u32 envelope_offset;
    u16 envelope_count;
    u16 envelope_index;
    s8 channel;
    u8 allocated;
    u8 live;
    u8 _pad;
} NDSAudioFgmHandle;

volatile u32 gNdsAudioFgmResult;
volatile u32 gNdsAudioFgmMask;
volatile u32 gNdsAudioFgmLoaded;
volatile u32 gNdsAudioFgmResidentBytes;
volatile u32 gNdsAudioFgmSupportedCount;
volatile u32 gNdsAudioFgmOpenFailCount;
volatile u32 gNdsAudioFgmReadFailCount;
volatile u32 gNdsAudioFgmFormatFailCount;
volatile u32 gNdsAudioFgmPlayCalls;
volatile u32 gNdsAudioFgmSupportedPlayCount;
volatile u32 gNdsAudioFgmUnsupportedCallCount;
volatile u32 gNdsAudioFgmIncludedLookupFailCount;
volatile u32 gNdsAudioFgmPlayFailCount;
volatile u32 gNdsAudioFgmPhasePlayMask;
volatile u32 gNdsAudioFgmPhasePlayCounts[NDS_AUDIO_FGM_PHASE_COUNT];
volatile u32 gNdsAudioFgmLoopPlayCount;
volatile u32 gNdsAudioFgmStopCalls;
volatile u32 gNdsAudioFgmStopAllCalls;
volatile u32 gNdsAudioFgmDurationStopCount;
volatile u32 gNdsAudioFgmStaleStopCount;
volatile u32 gNdsAudioFgmGenerationMismatchCount;
volatile u32 gNdsAudioFgmActiveHandles;
volatile u32 gNdsAudioFgmMaxActiveHandles;
volatile u32 gNdsAudioFgmChannelMask;
volatile u32 gNdsAudioFgmLastChannel;
volatile u32 gNdsAudioFgmLastID;
volatile u32 gNdsAudioFgmLastGeneration;
volatile u32 gNdsAudioFgmPoolExhaustCount;
volatile u32 gNdsAudioFgmAllocatedHandles;
volatile u32 gNdsAudioFgmNonReuseCapacity;
volatile u32 gNdsAudioFgmEnvelopeStepCount;
volatile u32 gNdsAudioFgmFidelityDebtMask;

static u8 sNdsAudioFgmPack[NDS_AUDIO_FGM_PACK_BYTES]
    __attribute__((aligned(4)));
static NDSAudioFgmPackEntry
    sNdsAudioFgmEntries[NDS_AUDIO_FGM_PHASE_COUNT];
static NDSAudioFgmHandle sNdsAudioFgmHandles[NDS_AUDIO_FGM_HANDLE_COUNT];
static NDSAudioFgmHandle *sNdsAudioFgmChannelOwners[NDS_AUDIO_FGM_CHANNEL_COUNT];
static u32 sNdsAudioFgmChannelGenerations[NDS_AUDIO_FGM_CHANNEL_COUNT];
static u32 sNdsAudioFgmNextGeneration = 1u;

_Static_assert(NDS_AUDIO_FGM_PACK_DATA_OFFSET == 176u,
               "FGM phase pack header layout changed");
_Static_assert(NDS_AUDIO_FGM_PACK_BYTES <= (64u * 1024u),
               "FGM phase pack exceeds its resident-memory gate");
_Static_assert(offsetof(NDSAudioFgmHandle, effect) == 0u,
               "BattleShip audio handle must be the backend handle prefix");
_Static_assert(NDS_AUDIO_FGM_HANDLE_COUNT > NDS_AUDIO_FGM_PHASE_COUNT,
               "one-match non-reuse pool must exceed the five phase calls");

static u16 ndsAudioFgmReadLe16(const u8 *data)
{
    return (u16)(((u16)data[1] << 8) | data[0]);
}

static u32 ndsAudioFgmReadLe32(const u8 *data)
{
    return ((u32)data[3] << 24) | ((u32)data[2] << 16) |
           ((u32)data[1] << 8) | data[0];
}

static s32 ndsAudioFgmIDIsIncluded(u16 id)
{
    switch (id)
    {
    case nSYAudioVoicePublicExcited:
    case nSYAudioVoiceAnnounceThree:
    case nSYAudioVoiceAnnounceTwo:
    case nSYAudioVoiceAnnounceOne:
    case nSYAudioVoiceAnnounceGo:
        return TRUE;
    default:
        return FALSE;
    }
}

static s32 ndsAudioFgmPhaseIndex(u16 id)
{
    switch (id)
    {
    case nSYAudioVoicePublicExcited:
        return 0;
    case nSYAudioVoiceAnnounceThree:
        return 1;
    case nSYAudioVoiceAnnounceTwo:
        return 2;
    case nSYAudioVoiceAnnounceOne:
        return 3;
    case nSYAudioVoiceAnnounceGo:
        return 4;
    default:
        return -1;
    }
}

static NDSAudioFgmPackEntry *ndsAudioFgmFindEntry(u16 id)
{
    u32 i;

    for (i = 0u; i < NDS_AUDIO_FGM_PHASE_COUNT; i++)
    {
        if (sNdsAudioFgmEntries[i].id == id)
        {
            return &sNdsAudioFgmEntries[i];
        }
    }
    return NULL;
}

static void ndsAudioFgmReleaseHandle(NDSAudioFgmHandle *handle,
                                     s32 kill_channel)
{
    s32 channel = handle->channel;

    if ((channel >= 0) && (channel < (s32)NDS_AUDIO_FGM_CHANNEL_COUNT) &&
        (sNdsAudioFgmChannelOwners[channel] == handle) &&
        (sNdsAudioFgmChannelGenerations[channel] == handle->generation))
    {
        if (kill_channel != FALSE)
        {
            soundKill(channel);
        }
        sNdsAudioFgmChannelOwners[channel] = NULL;
        sNdsAudioFgmChannelGenerations[channel] = 0u;
    }
    else if (handle->live != FALSE)
    {
        gNdsAudioFgmGenerationMismatchCount++;
    }
    if (handle->live != FALSE)
    {
        handle->live = FALSE;
        handle->channel = -1;
        handle->effect.sfx_id = 0u;
        if (gNdsAudioFgmActiveHandles != 0u)
        {
            gNdsAudioFgmActiveHandles--;
        }
    }
}

static NDSAudioFgmHandle *ndsAudioFgmHandleFromEffect(alSoundEffect *effect)
{
    uintptr_t start = (uintptr_t)&sNdsAudioFgmHandles[0];
    uintptr_t end = (uintptr_t)&sNdsAudioFgmHandles[NDS_AUDIO_FGM_HANDLE_COUNT];
    uintptr_t address = (uintptr_t)effect;

    if ((address < start) || (address >= end) ||
        (((address - start) % sizeof(NDSAudioFgmHandle)) != 0u))
    {
        return NULL;
    }
    return (NDSAudioFgmHandle *)effect;
}

static s32 ndsAudioFgmValidateEntry(u32 index, const u8 *raw)
{
    static const u16 expected_ids[NDS_AUDIO_FGM_PHASE_COUNT] = {
        nSYAudioVoicePublicExcited,
        nSYAudioVoiceAnnounceThree,
        nSYAudioVoiceAnnounceTwo,
        nSYAudioVoiceAnnounceOne,
        nSYAudioVoiceAnnounceGo
    };
    static const u16 expected_frequencies[NDS_AUDIO_FGM_PHASE_COUNT] = {
        15102u, 16000u, 16000u, 16000u, 16000u
    };
    static const u16 expected_durations[NDS_AUDIO_FGM_PHASE_COUNT] = {
        1200u, 99u, 100u, 85u, 150u
    };
    static const u8 expected_volumes[NDS_AUDIO_FGM_PHASE_COUNT] = {
        17u, 106u, 106u, 111u, 124u
    };
    static const u16 expected_sounds[NDS_AUDIO_FGM_PHASE_COUNT] = {
        320u, 208u, 209u, 210u, 211u
    };
    NDSAudioFgmPackEntry *entry = &sNdsAudioFgmEntries[index];
    u32 expected_flags = (index == 0u) ? 1u : 0u;

    entry->id = ndsAudioFgmReadLe16(&raw[0]);
    entry->flags = ndsAudioFgmReadLe16(&raw[2]);
    entry->data_offset = ndsAudioFgmReadLe32(&raw[4]);
    entry->data_bytes = ndsAudioFgmReadLe32(&raw[8]);
    entry->sample_count = ndsAudioFgmReadLe32(&raw[12]);
    entry->frequency = ndsAudioFgmReadLe16(&raw[16]);
    entry->duration_ticks = ndsAudioFgmReadLe16(&raw[18]);
    entry->volume = raw[20];
    entry->pan = raw[21];
    entry->source_sound_index = ndsAudioFgmReadLe16(&raw[22]);
    entry->envelope_offset = ndsAudioFgmReadLe32(&raw[24]);
    entry->envelope_count = ndsAudioFgmReadLe16(&raw[28]);
    entry->reserved = ndsAudioFgmReadLe16(&raw[30]);

    if ((entry->id != expected_ids[index]) ||
        (entry->flags != expected_flags) ||
        (entry->frequency != expected_frequencies[index]) ||
        (entry->duration_ticks != expected_durations[index]) ||
        (entry->volume != expected_volumes[index]) ||
        (entry->pan != 64u) ||
        (entry->source_sound_index != expected_sounds[index]) ||
        (entry->data_bytes < 4u) || ((entry->data_bytes & 3u) != 0u) ||
        (entry->sample_count == 0u) || (entry->volume > 127u) ||
        (entry->reserved != 0u) ||
        (entry->data_offset < NDS_AUDIO_FGM_PACK_DATA_OFFSET) ||
        ((entry->data_offset & 3u) != 0u) ||
        (entry->data_offset > NDS_AUDIO_FGM_PACK_BYTES) ||
        (entry->data_bytes >
         (NDS_AUDIO_FGM_PACK_BYTES - entry->data_offset)))
    {
        return FALSE;
    }
    if ((index == 0u) && (entry->sample_count != 28214u))
    {
        return FALSE;
    }
    if ((index != 0u) &&
        (entry->data_offset !=
         (sNdsAudioFgmEntries[index - 1u].data_offset +
          sNdsAudioFgmEntries[index - 1u].data_bytes)))
    {
        return FALSE;
    }
    if (index == 0u)
    {
        u32 point_index;
        u16 previous_tick = 0u;

        if ((entry->envelope_count != 28u) ||
            ((entry->envelope_offset & 3u) != 0u) ||
            (entry->envelope_offset > NDS_AUDIO_FGM_PACK_BYTES) ||
            (((u32)entry->envelope_count *
              NDS_AUDIO_FGM_ENVELOPE_POINT_BYTES) >
             (NDS_AUDIO_FGM_PACK_BYTES - entry->envelope_offset)))
        {
            return FALSE;
        }
        for (point_index = 0u; point_index < entry->envelope_count;
             point_index++)
        {
            const u8 *point = &sNdsAudioFgmPack[
                entry->envelope_offset +
                (point_index * NDS_AUDIO_FGM_ENVELOPE_POINT_BYTES)];
            u16 tick = ndsAudioFgmReadLe16(point);

            if ((tick <= previous_tick) ||
                (tick >= entry->duration_ticks) ||
                (point[2] > 127u) || (point[3] != 0u))
            {
                return FALSE;
            }
            previous_tick = tick;
        }
    }
    else if ((entry->envelope_offset != 0u) ||
             (entry->envelope_count != 0u))
    {
        return FALSE;
    }
    return TRUE;
}

void ndsAudioFgmDiagnosticsReset(void)
{
    u32 i;

    /* A raw BattleShip alSoundEffect pointer carries no generation token.
     * Therefore phase handles are deliberately never reused during one
     * match: five natural calls fit in the eight-slot pool, and scene reset
     * retires the whole epoch.  Clearing `allocated` on expiry would let a
     * stale source pointer alias and stop a later sound. */
    for (i = 0u; i < NDS_AUDIO_FGM_HANDLE_COUNT; i++)
    {
        if (sNdsAudioFgmHandles[i].live != FALSE)
        {
            ndsAudioFgmReleaseHandle(&sNdsAudioFgmHandles[i], TRUE);
        }
    }
    memset(sNdsAudioFgmHandles, 0, sizeof(sNdsAudioFgmHandles));
    memset(sNdsAudioFgmChannelOwners, 0,
           sizeof(sNdsAudioFgmChannelOwners));
    memset(sNdsAudioFgmChannelGenerations, 0,
           sizeof(sNdsAudioFgmChannelGenerations));
    memset(sNdsAudioFgmEntries, 0, sizeof(sNdsAudioFgmEntries));
    for (i = 0u; i < NDS_AUDIO_FGM_HANDLE_COUNT; i++)
    {
        sNdsAudioFgmHandles[i].channel = -1;
    }
    if (++sNdsAudioFgmNextGeneration == 0u)
    {
        sNdsAudioFgmNextGeneration = 1u;
    }

    gNdsAudioFgmResult = 0u;
    gNdsAudioFgmMask = 0u;
    gNdsAudioFgmLoaded = 0u;
    gNdsAudioFgmResidentBytes = 0u;
    gNdsAudioFgmSupportedCount = 0u;
    gNdsAudioFgmOpenFailCount = 0u;
    gNdsAudioFgmReadFailCount = 0u;
    gNdsAudioFgmFormatFailCount = 0u;
    gNdsAudioFgmPlayCalls = 0u;
    gNdsAudioFgmSupportedPlayCount = 0u;
    gNdsAudioFgmUnsupportedCallCount = 0u;
    gNdsAudioFgmIncludedLookupFailCount = 0u;
    gNdsAudioFgmPlayFailCount = 0u;
    gNdsAudioFgmPhasePlayMask = 0u;
    memset((void *)gNdsAudioFgmPhasePlayCounts, 0,
           sizeof(gNdsAudioFgmPhasePlayCounts));
    gNdsAudioFgmLoopPlayCount = 0u;
    gNdsAudioFgmStopCalls = 0u;
    gNdsAudioFgmStopAllCalls = 0u;
    gNdsAudioFgmDurationStopCount = 0u;
    gNdsAudioFgmStaleStopCount = 0u;
    gNdsAudioFgmGenerationMismatchCount = 0u;
    gNdsAudioFgmActiveHandles = 0u;
    gNdsAudioFgmMaxActiveHandles = 0u;
    gNdsAudioFgmChannelMask = 0u;
    gNdsAudioFgmLastChannel = 0xffffffffu;
    gNdsAudioFgmLastID = 0u;
    gNdsAudioFgmLastGeneration = 0u;
    gNdsAudioFgmPoolExhaustCount = 0u;
    gNdsAudioFgmAllocatedHandles = 0u;
    gNdsAudioFgmNonReuseCapacity = NDS_AUDIO_FGM_HANDLE_COUNT;
    gNdsAudioFgmEnvelopeStepCount = 0u;
    gNdsAudioFgmFidelityDebtMask = 0u;
}

void ndsAudioFgmLoadFenced(void)
{
    FILE *file;
    u8 *header = sNdsAudioFgmPack;
    long file_size;
    u32 i;

    if (gNdsAudioFgmLoaded != 0u)
    {
        return;
    }
    file = fopen(NDS_AUDIO_FGM_PATH, "rb");
    if (file == NULL)
    {
        gNdsAudioFgmOpenFailCount++;
        return;
    }
    if ((fseek(file, 0, SEEK_END) != 0) ||
        ((file_size = ftell(file)) != (long)NDS_AUDIO_FGM_PACK_BYTES) ||
        (fseek(file, 0, SEEK_SET) != 0))
    {
        fclose(file);
        gNdsAudioFgmFormatFailCount++;
        return;
    }
    if (fread(sNdsAudioFgmPack, 1, NDS_AUDIO_FGM_PACK_BYTES, file) !=
        NDS_AUDIO_FGM_PACK_BYTES)
    {
        fclose(file);
        gNdsAudioFgmReadFailCount++;
        return;
    }
    fclose(file);

    if ((memcmp(header, "FGM1", 4) != 0) ||
        (ndsAudioFgmReadLe16(&header[4]) != 2u) ||
        (ndsAudioFgmReadLe16(&header[6]) != NDS_AUDIO_FGM_PHASE_COUNT) ||
        (ndsAudioFgmReadLe32(&header[8]) != NDS_AUDIO_FGM_PACK_BYTES) ||
        (ndsAudioFgmReadLe32(&header[12]) !=
         NDS_AUDIO_FGM_PACK_MAPPING_SHA256_LO))
    {
        gNdsAudioFgmFormatFailCount++;
        return;
    }
    for (i = 0u; i < NDS_AUDIO_FGM_PHASE_COUNT; i++)
    {
        const u8 *raw = &sNdsAudioFgmPack[
            NDS_AUDIO_FGM_PACK_HEADER_BYTES +
            (i * NDS_AUDIO_FGM_PACK_ENTRY_BYTES)];

        if (ndsAudioFgmValidateEntry(i, raw) == FALSE)
        {
            memset(sNdsAudioFgmEntries, 0,
                   sizeof(sNdsAudioFgmEntries));
            gNdsAudioFgmFormatFailCount++;
            return;
        }
    }
    if ((sNdsAudioFgmEntries[0].data_offset !=
         NDS_AUDIO_FGM_PACK_DATA_OFFSET) ||
        ((sNdsAudioFgmEntries[NDS_AUDIO_FGM_PHASE_COUNT - 1u].data_offset +
          sNdsAudioFgmEntries[NDS_AUDIO_FGM_PHASE_COUNT - 1u].data_bytes) !=
         sNdsAudioFgmEntries[0].envelope_offset) ||
        ((sNdsAudioFgmEntries[0].envelope_offset +
          ((u32)sNdsAudioFgmEntries[0].envelope_count *
           NDS_AUDIO_FGM_ENVELOPE_POINT_BYTES)) !=
         NDS_AUDIO_FGM_PACK_BYTES))
    {
        memset(sNdsAudioFgmEntries, 0, sizeof(sNdsAudioFgmEntries));
        gNdsAudioFgmFormatFailCount++;
        return;
    }

    DC_FlushRange(sNdsAudioFgmPack, NDS_AUDIO_FGM_PACK_BYTES);
    gNdsAudioFgmLoaded = 1u;
    gNdsAudioFgmResidentBytes = NDS_AUDIO_FGM_PACK_BYTES;
    gNdsAudioFgmSupportedCount = NDS_AUDIO_FGM_PHASE_COUNT;
    gNdsAudioFgmNonReuseCapacity = NDS_AUDIO_FGM_HANDLE_COUNT;
    gNdsAudioFgmMask |= NDS_AUDIO_FGM_MASK_PACK_LOADED;
    gNdsAudioFgmFidelityDebtMask =
        NDS_AUDIO_FGM_EXPECTED_FIDELITY_DEBT_MASK;
    gNdsAudioFgmResult = NDS_AUDIO_FGM_PASS;
}

void ndsAudioFgmUpdate(void)
{
    u32 now = cpuGetTiming();
    u32 i;

    for (i = 0u; i < NDS_AUDIO_FGM_HANDLE_COUNT; i++)
    {
        NDSAudioFgmHandle *handle = &sNdsAudioFgmHandles[i];

        if (handle->live != FALSE)
        {
            u32 elapsed_cpu_ticks = now - handle->start_tick;
            u32 elapsed_fgm_ticks = (u32)(
                ((u64)elapsed_cpu_ticks * 1000000u) /
                ((u64)BUS_CLOCK * NDS_AUDIO_FGM_TIMER_MICROSECONDS));

            while (handle->envelope_index < handle->envelope_count)
            {
                const u8 *point = &sNdsAudioFgmPack[
                    handle->envelope_offset +
                    ((u32)handle->envelope_index *
                     NDS_AUDIO_FGM_ENVELOPE_POINT_BYTES)];
                u16 point_tick = ndsAudioFgmReadLe16(point);

                if (elapsed_fgm_ticks < point_tick)
                {
                    break;
                }
                if ((handle->channel < 0) ||
                    (sNdsAudioFgmChannelOwners[(u32)handle->channel] !=
                     handle) ||
                    (sNdsAudioFgmChannelGenerations[(u32)handle->channel] !=
                     handle->generation))
                {
                    gNdsAudioFgmGenerationMismatchCount++;
                    ndsAudioFgmReleaseHandle(handle, FALSE);
                    break;
                }
                soundSetVolume(handle->channel, point[2]);
                handle->envelope_index++;
                gNdsAudioFgmEnvelopeStepCount++;
            }
            if ((handle->live != FALSE) &&
                ((s32)(now - handle->end_tick) >= 0))
            {
                ndsAudioFgmReleaseHandle(handle, TRUE);
                gNdsAudioFgmDurationStopCount++;
            }
        }
    }
}

void ndsAudioFgmStopAll(void)
{
    u32 i;

    gNdsAudioFgmStopAllCalls++;
    for (i = 0u; i < NDS_AUDIO_FGM_HANDLE_COUNT; i++)
    {
        if (sNdsAudioFgmHandles[i].live != FALSE)
        {
            ndsAudioFgmReleaseHandle(&sNdsAudioFgmHandles[i], TRUE);
        }
    }
}

void ndsAudioFgmStop(alSoundEffect *effect)
{
    NDSAudioFgmHandle *handle;

    gNdsAudioFgmStopCalls++;
    if (effect == NULL)
    {
        return;
    }
    handle = ndsAudioFgmHandleFromEffect(effect);
    if ((handle == NULL) || (handle->allocated == FALSE) ||
        (handle->live == FALSE))
    {
        gNdsAudioFgmStaleStopCount++;
        return;
    }
    ndsAudioFgmReleaseHandle(handle, TRUE);
}

alSoundEffect *ndsAudioFgmPlay(u16 fgm_id)
{
    NDSAudioFgmPackEntry *entry;
    NDSAudioFgmHandle *handle = NULL;
    s32 phase_index;
    s32 channel;
    u32 i;
    u32 duration_cpu_ticks;

    gNdsAudioFgmPlayCalls++;
    gNdsAudioFgmLastID = fgm_id;
    ndsAudioFgmUpdate();
    entry = ndsAudioFgmFindEntry(fgm_id);
    if (entry == NULL)
    {
        if (ndsAudioFgmIDIsIncluded(fgm_id) != FALSE)
        {
            gNdsAudioFgmIncludedLookupFailCount++;
            gNdsAudioFgmPlayFailCount++;
        }
        else
        {
            gNdsAudioFgmUnsupportedCallCount++;
        }
        return NULL;
    }
    if ((gNdsAudioFgmLoaded == 0u) ||
        (gNdsAudioFgmResult != NDS_AUDIO_FGM_PASS))
    {
        gNdsAudioFgmIncludedLookupFailCount++;
        gNdsAudioFgmPlayFailCount++;
        return NULL;
    }
    for (i = 0u; i < NDS_AUDIO_FGM_HANDLE_COUNT; i++)
    {
        if (sNdsAudioFgmHandles[i].allocated == FALSE)
        {
            handle = &sNdsAudioFgmHandles[i];
            break;
        }
    }
    if (handle == NULL)
    {
        gNdsAudioFgmPoolExhaustCount++;
        gNdsAudioFgmPlayFailCount++;
        return NULL;
    }

    soundEnable();
    channel = soundPlaySample(
        &sNdsAudioFgmPack[entry->data_offset], SoundFormat_ADPCM,
        entry->data_bytes, entry->frequency, entry->volume, entry->pan,
        ((entry->flags & 1u) != 0u), 0u);
    if ((channel < 0) || (channel >= (s32)NDS_AUDIO_FGM_CHANNEL_COUNT))
    {
        gNdsAudioFgmPlayFailCount++;
        return NULL;
    }
    if (sNdsAudioFgmChannelOwners[channel] != NULL)
    {
        soundKill(channel);
        gNdsAudioFgmGenerationMismatchCount++;
        gNdsAudioFgmPlayFailCount++;
        return NULL;
    }

    memset(&handle->effect, 0, sizeof(handle->effect));
    handle->effect.sfx_id = fgm_id;
    handle->effect.sfx_max = fgm_id;
    handle->effect.balance = entry->pan;
    handle->generation = sNdsAudioFgmNextGeneration++;
    if (sNdsAudioFgmNextGeneration == 0u)
    {
        sNdsAudioFgmNextGeneration = 1u;
    }
    duration_cpu_ticks = (u32)(((u64)BUS_CLOCK * entry->duration_ticks *
                                NDS_AUDIO_FGM_TIMER_MICROSECONDS) /
                               1000000u);
    handle->start_tick = cpuGetTiming();
    handle->end_tick = handle->start_tick + duration_cpu_ticks;
    handle->envelope_offset = entry->envelope_offset;
    handle->envelope_count = entry->envelope_count;
    handle->envelope_index = 0u;
    handle->channel = (s8)channel;
    handle->allocated = TRUE;
    handle->live = TRUE;
    sNdsAudioFgmChannelOwners[channel] = handle;
    sNdsAudioFgmChannelGenerations[channel] = handle->generation;

    gNdsAudioFgmSupportedPlayCount++;
    gNdsAudioFgmAllocatedHandles++;
    gNdsAudioFgmActiveHandles++;
    if (gNdsAudioFgmActiveHandles > gNdsAudioFgmMaxActiveHandles)
    {
        gNdsAudioFgmMaxActiveHandles = gNdsAudioFgmActiveHandles;
    }
    gNdsAudioFgmChannelMask |= 1u << channel;
    gNdsAudioFgmLastChannel = (u32)channel;
    gNdsAudioFgmLastGeneration = handle->generation;
    gNdsAudioFgmMask |= NDS_AUDIO_FGM_MASK_SUPPORTED_PLAY;
    if ((entry->flags & 1u) != 0u)
    {
        gNdsAudioFgmLoopPlayCount++;
        gNdsAudioFgmMask |= NDS_AUDIO_FGM_MASK_LOOP_PLAY;
    }
    phase_index = ndsAudioFgmPhaseIndex(fgm_id);
    if (phase_index >= 0)
    {
        gNdsAudioFgmPhasePlayCounts[phase_index]++;
        gNdsAudioFgmPhasePlayMask |= 1u << phase_index;
        if (gNdsAudioFgmPhasePlayMask ==
            NDS_AUDIO_FGM_PHASE_COMPLETE_MASK)
        {
            gNdsAudioFgmMask |= NDS_AUDIO_FGM_MASK_PHASE_COMPLETE;
        }
    }
    return &handle->effect;
}
