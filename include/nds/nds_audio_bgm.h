#ifndef SSB64_NDS_AUDIO_BGM_H
#define SSB64_NDS_AUDIO_BGM_H

#include <PR/ultratypes.h>

#define NDS_AUDIO_BGM_PASS 0x42474d31u /* BGM1 */
#define NDS_AUDIO_BGM_TRACK_PUPUPU 0u
#define NDS_AUDIO_BGM_SAMPLE_RATE 22050u
#define NDS_AUDIO_BGM_STREAM_BYTES 2886710u
#define NDS_AUDIO_BGM_STREAM_SHA256_LO 0x9138effau
#define NDS_AUDIO_BGM_CHUNK_BYTES 65536u
#define NDS_AUDIO_BGM_HALF_BYTES (NDS_AUDIO_BGM_CHUNK_BYTES / 2u)
#define NDS_AUDIO_BGM_BYTES_PER_SECOND (NDS_AUDIO_BGM_SAMPLE_RATE * 2u)
#define NDS_AUDIO_BGM_TRACK_FRAMES 3928u
#define NDS_AUDIO_BGM_RATE_GUARD_FRAMES 3200u

void ndsAudioBgmDiagnosticsReset(void);
void ndsAudioBgmUpdate(void);
void ndsAudioBgmStopAll(void);
void ndsAudioBgmPlay(s32 player, s32 bgm_id);
s32 ndsAudioBgmCheckPlaying(s32 player);
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
extern volatile u32 gNdsAudioBgmPlaybackPositionBytes;
extern volatile u32 gNdsAudioBgmWritePositionBytes;
extern volatile u32 gNdsAudioBgmPlaybackHalf;
extern volatile u32 gNdsAudioBgmWriteHalf;
extern volatile u32 gNdsAudioBgmUnsafeWriteCount;
extern volatile u32 gNdsAudioBgmTimerTicks;
extern volatile u32 gNdsAudioBgmPlaybackBytes;
extern volatile u32 gNdsAudioBgmPlaybackLoopCount;
extern volatile u32 gNdsAudioBgmOverrunCount;

#endif
