#ifndef SSB64_NDS_AUDIO_BGM_H
#define SSB64_NDS_AUDIO_BGM_H

#include <PR/ultratypes.h>

#define NDS_AUDIO_BGM_PASS 0x42474d31u /* BGM1 */
#define NDS_AUDIO_BGM_TRACK_PUPUPU 0u
#define NDS_AUDIO_BGM_TRACK_WIN_MARIO 12u
#define NDS_AUDIO_BGM_TRACK_WIN_FOX 16u
#define NDS_AUDIO_BGM_TRACK_RESULTS 22u
#define NDS_AUDIO_BGM_SAMPLE_RATE 22050u
#define NDS_AUDIO_BGM_PUPUPU_STREAM_BYTES 2886710u
#define NDS_AUDIO_BGM_PUPUPU_LOOP_START_BYTES 8798u
#define NDS_AUDIO_BGM_PUPUPU_STREAM_SHA256_LO 0x9138effau
#define NDS_AUDIO_BGM_PUPUPU_ASSET_BYTES 722788u
#define NDS_AUDIO_BGM_PUPUPU_ASSET_SHA256_LO 0x3a9ad956u
#define NDS_AUDIO_BGM_PUPUPU_PACKET_COUNT 89u
#define NDS_AUDIO_BGM_PUPUPU_LOOP_PACKET 1u
#define NDS_AUDIO_BGM_PUPUPU_LOOP_RECORD 2252u
#define NDS_AUDIO_BGM_WIN_MARIO_STREAM_BYTES 326800u
#define NDS_AUDIO_BGM_WIN_MARIO_STREAM_SHA256_LO 0xa9239018u
#define NDS_AUDIO_BGM_WIN_MARIO_ASSET_BYTES 81860u
#define NDS_AUDIO_BGM_WIN_MARIO_ASSET_SHA256_LO 0x31d45e75u
#define NDS_AUDIO_BGM_WIN_MARIO_PACKET_COUNT 10u
#define NDS_AUDIO_BGM_WIN_FOX_STREAM_BYTES 291154u
#define NDS_AUDIO_BGM_WIN_FOX_STREAM_SHA256_LO 0xb784d66cu
#define NDS_AUDIO_BGM_WIN_FOX_ASSET_BYTES 72940u
#define NDS_AUDIO_BGM_WIN_FOX_ASSET_SHA256_LO 0xfe2501e9u
#define NDS_AUDIO_BGM_WIN_FOX_PACKET_COUNT 9u
#define NDS_AUDIO_BGM_RESULTS_STREAM_BYTES 1624750u
#define NDS_AUDIO_BGM_RESULTS_LOOP_START_BYTES 34912u
#define NDS_AUDIO_BGM_RESULTS_STREAM_SHA256_LO 0x68d32bd8u
#define NDS_AUDIO_BGM_RESULTS_ASSET_BYTES 406840u
#define NDS_AUDIO_BGM_RESULTS_ASSET_SHA256_LO 0xf27c7e0cu
#define NDS_AUDIO_BGM_RESULTS_PACKET_COUNT 51u
#define NDS_AUDIO_BGM_RESULTS_LOOP_PACKET 2u
#define NDS_AUDIO_BGM_RESULTS_LOOP_RECORD 8792u
/* Retain the first stream's names for older Boundary diagnostics. */
#define NDS_AUDIO_BGM_STREAM_BYTES NDS_AUDIO_BGM_PUPUPU_STREAM_BYTES
#define NDS_AUDIO_BGM_STREAM_SHA256_LO NDS_AUDIO_BGM_PUPUPU_STREAM_SHA256_LO
#define NDS_AUDIO_BGM_CONTAINER_MAGIC 0x31414742u /* BGA1 */
#define NDS_AUDIO_BGM_CONTAINER_VERSION 1u
#define NDS_AUDIO_BGM_CONTAINER_HEADER_BYTES 40u
#define NDS_AUDIO_BGM_PACKET_HEADER_BYTES 8u
#define NDS_AUDIO_BGM_PACKET_SAMPLES 16384u
#define NDS_AUDIO_BGM_PACKET_BYTES 8196u
#define NDS_AUDIO_BGM_BUFFER_COUNT 2u
#define NDS_AUDIO_BGM_RESIDENT_BYTES \
    (NDS_AUDIO_BGM_BUFFER_COUNT * NDS_AUDIO_BGM_PACKET_BYTES)
#define NDS_AUDIO_BGM_BYTES_PER_SECOND (NDS_AUDIO_BGM_SAMPLE_RATE * 2u)
#define NDS_AUDIO_BGM_TRACK_FRAMES 3928u
#define NDS_AUDIO_BGM_RATE_GUARD_FRAMES 3200u
#define NDS_AUDIO_BGM_RESULTS_FAST_UPDATE_MAX 600u

