/* Compile the original BattleShip object-animation/setup translation unit.
 * Its public add entrypoints normalize O2R AObjEvent32 command words before
 * the unchanged original parser sees them. */
#define gcAddDObjAnimJoint ndsBaseGcAddDObjAnimJoint
#define gcAddMObjMatAnimJoint ndsBaseGcAddMObjMatAnimJoint
#define gcAddAnimJointAll ndsBaseGcAddAnimJointAll
#define gcAddMatAnimJointAll ndsBaseGcAddMatAnimJointAll
#define gcAddAnimAll ndsBaseGcAddAnimAll
#define gcAddCObjCamAnimJoint ndsBaseGcAddCObjCamAnimJoint
#define gcPlayMObjMatAnim ndsBaseGcPlayMObjMatAnim
#define gcPlayAnimAll ndsBaseGcPlayAnimAll

#include "../../decomp/BattleShip-main/decomp/src/sys/objanim.c"

#undef gcAddDObjAnimJoint
#undef gcAddMObjMatAnimJoint
#undef gcAddAnimJointAll
#undef gcAddMatAnimJointAll
#undef gcAddAnimAll
#undef gcAddCObjCamAnimJoint
#undef gcPlayMObjMatAnim
#undef gcPlayAnimAll

#define NDS_AOBJ_EVENT32_NORMALIZED_MAX 1024u
#define NDS_AOBJ_EVENT32_PLAN_MAX 128u
#define NDS_AOBJ_EVENT32_BRANCH_DEPTH_MAX 16u

typedef enum NDSAObjEvent32OwnerKind
{
    nNDSAObjEvent32OwnerDObj,
    nNDSAObjEvent32OwnerMObj,
    nNDSAObjEvent32OwnerCObj
} NDSAObjEvent32OwnerKind;

typedef struct NDSAObjEvent32Normalized
{
    AObjEvent32 *command;
    u32 native_word;
} NDSAObjEvent32Normalized;

typedef struct NDSAObjEvent32Plan
{
    AObjEvent32 *command;
    u32 source_word;
    u32 native_word;
} NDSAObjEvent32Plan;

static NDSAObjEvent32Normalized
    sNdsAObjEvent32Normalized[NDS_AOBJ_EVENT32_NORMALIZED_MAX];
static NDSAObjEvent32Plan sNdsAObjEvent32Plan[NDS_AOBJ_EVENT32_PLAN_MAX];
static u32 sNdsAObjEvent32NormalizedCount;
static u32 sNdsAObjEvent32PlanCount;

volatile u32 gNdsAObjEvent32NormalizeScriptCount;
volatile u32 gNdsAObjEvent32NormalizeCommandCount;
volatile u32 gNdsAObjEvent32NormalizeReuseCount;
volatile u32 gNdsAObjEvent32NormalizeFailCount;
volatile u32 gNdsAObjEvent32NormalizeFirstSourceWord;
volatile u32 gNdsAObjEvent32NormalizeFirstNativeWord;
volatile u32 gNdsAObjEvent32NormalizeLastFailReason;
volatile u32 gNdsAObjEvent32NormalizeLastFailOwner;
volatile u32 gNdsAObjEvent32NormalizeLastFailAddress;
volatile u32 gNdsAObjEvent32NormalizeLastFailWord;
volatile u32 gNdsAObjEvent32NormalizeLastFailOpcode;
volatile u32 gNdsAObjEvent32NormalizeLastFailFlags;
volatile u32 gNdsAObjEvent32ColorCorrectionCount;

extern s32 ndsRelocPointerRangeInLoadedFiles(const void *ptr, size_t size);
extern s32 ndsRelocPointerIsFighterAObj16(const void *ptr);

static u32 ndsAObjEvent32CountFlags(u32 flags)
{
    u32 count = 0u;

    while (flags != 0u)
    {
        count += flags & 1u;
        flags >>= 1;
    }
    return count;
}

