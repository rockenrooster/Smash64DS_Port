#include <nds.h>
#include <stdio.h>
#include <string.h>

#include <gm/gmsound.h>
#include <nds/nds_audio_bgm.h>

#define NDS_AUDIO_BGM_PATH "nitro:/audio/bgm_pupupu_pcm16.raw"
#define NDS_AUDIO_BGM_FRAMES_PER_SECOND 60u
#define NDS_AUDIO_BGM_REFILL_THRESHOLD \
    (NDS_AUDIO_BGM_HALF_BYTES * NDS_AUDIO_BGM_FRAMES_PER_SECOND)

_Static_assert((NDS_AUDIO_BGM_CHUNK_BYTES % 2u) == 0u,
               "BGM ring buffer must split into two whole halves");
_Static_assert(NDS_AUDIO_BGM_BYTES_PER_SECOND == 44100u,
               "Pupupu PCM16 mono stream must consume 44100 bytes/sec");

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
volatile u32 gNdsAudioBgmPlaybackPositionBytes;
volatile u32 gNdsAudioBgmWritePositionBytes;
volatile u32 gNdsAudioBgmPlaybackHalf;
volatile u32 gNdsAudioBgmWriteHalf;
volatile u32 gNdsAudioBgmUnsafeWriteCount;

static u8 sNdsAudioBgmRing[NDS_AUDIO_BGM_CHUNK_BYTES] __attribute__((aligned(4)));
static FILE *sNdsAudioBgmFile;
static int sNdsAudioBgmSoundID = -1;
static u32 sNdsAudioBgmOffset;
static u32 sNdsAudioBgmFrameByteAccumulator;
static u32 sNdsAudioBgmNextWriteHalf;

static u8 ndsAudioBgmScaleVolume(u32 vol)
{
    if (vol >= 0x7800u)
    {
        return 127u;
    }
    return (u8)((vol * 127u) / 0x7800u);
}

static void ndsAudioBgmCloseFile(void)
{
    if (sNdsAudioBgmFile != NULL)
    {
        fclose(sNdsAudioBgmFile);
        sNdsAudioBgmFile = NULL;
    }
}

static s32 ndsAudioBgmOpenFile(void)
{
    if (sNdsAudioBgmFile != NULL)
    {
        return TRUE;
    }

    sNdsAudioBgmFile = fopen(NDS_AUDIO_BGM_PATH, "rb");
    if (sNdsAudioBgmFile == NULL)
    {
        gNdsAudioBgmOpenFailCount++;
        return FALSE;
    }
    return TRUE;
}

static s32 ndsAudioBgmRewindFile(void)
{
    if (fseek(sNdsAudioBgmFile, 0, SEEK_SET) != 0)
    {
        gNdsAudioBgmReadFailCount++;
        return FALSE;
    }
    sNdsAudioBgmOffset = 0;
    gNdsAudioBgmLoopCount++;
    gNdsAudioBgmMask |= 1u << 4;
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

        if (sNdsAudioBgmOffset >= NDS_AUDIO_BGM_STREAM_BYTES)
        {
            if (ndsAudioBgmRewindFile() == FALSE)
            {
                return FALSE;
            }
        }

        remain = NDS_AUDIO_BGM_STREAM_BYTES - sNdsAudioBgmOffset;
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
    u32 playback_bytes =
        (gNdsAudioBgmElapsedFrames * NDS_AUDIO_BGM_BYTES_PER_SECOND) /
        NDS_AUDIO_BGM_FRAMES_PER_SECOND;

    if (gNdsAudioBgmReadBytes > NDS_AUDIO_BGM_CHUNK_BYTES)
    {
        gNdsAudioBgmStreamedBytes =
            gNdsAudioBgmReadBytes - NDS_AUDIO_BGM_CHUNK_BYTES;
    }
    else
    {
        gNdsAudioBgmStreamedBytes = 0;
    }
    gNdsAudioBgmExpectedBytesPerSecond = NDS_AUDIO_BGM_BYTES_PER_SECOND;
    if (gNdsAudioBgmElapsedFrames != 0)
    {
        gNdsAudioBgmStreamBytesPerSecond =
            (gNdsAudioBgmStreamedBytes * NDS_AUDIO_BGM_FRAMES_PER_SECOND) /
            gNdsAudioBgmElapsedFrames;
    }
    gNdsAudioBgmPlaybackPositionBytes =
        playback_bytes % NDS_AUDIO_BGM_CHUNK_BYTES;
}

static s32 ndsAudioBgmRefillHalf(void)
{
    u32 write_half = sNdsAudioBgmNextWriteHalf;
    u32 playback_half = write_half ^ 1u;
    u32 write_pos = write_half * NDS_AUDIO_BGM_HALF_BYTES;

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
    sNdsAudioBgmNextWriteHalf = playback_half;
    gNdsAudioBgmRefillCount++;
    gNdsAudioBgmChunkPlayCount++;
    return TRUE;
}

