/*
 * BattleShip fighter submotion descriptor data.
 *
 * ft/ftdata.c references every dFT*SubMotionDescs array, so import the
 * source data as a group without importing scsubsys runtime code.
 */
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdataboss.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatacaptain.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatadonkey.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatafox.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatagdonkey.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatakirby.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatalink.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdataluigi.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatamario.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatammario.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatancaptain.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatandonkey.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdataness.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatanfox.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatankirby.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatanlink.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatanluigi.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatanmario.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatanness.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatanpikachu.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatanpurin.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatansamus.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatanyoshi.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatapikachu.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatapurin.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatasamus.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatayoshi.c"

/* PlayersVS selection uses demo submotions outside the contiguous gameplay
 * animation banks. `mnPlayersVSGetStatusSelected` chooses Win3 for Mario,
 * Win4 for Fox and Win1 for Luigi; `ftMainSetStatus` maps those through the
 * identity D_ovl1_80390BE8 table to submotion 3/4/1 respectively. Those are
 * Mario Selected (reloc 359), Fox Selected (372), and Luigi Selected (462) --
 * importantly NOT Luigi's separate Win1 victory clip (463). The decomp
 * contains their complete AObjEvent16 FIGATREEs. Keep
 * these exact source tables resident and let the reloc seam copy only the
 * top-level joint-pointer array into each fighter's normal figatree heap. */
/* relocdata_types.h also pulls the decomp's full mp/mptypes.h, which this port
 * intentionally replaces with project-owned ABI-compatible map declarations.
 * The two animation data files need only the AObjEvent16 command encoders, so
 * provide that exact narrow subset here and suppress the heavyweight header. */
#define _RELOCDATA_TYPES_H_
#define FT_ANIM_ROTX (1 << 0)
#define FT_ANIM_ROTY (1 << 1)
#define FT_ANIM_ROTZ (1 << 2)
#define FT_ANIM_TRAX (1 << 4)
#define FT_ANIM_TRAY (1 << 5)
#define FT_ANIM_TRAZ (1 << 6)
#define FT_ANIM_SCAX (1 << 7)
#define FT_ANIM_SCAY (1 << 8)
#define FT_ANIM_SCAZ (1 << 9)
/* relocdata_types.h's source macro emits the N64/O2R file representation:
 * opcode[15:11], flags[10:1], toggle[0].  Normal fighter animation files enter
 * the DS through ndsRelocNormalizeAObj16Script, which repacks that header for
 * ARM GCC's native AObjEvent16 bitfields (objtypes.h: opcode[4:0],
 * flags[14:5], toggle[15]).  These two Selected clips are compiled directly
 * from the decomp instead of passing through that reloc-file normalizer, so do
 * the SAME representation conversion here at compile time.  Values, durations,
 * command ordering and source joint tables remain byte-for-byte semantic source
 * data; only the device-native command header layout changes. */
#define _FT_ANIM_CMD(op, flags, toggle) \
    ((op) | ((flags) << 5) | ((toggle) << 15))
_Static_assert(_FT_ANIM_CMD(5, FT_ANIM_ROTZ, 0) == 0x0085,
               "Selected FIGATREE command header must use ARM AObjEvent16 layout");
