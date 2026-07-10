#ifndef SSB64_NDS_FTCOMPUTER_H
#define SSB64_NDS_FTCOMPUTER_H

#include <ft/fighter.h>
#include <mp/map.h>

#define ftGetComTargetFighter(com) ((FTStruct *)(com)->target_user)
#define ftGetComTargetWeapon(com) ((WPStruct *)(com)->target_user)
#define ftGetComTargetItem(com) ((ITStruct *)(com)->target_user)

#define DISTANCE(a, b) (((a) < (b)) ? -((a) - (b)) : ((a) - (b)))

#define FTCOMPUTER_LEVEL_MAX 9

#define FTDONKEY_GIANTPUNCH_CHARGE_MAX 10
#define FTSAMUS_CHARGE_MAX 7
#define FTKIRBY_COPYDONKEY_GIANTPUNCH_CHARGE_MAX 10
#define FTKIRBY_COPYSAMUS_CHARGE_MAX 7

/* Character status IDs compared by the all-fighter source AI. */
#define nFTDonkeyStatusSpecialNStart 222
#define nFTDonkeyStatusSpecialAirNStart 223
#define nFTDonkeyStatusSpecialNLoop 224
#define nFTDonkeyStatusSpecialAirNLoop 225
#define nFTDonkeyStatusSpecialLwStart 232
#define nFTDonkeyStatusSpecialLwLoop 233
#define nFTDonkeyStatusSpecialLwEnd 234
#define nFTDonkeyStatusThrowFWait 235
#define nFTDonkeyStatusThrowFWalkSlow 236
#define nFTDonkeyStatusThrowFWalkMiddle 237
#define nFTDonkeyStatusThrowFWalkFast 238
#define nFTDonkeyStatusThrowFTurn 239

#define nFTSamusStatusSpecialNStart 222
#define nFTSamusStatusSpecialNLoop 223
#define nFTSamusStatusSpecialAirNStart 225
#define nFTSamusStatusSpecialAirNEnd 226

#define nFTKirbyStatusCopySamusSpecialNStart 237
#define nFTKirbyStatusCopySamusSpecialNLoop 238
#define nFTKirbyStatusCopySamusSpecialAirNStart 240
#define nFTKirbyStatusCopySamusSpecialAirNEnd 241
#define nFTKirbyStatusCopyDonkeySpecialNStart 242
#define nFTKirbyStatusCopyDonkeySpecialAirNStart 243
#define nFTKirbyStatusCopyDonkeySpecialNLoop 244
#define nFTKirbyStatusCopyDonkeySpecialAirNLoop 245
#define nFTKirbyStatusSpecialNCatch 272
#define nFTKirbyStatusSpecialNEat 273
#define nFTKirbyStatusSpecialNWait 275
#define nFTKirbyStatusSpecialAirNCatch 281
#define nFTKirbyStatusSpecialAirNEat 282
#define nFTKirbyStatusSpecialAirNWait 284

#define nFTNessStatusSpecialHiHold 229
#define nFTNessStatusSpecialAirHiHold 233
#define nFTNessStatusSpecialLwScopeStart 237
#define nFTNessStatusSpecialLwScopeEnd 244

#define nFTPikachuStatusSpecialAirHiStart 235
#define nFTPikachuStatusSpecialAirHi 236

#define FTCOMPUTER_COMMAND_TIMER_BITS 4
#define FTCOMPUTER_COMMAND_TIMER_MASK 0x0F
#define FTCOMPUTER_COMMAND_OPCODE_MASK 0xF0
#define FTCOMPUTER_COMMAND_PKTHUNDER 0xF3
#define FTCOMPUTER_COMMAND_END 0xFF

#define FTCOMPUTER_STICK_AUTOFULL 0x7F
#define FTCOMPUTER_STICK_AUTOHALF 0x80

typedef enum FTComputerCommandKind {
    nFTComputerCommandButtonAPress,
    nFTComputerCommandButtonARelease,
    nFTComputerCommandButtonBPress,
    nFTComputerCommandButtonBRelease,
    nFTComputerCommandButtonZPress,
    nFTComputerCommandButtonZRelease,
    nFTComputerCommandButtonLPress,
    nFTComputerCommandButtonLRelease,
    nFTComputerCommandButtonStartPress,
    nFTComputerCommandButtonStartRelease,
    nFTComputerCommandStickX,
    nFTComputerCommandStickY,
    nFTComputerCommandMoveAuto,
    nFTComputerCommandStickXVar,
    nFTComputerCommandStickYVar,
    nFTComputerCommandEnumCount
} FTComputerCommandKind;

#define FTCOMPUTER_COMMAND_BUTTON_A_PRESS \
    (nFTComputerCommandButtonAPress << FTCOMPUTER_COMMAND_TIMER_BITS)