static s32 ndsAObjEvent32FindNormalized(AObjEvent32 *command)
{
    u32 i;

    for (i = 0u; i < sNdsAObjEvent32NormalizedCount; i++)
    {
        if (sNdsAObjEvent32Normalized[i].command == command)
        {
            return (s32)i;
        }
    }
    return -1;
}

static s32 ndsAObjEvent32FindPlanned(AObjEvent32 *command)
{
    u32 i;

    for (i = 0u; i < sNdsAObjEvent32PlanCount; i++)
    {
        if (sNdsAObjEvent32Plan[i].command == command)
        {
            return (s32)i;
        }
    }
    return -1;
}

static sb32 ndsAObjEvent32Reject(u32 reason, AObjEvent32 *command,
                                 NDSAObjEvent32OwnerKind owner_kind,
                                 u32 source_word)
{
    gNdsAObjEvent32NormalizeLastFailReason = reason;
    gNdsAObjEvent32NormalizeLastFailOwner = (u32)owner_kind;
    gNdsAObjEvent32NormalizeLastFailAddress = (u32)(uintptr_t)command;
    gNdsAObjEvent32NormalizeLastFailWord = source_word;
    gNdsAObjEvent32NormalizeLastFailOpcode =
        (source_word >> 25) & 0x7fu;
    gNdsAObjEvent32NormalizeLastFailFlags =
        (source_word >> 15) & 0x03ffu;
    return FALSE;
}

/* objdef.h:272-281 defines the source word as opcode[31:25], flags[24:15],
 * payload[14:0]. ARM GCC allocates objtypes.h:94-107 bitfields from the low
 * bit, so only command words are repacked; following values and pointers stay
 * in their already-relocated O2R representation. */
