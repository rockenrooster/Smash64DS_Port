/**
 * segment_symbols.c — Zero-valued definitions for N64 linker-script symbols.
 *
 * On N64, the linker script provides symbols that describe ROM offsets and
 * VRAM addresses for each overlay code segment, audio data region, and
 * particle data bank.  In the PC port all code is statically linked and
 * assets are loaded from O2R archives, so these symbols are unused — but
 * the decomp source still references them, so they must exist to satisfy
 * the linker.
 *
 * Every symbol is initialised to zero.
 */

#include <ssb_types.h>

/* ========================================================================= */
/*  Overlay segment symbols (ovl0–ovl65 + scmanager)                         */
/* ========================================================================= */

#define OVERLAY_SEGMENTS \
	X(ovl0)  X(ovl1)  X(ovl2)  X(ovl3)  X(ovl4)  X(ovl5)  X(ovl6)  X(ovl7)  \
	X(ovl8)  X(ovl9)  X(ovl10) X(ovl11) X(ovl12) X(ovl13) X(ovl14) X(ovl15) \
	X(ovl16) X(ovl17) X(ovl18) X(ovl19) X(ovl20) X(ovl21) X(ovl22) X(ovl23) \
	X(ovl24) X(ovl25) X(ovl26) X(ovl27) X(ovl28) X(ovl29) X(ovl30) X(ovl31) \
	X(ovl32) X(ovl33) X(ovl34) X(ovl35) X(ovl36) X(ovl37) X(ovl38) X(ovl39) \
	X(ovl40) X(ovl41) X(ovl42) X(ovl43) X(ovl44) X(ovl45) X(ovl46) X(ovl47) \
	X(ovl48) X(ovl49) X(ovl50) X(ovl51) X(ovl52) X(ovl53) X(ovl54) X(ovl55) \
	X(ovl56) X(ovl57) X(ovl58) X(ovl59) X(ovl60) X(ovl61) X(ovl62) X(ovl63) \
	X(ovl64) X(ovl65) X(scmanager)

#define X(name) \
	uintptr_t name##_ROM_START  = 0; \
	uintptr_t name##_ROM_END    = 0; \
	uintptr_t name##_VRAM       = 0; \
	uintptr_t name##_TEXT_START = 0; \
	uintptr_t name##_TEXT_END   = 0; \
	uintptr_t name##_DATA_START = 0; \
	uintptr_t name##_RODATA_END = 0; \
	uintptr_t name##_BSS_START  = 0; \
	uintptr_t name##_BSS_END    = 0;
OVERLAY_SEGMENTS
#undef X

/* ========================================================================= */
/*  Audio ROM segment symbols                                                */
/* ========================================================================= */

/*
 * On N64, these mark audio data locations in the ROM cartridge.
 * In the port, audio data is loaded from O2R archives instead.
 */

uintptr_t S1_music_sbk_ROM_START   = 0;
uintptr_t S1_music_sbk_ROM_END     = 0;
uintptr_t B1_sounds1_ctl_ROM_START = 0;
uintptr_t B1_sounds1_ctl_ROM_END   = 0;
uintptr_t B1_sounds2_ctl_ROM_START = 0;
uintptr_t B1_sounds2_ctl_ROM_END   = 0;
uintptr_t fgm_unk_ROM_START        = 0;
uintptr_t fgm_unk_ROM_END          = 0;
uintptr_t fgm_tbl_ROM_START        = 0;
uintptr_t fgm_tbl_ROM_END          = 0;
uintptr_t fgm_ucd_ROM_START        = 0;
uintptr_t fgm_ucd_ROM_END          = 0;

/* ========================================================================= */
/*  Particle ROM segment symbols (declared as s32 in decomp)                 */
/* ========================================================================= */

/*
 * Declared as extern s32 in src/ft/ftdata.c.
 */

s32 particles_unk0_scb_ROM_START = 0;
s32 particles_unk0_scb_ROM_END   = 0;
s32 particles_unk0_txb_ROM_START = 0;
s32 particles_unk0_txb_ROM_END   = 0;
s32 particles_unk1_scb_ROM_START = 0;
s32 particles_unk1_scb_ROM_END   = 0;
s32 particles_unk1_txb_ROM_START = 0;
s32 particles_unk1_txb_ROM_END   = 0;
s32 particles_unk2_scb_ROM_START = 0;
s32 particles_unk2_scb_ROM_END   = 0;
s32 particles_unk2_txb_ROM_START = 0;
s32 particles_unk2_txb_ROM_END   = 0;

/* ========================================================================= */
/*  Particle bank ROM pointers                                               */
/* ========================================================================= */

/*
 * On N64 the linker places these at specific ROM addresses.
 * Types must match the extern declarations in the decomp headers.
 */

/* uintptr_t — from src/ef/efdisplay.h */
uintptr_t lEFCommonParticleScriptBankLo  = 0;
uintptr_t lEFCommonParticleScriptBankHi  = 0;
uintptr_t lEFCommonParticleTextureBankLo = 0;
uintptr_t lEFCommonParticleTextureBankHi = 0;

/* intptr_t — from src/gr/grcommon/gryoster.h */
intptr_t lGRYosterParticleScriptBankLo  = 0;
intptr_t lGRYosterParticleScriptBankHi  = 0;
intptr_t lGRYosterParticleTextureBankLo = 0;
intptr_t lGRYosterParticleTextureBankHi = 0;

/* intptr_t — from src/it/itmanager.h */
intptr_t lITManagerParticleScriptBankLo  = 0;
intptr_t lITManagerParticleScriptBankHi  = 0;
intptr_t lITManagerParticleTextureBankLo = 0;
intptr_t lITManagerParticleTextureBankHi = 0;

/* intptr_t — from src/gr/grcommon/grpupupu.h */
intptr_t lGRPupupuParticleScriptBankLo  = 0;
intptr_t lGRPupupuParticleScriptBankHi  = 0;
intptr_t lGRPupupuParticleTextureBankLo = 0;
intptr_t lGRPupupuParticleTextureBankHi = 0;

/* intptr_t — from src/gr/grcommon/grhyrule.h */
intptr_t lGRHyruleParticleScriptBankLo  = 0;
intptr_t lGRHyruleParticleScriptBankHi  = 0;
intptr_t lGRHyruleParticleTextureBankLo = 0;
intptr_t lGRHyruleParticleTextureBankHi = 0;

/* uintptr_t — from src/mn/mncommon/mntitle.h */
uintptr_t lMNTitleParticleScriptBankLo  = 0;
uintptr_t lMNTitleParticleScriptBankHi  = 0;
uintptr_t lMNTitleParticleTextureBankLo = 0;
uintptr_t lMNTitleParticleTextureBankHi = 0;

/* ========================================================================= */
/*  File relocation table ROM address                                        */
/* ========================================================================= */

/*
 * On N64, this points to the file table at 0x001AC870.
 * In the port, file loading is handled by the O2R resource system.
 * Declared in src/lb/lbreloc.h.
 */
uintptr_t lLBRelocTableAddr = 0;