#define ftAnimEnd() _FT_ANIM_CMD(0, 0, 0)
#define ftAnimBlock(flags, dur) _FT_ANIM_CMD(1, flags, 1), (dur)
#define ftAnimSetValBlockT(flags, dur) _FT_ANIM_CMD(2, flags, 1), (dur)
#define ftAnimSetValBlock(flags) _FT_ANIM_CMD(2, flags, 0)
#define ftAnimSetValT(flags, dur) _FT_ANIM_CMD(3, flags, 1), (dur)
#define ftAnimSetVal(flags) _FT_ANIM_CMD(3, flags, 0)
#define ftAnimSetValRateBlockT(flags, dur) _FT_ANIM_CMD(4, flags, 1), (dur)
#define ftAnimSetValRateBlock(flags) _FT_ANIM_CMD(4, flags, 0)
#define ftAnimSetValRateT(flags, dur) _FT_ANIM_CMD(5, flags, 1), (dur)
#define ftAnimSetValRate(flags) _FT_ANIM_CMD(5, flags, 0)
#define ftAnimSetTargetRateBlock(flags) _FT_ANIM_CMD(6, flags, 0)
#define ftAnimSetVal0RateBlockT(flags, dur) _FT_ANIM_CMD(7, flags, 1), (dur)
#define ftAnimSetVal0RateBlock(flags) _FT_ANIM_CMD(7, flags, 0)
#define ftAnimSetVal0RateT(flags, dur) _FT_ANIM_CMD(8, flags, 1), (dur)
#define ftAnimSetVal0Rate(flags) _FT_ANIM_CMD(8, flags, 0)
#define ftAnimSetValAfterBlock(flags) _FT_ANIM_CMD(9, flags, 0)
#define ftAnimSetValAfter(flags) _FT_ANIM_CMD(10, flags, 0)
#include "../../decomp/BattleShip-main/decomp/src/relocData/359_FTMarioAnimSelected.c"
#include "../../decomp/BattleShip-main/decomp/src/relocData/372_FTFoxAnimSelected.c"
#if NDS_P2_LUIGI
#include "../../decomp/BattleShip-main/decomp/src/relocData/462_FTLuigiAnimSelected.c"
#endif
#if NDS_P2_DONKEY
/* PlayersVS maps Donkey to Demo Win1 exactly like Luigi.  File 381 is the
 * source Selected clip; despite its historical AObjEvent32 pointer typedef the
 * generated payload is the same u16 FIGATREE command stream normalized by the
 * compile-time command encoders above. */
#include "../../decomp/BattleShip-main/decomp/src/relocData/381_FTDonkeyAnimSelected.c"
#endif
#if NDS_P2_CAPTAIN
/* File 429 is Falcon's source Selected clip, the same u16 FIGATREE command
 * stream the encoders above normalize for Mario/Fox/Luigi/Donkey. */
#include "../../decomp/BattleShip-main/decomp/src/relocData/429_FTCaptainAnimSelected.c"
#endif
#if NDS_P2_SAMUS
/* PlayersVS maps Samus to Demo Win4, and both Samus Win3/Win4 submotion slots
 * name file 395 (Selected).  Like Donkey/Link the decomp preserves the pointer
 * table as AObjEvent32* for historical type reasons while every referenced
 * joint payload is the same u16 AObjEvent16 stream normalized above. */
#include "../../decomp/BattleShip-main/decomp/src/relocData/395_FTSamusAnimSelected.c"
#endif
#if NDS_P2_LINK
/* PlayersVS maps Link to Demo Win1 -> submotion 1 -> file 404 Selected.  Keep
 * the source clip on the same resident path as every other landed CSS fighter;
 * llFTLinkAnimSelectedFileID is intentionally only a token/address in this
 * port, so the direct table is what makes this demo-only clip authoritative. */
#include "../../decomp/BattleShip-main/decomp/src/relocData/404_FTLinkAnimSelected.c"
#endif
#if NDS_P2_PIKACHU
/* scsubsysdatapikachu.c:67 maps Pikachu's Selected demo to file 476 with the
 * same demo-only clip contract as the fighters above. */
#include "../../decomp/BattleShip-main/decomp/src/relocData/476_FTPikachuAnimSelected.c"
#endif
#if NDS_P2_YOSHI
/* scsubsysdatayoshi.c:101 maps Yoshi's Selected demo to file 444 with the
 * same demo-only clip contract as the fighters above. */
#include "../../decomp/BattleShip-main/decomp/src/relocData/444_FTYoshiAnimSelected.c"
#endif
#if NDS_P2_NESS
/* scsubsysdataness.c maps Ness's Selected demo to file 437 with the same
 * demo-only clip contract as the fighters above. */
