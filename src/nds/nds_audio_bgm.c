#include <calico.h>
#include <nds.h>
#include <stdio.h>
#include <string.h>

#include <gm/gmsound.h>
#include <nds/nds_audio_bgm.h>

#define NDS_AUDIO_BGM_PATH_PUPUPU "nitro:/audio/bgm_pupupu_ima.bin"
#define NDS_AUDIO_BGM_PATH_WIN_MARIO "nitro:/audio/bgm_win_mario_ima.bin"
#define NDS_AUDIO_BGM_PATH_WIN_FOX "nitro:/audio/bgm_win_fox_ima.bin"
#define NDS_AUDIO_BGM_PATH_RESULTS "nitro:/audio/bgm_results_ima.bin"
#define NDS_AUDIO_BGM_PATH_MODE_SELECT "nitro:/audio/bgm_mode_select_ima.bin"
#define NDS_AUDIO_BGM_PATH_BATTLE_SELECT "nitro:/audio/bgm_battle_select_ima.bin"
#define NDS_AUDIO_BGM_CHANNEL_BASE 14u
#define NDS_AUDIO_BGM_CHANNEL_MASK (3u << NDS_AUDIO_BGM_CHANNEL_BASE)
#define NDS_AUDIO_BGM_TIMER 0u
#define NDS_AUDIO_BGM_WORKER_STACK_BYTES 1024u
#define NDS_AUDIO_BGM_NO_LOOP 0xffffffffu

_Static_assert(NDS_AUDIO_BGM_RESIDENT_BYTES == 16392u,
               "BGM ADPCM residency changed");
_Static_assert((NDS_AUDIO_BGM_PACKET_BYTES % 4u) == 0u,
               "BGM ADPCM packet must contain whole DS words");
_Static_assert(NDS_AUDIO_BGM_BYTES_PER_SECOND == 44100u,
               "BGM source-time byte rate changed");
_Static_assert(NDS_AUDIO_BGM_CHANNEL_BASE + NDS_AUDIO_BGM_BUFFER_COUNT <=
                   SOUND_NUM_CHANNELS,
               "BGM channels exceed the Calico mixer");

typedef struct NDSAudioBgmTrack {
    s32 id;
    const char *path;
    u32 stream_bytes;
    u32 loop_start_bytes;
    u32 asset_bytes;
    u32 packet_count;
    u32 loop_packet;
    u32 loop_record;
    s32 is_looping;
} NDSAudioBgmTrack;

/* P2-1L bug (b1), 2026-08-19: [loop_start_bytes, stream_bytes) is played
 * forever once the file runs out of packets (ndsAudioBgmReadPacket()
 * below seeks to loop_record/loop_packet). Both fields, and every derived
 * *_LOOP_PACKET/*_LOOP_RECORD/*_PACKET_COUNT constant below, come only
 * from scripts/sfx/bgm/render-audio-bgm-pupupu.py's collect_loop_metadata()
 * -- see its docstring for why a naive max() over every CSEQ channel's
 * raw loop marker is wrong (Mode Select had an outlier channel dominate
 * it) and why the render used to leave up to a second of true digital
 * silence inside the loop for every track (measured offline, RMS 0.0
 * right before the wrap). Never hand-adjust these constants; re-render
 * and re-pin (include/nds/nds_audio_bgm.h,
 * scripts/check-audio-bgm-derived-assets.ps1) instead. */
static const NDSAudioBgmTrack sNdsAudioBgmTracks[] = {
    {
        nSYAudioBGMPupupu,
        NDS_AUDIO_BGM_PATH_PUPUPU,
        NDS_AUDIO_BGM_PUPUPU_STREAM_BYTES,
        NDS_AUDIO_BGM_PUPUPU_LOOP_START_BYTES,
        NDS_AUDIO_BGM_PUPUPU_ASSET_BYTES,
        NDS_AUDIO_BGM_PUPUPU_PACKET_COUNT,
        NDS_AUDIO_BGM_PUPUPU_LOOP_PACKET,
        NDS_AUDIO_BGM_PUPUPU_LOOP_RECORD,
        TRUE
    },
    {
        nSYAudioBGMWinMario,
        NDS_AUDIO_BGM_PATH_WIN_MARIO,
        NDS_AUDIO_BGM_WIN_MARIO_STREAM_BYTES,
        0u,
        NDS_AUDIO_BGM_WIN_MARIO_ASSET_BYTES,
        NDS_AUDIO_BGM_WIN_MARIO_PACKET_COUNT,
        NDS_AUDIO_BGM_NO_LOOP,
        0u,
        FALSE
    },
    {
        nSYAudioBGMWinFox,
        NDS_AUDIO_BGM_PATH_WIN_FOX,
        NDS_AUDIO_BGM_WIN_FOX_STREAM_BYTES,
        0u,
        NDS_AUDIO_BGM_WIN_FOX_ASSET_BYTES,
        NDS_AUDIO_BGM_WIN_FOX_PACKET_COUNT,
        NDS_AUDIO_BGM_NO_LOOP,
        0u,
        FALSE
    },
    {
        nSYAudioBGMResults,
        NDS_AUDIO_BGM_PATH_RESULTS,
        NDS_AUDIO_BGM_RESULTS_STREAM_BYTES,
        NDS_AUDIO_BGM_RESULTS_LOOP_START_BYTES,
        NDS_AUDIO_BGM_RESULTS_ASSET_BYTES,
        NDS_AUDIO_BGM_RESULTS_PACKET_COUNT,
        NDS_AUDIO_BGM_RESULTS_LOOP_PACKET,
        NDS_AUDIO_BGM_RESULTS_LOOP_RECORD,
        TRUE
    },
    {
        nSYAudioBGMModeSelect,
        NDS_AUDIO_BGM_PATH_MODE_SELECT,
        NDS_AUDIO_BGM_MODE_SELECT_STREAM_BYTES,
        NDS_AUDIO_BGM_MODE_SELECT_LOOP_START_BYTES,
        NDS_AUDIO_BGM_MODE_SELECT_ASSET_BYTES,
        NDS_AUDIO_BGM_MODE_SELECT_PACKET_COUNT,
        NDS_AUDIO_BGM_MODE_SELECT_LOOP_PACKET,
        NDS_AUDIO_BGM_MODE_SELECT_LOOP_RECORD,
        TRUE
    },
    {
        nSYAudioBGMBattleSelect,
        NDS_AUDIO_BGM_PATH_BATTLE_SELECT,
        NDS_AUDIO_BGM_BATTLE_SELECT_STREAM_BYTES,
        NDS_AUDIO_BGM_BATTLE_SELECT_LOOP_START_BYTES,
        NDS_AUDIO_BGM_BATTLE_SELECT_ASSET_BYTES,
        NDS_AUDIO_BGM_BATTLE_SELECT_PACKET_COUNT,
        NDS_AUDIO_BGM_BATTLE_SELECT_LOOP_PACKET,
        NDS_AUDIO_BGM_BATTLE_SELECT_LOOP_RECORD,
        TRUE
    }
};

