#ifndef SSB64_NDS_AUDIO_FGM_H
#define SSB64_NDS_AUDIO_FGM_H

#include <PR/ultratypes.h>
#include <sys/audio.h>

#define NDS_AUDIO_FGM_PASS 0x46474d31u /* FGM1 */
#define NDS_AUDIO_FGM_PHASE_COUNT 5u
#define NDS_AUDIO_FGM_PHASE_COMPLETE_MASK 0x1fu
#define NDS_AUDIO_FGM_PACK_BYTES 39120u
#define NDS_AUDIO_FGM_PACK_MAPPING_SHA256_LO 0xca162f4eu
#define NDS_AUDIO_FGM_NONREUSE_HANDLE_CAPACITY 8u
#define NDS_AUDIO_FGM_FIDELITY_DEBT_LOOP_PREROLL (1u << 0)
#define NDS_AUDIO_FGM_FIDELITY_DEBT_ENVELOPE_QUANTIZATION (1u << 1)
#define NDS_AUDIO_FGM_EXPECTED_FIDELITY_DEBT_MASK \
    (NDS_AUDIO_FGM_FIDELITY_DEBT_LOOP_PREROLL | \
     NDS_AUDIO_FGM_FIDELITY_DEBT_ENVELOPE_QUANTIZATION)

void ndsAudioFgmDiagnosticsReset(void);
void ndsAudioFgmLoadFenced(void);
void ndsAudioFgmUpdate(void);
void ndsAudioFgmStopAll(void);
void ndsAudioFgmStop(alSoundEffect *effect);
alSoundEffect *ndsAudioFgmPlay(u16 fgm_id);

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
extern volatile u32 gNdsAudioFgmIncludedLookupFailCount;
extern volatile u32 gNdsAudioFgmPlayFailCount;
extern volatile u32 gNdsAudioFgmPhasePlayMask;
extern volatile u32 gNdsAudioFgmPhasePlayCounts[NDS_AUDIO_FGM_PHASE_COUNT];
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
extern volatile u32 gNdsAudioFgmPoolExhaustCount;
extern volatile u32 gNdsAudioFgmAllocatedHandles;
extern volatile u32 gNdsAudioFgmNonReuseCapacity;
extern volatile u32 gNdsAudioFgmEnvelopeStepCount;
extern volatile u32 gNdsAudioFgmFidelityDebtMask;

#endif
