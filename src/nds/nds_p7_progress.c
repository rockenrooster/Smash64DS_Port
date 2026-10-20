/* P7 U1 (docs/P7/P7_Implementation_Plan.md): the unlock progress beyond the
 * source's, a small block in the save image's unused tail after the source's
 * two copies of LBBackupData (lbbackup.c writes them at 0 and 1,520, ending
 * at 3,036). Two copies of its own, each signed, versioned and
 * checksummed as the source's are, so a torn or old save falls back the way
 * the source's does: the first valid copy, else the second, else the
 * defaults (nothing new unlocked) -- and the source block is never touched
 * by that. The block lives in the image itself (no static RAM); its writes
 * update both copies, and lbBackupWrite's one flush commits them with the
 * source block, so the two can never disagree on disk. */
#include <string.h>

#include <ssb_types.h>
#include <ft/fighter.h>
#include <sc/scene.h>
#include <nds/nds_backup.h>
#include <nds/nds_p7.h>

#define NDS_P7_SIGNATURE 0x50375042u /* "P7PB" */
#define NDS_P7_VERSION 1u
#define NDS_P7_COPY0 3040u
#define NDS_P7_COPY1 3104u
#define NDS_P7_FLAG_CHEAT 1u

typedef struct NDSP7Progress
{
    u32 signature;
    u16 version;
    u16 size;
    u32 fighters[2];            /* selection ids 0-31, 32-63 */
    u32 stages;                 /* stage unlock ids 0-7 */
    u32 flags;
    u32 reserved[7];
    u32 checksum;
} NDSP7Progress;

_Static_assert(sizeof(NDSP7Progress) <= (NDS_P7_COPY1 - NDS_P7_COPY0),
               "a progress copy must fit its slot");
_Static_assert(NDS_P7_COPY1 + sizeof(NDSP7Progress) <= NDS_BACKUP_IMAGE_BYTES,
               "both progress copies must fit the backup image");
_Static_assert(((sizeof(LBBackupData) + 0xfu) & ~(size_t)0xfu) + sizeof(LBBackupData) <=
                   NDS_P7_COPY0,
               "the progress block follows the source's second copy");

volatile u32 gNdsP7ProgressLoadResult; /* 1 copy 0, 2 copy 1, 3 defaults */

static u32 ndsP7Checksum(const NDSP7Progress *p)
{
    const u8 *bytes = (const u8 *)p;
    u32 sum = 0u;
    u32 i;

    /* The source's weighted byte sum (lbBackupCreateChecksum). */
    for (i = 0u; i < (u32)offsetof(NDSP7Progress, checksum); i++)
    {
        sum += (u32)bytes[i] * (i + 1u);
    }
    return sum;
}

static u32 ndsP7Valid(const NDSP7Progress *p)
{
    return ((p->signature == NDS_P7_SIGNATURE) && (p->version == NDS_P7_VERSION) &&
            (p->size == (u16)sizeof(NDSP7Progress)) && (p->checksum == ndsP7Checksum(p))) ?
        TRUE : FALSE;
}

static void ndsP7Read(NDSP7Progress *p)
{
    ndsBackupSramRead(NDS_P7_COPY0, p, sizeof(*p));
}

/* Both copies, signed and summed, into the image. */
static void ndsP7Write(NDSP7Progress *p)
{
    p->signature = NDS_P7_SIGNATURE;
    p->version = NDS_P7_VERSION;
    p->size = (u16)sizeof(NDSP7Progress);
    p->checksum = ndsP7Checksum(p);
    ndsBackupSramWrite(p, NDS_P7_COPY0, sizeof(*p));
    ndsBackupSramWrite(p, NDS_P7_COPY1, sizeof(*p));
}

void ndsP7ProgressLoad(void)
{
    NDSP7Progress p;

    ndsBackupSramRead(NDS_P7_COPY0, &p, sizeof(p));
    if (ndsP7Valid(&p) != FALSE)
    {
        gNdsP7ProgressLoadResult = 1u;
        /* Copy 1 may be the torn one: rewrite it from copy 0. */
        ndsP7Write(&p);
        return;
    }
    ndsBackupSramRead(NDS_P7_COPY1, &p, sizeof(p));
    if (ndsP7Valid(&p) != FALSE)
    {
        gNdsP7ProgressLoadResult = 2u;
        ndsP7Write(&p);
        return;
    }
    memset(&p, 0, sizeof(p));
    gNdsP7ProgressLoadResult = 3u;
    ndsP7Write(&p);
}

/* By build, not by gNdsSceneHarnessMode: the shipping ROM runs the game's
 * own loop as scene harness 163 (battle_playable_realtime), so the mode
 * cannot tell a probe from a player. The wireless lobby autopilot is one of
 * these: its stress descriptors play contents a fresh save keeps locked, and
 * the character select cleared those picks on the way back from the first
 * match, so the host's START never took (2026-10-20). */
