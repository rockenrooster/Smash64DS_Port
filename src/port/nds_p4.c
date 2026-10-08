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
void func_ovl2_800EDBA4(DObj *main_dobj);
void ftComputerSetCommandWaitShort(FTStruct *fp, s32 index);
void ftComputerSetCommandImmediate(FTStruct *fp, s32 index);
void gcSetAnimSpeed(GObj *gobj, f32 anim_speed);

u8 gNdsP4PlayerContent[GMCOMMON_PLAYERS_MAX];
f32 gNdsP4TranslationMultiplier[GMCOMMON_PLAYERS_MAX] = { 1.0F, 1.0F, 1.0F, 1.0F };
/* Remix custom commands no P4 content has needed yet: counted, never silent. */
__attribute__((used)) volatile u32 gNdsP4UnportedMotionEvents;
__attribute__((used)) volatile u32 gNdsP4UnportedMotionEventLast;

/* Each content's generated TU (scripts/p4/generate_p4_fighter.py) defines
 * the same family of symbols under its title; one row per content. */
#define NDS_P4_DECLARE(T) \
    extern FTData gNdsP4##T##Data; \
    extern const NDSP4StatusOverride gNdsP4##T##StatusOverrides[]; \
    extern const u32 gNdsP4##T##StatusOverrideCount; \
    extern const NDSP4RelocAsset gNdsP4##T##RelocAssets[]; \
    extern const u32 gNdsP4##T##RelocAssetCount; \
    extern const FTFileSize gNdsP4##T##FileSize; \
    extern const u16 gNdsP4##T##Anims[]; \
    extern const u32 gNdsP4##T##AnimCount; \
    extern const u8 gNdsP4##T##StockGfx[]; \
    extern const u16 gNdsP4##T##StockPalettes[][16]; \
    extern const u32 gNdsP4##T##StockPaletteCount; \
    extern const NDSP4SpriteDesc gNdsP4##T##Sprites[]; \
    extern const u32 gNdsP4##T##SpriteCount; \
    extern const NDSP4Present gNdsP4##T##Present; \
    extern const NDSP4SwordTrail gNdsP4##T##SwordTrails[]; \
    extern const u32 gNdsP4##T##SwordTrailCount;
#define NDS_P4_ROW(T, title, parent, on_status_hook, computer_rows) \
    { \
        .name = (title), .parent_kind = (parent), \
        .data = &gNdsP4##T##Data, \
        .overrides = gNdsP4##T##StatusOverrides, \
        .override_count = &gNdsP4##T##StatusOverrideCount, \
        .assets = gNdsP4##T##RelocAssets, \
        .asset_count = &gNdsP4##T##RelocAssetCount, \
        .file_size = &gNdsP4##T##FileSize, \
        .anims = gNdsP4##T##Anims, .anim_count = &gNdsP4##T##AnimCount, \
        .on_status = (on_status_hook), \
        .computer = (computer_rows), \
        .stock_gfx = gNdsP4##T##StockGfx, \
        .stock_palettes = gNdsP4##T##StockPalettes, \
        .stock_palette_count = &gNdsP4##T##StockPaletteCount, \
        .sprites = gNdsP4##T##Sprites, \
        .sprite_count = &gNdsP4##T##SpriteCount, \
        .present = &gNdsP4##T##Present, \
        .sword_trails = gNdsP4##T##SwordTrails, \
        .sword_trail_count = &gNdsP4##T##SwordTrailCount, \
    }

/* Every compiled content (nds_p4_contents.h). Its native hooks live in
 * src/port/nds_p4_<name>.c when it has any: ndsP4<Title>OnStatus (donor init
 * hooks after the status callbacks) and gNdsP4<Title>Computer (CPU rows).
 * Both are weak, so a content without them gets NULL. */
#define NDS_P4_DECLARE_ROW(id_, T_, N_, n_, parent_, model_, main_) \
    NDS_P4_DECLARE(T_) \
    void ndsP4##T_##OnStatus(GObj *fighter_gobj, s32 status_id) \
        __attribute__((weak)); \
    extern const NDSP4Computer gNdsP4##T_##Computer __attribute__((weak));
