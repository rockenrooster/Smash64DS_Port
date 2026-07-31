/* Source-backed matrix utilities. lb/lbParticleGetPosVelDObj calls guMtxIdentF,
 * which nothing in the build defined -- battleship_wpmanager_core.c only
 * declared it, and never called it, so the omission never reached the linker.
 * Same shape as battleship_libultra_gu_mtxcatf.c: compile the original in place.
 * guMtxF2L, guMtxIdent and guMtxL2F come along and are dropped by
 * --gc-sections; none of them is defined anywhere else in the build. */
#include "../../decomp/BattleShip-main/decomp/src/libultra/gu/mtxutil.c"
