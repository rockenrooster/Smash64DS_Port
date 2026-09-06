/* --- Screen: VS Options --------------------------------------------------
 *
 * Five rows in the source's own enum order (decomp mn/mndef.h:180-188):
 * Handicap, TeamAttack, StageSelect, Damage, ItemSwitch. UP and DOWN walk the
 * five with wraparound, LEFT and RIGHT edit the row under the cursor, A or
 * START on the ItemSwitch row opens it, and B commits and leaves. Transcribed
 * from decomp mn/mnvsmode/mnvsoptions.c:
 *
 *   cursor   UP from Handicap wraps to ItemSwitch, DOWN from ItemSwitch wraps
 *            to Handicap (:1314/:1341), cue MenuScroll2.
 *   entry    the cursor starts on ItemSwitch when the previous scene was
 *            nSCKindVSItemSwitch and on Handicap otherwise (:1173).
 *   handi-   LEFT: Off -> Auto, Auto -> On, On stays; the cue is spent only
 *   cap      when the value actually changes (:1364-1376). RIGHT: On -> Auto,
 *            Auto -> Off, Off stays, same cue rule (:1432-1444). A walks
 *            Off -> On -> Auto -> Off and always cues (:1495-1512).
 *            Cue MenuScroll1 in all three cases.
 *   team /   LEFT turns On only when Off, RIGHT turns Off only when On, and A
 *   stage    flips either way; the cue is spent only on a real change for
 *            LEFT/RIGHT and always for A (:1378-1402/:1446-1470/:1514-1530).
 *            Cue MenuScroll1.
 *   damage   a percentage, 50..200 step 1, and it WRAPS both ways:
 *            decrementing at 50 gives 200 (:1407-1411) and incrementing at 200
 *            gives 50 (:1473-1477). LEFT/RIGHT always cue MenuScroll1. A does
 *            nothing on this row -- the source's A handler has no Damage arm
 *            (:1491-1531).
 *   item     A or START opens nSCKindVSItemSwitch (:1284-1289), cue
 *   switch   MenuSelect. LEFT/RIGHT do nothing on this row.
 *   B        commits and returns to nSCKindVSMode (:1294-1301), and the source
 *            spends no cue on it.
 *
 * WHAT IT COMMITS is the four descriptor fields this screen owns --
 * handicap_mode, is_team_attack, is_stage_select and damage_ratio -- written
 * into gNdsMatchConfig and installed with ndsMatchConfigApply, as every other
 * screen in this shell does. The initial values come back out of the same
 * descriptor, so a return trip shows what the last visit committed
 * (mnVSOptionsInitVars, :1175-1178).
 *
 * THE FIVE ROWS ARE ALWAYS PRESENT. The source hides the ItemSwitch row while
 * the item switch is still locked (sMNVSOptionsIsHaveItemSwitch, :1182-1191);
 * this build has no lock progression, so there is nothing to gate on and the
 * cursor always walks all five.
 *
 * THE CURSOR IS THE SELECTED ROW'S BAKE. The source marks it with the
 * bubble HIGHLIGHT pair plus a red underline (mnVSOptionsSetOptionSpriteColors
 * :251, mnVSOptionsUnderlineProcDisplay :887), so each row state ships a HI
 * twin: the bubble in the HI colours with the underline baked under the
 * active value (handicap/team/stage), bubble HI alone where the source draws
 * no underline (damage/item switch, whose rows have no underline arm).
 *
 * HOW THE DAMAGE NUMBER IS DRAWN. ndsUiKitSetNumber is NOT used here, even
 * though it is what the VS rules value and the CSS CPU level go through. It
 * hardcodes the white NDS_MN_UI_KIT_IMAGE_DIGIT_0..9 cells
 * (nds_ui_kit.c:761), while the source draws the damage digits in pink
 * (0xFF/0x00/0x28, :443-447) with a black percent glyph beside them
 * (:499-509). The bake therefore ships ten pink digit images plus a black
 * percent image (VS_OPTIONS_DIGIT_0..9, VS_OPTIONS_PERCENT), and this screen
 * places them with ndsUiKitSetSprite under SetNumber's own contract: the ones
 * digit at right - 11, each further place 11 left, the percent fixed --
 * rather than routing a pink number through a white-digit path.
 */

