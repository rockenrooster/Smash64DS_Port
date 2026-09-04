/* 1P Game ladder tables, transcribed verbatim from BattleShip decomp.
 *
 * Plain textual include of sc1pgame.c does NOT work: the file is not
 * data-only. Past the initialized-data block it defines function bodies
 * (sc1PGameSetupFiles, sc1PGameFuncUpdate, sc1PGameSetupStageAll, ...) and
 * taskman/video globals (dSC1PGameTaskmanSetup, dSC1PGameVideoSetup) that
 * reference 1P scene state the port does not link yet. The ftchar data-slot
 * pattern (battleship_ftchar_data_slots.c) only works because those .c files
 * are pure asset-pointer storage. So the ladder tables are transcribed below
 * with source ordering and every field value preserved; no loops, no
 * rounding, no reordering.
 */

#if NDS_P2_1P_GAME

#include <ssb_types.h>
#include <ft/fighter.h>
#include <ft/ftcomputer.h>
#include <sc/scene.h>

/* decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pgame.c:27-36 */
u8 dSC1PGameKirbyTeamCopyKinds[] =
{
    nFTKindMario,
    nFTKindDonkey,
    nFTKindLink,
    nFTKindSamus,
    nFTKindYoshi,
    nFTKindFox,
    nFTKindPikachu
};

/* decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pgame.c:45-292 */
SC1PGameComputer dSC1PGameComputerDesc[] =
{
    // VS Link
    {
        TRUE,                       // Is team attack enabled?
        nSCBattleItemSwitchLow,     // Item Switch
#if defined(REGION_US)
        {  1,  2,  3,  6,  8 },     // Enemy CPU levels per difficulty setting
#else
        {  1,  1,  3,  6,  8 },     // Enemy CPU levels per difficulty setting
#endif
        {  9,  9,  9,  9,  9 },     // Enemy handicaps per difficulty setting
        {  1,  1,  1,  1,  1 },     // Ally CPU levels per difficulty setting
        {  9,  9,  9,  9,  9 }      // Ally handicaps per difficulty setting
    },

    // VS Yoshi Team
    {
        TRUE,                       // Is team attack enabled?
        nSCBattleItemSwitchMiddle,  // Item Switch
#if defined(REGION_US)
        {  1,  2,  4,  6,  8 },     // Enemy CPU levels per difficulty setting
#else
        {  1,  2,  3,  5,  6 },     // Enemy CPU levels per difficulty setting
#endif
        { 10, 11, 12, 13, 14 },     // Enemy handicaps per difficulty setting
        {  1,  1,  1,  1,  1 },     // Ally CPU levels per difficulty setting
        {  9,  9,  9,  9,  9 }      // Ally handicaps per difficulty setting
    },

    // VS Fox
    {
        TRUE,                       // Is team attack enabled?
        nSCBattleItemSwitchMiddle,  // Item Switch
#if defined(REGION_US)
        {  2,  3,  5,  7,  9 },     // Enemy CPU levels per difficulty setting
#else
        {  2,  3,  4,  6,  8 },     // Enemy CPU levels per difficulty setting
#endif
        {  9,  9,  9,  9,  9 },     // Enemy handicaps per difficulty setting
        {  1,  1,  1,  1,  1 },     // Ally CPU levels per difficulty setting
        {  9,  9,  9,  9,  9 }      // Ally handicaps per difficulty setting
    },

    // Bonus Stage: Break the Targets!
    {
        TRUE,                       // Is team attack enabled?
        nSCBattleItemSwitchNone,    // Item Switch
        {  1,  1,  1,  1,  1 },     // Enemy CPU levels per difficulty setting
        {  9,  9,  9,  9,  9 },     // Enemy handicaps per difficulty setting
        {  1,  1,  1,  1,  1 },     // Ally CPU levels per difficulty setting
        {  9,  9,  9,  9,  9 }      // Ally handicaps per difficulty setting
    },

    // VS Mario Bros.
    {
        FALSE,                      // Is team attack enabled?
        nSCBattleItemSwitchMiddle,  // Item Switch
#if defined(REGION_US)
        {  2,  3,  5,  7,  9 },     // Enemy CPU levels per difficulty setting
#else
        {  2,  3,  5,  6,  7 },     // Enemy CPU levels per difficulty setting
#endif
        {  9,  9,  9,  9,  9 },     // Enemy handicaps per difficulty setting
#if defined(REGION_US)
        {  5,  5,  5,  4,  2 },     // Ally CPU levels per difficulty setting
#else
        {  6,  5,  4,  3,  2 },     // Ally CPU levels per difficulty setting
#endif
        {  9,  9,  9,  9,  9 }      // Ally handicaps per difficulty setting
    },

    // VS Pikachu
    {
        TRUE,                       // Is team attack enabled?
        nSCBattleItemSwitchMiddle,  // Item Switch
        {  3,  4,  5,  7,  9 },     // Enemy CPU levels per difficulty setting
        {  9,  9,  9,  9,  9 },     // Enemy handicaps per difficulty setting
        {  1,  1,  1,  1,  1 },     // Ally CPU levels per difficulty setting
        {  9,  9,  9,  9,  9 }      // Ally handicaps per difficulty setting
    },

    // VS Giant Donkey Kong
    {
        FALSE,                      // Is team attack enabled?
        nSCBattleItemSwitchHigh,    // Item Switch
#if defined(REGION_US)
        {  2,  4,  6,  7,  8 },     // Enemy CPU levels per difficulty setting
#else
        {  2,  3,  4,  6,  7 },     // Enemy CPU levels per difficulty setting
#endif
        { 25, 26, 27, 28, 29 },     // Enemy handicaps per difficulty setting
#if defined(REGION_US)
        {  4,  4,  4,  3,  2 },     // Ally CPU levels per difficulty setting
        {  7,  7,  7,  7,  7 }      // Ally handicaps per difficulty setting
#else
        {  4,  3,  2,  1,  1 },     // Ally CPU levels per difficulty setting
        {  8,  7,  7,  7,  7 }      // Ally handicaps per difficulty setting
#endif
    },

    // Bonus Stage: Board the Platforms!
    {
        TRUE,                       // Is team attack enabled?
        nSCBattleItemSwitchNone,    // Item Switch
        {  1,  1,  1,  1,  1 },     // Enemy CPU levels per difficulty setting
        {  9,  9,  9,  9,  9 },     // Enemy handicaps per difficulty setting
        {  1,  1,  1,  1,  1 },     // Ally CPU levels per difficulty setting
        {  9,  9,  9,  9,  9 }      // Ally handicaps per difficulty setting
    },

    // VS Kirby Team
    {
        TRUE,                       // Is team attack enabled?
        nSCBattleItemSwitchMiddle,  // Item Switch
#if defined(REGION_US)
        {  3,  4,  5,  6,  7 },     // Enemy CPU levels per difficulty setting
#else
        {  2,  3,  4,  5,  7 },     // Enemy CPU levels per difficulty setting
#endif
        { 15, 16, 17, 18, 19 },     // Enemy handicaps per difficulty setting
        {  1,  1,  1,  1,  1 },     // Ally CPU levels per difficulty setting
        {  9,  9,  9,  9,  9 }      // Ally handicaps per difficulty setting
    },

    // VS Samus
    {
        TRUE,                       // Is team attack enabled?
        nSCBattleItemSwitchMiddle,  // Item Switch
#if defined(REGION_US)
        {  5,  6,  8,  9,  9 },     // Enemy CPU levels per difficulty setting
        {  9,  9,  9,  9, 40 },     // Enemy handicaps per difficulty setting
#else
        {  4,  5,  7,  8,  9 },     // Enemy CPU levels per difficulty setting
        {  9,  9,  9,  9,  9 },     // Enemy handicaps per difficulty setting
#endif
        {  1,  1,  1,  1,  1 },     // Ally CPU levels per difficulty setting
        {  9,  9,  9,  9,  9 }      // Ally handicaps per difficulty setting
    },

    // VS Metal Mario
    {
        TRUE,                       // Is team attack enabled?
        nSCBattleItemSwitchVeryLow, // Item Switch
#if defined(REGION_US)
        {  1,  3,  4,  6,  8 },     // Enemy CPU levels per difficulty setting
#else
        {  1,  2,  3,  4,  6 },     // Enemy CPU levels per difficulty setting
#endif
        { 30, 31, 32, 33, 34 },     // Enemy handicaps per difficulty setting
        {  1,  1,  1,  1,  1 },     // Ally CPU levels per difficulty setting
        {  9,  9,  9,  9,  9 }      // Ally handicaps per difficulty setting
    },

    // Bonus Stage: Race to the Finish!
    {
        TRUE,                       // Is team attack enabled?
        nSCBattleItemSwitchNone,    // Item Switch
#if defined(REGION_US)
        {  6,  8,  9,  9,  9 },     // Enemy CPU levels per difficulty setting
#else
        {  6,  8,  8,  9,  9 },     // Enemy CPU levels per difficulty setting
#endif
        {  1,  3,  5,  7,  9 },     // Enemy handicaps per difficulty setting
        {  1,  1,  1,  1,  1 },     // Ally CPU levels per difficulty setting
        {  9,  9,  9,  9,  9 }      // Ally handicaps per difficulty setting
    },

    // VS Fighting Polygon Team
    {
        TRUE,                       // Is team attack enabled?
        nSCBattleItemSwitchHigh,    // Item Switch
#if defined(REGION_US)
        {  2,  3,  4,  5,  7 },     // Enemy CPU levels per difficulty setting
#else
        {  2,  2,  3,  4,  5 },     // Enemy CPU levels per difficulty setting
#endif
        { 20, 21, 22, 23, 24 },     // Enemy handicaps per difficulty setting
        {  1,  1,  1,  1,  1 },     // Ally CPU levels per difficulty setting
        {  9,  9,  9,  9,  9 }      // Ally handicaps per difficulty setting
    },

    // Final Stage
    {
        TRUE,                       // Is team attack enabled?
        nSCBattleItemSwitchNone,    // Item Switch
        {  1,  2,  3,  4,  5 },     // Enemy CPU levels per difficulty setting
        { 35, 36, 37, 38, 39 },     // Enemy handicaps per difficulty setting
        {  1,  1,  1,  1,  1 },     // Ally CPU levels per difficulty setting
        {  9,  9,  9,  9,  9 }      // Ally handicaps per difficulty setting
    },

    // Challenger Approaching: Luigi
    {
        TRUE,                       // Is team attack enabled?
        nSCBattleItemSwitchNone,    // Item Switch
#if defined(REGION_US)
        {  6,  7,  7,  8,  9 },     // Enemy CPU levels per difficulty setting
#else
        {  6,  7,  7,  7,  8 },     // Enemy CPU levels per difficulty setting
#endif
        {  6,  6,  6,  6,  6 },     // Enemy handicaps per difficulty setting
        {  1,  1,  1,  1,  1 },     // Ally CPU levels per difficulty setting
        {  9,  9,  9,  9,  9 }      // Ally handicaps per difficulty setting
    },

    // Challenger Approaching: Ness
    {
        TRUE,                       // Is team attack enabled?
        nSCBattleItemSwitchNone,    // Item Switch
#if defined(REGION_US)
        {  6,  7,  7,  8,  9 },     // Enemy CPU levels per difficulty setting
#else
        {  6,  7,  7,  7,  8 },     // Enemy CPU levels per difficulty setting
#endif
        {  6,  6,  6,  6,  6 },     // Enemy handicaps per difficulty setting
        {  1,  1,  1,  1,  1 },     // Ally CPU levels per difficulty setting
        {  9,  9,  9,  9,  9 }      // Ally handicaps per difficulty setting
    },

    // Challenger Approaching: Jigglypuff
    {
        TRUE,                       // Is team attack enabled?
        nSCBattleItemSwitchNone,    // Item Switch
#if defined(REGION_US)
        {  6,  7,  7,  8,  9 },     // Enemy CPU levels per difficulty setting
#else
        {  6,  7,  7,  7,  8 },     // Enemy CPU levels per difficulty setting
#endif
        {  6,  6,  6,  6,  6 },     // Enemy handicaps per difficulty setting
        {  1,  1,  1,  1,  1 },     // Ally CPU levels per difficulty setting
        {  9,  9,  9,  9,  9 }      // Ally handicaps per difficulty setting
    },

    // Challenger Approaching: Captain Falcon
    {
        TRUE,                       // Is team attack enabled?
        nSCBattleItemSwitchNone,    // Item Switch
#if defined(REGION_US)
        {  6,  7,  7,  8,  9 },     // Enemy CPU levels per difficulty setting
#else
        {  6,  7,  7,  7,  8 },     // Enemy CPU levels per difficulty setting
#endif
        {  6,  6,  6,  6,  6 },     // Enemy handicaps per difficulty setting
        {  1,  1,  1,  1,  1 },     // Ally CPU levels per difficulty setting
        {  9,  9,  9,  9,  9 }      // Ally handicaps per difficulty setting
    }
};

/* decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pgame.c:295-566 */
SC1PGameStage dSC1PGameStageDesc[] =
{
    // VS Link
    {
        0xFF,                               // Screen flash alpha transparency
        nGRKindHyrule,                      // Stage ID
        0xFFFFFFFF,                         // Item toggles
        1,                                  // Number of enemies
        {
            nFTKindLink,                    // Enemy1 ID
            nFTKindNull                     // Enemy2 ID
        },
        nFTComputerTraitLink,               // Enemy trait
        0,                                  // Number of allies
        nFTComputerTraitDefault             // Ally trait
    },

    // VS Yoshi Team
    {
        0x80,                               // Screen flash alpha transparency
        nGRKindYosterSmall,                 // Stage ID
        0xFFFFFFFF,                         // Item toggles
        SC1PGAME_STAGE_YOSHI_TEAM_COUNT,    // Number of enemies
        {
            nFTKindYoshi,                   // Enemy1 ID
            nFTKindNull                     // Enemy2 ID
        },
        nFTComputerTraitYoshiTeam,          // Enemy trait
        0,                                  // Number of allies
        nFTComputerTraitDefault             // Ally trait
    },

    // VS Fox
    {
        0xFF,                               // Screen flash alpha transparency
        nGRKindSector,                      // Stage ID
        0xFFFFFFFF,                         // Item toggles
        1,                                  // Number of enemies
        {
            nFTKindFox,                     // Enemy Team Member 1 ID
            nFTKindNull                     // Enemy Team Member 2 ID
        },
        nFTComputerTraitDefault,            // Enemy trait
        0,                                  // Number of allies
        nFTComputerTraitDefault             // Ally trait
    },

    // Bonus Stage: Break the Targets!
    {
        0xFF,                               // Screen flash alpha transparency
        nGRKindCastle,                      // Stage ID
        0xFFFFFFFF,                         // Item toggles
        1,                                  // Number of enemies
        {
            nFTKindNull,                    // Enemy Team Member 1 ID
            nFTKindNull                     // Enemy Team Member 2 ID
        },
        nFTComputerTraitDefault,            // Enemy trait
        0,                                  // Number of allies
        nFTComputerTraitDefault             // Ally trait
    },

    // VS Mario Bros.
    {
        0xFF,                               // Screen flash alpha transparency
        nGRKindCastle,                      // Stage ID
        0xFFFFFFFF,                         // Item toggles
        2,                                  // Number of enemies
        {
            nFTKindMario,                   // Enemy Team Member 1 ID
            nFTKindLuigi                    // Enemy Team Member 2 ID
        },
        nFTComputerTraitMarioBros,          // Enemy trait
        1,                                  // Number of allies
        nFTComputerTraitAlly                // Ally trait
    },

    // VS Pikachu
    {
        0xFF,                               // Screen flash alpha transparency
        nGRKindYamabuki,                    // Stage ID
        0xFFFFFFFF,                         // Item toggles
        1,                                  // Number of enemies
        {
            nFTKindPikachu,                 // Enemy Team Member 1 ID
            nFTKindNull                     // Enemy Team Member 2 ID
        },
        nFTComputerTraitDefault,            // Enemy trait
        0,                                  // Number of allies
        nFTComputerTraitDefault             // Ally trait
    },

    // VS Giant Donkey Kong
    {
        0xFF,                               // Screen flash alpha transparency
        nGRKindJungle,                      // Stage ID
        0xFFFFFFFF,                         // Item toggles
        1,                                  // Number of enemies
        {
            nFTKindGDonkey,                 // Enemy Team Member 1 ID
            nFTKindNull                     // Enemy Team Member 2 ID
        },
        nFTComputerTraitGDonkey,            // Enemy trait
        2,                                  // Number of allies
        nFTComputerTraitAlly                // Ally trait
    },

    // Bonus Stage: Board the Platforms!
    {
        0xFF,                               // Screen flash alpha transparency
        nGRKindCastle,                      // Stage ID
        0xFFFFFFFF,                         // Item toggles
        1,                                  // Number of enemies
        {
            nFTKindNull,                    // Enemy Team Member 1 ID
            nFTKindNull                     // Enemy Team Member 2 ID
        },
        nFTComputerTraitDefault,            // Enemy trait
        0,                                  // Number of allies
        nFTComputerTraitDefault             // Ally trait
    },

    // VS Kirby Team
    {
        0x80,                               // Screen flash alpha transparency
        nGRKindPupupu,                      // Stage ID
        0xFFFFFFFF,                         // Item toggles
        SC1PGAME_STAGE_KIRBY_TEAM_COUNT,    // Number of enemies
        {
            nFTKindKirby,                   // Enemy Team Member 1 ID
            nFTKindNull                     // Enemy Team Member 2 ID
        },
        nFTComputerTraitKirbyTeam,          // Enemy trait
        0,                                  // Number of allies
        nFTComputerTraitDefault             // Ally trait
    },

    // VS Samus
    {
        0xFF,                               // Screen flash alpha transparency
        nGRKindZebes,                       // Stage ID
        0xFFFFFFFF,                         // Item toggles
        1,                                  // Number of enemies
        {
            nFTKindSamus,                   // Enemy Team Member 1 ID
            nFTKindNull                     // Enemy Team Member 2 ID
        },
        nFTComputerTraitDefault,            // Enemy trait
        0,                                  // Number of allies
        nFTComputerTraitDefault             // Ally trait
    },

    // VS Metal Mario
    {
        0xFF,                               // Screen flash alpha transparency
        nGRKindMetal,                       // Stage ID
        0xFFFFFFFF,                         // Item toggles
        1,                                  // Number of enemies
        {
            nFTKindMMario,                  // Enemy Team Member 1 ID
            nFTKindNull                     // Enemy Team Member 2 ID
        },
        nFTComputerTraitDefault,            // Enemy trait
        0,                                  // Number of allies
        nFTComputerTraitDefault             // Ally trait
    },

    // Bonus Stage: Race to the Finish!
    {
        0xFF,                               // Screen flash alpha transparency
        nGRKindBonus3,                      // Stage ID
        0xFFFFFFFF,                         // Item toggles
        3,                                  // Number of enemies
        {
            nFTKindNull,                    // Enemy Team Member 1 ID
            nFTKindNull                     // Enemy Team Member 2 ID
        },
        nFTComputerTraitBonus3,             // Enemy trait
        0,                                  // Number of allies
        nFTComputerTraitDefault             // Ally trait
    },

    // VS Fighting Polygon Team
    {
        0x80,                               // Screen flash alpha transparency
        nGRKindZako,                        // Stage ID
        0xFFFFFFFF,                         // Item toggles
        SC1PGAME_STAGE_MAX_TEAM_COUNT,      // Number of enemies
        {
            nFTKindNull,                    // Enemy Team Member 1 ID
            nFTKindNull                     // Enemy Team Member 2 ID
        },
        nFTComputerTraitPolyTeam,           // Enemy trait
        0,                                  // Number of allies
        nFTComputerTraitDefault             // Ally trait
    },

    // Final Stage
    {
        0xFF,                               // Screen flash alpha transparency
        nGRKindLast,                        // Stage ID
        0xFFFFFFFF,                         // Item toggles
        1,                                  // Number of enemies
        {
            nFTKindBoss,                    // Enemy Team Member 1 ID
            nFTKindNull                     // Enemy Team Member 2 ID
        },
        nFTComputerTraitDefault,            // Enemy trait
        0,                                  // Number of allies
        nFTComputerTraitDefault             // Ally trait
    },

    // Challenger Approaching: Luigi
    {
        0xFF,                               // Screen flash alpha transparency
        nGRKindCastle,                      // Stage ID
        0xFFFFFFFF,                         // Item toggles
        1,                                  // Number of enemies
        {
            nFTKindLuigi,                   // Enemy Team Member 1 ID
            nFTKindNull                     // Enemy Team Member 2 ID
        },
        nFTComputerTraitDefault,            // Enemy trait
        0,                                  // Number of allies
        nFTComputerTraitDefault             // Ally trait
    },

    // Challenger Approaching: Ness
    {
        0xFF,                               // Screen flash alpha transparency
        nGRKindPupupu,                      // Stage ID
        0xFFFFFFFF,                         // Item toggles
        1,                                  // Number of enemies
        {
            nFTKindNess,                    // Enemy Team Member 1 ID
            nFTKindNull                     // Enemy Team Member 2 ID
        },
        nFTComputerTraitDefault,            // Enemy trait
        0,                                  // Number of allies
        nFTComputerTraitDefault             // Ally trait
    },

    // Challenger Approaching: Jigglypuff
    {
        0xFF,                               // Screen flash alpha transparency
        nGRKindYamabuki,                    // Stage ID
        0xFFFFFFFF,                         // Item toggles
        1,                                  // Number of enemies
        {
            nFTKindPurin,                   // Enemy Team Member 1 ID
            nFTKindNull                     // Enemy Team Member 2 ID
        },
        nFTComputerTraitDefault,            // Enemy trait
        0,                                  // Number of allies
        nFTComputerTraitDefault             // Ally trait
    },

    // Challenger Approaching: Captain Falcon
    {
        0xFF,                               // Screen flash alpha transparency
        nGRKindZebes,                       // Stage ID
        0xFFFFFFFF,                         // Item toggles
        1,                                  // Number of enemies
        {
            nFTKindCaptain,                 // Enemy Team Member 1 ID
            nFTKindNull                     // Enemy Team Member 2 ID
        },
        nFTComputerTraitDefault,            // Enemy trait
        0,                                  // Number of allies
        nFTComputerTraitDefault             // Ally trait
    }
};

#endif /* NDS_P2_1P_GAME */
