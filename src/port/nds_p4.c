/*
 * P4 runtime: Smash Remix fighters as setup parent + resolved donor content.
 * See include/nds/nds_p4.h for the identity model and docs/P4/P4_STATUS.md for
 * the source adapter that generates each content's data from the user's ROM.
 */
#include <nds/nds_p4.h>

#if NDS_P4

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_scene_manager.h>
#include <nds/nds_startup.h>
#include <macros.h>
#include <sc/scene.h>
#include <sys/objman.h>
#include <sys/audio.h>
#include <sys/taskman.h>

#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))

DObj *gcGetTreeDObjNext(DObj *dobj);
void func_ovl2_800EDBA4(DObj *main_dobj);
void ftComputerSetCommandWaitShort(FTStruct *fp, s32 index);
void ftComputerSetCommandImmediate(FTStruct *fp, s32 index);
void ftComputerSetControlPKThunder(FTStruct *fp);
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
    extern FTStatusDesc gNdsP4##T##SpecialStatusDescs[]; \
    extern const u32 gNdsP4##T##SpecialStatusCount; \
    extern const u32 gNdsP4##T##LabFallbackCount; \
    extern const u16 gNdsP4##T##RelocAssets[]; \
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
    extern const u32 gNdsP4##T##SwordTrailCount; \
    extern const NDSP4Entry gNdsP4##T##Entry; \
    extern const NDSP4SpecialStart gNdsP4##T##SpecialStarts[]; \
    extern const NDSP4Jab gNdsP4##T##Jab; \
    extern const f32 gNdsP4##T##YoshiEgg[]; \
    extern const u16 gNdsP4##T##LabSkipFiles[]; \
    extern const u32 gNdsP4##T##LabSkipFileCount; \
    extern const u8 gNdsP4##T##OpenSpecialMask; \
    extern const NDSP4KindCases gNdsP4##T##KindCases; \
    extern const NDSP4KirbyInhale gNdsP4##T##KirbyInhale; \
    extern const u8 gNdsP4##T##ComputerLongRange; \
    extern const u8 gNdsP4##T##ComputerReflect; \
    extern const NDSP4BakedRef gNdsP4##T##BakedRefs[]; \
    extern const u32 gNdsP4##T##BakedRefCount; \
    extern const NDSP4ShieldPose gNdsP4##T##ShieldPose;
#define NDS_P4_ROW(T, title, parent, on_status_hook, computer_rows, override_rows) \
    { \
        .name = (title), .parent_kind = (parent), \
        .data = &gNdsP4##T##Data, \
        .special_statuses = gNdsP4##T##SpecialStatusDescs, \
        .special_status_count = &gNdsP4##T##SpecialStatusCount, \
        .lab_fallback_count = &gNdsP4##T##LabFallbackCount, \
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
        .entry = &gNdsP4##T##Entry, \
        .special_starts = gNdsP4##T##SpecialStarts, \
        .jab = &gNdsP4##T##Jab, \
        .yoshi_egg = gNdsP4##T##YoshiEgg, \
        .lab_skip_files = gNdsP4##T##LabSkipFiles, \
        .lab_skip_file_count = &gNdsP4##T##LabSkipFileCount, \
        .open_special_mask = &gNdsP4##T##OpenSpecialMask, \
        .kind_cases = &gNdsP4##T##KindCases, \
        .kirby_inhale = &gNdsP4##T##KirbyInhale, \
        .computer_long_range = &gNdsP4##T##ComputerLongRange, \
        .computer_reflect = &gNdsP4##T##ComputerReflect, \
        .baked_refs = gNdsP4##T##BakedRefs, \
        .baked_ref_count = &gNdsP4##T##BakedRefCount, \
        .shield_pose = &gNdsP4##T##ShieldPose, \
        .overrides = (override_rows), \
    }

/* Every compiled content (nds_p4_contents.h). Its native hooks live in
 * src/port/nds_p4_<name>.c when it has any: ndsP4<Title>OnStatus (donor init
 * hooks after the status callbacks), gNdsP4<Title>Computer (CPU routines)
 * and gNdsP4<Title>Overrides (the parent's routines it replaces). All
 * are weak, so a content without them gets NULL. */
#define NDS_P4_DECLARE_ROW(id_, T_, N_, n_, parent_, model_, main_) \
    NDS_P4_DECLARE(T_) \
    void ndsP4##T_##OnStatus(GObj *fighter_gobj, s32 status_id) \
        __attribute__((weak)); \
    extern const NDSP4Computer gNdsP4##T_##Computer __attribute__((weak)); \
    extern const NDSP4Overrides gNdsP4##T_##Overrides __attribute__((weak));
NDS_P4_CONTENT_ROWS(NDS_P4_DECLARE_ROW)
#undef NDS_P4_DECLARE_ROW

static const NDSP4Fighter sNdsP4Fighters[NDS_P4_CONTENT_LIMIT] = {
#define NDS_P4_FIGHTER_ROW(id_, T_, N_, n_, parent_, model_, main_) \
    [(id_)] = NDS_P4_ROW(T_, #T_, parent_, ndsP4##T_##OnStatus, \
                         &gNdsP4##T_##Computer, &gNdsP4##T_##Overrides),
    NDS_P4_CONTENT_ROWS(NDS_P4_FIGHTER_ROW)
#undef NDS_P4_FIGHTER_ROW
};

/* Menu-motion scripts are absolute pointers in the source (opening statuses
 * take motion_desc->offset as the script itself); the generator emits them as
 * offsets into the content's motion file, rebased here per load. */
static void *sNdsP4MenuScriptBase[NDS_P4_CONTENT_LIMIT];

/* Each content's match tables (ndsP4LoadTables) and the heap generation
 * they were read in: a later scene's heap is a new one. */
static const NDSP4TablesHeader *sNdsP4Tables[NDS_P4_CONTENT_LIMIT];
static u32 sNdsP4TablesGeneration[NDS_P4_CONTENT_LIMIT];
static const char *const sNdsP4TableNames[NDS_P4_CONTENT_LIMIT] = {
#define NDS_P4_TABLE_NAME_ROW(id_, T_, N_, n_, parent_, model_, main_) [(id_)] = #n_,
    NDS_P4_CONTENT_ROWS(NDS_P4_TABLE_NAME_ROW)
#undef NDS_P4_TABLE_NAME_ROW
};
/* The content whose tables failed to load (a halt follows), for probes. */
__attribute__((used)) volatile u32 gNdsP4TablesFailure;
static const NDSP4TablesHeader *ndsP4FighterTables(const FTStruct *fp);

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

s32 ndsP4GroundedKind(const FTStruct *fp)
{
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(fp));

    return ((f != NULL) && (f->kind_cases != NULL)) ?
        (s32)f->kind_cases->grounded : (s32)fp->fkind;
}

s32 ndsP4PipeTurnKind(const FTStruct *fp)
{
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(fp));

    return ((f != NULL) && (f->kind_cases != NULL) && (f->kind_cases->pipe_turn != 0u)) ?
        (s32)fp->fkind : NDS_P4_FOREIGN_FKIND;
}

sb32 ndsP4LowDetailBattle(void)
{
    if (gSCManagerSceneData.scene_curr != nSCKindVSBattle)
    {
        return FALSE;
    }
    return ((gSCManagerBattleState->pl_count + gSCManagerBattleState->cp_count) >= 3) ?
        TRUE : FALSE;
}

