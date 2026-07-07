#ifndef SSB64_NDS_AUDIO_ASSETS_H
#define SSB64_NDS_AUDIO_ASSETS_H

#include <PR/ultratypes.h>

#define NDS_AUDIO_ASSET_PASS 0x41554431u /* AUD1 */
#define NDS_AUDIO_ASSET_EXPECTED_MASK 0xffu

void ndsAudioAssetDiagnosticsReset(void);
void ndsAudioAssetLoadFenced(void);

extern volatile u32 gNdsAudioAssetResult;
extern volatile u32 gNdsAudioAssetMask;
extern volatile u32 gNdsAudioAssetOpenCount;
extern volatile u32 gNdsAudioAssetOpenFailCount;
extern volatile u32 gNdsAudioAssetFormatFailCount;
extern volatile u32 gNdsAudioAssetShortReadCount;
extern volatile u32 gNdsAudioAssetRawBytes;
extern volatile u32 gNdsAudioAssetResidentBytes;
extern volatile u32 gNdsAudioAssetScratchMaxBytes;
extern volatile u32 gNdsAudioAssetSeqCount;
extern volatile u32 gNdsAudioAssetSeqFirstOffset;
extern volatile u32 gNdsAudioAssetSeqFirstLength;
extern volatile u32 gNdsAudioAssetSeqMaxLength;
extern volatile u32 gNdsAudioAssetBank1BankCount;
extern volatile u32 gNdsAudioAssetBank1InstrumentCount;
extern volatile u32 gNdsAudioAssetBank1WaveCount;
extern volatile u32 gNdsAudioAssetBank1SampleRate;
extern volatile u32 gNdsAudioAssetBank2BankCount;
extern volatile u32 gNdsAudioAssetBank2InstrumentCount;
extern volatile u32 gNdsAudioAssetBank2WaveCount;
extern volatile u32 gNdsAudioAssetBank2SampleRate;
extern volatile u32 gNdsAudioAssetFgmUnkCount;
extern volatile u32 gNdsAudioAssetFgmTableCount;
extern volatile u32 gNdsAudioAssetFgmUcodeCount;

#endif
