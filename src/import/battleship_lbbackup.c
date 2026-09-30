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
#if NDS_P4_METAKNIGHT
#include <string.h>
#include <nds/nds_meta_record_codec.h>
#include <nds/nds_metaknight_lifecycle.h>
#include <nds/nds_p4_roster.h>
#endif

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

#if NDS_P4_METAKNIGHT
/* Compiler-qualified source ARM32 layout: VSRecord92, Backup1516, copy1
 * begins1520 and ends3036. Derived offsets below reserve the unused tail;
 * no legacy data/checksum/meaning moves. Own copies occupy3040..3420. */
#define NDS_META_RECORD_COPY0 \
    ((NDS_LBBACKUP_COPY1_OFFSET + sizeof(LBBackupData) + 15u) & ~(uintptr_t)15u)
#define NDS_META_RECORD_COPY_STRIDE ((NDS_META_RECORD_BYTES + 15u) & ~15u)
#define NDS_META_RECORD_COPY1 (NDS_META_RECORD_COPY0 + NDS_META_RECORD_COPY_STRIDE)
typedef struct NDSMetaVSRecordPair {
    u16 ko;
    u16 player_tally;
    u16 played_against;
} NDSMetaVSRecordPair;
static LBBackupVSRecord sNdsMetaVSRecord;
static NDSMetaVSRecordPair sNdsMetaMirrorRecord;
static NDSMetaVSRecordPair sNdsLegacyAgainstMeta[12];
static sb32 sNdsMetaVSRecordLoaded;

_Static_assert(sizeof(LBBackupVSRecord) == 92u, "Meta record codec source aggregate extent changed");
_Static_assert(sizeof(NDSMetaVSRecordPair) == 6u, "Meta pair codec extent changed");
_Static_assert(NDS_META_RECORD_COPY0 >= NDS_LBBACKUP_COPY1_OFFSET + sizeof(LBBackupData),
               "Meta save extension overlaps original source copies");
_Static_assert(NDS_META_RECORD_COPY1 + NDS_META_RECORD_BYTES <= NDS_BACKUP_IMAGE_BYTES,
               "Meta save extension exceeds backup storage");

static void ndsMetaVSBackupClear(void)
{
    memset(&sNdsMetaVSRecord, 0, sizeof(sNdsMetaVSRecord));
    memset(&sNdsMetaMirrorRecord, 0, sizeof(sNdsMetaMirrorRecord));
    memset(sNdsLegacyAgainstMeta, 0, sizeof(sNdsLegacyAgainstMeta));
    sNdsMetaVSRecordLoaded = TRUE;
}

static void ndsMetaVSBackupLoad(void)
{
    u8 copy[NDS_META_RECORD_BYTES];
    u8 payload[NDS_META_RECORD_PAYLOAD_BYTES];
    sb32 valid;
    if (sNdsMetaVSRecordLoaded != FALSE) return;
    ndsBackupSramRead(NDS_META_RECORD_COPY0, copy, sizeof(copy));
    valid = ndsMetaRecordDecode(copy, payload);
    if (valid == FALSE)
    {
        ndsBackupSramRead(NDS_META_RECORD_COPY1, copy, sizeof(copy));
        valid = ndsMetaRecordDecode(copy, payload);
    }
    ndsMetaVSBackupClear();
    if (valid != FALSE)
    {
        memcpy(&sNdsMetaVSRecord, payload, sizeof(sNdsMetaVSRecord));
        memcpy(&sNdsMetaMirrorRecord, payload + 92u, sizeof(sNdsMetaMirrorRecord));
        memcpy(sNdsLegacyAgainstMeta, payload + 98u, sizeof(sNdsLegacyAgainstMeta));
    }
    /* Old saves have no recognized extension and migrate to zero Meta-only
     * records. Twelve original source rows retain exactly their old bytes. */
}

