/*
 * Compile-only import of BattleShip ft/ftanim.c.
 *
 * The public ftAnim* entry points still resolve to the current DS seams until
 * the fighter-data/status-manager slice can graduate naturally.
 */
#include <ft/fighter.h>
#include <sys/objman.h>

#ifndef AObjAnimAdvance
#define AObjAnimAdvance(script) ((script)++)
#endif
#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif
void gcPlayDObjAnimJoint(DObj *dobj);
void gcParseMObjMatAnimJoint(MObj *mobj);
void gcPlayMObjMatAnim(MObj *mobj);

#define ftAnimGetTargetValue battleship_ftAnimGetTargetValue
#define ftAnimParseDObjFigatree battleship_ftAnimParseDObjFigatree
#define func_ovl2_800ECCA4 battleship_func_ovl2_800ECCA4
#include "../../decomp/BattleShip-main/decomp/src/ft/ftanim.c"
#undef ftAnimGetTargetValue
#undef ftAnimParseDObjFigatree
#undef func_ovl2_800ECCA4

/* ------------------------------------------------------------------------- *
 * R2-07 cycle 109 - the figatree parser without the soft float.
 *
 * `ftAnimParseDObjFigatree` is the #1 soft-float caller in the build:
 * 24,124,071 cycles inclusive, of which 7,931,031 is soft float. The shipped
 * ARM has 45 `bl __aeabi_*` sites in this ONE function (12 i2f, 11 fadd, 6
 * fsub, 6 fmul, 5 fcmpeq, 3 fdiv, 1 fcmple, 1 fcmpgt), and the three fdiv alone
 * are 1,494,619 cycles at 109.4 a call - the most expensive helper in the build
 * by 3x. `ftAnimGetTargetValue` does not appear as a symbol at all because it is
 * inlined into every call site, which is where most of the i2f and fmul live.
 *
 * This is a VERBATIM transcription of the decomp body with only the arithmetic
 * swapped. It is not a redesign: every `AObjAnimAdvance` is preserved in count
 * and order (it is `p++`, used more than once per expression and conditionally,
 * so the advance count is load-bearing), every branch sits in the same place,
 * and the deliberately odd parts - `track <= ARRAY_COUNT(track_aobjs)` at the
 * gather, `anim_frame` written from `anim_wait` on both exit paths - are
 * reproduced exactly rather than corrected.
 *
 * Why it lives HERE and not in the shim: this TU already compiles the decomp
 * file, so every type, enum and helper is in scope. It cannot simply redefine
 * the name - the `#define` above renames the definition and its internal call
 * sites TOGETHER - so the port body takes its own name and
 * `reloc_backend_compat_shims.c`'s `ftAnimParseDObjFigatree` (the one
 * `ftParamUpdateAnimKeys` actually calls) selects it. That is the same
 * interposition `battleship_sys_objanim.c` uses for `gcPlayDObjAnimJoint`.
 *
 * WHAT IS AND IS NOT BIT-IDENTICAL, stated rather than implied:
 *
 *   - `1.0F / payload` (two sites): bit-identical. `payload` is a u16 frame
 *     count (`relocdata_types.h`: the payload word follows the command word),
 *     and this build passes no `-ffast-math`, so the compile-time `1.0f/(f32)n`
 *     initialisers below are correctly rounded - the same value the runtime
 *     divide produces, for every n the table covers.
 *   - `ftAnimGetTargetValue`: bit-identical on the six power-of-two fracs. An
 *     s16 carries at most 16 significant bits against f32's 24, so the
 *     conversion is exact with no rounding decision at all, and scaling by 2^-k
 *     is an exponent subtraction that cannot underflow at these ranges (worst
 *     case k=13 with |arg|=1 leaves exponent 114).
 *   - `(value_target - value_base) / payload` (one site, Linear): the divide is
 *     KEPT. x/n and x*(1/n) round differently, and this feeds `rate_base`, which
 *     the cubic amplifies by `length`. Linear is 1.7% of nodes, so buying a
 *     1-ulp risk there would trade real fidelity for nothing measurable.
 *   - The comparisons go through `nds_fcmp.h`, exact by that header's own
 *     exhaustive proof. Equality against the `F32_MIN`-derived sentinels is a
 *     bit-pattern compare (IEEE gives every non-zero value a unique
 *     representation); that header's positive-constant restriction binds the
 *     ORDERED predicates, not `EQ`/`NE`.
 * ------------------------------------------------------------------------- */

#include <nds/nds_fcmp.h>