#define NDS_MENU_VSOPTIONS_ROWS 5u
#define NDS_MENU_VSOPTIONS_HANDICAP 0u
#define NDS_MENU_VSOPTIONS_TEAM 1u
#define NDS_MENU_VSOPTIONS_STAGE 2u
#define NDS_MENU_VSOPTIONS_DAMAGE 3u
#define NDS_MENU_VSOPTIONS_ITEMSWITCH 4u

/* mnvsoptions.c:1407-1411 and :1473-1477. Both ends wrap; neither clamps. */
#define NDS_MENU_VSOPTIONS_DAMAGE_MIN 50u
#define NDS_MENU_VSOPTIONS_DAMAGE_MAX 200u

/* The damage number's geometry, in the source's own 320x240 frame: the digit
 * GObj is built at right edge 220 (the ones digit lands at 220 - 11 and each
 * further place walks 11 left, :221-247) at y 151 (:450), and the percent
 * glyph sits fixed at (226, 151) (:499-509). NDS_MENU_VS_DS lands them on the
 * DS frame exactly as the VS rules screen's own numbers land. */
#define NDS_MENU_VSOPTIONS_DAMAGE_RIGHT NDS_MENU_VS_DS(220)
#define NDS_MENU_VSOPTIONS_DAMAGE_Y NDS_MENU_VS_DS(151)
#define NDS_MENU_VSOPTIONS_PERCENT_X NDS_MENU_VS_DS(226)

/* The composed number rides the kit's number slots; the percent takes the
 * left arrow slot, and the right arrow slot is hidden at populate because
 * this screen shows no arrows. */
#define NDS_MENU_VSOPTIONS_DIGIT_SLOT NDS_MENU_SPRITE_NUM0
#define NDS_MENU_VSOPTIONS_PERCENT_SLOT NDS_MENU_SPRITE_ARROW_L

static u32 sMenuVsOptionsCursor;
/* Handicap holds nSCBattleHandicapOff/On/Auto, team and stage hold 0/1, and
 * damage holds the percentage. Separate statics rather than one array because
 * the four have different domains; the source keeps four statics too
 * (sMNVSOptionsHandicapStatus and friends). */
static u8 sMenuVsOptionsHandicap;
static u8 sMenuVsOptionsTeam;
static u8 sMenuVsOptionsStage;
static u8 sMenuVsOptionsDamage;
/* What is currently ON SCREEN, so a frame that changed nothing blits nothing
 * and a change blits exactly the row that changed. Same discipline as the
 * Item Switch screen's row surfaces. The damage row's cache entry is its
 * static label; the digits over it are OBJ and follow in
 * ndsMenuShellVsOptionsDrawDamage. */
static NdsUiKitSurfaceId sMenuVsOptionsRowSurface[NDS_MENU_VSOPTIONS_ROWS];

__attribute__((used)) volatile u32 gNdsMenuShellVsOptionsBlitCount;
__attribute__((used)) volatile u32 gNdsMenuShellVsOptionsCommitCount;
__attribute__((used)) volatile u32 gNdsMenuShellVsOptionsCommitHandicap;
__attribute__((used)) volatile u32 gNdsMenuShellVsOptionsCommitDamage;

static const NdsUiKitSurfaceId kNdsMenuVsOptionsPlate[] = {
    NDS_MN_UI_KIT_SURFACE_VS_OPTIONS
};

/* Which surface each row SHOULD be showing, given its value and the cursor.
 * The handicap row has one full-row bake per state; team and stage have one
 * per on/off; damage and item switch are static -- damage's number is
 * composed OBJ, not a surface, because baking one surface per value would be
 * 151 of them. The row under the cursor shows the HI twin (bubble in the
 * HIGHLIGHT pair with the red underline, :251/:887); every other row shows
 * the NOT twin. */