volatile u32 gNdsAudioBgmResult;
volatile u32 gNdsAudioBgmMask;
volatile u32 gNdsAudioBgmPlaying;
volatile u32 gNdsAudioBgmTrackID;
volatile u32 gNdsAudioBgmVolume;
volatile u32 gNdsAudioBgmPlayCalls;
volatile u32 gNdsAudioBgmStopCalls;
volatile u32 gNdsAudioBgmCheckCalls;
volatile u32 gNdsAudioBgmSetVolumeCalls;
volatile u32 gNdsAudioBgmOpenFailCount;
volatile u32 gNdsAudioBgmReadFailCount;
volatile u32 gNdsAudioBgmUnsupportedTrackCount;
volatile u32 gNdsAudioBgmReadBytes;
volatile u32 gNdsAudioBgmResidentBytes;
volatile u32 gNdsAudioBgmChunkBytes;
volatile u32 gNdsAudioBgmChunkPlayCount;
volatile u32 gNdsAudioBgmStoppedOnTeardown;
volatile u32 gNdsAudioBgmElapsedFrames;
volatile u32 gNdsAudioBgmStreamedBytes;
volatile u32 gNdsAudioBgmStreamBytesPerSecond;
volatile u32 gNdsAudioBgmExpectedBytesPerSecond;
volatile u32 gNdsAudioBgmLoopCount;
volatile u32 gNdsAudioBgmRefillCount;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
volatile u32 gNdsAudioBgmRefillTicksLast;
volatile u32 gNdsAudioBgmRefillTicksMax;
#endif
#if NDS_BGM_FALSIFIER_OFF
volatile u32 gNdsAudioBgmFalsifierOff = 1u;
#else
volatile u32 gNdsAudioBgmFalsifierOff;
#endif
volatile u32 gNdsAudioBgmPlaybackPositionBytes;
volatile u32 gNdsAudioBgmWritePositionBytes;
volatile u32 gNdsAudioBgmPlaybackHalf;
volatile u32 gNdsAudioBgmWriteHalf;
volatile u32 gNdsAudioBgmUnsafeWriteCount;
volatile u32 gNdsAudioBgmTimerTicks;
volatile u32 gNdsAudioBgmPlaybackBytes;
volatile u32 gNdsAudioBgmPlaybackLoopCount;
volatile u32 gNdsAudioBgmOverrunCount;
volatile u32 gNdsAudioBgmStreamBytes;
volatile u32 gNdsAudioBgmLoopStartBytes;
volatile u32 gNdsAudioBgmIsLooping;
volatile u32 gNdsAudioBgmPupupuPlayCount;
volatile u32 gNdsAudioBgmWinMarioPlayCount;
volatile u32 gNdsAudioBgmWinFoxPlayCount;
volatile u32 gNdsAudioBgmResultsPlayCount;
volatile u32 gNdsAudioBgmModeSelectPlayCount;
volatile u32 gNdsAudioBgmBattleSelectPlayCount;
volatile u32 gNdsAudioBgmNaturalStopCount;
volatile u32 gNdsAudioBgmLastNaturalStopTrackID;
volatile u32 gNdsAudioBgmPostNaturalTransitionCount;
volatile u32 gNdsAudioBgmPostNaturalTransitionFromTrackID;
volatile u32 gNdsAudioBgmPostNaturalTransitionToTrackID;
volatile u32 gNdsAudioBgmTrackSwitchCount;
volatile u32 gNdsAudioBgmFinitePaddingBytes;
volatile u32 gNdsAudioBgmFileOpen;
volatile u32 gNdsAudioBgmSoundActive;
volatile u32 gNdsAudioBgmPlayFailCount;
volatile u32 gNdsAudioBgmHeaderFailCount;
volatile u32 gNdsAudioBgmPacketFailCount;
volatile u32 gNdsAudioBgmPreparedCount;
volatile u32 gNdsAudioBgmSeamStartCount;
volatile u32 gNdsAudioBgmSeamMissCount;
volatile u32 gNdsAudioBgmTimerEventDropCount;
volatile u32 gNdsAudioBgmWorkerWakeCount;
volatile u32 gNdsAudioBgmErrorStopCount;
volatile u32 gNdsAudioBgmErrorCleanupFailCount;

static u8 sNdsAudioBgmBuffers[NDS_AUDIO_BGM_BUFFER_COUNT]
    [NDS_AUDIO_BGM_PACKET_BYTES] __attribute__((aligned(4)));
static FILE *sNdsAudioBgmFile;
static const NDSAudioBgmTrack *sNdsAudioBgmTrack;
static Thread sNdsAudioBgmWorker;
static Mailbox sNdsAudioBgmMailbox;
static u32 sNdsAudioBgmMailboxSlot;
static u8 sNdsAudioBgmWorkerStack[NDS_AUDIO_BGM_WORKER_STACK_BYTES]
    __attribute__((aligned(8)));
static u32 sNdsAudioBgmWorkerStarted;
static volatile u32 sNdsAudioBgmTimerArmed;
static volatile u32 sNdsAudioBgmGeneration;
static volatile u32 sNdsAudioBgmWorkerActive;
static volatile u32 sNdsAudioBgmCurrentBuffer;
static volatile u32 sNdsAudioBgmPreparedMask;
static volatile u32 sNdsAudioBgmRefillPendingMask;
static volatile u32 sNdsAudioBgmStreamExhausted;
static volatile u32 sNdsAudioBgmNaturalStopPending;
static volatile u32 sNdsAudioBgmErrorPending;
static u32 sNdsAudioBgmPacketSamples[NDS_AUDIO_BGM_BUFFER_COUNT];
static u32 sNdsAudioBgmPacketBytes[NDS_AUDIO_BGM_BUFFER_COUNT];
static u32 sNdsAudioBgmPacketLoopRestart[NDS_AUDIO_BGM_BUFFER_COUNT];
static u32 sNdsAudioBgmOffset;
static u32 sNdsAudioBgmNextPacket;
static u32 sNdsAudioBgmLastTimerTick;
static u64 sNdsAudioBgmTimerTicksTotal;
static u64 sNdsAudioBgmNextSeamTick;
static u64 sNdsAudioBgmSourceBytesLoaded;
static u64 sNdsAudioBgmInitialSourceBytes;
static s32 sNdsAudioBgmNaturalStopArmed;

