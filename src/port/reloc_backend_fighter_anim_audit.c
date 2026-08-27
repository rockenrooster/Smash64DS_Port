
#if NDS_FIGHTER_ANIM_AUDIT
extern FTStatusDesc dFTCommonActionStatusDescs[];

static GObj *sNdsFighterAnimAuditTarget;
static u32 sNdsFighterAnimAuditMotion;
static u32 sNdsFighterAnimAuditMotionCount;
static u32 sNdsFighterAnimAuditPlayed;
static u32 sNdsFighterAnimAuditExpected;
static u32 sNdsFighterAnimAuditFirstState;
static u32 sNdsFighterAnimAuditExternFailStart;
static u32 sNdsFighterAnimAuditFigatreeInvalidStart;
static u32 sNdsFighterAnimAuditUnsafeStart;
static u32 sNdsFighterAnimAuditLastLoadSerial;
static u32 sNdsFighterAnimAuditDurationFlag;
static sb32 sNdsFighterAnimAuditStarted;
static sb32 sNdsFighterAnimAuditInternalSetStatus;
static sb32 sNdsFighterAnimAuditCapturePending;
static sb32 sNdsFighterAnimAuditCaptured;
static sb32 sNdsFighterAnimAuditMotionComplete;

extern s32 ndsRelocPointerRangeInLoadedFiles(const void *ptr, size_t size);
extern s32 ndsRelocPointerIsFighterAObj16(const void *ptr);
extern s32 ndsRelocPointerIsFighterAObj32(const void *ptr);

__attribute__((noinline)) void ndsFighterAnimAuditCaptureMarker(u32 kind,
                                                                u32 motion)
{
    __asm__ volatile("" : : "r"(kind), "r"(motion) : "memory");
}

__attribute__((noinline)) void ndsFighterAnimAuditCompleteMarker(u32 kind,
                                                                 u32 rows)
{
    __asm__ volatile("" : : "r"(kind), "r"(rows) : "memory");
}

static s32 ndsFighterAnimAuditKindIndex(const FTStruct *fp)
{
    if (fp == NULL)
    {
        return -1;
    }
    if (fp->fkind == nFTKindMario)
    {
        return 0;
    }
    if (fp->fkind == nFTKindFox)
    {
        return 1;
    }
    return -1;
}

static void ndsFighterAnimAuditIncrement(volatile u16 *value)
{
    if (*value != 0xffffu)
    {
        (*value)++;
    }
}

static u32 ndsFighterAnimAuditState(const FTStruct *fp)
{
    union {
        f32 f;
        u32 u;
    } bits;
    u32 hash = 2166136261u;
    u32 i;

    for (i = 0u; i < FTPARTS_JOINT_NUM_MAX; i++)
    {
        DObj *joint = fp->joints[i];

        if (joint == NULL)
        {
            continue;
        }
        hash = (hash ^ (u32)(uintptr_t)joint->anim_joint.event32) * 16777619u;
        bits.f = joint->anim_frame;
        hash = (hash ^ bits.u) * 16777619u;
        bits.f = joint->anim_wait;
        hash = (hash ^ bits.u) * 16777619u;
    }
    return hash;
}

static void ndsFighterAnimAuditFreeze(FTStruct *fp)
{
    fp->proc_update = NULL;
    fp->proc_accessory = NULL;
    fp->proc_interrupt = NULL;
    fp->proc_physics = NULL;
    fp->proc_map = NULL;
    fp->proc_slope = NULL;
    fp->proc_damage = NULL;
    fp->proc_trap = NULL;
    fp->proc_status = NULL;
    fp->physics.vel_air.x = 0.0F;
    fp->physics.vel_air.y = 0.0F;
    fp->physics.vel_air.z = 0.0F;
    fp->physics.vel_ground.x = 0.0F;
    fp->physics.vel_ground.y = 0.0F;
    fp->physics.vel_ground.z = 0.0F;
}

static u32 ndsFighterAnimAuditFlagCount(u16 flags)
{
    u32 count = 0u;

    while (flags != 0u)
    {
        count += flags & 1u;
        flags >>= 1u;
    }
    return count;
}

static sb32 ndsFighterAnimAuditRead16(AObjEvent16 **cursor, u16 *value)
{
    if ((cursor == NULL) || (*cursor == NULL) || (value == NULL) ||
        (ndsRelocPointerRangeInLoadedFiles(*cursor, sizeof(**cursor)) == FALSE))
    {
        return FALSE;
    }
    *value = (*cursor)->u;
    (*cursor)++;
    return TRUE;
}