static NdsUiKitSurfaceId ndsMenuShellVsOptionsWantSurface(u32 row)
{
    u32 hi = (row == sMenuVsOptionsCursor) ? 1u : 0u;

    switch (row)
    {
    case NDS_MENU_VSOPTIONS_HANDICAP:
        switch (sMenuVsOptionsHandicap)
        {
        case nSCBattleHandicapOn:
            return (hi != 0u) ?
                NDS_MN_UI_KIT_SURFACE_VS_OPTIONS_HANDICAP_ON_HI :
                NDS_MN_UI_KIT_SURFACE_VS_OPTIONS_HANDICAP_ON;
        case nSCBattleHandicapAuto:
            return (hi != 0u) ?
                NDS_MN_UI_KIT_SURFACE_VS_OPTIONS_HANDICAP_AUTO_HI :
                NDS_MN_UI_KIT_SURFACE_VS_OPTIONS_HANDICAP_AUTO;
        default:
            break;
        }
        return (hi != 0u) ?
            NDS_MN_UI_KIT_SURFACE_VS_OPTIONS_HANDICAP_OFF_HI :
            NDS_MN_UI_KIT_SURFACE_VS_OPTIONS_HANDICAP_OFF;
    case NDS_MENU_VSOPTIONS_TEAM:
        if (sMenuVsOptionsTeam != 0u)
        {
            return (hi != 0u) ?
                NDS_MN_UI_KIT_SURFACE_VS_OPTIONS_TEAM_ON_HI :
                NDS_MN_UI_KIT_SURFACE_VS_OPTIONS_TEAM_ON;
        }
        return (hi != 0u) ?
            NDS_MN_UI_KIT_SURFACE_VS_OPTIONS_TEAM_OFF_HI :
            NDS_MN_UI_KIT_SURFACE_VS_OPTIONS_TEAM_OFF;
    case NDS_MENU_VSOPTIONS_STAGE:
        if (sMenuVsOptionsStage != 0u)
        {
            return (hi != 0u) ?
                NDS_MN_UI_KIT_SURFACE_VS_OPTIONS_STAGE_ON_HI :
                NDS_MN_UI_KIT_SURFACE_VS_OPTIONS_STAGE_ON;
        }
        return (hi != 0u) ?
            NDS_MN_UI_KIT_SURFACE_VS_OPTIONS_STAGE_OFF_HI :
            NDS_MN_UI_KIT_SURFACE_VS_OPTIONS_STAGE_OFF;
    case NDS_MENU_VSOPTIONS_DAMAGE:
        return (hi != 0u) ?
            NDS_MN_UI_KIT_SURFACE_VS_OPTIONS_DAMAGE_LABEL_HI :
            NDS_MN_UI_KIT_SURFACE_VS_OPTIONS_DAMAGE_LABEL;
    default:
        break;
    }
    return (hi != 0u) ?
        NDS_MN_UI_KIT_SURFACE_VS_OPTIONS_ITEM_SWITCH_HI :
        NDS_MN_UI_KIT_SURFACE_VS_OPTIONS_ITEM_SWITCH;
}

/* At most `budget` successful row blits per call. Zero means no work.
 * A cursor move changes two rows; entry may change all five. Only remember
 * a row after a successful blit, so a refused surface remains dirty. */
static void ndsMenuShellVsOptionsSyncRows(u32 budget)
{
    u32 row;

    for (row = 0u;
         (row < NDS_MENU_VSOPTIONS_ROWS) && (budget != 0u);
         row++)
    {
        NdsUiKitSurfaceId want = ndsMenuShellVsOptionsWantSurface(row);

        if (want != sMenuVsOptionsRowSurface[row])
        {
            if (ndsUiKitBlitSurfaces(&want, 1u) == FALSE)
            {
                return;
            }
            sMenuVsOptionsRowSurface[row] = want;
            gNdsMenuShellVsOptionsBlitCount++;
            budget--;
        }
    }
}

/* The composed damage number: pink digits at SetNumber's own 11 px pitch from
 * the right edge, black percent fixed beside them. The hundreds place is
 * hidden below 100 so a two-digit value cannot inherit it. */
