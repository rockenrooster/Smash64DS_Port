#ifndef SSB64_NDS_IF_INTERFACE_H
#define SSB64_NDS_IF_INTERFACE_H

#include <sys/objtypes.h>

/*
 * Narrow interface shadow for imported movie scenes.
 *
 * The upstream interface header pulls the full HUD/game interface tree. The
 * bounded OpeningMario slice only needs this include to exist; no interface
 * data or functions are used before the DS seam defers fighter/ground setup.
 */

typedef struct LBGenerator {
    DObj *dobj;
} LBGenerator;

typedef struct IFPlayerCommon {
    ub8 is_magnify_display;
    u8 magnify_mode;
    f32 magnify_scale;
    s32 *player_pos_x;
    u16 player_pos_y;
    u8 arrows_flags;
    u8 arrows_left_status;
    u8 arrows_right_status;
} IFPlayerCommon;

extern u8 dIFCommonPlayerTeamColorIDs[];
extern IFPlayerCommon gIFCommonPlayerInterface;

void efParticleInitAll(void);
s32 efParticleGetLoadBankID(void *script_lo, void *script_hi,
                            void *texture_lo, void *texture_hi);
LBGenerator *lbParticleMakeGenerator(s32 bank_id, s32 generator_id);
void lbParticleDrawTextures(GObj *gobj);
void ifCommonBattleUpdateInterfaceAll(void);
void ifCommonBattleSetGameStatusWait(void);
void ifCommonPlayerArrowsInitInterface(void);
void ifCommonPlayerMagnifyMakeInterface(void);
void ifCommonPlayerTagMakeInterface(void);
void ifCommonPlayerDamageSetDigitPositions(void);
void ifCommonPlayerDamageInitInterface(void);
void ifCommonPlayerStockInitInterface(void);
void ifCommonEntryAllMakeInterface(void);
void ifCommonTimerMakeInterface(void (*func_init)(void));
void ifCommonTimerMakeDigits(void);
void ifCommonSuddenDeathMakeInterface(void);
void ifCommonAnnounceTimeUpInitInterface(void);
void ifScreenFlashMakeInterface(u8 alpha);

#endif
