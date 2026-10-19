/* P7 U2 (docs/P7/P7_Implementation_Plan.md): wide VS records. One 260 B row
 * per selection id (nds_p7.h: the twelve originals, Metal Mario, the
 * contents; 40 rows), the source's LBBackupVSRecord widened to 40 opponents,
 * in a file beside the save that is written through the save's transaction
 * (nds_backup.c: a temporary, the old file kept as .bak through the
 * promotion). A Falco KO counts against Falco, never against Fox. The
 * source's twelve-by-twelve block in LBBackupData stays the source's, for its
 * own readers and Backup Clear (battleship_mnvsresults.c keeps writing it).
 * Row I/O goes through a 1 KB buffer: the 10 KB table is never resident.
 *
 * P7 C3: the 1P records the same way, in a second file: the source's
 * LBBackup1PRecord, one 32 B row per selection id. */
#include <stdio.h>
#include <string.h>
#include <macros.h>
#include <ft/fighter.h>
#include <nds/nds_backup.h>
#include <nds/nds_p7.h>
#include <sc/scene.h>

#define NDS_P7_RECORDS_MAGIC 0x52565037u /* "7PVR" little-endian: P7 VS records */
#define NDS_P7_1P_RECORDS_MAGIC 0x52315037u /* "7P1R": P7 1P records */
#define NDS_P7_RECORDS_VERSION 1u
#define NDS_P7_RECORDS_CHUNK 1024u

typedef struct NDSP7RecordsHeader
{
    u32 magic;
    u16 version;
    u16 row_bytes;
    u16 rows;
    u16 reserved;
    u32 sum;        /* the rows' bytes, summed as u32 words */
} NDSP7RecordsHeader;

_Static_assert(sizeof(NDSP7VSRecord) == NDS_P7_RECORD_BYTES, "VS record row ABI");
_Static_assert(sizeof(LBBackup1PRecord) == NDS_P7_1P_RECORD_BYTES, "1P record row ABI");
_Static_assert(sizeof(NDSP7RecordsHeader) == 16u, "records header ABI");

/* One records file: its sibling, its header's magic and its row size. */
typedef struct NDSP7RecordsTable
{
    u32 sibling;
    u32 magic;
    u32 row_bytes;
} NDSP7RecordsTable;

static const NDSP7RecordsTable kNdsP7VSTable = {
    NDS_BACKUP_SIBLING_RECORDS, NDS_P7_RECORDS_MAGIC, sizeof(NDSP7VSRecord)
};
static const NDSP7RecordsTable kNdsP7OnePTable = {
    NDS_BACKUP_SIBLING_1P_RECORDS, NDS_P7_1P_RECORDS_MAGIC, sizeof(LBBackup1PRecord)
};

static u32 sNdsP7RecordsChunk[NDS_P7_RECORDS_CHUNK / sizeof(u32)];

__attribute__((used)) volatile u32 gNdsP7RecordsWriteCount;
__attribute__((used)) volatile u32 gNdsP7RecordsReadFailCount;
__attribute__((used)) volatile u32 gNdsP7OnePRecordsWriteCount;

/* The file's rows summed as words, streamed through the chunk buffer. FALSE
 * when it is short or its header is not this table's. */
static s32 ndsP7RecordsValid(const NDSP7RecordsTable *table, FILE *file)
{
    NDSP7RecordsHeader head;
    u32 sum = 0u;
    u32 left = NDS_P7_SEL_COUNT * table->row_bytes;

    if ((fseek(file, 0, SEEK_SET) != 0) || (fread(&head, sizeof(head), 1u, file) != 1u) ||
        (head.magic != table->magic) || (head.version != NDS_P7_RECORDS_VERSION) ||
        (head.row_bytes != table->row_bytes) || (head.rows != NDS_P7_SEL_COUNT))
    {
        return FALSE;
    }
    while (left != 0u)
    {
        u32 n = (left < sizeof(sNdsP7RecordsChunk)) ? left : sizeof(sNdsP7RecordsChunk);
        u32 i;

        if (fread(sNdsP7RecordsChunk, 1u, n, file) != n)
        {
            return FALSE;
        }
        for (i = 0u; i < n / sizeof(u32); i++)
        {
            sum += sNdsP7RecordsChunk[i];
        }
        left -= n;
    }
    return (sum == head.sum) ? TRUE : FALSE;
}

/* The table's file on the save's volume (canonical, then its .bak, then its
 * .tmp: the save's recovery order), open and validated; NULL when none is. A
 * temporary that is the only valid copy (an interrupted promotion) is
 * promoted first, since the next write opens that name. Under the lock. */
