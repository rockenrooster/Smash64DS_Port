/* Calico/ndstool overlay 0. Keep persistent menu state in the resident image;
 * code/read-only data and scene relocation workspaces share the VS loan.
 * This script is passed BEFORE nds_hot_text.ld, as in Calico's overlay example,
 * so these input sections are claimed before the resident .main wildcard. */
PHDRS {
    ovltable PT_LOAD FLAGS(0x200007);
    ovl_frontend PT_LOAD FLAGS(0x200007);
}

/* These guards are called from shared frame/input/sprite code even in battle.
 * Their mutable state already stays resident. Keep the complete UI commit
 * dependency set here so that no unloaded-code call is hidden behind a branch. */
SECTIONS {
    .text.frontend_resident : {
        :nds_menu_shell.o (.text.ndsMenuShellOnePlayerCssOwnsSource2D
            .text.ndsMenuShellWalkWantsResultsStart)
        :nds_ui_kit.o (.text.ndsUiKitCommit .text.ndsUiKitOam
            .text.ndsUiKitObjBase .text.ndsUiKitObjAt .text.ndsUiKitSpriteSize
            .rodata.kNdsUiKitImageMetrics .rodata.ndsUiKitCommit.*)
        . = ALIGN(4);
    } >main AT>lma9 :main = 0xff
}
INSERT BEFORE .main;