static sb32 ndsAObjEvent32PlanStream(AObjEvent32 *script,
                                     NDSAObjEvent32OwnerKind owner_kind,
                                     u32 branch_depth)
{
    AObjEvent32 *command = script;

    if ((script == NULL) ||
        (branch_depth > NDS_AOBJ_EVENT32_BRANCH_DEPTH_MAX))
    {
        return ndsAObjEvent32Reject(1u, script, owner_kind, 0u);
    }

    while (TRUE)
    {
        AObjEvent32 *branch_target = NULL;
        u32 source_word;
        u32 opcode;
        u32 flags;
        u32 payload;
        u32 value_words = 0u;
        s32 normalized_index;
        sb32 is_end = FALSE;
        sb32 is_branch = FALSE;

        if (ndsRelocPointerRangeInLoadedFiles(command, sizeof(*command)) ==
            FALSE)
        {
            return ndsAObjEvent32Reject(2u, command, owner_kind, 0u);
        }

        normalized_index = ndsAObjEvent32FindNormalized(command);
        if (normalized_index >= 0)
        {
            return (command->u ==
                    sNdsAObjEvent32Normalized[normalized_index].native_word) ?
                       TRUE : ndsAObjEvent32Reject(3u, command, owner_kind,
                                                   command->u);
        }
        if (ndsAObjEvent32FindPlanned(command) >= 0)
        {
            return TRUE;
        }

        source_word = command->u;
        opcode = (source_word >> 25) & 0x7fu;
        flags = (source_word >> 15) & 0x03ffu;
        payload = source_word & 0x7fffu;

        switch (opcode)
        {
        case nGCAnimEvent32End:
            is_end = TRUE;
            break;

        case nGCAnimEvent32Jump:
        case nGCAnimEvent32SetAnim:
            value_words = 1u;
            is_branch = TRUE;
            break;

        case nGCAnimEvent32Wait:
        case ANIM_CMD_12:
            break;

        case nGCAnimEvent32SetValBlock:
        case nGCAnimEvent32SetVal:
        case nGCAnimEvent32SetTargetRate:
        case nGCAnimEvent32SetVal0RateBlock:
        case nGCAnimEvent32SetVal0Rate:
        case nGCAnimEvent32SetValAfterBlock:
        case nGCAnimEvent32SetValAfter:
            value_words = ndsAObjEvent32CountFlags(flags);
            break;

        case nGCAnimEvent32SetValRateBlock:
        case nGCAnimEvent32SetValRate:
            value_words = ndsAObjEvent32CountFlags(flags) * 2u;
            break;

        case nGCAnimEvent32SetInterp:
            if (owner_kind == nNDSAObjEvent32OwnerDObj)
            {
                value_words = 1u;
            }
            else if (owner_kind == nNDSAObjEvent32OwnerCObj)
            {
                value_words = ((flags & 0x08u) != 0u) +
                              ((flags & 0x80u) != 0u);
            }
            else
            {
                return ndsAObjEvent32Reject(4u, command, owner_kind,
                                            source_word);
            }
            break;

        case nGCAnimEvent32SetFlags:
        case ANIM_CMD_16:
            if (owner_kind != nNDSAObjEvent32OwnerDObj)
            {
                return ndsAObjEvent32Reject(5u, command, owner_kind,
                                            source_word);
            }
            break;

        case ANIM_CMD_17:
            if (owner_kind != nNDSAObjEvent32OwnerDObj)
            {
                return ndsAObjEvent32Reject(6u, command, owner_kind,
                                            source_word);
            }
            value_words = ndsAObjEvent32CountFlags(flags);
            break;

        case nGCAnimEvent32SetExtValAfterBlock:
        case nGCAnimEvent32SetExtValAfter:
        case nGCAnimEvent32SetExtValBlock:
        case nGCAnimEvent32SetExtVal:
            if (owner_kind != nNDSAObjEvent32OwnerMObj)
            {
                return ndsAObjEvent32Reject(7u, command, owner_kind,
                                            source_word);
            }
            value_words = ndsAObjEvent32CountFlags(flags);
            break;

        case ANIM_CMD_22:
            if (owner_kind != nNDSAObjEvent32OwnerMObj)
            {
                return ndsAObjEvent32Reject(8u, command, owner_kind,
                                            source_word);
            }
            value_words = ndsAObjEvent32CountFlags(flags & 0x1fu);
            break;

        case ANIM_CMD_23:
            if (owner_kind != nNDSAObjEvent32OwnerCObj)
            {
                return ndsAObjEvent32Reject(9u, command, owner_kind,
                                            source_word);
            }
            /* objanim.c:2811-2813 consumes this command, then skips two
             * payload words before parsing the next command. */
            value_words = 2u;
            break;

        default:
            return ndsAObjEvent32Reject(10u, command, owner_kind,
                                        source_word);
        }

        if ((sNdsAObjEvent32PlanCount >= NDS_AOBJ_EVENT32_PLAN_MAX) ||
            (ndsRelocPointerRangeInLoadedFiles(
                 command, (1u + value_words) * sizeof(*command)) == FALSE))
        {
            return ndsAObjEvent32Reject(11u, command, owner_kind,
                                        source_word);
        }

        sNdsAObjEvent32Plan[sNdsAObjEvent32PlanCount].command = command;
        sNdsAObjEvent32Plan[sNdsAObjEvent32PlanCount].source_word = source_word;
        sNdsAObjEvent32Plan[sNdsAObjEvent32PlanCount].native_word =
            opcode | (flags << 7) | (payload << 17);
        sNdsAObjEvent32PlanCount++;

        if (is_end != FALSE)
        {
            return TRUE;
        }
        if (is_branch != FALSE)
        {
            branch_target = command[1].p;
            return ndsAObjEvent32PlanStream(branch_target, owner_kind,
                                             branch_depth + 1u);
        }
        command += 1u + value_words;
    }
}

