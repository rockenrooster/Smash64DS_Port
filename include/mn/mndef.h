#ifndef SSB64_NDS_MNDEF_H
#define SSB64_NDS_MNDEF_H

/* Narrow menu definitions required by the imported MNTitle slice. Keep this
 * local shim small; broader menu contracts should be added only when imported
 * source needs them. */

typedef enum MNTitleLayout
{
    nMNTitleLayoutOpening,
    nMNTitleLayoutAnimate,
    nMNTitleLayoutFinal
} MNTitleLayout;

typedef enum MNTitleSpriteKind
{
    nMNTitleSpriteKindDropShadow,
    nMNTitleSpriteKindSmash,
    nMNTitleSpriteKindSuper,
    nMNTitleSpriteKindBros,
    nMNTitleSpriteKindTM,
#if defined(REGION_US)
    nMNTitleSpriteKindFooter,
#else
    nMNTitleSpriteKindFooter = 13,
#endif
    nMNTitleSpriteKindHeader,
    nMNTitleSpriteKindPressStart,
    nMNTitleSpriteKindLogo,
    nMNTitleSpriteKindTM2
} MNTitleSpriteKind;

typedef enum MNVSModeOptions
{
    nMNVSModeOptionStart,
    nMNVSModeOptionRule,
    nMNVSModeOptionTimeStock,
    nMNVSModeOptionOptions,
    nMNVSModeOptionEnumCount
} MNVSModeOptions;

typedef enum MNVSModeRule
{
    nMNVSModeRuleTime,
    nMNVSModeRuleStock,
    nMNVSModeRuleTimeTeam,
    nMNVSModeRuleStockTeam
} MNVSModeRule;

typedef enum MNVSModeInputDirection
{
    nMNVSModeInputDirectionNone,
    nMNVSModeInputDirectionUp,
    nMNVSModeInputDirectionDown,
    nMNVSModeInputDirectionLeft,
    nMNVSModeInputDirectionRight
} MNVSModeInputDirection;

typedef enum MNOptionTabStatus
{
    nMNOptionTabStatusNot,
    nMNOptionTabStatusHighlight,
    nMNOptionTabStatusSelected,
    nMNOptionTabStatusEnumCount
} MNOptionTabStatus;

typedef enum MNPlayersSelectButton
{
    nMNPlayersSelectButtonCU,
    nMNPlayersSelectButtonCR,
    nMNPlayersSelectButtonCD,
    nMNPlayersSelectButtonCL,
    nMNPlayersSelectButtonA
} MNPlayersSelectButton;

typedef enum MNPlayersCursorStatus
{
    nMNPlayersCursorStatusPointer,
    nMNPlayersCursorStatusGrab,
    nMNPlayersCursorStatusHover
} MNPlayersCursorStatus;

typedef enum MNVSResultsKind
{
    nMNVSResultsKindTimeRoyal,
    nMNVSResultsKindStockRoyal,
    nMNVSResultsKindTimeTeam,
    nMNVSResultsKindStockTeam,
    nMNVSResultsKindNoContest,
    nMNVSResultsKindEnumCount
} MNVSResultsKind;

typedef enum MNCharactersMotionKind
{
	nMNCharactersMotionKindSpecialHi,
	nMNCharactersMotionKindSpecialN,
	nMNCharactersMotionKindSpecialLw,
	nMNCharactersMotionKindCommonStart,
	nMNCharactersMotionKindWalkSlow = nMNCharactersMotionKindCommonStart,
	nMNCharactersMotionKindWalkMiddle,
	nMNCharactersMotionKindWalkFast,
	nMNCharactersMotionKindRun,
	nMNCharactersMotionKindJumpF,
	nMNCharactersMotionKindJumpB,
	nMNCharactersMotionKindJumpAerialF,
	nMNCharactersMotionKindJumpAerialB,
	nMNCharactersMotionKindSquat,
	nMNCharactersMotionKindOttotto,
	nMNCharactersMotionKindFuraFura,
	nMNCharactersMotionKindWait,
	nMNCharactersMotionKindDamageE1,
	nMNCharactersMotionKindEscapeF,
	nMNCharactersMotionKindEscapeB,
	nMNCharactersMotionKindAttack1,
	nMNCharactersMotionKindAttackDash,
	nMNCharactersMotionKindAttackS3,
	nMNCharactersMotionKindAttackHi3,
	nMNCharactersMotionKindAttackLw3,
	nMNCharactersMotionKindAttackS4,
	nMNCharactersMotionKindAttackHi4,
	nMNCharactersMotionKindAttackLw4,
	nMNCharactersMotionKindAttackAirStart = 26,
	nMNCharactersMotionKindAttackAirN = nMNCharactersMotionKindAttackAirStart,
	nMNCharactersMotionKindAttackAirF,
	nMNCharactersMotionKindAttackAirB,
	nMNCharactersMotionKindAttackAirHi,
	nMNCharactersMotionKindAttackAirLw,
	nMNCharactersMotionKindAttackAirEnd = nMNCharactersMotionKindAttackAirLw,
	nMNCharactersMotionKindThrowF,
	nMNCharactersMotionKindThrowB,
	nMNCharactersMotionKindWin1,
	nMNCharactersMotionKindWin2,
	nMNCharactersMotionKindWin3,
	nMNCharactersMotionKindWin4,
	nMNCharactersMotionKindLose,
	nMNCharactersMotionKindAppeal,
	nMNCharactersMotionKindEnumCount

} MNCharactersMotionKind;

