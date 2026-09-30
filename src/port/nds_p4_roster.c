/* Native P4 identity/data extension. Assets still use the existing reloc,
 * preview, figatree and owner-image loaders. This file owns no loader. */
#include <ft/ftcomputer.h>
#include <nds/nds_p4_runtime.h>
#include <sys/debug.h>

#if NDS_P4_METAKNIGHT
#include <nds/nds_metaknight.h>
#include <nds/generated/nds_p4_audio.generated.h>
#include <nds/generated/nds_p4_runtime.generated.inc>
#endif

u32 ndsP4MetaKnightAnnouncerID(void)
{
#if NDS_P4_METAKNIGHT
    return NDS_P4_METAKNIGHT_ANNOUNCER_FGM;
#else
    return 0u;
#endif
}

sb32 ndsP4IsMetaKnight(s32 kind)
{
#if NDS_P4_METAKNIGHT
    return (kind == (s32)NDS_P4_RUNTIME_METAKNIGHT) ? TRUE : FALSE;
#else
    (void)kind;
    return FALSE;
#endif
}

FTData *ndsP4GetFighterData(s32 kind)
{
    if ((kind >= 0) && (kind < nFTKindEnumCount))
    {
        return dFTManagerDataFiles[kind];
    }
#if NDS_P4_METAKNIGHT
    if (ndsP4IsMetaKnight(kind)) return &sNdsP4MetaKnightData;
#endif
    return NULL;
}

FTStatusDesc *ndsP4GetStatusDesc(s32 kind, s32 status)
{
#if NDS_P4_METAKNIGHT
    if (ndsP4IsMetaKnight(kind) && (status >= 0) &&
        ((u32)status < ARRAY_COUNT(sNdsP4MetaKnightStatus)))
    {
        return &sNdsP4MetaKnightStatus[status];
    }
#else
    (void)kind;
    (void)status;
#endif
    /* Reaching an unsupported action is corrupt state, not an empty action.
     * Callers select this accessor only for an admitted new fighter. */
    syDebugPrintf("P4: unadmitted status kind=%d status=%d\n", kind, status);
    while (TRUE) { }
}

FTComputerAttack *ndsP4GetComputerAttacks(s32 kind)
{
    extern FTComputerAttack *dFTComputerAttackList[];

    if ((kind >= 0) && (kind < nFTKindEnumCount))
    {
        return dFTComputerAttackList[kind];
    }
#if NDS_P4_METAKNIGHT
    if (ndsP4IsMetaKnight(kind)) return sNdsP4MetaKnightComputerAttacks;
#endif
    syDebugPrintf("P4: unadmitted CPU fighter kind=%d\n", kind);
    while (TRUE) { }
}

u8 *ndsP4GetComputerInputScript(s32 input)
{
    extern u8 *dFTComputerPlayerInputScripts[];

    if ((input >= 0) && (input <= nFTComputerInputNessSpecialHiAim))
    {
        return dFTComputerPlayerInputScripts[input];
    }
#if NDS_P4_METAKNIGHT
    switch (input)
    {
    case NDS_P4_CPU_FAIR: return sNdsP4ComputerFAir;
    case NDS_P4_CPU_BAIR: return sNdsP4ComputerBAir;
    case NDS_P4_CPU_DASH_ATTACK: return sNdsP4ComputerDashAttack;
    }
#endif
    syDebugPrintf("P4: unadmitted CPU input=%d\n", input);
    while (TRUE) { }
}

s32 ndsP4ReplaceStatus(s32 kind, s32 status)
{
#if NDS_P4_METAKNIGHT
    if (ndsP4IsMetaKnight(kind))
    {
        /* The linked source's action_replace_map, not donor-parent actions. */
        if (status == nFTCommonStatusAttack11) return nNDSMetaKnightStatusJabStart;
        if (status == nFTCommonStatusAttackS3) return nNDSMetaKnightStatusTiltF;
    }
#else
    (void)kind;
#endif
    return status;
}

void ndsP4ResetFighterData(u32 data_flags)
{
#if NDS_P4_METAKNIGHT
    FTData *data = &sNdsP4MetaKnightData;
    u32 size = 0u;

    /* Match ftManagerAllocFighter's existing per-scene metadata reset. Each
     * live fighter continues to allocate its own mutable state and figatree. */
    *data->p_file_main = NULL;
    data->file_main_size = NDS_P4_MAIN_FILE_SIZE;
    if (data_flags & FTDATA_FLAG_MAINMOTION) size = NDS_P4_MAIN_ANIM_SIZE;
    if ((data_flags & FTDATA_FLAG_SUBMOTION) && (size < NDS_P4_MENU_ANIM_SIZE))
        size = NDS_P4_MENU_ANIM_SIZE;
    data->file_anim_size = size;
#else
    (void)data_flags;
#endif
}

void *ndsP4GetMenuEventScript(FTStruct *fp, FTMotionDesc *motion)
{
    if (motion->offset == (intptr_t)0x80000000u) return NULL;
    if (ndsP4IsMetaKnight(fp->fkind))
    {
        return (void *)((uintptr_t)*fp->data->p_file_submotion + motion->offset);
    }
    return (void *)motion->offset;
}

void ndsP4BindFighterMotionData(s32 kind)
{
#if NDS_P4_METAKNIGHT
    if (ndsP4IsMetaKnight(kind))
    {
        FTData *data = &sNdsP4MetaKnightData;
        uintptr_t base = (uintptr_t)*data->p_file_mainmotion;

        if (base == 0u)
        {
            syDebugPrintf("P4: Meta Knight motion closure is absent\n");
            while (TRUE) { }
        }
        data->mainmotion = (FTMotionDescArray *)(base + NDS_P4_MAIN_TABLE_OFFSET);
        data->submotion = (FTMotionDescArray *)(base + NDS_P4_MENU_TABLE_OFFSET);
    }
#else
    (void)kind;
#endif
}

FTOpeningDesc *ndsP4GetOpeningDescs(s32 kind)
{
    if ((kind >= 0) && (kind < nFTKindEnumCount)) return D_ovl1_80390D20[kind];
#if NDS_P4_METAKNIGHT
    if (ndsP4IsMetaKnight(kind))
    {
        /* The donor's 15 menu rows contain no extra opening-status array.
         * This is the source no-motion opening descriptor, also used by
         * legacy fighters without an opening-scene animation. */
        static FTOpeningDesc no_opening[] = { { -1, NULL } };
        return no_opening;
    }
#endif
    syDebugPrintf("P4: unadmitted opening fighter kind=%d\n", kind);
    while (TRUE) { }
}

s32 ndsP4GetThrownScriptColumn(s32 victim_kind)
{
    if ((victim_kind >= 0) && (victim_kind < nFTKindEnumCount)) return victim_kind;
#if NDS_P4_METAKNIGHT
    /* Character.define_character explicitly initializes f_thrown_action,
     * b_thrown_action and falcon_dive_id from the JIGGLYPUFF parent. This
     * translates the source throw-script column only: the victim's kind,
     * attributes, animation descriptors and own skeleton remain Meta Knight. */
    if (ndsP4IsMetaKnight(victim_kind)) return nFTKindPurin;
#endif
    syDebugPrintf("P4: unadmitted throw-script victim=%d\n", victim_kind);
    while (TRUE) { }
}
