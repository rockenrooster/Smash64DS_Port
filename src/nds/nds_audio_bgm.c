#include <nds.h>
#include <stdio.h>
#include <string.h>

#include <gm/gmsound.h>
#include <nds/nds_audio_bgm.h>

#define NDS_AUDIO_BGM_PATH_PUPUPU "nitro:/audio/bgm_pupupu_pcm16.raw"
#define NDS_AUDIO_BGM_PATH_WIN_MARIO "nitro:/audio/bgm_win_mario_pcm16.raw"
#define NDS_AUDIO_BGM_PATH_WIN_FOX "nitro:/audio/bgm_win_fox_pcm16.raw"
#define NDS_AUDIO_BGM_PATH_RESULTS "nitro:/audio/bgm_results_pcm16.raw"
#define NDS_AUDIO_BGM_HALF_TICKS \
    (((u64)BUS_CLOCK * NDS_AUDIO_BGM_HALF_BYTES) / \
     NDS_AUDIO_BGM_BYTES_PER_SECOND)
_Static_assert((NDS_AUDIO_BGM_CHUNK_BYTES % 2u) == 0u,
               "BGM ring buffer must split into two whole halves");
_Static_assert(NDS_AUDIO_BGM_BYTES_PER_SECOND == 44100u,
               "BGM PCM16 mono streams must consume 44100 bytes/sec");

typedef struct NDSAudioBgmTrack {
    s32 id;
    const char *path;
    u32 stream_bytes;
    u32 loop_start_bytes;
    s32 is_looping;
} NDSAudioBgmTrack;