#include "../../decomp/BattleShip-main/decomp/src/relocData/437_FTNessAnimSelected.c"
#endif
#if NDS_P2_PURIN
/* scsubsysdatapurin.c maps Purin's Selected demo to file 470 with the same
 * demo-only clip contract as the fighters above. */
#include "../../decomp/BattleShip-main/decomp/src/relocData/470_FTPurinAnimSelected.c"
#endif
#if NDS_P2_KIRBY
/* scsubsysdatakirby.c maps Kirby's Selected demo to file 418 with the same
 * demo-only clip contract as the fighters above. */
#include "../../decomp/BattleShip-main/decomp/src/relocData/418_FTKirbyAnimSelected.c"
#endif
#undef ftAnimSetValAfter
#undef ftAnimSetValAfterBlock
#undef ftAnimSetVal0Rate
#undef ftAnimSetVal0RateT
#undef ftAnimSetVal0RateBlock
#undef ftAnimSetVal0RateBlockT
#undef ftAnimSetTargetRateBlock
#undef ftAnimSetValRate
#undef ftAnimSetValRateT
#undef ftAnimSetValRateBlock
#undef ftAnimSetValRateBlockT
#undef ftAnimSetVal
#undef ftAnimSetValT
#undef ftAnimSetValBlock
#undef ftAnimSetValBlockT
#undef ftAnimBlock
#undef ftAnimEnd
#undef _FT_ANIM_CMD
#undef FT_ANIM_TRAZ
#undef FT_ANIM_TRAY
#undef FT_ANIM_TRAX
#undef FT_ANIM_SCAZ
#undef FT_ANIM_SCAY
#undef FT_ANIM_SCAX
#undef FT_ANIM_ROTZ
#undef FT_ANIM_ROTY
#undef FT_ANIM_ROTX
#undef _RELOCDATA_TYPES_H_

#include <string.h>

size_t ndsBattleShipCSSSelectedFigatreeSize(const void *file_id)
{
    if (file_id == &llFTMarioAnimSelectedFileID)
    {
        return sizeof(dFTMarioAnimSelected_joints);
    }
    if (file_id == &llFTFoxAnimSelectedFileID)
    {
        return sizeof(dFTFoxAnimSelected_joints);
    }
#if NDS_P2_LUIGI
    if (file_id == &llFTLuigiAnimSelectedFileID)
    {
        return sizeof(dFTLuigiAnimSelected_joints);
    }
#endif
#if NDS_P2_DONKEY
    if (file_id == &llFTDonkeyAnimSelectedFileID)
    {
        return sizeof(dFTDonkeyAnimSelected_joints);
    }
#endif
#if NDS_P2_CAPTAIN
    if (file_id == &llFTCaptainAnimSelectedFileID)
    {
        return sizeof(dFTCaptainAnimSelected_joints);
    }
#endif
#if NDS_P2_SAMUS
    if (file_id == &llFTSamusAnimSelectedFileID)
    {
        return sizeof(dFTSamusAnimSelected_joints);
    }
#endif
#if NDS_P2_LINK
    if (file_id == &llFTLinkAnimSelectedFileID)
    {
        return sizeof(dFTLinkAnimSelected_joints);
    }
#endif
#if NDS_P2_PIKACHU
    if (file_id == &llFTPikachuAnimSelectedFileID)
    {
        return sizeof(dFTPikachuAnimSelected_joints);
    }
#endif
#if NDS_P2_YOSHI
    if (file_id == &llFTYoshiAnimSelectedFileID)
    {
        return sizeof(dFTYoshiAnimSelected_joints);
    }
#endif
#if NDS_P2_NESS
    if (file_id == &llFTNessAnimSelectedFileID)
    {
        return sizeof(dFTNessAnimSelected_joints);
    }
#endif
#if NDS_P2_PURIN
    if (file_id == &llFTPurinAnimSelectedFileID)
    {
        return sizeof(dFTPurinAnimSelected_joints);
    }
#endif
#if NDS_P2_KIRBY
    if (file_id == &llFTKirbyAnimSelectedFileID)
    {
        return sizeof(dFTKirbyAnimSelected_joints);
    }
#endif
    return 0u;
}

