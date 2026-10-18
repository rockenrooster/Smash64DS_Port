/* P7 U2 (docs/P7/P7_Implementation_Plan.md): wide VS records. One 260 B row
 * per selection id (nds_p7.h: the twelve originals, Metal Mario, the
 * contents; 40 rows), the source's LBBackupVSRecord widened to 40 opponents,
 * in a file beside the save that is written through the save's transaction
 * (nds_backup.c: a temporary, the old file kept as .bak through the
 * promotion). A Falco KO counts against Falco, never against Fox. The
 * source's twelve-by-twelve block in LBBackupData stays the source's, for its
 * own readers and Backup Clear (battleship_mnvsresults.c keeps writing it).
 * Row I/O goes through a 1 KB buffer: the 10 KB table is never resident. */
#include <stdio.h>
#include <string.h>
#include <macros.h>
#include <ft/fighter.h>
#include <nds/nds_backup.h>
#include <nds/nds_p7.h>
#include <sc/scene.h>

#define NDS_P7_RECORDS_MAGIC 0x52565037u /* "7PVR" little-endian: P7 VS records */
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
_Static_assert(sizeof(NDSP7RecordsHeader) == 16u, "VS records header ABI");

#define NDS_P7_RECORDS_FILE_BYTES \
    (sizeof(NDSP7RecordsHeader) + (NDS_P7_SEL_COUNT * sizeof(NDSP7VSRecord)))

static u32 sNdsP7RecordsChunk[NDS_P7_RECORDS_CHUNK / sizeof(u32)];

__attribute__((used)) volatile u32 gNdsP7RecordsWriteCount;
__attribute__((used)) volatile u32 gNdsP7RecordsReadFailCount;

/* The file's rows summed as words, streamed through the chunk buffer. FALSE
 * when it is short or its header is not this format's. */
static s32 ndsP7RecordsValid(FILE *file)
{
    NDSP7RecordsHeader head;
    u32 sum = 0u;
    u32 left = NDS_P7_SEL_COUNT * sizeof(NDSP7VSRecord);

    if ((fseek(file, 0, SEEK_SET) != 0) || (fread(&head, sizeof(head), 1u, file) != 1u) ||
        (head.magic != NDS_P7_RECORDS_MAGIC) || (head.version != NDS_P7_RECORDS_VERSION) ||
        (head.row_bytes != sizeof(NDSP7VSRecord)) || (head.rows != NDS_P7_SEL_COUNT))
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

/* The records file the save's volume holds (canonical, then its .bak, then
 * its .tmp: the save's recovery order), open and validated; NULL when none
 * is. A temporary that is the only valid copy (an interrupted promotion) is
 * promoted first, since the next write opens that name. Under the lock. */
static FILE *ndsP7RecordsOpen(void)
{
    u32 i;

    for (i = 0u; i < NDS_BACKUP_RECOVERY_NAMES; i++)
    {
        FILE *file = ndsBackupOpenSibling(NDS_BACKUP_SIBLING_RECORDS, i, "rb");

        if (file == NULL)
        {
            continue;
        }
        if (ndsP7RecordsValid(file) == FALSE)
        {
            fclose(file);
            continue;
        }
        if (i != (NDS_BACKUP_RECOVERY_NAMES - 1u))
        {
            return file;
        }
        fclose(file);
        if (ndsBackupPromoteSibling(NDS_BACKUP_SIBLING_RECORDS) == FALSE)
        {
            return NULL;
        }
        file = ndsBackupOpenSibling(NDS_BACKUP_SIBLING_RECORDS, 0u, "rb");
        if ((file != NULL) && (ndsP7RecordsValid(file) == FALSE))
        {
            fclose(file);
            file = NULL;
        }
        return file;
    }
    return NULL;
}

/* Row `sel` of an open, validated file (zero without one). */
static void ndsP7RecordsReadFrom(FILE *file, u32 sel, NDSP7VSRecord *out)
{
    memset(out, 0, sizeof(*out));
    if ((file == NULL) || (sel >= NDS_P7_SEL_COUNT))
    {
        return;
    }
    if ((fseek(file, (long)(sizeof(NDSP7RecordsHeader) + (sel * sizeof(NDSP7VSRecord))),
               SEEK_SET) != 0) ||
        (fread(out, sizeof(*out), 1u, file) != 1u))
    {
        memset(out, 0, sizeof(*out));
        gNdsP7RecordsReadFailCount++;
    }
}

void ndsP7RecordsRead(u32 sel, NDSP7VSRecord *out)
{
    FILE *file;

    if (out == NULL)
    {
        return;
    }
    ndsFsLock();
    file = ndsP7RecordsOpen();
    ndsP7RecordsReadFrom(file, sel, out);
    if (file != NULL)
    {
        fclose(file);
    }
    ndsFsUnlock();
}

/* The new file: the old one's rows (or zero rows) with `count` rows
 * replaced, streamed through the chunk buffer. Two passes over the source:
 * the first sums the new rows for the header. */
typedef struct NDSP7RecordsEdit
{
    FILE *src;
    const u8 *sels;
    const NDSP7VSRecord *rows;
    u32 count;
} NDSP7RecordsEdit;

/* Row `sel`'s bytes [off, off + n) of the new file into the chunk buffer. */
static s32 ndsP7RecordsFill(const NDSP7RecordsEdit *edit, u32 at, u32 n)
{
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
        u32 sel = pos / sizeof(NDSP7VSRecord);
        u32 off = pos % sizeof(NDSP7VSRecord);
        u32 span = sizeof(NDSP7VSRecord) - off;
        u32 i;

        if (span > n - done)
        {
            span = n - done;
        }
        for (i = 0u; i < edit->count; i++)
        {
            if (edit->sels[i] == sel)
            {
                memcpy(dst + done, (const u8 *)&edit->rows[i] + off, span);
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
    const u32 total = NDS_P7_SEL_COUNT * sizeof(NDSP7VSRecord);
    NDSP7RecordsHeader head;
    u32 at;

    memset(&head, 0, sizeof(head));
    head.magic = NDS_P7_RECORDS_MAGIC;
    head.version = NDS_P7_RECORDS_VERSION;
    head.row_bytes = sizeof(NDSP7VSRecord);
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
static void ndsP7RecordsCommitUnlocked(FILE *src, const u8 *sels,
                                       const NDSP7VSRecord *rows, u32 count)
{
    NDSP7RecordsEdit edit;

    edit.src = src;
    edit.sels = sels;
    edit.rows = rows;
    edit.count = count;
    if (ndsBackupTransactSibling(NDS_BACKUP_SIBLING_RECORDS, ndsP7RecordsWrite, &edit,
                                 &edit.src) != FALSE)
    {
        NDS_DIAG(gNdsP7RecordsWriteCount++);
    }
    if (edit.src != NULL)
    {
        fclose(edit.src);
    }
}

void ndsP7RecordsClear(void)
{
    ndsFsLock();
    ndsP7RecordsCommitUnlocked(NULL, NULL, NULL, 0u);
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
    src = ndsP7RecordsOpen();
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
            ndsP7RecordsReadFrom(src, sel[i], &rows[count]);
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
        ndsP7RecordsCommitUnlocked(src, sels, rows, count);
    }
    else if (src != NULL)
    {
        fclose(src);
    }
    ndsFsUnlock();
}