static void ndsMetaVSBackupWrite(void)
{
    u8 copy[NDS_META_RECORD_BYTES];
    u8 payload[NDS_META_RECORD_PAYLOAD_BYTES];
    ndsMetaVSBackupLoad();
    memcpy(payload, &sNdsMetaVSRecord, sizeof(sNdsMetaVSRecord));
    memcpy(payload + 92u, &sNdsMetaMirrorRecord, sizeof(sNdsMetaMirrorRecord));
    memcpy(payload + 98u, sNdsLegacyAgainstMeta, sizeof(sNdsLegacyAgainstMeta));
    ndsMetaRecordEncode(copy, payload);
    ndsBackupSramWrite(copy, NDS_META_RECORD_COPY0, sizeof(copy));
    ndsBackupSramWrite(copy, NDS_META_RECORD_COPY1, sizeof(copy));
}

LBBackupVSRecord *ndsMetaVSRecord(s32 kind)
{
    ndsMetaVSBackupLoad();
    if ((u32)kind < NDS_P4_LEGACY_SELECTION_COUNT) return &gSCManagerBackupData.vs_records[kind];
    return (kind == (s32)NDS_P4_RUNTIME_METAKNIGHT) ? &sNdsMetaVSRecord : NULL;
}

static NDSMetaVSRecordPair *ndsMetaVSRecordExtendedPair(s32 kind, s32 opponent)
{
    ndsMetaVSBackupLoad();
    if (kind == (s32)NDS_P4_RUNTIME_METAKNIGHT && opponent == (s32)NDS_P4_RUNTIME_METAKNIGHT)
        return &sNdsMetaMirrorRecord;
    if ((u32)kind < NDS_P4_LEGACY_SELECTION_COUNT && opponent == (s32)NDS_P4_RUNTIME_METAKNIGHT)
        return &sNdsLegacyAgainstMeta[kind];
    return NULL;
}

u16 *ndsMetaVSRecordKO(s32 kind, s32 opponent)
{
    NDSMetaVSRecordPair *pair;
    LBBackupVSRecord *record = ndsMetaVSRecord(kind);
    if (record != NULL && (u32)opponent < NDS_P4_LEGACY_SELECTION_COUNT) return &record->ko_count[opponent];
    pair = ndsMetaVSRecordExtendedPair(kind, opponent);
    return pair != NULL ? &pair->ko : NULL;
}

u16 *ndsMetaVSRecordPlayerTally(s32 kind, s32 opponent)
{
    NDSMetaVSRecordPair *pair;
    LBBackupVSRecord *record = ndsMetaVSRecord(kind);
    if (record != NULL && (u32)opponent < NDS_P4_LEGACY_SELECTION_COUNT) return &record->player_count_tallies[opponent];
    pair = ndsMetaVSRecordExtendedPair(kind, opponent);
    return pair != NULL ? &pair->player_tally : NULL;
}

u16 *ndsMetaVSRecordPlayedAgainst(s32 kind, s32 opponent)
{
    NDSMetaVSRecordPair *pair;
    LBBackupVSRecord *record = ndsMetaVSRecord(kind);
    if (record != NULL && (u32)opponent < NDS_P4_LEGACY_SELECTION_COUNT) return &record->played_against[opponent];
    pair = ndsMetaVSRecordExtendedPair(kind, opponent);
    return pair != NULL ? &pair->played_against : NULL;
}

static sb32 ndsMetaBackupSelectionUnlocked(u32 kind, sb32 allow_meta)
{
    if (allow_meta != FALSE && kind == NDS_P4_RUNTIME_METAKNIGHT) return TRUE;
    return kind < NDS_P4_LEGACY_SELECTION_COUNT &&
        (((u32)gSCManagerBackupData.fighter_mask | LBBACKUP_CHARACTER_MASK_STARTER) & (1u << kind));
}
#endif

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
#if NDS_P4_METAKNIGHT
    ndsMetaVSBackupLoad();