void ndsAudioBgmDiagnosticsReset(void);
void ndsAudioBgmUpdate(void);
void ndsAudioBgmStopAll(void);
void ndsAudioBgmPlay(s32 player, s32 bgm_id);
s32 ndsAudioBgmCheckPlaying(s32 player);
s32 ndsAudioBgmIsPlaying(void);
void ndsAudioBgmSetVolume(s32 player, u32 vol);

extern volatile u32 gNdsAudioBgmResult;
extern volatile u32 gNdsAudioBgmMask;
extern volatile u32 gNdsAudioBgmPlaying;
extern volatile u32 gNdsAudioBgmTrackID;
extern volatile u32 gNdsAudioBgmVolume;
extern volatile u32 gNdsAudioBgmPlayCalls;
extern volatile u32 gNdsAudioBgmStopCalls;
extern volatile u32 gNdsAudioBgmCheckCalls;
extern volatile u32 gNdsAudioBgmSetVolumeCalls;
extern volatile u32 gNdsAudioBgmOpenFailCount;
extern volatile u32 gNdsAudioBgmReadFailCount;
extern volatile u32 gNdsAudioBgmUnsupportedTrackCount;
extern volatile u32 gNdsAudioBgmReadBytes;
extern volatile u32 gNdsAudioBgmResidentBytes;
extern volatile u32 gNdsAudioBgmChunkBytes;
extern volatile u32 gNdsAudioBgmChunkPlayCount;
extern volatile u32 gNdsAudioBgmStoppedOnTeardown;
extern volatile u32 gNdsAudioBgmElapsedFrames;
extern volatile u32 gNdsAudioBgmStreamedBytes;
extern volatile u32 gNdsAudioBgmStreamBytesPerSecond;
extern volatile u32 gNdsAudioBgmExpectedBytesPerSecond;
extern volatile u32 gNdsAudioBgmLoopCount;
extern volatile u32 gNdsAudioBgmRefillCount;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
extern volatile u32 gNdsAudioBgmRefillTicksLast;
extern volatile u32 gNdsAudioBgmRefillTicksMax;
#endif
extern volatile u32 gNdsAudioBgmFalsifierOff;
extern volatile u32 gNdsAudioBgmPlaybackPositionBytes;
extern volatile u32 gNdsAudioBgmWritePositionBytes;
extern volatile u32 gNdsAudioBgmPlaybackHalf;
extern volatile u32 gNdsAudioBgmWriteHalf;
extern volatile u32 gNdsAudioBgmUnsafeWriteCount;
extern volatile u32 gNdsAudioBgmTimerTicks;
extern volatile u32 gNdsAudioBgmPlaybackBytes;
extern volatile u32 gNdsAudioBgmPlaybackLoopCount;
extern volatile u32 gNdsAudioBgmOverrunCount;
extern volatile u32 gNdsAudioBgmStreamBytes;
extern volatile u32 gNdsAudioBgmLoopStartBytes;
extern volatile u32 gNdsAudioBgmIsLooping;
extern volatile u32 gNdsAudioBgmPupupuPlayCount;
extern volatile u32 gNdsAudioBgmWinMarioPlayCount;
extern volatile u32 gNdsAudioBgmWinFoxPlayCount;
extern volatile u32 gNdsAudioBgmResultsPlayCount;
extern volatile u32 gNdsAudioBgmNaturalStopCount;
extern volatile u32 gNdsAudioBgmLastNaturalStopTrackID;
extern volatile u32 gNdsAudioBgmPostNaturalTransitionCount;
extern volatile u32 gNdsAudioBgmPostNaturalTransitionFromTrackID;
extern volatile u32 gNdsAudioBgmPostNaturalTransitionToTrackID;
extern volatile u32 gNdsAudioBgmTrackSwitchCount;
extern volatile u32 gNdsAudioBgmFinitePaddingBytes;
extern volatile u32 gNdsAudioBgmFileOpen;
extern volatile u32 gNdsAudioBgmSoundActive;
extern volatile u32 gNdsAudioBgmPlayFailCount;
extern volatile u32 gNdsAudioBgmHeaderFailCount;
extern volatile u32 gNdsAudioBgmPacketFailCount;
extern volatile u32 gNdsAudioBgmPreparedCount;
extern volatile u32 gNdsAudioBgmSeamStartCount;
extern volatile u32 gNdsAudioBgmSeamMissCount;
extern volatile u32 gNdsAudioBgmTimerEventDropCount;
extern volatile u32 gNdsAudioBgmWorkerWakeCount;
extern volatile u32 gNdsAudioBgmErrorStopCount;
extern volatile u32 gNdsAudioBgmErrorCleanupFailCount;

#endif