#define FTCOMPUTER_COMMAND_BUTTON_A_RELEASE \
    (nFTComputerCommandButtonARelease << FTCOMPUTER_COMMAND_TIMER_BITS)
#define FTCOMPUTER_COMMAND_BUTTON_B_PRESS \
    (nFTComputerCommandButtonBPress << FTCOMPUTER_COMMAND_TIMER_BITS)
#define FTCOMPUTER_COMMAND_BUTTON_B_RELEASE \
    (nFTComputerCommandButtonBRelease << FTCOMPUTER_COMMAND_TIMER_BITS)
#define FTCOMPUTER_COMMAND_BUTTON_Z_PRESS \
    (nFTComputerCommandButtonZPress << FTCOMPUTER_COMMAND_TIMER_BITS)
#define FTCOMPUTER_COMMAND_BUTTON_Z_RELEASE \
    (nFTComputerCommandButtonZRelease << FTCOMPUTER_COMMAND_TIMER_BITS)
#define FTCOMPUTER_COMMAND_BUTTON_L_PRESS \
    (nFTComputerCommandButtonLPress << FTCOMPUTER_COMMAND_TIMER_BITS)
#define FTCOMPUTER_COMMAND_BUTTON_L_RELEASE \
    (nFTComputerCommandButtonLRelease << FTCOMPUTER_COMMAND_TIMER_BITS)
#define FTCOMPUTER_COMMAND_BUTTON_START_PRESS \
    (nFTComputerCommandButtonStartPress << FTCOMPUTER_COMMAND_TIMER_BITS)
#define FTCOMPUTER_COMMAND_BUTTON_START_RELEASE \
    (nFTComputerCommandButtonStartRelease << FTCOMPUTER_COMMAND_TIMER_BITS)
#define FTCOMPUTER_COMMAND_STICK_X_TILT \
    (nFTComputerCommandStickX << FTCOMPUTER_COMMAND_TIMER_BITS)
#define FTCOMPUTER_COMMAND_STICK_Y_TILT \
    (nFTComputerCommandStickY << FTCOMPUTER_COMMAND_TIMER_BITS)
#define FTCOMPUTER_COMMAND_MOVEAUTO \
    (nFTComputerCommandMoveAuto << FTCOMPUTER_COMMAND_TIMER_BITS)
#define FTCOMPUTER_COMMAND_STICK_X_VAR \
    (nFTComputerCommandStickXVar << FTCOMPUTER_COMMAND_TIMER_BITS)
#define FTCOMPUTER_COMMAND_STICK_Y_VAR \
    (nFTComputerCommandStickYVar << FTCOMPUTER_COMMAND_TIMER_BITS)
#define FTCOMPUTER_COMMAND_DEFAULT_MAX \
    (nFTComputerCommandEnumCount << FTCOMPUTER_COMMAND_TIMER_BITS)

#define FTCOMPUTER_EVENT_INSTRUCTION(kind, timer)                         \
    (((((kind) << FTCOMPUTER_COMMAND_TIMER_BITS) &                        \
       FTCOMPUTER_COMMAND_OPCODE_MASK) |                                  \
      ((timer) & FTCOMPUTER_COMMAND_TIMER_MASK)) & U8_MAX)
#define FTCOMPUTER_EVENT_STICK_X(value, timer)                            \
    FTCOMPUTER_EVENT_INSTRUCTION(nFTComputerCommandStickX, timer), value
#define FTCOMPUTER_EVENT_STICK_Y(value, timer)                            \
    FTCOMPUTER_EVENT_INSTRUCTION(nFTComputerCommandStickY, timer), value
#define FTCOMPUTER_EVENT_PKTHUNDER() FTCOMPUTER_COMMAND_PKTHUNDER
#define FTCOMPUTER_EVENT_END() FTCOMPUTER_COMMAND_END

