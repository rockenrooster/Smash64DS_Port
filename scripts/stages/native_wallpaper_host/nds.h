/* Host-only libnds shadow for the native wallpaper runtime test.
 * The real TU compiles unmodified against this plus the repo headers;
 * only the pieces src/nds/nds_native_wallpaper.c pulls from libnds live
 * here (fixed types come from PR/ultratypes.h as on DS). */
#ifndef SSB64_NDS_HOST_NDS_H
#define SSB64_NDS_HOST_NDS_H

#include <PR/ultratypes.h>

void dmaFillHalfWords(u16 value, void *dest, u32 size);

#endif