static const NDSAudioBgmTrack sNdsAudioBgmTracks[] = {
    {
        nSYAudioBGMPupupu,
        NDS_AUDIO_BGM_PATH_PUPUPU,
        NDS_AUDIO_BGM_PUPUPU_STREAM_BYTES,
        NDS_AUDIO_BGM_PUPUPU_LOOP_START_BYTES,
        TRUE
    },
    {
        nSYAudioBGMWinMario,
        NDS_AUDIO_BGM_PATH_WIN_MARIO,
        NDS_AUDIO_BGM_WIN_MARIO_STREAM_BYTES,
        0u,
        FALSE
    },
    {
        nSYAudioBGMWinFox,
        NDS_AUDIO_BGM_PATH_WIN_FOX,
        NDS_AUDIO_BGM_WIN_FOX_STREAM_BYTES,
        0u,
        FALSE
    },
    {
        nSYAudioBGMResults,
        NDS_AUDIO_BGM_PATH_RESULTS,
        NDS_AUDIO_BGM_RESULTS_STREAM_BYTES,
        NDS_AUDIO_BGM_RESULTS_LOOP_START_BYTES,
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
/* Last/max cpuGetTiming() delta around a BGM half-ring refill. Profile-1 only;
 * the published profile-0 ROM never declares or touches these. */
volatile u32 gNdsAudioBgmRefillTicksLast;
volatile u32 gNdsAudioBgmRefillTicksMax;
#endif
#if NDS_BGM_FALSIFIER_OFF
/* Compile-time label so the HUD can prove which A/B ROM is running. Always 0
 * in any build that did not set NDS_BGM_FALSIFIER_OFF=1. */
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
volatile u32 gNdsAudioBgmErrorStopCount;
volatile u32 gNdsAudioBgmErrorCleanupFailCount;

static u8 sNdsAudioBgmRing[NDS_AUDIO_BGM_CHUNK_BYTES] __attribute__((aligned(4)));
static FILE *sNdsAudioBgmFile;
static const NDSAudioBgmTrack *sNdsAudioBgmTrack;
static int sNdsAudioBgmSoundID = -1;
static u32 sNdsAudioBgmOffset;
static u32 sNdsAudioBgmLastTimerTick;
static u64 sNdsAudioBgmTimerTicksTotal;
static u64 sNdsAudioBgmNextRefillByte;
static u64 sNdsAudioBgmNextRefillTick;
static s32 sNdsAudioBgmNaturalStopArmed;
static u32 sNdsAudioBgmTrackReadStartBytes;

static void ndsAudioBgmKillSound(void);

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
    gNdsAudioBgmFileOpen = 0;
}

static s32 ndsAudioBgmOpenFile(void)
{
    if (sNdsAudioBgmFile != NULL)
    {
        return TRUE;
    }

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
    gNdsAudioBgmFileOpen = 1;
    return TRUE;
}

static s32 ndsAudioBgmLoopFile(void)
{
    if ((sNdsAudioBgmTrack == NULL) ||
        (sNdsAudioBgmTrack->is_looping == FALSE) ||
        (fseek(sNdsAudioBgmFile,
               (long)sNdsAudioBgmTrack->loop_start_bytes,
               SEEK_SET) != 0))
    {
        gNdsAudioBgmReadFailCount++;
        return FALSE;
    }
    sNdsAudioBgmOffset = sNdsAudioBgmTrack->loop_start_bytes;
    gNdsAudioBgmLoopCount++;
    gNdsAudioBgmMask |= 1u << 4;
    return TRUE;
}

static s32 ndsAudioBgmSeekFile(u32 offset)
{
    if (sNdsAudioBgmTrack == NULL)
    {
        gNdsAudioBgmReadFailCount++;
        return FALSE;
    }
    if (offset > sNdsAudioBgmTrack->stream_bytes)
    {
        offset = sNdsAudioBgmTrack->stream_bytes;
    }
    if (fseek(sNdsAudioBgmFile, (long)offset, SEEK_SET) != 0)
    {
        gNdsAudioBgmReadFailCount++;
        return FALSE;
    }
    sNdsAudioBgmOffset = offset;
    return TRUE;
}

static s32 ndsAudioBgmReadInto(u8 *dst, u32 byte_count)
{
    u32 copied = 0;

    while (copied < byte_count)
    {
        u32 remain;
        u32 want;
        size_t read_count;

        if (sNdsAudioBgmTrack == NULL)
        {
            gNdsAudioBgmReadFailCount++;
            return FALSE;
        }
        if (sNdsAudioBgmOffset >= sNdsAudioBgmTrack->stream_bytes)
        {
            if (sNdsAudioBgmTrack->is_looping == FALSE)
            {
                u32 silence_bytes = byte_count - copied;

                memset(&dst[copied], 0, silence_bytes);
                gNdsAudioBgmFinitePaddingBytes += silence_bytes;
                copied += silence_bytes;
                break;
            }
            if (ndsAudioBgmLoopFile() == FALSE)
            {
                return FALSE;
            }
        }

        remain = sNdsAudioBgmTrack->stream_bytes - sNdsAudioBgmOffset;
        want = byte_count - copied;
        if (want > remain)
        {
            want = remain;
        }
        read_count = fread(&dst[copied], 1, want, sNdsAudioBgmFile);
        if (read_count != want)
        {
            gNdsAudioBgmReadFailCount++;
            return FALSE;
        }
        copied += (u32)read_count;
        sNdsAudioBgmOffset += (u32)read_count;
        gNdsAudioBgmReadBytes += (u32)read_count;
    }

    DC_FlushRange(dst, byte_count);
    gNdsAudioBgmChunkBytes = byte_count;
    gNdsAudioBgmResidentBytes = NDS_AUDIO_BGM_CHUNK_BYTES;
    return TRUE;
}

static void ndsAudioBgmUpdateRateMarkers(void)
{
    u64 playback_bytes =
        (sNdsAudioBgmTimerTicksTotal * NDS_AUDIO_BGM_BYTES_PER_SECOND) /
        BUS_CLOCK;

    if ((gNdsAudioBgmReadBytes - sNdsAudioBgmTrackReadStartBytes) >
        NDS_AUDIO_BGM_CHUNK_BYTES)
    {
        gNdsAudioBgmStreamedBytes =
            (gNdsAudioBgmReadBytes - sNdsAudioBgmTrackReadStartBytes) -
            NDS_AUDIO_BGM_CHUNK_BYTES;
    }
    else
    {
        gNdsAudioBgmStreamedBytes = 0;
    }
    gNdsAudioBgmExpectedBytesPerSecond = NDS_AUDIO_BGM_BYTES_PER_SECOND;
    if (sNdsAudioBgmTimerTicksTotal != 0u)
    {
        gNdsAudioBgmStreamBytesPerSecond =
            (u32)((playback_bytes * BUS_CLOCK) /
                  sNdsAudioBgmTimerTicksTotal);
    }
    gNdsAudioBgmPlaybackPositionBytes =
        (u32)(playback_bytes % NDS_AUDIO_BGM_CHUNK_BYTES);
    gNdsAudioBgmPlaybackHalf =
        (gNdsAudioBgmPlaybackPositionBytes >= NDS_AUDIO_BGM_HALF_BYTES) ?
        1u : 0u;
    gNdsAudioBgmTimerTicks = (u32)sNdsAudioBgmTimerTicksTotal;
    gNdsAudioBgmPlaybackBytes = (u32)playback_bytes;
    if ((sNdsAudioBgmTrack != NULL) &&
        (sNdsAudioBgmTrack->is_looping != FALSE) &&
        (playback_bytes >= sNdsAudioBgmTrack->stream_bytes))
    {
        u32 loop_bytes = sNdsAudioBgmTrack->stream_bytes -
            sNdsAudioBgmTrack->loop_start_bytes;

        gNdsAudioBgmPlaybackLoopCount = 1u +
            (u32)((playback_bytes - sNdsAudioBgmTrack->stream_bytes) /
                  loop_bytes);
    }
    else
    {
        gNdsAudioBgmPlaybackLoopCount = 0u;
    }
}

static s32 ndsAudioBgmRefillHalf(u32 write_half, u32 playback_half)
{
    u32 write_pos = write_half * NDS_AUDIO_BGM_HALF_BYTES;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 refill_start = cpuGetTiming();
#endif

    gNdsAudioBgmPlaybackHalf = playback_half;
    gNdsAudioBgmWriteHalf = write_half;
    gNdsAudioBgmWritePositionBytes = write_pos;
    if (write_half == playback_half)
    {
        gNdsAudioBgmUnsafeWriteCount++;
    }
    if (ndsAudioBgmReadInto(&sNdsAudioBgmRing[write_pos],
                            NDS_AUDIO_BGM_HALF_BYTES) == FALSE)
    {
        return FALSE;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    /* Measure the half-ring refill cost (fread + DC_FlushRange inside
     * ndsAudioBgmReadInto). Device A/B correlation of these ticks against
     * 5-VBlank presentation intervals is the falsifier's decisive evidence. */
    {
        u32 refill_dt = cpuGetTiming() - refill_start;

        gNdsAudioBgmRefillTicksLast = refill_dt;
        if (refill_dt > gNdsAudioBgmRefillTicksMax)
        {
            gNdsAudioBgmRefillTicksMax = refill_dt;
        }
    }
#endif
    gNdsAudioBgmRefillCount++;
    gNdsAudioBgmChunkPlayCount++;
    return TRUE;
}

static s32 ndsAudioBgmResync(u64 playback_bytes)
{
    u64 current_half = playback_bytes / NDS_AUDIO_BGM_HALF_BYTES;
    u64 refill_boundary = (current_half + 1u) *
        NDS_AUDIO_BGM_HALF_BYTES;
    u32 write_half = (u32)((current_half + 1u) & 1u);
    u32 playback_half = (u32)(current_half & 1u);
    u32 file_offset = ndsAudioBgmMapPlaybackByte(refill_boundary);

    gNdsAudioBgmOverrunCount++;
    if (ndsAudioBgmSeekFile(file_offset) == FALSE)
    {
        return FALSE;
    }
    sNdsAudioBgmNextRefillByte = refill_boundary;
    sNdsAudioBgmNextRefillTick =
        ((refill_boundary / NDS_AUDIO_BGM_HALF_BYTES) *
         NDS_AUDIO_BGM_HALF_TICKS);
    return ndsAudioBgmRefillHalf(write_half, playback_half);
}

static s32 ndsAudioBgmPlayRing(void)
{
    ndsAudioBgmKillSound();

    sNdsAudioBgmSoundID = soundPlaySample(sNdsAudioBgmRing,
                                          SoundFormat_16Bit,
                                          NDS_AUDIO_BGM_CHUNK_BYTES,
                                          NDS_AUDIO_BGM_SAMPLE_RATE,
                                          ndsAudioBgmScaleVolume(gNdsAudioBgmVolume),
                                          64,
                                          true,
                                          0);
    if (sNdsAudioBgmSoundID < 0)
    {
        gNdsAudioBgmPlayFailCount++;
        return FALSE;
    }
    gNdsAudioBgmSoundActive = 1;
    gNdsAudioBgmChunkBytes = NDS_AUDIO_BGM_CHUNK_BYTES;
    gNdsAudioBgmResidentBytes = NDS_AUDIO_BGM_CHUNK_BYTES;
    gNdsAudioBgmPlaybackHalf = 0;
    gNdsAudioBgmWriteHalf = 0;
    gNdsAudioBgmWritePositionBytes = 0;
    gNdsAudioBgmPlaybackPositionBytes = 0;
    sNdsAudioBgmLastTimerTick = cpuGetTiming();
    sNdsAudioBgmTimerTicksTotal = 0;
    sNdsAudioBgmNextRefillByte = NDS_AUDIO_BGM_HALF_BYTES;
    sNdsAudioBgmNextRefillTick = NDS_AUDIO_BGM_HALF_TICKS;
    gNdsAudioBgmChunkPlayCount++;
    return TRUE;
}

static void ndsAudioBgmKillSound(void)
{
    if (sNdsAudioBgmSoundID >= 0)
    {
        soundKill(sNdsAudioBgmSoundID);
        sNdsAudioBgmSoundID = -1;
    }
    gNdsAudioBgmSoundActive = 0;
}

static void ndsAudioBgmFailPlayback(void)
{
    ndsAudioBgmKillSound();
    ndsAudioBgmCloseFile();
    gNdsAudioBgmPlaying = 0;
    gNdsAudioBgmErrorStopCount++;
    if ((sNdsAudioBgmSoundID >= 0) || (sNdsAudioBgmFile != NULL) ||
        (gNdsAudioBgmSoundActive != 0) || (gNdsAudioBgmFileOpen != 0))
    {
        gNdsAudioBgmErrorCleanupFailCount++;
    }
}

static void ndsAudioBgmFinishNaturally(void)
{
    ndsAudioBgmUpdateRateMarkers();
    ndsAudioBgmKillSound();
    ndsAudioBgmCloseFile();
    gNdsAudioBgmPlaying = 0;
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
    sNdsAudioBgmOffset = 0;
    sNdsAudioBgmLastTimerTick = 0;
    sNdsAudioBgmTimerTicksTotal = 0;
    sNdsAudioBgmNextRefillByte = 0;
    sNdsAudioBgmNextRefillTick = 0;
    sNdsAudioBgmNaturalStopArmed = FALSE;
    sNdsAudioBgmTrackReadStartBytes = 0;

    gNdsAudioBgmResult = 0;
    gNdsAudioBgmMask = 0;
    gNdsAudioBgmPlaying = 0;
    gNdsAudioBgmTrackID = 0;
    gNdsAudioBgmVolume = 0x7800u;
    gNdsAudioBgmPlayCalls = 0;
    gNdsAudioBgmStopCalls = 0;
    gNdsAudioBgmCheckCalls = 0;
    gNdsAudioBgmSetVolumeCalls = 0;
    gNdsAudioBgmOpenFailCount = 0;
    gNdsAudioBgmReadFailCount = 0;
    gNdsAudioBgmUnsupportedTrackCount = 0;
    gNdsAudioBgmReadBytes = 0;
    gNdsAudioBgmResidentBytes = 0;
    gNdsAudioBgmChunkBytes = 0;
    gNdsAudioBgmChunkPlayCount = 0;
    gNdsAudioBgmStoppedOnTeardown = 0;
    gNdsAudioBgmElapsedFrames = 0;
    gNdsAudioBgmStreamedBytes = 0;
    gNdsAudioBgmStreamBytesPerSecond = 0;
    gNdsAudioBgmExpectedBytesPerSecond = NDS_AUDIO_BGM_BYTES_PER_SECOND;
    gNdsAudioBgmLoopCount = 0;
    gNdsAudioBgmRefillCount = 0;
    gNdsAudioBgmPlaybackPositionBytes = 0;
    gNdsAudioBgmWritePositionBytes = 0;
    gNdsAudioBgmPlaybackHalf = 0;
    gNdsAudioBgmWriteHalf = 0;
    gNdsAudioBgmUnsafeWriteCount = 0;
    gNdsAudioBgmTimerTicks = 0;
    gNdsAudioBgmPlaybackBytes = 0;
    gNdsAudioBgmPlaybackLoopCount = 0;
    gNdsAudioBgmOverrunCount = 0;
    gNdsAudioBgmStreamBytes = 0;
    gNdsAudioBgmLoopStartBytes = 0;
    gNdsAudioBgmIsLooping = 0;
    gNdsAudioBgmPupupuPlayCount = 0;
    gNdsAudioBgmWinMarioPlayCount = 0;
    gNdsAudioBgmWinFoxPlayCount = 0;
    gNdsAudioBgmResultsPlayCount = 0;
    gNdsAudioBgmNaturalStopCount = 0;
    gNdsAudioBgmLastNaturalStopTrackID = 0xFFFFFFFFu;
    gNdsAudioBgmPostNaturalTransitionCount = 0;
    gNdsAudioBgmPostNaturalTransitionFromTrackID = 0xFFFFFFFFu;
    gNdsAudioBgmPostNaturalTransitionToTrackID = 0xFFFFFFFFu;
    gNdsAudioBgmTrackSwitchCount = 0;
    gNdsAudioBgmFinitePaddingBytes = 0;
    gNdsAudioBgmFileOpen = 0;
    gNdsAudioBgmSoundActive = 0;
    gNdsAudioBgmPlayFailCount = 0;
    gNdsAudioBgmErrorStopCount = 0;
    gNdsAudioBgmErrorCleanupFailCount = 0;
}

void ndsAudioBgmPlay(s32 player, s32 bgm_id)
{
    const NDSAudioBgmTrack *track;

    (void)player;

    gNdsAudioBgmPlayCalls++;

    track = ndsAudioBgmFindTrack(bgm_id);
    if (track == NULL)
    {
        gNdsAudioBgmUnsupportedTrackCount++;
        return;
    }
    gNdsAudioBgmTrackID = (u32)bgm_id;
    if (gNdsAudioBgmPlaying != 0)
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
    sNdsAudioBgmOffset = 0;
    sNdsAudioBgmTrackReadStartBytes = gNdsAudioBgmReadBytes;
    gNdsAudioBgmStreamBytes = track->stream_bytes;
    gNdsAudioBgmLoopStartBytes = track->loop_start_bytes;
    gNdsAudioBgmIsLooping = (track->is_looping != FALSE) ? 1u : 0u;
    gNdsAudioBgmStreamedBytes = 0;
    gNdsAudioBgmPlaybackLoopCount = 0;
    switch (bgm_id)
    {
    case nSYAudioBGMPupupu:
        gNdsAudioBgmPupupuPlayCount++;
        break;
    case nSYAudioBGMWinMario:
        gNdsAudioBgmWinMarioPlayCount++;
        break;
    case nSYAudioBGMWinFox:
        gNdsAudioBgmWinFoxPlayCount++;
        break;
    case nSYAudioBGMResults:
        gNdsAudioBgmResultsPlayCount++;
        break;
    }
    if ((gNdsAudioBgmSetVolumeCalls == 0) && (gNdsAudioBgmVolume == 0))
    {
        gNdsAudioBgmVolume = 0x7800u;
    }

    soundEnable();

#if NDS_BGM_FALSIFIER_OFF
    /* Falsifier B path: skip open/read/play so no storage I/O or cache flush
     * runs, but still mark BGM active so the rest of the system (and every
     * BGM counter above) believes playback is running. ndsAudioBgmUpdate then
     * short-circuits on sNdsAudioBgmFile == NULL, so no refill work happens. */
    (void)ndsAudioBgmOpenFile;
    (void)ndsAudioBgmReadInto;
    (void)ndsAudioBgmPlayRing;
#else
    if ((ndsAudioBgmOpenFile() == FALSE) ||
        (ndsAudioBgmReadInto(sNdsAudioBgmRing,
                             NDS_AUDIO_BGM_CHUNK_BYTES) == FALSE))
    {
        ndsAudioBgmFailPlayback();
        return;
    }

    if (ndsAudioBgmPlayRing() == FALSE)
    {
        ndsAudioBgmFailPlayback();
        return;
    }
#endif
    gNdsAudioBgmPlaying = 1;
    gNdsAudioBgmMask |= 1u << 0;
    gNdsAudioBgmResult = NDS_AUDIO_BGM_PASS;
}

void ndsAudioBgmStopAll(void)
{
    gNdsAudioBgmStopCalls++;
    ndsAudioBgmKillSound();
    if (gNdsAudioBgmPlaying != 0)
    {
        gNdsAudioBgmStoppedOnTeardown = 1;
    }
    gNdsAudioBgmPlaying = 0;
    ndsAudioBgmCloseFile();
    sNdsAudioBgmOffset = 0;
    sNdsAudioBgmLastTimerTick = 0;
    sNdsAudioBgmTimerTicksTotal = 0;
    sNdsAudioBgmNextRefillByte = 0;
    sNdsAudioBgmNextRefillTick = 0;
    sNdsAudioBgmNaturalStopArmed = FALSE;
    gNdsAudioBgmMask |= 1u << 1;
}

s32 ndsAudioBgmCheckPlaying(s32 player)
{
    (void)player;

    gNdsAudioBgmCheckCalls++;
    gNdsAudioBgmMask |= 1u << 2;
    return (gNdsAudioBgmPlaying != 0) ? TRUE : FALSE;
}

s32 ndsAudioBgmIsPlaying(void)
{
    return (gNdsAudioBgmPlaying != 0) ? TRUE : FALSE;
}

void ndsAudioBgmSetVolume(s32 player, u32 vol)
{
    (void)player;

    gNdsAudioBgmSetVolumeCalls++;
    gNdsAudioBgmVolume = vol;
    if (sNdsAudioBgmSoundID >= 0)
    {
        soundSetVolume(sNdsAudioBgmSoundID, ndsAudioBgmScaleVolume(vol));
    }
    gNdsAudioBgmMask |= 1u << 3;
}

void ndsAudioBgmUpdate(void)
{
    u32 delta;
    u64 playback_bytes;

    if ((gNdsAudioBgmPlaying == 0) || (sNdsAudioBgmFile == NULL))
    {
        return;
    }
    gNdsAudioBgmElapsedFrames++;
#if NDS_HARNESS_FAST_LOGIC
    /* Fast lifecycle verification removes retrace waits but retains one source
     * update per tic. Advance audio by that same 60 Hz contract so finite
     * winner sequences can stop and wake the imported Results audio thread. */
    delta = BUS_CLOCK / 60u;
#else
    {
        u32 now = cpuGetTiming();

        delta = now - sNdsAudioBgmLastTimerTick;
        sNdsAudioBgmLastTimerTick = now;
    }
#endif
    sNdsAudioBgmTimerTicksTotal += delta;
    playback_bytes =
        (sNdsAudioBgmTimerTicksTotal * NDS_AUDIO_BGM_BYTES_PER_SECOND) /
        BUS_CLOCK;
    ndsAudioBgmUpdateRateMarkers();

    if ((sNdsAudioBgmTrack->is_looping == FALSE) &&
        (playback_bytes >= sNdsAudioBgmTrack->stream_bytes))
    {
        ndsAudioBgmFinishNaturally();
        return;
    }

    while (sNdsAudioBgmTimerTicksTotal >= sNdsAudioBgmNextRefillTick)
    {
        u64 refill_half_index =
            sNdsAudioBgmNextRefillByte / NDS_AUDIO_BGM_HALF_BYTES;
        u32 playback_half = (u32)(refill_half_index & 1u);
        u32 write_half = (u32)((refill_half_index - 1u) & 1u);

        if ((sNdsAudioBgmTimerTicksTotal - sNdsAudioBgmNextRefillTick) >=
            NDS_AUDIO_BGM_HALF_TICKS)
        {
            playback_bytes =
                (sNdsAudioBgmTimerTicksTotal *
                 NDS_AUDIO_BGM_BYTES_PER_SECOND) / BUS_CLOCK;
            if (ndsAudioBgmResync(playback_bytes) == FALSE)
            {
                ndsAudioBgmFailPlayback();
                return;
            }
            break;
        }
        if (write_half == playback_half)
        {
            gNdsAudioBgmUnsafeWriteCount++;
            break;
        }
        if (ndsAudioBgmRefillHalf(write_half, playback_half) == FALSE)
        {
            ndsAudioBgmFailPlayback();
            return;
        }
        sNdsAudioBgmNextRefillByte += NDS_AUDIO_BGM_HALF_BYTES;
        sNdsAudioBgmNextRefillTick += NDS_AUDIO_BGM_HALF_TICKS;
        ndsAudioBgmUpdateRateMarkers();
    }
}
