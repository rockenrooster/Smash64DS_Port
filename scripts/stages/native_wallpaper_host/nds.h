/* Host-only libnds shadow for the native wallpaper runtime test.
 * The real TU compiles unmodified against this plus the repo headers;
 * only the pieces src/nds/nds_native_wallpaper.c pulls from libnds live
 * here (fixed types come from PR/ultratypes.h as on DS). */
#ifndef SSB64_NDS_HOST_NDS_H
#define SSB64_NDS_HOST_NDS_H

#include <PR/ultratypes.h>

void dmaFillHalfWords(u16 value, void *dest, u32 size);

/* P8 V5's VS battle tiles reach libnds's texture allocator and the VRAM
 * control registers. The host never runs a VS battle scene, so they only
 * have to exist (harness.c stops the run if any is reached). */
typedef struct s_vramBlock s_vramBlock;
typedef struct gl_hidden_globals
{
    s_vramBlock *vramBlocks[2];
    int vramLock[2];
} gl_hidden_globals;
extern gl_hidden_globals glGlobalData;
#define VRAM_ENABLE 0x80
#define VRAM_C ((u16 *)0x06840000)
int glLockVRAMBank(u16 *addr);

/* The runtime's ARM-state functions (`__attribute__((target("arm")))`) mean
 * nothing to the host compiler: the attribute becomes `unused`. */
#define target(isa) unused
/* The Makefile's generated config header, counters-on arm. */
#define NDS_DIAG(...) do { __VA_ARGS__; } while (0)

#endif