NDS_P4_CONTENT_ROWS(NDS_P4_DECLARE_ROW)
#undef NDS_P4_DECLARE_ROW

static const NDSP4Fighter sNdsP4Fighters[NDS_P4_CONTENT_LIMIT] = {
#define NDS_P4_FIGHTER_ROW(id_, T_, N_, n_, parent_, model_, main_) \
    [(id_)] = NDS_P4_ROW(T_, #T_, parent_, ndsP4##T_##OnStatus, \
                         &gNdsP4##T_##Computer),
    NDS_P4_CONTENT_ROWS(NDS_P4_FIGHTER_ROW)
#undef NDS_P4_FIGHTER_ROW
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

u32 ndsP4PlayerContent(u32 player)
{
    GObj *gobj;

    if ((player >= GMCOMMON_PLAYERS_MAX) || (gSCManagerBattleState == NULL))
    {
        return 0u;
    }
    gobj = gSCManagerBattleState->players[player].fighter_gobj;
    if ((gobj == NULL) || (gobj->user_data.p == NULL))
    {
        return 0u;
    }
    return ((FTStruct *)gobj->user_data.p)->nds_p4_content;
}

static u8 sNdsP4PreviewContent[GMCOMMON_PLAYERS_MAX];

void ndsP4SetPreviewContent(u32 slot, u32 content)
{
    if (slot < GMCOMMON_PLAYERS_MAX)
    {
        sNdsP4PreviewContent[slot] = (u8)content;
    }
}

u32 ndsP4MatchContent(s32 player)
{
    if ((player < 0) || (player >= GMCOMMON_PLAYERS_MAX))
    {
        return 0u;
    }
    switch (gSCManagerSceneData.scene_curr)
    {
    case nSCKindVSBattle:
    case nSCKindVSResults:
        return gNdsP4PlayerContent[player];
    default:
        return 0u;
    }
}

u32 ndsP4MakeContent(s32 player, s32 fkind)
{
    const NDSP4Fighter *f;
    u32 content;

    if ((player < 0) || (player >= GMCOMMON_PLAYERS_MAX))
    {
        return 0u;
    }
    content = (gSCManagerSceneData.scene_curr == nSCKindPlayersVS) ?
        sNdsP4PreviewContent[player] : ndsP4MatchContent(player);
    f = ndsP4Fighter(content);
    return ((f != NULL) && (f->parent_kind == fkind)) ? content : 0u;
}

sb32 ndsP4ParentFilesNeeded(s32 fkind)
{
    const SCBattleState *state;
    s32 player;
    sb32 child = FALSE;

    /* The two scenes whose fighters are exactly the match's players: the
     * battle, and the Results podium built from the transfer state. */
    if (gSCManagerSceneData.scene_curr == nSCKindVSBattle)
    {
        state = gSCManagerBattleState;
    }
    else if (gSCManagerSceneData.scene_curr == nSCKindVSResults)
    {
        state = &gSCManagerTransferBattleState;
    }
    else
    {
        return TRUE;
    }
    if (state == NULL)
    {
        return TRUE;
    }
    for (player = 0; player < GMCOMMON_PLAYERS_MAX; player++)
    {
        if ((state->players[player].pkind == nFTPlayerKindNot) ||
            (state->players[player].fkind != fkind))
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
            const NDSP4Fighter *f = ndsP4Fighter(ndsP4MatchContent(player));
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

const NDSP4Computer *ndsP4Computer(const FTStruct *fp)
{
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(fp));

    return (f != NULL) ? f->computer : NULL;
}

void ndsP4ComputerRecover(FTStruct *fp)
{
    const NDSP4Computer *c = ndsP4Computer(fp);

    if ((c != NULL) && (c->recover != NULL))
    {
        c->recover(fp);
    }
}

/* Remix's added input routines (AI.asm), as assembled (Remix 5e04fe7).
 * Their macros spell some steps oddly -- UNPRESS_A() is a Z release and
 * UNPRESS_Z() a move-toward-target step before one, MULTI_SHINE's "press
 * B" presses A, then B -- and the bytes are what Remix's CPUs run. */
static const struct
{
    u8 multi_shine[19];
    u8 nsp_towards[13];
    u8 fair[13];
    u8 bair[13];
    u8 dash_attack[13];
    u8 null_routine[1];
} sNdsP4ComputerScripts = {
    { 0xA0, 0x00, 0xB1, 0x00, 0xA0, 0x7F, 0xB1, 0x00, 0xA0, 0x00, 0xB1, 0x35,
      0xA0, 0x00, 0xB0, 0xB0, 0x02, 0x21, 0xFF },
    { 0xC0, 0x50, 0x50, 0x30, 0xB0, 0x00, 0xA0, 0x7F, 0x21, 0xA0, 0x00, 0x30,
      0xFF },
    { 0xC0, 0x50, 0x50, 0x30, 0xB0, 0x00, 0xA0, 0x83, 0x01, 0xA0, 0x7F, 0x10,
      0xFF },
    { 0xC0, 0x50, 0x50, 0x30, 0xB0, 0x00, 0xA0, 0x84, 0x01, 0xA0, 0x7F, 0x10,
      0xFF },
    { 0xC0, 0x50, 0x50, 0x30, 0xB0, 0x00, 0xA5, 0x7F, 0x01, 0xB0, 0x00, 0x10,
      0xFF },
    { 0xFF },
};

static const u8 *ndsP4ComputerScript(s32 index)
{
    switch (index)
    {
    case nNDSP4ComputerInputMultiShine:
        return sNdsP4ComputerScripts.multi_shine;
    case nNDSP4ComputerInputNSPTowards:
        return sNdsP4ComputerScripts.nsp_towards;
    case nNDSP4ComputerInputFair:
        return sNdsP4ComputerScripts.fair;
    case nNDSP4ComputerInputBair:
        return sNdsP4ComputerScripts.bair;
    case nNDSP4ComputerInputDashAttack:
        return sNdsP4ComputerScripts.dash_attack;
    default:
        return sNdsP4ComputerScripts.null_routine;
    }
}

/* Remix extends the command table past the vanilla 0x31 (AI.asm
 * extend_commands_1/3); the vanilla setter keeps its wait and its random
 * draws, and the routine replaces the vanilla script it set. */
void ndsP4ComputerSetCommandWaitShort(FTStruct *fp, s32 index)
{
    if (index < NDS_P4_COMPUTER_INPUT_BASE)
    {
        ftComputerSetCommandWaitShort(fp, index);
        return;
    }
    ftComputerSetCommandWaitShort(fp, nFTComputerInputStickN);
    fp->computer.p_command = (u8 *)ndsP4ComputerScript(index);
}

void ndsP4ComputerSetCommandImmediate(FTStruct *fp, s32 index)
{
    if (index < NDS_P4_COMPUTER_INPUT_BASE)
    {
        ftComputerSetCommandImmediate(fp, index);
        return;
    }
    ftComputerSetCommandImmediate(fp, nFTComputerInputStickN);
    fp->computer.p_command = (u8 *)ndsP4ComputerScript(index);
}

/* AI.asm extend_stick_x_commands: in a stick-X step, 0x81/0x82 point the
 * stick away from the target at 80/40 and 0x83/0x84 forward/back from the
 * facing at 80. Only Remix routines use them (vanilla scripts keep their
 * raw values), so this decodes the run a Remix routine executes this tick
 * (the interpreter runs when input_wait counts 1 -> 0) and returns the
 * stick X it ends on when the run's last X write is one of these; the
 * caller stores it after the run. Nothing in a run reads the stick. */
s32 ndsP4ComputerStickX(const FTStruct *fp)
{
    const FTComputer *com = &fp->computer;
    const u8 *p = com->p_command;
    u32 value = 0u;

    if ((com->input_wait != 1) ||
        (p < (const u8 *)&sNdsP4ComputerScripts) ||
        (p >= (const u8 *)(&sNdsP4ComputerScripts + 1)))
    {
        return NDS_P4_COMPUTER_STICK_KEEP;
    }
    for (;;)
    {
        u32 command = *p++;
        u32 wait = 0u;

        if (command < FTCOMPUTER_COMMAND_DEFAULT_MAX)
        {
            wait = command & FTCOMPUTER_COMMAND_TIMER_MASK;

            switch (command & FTCOMPUTER_COMMAND_OPCODE_MASK)
            {
            case FTCOMPUTER_COMMAND_STICK_X_TILT:
                value = *p++;
                break;
            case FTCOMPUTER_COMMAND_STICK_Y_TILT:
                p++;
                break;
            case FTCOMPUTER_COMMAND_MOVEAUTO:
            case FTCOMPUTER_COMMAND_STICK_X_VAR:
                value = 0u;
                break;
            }
        }
        else if (command == FTCOMPUTER_COMMAND_DEFAULT_MAX)
        {
            wait = *p++;
        }
        else if (command == FTCOMPUTER_COMMAND_END)
        {
            break;
        }
        if (wait != 0u)
        {
            break;
        }
    }
    switch (value)
    {
    case 0x81:
    case 0x82:
    {
        s32 range = (value == 0x81) ? 80 : 40;

        return (fp->joints[nFTPartsJointTopN]->translate.vec.f.x <
                com->target_pos.x) ? -range : range;
    }
    case 0x83:
        return (fp->lr >= 0) ? 80 : -80;
    case 0x84:
        return (fp->lr >= 0) ? -80 : 80;
    default:
        return NDS_P4_COMPUTER_STICK_KEEP;
    }
}

/* AI.asm cpu_post_process (0x8013A884): its checks for every CPU, then the
 * character's row. Of the shared checks, only "a dash attack that cannot
 * start here is dropped" applies below level 10 without Remix's improved-AI
 * toggle, and only Remix routines issue a dash attack. */
void ndsP4ComputerPostProcess(FTStruct *fp)
{
    const NDSP4Computer *c = ndsP4Computer(fp);

    if (c == NULL)
    {
        return;
    }
    if ((fp->computer.p_command == sNdsP4ComputerScripts.dash_attack) &&
        (fp->status_id != nFTCommonStatusWait) &&
        (fp->status_id != nFTCommonStatusRun))
    {
        ndsP4ComputerSetCommandImmediate(fp, nNDSP4ComputerInputNull);
    }
    if (c->post_process != NULL)
    {
        c->post_process(fp);
    }
}

const NDSP4SwordTrail *ndsP4SwordTrail(const FTStruct *fp, u32 id)
{
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(fp));
    u32 i;

    if (f == NULL)
    {
        return NULL;
    }
    for (i = 0u; i < *f->sword_trail_count; i++)
    {
        if (f->sword_trails[i].id == id)
        {
            return &f->sword_trails[i];
        }
    }
    return NULL;
}

void ndsP4UpdateSwordTrail(FTStruct *fp)
{
    const NDSP4SwordTrail *row =
        ndsP4SwordTrail(fp, (u32)fp->afterimage.is_itemswing);
    FTAfterImage *desc;
    FTParts *parts;
    DObj *joint;

    if (row == NULL)
    {
        return;
    }
    /* initial_setup_: the row's model part instead of Link's joint 11, then
     * the source's Link-sword step (ftmain.c, ftMainProcParams' afterimage
     * switch); axis_setup_: the row's matrix row instead of Z. */
    joint = fp->joints[row->model_part + nFTPartsJointCommonStart];
    if ((joint == NULL) || (row->axis > 2u))
    {
        return;
    }
    parts = joint->user_data.p;
    func_ovl2_800EDBA4(joint);
    desc = &fp->afterimage.desc[fp->afterimage.desc_id];
    desc->translate_x = parts->mtx_translate[3][0];
    desc->translate_y = parts->mtx_translate[3][1];
    desc->translate_z = parts->mtx_translate[3][2];
    desc->vec.x = parts->mtx_translate[row->axis][0];
    desc->vec.y = parts->mtx_translate[row->axis][1];
    desc->vec.z = parts->mtx_translate[row->axis][2];

    if (fp->afterimage.desc_id == 2)
    {
        fp->afterimage.desc_id = 0;
    }
    else fp->afterimage.desc_id++;

    if (fp->afterimage.drawstatus <= 2)
    {
        fp->afterimage.drawstatus++;
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