static sb32 ndsFighterAnimAuditRead32(AObjEvent32 **cursor,
                                       AObjEvent32 *value)
{
    if ((cursor == NULL) || (*cursor == NULL) || (value == NULL) ||
        (ndsRelocPointerRangeInLoadedFiles(*cursor, sizeof(**cursor)) == FALSE))
    {
        return FALSE;
    }
    *value = **cursor;
    (*cursor)++;
    return TRUE;
}

static sb32 ndsFighterAnimAuditScriptFrames(AObjEvent16 *script,
                                             u32 *frames)
{
    AObjEvent16 *cursor = script;
    u32 total = 0u;
    u32 guard;

    if ((script == NULL) || (frames == NULL) ||
        (ndsRelocPointerIsFighterAObj16(script) == FALSE))
    {
        return FALSE;
    }
    for (guard = 0u; guard < 4096u; guard++)
    {
        u16 header;
        u16 opcode;
        u16 flags;
        u16 payload = 0u;
        u32 values = 0u;
        sb32 blocking = FALSE;
        sb32 uses_toggle = TRUE;

        if (ndsFighterAnimAuditRead16(&cursor, &header) == FALSE)
        {
            return FALSE;
        }
        opcode = header & 0x1fu;
        flags = (header >> 5u) & 0x3ffu;
        if ((opcode == nGCAnimEvent16End) ||
            (opcode == nGCAnimEvent16Loop))
        {
            *frames = total + 1u;
            return TRUE;
        }
        switch (opcode)
        {
        case nGCAnimEvent16SetVal0RateBlock:
        case nGCAnimEvent16SetValBlock:
        case nGCAnimEvent16SetValAfterBlock:
            blocking = TRUE;
            /* fallthrough */
        case nGCAnimEvent16SetVal0Rate:
        case nGCAnimEvent16SetVal:
        case nGCAnimEvent16SetValAfter:
            values = ndsFighterAnimAuditFlagCount(flags);
            break;

        case nGCAnimEvent16SetValRateBlock:
            blocking = TRUE;
            /* fallthrough */
        case nGCAnimEvent16SetValRate:
            values = ndsFighterAnimAuditFlagCount(flags) * 2u;
            break;

        case nGCAnimEvent16SetTargetRate:
            values = ndsFighterAnimAuditFlagCount(flags);
            break;

        case nGCAnimEvent16Block:
        case nGCAnimEvent16SetFlags:
            blocking = TRUE;
            break;

        case nGCAnimEvent1611:
            break;

        case nGCAnimEvent16SetTranslateInterp:
            values = 1u;
            uses_toggle = FALSE;
            break;

        default:
            break;
        }
        if ((uses_toggle != FALSE) && ((header & 0x8000u) != 0u))
        {
            if (ndsFighterAnimAuditRead16(&cursor, &payload) == FALSE)
            {
                return FALSE;
            }
        }
        while (values-- != 0u)
        {
            u16 ignored;

            if (ndsFighterAnimAuditRead16(&cursor, &ignored) == FALSE)
            {
                return FALSE;
            }
        }
        if (blocking != FALSE)
        {
            total += payload;
        }
    }
    return FALSE;
}

static sb32 ndsFighterAnimAuditScriptFrames32(AObjEvent32 *script,
                                               u32 *frames)
{
    AObjEvent32 *cursor = script;
    u32 total = 0u;
    u32 guard;

    if ((script == NULL) || (frames == NULL) ||
        (ndsRelocPointerIsFighterAObj32(script) == FALSE))
    {
        return FALSE;
    }
    for (guard = 0u; guard < 4096u; guard++)
    {
        AObjEvent32 event;
        u32 values = 0u;

        if (ndsFighterAnimAuditRead32(&cursor, &event) == FALSE)
        {
            return FALSE;
        }
        switch (event.command.opcode)
        {
        case nGCAnimEvent32SetValBlock:
        case nGCAnimEvent32SetVal0RateBlock:
        case nGCAnimEvent32SetValAfterBlock:
            total += event.command.payload;
            /* fallthrough */
        case nGCAnimEvent32SetVal:
        case nGCAnimEvent32SetTargetRate:
        case nGCAnimEvent32SetVal0Rate:
        case nGCAnimEvent32SetValAfter:
            values = ndsFighterAnimAuditFlagCount(event.command.flags);
            break;

        case nGCAnimEvent32SetValRateBlock:
            total += event.command.payload;
            /* fallthrough */
        case nGCAnimEvent32SetValRate:
            values = ndsFighterAnimAuditFlagCount(event.command.flags) * 2u;
            break;

        case nGCAnimEvent32Wait:
        case nGCAnimEvent32SetFlags:
            total += event.command.payload;
            break;

        case nGCAnimEvent32SetInterp:
            values = 1u;
            break;

        case ANIM_CMD_17:
            total += event.command.payload;
            values = ndsFighterAnimAuditFlagCount(event.command.flags);
            break;

        case nGCAnimEvent32End:
        case nGCAnimEvent32Jump:
        case nGCAnimEvent32SetAnim:
            *frames = total + 1u;
            return TRUE;

        case ANIM_CMD_12:
        case ANIM_CMD_16:
            break;

        default:
            return FALSE;
        }
        while (values-- != 0u)
        {
            AObjEvent32 ignored;

            if (ndsFighterAnimAuditRead32(&cursor, &ignored) == FALSE)
            {
                return FALSE;
            }
        }
    }
    return FALSE;
}

