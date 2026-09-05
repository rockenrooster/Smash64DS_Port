/* Save data, transcribed verbatim from BattleShip decomp/src/lb/lbbackup.c.
 *
 * A textual include does not work here: lbbackup.c includes <lb/library.h>,
 * which is the decomp's own lbtypes.h with a second LBBackupData, and the port
 * defines that struct in include/sc/scene.h. So the twelve functions are
 * transcribed with source ordering and every expression preserved, cited by
 * source line.
 *
 * Two adaptations, both at the hardware seam and nowhere else:
 *   - syDmaReadSram / syDmaWriteSram become the nds_backup.c image, addressed
 *     by the same byte offsets the source addressed the SRAM chip with, and
 *     lbBackupWrite flushes that image to the save file once after both copies
 *     land (two DMA writes in the source, one file write here).
 *   - lbBackupApplyOptions' two sys calls: syAudioSetQuality is applied to the
 *     mixer at boot (src/nds/nds_audio_bgm.c owns dSYAudioSoundQuality and the
 *     setter; the value is also kept in gNdsBackupSoundMode), and
 *     syVideoSetCenterOffsets is an N64 screen-adjust with no DS meaning -- an
 *     intentional delta, docs/p2/P2-7-modes-meta.md item 5.
 * Everything else, including the double copy, the checksum, the 666 signature
 * and lbBackupCorrectErrors' fallbacks, is the source's. */
#include <ssb_types.h>
#include <ft/fighter.h>
#include <sc/scene.h>
#include <sys/audio.h>
#include <nds/nds_backup.h>

#ifndef ARRAY_COUNT
#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))
#endif

/* ALIGN(sizeof(LBBackupData), 0x0) and ALIGN(sizeof(LBBackupData), 0x10):
 * the source's two SRAM offsets (lbbackup.c:38-39, :47, :50). */
#define NDS_LBBACKUP_COPY0_OFFSET 0u
#define NDS_LBBACKUP_COPY1_OFFSET \
    ((uintptr_t)((sizeof(LBBackupData) + 0xfu) & ~(size_t)0xfu))

_Static_assert(NDS_LBBACKUP_COPY1_OFFSET + sizeof(LBBackupData) <=
                   NDS_BACKUP_IMAGE_BYTES,
               "both save copies must fit the backup image");

/* decomp/BattleShip-main/decomp/src/lb/lbbackup.c:13-23 */
s32 lbBackupCreateChecksum(LBBackupData *backup)
{
    s32 i, checksum = 0;
    u8 *bytes = (u8 *)backup;

    for (i = 0; i < (s32)(sizeof(LBBackupData) - sizeof(gSCManagerBackupData.checksum)); i++)
    {
        checksum += *bytes++ * (i + 1);
    }
    return checksum;
}

/* lbbackup.c:26-33 */
sb32 lbBackupIsChecksumValid(void)
{
    if ((lbBackupCreateChecksum(&gSCManagerBackupData) == gSCManagerBackupData.checksum) &&
        (gSCManagerBackupData.signature == 666))
    {
        return TRUE;
    }
    else return FALSE;
}

/* lbbackup.c:36-41 */
void lbBackupWrite(void)
{
    gSCManagerBackupData.checksum = lbBackupCreateChecksum(&gSCManagerBackupData);
    ndsBackupSramWrite(&gSCManagerBackupData, NDS_LBBACKUP_COPY0_OFFSET, sizeof(LBBackupData));
    ndsBackupSramWrite(&gSCManagerBackupData, NDS_LBBACKUP_COPY1_OFFSET, sizeof(LBBackupData));
    (void)ndsBackupFlush();
}

/* lbbackup.c:44-63 */
sb32 lbBackupIsSramValid(void)
{
    ndsBackupSramRead(NDS_LBBACKUP_COPY0_OFFSET, &gSCManagerBackupData, sizeof(LBBackupData));
    if (lbBackupIsChecksumValid() == FALSE)
    {
        ndsBackupSramRead(NDS_LBBACKUP_COPY1_OFFSET, &gSCManagerBackupData, sizeof(LBBackupData));
        if (lbBackupIsChecksumValid() == FALSE)
        {
            gSCManagerBackupData = dSCManagerDefaultBackupData;
            lbBackupWrite();
            return FALSE;
        }
        lbBackupWrite();
    }
    return TRUE;
}

/* lbbackup.c:66-74. See the file comment for the two sys calls. */
void lbBackupApplyOptions(void)
{
    gNdsBackupSoundMode = gSCManagerBackupData.sound_mono_or_stereo;
    syAudioSetQuality(gSCManagerBackupData.sound_mono_or_stereo);
    /* syVideoSetCenterOffsets(screen_adjust_h, screen_adjust_h,
     *                         screen_adjust_v, screen_adjust_v): no DS meaning. */
}