static FILE *ndsP7RecordsOpen(const NDSP7RecordsTable *table)
{
    u32 i;

    for (i = 0u; i < NDS_BACKUP_RECOVERY_NAMES; i++)
    {
        FILE *file = ndsBackupOpenSibling(table->sibling, i, "rb");

        if (file == NULL)
        {
            continue;
        }
        if (ndsP7RecordsValid(table, file) == FALSE)
        {
            fclose(file);
            continue;
        }
        if (i != (NDS_BACKUP_RECOVERY_NAMES - 1u))
        {
            return file;
        }
        fclose(file);
        if (ndsBackupPromoteSibling(table->sibling) == FALSE)
        {
            return NULL;
        }
        file = ndsBackupOpenSibling(table->sibling, 0u, "rb");
        if ((file != NULL) && (ndsP7RecordsValid(table, file) == FALSE))
        {
            fclose(file);
            file = NULL;
        }
        return file;
    }
    return NULL;
}

/* The source's blank 1P row (scmanager.c dSCManagerDefaultBackupData): both
 * bonus stages' best times an hour, everything else zero. A stored time of 0
 * (a row never written, or written before 2026-10-19 by the zero blank) is
 * that hour: no board is cleared in no time, and the source's record logic
 * writes only a time below the stored one. */
#define NDS_P7_1P_NO_TIME ((u32)I_MIN_TO_TICS(60))

/* Row `sel` of an open, validated file (the blank row without one). */
static void ndsP7RecordsReadFrom(const NDSP7RecordsTable *table, FILE *file, u32 sel,
                                 void *out)
{
    memset(out, 0, table->row_bytes);
    if ((file != NULL) && (sel < NDS_P7_SEL_COUNT) &&
        ((fseek(file, (long)(sizeof(NDSP7RecordsHeader) + (sel * table->row_bytes)),
                SEEK_SET) != 0) ||
         (fread(out, table->row_bytes, 1u, file) != 1u)))
    {
        memset(out, 0, table->row_bytes);
        gNdsP7RecordsReadFailCount++;
    }
    if (table == &kNdsP7OnePTable)
    {
        LBBackup1PRecord *row = (LBBackup1PRecord *)out;

        if (row->bonus1_time == 0u)
        {
            row->bonus1_time = NDS_P7_1P_NO_TIME;
        }
        if (row->bonus2_time == 0u)
        {
            row->bonus2_time = NDS_P7_1P_NO_TIME;
        }
    }
}

static void ndsP7RecordsReadRow(const NDSP7RecordsTable *table, u32 sel, void *out)
{
    FILE *file;

    if (out == NULL)
    {
        return;
    }
    ndsFsLock();
    file = ndsP7RecordsOpen(table);
    ndsP7RecordsReadFrom(table, file, sel, out);
    if (file != NULL)
    {
        fclose(file);
    }
    ndsFsUnlock();
}

void ndsP7RecordsRead(u32 sel, NDSP7VSRecord *out)
{
    ndsP7RecordsReadRow(&kNdsP7VSTable, sel, out);
}

void ndsP7OnePRecordsRead(u32 sel, LBBackup1PRecord *out)
{
    ndsP7RecordsReadRow(&kNdsP7OnePTable, sel, out);
}

void ndsP7RecordsReadRange(u32 first, u32 count, NDSP7VSRecord *rows)
{
    FILE *file;
    u32 i;

    if (rows == NULL)
    {
        return;
    }
    ndsFsLock();
    file = ndsP7RecordsOpen(&kNdsP7VSTable);
    for (i = 0u; i < count; i++)
    {
        ndsP7RecordsReadFrom(&kNdsP7VSTable, file, first + i, &rows[i]);
    }
    if (file != NULL)
    {
        fclose(file);
    }
    ndsFsUnlock();
}

void ndsP7OnePRecordsVisit(u32 first, u32 count,
                           void (*visit)(u32 sel, const LBBackup1PRecord *row, void *ctx),
                           void *ctx)
{
    LBBackup1PRecord row;
    FILE *file;
    u32 sel;

    if (visit == NULL)
    {
        return;
    }
    ndsFsLock();
    file = ndsP7RecordsOpen(&kNdsP7OnePTable);
    for (sel = first; (sel < NDS_P7_SEL_COUNT) && (sel - first < count); sel++)
    {
        ndsP7RecordsReadFrom(&kNdsP7OnePTable, file, sel, &row);
        visit(sel, &row, ctx);
    }
    if (file != NULL)
    {
        fclose(file);
    }
    ndsFsUnlock();
}

