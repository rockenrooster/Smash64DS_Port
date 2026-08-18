#ifndef SSB64_NDS_SYS_AUDIO_H
#define SSB64_NDS_SYS_AUDIO_H

#include <PR/ultratypes.h>
#include <ssb_types.h>

#define AL_FX_CUSTOM 6
#define AL_STOPPED 0
#define AL_PLAYING 1

typedef struct SYAudioCSPlayerCompat {
    s32 state;
} SYAudioCSPlayerCompat;

enum {
    nSYAudioBGMExplain = 0
};

enum {
    /* Was stubbed = 0 (colliding with nSYAudioFGMExplodeS) until P2-1f-1.
     * 157 is the REGION_US gm/gmsound.h value (mntitle.c:501's own confirm
     * cue), independently re-verified the same way this file's audio ids
     * always are; nds_menu_shell.c's NDS_CSS_FGM_PRESS_START and
     * nds_ui_kit.c's SFX table already used the correct literal 157u
     * directly, so this stub's wrong value was inert everywhere until
     * ndsAudioFgmIDIsIncluded's switch became the first live reader of the
     * symbol itself (P2-1f-1, closing the case P2-1e-1 recorded as missing).
     * nSYAudioFGMOpeningBatM/PublicPrologue below carry the same
     * placeholder-0/1/2 pattern and are ALSO wrong (real values 152 and 150),
     * but neither is referenced anywhere else in this tree -- out of this
     * row's scope, left for whoever next needs them. */
    nSYAudioFGMTitlePressStart = 157,
    nSYAudioFGMOpeningBatM = 1,
    nSYAudioFGMPublicPrologue = 2
};

typedef struct SYAudioPublicSettings {
    u8 unk31;
} SYAudioPublicSettings;

#ifndef SSB64_NDS_ALSOUNDEFFECT_TYPEDEF
#define SSB64_NDS_ALSOUNDEFFECT_TYPEDEF
typedef struct alSoundEffect alSoundEffect;
#endif

#ifndef SSB64_NDS_ALSOUNDEFFECT_STRUCT
#define SSB64_NDS_ALSOUNDEFFECT_STRUCT
struct alSoundEffect {
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
};
#endif

extern SYAudioPublicSettings dSYAudioPublicSettings;
extern SYAudioCSPlayerCompat *gSYAudioCSPlayers[1];

void syAudioThreadMain(void *arg);
void syAudioStopBGMAll(void);
void syAudioPlayBGM(s32 player, s32 bgm_id);
s32 syAudioCheckBGMPlaying(s32 sngplayer);
void syAudioSetBGMVolume(s32 sngplayer, u32 vol);
void syAudioUpdateBGMState(void);
void func_800266A0_272A0(void);
void func_80026738_27338(alSoundEffect *sfx);
void *func_800269C0_275C0(u16 fgm_id);
void syAudioSetSettingsUpdated(void);
sb32 syAudioGetSettingsUpdated(void);
void syAudioSetFXType(u8 type);
sb32 syAudioGetRestarting(void);

#endif
