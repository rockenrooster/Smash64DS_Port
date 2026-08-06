/**
 * debug_stubs.c — Stubs for debug scene functions and misc remaining symbols.
 *
 * The debug scene code (src/db/) is excluded from the port build, but
 * the scene manager (src/sc/scmanager.c) still references the debug
 * scene entry points in its scene switch table.  These empty stubs
 * allow linking — the debug scenes simply do nothing in the port.
 */

#include <ssb_types.h>

/* ========================================================================= */
/*  Debug scene entry points (from src/db/)                                  */
/* ========================================================================= */

void dbBattleStartScene(void)
{
}

void dbCubeStartScene(void)
{
}

void dbFallsStartScene(void)
{
}

void dbMapsStartScene(void)
{
}

/* ========================================================================= */
/*  Misc data symbols                                                        */
/* ========================================================================= */

/*
 * D_8009EDD0_406D0 was previously stubbed here as alSoundEffect (64
 * bytes) when src/libultra/n_audio/ was excluded from the build.  It is
 * the global ALWhatever8009EDD0 audio-engine state struct, ~168 bytes
 * on LP64.  Now that n_audio is in the build, the n_env.c definition is
 * authoritative — keeping this stub silently corrupted memory because
 * the linker chose the smaller "real" symbol over n_env.c's tentative
 * definition, so all writes past byte 64 (fgm_table_count, every
 * unk_alsound_* freelist head, etc.) hit adjacent BSS — most visibly
 * the FGM siz34 array, where byte 0x3F of unk_0x28 was getting
 * clobbered by writes intended for unk_alsound_0x* fields.
 */

/*
 * D_NF_00006010 and D_NF_00006450 are file-relative offsets used in
 * src/sc/sc1pmode/sc1pgame.c for camera animation setup.  On N64, the
 * linker places these symbols at absolute addresses matching the offset
 * values, and the code uses &D_NF_00006010 to obtain the address 0x6010.
 *
 * On PC, taking &variable gives a heap/stack address, not the offset.
 * For now, these are defined as intptr_t with the offset as the value.
 * The code in sc1pgame.c that uses (intptr_t)&D_NF_* will need #ifdef
 * PORT adjustments to use the value directly instead of the address.
 *
 * Declared as extern intptr_t in src/sc/sc1pmode/sc1pgame.c.
 */
intptr_t D_NF_00006010 = 0x6010;
intptr_t D_NF_00006450 = 0x6450;