sb32 ndsP4ContentLowDetailOnly(s32 player)
{
    if (ndsP4MatchContent(player) == 0u)
    {
        return FALSE;
    }
    return ndsP4LowDetailBattle();
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
        /* A new scene: its tables are read again at first use. */
        f->data->mainmotion = NULL;
        sNdsP4Tables[c] = NULL;
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

/* Remix's added input routines (AI.asm add_cpu_input_routine), as
 * assembled: each content's attack list names its own (generated,
 * ComputerScriptBytes). These are the ones hand-ported code issues outside
 * a list: Falco's recovery steers with NSP_TOWARDS, NULL drops a command,
 * and Bowser's Whirling Fortress steers with POINT_STICK_TO_TARGET (custom
 * command 3). Their macros spell some steps oddly -- UNPRESS_A() is a Z
 * release and UNPRESS_Z() a move-toward-target step before one -- and the
 * bytes are what Remix's CPUs run. */
static const struct
{
    u8 nsp_towards[13];
    u8 null_routine[1];
    u8 point_stick_to_target[3];
} sNdsP4ComputerScripts = {
    { 0xC0, 0x50, 0x50, 0x30, 0xB0, 0x00, 0xA0, 0x7F, 0x21, 0xA0, 0x00, 0x30,
      0xFF },
    { 0xFF },
    { 0xFE, 0x03, 0xFF },
};

/* The content's generated routine for a Remix input id, or NULL. */
static const u8 *ndsP4ComputerOwnScript(const FTStruct *fp, s32 index)
{
    const NDSP4TablesHeader *t = ndsP4FighterTables(fp);
    const NDSP4ComputerScript *scripts;
    const u8 *bytes;
    u32 i;

    if (t == NULL)
    {
        return NULL;
    }
    scripts = (const NDSP4ComputerScript *)(const void *)((const u8 *)t + t->script_off);
    bytes = (const u8 *)t + t->script_bytes_off;
    for (i = 0u; i < t->script_count; i++)
    {
        if (scripts[i].input == (u32)index)
        {
            return &bytes[scripts[i].offset];
        }
    }
    return NULL;
}

static const u8 *ndsP4ComputerScript(const FTStruct *fp, s32 index)
{
    const u8 *own = ndsP4ComputerOwnScript(fp, index);

    if (own != NULL)
    {
        return own;
    }
    switch (index)
    {
    case nNDSP4ComputerInputNSPTowards:
        return sNdsP4ComputerScripts.nsp_towards;
    case nNDSP4ComputerInputPointStickToTarget:
        return sNdsP4ComputerScripts.point_stick_to_target;
    default:
        return sNdsP4ComputerScripts.null_routine;
    }
}

const FTComputerAttack *ndsP4ComputerAttacks(const FTStruct *fp)
{
    const NDSP4TablesHeader *t = ndsP4FighterTables(fp);

    return ((t != NULL) && (t->attack_count != 0u)) ?
        (const FTComputerAttack *)(const void *)((const u8 *)t + t->attack_off) : NULL;
}

u32 ndsP4ComputerLongRange(const FTStruct *fp)
{
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(fp));

    return (f != NULL) ? *f->computer_long_range :
                         NDS_P4_COMPUTER_LONG_RANGE_PARENT;
}

u32 ndsP4ComputerReflect(const FTStruct *fp)
{
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(fp));

    return (f != NULL) ? *f->computer_reflect : 0u;
}

s32 ndsP4ComputerReflectKind(const FTStruct *fp)
{
    if (ndsP4Content(fp) == 0u)
    {
        return fp->fkind;
    }
    return ((ndsP4ComputerReflect(fp) & NDS_P4_COMPUTER_REFLECT_FOX) != 0u) ?
        nFTKindFox : NDS_P4_FOREIGN_FKIND;
}

sb32 ndsP4ComputerTargetReflects(const FTStruct *fp)
{
    if (ndsP4Content(fp) != 0u)
    {
        return ((ndsP4ComputerReflect(fp) & NDS_P4_COMPUTER_REFLECT_TABLE) != 0u) ?
            TRUE : FALSE;
    }
    return ((fp->fkind == nFTKindNess) || (fp->fkind == nFTKindFox)) ? TRUE : FALSE;
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
    fp->computer.p_command = (u8 *)ndsP4ComputerScript(fp, index);
}

void ndsP4ComputerSetCommandImmediate(FTStruct *fp, s32 index)
{
    if (index < NDS_P4_COMPUTER_INPUT_BASE)
    {
        ftComputerSetCommandImmediate(fp, index);
        return;
    }
    ftComputerSetCommandImmediate(fp, nFTComputerInputStickN);
    fp->computer.p_command = (u8 *)ndsP4ComputerScript(fp, index);
}

/* Remix's CPU custom commands (AI.asm CUSTOM_COMMANDS): the opcode, then the
 * routine's index. */
#define NDS_P4_COMPUTER_COMMAND_CUSTOM 0xFE
enum
{
    NDS_P4_COMPUTER_CUSTOM_JUMPSQUAT_WAIT,
    NDS_P4_COMPUTER_CUSTOM_PRESS_C,
    NDS_P4_COMPUTER_CUSTOM_UNPRESS_C,
    NDS_P4_COMPUTER_CUSTOM_STICK_TO_TARGET,
    NDS_P4_COMPUTER_CUSTOM_TURNAROUND_WAIT
};
_Static_assert((nFTCommonStatusTurn == 0x12) && (nFTCommonStatusTurnRun == 0x13) &&
               (nFTCommonStatusKneeBend == 0x14), "Remix Action.Turn/TurnRun/JumpSquat");
_Static_assert((offsetof(FTStruct, input.cp.button_inputs) == 0x1C6) &&
               (offsetof(FTStruct, input.cp.stick_range) == 0x1C8),
               "Remix CPU button and stick offsets");

/* ftcomputer.c:3410 ftComputerUpdateInputs as a content's CPU runs it on the
 * N64: with Remix's two extensions, the stick-X values 0x81-0x84 (AI.asm
 * extend_stick_x_commands: away from the target at 80/40, forward/back from
 * the facing at 80) and the 0xFE custom commands (CUSTOM_COMMANDS: wait out
 * the jump squat or a turn, press or release C, point the stick at the
 * target), and with the Fox up-special test taking the content only where
 * Remix's usp_check_ does. The source's scripts carry neither extension, so
 * they run here exactly as in the source's interpreter. */
