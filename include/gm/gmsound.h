#ifndef SSB64_NDS_GM_GMSOUND_H
#define SSB64_NDS_GM_GMSOUND_H

typedef enum gmMusicID
{
    nSYAudioBGMModeSelect = 43,
    nSYAudioBGMBattleSelect,
    nSYAudioBGMStar = 45
} gmMusicID;

typedef enum gmFGMID
{
    nSYAudioFGMGuardOn = 13,
    nSYAudioFGMGuardOff = 14,
    nSYAudioFGMShieldBreak = 15,
    nSYAudioFGMShockL = 22,
    nSYAudioFGMShockM = 23,
    nSYAudioFGMShockS = 24,
    nSYAudioFGMShockML = 29,
    nSYAudioFGMBurnL = 25,
    nSYAudioFGMBurnM = 27,
    nSYAudioFGMBurnS = 28,
    nSYAudioFGMKickL = 31,
    nSYAudioFGMKickM = 32,
    nSYAudioFGMKickS = 34,
    nSYAudioFGMPunchL = 37,
    nSYAudioFGMPunchM = 38,
    nSYAudioFGMPunchS = 40,
    nSYAudioFGMHarisenHit = 51,
    nSYAudioFGMBatHit = 52,
    nSYAudioFGMStarGet = 54,
    nSYAudioFGMMarioDash = 121,
    nSYAudioFGMSamusDash = 127,
    nSYAudioFGMMenuSelect = 168,
    nSYAudioFGMStageSelect = 169,
    nSYAudioFGMMenuScroll1 = 173,
    nSYAudioFGMMenuScroll2 = 174,
    nSYAudioFGMMenuDenied = 175,
    nSYAudioFGMAltitudeWarn = 153,
    nSYAudioFGMPlayerSlotClose = 176,
    nSYAudioFGMPlayerSlotWhoosh = 177,
    nSYAudioFGMKirbySpecialNLoseCopy = 204,
    nSYAudioFGMMarioSpecialHiCoin = 216,
    nSYAudioFGMSlashL = 261,
    nSYAudioFGMSlashM = 262,
    nSYAudioFGMSlashS = 263,
    nSYAudioFGMPlayerHeal = 274,
    nSYAudioFGMJungleTaruCannShoot = 281,
    nSYAudioFGMJungleTaruCannEnter = 282,
    nSYAudioFGMHyruleTwisterAppear = 283,
    nSYAudioFGMHyruleTwisterTrapped = 284,
    nSYAudioFGMPupupuWhispyWind = 0x11d,
    nSYAudioFGMFloorDamageFire = 286
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
