/* Save data on the card (src/nds/nds_backup.c): the SRAM image the
 * transcribed lbbackup.c addresses by offset, and the sibling files the P7
 * records use; the note below gives the write and recovery order. */

#ifndef SSB64_NDS_BACKUP_H
#define SSB64_NDS_BACKUP_H

#include <PR/ultratypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

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

/* Siblings of the save (P7 U2's VS records): other files kept beside it on
 * the volume the save was read from, written through the same transaction.
 * Callers hold the file-system lock (ndsFsLock). Recovery order: the
 * canonical name, its .bak, its .tmp. */
#define NDS_BACKUP_SIBLING_RECORDS 0u
/* P7 C3's 1P records (nds_p7_records.c). */
#define NDS_BACKUP_SIBLING_1P_RECORDS 1u
#define NDS_BACKUP_RECOVERY_NAMES 3u
typedef s32 (*NDSBackupWriter)(FILE *tmp, void *ctx);
FILE *ndsBackupOpenSibling(u32 sibling, u32 recovery, const char *mode);
/* `write` fills the temporary; `*src` (a file being read for it, or NULL)
 * is closed before the promotion renames anything; the canonical file is
 * kept as .bak. TRUE when the new file is in place. */
s32 ndsBackupTransactSibling(u32 sibling, NDSBackupWriter write, void *ctx,
                             FILE **src);
/* A sibling's temporary promoted to its canonical name: the one valid copy
 * left by an interrupted promotion, before that name is written again. */
s32 ndsBackupPromoteSibling(u32 sibling);
void ndsFsLock(void);
void ndsFsUnlock(void);

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