static void ndsP4ComputerRunScript(FTStruct *this_fp)
{
    FTComputer *com = &this_fp->computer;
    const NDSP4Computer *rows = ndsP4Computer(this_fp);
    sb32 is_fox_usp = (rows != NULL) && (rows->fox_usp_check != FALSE);
    u8 *p_command;
    u8 command;
    s8 var_t1 = 0;
    s16 stick_range_y;
    s16 stick_range_x;
    f32 dist_x;
    f32 dist_y;

    if (com->input_wait == 0)
    {
        return;
    }
    com->input_wait--;

    if (com->input_wait == 0)
    {
        p_command = com->p_command;

        while (com->input_wait == 0)
        {
            command = *p_command++;

            if (command < FTCOMPUTER_COMMAND_DEFAULT_MAX)
            {
                com->input_wait = command & FTCOMPUTER_COMMAND_TIMER_MASK;

                switch (command & FTCOMPUTER_COMMAND_OPCODE_MASK)
                {
                case FTCOMPUTER_COMMAND_BUTTON_A_PRESS:
                    this_fp->input.cp.button_inputs |= A_BUTTON;
                    break;

                case FTCOMPUTER_COMMAND_BUTTON_A_RELEASE:
                    this_fp->input.cp.button_inputs &= ~A_BUTTON;
                    break;

                case FTCOMPUTER_COMMAND_BUTTON_B_PRESS:
                    this_fp->input.cp.button_inputs |= B_BUTTON;
                    break;

                case FTCOMPUTER_COMMAND_BUTTON_B_RELEASE:
                    this_fp->input.cp.button_inputs &= ~B_BUTTON;
                    break;

                case FTCOMPUTER_COMMAND_BUTTON_Z_PRESS:
                    this_fp->input.cp.button_inputs |= Z_TRIG;
                    break;

                case FTCOMPUTER_COMMAND_BUTTON_Z_RELEASE:
                    this_fp->input.cp.button_inputs &= ~Z_TRIG;
                    break;

                case FTCOMPUTER_COMMAND_BUTTON_L_PRESS:
                    this_fp->input.cp.button_inputs |= L_TRIG;
                    break;

                case FTCOMPUTER_COMMAND_BUTTON_L_RELEASE:
                    this_fp->input.cp.button_inputs &= ~L_TRIG;
                    break;

                case FTCOMPUTER_COMMAND_BUTTON_START_PRESS:
                    this_fp->input.cp.button_inputs |= START_BUTTON;
                    break;

                case FTCOMPUTER_COMMAND_BUTTON_START_RELEASE:
                    this_fp->input.cp.button_inputs &= ~START_BUTTON;
                    break;

                case FTCOMPUTER_COMMAND_STICK_X_TILT:
                    switch (*p_command)
                    {
                    default:
                        this_fp->input.cp.stick_range.x = *p_command++;
                        break;

                    case FTCOMPUTER_STICK_AUTOFULL:
                        this_fp->input.cp.stick_range.x = (this_fp->joints[nFTPartsJointTopN]->translate.vec.f.x < com->target_pos.x) ? (I_CONTROLLER_RANGE_MAX) : -(I_CONTROLLER_RANGE_MAX);
                        p_command++;
                        break;

                    case FTCOMPUTER_STICK_AUTOHALF:
                        this_fp->input.cp.stick_range.x = (this_fp->joints[nFTPartsJointTopN]->translate.vec.f.x < com->target_pos.x) ? (I_CONTROLLER_RANGE_MAX / 2) : -(I_CONTROLLER_RANGE_MAX / 2);
                        p_command++;
                        break;

                    /* extend_stick_x_commands. */
                    case 0x81:
                        this_fp->input.cp.stick_range.x = (this_fp->joints[nFTPartsJointTopN]->translate.vec.f.x < com->target_pos.x) ? -(I_CONTROLLER_RANGE_MAX) : (I_CONTROLLER_RANGE_MAX);
                        p_command++;
                        break;

                    case 0x82:
                        this_fp->input.cp.stick_range.x = (this_fp->joints[nFTPartsJointTopN]->translate.vec.f.x < com->target_pos.x) ? -(I_CONTROLLER_RANGE_MAX / 2) : (I_CONTROLLER_RANGE_MAX / 2);
                        p_command++;
                        break;

                    case 0x83:
                        this_fp->input.cp.stick_range.x = (this_fp->lr >= 0) ? (I_CONTROLLER_RANGE_MAX) : -(I_CONTROLLER_RANGE_MAX);
                        p_command++;
                        break;

                    case 0x84:
                        this_fp->input.cp.stick_range.x = (this_fp->lr >= 0) ? -(I_CONTROLLER_RANGE_MAX) : (I_CONTROLLER_RANGE_MAX);
                        p_command++;
                        break;
                    }
                    break;

                case FTCOMPUTER_COMMAND_STICK_Y_TILT:
                    switch (*p_command)
                    {
                    default:
                        this_fp->input.cp.stick_range.y = *p_command++;
                        break;

                    case FTCOMPUTER_STICK_AUTOFULL:
                        this_fp->input.cp.stick_range.y = (this_fp->joints[nFTPartsJointTopN]->translate.vec.f.y < com->target_pos.y) ? (I_CONTROLLER_RANGE_MAX) : -(I_CONTROLLER_RANGE_MAX);
                        p_command++;
                        break;

                    case FTCOMPUTER_STICK_AUTOHALF:
                        this_fp->input.cp.stick_range.y = (this_fp->joints[nFTPartsJointTopN]->translate.vec.f.y < com->target_pos.y) ? (I_CONTROLLER_RANGE_MAX / 2) : -(I_CONTROLLER_RANGE_MAX / 2);
                        p_command++;
                        break;
                    }
                    break;

                case FTCOMPUTER_COMMAND_MOVEAUTO:
                    dist_x = com->target_pos.x - this_fp->joints[nFTPartsJointTopN]->translate.vec.f.x;
                    dist_y = com->target_pos.y - this_fp->joints[nFTPartsJointTopN]->translate.vec.f.y;

                    if ((this_fp->ga == nMPKineticsGround) && (this_fp->level < 5))
                    {
                        stick_range_x = (ABSF(dist_x) > 100.0F) ? (I_CONTROLLER_RANGE_MAX / 2) : 0;
                    }
                    else if (this_fp->ga == nMPKineticsGround)
                    {
                        if ((com->dash_predict * 1.5F) < ABSF(dist_x))
                        {
                            stick_range_x = (I_CONTROLLER_RANGE_MAX);
                        }
                        else
                        {
                            if (com->dash_predict < ABSF(dist_x))
                            {
                                stick_range_x = ((2.0F * ((ABSF(dist_x) - com->dash_predict) / com->dash_predict) * (F_CONTROLLER_RANGE_MAX / 2)) + (F_CONTROLLER_RANGE_MAX / 2));
                            }
                            else
                            {
                                stick_range_x = (ABSF(dist_x) > 100.0F) ? (I_CONTROLLER_RANGE_MAX / 2) : 0;
                            }
                        }
                    }
                    else
                    {
                        stick_range_x = ((ABSF(dist_x) > 100.0F) || ((this_fp->lr * dist_x) < 0.0F)) ? (I_CONTROLLER_RANGE_MAX) : (I_CONTROLLER_RANGE_MAX / 4);
                    }
                    stick_range_y = I_CONTROLLER_RANGE_MAX;

                    if (this_fp->ga == nMPKineticsGround)
                    {
                        if (this_fp->status_id != nFTCommonStatusKneeBend)
                        {
                            if (com->target_line_id == this_fp->coll_data.floor_line_id)
                            {
                                stick_range_y = dist_y = 0.0F;
                            }
                            if
                            (
                                (com->ftcom_flags_0x4A_b1) &&
                                (
                                    (ftGetComTargetFighter(com)->status_id == nFTCommonStatusCliffCatch) ||
                                    (ftGetComTargetFighter(com)->status_id == nFTCommonStatusCliffWait)
                                )
                            )
                            {
                                stick_range_y = dist_y = 0.0F;
                            }
                        }
                    }
                    else
                    {
                        /* The source's Fox test, as usp_check_ makes it. */
                        if
                        (
                            (
                                (is_fox_usp == FALSE) ||
                                (
                                    (this_fp->status_id != nFTFoxStatusSpecialHiStart) &&
                                    (this_fp->status_id != nFTFoxStatusSpecialAirHiStart) &&
                                    (this_fp->status_id != nFTFoxStatusSpecialHiHold) &&
                                    (this_fp->status_id != nFTFoxStatusSpecialAirHiHold)
                                )
                            )
                            &&
                            (dist_y < 0)
                        )
                        {
                            stick_range_y = dist_y = 0.0F;
                        }
                        switch (com->behavior)
                        {
                        case nFTComputerBehaviorYoshiTeam:
                            if (this_fp->joints[nFTPartsJointTopN]->translate.vec.f.y < 0)
                            {
                                stick_range_y = dist_y = 0.0F;
                            }
                            break;

                        case nFTComputerBehaviorKirbyTeam:
                        case nFTComputerBehaviorPolyTeam:
                            if (this_fp->joints[nFTPartsJointTopN]->translate.vec.f.y < -300.0F)
                            {
                                stick_range_y = dist_y = 0.0F;
                            }
                            break;
                        }
                    }
                    if ((dist_x != 0.0F) && (dist_y != 0.0F))
                    {
                        if (ABSF(dist_y) < ABSF(dist_x))
                        {
                            this_fp->input.cp.stick_range.x = (dist_x > 0.0F) ? stick_range_x : -stick_range_x;

                            this_fp->input.cp.stick_range.y = (ABSF((dist_y / dist_x)) * ((dist_y > 0.0F) ? stick_range_y : -stick_range_y));
                        }
                        else
                        {
                            this_fp->input.cp.stick_range.x = (ABSF((dist_x / dist_y)) * ((dist_x > 0.0F) ? stick_range_x : -stick_range_x));

                            this_fp->input.cp.stick_range.y = (dist_y > 0.0F) ? stick_range_y : -stick_range_y;
                        }
                    }
                    else if (dist_x != 0.0F)
                    {
                        this_fp->input.cp.stick_range.x = (dist_x > 0.0F) ? stick_range_x : -stick_range_x;

                        this_fp->input.cp.stick_range.y = (ABSF((dist_y / dist_x)) * ((dist_y > 0.0F) ? stick_range_y : -stick_range_y));
                    }
                    else if (dist_y != 0.0F)
                    {
                        this_fp->input.cp.stick_range.x = (ABSF((dist_x / dist_y)) * ((dist_x > 0.0F) ? stick_range_x : -stick_range_x));

                        this_fp->input.cp.stick_range.y = (dist_y > 0.0F) ? stick_range_y : -stick_range_y;
                    }
                    else
                    {
                        this_fp->input.cp.stick_range.x = this_fp->input.cp.stick_range.y = 0;
                    }
                    break;

                case FTCOMPUTER_COMMAND_STICK_X_VAR:
                    this_fp->input.cp.stick_range.x = var_t1;
                    break;

                case FTCOMPUTER_COMMAND_STICK_Y_VAR:
                    this_fp->input.cp.stick_range.y = var_t1;
                    break;
                }
            }
            else switch (command)
            {
            case FTCOMPUTER_COMMAND_DEFAULT_MAX + 0:
                com->input_wait = *p_command++;
                break;

            case FTCOMPUTER_COMMAND_DEFAULT_MAX + 1:
                var_t1 = 1;
                break;

            case FTCOMPUTER_COMMAND_DEFAULT_MAX + 2:
                com->input_wait = var_t1;
                break;

            case FTCOMPUTER_COMMAND_DEFAULT_MAX + 3:
                ftComputerSetControlPKThunder(this_fp);
                break;

            case NDS_P4_COMPUTER_COMMAND_CUSTOM:
                switch (*p_command++)
                {
                /* Hold this command until the jump squat (a turn) ends:
                 * one tick, then read it again. */
                case NDS_P4_COMPUTER_CUSTOM_JUMPSQUAT_WAIT:
                    if (this_fp->status_id == nFTCommonStatusKneeBend)
                    {
                        com->input_wait = 1;
                        p_command -= 2;
                    }
                    break;

                case NDS_P4_COMPUTER_CUSTOM_TURNAROUND_WAIT:
                    if ((this_fp->status_id == nFTCommonStatusTurn) ||
                        (this_fp->status_id == nFTCommonStatusTurnRun))
                    {
                        com->input_wait = 1;
                        p_command -= 2;
                    }
                    break;

                /* R_CBUTTONS; the release clears all four C buttons. */
                case NDS_P4_COMPUTER_CUSTOM_PRESS_C:
                    this_fp->input.cp.button_inputs |= 0x0001;
                    break;

                case NDS_P4_COMPUTER_CUSTOM_UNPRESS_C:
                    this_fp->input.cp.button_inputs &= 0xFFF0;
                    break;

                /* The stick at full range toward the target, rounded to
                 * nearest as the routine's cvt.w.s does. */
                case NDS_P4_COMPUTER_CUSTOM_STICK_TO_TARGET:
                {
                    const Vec3f *pos = this_fp->coll_data.p_translate;
                    f32 dx = com->target_pos.x - pos->x;
                    f32 dy = com->target_pos.y - pos->y;
                    f32 d2 = (dx * dx) + (dy * dy);
                    f32 magnitude = (d2 != 0.0F) ? sqrtf(d2) : 0.0F;

                    if (magnitude == 0.0F)
                    {
                        this_fp->input.cp.stick_range.x = 0;
                        this_fp->input.cp.stick_range.y = 0;
                    }
                    else
                    {
                        f32 scale = F_CONTROLLER_RANGE_MAX / magnitude;

                        this_fp->input.cp.stick_range.x = (s32)rintf(dx * scale);
                        this_fp->input.cp.stick_range.y = (s32)rintf(dy * scale);
                    }
                    break;
                }
                }
                break;

            case FTCOMPUTER_COMMAND_END:
                com->input_wait = 0;
                com->p_command = NULL;
                return;
            }
        }
        com->p_command = p_command;
    }
}

