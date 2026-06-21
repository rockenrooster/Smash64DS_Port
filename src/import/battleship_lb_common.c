/* PARKED: compile the original BattleShip lbcommon translation unit.
 *
 * This wrapper is intentionally not listed in Makefile CFILES right now. The
 * full lbcommon.c import currently needs fighter part layouts, camera look-at
 * helpers, and many N64 display-list macros beyond the current boundary. The
 * active build uses a narrow startup SObj shim in src/port/scene_backend.c.
 *
 * Provides real lbCommonMakeSObjForGObj, lbCommonDrawSObjAttr,
 * lbCommonDrawSprite, and the sprite-pipeline helpers. The display
 * callbacks are only reached inside the (currently parked) task loop. */
#include "../../decomp/BattleShip-main/decomp/src/lb/lbcommon.c"