void *ndsBattleShipLoadCSSSelectedFigatree(const void *file_id, void *heap)
{
    const void *source;
    size_t size;

    if (file_id == &llFTMarioAnimSelectedFileID)
    {
        source = dFTMarioAnimSelected_joints;
        size = sizeof(dFTMarioAnimSelected_joints);
    }
    else if (file_id == &llFTFoxAnimSelectedFileID)
    {
        source = dFTFoxAnimSelected_joints;
        size = sizeof(dFTFoxAnimSelected_joints);
    }
#if NDS_P2_LUIGI
    else if (file_id == &llFTLuigiAnimSelectedFileID)
    {
        source = dFTLuigiAnimSelected_joints;
        size = sizeof(dFTLuigiAnimSelected_joints);
    }
#endif
#if NDS_P2_DONKEY
    else if (file_id == &llFTDonkeyAnimSelectedFileID)
    {
        source = dFTDonkeyAnimSelected_joints;
        size = sizeof(dFTDonkeyAnimSelected_joints);
    }
#endif
#if NDS_P2_CAPTAIN
    else if (file_id == &llFTCaptainAnimSelectedFileID)
    {
        source = dFTCaptainAnimSelected_joints;
        size = sizeof(dFTCaptainAnimSelected_joints);
    }
#endif
#if NDS_P2_SAMUS
    else if (file_id == &llFTSamusAnimSelectedFileID)
    {
        source = dFTSamusAnimSelected_joints;
        size = sizeof(dFTSamusAnimSelected_joints);
    }
#endif
#if NDS_P2_LINK
    else if (file_id == &llFTLinkAnimSelectedFileID)
    {
        source = dFTLinkAnimSelected_joints;
        size = sizeof(dFTLinkAnimSelected_joints);
    }
#endif
#if NDS_P2_PIKACHU
    else if (file_id == &llFTPikachuAnimSelectedFileID)
    {
        source = dFTPikachuAnimSelected_joints;
        size = sizeof(dFTPikachuAnimSelected_joints);
    }
#endif
#if NDS_P2_YOSHI
    else if (file_id == &llFTYoshiAnimSelectedFileID)
    {
        source = dFTYoshiAnimSelected_joints;
        size = sizeof(dFTYoshiAnimSelected_joints);
    }
#endif
#if NDS_P2_NESS
    else if (file_id == &llFTNessAnimSelectedFileID)
    {
        source = dFTNessAnimSelected_joints;
        size = sizeof(dFTNessAnimSelected_joints);
    }
#endif
#if NDS_P2_PURIN
    else if (file_id == &llFTPurinAnimSelectedFileID)
    {
        source = dFTPurinAnimSelected_joints;
        size = sizeof(dFTPurinAnimSelected_joints);
    }
#endif
#if NDS_P2_KIRBY
    else if (file_id == &llFTKirbyAnimSelectedFileID)
    {
        source = dFTKirbyAnimSelected_joints;
        size = sizeof(dFTKirbyAnimSelected_joints);
    }
#endif
    else
    {
        return NULL;
    }
    if (heap == NULL)
    {
        return (void *)source;
    }
    memcpy(heap, source, size);
    return heap;
}