static u16 ndsAudioBgmReadLe16(const u8 *data)
{
    return (u16)(((u16)data[1] << 8) | data[0]);
}

static u32 ndsAudioBgmReadLe32(const u8 *data)
{
    return ((u32)data[3] << 24) | ((u32)data[2] << 16) |
           ((u32)data[1] << 8) | data[0];
}

static u32 ndsAudioBgmPacketDurationTicks(u32 samples)
{
    return (u32)(((u64)samples * BUS_CLOCK) / NDS_AUDIO_BGM_SAMPLE_RATE);
}

static u8 ndsAudioBgmScaleVolume(u32 vol)
{
    if (vol >= 0x7800u)
    {
        return 127u;
    }
    return (u8)((vol * 127u) / 0x7800u);
}

static const NDSAudioBgmTrack *ndsAudioBgmFindTrack(s32 bgm_id)
{
    u32 i;

    for (i = 0u;
         i < (sizeof(sNdsAudioBgmTracks) / sizeof(sNdsAudioBgmTracks[0]));
         i++)
    {
        if (sNdsAudioBgmTracks[i].id == bgm_id)
        {
            return &sNdsAudioBgmTracks[i];
        }
    }
    return NULL;
}

static u32 ndsAudioBgmMapPlaybackByte(u64 playback_byte)
{
    u32 loop_bytes;

    if ((sNdsAudioBgmTrack == NULL) ||
        (playback_byte < sNdsAudioBgmTrack->stream_bytes))
    {
        return (u32)playback_byte;
    }
    if (sNdsAudioBgmTrack->is_looping == FALSE)
    {
        return sNdsAudioBgmTrack->stream_bytes;
    }
    loop_bytes = sNdsAudioBgmTrack->stream_bytes -
        sNdsAudioBgmTrack->loop_start_bytes;
    return sNdsAudioBgmTrack->loop_start_bytes +
        (u32)((playback_byte - sNdsAudioBgmTrack->stream_bytes) % loop_bytes);
}

static void ndsAudioBgmCloseFile(void)
{
    if (sNdsAudioBgmFile != NULL)
    {
        fclose(sNdsAudioBgmFile);
        sNdsAudioBgmFile = NULL;
    }
    gNdsAudioBgmFileOpen = 0u;
}

static s32 ndsAudioBgmOpenFile(void)
{
    if (sNdsAudioBgmTrack == NULL)
    {
        gNdsAudioBgmOpenFailCount++;
        return FALSE;
    }
    sNdsAudioBgmFile = fopen(sNdsAudioBgmTrack->path, "rb");
    if (sNdsAudioBgmFile == NULL)
    {
        gNdsAudioBgmOpenFailCount++;
        return FALSE;
    }
    gNdsAudioBgmFileOpen = 1u;
    return TRUE;
}

static s32 ndsAudioBgmReadExact(u8 *dst, u32 bytes)
{
    if ((sNdsAudioBgmFile == NULL) ||
        (fread(dst, 1u, bytes, sNdsAudioBgmFile) != bytes))
    {
        gNdsAudioBgmReadFailCount++;
        return FALSE;
    }
    sNdsAudioBgmOffset += bytes;
    gNdsAudioBgmReadBytes += bytes;
    return TRUE;
}

static s32 ndsAudioBgmReadHeader(void)
{
    u8 header[NDS_AUDIO_BGM_CONTAINER_HEADER_BYTES];
    long asset_bytes;
    u32 sample_count;
    u32 loop_sample;
    u32 flags;

    if ((sNdsAudioBgmFile == NULL) || (sNdsAudioBgmTrack == NULL) ||
        (fseek(sNdsAudioBgmFile, 0, SEEK_END) != 0) ||
        ((asset_bytes = ftell(sNdsAudioBgmFile)) < 0) ||
        ((u32)asset_bytes != sNdsAudioBgmTrack->asset_bytes) ||
        (fseek(sNdsAudioBgmFile, 0, SEEK_SET) != 0))
    {
        gNdsAudioBgmHeaderFailCount++;
        return FALSE;
    }
    sNdsAudioBgmOffset = 0u;
    if (ndsAudioBgmReadExact(header, sizeof(header)) == FALSE)
    {
        gNdsAudioBgmHeaderFailCount++;
        return FALSE;
    }
    sample_count = ndsAudioBgmReadLe32(&header[12]);
    loop_sample = ndsAudioBgmReadLe32(&header[16]);
    flags = ndsAudioBgmReadLe32(&header[36]);
    if ((ndsAudioBgmReadLe32(&header[0]) != NDS_AUDIO_BGM_CONTAINER_MAGIC) ||
        (ndsAudioBgmReadLe16(&header[4]) != NDS_AUDIO_BGM_CONTAINER_VERSION) ||
        (ndsAudioBgmReadLe16(&header[6]) !=
            NDS_AUDIO_BGM_CONTAINER_HEADER_BYTES) ||
        (ndsAudioBgmReadLe32(&header[8]) != NDS_AUDIO_BGM_SAMPLE_RATE) ||
        ((sample_count * 2u) != sNdsAudioBgmTrack->stream_bytes) ||
        (ndsAudioBgmReadLe32(&header[20]) != NDS_AUDIO_BGM_PACKET_SAMPLES) ||
        (ndsAudioBgmReadLe32(&header[24]) !=
            sNdsAudioBgmTrack->packet_count) ||
        (ndsAudioBgmReadLe32(&header[28]) !=
            sNdsAudioBgmTrack->loop_packet) ||
        (ndsAudioBgmReadLe32(&header[32]) !=
            sNdsAudioBgmTrack->loop_record) ||
        (((flags & 1u) != 0u) !=
            (sNdsAudioBgmTrack->is_looping != FALSE)) ||
        ((sNdsAudioBgmTrack->is_looping != FALSE) &&
            ((loop_sample * 2u) !=
             sNdsAudioBgmTrack->loop_start_bytes)) ||
        ((sNdsAudioBgmTrack->is_looping == FALSE) &&
            (loop_sample != NDS_AUDIO_BGM_NO_LOOP)))
    {
        gNdsAudioBgmHeaderFailCount++;
        return FALSE;
    }
    sNdsAudioBgmNextPacket = 0u;
    return TRUE;
}