sb32 ndsP4ComputerRunInputs(FTStruct *this_fp)
{
    if (this_fp->nds_p4_content == 0u)
    {
        return FALSE;
    }
    ndsP4ComputerRunScript(this_fp);

    return TRUE;
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
    if ((fp->computer.p_command != NULL) &&
        (fp->computer.p_command ==
         ndsP4ComputerOwnScript(fp, nNDSP4ComputerInputDashAttack)) &&
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

static u8 sNdsP4SpecialStatusResolved[NDS_P4_CONTENT_LIMIT];

FTStatusDesc *ndsP4SpecialStatusDescs(const FTStruct *fp,
                                      const FTStatusDesc *parent)
{
    u32 content = ndsP4Content(fp);
    const NDSP4Fighter *f = ndsP4Fighter(content);
    u32 i;

    if (f == NULL)
    {
        return NULL;
    }
    if (sNdsP4SpecialStatusResolved[content] == 0u)
    {
        /* Only statuses the parent has carry the marker: an appended status
         * names every routine. */
        for (i = 0u; i < *f->special_status_count; i++)
        {
            FTStatusDesc *row = &f->special_statuses[i];

#define NDS_P4_RESOLVE_PROC(proc_) \
            if (row->proc_ == NDS_P4_PROC_INHERIT) row->proc_ = parent[i].proc_
            NDS_P4_RESOLVE_PROC(proc_update);
            NDS_P4_RESOLVE_PROC(proc_interrupt);
            NDS_P4_RESOLVE_PROC(proc_physics);
            NDS_P4_RESOLVE_PROC(proc_map);
#undef NDS_P4_RESOLVE_PROC
        }
        sNdsP4SpecialStatusResolved[content] = 1u;
    }
    return f->special_statuses;
}

const NDSP4Entry *ndsP4Entry(const FTStruct *fp)
{
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(fp));

    return (f != NULL) ? f->entry : NULL;
}

const NDSP4Overrides *ndsP4Overrides(const FTStruct *fp)
{
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(fp));

    return (f != NULL) ? f->overrides : NULL;
}