sb32 ndsBattleShipIsCSSSelectedFigatreeJoint(const void *ptr)
{
    static uintptr_t selected_low;
    static uintptr_t selected_high;
    uintptr_t value;
    size_t i;

    if (ptr == NULL)
    {
        return FALSE;
    }
    if (selected_high == 0u)
    {
        uintptr_t low = (uintptr_t)-1;
        uintptr_t high = 0u;

#define NDS_CSS_SELECTED_ACCUM_BOUNDS(table)                             \
        do                                                               \
        {                                                                \
            for (i = 0u; i < ARRAY_COUNT(table); i++)                   \
            {                                                            \
                uintptr_t candidate = (uintptr_t)(table)[i];             \
                                                                         \
                if ((candidate != 0u) && (candidate < low))              \
                {                                                        \
                    low = candidate;                                     \
                }                                                        \
                if (candidate > high)                                    \
                {                                                        \
                    high = candidate;                                    \
                }                                                        \
            }                                                            \
        } while (0)

        NDS_CSS_SELECTED_ACCUM_BOUNDS(dFTMarioAnimSelected_joints);
        NDS_CSS_SELECTED_ACCUM_BOUNDS(dFTFoxAnimSelected_joints);
#if NDS_P2_LUIGI
        NDS_CSS_SELECTED_ACCUM_BOUNDS(dFTLuigiAnimSelected_joints);
#endif
#if NDS_P2_DONKEY
        NDS_CSS_SELECTED_ACCUM_BOUNDS(dFTDonkeyAnimSelected_joints);
#endif
#if NDS_P2_CAPTAIN
        NDS_CSS_SELECTED_ACCUM_BOUNDS(dFTCaptainAnimSelected_joints);
#endif
#if NDS_P2_SAMUS
        NDS_CSS_SELECTED_ACCUM_BOUNDS(dFTSamusAnimSelected_joints);
#endif
#if NDS_P2_LINK
        NDS_CSS_SELECTED_ACCUM_BOUNDS(dFTLinkAnimSelected_joints);
#endif
#if NDS_P2_PIKACHU
        NDS_CSS_SELECTED_ACCUM_BOUNDS(dFTPikachuAnimSelected_joints);
#endif
#undef NDS_CSS_SELECTED_ACCUM_BOUNDS
        if (high == 0u)
        {
            return FALSE;
        }
        selected_low = low;
        selected_high = high;
    }
    value = (uintptr_t)ptr;
    if ((value < selected_low) || (value > selected_high))
    {
        return FALSE;
    }
    for (i = 0u; i < ARRAY_COUNT(dFTMarioAnimSelected_joints); i++)
    {
        if ((const void *)dFTMarioAnimSelected_joints[i] == ptr)
        {
            return TRUE;
        }
    }
    for (i = 0u; i < ARRAY_COUNT(dFTFoxAnimSelected_joints); i++)
    {
        if ((const void *)dFTFoxAnimSelected_joints[i] == ptr)
        {
            return TRUE;
        }
    }
#if NDS_P2_LUIGI
    for (i = 0u; i < ARRAY_COUNT(dFTLuigiAnimSelected_joints); i++)
    {
        if ((const void *)dFTLuigiAnimSelected_joints[i] == ptr)
        {
            return TRUE;
        }
    }
#endif
#if NDS_P2_DONKEY
    for (i = 0u; i < ARRAY_COUNT(dFTDonkeyAnimSelected_joints); i++)
    {
        if ((const void *)dFTDonkeyAnimSelected_joints[i] == ptr)
        {
            return TRUE;
        }
    }
#endif
#if NDS_P2_CAPTAIN
    for (i = 0u; i < ARRAY_COUNT(dFTCaptainAnimSelected_joints); i++)
    {
        if ((const void *)dFTCaptainAnimSelected_joints[i] == ptr)
        {
            return TRUE;
        }
    }
#endif
#if NDS_P2_SAMUS
    for (i = 0u; i < ARRAY_COUNT(dFTSamusAnimSelected_joints); i++)
    {
        if ((const void *)dFTSamusAnimSelected_joints[i] == ptr)
        {
            return TRUE;
        }
    }
#endif
#if NDS_P2_LINK
    for (i = 0u; i < ARRAY_COUNT(dFTLinkAnimSelected_joints); i++)
    {
        if ((const void *)dFTLinkAnimSelected_joints[i] == ptr)
        {
            return TRUE;
        }
    }
#endif
#if NDS_P2_PIKACHU
    for (i = 0u; i < ARRAY_COUNT(dFTPikachuAnimSelected_joints); i++)
    {
        if ((const void *)dFTPikachuAnimSelected_joints[i] == ptr)
        {
            return TRUE;
        }
    }
#endif
    return FALSE;
}