/* lbbackup.c:77-123 */
void lbBackupCorrectErrors(void)
{
    s32 i;

    if (!((gSCManagerBackupData.fighter_mask | LBBACKUP_CHARACTER_MASK_STARTER) & (1 << gSCManagerBackupData.characters_fkind)))
    {
        gSCManagerBackupData.characters_fkind = dSCManagerDefaultBackupData.characters_fkind;
    }
    if (!((gSCManagerBackupData.fighter_mask | LBBACKUP_CHARACTER_MASK_STARTER) & (1 << gSCManagerSceneData.fkind)))
    {
        gSCManagerSceneData.fkind = nFTKindNull;
    }
    if (!((gSCManagerBackupData.fighter_mask | LBBACKUP_CHARACTER_MASK_STARTER) & (1 << gSCManagerSceneData.training_man_fkind)))
    {
        gSCManagerSceneData.training_man_fkind = nFTKindNull;
    }
    if (!((gSCManagerBackupData.fighter_mask | LBBACKUP_CHARACTER_MASK_STARTER) & (1 << gSCManagerSceneData.training_com_fkind)))
    {
        gSCManagerSceneData.training_com_fkind = nFTKindNull;
    }
    for (i = 0; i < ARRAY_COUNT(gSCManagerTransferBattleState.players); i++)
    {
        if (!((1 << gSCManagerTransferBattleState.players[i].fkind) & (gSCManagerBackupData.fighter_mask | LBBACKUP_CHARACTER_MASK_STARTER)))
        {
            gSCManagerTransferBattleState.players[i].fkind = nFTKindNull;
            gSCManagerTransferBattleState.players[i].pkind = nFTPlayerKindMan;
        }
    }
    if (!(gSCManagerBackupData.unlock_mask & LBBACKUP_UNLOCK_MASK_INISHIE))
    {
        if (gSCManagerSceneData.maps_vsmode_gkind == nGRKindInishie)
        {
            gSCManagerSceneData.maps_vsmode_gkind = dSCManagerDefaultSceneData.maps_vsmode_gkind;
        }
        if (gSCManagerSceneData.maps_training_gkind == nGRKindInishie)
        {
            gSCManagerSceneData.maps_training_gkind = dSCManagerDefaultSceneData.maps_training_gkind;
        }
    }
    /* REGION_US arm, lbbackup.c:113-119. */
    if (!(gSCManagerBackupData.unlock_mask & LBBACKUP_UNLOCK_MASK_ITEMSWITCH))
    {
        gSCManagerTransferBattleState.item_toggles = dSCManagerDefaultBattleState.item_toggles;
        gSCManagerTransferBattleState.item_appearance_rate = dSCManagerDefaultBattleState.item_appearance_rate;
    }
}

/* lbbackup.c:126-131 */
void lbBackupClearNewcomers(void)
{
    gSCManagerBackupData.unlock_mask &= ~LBBACKUP_UNLOCK_MASK_NEWCOMERS;
    gSCManagerBackupData.unlock_mask |= dSCManagerDefaultBackupData.unlock_mask;
    gSCManagerBackupData.fighter_mask = dSCManagerDefaultBackupData.fighter_mask;
}

/* lbbackup.c:135-146 */
void lbBackupClear1PHighScore(void)
{
    s32 i;

    for (i = 0; i < ARRAY_COUNT(gSCManagerBackupData.spgame_records); i++)
    {
        gSCManagerBackupData.spgame_records[i].spgame_hiscore         = dSCManagerDefaultBackupData.spgame_records[i].spgame_hiscore;
        gSCManagerBackupData.spgame_records[i].spgame_continues       = dSCManagerDefaultBackupData.spgame_records[i].spgame_continues;
        gSCManagerBackupData.spgame_records[i].spgame_total_bonuses   = dSCManagerDefaultBackupData.spgame_records[i].spgame_total_bonuses;
        gSCManagerBackupData.spgame_records[i].spgame_best_difficulty = dSCManagerDefaultBackupData.spgame_records[i].spgame_best_difficulty;
        gSCManagerBackupData.spgame_records[i].is_spgame_complete     = dSCManagerDefaultBackupData.spgame_records[i].is_spgame_complete;
    }
}

/* lbbackup.c:150-159 */
void lbBackupClearVSRecord(void)
{
    s32 i;

    for (i = 0; i < ARRAY_COUNT(gSCManagerBackupData.vs_records); i++)
    {
        gSCManagerBackupData.vs_records[i] = dSCManagerDefaultBackupData.vs_records[i];
    }
    gSCManagerBackupData.vs_total_battles = dSCManagerDefaultBackupData.vs_total_battles;
}

/* lbbackup.c:162-173 */
void lbBackupClearBonusStageTime(void)
{
    s32 i;

    for (i = 0; i < ARRAY_COUNT(gSCManagerBackupData.spgame_records); i++)
    {
        gSCManagerBackupData.spgame_records[i].bonus1_time       = dSCManagerDefaultBackupData.spgame_records[i].bonus1_time;
        gSCManagerBackupData.spgame_records[i].bonus1_task_count = dSCManagerDefaultBackupData.spgame_records[i].bonus1_task_count;
        gSCManagerBackupData.spgame_records[i].bonus2_time       = dSCManagerDefaultBackupData.spgame_records[i].bonus2_time;
        gSCManagerBackupData.spgame_records[i].bonus2_task_count = dSCManagerDefaultBackupData.spgame_records[i].bonus2_task_count;
    }
}

/* lbbackup.c:176-183 */
void lbBackupClearPrize(void)
{
    gSCManagerBackupData.unlock_mask &= ~LBBACKUP_UNLOCK_MASK_PRIZE;
    gSCManagerBackupData.unlock_mask |= dSCManagerDefaultBackupData.unlock_mask;
    gSCManagerBackupData.ground_mask = dSCManagerDefaultBackupData.ground_mask;
    gSCManagerBackupData.vs_itemswitch_battles = dSCManagerDefaultBackupData.vs_itemswitch_battles;
}

/* lbbackup.c:186-189 */
void lbBackupClearAllData(void)
{
    gSCManagerBackupData = dSCManagerDefaultBackupData;
}
