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

#endif