static void ndsAudioBgmPlayRing(void)
{
    if (sNdsAudioBgmSoundID >= 0)
    {
        soundKill(sNdsAudioBgmSoundID);
    }

    sNdsAudioBgmSoundID = soundPlaySample(sNdsAudioBgmRing,
                                          SoundFormat_16Bit,
                                          NDS_AUDIO_BGM_CHUNK_BYTES,
                                          NDS_AUDIO_BGM_SAMPLE_RATE,
                                          ndsAudioBgmScaleVolume(gNdsAudioBgmVolume),
                                          64,
                                          true,
                                          0);
    gNdsAudioBgmChunkBytes = NDS_AUDIO_BGM_CHUNK_BYTES;
    gNdsAudioBgmResidentBytes = NDS_AUDIO_BGM_CHUNK_BYTES;
    gNdsAudioBgmPlaybackHalf = 0;
    gNdsAudioBgmWriteHalf = 0;
    gNdsAudioBgmWritePositionBytes = 0;
    gNdsAudioBgmPlaybackPositionBytes = 0;
    gNdsAudioBgmChunkPlayCount++;
}

void ndsAudioBgmDiagnosticsReset(void)
{
    if (sNdsAudioBgmSoundID >= 0)
    {
        soundKill(sNdsAudioBgmSoundID);
    }
    sNdsAudioBgmSoundID = -1;
    ndsAudioBgmCloseFile();
    sNdsAudioBgmOffset = 0;
    sNdsAudioBgmFrameByteAccumulator = 0;
    sNdsAudioBgmNextWriteHalf = 0;

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
}

void ndsAudioBgmPlay(s32 player, s32 bgm_id)
{
    (void)player;

    gNdsAudioBgmPlayCalls++;
    gNdsAudioBgmTrackID = (u32)bgm_id;

    if (bgm_id != nSYAudioBGMPupupu)
    {
        gNdsAudioBgmUnsupportedTrackCount++;
        gNdsAudioBgmPlaying = 0;
        return;
    }
    if ((gNdsAudioBgmSetVolumeCalls == 0) && (gNdsAudioBgmVolume == 0))
    {
        gNdsAudioBgmVolume = 0x7800u;
    }

    soundEnable();

    if ((ndsAudioBgmOpenFile() == FALSE) ||
        (ndsAudioBgmReadInto(sNdsAudioBgmRing,
                             NDS_AUDIO_BGM_CHUNK_BYTES) == FALSE))
    {
        gNdsAudioBgmPlaying = 0;
        return;
    }

    ndsAudioBgmPlayRing();
    gNdsAudioBgmPlaying = 1;
    gNdsAudioBgmMask |= 1u << 0;
    gNdsAudioBgmResult = NDS_AUDIO_BGM_PASS;
}

void ndsAudioBgmStopAll(void)
{
    gNdsAudioBgmStopCalls++;
    if (sNdsAudioBgmSoundID >= 0)
    {
        soundKill(sNdsAudioBgmSoundID);
        sNdsAudioBgmSoundID = -1;
    }
    if (gNdsAudioBgmPlaying != 0)
    {
        gNdsAudioBgmStoppedOnTeardown = 1;
    }
    gNdsAudioBgmPlaying = 0;
    ndsAudioBgmCloseFile();
    sNdsAudioBgmOffset = 0;
    sNdsAudioBgmFrameByteAccumulator = 0;
    sNdsAudioBgmNextWriteHalf = 0;
    gNdsAudioBgmMask |= 1u << 1;
}

s32 ndsAudioBgmCheckPlaying(s32 player)
{
    (void)player;

    gNdsAudioBgmCheckCalls++;
    gNdsAudioBgmMask |= 1u << 2;
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
    if ((gNdsAudioBgmPlaying == 0) || (sNdsAudioBgmFile == NULL))
    {
        return;
    }
    gNdsAudioBgmElapsedFrames++;
    sNdsAudioBgmFrameByteAccumulator += NDS_AUDIO_BGM_BYTES_PER_SECOND;
    while (sNdsAudioBgmFrameByteAccumulator >= NDS_AUDIO_BGM_REFILL_THRESHOLD)
    {
        sNdsAudioBgmFrameByteAccumulator -= NDS_AUDIO_BGM_REFILL_THRESHOLD;
        if (ndsAudioBgmRefillHalf() == FALSE)
        {
            gNdsAudioBgmPlaying = 0;
            return;
        }
    }
    ndsAudioBgmUpdateRateMarkers();
}