/* 1/n for n in [1,255], built by the compiler. Entry 0 is a placeholder: every
 * reader is guarded by a payload-not-zero test, and `1.0f/0.0f` in a constant
 * initialiser is a diagnostic. 1 KiB of .rodata against 34,816 bytes of proven
 * static headroom - priced before writing, as the RAM rule requires. */
#define NDS_R2R1(n)   (1.0f / (f32)(n))
#define NDS_R2R4(n)   NDS_R2R1((n)), NDS_R2R1((n) + 1), \
                      NDS_R2R1((n) + 2), NDS_R2R1((n) + 3)
#define NDS_R2R16(n)  NDS_R2R4((n)), NDS_R2R4((n) + 4), \
                      NDS_R2R4((n) + 8), NDS_R2R4((n) + 12)
#define NDS_R2R64(n)  NDS_R2R16((n)), NDS_R2R16((n) + 16), \
                      NDS_R2R16((n) + 32), NDS_R2R16((n) + 48)
#define NDS_R2_RECIP_COUNT 256u

static const f32 sNdsR2Recip[NDS_R2_RECIP_COUNT] = {
    0.0f, NDS_R2R1(1), NDS_R2R1(2), NDS_R2R1(3),
    NDS_R2R4(4), NDS_R2R4(8), NDS_R2R16(12), NDS_R2R4(28),
    NDS_R2R64(32), NDS_R2R64(96), NDS_R2R64(160), NDS_R2R16(224),
    NDS_R2R16(240)
};

volatile u32 gNdsR2FtAnimRecipHits;
volatile u32 gNdsR2FtAnimRecipMisses;
volatile u32 gNdsR2FtAnimParseCalls;

static inline f32 ndsR2BitsToF32(u32 bits)
{
    f32 v;

    __builtin_memcpy(&v, &bits, sizeof(v));
    return v;
}

/* Exactly (f32)v for v < 2^16: at most 16 significant bits against f32's 24, so
 * there is no rounding decision to make. */
static inline f32 ndsR2U16ToF32(u32 v)
{
    u32 shift;

    if (v == 0u)
    {
        return 0.0f;
    }
    shift = (u32)__builtin_clz(v);
    return ndsR2BitsToF32((((127u + 31u) - shift) << 23) |
                          (((v << shift) & 0x7fffffffu) >> 8));
}

/* `1.0F / payload`, a table hit whenever the frame count is in range. */
static inline f32 ndsR2Recip(u32 n)
{
    if (n < NDS_R2_RECIP_COUNT)
    {
        gNdsR2FtAnimRecipHits++;
        return sNdsR2Recip[n];
    }
    gNdsR2FtAnimRecipMisses++;
    return 1.0f / (f32)n;
}

/* `ftAnimGetTargetValue` with the i2f and the fmul replaced by an exponent
 * subtraction on the six power-of-two fracs. The shift per id is the decomp's
 * own table read as a power of two: 1/512, 1/4, 1/4096 for values, then 1/512,
 * 1/32, 1/8192 for rates. Ids 3 and 7 (TraI) are `1/16384 - 3e-12`, which is
 * not a power of two, so they keep the original expression - there is no exact
 * shift for them and TraI is one track of ten. */
static const u8 sNdsR2AnimFracShift[8] = { 9u, 2u, 12u, 0u, 9u, 5u, 13u, 0u };

static f32 ndsR2AnimTargetValue(s16 arg, s32 track, sb32 value_or_step)
{
    s32 id;
    u32 mag;
    u32 shift;
    u32 k;

    switch (track)
    {
    case nGCAnimTrackRotX:
    case nGCAnimTrackRotY:
    case nGCAnimTrackRotZ:
        id = 0;
        break;

    case nGCAnimTrackTraX:
    case nGCAnimTrackTraY:
    case nGCAnimTrackTraZ:
        id = 1;
        break;

    case nGCAnimTrackScaX:
    case nGCAnimTrackScaY:
    case nGCAnimTrackScaZ:
        id = 2;
        break;

    case nGCAnimTrackTraI:
        id = 3;
        break;

    default:
        /* The decomp leaves `id` uninitialised here. Every caller passes a
         * joint track, so the arm is unreachable; pinning it to 0 makes the
         * unreachable case defined instead of whatever was on the stack. */
        id = 0;
        break;
    }
    if (value_or_step != 0)
    {
        id += 4;
    }
    if ((id == 3) || (id == 7))
    {
        return (f32)arg * (1.0F / 16384.0F - (3.0F / 1000000000000.0F));
    }
    if (arg == 0)
    {
        return 0.0f;
    }
    mag = (arg < 0) ? (u32)(-(s32)arg) : (u32)arg;
    shift = (u32)__builtin_clz(mag);
    k = sNdsR2AnimFracShift[id];
    /* mag = 1.m * 2^(31-shift), so mag * 2^-k = 1.m * 2^(31-shift-k). */
    return ndsR2BitsToF32(
        ((arg < 0) ? 0x80000000u : 0u) |
        ((((127u + 31u) - shift) - k) << 23) |
        (((mag << shift) & 0x7fffffffu) >> 8));
}

