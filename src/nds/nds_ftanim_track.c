#include <nds/nds_ftanim_track.h>

/* `used` on every counter: `--gc-sections` has removed diagnostic globals here
 * before and Boundary reported "Missing ELF symbol" rather than a flag. */
__attribute__((used)) volatile u32 gNdsFtAnimTrackDispatch =
    NDS_R2_FTANIM_TRACK_DISPATCH;
__attribute__((used)) volatile u32 gNdsFtAnimTrackBinds;
__attribute__((used)) volatile u32 gNdsFtAnimTrackBindMiss;
__attribute__((used)) volatile u32 gNdsFtAnimTrackBindFull;
__attribute__((used)) volatile u32 gNdsFtAnimTrackSteps;
__attribute__((used)) volatile u32 gNdsFtAnimTrackEarlyOut;
__attribute__((used)) volatile u32 gNdsFtAnimTrackRowsRun;
__attribute__((used)) volatile u32 gNdsFtAnimTrackOracleRows;
__attribute__((used)) volatile u32 gNdsFtAnimTrackOracleBad;
__attribute__((used)) volatile u32 gNdsFtAnimTrackOracleFirst;

#if NDS_R2_FTANIM_TRACK && NDS_R2_CUBIC_FIXED

#include <ft/fighter.h>
#include <sys/objman.h>
#include <nds/nds_fcmp.h>
#include <nds/nds_anim_fixed.h>
#include <nds/nds_startup.h>
#include <nds/nds_battlepack_anim.h>

/* `battleship_ftanim.c`'s own runaway budget, which lives in the overlay copy
 * of the decomp file and is therefore not in a shared header. Kept equal on
 * purpose: the two parsers must give a malformed script the same rope. */
#define NDS_FTANIM_EVENT_LIMIT 4096u

/* Exported out of `src/import/battleship_ftanim.c`, so the two paths share ONE
 * body rather than two that can drift: the Q migration the bind now performs
 * once per clip instead of once per stepped call, and the script-exhausted tail
 * that `End` runs over the whole AObj list. */
void ndsR2FtAnimAObjToQ(AObj *a);
void ndsR2FtAnimAdvanceTailQ(DObj *root_dobj);

#include <nds/generated/nds_ftanim_track_pack.generated.h>

/* Row kinds, generator `K_*`. Nine, because the fifteen opcodes collapse once
 * each `*Block` half folds into the header's block bit. */
#define TRK_END     0u
#define TRK_BLOCK   1u
#define TRK_LINEAR  2u
#define TRK_CUBIC2  3u
#define TRK_RATE    4u
#define TRK_CUBIC0  5u
#define TRK_STEP    6u
#define TRK_ADDLEN  7u
#define TRK_LOOP    8u

#define TRK_HDR_FRAMES (1u << 14)
#define TRK_HDR_BLOCK  (1u << 15)

/* `NDS_R2_AQ_VF - sNdsR2AnimFracShift[id]` (`battleship_ftanim.c:187`) with the
 * id folded through the decomp's own track class. Bit 3 is TraI, whose
 * 1/16384-3e-12 is not a power of two -- the generator REFUSES any clip that
 * uses it, so these two entries are unreachable and pinned to 0. */
static const s8 sTrkShiftVal[10] = { 3, 3, 3, 0, 10, 10, 10, 0, 0, 0 };
static const s8 sTrkShiftRate[10] = { 3, 3, 3, 0, 7, 7, 7, -1, -1, -1 };

#define NDS_FTANIM_TRACK_FIGHTERS 2
#define NDS_FTANIM_TRACK_JOINTS ((s32)nFTPartsJointNumMax)
#define NDS_FTANIM_TRACK_BLOCKS \
    (NDS_FTANIM_TRACK_FIGHTERS * NDS_FTANIM_TRACK_JOINTS)

/* The whole mutable state a converted joint owns: a cursor into the immutable
 * row stream and the ten AObj slots the bind resolved once. No `next`, no
 * `track` byte, no per-call list walk, no per-call Q migration.
 *
 * Sized for the P1 matchup on purpose (`RAM is not free -- the GObj cap`): two
 * fighters at `nFTPartsJointNumMax` is 74 blocks. A third root fails OPEN --
 * `gNdsFtAnimTrackBindFull` counts it and those joints keep the generic parser,
 * which is the shipped behaviour and not a degradation. */
