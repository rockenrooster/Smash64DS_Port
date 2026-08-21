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

/* Implemented by the imported BattleShip ifCommon translation unit.  Returns
 * FALSE when the source slot does not exist; otherwise `out` is one coherent
 * copy of the source damage-display state for that player. */
u32 ndsIFCommonGetBattleHudDamageState(u32 player,
                                       NDSBattleHudDamageState *out);

extern volatile u32 gNdsBattleHudPrepareCount;
extern volatile u32 gNdsBattleHudRenderCount;
extern volatile u32 gNdsBattleHudChangeCount;
extern volatile u32 gNdsBattleHudOamCount;
extern volatile u32 gNdsBattleHudActiveMask;

#endif /* SSB64_NDS_BATTLE_HUD_H */