typedef enum MNSoundTestOptions
{
	nMNSoundTestOptionStart,
	nMNSoundTestOptionMusic = nMNSoundTestOptionStart,
	nMNSoundTestOptionSound,
	nMNSoundTestOptionVoice,
	nMNSoundTestOptionEnd = nMNSoundTestOptionVoice,

	nMNSoundTestOptionEnumCount

} MNSoundTestOptions;

typedef enum MNDataOptions
{
	nMNDataOptionStart,
	nMNDataOptionCharacters = nMNDataOptionStart,
	nMNDataOptionVSRecord,
	nMNDataOptionSoundTest,
	nMNDataOptionEnd = nMNDataOptionSoundTest,

	nMNDataOptionEnumCount

} MNDataOptions;

typedef enum MNOptionOptions
{
	nMNOptionOptionStart,
	nMNOptionOptionSound = nMNOptionOptionStart,
	nMNOptionOptionScreenAdjust,
	nMNOptionOptionBackupClear,
	nMNOptionOptionEnd = nMNOptionOptionBackupClear,

	nMNOptionOptionEnumCount

} MNOptionOptions;

typedef enum MNBackupClearOptions
{
	nMNBackupClearOptionStart,
	nMNBackupClearOptionNewcomers = nMNBackupClearOptionStart,
	nMNBackupClearOption1PHighScore,
	nMNBackupClearOptionBonusStageTime,
	nMNBackupClearOptionVSRecord,
	nMNBackupClearOptionPrize,
	nMNBackupClearOptionAllDataClear,
	nMNBackupClearOptionEnd = nMNBackupClearOptionAllDataClear,

	nMNBackupClearOptionEnumCount

} MNBackupClearOptions;

typedef enum MNVSOptionsOptions
{
	nMNVSOptionsOptionStart,
	nMNVSOptionsOptionHandicap = nMNVSOptionsOptionStart,
	nMNVSOptionsOptionTeamAttack,
	nMNVSOptionsOptionStageSelect,
	nMNVSOptionsOptionDamage,
	nMNVSOptionsOptionItemSwitch,
	nMNVSOptionsOptionEnd = nMNVSOptionsOptionItemSwitch,

	nMNVSOptionsOptionEnumCount

} MNVSOptionsOptions;

typedef enum MNModeSelectOptions
{
	nMNModeSelectOptionStart,
	nMNModeSelectOption1PMode = nMNModeSelectOptionStart,
	nMNModeSelectOptionVSMode,
	nMNModeSelectOptionOption,
	nMNModeSelectOptionData,
	nMNModeSelectOptionEnd = nMNModeSelectOptionData,

	nMNModeSelectOptionEnumCount

} MNModeSelectOptions;

typedef enum MN1PModeOptions
{
	nMN1PModeOptionStart,
	nMN1PModeOption1PGame = nMN1PModeOptionStart,
	nMN1PModeOptionTrainingMode,
	nMN1PModeOptionBonus1Practice,
	nMN1PModeOptionBonus2Practice,
	nMN1PModeOptionEnd = nMN1PModeOptionBonus2Practice,
	
	nMN1PModeOptionEnumCount

} MN1PModeOptions;

typedef enum MNOptionTabOnOff
{
	nMNOptionTabStatusOff,
	nMNOptionTabStatusOn

} MNOptionTabOnOff;

typedef enum MNVSRecordKind
{
	nMNVSRecordKindStart,	
	nMNVSRecordKindBattleScore = nMNVSRecordKindStart,	// BattleScore
	nMNVSRecordKindRanking,         					// Ranking
	nMNVSRecordKindIndiv,       						// Individual
	nMNVSRecordKindEnd = nMNVSRecordKindIndiv,
	nMNVSRecordKindEnumCount

} MNVSRecordKind;

typedef enum MNVSRecordRankingKind
{
	nMNVSRecordRankingKindStart,
	nMNVSRecordRankingKindWinPercent = nMNVSRecordRankingKindStart,	// Win %
	nMNVSRecordRankingKindKOs,              						// KOs
	nMNVSRecordRankingKindTKO,             							// TKO
	nMNVSRecordRankingKindSDPercent,     							// SD %
	nMNVSRecordRankingKindTime,             						// Time
	nMNVSRecordRankingKindUsePercent,   							// Use %
	nMNVSRecordRankingKindAvg,           							// Avg
	nMNVSRecordRankingKindEnd = nMNVSRecordRankingKindAvg,
	nMNVSRecordRankingKindEnumCount

} MNVSRecordRankingKind;

#define mnCommonCheckGetOptionButtonInput(wait, is_button, mask) \
    (((wait) == 0) && (((is_button) = scSubsysControllerGetPlayerHoldButtons(mask)), (is_button) != FALSE))
#define mnCommonCheckGetOptionStickInputUD(wait, stick_range, min, b) \
    (((wait) == 0) && (((stick_range) = scSubsysControllerGetPlayerStickUD(min, b)), (stick_range) != 0))
#define mnCommonCheckGetOptionStickInputLR(wait, stick_range, min, b) \
    (((wait) == 0) && (((stick_range) = scSubsysControllerGetPlayerStickLR(min, b)), (stick_range) != 0))
#define mnCommonGetOptionChangeWaitP(stick_range, div) \
    ((160 - (stick_range)) / (div))
#define mnCommonGetOptionChangeWaitN(stick_range, div) \
    (((stick_range) + 160) / (div))
#define mnCommonSetOptionChangeWaitP(wait, is_button, stick_range, div) \
    ((wait) = (((is_button) != FALSE) ? 12 : mnCommonGetOptionChangeWaitP(stick_range, div)))
#define mnCommonSetOptionChangeWaitN(wait, is_button, stick_range, div) \
    ((wait) = (((is_button) != FALSE) ? 12 : mnCommonGetOptionChangeWaitN(stick_range, div)))

#endif