typedef struct NDSFtAnimTrackJoint
{
    const u16 *cursor;
    AObj *slot[10];
#if NDS_R2_FTANIM_TRACK_ORACLE
    /* The o2r script the generic parser would be walking. Stage 4 compares
     * DECISION POINTS against it -- see `ndsFtAnimTrackOracle`. */
    const AObjEvent16 *ref;
#endif
    u16 mask;
    u16 pad;
} NDSFtAnimTrackJoint;

static NDSFtAnimTrackJoint sTrkBlocks[NDS_FTANIM_TRACK_BLOCKS];
static DObj *sTrkOwner[NDS_FTANIM_TRACK_FIGHTERS];
static const NDSFtAnimTrackClip *sTrkOpen[NDS_FTANIM_TRACK_FIGHTERS];

/* `ndsR2U16ToF32`, the parser's own exact u16 -> f32 (a 16-bit integer has at
 * most 16 significant bits against f32's 24, so there is no rounding decision).
 * Duplicated rather than exported because it is a pure bit formula and the call
 * would cost more than the body; `battleship_ftanim.c:156` is the original. */
static inline f32 ndsTrkU16ToF32(u32 v)
{
    u32 shift;
    f32 out;
    u32 bits;

    if (v == 0u)
    {
        return 0.0f;
    }
    shift = (u32)__builtin_clz(v);
    bits = (((127u + 31u) - shift) << 23) | (((v << shift) & 0x7fffffffu) >> 8);
    __builtin_memcpy(&out, &bits, sizeof(out));
    return out;
}

static const NDSFtAnimTrackClip *ndsTrkFindClip(u32 asset_id)
{
    u32 lo = 0u;
    u32 hi = NDS_FTANIM_TRACK_PACK_CLIPS;

    if ((asset_id < NDS_FTANIM_TRACK_PACK_FIRST_ID) ||
        (asset_id > NDS_FTANIM_TRACK_PACK_LAST_ID))
    {
        return NULL;
    }
    while (lo < hi)
    {
        u32 mid = lo + ((hi - lo) >> 1);
        u32 got = sNdsFtAnimTrackClips[mid].asset_id;

        if (got == asset_id)
        {
            return &sNdsFtAnimTrackClips[mid];
        }
        if (got < asset_id)
        {
            lo = mid + 1u;
        }
        else
        {
            hi = mid;
        }
    }
    return NULL;
}

s32 ndsFtAnimTrackIsDense(const void *p)
{
    return ((p >= (const void *)&sTrkBlocks[0]) &&
            (p < (const void *)&sTrkBlocks[NDS_FTANIM_TRACK_BLOCKS])) ?
        TRUE : FALSE;
}

s32 ndsFtAnimTrackBeginClip(DObj *root_dobj, const void *figatree)
{
    const NDSFtAnimTrackClip *clip;
    s32 asset_id;
    s32 i;
    s32 free_slot = -1;

    if ((gNdsFtAnimTrackDispatch == 0u) || (root_dobj == NULL) ||
        (figatree == NULL))
    {
        return -1;
    }
    /* The bind site has a POINTER, never an id. The resident BattlePack knows
     * which clip a slot table belongs to, so the reverse lookup lives there
     * rather than in a second address map that could disagree with it. */
    asset_id = ndsBattlePackAssetIdForSlotTable(figatree);
    if (asset_id < 0)
    {
        gNdsFtAnimTrackBindMiss++;
        return -1;
    }
    clip = ndsTrkFindClip((u32)asset_id);
    if (clip == NULL)
    {
        gNdsFtAnimTrackBindMiss++;
        return -1;
    }
    for (i = 0; i < NDS_FTANIM_TRACK_FIGHTERS; i++)
    {
        if (sTrkOwner[i] == root_dobj)
        {
            sTrkOpen[i] = clip;
            return i;
        }
        if ((free_slot < 0) && (sTrkOwner[i] == NULL))
        {
            free_slot = i;
        }
    }
    if (free_slot < 0)
    {
        gNdsFtAnimTrackBindFull++;
        return -1;
    }
    sTrkOwner[free_slot] = root_dobj;
    sTrkOpen[free_slot] = clip;
    return free_slot;
}

