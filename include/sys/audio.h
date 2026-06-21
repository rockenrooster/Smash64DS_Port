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

typedef struct alSoundEffect {
    void *unk_0x0;
    void *unk_0x4;
    void *unk_0x8;
    void *unk_0xC;
    u16 unk_0x10;
    u16 unk_0x12;
    u16 unk_0x14;
    u16 unk_0x16;
    u16 unk_0x18;
    u16 unk_0x1A;
    u16 unk_0x1C;
    u8 unk_0x1E;
    u8 unk_0x1F;
    u16 unk_0x20;
    u16 unk_0x22;
    u16 unk_0x24;
    u16 sfx_id;
    u16 sfx_max;
    u8 filler_0x2A[0x2F - 0x2A];
    u8 balance;
} alSoundEffect;

extern SYAudioPublicSettings dSYAudioPublicSettings;

void syAudioThreadMain(void *arg);
void syAudioStopBGMAll(void);
void syAudioPlayBGM(s32 player, s32 bgm_id);
void func_800266A0_272A0(void);
void func_80026738_27338(alSoundEffect *sfx);
void *func_800269C0_275C0(u16 fgm_id);
void syAudioSetSettingsUpdated(void);
sb32 syAudioGetSettingsUpdated(void);
void syAudioSetFXType(u8 type);
sb32 syAudioGetRestarting(void);

#endif
