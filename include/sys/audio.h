#ifndef SSB64_NDS_SYS_AUDIO_H
#define SSB64_NDS_SYS_AUDIO_H

#include <PR/ultratypes.h>
#include <ssb_types.h>

#define AL_FX_CUSTOM 6

enum {
    nSYAudioBGMExplain = 0
};

enum {
    nSYAudioFGMTitlePressStart = 0,
    nSYAudioFGMOpeningBatM = 1,
    nSYAudioFGMPublicPrologue = 2
};

typedef struct SYAudioPublicSettings {
    u8 unk31;
} SYAudioPublicSettings;

extern SYAudioPublicSettings dSYAudioPublicSettings;

void syAudioThreadMain(void *arg);
void syAudioStopBGMAll(void);
void syAudioPlayBGM(s32 player, s32 bgm_id);
void func_800266A0_272A0(void);
void func_800269C0_275C0(s32 fgm_id);
void syAudioSetSettingsUpdated(void);
sb32 syAudioGetSettingsUpdated(void);
void syAudioSetFXType(u8 type);
sb32 syAudioGetRestarting(void);

#endif