/* 1 = loaded, 0 = finite end, -1 = malformed/read failure. */
static s32 ndsAudioBgmReadPacket(u32 buffer)
{
    u8 record[NDS_AUDIO_BGM_PACKET_HEADER_BYTES];
    u32 samples;
    u32 payload_bytes;
    u32 expected_bytes;
    u32 loop_restart = 0u;

    if ((buffer >= NDS_AUDIO_BGM_BUFFER_COUNT) ||
        (sNdsAudioBgmTrack == NULL) || (sNdsAudioBgmFile == NULL))
    {
        gNdsAudioBgmPacketFailCount++;
        return -1;
    }
    if (sNdsAudioBgmNextPacket >= sNdsAudioBgmTrack->packet_count)
    {
        if (sNdsAudioBgmTrack->is_looping == FALSE)
        {
            sNdsAudioBgmStreamExhausted = 1u;
            return 0;
        }
        if (fseek(sNdsAudioBgmFile,
                  (long)sNdsAudioBgmTrack->loop_record, SEEK_SET) != 0)
        {
            gNdsAudioBgmReadFailCount++;
            return -1;
        }
        sNdsAudioBgmOffset = sNdsAudioBgmTrack->loop_record;
        sNdsAudioBgmNextPacket = sNdsAudioBgmTrack->loop_packet;
        loop_restart = 1u;
        gNdsAudioBgmLoopCount++;
        gNdsAudioBgmMask |= 1u << 4;
    }
    if (ndsAudioBgmReadExact(record, sizeof(record)) == FALSE)
    {
        return -1;
    }
    samples = ndsAudioBgmReadLe32(&record[0]);
    payload_bytes = ndsAudioBgmReadLe32(&record[4]);
    expected_bytes = 4u + (((samples + 7u) / 8u) * 4u);
    if ((samples == 0u) || (samples > NDS_AUDIO_BGM_PACKET_SAMPLES) ||
        (payload_bytes != expected_bytes) ||
        (payload_bytes > NDS_AUDIO_BGM_PACKET_BYTES) ||
        ((payload_bytes & 3u) != 0u) ||
        ((sNdsAudioBgmOffset + payload_bytes) >
            sNdsAudioBgmTrack->asset_bytes))
    {
        gNdsAudioBgmPacketFailCount++;
        return -1;
    }
    if (ndsAudioBgmReadExact(sNdsAudioBgmBuffers[buffer],
                             payload_bytes) == FALSE)
    {
        return -1;
    }
    DC_FlushRange(sNdsAudioBgmBuffers[buffer], payload_bytes);
    sNdsAudioBgmPacketSamples[buffer] = samples;
    sNdsAudioBgmPacketBytes[buffer] = payload_bytes;
    sNdsAudioBgmPacketLoopRestart[buffer] = loop_restart;
    sNdsAudioBgmNextPacket++;
    sNdsAudioBgmSourceBytesLoaded += (u64)samples * 2u;
    gNdsAudioBgmChunkBytes = payload_bytes;
    gNdsAudioBgmResidentBytes = NDS_AUDIO_BGM_RESIDENT_BYTES;
    if ((sNdsAudioBgmTrack->is_looping == FALSE) &&
        (sNdsAudioBgmNextPacket == sNdsAudioBgmTrack->packet_count))
    {
        sNdsAudioBgmStreamExhausted = 1u;
    }
    return 1;
}

static void ndsAudioBgmPrepareBuffer(u32 buffer)
{
    u32 channel = NDS_AUDIO_BGM_CHANNEL_BASE + buffer;

    soundPreparePcm(channel,
                    (u32)ndsAudioBgmScaleVolume(gNdsAudioBgmVolume) << 4,
                    64u,
                    soundTimerFromHz(NDS_AUDIO_BGM_SAMPLE_RATE),
                    SoundMode_OneShot,
                    SoundFmt_ImaAdpcm,
                    sNdsAudioBgmBuffers[buffer],
                    0u,
                    sNdsAudioBgmPacketBytes[buffer] / 4u);
    sNdsAudioBgmPreparedMask |= 1u << buffer;
    gNdsAudioBgmPreparedCount++;
}

static void ndsAudioBgmStopTimer(void)
{
#if !NDS_HARNESS_FAST_LOGIC
    if (sNdsAudioBgmTimerArmed != 0u)
    {
        timerStop(NDS_AUDIO_BGM_TIMER);
        sNdsAudioBgmTimerArmed = 0u;
    }
#endif
}

static void ndsAudioBgmTimerCallback(void)
{
    TIMER_CR(NDS_AUDIO_BGM_TIMER) = 0u;
    sNdsAudioBgmTimerArmed = 0u;
    if ((sNdsAudioBgmWorkerActive != 0u) &&
        (mailboxTrySend(&sNdsAudioBgmMailbox,
                        sNdsAudioBgmGeneration) == false))
    {
        gNdsAudioBgmTimerEventDropCount++;
        sNdsAudioBgmErrorPending = 1u;
    }
}

static void ndsAudioBgmArmTimer(u32 samples)
{
#if !NDS_HARNESS_FAST_LOGIC
    u32 timer_ticks = (u32)((((u64)samples * (BUS_CLOCK >> 10)) +
                             (NDS_AUDIO_BGM_SAMPLE_RATE / 2u)) /
                            NDS_AUDIO_BGM_SAMPLE_RATE);

    if (timer_ticks == 0u)
    {
        timer_ticks = 1u;
    }
    timerStart(NDS_AUDIO_BGM_TIMER,
               ClockDivider_1024,
               (u16)(0u - timer_ticks),
               ndsAudioBgmTimerCallback);
    sNdsAudioBgmTimerArmed = 1u;
#else
    (void)samples;
#endif
}

static s32 ndsAudioBgmHandleSeam(void)
{
    u32 current = sNdsAudioBgmCurrentBuffer;
    u32 next = 1u - current;

    if (sNdsAudioBgmWorkerActive == 0u)
    {
        return FALSE;
    }
    if ((sNdsAudioBgmPreparedMask & (1u << next)) == 0u)
    {
        sNdsAudioBgmWorkerActive = 0u;
        if (sNdsAudioBgmStreamExhausted != 0u)
        {
            sNdsAudioBgmNaturalStopPending = 1u;
        }
        else
        {
            gNdsAudioBgmSeamMissCount++;
            gNdsAudioBgmOverrunCount++;
            sNdsAudioBgmErrorPending = 1u;
        }
        return FALSE;
    }
    soundStart(1u << (NDS_AUDIO_BGM_CHANNEL_BASE + next));
    sNdsAudioBgmPreparedMask &= ~(1u << next);
    sNdsAudioBgmCurrentBuffer = next;
    sNdsAudioBgmRefillPendingMask |= 1u << current;
    gNdsAudioBgmPlaybackHalf = next;
    gNdsAudioBgmWriteHalf = current;
    gNdsAudioBgmWritePositionBytes =
        current * NDS_AUDIO_BGM_PACKET_BYTES;
    gNdsAudioBgmChunkBytes = sNdsAudioBgmPacketBytes[next];
    gNdsAudioBgmChunkPlayCount++;
    gNdsAudioBgmSeamStartCount++;
    if (sNdsAudioBgmPacketLoopRestart[next] != 0u)
    {
        gNdsAudioBgmPlaybackLoopCount++;
    }
    ndsAudioBgmArmTimer(sNdsAudioBgmPacketSamples[next]);
    return TRUE;
}