static sb32 ndsAObjEvent32NormalizeScript(
    AObjEvent32 *script, NDSAObjEvent32OwnerKind owner_kind)
{
    u32 i;
    s32 normalized_index;

    if (script == NULL)
    {
        return TRUE;
    }

    normalized_index = ndsAObjEvent32FindNormalized(script);
    if (normalized_index >= 0)
    {
        if (script->u ==
            sNdsAObjEvent32Normalized[normalized_index].native_word)
        {
            gNdsAObjEvent32NormalizeReuseCount++;
            return TRUE;
        }
        (void)ndsAObjEvent32Reject(3u, script, owner_kind, script->u);
        gNdsAObjEvent32NormalizeFailCount++;
        return FALSE;
    }

    sNdsAObjEvent32PlanCount = 0u;
    if (ndsAObjEvent32PlanStream(script, owner_kind, 0u) == FALSE)
    {
        gNdsAObjEvent32NormalizeFailCount++;
        return FALSE;
    }
    if ((sNdsAObjEvent32NormalizedCount + sNdsAObjEvent32PlanCount) >
        NDS_AOBJ_EVENT32_NORMALIZED_MAX)
    {
        (void)ndsAObjEvent32Reject(12u, script, owner_kind, script->u);
        gNdsAObjEvent32NormalizeFailCount++;
        return FALSE;
    }

    if ((gNdsAObjEvent32NormalizeCommandCount == 0u) &&
        (sNdsAObjEvent32PlanCount != 0u))
    {
        gNdsAObjEvent32NormalizeFirstSourceWord =
            sNdsAObjEvent32Plan[0].source_word;
        gNdsAObjEvent32NormalizeFirstNativeWord =
            sNdsAObjEvent32Plan[0].native_word;
    }

    for (i = 0u; i < sNdsAObjEvent32PlanCount; i++)
    {
        sNdsAObjEvent32Plan[i].command->u =
            sNdsAObjEvent32Plan[i].native_word;
        sNdsAObjEvent32Normalized[sNdsAObjEvent32NormalizedCount].command =
            sNdsAObjEvent32Plan[i].command;
        sNdsAObjEvent32Normalized[sNdsAObjEvent32NormalizedCount].native_word =
            sNdsAObjEvent32Plan[i].native_word;
        sNdsAObjEvent32NormalizedCount++;
    }

    gNdsAObjEvent32NormalizeScriptCount++;
    gNdsAObjEvent32NormalizeCommandCount += sNdsAObjEvent32PlanCount;
    return TRUE;
}

void ndsAObjEvent32ResetNormalizedScripts(void)
{
    sNdsAObjEvent32NormalizedCount = 0u;
    sNdsAObjEvent32PlanCount = 0u;
}

static u32 ndsAObjEvent32FloatBits(f32 value)
{
    union
    {
        f32 f;
        u32 u;
    } bits;

    bits.f = value;
    return bits.u;
}

static u8 ndsAObjEvent32LerpColorChannel(u32 base, u32 target,
                                         u32 shift, s32 interp)
{
    s32 base_channel = (s32)((base >> shift) & 0xffu);
    s32 target_channel = (s32)((target >> shift) & 0xffu);

    return (u8)((base_channel * (256 - interp) +
                 target_channel * interp) >> 8);
}

/* objanim.c:1363-1388 interpolates packed RGBA by multiplying carefully
 * spaced bytes inside a u32. That arithmetic depends on N64 big-endian byte
 * lanes. Keep the original player for timing/state, then rewrite only the
 * five color outputs from the source 0xRRGGBBAA payload bits. */