s32 ndsFtAnimTrackBindJoint(DObj *dobj, s32 base, s32 index)
{
    const NDSFtAnimTrackClip *clip;
    NDSFtAnimTrackJoint *blk;
    AObj *a;
    u32 word;
    u32 mask;
    u32 bit;

    if ((base < 0) || (dobj == NULL) || (index >= NDS_FTANIM_TRACK_JOINTS))
    {
        return FALSE;
    }
    clip = sTrkOpen[base];
    if ((clip == NULL) || (index >= (s32)clip->entry_count))
    {
        return FALSE;
    }
    word = sNdsFtAnimTrackEntries[clip->entry_first + (u32)index];
    if (word == 0xFFFFFFFFu)
    {
        return FALSE;       /* NULL figatree entry: joint has no script */
    }
    blk = &sTrkBlocks[(base * NDS_FTANIM_TRACK_JOINTS) + index];
    blk->cursor = &sNdsFtAnimTrackRows[word >> 10];
    mask = word & 0x3FFu;
    blk->mask = (u16)mask;
#if NDS_R2_FTANIM_TRACK_ORACLE
    blk->ref = dobj->anim_joint.event16;
#endif
    /* ONE list walk per bind, where the shipped parser does one per stepped
     * call: resolve every track the script can write, create the ones the DObj
     * does not have yet, and migrate each into the Q representation. */
    for (bit = 0u; bit < 10u; bit++)
    {
        blk->slot[bit] = NULL;
    }
    a = dobj->aobj;
    while (a != NULL)
    {
        s32 t = (s32)a->track - (s32)nGCAnimTrackJointStart;

        if ((t >= 0) && (t < 10))
        {
            blk->slot[t] = a;
        }
        a = a->next;
    }
    for (bit = 0u; bit < 10u; bit++)
    {
        if ((mask & (1u << bit)) == 0u)
        {
            continue;
        }
        if (blk->slot[bit] == NULL)
        {
            blk->slot[bit] = gcAddAObjForDObj(
                dobj, (s32)bit + (s32)nGCAnimTrackJointStart);
        }
        if (blk->slot[bit] == NULL)
        {
            return FALSE;   /* out of AObjs: leave the joint on the generic path */
        }
        ndsR2FtAnimAObjToQ(blk->slot[bit]);
    }
    dobj->anim_joint.event16 = (AObjEvent16 *)(void *)blk;
    gNdsFtAnimTrackBinds++;
    return TRUE;
}

#if NDS_R2_FTANIM_TRACK_ORACLE
/* STAGE 4 -- the oracle, and the trap it exists to avoid.
 *
 * `func_anim` has NO writer anywhere in `decomp/src` or `src/` (only `= NULL`
 * at `objman.c:1717`), so the -1/-2 callbacks are INERT and an oracle that
 * compares OBSERVED callbacks is a control that cannot fail. This compares
 * DECISION POINTS instead: for every converted logical update, the command the
 * generic parser would have dispatched -- its opcode class, its block bit, its
 * flag mask, its payload and every per-track target word, in order -- against
 * the row the dense stepper actually consumed.
 *
 * The reference cursor is advanced by the parser's own rules (including Loop's
 * `event16 += event16->s / 2`), so a wrong jump does not merely mis-compare one
 * row: it desynchronises the reference and every later row fails.
 *
 * Fail-closed: the first mismatch clears the route word, so every later bind
 * takes the generic parser. */
static void ndsTrkOracleFail(u32 why, u32 kind, u32 op)
{
    gNdsFtAnimTrackOracleBad++;
    if (gNdsFtAnimTrackOracleFirst == 0u)
    {
        gNdsFtAnimTrackOracleFirst = (why << 16) | (kind << 8) | op;
    }
    gNdsFtAnimTrackDispatch = 0u;
}