static int ndsAudioBgmWorkerMain(void *arg)
{
    (void)arg;
    for (;;)
    {
        u32 generation = mailboxRecv(&sNdsAudioBgmMailbox);

        gNdsAudioBgmWorkerWakeCount++;
        if ((sNdsAudioBgmWorkerActive != 0u) &&
            (generation == sNdsAudioBgmGeneration))
        {
            (void)ndsAudioBgmHandleSeam();
        }
    }
    return 0;
}

/* Slice 48. Calico: "Higher numerical values correspond to lower thread
 * priority", MAIN_THREAD_PRIO is 0x1c, and "higher priority threads are always
 * guaranteed to preempt the current thread when they become runnable". So the
 * original MAIN_THREAD_PRIO - 1 put the refill ABOVE the gameplay thread: every
 * seam interrupted whatever frame was running to do an ~8 KB cartridge read.
 *
 * That is the 49-of-80 FAT lane in the c123 top-80 attribution -- armCopyMem32
 * 27,331 + get_fat 16,418 + f_lseek 10,375 + _dvmDiscCacheReadWrite 4,552 =
 * 58,676 cyc/frame, ~29,338 tk. It was invisible to the AUD bucket, which
 * brackets only the main thread's ndsAudioBgmUpdate, and an earlier draft of
 * docs/RAM_RECOVERY_PLAN.md read AUD's 0.2% as proof BGM was not the tail. A
 * preempting thread's cycles land on whatever bucket the main thread was inside.
 *
 * The counters that identify it: gNdsAudioBgmRefillCount 104 ==
 * gNdsAudioBgmWorkerWakeCount 104, so every read is on this thread, and 104
 * reads over 1600 frames is the only file traffic large enough -- the anim cache
 * is at 2 misses since slice 46 and 85 of its 123 payload reads happen in the
 * warm preload before the window opens.
 *
 * +1 puts the worker BELOW main, so it runs when main blocks, which is the
 * VBlank wait. The same bytes are read at the same rate; only the placement in
 * the frame changes, so there is no audio fidelity question here. It is safe by
 * two independent margins: WAIT P50 is 207,104 ticks of idle per frame, far more
 * than one refill, and the seam budget is ~186 ms against a 16.7 ms frame, so
 * the read has ~11 VBlank windows to finish. A failure would be loud and is
 * already covered -- Boundary's ADPCM smoke watches gNdsAudioBgmPlaying and
 * SeamMissCount, and gNdsAudioBgmOverrunCount catches a late buffer.
 *
 * `.data` aligned(32) so the A/B is ONE binary: -SetGlobals
 * gNdsAudioBgmWorkerPrio=27 restores the preempting arm at identical placement.
 * E11 lost a proven -7,667 cut to +15,744 of relink movement measuring this
 * subsystem across two builds; a route bit removes that floor entirely. */
volatile u32 gNdsAudioBgmWorkerPrio
    __attribute__((section(".data"), aligned(32))) = MAIN_THREAD_PRIO + 1u;

/* The priority the worker RUNS at, applied once BGM is updating. It is not the
 * creation value and the split is measured, not aesthetic -- at byte-identical
 * placement (the two ROMs differ in 41 bytes, the .data word plus build stamps):
 *
 *   creation 27, run 27  ->  WORK-H P95 1,101,248   (build-c124-bgmprio-create27)
 *   creation 29, run 29  ->                1,091,520
 *   creation 29, run 27  ->                1,083,456   <- ships
 *
 * So preempting during SCENE SETUP costs ~17,792 and preempting during the match
 * saves ~8,064; the two pull opposite ways and the best combination is low at
 * creation, high while playing. Setup is where the anim warm preload and the
 * asset loads run, and a higher-priority cartridge reader interleaving with them
 * is the only difference between those two arms.
 *
 * Do NOT read the c123 bank's 1,196,224 as this lever's size. create27 restores
 * the bank's exact behaviour and still measures 1,101,248, so ~94,976 of that
 * gap is PLACEMENT -- see SLICE48.md. */
volatile u32 gNdsAudioBgmWorkerRunPrio
    __attribute__((section(".data"), aligned(32))) = MAIN_THREAD_PRIO - 1u;

/* Engagement proof, and it is not optional here. -SetGlobals pokes at the FIRST
 * frame-complete marker, while the worker is created the moment BGM starts, so
 * a poke can easily arrive after threadPrepare has already taken the value. If
 * that happened the control arm would be the candidate wearing a different
 * label -- the failure `prove-the-control-differs` records. This records what
 * threadPrepare was ACTUALLY handed, so the two arms' readbacks either differ
 * (the A/B is real) or they do not (the route needs re-applying, not a
 * conclusion). */
volatile u32 gNdsAudioBgmWorkerPrioApplied;

/* Clamp to the documented range so a poke cannot hand the scheduler a priority
 * it rejects and leave the match with no BGM worker at all -- the same reason
 * gNdsR2AnimWarmStep clamps a poke of 0. */
static u32 ndsAudioBgmClampPrio(u32 prio)
{
    return (prio > (u32)THREAD_MIN_PRIO) ? (u32)THREAD_MIN_PRIO : prio;
}

/* The route has to be re-appliable, and finding that out cost a run. The worker
 * is created the moment BGM starts, which is BEFORE -SetGlobals fires at the
 * first frame-complete marker, so the first attempt at this A/B poked 27 into
 * gNdsAudioBgmWorkerPrio and measured a thread still running at 29: both arms
 * returned WORK-H P95 1,102,208 to the byte. gNdsAudioBgmWorkerPrioApplied is
 * what caught it. This is also HANDOFF's own slice-47 lesson -- a tunable
 * belongs in a pokeable global or a sweep costs a rebuild per value.
 *
 * Called once per frame from ndsAudioBgmUpdate, which has already returned early
 * unless BGM is playing. Two .data loads and a compare off one cache line when
 * the route is untouched, which is every shipping frame. */