/* The new file: the old one's rows (or zero rows) with `count` rows
 * replaced, streamed through the chunk buffer. Two passes over the source: the first sums the
 * new rows for the header. */
typedef struct NDSP7RecordsEdit
{
    const NDSP7RecordsTable *table;
    FILE *src;
    const u8 *sels;
    const void *rows;
    u32 count;
} NDSP7RecordsEdit;

/* The new file's row bytes [at, at + n) into the chunk buffer. */
static s32 ndsP7RecordsFill(const NDSP7RecordsEdit *edit, u32 at, u32 n)
{
    const u32 row_bytes = edit->table->row_bytes;
    u8 *dst = (u8 *)sNdsP7RecordsChunk;
    u32 done = 0u;

    if (edit->src != NULL)
    {
        if ((fseek(edit->src, (long)(sizeof(NDSP7RecordsHeader) + at), SEEK_SET) != 0) ||
            (fread(dst, 1u, n, edit->src) != n))
        {
            return FALSE;
        }
    }
    else
    {
        memset(dst, 0, n);
    }
    while (done < n)
    {
        u32 pos = at + done;
        u32 sel = pos / row_bytes;
        u32 off = pos % row_bytes;
        u32 span = row_bytes - off;
        u32 i;

        if (span > n - done)
        {
            span = n - done;
        }
        for (i = 0u; i < edit->count; i++)
        {
            if (edit->sels[i] == sel)
            {
                memcpy(dst + done, (const u8 *)edit->rows + (i * row_bytes) + off, span);
                break;
            }
        }
        done += span;
    }
    return TRUE;
}