static void ndsP4TablesHalt(u32 content)
{
    gNdsP4TablesFailure = content;
    for (;;)
    {
    }
}

const NDSP4TablesHeader *ndsP4LoadTables(u32 content)
{
    const NDSP4Fighter *f = ndsP4Fighter(content);
    NDSP4TablesHeader *t;
    char path[48];
    FILE *file;
    long bytes;

    if (f == NULL)
    {
        return NULL;
    }
    if ((sNdsP4Tables[content] != NULL) &&
        (sNdsP4TablesGeneration[content] == gNdsTaskmanHeapGeneration))
    {
        return sNdsP4Tables[content];
    }
    snprintf(path, sizeof(path), "nitro:/p4/%s.tab", sNdsP4TableNames[content]);
    ndsFsLock();
    file = fopen(path, "rb");
    if ((file == NULL) || (fseek(file, 0, SEEK_END) != 0) ||
        ((bytes = ftell(file)) < (long)sizeof(NDSP4TablesHeader)) ||
        (fseek(file, 0, SEEK_SET) != 0))
    {
        if (file != NULL)
        {
            fclose(file);
        }
        ndsFsUnlock();
        ndsP4TablesHalt(content);
    }
    t = syTaskmanMalloc((size_t)bytes, 4);
    if (fread(t, 1u, (size_t)bytes, file) != (size_t)bytes)
    {
        fclose(file);
        ndsFsUnlock();
        ndsP4TablesHalt(content);
    }
    fclose(file);
    ndsFsUnlock();
    if ((t->magic != NDS_P4_TABLES_MAGIC) || (t->version != NDS_P4_TABLES_VERSION) ||
        (t->bytes != (u32)bytes) ||
        (t->motion_off + (t->motion_count * sizeof(FTMotionDesc)) > t->bytes) ||
        ((s32)t->motion_count != f->data->mainmotion_array_count) ||
        (t->attack_off + (t->attack_count * sizeof(FTComputerAttack)) > t->bytes) ||
        (t->script_off + (t->script_count * sizeof(NDSP4ComputerScript)) > t->bytes) ||
        (t->script_bytes_off > t->bytes))
    {
        ndsP4TablesHalt(content);
    }
    sNdsP4Tables[content] = t;
    sNdsP4TablesGeneration[content] = gNdsTaskmanHeapGeneration;
    f->data->mainmotion = (FTMotionDescArray *)(void *)((u8 *)t + t->motion_off);

    return t;
}

/* The loaded tables of a fighter's content, NULL before its first use. */
static const NDSP4TablesHeader *ndsP4FighterTables(const FTStruct *fp)
{
    u32 content = ndsP4Content(fp);

    if ((content == 0u) || (content >= NDS_P4_CONTENT_LIMIT) ||
        (sNdsP4Tables[content] == NULL) ||
        (sNdsP4TablesGeneration[content] != gNdsTaskmanHeapGeneration))
    {
        return NULL;
    }
    return sNdsP4Tables[content];
}

const f32 *ndsP4YoshiEggRow(const FTStruct *fp)
{
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(fp));

    return (f != NULL) ? f->yoshi_egg : NULL;
}

const NDSP4Jab *ndsP4JabRows(const FTStruct *fp)
{
    static const NDSP4Jab none;
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(fp));

    return ((f != NULL) && (f->jab != NULL)) ? f->jab : &none;
}

void ndsP4AfterProcMapSlow(GObj *fighter_gobj)
{
    const NDSP4Overrides *o = ndsP4Overrides(ftGetStruct(fighter_gobj));

    if ((o != NULL) && (o->after_proc_map != NULL))
    {
        o->after_proc_map(fighter_gobj);
    }
}

void ndsP4OnDead(GObj *fighter_gobj)
{
    const NDSP4Overrides *o = ndsP4Overrides(ftGetStruct(fighter_gobj));

    if ((o != NULL) && (o->on_dead != NULL))
    {
        o->on_dead(fighter_gobj);
    }
}

void ndsP4OnHitInteractSlow(FTStruct *fp, s32 attack_type)
{
    const NDSP4Overrides *o = ndsP4Overrides(fp);

    if ((o != NULL) && (o->on_hit_interact != NULL))
    {
        o->on_hit_interact(fp, attack_type);
    }
}

sb32 ndsP4GravitySlow(FTStruct *fp)
{
    const NDSP4Overrides *o = ndsP4Overrides(fp);

    return ((o != NULL) && (o->gravity != NULL)) ? o->gravity(fp) : FALSE;
}

sb32 ndsP4AirJumpCheck(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    const NDSP4Overrides *o;

    if (fp->nds_p4_content == 0u)
    {
        return FALSE;
    }
    o = ndsP4Overrides(fp);
    return ((o != NULL) && (o->air_jump_check != NULL)) ? o->air_jump_check(fighter_gobj)
                                                        : FALSE;
}

void ndsP4FallSetStatus(GObj *fighter_gobj, s32 status_id, f32 frame_begin,
                        f32 anim_speed, u32 flags)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->nds_p4_content != 0u)
    {
        const NDSP4Overrides *o = ndsP4Overrides(fp);

        if ((o != NULL) && (o->fall_status != NULL) && (o->fall_status(fighter_gobj) != FALSE))
        {
            return;
        }
    }
    ftMainSetStatus(fighter_gobj, status_id, frame_begin, anim_speed, flags);
}

sb32 ndsP4AirLightThrowCheck(FTStruct *fp)
{
    if (fp->nds_p4_content != 0u)
    {
        const NDSP4Overrides *o = ndsP4Overrides(fp);

        if ((o != NULL) && (o->air_item_throw_block != NULL) &&
            (o->air_item_throw_block(fp) != FALSE))
        {
            return FALSE;
        }
    }
    return ftCommonLightThrowCheckItemTypeThrow(fp);
}

void ndsP4OnDamage(FTStruct *fp)
{
    const NDSP4Overrides *o;

    if (fp->nds_p4_content == 0u)
    {
        return;
    }
    o = ndsP4Overrides(fp);
    if ((o != NULL) && (o->on_damage != NULL))
    {
        o->on_damage(fp);
    }
}

void ndsP4OnLanding(FTStruct *fp, sb32 cliff)
{
    const NDSP4Overrides *o;

    if (fp->nds_p4_content == 0u)
    {
        return;
    }
    o = ndsP4Overrides(fp);
    if ((o != NULL) && (o->on_landing != NULL))
    {
        o->on_landing(fp, cliff);
    }
}

void ndsP4BeforePhysicsMapSlow(GObj *fighter_gobj)
{
    const NDSP4Overrides *o = ndsP4Overrides(ftGetStruct(fighter_gobj));

    if ((o != NULL) && (o->before_physics_map != NULL))
    {
        o->before_physics_map(fighter_gobj);
    }
}