static void ndsAObjEvent32CorrectMObjColors(MObj *mobj, sb32 force)
{
    AObj *aobj;

    if ((mobj == NULL) ||
        ((force == FALSE) && (mobj->anim_wait == AOBJ_ANIM_NULL)))
    {
        return;
    }

    for (aobj = mobj->aobj; aobj != NULL; aobj = aobj->next)
    {
        SYColorPack color;
        u32 base;
        u32 target;
        s32 interp;

        if ((aobj->kind == nGCAnimKindNone) ||
            (aobj->track < nGCAnimTrackPrimColor) ||
            (aobj->track > nGCAnimTrackLight2Color))
        {
            continue;
        }

        base = ndsAObjEvent32FloatBits(aobj->value_base);
        target = ndsAObjEvent32FloatBits(aobj->value_target);

        if (aobj->kind == nGCAnimKindLinear)
        {
            interp = (s32)(aobj->length * aobj->length_invert * 256.0F);
            if (interp < 0)
            {
                interp = 0;
            }
            else if (interp > 256)
            {
                interp = 256;
            }

            color.s.r = ndsAObjEvent32LerpColorChannel(
                base, target, 24u, interp);
            color.s.g = ndsAObjEvent32LerpColorChannel(
                base, target, 16u, interp);
            color.s.b = ndsAObjEvent32LerpColorChannel(
                base, target, 8u, interp);
            color.s.a = ndsAObjEvent32LerpColorChannel(
                base, target, 0u, interp);
        }
        else if (aobj->kind == nGCAnimKindStep)
        {
            u32 packed = (aobj->length_invert <= aobj->length) ?
                             target : base;

            color.s.r = (u8)(packed >> 24);
            color.s.g = (u8)(packed >> 16);
            color.s.b = (u8)(packed >> 8);
            color.s.a = (u8)packed;
        }
        else
        {
            continue;
        }

        switch (aobj->track)
        {
        case nGCAnimTrackPrimColor:
            mobj->sub.primcolor = color;
            break;
        case nGCAnimTrackEnvColor:
            mobj->sub.envcolor = color;
            break;
        case nGCAnimTrackBlendColor:
            mobj->sub.blendcolor = color;
            break;
        case nGCAnimTrackLight1Color:
            mobj->sub.light1color = color;
            break;
        case nGCAnimTrackLight2Color:
            mobj->sub.light2color = color;
            break;
        default:
            continue;
        }
        gNdsAObjEvent32ColorCorrectionCount++;
    }
}

void gcPlayMObjMatAnim(MObj *mobj)
{
    sb32 was_active = ((mobj != NULL) &&
                       (mobj->anim_wait != AOBJ_ANIM_NULL));

    ndsBaseGcPlayMObjMatAnim(mobj);
    if (was_active != FALSE)
    {
        ndsAObjEvent32CorrectMObjColors(mobj, TRUE);
    }
}

static u32 ndsAObjEvent32CollectActiveMObjs(GObj *gobj, MObj **active_mobjs)
{
    DObj *dobj;
    u32 count = 0u;

    for (dobj = (gobj != NULL) ? DObjGetStruct(gobj) : NULL;
         dobj != NULL;
         dobj = gcGetTreeDObjNext(dobj))
    {
        MObj *mobj;

        for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
        {
            if (mobj->anim_wait != AOBJ_ANIM_NULL)
            {
                if (active_mobjs != NULL)
                {
                    active_mobjs[count] = mobj;
                }
                count++;
            }
        }
    }
    return count;
}

void gcPlayAnimAll(GObj *gobj)
{
    DObj *dobj;
    u32 active_count = ndsAObjEvent32CollectActiveMObjs(gobj, NULL);
    MObj *active_mobjs[(active_count != 0u) ? active_count : 1u];
    u32 i;

    (void)ndsAObjEvent32CollectActiveMObjs(gobj, active_mobjs);

    ndsBaseGcPlayAnimAll(gobj);

    for (dobj = (gobj != NULL) ? DObjGetStruct(gobj) : NULL;
         dobj != NULL;
         dobj = gcGetTreeDObjNext(dobj))
    {
        MObj *mobj;

        for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
        {
            ndsAObjEvent32CorrectMObjColors(mobj, FALSE);
        }
    }

    /* The original player changes END to NULL after writing the last frame.
     * Correct only objects that crossed that edge during this call. */
    for (i = 0u; i < active_count; i++)
    {
        MObj *mobj = active_mobjs[i];

        if (mobj->anim_wait == AOBJ_ANIM_NULL)
        {
            ndsAObjEvent32CorrectMObjColors(mobj, TRUE);
        }
    }
}