static void ndsAudioBgmApplyWorkerPrio(void)
{
    u32 prio = ndsAudioBgmClampPrio(gNdsAudioBgmWorkerRunPrio);

    if ((sNdsAudioBgmWorkerStarted == 0u) ||
        (prio == gNdsAudioBgmWorkerPrioApplied))
    {
        return;
    }
    threadSetPrio(&sNdsAudioBgmWorker, (u8)prio);
    gNdsAudioBgmWorkerPrioApplied = prio;
}

static void ndsAudioBgmEnsureWorker(void)
{
    u32 prio;

    if (sNdsAudioBgmWorkerStarted != 0u)
    {
        return;
    }

    prio = ndsAudioBgmClampPrio(gNdsAudioBgmWorkerPrio);
    gNdsAudioBgmWorkerPrioApplied = prio;

    mailboxPrepare(&sNdsAudioBgmMailbox, &sNdsAudioBgmMailboxSlot, 1u);
    threadPrepare(&sNdsAudioBgmWorker,
                  ndsAudioBgmWorkerMain,
                  NULL,
                  &sNdsAudioBgmWorkerStack[sizeof(sNdsAudioBgmWorkerStack)],
                  (u8)prio);
    threadStart(&sNdsAudioBgmWorker);
    sNdsAudioBgmWorkerStarted = 1u;
}

static void ndsAudioBgmKillSound(void)
{
    sNdsAudioBgmWorkerActive = 0u;
    sNdsAudioBgmGeneration++;
    ndsAudioBgmStopTimer();
    if (gNdsAudioBgmSoundActive != 0u)
    {
        soundStop(NDS_AUDIO_BGM_CHANNEL_MASK);
    }
    sNdsAudioBgmPreparedMask = 0u;
    sNdsAudioBgmRefillPendingMask = 0u;
    gNdsAudioBgmSoundActive = 0u;
}

