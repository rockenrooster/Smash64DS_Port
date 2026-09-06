/* GBI packet source-exactness fixture (F3DEX2, GBI_UCODE=F3DEX_GBI_2).
 *
 * Dual-mode host comparison, following scripts/mp_floor_crossing_fixture.c:
 * the SAME file compiles twice on the host --
 *   PORT mode (default, -I include):        <PR/gbi.h> is the port header.
 *   SDK mode (-DSSB64DS_GBI_REFERENCE_SDK
 *     -D_LANGUAGE_C -DF3DEX_GBI_2
 *     -I decomp/BattleShip-main/decomp/include):
 *     <PR/gbi.h> is the ACTUAL source SDK header, so every reference word is
 *     the preprocessor's own expansion of the source, not a transcription.
 * Both binaries print the identical case list; the runner fails unless the
 * outputs are byte-equal. A negative control (MODULATEI vs MODULATEI_PRIM)
 * fails the binary itself if the two ever collide, so the test cannot pass
 * vacuously.
 *
 * PORT mode overrides the main-RAM packet guard: on the host no buffer lives
 * at 0x02000000, so without the override every macro would (correctly) store
 * nothing. The override changes only this TU's test path, never the header.
 *
 * Every case is a source call site:
 *   0  gDPSetCycleType G_CYC_2CYCLE            itdisplay.c:242, ftdisplaymain.c:1176
 *   1  gDPSetCycleType G_CYC_1CYCLE            itdisplay.c:926 (restore)
 *   2  gDPSetCycleType G_CYC_FILL              scstaffroll.c:790 (highlight)
 *   3  gDPSetRenderMode G_RM_PASS              itdisplay.c:243 (already-real anchor)
 *   4  gDPSetCombineMode MODULATEI_PRIM x2     sc1pstageclear.c:1547
 *   5  gDPSetCombineLERP staffroll tokens      scstaffroll.c:1507 (literal-0 CCMUX probe)
 *   6  gDPSetFillColor staffroll brown         scstaffroll.c:481
 *   7  gDPFillRectangle wallpaper              grwallpaper.c
 *   8  gSPTexture staffroll enable             scstaffroll.c:1503
 *   9  gSPVertex 4 verts at 0                 scstaffroll.c:2098
 *   10 gSP2Triangles credits quad             scstaffroll.c:2099
 *   static gsDPSetCycleType / gsDPSetFillColor /
 *      gsDPFillRectangle static forms          grwallpaper.c, scstaffroll.c:479-484
 *   value G_RM_PASS / G_CYC_2CYCLE / G_CYC_FILL /
 *      GCONVERT5551_RGBA8888 values
 *   N  negative: MODULATEI must differ from MODULATEI_PRIM
 */

#if defined(SSB64DS_GBI_REFERENCE_SDK)
/* mbi.h provides _SHIFTL plus gbi.h itself, exactly like the N64 build. */
#include <PR/mbi.h>
#else
#include <PR/gbi.h>
#endif

/* SDK mode includes <PR/gbi.h> directly (not mbi.h, its canonical G_ON
 * home); spell the SDK mbi.h value locally so both modes see G_ON == 1. */
#ifndef G_ON
#define G_ON 1
#endif

#if !defined(SSB64DS_GBI_REFERENCE_SDK)
#undef NDS_GBI_PACKET_IN_MAIN_RAM
#define NDS_GBI_PACKET_IN_MAIN_RAM(pkt) (1)
#endif

#define GBI_PACKETS_CASE_COUNT 11

/* Deterministic vertex-base address (real DLs carry an arena pointer). */
#define GBI_PACKETS_VTX_ADDR 0x01100000u

static Gfx sGBIPacketsCases[GBI_PACKETS_CASE_COUNT];
static Gfx sGBIPacketsNegative[2];

