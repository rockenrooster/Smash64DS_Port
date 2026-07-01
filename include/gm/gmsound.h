#ifndef SSB64_NDS_GM_GMSOUND_H
#define SSB64_NDS_GM_GMSOUND_H

typedef enum gmMusicID
{
    nSYAudioBGMModeSelect = 43,
    nSYAudioBGMBattleSelect
} gmMusicID;

typedef enum gmFGMID
{
    nSYAudioFGMGuardOn = 13,
    nSYAudioFGMGuardOff = 14,
    nSYAudioFGMShieldBreak = 15,
    nSYAudioFGMBatHit = 52,
    nSYAudioFGMMarioDash = 121,
    nSYAudioFGMSamusDash = 127,
    nSYAudioFGMMenuSelect = 168,
    nSYAudioFGMStageSelect = 169,
    nSYAudioFGMMenuScroll1 = 173,
    nSYAudioFGMMenuScroll2 = 174,
    nSYAudioFGMMenuDenied = 175,
    nSYAudioFGMPlayerSlotClose = 176,
    nSYAudioFGMPlayerSlotWhoosh = 177,
    nSYAudioFGMKirbySpecialNLoseCopy = 204,
    nSYAudioFGMJungleTaruCannShoot = 281,
    nSYAudioFGMJungleTaruCannEnter = 282,
    nSYAudioFGMHyruleTwisterAppear = 283,
    nSYAudioFGMHyruleTwisterTrapped = 284,
    nSYAudioFGMPupupuWhispyWind = 0x11d
} gmFGMID;

typedef enum gmVoiceID
{
    nSYAudioVoiceAnnounceBlueTeam = 566,
    nSYAudioVoiceAnnounceDonkey = 574,
    nSYAudioVoiceAnnounceCaptain = 576,
    nSYAudioVoiceAnnounceFox = 577,
    nSYAudioVoiceAnnounceGreenTeam = 582,
    nSYAudioVoiceAnnounceKirby = 587,
    nSYAudioVoiceAnnounceLink = 588,
    nSYAudioVoiceAnnounceLuigi = 589,
    nSYAudioVoiceAnnounceMario = 590,
    nSYAudioVoiceAnnounceNess = 592,
    nSYAudioVoiceAnnouncePikachu = 598,
    nSYAudioVoiceAnnouncePurin = 599,
    nSYAudioVoiceAnnounceRedTeam = 601,
    nSYAudioVoiceAnnounceFreeForAll = 521,
    nSYAudioVoiceAnnounceSamus = 604,
    nSYAudioVoiceAnnounceTeamBattle = 535,
    nSYAudioVoiceAnnounceYoshi = 626,
    nSYAudioVoicePublicCheer = 618,
    nSYAudioVoicePublicExcited = 626,
    nSYAudioFGMVoiceEnd = 0x2B7
} gmVoiceID;

#endif