static void ndsMenuShellVsOptionsDrawDamage(void)
{
    u32 value = (u32)sMenuVsOptionsDamage;
    s32 right = (s32)NDS_MENU_VSOPTIONS_DAMAGE_RIGHT;
    s32 y = (s32)NDS_MENU_VSOPTIONS_DAMAGE_Y;

    (void)ndsUiKitSetSprite(NDS_MENU_VSOPTIONS_DIGIT_SLOT,
                            NDS_MN_UI_KIT_IMAGE_VS_OPTIONS_DIGIT_0 +
                                (value % 10u),
                            right - (s32)NDS_UI_KIT_DIGIT_PITCH, y);
    (void)ndsUiKitSetSprite(NDS_MENU_VSOPTIONS_DIGIT_SLOT + 1u,
                            NDS_MN_UI_KIT_IMAGE_VS_OPTIONS_DIGIT_0 +
                                ((value / 10u) % 10u),
                            right - (s32)(2u * NDS_UI_KIT_DIGIT_PITCH), y);
    if (value >= 100u)
    {
        (void)ndsUiKitSetSprite(NDS_MENU_VSOPTIONS_DIGIT_SLOT + 2u,
                                NDS_MN_UI_KIT_IMAGE_VS_OPTIONS_DIGIT_0 +
                                    (value / 100u),
                                right - (s32)(3u * NDS_UI_KIT_DIGIT_PITCH), y);
    }
    else
    {
        ndsUiKitHideSprite(NDS_MENU_VSOPTIONS_DIGIT_SLOT + 2u);
    }
    (void)ndsUiKitSetSprite(NDS_MENU_VSOPTIONS_PERCENT_SLOT,
                            NDS_MN_UI_KIT_IMAGE_VS_OPTIONS_PERCENT,
                            (s32)NDS_MENU_VSOPTIONS_PERCENT_X, y);
}

/* mnVSOptionsInitVars (:1171-1178): the screen opens on what the descriptor
 * already carries, so a return trip shows what the last visit committed, and
 * the cursor restarts on the ItemSwitch row after coming back from it. */
static void ndsMenuShellVsOptionsLoad(void)
{
    u32 row;

    sMenuVsOptionsCursor =
        (((u8)gSCManagerSceneData.scene_prev == (u8)nSCKindVSItemSwitch) ?
         NDS_MENU_VSOPTIONS_ITEMSWITCH : NDS_MENU_VSOPTIONS_HANDICAP);
    sMenuVsOptionsHandicap = gNdsMatchConfig.handicap_mode;
    sMenuVsOptionsTeam = (gNdsMatchConfig.is_team_attack != FALSE) ? 1u : 0u;
    sMenuVsOptionsStage = (gNdsMatchConfig.is_stage_select != FALSE) ? 1u : 0u;
    sMenuVsOptionsDamage = gNdsMatchConfig.damage_ratio;
    for (row = 0u; row < NDS_MENU_VSOPTIONS_ROWS; row++)
    {
        /* Nothing is on the freshly cleared BG2, so every row differs and all
         * five blit across the entry frames. */
        sMenuVsOptionsRowSurface[row] = NDS_MENU_VS_SURFACE_NONE;
    }
}

/* mnVSOptionsSetAllSettings (:1199-1204), through the descriptor rather than
 * straight into the battle state, as every other screen in this shell does. */
static void ndsMenuShellVsOptionsSave(void)
{
    gNdsMatchConfig.handicap_mode = sMenuVsOptionsHandicap;
    gNdsMatchConfig.is_team_attack = sMenuVsOptionsTeam;
    gNdsMatchConfig.is_stage_select = sMenuVsOptionsStage;
    gNdsMatchConfig.damage_ratio = sMenuVsOptionsDamage;
    ndsMatchConfigApply(&gNdsMatchConfig);

    gNdsMenuShellVsOptionsCommitCount++;
    gNdsMenuShellVsOptionsCommitHandicap =
        (u32)gNdsMatchConfig.handicap_mode;
    gNdsMenuShellVsOptionsCommitDamage = (u32)gNdsMatchConfig.damage_ratio;
}

