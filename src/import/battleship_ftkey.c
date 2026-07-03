/*
 * Compile-only import of BattleShip ft/ftkey.c.
 *
 * Imported ftmain.c still calls the current public ftKeyProcessKeyEvents until
 * this slice can switch the full animation/status runtime live.
 */
#include <ft/fighter.h>

#ifndef ftKeyEventCast
#define ftKeyEventCast(script, type) ((type *)(script))
#endif
#ifndef ftKeyGetButtons
#define ftKeyGetButtons(script) (*(ftKeyEventCast((script), u16)))
#endif
#ifndef ftKeyGetStickRange
#define ftKeyGetStickRange(script) (ftKeyEventCast((script), Vec2b))
#endif

#define ftKeyProcessKeyEvents battleship_ftKeyProcessKeyEvents
#include "../../decomp/BattleShip-main/decomp/src/ft/ftkey.c"
#undef ftKeyProcessKeyEvents