typedef enum FTComputerInputKind {
    nFTComputerInputStickN,
    nFTComputerInputMoveAuto,
    nFTComputerInputStickTiltAutoX,
    nFTComputerInputStickNMoveAuto,
    nFTComputerInputMoveAutoStickTiltHiReleaseZ,
    nFTComputerInputStickTiltAutoXButtonA,
    nFTComputerInputStickTiltAutoXD5NButtonA,
    nFTComputerInputMoveAutoButtonA,
    nFTComputerInputStickSmashAutoXButtonA,
    nFTComputerInputStickSmashAutoXButtonB,
    nFTComputerInputStickTiltAutoXNYD5SmashAutoXButtonB,
    nFTComputerInputStickTiltAutoXNYD1ButtonB,
    nFTComputerInputButtonZ1,
    nFTComputerInputStickSmashHiButtonB,
    nFTComputerInputStickTiltAutoXD5SmashSButtonB,
    nFTComputerInputStickNButtonL,
    nFTComputerInputButtonZ2,
    nFTComputerInputStickTiltAutoXD5,
    nFTComputerInputStickTiltAutoXD1,
    nFTComputerInputStickNButtonA,
    nFTComputerInputStickTiltAutoXD5ButtonA,
    nFTComputerInputStickSmashAutoXNYButtonA,
    nFTComputerInputStickTiltAutoXD1SmashSButtonA,
    nFTComputerInputStickTiltHiButtonA,
    nFTComputerInputStickTiltAutoXD5TiltAutoYButtonA,
    nFTComputerInputStickSmashHiButtonA,
    nFTComputerInputStickTiltAutoXD5SmashAutoYButtonA,
    nFTComputerInputStickSmashLwButtonB,
    nFTComputerInputStickNButtonZButtonA,
    nFTComputerInputStickTiltAutoXD5ButtonZButtonA,
    nFTComputerInputStickSmashL,
    nFTComputerInputStickSmashR,
    nFTComputerInputStickTiltLwButtonA,
    nFTComputerInputStickTiltAutoXD5TiltLwButtonA,
    nFTComputerInputStickSmashLwButtonA,
    nFTComputerInputStickNButtonZHold,
    nFTComputerInputButtonZRelease,
    nFTComputerInputStickNXSmashLwButtonBReleaseBHold,
    nFTComputerInputStickNButtonBRelease,
    nFTComputerInputStickND1MoveAutoSmashLw,
    nFTComputerInputStickNButtonBZReleaseAPress,
    nFTComputerInputStickTiltAutoXButtonBZReleaseAPress,
    nFTComputerInputThrowItemImmediate,
    nFTComputerInputThrowItemWait,
    nFTComputerInputWiggle,
    nFTComputerInputEscapeL,
    nFTComputerInputEscapeR,
    nFTComputerInputYoshiSpecialHiAim,
    nFTComputerInputNessSpecialHiAim
} FTComputerInputKind;

typedef enum FTComputerTraitKind {
    nFTComputerTraitDefault,
    nFTComputerTraitLink,
    nFTComputerTraitYoshiTeam,
    nFTComputerTraitKirbyTeam,
    nFTComputerTraitPolyTeam,
    nFTComputerTraitMarioBros,
    nFTComputerTraitGDonkey,
    nFTComputerTraitUnk1,
    nFTComputerTraitBonus3,
    nFTComputerTraitAlly,
    nFTComputerTraitNone
} FTComputerTraitKind;

typedef enum FTComputerBehaviorKind {
    nFTComputerBehaviorDefault,
    nFTComputerBehaviorUnk1,
    nFTComputerBehaviorUnk2,
    nFTComputerBehaviorAlly,
    nFTComputerBehaviorCaptain,
    nFTComputerBehaviorUnk3,
    nFTComputerBehaviorUnk4,
    nFTComputerBehaviorYoshiTeam,
    nFTComputerBehaviorKirbyTeam,
    nFTComputerBehaviorPolyTeam,
    nFTComputerBehaviorUnused1,
    nFTComputerBehaviorUnused2,
    nFTComputerBehaviorUnused3,
    nFTComputerBehaviorBonus3,
    nFTComputerBehaviorUnused4,
    nFTComputerBehaviorStand,
    nFTComputerBehaviorWalk,
    nFTComputerBehaviorEvade,
    nFTComputerBehaviorJump,
    nFTComputerBehaviorUnk5
} FTComputerBehaviorKind;

typedef enum FTComputerObjectiveKind {
    nFTComputerObjectiveStand,
    nFTComputerObjectiveWalk,
    nFTComputerObjectiveAttack,
    nFTComputerObjectiveEvade,
    nFTComputerObjectiveRecover,
    nFTComputerObjectiveTrackItem,
    nFTComputerObjectiveUseItem,
    nFTComputerObjectiveCounterAttack,
    nFTComputerObjectiveUnknown1,
    nFTComputerObjectiveAlly,
    nFTComputerObjectivePatrol,
    nFTComputerObjectiveRush
} FTComputerObjectiveKind;

typedef struct FTComputerAttack {
    s32 input_kind;
    s32 hit_start_frame;
    s32 hit_end_frame;
    f32 detect_near_x;
    f32 detect_far_x;
    f32 detect_near_y;
    f32 detect_far_y;
} FTComputerAttack;

_Static_assert(sizeof(FTComputerAttack) == 0x1Cu,
               "FTComputerAttack size changed");

u16 syUtilsRandUShort(void);
f32 syUtilsRandFloat(void);
sb32 func_ovl2_800F8FFC(Vec3f *position);
sb32 grHyruleTwisterCheckGetPosition(Vec3f *position);
void grJungleTaruCannGetPosition(Vec3f *position);
f32 grJungleTaruCannGetRotate(void);
void grZebesAcidGetLevelInfo(f32 *current, f32 *step);
void ndsMPCollisionEnsureLineGroups(void);

#endif
