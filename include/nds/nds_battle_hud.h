#ifndef SSB64_NDS_BATTLE_HUD_H
#define SSB64_NDS_BATTLE_HUD_H

#include <PR/ultratypes.h>

/* Exact presentation state owned by BattleShip's IFPlayerDamage.  Keeping this
 * small POD snapshot at the source/DS boundary lets the lower-screen renderer
 * move the HUD without replacing the source damage bounce, flash, death linger
 * or shield-break digit motion with a second state machine. */
#define NDS_BATTLE_HUD_DAMAGE_CHARS 4u

typedef struct NDSBattleHudDamageCharState {
    f32 pos_x;
    f32 pos_y;
    u8 image_id;
    u8 visible;
} NDSBattleHudDamageCharState;

typedef struct NDSBattleHudDamageState {
    f32 scale;
    s32 damage;
    u8 color_r;
    u8 color_g;
    u8 color_b;
    u8 color_id;
    u8 is_update_anim;
    u8 char_count;
    u8 visible;
    NDSBattleHudDamageCharState chars[NDS_BATTLE_HUD_DAMAGE_CHARS];
} NDSBattleHudDamageState;

/* P2-2 lower-screen presentation sink.
 *
 * BattleShip's imported ifCommon GObjs remain the gameplay/state authority.
 * This module owns only sub-engine OBJ VRAM/OAM and renders the four-wide state
 * published by battleship_ifcommon.c with source-derived AOT artwork. */
void ndsBattleHudRender(void);
void ndsBattleHudClear(void);
/* Per-present output of the source score particles; no score/timer authority. */
u32 ndsBattleHudSubmitScoreParticle(u32 frame, f32 source_x, f32 source_y, f32 size);

/* Implemented by the imported BattleShip ifCommon translation unit.  Returns
 * FALSE when the source slot does not exist; otherwise `out` is one coherent
 * copy of the source damage-display state for that player. */
u32 ndsIFCommonGetBattleHudDamageState(u32 player,
                                       NDSBattleHudDamageState *out);

/* The 1P team stock row (sc1PGameTeamStockDisplayProcDisplay), one look per
 * shown icon in source order, published by battleship_ifcommon.c's lower-HUD
 * route: the index of the team fighter's stock LUT the source drew the icon
 * with, or the Polygon Team's own sprite. 30 is the largest team
 * (SC1PGAME_STAGE_MAX_TEAM_COUNT). */
#define NDS_BATTLE_HUD_TEAM_STOCK_MAX 30u
#define NDS_BATTLE_HUD_TEAM_LOOK_ZAKO 0xfeu
#define NDS_BATTLE_HUD_TEAM_LOOK_NONE 0xffu
extern volatile u8 gNdsIFCommonHUDTeamStockLook[NDS_BATTLE_HUD_TEAM_STOCK_MAX];

/* Bonus Practice's count-up timer (sc1PBonusStageMakeTimer), published by the
 * same route on each pass that draws it and consumed by each render: its six
 * digits (sSC1PBonusStageTimerDigits) and which of its eight SObjs -- six
 * digits, then the two marks -- the source shows. */
extern volatile u32 gNdsIFCommonHUDBonusTimerVisible;
extern volatile u32 gNdsIFCommonHUDBonusTimerMask;
extern volatile u8 gNdsIFCommonHUDBonusTimerDigits[6];

extern volatile u32 gNdsBattleHudPrepareCount;
extern volatile u32 gNdsBattleHudRenderCount;
extern volatile u32 gNdsBattleHudChangeCount;
extern volatile u32 gNdsBattleHudOamCount;
extern volatile u32 gNdsBattleHudActiveMask;

#endif /* SSB64_NDS_BATTLE_HUD_H */