/* The six payload reads are textually identical in the decomp, so they are one
 * macro here and a slip cannot differ between sites. `AObjAnimAdvance` is `p++`,
 * and the second advance happens ONLY when the toggle bit is set - that
 * asymmetry is the format (a payload word follows the command only when the
 * toggle says so), not an accident. */
#define NDS_R2_FTANIM_PAYLOAD()                                              \
    (payload_u =                                                             \
        (AObjAnimAdvance(root_dobj->anim_joint.event16)->command.toggle)      \
            ? AObjAnimAdvance(root_dobj->anim_joint.event16)->u              \
            : 0u,                                                            \
     payload = ndsR2U16ToF32(payload_u))

#define NDS_R2_FTANIM_TARGET(vos)                                            \
    ndsR2AnimTargetValue(                                                    \
        AObjAnimAdvance(root_dobj->anim_joint.event16)->s,                   \
        i + nGCAnimTrackJointStart, (vos))

#define NDS_R2_FTANIM_ENSURE()                                               \
    do {                                                                     \
        if (track_aobjs[i] == NULL)                                          \
        {                                                                    \
            track_aobjs[i] =                                                 \
                gcAddAObjForDObj(root_dobj, i + nGCAnimTrackJointStart);     \
        }                                                                    \
    } while (0)

