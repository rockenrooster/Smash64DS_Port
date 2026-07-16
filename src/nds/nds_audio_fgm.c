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
     (NDS_AUDIO_FGM_ENTRY_COUNT * NDS_AUDIO_FGM_PACK_ENTRY_BYTES))
#define NDS_AUDIO_FGM_HANDLE_COUNT NDS_AUDIO_FGM_HANDLE_CAPACITY
#define NDS_AUDIO_FGM_CHANNEL_COUNT 16u
#define NDS_AUDIO_FGM_TIMER_MICROSECONDS 5750u

#define NDS_AUDIO_FGM_MASK_PACK_LOADED (1u << 0)
#define NDS_AUDIO_FGM_MASK_SUPPORTED_PLAY (1u << 1)
#define NDS_AUDIO_FGM_MASK_LOOP_PLAY (1u << 2)
#define NDS_AUDIO_FGM_MASK_PHASE_COMPLETE (1u << 3)

#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
#define NDS_AUDIO_FGM_ACK_RELEASE_PARAMS \
    , u32 release_reason, u32 service_tick
#define NDS_AUDIO_FGM_ACK_RELEASE_ARGS(reason, service_tick) \
    , reason, service_tick
#else
#define NDS_AUDIO_FGM_ACK_RELEASE_PARAMS
#define NDS_AUDIO_FGM_ACK_RELEASE_ARGS(reason, service_tick)
#endif

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
    u16 loop_point_words;
} NDSAudioFgmPackEntry;

typedef struct NDSAudioFgmHandle {
    alSoundEffect effect;
    u32 generation;
    u32 start_tick;
    u32 end_tick;
    u32 envelope_offset;
    u16 envelope_count;
    u16 envelope_index;
    u16 fgm_id;
    s8 channel;
    u8 allocated;
    u8 live;
    u8 ever_allocated;
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
volatile u32 gNdsAudioFgmKoPlayMask;
volatile u32 gNdsAudioFgmKoPlayCounts[NDS_AUDIO_FGM_KO_COUNT];
volatile u32 gNdsAudioFgmKoTraceCount;
volatile u32 gNdsAudioFgmKoTrace[NDS_AUDIO_FGM_KO_TRACE_CAPACITY];
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
volatile u32 gNdsAudioFgmLastInstanceToken;
volatile u32 gNdsAudioFgmInstanceTokenWrapCount;
volatile u32 gNdsAudioFgmPoolExhaustCount;
volatile u32 gNdsAudioFgmHandleAcquireCount;
volatile u32 gNdsAudioFgmHandleReleaseCount;
volatile u32 gNdsAudioFgmHandleRecycleCount;
volatile u32 gNdsAudioFgmHandleCapacity;
volatile u32 gNdsAudioFgmEnvelopeStepCount;
volatile u32 gNdsAudioFgmFidelityDebtMask;
#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
volatile NDSAudioFgmArm7AckTrace gNdsAudioFgmArm7AckTrace;
#endif

static u8 sNdsAudioFgmPack[NDS_AUDIO_FGM_PACK_BYTES]
    __attribute__((aligned(4)));
static NDSAudioFgmPackEntry
    sNdsAudioFgmEntries[NDS_AUDIO_FGM_ENTRY_COUNT];
static NDSAudioFgmHandle sNdsAudioFgmHandles[NDS_AUDIO_FGM_HANDLE_COUNT];
static NDSAudioFgmHandle *sNdsAudioFgmChannelOwners[NDS_AUDIO_FGM_CHANNEL_COUNT];
static u32 sNdsAudioFgmChannelGenerations[NDS_AUDIO_FGM_CHANNEL_COUNT];
static u32 sNdsAudioFgmNextGeneration = 1u;
#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
static u32 sNdsAudioFgmArm7AckSequence;
#endif
static u16 sNdsAudioFgmInstanceToken;

_Static_assert(NDS_AUDIO_FGM_PACK_DATA_OFFSET == 816u,
               "FGM pack header layout changed");
_Static_assert(NDS_AUDIO_FGM_PACK_BYTES <= (160u * 1024u),
               "FGM pack exceeds its resident-memory gate");
_Static_assert(offsetof(NDSAudioFgmHandle, effect) == 0u,
               "BattleShip audio handle must be the backend handle prefix");
_Static_assert(offsetof(alSoundEffect, sfx_id) == 0x26u,
               "BattleShip FGM instance-token field moved");
_Static_assert(NDS_AUDIO_FGM_HANDLE_COUNT >= 3u,
               "FGM pool must support the source KO call burst");
_Static_assert(NDS_AUDIO_FGM_CHANNEL_COUNT == SOUND_NUM_CHANNELS,
               "FGM channel count must match the Calico mixer");
#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
_Static_assert(sizeof(NDSAudioFgmArm7AckEvent) == 32u,
               "FGM ARM7 ACK event layout changed");
_Static_assert(offsetof(NDSAudioFgmArm7AckTrace, events) == 48u,
               "FGM ARM7 ACK event array moved");
_Static_assert(sizeof(NDSAudioFgmArm7AckTrace) == 112u,
               "FGM ARM7 ACK trace layout changed");
#endif

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
    case nSYAudioFGMFoxLanding:
    case nSYAudioVoiceFoxJumpAerial:
    case nSYAudioVoiceFoxEscape:
    case nSYAudioVoiceFoxSmash1:
    case nSYAudioVoiceMarioSmash2:
    case nSYAudioVoiceMarioDead:
    case nSYAudioFGMMarioDeadSlam:
    case nSYAudioVoiceFoxDead:
    case nSYAudioFGMFoxDeadSlam:
    case nSYAudioFGMFoxDownBounce:
    case nSYAudioFGMDeadExplodeL:
    case nSYAudioFGMMarioLanding:
    case nSYAudioVoiceMarioSmash1:
    case nSYAudioVoiceMarioJump:
    case nSYAudioFGMLightSwingM:
    case nSYAudioFGMLightSwingS:
    case nSYAudioFGMFoxAttackAirLw:
    case nSYAudioFGMMarioSpecialN:
    case nSYAudioFGMMarioUnkSwing1:
    case nSYAudioFGMMarioUnkSwing2:
        return TRUE;
    default:
        return FALSE;
    }
}