static void ndsMenuShellPopulateVsOptions(void)
{
    (void)ndsUiKitBlitSurfaces(kNdsMenuVsOptionsPlate, 1u);
    ndsMenuShellVsOptionsSyncRows(NDS_MENU_VSOPTIONS_ROWS);
    ndsUiKitHideSprite(NDS_MENU_SPRITE_ARROW_R);
    ndsMenuShellVsOptionsDrawDamage();
}

/* LEFT and RIGHT on the row under the cursor. Handicap walks its three states
 * in opposite directions and stops at the far end; team and stage turn one
 * way each; damage walks 50..200 and wraps both ends. */
static void ndsMenuShellVsOptionsAdjust(s32 direction)
{
    switch (sMenuVsOptionsCursor)
    {
    case NDS_MENU_VSOPTIONS_HANDICAP:
        if (direction < 0)
        {
            if (sMenuVsOptionsHandicap != (u8)nSCBattleHandicapOn)
            {
                sMenuVsOptionsHandicap =
                    (sMenuVsOptionsHandicap == (u8)nSCBattleHandicapOff) ?
                    (u8)nSCBattleHandicapAuto : (u8)nSCBattleHandicapOn;
                ndsUiKitSfx(NDS_UI_KIT_SFX_VALUE);
            }
        }
        else if (sMenuVsOptionsHandicap != (u8)nSCBattleHandicapOff)
        {
            sMenuVsOptionsHandicap =
                (sMenuVsOptionsHandicap == (u8)nSCBattleHandicapOn) ?
                (u8)nSCBattleHandicapAuto : (u8)nSCBattleHandicapOff;
            ndsUiKitSfx(NDS_UI_KIT_SFX_VALUE);
        }
        break;
    case NDS_MENU_VSOPTIONS_TEAM:
        if ((direction < 0) && (sMenuVsOptionsTeam == 0u))
        {
            sMenuVsOptionsTeam = 1u;
            ndsUiKitSfx(NDS_UI_KIT_SFX_VALUE);
        }
        else if ((direction > 0) && (sMenuVsOptionsTeam != 0u))
        {
            sMenuVsOptionsTeam = 0u;
            ndsUiKitSfx(NDS_UI_KIT_SFX_VALUE);
        }
        break;
    case NDS_MENU_VSOPTIONS_STAGE:
        if ((direction < 0) && (sMenuVsOptionsStage == 0u))
        {
            sMenuVsOptionsStage = 1u;
            ndsUiKitSfx(NDS_UI_KIT_SFX_VALUE);
        }
        else if ((direction > 0) && (sMenuVsOptionsStage != 0u))
        {
            sMenuVsOptionsStage = 0u;
            ndsUiKitSfx(NDS_UI_KIT_SFX_VALUE);
        }
        break;
    case NDS_MENU_VSOPTIONS_DAMAGE:
        if (direction < 0)
        {
            sMenuVsOptionsDamage =
                (sMenuVsOptionsDamage == (u8)NDS_MENU_VSOPTIONS_DAMAGE_MIN) ?
                (u8)NDS_MENU_VSOPTIONS_DAMAGE_MAX :
                (u8)(sMenuVsOptionsDamage - 1u);
        }
        else
        {
            sMenuVsOptionsDamage =
                (sMenuVsOptionsDamage == (u8)NDS_MENU_VSOPTIONS_DAMAGE_MAX) ?
                (u8)NDS_MENU_VSOPTIONS_DAMAGE_MIN :
                (u8)(sMenuVsOptionsDamage + 1u);
        }
        ndsUiKitSfx(NDS_UI_KIT_SFX_VALUE);
        ndsMenuShellVsOptionsDrawDamage();
        break;
    default:
        break;
    }
}

/* A on a value row. Handicap cycles Off -> On -> Auto -> Off; team and stage
 * flip. The source reads A alone here, not A-or-START (:1491); START only
 * opens the ItemSwitch row above. */