void ndsR2FtAnimParseDObjFigatree(DObj *root_dobj)
{
    AObj *track_aobjs[nGCAnimTrackJointEnd - nGCAnimTrackJointStart + 1];
    AObj *current_aobj;
    f32 payload;
    u32 payload_u;
    s32 i;
    u16 command_kind;
    u16 flags;
    u32 events = 0;

    gNdsR2FtAnimParseCalls++;

    if (NDS_FCMP_NE_C(root_dobj->anim_wait, AOBJ_ANIM_NULL))
    {
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
                return;
            }
        }
        for (i = 0; i < (s32)ARRAY_COUNT(track_aobjs); i++)
        {
            track_aobjs[i] = NULL;
        }
        current_aobj = root_dobj->aobj;

        while (current_aobj != NULL)
        {
            if ((current_aobj->track >= nGCAnimTrackJointStart) &&
                (current_aobj->track <= ARRAY_COUNT(track_aobjs)))
            {
                track_aobjs[current_aobj->track - nGCAnimTrackJointStart] =
                    current_aobj;
            }
            current_aobj = current_aobj->next;
        }
        do
        {
            if (root_dobj->anim_joint.event16 == NULL)
            {
                current_aobj = root_dobj->aobj;

                while (current_aobj != NULL)
                {
                    if (current_aobj->kind != nGCAnimKindNone)
                    {
                        current_aobj->length +=
                            root_dobj->anim_speed + root_dobj->anim_wait;
                    }
                    current_aobj = current_aobj->next;
                }
                root_dobj->anim_frame = root_dobj->anim_wait;
                root_dobj->parent_gobj->anim_frame = root_dobj->anim_wait;
                root_dobj->anim_wait = AOBJ_ANIM_END;

                return;
            }
            command_kind = root_dobj->anim_joint.event16->command.opcode;

            switch (command_kind)
            {
            case nGCAnimEvent16SetVal0RateBlock:
            case nGCAnimEvent16SetVal0Rate:
                flags = root_dobj->anim_joint.event16->command.flags;
                NDS_R2_FTANIM_PAYLOAD();

                for (i = 0; i < (s32)ARRAY_COUNT(track_aobjs);
                     i++, flags = flags >> 1)
                {
                    if (!(flags))
                    {
                        break;
                    }
                    if (flags & 1)
                    {
                        NDS_R2_FTANIM_ENSURE();
                        track_aobjs[i]->value_base =
                            track_aobjs[i]->value_target;
                        track_aobjs[i]->value_target = NDS_R2_FTANIM_TARGET(0);

                        track_aobjs[i]->rate_base = track_aobjs[i]->rate_target;
                        track_aobjs[i]->rate_target = 0.0F;

                        track_aobjs[i]->kind = nGCAnimKindCubic;

                        if (payload_u != 0u)
                        {
                            track_aobjs[i]->length_invert =
                                ndsR2Recip(payload_u);
                        }
                        track_aobjs[i]->length =
                            -root_dobj->anim_wait - root_dobj->anim_speed;
                    }
                }
                if (command_kind == nGCAnimEvent16SetVal0RateBlock)
                {
                    root_dobj->anim_wait += payload;
                }
                break;

            case nGCAnimEvent16SetValBlock:
            case nGCAnimEvent16SetVal:
                flags = root_dobj->anim_joint.event16->command.flags;
                NDS_R2_FTANIM_PAYLOAD();

                for (i = 0; i < (s32)ARRAY_COUNT(track_aobjs);
                     i++, flags = flags >> 1)
                {
                    if (!(flags))
                    {
                        break;
                    }
                    if (flags & 1)
                    {
                        NDS_R2_FTANIM_ENSURE();
                        track_aobjs[i]->value_base =
                            track_aobjs[i]->value_target;
                        track_aobjs[i]->value_target = NDS_R2_FTANIM_TARGET(0);

                        track_aobjs[i]->kind = nGCAnimKindLinear;

                        if (payload_u != 0u)
                        {
                            /* The one divide that STAYS a divide. See the head
                             * of this block: x*(1/n) rounds differently, and
                             * this feeds a rate the cubic amplifies by
                             * `length`. */
                            track_aobjs[i]->rate_base =
                                (track_aobjs[i]->value_target -
                                 track_aobjs[i]->value_base) / payload;
                        }
                        track_aobjs[i]->length =
                            -root_dobj->anim_wait - root_dobj->anim_speed;
                        track_aobjs[i]->rate_target = 0.0F;
                    }
                }
                if (command_kind == nGCAnimEvent16SetValBlock)
                {
                    root_dobj->anim_wait += payload;
                }
                break;

            case nGCAnimEvent16SetValRateBlock:
            case nGCAnimEvent16SetValRate:
                flags = root_dobj->anim_joint.event16->command.flags;
                NDS_R2_FTANIM_PAYLOAD();

                for (i = 0; i < (s32)ARRAY_COUNT(track_aobjs);
                     i++, flags = flags >> 1)
                {
                    if (!(flags))
                    {
                        break;
                    }
                    if (flags & 1)
                    {
                        NDS_R2_FTANIM_ENSURE();
                        track_aobjs[i]->value_base =
                            track_aobjs[i]->value_target;
                        track_aobjs[i]->value_target = NDS_R2_FTANIM_TARGET(0);

                        track_aobjs[i]->rate_base = track_aobjs[i]->rate_target;
                        track_aobjs[i]->rate_target = NDS_R2_FTANIM_TARGET(1);

                        track_aobjs[i]->kind = nGCAnimKindCubic;

                        if (payload_u != 0u)
                        {
                            track_aobjs[i]->length_invert =
                                ndsR2Recip(payload_u);
                        }
                        track_aobjs[i]->length =
                            -root_dobj->anim_wait - root_dobj->anim_speed;
                    }
                }
                if (command_kind == nGCAnimEvent16SetValRateBlock)
                {
                    root_dobj->anim_wait += payload;
                }
                break;

            case nGCAnimEvent16SetTargetRate:
                flags = root_dobj->anim_joint.event16->command.flags;

                NDS_R2_FTANIM_PAYLOAD();

                for (i = 0; i < (s32)ARRAY_COUNT(track_aobjs);
                     i++, flags = flags >> 1)
                {
                    if (!(flags))
                    {
                        break;
                    }
                    if (flags & 1)
                    {
                        NDS_R2_FTANIM_ENSURE();
                        track_aobjs[i]->rate_target = NDS_R2_FTANIM_TARGET(1);
                    }
                }
                break;

            case nGCAnimEvent16Block:
                if (AObjAnimAdvance(
                        root_dobj->anim_joint.event16)->command.toggle)
                {
                    root_dobj->anim_wait += ndsR2U16ToF32(
                        AObjAnimAdvance(root_dobj->anim_joint.event16)->u);
                }
                break;

            case nGCAnimEvent16SetValAfterBlock:
            case nGCAnimEvent16SetValAfter:
                flags = root_dobj->anim_joint.event16->command.flags;
                NDS_R2_FTANIM_PAYLOAD();

                for (i = 0; i < (s32)ARRAY_COUNT(track_aobjs);
                     i++, flags = flags >> 1)
                {
                    if (!(flags))
                    {
                        break;
                    }
                    if (flags & 1)
                    {
                        NDS_R2_FTANIM_ENSURE();
                        track_aobjs[i]->value_base =
                            track_aobjs[i]->value_target;
                        track_aobjs[i]->value_target = NDS_R2_FTANIM_TARGET(0);

                        track_aobjs[i]->kind = nGCAnimKindStep;

                        track_aobjs[i]->length_invert = payload;
                        track_aobjs[i]->length =
                            -root_dobj->anim_wait - root_dobj->anim_speed;

                        track_aobjs[i]->rate_target = 0.0F;
                    }
                }
                if (command_kind == nGCAnimEvent16SetValAfterBlock)
                {
                    root_dobj->anim_wait += payload;
                }
                break;

            case nGCAnimEvent16Loop:
                AObjAnimAdvance(root_dobj->anim_joint.event16);

                root_dobj->anim_joint.event16 +=
                    root_dobj->anim_joint.event16->s / 2;

                root_dobj->anim_frame = -root_dobj->anim_wait;
                root_dobj->parent_gobj->anim_frame = -root_dobj->anim_wait;

                if (root_dobj->is_anim_root != FALSE)
                {
                    if (root_dobj->parent_gobj->func_anim != NULL)
                    {
                        root_dobj->parent_gobj->func_anim(root_dobj, -2, 0);
                    }
                }
                break;

            case nGCAnimEvent1611:
                flags = root_dobj->anim_joint.event16->command.flags;

                NDS_R2_FTANIM_PAYLOAD();

                for (i = 0; i < (s32)ARRAY_COUNT(track_aobjs);
                     i++, flags = flags >> 1)
                {
                    if (!(flags))
                    {
                        break;
                    }
                    if (flags & 1)
                    {
                        NDS_R2_FTANIM_ENSURE();
                        track_aobjs[i]->length += payload;
                    }
                }
                break;

            case nGCAnimEvent16SetTranslateInterp:
                AObjAnimAdvance(root_dobj->anim_joint.event16);

                if (track_aobjs[nGCAnimTrackTraI - nGCAnimTrackJointStart] ==
                    NULL)
                {
                    track_aobjs[nGCAnimTrackTraI - nGCAnimTrackJointStart] =
                        gcAddAObjForDObj(root_dobj, nGCAnimTrackTraI);
                }
                track_aobjs[nGCAnimTrackTraI -
                    nGCAnimTrackJointStart]->interpolate =
                        root_dobj->anim_joint.event16 +
                            (root_dobj->anim_joint.event16->s / 2);

                AObjAnimAdvance(root_dobj->anim_joint.event16);
                break;

            case nGCAnimEvent16End:
                current_aobj = root_dobj->aobj;

                while (current_aobj != NULL)
                {
                    if (current_aobj->kind != nGCAnimKindNone)
                    {
                        current_aobj->length +=
                            root_dobj->anim_speed + root_dobj->anim_wait;
                    }
                    current_aobj = current_aobj->next;
                }
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

            case nGCAnimEvent16SetFlags:
                root_dobj->flags =
                    root_dobj->anim_joint.event16->command.flags;

                if (AObjAnimAdvance(
                        root_dobj->anim_joint.event16)->command.toggle)
                {
                    root_dobj->anim_wait += ndsR2U16ToF32(
                        AObjAnimAdvance(root_dobj->anim_joint.event16)->u);
                }
                break;

            default:
                /* Same recorded fault as the patch this body replaces: an
                 * unknown opcode is a runaway script, not something to spin on.
                 * Mask bits 6 and 7 are this parser's. */
                gNdsObjAnimRunawayCount++;
                gNdsObjAnimRunawayMask |= 1u << 6u;
                gNdsObjAnimRunawayScript =
                    (u32)(uintptr_t)root_dobj->anim_joint.event16;
                gNdsObjAnimRunawayOpcode = command_kind;
                root_dobj->anim_wait = AOBJ_ANIM_NULL;
                return;
            }
            if (++events >= NDS_FTANIM_EVENT_LIMIT)
            {
                gNdsObjAnimRunawayCount++;
                gNdsObjAnimRunawayMask |= 1u << 7u;
                gNdsObjAnimRunawayScript =
                    (u32)(uintptr_t)root_dobj->anim_joint.event16;
                gNdsObjAnimRunawayOpcode = command_kind;
                root_dobj->anim_wait = AOBJ_ANIM_NULL;
                return;
            }
        }
        while (NDS_FCMP_LE0(root_dobj->anim_wait));
    }
}