static s32 ndsAudioFgmKoIndex(u16 id)
{
    switch (id)
    {
    case nSYAudioVoiceMarioDead:
        return 0;
    case nSYAudioFGMMarioDeadSlam:
        return 1;
    case nSYAudioVoiceFoxDead:
        return 2;
    case nSYAudioFGMFoxDeadSlam:
        return 3;
    case nSYAudioFGMDeadExplodeL:
        return 4;
    default:
        return -1;
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

    for (i = 0u; i < NDS_AUDIO_FGM_ENTRY_COUNT; i++)
    {
        if (sNdsAudioFgmEntries[i].id == id)
        {
            return &sNdsAudioFgmEntries[i];
        }
    }
    return NULL;
}

static u16 ndsAudioFgmNextInstanceToken(void)
{
    sNdsAudioFgmInstanceToken++;
    if (sNdsAudioFgmInstanceToken == 0u)
    {
        sNdsAudioFgmInstanceToken++;
        gNdsAudioFgmInstanceTokenWrapCount++;
    }
    return sNdsAudioFgmInstanceToken;
}

#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
static s32 ndsAudioFgmIsArm7AckTarget(u16 fgm_id)
{
    return (fgm_id == nSYAudioVoicePublicExcited) ? TRUE : FALSE;
}

static void ndsAudioFgmArm7AckTraceBegin(
    const NDSAudioFgmHandle *handle, const NDSAudioFgmPackEntry *entry)
{
    memset((void *)&gNdsAudioFgmArm7AckTrace, 0,
           sizeof(gNdsAudioFgmArm7AckTrace));
    if (++sNdsAudioFgmArm7AckSequence == 0u)
    {
        sNdsAudioFgmArm7AckSequence = 1u;
    }
    gNdsAudioFgmArm7AckTrace.sequence = sNdsAudioFgmArm7AckSequence;
    gNdsAudioFgmArm7AckTrace.fgm_id = handle->fgm_id;
    gNdsAudioFgmArm7AckTrace.generation = handle->generation;
    gNdsAudioFgmArm7AckTrace.channel = (u32)handle->channel;
    gNdsAudioFgmArm7AckTrace.instance_token = handle->effect.sfx_id;
    gNdsAudioFgmArm7AckTrace.handle_start_tick = handle->start_tick;
    gNdsAudioFgmArm7AckTrace.handle_end_tick = handle->end_tick;
    gNdsAudioFgmArm7AckTrace.duration_ticks = entry->duration_ticks;
    gNdsAudioFgmArm7AckTrace.envelope_count = entry->envelope_count;
}

static void ndsAudioFgmArm7AckTraceRecord(
    const NDSAudioFgmHandle *handle, u32 kind, u32 source_tick, u32 value,
    u32 service_tick, u32 command_tick, u32 command_return_tick,
    u32 acknowledge_tick, u32 active_channels)
{
    volatile NDSAudioFgmArm7AckEvent *event;
    u32 event_index = gNdsAudioFgmArm7AckTrace.event_count;

    if ((gNdsAudioFgmArm7AckTrace.fgm_id != handle->fgm_id) ||
        (gNdsAudioFgmArm7AckTrace.generation != handle->generation) ||
        (gNdsAudioFgmArm7AckTrace.channel != (u32)handle->channel))
    {
        gNdsAudioFgmArm7AckTrace.mismatch_count++;
        return;
    }
    if (event_index >= NDS_AUDIO_FGM_ARM7_ACK_EVENT_CAPACITY)
    {
        gNdsAudioFgmArm7AckTrace.overflow_count++;
        return;
    }

    event = &gNdsAudioFgmArm7AckTrace.events[event_index];
    event->kind = kind;
    event->source_tick = source_tick;
    event->value = value;
    event->service_tick = service_tick;
    event->command_tick = command_tick;
    event->command_return_tick = command_return_tick;
    event->acknowledge_tick = acknowledge_tick;
    event->active_channels = active_channels;
    gNdsAudioFgmArm7AckTrace.event_count = event_index + 1u;
}
#endif

static void ndsAudioFgmReleaseHandle(
    NDSAudioFgmHandle *handle, s32 kill_channel
    NDS_AUDIO_FGM_ACK_RELEASE_PARAMS)
{
    s32 channel = handle->channel;

    if (handle->allocated == FALSE)
    {
        return;
    }

    if ((channel >= 0) && (channel < (s32)NDS_AUDIO_FGM_CHANNEL_COUNT) &&
        (sNdsAudioFgmChannelOwners[channel] == handle) &&
        (sNdsAudioFgmChannelGenerations[channel] == handle->generation))
    {
        if (kill_channel != FALSE)
        {
#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
            if ((release_reason == NDS_AUDIO_FGM_RELEASE_REASON_DURATION) &&
                (ndsAudioFgmIsArm7AckTarget(handle->fgm_id) != FALSE))
            {
                u32 command_tick = cpuGetTiming();
                u32 command_return_tick;
                u32 active_channels;
                u32 acknowledge_tick;

                soundKill(channel);
                command_return_tick = cpuGetTiming();
                active_channels = (u32)soundGetActiveChannels();
                acknowledge_tick = cpuGetTiming();
                ndsAudioFgmArm7AckTraceRecord(
                    handle, NDS_AUDIO_FGM_ARM7_ACK_KIND_STOP,
                    gNdsAudioFgmArm7AckTrace.duration_ticks,
                    release_reason, service_tick, command_tick,
                    command_return_tick, acknowledge_tick, active_channels);
            }
            else
            {
#endif
                soundKill(channel);
#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
            }
#endif
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
        if (gNdsAudioFgmActiveHandles != 0u)
        {
            gNdsAudioFgmActiveHandles--;
        }
    }
    handle->effect.sfx_id = 0u;
    handle->fgm_id = 0u;
    handle->allocated = FALSE;
    gNdsAudioFgmHandleReleaseCount++;
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
    static const u16 expected_ids[NDS_AUDIO_FGM_ENTRY_COUNT] = {
        nSYAudioVoicePublicExcited,
        nSYAudioVoiceAnnounceThree,
        nSYAudioVoiceAnnounceTwo,
        nSYAudioVoiceAnnounceOne,
        nSYAudioVoiceAnnounceGo,
        nSYAudioFGMFoxLanding,
        nSYAudioVoiceFoxJumpAerial,
        nSYAudioVoiceFoxEscape,
        nSYAudioVoiceFoxSmash1,
        nSYAudioVoiceMarioSmash2,
        nSYAudioVoiceMarioDead,
        nSYAudioFGMMarioDeadSlam,
        nSYAudioVoiceFoxDead,
        nSYAudioFGMFoxDeadSlam,
        nSYAudioFGMFoxDownBounce,
        nSYAudioFGMDeadExplodeL,
        nSYAudioFGMMarioLanding,
        nSYAudioVoiceMarioSmash1,
        nSYAudioVoiceMarioJump,
        nSYAudioFGMLightSwingM,
        nSYAudioFGMLightSwingS,
        nSYAudioFGMFoxAttackAirLw,
        nSYAudioFGMMarioSpecialN,
        nSYAudioFGMMarioUnkSwing1,
        nSYAudioFGMMarioUnkSwing2
    };
    static const u16 expected_frequencies[NDS_AUDIO_FGM_ENTRY_COUNT] = {
        15102u, 16000u, 16000u, 16000u, 16000u, 35919u, 16000u,
        16000u, 16000u, 16280u, 16009u, 16951u, 16000u, 16951u,
        16000u, 8476u,
        35919u, 15111u, 15455u,
        39170u, 32938u, 36971u, 32000u, 32938u, 36971u
    };
    static const u16 expected_durations[NDS_AUDIO_FGM_ENTRY_COUNT] = {
        1200u, 99u, 100u, 85u, 150u, 3u, 45u, 65u, 46u, 236u,
        96u, 53u, 120u, 53u, 25u, 300u, 3u, 96u, 96u,
        35u, 35u, 25u, 15u, 27u, 25u
    };
    static const u8 expected_volumes[NDS_AUDIO_FGM_ENTRY_COUNT] = {
        92u, 106u, 106u, 111u, 124u, 21u, 76u, 42u, 108u, 86u,
        95u, 30u, 114u, 30u, 20u, 124u, 21u, 95u, 84u,
        87u, 73u, 73u, 121u, 73u, 73u
    };
    static const u16 expected_sounds[NDS_AUDIO_FGM_ENTRY_COUNT] = {
        320u, 208u, 209u, 210u, 211u, 1u, 108u, 102u, 105u, 174u,
        183u, 28u, 104u, 28u, 28u, 0u, 1u, 173u, 179u,
        71u, 71u, 71u, 19u, 71u, 71u
    };
    static const u32 expected_data_bytes[NDS_AUDIO_FGM_ENTRY_COUNT] = {
        52108u, 4560u, 4604u, 3916u, 6904u, 316u, 1884u, 1460u,
        844u, 8620u, 4424u, 3348u, 4908u, 3348u, 3348u, 7460u,
        316u, 1828u, 1676u,
        2596u, 2596u, 2596u, 1092u, 2596u, 2596u
    };
    static const u32 expected_sample_counts[NDS_AUDIO_FGM_ENTRY_COUNT] = {
        104204u, 9109u, 9201u, 7821u, 13801u, 621u, 3760u, 2912u,
        1680u, 17232u, 8838u, 6688u, 9808u, 6688u, 6688u, 14913u,
        621u, 3648u, 3344u,
        5184u, 5184u, 5184u, 2176u, 5184u, 5184u
    };
    static const u16 expected_envelope_counts[NDS_AUDIO_FGM_ENTRY_COUNT] = {
        0u, 0u, 0u, 0u, 0u, 2u, 0u, 5u, 0u, 1u, 0u, 3u, 5u,
        3u, 2u, 13u, 2u, 0u, 3u,
        0u, 0u, 0u, 0u, 0u, 0u
    };
    NDSAudioFgmPackEntry *entry = &sNdsAudioFgmEntries[index];
    u32 expected_flags = 0u;
    u32 expected_loop_point_words = 0u;
    u32 next_unique_data_offset = NDS_AUDIO_FGM_PACK_DATA_OFFSET;
    s32 is_duplicate = FALSE;
    u32 prior_index;

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
    entry->loop_point_words = ndsAudioFgmReadLe16(&raw[30]);

    if ((entry->id != expected_ids[index]) ||
        (entry->flags != expected_flags) ||
        (entry->frequency != expected_frequencies[index]) ||
        (entry->duration_ticks != expected_durations[index]) ||
        (entry->volume != expected_volumes[index]) ||
        (entry->pan != 64u) ||
        (entry->source_sound_index != expected_sounds[index]) ||
        (entry->data_bytes != expected_data_bytes[index]) ||
        (entry->sample_count != expected_sample_counts[index]) ||
        (entry->envelope_count != expected_envelope_counts[index]) ||
        (entry->loop_point_words != expected_loop_point_words) ||
        ((entry->data_bytes & 3u) != 0u) || (entry->volume > 127u) ||
        (((u32)entry->loop_point_words * 4u) >= entry->data_bytes) ||
        (entry->data_offset < NDS_AUDIO_FGM_PACK_DATA_OFFSET) ||
        ((entry->data_offset & 3u) != 0u) ||
        (entry->data_offset > NDS_AUDIO_FGM_PACK_BYTES) ||
        (entry->data_bytes >
         (NDS_AUDIO_FGM_PACK_BYTES - entry->data_offset)))
    {
        return FALSE;
    }
    for (prior_index = 0u; prior_index < index; prior_index++)
    {
        const NDSAudioFgmPackEntry *prior =
            &sNdsAudioFgmEntries[prior_index];
        u32 prior_end = prior->data_offset + prior->data_bytes;

        if (prior_end > next_unique_data_offset)
        {
            next_unique_data_offset = prior_end;
        }
        if (entry->data_offset == prior->data_offset)
        {
            if ((entry->data_bytes != prior->data_bytes) ||
                (entry->sample_count != prior->sample_count) ||
                (entry->loop_point_words != prior->loop_point_words))
            {
                return FALSE;
            }
            is_duplicate = TRUE;
        }
    }
    if ((is_duplicate == FALSE) &&
        (entry->data_offset != next_unique_data_offset))
    {
        return FALSE;
    }
    if (entry->envelope_count != 0u)
    {
        u32 point_index;
        u16 previous_tick = 0u;

        if (((entry->envelope_offset & 3u) != 0u) ||
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
    else if (entry->envelope_offset != 0u)
    {
        return FALSE;
    }
    return TRUE;
}

void ndsAudioFgmDiagnosticsReset(void)
{
    u32 i;

    /* BattleShip stores a nonzero instance token in sfx_id, snapshots that
     * token in source-side holders, and compares it before stopping a handle.
     * Keep that contract: completed handles clear the token and return to the
     * reusable backend pool. */
    for (i = 0u; i < NDS_AUDIO_FGM_HANDLE_COUNT; i++)
    {
        if (sNdsAudioFgmHandles[i].live != FALSE)
        {
            ndsAudioFgmReleaseHandle(
                &sNdsAudioFgmHandles[i], TRUE
                NDS_AUDIO_FGM_ACK_RELEASE_ARGS(
                    NDS_AUDIO_FGM_RELEASE_REASON_RESET, 0u));
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
#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
    memset((void *)&gNdsAudioFgmArm7AckTrace, 0,
           sizeof(gNdsAudioFgmArm7AckTrace));
    sNdsAudioFgmArm7AckSequence = 0u;
#endif

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
    gNdsAudioFgmKoPlayMask = 0u;
    memset((void *)gNdsAudioFgmKoPlayCounts, 0,
           sizeof(gNdsAudioFgmKoPlayCounts));
    gNdsAudioFgmKoTraceCount = 0u;
    memset((void *)gNdsAudioFgmKoTrace, 0,
           sizeof(gNdsAudioFgmKoTrace));
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
    gNdsAudioFgmLastInstanceToken = 0u;
    gNdsAudioFgmInstanceTokenWrapCount = 0u;
    gNdsAudioFgmPoolExhaustCount = 0u;
    gNdsAudioFgmHandleAcquireCount = 0u;
    gNdsAudioFgmHandleReleaseCount = 0u;
    gNdsAudioFgmHandleRecycleCount = 0u;
    gNdsAudioFgmHandleCapacity = NDS_AUDIO_FGM_HANDLE_COUNT;
    gNdsAudioFgmEnvelopeStepCount = 0u;
    gNdsAudioFgmFidelityDebtMask = 0u;
}

void ndsAudioFgmLoadFenced(void)
{
    FILE *file;
    u8 *header = sNdsAudioFgmPack;
    long file_size;
    u32 i;
    u32 sample_end = NDS_AUDIO_FGM_PACK_DATA_OFFSET;
    u32 envelope_cursor;

#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
    /* Keep Calico's low-load manual mode for the two explicit, blocking
     * ID-626 command acknowledgments in this diagnostic-only build. */
    soundSetAutoUpdate(false);
#endif
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
        (ndsAudioFgmReadLe16(&header[4]) != 3u) ||
        (ndsAudioFgmReadLe16(&header[6]) != NDS_AUDIO_FGM_ENTRY_COUNT) ||
        (ndsAudioFgmReadLe32(&header[8]) != NDS_AUDIO_FGM_PACK_BYTES) ||
        (ndsAudioFgmReadLe32(&header[12]) !=
         NDS_AUDIO_FGM_PACK_MAPPING_SHA256_LO))
    {
        gNdsAudioFgmFormatFailCount++;
        return;
    }
    for (i = 0u; i < NDS_AUDIO_FGM_ENTRY_COUNT; i++)
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
    for (i = 0u; i < NDS_AUDIO_FGM_ENTRY_COUNT; i++)
    {
        u32 entry_end = sNdsAudioFgmEntries[i].data_offset +
                        sNdsAudioFgmEntries[i].data_bytes;

        if (entry_end > sample_end)
        {
            sample_end = entry_end;
        }
    }
    envelope_cursor = sample_end;
    for (i = 0u; i < NDS_AUDIO_FGM_ENTRY_COUNT; i++)
    {
        const NDSAudioFgmPackEntry *entry = &sNdsAudioFgmEntries[i];

        if (entry->envelope_count != 0u)
        {
            if (entry->envelope_offset != envelope_cursor)
            {
                break;
            }
            envelope_cursor += (u32)entry->envelope_count *
                               NDS_AUDIO_FGM_ENVELOPE_POINT_BYTES;
        }
    }
    if ((sNdsAudioFgmEntries[0].data_offset !=
         NDS_AUDIO_FGM_PACK_DATA_OFFSET) ||
        (i != NDS_AUDIO_FGM_ENTRY_COUNT) ||
        (envelope_cursor != NDS_AUDIO_FGM_PACK_BYTES))
    {
        memset(sNdsAudioFgmEntries, 0, sizeof(sNdsAudioFgmEntries));
        gNdsAudioFgmFormatFailCount++;
        return;
    }

    DC_FlushRange(sNdsAudioFgmPack, NDS_AUDIO_FGM_PACK_BYTES);
    gNdsAudioFgmLoaded = 1u;
    gNdsAudioFgmResidentBytes = NDS_AUDIO_FGM_PACK_BYTES;
    gNdsAudioFgmSupportedCount = NDS_AUDIO_FGM_ENTRY_COUNT;
    gNdsAudioFgmHandleCapacity = NDS_AUDIO_FGM_HANDLE_COUNT;
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
                    ndsAudioFgmReleaseHandle(
                        handle, FALSE
                        NDS_AUDIO_FGM_ACK_RELEASE_ARGS(
                            NDS_AUDIO_FGM_RELEASE_REASON_GENERATION_LOST,
                            now));
                    break;
                }
                soundSetVolume(handle->channel, point[2]);
                handle->envelope_index++;
                gNdsAudioFgmEnvelopeStepCount++;
            }
            if ((handle->live != FALSE) &&
                ((s32)(now - handle->end_tick) >= 0))
            {
                ndsAudioFgmReleaseHandle(
                    handle, TRUE
                    NDS_AUDIO_FGM_ACK_RELEASE_ARGS(
                        NDS_AUDIO_FGM_RELEASE_REASON_DURATION, now));
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
            ndsAudioFgmReleaseHandle(
                &sNdsAudioFgmHandles[i], TRUE
                NDS_AUDIO_FGM_ACK_RELEASE_ARGS(
                    NDS_AUDIO_FGM_RELEASE_REASON_STOP_ALL, 0u));
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
    ndsAudioFgmReleaseHandle(
        handle, TRUE
        NDS_AUDIO_FGM_ACK_RELEASE_ARGS(
            NDS_AUDIO_FGM_RELEASE_REASON_EXPLICIT, 0u));
}

alSoundEffect *ndsAudioFgmPlay(u16 fgm_id)
{
    NDSAudioFgmPackEntry *entry;
    NDSAudioFgmHandle *handle = NULL;
    s32 phase_index;
    s32 ko_index;
    s32 channel;
    u32 i;
    u32 duration_cpu_ticks;
#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
    u32 play_command_tick = 0u;
    u32 play_command_return_tick = 0u;
#endif

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
#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
    if (ndsAudioFgmIsArm7AckTarget(fgm_id) != FALSE)
    {
        play_command_tick = cpuGetTiming();
    }
#endif
    channel = soundPlaySample(
        &sNdsAudioFgmPack[entry->data_offset], SoundFormat_ADPCM,
        entry->data_bytes - ((u32)entry->loop_point_words * 4u),
        entry->frequency, entry->volume, entry->pan,
        ((entry->flags & 1u) != 0u), entry->loop_point_words);
#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
    if (ndsAudioFgmIsArm7AckTarget(fgm_id) != FALSE)
    {
        play_command_return_tick = cpuGetTiming();
    }
#endif
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
    handle->effect.sfx_id = ndsAudioFgmNextInstanceToken();
    handle->effect.balance = entry->pan;
    handle->fgm_id = fgm_id;
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
    if (handle->ever_allocated != FALSE)
    {
        gNdsAudioFgmHandleRecycleCount++;
    }
    handle->ever_allocated = TRUE;
    handle->allocated = TRUE;
    handle->live = TRUE;
    sNdsAudioFgmChannelOwners[channel] = handle;
    sNdsAudioFgmChannelGenerations[channel] = handle->generation;

#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
    if (ndsAudioFgmIsArm7AckTarget(fgm_id) != FALSE)
    {
        u32 active_channels;
        u32 acknowledge_tick;

        ndsAudioFgmArm7AckTraceBegin(handle, entry);
        active_channels = (u32)soundGetActiveChannels();
        acknowledge_tick = cpuGetTiming();
        ndsAudioFgmArm7AckTraceRecord(
            handle, NDS_AUDIO_FGM_ARM7_ACK_KIND_PLAY, 0u, entry->volume,
            handle->start_tick, play_command_tick, play_command_return_tick,
            acknowledge_tick, active_channels);
    }
#endif

    gNdsAudioFgmSupportedPlayCount++;
    gNdsAudioFgmHandleAcquireCount++;
    gNdsAudioFgmActiveHandles++;
    if (gNdsAudioFgmActiveHandles > gNdsAudioFgmMaxActiveHandles)
    {
        gNdsAudioFgmMaxActiveHandles = gNdsAudioFgmActiveHandles;
    }
    gNdsAudioFgmChannelMask |= 1u << channel;
    gNdsAudioFgmLastChannel = (u32)channel;
    gNdsAudioFgmLastGeneration = handle->generation;
    gNdsAudioFgmLastInstanceToken = handle->effect.sfx_id;
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
    ko_index = ndsAudioFgmKoIndex(fgm_id);
    if (ko_index >= 0)
    {
        gNdsAudioFgmKoPlayCounts[ko_index]++;
        gNdsAudioFgmKoPlayMask |= 1u << ko_index;
        if (gNdsAudioFgmKoTraceCount < NDS_AUDIO_FGM_KO_TRACE_CAPACITY)
        {
            gNdsAudioFgmKoTrace[gNdsAudioFgmKoTraceCount++] = fgm_id;
        }
    }
    return &handle->effect;
}
