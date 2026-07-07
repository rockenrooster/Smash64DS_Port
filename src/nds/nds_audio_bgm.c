#include <nds.h>
#include <stdio.h>
#include <string.h>

#include <gm/gmsound.h>
#include <nds/nds_audio_bgm.h>

#define NDS_AUDIO_BGM_PATH "nitro:/audio/bgm_pupupu_pcm16.raw"

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

static u8 sNdsAudioBgmChunk[NDS_AUDIO_BGM_CHUNK_BYTES] __attribute__((aligned(4)));
static FILE *sNdsAudioBgmFile;
static int sNdsAudioBgmSoundID = -1;
static u32 sNdsAudioBgmOffset;
static u32 sNdsAudioBgmFramesUntilNext;

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

static s32 ndsAudioBgmReadNextChunk(void)
{
    size_t read_count;

    if (sNdsAudioBgmOffset >= NDS_AUDIO_BGM_STREAM_BYTES)
    {
        if (fseek(sNdsAudioBgmFile, 0, SEEK_SET) != 0)
        {
            gNdsAudioBgmReadFailCount++;
            return FALSE;
        }
        sNdsAudioBgmOffset = 0;
    }

    read_count = fread(sNdsAudioBgmChunk, 1, NDS_AUDIO_BGM_CHUNK_BYTES, sNdsAudioBgmFile);
    if (read_count == 0)
    {
        gNdsAudioBgmReadFailCount++;
        return FALSE;
    }

    sNdsAudioBgmOffset += (u32)read_count;
    gNdsAudioBgmReadBytes += (u32)read_count;
    gNdsAudioBgmChunkBytes = (u32)read_count;
    gNdsAudioBgmResidentBytes = NDS_AUDIO_BGM_CHUNK_BYTES;
    return TRUE;
}

static void ndsAudioBgmPlayChunk(void)
{
    u32 samples;

    if (sNdsAudioBgmSoundID >= 0)
    {
        soundKill(sNdsAudioBgmSoundID);
    }

    sNdsAudioBgmSoundID = soundPlaySample(sNdsAudioBgmChunk,
                                          SoundFormat_16Bit,
                                          gNdsAudioBgmChunkBytes,
                                          NDS_AUDIO_BGM_SAMPLE_RATE,
                                          ndsAudioBgmScaleVolume(gNdsAudioBgmVolume),
                                          64,
                                          false,
                                          0);
    samples = gNdsAudioBgmChunkBytes / 2u;
    sNdsAudioBgmFramesUntilNext =
        ((samples * 60u) + (NDS_AUDIO_BGM_SAMPLE_RATE - 1u)) /
        NDS_AUDIO_BGM_SAMPLE_RATE;
    if (sNdsAudioBgmFramesUntilNext == 0)
    {
        sNdsAudioBgmFramesUntilNext = 1;
    }
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
    sNdsAudioBgmFramesUntilNext = 0;

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

    if ((ndsAudioBgmOpenFile() == FALSE) || (ndsAudioBgmReadNextChunk() == FALSE))
    {
        gNdsAudioBgmPlaying = 0;
        return;
    }

    ndsAudioBgmPlayChunk();
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
    sNdsAudioBgmFramesUntilNext = 0;
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
    if (sNdsAudioBgmFramesUntilNext > 0)
    {
        sNdsAudioBgmFramesUntilNext--;
        return;
    }

    if (ndsAudioBgmReadNextChunk() == FALSE)
    {
        gNdsAudioBgmPlaying = 0;
        return;
    }
    ndsAudioBgmPlayChunk();
}