static sb32 ndsFighterAnimAuditExpectedFrames(const FTStruct *fp,
                                               u32 *frames)
{
    u32 expected = 0u;
    u32 i;
    u32 joint_limit;
    sb32 found = FALSE;

    if ((fp == NULL) || (frames == NULL))
    {
        return FALSE;
    }
    joint_limit = (u32)ndsFTStructJointLoopLimit(fp);
    for (i = 0u; i < joint_limit; i++)
    {
        DObj *joint = fp->joints[i];
        u32 joint_frames;

        if ((joint == NULL) || (joint->anim_joint.event16 == NULL))
        {
            continue;
        }
        if (((fp->anim_desc.flags.is_anim_joint != FALSE) &&
             (ndsFighterAnimAuditScriptFrames32(joint->anim_joint.event32,
                                                &joint_frames) == FALSE)) ||
            ((fp->anim_desc.flags.is_anim_joint == FALSE) &&
             (ndsFighterAnimAuditScriptFrames(joint->anim_joint.event16,
                                              &joint_frames) == FALSE)))
        {
            return FALSE;
        }
        if (joint_frames > expected)
        {
            expected = joint_frames;
        }
        found = TRUE;
    }
    *frames = expected;
    return found;
}

static void ndsFighterAnimAuditRecordSetStatus(FTStruct *fp)
{
    FTMotionDesc *motion_desc;
    s32 kind = ndsFighterAnimAuditKindIndex(fp);
    s32 motion;

    if ((kind < 0) || (fp->data == NULL) || (fp->data->mainmotion == NULL))
    {
        return;
    }
    motion = fp->motion_id;
    if ((motion < 0) ||
        ((u32)motion >= NDS_FIGHTER_ANIM_AUDIT_MOTION_CAPACITY) ||
        (motion >= fp->data->mainmotion_array_count))
    {
        gNdsFighterAnimAuditInvalidMotionCount++;
        return;
    }

    ndsFighterAnimAuditIncrement(&gNdsFighterAnimAuditRequested[kind][motion]);
    motion_desc = &fp->data->mainmotion->motion_desc[motion];
    if (motion_desc->anim_file_id == 0u)
    {
        gNdsFighterAnimAuditFlags[kind][motion] |=
            NDS_FIGHTER_ANIM_AUDIT_FLAG_SOURCE_NULL;
        return;
    }
    if (motion_desc->anim_desc.flags.is_use_shieldpose != FALSE)
    {
        ndsFighterAnimAuditIncrement(
            &gNdsFighterAnimAuditResolved[kind][motion]);
        return;
    }
    if ((gNdsFighterAnimAuditLoadSerial !=
         sNdsFighterAnimAuditLastLoadSerial) &&
        (gNdsFighterAnimAuditLoadResolved != 0u))
    {
        ndsFighterAnimAuditIncrement(
            &gNdsFighterAnimAuditResolved[kind][motion]);
    }
    else
    {
        ndsFighterAnimAuditIncrement(
            &gNdsFighterAnimAuditFallback[kind][motion]);
        gNdsFighterAnimAuditFlags[kind][motion] |=
            NDS_FIGHTER_ANIM_AUDIT_FLAG_LOAD_FALLBACK;
    }
    sNdsFighterAnimAuditLastLoadSerial = gNdsFighterAnimAuditLoadSerial;
}