#endif
    gSCManagerBackupData.checksum = lbBackupCreateChecksum(&gSCManagerBackupData);
    ndsBackupSramWrite(&gSCManagerBackupData, NDS_LBBACKUP_COPY0_OFFSET, sizeof(LBBackupData));
    ndsBackupSramWrite(&gSCManagerBackupData, NDS_LBBACKUP_COPY1_OFFSET, sizeof(LBBackupData));
#if NDS_P4_METAKNIGHT
    ndsMetaVSBackupWrite();
#endif
    (void)ndsBackupFlush();
}

/* lbbackup.c:44-63 */
sb32 lbBackupIsSramValid(void)
{
#if NDS_P4_METAKNIGHT
    /* Load own tail before source copy repair calls lbBackupWrite. */
    ndsMetaVSBackupLoad();
#endif
    ndsBackupSramRead(NDS_LBBACKUP_COPY0_OFFSET, &gSCManagerBackupData, sizeof(LBBackupData));
    if (lbBackupIsChecksumValid() == FALSE)
    {
        ndsBackupSramRead(NDS_LBBACKUP_COPY1_OFFSET, &gSCManagerBackupData, sizeof(LBBackupData));
        if (lbBackupIsChecksumValid() == FALSE)
        {
            gSCManagerBackupData = dSCManagerDefaultBackupData;
#if NDS_P4_METAKNIGHT
            ndsMetaVSBackupClear();
#endif
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

#if NDS_P4_METAKNIGHT
    /* The source character-data viewer remains a legacy twelve-entry tier. */
    if (ndsMetaBackupSelectionUnlocked(gSCManagerBackupData.characters_fkind, FALSE) == FALSE)
#else
    if (!((gSCManagerBackupData.fighter_mask | LBBACKUP_CHARACTER_MASK_STARTER) & (1 << gSCManagerBackupData.characters_fkind)))
#endif
    {
        gSCManagerBackupData.characters_fkind = dSCManagerDefaultBackupData.characters_fkind;
    }
#if NDS_P4_METAKNIGHT
    if (ndsMetaBackupSelectionUnlocked(gSCManagerSceneData.fkind, TRUE) == FALSE)
#else
    if (!((gSCManagerBackupData.fighter_mask | LBBACKUP_CHARACTER_MASK_STARTER) & (1 << gSCManagerSceneData.fkind)))
#endif
    {
        gSCManagerSceneData.fkind = nFTKindNull;
    }
#if NDS_P4_METAKNIGHT
    if (ndsMetaBackupSelectionUnlocked(gSCManagerSceneData.training_man_fkind, TRUE) == FALSE)
#else
    if (!((gSCManagerBackupData.fighter_mask | LBBACKUP_CHARACTER_MASK_STARTER) & (1 << gSCManagerSceneData.training_man_fkind)))
#endif
    {
        gSCManagerSceneData.training_man_fkind = nFTKindNull;
    }
#if NDS_P4_METAKNIGHT
    if (ndsMetaBackupSelectionUnlocked(gSCManagerSceneData.training_com_fkind, TRUE) == FALSE)
#else
    if (!((gSCManagerBackupData.fighter_mask | LBBACKUP_CHARACTER_MASK_STARTER) & (1 << gSCManagerSceneData.training_com_fkind)))
#endif
    {
        gSCManagerSceneData.training_com_fkind = nFTKindNull;
    }
    for (i = 0; i < ARRAY_COUNT(gSCManagerTransferBattleState.players); i++)
    {
#if NDS_P4_METAKNIGHT
        if (ndsMetaBackupSelectionUnlocked(gSCManagerTransferBattleState.players[i].fkind, TRUE) == FALSE)
#else
        if (!((1 << gSCManagerTransferBattleState.players[i].fkind) & (gSCManagerBackupData.fighter_mask | LBBACKUP_CHARACTER_MASK_STARTER)))
#endif
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
#if NDS_P4_METAKNIGHT
    ndsMetaVSBackupClear();
#endif
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
#if NDS_P4_METAKNIGHT
    ndsMetaVSBackupClear();
#endif
}