sb32 ndsP4CatchDamageCollide(FTStruct *victim_fp, FTAttackColl *attack_coll,
                             FTDamageColl *damage_coll)
{
    if (victim_fp->nds_p4_content != 0u)
    {
        const NDSP4Overrides *o = ndsP4Overrides(victim_fp);

        if ((o != NULL) && (o->grabbable != NULL) &&
            (o->grabbable(victim_fp, (s32)(damage_coll - victim_fp->damage_colls)) == FALSE))
        {
            return FALSE;
        }
    }
    return gmCollisionCheckFighterAttackDamageCollide(attack_coll, damage_coll);
}

void ndsP4AttackAirStart(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    const NDSP4Overrides *o;

    if ((fp->nds_p4_content == 0u) || (fp->status_id != nFTCommonStatusAttackAirLw))
    {
        return;
    }
    o = ndsP4Overrides(fp);
    if ((o != NULL) && (o->attack_air_lw_hit != NULL))
    {
        fp->proc_hit = o->attack_air_lw_hit;
    }
}

s32 ndsP4DamageStatus(GObj *fighter_gobj, s32 status_id, s32 *status_id_after)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    const NDSP4Overrides *o;

    if (fp->nds_p4_content == 0u)
    {
        return status_id;
    }
    o = ndsP4Overrides(fp);
    if ((o != NULL) && (o->damage_status != NULL))
    {
        return o->damage_status(fp, status_id, status_id_after);
    }
    return status_id;
}

sb32 ndsP4AirSpecialBlocked(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    const NDSP4Overrides *o;

    if (fp->nds_p4_content == 0u)
    {
        return FALSE;
    }
    o = ndsP4Overrides(fp);
    return ((o != NULL) && (o->air_special_block != NULL)) ? o->air_special_block(fp) : FALSE;
}

const f32 *ndsP4MultiJumpVelocities(const FTStruct *fp)
{
    const NDSP4Overrides *o;

    if (fp->nds_p4_content == 0u)
    {
        return NULL;
    }
    o = ndsP4Overrides(fp);
    return (o != NULL) ? o->multi_jump_vel : NULL;
}

void ndsP4OnInitFighter(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    const NDSP4Overrides *o;

    if (fp->nds_p4_content == 0u)
    {
        return;
    }
    o = ndsP4Overrides(fp);
    if ((o != NULL) && (o->on_init != NULL))
    {
        o->on_init(fp);
    }
}

s32 ndsP4KirbyInhaleStatus(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    const NDSP4Overrides *o;

    if (fp->nds_p4_content == 0u)
    {
        return status_id;
    }
    o = ndsP4Overrides(fp);
    return ((o != NULL) && (o->kirby_inhale_status != NULL)) ? o->kirby_inhale_status(status_id)
                                                             : status_id;
}

/* relocData/228_KirbyMainMotion.c's FTKirbyCopy[27], the table Kirby's
 * inhale reads from his main motion file; Remix's kirby_inhale_struct holds
 * the same rows for the original fighters. The inhale a content runs reads
 * it here, Kirby's file or not in the match (a pointer no loaded file owns,
 * so lbRelocGetFileData answers it as is). */
static const FTKirbyCopy sNdsP4KirbyCopyTable[27] = {
    { nFTKindMario, 12, 1.5F, 17 },
    { nFTKindFox, 7, 1.5F, 17 },
    { nFTKindDonkey, 4, 2.0F, 30 },
    { nFTKindSamus, 8, 1.6F, 17 },
    { nFTKindLuigi, 11, 1.6F, 17 },
    { nFTKindLink, 10, 1.5F, 17 },
    { nFTKindYoshi, 5, 1.7F, 25 },
    { nFTKindCaptain, 9, 1.7F, 17 },
    { nFTKindKirby, 0, 1.6F, 17 },
    { nFTKindPikachu, 6, 1.5F, 17 },
    { nFTKindPurin, 3, 1.6F, 17 },
    { nFTKindNess, 13, 1.6F, 17 },
    { nFTKindKirby, 0, 1.0F, 17 },
    { nFTKindKirby, 0, 1.5F, 17 },
    { nFTKindKirby, 0, 1.5F, 17 },
    { nFTKindKirby, 0, 1.5F, 17 },
    { nFTKindKirby, 0, 2.0F, 30 },
    { nFTKindKirby, 0, 1.6F, 17 },
    { nFTKindKirby, 0, 1.6F, 17 },
    { nFTKindKirby, 0, 1.5F, 17 },
    { nFTKindKirby, 0, 1.7F, 17 },
    { nFTKindKirby, 0, 1.7F, 17 },
    { nFTKindKirby, 0, 1.6F, 17 },
    { nFTKindKirby, 0, 1.5F, 17 },
    { nFTKindKirby, 0, 1.6F, 17 },
    { nFTKindKirby, 0, 1.6F, 17 },
    { nFTKindDonkey, 4, 2.0F, 50 },
};

void *ndsP4KirbyCopyFile(void)
{
    return (void *)sNdsP4KirbyCopyTable;
}

s32 ndsP4KirbyCaptureWait(FTStruct *victim_fp, s32 breakout_wait)
{
    FTStruct *holder_fp;
    const NDSP4Overrides *o;

    if ((victim_fp->status_id != nFTCommonStatusCaptureWaitKirby) ||
        (victim_fp->capture_gobj == NULL))
    {
        return breakout_wait;
    }
    holder_fp = ftGetStruct(victim_fp->capture_gobj);
    if (holder_fp->nds_p4_content == 0u)
    {
        return breakout_wait;
    }
    o = ndsP4Overrides(holder_fp);
    return ((o != NULL) && (o->kirby_capture_wait != NULL)) ?
        o->kirby_capture_wait(victim_fp, breakout_wait) : breakout_wait;
}

s32 ndsP4KirbyStarDamage(const FTStruct *victim_fp)
{
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(victim_fp));

    return ((f != NULL) && (f->kirby_inhale != NULL)) ? f->kirby_inhale->star_damage : -1;
}

f32 ndsP4KirbyInhaleWindX(GObj *fighter_gobj, f32 source_x)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    const NDSP4Overrides *o;

    if (fp->nds_p4_content == 0u)
    {
        return source_x;
    }
    o = ndsP4Overrides(fp);
    return ((o != NULL) && (o->kirby_inhale_wind_x != 0.0F)) ? o->kirby_inhale_wind_x : source_x;
}

sb32 ndsP4CustomAbsorber(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 kind;

    if ((fp->nds_p4_content == 0u) || (fp->special_coll == NULL))
    {
        return FALSE;
    }
    kind = fp->special_coll->kind;
    return (((kind & 0xFFFF) == NDS_P4_SPECIAL_COLL_CUSTOM) && ((kind >> 16) != 0)) ? TRUE : FALSE;
}

void ndsP4OnCustomReflect(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    const NDSP4Overrides *o;

    if (fp->nds_p4_content == 0u)
    {
        return;
    }
    o = ndsP4Overrides(fp);
    if ((o != NULL) && (o->on_custom_reflect != NULL))
    {
        o->on_custom_reflect(fighter_gobj);
    }
}

