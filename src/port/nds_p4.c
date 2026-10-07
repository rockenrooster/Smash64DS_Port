/*
 * P4 runtime: Smash Remix fighters as setup parent + resolved donor content.
 * See include/nds/nds_p4.h for the identity model and docs/P4/P4_STATUS.md for
 * the source adapter that generates each content's data from the user's ROM.
 */
#include <nds/nds_p4.h>

#if NDS_P4

#include <string.h>
#include <sc/scene.h>
#include <sys/objman.h>

#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))

DObj *gcGetTreeDObjNext(DObj *dobj);
void gcSetAnimSpeed(GObj *gobj, f32 anim_speed);

u8 gNdsP4PlayerContent[GMCOMMON_PLAYERS_MAX];
f32 gNdsP4TranslationMultiplier[GMCOMMON_PLAYERS_MAX] = { 1.0F, 1.0F, 1.0F, 1.0F };
/* Remix custom commands no P4 content has needed yet: counted, never silent. */
__attribute__((used)) volatile u32 gNdsP4UnportedMotionEvents;
__attribute__((used)) volatile u32 gNdsP4UnportedMotionEventLast;

#if NDS_P4_FALCO
extern FTData gNdsP4FalcoData;
extern const NDSP4StatusOverride gNdsP4FalcoStatusOverrides[];
extern const u32 gNdsP4FalcoStatusOverrideCount;
extern const NDSP4RelocAsset gNdsP4FalcoRelocAssets[];
extern const u32 gNdsP4FalcoRelocAssetCount;
extern const FTFileSize gNdsP4FalcoFileSize;
extern const u16 gNdsP4FalcoAnims[];
extern const u32 gNdsP4FalcoAnimCount;
void ndsP4FalcoOnStatus(GObj *fighter_gobj, s32 status_id);
#endif

static const NDSP4Fighter sNdsP4Fighters[NDS_P4_CONTENT_LIMIT] = {
#if NDS_P4_FALCO
    [NDS_P4_CONTENT_FALCO] = {
        "Falco", nFTKindFox, &gNdsP4FalcoData,
        gNdsP4FalcoStatusOverrides, &gNdsP4FalcoStatusOverrideCount,
        gNdsP4FalcoRelocAssets, &gNdsP4FalcoRelocAssetCount,
        &gNdsP4FalcoFileSize, gNdsP4FalcoAnims, &gNdsP4FalcoAnimCount,
        ndsP4FalcoOnStatus,
    },
#endif
};

/* Menu-motion scripts are absolute pointers in the source (opening statuses
 * take motion_desc->offset as the script itself); the generator emits them as
 * offsets into the content's motion file, rebased here per load. */
static void *sNdsP4MenuScriptBase[NDS_P4_CONTENT_LIMIT];

const NDSP4Fighter *ndsP4Fighter(u32 content)
{
    if ((content == 0u) || (content >= NDS_P4_CONTENT_LIMIT) ||
        (sNdsP4Fighters[content].data == NULL))
    {
        return NULL;
    }
    return &sNdsP4Fighters[content];
}

FTData *ndsP4PlayerData(s32 player)
{
    const NDSP4Fighter *f;

    if ((player < 0) || (player >= GMCOMMON_PLAYERS_MAX))
    {
        return NULL;
    }
    f = ndsP4Fighter(gNdsP4PlayerContent[player]);
    return (f != NULL) ? f->data : NULL;
}

sb32 ndsP4ParentFilesNeeded(s32 fkind)
{
    s32 player;
    sb32 child = FALSE;

    if ((gSCManagerSceneData.scene_curr != nSCKindVSBattle) ||
        (gSCManagerBattleState == NULL))
    {
        return TRUE;
    }
    for (player = 0; player < GMCOMMON_PLAYERS_MAX; player++)
    {
        if ((gSCManagerBattleState->players[player].pkind == nFTPlayerKindNot) ||
            (gSCManagerBattleState->players[player].fkind != fkind))
        {
            continue;
        }
        if (ndsP4Fighter(gNdsP4PlayerContent[player]) == NULL)
        {
            return TRUE;
        }
        child = TRUE;
    }
    return (child != FALSE) ? FALSE : TRUE;
}