static s32 ndsP7RecordsWrite(FILE *tmp, void *ctx)
{
    const NDSP7RecordsEdit *edit = (const NDSP7RecordsEdit *)ctx;
    const u32 total = NDS_P7_SEL_COUNT * edit->table->row_bytes;
    NDSP7RecordsHeader head;
    u32 at;

    memset(&head, 0, sizeof(head));
    head.magic = edit->table->magic;
    head.version = NDS_P7_RECORDS_VERSION;
    head.row_bytes = (u16)edit->table->row_bytes;
    head.rows = NDS_P7_SEL_COUNT;
    for (at = 0u; at < total; at += sizeof(sNdsP7RecordsChunk))
    {
        u32 n = ((total - at) < sizeof(sNdsP7RecordsChunk)) ? (total - at) : sizeof(sNdsP7RecordsChunk);
        u32 i;

        if (ndsP7RecordsFill(edit, at, n) == FALSE)
        {
            return FALSE;
        }
        for (i = 0u; i < n / sizeof(u32); i++)
        {
            head.sum += sNdsP7RecordsChunk[i];
        }
    }
    if (fwrite(&head, sizeof(head), 1u, tmp) != 1u)
    {
        return FALSE;
    }
    for (at = 0u; at < total; at += sizeof(sNdsP7RecordsChunk))
    {
        u32 n = ((total - at) < sizeof(sNdsP7RecordsChunk)) ? (total - at) : sizeof(sNdsP7RecordsChunk);

        if ((ndsP7RecordsFill(edit, at, n) == FALSE) ||
            (fwrite(sNdsP7RecordsChunk, 1u, n, tmp) != n))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/* `count` rows replaced in one transaction over `src` (the validated file,
 * or NULL), which the transaction closes before its promotion renames it.
 * Under the file-system lock. */
static s32 ndsP7RecordsCommitUnlocked(const NDSP7RecordsTable *table, FILE *src,
                                      const u8 *sels, const void *rows, u32 count)
{
    NDSP7RecordsEdit edit;
    s32 ok;

    edit.table = table;
    edit.src = src;
    edit.sels = sels;
    edit.rows = rows;
    edit.count = count;
    ok = ndsBackupTransactSibling(table->sibling, ndsP7RecordsWrite, &edit, &edit.src);
    if (edit.src != NULL)
    {
        fclose(edit.src);
    }
    return ok;
}

void ndsP7RecordsClear(void)
{
    ndsFsLock();
    if (ndsP7RecordsCommitUnlocked(&kNdsP7VSTable, NULL, NULL, NULL, 0u) != FALSE)
    {
        NDS_DIAG(gNdsP7RecordsWriteCount++);
    }
    ndsFsUnlock();
}

u32 ndsP7SelOf(s32 fkind, u32 content)
{
    if (content != 0u)
    {
        return NDS_P7_SEL_CONTENT(content);
    }
    if (fkind == nFTKindMMario)
    {
        return NDS_P7_SEL_METAL;
    }
    return ((fkind >= 0) && (fkind < GMCOMMON_FIGHTERS_PLAYABLE_NUM)) ?
        (u32)fkind : NDS_P7_SEL_COUNT;
}

static void ndsP7Cap16(u16 *v, u32 add, u32 cap)
{
    u32 sum = (u32)*v + add;

    *v = (u16)((sum > cap) ? cap : sum);
}

static void ndsP7Cap32(u32 *v, u32 add, u32 cap)
{
    u32 sum = *v + add;

    *v = ((sum < *v) || (sum > cap)) ? cap : sum;
}

/* mnVSResultsSaveBackup's arithmetic (mnvsresults.c), over selection ids:
 * every player's row, its opponents by their own ids. */
void ndsP7RecordsAddMatch(const SCBattleState *state, const u8 sel[GMCOMMON_PLAYERS_MAX])
{
    NDSP7VSRecord rows[GMCOMMON_PLAYERS_MAX];
    u8 sels[GMCOMMON_PLAYERS_MAX];
    u32 count = 0u;
    u32 players = 0u;
    FILE *src;
    s32 i;
    s32 j;

    if (state == NULL)
    {
        return;
    }
    for (i = 0; i < GMCOMMON_PLAYERS_MAX; i++)
    {
        if (state->players[i].pkind != nFTPlayerKindNot)
        {
            players++;
        }
    }
    ndsFsLock();
    src = ndsP7RecordsOpen(&kNdsP7VSTable);
    for (i = 0; i < GMCOMMON_PLAYERS_MAX; i++)
    {
        NDSP7VSRecord *row;
        u32 k;

        if ((state->players[i].pkind == nFTPlayerKindNot) || (sel[i] >= NDS_P7_SEL_COUNT))
        {
            continue;
        }
        /* Two players of one fighter add to one row. */
        for (k = 0u; k < count; k++)
        {
            if (sels[k] == sel[i])
            {
                break;
            }
        }
        if (k == count)
        {
            sels[count] = sel[i];
            ndsP7RecordsReadFrom(&kNdsP7VSTable, src, sel[i], &rows[count]);
            count++;
        }
        row = &rows[k];
        ndsP7Cap32(&row->time_used, state->time_passed / UPDATE_INTERVAL, I_MIN_TO_TICS(1000) - 1);
        ndsP7Cap32(&row->damage_given, state->players[i].total_damage_given, 999999u);
        ndsP7Cap32(&row->damage_taken, state->players[i].total_damage_all, 999999u);
        ndsP7Cap16(&row->selfdestructs, state->players[i].total_selfdestructs, 9999u);
        row->games_played++;
        row->player_count_tally += (u16)players;
        for (j = 0; j < GMCOMMON_PLAYERS_MAX; j++)
        {
            if ((i == j) || (state->players[j].pkind == nFTPlayerKindNot) ||
                (sel[j] >= NDS_P7_SEL_COUNT))
            {
                continue;
            }
            ndsP7Cap16(&row->ko_count[sel[j]], state->players[i].total_kos_players[j], 9999u);
            row->player_count_tallies[sel[j]] += (u16)players;
            row->played_against[sel[j]]++;
        }
    }
    if (count != 0u)
    {
        if (ndsP7RecordsCommitUnlocked(&kNdsP7VSTable, src, sels, rows, count) != FALSE)
        {
            NDS_DIAG(gNdsP7RecordsWriteCount++);
        }
    }
    else if (src != NULL)
    {
        fclose(src);
    }
    ndsFsUnlock();
}

/* --- C3: the 1P records ------------------------------------------------ */

/* Backup Clear: the classes' fields reset in every row to the blank row's,
 * as the source's lbBackupClear1PHighScore / lbBackupClearBonusStageTime
 * reset its twelve (zero, the bonus times an hour). One pass reading the old
 * file row by row. */
void ndsP7OnePRecordsClear(u32 fields)
{
    LBBackup1PRecord rows[NDS_P7_SEL_COUNT / 4u];
    u8 sels[NDS_P7_SEL_COUNT / 4u];
    FILE *src;
    u32 base;

    ndsFsLock();
    src = ndsP7RecordsOpen(&kNdsP7OnePTable);
    if (src == NULL)
    {
        ndsFsUnlock();
        return;
    }
    /* Ten rows a transaction keeps the stack small; four transactions. */
    for (base = 0u; (base < NDS_P7_SEL_COUNT) && (src != NULL); base += NDS_P7_SEL_COUNT / 4u)
    {
        u32 i;

        for (i = 0u; i < NDS_P7_SEL_COUNT / 4u; i++)
        {
            LBBackup1PRecord *row = &rows[i];

            sels[i] = (u8)(base + i);
            ndsP7RecordsReadFrom(&kNdsP7OnePTable, src, base + i, row);
            if ((fields & NDS_P7_1P_CLEAR_RUN) != 0u)
            {
                row->spgame_hiscore = 0u;
                row->spgame_continues = 0u;
                row->spgame_total_bonuses = 0u;
                row->spgame_best_difficulty = 0u;
                row->is_spgame_complete = FALSE;
            }
            if ((fields & NDS_P7_1P_CLEAR_BONUS) != 0u)
            {
                row->bonus1_time = NDS_P7_1P_NO_TIME;
                row->bonus1_task_count = 0u;
                row->bonus2_time = NDS_P7_1P_NO_TIME;
                row->bonus2_task_count = 0u;
            }
        }
        if (ndsP7RecordsCommitUnlocked(&kNdsP7OnePTable, src, sels, rows,
                                       NDS_P7_SEL_COUNT / 4u) != FALSE)
        {
            NDS_DIAG(gNdsP7OnePRecordsWriteCount++);
        }
        src = ((base + (NDS_P7_SEL_COUNT / 4u)) < NDS_P7_SEL_COUNT) ?
            ndsP7RecordsOpen(&kNdsP7OnePTable) : NULL;
    }
    if (src != NULL)
    {
        fclose(src);
    }
    ndsFsUnlock();
}

/* The run's swap (nds_p7.h). The source keys every 1P record by
 * gSCManagerSceneData.fkind, which is the parent's kind for a content and
 * Mario's for Metal Mario; while one plays, the parent's row holds the
 * player's own. */
static s32 sNdsP7OnePRunKind = -1;
static u32 sNdsP7OnePRunSel;
static LBBackup1PRecord sNdsP7OnePParentRow;
static LBBackup1PRecord sNdsP7OnePPlayerRow;
static u32 sNdsP7OnePSaveDepth;

void ndsP7OnePRunBegin(s32 parent_kind, u32 sel)
{
    LBBackup1PRecord *row;

    ndsP7OnePRunEnd();
    if ((parent_kind < 0) || (parent_kind >= GMCOMMON_FIGHTERS_PLAYABLE_NUM) ||
        (sel < GMCOMMON_FIGHTERS_PLAYABLE_NUM) || (sel >= NDS_P7_SEL_COUNT))
    {
        return;
    }
    row = &gSCManagerBackupData.spgame_records[parent_kind];
    sNdsP7OnePParentRow = *row;
    ndsP7OnePRecordsRead(sel, row);
    sNdsP7OnePRunKind = parent_kind;
    sNdsP7OnePRunSel = sel;
}

void ndsP7OnePRunEnd(void)
{
    if (sNdsP7OnePRunKind < 0)
    {
        return;
    }
    /* Any change the run made has been saved (ndsP7OnePSaveEnd). */
    gSCManagerBackupData.spgame_records[sNdsP7OnePRunKind] = sNdsP7OnePParentRow;
    sNdsP7OnePRunKind = -1;
}

void ndsP7OnePSaveBegin(void)
{
    LBBackup1PRecord *row;

    if ((sNdsP7OnePRunKind < 0) || (sNdsP7OnePSaveDepth++ != 0u))
    {
        return;
    }
    row = &gSCManagerBackupData.spgame_records[sNdsP7OnePRunKind];
    sNdsP7OnePPlayerRow = *row;
    *row = sNdsP7OnePParentRow;
}

void ndsP7OnePSaveEnd(void)
{
    u8 sel;
    FILE *src;

    if ((sNdsP7OnePRunKind < 0) || (sNdsP7OnePSaveDepth == 0u) ||
        (--sNdsP7OnePSaveDepth != 0u))
    {
        return;
    }
    gSCManagerBackupData.spgame_records[sNdsP7OnePRunKind] = sNdsP7OnePPlayerRow;
    sel = (u8)sNdsP7OnePRunSel;
    ndsFsLock();
    src = ndsP7RecordsOpen(&kNdsP7OnePTable);
    if (ndsP7RecordsCommitUnlocked(&kNdsP7OnePTable, src, &sel, &sNdsP7OnePPlayerRow,
                                   1u) != FALSE)
    {
        NDS_DIAG(gNdsP7OnePRecordsWriteCount++);
    }
    ndsFsUnlock();
}
