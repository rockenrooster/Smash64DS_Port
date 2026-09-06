#ifndef SSB64_NDS_BACKUP_H
#define SSB64_NDS_BACKUP_H

#include <PR/ultratypes.h>
#include <stddef.h>
#include <stdint.h>

/* P2-7 save data. The source keeps its save in N64 SRAM: lbbackup.c writes
 * two copies of LBBackupData (at byte 0 and at the next 16-byte boundary past
 * the first), each with a checksum and a signature, and reads the second when
 * the first fails. The DS has no SRAM, so this module is the SRAM: a fixed
 * image in main RAM that the transcribed lbbackup.c reads and writes by byte
 * offset exactly as it addressed the chip, backed by one file on the FAT
 * volume the ROM runs from (`fat:` on a flashcart through DLDI, `sd:` on a
 * DSi). The file is written whole to a temporary name, while the previous
 * canonical file is retained as .sav.bak through promotion. Boot validates
 * the source's checksum/signature in .sav, .sav.bak and .sav.tmp before any
 * write probe, so interrupted promotion can recover a complete image.
 * Automated/forced harness builds use smash64ds-diagnostic.sav and the same
 * recovery suffixes; normal human-input builds use smash64ds.sav.
 * With no writable volume the image still works for the session and the
 * source's own defaults apply, exactly as a fresh cartridge would.
 *
 * 4 KiB holds both copies of the 12-fighter LBBackupData with room to spare;
 * battleship_lbbackup.c asserts that at compile time. */
#define NDS_BACKUP_IMAGE_BYTES 4096u

/* The two SRAM DMA calls lbbackup.c makes, with their source argument order. */
void ndsBackupSramRead(uintptr_t sram_src, void *ram_dst, size_t size);
void ndsBackupSramWrite(void *ram_src, uintptr_t sram_dst, size_t size);
/* Write the image to the file. lbBackupWrite calls it once after both of its
 * copies land, so a save is one file write, not two. TRUE on success. */
s32 ndsBackupFlush(void);

/* One read each, for the verifier and for gdb. */
#define NDS_BACKUP_LOAD_UNTRIED 0u
#define NDS_BACKUP_LOAD_FILE 1u    /* image came from the save file */
#define NDS_BACKUP_LOAD_ABSENT 2u  /* volume present, no file: fresh save */
#define NDS_BACKUP_LOAD_NO_VOLUME 3u /* no writable volume: session only */
extern volatile u32 gNdsBackupLoadResult;
extern volatile u32 gNdsBackupWriteCount;
extern volatile u32 gNdsBackupWriteFailCount;
/* lbBackupApplyOptions' sound_mono_or_stereo, recorded for the mixer. */
extern volatile u32 gNdsBackupSoundMode;

#endif