SECTIONS {
    .ovl.frontend ALIGN(32) : {
        __nds_frontend_start = .;
        /* 1P BATTLE-TIME HEAD (2026-10-01). Everything a 1P, bonus or
         * training battle can execute or read from this overlay sits
         * first; a battle of those kinds keeps it and borrows the tail
         * (__nds_frontend_1p_end..__nds_frontend_end, menu and scene code
         * plus the scene status buffers) as asset storage, as a VS battle
         * borrows the whole range. ndsFrontendOverlayRestoreTail reloads
         * the overlay before any tail code runs again. */
        :battleship_sc1pmanager.o (.text .text.* .rodata .rodata.*)
        :battleship_sc1pgame_runtime.o (.text .text.* .rodata .rodata.*)
        :battleship_sc1ptrainingmode.o (.text .text.* .rodata .rodata.*)
        :battleship_sc1pbonusstage.o (.text .text.* .rodata .rodata.*)
        :battleship_sc1pbonusstagefiles.o (.text .text.* .rodata .rodata.*)
        :battleship_sc1pgameboss.o (.text .text.* .rodata .rodata.*)
        :battleship_ftboss.o (.text .text.* .rodata .rodata.*)
        :battleship_ftboss_status_1.o (.text .text.* .rodata .rodata.*)
        :battleship_ftboss_status_2.o (.text .text.* .rodata .rodata.*)
        :battleship_ftboss_status_3.o (.text .text.* .rodata .rodata.*)
        :battleship_ftboss_status_4.o (.text .text.* .rodata .rodata.*)
        :battleship_wpbossbullet.o (.text .text.* .rodata .rodata.*)
        :battleship_grbonus3.o (.text .text.* .rodata .rodata.*)
        :battleship_item_tarubomb.o (.text .text.* .rodata .rodata.*)
        :battleship_item_target.o (.text .text.* .rodata .rodata.*)
        :battleship_ftboss.o (.data.dFTBossSpecialStatusDescs
            .data.dFTBossWaitRandomArrayLookup .data.dFTBossWaitRandomNoGround
            .data.dFTBossWaitRandomGround .data.dFTBossWaitRandomStatusLookup
            .data.dFTBossWaitRandomStatusIDs)
        :battleship_wpbossbullet.o (.data.dWPBossBulletNormalWeaponDesc
            .data.dWPBossBulletHardWeaponDesc)
        :battleship_item_target.o (.data.dITTargetItemDesc)
        :battleship_item_tarubomb.o (.data.dITTaruBombItemDesc
            .data.dITTaruBombStatusDescs)
        :battleship_sc1pgameboss.o (.data.dSC1PGameBossWallpapers
            .data.dSC1PGameBossPlans0 .data.dSC1PGameBossPlans1
            .data.dSC1PGameBossPlans2 .data.dSC1PGameBossPlans3
            .data.dSC1PGameBossAnims0 .data.dSC1PGameBossAnims1
            .data.dSC1PGameBossAnims2 .data.dSC1PGameBossAnims3
            .data.dSC1PGameBossEffects0 .data.dSC1PGameBossEffects1
            .data.dSC1PGameBossEffects2 .data.dSC1PGameBossEffects3
            .data.dSC1PGameBossCometEnvColorR .data.dSC1PGameBossCometEnvColorG
            .data.dSC1PGameBossCometEnvColorB)
        :battleship_sc1pbonusstage.o (.data.dSC1PBonusStageTargetDescs
            .data.dSC1PBonusStageBumperDescs .data.dSC1PBonusStagePlatformDescs
            .data.dSC1PBonusStageBoardedPlatformDescs
            .data.dSC1PBonusStageTimerUnitLengths .data.dSC1PBonusStageTimerDigitPositions)
        :battleship_sc1pgame_runtime.o (.data.dSC1PGameStageDesc
            .data.dSC1PGameComputerDesc .data.dSC1PGameZoomEyeX .data.dSC1PGameZoomEyeY
            .data.dSC1PGameKirbyTeamCopyKinds .data.dSC1PGameStageCommonSleepTics)
        :battleship_sc1pmanager.o (.data.dSC1PManagerSubsysOverlay
            .data.dSC1PManagerObjectsOverlay .data.dSC1PManager1PIntroOverlay
            .data.dSC1PManager1PBonusStageOverlay .data.dSC1PManager1PGameOverlay
            .data.dSC1PManager1PContinueOverlay .data.dSC1PManager1PStageClearOverlay
            .data.dSC1PManager1PChallengerOverlay .data.dSC1PManagerEndingOverlay
            .data.dSC1PManagerStaffrollOverlay .data.dSC1PManagerCongraOverlay
            .data.dSC1PManagerMessageOverlay)
        . = ALIGN(32);
        __nds_frontend_1p_end = .;
        :battleship_mntitlefiles.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mntitle.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mnvsmode.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mnplayersvs.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mnmaps.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mnoption.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mnbackupclear.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mnsoundtest.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mnscreenadjust.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mnmessage.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mndata.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mnvsrecord.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mncharacters.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mn1pmode.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mn1pcontinue.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mnplayers1pgame.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mnplayers1pbonus.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mncongra.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mnvsresults.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mvopeningroom.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mvopeningportraits.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mvopeningmario.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mvopeningdonkey.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mvopeninglink.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mvopeningsamus.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mvopeningyoshi.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mvopeningkirby.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mvopeningfox.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mvopeningpikachu.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mvending.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :nds_ui_kit.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :nds_menu_shell.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        /* Non-VS scenes retain this overlay for their complete nested call.
         * The 1P runtime's boss/death/camera closure moved here too: its shared
         * callers test the 1P game rule/type or are themselves boss code. */
        :battleship_sc1pintro.o (.text .text.* .rodata .rodata.*)
        :battleship_sc1pstageclear.o (.text .text.* .rodata .rodata.*)
        :battleship_sc1pchallenger.o (.text .text.* .rodata .rodata.*)
        :battleship_scstaffroll.o (.text .text.* .rodata .rodata.*)
        :battleship_scautodemo.o (.text .text.* .rodata .rodata.*)
        :battleship_scautodemofiles.o (.text .text.* .rodata .rodata.*)
        :battleship_scexplain.o (.text .text.* .rodata .rodata.*)
        :battleship_scexplainfiles.o (.text .text.* .rodata .rodata.*)
        /* 1P-only TUs (2026-09-28): Training CSS, bonus stages, Master Hand
         * (fighter, bullet, Final Destination wallpaper), Race to the Finish
         * ground and its two ground items. Every resident entry is selected by
         * a non-VS scene, a 1P game rule/type, fighter kind Boss, item kind
         * Target/TaruBomb or ground kind Bonus1/2/3/Last; none is reachable in
         * VS (VS CSS/SSS cannot select them). Each pair is in the allowlist.
         * The VS loan grows with this range. See the 2026-09-28 1P overlay
         * receipt under artifacts/performance/. */
        :battleship_mntraining.o (.text .text.* .rodata .rodata.*)
        /* Read-only descriptor/lookup tables of 1P-only code, declared without
         * const by BattleShip (readers audited in the 2026-09-28 receipt; none
         * writes). Taskman/video setups, ll* offsets and tables whose address
         * is stored in resident interface state stay resident. */
        :battleship_mntraining.o (.data.dMNPlayers1PTrainingFileIDs)
        :battleship_sc1pstageclear.o (.data.dSC1PStageClearBonusData)
        :battleship_mncongra.o (.data.dMNCongraPictures)
        :battleship_scstaffroll.o (.data.dSCStaffrollTextBoxSpriteInfo
            .data.dSCStaffrollNameAndJobSpriteInfo .data.dSCStaffrollTextBoxDisplayList)
        /* Read-only source lookup tables declared without const by BattleShip.
         * Their scene code copies/reads them; persistent selections stay in BSS.
         * Keep StaffRoleCharacters resident: TryHideUnlocks mutates that array. */
        :battleship_mncharacters.o (
            .data.dMNCharactersCommonMotionDescs .data.dMNCharactersAttack1MotionDescs
            .data.dMNCharactersKirbyJumpAerialMotionDescs .data.dMNCharactersKirbyFallMotionDesc
            .data.dMNCharactersPurinJumpAerialMotionDescs .data.dMNCharactersPurinFallMotionDesc
            .data.dMNCharactersSpecialMotionMario .data.dMNCharactersSpecialMotionFox
            .data.dMNCharactersSpecialMotionDonkey .data.dMNCharactersSpecialMotionSamus
            .data.dMNCharactersSpecialMotionLuigi .data.dMNCharactersSpecialMotionLink
            .data.dMNCharactersSpecialMotionYoshi .data.dMNCharactersSpecialMotionCaptain
            .data.dMNCharactersSpecialMotionKirby .data.dMNCharactersSpecialMotionPikachu
            .data.dMNCharactersSpecialMotionPurin .data.dMNCharactersSpecialMotionNess)
        :battleship_mnsoundtest.o (.data.dMNSoundTestMusicIDs
            .data.dMNSoundTestSoundIDs .data.dMNSoundTestVoiceIDs)
        :battleship_scstaffroll.o (.data.dSCStaffrollNameCharacters
            .data.dSCStaffrollNameTextInfo .data.dSCStaffrollStaffRoleTextInfo
            .data.dSCStaffrollJobDescriptions .data.dSCStaffrollJobCharacters
            .data.dSCStaffrollJobTextInfo .data.dSCStaffrollCompanyCharacters
            .data.dSCStaffrollCompanyTextInfo .data.dSCStaffrollCompanyIDs)
        . = ALIGN(32);
    } AT>ovlmem :ovl_frontend = 0xff

    /* Each LBFileNode workspace is installed with count zero by its own
     * scene's lbRelocInitSetup. None is menu preference or campaign state.
     * Calico clears this BSS whenever frontend code is reloaded after VS. */
    .ovl.frontend.bss . (NOLOAD) : {
        :battleship_mvopeningroom.o (.bss.sMVOpeningRoomForceStatusBuffer)
        :battleship_mvopeningroom.o (.bss.sMVOpeningRoomStatusBuffer)
        :battleship_mvopeningportraits.o (.bss.sMVOpeningPortraitsForceStatusBuffer)
        :battleship_mvopeningportraits.o (.bss.sMVOpeningPortraitsStatusBuffer)
        :battleship_mvopeningmario.o (.bss.sMVOpeningMarioForceStatusBuffer)
        :battleship_mvopeningmario.o (.bss.sMVOpeningMarioStatusBuffer)
        :battleship_mvopeningdonkey.o (.bss.sMVOpeningDonkeyForceStatusBuffer)
        :battleship_mvopeningdonkey.o (.bss.sMVOpeningDonkeyStatusBuffer)
        :battleship_mvopeninglink.o (.bss.sMVOpeningLinkForceStatusBuffer)
        :battleship_mvopeninglink.o (.bss.sMVOpeningLinkStatusBuffer)
        :battleship_mvopeningsamus.o (.bss.sMVOpeningSamusForceStatusBuffer)
        :battleship_mvopeningsamus.o (.bss.sMVOpeningSamusStatusBuffer)
        :battleship_mvopeningyoshi.o (.bss.sMVOpeningYoshiForceStatusBuffer)
        :battleship_mvopeningyoshi.o (.bss.sMVOpeningYoshiStatusBuffer)
        :battleship_mvopeningkirby.o (.bss.sMVOpeningKirbyForceStatusBuffer)
        :battleship_mvopeningkirby.o (.bss.sMVOpeningKirbyStatusBuffer)
        :battleship_mvopeningfox.o (.bss.sMVOpeningFoxForceStatusBuffer)
        :battleship_mvopeningfox.o (.bss.sMVOpeningFoxStatusBuffer)
        :battleship_mvopeningpikachu.o (.bss.sMVOpeningPikachuForceStatusBuffer)
        :battleship_mvopeningpikachu.o (.bss.sMVOpeningPikachuStatusBuffer)
        :battleship_mntitlefiles.o (.bss.sMNTitleStatusBuffer)
        :battleship_mnvsmode.o (.bss.sMNVSModeStatusBuffer)
        :battleship_mnplayersvs.o (.bss.sMNPlayersVSStatusBuffer)
        :battleship_mnplayersvs.o (.bss.sMNPlayersVSForceStatusBuffer)
        :battleship_mnmaps.o (.bss.sMNMapsForceStatusBuffer)
        :battleship_mnmaps.o (.bss.sMNMapsStatusBuffer)
        :battleship_mnscreenadjust.o (.bss.sMNScreenAdjustStatusBuffer)
        :battleship_mnscreenadjust.o (.bss.sMNScreenAdjustForceStatusBuffer)
        :battleship_sc1pstageclear.o (.bss.sSC1PStageClearForceStatusBuffer)
        :battleship_sc1pstageclear.o (.bss.sSC1PStageClearStatusBuffer)
        :battleship_sc1pintro.o (.bss.sSC1PIntroStatusBuffer)
        :battleship_sc1pintro.o (.bss.sSC1PIntroForceStatusBuffer)
        :battleship_sc1pchallenger.o (.bss.sSC1PChallengerStatusBuffer)
        :battleship_sc1pchallenger.o (.bss.sSC1PChallengerForceStatusBuffer)
        :battleship_mnmessage.o (.bss.sMNMessageStatusBuffer)
        :battleship_mncharacters.o (.bss.sMNCharactersForceStatusBuffer)
        :battleship_mncharacters.o (.bss.sMNCharactersStatusBuffer)
        :battleship_mn1pmode.o (.bss.sMN1PModeStatusBuffer)
        :battleship_mn1pcontinue.o (.bss.sMN1PContinueForceStatusBuffer)
        :battleship_mn1pcontinue.o (.bss.sMN1PContinueStatusBuffer)
        :battleship_mnplayers1pgame.o (.bss.sMNPlayers1PGameStatusBuffer)
        :battleship_mnplayers1pgame.o (.bss.sMNPlayers1PGameForceStatusBuffer)
        :battleship_mnplayers1pbonus.o (.bss.sMNPlayers1PBonusStatusBuffer)
        :battleship_mnplayers1pbonus.o (.bss.sMNPlayers1PBonusForceStatusBuffer)
        :battleship_mncongra.o (.bss.sMNCongraStatusBuffer)
        :battleship_mvending.o (.bss.sMVEndingForceStatusBuffer)
        :battleship_mvending.o (.bss.sMVEndingStatusBuffer)
        :battleship_scstaffroll.o (.bss.sSCStaffrollStatusBuffer)
        :battleship_scautodemofiles.o (.bss.sSCAutoDemoForceStatusBuffer)
        :battleship_scautodemofiles.o (.bss.sSCAutoDemoStatusBuffer)
        :battleship_scexplainfiles.o (.bss.sSCExplainForceStatusBuffer)
        :battleship_scexplainfiles.o (.bss.sSCExplainStatusBuffer)
        :battleship_mnvsresults.o (.bss.sMNVSResultsForceStatusBuffer)
        :battleship_mnvsresults.o (.bss.sMNVSResultsStatusBuffer)
        :battleship_mntraining.o (.bss.sMNPlayers1PTrainingStatusBuffer)
        :battleship_mntraining.o (.bss.sMNPlayers1PTrainingForceStatusBuffer)
        . = ALIGN(32);
        __nds_frontend_end = .;
    } AT>ovlmem :ovl_frontend
}
INSERT AFTER .main.bss;

SECTIONS {
    .ovltable 0 : {
        /* No constructors or destructors. Persistent menu state stays resident. */
        LONG(0)
        LONG(0)
        LONG(0)
    } AT>ovlmem :ovltable
}
