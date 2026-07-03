/*
 * Compile-only import of BattleShip ft/ftanimend.c.
 *
 * Public ftAnimEnd* callbacks remain on the current compatibility path until
 * original status tables and fighter-data loading are ready together.
 */
#include <ft/fighter.h>

#define ftAnimEndCheckSetStatus battleship_ftAnimEndCheckSetStatus
#define ftAnimEndSetWait battleship_ftAnimEndSetWait
#define ftAnimEndSetFall battleship_ftAnimEndSetFall
#include "../../decomp/BattleShip-main/decomp/src/ft/ftanimend.c"
#undef ftAnimEndCheckSetStatus
#undef ftAnimEndSetWait
#undef ftAnimEndSetFall