static sb32 ndsAObjEvent32NormalizeDObjTable(GObj *gobj,
                                             AObjEvent32 **anim_joints)
{
    DObj *dobj = DObjGetStruct(gobj);

    while ((dobj != NULL) && (anim_joints != NULL))
    {
        if ((*anim_joints != NULL) &&
            (ndsRelocPointerIsFighterAObj16(*anim_joints) == FALSE) &&
            (ndsAObjEvent32NormalizeScript(
                 *anim_joints, nNDSAObjEvent32OwnerDObj) == FALSE))
        {
            return FALSE;
        }
        anim_joints++;
        dobj = gcGetTreeDObjNext(dobj);
    }
    return TRUE;
}

static sb32 ndsAObjEvent32NormalizeMObjTable(
    GObj *gobj, AObjEvent32 ***p_matanim_joints)
{
    DObj *dobj = DObjGetStruct(gobj);

    while (dobj != NULL)
    {
        if ((p_matanim_joints != NULL) && (*p_matanim_joints != NULL))
        {
            AObjEvent32 **matanim_joints = *p_matanim_joints;
            MObj *mobj = dobj->mobj;

            while (mobj != NULL)
            {
                if (ndsAObjEvent32NormalizeScript(
                        *matanim_joints, nNDSAObjEvent32OwnerMObj) == FALSE)
                {
                    return FALSE;
                }
                matanim_joints++;
                mobj = mobj->next;
            }
        }
        if (p_matanim_joints != NULL)
        {
            p_matanim_joints++;
        }
        dobj = gcGetTreeDObjNext(dobj);
    }
    return TRUE;
}

void gcAddDObjAnimJoint(DObj *dobj, AObjEvent32 *anim_joint,
                        f32 anim_frame)
{
    if ((anim_joint == NULL) ||
        (ndsRelocPointerIsFighterAObj16(anim_joint) != FALSE) ||
        (ndsAObjEvent32NormalizeScript(
             anim_joint, nNDSAObjEvent32OwnerDObj) != FALSE))
    {
        ndsBaseGcAddDObjAnimJoint(dobj, anim_joint, anim_frame);
    }
}

void gcAddMObjMatAnimJoint(MObj *mobj, AObjEvent32 *matanim_joint,
                           f32 anim_frame)
{
    if (ndsAObjEvent32NormalizeScript(
            matanim_joint, nNDSAObjEvent32OwnerMObj) != FALSE)
    {
        ndsBaseGcAddMObjMatAnimJoint(mobj, matanim_joint, anim_frame);
    }
}

void gcAddAnimJointAll(GObj *gobj, AObjEvent32 **anim_joints,
                       f32 anim_frame)
{
    if ((gobj != NULL) &&
        (ndsAObjEvent32NormalizeDObjTable(gobj, anim_joints) != FALSE))
    {
        ndsBaseGcAddAnimJointAll(gobj, anim_joints, anim_frame);
    }
}

void gcAddMatAnimJointAll(GObj *gobj, AObjEvent32 ***p_matanim_joints,
                          f32 anim_frame)
{
    if ((gobj != NULL) &&
        (ndsAObjEvent32NormalizeMObjTable(gobj, p_matanim_joints) != FALSE))
    {
        ndsBaseGcAddMatAnimJointAll(gobj, p_matanim_joints, anim_frame);
    }
}

void gcAddAnimAll(GObj *gobj, AObjEvent32 **anim_joints,
                  AObjEvent32 ***p_matanim_joints, f32 anim_frame)
{
    if ((gobj != NULL) &&
        (ndsAObjEvent32NormalizeDObjTable(gobj, anim_joints) != FALSE) &&
        (ndsAObjEvent32NormalizeMObjTable(gobj, p_matanim_joints) != FALSE))
    {
        ndsBaseGcAddAnimAll(gobj, anim_joints, p_matanim_joints, anim_frame);
    }
}

void gcAddCObjCamAnimJoint(CObj *cobj, AObjEvent32 *camanim_joint,
                           f32 anim_frame)
{
    if (ndsAObjEvent32NormalizeScript(
            camanim_joint, nNDSAObjEvent32OwnerCObj) != FALSE)
    {
        ndsBaseGcAddCObjCamAnimJoint(cobj, camanim_joint, anim_frame);
    }
}