void ndsP4SetupFileSizes(u32 data_flags)
{
    u32 c;

    for (c = 1u; c < NDS_P4_CONTENT_LIMIT; c++)
    {
        const NDSP4Fighter *f = ndsP4Fighter(c);
        u32 largest = 0u;

        if (f == NULL)
        {
            continue;
        }
        *f->data->p_file_main = NULL;
        f->data->file_main_size = f->file_size->main;
        if ((data_flags & FTDATA_FLAG_MAINMOTION) &&
            (f->file_size->mainmotion_largest_anim > largest))
        {
            largest = f->file_size->mainmotion_largest_anim;
        }
        if ((data_flags & FTDATA_FLAG_SUBMOTION) &&
            (f->file_size->submotion_largest_anim > largest))
        {
            largest = f->file_size->submotion_largest_anim;
        }
        f->data->file_anim_size = largest;
        if (gFTManagerFigatreeHeapSize < largest)
        {
            gFTManagerFigatreeHeapSize = largest;
        }
    }
    /* The battle scene sizes each player's figatree heap from the kind it
     * passes, which for a P4 player is the parent (ftManagerAllocFigatreeHeapKind
     * has no player argument). Raise the parent's row to its largest selected
     * child for this battle only; ftManagerAllocFighter rewrote every legacy row
     * from the source census just before this runs. */
    if (gSCManagerBattleState != NULL)
    {
        s32 player;

        for (player = 0; player < GMCOMMON_PLAYERS_MAX; player++)
        {
            const NDSP4Fighter *f = ndsP4Fighter(gNdsP4PlayerContent[player]);
            FTData *parent;

            if ((f == NULL) ||
                (gSCManagerBattleState->players[player].pkind == nFTPlayerKindNot))
            {
                continue;
            }
            parent = dFTManagerDataFiles[f->parent_kind];
            if (parent->file_anim_size < f->data->file_anim_size)
            {
                parent->file_anim_size = f->data->file_anim_size;
            }
        }
    }
}

void ndsP4OnSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    /* Remix Command.asm change_action_: every action change restores the
     * top-joint translation multiplier to 1.0 for that port. */
    gNdsP4TranslationMultiplier[fp->player & 3u] = 1.0F;
}

void ndsP4ApplyStatusOverrides(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    const NDSP4Fighter *f = ndsP4Fighter(fp->nds_p4_content);
    u32 i;

    if (f == NULL)
    {
        return;
    }
    for (i = 0u; i < *f->override_count; i++)
    {
        const NDSP4StatusOverride *o = &f->overrides[i];

        if (o->status_id == (u32)status_id)
        {
            fp->proc_update = o->procs[0];
            fp->proc_interrupt = o->procs[1];
            fp->proc_physics = o->procs[2];
            fp->proc_map = o->procs[3];
            break;
        }
    }
    if (f->on_status != NULL)
    {
        f->on_status(fighter_gobj, status_id);
    }
}

void ndsP4BindMenuScripts(u32 content)
{
    const NDSP4Fighter *f = ndsP4Fighter(content);
    FTMotionDesc *descs;
    uintptr_t old_base;
    uintptr_t new_base;
    s32 i;

    if ((f == NULL) || (f->data->p_file_mainmotion == NULL) ||
        (*f->data->p_file_mainmotion == NULL))
    {
        return;
    }
    new_base = (uintptr_t)*f->data->p_file_mainmotion;
    old_base = (uintptr_t)sNdsP4MenuScriptBase[content];
    if (old_base == new_base)
    {
        return;
    }
    descs = f->data->submotion->motion_desc;
    for (i = 0; i < *f->data->submotion_array_count; i++)
    {
        if ((u32)descs[i].offset != 0x80000000u)
        {
            descs[i].offset = (intptr_t)((uintptr_t)descs[i].offset -
                                         old_base + new_base);
        }
    }
    sNdsP4MenuScriptBase[content] = (void *)new_base;
}

const char *ndsP4RelocAssetPath(u32 file_id)
{
    u32 c;

    for (c = 1u; c < NDS_P4_CONTENT_LIMIT; c++)
    {
        const NDSP4Fighter *f = ndsP4Fighter(c);
        u32 i;

        if (f == NULL)
        {
            continue;
        }
        for (i = 0u; i < *f->asset_count; i++)
        {
            if (f->assets[i].file_id == file_id)
            {
                return f->assets[i].path;
            }
        }
    }
    return NULL;
}