u32 ndsP7DevOpen(void)
{
#if NDS_P2_MENU_WALK || (defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP) || \
    NDS_P2_FOUR_CPU_STRESS || NDS_R2_BOTH_CPU || NDS_HARNESS_FAST_LOGIC || \
    NDS_NET_LAB_LOBBY
    return TRUE;
#else
    return FALSE;
#endif
}

u32 ndsP7FighterUnlocked(u32 sel)
{
    NDSP7Progress p;

    if (sel >= NDS_P7_SEL_COUNT)
    {
        return FALSE;
    }
    if (ndsP7DevOpen() != FALSE)
    {
        return TRUE;
    }
    ndsP7Read(&p);
    return ((p.fighters[sel >> 5] >> (sel & 31u)) & 1u) ? TRUE : FALSE;
}

void ndsP7SetFighterUnlocked(u32 sel)
{
    NDSP7Progress p;

    if (sel >= NDS_P7_SEL_COUNT)
    {
        return;
    }
    ndsP7Read(&p);
    p.fighters[sel >> 5] |= 1u << (sel & 31u);
    ndsP7Write(&p);
}

u32 ndsP7StageUnlocked(u32 stage)
{
    NDSP7Progress p;

    if (stage >= NDS_P7_STAGE_COUNT)
    {
        return FALSE;
    }
    if (ndsP7DevOpen() != FALSE)
    {
        return TRUE;
    }
    ndsP7Read(&p);
    return ((p.stages >> stage) & 1u) ? TRUE : FALSE;
}

void ndsP7SetStageUnlocked(u32 stage)
{
    NDSP7Progress p;

    if (stage >= NDS_P7_STAGE_COUNT)
    {
        return;
    }
    ndsP7Read(&p);
    p.stages |= 1u << stage;
    ndsP7Write(&p);
}

u32 ndsP7Cheated(void)
{
    NDSP7Progress p;

    ndsP7Read(&p);
    return ((p.flags & NDS_P7_FLAG_CHEAT) != 0u) ? TRUE : FALSE;
}

void ndsP7Masks(u32 fighters[2], u32 *stages)
{
    NDSP7Progress p;

    if (ndsP7DevOpen() != FALSE)
    {
        fighters[0] = 0xFFFFFFFFu;
        fighters[1] = 0xFFFFFFFFu;
        *stages = 0xFFFFFFFFu;
        return;
    }
    ndsP7Read(&p);
    fighters[0] = p.fighters[0];
    fighters[1] = p.fighters[1];
    *stages = p.stages;
}

u32 ndsP7UnlockAll(void)
{
    NDSP7Progress p;
    u32 changed = FALSE;
    u32 fighters0 = 0xFFFFFFFFu;
    u32 fighters1 = (1u << (NDS_P7_SEL_COUNT - 32u)) - 1u;
    u32 stages = (1u << NDS_P7_STAGE_COUNT) - 1u;

    ndsP7Read(&p);
    if ((p.fighters[0] != fighters0) || (p.fighters[1] != fighters1) || (p.stages != stages))
    {
        changed = TRUE;
    }
    if (((gSCManagerBackupData.unlock_mask & LBBACKUP_UNLOCK_MASK_ALL) != LBBACKUP_UNLOCK_MASK_ALL) ||
        (gSCManagerBackupData.fighter_mask != (u16)LBBACKUP_CHARACTER_MASK_ALL) ||
        ((gSCManagerBackupData.ground_mask & LBBACKUP_GROUND_MASK_ALL) != LBBACKUP_GROUND_MASK_ALL))
    {
        changed = TRUE;
    }
    gSCManagerBackupData.unlock_mask |= LBBACKUP_UNLOCK_MASK_ALL;
    gSCManagerBackupData.fighter_mask = (u16)LBBACKUP_CHARACTER_MASK_ALL;
    gSCManagerBackupData.ground_mask |= LBBACKUP_GROUND_MASK_ALL;
    p.fighters[0] = fighters0;
    p.fighters[1] = fighters1;
    p.stages = stages;
    p.flags |= NDS_P7_FLAG_CHEAT;
    ndsP7Write(&p);
    return changed;
}

void ndsP7ClearAll(void)
{
    NDSP7Progress p;

    memset(&p, 0, sizeof(p));
    ndsP7Write(&p);
}

void ndsP7ClearFighters(void)
{
    NDSP7Progress p;

    ndsP7Read(&p);
    p.fighters[0] = 0u;
    p.fighters[1] = 0u;
    ndsP7Write(&p);
}

void ndsP7ClearStages(void)
{
    NDSP7Progress p;

    ndsP7Read(&p);
    p.stages = 0u;
    ndsP7Write(&p);
}