static void ndsFighterAnimAuditBeginNext(FTStruct *fp)
{
    s32 kind = ndsFighterAnimAuditKindIndex(fp);

    while (sNdsFighterAnimAuditMotion < sNdsFighterAnimAuditMotionCount)
    {
        FTMotionDesc *motion_desc =
            &fp->data->mainmotion->motion_desc[sNdsFighterAnimAuditMotion];
        FTStatusDesc *wait_desc;
        s32 saved_motion;

        gNdsFighterAnimAuditActiveKind = (u32)kind;
        gNdsFighterAnimAuditActiveMotion = sNdsFighterAnimAuditMotion;
        gNdsFighterAnimAuditEpoch++;
        if (motion_desc->anim_file_id == 0u)
        {
            ndsFighterAnimAuditIncrement(
                &gNdsFighterAnimAuditRequested[kind]
                                                [sNdsFighterAnimAuditMotion]);
            gNdsFighterAnimAuditFlags[kind][sNdsFighterAnimAuditMotion] |=
                NDS_FIGHTER_ANIM_AUDIT_FLAG_SOURCE_NULL;
            sNdsFighterAnimAuditMotion++;
            continue;
        }

        sNdsFighterAnimAuditExternFailStart =
            gNdsFighterMarioFoxExternalFixupFailCount;
        sNdsFighterAnimAuditFigatreeInvalidStart =
            gNdsFighterNaturalMotionFigatreeAnimInvalidCount;
        sNdsFighterAnimAuditUnsafeStart =
            gNdsFighterNaturalMotionUnsafeCount;
        wait_desc = &dFTCommonActionStatusDescs[
            nFTCommonStatusWait - nFTCommonStatusActionStart];
        saved_motion = wait_desc->mflags.motion_id;
        wait_desc->mflags.motion_id = (s16)sNdsFighterAnimAuditMotion;
        sNdsFighterAnimAuditInternalSetStatus = TRUE;
        ftMainSetStatus(sNdsFighterAnimAuditTarget, nFTCommonStatusWait,
                        0.0F, 1.0F, 0u);
        sNdsFighterAnimAuditInternalSetStatus = FALSE;
        wait_desc->mflags.motion_id = (s16)saved_motion;
        fp = ftGetStruct(sNdsFighterAnimAuditTarget);
        if (fp == NULL)
        {
            return;
        }
        ndsFighterAnimAuditFreeze(fp);
        sNdsFighterAnimAuditPlayed = 0u;
        sNdsFighterAnimAuditExpected = 0u;
        sNdsFighterAnimAuditFirstState = 0u;
        sNdsFighterAnimAuditCapturePending = FALSE;
        sNdsFighterAnimAuditCaptured = FALSE;
        sNdsFighterAnimAuditMotionComplete = FALSE;
        sNdsFighterAnimAuditDurationFlag = 0u;
        if (ndsFighterAnimAuditExpectedFrames(
                fp, &sNdsFighterAnimAuditExpected) == FALSE)
        {
            gNdsFighterAnimAuditFlags[kind][sNdsFighterAnimAuditMotion] |=
                NDS_FIGHTER_ANIM_AUDIT_FLAG_EXPECTED_INVALID;
        }
        if ((gNdsFighterAnimAuditFlags[kind][sNdsFighterAnimAuditMotion] &
             NDS_FIGHTER_ANIM_AUDIT_FLAG_LOAD_FALLBACK) != 0u)
        {
            sNdsFighterAnimAuditMotion++;
            continue;
        }
        return;
    }

    if (gNdsFighterAnimAuditCompleteCount == 0u)
    {
        gNdsFighterAnimAuditCompleteCount = 1u;
        ndsFighterAnimAuditCompleteMarker((u32)kind,
                                          sNdsFighterAnimAuditMotionCount);
    }
}

static sb32 ndsFighterAnimAuditCheckComplete(FTStruct *fp)
{
    u32 state = ndsFighterAnimAuditState(fp);

    if (sNdsFighterAnimAuditPlayed == 1u)
    {
        sNdsFighterAnimAuditFirstState = state;
    }
    if (sNdsFighterAnimAuditPlayed > 1u &&
        state == sNdsFighterAnimAuditFirstState)
    {
        return NDS_FIGHTER_ANIM_AUDIT_FLAG_DURATION_LOOP;
    }
    if (sNdsFighterAnimAuditPlayed >= 600u)
    {
        return NDS_FIGHTER_ANIM_AUDIT_FLAG_DURATION_TIMEOUT;
    }
    if (sNdsFighterAnimAuditPlayed > 1u &&
        sNdsFighterAnimAuditTarget->anim_frame <= 0.0F)
    {
        return NDS_FIGHTER_ANIM_AUDIT_FLAG_DURATION_END;
    }
    return 0u;
}

