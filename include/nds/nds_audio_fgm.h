#ifndef SSB64_NDS_AUDIO_FGM_H
#define SSB64_NDS_AUDIO_FGM_H

#include <PR/ultratypes.h>
#include <sys/audio.h>

#define NDS_AUDIO_FGM_PASS 0x46474d31u /* FGM1 */
#define NDS_AUDIO_FGM_ENTRY_COUNT 49u
#define NDS_AUDIO_FGM_PHASE_COUNT 5u
#define NDS_AUDIO_FGM_PHASE_COMPLETE_MASK 0x1fu
#define NDS_AUDIO_FGM_KO_COUNT 5u
#define NDS_AUDIO_FGM_KO_TRACE_CAPACITY 8u
#define NDS_AUDIO_FGM_MISS_RING_CAPACITY 16u
#ifndef NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
#define NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS 0
#endif
#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
#define NDS_AUDIO_FGM_ARM7_ACK_EVENT_CAPACITY 2u
#define NDS_AUDIO_FGM_ARM7_ACK_KIND_PLAY 1u
#define NDS_AUDIO_FGM_ARM7_ACK_KIND_STOP 3u
#define NDS_AUDIO_FGM_RELEASE_REASON_RESET 1u
#define NDS_AUDIO_FGM_RELEASE_REASON_GENERATION_LOST 2u
#define NDS_AUDIO_FGM_RELEASE_REASON_DURATION 3u
#define NDS_AUDIO_FGM_RELEASE_REASON_STOP_ALL 4u
#define NDS_AUDIO_FGM_RELEASE_REASON_EXPLICIT 5u
#endif
#define NDS_AUDIO_FGM_PACK_BYTES 415432u
#define NDS_AUDIO_FGM_PACK_MAPPING_SHA256_LO 0xde193efau
#define NDS_AUDIO_FGM_CACHE_BYTES 204800u
#define NDS_AUDIO_FGM_HANDLE_CAPACITY 8u
#define NDS_AUDIO_FGM_FIDELITY_DEBT_PITCH_AUTOMATION (1u << 2)
#define NDS_AUDIO_FGM_FIDELITY_DEBT_FORK_VOICE (1u << 3)
#define NDS_AUDIO_FGM_FIDELITY_DEBT_VOLUME_AUTOMATION (1u << 4)
#define NDS_AUDIO_FGM_FIDELITY_DEBT_CUSTOM_FX (1u << 5)
#define NDS_AUDIO_FGM_EXPECTED_FIDELITY_DEBT_MASK \
    (NDS_AUDIO_FGM_FIDELITY_DEBT_PITCH_AUTOMATION | \
     NDS_AUDIO_FGM_FIDELITY_DEBT_VOLUME_AUTOMATION | \
     NDS_AUDIO_FGM_FIDELITY_DEBT_CUSTOM_FX)

#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
typedef struct NDSAudioFgmArm7AckEvent {
    u32 kind;
    u32 source_tick;
    u32 value;
    u32 service_tick;
    u32 command_tick;
    u32 command_return_tick;
    u32 acknowledge_tick;
    u32 active_channels;
} NDSAudioFgmArm7AckEvent;

typedef struct NDSAudioFgmArm7AckTrace {
    u32 sequence;
    u32 event_count;
    u32 overflow_count;
    u32 mismatch_count;
    u32 fgm_id;
    u32 generation;
    u32 channel;
    u32 instance_token;
    u32 handle_start_tick;
    u32 handle_end_tick;
    u32 duration_ticks;
    u32 envelope_count;
    NDSAudioFgmArm7AckEvent
        events[NDS_AUDIO_FGM_ARM7_ACK_EVENT_CAPACITY];
} NDSAudioFgmArm7AckTrace;
#endif

void ndsAudioFgmDiagnosticsReset(void);
void ndsAudioFgmLoadFenced(void);
void ndsAudioFgmUpdate(void);
void ndsAudioFgmStopAll(void);
void ndsAudioFgmStop(alSoundEffect *effect);
alSoundEffect *ndsAudioFgmPlay(u16 fgm_id);
alSoundEffect *ndsAudioFgmPlayAtPan(u16 fgm_id, u8 pan);

extern volatile u32 gNdsAudioFgmResult;
extern volatile u32 gNdsAudioFgmMask;
extern volatile u32 gNdsAudioFgmLoaded;
extern volatile u32 gNdsAudioFgmResidentBytes;
extern volatile u32 gNdsAudioFgmSupportedCount;
extern volatile u32 gNdsAudioFgmOpenFailCount;
extern volatile u32 gNdsAudioFgmReadFailCount;
extern volatile u32 gNdsAudioFgmFormatFailCount;
extern volatile u32 gNdsAudioFgmPlayCalls;
extern volatile u32 gNdsAudioFgmSupportedPlayCount;
extern volatile u32 gNdsAudioFgmUnsupportedCallCount;
extern volatile u32 gNdsAudioFgmMissRingCount;
extern volatile u32 gNdsAudioFgmMissRingNext;
extern volatile u16
    gNdsAudioFgmMissRingIDs[NDS_AUDIO_FGM_MISS_RING_CAPACITY];
extern volatile u32
    gNdsAudioFgmMissRingCounts[NDS_AUDIO_FGM_MISS_RING_CAPACITY];
extern volatile u32 gNdsAudioFgmIncludedLookupFailCount;
extern volatile u32 gNdsAudioFgmPlayFailCount;
extern volatile u32 gNdsAudioFgmPhasePlayMask;
extern volatile u32 gNdsAudioFgmPhasePlayCounts[NDS_AUDIO_FGM_PHASE_COUNT];
extern volatile u32 gNdsAudioFgmKoPlayMask;
extern volatile u32 gNdsAudioFgmKoPlayCounts[NDS_AUDIO_FGM_KO_COUNT];
extern volatile u32 gNdsAudioFgmKoTraceCount;
extern volatile u32
    gNdsAudioFgmKoTrace[NDS_AUDIO_FGM_KO_TRACE_CAPACITY];
extern volatile u32 gNdsAudioFgmLoopPlayCount;
extern volatile u32 gNdsAudioFgmStopCalls;
extern volatile u32 gNdsAudioFgmStopAllCalls;
extern volatile u32 gNdsAudioFgmDurationStopCount;
extern volatile u32 gNdsAudioFgmStaleStopCount;
extern volatile u32 gNdsAudioFgmGenerationMismatchCount;
extern volatile u32 gNdsAudioFgmActiveHandles;
extern volatile u32 gNdsAudioFgmMaxActiveHandles;
extern volatile u32 gNdsAudioFgmChannelMask;
extern volatile u32 gNdsAudioFgmLastChannel;
extern volatile u32 gNdsAudioFgmLastID;
extern volatile u32 gNdsAudioFgmLastGeneration;
extern volatile u32 gNdsAudioFgmLastInstanceToken;
extern volatile u32 gNdsAudioFgmInstanceTokenWrapCount;
extern volatile u32 gNdsAudioFgmPoolExhaustCount;
extern volatile u32 gNdsAudioFgmHandleAcquireCount;
extern volatile u32 gNdsAudioFgmHandleReleaseCount;
extern volatile u32 gNdsAudioFgmHandleRecycleCount;
extern volatile u32 gNdsAudioFgmHandleCapacity;
extern volatile u32 gNdsAudioFgmEnvelopeStepCount;
extern volatile u32 gNdsAudioFgmFidelityDebtMask;
#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
extern volatile NDSAudioFgmArm7AckTrace gNdsAudioFgmArm7AckTrace;
#endif

#endif