static void ndsMenuShellVsOptionsConfirm(void)
{
    switch (sMenuVsOptionsCursor)
    {
    case NDS_MENU_VSOPTIONS_HANDICAP:
        ndsUiKitSfx(NDS_UI_KIT_SFX_VALUE);
        if (sMenuVsOptionsHandicap == (u8)nSCBattleHandicapOff)
        {
            sMenuVsOptionsHandicap = (u8)nSCBattleHandicapOn;
        }
        else if (sMenuVsOptionsHandicap == (u8)nSCBattleHandicapAuto)
        {
            sMenuVsOptionsHandicap = (u8)nSCBattleHandicapOff;
        }
        else
        {
            sMenuVsOptionsHandicap = (u8)nSCBattleHandicapAuto;
        }
        break;
    case NDS_MENU_VSOPTIONS_TEAM:
        ndsUiKitSfx(NDS_UI_KIT_SFX_VALUE);
        sMenuVsOptionsTeam = (sMenuVsOptionsTeam != 0u) ? 0u : 1u;
        break;
    case NDS_MENU_VSOPTIONS_STAGE:
        ndsUiKitSfx(NDS_UI_KIT_SFX_VALUE);
        sMenuVsOptionsStage = (sMenuVsOptionsStage != 0u) ? 0u : 1u;
        break;
    default:
        break;
    }
}

static void ndsMenuShellUpdateVsOptions(u32 held, u32 taps)
{
    u32 moved = FALSE;

    ndsMenuShellVsOptionsSyncRows(1u);

    if (ndsMenuShellDirection(held, taps, NDS_INPUT_UP) != FALSE)
    {
        sMenuVsOptionsCursor = (sMenuVsOptionsCursor ==
                                NDS_MENU_VSOPTIONS_HANDICAP) ?
            (NDS_MENU_VSOPTIONS_ROWS - 1u) : (sMenuVsOptionsCursor - 1u);
        moved = TRUE;
    }
    else if (ndsMenuShellDirection(held, taps, NDS_INPUT_DOWN) != FALSE)
    {
        sMenuVsOptionsCursor =
            (sMenuVsOptionsCursor + 1u) % NDS_MENU_VSOPTIONS_ROWS;
        moved = TRUE;
    }
    if (moved != FALSE)
    {
        /* Selection is baked per row, so a cursor move changes two rows:
         * the old one back to NOT and the new one to HI. Re-sync both now
         * rather than one per frame. */
        ndsMenuShellVsOptionsSyncRows(NDS_MENU_VSOPTIONS_ROWS);
        ndsUiKitSfx(NDS_UI_KIT_SFX_MOVE);
    }
    else if (ndsMenuShellDirection(held, taps, NDS_INPUT_LEFT) != FALSE)
    {
        ndsMenuShellVsOptionsAdjust(-1);
    }
    else if (ndsMenuShellDirection(held, taps, NDS_INPUT_RIGHT) != FALSE)
    {
        ndsMenuShellVsOptionsAdjust(1);
    }

    /* A or START on the ItemSwitch row opens it (:1284-1289). */
    if (((taps & (NDS_INPUT_A | NDS_INPUT_START)) != 0u) &&
        (sMenuVsOptionsCursor == NDS_MENU_VSOPTIONS_ITEMSWITCH))
    {
        ndsUiKitSfx(NDS_UI_KIT_SFX_CONFIRM);
        ndsMenuShellVsOptionsSave();
        ndsMenuShellGoto((u32)nSCKindVSItemSwitch);
        return;
    }
    /* B commits and returns to the VS menu (:1294-1301). A and START do
     * nothing elsewhere here except confirm a value row below. */
    if ((taps & NDS_INPUT_B) != 0u)
    {
        ndsMenuShellVsOptionsSave();
        ndsMenuShellGoto((u32)nSCKindVSMode);
        return;
    }
    if ((taps & NDS_INPUT_A) != 0u)
    {
        ndsMenuShellVsOptionsConfirm();
    }
}