/* generator `OP_TO_KIND`, as a table so the two cannot drift silently. 0xFF is
 * an opcode a track row cannot represent (12 SetTranslateInterp, 14 SetFlags);
 * the generator refuses any clip containing one, so seeing it here is a fault. */
static const u8 sTrkOpKind[16] = {
    TRK_END, TRK_BLOCK, TRK_LINEAR, TRK_LINEAR, TRK_CUBIC2, TRK_CUBIC2,
    TRK_RATE, TRK_CUBIC0, TRK_CUBIC0, TRK_STEP, TRK_STEP, TRK_ADDLEN,
    0xFFu, TRK_LOOP, 0xFFu, 0xFFu
};

static void ndsFtAnimTrackOracle(NDSFtAnimTrackJoint *blk, const u16 *row)
{
    const AObjEvent16 *e = blk->ref;
    u32 hdr = *row;
    u32 kind = hdr & 0xFu;
    u32 mask = (hdr >> 4) & 0x3FFu;
    const u16 *p = row + 1;
    u32 frames = 0u;
    u32 op;
    u32 flags;
    u32 toggle;
    u32 payload = 0u;
    u32 per;
    u32 bit;

    gNdsFtAnimTrackOracleRows++;
    if (e == NULL)
    {
        ndsTrkOracleFail(1u, kind, 0u);
        return;
    }
    if ((hdr & TRK_HDR_FRAMES) != 0u)
    {
        frames = *p++;
    }
    op = (u32)e->command.opcode;
    flags = (u32)e->command.flags;
    toggle = (u32)e->command.toggle;
    if ((op > 15u) || (sTrkOpKind[op] != (u8)kind))
    {
        ndsTrkOracleFail(2u, kind, op);
        return;
    }
    if (kind == TRK_END)
    {
        blk->ref = NULL;                    /* the script stops here */
        return;
    }
    if (kind == TRK_LOOP)
    {
        const AObjEvent16 *jump = e + 1;

        blk->ref = jump + (jump->s / 2);
        return;
    }
    e++;
    if (toggle != 0u)
    {
        payload = (u32)e->u;
        e++;
    }
    if (((hdr & TRK_HDR_FRAMES) != 0u) != (toggle != 0u))
    {
        ndsTrkOracleFail(3u, kind, op);
        return;
    }
    if (payload != frames)
    {
        ndsTrkOracleFail(4u, kind, op);
        return;
    }
    if (flags != mask)
    {
        ndsTrkOracleFail(5u, kind, op);
        return;
    }
    if ((((hdr & TRK_HDR_BLOCK) != 0u) ? 1u : 0u) !=
        (((op == 2u) || (op == 4u) || (op == 7u) || (op == 9u)) ? 1u : 0u))
    {
        ndsTrkOracleFail(6u, kind, op);
        return;
    }
    per = (kind == TRK_CUBIC2) ? 2u :
        (((kind == TRK_LINEAR) || (kind == TRK_RATE) || (kind == TRK_CUBIC0) ||
          (kind == TRK_STEP)) ? 1u : 0u);
    for (bit = 0u; bit < 10u; bit++)
    {
        u32 k;

        if ((flags & (1u << bit)) == 0u)
        {
            continue;
        }
        for (k = 0u; k < per; k++)
        {
            if ((s32)e->s != (s32)*(const s16 *)p)
            {
                ndsTrkOracleFail(7u, kind, op);
                return;
            }
            e++;
            p++;
        }
    }
    blk->ref = e;
}
#endif /* NDS_R2_FTANIM_TRACK_ORACLE */