void ndsP4OnEatTomato(FTStruct *fp)
{
    const NDSP4Overrides *o;

    if (fp->nds_p4_content == 0u)
    {
        return;
    }
    o = ndsP4Overrides(fp);
    if ((o != NULL) && (o->on_eat_tomato != NULL))
    {
        o->on_eat_tomato(fp);
    }
}

void ndsP4FallSpecialSetStatus(GObj *fighter_gobj, s32 status_id, f32 frame_begin,
                               f32 anim_speed, u32 flags)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->nds_p4_content != 0u)
    {
        const NDSP4Overrides *o = ndsP4Overrides(fp);

        if ((o != NULL) && (o->fall_special_status != NULL))
        {
            status_id = o->fall_special_status(fp, status_id);
        }
    }
    ftMainSetStatus(fighter_gobj, status_id, frame_begin, anim_speed, flags);
}

#if NDS_P2_YOSHI
GObj *wpYoshiStarMakeStars(GObj *fighter_gobj, Vec3f *pos);

GObj *ndsP4YoshiStarMakeStars(GObj *fighter_gobj, Vec3f *pos)
{
    const NDSP4Overrides *o = ndsP4Overrides(ftGetStruct(fighter_gobj));

    if ((o != NULL) && (o->no_yoshi_lw_stars != 0u))
    {
        return NULL;
    }
    return wpYoshiStarMakeStars(fighter_gobj, pos);
}
#endif

void ndsP4ThrowMainSetStatus(GObj *fighter_gobj, s32 status_id, f32 frame_begin,
                             f32 anim_speed, u32 flags)
{
    if (status_id == nFTCommonStatusThrowF)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        const NDSP4Overrides *o = ndsP4Overrides(fp);

        /* jigglypuffkirbyshared.asm kirby_fthrow_1_fix: the content takes
         * Kirby's branch, which sets the fighter airborne before the status
         * and names its own; nothing between the two reads the kinetics. */
        if ((o != NULL) && (o->throw_f_kirby_status != 0))
        {
            mpCommonSetFighterAir(fp);
            status_id = o->throw_f_kirby_status;
        }
    }
    ftMainSetStatus(fighter_gobj, status_id, frame_begin, anim_speed, flags);
}

/* Lab stand-ins for unported special-move starters, counted. */
__attribute__((used)) volatile u32 gNdsP4LabSpecialStandIns;

void ndsP4LabSpecialStandIn(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    gNdsP4LabSpecialStandIns++;
}

sb32 ndsP4CheckSpecialLent(GObj *fighter_gobj, sb32 (*check)(GObj *),
                           NDSP4SpecialStart *const *tables,
                           const u8 *slots, u32 count)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    const NDSP4Fighter *f = ndsP4Fighter(ndsP4Content(fp));
    NDSP4SpecialStart saved[3];
    sb32 result;
    u32 i;

    if ((f == NULL) || (count > (u32)ARRAY_COUNT(saved)))
    {
        return check(fighter_gobj);
    }
    for (i = 0; i < count; i++)
    {
        saved[i] = tables[i][fp->fkind];
        if (f->special_starts[slots[i]] != NULL)
        {
            tables[i][fp->fkind] = f->special_starts[slots[i]];
        }
    }
    result = check(fighter_gobj);
    for (i = 0; i < count; i++)
    {
        tables[i][fp->fkind] = saved[i];
    }
    return result;
}

void ndsP4AfterSetStatus(GObj *fighter_gobj, s32 status_id)
{
    const NDSP4Fighter *f =
        ndsP4Fighter(ftGetStruct(fighter_gobj)->nds_p4_content);

    if ((f != NULL) && (f->on_status != NULL))
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

/* A content file's NitroFS path, nitro:/reloc/p4/<id as 4 hex digits>; NULL
 * for a file no content has. One buffer, rewritten by each lookup: the
 * reloc loader's caller (ndsRelocAssetFind) keeps it in its own single
 * static entry, which has the same lifetime. */
const char *ndsP4RelocAssetPath(u32 file_id)
{
    static char path[] = "nitro:/reloc/p4/0000";
    static const char hex[] = "0123456789abcdef";
    u32 c;

    if (file_id > 0xFFFFu)
    {
        return NULL;
    }
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
            if (f->assets[i] == file_id)
            {
                u32 at = sizeof(path) - 5u;

                path[at + 0u] = hex[(file_id >> 12) & 0xFu];
                path[at + 1u] = hex[(file_id >> 8) & 0xFu];
                path[at + 2u] = hex[(file_id >> 4) & 0xFu];
                path[at + 3u] = hex[file_id & 0xFu];
                return path;
            }
        }
    }
    return NULL;
}