static u32 ndsP4FindAnim(u32 asset_id)
{
    u32 c;

    if ((asset_id < 0x854u) || (asset_id >= 0x8000u))
    {
        return 0u;
    }
    for (c = 1u; c < NDS_P4_CONTENT_LIMIT; c++)
    {
        const NDSP4Fighter *f = ndsP4Fighter(c);
        u32 i;

        if (f == NULL)
        {
            continue;
        }
        for (i = 0u; i < *f->anim_count; i++)
        {
            if ((f->anims[i] & 0x7FFFu) == asset_id)
            {
                return 1u | ((f->anims[i] >> 15) << 1);
            }
        }
    }
    return 0u;
}

sb32 ndsP4IsFighterAnim(u32 asset_id)
{
    return (ndsP4FindAnim(asset_id) != 0u) ? TRUE : FALSE;
}

sb32 ndsP4IsFighterAnimEvent32(u32 asset_id)
{
    return ((ndsP4FindAnim(asset_id) & 2u) != 0u) ? TRUE : FALSE;
}

u32 ndsP4MainAttributesOffset(u32 asset_id)
{
    u32 c;

    for (c = 1u; c < NDS_P4_CONTENT_LIMIT; c++)
    {
        const NDSP4Fighter *f = ndsP4Fighter(c);

        if ((f != NULL) && (f->data->file_main_id == asset_id))
        {
            return (u32)f->data->o_attributes;
        }
    }
    return 0u;
}

/* Upper half-word of an f32, the operand form of Remix's FSM and multiplier
 * commands (`lhu; sll 16`). */
static f32 ndsP4HalfFloat(u32 word)
{
    union { u32 u; f32 f; } v;

    v.u = (word & 0xFFFFu) << 16;
    return v.f;
}

/* Remix 0xD0 SET FRAME SPEED MULTIPLIER (Command.asm fsm_). Flag 0 sets every
 * joint, like gcSetAnimSpeed. Flag 1 leaves the top joint, whose speed is the
 * motion-script clock, and nudges it off exactly 1.0 so ftMainSetStatus's
 * `anim_speed != root speed` test still restores the children afterwards. */
static void ndsP4RemixFrameSpeed(GObj *fighter_gobj, u32 word)
{
    f32 speed = ndsP4HalfFloat(word);
    DObj *root = DObjGetStruct(fighter_gobj);
    DObj *dobj;

    if (((word >> 16) & 0xFFu) == 0u)
    {
        gcSetAnimSpeed(fighter_gobj, speed);
        return;
    }
    if (root->anim_speed == 1.0F)
    {
        union { u32 u; f32 f; } nudge;

        nudge.u = 0x3F800001u;
        root->anim_speed = nudge.f;
    }
    for (dobj = root->child; dobj != NULL; dobj = gcGetTreeDObjNext(dobj))
    {
        dobj->anim_speed = speed;
    }
}

u32 *ndsP4RunRemixMotionEvents(GObj *fighter_gobj, FTMotionScript *ms,
                               sb32 forward)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 *p = ms->p_script;

    while ((p != NULL) && ((*p >> 24) >= 0xD0u))
    {
        u32 word = *p;
        u32 op = word >> 24;
        u32 length = 4u;

        switch (op)
        {
        case 0xD0:
            ndsP4RemixFrameSpeed(fighter_gobj, word);
            break;
        case 0xD3:
            gNdsP4TranslationMultiplier[fp->player & 3u] = ndsP4HalfFloat(word);
            break;
        case 0xD6: /* RANDOM SFX */
        case 0xD9: /* SET ENV COLOR */
        case 0xDC: /* L VOICE SFX */
            length = 8u;
            if (forward == FALSE)
            {
                gNdsP4UnportedMotionEvents++;
                gNdsP4UnportedMotionEventLast = word;
            }
            break;
        case 0xD4: /* SET Y VELOCITY */
        case 0xD5: /* FAST FALL */
        case 0xDA: /* SWITCH DIRECTION */
            if (forward == FALSE)
            {
                gNdsP4UnportedMotionEvents++;
                gNdsP4UnportedMotionEventLast = word;
            }
            break;
        default:
            /* D1 armour, D2 hitbox direction, D7 kinetic, D8 hitbox FGM and
             * DB moveset-file goto run in both tables; E0+ are donor no-ops. */
            if (op <= 0xDCu)
            {
                gNdsP4UnportedMotionEvents++;
                gNdsP4UnportedMotionEventLast = word;
            }
            break;
        }
        p = (u32 *)((uintptr_t)p + length);
        ms->p_script = p;
    }
    return p;
}

#endif /* NDS_P4 */