void ndsFtAnimTrackStep(DObj *root_dobj)
{
    NDSFtAnimTrackJoint *blk =
        (NDSFtAnimTrackJoint *)(void *)root_dobj->anim_joint.event16;
    u32 events = 0u;

    if (NDS_FCMP_EQ_C(root_dobj->anim_wait, AOBJ_ANIM_NULL))
    {
        return;
    }
    if (NDS_FCMP_EQ_C(root_dobj->anim_wait, AOBJ_ANIM_CHANGED))
    {
        root_dobj->anim_wait = -root_dobj->anim_frame;
    }
    else
    {
        root_dobj->anim_wait -= root_dobj->anim_speed;
        root_dobj->anim_frame += root_dobj->anim_speed;
        root_dobj->parent_gobj->anim_frame = root_dobj->anim_frame;

        if (NDS_FCMP_GT0(root_dobj->anim_wait))
        {
            gNdsFtAnimTrackEarlyOut++;
            return;
        }
    }
    gNdsFtAnimTrackSteps++;
    do
    {
        const u16 *row = blk->cursor;
        u32 hdr = *row;
        u32 kind = hdr & 0xFu;
        u32 mask = (hdr >> 4) & 0x3FFu;
        const u16 *p = row + 1;
        u32 frames = 0u;
        s32 seg = 0;
        u32 bit;

        gNdsFtAnimTrackRowsRun++;
#if NDS_R2_FTANIM_TRACK_ORACLE
        ndsFtAnimTrackOracle(blk, row);
#endif
        if ((hdr & TRK_HDR_FRAMES) != 0u)
        {
            frames = *p++;
        }
        if (kind == TRK_END)
        {
            ndsR2FtAnimAdvanceTailQ(root_dobj);
            root_dobj->anim_frame = root_dobj->anim_wait;
            root_dobj->parent_gobj->anim_frame = root_dobj->anim_wait;
            root_dobj->anim_wait = AOBJ_ANIM_END;

            if (root_dobj->is_anim_root != FALSE)
            {
                if (root_dobj->parent_gobj->func_anim != NULL)
                {
                    root_dobj->parent_gobj->func_anim(root_dobj, -1, 0);
                }
            }
            return;
        }
        if (kind == TRK_LOOP)
        {
            blk->cursor = (const u16 *)(const void *)
                ((const u8 *)(const void *)row + (s32)*(const s16 *)p);
            root_dobj->anim_frame = -root_dobj->anim_wait;
            root_dobj->parent_gobj->anim_frame = -root_dobj->anim_wait;

            if (root_dobj->is_anim_root != FALSE)
            {
                if (root_dobj->parent_gobj->func_anim != NULL)
                {
                    root_dobj->parent_gobj->func_anim(root_dobj, -2, 0);
                }
            }
        }
        else
        {
            if (kind > TRK_LOOP)
            {
                gNdsObjAnimRunawayCount++;
                gNdsObjAnimRunawayMask |= 1u << 8u;
                gNdsObjAnimRunawayScript = (u32)(uintptr_t)row;
                gNdsObjAnimRunawayOpcode = (u16)kind;
                root_dobj->anim_wait = AOBJ_ANIM_NULL;
                return;
            }
            if ((kind != TRK_BLOCK) && (kind != TRK_RATE) &&
                (kind != TRK_ADDLEN))
            {
                /* The segment's starting phase, once per row and never per
                 * track -- and per ROW, which is what the decomp does in each
                 * of its four write cases (`ft/ftanim.c:158/193/232/293`). */
                seg = ndsR2F32ToFixed(
                    -root_dobj->anim_wait - root_dobj->anim_speed,
                    NDS_R2_AQ_LF);
            }
            for (bit = 0u; mask != 0u; bit++, mask >>= 1)
            {
                AObj *a;
                s32 v;

                if ((mask & 1u) == 0u)
                {
                    continue;
                }
                a = blk->slot[bit];
                if (a == NULL)
                {
                    continue;
                }
                switch (kind)
                {
                case TRK_LINEAR:
                    v = ndsR2AnimArgToQ((s32)*(const s16 *)p++,
                                        (s32)sTrkShiftVal[bit]);
                    if (frames != 0u)
                    {
                        s32 d = (v - ndsR2AQLoad(a->value_target)) <<
                            (NDS_R2_AQ_RF - NDS_R2_AQ_VF);
                        u32 h = frames >> 1;
                        s32 r = (d < 0) ?
                            -(s32)(((u32)(-d) + h) / frames) :
                            (s32)(((u32)d + h) / frames);

                        a->rate_base = ndsR2AQStore(r);
                    }
                    a->value_base = a->value_target;
                    a->value_target = ndsR2AQStore(v);
                    a->kind = (u8)NDS_R2_AQ_KIND_LINEAR;
                    a->length = ndsR2AQStore(seg);
                    a->rate_target = ndsR2AQStore(0);
                    break;

                case TRK_CUBIC2:
                    v = ndsR2AnimArgToQ((s32)*(const s16 *)p++,
                                        (s32)sTrkShiftVal[bit]);
                    a->value_base = a->value_target;
                    a->value_target = ndsR2AQStore(v);
                    a->rate_base = a->rate_target;
                    a->rate_target = ndsR2AQStore(
                        ndsR2AnimArgToQ((s32)*(const s16 *)p++,
                                        (s32)sTrkShiftRate[bit]));
                    a->kind = (u8)NDS_R2_AQ_KIND_CUBIC;
                    if (frames != 0u)
                    {
                        a->length_invert =
                            ndsR2AQStore((s32)sNdsFtAnimTrackRecipQ[frames]);
                    }
                    a->length = ndsR2AQStore(seg);
                    break;

                case TRK_CUBIC0:
                    v = ndsR2AnimArgToQ((s32)*(const s16 *)p++,
                                        (s32)sTrkShiftVal[bit]);
                    a->value_base = a->value_target;
                    a->value_target = ndsR2AQStore(v);
                    a->rate_base = a->rate_target;
                    a->rate_target = ndsR2AQStore(0);
                    a->kind = (u8)NDS_R2_AQ_KIND_CUBIC;
                    if (frames != 0u)
                    {
                        a->length_invert =
                            ndsR2AQStore((s32)sNdsFtAnimTrackRecipQ[frames]);
                    }
                    a->length = ndsR2AQStore(seg);
                    break;

                case TRK_STEP:
                    v = ndsR2AnimArgToQ((s32)*(const s16 *)p++,
                                        (s32)sTrkShiftVal[bit]);
                    a->value_base = a->value_target;
                    a->value_target = ndsR2AQStore(v);
                    a->kind = (u8)NDS_R2_AQ_KIND_STEP;
                    /* Step's `length_invert` carries a FRAME COUNT, not a
                     * reciprocal -- the original's own double meaning -- and it
                     * is written unconditionally. */
                    a->length_invert =
                        ndsR2AQStore((s32)frames << NDS_R2_AQ_LF);
                    a->length = ndsR2AQStore(seg);
                    a->rate_target = ndsR2AQStore(0);
                    break;

                case TRK_RATE:
                    a->rate_target = ndsR2AQStore(
                        ndsR2AnimArgToQ((s32)*(const s16 *)p++,
                                        (s32)sTrkShiftRate[bit]));
                    break;

                case TRK_ADDLEN:
                    a->length = ndsR2AQStore(ndsR2AQLoad(a->length) +
                                             ((s32)frames << NDS_R2_AQ_LF));
                    break;

                default:
                    break;
                }
            }
            if (((hdr & TRK_HDR_BLOCK) != 0u) || (kind == TRK_BLOCK))
            {
                root_dobj->anim_wait += ndsTrkU16ToF32(frames);
            }
            blk->cursor = p;
        }
        if (++events >= NDS_FTANIM_EVENT_LIMIT)
        {
            gNdsObjAnimRunawayCount++;
            gNdsObjAnimRunawayMask |= 1u << 9u;
            gNdsObjAnimRunawayScript = (u32)(uintptr_t)blk->cursor;
            gNdsObjAnimRunawayOpcode = 0u;
            root_dobj->anim_wait = AOBJ_ANIM_NULL;
            return;
        }
    }
    while (NDS_FCMP_LE0(root_dobj->anim_wait));
}

#else /* !(NDS_R2_FTANIM_TRACK && NDS_R2_CUBIC_FIXED) */

s32 ndsFtAnimTrackBeginClip(DObj *root_dobj, const void *figatree)
{
    (void)root_dobj;
    (void)figatree;
    return -1;
}

s32 ndsFtAnimTrackBindJoint(DObj *dobj, s32 base, s32 index)
{
    (void)dobj;
    (void)base;
    (void)index;
    return FALSE;
}

s32 ndsFtAnimTrackIsDense(const void *p)
{
    (void)p;
    return FALSE;
}

void ndsFtAnimTrackStep(DObj *root_dobj)
{
    (void)root_dobj;
}

#endif
