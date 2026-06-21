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

#define mnCommonCheckGetOptionButtonInput(wait, is_button, mask) \
    (((wait) == 0) && ((is_button) & (mask)))
#define mnCommonCheckGetOptionStickInputUD(wait, stick_range, min, b) \
    (((wait) == 0) && (((b) != 0) ? ((stick_range) >= (min)) : ((stick_range) <= (min))))
#define mnCommonCheckGetOptionStickInputLR(wait, stick_range, min, b) \
    mnCommonCheckGetOptionStickInputUD((wait), (stick_range), (min), (b))
#define mnCommonSetOptionChangeWaitP(wait, is_button, stick_range, div) \
    ((wait) = ((is_button) ? 10 : ((stick_range) / (div))))
#define mnCommonSetOptionChangeWaitN(wait, is_button, stick_range, div) \
    ((wait) = ((is_button) ? 10 : -((stick_range) / (div))))

#endif
