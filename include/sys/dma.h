#ifndef SSB64_NDS_SYS_DMA_H
#define SSB64_NDS_SYS_DMA_H

#include <stddef.h>
#include <stdint.h>
#include <PR/os.h>

typedef struct SYOverlay {
    uintptr_t rom_start;
    uintptr_t rom_end;
    uintptr_t ram_load_start;
    uintptr_t ram_text_start;
    uintptr_t ram_text_end;
    uintptr_t ram_data_start;
    uintptr_t ram_data_end;
    uintptr_t ram_noload_start;
    uintptr_t ram_noload_end;
} SYOverlay;

extern OSPiHandle *gSYDmaRomPiHandle;

OSPiHandle *syDmaSramPiInit(void);
void syDmaCreateMesgQueue(void);
void syDmaReadRom(uintptr_t rom_source, void *ram_destination, size_t size);
void syDmaLoadOverlay(SYOverlay *overlay);

#endif