static s32 ndsAudioBgmServiceRefills(void)
{
    u32 pending = sNdsAudioBgmRefillPendingMask;
    u32 buffer;

    for (buffer = 0u; buffer < NDS_AUDIO_BGM_BUFFER_COUNT; buffer++)
    {
        s32 read_result;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        u32 refill_start;
#endif
        if ((pending & (1u << buffer)) == 0u)
        {
            continue;
        }
        sNdsAudioBgmRefillPendingMask &= ~(1u << buffer);
        if (buffer == sNdsAudioBgmCurrentBuffer)
        {
            gNdsAudioBgmUnsafeWriteCount++;
            return FALSE;
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        refill_start = cpuGetTiming();
#endif
        read_result = ndsAudioBgmReadPacket(buffer);
        if (read_result < 0)
        {
            return FALSE;
        }
        if (read_result > 0)
        {
            ndsAudioBgmPrepareBuffer(buffer);
            gNdsAudioBgmRefillCount++;
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        gNdsAudioBgmRefillTicksLast = cpuGetTiming() - refill_start;
        if (gNdsAudioBgmRefillTicksLast > gNdsAudioBgmRefillTicksMax)
        {
            gNdsAudioBgmRefillTicksMax = gNdsAudioBgmRefillTicksLast;
        }
#endif
    }
    return TRUE;
}

#if NDS_SHIP_TELEMETRY
static void ndsAudioBgmUpdateRateMarkers(void)
{
    u64 playback_bytes =
        (sNdsAudioBgmTimerTicksTotal * NDS_AUDIO_BGM_BYTES_PER_SECOND) /
        BUS_CLOCK;

    gNdsAudioBgmStreamedBytes =
        (sNdsAudioBgmSourceBytesLoaded > sNdsAudioBgmInitialSourceBytes) ?
        (u32)(sNdsAudioBgmSourceBytesLoaded -
              sNdsAudioBgmInitialSourceBytes) : 0u;
    gNdsAudioBgmExpectedBytesPerSecond = NDS_AUDIO_BGM_BYTES_PER_SECOND;
    if (sNdsAudioBgmTimerTicksTotal != 0u)
    {
        gNdsAudioBgmStreamBytesPerSecond =
            (u32)((playback_bytes * BUS_CLOCK) /
                  sNdsAudioBgmTimerTicksTotal);
    }
    gNdsAudioBgmPlaybackPositionBytes =
        ndsAudioBgmMapPlaybackByte(playback_bytes);
    gNdsAudioBgmPlaybackHalf = sNdsAudioBgmCurrentBuffer;
    gNdsAudioBgmTimerTicks = (u32)sNdsAudioBgmTimerTicksTotal;
    gNdsAudioBgmPlaybackBytes = (u32)playback_bytes;
}
#endif

static void ndsAudioBgmFailPlayback(void)
{
    ndsAudioBgmKillSound();
    ndsAudioBgmCloseFile();
    gNdsAudioBgmPlaying = 0u;
    gNdsAudioBgmErrorStopCount++;
    if ((gNdsAudioBgmSoundActive != 0u) ||
        (sNdsAudioBgmFile != NULL) || (gNdsAudioBgmFileOpen != 0u))
    {
        gNdsAudioBgmErrorCleanupFailCount++;
    }
}

static void ndsAudioBgmFinishNaturally(void)
{
#if NDS_SHIP_TELEMETRY
    ndsAudioBgmUpdateRateMarkers();
#endif
    ndsAudioBgmKillSound();
    ndsAudioBgmCloseFile();
    gNdsAudioBgmPlaying = 0u;
    gNdsAudioBgmNaturalStopCount++;
    gNdsAudioBgmLastNaturalStopTrackID = gNdsAudioBgmTrackID;
    sNdsAudioBgmNaturalStopArmed = TRUE;
    gNdsAudioBgmMask |= 1u << 5;
}

void ndsAudioBgmDiagnosticsReset(void)
{
    ndsAudioBgmKillSound();
    ndsAudioBgmCloseFile();
    sNdsAudioBgmTrack = NULL;
    sNdsAudioBgmOffset = 0u;
    sNdsAudioBgmNextPacket = 0u;
    sNdsAudioBgmLastTimerTick = 0u;
    sNdsAudioBgmTimerTicksTotal = 0u;
    sNdsAudioBgmNextSeamTick = 0u;
    sNdsAudioBgmSourceBytesLoaded = 0u;
    sNdsAudioBgmInitialSourceBytes = 0u;
    sNdsAudioBgmNaturalStopArmed = FALSE;
    sNdsAudioBgmStreamExhausted = 0u;
    sNdsAudioBgmNaturalStopPending = 0u;
    sNdsAudioBgmErrorPending = 0u;

    gNdsAudioBgmResult = 0u;
    gNdsAudioBgmMask = 0u;
    gNdsAudioBgmPlaying = 0u;
    gNdsAudioBgmTrackID = 0u;
    gNdsAudioBgmVolume = 0x7800u;
    gNdsAudioBgmPlayCalls = 0u;
    gNdsAudioBgmStopCalls = 0u;
    gNdsAudioBgmCheckCalls = 0u;
    gNdsAudioBgmSetVolumeCalls = 0u;
    gNdsAudioBgmOpenFailCount = 0u;
    gNdsAudioBgmReadFailCount = 0u;
    gNdsAudioBgmUnsupportedTrackCount = 0u;
    gNdsAudioBgmReadBytes = 0u;
    gNdsAudioBgmResidentBytes = 0u;
    gNdsAudioBgmChunkBytes = 0u;
    gNdsAudioBgmChunkPlayCount = 0u;
    gNdsAudioBgmStoppedOnTeardown = 0u;
    gNdsAudioBgmElapsedFrames = 0u;
    gNdsAudioBgmStreamedBytes = 0u;
    gNdsAudioBgmStreamBytesPerSecond = 0u;
    gNdsAudioBgmExpectedBytesPerSecond = NDS_AUDIO_BGM_BYTES_PER_SECOND;
    gNdsAudioBgmLoopCount = 0u;
    gNdsAudioBgmRefillCount = 0u;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsAudioBgmRefillTicksLast = 0u;
    gNdsAudioBgmRefillTicksMax = 0u;
#endif
    gNdsAudioBgmPlaybackPositionBytes = 0u;
    gNdsAudioBgmWritePositionBytes = 0u;
    gNdsAudioBgmPlaybackHalf = 0u;
    gNdsAudioBgmWriteHalf = 0u;
    gNdsAudioBgmUnsafeWriteCount = 0u;
    gNdsAudioBgmTimerTicks = 0u;
    gNdsAudioBgmPlaybackBytes = 0u;
    gNdsAudioBgmPlaybackLoopCount = 0u;
    gNdsAudioBgmOverrunCount = 0u;
    gNdsAudioBgmStreamBytes = 0u;
    gNdsAudioBgmLoopStartBytes = 0u;
    gNdsAudioBgmIsLooping = 0u;
    gNdsAudioBgmPupupuPlayCount = 0u;
    gNdsAudioBgmWinMarioPlayCount = 0u;
    gNdsAudioBgmWinFoxPlayCount = 0u;
    gNdsAudioBgmResultsPlayCount = 0u;
    gNdsAudioBgmNaturalStopCount = 0u;
    gNdsAudioBgmLastNaturalStopTrackID = NDS_AUDIO_BGM_NO_LOOP;
    gNdsAudioBgmPostNaturalTransitionCount = 0u;
    gNdsAudioBgmPostNaturalTransitionFromTrackID = NDS_AUDIO_BGM_NO_LOOP;
    gNdsAudioBgmPostNaturalTransitionToTrackID = NDS_AUDIO_BGM_NO_LOOP;
    gNdsAudioBgmTrackSwitchCount = 0u;
    gNdsAudioBgmFinitePaddingBytes = 0u;
    gNdsAudioBgmFileOpen = 0u;
    gNdsAudioBgmSoundActive = 0u;
    gNdsAudioBgmPlayFailCount = 0u;
    gNdsAudioBgmHeaderFailCount = 0u;
    gNdsAudioBgmPacketFailCount = 0u;
    gNdsAudioBgmPreparedCount = 0u;
    gNdsAudioBgmSeamStartCount = 0u;
    gNdsAudioBgmSeamMissCount = 0u;
    gNdsAudioBgmTimerEventDropCount = 0u;
    gNdsAudioBgmWorkerWakeCount = 0u;
    gNdsAudioBgmErrorStopCount = 0u;
    gNdsAudioBgmErrorCleanupFailCount = 0u;
}

void ndsAudioBgmPlay(s32 player, s32 bgm_id)
{
    const NDSAudioBgmTrack *track;
    s32 second_read;
    u32 stale_message;

    (void)player;
    gNdsAudioBgmPlayCalls++;
    track = ndsAudioBgmFindTrack(bgm_id);
    if (track == NULL)
    {
        gNdsAudioBgmUnsupportedTrackCount++;
        return;
    }
    gNdsAudioBgmTrackID = (u32)bgm_id;
    if (gNdsAudioBgmPlaying != 0u)
    {
        if ((sNdsAudioBgmTrack != NULL) &&
            (sNdsAudioBgmTrack->id != track->id))
        {
            gNdsAudioBgmTrackSwitchCount++;
        }
        ndsAudioBgmKillSound();
        ndsAudioBgmCloseFile();
    }
    if (sNdsAudioBgmNaturalStopArmed != FALSE)
    {
        gNdsAudioBgmPostNaturalTransitionCount++;
        gNdsAudioBgmPostNaturalTransitionFromTrackID =
            gNdsAudioBgmLastNaturalStopTrackID;
        gNdsAudioBgmPostNaturalTransitionToTrackID = (u32)track->id;
        sNdsAudioBgmNaturalStopArmed = FALSE;
    }

    sNdsAudioBgmTrack = track;
    sNdsAudioBgmStreamExhausted = 0u;
    sNdsAudioBgmNaturalStopPending = 0u;
    sNdsAudioBgmErrorPending = 0u;
    sNdsAudioBgmSourceBytesLoaded = 0u;
    sNdsAudioBgmPreparedMask = 0u;
    sNdsAudioBgmRefillPendingMask = 0u;
    gNdsAudioBgmStreamBytes = track->stream_bytes;
    gNdsAudioBgmLoopStartBytes = track->loop_start_bytes;
    gNdsAudioBgmIsLooping = (track->is_looping != FALSE) ? 1u : 0u;
    gNdsAudioBgmStreamedBytes = 0u;
    gNdsAudioBgmPlaybackLoopCount = 0u;
    switch (bgm_id)
    {
    case nSYAudioBGMPupupu: gNdsAudioBgmPupupuPlayCount++; break;
    case nSYAudioBGMWinMario: gNdsAudioBgmWinMarioPlayCount++; break;
    case nSYAudioBGMWinFox: gNdsAudioBgmWinFoxPlayCount++; break;
    case nSYAudioBGMResults: gNdsAudioBgmResultsPlayCount++; break;
    case nSYAudioBGMModeSelect: gNdsAudioBgmModeSelectPlayCount++; break;
    case nSYAudioBGMBattleSelect: gNdsAudioBgmBattleSelectPlayCount++; break;
    }
    if ((gNdsAudioBgmSetVolumeCalls == 0u) &&
        (gNdsAudioBgmVolume == 0u))
    {
        gNdsAudioBgmVolume = 0x7800u;
    }
    soundEnable();

#if NDS_BGM_FALSIFIER_OFF
    gNdsAudioBgmPlaying = 1u;
    gNdsAudioBgmMask |= 1u << 0;
    gNdsAudioBgmResult = NDS_AUDIO_BGM_PASS;
    return;
#endif
    if ((ndsAudioBgmOpenFile() == FALSE) ||
        (ndsAudioBgmReadHeader() == FALSE) ||
        (ndsAudioBgmReadPacket(0u) != 1))
    {
        ndsAudioBgmFailPlayback();
        return;
    }
    second_read = ndsAudioBgmReadPacket(1u);
    if (second_read < 0)
    {
        ndsAudioBgmFailPlayback();
        return;
    }
    sNdsAudioBgmInitialSourceBytes = sNdsAudioBgmSourceBytesLoaded;
    ndsAudioBgmEnsureWorker();
    while (mailboxTryRecv(&sNdsAudioBgmMailbox, &stale_message))
    {
    }
    ndsAudioBgmPrepareBuffer(0u);
    if (second_read > 0)
    {
        ndsAudioBgmPrepareBuffer(1u);
    }
    soundSynchronize();
    sNdsAudioBgmGeneration++;
    if (sNdsAudioBgmGeneration == 0u)
    {
        sNdsAudioBgmGeneration = 1u;
    }
    sNdsAudioBgmCurrentBuffer = 0u;
    sNdsAudioBgmWorkerActive = 1u;
    soundStart(1u << NDS_AUDIO_BGM_CHANNEL_BASE);
    sNdsAudioBgmPreparedMask &= ~1u;
    gNdsAudioBgmSoundActive = 1u;
    gNdsAudioBgmResidentBytes = NDS_AUDIO_BGM_RESIDENT_BYTES;
    gNdsAudioBgmChunkBytes = sNdsAudioBgmPacketBytes[0];
    gNdsAudioBgmPlaybackHalf = 0u;
    gNdsAudioBgmWriteHalf = 1u;
    gNdsAudioBgmWritePositionBytes = NDS_AUDIO_BGM_PACKET_BYTES;
    gNdsAudioBgmPlaybackPositionBytes = 0u;
    gNdsAudioBgmChunkPlayCount++;
    sNdsAudioBgmLastTimerTick = cpuGetTiming();
    sNdsAudioBgmTimerTicksTotal = 0u;
    sNdsAudioBgmNextSeamTick =
        ndsAudioBgmPacketDurationTicks(sNdsAudioBgmPacketSamples[0]);
    ndsAudioBgmArmTimer(sNdsAudioBgmPacketSamples[0]);
    gNdsAudioBgmPlaying = 1u;
    gNdsAudioBgmMask |= 1u << 0;
    gNdsAudioBgmResult = NDS_AUDIO_BGM_PASS;
}

void ndsAudioBgmStopAll(void)
{
    gNdsAudioBgmStopCalls++;
    ndsAudioBgmKillSound();
    if (gNdsAudioBgmPlaying != 0u)
    {
        gNdsAudioBgmStoppedOnTeardown = 1u;
    }
    gNdsAudioBgmPlaying = 0u;
    ndsAudioBgmCloseFile();
    sNdsAudioBgmNaturalStopArmed = FALSE;
    sNdsAudioBgmNaturalStopPending = 0u;
    sNdsAudioBgmErrorPending = 0u;
    gNdsAudioBgmMask |= 1u << 1;
}

s32 ndsAudioBgmCheckPlaying(s32 player)
{
    (void)player;
    gNdsAudioBgmCheckCalls++;
    gNdsAudioBgmMask |= 1u << 2;
    return (gNdsAudioBgmPlaying != 0u) ? TRUE : FALSE;
}

s32 ndsAudioBgmIsPlaying(void)
{
    return (gNdsAudioBgmPlaying != 0u) ? TRUE : FALSE;
}

void ndsAudioBgmSetVolume(s32 player, u32 vol)
{
    u32 volume;

    (void)player;
    gNdsAudioBgmSetVolumeCalls++;
    gNdsAudioBgmVolume = vol;
    volume = (u32)ndsAudioBgmScaleVolume(vol) << 4;
    if (gNdsAudioBgmSoundActive != 0u)
    {
        soundChSetVolume(NDS_AUDIO_BGM_CHANNEL_BASE, volume);
        soundChSetVolume(NDS_AUDIO_BGM_CHANNEL_BASE + 1u, volume);
    }
    gNdsAudioBgmMask |= 1u << 3;
}

void ndsAudioBgmUpdate(void)
{
    u32 delta;

    if ((gNdsAudioBgmPlaying == 0u) || (sNdsAudioBgmFile == NULL))
    {
        return;
    }
    gNdsAudioBgmElapsedFrames++;
    ndsAudioBgmApplyWorkerPrio();
#if NDS_HARNESS_FAST_LOGIC
    delta = BUS_CLOCK / 60u;
#else
    {
        u32 now = cpuGetTiming();

        delta = now - sNdsAudioBgmLastTimerTick;
        sNdsAudioBgmLastTimerTick = now;
    }
#endif
    sNdsAudioBgmTimerTicksTotal += delta;
#if NDS_SHIP_TELEMETRY
    ndsAudioBgmUpdateRateMarkers();
#endif
    if (sNdsAudioBgmErrorPending != 0u)
    {
        ndsAudioBgmFailPlayback();
        return;
    }
    if (sNdsAudioBgmNaturalStopPending != 0u)
    {
        ndsAudioBgmFinishNaturally();
        return;
    }
    if (ndsAudioBgmServiceRefills() == FALSE)
    {
        ndsAudioBgmFailPlayback();
        return;
    }
#if NDS_HARNESS_FAST_LOGIC
    while ((sNdsAudioBgmWorkerActive != 0u) &&
           (sNdsAudioBgmTimerTicksTotal >= sNdsAudioBgmNextSeamTick))
    {
        if (ndsAudioBgmHandleSeam() == FALSE)
        {
            break;
        }
        sNdsAudioBgmNextSeamTick += ndsAudioBgmPacketDurationTicks(
            sNdsAudioBgmPacketSamples[sNdsAudioBgmCurrentBuffer]);
        if (ndsAudioBgmServiceRefills() == FALSE)
        {
            sNdsAudioBgmErrorPending = 1u;
            break;
        }
    }
#endif
    if (sNdsAudioBgmErrorPending != 0u)
    {
        ndsAudioBgmFailPlayback();
    }
    else if (sNdsAudioBgmNaturalStopPending != 0u)
    {
        ndsAudioBgmFinishNaturally();
    }
}