sb32 ndsP4LabSkipsDependency(u32 owner_asset, u32 dep_asset)
{
    u32 c;

    if (ndsP4RelocAssetPath(owner_asset) == NULL)
    {
        return FALSE;
    }
    for (c = 1u; c < NDS_P4_CONTENT_LIMIT; c++)
    {
        const NDSP4Fighter *f = ndsP4Fighter(c);
        u32 i;

        if (f == NULL)
        {
            continue;
        }
        for (i = 0u; i < *f->lab_skip_file_count; i++)
        {
            if (f->lab_skip_files[i] == dep_asset)
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/* The content's baked texture pointer from owner into dep at slot (any slot
 * when slot is UINT32_MAX), or NULL. Battle only: the entry articles that
 * carry the pixels draw in matches. */
static const NDSP4BakedRef *ndsP4FindBakedRef(u32 owner_asset, u32 dep_asset, u32 slot)
{
    u32 c;

    if (gNdsSceneManagerCurrIsBattle == 0u)
    {
        return NULL;
    }
    for (c = 1u; c < NDS_P4_CONTENT_LIMIT; c++)
    {
        const NDSP4Fighter *f = ndsP4Fighter(c);
        u32 i;

        if (f == NULL)
        {
            continue;
        }
        for (i = 0u; i < *f->baked_ref_count; i++)
        {
            const NDSP4BakedRef *r = &f->baked_refs[i];

            if ((r->owner == owner_asset) && (r->dep == dep_asset) &&
                ((slot == UINT32_MAX) || (r->slot == slot)))
            {
                return r;
            }
        }
    }
    return NULL;
}

sb32 ndsP4NativeOwnsDependency(u32 owner_asset, u32 dep_asset)
{
    return (ndsP4FindBakedRef(owner_asset, dep_asset, UINT32_MAX) != NULL) ? TRUE : FALSE;
}

sb32 ndsP4NativeBakedRef(u32 owner_asset, u32 dep_asset, u32 slot, u32 target)
{
    const NDSP4BakedRef *r = ndsP4FindBakedRef(owner_asset, dep_asset, slot);

    return ((r != NULL) && (r->target == target)) ? TRUE : FALSE;
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

/* Remix hitbox overrides by port and hitbox: a launch direction (0 none,
 * NDS_P4_HITBOX_FORWARD, NDS_P4_HITBOX_BACKWARD) and a hit sound (-1 none;
 * bit 15 set plays it after the source's). */
#define NDS_P4_HITBOX_FORWARD 1u
#define NDS_P4_HITBOX_BACKWARD 2u
static u8 sNdsP4HitboxDir[GMCOMMON_PLAYERS_MAX][4];
static u16 sNdsP4HitboxFgm[GMCOMMON_PLAYERS_MAX][4] = {
    { 0xFFFFu, 0xFFFFu, 0xFFFFu, 0xFFFFu }, { 0xFFFFu, 0xFFFFu, 0xFFFFu, 0xFFFFu },
    { 0xFFFFu, 0xFFFFu, 0xFFFFu, 0xFFFFu }, { 0xFFFFu, 0xFFFFu, 0xFFFFu, 0xFFFFu }
};

/* Defined in the decomp's lb and sys code (no DS header declares them). */
void *lbCommonMakePositionFGM(u16 fgm, f32 pos);
s32 syUtilsRandIntRange(s32 range);

void ndsP4ResetHitboxOverrides(const FTStruct *fp, u32 attack_id)
{
    u32 port = fp->player & 3u;
    u32 i;

    for (i = 0u; i < 4u; i++)
    {
        if ((attack_id >= 4u) || (i == attack_id))
        {
            sNdsP4HitboxDir[port][i] = 0u;
            sNdsP4HitboxFgm[port][i] = 0xFFFFu;
        }
    }
}

void ndsP4MakeHitPositionFGM(FTStruct *attacker_fp, FTAttackColl *attack_coll,
                             u16 fgm_id, f32 pos_x)
{
    u32 hitbox = (u32)(attack_coll - attacker_fp->attack_colls);
    u16 own = 0xFFFFu;

    if ((ndsP4Content(attacker_fp) != 0u) && (hitbox < 4u))
    {
        own = sNdsP4HitboxFgm[attacker_fp->player & 3u][hitbox];
    }
    if ((own == 0xFFFFu) || ((own & 0x8000u) != 0u))
    {
        lbCommonMakePositionFGM(fgm_id, pos_x);
    }
    if (own != 0xFFFFu)
    {
        lbCommonMakePositionFGM(own & 0x7FFFu, pos_x);
    }
}

void ndsP4UpdateHitDamageStats(const FTHitLog *hitlog, FTStruct *fp,
                               s32 damage_player, s32 damage_object_class,
                               s32 damage_object_kind, u16 flags,
                               u16 damage_stat_count)
{
    if (hitlog->attacker_object_class == nFTHitLogObjectFighter)
    {
        const FTStruct *attacker_fp = ftGetStruct(hitlog->attacker_gobj);
        u32 hitbox = (u32)((const FTAttackColl *)hitlog->attack_coll -
                           attacker_fp->attack_colls);

        if ((ndsP4Content(attacker_fp) != 0u) && (hitbox < 4u))
        {
            u32 dir = sNdsP4HitboxDir[attacker_fp->player & 3u][hitbox];

            if (dir == NDS_P4_HITBOX_BACKWARD)
            {
                fp->damage_lr = attacker_fp->lr;
            }
            else if (dir == NDS_P4_HITBOX_FORWARD)
            {
                fp->damage_lr = -attacker_fp->lr;
            }
        }
    }
    ftParamUpdate1PGameDamageStats(fp, damage_player, damage_object_class,
                                   damage_object_kind, flags,
                                   damage_stat_count);
}

/* The u16 at a byte address in a file the loader word-swapped. */
static u16 ndsP4ReadSwappedU16(const void *ptr)
{
    uintptr_t at = (uintptr_t)ptr;
    u32 word = *(const u32 *)(at & ~(uintptr_t)3u);

    return (u16)(((at & 2u) != 0u) ? word : (word >> 16));
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
        case 0xD1:
            /* SET ARMOUR (armour_), both tables: knockback-based armour,
             * the field Yoshi's double jump sets. */
            fp->knockback_resist_status = ndsP4HalfFloat(word);
            break;
        case 0xD2:
            /* OVERRIDE HITBOX DIRECTION, both tables: byte 2 the hitbox,
             * byte 3 the direction. */
            if (((word >> 8) & 0xFFu) < 4u)
            {
                sNdsP4HitboxDir[fp->player & 3u][(word >> 8) & 0xFFu] =
                    (u8)word;
            }
            break;
        case 0xD3:
            gNdsP4TranslationMultiplier[fp->player & 3u] = ndsP4HalfFloat(word);
            break;
        case 0xD4:
            /* SET Y VELOCITY; the fast-forward table skips it. */
            if (forward == FALSE)
            {
                fp->physics.vel_air.y = ndsP4HalfFloat(word);
            }
            break;
        case 0xD5:
            /* FAST FALL; the fast-forward table skips it. */
            if (forward == FALSE)
            {
                fp->is_fastfall = ((word & 0xFFu) != 0u) ? TRUE : FALSE;
            }
            break;
        case 0xD6:
            /* RANDOM SFX (play_sfx_random_), 8 bytes: byte 1 the chance in
             * 100, byte 2 sound (0) or voice (1), byte 3 the table size; the
             * second word points at the table of u16 sound ids. The
             * fast-forward table skips it. */
            length = 8u;
            if ((forward == FALSE) && (fp->is_muted == FALSE) &&
                ((u32)syUtilsRandIntRange(100) < ((word >> 16) & 0xFFu)))
            {
                const u8 *table = (const u8 *)(uintptr_t)p[1];
                u16 fgm_id = ndsP4ReadSwappedU16(
                    table + 2 * syUtilsRandIntRange((s32)(word & 0xFFu)));

                if (((word >> 8) & 0xFFu) == 1u)
                {
                    ftParamPlayVoice(fp, fgm_id);
                }
                else
                {
                    func_800269C0_275C0(fgm_id);
                }
            }
            break;
        case 0xD7:
            /* SET KINETIC STATE, both tables: aerial (byte 3 != 0) also
             * counts one jump used. */
            fp->jumps_used = ((word & 0xFFu) != 0u) ? 1u : 0u;
            fp->ga = ((word & 0xFFu) != 0u) ? nMPKineticsAir : nMPKineticsGround;
            break;
        case 0xD8:
            /* SET HITBOX FGM, both tables: byte 1's high nibble sets all four
             * hitboxes, else its low nibble names one; halfword 1 the id. */
            if (((word >> 20) & 0xFu) != 0u)
            {
                u32 i;

                for (i = 0u; i < 4u; i++)
                {
                    sNdsP4HitboxFgm[fp->player & 3u][i] = (u16)word;
                }
            }
            else if (((word >> 16) & 0xFu) < 4u)
            {
                sNdsP4HitboxFgm[fp->player & 3u][(word >> 16) & 0xFu] =
                    (u16)word;
            }
            break;
        case 0xD9: /* SET ENV COLOR: the fighter's draw colour, not ported */
            length = 8u;
            if (forward == FALSE)
            {
                gNdsP4UnportedMotionEvents++;
                gNdsP4UnportedMotionEventLast = word;
            }
            break;
        case 0xDA:
            /* SWITCH DIRECTION; the fast-forward table skips it. */
            if (forward == FALSE)
            {
                fp->lr = -fp->lr;
            }
            break;
        case 0xDC:
            /* L VOICE SFX, 8 bytes: the voice in halfword 1, or the one in
             * the second word's low half while the taunt button is held. The
             * fast-forward table skips it. */
            length = 8u;
            if (forward == FALSE)
            {
                ftParamPlayVoice(fp, ((fp->input.pl.button_hold &
                                       fp->input.button_mask_l) != 0u) ?
                                         (u16)p[1] : (u16)word);
            }
            break;
        default:
            /* E0+ are donor no-ops. */
            break;
        }
        p = (u32 *)((uintptr_t)p + length);
        ms->p_script = p;
    }
    return p;
}

#endif /* NDS_P4 */