static void gbiPacketsEmit(void)
{
    /* Post-increment cursor, like the gSYTaskmanDLHeads[0]++ call sites:
     * each macro must evaluate pkt exactly once, or the words land in the
     * wrong slots and the SDK comparison below fails. */
    Gfx *cursor = &sGBIPacketsCases[0];
    Gfx *neg = &sGBIPacketsNegative[0];

    gDPSetCycleType(cursor++, G_CYC_2CYCLE);
    gDPSetCycleType(cursor++, G_CYC_1CYCLE);
    gDPSetCycleType(cursor++, G_CYC_FILL);
    gDPSetRenderMode(cursor++, G_RM_PASS, G_RM_AA_ZB_OPA_SURF2);
    gDPSetCombineMode(cursor++, G_CC_MODULATEI_PRIM, G_CC_MODULATEI_PRIM);
    gDPSetCombineLERP(cursor++, 0, 0, 0, PRIMITIVE, 0, 0, 0, TEXEL0,
                       0, 0, 0, PRIMITIVE, 0, 0, 0, TEXEL0);
    gDPSetFillColor(cursor++,
                    GPACK_FILL16(GPACK_RGBA5551(0x42, 0x3A, 0x31, 0x01)));
    gDPFillRectangle(cursor++, 10, 10, 310, 230);
    gSPTexture(cursor++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    /* Integer address: both macros reduce it to w1 verbatim, and a real
     * pointer would trip the SDK's own (unsigned int) cast on 64-bit. */
    gSPVertex(cursor++, GBI_PACKETS_VTX_ADDR, 4, 0);
    gSP2Triangles(cursor++, 3, 2, 1, 0, 0, 3, 1, 0);

    /* Negative control pair: same macro, neighbouring source variants. */
    gDPSetCombineMode(neg++, G_CC_MODULATEI, G_CC_MODULATEI);
    gDPSetCombineMode(neg++, G_CC_MODULATEI_PRIM, G_CC_MODULATEI_PRIM);
}

/* Static-initializer forms, exactly as the wallpaper / text-box lists use
 * them. Both headers' gs macros expand to {w0, w1} pairs. */
static const Gfx sGBIPacketsStatic[] = {
    { gsDPSetCycleType(G_CYC_FILL) },
    { gsDPSetFillColor(GPACK_FILL16(GPACK_RGBA5551(0x00, 0x00, 0x00, 0x01))) },
    { gsDPFillRectangle(346, 35, 348, 164) },
};

int smash64dsGBIPacketsFixture(void)
{
    /* Fold the static words so the device compile also proves the gs forms
     * (otherwise the array is host-only and -Werror drops it there). */
    u32 acc = 0u;
    u32 i;

    gbiPacketsEmit();
    for (i = 0u;
         i < sizeof(sGBIPacketsStatic) / sizeof(sGBIPacketsStatic[0]); i++)
    {
        acc ^= sGBIPacketsStatic[i].words.w0;
        acc ^= sGBIPacketsStatic[i].words.w1;
    }
    return (acc == 0xFFFFFFFFu) ? 1 : 0;
}

#if GBI_PACKETS_HOST_MAIN
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    u32 i;
    int result;

    result = smash64dsGBIPacketsFixture();
    if (result != 0)
    {
        fprintf(stderr, "gbi packets fixture setup failed: %d\n", result);
        return result;
    }
    gbiPacketsEmit();
    for (i = 0u; i < 11u; i++)
    {
        printf("case %u: %08x %08x\n", (unsigned int)i,
               (unsigned int)sGBIPacketsCases[i].words.w0,
               (unsigned int)sGBIPacketsCases[i].words.w1);
    }
    for (i = 0u; i < 3u; i++)
    {
        printf("static %u: %08x %08x\n", (unsigned int)i,
               (unsigned int)sGBIPacketsStatic[i].words.w0,
               (unsigned int)sGBIPacketsStatic[i].words.w1);
    }
    printf("value RM_PASS: %08x\n", (unsigned int)G_RM_PASS);
    printf("value CYC_2CYCLE: %08x\n", (unsigned int)G_CYC_2CYCLE);
    printf("value CYC_FILL: %08x\n", (unsigned int)G_CYC_FILL);
    printf("value CONVERT: %04x\n",
           (unsigned int)GCONVERT5551_RGBA8888(0xFF804020u));
    printf("negative MODULATEI: %08x %08x\n",
           (unsigned int)sGBIPacketsNegative[0].words.w0,
           (unsigned int)sGBIPacketsNegative[0].words.w1);
    printf("negative MODULATEI_PRIM: %08x %08x\n",
           (unsigned int)sGBIPacketsNegative[1].words.w0,
           (unsigned int)sGBIPacketsNegative[1].words.w1);
    if ((sGBIPacketsNegative[0].words.w0 ==
         sGBIPacketsNegative[1].words.w0) &&
        (sGBIPacketsNegative[0].words.w1 ==
         sGBIPacketsNegative[1].words.w1))
    {
        fprintf(stderr, "gbi packets negative control collided\n");
        return 3;
    }
    printf("gbi packets fixtures passed: 11 cases + 3 static + 4 values\n");
    return 0;
}
#endif