static void ndsFighterAnimAuditFinishMotion(FTStruct *fp, u32 duration_flag)
{
    s32 kind = ndsFighterAnimAuditKindIndex(fp);
    u32 motion = sNdsFighterAnimAuditMotion;
    u32 played = (sNdsFighterAnimAuditPlayed > 0xffffu) ?
        0xffffu : sNdsFighterAnimAuditPlayed;

    gNdsFighterAnimAuditExpectedFrames[kind][motion] =
        (sNdsFighterAnimAuditExpected > 0xffffu) ?
            0xffffu : (u16)sNdsFighterAnimAuditExpected;
    gNdsFighterAnimAuditPlayedFrames[kind][motion] = (u16)played;
    gNdsFighterAnimAuditFlags[kind][motion] |= (u16)duration_flag;
    if ((sNdsFighterAnimAuditExpected != 0u) &&
        (sNdsFighterAnimAuditExpected != played))
    {
        gNdsFighterAnimAuditFlags[kind][motion] |=
            NDS_FIGHTER_ANIM_AUDIT_FLAG_DURATION_MISMATCH;
    }
    if (gNdsFighterMarioFoxExternalFixupFailCount !=
        sNdsFighterAnimAuditExternFailStart)
    {
        gNdsFighterAnimAuditFlags[kind][motion] |=
            NDS_FIGHTER_ANIM_AUDIT_FLAG_EXTERN_FAIL;
    }
    if (gNdsFighterNaturalMotionFigatreeAnimInvalidCount !=
        sNdsFighterAnimAuditFigatreeInvalidStart)
    {
        gNdsFighterAnimAuditFlags[kind][motion] |=
            NDS_FIGHTER_ANIM_AUDIT_FLAG_FIGATREE_INVALID;
    }
    if (gNdsFighterNaturalMotionUnsafeCount !=
        sNdsFighterAnimAuditUnsafeStart)
    {
        gNdsFighterAnimAuditFlags[kind][motion] |=
            NDS_FIGHTER_ANIM_AUDIT_FLAG_UNSAFE;
    }
    sNdsFighterAnimAuditMotion++;
    ndsFighterAnimAuditBeginNext(fp);
}

static void ndsFighterAnimAuditUpdate(GObj *fighter_gobj)
{
    FTStruct *fp;
    u32 duration_flag;

    if (NDS_FIGHTER_ANIM_CYCLER_KIND < 0)
    {
        return;
    }
    fp = ftGetStruct(fighter_gobj);
    if ((fp == NULL) || (fp->fkind != NDS_FIGHTER_ANIM_CYCLER_KIND) ||
        (fp->data == NULL) || (fp->data->mainmotion == NULL) ||
        (fp->figatree_heap == NULL))
    {
        return;
    }
    if (sNdsFighterAnimAuditTarget == NULL)
    {
        sNdsFighterAnimAuditTarget = fighter_gobj;
        if (gSCManagerBattleState != NULL)
        {
            gSCManagerBattleState->time_limit = SCBATTLE_TIMELIMIT_INFINITE;
        }
        sNdsFighterAnimAuditMotionCount =
            (u32)fp->data->mainmotion_array_count;
        sNdsFighterAnimAuditStarted = TRUE;
        ndsFighterAnimAuditBeginNext(fp);
        return;
    }
    if ((fighter_gobj != sNdsFighterAnimAuditTarget) ||
        (gNdsFighterAnimAuditCompleteCount != 0u))
    {
        return;
    }

    ndsFighterAnimAuditFreeze(fp);
    sNdsFighterAnimAuditPlayed++;
    duration_flag = ndsFighterAnimAuditCheckComplete(fp);
    if (duration_flag != 0u)
    {
        sNdsFighterAnimAuditMotionComplete = TRUE;
        sNdsFighterAnimAuditDurationFlag = duration_flag;
    }
    if (sNdsFighterAnimAuditCapturePending != FALSE)
    {
        ndsFighterAnimAuditCaptureMarker((u32)ndsFighterAnimAuditKindIndex(fp),
                                         sNdsFighterAnimAuditMotion);
        gNdsFighterAnimAuditCaptureCount++;
        sNdsFighterAnimAuditCapturePending = FALSE;
        sNdsFighterAnimAuditCaptured = TRUE;
        if (sNdsFighterAnimAuditMotionComplete != FALSE)
        {
            ndsFighterAnimAuditFinishMotion(
                fp, sNdsFighterAnimAuditDurationFlag);
        }
        return;
    }
    if ((sNdsFighterAnimAuditCaptured == FALSE) &&
        ((sNdsFighterAnimAuditPlayed >= 8u) || (duration_flag != 0u)))
    {
        sNdsFighterAnimAuditCapturePending = TRUE;
        return;
    }
    if (duration_flag != 0u)
    {
        ndsFighterAnimAuditFinishMotion(fp, duration_flag);
    }
}
#endif
