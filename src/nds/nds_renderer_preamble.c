/* The renderer's preamble: the hardware state shadows and texture keys, the
 * fighter packet arena's DMA wait and idle region, the GX recorders' hooks,
 * the ITCM placement macros (placement only: include/nds/nds_task37_itcm.h's
 * rule), and the native owners' diagnostic words. Generated inputs:
 * nds_particle_banks.generated.h and nds_rebirth_halo.generated.inc. The
 * first slice of nds_renderer.c. */

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <nds/battle_playable_static_textures.h>
#include <nds/nds_effects.h>
#include <nds/nds_fox_gun.h>
#include <nds/nds_gbi_decode.h>
#include <nds/nds_ifcommon_oam.h>
#include <nds/nds_platform.h>
#include <nds/nds_r2_hwmath_unit.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_renderer.h>
#include <nds/nds_gx_dma.h>
#include <nds/nds_startup.h>
#include <nds/nds_task37_itcm.h>
#include <nds/renderer_fighter_lean.h>
#if NDS_P2_LINK
const LookAt *ndsRendererAdapterCurrentLookAt(void);
#endif
#include <nds/generated/nds_particle_banks.generated.h>






/* The measured mode-163 renderer wins in ARM state on retail hardware.  Its
 * six zero-wait ITCM paths retain explicit ARM state and O3; host fixtures
 * retain only the portable optimization annotations. */
/* P2-2p8 Phase 1 slice 5: NDS_RENDERER_NATIVE_FIGHTER_CODE marks the
 * production execute and its run helpers (prepare, shade, the two emitters):
 * the old path's record path. A lean draw never runs them (the materializer
 * has its own twins), and route 0 runs them only when a packet misses, so
 * their ITCM goes to the lean joint kernel and its sine half-table
 * (src/nds/nds_ftr_lean_kernel.c). Placement only: same noinline/hot/O3/ARM
 * in main RAM, no emitted instruction changes. Only a roster with a lean kind
 * (P2: Donkey, Samus, Link or Kirby compiled in) trades; P1's Mario/Fox build
 * never engages the lean path and keeps production resident
 * (scripts/check-renderer-itcm-placement.ps1 pins it there). 0 restores the
 * old placement everywhere. */
#if defined(__arm__)
#define NDS_RENDERER_HOT_CODE \
    __attribute__((hot, optimize("O3"), target("arm"), section(".itcm")))
#define NDS_RENDERER_FAST_RUN_CODE \
    __attribute__((noinline, optimize("O3"), target("arm")))
#define NDS_RENDERER_NATIVE_FIGHTER_CODE \
    __attribute__((noinline, hot, optimize("O3"), target("arm")))
#define NDS_RENDERER_NATIVE_FIGHTER_MAIN_CODE \
    __attribute__((noinline, hot, optimize("O3"), target("arm")))
#else
#define NDS_RENDERER_HOT_CODE __attribute__((hot, optimize("O3")))
#define NDS_RENDERER_FAST_RUN_CODE \
    __attribute__((noinline, optimize("O3")))
#define NDS_RENDERER_NATIVE_FIGHTER_CODE \
    __attribute__((noinline, hot, optimize("O3")))
#define NDS_RENDERER_NATIVE_FIGHTER_MAIN_CODE \
    __attribute__((noinline, hot, optimize("O3")))
#endif

/* Task 82 repack. Placement and nothing else, in both directions.
 *
 * include/nds/nds_task37_itcm.h states the rule this obeys: a task that moved
 * code and recompiled it differently at the same time could not attribute its
 * own result. Admission adds a section attribute and no more; eviction drops
 * the section while keeping hot/O3/ARM exactly as NDS_RENDERER_HOT_CODE had
 * them. Neither direction changes an emitted instruction, only where it lives.
 *
 * The pack this replaces was measured on 2026-07-22 and drifted: 27 of the 71
 * .itcm residents no longer execute in the battle window (Task 82 E0). ITCM is
 * a hard 32 KiB and was 32,596 bytes full, so admitting anything required
 * evicting something first.
 *
 * Exactly one eviction, on the owner's decision: the animated-CI4 texel path.
 * It measures zero cycles because PROJECT_GOAL.md freezes Dream Land water at
 * source frame 0, so this is a stage-specific specialisation with a named
 * reason -- not a general claim that the path is dead. A stage with live water
 * needs this re-packed, and the checkers were narrowed from six paths to five
 * rather than deleted so that stays visible. */
#if defined(__arm__)
#define NDS_TASK82_ITCM_CODE __attribute__((section(".itcm")))
#define NDS_TASK82_EVICTED_HOT_CODE     __attribute__((hot, optimize("O3"), target("arm")))
#else
#define NDS_TASK82_ITCM_CODE
#define NDS_TASK82_EVICTED_HOT_CODE NDS_RENDERER_HOT_CODE
#endif

/* Campaign 01 re-knapsack, 2026-08-17. See include/nds/nds_task37_itcm.h for
 * the ranking and the measured rents. The generic display-list interpreter and
 * its submit chain are the fallback renderer that the native stage and fighter
 * owner paths replaced; GX compose retired the affine multiply on top of that.
 * They stay exact and stay callable -- they move to main RAM, they do not go
 * away -- and the bytes go to residents the shipping census actually measures.
 *
 * Placement only, in both directions: NDS_R2_ITCM_PACK2_EVICTED_CODE is
 * NDS_RENDERER_HOT_CODE minus the section. */
#if NDS_R2_ITCM_PACK2 && defined(__arm__)
#define NDS_R2_ITCM_PACK2_EVICTED_CODE \
    __attribute__((hot, optimize("O3"), target("arm")))
#define NDS_R2_ITCM_PACK2_EVICTED_PLAIN_CODE
#else
#define NDS_R2_ITCM_PACK2_EVICTED_CODE NDS_RENDERER_HOT_CODE
#define NDS_R2_ITCM_PACK2_EVICTED_PLAIN_CODE NDS_TASK82_ITCM_CODE
#endif

/* R2-03 E26 SIZING ARM ONLY -- THE SHIPPED ROM'S ITCM PACK IS UNTOUCHED.
 *
 * docs/P1_EXECUTION_BOARD.md names the delta-redundancy census as the thing that
 * sizes the state-span bake and records it as blocked: "does not build: region
 * 'itcm' overflowed by 64 bytes". On this tree it overflows by **616**, because
 * the census block is inline in ndsRendererNativeApplyStateDelta and
 * ndsRendererNativeApplyStateSpan, both of which are ITCM residents, so arming
 * the diagnostic grows ITCM itself. The board's own remedy is "evict a resident
 * for the diagnostic arm before designing the bake".
 *
 * ndsRendererScanList is that resident: 7,728 bytes, the largest in ITCM, and it
 * is the GENERIC display-list interpreter that the native fighter owner path
 * exists to replace -- ndsRendererExecuteNativeFighterOwnerProduction does not
 * call it. Evicting it frees twelve times what the census needs.
 *
 * WHAT THIS ARM MAY AND MAY NOT BE READ FOR. The census's redundancy figures
 * (gNdsR2SpanDeltaRepeats, gNdsR2DeltaEffectCounts, gNdsR2SpanIdenticalOperands)
 * are COUNTS, and a count does not depend on where the code lives, so they are
 * exactly as valid here as in any build. The Task 91 tick brackets in the same
 * arm are NOT: moving 7,728 bytes out of ITCM changes instruction fetch for
 * everything that shares it. Size the bake from the counts; take ticks from an
 * unevicted arm.
 *
 * Placement only, in the spirit of Task 82: NDS_TASK82_EVICTED_HOT_CODE drops
 * the section and keeps hot/O3/ARM, so no emitted instruction changes. */
/* 2026-08-17: the re-knapsack makes this eviction unconditional. The shipping
 * census measures ndsRendererScanList at 599 tk/fr on the gate's own rank-80
 * frames over 6,188 bytes -- 0.1 tk/byte, the second-cheapest rent in a table
 * whose top residents run 100,000 cyc/byte -- so the census arm and the
 * shipping arm now want the same placement for the same measured reason. */
#define NDS_R2_CENSUS_EVICTED_CODE NDS_R2_ITCM_PACK2_EVICTED_CODE

/* R2-03 E46. ndsRendererNativeApplyStateDelta is already ITCM-resident, but the
 * helpers its switch calls are not: in the census ELF the switch sits at
 * 0x01ff9934 while ndsRendererRecordSetTile is at 0x0200d4e8 and
 * ndsRendererNativeApplyStateSpan -- the loop itself -- at 0x02003a14. So every
 * one of the before-span's 134.5 applications a frame leaves zero-wait ITCM for
 * icache-served main RAM and comes back.
 *
 * E45 left ~186 ticks per application unexplained after ruling out the tile
 * republish (~23 ticks, E44), the invalidation macro (one store) and the span
 * entry (~33 ticks). Instruction fetch is what is left, and the repo emulator
 * models icache, so it is measurable. The whole path is ~1,088 bytes against
 * 2,912 free in .itcm.
 *
 * Placement and nothing else, in the spirit of Task 82 -- no behaviour changes,
 * so a win is attributable to fetch and a null refutes the mechanism. */
#define NDS_R2_DELTA_PATH_CODE NDS_TASK82_ITCM_CODE

/* Profiles 0/1 publish the shipping contract through the compact frame
 * summary.  Profile 1 is the low-frequency O2 coarse build, so it must not
 * maintain the generic per-command proof ledger either.  Profile 2 retains
 * the full forensic counters and semantic observer. */
#define NDS_RENDERER_RECORD_PROOF_ONLY(statement) ((void)0)

#include <math.h>
#include <nds.h>
#include <nds/arm9/postest.h>


#if NDS_TICK_HUD
#define NDS_TASK29_GX_MAX_WORDS 16u
#define NDS_TASK29_GX_CENSUS_CODE \
    __attribute__((noinline, noclone, cold, optimize("Os"), \
                   section(".text.task29_gx_census")))



static inline void ndsRendererTask29GXRecord(
    NDSRendererTask29GXClass command_class,
    const u32 *words,
    u32 word_count)
{
    /* The GX recorders' hook; no recorder is built. */
}

static inline void ndsRendererTask29GlEnable(int bits)
{
    u32 value;

    glEnable(bits);
    value = (u32)GFX_CONTROL;
    ndsRendererTask29GXRecord(NDS_TASK29_GX_CONTROL, &value, 1u);
}

static inline void ndsRendererTask29GlDisable(int bits)
{
    u32 value;

    glDisable(bits);
    value = (u32)GFX_CONTROL;
    ndsRendererTask29GXRecord(NDS_TASK29_GX_CONTROL, &value, 1u);
}

static inline void ndsRendererTask29GlAlphaFunc(int threshold)
{
    u32 value = (u32)threshold;

    glAlphaFunc(threshold);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_ALPHA_TEST, &value, 1u);
}

static inline void ndsRendererTask29GlFogDensity(int index, int density)
{
    u32 words[2] = {(u32)index, (u32)density};

    glFogDensity(index, density);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_FOG_TABLE, words, 2u);
}

static inline void ndsRendererTask29GlFogShift(int shift)
{
    u32 value;

    glFogShift(shift);
    value = (u32)GFX_CONTROL;
    ndsRendererTask29GXRecord(NDS_TASK29_GX_CONTROL, &value, 1u);
}

static inline void ndsRendererTask29GlFogOffset(int offset)
{
    u32 value = (u32)offset;

    glFogOffset(offset);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_FOG_OFFSET, &value, 1u);
}

static inline void ndsRendererTask29GlFogColor(
    u8 red, u8 green, u8 blue, u8 alpha)
{
    u32 value = RGB15(red, green, blue) | ((u32)alpha << 16);

    glFogColor(red, green, blue, alpha);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_FOG_COLOR, &value, 1u);
}

static inline void ndsRendererTask29GlTexParameter(int target, int param)
{
    u32 value;

    glTexParameter(target, param);
    value = glGetTexParameter();
    ndsRendererTask29GXRecord(NDS_TASK29_GX_TEXTURE_PARAM, &value, 1u);
}

static inline void ndsRendererTask29GlBindTexture(int target, int name)
{
    u32 words[2];

    glBindTexture(target, name);
    words[0] = (u32)name;
    words[1] = glGetTexParameter();
    ndsRendererTask29GXRecord(NDS_TASK29_GX_TEXTURE_BIND, words, 2u);
}

static inline void ndsRendererTask29GlMatrixMode(int mode)
{
    u32 value = (u32)mode;

    glMatrixMode(mode);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_MATRIX_MODE, &value, 1u);
}

static inline void ndsRendererTask29GlLoadIdentity(void)
{
    u32 value = 0u;

    glLoadIdentity();
    ndsRendererTask29GXRecord(NDS_TASK29_GX_MATRIX_IDENTITY, &value, 1u);
}

static inline void ndsRendererTask29GlLoadMatrix4x4(const m4x4 *matrix)
{
    glLoadMatrix4x4(matrix);
    ndsRendererTask29GXRecord(
        NDS_TASK29_GX_MATRIX_LOAD4X4, (const u32 *)matrix, 16u);
}

static inline void ndsRendererTask29GlMultMatrix4x4(const m4x4 *matrix)
{
    glMultMatrix4x4(matrix);
    ndsRendererTask29GXRecord(
        NDS_TASK29_GX_MATRIX_MULT4X4, (const u32 *)matrix, 16u);
}

/* Task 51: wrapped emit so the differ (Task 49) observes the 12-word 4x3
 * model-matrix command. Mirrors the MULT4X4 wrapper; records the new class. */
static inline void ndsRendererTask29GlMultMatrix4x3(const m4x3 *matrix)
{
    glMultMatrix4x3(matrix);
    ndsRendererTask29GXRecord(
        NDS_TASK29_GX_MATRIX_MULT4x3, (const u32 *)matrix, 12u);
}

static inline void ndsRendererTask29GlPushMatrix(void)
{
    u32 value = 0u;

    glPushMatrix();
    ndsRendererTask29GXRecord(NDS_TASK29_GX_MATRIX_PUSH, &value, 1u);
}

static inline void ndsRendererTask29GlPopMatrix(int count)
{
    u32 value = (u32)count;

    glPopMatrix(count);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_MATRIX_POP, &value, 1u);
}

static inline void ndsRendererTask29GlStoreMatrix(int index)
{
    u32 value = (u32)index;

    glStoreMatrix(index);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_MATRIX_STORE, &value, 1u);
}

static inline void ndsRendererTask29GlRestoreMatrix(int index)
{
    u32 value = (u32)index;

    glRestoreMatrix(index);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_MATRIX_RESTORE, &value, 1u);
}

static inline void ndsRendererTask29GlPolyFmt(u32 params)
{
    glPolyFmt(params);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_POLY_FORMAT, &params, 1u);
}

static inline void ndsRendererTask29GlBegin(GL_GLBEGIN_ENUM mode)
{
    u32 value = (u32)mode;

    glBegin(mode);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_BEGIN, &value, 1u);
}

static inline void ndsRendererTask29GlEnd(void)
{
    u32 value = 0u;

    glEnd();
    ndsRendererTask29GXRecord(NDS_TASK29_GX_END, &value, 1u);
}

static inline void ndsRendererTask29GlColor(rgb color)
{
    u32 value = (u32)color;

    glColor(color);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_COLOR, &value, 1u);
}

static inline void ndsRendererTask29GlTexCoord2t16(t16 s, t16 t)
{
    u32 value = (u32)(u16)s | ((u32)(u16)t << 16);

    glTexCoord2t16(s, t);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_TEX_COORD, &value, 1u);
}

static inline void ndsRendererTask29GlVertex3v16(v16 x, v16 y, v16 z)
{
    u32 words[2] = {
        (u32)(u16)x | ((u32)(u16)y << 16),
        (u32)(u16)z
    };

    glVertex3v16(x, y, z);
    ndsRendererTask29GXRecord(NDS_TASK29_GX_VERTEX16, words, 2u);
}

#define glEnable ndsRendererTask29GlEnable
#define glDisable ndsRendererTask29GlDisable
#define glAlphaFunc ndsRendererTask29GlAlphaFunc
#define glFogDensity ndsRendererTask29GlFogDensity
#define glFogShift ndsRendererTask29GlFogShift
#define glFogOffset ndsRendererTask29GlFogOffset
#define glFogColor ndsRendererTask29GlFogColor
#define glTexParameter ndsRendererTask29GlTexParameter
#define glBindTexture ndsRendererTask29GlBindTexture
#define glMatrixMode ndsRendererTask29GlMatrixMode
#define glLoadIdentity ndsRendererTask29GlLoadIdentity
#define glLoadMatrix4x4 ndsRendererTask29GlLoadMatrix4x4
#define glMultMatrix4x4 ndsRendererTask29GlMultMatrix4x4
#define glMultMatrix4x3 ndsRendererTask29GlMultMatrix4x3
#define glPushMatrix ndsRendererTask29GlPushMatrix
#define glPopMatrix ndsRendererTask29GlPopMatrix
#define glStoreMatrix ndsRendererTask29GlStoreMatrix
#define glRestoreMatrix ndsRendererTask29GlRestoreMatrix
#define glPolyFmt ndsRendererTask29GlPolyFmt
#define glBegin ndsRendererTask29GlBegin
#define glEnd ndsRendererTask29GlEnd
#define glColor ndsRendererTask29GlColor
#define glTexCoord2t16 ndsRendererTask29GlTexCoord2t16
#define glVertex3v16 ndsRendererTask29GlVertex3v16
#endif

/* The bin macro is defined by the G3 capture block above, which is itself
 * nested inside the GX-record configuration guard. This classifier is not, so
 * the fallback has to be unconditional rather than an #else on that block --
 * otherwise a configuration that compiles the classifier without the capture
 * fails at the first bin call rather than simply not counting. These three
 * must sit OUTSIDE every NDS_RENDERER_HW_TRIANGLES-guarded block AND before
 * the first use: the primaries live under NDS_TICK_HUD at line 886, the first
 * use is inside the hardware block at 17434, and d34917b9 put the fallback
 * inside that same block, which left a default build with neither flag -- the
 * plain `make` target -- with no definition at all. The #ifndefs are belt and
 * braces now, since nothing else defines them on this path. */
#ifndef NDS_EFFECT_SUBMIT_BIN
#define NDS_EFFECT_SUBMIT_BIN(bin) ((void)0)
#endif
#ifndef NDS_EFFECT_PHASE_VTX
#define NDS_EFFECT_PHASE_VTX(call) do { call; } while (0)
#endif
#ifndef NDS_EFFECT_PHASE_TRI
#define NDS_EFFECT_PHASE_TRI(call) do { call; } while (0)
#endif

/* These three carry the GX record hook. A fighter emit path must NOT use them --
 * see ndsRendererHardwareWriteFighter*Word below, and the measurement that made
 * the split: the hook's two capture flags can never be set on the fighter path
 * and testing them once per corner cost 24% of the untextured emit. */
static inline void ndsRendererHardwareWriteColorWord(u32 value)
{
    GFX_COLOR = value;
}

static inline void ndsRendererHardwareWriteNormalWord(u32 value)
{
    GFX_NORMAL = value;
}

static inline void ndsRendererHardwareWriteDiffuseAmbient(u32 value)
{
    GFX_DIFFUSE_AMBIENT = value;
}

static inline void ndsRendererHardwareWriteTexCoordWord(u32 value)
{
    GFX_TEX_COORD = value;
}

static inline void ndsRendererHardwareWriteVertex16Words(u32 xy, u32 z)
{
    GFX_VERTEX16 = xy;
    GFX_VERTEX16 = z;
}

static inline void ndsRendererHardwareWriteFighterColorWord(u32 value)
{
    GFX_COLOR = value;
}

static inline void ndsRendererHardwareWriteFighterTexCoordWord(u32 value)
{
    GFX_TEX_COORD = value;
}

static inline void ndsRendererHardwareWriteFighterVertex16Words(u32 xy, u32 z)
{
    GFX_VERTEX16 = xy;
    GFX_VERTEX16 = z;
}


#define NDS_RENDERER_OP_NOOP 0x00u
#define NDS_RENDERER_OP_VTX 0x01u
#define NDS_RENDERER_OP_MODIFYVTX 0x02u
#define NDS_RENDERER_OP_CULLDL 0x03u
#define NDS_RENDERER_OP_TRI1 0x05u
#define NDS_RENDERER_OP_TRI2 0x06u
#define NDS_RENDERER_OP_TEXTURE 0xd7u
#define NDS_RENDERER_OP_POPMTX 0xd8u
#define NDS_RENDERER_OP_MTX 0xdau
#define NDS_RENDERER_OP_GEOMETRYMODE 0xd9u
#define NDS_RENDERER_OP_MOVEWORD 0xdbu
#define NDS_RENDERER_OP_MOVEMEM 0xdcu
#define NDS_RENDERER_OP_SPECIAL_1 0xd5u
#define NDS_RENDERER_OP_DL 0xdeu
#define NDS_RENDERER_OP_ENDDL 0xdfu
#define NDS_RENDERER_OP_SETOTHERMODE_H 0xe3u
#define NDS_RENDERER_OP_SETOTHERMODE_L 0xe2u
#define NDS_RENDERER_OP_SETSCISSOR 0xedu
#define NDS_RENDERER_OP_SETPRIMDEPTH 0xeeu
#define NDS_RENDERER_OP_SETCOMBINE 0xfcu
#define NDS_RENDERER_OP_SETCIMG 0xffu
#define NDS_RENDERER_OP_SETFOGCOLOR 0xf8u
#define NDS_RENDERER_OP_SETBLENDCOLOR 0xf9u
#define NDS_RENDERER_OP_SETENVCOLOR 0xfbu
#define NDS_RENDERER_OP_SETPRIMCOLOR 0xfau
#define NDS_RENDERER_OP_SETTIMG 0xfdu
#define NDS_RENDERER_OP_SETTILE 0xf5u
#define NDS_RENDERER_OP_LOADTILE 0xf4u
#define NDS_RENDERER_OP_LOADBLOCK 0xf3u
#define NDS_RENDERER_OP_LOADTLUT 0xf0u
#define NDS_RENDERER_OP_SETTILESIZE 0xf2u
#define NDS_RENDERER_OP_RDPSETOTHERMODE 0xefu
#define NDS_RENDERER_OP_RDPPIPESYNC 0xe7u
#define NDS_RENDERER_OP_RDPLOADSYNC 0xe6u
#define NDS_RENDERER_OP_RDPTILESYNC 0xe8u
#define NDS_RENDERER_OP_RDPFULLSYNC 0xe9u

#define NDS_RENDERER_TX_CLAMP 0x2u
#define NDS_RENDERER_TX_MIRROR 0x1u
#define NDS_RENDERER_RENDER_TILE 0u
#define NDS_RENDERER_RENDER_TILE_1 1u
#define NDS_RENDERER_LOAD_TILE 7u

#define NDS_RENDERER_MAX_VTX NDS_RENDERER_VERTEX_CACHE_SIZE
/* G_MTX push and G_POPMTX are interpreted only by the host reference
 * (src/host/graphics_reference/nds_renderer_reference.c); the DS build has no
 * generic list interpreter, so nothing on DS pushes a modelview. One slot
 * keeps the DS traversal state 2,176 B smaller: every native owner declares
 * one on its stack, and the stage DL fast lane runs on the 6 KB DTCM hot
 * stack (P2-2p8, 2026-10-04). */
#if defined(ARM9)
#define NDS_RENDERER_MODELVIEW_STACK_SIZE 1u
#else
#define NDS_RENDERER_MODELVIEW_STACK_SIZE 32u
#endif
#define NDS_RENDERER_N64_MTX_FRAC_BITS 16u
#define NDS_RENDERER_DS_MTX_FRAC_BITS 12u
#define NDS_RENDERER_MTX_PUSH_XOR 0x01u
#define NDS_RENDERER_MTX_PUSH 0x01u
#define NDS_RENDERER_MTX_LOAD 0x02u
#define NDS_RENDERER_MTX_PROJECTION 0x04u
#define NDS_RENDERER_MOVEWORD_MATRIX 0x00u
#define NDS_RENDERER_MOVEWORD_FOG 0x08u
#define NDS_RENDERER_MOVEWORD_LIGHTCOL 0x0au
#define NDS_RENDERER_MOVEWORD_FOG_OFFSET 0x00u
#define NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_1_A 0x00u
#define NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_1_B 0x04u
#define NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_2_A 0x18u
#define NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_2_B 0x1cu
#define NDS_RENDERER_MOVEWORD_INDEX_SHIFT 16u
#define NDS_RENDERER_MOVEWORD_OFFSET_MASK 0xffffu
#define NDS_RENDERER_MOVEWORD_INDEX_MASK 0xffu
#define NDS_RENDERER_SPECIAL_1_OFFSET_SHIFT 8u
#define NDS_RENDERER_SPECIAL_1_OFFSET_MASK 0xffffu
#define NDS_RENDERER_SPECIAL_1_INDEX_MASK 0xffu
#define NDS_RENDERER_MWO_POINT_ST 0x14u
#define NDS_RENDERER_MOVEMEM_LIGHT 10u
#define NDS_RENDERER_MOVEMEM_OFFSET_SHIFT 8u
#define NDS_RENDERER_MOVEMEM_OFFSET_MASK 0xffu
#define NDS_RENDERER_MOVEMEM_LENGTH_SHIFT 19u
#define NDS_RENDERER_MOVEMEM_LENGTH_MASK 0x1fu
#define NDS_RENDERER_MOVEMEM_LIGHT_BASE_OFFSET 24u
#define NDS_RENDERER_MOVEMEM_LIGHT_STRIDE 24u
#define NDS_RENDERER_MATRIX_WORD_BYTES 4u
#define NDS_RENDERER_MATRIX_WORD_COUNT 16u
#define NDS_RENDERER_HW_TEXTURE_MAX_WIDTH 128u
#define NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT 128u
#define NDS_RENDERER_HW_TEXTURE_MAX_TEXELS \
    (NDS_RENDERER_HW_TEXTURE_MAX_WIDTH * NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT)
#define NDS_RENDERER_HW_TEXEL01_CI4_LUT_COUNT (16u * 16u)
/* Sized from the measured working set, not chosen: Task 90 E0 traced 128
 * consecutive light-shade requests on the Boundary battle and found exactly 6
 * distinct (diffuse, ambient) pairs against 49 requests per frame. At 4 entries
 * the round-robin cache evicted live pairs and rebuilt a 128-entry table 6
 * times every frame; at 8 the trace replay drops to 6 misses total, which is
 * the compulsory floor -- one build per distinct pair for the whole match.
 * Must stay a power of two: the eviction cursor masks with COUNT - 1.
 * Re-run scripts/census-light-shade-lut.ps1 if the fighter or stage set
 * changes; a working set above 8 would put this back into steady-state thrash. */
#define NDS_RENDERER_HW_LIGHT_SHADE_CACHE_COUNT 8u
#define NDS_RENDERER_HW_LIGHT_SHADE_LUT_COUNT 128u
#define NDS_RENDERER_HW_CI4_INDEX_CACHE_COUNT 2u
#define NDS_RENDERER_HW_CI4_INDEX_CACHE_TEXELS 1024u
#define NDS_RENDERER_HW_CI4_CLASS_TABLE_COUNT 256u
#define NDS_RENDERER_HW_CI4_CLASS_KEY_MASK 0x7ffffu
#define NDS_RENDERER_HW_CI4_CLASS_INDEX_SHIFT 19u
#define NDS_RENDERER_HW_TEXEL01_RGB_MASK 0x7fffu
#define NDS_RENDERER_HW_TEXEL01_COVERAGE_SHIFT 15u
#define NDS_RENDERER_HW_TEXTURE_FMT_RGBA16 0u
#define NDS_RENDERER_HW_TEXTURE_FMT_CI 2u
#define NDS_RENDERER_HW_TEXTURE_FMT_IA 3u
#define NDS_RENDERER_HW_TEXTURE_FMT_I16 4u
#define NDS_RENDERER_HW_TEXTURE_SIZ_4B 0u
#define NDS_RENDERER_HW_TEXTURE_SIZ_8B 1u
#define NDS_RENDERER_HW_TEXTURE_SIZ_16B 2u
#define NDS_RENDERER_HW_TEXTURE_SIZ_32B 3u
#define NDS_RENDERER_G_TX_DXT_ONE (1u << 11)
#define NDS_RENDERER_HW_TEXREJECT_MISSING_STATE (1u << 0)
#define NDS_RENDERER_HW_TEXREJECT_BAD_CI_SIZE (1u << 1)
#define NDS_RENDERER_HW_TEXREJECT_UNSUPPORTED_FORMAT (1u << 2)
#define NDS_RENDERER_HW_TEXREJECT_BAD_DIMENSIONS (1u << 3)
#define NDS_RENDERER_HW_TEXREJECT_BAD_UPLOAD_SIZE (1u << 4)
#define NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_RANGE (1u << 5)
#define NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_BYTES (1u << 6)
#define NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_PTR (1u << 7)
#define NDS_RENDERER_HW_TEXREJECT_BAD_TLUT (1u << 8)
#define NDS_RENDERER_HW_TEXREJECT_BAD_TLUT_PTR (1u << 9)
#define NDS_RENDERER_HW_TEXREJECT_ALLOC (1u << 10)
#define NDS_RENDERER_HW_TEXREJECT_GENTEX (1u << 11)
#define NDS_RENDERER_HW_TEXREJECT_TEXIMAGE (1u << 12)
#define NDS_RENDERER_HW_TEXEL1_REJECT_ACTIVE_TILE (1u << 0)
#define NDS_RENDERER_HW_TEXEL1_REJECT_TILE_STATE (1u << 1)
#define NDS_RENDERER_HW_TEXEL1_REJECT_LOAD_STATE (1u << 2)
#define NDS_RENDERER_HW_TEXEL1_REJECT_DIMENSIONS (1u << 3)
#define NDS_RENDERER_HW_TEXEL1_REJECT_PAIR_SIZE (1u << 4)
#define NDS_RENDERER_HW_TEXEL1_REJECT_SOURCE_RANGE (1u << 5)
#define NDS_RENDERER_HW_TEXEL1_REJECT_SOURCE_BYTES (1u << 6)
#define NDS_RENDERER_HW_TEXEL1_REJECT_SOURCE_PTR (1u << 7)
#define NDS_RENDERER_HW_USETEX_REJECT_NO_STATS (1u << 0)
#define NDS_RENDERER_HW_USETEX_REJECT_STATE_OFF (1u << 1)
#define NDS_RENDERER_HW_USETEX_REJECT_NO_COMBINE (1u << 2)
#define NDS_RENDERER_HW_USETEX_REJECT_PRIMITIVE_DECAL (1u << 3)
#define NDS_RENDERER_HW_USETEX_REJECT_NO_TEXEL0 (1u << 4)
#define NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE 0xffffu
#define NDS_RENDERER_HW_WORLD_UNIT_SHIFT 8u
#define NDS_RENDERER_HW_RAW_COORD_MIN (-2048)
#define NDS_RENDERER_HW_RAW_COORD_MAX 2047
#define NDS_RENDERER_HW_MATRIX_MODE_NONE 0u
#define NDS_RENDERER_HW_MATRIX_MODE_PROJECTED_IDENTITY 1u
#define NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED 2u
#define NDS_RENDERER_HW_MATRIX_MODE_FIGHTER_HIERARCHY 3u
#define NDS_RENDERER_HW_MATRIX_MODE_STAGE_HW_COMPOSE 4u
#define NDS_RENDERER_HW_POS_TEST_MAX 40u
#define NDS_RENDERER_MATRIX_SNAPSHOT_INVALID 0u
#define NDS_RENDERER_HW_PROJECTED_DEPTH_BACKGROUND_START (0x1000 * 6)
#define NDS_RENDERER_HW_PROJECTED_DEPTH_FOREGROUND_START \
    ((128 - 0x1000) * 6)
#define NDS_RENDERER_HW_PROJECTED_DEPTH_STEP 6
#define NDS_RENDERER_HW_SOURCE_DEPTH_MIN (128 - 0x1000)
#define NDS_RENDERER_HW_SOURCE_DEPTH_MAX (0x1000 - 129)
#define NDS_RENDERER_NATIVE_STAGE_STATIC_OWNER_COUNT 4u
#define NDS_RENDERER_HW_PROJECTED_VERTEX (1 << 12)
#define NDS_RENDERER_HW_DECAL_DEPTH_BIAS (3 << 4)
/* Texture-cache sizing is measured at the request site, not inferred from draw
 * count. The original 48-entry resident-key layout could not survive post-KO
 * demand and also cost 14,016 B of main RAM. Moving static keys back to their
 * generated ROM records shrank each resident entry enough to reach 69 slots
 * within that old footprint.
 *
 * BUGS.md's four-kind stress later exposed a larger simultaneous LOW-detail
 * fighter working set. First, immutable AOT owners (particle atlas, Whispy,
 * Fox glow and Fox gun) were corrected to own dedicated GL names rather than
 * consuming general material-cache entries. With those false pins gone, a
 * roomy diagnostic ROM and then the exact packet-recording path measured a
 * dynamic demand high-water of 62 entries with zero packet texture rejects.
 * The exact 87-slot arm still had 165,024 B general-heap low-water and 185,920 B
 * scene-arena headroom, so the old 14 KiB storage ceiling is no longer the
 * active memory constraint.
 *
 * Final sizing was 24 static + 62 measured dynamic + 4 headroom = 90, but the
 * four-kind BUGS.md stress subsequently proved that estimate stale: the exact
 * shipping-shaped frame reached all 66 dynamic slots in one renderer frame and
 * refused the next texture with ALLOC. A roomy 110-slot route-probe arm then
 * measured the whole 1024-frame run: resident high-water 86, but the allocator-
 * relevant same-frame touched high-water is 78. Stale residents are evictable;
 * current-frame residents are not. Bank 78 measured + 4 headroom = 82 dynamic.
 * The fixed partition remains deliberate: static record i owns slot i, while
 * dynamic slot i owns key-pool entry i - STATIC_COUNT. No free-list or resident
 * key pointer is needed. Re-measure with scripts/probe-p2-fourcpu-sparse.ps1
 * before changing this count; do not grow it from a theoretical roster sum. */
/* P2-2p8 Phase 1 slice 2b did NOT grow the count for fighter texture
 * admission: at 44 B of entry plus a 236-byte pool key per dynamic slot, the
 * ~192 slots the stress roster's whole admitted set needs cost +19 KB of static
 * RAM, which the taskman arena pays for.
 *
 * Slice 2c makes the slot cheap instead of the RAM bigger. A dynamic slot no
 * longer stores its key: it stores a compact identity (two 32-bit multiply
 * chains over every key word but the image, plus the image, TLUT and texel1
 * pointers exactly -- NDSRendererHardwareTextureIdent), and the entry drops
 * the five profile-2-only fields. 60 B per slot instead of 280, so the cache
 * reaches the lookup table's u8 ceiling (254 slots) at ~15 KB, well under the
 * old 24.8 KB: the whole four-CPU admission (<= 153 entries, the partition
 * minus NDS_FTR_ADMIT_SLOT_RESERVE) plus the draws' measured same-frame set.
 * The identity was collision-checked, not assumed: a lab build kept an exact
 * shadow of every dynamic key and counted any identity hit whose exact key
 * differed, and the admission's lab census proved the admission tables
 * collision-free (artifacts/performance/2026-09-23_p2-2p8-phase1-slice2c). */
#define NDS_RENDERER_HW_TEXTURE_CACHE_COUNT 254u
#define NDS_RENDERER_HW_TEXTURE_STATIC_COUNT 45u
#define NDS_RENDERER_HW_TEXTURE_DYNAMIC_COUNT \
    (NDS_RENDERER_HW_TEXTURE_CACHE_COUNT - NDS_RENDERER_HW_TEXTURE_STATIC_COUNT)
#define NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT 512u
#define NDS_RENDERER_HW_TEXTURE_LOOKUP_EMPTY 0u
#define NDS_RENDERER_CCMUX_COMBINED 0u
#define NDS_RENDERER_CCMUX_TEXEL0 1u
#define NDS_RENDERER_CCMUX_TEXEL1 2u
#define NDS_RENDERER_CCMUX_PRIMITIVE 3u
#define NDS_RENDERER_CCMUX_SHADE 4u
#define NDS_RENDERER_CCMUX_ENVIRONMENT 5u
#define NDS_RENDERER_CCMUX_PRIM_LOD_FRAC 14u
#define NDS_RENDERER_CCMUX_ZERO_AB 15u
/* G_CCMUX_0 is 31, and every colour field masks it to its own width: 4 bits for
 * a and b (15), 5 bits for c (31), 3 bits for d (7). One name per width, so a
 * gate cannot silently compare a c field against the a/b encoding. */
#define NDS_RENDERER_CCMUX_ZERO_C 31u
#define NDS_RENDERER_CCMUX_ZERO_D 7u
#define NDS_RENDERER_ACMUX_COMBINED 0u
#define NDS_RENDERER_ACMUX_TEXEL0 1u
#define NDS_RENDERER_ACMUX_TEXEL1 2u
#define NDS_RENDERER_ACMUX_PRIMITIVE 3u
#define NDS_RENDERER_ACMUX_SHADE 4u
#define NDS_RENDERER_ACMUX_ENVIRONMENT 5u
#define NDS_RENDERER_ACMUX_1 6u
#define NDS_RENDERER_ACMUX_0 7u
#define NDS_RENDERER_PRIM_ENV_BLEND_NONE 0u
#define NDS_RENDERER_PRIM_ENV_BLEND_PRIM_ALPHA 1u
#define NDS_RENDERER_PRIM_ENV_BLEND_SOURCE_ALPHA 2u
/* The rebirth halo's beam (85.vpk0.bin 0x28a0, G_SETCOMBINE FCFFFFFF FFFDF2F9)
 * asks for rgb = PRIMITIVE and alpha = TEXEL0 in both cycles -- a constant
 * colour, not the BLENDPE endpoint lerp above. Its texture is I4, so without
 * this mode ndsRendererHardwareConvertI replicates the intensity into RGB and
 * forces the alpha bit, and the beam draws as an opaque grey ramp on an opaque
 * black surround. Modes 1 and 2 share one bake; this one does not, which is
 * why it needs its own cache-key bit below. */
#define NDS_RENDERER_PRIM_ENV_BLEND_PRIM_RGB_TEXEL0_ALPHA 3u
#define NDS_RENDERER_HW_TEXTURE_KEY_PRIM_ENV_BLEND (1u << 31)
/* Bit 30 is free: key.flags carries render_tile_flags in bits 0-7 and
 * (load_kind << 8), and load_kind's largest value is NDS_RENDERER_TEXTURE_
 * LOADTILE (1u << 6), so nothing above bit 14 is ever set there. */
#define NDS_RENDERER_HW_TEXTURE_KEY_PRIM_RGB_TEXEL0_ALPHA (1u << 30)
/* The source image can contain useful RGB/intensity behind alpha zero while
 * the active N64 combiner takes its final alpha entirely from another source
 * (Castle's steep-roof CI4 is one example).  Such a texture must be uploaded
 * with its source colour preserved and DS texel alpha forced opaque; keep that
 * representation distinct from the ordinary alpha-carrying conversion. */
#define NDS_RENDERER_HW_TEXTURE_KEY_ALPHA_IGNORES_TEXELS (1u << 29)
/* An I tile baked with its intensity as graded coverage (GL_RGB8_A5, a grey
 * ramp palette) for an owner that asked for it: see
 * sNdsRendererHardwareIntensityCoverage. The same image without the request
 * is the opaque I conversion, so the two must not share an entry. */
#define NDS_RENDERER_HW_TEXTURE_KEY_I_TEXEL_ALPHA (1u << 28)
/* An IA8 tile baked with its alpha nibble as graded alpha (GL_RGB8_A5 over a
 * grey ramp of its intensity) for a fighter owner that asked for it: see
 * sNdsRendererHardwareTexelAlphaGraded. The same image without the request
 * is the one-bit IA conversion, so the two must not share an entry. */
#define NDS_RENDERER_HW_TEXTURE_KEY_IA_GRADED_ALPHA (1u << 27)
#define NDS_RENDERER_MDSFT_CYCLETYPE 20u
#define NDS_RENDERER_CYCLETYPE_MASK (3u << NDS_RENDERER_MDSFT_CYCLETYPE)
#define NDS_RENDERER_CYC_2CYCLE (1u << NDS_RENDERER_MDSFT_CYCLETYPE)
#define NDS_RENDERER_MDSFT_TEXTPERSP 19u
#define NDS_RENDERER_TP_PERSP (1u << NDS_RENDERER_MDSFT_TEXTPERSP)
#define NDS_RENDERER_TEXTPERSP_MASK (1u << NDS_RENDERER_MDSFT_TEXTPERSP)
#define NDS_RENDERER_MDSFT_TEXTFILT 12u
#define NDS_RENDERER_TF_POINT (0u << NDS_RENDERER_MDSFT_TEXTFILT)
#define NDS_RENDERER_TF_BILERP (2u << NDS_RENDERER_MDSFT_TEXTFILT)
#define NDS_RENDERER_TEXTFILT_MASK (3u << NDS_RENDERER_MDSFT_TEXTFILT)
#define NDS_RENDERER_TEXCOORD_FILTER_OFFSET (1 << 4)
#define NDS_RENDERER_ALPHA_COMPARE_MASK 0x3u
#define NDS_RENDERER_ALPHA_COMPARE_THRESHOLD 0x1u
#define NDS_RENDERER_ZSOURCE_PRIM 0x00000004u
#define NDS_RENDERER_ZSOURCE_MASK 0x00000004u
#define NDS_RENDERER_ZMODE_MASK 0x00000c00u
#define NDS_RENDERER_ZMODE_XLU 0x00000800u
#define NDS_RENDERER_ZMODE_DEC 0x00000c00u
#define NDS_RENDERER_CVG_X_ALPHA 0x00001000u
#define NDS_RENDERER_FORCE_BL 0x00004000u
#define NDS_RENDERER_G_BL_A_MEM 1u
#define NDS_RENDERER_BLEND_ALPHA_BITS_MASK 0x3u
#define NDS_RENDERER_BLEND_ALPHA_CYCLE1_SHIFT 18u
#define NDS_RENDERER_BLEND_ALPHA_CYCLE2_SHIFT 16u
#define NDS_RENDERER_POLY_ID_MASK 0x3fu
#define NDS_RENDERER_TEXTURE_PARAM_MUTABLE_MASK \
    ((3u << 30) | GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T | \
     GL_TEXTURE_FLIP_S | GL_TEXTURE_FLIP_T)
#define NDS_RENDERER_LIGHT_COLOR_1_MASK (1u << 0)
#define NDS_RENDERER_LIGHT_COLOR_2_MASK (1u << 1)
#define NDS_RENDERER_LIGHT_DIR_1_MASK (1u << 0)
/* BattleShip's fighter display seed emits these light colors before its
 * collision overlay DL: ftdisplaymain.c:205-206. They are a source-shaped
 * fallback for lit lists whose scene callback supplied only light direction. */
#define NDS_RENDERER_LIGHT_COLOR_1_FALLBACK 0x40404000u
#define NDS_RENDERER_LIGHT_COLOR_2_FALLBACK 0xc0c0c000u
#define NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL (1u << 0)
#define NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX (1u << 1)
#define NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE (1u << 2)
#define NDS_RENDERER_VERTEX_CONTEXT_SCALE_WORLD (1u << 3)
#define NDS_RENDERER_VERTEX_CONTEXT_ZBUFFERED (1u << 4)
#define NDS_RENDERER_VERTEX_CONTEXT_DECAL_DEPTH (1u << 5)
#define NDS_RENDERER_VERTEX_CONTEXT_PRIM_DEPTH (1u << 6)
#define NDS_RENDERER_VERTEX_CONTEXT_SOURCE_CLIP_DEPTH (1u << 7)
#define NDS_RENDERER_VERTEX_CONTEXT_PREPARED_MASK \
    (NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL | \
     NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX | \
     NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE)
#define NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state) \
    ((state)->texture_prepare_valid = FALSE)
#define NDS_RENDERER_INVALIDATE_LIGHT_DIRECTION(state) \
    ((state)->prepared_light_direction_valid = FALSE)



/* Frame begin is shared by software and GX builds.  Keep these diagnostics
 * defined in both; only the hardware queue increments them. */
volatile u32 gNdsRendererIFCommonCloudQueuedCount;
volatile u32 gNdsRendererIFCommonCloudEmittedCount;

typedef enum NDSRendererHWSubmitClass
{
    NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX = 0,
    NDS_RENDERER_HW_SUBMIT_RAW_Z_SNAPSHOT_MATRIX,
    NDS_RENDERER_HW_SUBMIT_PROJECTED_CROSS_MATRIX,
    NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z,
    NDS_RENDERER_HW_SUBMIT_PROJECTED_DECAL,
    NDS_RENDERER_HW_SUBMIT_PROJECTED_PRIM_DEPTH,
    NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX,
    NDS_RENDERER_HW_SUBMIT_REJECT,
    NDS_RENDERER_HW_SUBMIT_CLASS_COUNT
} NDSRendererHWSubmitClass;

volatile u32 gNdsRendererFastRunMode =
    NDS_RENDERER_FAST_RUN_DEFAULT;
volatile u32 gNdsRendererFastRunCount;
volatile u32 gNdsRendererFastTriangleCount;
volatile u32 gNdsRendererFastOwnerTriangleCount[
    NDS_RENDERER_PROFILE_OWNER_COUNT];
volatile u32 gNdsRendererFastFallbackCount[3];
static NDSRendererProfileOwner sNdsRendererRuntimeOwner =
    NDS_RENDERER_PROFILE_OWNER_NONE;
static u32 sNdsRendererFastOwnerEnabled;
static u32 sNdsRendererStageTextureSitesEnabled;
static u32 sNdsRendererFastRunCount;
static u32 sNdsRendererFastTriangleCount;
static u32 sNdsRendererFastOwnerTriangleCount[
    NDS_RENDERER_PROFILE_OWNER_COUNT];
static u32 sNdsRendererFastFallbackCount[3];

static u32 sNdsRendererHardwareSubmitted;
/* GO peaks at ten traffic/flare SObjs plus three announce glyphs. Three spare
 * entries keep the exact source painter order fail-closed. */
#define NDS_RENDERER_IFCOMMON_CLOUD_QUEUE_COUNT 16u
typedef struct NDSRendererIFCommonCloudDraw
{
    u32 texture_name;
    s32 x_q16;
    s32 y_q16;
    s32 width_q16;
    s32 height_q16;
    u32 texture_x;
    u32 texture_y;
    u32 texture_width;
    u32 texture_height;
    u32 poly_id;
} NDSRendererIFCommonCloudDraw;
static NDSRendererIFCommonCloudDraw sNdsRendererIFCommonCloudQueue[
    NDS_RENDERER_IFCOMMON_CLOUD_QUEUE_COUNT];
static u32 sNdsRendererIFCommonCloudQueueCount;
static u32 sNdsRendererHardwareNoOracle;
static u32 sNdsRendererHardwareTriangleBatchOpen;
static u32 sNdsRendererHardwareTriangleBatchTextured;
static u32 sNdsRendererHardwareTriangleBatchTextureName;
static u32 sNdsRendererHardwareTriangleBatchPolyFmt;
static u32 sNdsRendererHardwareTriangleBatchAlphaKey;
static u32 sNdsRendererHardwareTriangleBatchFogKey;
static u32 sNdsRendererHardwareTriangleBatchMatrixMode;
static u32 sNdsRendererHardwareTriangleBatchMatrixGeneration;
static u32 sNdsRendererHardwareBoundTextureName;
static int sNdsRendererHardwareNoTextureName;
/* The rebirth-halo beam's dedicated A5I3 slot; the reasoning is at
 * ndsRendererHardwarePreparePrimRgbTexel0AlphaTexture. Identity rather than a
 * cache key: one source image, one upload extent, one primitive colour, all
 * three re-prepared when any of them moves. */
static u32 sNdsRendererHardwarePrimRgbTexel0AlphaName;
static u32 sNdsRendererHardwarePrimRgbTexel0AlphaImage;
static u32 sNdsRendererHardwarePrimRgbTexel0AlphaExtent;
static u32 sNdsRendererHardwarePrimRgbTexel0AlphaPrim;
static u32 sNdsRendererHardwarePrimAlphaTexel0Mode;
static u32 sNdsRendererHardwarePrimAlphaTexel0Env;
volatile u32 gNdsRendererPrimRgbTexel0AlphaPrepareCount;
/* Cache entries uploaded as GL_RGB8_A5 because their combine is a BLENDPE lerp
 * with source alpha over an I tile (the generic path's graded-coverage arm). */
__attribute__((used)) volatile u32 gNdsRendererGradedCoverageUploadCount;
/* Axes uploaded as one mask period because their clamped tile window is wider
 * than the largest upload and so could not be materialised. */
__attribute__((used)) volatile u32 gNdsRendererClampedWindowPeriodUploadCount;
/* Axes uploaded as one mask period because they wrap (G_TX_WRAP) and the
 * extent the load or window gave them is wider than that period. */
__attribute__((used)) volatile u32 gNdsRendererWrapPeriodUploadCount;
volatile u32 gNdsRendererPrimRgbTexel0AlphaBindCount;
#define NDS_RENDERER_IMPACT_WAVE_VARIANT_COUNT 5u
#define NDS_RENDERER_IMPACT_WAVE_TEX_WIDTH 16u
#define NDS_RENDERER_IMPACT_WAVE_TEX_HEIGHT 32u
#define NDS_RENDERER_IMPACT_WAVE_TEX_BYTES 256u
/* Source file 83, DL_0x7C28: CI4 texels @ 0x7980 and RGBA5551 TLUT @
 * 0x7958. The live sampler is 16x32: SETTILE uses CI4 line=1, S mask=4 and
 * T mask=5. That live state is authoritative over the reloc-data annotation
 * that describes the same 256 bytes as 32x16. Effect assets use the port's O2R
 * word-swapped layout, so logical CI4 bytes are physical byte[index ^ 3] and
 * logical TLUT halfwords are physical halfword[index ^ 1]. After undoing that
 * storage layout, N64's first/high nibble is packed into DS PAL16's first/low
 * nibble. Logical palette index 0 is already the sole transparent source entry,
 * so GL_TEXTURE_COLOR0_TRANSPARENT is lossless and no runtime N64 texture/TLUT
 * conversion remains. */
static const u8 sNdsRendererImpactWaveTexels[NDS_RENDERER_IMPACT_WAVE_TEX_BYTES] = {
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0xfcu, 0xfdu, 0xffu, 0xffu, 0xffu, 0xffu, 0xcfu, 0x99u,
    0xebu, 0xfcu, 0xffu, 0xffu, 0xffu, 0xffu, 0xbfu, 0x98u,
    0xdau, 0xfbu, 0xffu, 0xffu, 0xffu, 0xffu, 0xafu, 0x97u,
    0xc9u, 0xeau, 0xffu, 0xffu, 0xffu, 0xffu, 0x9fu, 0x86u,
    0xb9u, 0xd9u, 0xffu, 0xffu, 0xfeu, 0xffu, 0x9eu, 0x75u,
    0xa9u, 0xc9u, 0xffu, 0xffu, 0xedu, 0xffu, 0x9du, 0x64u,
    0x98u, 0xb9u, 0xffu, 0xefu, 0xdcu, 0xffu, 0x8cu, 0x53u,
    0x97u, 0xa9u, 0xfeu, 0xdfu, 0xcbu, 0xeeu, 0x7bu, 0x42u,
    0x96u, 0x97u, 0xfdu, 0xcfu, 0xbau, 0xddu, 0x6au, 0x30u,
    0x85u, 0x97u, 0xfcu, 0xbeu, 0xa9u, 0xccu, 0x59u, 0x20u,
    0x74u, 0x96u, 0xebu, 0xadu, 0x99u, 0xbbu, 0x49u, 0x20u,
    0x63u, 0x05u, 0xdau, 0x9cu, 0x99u, 0xaau, 0x39u, 0x10u,
    0x52u, 0x04u, 0xc9u, 0x9bu, 0x98u, 0x99u, 0x08u, 0x10u,
    0x42u, 0x03u, 0xb9u, 0x9au, 0x87u, 0x99u, 0x07u, 0x10u,
    0x31u, 0x02u, 0xa9u, 0x89u, 0x76u, 0x99u, 0x06u, 0x10u,
    0x21u, 0x02u, 0x99u, 0x79u, 0x65u, 0x98u, 0x05u, 0x10u,
    0x21u, 0x01u, 0x98u, 0x69u, 0x04u, 0x87u, 0x04u, 0x10u,
    0x11u, 0x01u, 0x97u, 0x58u, 0x03u, 0x76u, 0x00u, 0x00u,
    0x11u, 0x01u, 0x80u, 0x47u, 0x02u, 0x65u, 0x00u, 0x00u,
    0x11u, 0x01u, 0x70u, 0x36u, 0x02u, 0x54u, 0x00u, 0x00u,
    0x11u, 0x00u, 0x60u, 0x25u, 0x00u, 0x43u, 0x00u, 0x00u,
    0x11u, 0x00u, 0x50u, 0x24u, 0x00u, 0x30u, 0x00u, 0x00u,
    0x11u, 0x00u, 0x40u, 0x13u, 0x00u, 0x20u, 0x00u, 0x00u,
    0x10u, 0x00u, 0x30u, 0x12u, 0x00u, 0x20u, 0x00u, 0x00u,
    0x10u, 0x00u, 0x20u, 0x02u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x10u, 0x00u, 0x20u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x10u, 0x00u, 0x00u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x10u, 0x00u, 0x00u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
};

/* DL_0x7C28 combines (PRIM-ENV)*TEXEL0+ENV; ImpactWave ENV is black.
 * These five RGB555 palettes are the exact 5-bit result of that combine for
 * the source variants (red, green, blue, yellow, white), generated from the
 * real 0x7958 TLUT after undoing its O2R halfword swap. Logical source index 0
 * is transparent, so GL_TEXTURE_COLOR0_TRANSPARENT supplies the source A1
 * coverage and polygon alpha supplies the live PRIMITIVE alpha. */
static const u16 sNdsRendererImpactWavePalettes[NDS_RENDERER_IMPACT_WAVE_VARIANT_COUNT][16] = {
    {
        0x0007u, 0x0008u, 0x000bu, 0x000eu, 0x0011u, 0x0013u, 0x0016u, 0x0019u,
        0x001bu, 0x001eu, 0x001eu, 0x001eu, 0x001eu, 0x001eu, 0x001eu, 0x001eu,
    },
    {
        0x02c0u, 0x0300u, 0x0320u, 0x0340u, 0x0340u, 0x0360u, 0x0380u, 0x03a0u,
        0x03a0u, 0x03c0u, 0x03c0u, 0x03c0u, 0x03c0u, 0x03c0u, 0x03c0u, 0x03c0u,
    },
    {
        0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000u,
        0x0000u, 0x0000u, 0x1000u, 0x2800u, 0x4000u, 0x5400u, 0x6c00u, 0x7800u,
    },
    {
        0x02c7u, 0x0308u, 0x032bu, 0x034eu, 0x0351u, 0x0373u, 0x0396u, 0x03b9u,
        0x03bbu, 0x03deu, 0x03deu, 0x03deu, 0x03deu, 0x03deu, 0x03deu, 0x03deu,
    },
    {
        0x02c7u, 0x0308u, 0x032bu, 0x034eu, 0x0351u, 0x0373u, 0x0396u, 0x03b9u,
        0x03bbu, 0x03deu, 0x13deu, 0x2bdeu, 0x43deu, 0x57deu, 0x6fdeu, 0x7bdeu,
    },
};
static u32 sNdsRendererImpactWaveTextureName[NDS_RENDERER_IMPACT_WAVE_VARIANT_COUNT];
extern volatile u32 gNdsRebirthHaloNativeTexturePrepareCount;
extern volatile u32 gNdsRebirthHaloNativeTextureBindCount;

typedef struct NDSRebirthHaloGroup
{
    u16 first_vertex;
    u8 triangle_count;
    u8 reserved;
} NDSRebirthHaloGroup;

typedef struct NDSRebirthHaloBounds
{
    s16 min_x;
    s16 min_y;
    s16 min_z;
    s16 max_x;
    s16 max_y;
    s16 max_z;
} NDSRebirthHaloBounds;

#include "nds_rebirth_halo.generated.inc"

static u32 sNdsRendererRebirthHaloTextureName[NDS_REBIRTH_HALO_TEXTURE_COUNT];
volatile u32 gNdsRebirthHaloFullOffloadRootCount;
volatile u32 gNdsRebirthHaloFullOffloadBoundCornerCount;
volatile u32 gNdsRebirthHaloFullOffloadBoundRejectCount;
/* Roots whose bounds crossed the near plane and drew through the GX's own
 * clip (the split-matrix route); the other routes still decline them. */
volatile u32 gNdsRebirthHaloNearPlaneGxCount;
/* Visual-effect template triangles that crossed the near plane and went to the
 * clipper (a wholly-behind one fans nothing) instead of declining the owner. */
volatile u32 gNdsVisualEffectNearClipCount;
static s32 sNdsRendererHardwareProjectedDepth =
    NDS_RENDERER_HW_PROJECTED_DEPTH_BACKGROUND_START;
static u32 sNdsRendererHardwareProjectedBackground = TRUE;
static u32 sNdsRendererHardwareMatrixLoaded;
static u32 sNdsRendererHardwareMatrixMode;
static u32 sNdsRendererHardwareMatrixGeneration;
static u32 sNdsRendererMatrixGenerationSerial;

#define NDS_RENDERER_GX_STATE_TEXTURE_PARAMS (1u << 0)
#define NDS_RENDERER_GX_STATE_MATRIX_MODE    (1u << 1)
#define NDS_RENDERER_GX_STATE_POLY_FMT       (1u << 2)
#define NDS_RENDERER_GX_STATE_ALL            \
    (NDS_RENDERER_GX_STATE_TEXTURE_PARAMS |  \
     NDS_RENDERER_GX_STATE_MATRIX_MODE |     \
     NDS_RENDERER_GX_STATE_POLY_FMT)

typedef struct NDSRendererGXStateShadow
{
    u32 texture_params;
    u32 matrix_mode;
    u32 poly_fmt;
    u32 valid_mask;
} NDSRendererGXStateShadow;

static NDSRendererGXStateShadow sNdsRendererGXStateShadow;

static inline void ndsRendererHardwareInvalidateGXState(u32 mask)
{
    sNdsRendererGXStateShadow.valid_mask &= ~mask;
}

/* P2-2p8 Phase 1 slice 2c: carved names. A fighter texture the admission
 * uploads into VRAM it reserved itself has no libnds record; its cache entry
 * is named NDS_FTR_CARVE_NAME_BASE | slot and binds its own TEXIMAGE_PARAM /
 * PLTT_BASE words (ndsRendererHardwareBindCarvedState, with the admission in
 * nds_renderer_textures_effects.c). libnds names count up from 1 and are
 * recycled, so a name with these high bits can never be one of them. */
#define NDS_FTR_CARVE_NAME_BASE 0xC000u
#define NDS_FTR_CARVE_IS_NAME(name) \
    ((((u32)(name)) & 0xFF00u) == NDS_FTR_CARVE_NAME_BASE)
#define NDS_FTR_CARVE_F_CARVED 0x01u
#define NDS_FTR_CARVE_F_PALETTE 0x02u
static void ndsRendererHardwareBindCarvedState(u32 name);

static inline void ndsRendererHardwareBindTextureState(int name)
{
    if (NDS_FTR_CARVE_IS_NAME(name))
    {
        ndsRendererHardwareBindCarvedState((u32)name);
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, name);
    }
    ndsRendererHardwareInvalidateGXState(
        NDS_RENDERER_GX_STATE_TEXTURE_PARAMS);
}

/* Production preflight's source-root capacity; the fighter packet's per-root
 * patch table reuses it. */
#define NDS_NATIVE_FIGHTER_ROOT_MAX 32u
/* P2-2 fighter packet. The four-CPU stress arm measured the four fighter draws
 * at 607,040 ticks of a 1,600,832-tick median frame (NDS_R2_DRAW_SUPPRESS_MASK
 * =15 A/B, 2026-08-23), almost all of it CPU work re-deriving and re-pushing a
 * command stream that is identical from one frame to the next. This captures
 * the stream once per fighter as packed GXFIFO words -- the format the stage
 * replay already DMAs -- and replays it by DMA, patching only what moves: the
 * projection, each root's seed and joint-chain matrices, and the light vector.
 * Everything else the words depend on is in the key; a key miss re-records
 * through the ordinary path, with the hooks below teeing into the packet the
 * words the hardware receives. The words live in gSYFramebufferSets, which
 * nothing touches during a battle (include/sys/video.h), and the Results entry
 * restores that buffer's clear through ndsRendererFighterPacketRelease. */
#define NDS_FIGHTER_PACKET_SLOTS 4u
/* 141,440 B of the 147,840-byte framebuffer: stops short of the z-buffer start
 * pointer that sys/video.h documents as aliased into the buffer's tail. */
#define NDS_FIGHTER_PACKET_ARENA_WORDS 35360u
/* One 8,840-word region per source-player slot, two lean entries each. A
 * 6,528-word compact layout that gave the stage GX body a 36,992-byte tail
 * here (c116fffa03e) left an entry 2,304 list words: the four-CPU stress's
 * Donkey/Samus/Link lists (2,467/2,313/2,634) went wide, walked twice per
 * materialization and kept no variants -- FTR P95 268,032 -> 1,204,992
 * (artifacts/performance/2026-09-26_p2-2p8-ftr-item-tail). The stage body
 * lives in the general heap (nds_stage_gx.exec.inc). */
#define NDS_FIGHTER_PACKET_REGION_WORDS \
    (NDS_FIGHTER_PACKET_ARENA_WORDS / NDS_FIGHTER_PACKET_SLOTS)
_Static_assert(NDS_FIGHTER_PACKET_SLOTS == 4u,
               "fighter packet key encodes two source-player slot bits");
_Static_assert((NDS_FIGHTER_PACKET_ARENA_WORDS %
                NDS_FIGHTER_PACKET_SLOTS) == 0u,
               "fighter packet arena must divide evenly by player slot");
#define NDS_FIGHTER_PACKET_ROOT_MAX NDS_NATIVE_FIGHTER_ROOT_MAX
#define NDS_FIGHTER_PACKET_LOCAL_MAX 8u

/* AN ABSENT PLAYER'S REGION IS 35,360 IDLE BYTES FOR THE WHOLE MATCH. The arena
 * is four fixed, equal regions keyed by source-player slot, and a packet only
 * ever writes its own. Hand an idle one to a battle-lifetime pool instead of
 * charging the taskman arena for it; the Results entry's Release rewrites the
 * buffer after that scene's arena (and everything in it) is already dead. */
void *ndsRendererFighterPacketIdleRegion(u32 battle_slot, u32 *bytes)
{
    extern u16 gSYFramebufferSets[1][231][320];
    u32 region_words = NDS_FIGHTER_PACKET_REGION_WORDS;

    if ((battle_slot >= NDS_FIGHTER_PACKET_SLOTS) || (bytes == NULL))
    {
        return NULL;
    }
    *bytes = region_words * (u32)sizeof(u32);
    return (u32 *)(void *)&gSYFramebufferSets[0][0][0] +
           (battle_slot * region_words);
}
#define NDS_FIGHTER_PACKET_INDEX_NONE 0xffffu
#define NDS_FIGHTER_PACKET_KEY_WORDS 6u

typedef struct NDSFighterPacketRoot
{
    u16 seed_index;
    /* One index per MULT4x3: a packed header word can fall between two of
     * them, so their parameter blocks are not contiguous. */
    u16 local_index[NDS_FIGHTER_PACKET_LOCAL_MAX];
    u8 local_count;
    u8 parent_slot;
    u8 store_slot;
    u8 seed_is_identity;
} NDSFighterPacketRoot;

/* One DIF_AMB word the shade wrote. The churn census (2026-08-23, 480
 * frames of the four-CPU arm) put 222 of 350 re-records on the hurt flash --
 * the colour modulate and the root prim move every flash frame -- so the
 * word is re-derived on replay from the recorded light colours and the live
 * prim/modulate instead of invalidating the packet: exactly the shade's
 * ndsRendererR2MaterialColor15 inputs, with `prim_from_root` saying whether
 * the material colour was the root preamble's prim (follows the flash) or a
 * material/state-delta override (stays as recorded, as the source does). */
typedef struct NDSFighterPacketShadeSite
{
    u16 index;
    u8 root;
    u8 use_material;
    u8 prim_from_root;
    u8 reserved[3];
    u32 light_color_1;
    u32 light_color_2;
    u32 material_color;
} NDSFighterPacketShadeSite;

/* A texture the packet binds, validated on replay against the hardware
 * cache entry it was recorded from, so an eviction or re-upload of THIS
 * fighter's textures re-records this packet alone. */
typedef struct NDSFighterPacketTexture
{
    u16 slot_plus1;
    u16 name;
    u32 key_generation;
} NDSFighterPacketTexture;

/* One source G_TEXTURE_GEN run in a recorded fighter packet.  The GX packet is
 * static, but these five texture-preparation inputs plus the root's live
 * modelview and the camera LookAt vectors define the run's S/T words.  Keeping
 * them here lets replay patch only those FIFO_TEX_COORD parameters instead of
 * re-running Link's complete owner/state/run preparation every frame. */
typedef struct NDSFighterPacketTexgenGroup
{
    u32 scale_s;
    u32 scale_t;
    u32 origin_s;
    u32 origin_t;
    s32 offset;
    u16 first_site;
    u16 site_count;
    u8 root;
    u8 reserved[3];
} NDSFighterPacketTexgenGroup;

/* Packet word + source dense vertex for one live texgen coordinate. Groups own
 * contiguous site ranges; the dense id supplies the exact source normal used
 * by ndsRendererNativeRebuildProductionRunUv. */
typedef struct NDSFighterPacketTexgenSite
{
    u16 index;
    u16 dense_id;
} NDSFighterPacketTexgenSite;

/* One shade site per lit epoch. 96 since 2026-10-09: a P4 owner whose
 * canonical vector carries every joint its motions draw needs up to 70
 * (Banjo's with Kazooie, Crash's with the mask); 64 declined their draws as
 * Capacity in every high-detail frame. +640 B per packet: an entry keeps
 * 3,132 list words (the original cast's largest list is Link's 2,634). */
#define NDS_FIGHTER_PACKET_SITE_MAX 96u
/* 32 since 2026-10-05: a lean list's GX texgen groups (one per lit spherical
 * run) for the environment-mapped 1P owners -- a Polygon has 28. */
#define NDS_FIGHTER_PACKET_TEXGEN_GROUP_MAX 32u
#define NDS_FIGHTER_PACKET_TEXGEN_SITE_MAX 256u
#define NDS_FIGHTER_PACKET_TEXGEN_DENSE_MAX 32u
#define NDS_FIGHTER_PACKET_TEXGEN_GROUP_NONE 0xffffffffu
/* Four-distinct-kind Low-detail stress reaches more than 16 unique resident
 * cache identities in one fighter packet. Falling off this array is correct
 * but coarse: it reverts that packet to the global texture-generation fence,
 * so unrelated cache churn forces an otherwise-identical GX stream to be
 * re-recorded. 24 costs 64 B/packet (256 B total) and keeps residency proof
 * per texture; the packet words and BattleShip draw semantics are unchanged. */
#define NDS_FIGHTER_PACKET_TEXTURE_MAX 24u

/* P2-2p8 Phase 1 slice 1: one recorded tint-tile bind (r49: an untextured,
 * material-coloured, non-white-prim run binds an 8x8 tile of its prim). The
 * packet words are unchanged; this only names where the tile's
 * TEXIMAGE_PARAM / PLTT_BASE parameters sit and which colour chose the tile,
 * so the lean path can re-point them instead of re-recording.
 * A packet that records more sets tint_bind_overflow and is simply not
 * adopted by the lean path -- the words and the old path are unchanged.
 * Slice 2b cut this to 4 (Samus records 2); slice 3 measured Link above 4 and
 * Kirby at 19 (7 tinted shade sites, one bind per tinted run; tint_bind_seen),
 * so it is 24, and the never-read tile name is gone (12 B a bind): +224 B per
 * packet over the 2b layout. P4 measured Crash at 32-34 (2026-10-12): 300 of
 * his 305 lists overflowed, so each kept the global texture fence: 117 fence
 * rematerializations in 1,200 frames of four Crashes. 40 costs 192 B more per
 * packet (48 words of each entry's capacity; the packet sits in the borrowed
 * framebuffer). */
#define NDS_FIGHTER_PACKET_TINT_BIND_MAX 40u
typedef struct NDSFighterPacketTintBind
{
    u16 tex_index;
    u16 pal_index;
    u8 root;
    u8 prim_from_root;
    u8 reserved[2];
    u32 rgb;
} NDSFighterPacketTintBind;

typedef struct NDSFighterPacket
{
    u32 valid;
    u32 key[NDS_FIGHTER_PACKET_KEY_WORDS];
    u32 *words;
    u32 word_count;
    u32 word_capacity;
    u32 root_count;
    u16 projection_index;
    u16 light_index;
    u8 light_root;
    u8 light_valid;
    /* Set when a textured bind left no cache entry to validate; the packet
     * then keeps the global texture generation fence in its key. */
    u8 needs_fence;
    u8 texture_count;
    u8 texgen_group_count;
    /* Lean lists (2026-10-13): the root blocks recorded in the texgen site
     * table (ndsFtrLeanRootRecs), 0 when none. */
    u8 root_rec_count;
    u16 texgen_site_count;
    u32 triangle_count;
    u32 run_count;
    u32 raw_triangles;
    u32 raw_reuse;
    u32 cross_triangles;
    u32 cross_reuse;
    /* The frame-summary batch and texture-prepare counts the record frame
     * accrued between the adapter's pre-check and the finish, net of what
     * the raw/cross credits above already add back. A replay presents the
     * same batches to the hardware, so a hit credits these too and the
     * per-frame accounting every verifier reads stays exact. */
    u32 batch_begin;
    u32 batch_reuse;
    u32 batch_end;
    u32 prepare_begin;
    u32 prepare_reuse;
    u32 matrix_loads;
    u32 texture_binds;
    u32 vertex_loads;
    u32 site_count;
    /* The prim/modulate the shade sites currently encode. */
    u32 tint_modulate;
    u32 tint_prim_hash;
    NDSFighterPacketRoot roots[NDS_FIGHTER_PACKET_ROOT_MAX];
    NDSFighterPacketShadeSite sites[NDS_FIGHTER_PACKET_SITE_MAX];
    NDSFighterPacketTexture textures[NDS_FIGHTER_PACKET_TEXTURE_MAX];
    NDSFighterPacketTexgenGroup
        texgen_groups[NDS_FIGHTER_PACKET_TEXGEN_GROUP_MAX];
    NDSFighterPacketTexgenSite
        texgen_sites[NDS_FIGHTER_PACKET_TEXGEN_SITE_MAX];
    /* P2-2p8 Phase 1 slice 1 (metadata only). fence_other counts the
     * needs_fence causes that are NOT tint-tile binds. */
    u8 tint_bind_count;
    u8 tint_bind_overflow;
    u8 fence_other;
    u8 tint_bind_seen;          /* binds the record made, saturating */
    NDSFighterPacketTintBind tint_binds[NDS_FIGHTER_PACKET_TINT_BIND_MAX];
} NDSFighterPacket;

#define NDS_FIGHTER_PACKET_COLD_CODE \
    __attribute__((noinline, cold, optimize("Os")))

/* P2-2p3. The replay no longer waits for its own DMA: the geometry engine
 * drains the packet while the CPU prepares the next fighter, and whoever
 * writes the FIFO next waits first. Every GX writer outside the packet path
 * enters through one of the seams that call this (the production execute,
 * the stage owner prepare, the particle/effect/gun/halo/entry-effect submits,
 * the generic display-list executor and the end-of-frame flush); a writer
 * that bypasses them would interleave its words with the DMA's. */
/* P3 freeze report (owner hardware, 2026-10-07): a DMA0 geometry-FIFO
 * transfer that never finished -- the geometry engine stopped draining with
 * 148 commands queued and its busy bit set -- left the next FIFO writer
 * spinning on DMA0 for good (ndsFighterPacketDmaWait under the stage
 * commit). Every DMA0 wait in the renderer goes through NDS_GX_DMA0_WAIT:
 * unchanged while transfers complete; after four frames the stall is
 * recorded (the registers the freeze report prints) and cleared -- DMA0
 * stopped, the command FIFO cleared as libnds glInit clears it (GXSTAT bit
 * 29), a matrix stack error acknowledged (bit 15) -- so a frame's 3D is lost
 * instead of the console. */
volatile u32 gNdsGxStallCount;
volatile u32 gNdsGxStallSite;
volatile u32 gNdsGxStallGxstat;
volatile u32 gNdsGxStallDmaCnt;
volatile u32 gNdsGxStallPowcnt;
volatile u32 gNdsGxStallDisp3dcnt;
volatile u32 gNdsGxStallDispcnt;
volatile u32 gNdsGxStallVcount;
#define NDS_GX_DMA0_STALL_TICKS ticksFromUsec(67000u)

void __attribute__((noinline, cold)) ndsGxDma0WaitSlow(u32 site)
{
    const u32 start = (u32)tickGetCount();

    while ((DMA_CR(0) & DMA_BUSY) != 0u)
    {
        if (((u32)tickGetCount() - start) >= NDS_GX_DMA0_STALL_TICKS)
        {
            gNdsGxStallGxstat = GFX_STATUS;
            gNdsGxStallDmaCnt = DMA_CR(0);
            gNdsGxStallPowcnt = REG_POWERCNT;
            gNdsGxStallDisp3dcnt = GFX_CONTROL;
            gNdsGxStallDispcnt = REG_DISPCNT;
            gNdsGxStallVcount = REG_VCOUNT;
            gNdsGxStallSite = site;
            gNdsGxStallCount++;
            DMA_CR(0) = 0u;
            GFX_STATUS |= (1u << 29);
            GFX_STATUS |= (1u << 15);
            return;
        }
    }
}

static u32 sNdsFighterPacketDmaPending;
static inline void ndsFighterPacketDmaWait(void)
{
    if (sNdsFighterPacketDmaPending != 0u)
    {
        /* P2-2p8 Phase 1 slice 1 (Phase 0 leftover): the next FIFO writer's
         * DMA0 wait, counted only when it actually spins. */
        if ((DMA_CR(0) & DMA_BUSY) != 0u)
        {
            u32 wait_start = cpuGetTiming();

            ndsGxDma0WaitSlow(1u);
            NDS_FTR_LEAN_CTR(gNdsFtrLean.packet_dma_waits++);
            NDS_FTR_LEAN_CTR(gNdsFtrLean.packet_dma_wait_ticks +=
                                 cpuGetTiming() - wait_start);
            (void)wait_start;
        }
        sNdsFighterPacketDmaPending = 0u;
    }
}
#define NDS_FIGHTER_PACKET_DMA_WAIT() ndsFighterPacketDmaWait()
/* The packet recorder's hook sites cost ITCM (496 bytes at the second link;
 * the recorder is gone since 2026-10-05), and the region was full. Four
 * residents the 2026-08-22 four-CPU census ranks at the
 * bottom of its rent table (ndsRendererSetParticleCamera 320 B at 1,314
 * cycles/byte, ndsRendererMtxLoadN64ToDS20p12 256 B, ndsRendererRecordSetTileSize
 * 180 B, ndsRendererRecordLoadBlock 100 B -- under 3,000 ticks/frame between
 * them) go to main RAM in a packet build only, so the control arm keeps its
 * exact placement. */
#define NDS_FIGHTER_PACKET_EVICT(attr)

/* P2-6 (2026-10-02): GX TEXGEN. A lean list's spherical texgen run whose
 * corners send GFX_NORMAL (the lit runs) carries no per-corner TEXCOORD; its
 * texture bind takes TEXIMAGE_PARAM texgen mode 2 (normal source) and, before
 * its BEGIN, the list loads the texture matrix the patch rewrites per frame
 * (rows 0-2: the S and T lookat directions times the run's scale, see
 * ndsFighterPacketHwTexgenEntry) and one TEXCOORD with the constant part of
 * the source's texcoord. The engine then forms S = base + N.M per corner,
 * which is ndsRendererNativeTexgenCoord's expression to within 1/16 texel
 * (the clamp of the dot aside, which only a normal longer than 127 reaches).
 * The Fighting Polygons are environment-mapped all over. LINEAR and unlit runs
 * keep the CPU patch. Same-ROM A/B word (0 = CPU patch; read when a list is
 * made). */
volatile u32 gNdsFighterPacketHwTexgen __attribute__((used, section(".data"))) =
    1u;

static inline void ndsRendererHardwareSetMatrixMode(int mode)
{
    glMatrixMode(mode);
}

static inline void ndsRendererHardwareSetPolyFmt(u32 poly_fmt)
{
    if (((sNdsRendererGXStateShadow.valid_mask &
          NDS_RENDERER_GX_STATE_POLY_FMT) != 0u) &&
        (sNdsRendererGXStateShadow.poly_fmt == poly_fmt))
    {
        return;
    }
    glPolyFmt(poly_fmt);
    sNdsRendererGXStateShadow.poly_fmt = poly_fmt;
    sNdsRendererGXStateShadow.valid_mask |= NDS_RENDERER_GX_STATE_POLY_FMT;
}
static u32 sNdsRendererHardwareSubmitClassCounts[
    NDS_RENDERER_HW_SUBMIT_CLASS_COUNT];
/* The logical demand is derived exactly from the submitted class totals.
 * Profiles 1/2 pack actual call/clamp counts plus error flags into this one
 * existing summary word, avoiding forensic BSS growth in the tight P1 build. */
#define NDS_RENDERER_HW_DIVISION_MISMATCH           (1u << 29)
static u32 sNdsRendererHardwareDivideSummary;
static u32 sNdsRendererHardwareSourceVertexLoadCount;
static u32 sNdsRendererHardwareCPUTransformCount;
static u32 sNdsRendererHardwareTransformCacheHitCount;
static u32 sNdsRendererHardwareMatrixSnapshotCreateCount;
static u32 sNdsRendererHardwareMatrixSnapshotReuseCount;
static u32 sNdsRendererHardwareMatrixSnapshotOverflowCount;

/* Profile levels 0/1 accumulate the small runtime health summary in ordinary
 * memory and publish it once at frame completion. The performance build never
 * writes the exported volatile profile globals from command, triangle, or
 * vertex loops. Level 2 retains the exact forensic counters below. */
typedef struct NDSRendererRuntimeFrameSummary
{
    u32 texture_binds;
    u32 texture_uploads;
    u32 texture_upload_bytes;
    u32 texture_cache_alias_avoid_count;
    u32 texture_lookup_call_count;
    u32 texture_lookup_probe_count;
    u32 texture_lookup_active_hit_count;
    u32 texture_lookup_table_hit_count;
    u32 texture_lookup_miss_count;
    u32 texel1_composite_count;
    u32 texel1_load_match_count;
    u32 texel1_reject_count;
    u32 projected_submit_fallback_count;
    u32 matrix_load_count;
    u32 hardware_vertices;
    u32 hardware_triangles;
    u32 hardware_batch_begin_count;
    u32 hardware_batch_reuse_count;
    u32 hardware_batch_end_count;
    u32 texture_prepare_count;
    u32 texture_prepare_reuse_count;
    u32 hardware_over_limit;
    u32 hardware_vertex_saturate_count;
    u32 near_plane_triangle_reject_count;
    u32 raw_current_candidate_count;
    u32 raw_current_range_reject_count;
    u32 raw_cross_matrix_count;
} NDSRendererRuntimeFrameSummary;

/* 108 bytes in DTCM, which is the cheapest place on this machine to do a
 * read-modify-write. Cycle 110 priced these counters by compiling them out:
 * FTR fell 7,378 and STG 2,776, an order of magnitude past the ~1,300 the
 * per-line profile showed, because the profile only sees the two symbols the
 * lines live in and every hardware batch on every path pays them. They are not
 * removable -- verify-battle-mariofox-gcrunall-loop-harness.ps1 asserts exact
 * batch and texture-prepare accounting off them, and the Boundary profile runs
 * it -- so the fix is to stop paying main-memory latency and a cache line for
 * evidence.
 *
 * `.dtcm` (loaded), where check-task20-dtcm-layout.ps1 models it; it used to
 * ride `.dtcm.fighter`, which left DTCM with Mario's dense tables (P2-2p8,
 * the DTCM hot stack).
 *
 * Audited against that gate's DMA/IPC/ARM7 requirement, same as the two vertex
 * tables above it: written and read only by ARM9 renderer code, never a DMA
 * source or destination, never visible to the ARM7 or IPC. DMA cannot read
 * DTCM at all. Zero-initialised at every frame begin by memset, so nothing
 * depends on the crt's copy of its (zero) image. */
static NDSRendererRuntimeFrameSummary sNdsRendererRuntimeFrameSummary
    __attribute__((section(".dtcm")));
static u32 sNdsRendererRuntimeTexel1FractionRefreshCount;
static u32 sNdsRendererRuntimeTextureCacheEvictCount;
static u32 sNdsRendererRuntimeTextureCi4DirectPixels;
static u32 sNdsRendererRuntimeCi4IndexCacheBuildCount;
static u32 sNdsRendererRuntimeCi4IndexCacheReuseCount;
static u32 sNdsRendererRuntimeCi4RepresentativePixelCount;
static u32 sNdsRendererRuntimeCi4ReusePixelCount;

volatile u32 gNdsRendererBattleStaticTextureEnabled =
    NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT;
volatile u32 gNdsRendererBattleStaticTexturePrepareCount;
volatile u32 gNdsRendererBattleStaticTexturePrepareFailCount;
/* WHICH of the eleven declines in ndsRendererHardwarePrepareBattleStaticTextures
 * fired, 1..11 in source order. The count alone said the Dream Land pin set
 * failed to prepare in a Mario mirror match -- prepared 0, failed 1 -- and
 * could not say whether that was the payload open, the palette block, a
 * record field, or a key that was already resident. */
volatile u32 gNdsRendererBattleStaticTextureFailStep;
/* Mushroom Kingdom Pakkun admission: how far the nine-clause candidate
 * test got, plus what its material actually looked like and whether the
 * submit itself accepted. */
/* Sector Z Arwing laser admission: how far the seven-clause candidate test
 * got (7 = admitted), the submit's own step (9 = drew, 3 = texture bind,
 * 11/12 = argument/matrix), the two relocated texture pointers, the two
 * config matrices and the resolved polygon alpha. */
/* Peach's Castle bumper. Defined unconditionally like the Pakkun and Sector
 * laser rows so a non-Castle ELF still carries the symbols. */
/* Link's Bomb.  Defined unconditionally like the Pakkun, Sector laser and
 * Castle bumper rows so a non-Link ELF still carries the symbols and a
 * gc-sections pass cannot drop a witness the harness reads. */
volatile u32 gNdsLinkBombCandidateStep;
volatile u32 gNdsLinkBombItemKind;
volatile u32 gNdsLinkBombForeignKindCount;
volatile u32 gNdsLinkBombDrawCount;
volatile u32 gNdsLinkBombSubmitFailCount;
volatile u32 gNdsLinkBombSubmitStep;
volatile u32 gNdsLinkBombRoot;
volatile u32 gNdsLinkBombHead;
volatile u32 gNdsLinkBombTlut;
volatile u32 gNdsLinkBombImage;
volatile u32 gNdsLinkBombProjection;
volatile u32 gNdsLinkBombModelview;
volatile u32 gNdsLinkBombAlpha;
/* Saffron City Marumine.  Defined unconditionally like the Pakkun, Sector
 * laser, Castle bumper and Link bomb rows so a non-Yamabuki ELF still carries
 * the symbols and a gc-sections pass cannot drop a witness the harness reads. */
volatile u32 gNdsYamabukiMarumineCandidateStep;
volatile u32 gNdsYamabukiMarumineItemKind;
volatile u32 gNdsYamabukiMarumineForeignKindCount;
volatile u32 gNdsYamabukiMarumineDrawCount;
volatile u32 gNdsYamabukiMarumineSubmitFailCount;
volatile u32 gNdsYamabukiMarumineSubmitStep;
volatile u32 gNdsYamabukiMarumineTlut;
volatile u32 gNdsYamabukiMarumineImage;
volatile u32 gNdsYamabukiMarumineProjection;
volatile u32 gNdsYamabukiMarumineModelview;
volatile u32 gNdsYamabukiMarumineAlpha;
volatile u32 gNdsYamabukiGluckyCandidateStep;
volatile u32 gNdsYamabukiGluckyItemKind;
volatile u32 gNdsYamabukiGluckyForeignKindCount;
volatile u32 gNdsYamabukiGluckyDrawCount;
volatile u32 gNdsYamabukiGluckySubmitFailCount;
volatile u32 gNdsYamabukiGluckySubmitStep;
volatile u32 gNdsYamabukiGluckyTlut;
volatile u32 gNdsYamabukiGluckyImage;
volatile u32 gNdsYamabukiGluckyProjection;
volatile u32 gNdsYamabukiGluckyModelview;
volatile u32 gNdsYamabukiGluckyAlpha;
volatile u32 gNdsYamabukiPorygonCandidateStep;
volatile u32 gNdsYamabukiPorygonItemKind;
volatile u32 gNdsYamabukiPorygonForeignKindCount;
volatile u32 gNdsYamabukiPorygonDrawCount;
volatile u32 gNdsYamabukiPorygonSubmitFailCount;
volatile u32 gNdsYamabukiPorygonSubmitStep;
volatile u32 gNdsYamabukiPorygonTlut;
volatile u32 gNdsYamabukiPorygonImage;
volatile u32 gNdsYamabukiPorygonProjection;
volatile u32 gNdsYamabukiPorygonModelview;
volatile u32 gNdsYamabukiPorygonAlpha;
volatile u32 gNdsYamabukiHitokageCandidateStep;
volatile u32 gNdsYamabukiHitokageItemKind;
volatile u32 gNdsYamabukiHitokageForeignKindCount;
volatile u32 gNdsYamabukiHitokageDrawCount;
volatile u32 gNdsYamabukiHitokageSubmitFailCount;
volatile u32 gNdsYamabukiHitokageSubmitStep;
volatile u32 gNdsYamabukiHitokageEffectsSeen;
volatile u32 gNdsYamabukiHitokageEffectsRejected;
volatile u32 gNdsYamabukiHitokageSnapshotFailCount;
volatile u32 gNdsYamabukiHitokageTlut;
volatile u32 gNdsYamabukiHitokageImage;
volatile u32 gNdsYamabukiHitokageProjection;
volatile u32 gNdsYamabukiHitokageModelview;
volatile u32 gNdsYamabukiHitokageAlpha;
volatile u32 gNdsYamabukiFushigibanaCandidateStep;
volatile u32 gNdsYamabukiFushigibanaItemKind;
volatile u32 gNdsYamabukiFushigibanaForeignKindCount;
volatile u32 gNdsYamabukiFushigibanaDrawCount;
volatile u32 gNdsYamabukiFushigibanaSubmitFailCount;
volatile u32 gNdsYamabukiFushigibanaSubmitStep;
volatile u32 gNdsYamabukiFushigibanaEffectsSeen;
volatile u32 gNdsYamabukiFushigibanaEffectsRejected;
volatile u32 gNdsYamabukiFushigibanaSnapshotFailCount;
volatile u32 gNdsYamabukiFushigibanaTlut;
volatile u32 gNdsYamabukiFushigibanaImage;
volatile u32 gNdsYamabukiFushigibanaProjection;
volatile u32 gNdsYamabukiFushigibanaModelview;
volatile u32 gNdsYamabukiFushigibanaAlpha;
/* Samus Charge Shot admission and submit witnesses. */
volatile u32 gNdsChargeShotCandidateStep;
volatile u32 gNdsChargeShotDrawCount;
volatile u32 gNdsChargeShotSubmitFailCount;
volatile u32 gNdsChargeShotSubmitStep;
volatile u32 gNdsChargeShotProjection;
volatile u32 gNdsChargeShotModelview;
volatile u32 gNdsChargeShotAlpha;
/* Pikachu air Thunder Jolt.  Defined unconditionally like every other owner
 * row so a gc-sections pass cannot drop a witness the harness reads. */
volatile u32 gNdsThunderJoltCandidateStep;
volatile u32 gNdsThunderJoltDrawCount;
volatile u32 gNdsThunderJoltSubmitFailCount;
volatile u32 gNdsThunderJoltSubmitStep;
volatile u32 gNdsThunderJoltTlut;
volatile u32 gNdsThunderJoltImage;
volatile u32 gNdsThunderJoltProjection;
volatile u32 gNdsThunderJoltModelview;
volatile u32 gNdsThunderJoltAlpha;
volatile u32 gNdsThunderGroundEffectsSeen;
volatile u32 gNdsThunderGroundRootMask;
volatile u32 gNdsThunderGroundSnapshotFailCount;
volatile u32 gNdsThunderGroundCandidateStep;
volatile u32 gNdsThunderGroundDrawCount;
volatile u32 gNdsThunderGroundSubmitFailCount;
volatile u32 gNdsThunderGroundSubmitStep;
volatile u32 gNdsThunderGroundEffectsRejected;
volatile u32 gNdsThunderGroundProjection;
volatile u32 gNdsThunderGroundModelview;
volatile u32 gNdsThunderGroundAlpha;
volatile u32 gNdsThunderJoltFxEffectsSeen;
volatile u32 gNdsThunderJoltFxSnapshotFailCount;
volatile u32 gNdsThunderJoltFxCandidateStep;
volatile u32 gNdsThunderJoltFxDrawCount;
volatile u32 gNdsThunderJoltFxSubmitFailCount;
volatile u32 gNdsThunderJoltFxSubmitStep;
volatile u32 gNdsThunderJoltFxEffectsRejected;
volatile u32 gNdsThunderJoltFxProjection;
volatile u32 gNdsThunderJoltFxModelview;
volatile u32 gNdsThunderJoltFxTlut;
volatile u32 gNdsThunderJoltFxAlpha;
volatile u32 gNdsItemTomatoKind;
volatile u32 gNdsItemTomatoForeignKindCount;
volatile u32 gNdsItemTomatoCandidateStep;
volatile u32 gNdsItemTomatoDrawCount;
volatile u32 gNdsItemTomatoSubmitFailCount;
volatile u32 gNdsItemTomatoSubmitStep;
volatile u32 gNdsItemTomatoProjection;
volatile u32 gNdsItemTomatoModelview;
volatile u32 gNdsItemTomatoTlut;
volatile u32 gNdsItemTomatoImage;
volatile u32 gNdsItemTomatoAlpha;
/* Common-item native-owner witnesses. Keep the draw/failure counters used even
 * when a particular harness does not name them directly: --gc-sections has
 * previously removed diagnostic-only globals and made an inert owner
 * indistinguishable from one that never spawned. */
volatile u32 gNdsItemStarKind __attribute__((used));
volatile u32 gNdsItemStarForeignKindCount __attribute__((used));
volatile u32 gNdsItemStarCandidateStep __attribute__((used));
volatile u32 gNdsItemStarDrawCount __attribute__((used));
volatile u32 gNdsItemStarSubmitFailCount __attribute__((used));
volatile u32 gNdsItemStarSubmitStep __attribute__((used));
volatile u32 gNdsItemStarEffectsSeen __attribute__((used));
volatile u32 gNdsItemStarEffectsRejected __attribute__((used));
volatile u32 gNdsItemStarSnapshotFailCount __attribute__((used));
volatile u32 gNdsItemStarAlpha __attribute__((used));
volatile u32 gNdsItemSwordKind __attribute__((used));
volatile u32 gNdsItemSwordForeignKindCount __attribute__((used));
volatile u32 gNdsItemSwordCandidateStep __attribute__((used));
volatile u32 gNdsItemSwordDrawCount __attribute__((used));
volatile u32 gNdsItemSwordSubmitFailCount __attribute__((used));
volatile u32 gNdsItemSwordSubmitStep __attribute__((used));
volatile u32 gNdsItemSwordRoot __attribute__((used));
volatile u32 gNdsItemSwordAlpha __attribute__((used));
volatile u32 gNdsItemHammerKind __attribute__((used));
volatile u32 gNdsItemHammerForeignKindCount __attribute__((used));
volatile u32 gNdsItemHammerCandidateStep __attribute__((used));
volatile u32 gNdsItemHammerDrawCount __attribute__((used));
volatile u32 gNdsItemHammerSubmitFailCount __attribute__((used));
volatile u32 gNdsItemHammerSubmitStep __attribute__((used));
volatile u32 gNdsItemHammerAlpha __attribute__((used));
volatile u32 gNdsItemMBallKind __attribute__((used));
volatile u32 gNdsItemMBallForeignKindCount __attribute__((used));
volatile u32 gNdsItemMBallCandidateStep __attribute__((used));
volatile u32 gNdsItemMBallDrawCount __attribute__((used));
volatile u32 gNdsItemMBallSubmitFailCount __attribute__((used));
volatile u32 gNdsItemMBallSubmitStep __attribute__((used));
volatile u32 gNdsItemMBallRoot __attribute__((used));
volatile u32 gNdsItemMBallEffectsSeen __attribute__((used));
volatile u32 gNdsItemMBallEffectsRejected __attribute__((used));
volatile u32 gNdsItemMBallSnapshotFailCount __attribute__((used));
volatile u32 gNdsItemMBallAlpha __attribute__((used));
volatile u32 gNdsItemGShellKind __attribute__((used));
volatile u32 gNdsItemGShellForeignKindCount __attribute__((used));
volatile u32 gNdsItemGShellCandidateStep __attribute__((used));
volatile u32 gNdsItemGShellDrawCount __attribute__((used));
volatile u32 gNdsItemGShellSubmitFailCount __attribute__((used));
volatile u32 gNdsItemGShellSubmitStep __attribute__((used));
volatile u32 gNdsItemGShellEffectsSeen __attribute__((used));
volatile u32 gNdsItemGShellEffectsRejected __attribute__((used));
volatile u32 gNdsItemGShellSnapshotFailCount __attribute__((used));
volatile u32 gNdsItemGShellAlpha __attribute__((used));
volatile u32 gNdsItemBatKind __attribute__((used));
volatile u32 gNdsItemBatForeignKindCount __attribute__((used));
volatile u32 gNdsItemBatCandidateStep __attribute__((used));
volatile u32 gNdsItemBatDrawCount __attribute__((used));
volatile u32 gNdsItemBatSubmitFailCount __attribute__((used));
volatile u32 gNdsItemBatSubmitStep __attribute__((used));
volatile u32 gNdsItemBatRoot __attribute__((used));
volatile u32 gNdsItemBatAlpha __attribute__((used));
volatile u32 gNdsItemBombHeiKind __attribute__((used));
volatile u32 gNdsItemBombHeiForeignKindCount __attribute__((used));
volatile u32 gNdsItemBombHeiCandidateStep __attribute__((used));
volatile u32 gNdsItemBombHeiDrawCount __attribute__((used));
volatile u32 gNdsItemBombHeiSubmitFailCount __attribute__((used));
volatile u32 gNdsItemBombHeiSubmitStep __attribute__((used));
volatile u32 gNdsItemBombHeiEffectsSeen __attribute__((used));
volatile u32 gNdsItemBombHeiEffectsRejected __attribute__((used));
volatile u32 gNdsItemBombHeiSnapshotFailCount __attribute__((used));
volatile u32 gNdsItemBombHeiAlpha __attribute__((used));
volatile u32 gNdsItemRShellKind __attribute__((used));
volatile u32 gNdsItemRShellForeignKindCount __attribute__((used));
volatile u32 gNdsItemRShellCandidateStep __attribute__((used));
volatile u32 gNdsItemRShellDrawCount __attribute__((used));
volatile u32 gNdsItemRShellSubmitFailCount __attribute__((used));
volatile u32 gNdsItemRShellSubmitStep __attribute__((used));
volatile u32 gNdsItemRShellEffectsSeen __attribute__((used));
volatile u32 gNdsItemRShellEffectsRejected __attribute__((used));
volatile u32 gNdsItemRShellSnapshotFailCount __attribute__((used));
volatile u32 gNdsItemRShellAlpha __attribute__((used));
volatile u32 gNdsItemLGunKind __attribute__((used));
volatile u32 gNdsItemLGunForeignKindCount __attribute__((used));
volatile u32 gNdsItemLGunCandidateStep __attribute__((used));
volatile u32 gNdsItemLGunDrawCount __attribute__((used));
volatile u32 gNdsItemLGunSubmitFailCount __attribute__((used));
volatile u32 gNdsItemLGunSubmitStep __attribute__((used));
volatile u32 gNdsItemLGunAlpha __attribute__((used));
volatile u32 gNdsItemHarisenKind __attribute__((used));
volatile u32 gNdsItemHarisenForeignKindCount __attribute__((used));
volatile u32 gNdsItemHarisenCandidateStep __attribute__((used));
volatile u32 gNdsItemHarisenDrawCount __attribute__((used));
volatile u32 gNdsItemHarisenSubmitFailCount __attribute__((used));
volatile u32 gNdsItemHarisenSubmitStep __attribute__((used));
volatile u32 gNdsItemHarisenAlpha __attribute__((used));
volatile u32 gNdsItemHeartKind __attribute__((used));
volatile u32 gNdsItemHeartForeignKindCount __attribute__((used));
volatile u32 gNdsItemHeartCandidateStep __attribute__((used));
volatile u32 gNdsItemHeartDrawCount __attribute__((used));
volatile u32 gNdsItemHeartSubmitFailCount __attribute__((used));
volatile u32 gNdsItemHeartSubmitStep __attribute__((used));
volatile u32 gNdsItemHeartAlpha __attribute__((used));
volatile u32 gNdsItemStarRodKind __attribute__((used));
volatile u32 gNdsItemStarRodForeignKindCount __attribute__((used));
volatile u32 gNdsItemStarRodCandidateStep __attribute__((used));
volatile u32 gNdsItemStarRodDrawCount __attribute__((used));
volatile u32 gNdsItemStarRodSubmitFailCount __attribute__((used));
volatile u32 gNdsItemStarRodSubmitStep __attribute__((used));
volatile u32 gNdsItemStarRodRoot __attribute__((used));
volatile u32 gNdsItemStarRodAlpha __attribute__((used));
volatile u32 gNdsItemFFlowerKind __attribute__((used));
volatile u32 gNdsItemFFlowerForeignKindCount __attribute__((used));
volatile u32 gNdsItemFFlowerCandidateStep __attribute__((used));
volatile u32 gNdsItemFFlowerDrawCount __attribute__((used));
volatile u32 gNdsItemFFlowerSubmitFailCount __attribute__((used));
volatile u32 gNdsItemFFlowerSubmitStep __attribute__((used));
volatile u32 gNdsItemFFlowerRoot __attribute__((used));
volatile u32 gNdsItemFFlowerEffectsSeen __attribute__((used));
volatile u32 gNdsItemFFlowerEffectsRejected __attribute__((used));
volatile u32 gNdsItemFFlowerSnapshotFailCount __attribute__((used));
volatile u32 gNdsItemFFlowerAlpha __attribute__((used));
volatile u32 gNdsItemMSBombKind __attribute__((used));
volatile u32 gNdsItemMSBombForeignKindCount __attribute__((used));
volatile u32 gNdsItemMSBombCandidateStep __attribute__((used));
volatile u32 gNdsItemMSBombDrawCount __attribute__((used));
volatile u32 gNdsItemMSBombSubmitFailCount __attribute__((used));
volatile u32 gNdsItemMSBombSubmitStep __attribute__((used));
volatile u32 gNdsItemMSBombRoot __attribute__((used));
volatile u32 gNdsItemMSBombAlpha __attribute__((used));
volatile u32 gNdsItemNBumperKind __attribute__((used));
volatile u32 gNdsItemNBumperForeignKindCount __attribute__((used));
volatile u32 gNdsItemNBumperCandidateStep __attribute__((used));
volatile u32 gNdsItemNBumperDrawCount __attribute__((used));
volatile u32 gNdsItemNBumperSubmitFailCount __attribute__((used));
volatile u32 gNdsItemNBumperSubmitStep __attribute__((used));
volatile u32 gNdsItemNBumperEffectsSeen __attribute__((used));
volatile u32 gNdsItemNBumperEffectsRejected __attribute__((used));
volatile u32 gNdsItemNBumperSnapshotFailCount __attribute__((used));
volatile u32 gNdsItemNBumperAlpha __attribute__((used));
volatile u32 gNdsItemBoxKind __attribute__((used));
volatile u32 gNdsItemBoxForeignKindCount __attribute__((used));
volatile u32 gNdsItemBoxCandidateStep __attribute__((used));
volatile u32 gNdsItemBoxDrawCount __attribute__((used));
volatile u32 gNdsItemBoxSubmitFailCount __attribute__((used));
volatile u32 gNdsItemBoxSubmitStep __attribute__((used));
volatile u32 gNdsItemBoxAlpha __attribute__((used));
volatile u32 gNdsItemTaruKind __attribute__((used));
volatile u32 gNdsItemTaruForeignKindCount __attribute__((used));
volatile u32 gNdsItemTaruCandidateStep __attribute__((used));
volatile u32 gNdsItemTaruDrawCount __attribute__((used));
volatile u32 gNdsItemTaruSubmitFailCount __attribute__((used));
volatile u32 gNdsItemTaruSubmitStep __attribute__((used));
volatile u32 gNdsItemTaruAlpha __attribute__((used));
volatile u32 gNdsItemEggKind __attribute__((used));
volatile u32 gNdsItemEggForeignKindCount __attribute__((used));
volatile u32 gNdsItemEggCandidateStep __attribute__((used));
volatile u32 gNdsItemEggDrawCount __attribute__((used));
volatile u32 gNdsItemEggSubmitFailCount __attribute__((used));
volatile u32 gNdsItemEggSubmitStep __attribute__((used));
volatile u32 gNdsItemEggAlpha __attribute__((used));
volatile u32 gNdsItemIwarkKind __attribute__((used));
volatile u32 gNdsItemIwarkForeignKindCount __attribute__((used));
volatile u32 gNdsItemIwarkCandidateStep __attribute__((used));
volatile u32 gNdsItemIwarkDrawCount __attribute__((used));
volatile u32 gNdsItemIwarkSubmitFailCount __attribute__((used));
volatile u32 gNdsItemIwarkSubmitStep __attribute__((used));
volatile u32 gNdsItemIwarkAlpha __attribute__((used));
volatile u32 gNdsItemCapsuleKind __attribute__((used));
volatile u32 gNdsItemCapsuleForeignKindCount __attribute__((used));
volatile u32 gNdsItemCapsuleCandidateStep __attribute__((used));
volatile u32 gNdsItemCapsuleDrawCount __attribute__((used));
volatile u32 gNdsItemCapsuleSubmitFailCount __attribute__((used));
volatile u32 gNdsItemCapsuleSubmitStep __attribute__((used));
volatile u32 gNdsItemCapsuleRoot __attribute__((used));
volatile u32 gNdsItemCapsuleAlpha __attribute__((used));
volatile u32 gNdsCastleBumperCandidateStep;
volatile u32 gNdsCastleBumperItemKind;
volatile u32 gNdsCastleBumperForeignKindCount;
volatile u32 gNdsCastleBumperEffects;
volatile u32 gNdsCastleBumperDrawCount;
volatile u32 gNdsCastleBumperSubmitFailCount;
volatile u32 gNdsCastleBumperSubmitStep;
volatile u32 gNdsCastleBumperPaletteImage;
volatile u32 gNdsCastleBumperPaletteId;
volatile u32 gNdsCastleBumperProjection;
volatile u32 gNdsCastleBumperModelview;
volatile u32 gNdsCastleBumperAlpha;
volatile u32 gNdsSectorLaserCandidateStep;
volatile u32 gNdsSectorLaserDrawCount;
volatile u32 gNdsSectorLaserSubmitFailCount;
volatile u32 gNdsSectorLaserSubmitStep;
volatile u32 gNdsSectorLaserTlut;
volatile u32 gNdsSectorLaserImage;
volatile u32 gNdsSectorLaserProjection;
volatile u32 gNdsSectorLaserModelview;
volatile u32 gNdsSectorLaserAlpha;
volatile u32 gNdsInishiePakkunCandidateStep;
volatile u32 gNdsInishiePakkunMaterialFlags;
volatile u32 gNdsInishiePakkunEffects;
volatile u32 gNdsInishiePakkunDrawCount;
volatile u32 gNdsInishiePakkunSubmitFailCount;
volatile u32 gNdsInishiePakkunSubmitStep;
volatile u32 gNdsInishiePakkunImageW0;
volatile u32 gNdsInishiePakkunImage;
volatile u32 gNdsInishiePakkunProjection;
volatile u32 gNdsInishiePakkunModelview;
/* Mushroom Kingdom POW block.  Defined unconditionally like every other owner
 * row so a gc-sections pass cannot drop a witness the harness reads. */
volatile u32 gNdsInishiePowblockCandidateStep;
volatile u32 gNdsInishiePowblockDrawCount;
volatile u32 gNdsInishiePowblockSubmitFailCount;
volatile u32 gNdsInishiePowblockSubmitStep;
volatile u32 gNdsInishiePowblockTlut;
volatile u32 gNdsInishiePowblockImageA;
volatile u32 gNdsInishiePowblockImageB;
volatile u32 gNdsInishiePowblockProjection;
volatile u32 gNdsInishiePowblockModelview;
volatile u32 gNdsInishiePowblockAlpha;
/* Records the pin set skipped because their fighter is not in this match. */
volatile u32 gNdsRendererBattleStaticTextureSkippedCount;
volatile u32 gNdsRendererBattleStaticTexturePreparedCount;
volatile u32 gNdsRendererBattleStaticTexturePreparedBytes;
volatile u32 gNdsRendererBattleStaticTextureRefreshCount;
volatile u32 gNdsRendererBattleStaticTextureRefreshedEntryCount;
volatile u32 gNdsRendererBattleStaticTextureArmCount;
volatile u32 gNdsRendererBattleStaticTexturePinnedHitCount;
volatile u32 gNdsRendererBattleStaticTextureSeenMask;
volatile u32 gNdsRendererBattleStaticTextureSeenMaskHi;
volatile u32 gNdsRendererBattleStaticTextureOwnerMask;
volatile u32 gNdsRendererBattleStaticTextureViolationCount;
volatile u32 gNdsRendererBattleStaticTextureTeardownCount;
volatile u32 gNdsRendererBattleStaticTextureFirstAddress;
volatile u32 gNdsRendererBattleStaticTextureEndAddress;
volatile u32 gNdsRendererBattleStaticTextureAllocationSpanBytes;
volatile u32 gNdsRendererBattleStaticTextureBankMask;

/* BUGS ROW 6. THE MOST DESTRUCTIVE EVENT IN THIS RENDERER WAS INVISIBLE IN THE
 * CONFIGURATION THAT SHIPS.
 *
 * ndsRendererAdapterPrepareNativeStageOwner rejects through a label that calls
 * ndsRendererHardwareAbortBattleStaticTextures, which discards the entire
 * hardware texture cache and clears sNdsRendererBattleStaticTexturePrepared --
 * and ndsRendererHardwareArmBattleStaticTextures refuses to re-arm without
 * that flag. So one transient "this frame cannot use the native fast path"
 * removes every texture in the scene for the remainder of the match, with no
 * path back. That is scene-wide and permanent, which is verbatim what the
 * owner reported.
 *
 * The only existing records of it were gNdsRendererM3PostArmFailureCount and
 * gNdsRendererTask36AdapterRejectReason, both inside
 * `#if NDS_RENDERER_PROFILE_LEVEL == 1` and both nm-confirmed absent from the
 * shipping ELF -- so a shipping build could destroy its own texture cache and
 * leave no counter behind. These are unconditional.
 *
 * The FIRST-reject latch is the load-bearing field. A count only says it
 * happened; the reason names which of the six branches in that function to go
 * and read, which is the difference between a next cycle that measures and a
 * next cycle that hunts. Reason codes are that function's own: 2 null CObj,
 * 3 asset lookup/size/generation, 4 topology collect or stamp, 5 matrices,
 * 6 ndsRendererPrepareNativeStageOwner, 7 materials, 1 unreached default. */
volatile u32 gNdsRendererStageOwnerRejectCount;
volatile u32 gNdsRendererStageOwnerLastRejectReason;
volatile u32 gNdsRendererStageOwnerFirstRejectReason;
volatile u32 gNdsRendererStageOwnerAbortCount;
/* Post-arm rejects, which used to be the destructive case. This is the counter
 * that keeps the 2026-08-04 fix honest: it must climb exactly where the abort
 * used to, while gNdsRendererStageOwnerAbortCount stays 0 and
 * gNdsRendererStaticTexturePreparedNow stays 1. If this reads 0 across a match
 * with a death, the lever is not being exercised and the proof is vacuous. */
volatile u32 gNdsRendererStageOwnerPostArmRejectCount;
/* Mirrors sNdsRendererBattleStaticTexturePrepared, which is static and so
 * cannot be read from a probe. Without it the permanent-latch claim is an
 * inference from reading the source; with it the latch is observable. */
volatile u32 gNdsRendererStaticTexturePreparedNow;

static u32 sNdsRendererBattleStaticTexturePrepared;
static u32 sNdsRendererBattleStaticTextureArmed;
volatile u32 gNdsRendererBattleTextureFenceCounts[
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_COUNT];
volatile u32 gNdsRendererBattleTextureFenceFirstClassPlus1;
volatile u32 gNdsRendererBattleTextureFenceFirstFrame;
volatile u32 gNdsRendererBattleTextureFallbackFirstFrame;
volatile u32 gNdsRendererBattleTextureFallbackFirstKeyHash;
volatile u32 gNdsRendererBattleTextureFallbackFirstImageAsset;
volatile u32 gNdsRendererBattleTextureFallbackFirstImageOffset;
volatile u32 gNdsRendererBattleTextureFallbackFirstTlutAsset;
volatile u32 gNdsRendererBattleTextureFallbackFirstTlutOffset;
volatile u32 gNdsRendererBattleTextureFallbackFirstFormat;
volatile u32 gNdsRendererBattleTextureFallbackFirstSize;
volatile u32 gNdsRendererBattleTextureFallbackFirstWidth;
volatile u32 gNdsRendererBattleTextureFallbackFirstHeight;
volatile u32 gNdsRendererBattleTextureFallbackFirstFlags;
volatile u32 gNdsRendererBattleTextureFallbackFirstCombineW0;
volatile u32 gNdsRendererBattleTextureFallbackFirstCombineW1;

_Static_assert(NDS_RENDERER_BATTLE_TEXTURE_FENCE_COUNT == 10,
               "post-GO texture fence schema changed");
_Static_assert(sizeof(gNdsRendererBattleTextureFenceCounts) ==
                   (10u * sizeof(u32)),
               "post-GO texture fence must remain 40 bytes");
_Static_assert(sizeof(gNdsRendererBattleTextureFenceCounts) +
                   sizeof(gNdsRendererBattleTextureFenceFirstClassPlus1) +
                   sizeof(gNdsRendererBattleTextureFenceFirstFrame) == 48u,
               "post-GO texture fence diagnostics must remain 48 bytes");

static inline void ndsRendererHardwareRecordBattleTextureFence(
    NDSRendererBattleTextureFenceClass fence_class)
{
    if ((sNdsRendererBattleStaticTextureArmed == 0u) ||
        ((u32)fence_class >= NDS_RENDERER_BATTLE_TEXTURE_FENCE_COUNT))
    {
        return;
    }
    if (gNdsRendererBattleTextureFenceFirstClassPlus1 == 0u)
    {
        gNdsRendererBattleTextureFenceFirstClassPlus1 =
            (u32)fence_class + 1u;
        gNdsRendererBattleTextureFenceFirstFrame =
            gNdsRendererProfileFrameCount;
    }
    gNdsRendererBattleTextureFenceCounts[(u32)fence_class]++;
}

#if NDS_VRAM_CENSUS_LIVE
/* P2-2p8 Phase 1 slice 2a (lab only): one tag per libnds texture name.
 * bits 0-4 site index (gNdsVramCensusSitePc; 31 = registry full), 5-7 the
 * fighter draw slot + 1 active at the upload, 8-12 sNdsRendererRuntimeOwner,
 * 14 = born through the fenced glGenTextures, 15 = an upload was seen. The site is the return address of the call into
 * the tagging helper, i.e. a PC inside the function the fenced wrapper was
 * inlined into -- the host maps it to a function with nm. Nothing here reads
 * or writes GX or VRAM. */
#define NDS_VRAM_CENSUS_NAME_MAX 512u
#define NDS_VRAM_CENSUS_SITE_MAX 30u
#define NDS_VRAM_CENSUS_SITE_FULL 31u
static u16 sNdsVramCensusTag[NDS_VRAM_CENSUS_NAME_MAX];
volatile u32 gNdsVramCensusSitePc[NDS_VRAM_CENSUS_SITE_MAX]
    __attribute__((used));
volatile u32 gNdsVramCensusSiteCount __attribute__((used));
volatile u32 gNdsVramCensusNameOverflow __attribute__((used));
volatile u32 gNdsVramCensusDrawSlotPlus1 __attribute__((used));
/* The last glTexImage2D request: type << 16 | size_t << 8 | size_s. */
volatile u32 gNdsVramCensusLastRequest __attribute__((used));
/* Set by the shared IFCommon atlas wrappers to their caller's PC, so an
 * atlas texture is tagged with the effect/stage/UI function that asked for
 * it rather than the one shared allocator (consumed by the next upload). */
volatile u32 gNdsVramCensusAtlasUser __attribute__((used));

static u32 ndsVramCensusSiteIndex(u32 pc)
{
    u32 i;

    for (i = 0u; i < gNdsVramCensusSiteCount; i++)
    {
        if (gNdsVramCensusSitePc[i] == pc)
        {
            return i;
        }
    }
    if (gNdsVramCensusSiteCount < NDS_VRAM_CENSUS_SITE_MAX)
    {
        gNdsVramCensusSitePc[gNdsVramCensusSiteCount] = pc;
        return gNdsVramCensusSiteCount++;
    }
    return NDS_VRAM_CENSUS_SITE_FULL;
}

static void __attribute__((noinline))
ndsVramCensusTagBirth(int count, const int *names)
{
    u32 site = ndsVramCensusSiteIndex(
        (u32)(uintptr_t)__builtin_return_address(0) & ~1u);
    int i;

    for (i = 0; i < count; i++)
    {
        u32 name = (u32)names[i];

        if (name < NDS_VRAM_CENSUS_NAME_MAX)
        {
            sNdsVramCensusTag[name] = (u16)(site | 0x4000u);
        }
        else
        {
            NDS_DIAG(gNdsVramCensusNameOverflow++);
        }
    }
}

static void __attribute__((noinline)) ndsVramCensusTagUpload(void)
{
    u32 user = gNdsVramCensusAtlasUser;
    u32 site = ndsVramCensusSiteIndex((user != 0u) ? user :
        ((u32)(uintptr_t)__builtin_return_address(0) & ~1u));
    u32 name = (u32)glGlobalData.activeTexture;

    if (name < NDS_VRAM_CENSUS_NAME_MAX)
    {
        sNdsVramCensusTag[name] = (u16)(site |
            ((gNdsVramCensusDrawSlotPlus1 & 7u) << 5) |
            (((u32)sNdsRendererRuntimeOwner & 31u) << 8) | 0xC000u);
        gNdsVramCensusAtlasUser = 0u;
    }
    else
    {
        NDS_DIAG(gNdsVramCensusNameOverflow++);
    }
}

static void ndsVramCensusTagDelete(int count, const int *names)
{
    int i;

    for (i = 0; i < count; i++)
    {
        u32 name = (u32)names[i];

        if (name < NDS_VRAM_CENSUS_NAME_MAX)
        {
            sNdsVramCensusTag[name] = 0u;
        }
    }
}

static void ndsVramCensusTagReset(void)
{
    memset(sNdsVramCensusTag, 0, sizeof(sNdsVramCensusTag));
}
#endif

/* DEFERRED PALETTE REWRITES (P2-2p8, 2026-10-05; KO pillar variants and
 * entry ramps). A palette is copied in from its staging words at the VBlank
 * that swaps the frame in, with the banks holding it (bit 0 E, 1 F, 2 G)
 * mapped to the CPU only for the copy. */
static u32 ndsRendererPaletteBanksFor(const void *lcd, u32 bytes)
{
    const uintptr_t first = (uintptr_t)lcd;
    const uintptr_t last = first + bytes - 1u;
    u32 banks = 0u;

    if ((first < (uintptr_t)VRAM_F) && (last >= (uintptr_t)VRAM_E))
    {
        banks |= 1u;
    }
    if ((first < (uintptr_t)VRAM_G) && (last >= (uintptr_t)VRAM_F))
    {
        banks |= 2u;
    }
    if ((first < (uintptr_t)VRAM_H) && (last >= (uintptr_t)VRAM_G))
    {
        banks |= 4u;
    }
    return banks;
}

/* Maps `banks` to LCD; returns the E/F/G control bytes it replaced. */
static u32 ndsRendererPaletteBanksToLcd(u32 banks)
{
    const u32 saved = (u32)VRAM_E_CR | ((u32)VRAM_F_CR << 8) |
                      ((u32)VRAM_G_CR << 16);

    if ((banks & 1u) != 0u)
    {
        VRAM_E_CR = VRAM_ENABLE;
    }
    if ((banks & 2u) != 0u)
    {
        VRAM_F_CR = VRAM_ENABLE;
    }
    if ((banks & 4u) != 0u)
    {
        VRAM_G_CR = VRAM_ENABLE;
    }
    return saved;
}

static void ndsRendererPaletteBanksRestore(u32 banks, u32 saved)
{
    if ((banks & 4u) != 0u)
    {
        VRAM_G_CR = (u8)(saved >> 16);
    }
    if ((banks & 2u) != 0u)
    {
        VRAM_F_CR = (u8)(saved >> 8);
    }
    if ((banks & 1u) != 0u)
    {
        VRAM_E_CR = (u8)saved;
    }
}

static inline int ndsRendererHardwareFencedGlGenTextures(int count,
                                                         int *names)
{
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_GL_CREATE);
#if NDS_VRAM_CENSUS_LIVE
    {
        int result = glGenTextures(count, names);

        if (result != 0)
        {
            ndsVramCensusTagBirth(count, names);
        }
        return result;
    }
#else
    return glGenTextures(count, names);
#endif
}

static inline int ndsRendererHardwareFencedGlTexImage2D(
    int target, int empty1, GL_TEXTURE_TYPE_ENUM type,
    int size_x, int size_y, int empty2, int params, const void *texture)
{
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_GL_UPLOAD);
    ndsRendererHardwareInvalidateGXState(
        NDS_RENDERER_GX_STATE_TEXTURE_PARAMS);
#if NDS_VRAM_CENSUS_LIVE
    {
        int result;

        gNdsVramCensusLastRequest = (((u32)type & 0xffu) << 16) |
            (((u32)size_y & 0xffu) << 8) | ((u32)size_x & 0xffu);
        result = glTexImage2D(target, empty1, type, size_x, size_y,
                              empty2, params, texture);

        if (result != 0)
        {
            ndsVramCensusTagUpload();
        }
        return result;
    }
#else
    return glTexImage2D(target, empty1, type, size_x, size_y, empty2,
                        params, texture);
#endif
}

static inline int ndsRendererHardwareFencedGlDeleteTextures(int count,
                                                            int *names)
{
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_GL_DELETE);
    ndsRendererHardwareInvalidateGXState(
        NDS_RENDERER_GX_STATE_TEXTURE_PARAMS);
#if NDS_VRAM_CENSUS_LIVE
    ndsVramCensusTagDelete(count, names);
#endif
    return glDeleteTextures(count, names);
}

static FILE *ndsRendererHardwareFencedTextureFopen(const char *path,
                                                   const char *mode)
{
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_FILE_IO);
    return fopen(path, mode);
}

static int ndsRendererHardwareFencedTextureFseek(FILE *file, long offset,
                                                 int origin)
{
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_FILE_IO);
    return fseek(file, offset, origin);
}

static long ndsRendererHardwareFencedTextureFtell(FILE *file)
{
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_FILE_IO);
    return ftell(file);
}

static size_t ndsRendererHardwareFencedTextureFread(
    void *destination, size_t size, size_t count, FILE *file)
{
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_FILE_IO);
    return fread(destination, size, count, file);
}

static int ndsRendererHardwareFencedTextureFclose(FILE *file)
{
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_FILE_IO);
    return fclose(file);
}


/* Profile 2 compares every phase-masked palette-pair result against the
 * existing exact blend formula before the pixels are submitted to GX. Keep
 * symbols available in profile 1 so one synchronized benchmark marker can be
 * used for both coarse timing and forensic runs. */
volatile u32 gNdsRendererProfileTexturePairOracleChecks;
volatile u32 gNdsRendererProfileTexturePairOracleMismatches;
volatile u32 gNdsRendererProfileTextureVBlankQueuedUploads;
volatile u32 gNdsRendererProfileTextureVBlankQueuedBytes;
volatile u32 gNdsRendererProfileTextureVBlankCommittedUploads;
volatile u32 gNdsRendererProfileTextureVBlankCommitTicks;
volatile u32 gNdsRendererProfileTextureVBlankOutsideCount;
volatile u32 gNdsRendererProfileTextureVBlankFallbackCount;
volatile u32 gNdsRendererProfileTextureVBlankStartLine;
volatile u32 gNdsRendererProfileTextureVBlankEndLine;

static inline void ndsRendererProfileRecordTextureBind(void)
{
    sNdsRendererRuntimeFrameSummary.texture_binds++;
}

/* `count` binds at once (the stage segment fast path's runs). */
static inline void ndsRendererProfileRecordTextureBinds(u32 count)
{
    sNdsRendererRuntimeFrameSummary.texture_binds += count;
}

static inline void ndsRendererProfileRecordTextureUpload(u32 bytes)
{
    sNdsRendererRuntimeFrameSummary.texture_uploads++;
    sNdsRendererRuntimeFrameSummary.texture_upload_bytes += bytes;
#if NDS_TICK_HUD
    /* R2-07: cumulative mirrors. The struct above is memset every frame, so a
     * ring stop can only ever read one frame of it. */
    gNdsMiscTexUploadCount++;
    gNdsMiscTexUploadBytes += bytes;
#endif
}

static inline void ndsRendererProfileRecordTextureCi4Direct(u32 pixels)
{
    sNdsRendererRuntimeTextureCi4DirectPixels += pixels;
}

static inline void ndsRendererProfileRecordCi4IndexCacheBuild(void)
{
    sNdsRendererRuntimeCi4IndexCacheBuildCount++;
}

static inline void ndsRendererProfileRecordCi4IndexCacheReuse(void)
{
    sNdsRendererRuntimeCi4IndexCacheReuseCount++;
}

static inline void ndsRendererProfileRecordCi4RepresentativeReuse(
    u32 representative_pixels, u32 reused_pixels)
{
    sNdsRendererRuntimeCi4RepresentativePixelCount += representative_pixels;
    sNdsRendererRuntimeCi4ReusePixelCount += reused_pixels;
}

static inline void ndsRendererProfileRecordTextureAliasAvoid(void)
{
    sNdsRendererRuntimeFrameSummary.texture_cache_alias_avoid_count++;
}

static inline void ndsRendererProfileRecordTextureEvict(void)
{
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_EVICT);
    sNdsRendererRuntimeTextureCacheEvictCount++;
}

static inline void ndsRendererProfileRecordTexel1Composite(void)
{
    sNdsRendererRuntimeFrameSummary.texel1_composite_count++;
    sNdsRendererRuntimeFrameSummary.texel1_load_match_count++;
}

static inline void ndsRendererProfileRecordTexel1Reject(void)
{
    sNdsRendererRuntimeFrameSummary.texel1_reject_count++;
}

static inline void ndsRendererProfileRecordTexel1RejectReason(u32 reason)
{
    (void)reason;
}

static inline void ndsRendererProfileRecordTexel1Refresh(void)
{
    sNdsRendererRuntimeTexel1FractionRefreshCount++;
}

/* The six per-call frame-summary recorders below (matrix loads, batch
 * begin/reuse/end, texture prepare/reuse) are diagnostics: each is a
 * read-modify-write on sNdsRendererRuntimeFrameSummary, published once a frame
 * into the gNdsRendererProfile* globals, which the P1 harness verifier
 * (verify-battle-mariofox-gcrunall-loop-harness.ps1) and
 * probe-task56-fighter-path.ps1 read. In the fighter path alone: matrix load
 * 564, texture prepare 583 + 92 reuse, batch begin 68 ticks a frame. The
 * behaviour-bearing counters (hardware_triangles, hardware_vertices,
 * hardware_over_limit) are separate. */
static inline void ndsRendererProfileRecordMatrixLoad(void)
{
    sNdsRendererRuntimeFrameSummary.matrix_load_count++;
}

static inline void ndsRendererProfileRecordBatchBegin(void)
{
    sNdsRendererRuntimeFrameSummary.hardware_batch_begin_count++;
}

static inline void ndsRendererProfileRecordBatchReuse(void)
{
    sNdsRendererRuntimeFrameSummary.hardware_batch_reuse_count++;
}

static inline void ndsRendererProfileRecordBatchEnd(void)
{
    sNdsRendererRuntimeFrameSummary.hardware_batch_end_count++;
}

static inline void ndsRendererProfileRecordTexturePrepare(void)
{
    sNdsRendererRuntimeFrameSummary.texture_prepare_count++;
}

static inline void ndsRendererProfileRecordTexturePrepareReuse(void)
{
    sNdsRendererRuntimeFrameSummary.texture_prepare_reuse_count++;
}

static inline void ndsRendererProfileRecordVertexSaturate(void)
{
    sNdsRendererRuntimeFrameSummary.hardware_vertex_saturate_count++;
}

static inline void ndsRendererProfileRecordNearPlaneTriangleReject(void)
{
    sNdsRendererRuntimeFrameSummary.near_plane_triangle_reject_count++;
}

static inline void ndsRendererProfileRecordProjectedSubmit(void)
{
    sNdsRendererRuntimeFrameSummary.projected_submit_fallback_count++;
}

static inline void ndsRendererProfileRecordRawCurrentCandidate(void)
{
    sNdsRendererRuntimeFrameSummary.raw_current_candidate_count++;
}

static inline void ndsRendererProfileRecordRawCurrentRangeReject(void)
{
    sNdsRendererRuntimeFrameSummary.raw_current_range_reject_count++;
}

static inline void ndsRendererProfileRecordRawCrossMatrix(void)
{
    sNdsRendererRuntimeFrameSummary.raw_cross_matrix_count++;
}

static inline void ndsRendererProfileRecordSubmitClass(
    NDSRendererHWSubmitClass submit_class)
{
    if ((u32)submit_class < NDS_RENDERER_HW_SUBMIT_CLASS_COUNT)
    {
        sNdsRendererHardwareSubmitClassCounts[submit_class]++;
    }
}

static inline void ndsRendererProfileRecordSourceVertexLoad(void)
{
    sNdsRendererHardwareSourceVertexLoadCount++;
}

static inline void ndsRendererProfileRecordCPUTransform(void)
{
    sNdsRendererHardwareCPUTransformCount++;
}

static inline void ndsRendererProfileRecordTransformCacheHit(void)
{
    sNdsRendererHardwareTransformCacheHitCount++;
}

static inline void ndsRendererProfileRecordMatrixSnapshotCreate(void)
{
    sNdsRendererHardwareMatrixSnapshotCreateCount++;
}

static inline void ndsRendererProfileRecordMatrixSnapshotReuse(void)
{
    sNdsRendererHardwareMatrixSnapshotReuseCount++;
}

static inline void ndsRendererProfileRecordMatrixSnapshotOverflow(void)
{
    sNdsRendererHardwareMatrixSnapshotOverflowCount++;
}

static void ndsRendererProfileResetSubmitSummary(void)
{
    memset(sNdsRendererHardwareSubmitClassCounts, 0,
           sizeof(sNdsRendererHardwareSubmitClassCounts));
    sNdsRendererHardwareDivideSummary = 0u;
    sNdsRendererHardwareSourceVertexLoadCount = 0u;
    sNdsRendererHardwareCPUTransformCount = 0u;
    sNdsRendererHardwareTransformCacheHitCount = 0u;
    sNdsRendererHardwareMatrixSnapshotCreateCount = 0u;
    sNdsRendererHardwareMatrixSnapshotReuseCount = 0u;
    sNdsRendererHardwareMatrixSnapshotOverflowCount = 0u;
}

static void ndsRendererProfilePublishSubmitSummary(void)
{
    gNdsRendererProfileLevel = NDS_RENDERER_PROFILE_LEVEL;
    gNdsRendererProfileSubmitRawCurrentCount =
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX];
    NDS_DIAG(gNdsRendererProfileSubmitRawSnapshotCount =
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_RAW_Z_SNAPSHOT_MATRIX]);
    gNdsRendererProfileSubmitProjectedCrossCount =
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_PROJECTED_CROSS_MATRIX];
    gNdsRendererProfileSubmitProjectedNoZCount =
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z];
    gNdsRendererProfileSubmitProjectedDecalCount =
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_PROJECTED_DECAL];
    NDS_DIAG(gNdsRendererProfileSubmitProjectedPrimDepthCount =
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_PROJECTED_PRIM_DEPTH]);
    gNdsRendererProfileSubmitProjectedRangeOrMatrixCount =
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX];
    gNdsRendererProfileSubmitRejectCount =
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_REJECT];
    NDS_DIAG(gNdsRendererProfileHardwareDivideSummary =
        sNdsRendererHardwareDivideSummary);
    gNdsRendererProfileSourceVertexLoadCount =
        sNdsRendererHardwareSourceVertexLoadCount;
    NDS_DIAG(gNdsRendererProfileCPUTransformCount =
        sNdsRendererHardwareCPUTransformCount);
    gNdsRendererProfileTransformCacheHitCount =
        sNdsRendererHardwareTransformCacheHitCount;
    NDS_DIAG(gNdsRendererProfileMatrixSnapshotCreateCount =
        sNdsRendererHardwareMatrixSnapshotCreateCount);
    gNdsRendererProfileMatrixSnapshotReuseCount =
        sNdsRendererHardwareMatrixSnapshotReuseCount;
    gNdsRendererProfileMatrixSnapshotOverflowCount =
        sNdsRendererHardwareMatrixSnapshotOverflowCount;
}

static inline void ndsRendererProfileRecordHardwareTriangle(void)
{
    sNdsRendererRuntimeFrameSummary.hardware_triangles++;
    sNdsRendererRuntimeFrameSummary.hardware_vertices += 3u;
    if ((sNdsRendererRuntimeFrameSummary.hardware_triangles > 2048u) ||
        (sNdsRendererRuntimeFrameSummary.hardware_vertices > 6144u))
    {
        sNdsRendererRuntimeFrameSummary.hardware_over_limit = 1u;
    }
}

typedef struct NDSRendererHardwareTextureKey
{
    u32 image;
    u32 image_format;
    u32 image_size;
    u32 image_width;
    u32 tlut_image;
    u32 tlut_count;
    u32 data_layout;
    u32 format;
    u32 size;
    u32 width;
    u32 height;
    u32 render_tile;
    u32 render_tmem;
    u32 render_palette;
    u32 render_tile_cms;
    u32 render_tile_cmt;
    u32 render_tile_masks;
    u32 render_tile_maskt;
    u32 render_tile_shifts;
    u32 render_tile_shiftt;
    u32 load_tile;
    u32 load_uls;
    u32 load_ult;
    u32 load_lrs;
    u32 load_dxt;
    u32 load_texels;
    u32 tile_uls;
    u32 tile_ult;
    u32 tile_lrs;
    u32 tile_lrt;
    u32 line;
    u32 flags;
    u32 texel1_image;
    u32 texel1_image_format;
    u32 texel1_image_size;
    u32 texel1_image_width;
    u32 texel1_load_kind;
    u32 texel1_render_tmem;
    u32 texel1_render_line;
    u32 texel1_render_palette;
    u32 texel1_render_tile_cms;
    u32 texel1_render_tile_cmt;
    u32 texel1_render_tile_masks;
    u32 texel1_render_tile_maskt;
    u32 texel1_render_tile_shifts;
    u32 texel1_render_tile_shiftt;
    u32 texel1_load_tile;
    u32 texel1_load_uls;
    u32 texel1_load_ult;
    u32 texel1_load_lrs;
    u32 texel1_load_dxt;
    u32 texel1_load_texels;
    u32 texel1_tile_uls;
    u32 texel1_tile_ult;
    u32 texel1_tile_lrs;
    u32 texel1_tile_lrt;
    u32 prim_lod_fraction;
    u32 combine_w0;
    u32 combine_w1;
} NDSRendererHardwareTextureKey;

/* NO RESIDENT KEY. A static slot's key lives in its generated ROM record plus
 * three resident pointer words; a dynamic slot keeps only its compact identity
 * (NDSRendererHardwareTextureIdent, slice 2c) -- see the
 * NDS_RENDERER_HW_TEXTURE_CACHE_COUNT note, and ndsRendererHardwareEntryKeyEqual
 * for the one place that has to know which. Read a pointer word or the texel1
 * fraction through ndsRendererHardwareEntryKeyWord; there is no `entry->key` to
 * reach for, and no copy of a dynamic slot's whole key anywhere.
 *
 * The remaining fields are packed rather than one-u32-each because at 254
 * slots every four bytes spent here is 1,016 bytes. Widths are the source's
 * own: owner_mask and the upload dimensions are u16 in
 * NDSBattlePlayableStaticTextureRecord, static_record_plus1 is bounded by
 * NDS_RENDERER_HW_TEXTURE_STATIC_COUNT in
 * ndsRendererHardwareRecordBattleStaticTextureHit, and ready/pinned are
 * booleans. The texel census and the upload dimensions went with the profile-2
 * texture profiler, their only reader (slice 2c). */
typedef struct NDSRendererHardwareTextureCacheEntry
{
    int name;
    u32 params;
    u32 last_used_frame;
    u32 key_generation;
    u32 key_hash;
    u16 static_owner_mask;
    /* P2-2p8 Phase 1 slice 2c: a CARVED entry -- uploaded by the fighter
     * admission into VRAM the admission reserved itself, named
     * NDS_FTR_CARVE_NAME_BASE | slot -- has no libnds record: its bind writes
     * `params` and, with NDS_FTR_CARVE_F_PALETTE, this PLTT_BASE word. */
    u16 carve_pltt;
    u8 ready;
    u8 pinned;
    u8 static_record_plus1;
    /* Uploaded by a native stage owner's first prepare after a blob load
     * (the blob stage's pin set, realised at load time). Eviction skips it
     * for the scene, so a per-frame dynamic upload (a scrolling texture)
     * cannot push the stage's own textures out under its prepared runs. */
    u8 stage_warm;
    /* P2-2p8 Phase 1 slice 2b: an admitted fighter texture (uploaded by the
     * creation-time admission, gNdsFtrLeanAdmit). Exempt from eviction and
     * from being refreshed/repurposed for the rest of the battle, like
     * stage_warm, without the static-pin semantics `pinned` carries.
     * Cleared by release (the entry is zeroed) and at battle exit. */
    u8 admitted;
    u8 carve_flags;
    u8 carve_region;   /* the carve texture region holding it */
} NDSRendererHardwareTextureCacheEntry;


/* Slice 2c: a dynamic slot's identity. id_a/id_b are two independent 32-bit
 * multiply chains over key words 1..58 -- every word but the image -- and the
 * image, TLUT and texel1 pointers are kept exactly. The lookup
 * (ndsRendererHardwareEntryKeyEqual) requires all five; the stage
 * source-frame search, which asks for "this key with another image",
 * requires all but the image. refresh is the texel1 refresh class (data
 * layout, format, size, dimensions, combine: Texel1RefreshCompatible's seven
 * words) and prim_lod_fraction the one other word the stage site plan reads. */
typedef struct NDSRendererHardwareTextureIdent
{
    u32 id_a;
    u32 id_b;
    u32 image;
    u32 tlut_image;
    u32 texel1_image;
    u32 refresh;
    u32 prim_lod_fraction;
} NDSRendererHardwareTextureIdent;

typedef struct NDSRendererHardwareResolvedTexture
{
    NDSRendererHardwareTextureCacheEntry *entry;
    u32 name;
    u32 params;
    u32 format;
    u32 width;
    u32 height;
} NDSRendererHardwareResolvedTexture;

typedef struct NDSRendererHardwareTexel1Source
{
    const NDSRendererTextureLoadState *load;
    const NDSRendererTileState *render_tile;
    const u8 *texels;
    u32 format;
    u32 size;
    u32 width;
    u32 height;
    u32 source_width;
    u32 source_extent_width;
    u32 source_extent_height;
    u32 source_texels;
    u32 source_origin_s;
    u32 source_origin_t;
    u32 palette_base;
    s32 materialize_s;
    s32 materialize_t;
} NDSRendererHardwareTexel1Source;

typedef struct NDSRendererHardwareLightDirection
{
    s32 x;
    s32 y;
    s32 z;
} NDSRendererHardwareLightDirection;

typedef struct NDSRendererHardwareCi4IndexCacheEntry
{
    const u8 *source;
    u32 source_texels;
    u32 byte_lane_xor;
    u32 valid;
    u8 indices[NDS_RENDERER_HW_CI4_INDEX_CACHE_TEXELS];
} NDSRendererHardwareCi4IndexCacheEntry;

typedef struct NDSRendererHardwareLightShadeCacheEntry
{
    u32 valid;
    u32 diffuse;
    u32 ambient;
    u32 rgb[NDS_RENDERER_HW_LIGHT_SHADE_LUT_COUNT];
} NDSRendererHardwareLightShadeCacheEntry;

static NDSRendererHardwareTextureCacheEntry
    sNdsRendererHardwareTextureCache[NDS_RENDERER_HW_TEXTURE_CACHE_COUNT];
/* Dynamic slot i owns identity i - STATIC_COUNT. The partition IS the pool
 * index, which is why no entry carries one. */
static NDSRendererHardwareTextureIdent
    sNdsRendererHardwareTextureIdentPool[NDS_RENDERER_HW_TEXTURE_DYNAMIC_COUNT];
#if NDS_TICK_HUD
/* P2-2 capacity witnesses. These are request/use high-waters rather than
 * inferred dimensions so the stress ROM can prove the real live maxima before
 * any of these reservations are reduced. Keep them beside the buffers they
 * witness; the four-fighter verifier publishes them in its memory artifact. */
volatile u32 gNdsRendererTextureKeyPoolEntriesHighWater;

static u32 sNdsRendererHardwareTextureKeyGeneration; /* defined below */

static void ndsRendererRecordTextureKeyPoolUse(void)
{
    /* A whole-cache scan every frame (~600 entries, ~5.5K ticks inside
     * WORK-H) for a high-water that only moves when textures are taken. Scan
     * when the texture key generation moved since the last scan -- uploads
     * and evictions bump it -- and every 16th frame regardless, so a take
     * that does not bump it is still seen within 16 frames; entries stay
     * resident far longer than that. (A9, 2026-09-27.) */
    static u32 s_last_generation;
    static u32 s_frames;
    u32 i;
    u32 entries = 0u;

    if ((sNdsRendererHardwareTextureKeyGeneration == s_last_generation) &&
        ((++s_frames & 15u) != 0u))
    {
        return;
    }
    s_last_generation = sNdsRendererHardwareTextureKeyGeneration;
    for (i = NDS_RENDERER_HW_TEXTURE_STATIC_COUNT;
         i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        const NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[i];

        if ((entry->ready != 0u) || (entry->name != 0))
        {
            entries++;
        }
    }
    if (entries > gNdsRendererTextureKeyPoolEntriesHighWater)
    {
        gNdsRendererTextureKeyPoolEntriesHighWater = entries;
    }
}
#else
#define ndsRendererRecordTextureKeyPoolUse() ((void)0)
#endif
/* The three words a generated record cannot supply, because ROM stores asset
 * OFFSETS and a live key holds loaded ADDRESSES. Indexed by static slot, which
 * is the record index. Order is image, tlut, texel1. */
static u32 sNdsRendererHardwareStaticKeyPointers[
    NDS_RENDERER_HW_TEXTURE_STATIC_COUNT][3];
static u8 sNdsRendererHardwareTextureLookup[
    NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT];
#if defined(__arm__)
_Static_assert(sizeof(NDSRendererHardwareTextureKey) == 236u,
               "texture key layout must remain exact-match stable");
_Static_assert(
    (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT &
     (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u)) == 0u,
    "texture lookup count must remain a power of two");
_Static_assert(NDS_RENDERER_HW_TEXTURE_CACHE_COUNT <
                   NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT,
               "texture lookup must retain an empty cluster terminator");
/* The lookup stores slot + 1 in a u8, so 254 slots is its hard ceiling. */
_Static_assert(NDS_RENDERER_HW_TEXTURE_CACHE_COUNT <= 254u,
               "texture lookup stores slot+1 in a u8");
/* Slice 2c: 254 slots (45 static + 209 dynamic) at 32 B of entry and 28 B of
 * identity, the 45 static pointer triples and the 512-byte lookup cost
 * 15,032 B -- under the 24,768 B the 124-slot 2b cache cost with its 236-byte
 * pool keys. The ceiling stays the old budget: the slot count is bounded by
 * the lookup's u8 above, not by this. */
_Static_assert(sizeof(NDSRendererHardwareTextureCacheEntry) == 32u,
               "texture cache entry must stay 32 bytes");
_Static_assert(sizeof(NDSRendererHardwareTextureIdent) == 28u,
               "texture identity must stay 28 bytes");
_Static_assert(sizeof(sNdsRendererHardwareTextureCache) +
                       sizeof(sNdsRendererHardwareTextureIdentPool) +
                       sizeof(sNdsRendererHardwareStaticKeyPointers) +
                       sizeof(sNdsRendererHardwareTextureLookup) <=
                   24768u,
               "texture cache storage must stay within the measured budget");
#endif
#if defined(__arm__)
/* A static slot's key is its record's key_words with three words replaced, so
 * these four have to agree or the reconstruction silently compares the wrong
 * bytes. */
_Static_assert(sizeof(NDSRendererHardwareTextureKey) ==
                   NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_KEY_WORD_COUNT *
                       sizeof(u32),
               "texture key and generated record must hold the same words");
_Static_assert(__builtin_offsetof(NDSRendererHardwareTextureKey, image) ==
                   NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_IMAGE_WORD * sizeof(u32),
               "image word index disagrees with the generated record");
_Static_assert(__builtin_offsetof(NDSRendererHardwareTextureKey, tlut_image) ==
                   NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TLUT_WORD * sizeof(u32),
               "tlut word index disagrees with the generated record");
_Static_assert(__builtin_offsetof(NDSRendererHardwareTextureKey,
                                  texel1_image) ==
                   NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TEXEL1_WORD * sizeof(u32),
               "texel1 word index disagrees with the generated record");
#endif

#define NDS_RENDERER_HW_TEXTURE_KEY_WORD(field) \
    ((u32)(__builtin_offsetof(NDSRendererHardwareTextureKey, field) / \
           sizeof(u32)))

static u32 ndsRendererHardwareEntrySlot(
    const NDSRendererHardwareTextureCacheEntry *entry)
{
    return (u32)(entry - sNdsRendererHardwareTextureCache);
}

/* The identity of a dynamic slot; NULL for a static one. */
static NDSRendererHardwareTextureIdent *ndsRendererHardwareEntryIdent(
    const NDSRendererHardwareTextureCacheEntry *entry)
{
    u32 slot;

    if (entry == NULL)
    {
        return NULL;
    }
    slot = ndsRendererHardwareEntrySlot(entry);
    if ((slot < NDS_RENDERER_HW_TEXTURE_STATIC_COUNT) ||
        (slot >= NDS_RENDERER_HW_TEXTURE_CACHE_COUNT))
    {
        return NULL;
    }
    return &sNdsRendererHardwareTextureIdentPool[
        slot - NDS_RENDERER_HW_TEXTURE_STATIC_COUNT];
}

/* The generated record backing a static slot; NULL for a dynamic or empty one. */
static const NDSBattlePlayableStaticTextureRecord *
ndsRendererHardwareEntryStaticRecord(
    const NDSRendererHardwareTextureCacheEntry *entry)
{
    if ((entry == NULL) || (entry->static_record_plus1 == 0u) ||
        (ndsRendererHardwareEntrySlot(entry) >=
         NDS_RENDERER_HW_TEXTURE_STATIC_COUNT))
    {
        return NULL;
    }
    return ndsBattlePlayableStaticTextureRecordAt(
        (u32)entry->static_record_plus1 - 1u);
}

/* THE IDENTITY. Two multiply chains with different odd multipliers over key
 * words 1..58 -- the image (word 0) is compared exactly beside it, and left
 * out so the stage source-frame search can ask for "this key, any image" --
 * each finished with an avalanche. A one-word difference can never collide
 * (an odd multiplier is invertible mod 2^32); the multi-word case is what the
 * lab shadow and the admission census check. Word for word as cheap as the
 * 236-byte memcmp the lookup used to confirm a hit with, which is the only
 * place it runs: after the fingerprint (key_hash) already matched. */
static void ndsRendererHardwareTextureIdentOf(
    const NDSRendererHardwareTextureKey *key, u32 *out_a, u32 *out_b)
{
    const u32 *w = (const u32 *)(const void *)key;
    u32 a = 0x811c9dc5u;
    u32 b = 0x2545f491u;
    u32 i;

    for (i = 1u; i < 57u; i += 2u)
    {
        u32 v0 = w[i];
        u32 v1 = w[i + 1u];

        a = (a * 0x01000193u) + v0;
        b = (b * 0x5bd1e995u) + v0;
        a = (a * 0x01000193u) + v1;
        b = (b * 0x5bd1e995u) + v1;
    }
    a = (a * 0x01000193u) + w[57];
    b = (b * 0x5bd1e995u) + w[57];
    a = (a * 0x01000193u) + w[58];
    b = (b * 0x5bd1e995u) + w[58];
    a ^= a >> 16;
    a *= 0x7feb352du;
    a ^= a >> 15;
    a *= 0x846ca68bu;
    a ^= a >> 16;
    b ^= b >> 15;
    b *= 0x2c1b3c6du;
    b ^= b >> 12;
    b *= 0x297a2d39u;
    b ^= b >> 15;
    *out_a = a;
    *out_b = b;
}

/* ndsRendererHardwareTexel1RefreshCompatible's seven words, as one class. */
static u32 ndsRendererHardwareTextureRefreshClass(
    const NDSRendererHardwareTextureKey *key)
{
    u32 h = 0x9e3779b9u;

    h = (h * 0x01000193u) + key->data_layout;
    h = (h * 0x01000193u) + key->format;
    h = (h * 0x01000193u) + key->size;
    h = (h * 0x01000193u) + key->width;
    h = (h * 0x01000193u) + key->height;
    h = (h * 0x01000193u) + key->combine_w0;
    h = (h * 0x01000193u) + key->combine_w1;
    h ^= h >> 16;
    h *= 0x7feb352du;
    h ^= h >> 15;
    return h;
}



static u32 ndsRendererHardwareEntryKeyWord(
    const NDSRendererHardwareTextureCacheEntry *entry, u32 word)
{
    const NDSRendererHardwareTextureIdent *ident;
    const NDSBattlePlayableStaticTextureRecord *record;

    if (word >= NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_KEY_WORD_COUNT)
    {
        return 0u;
    }
    ident = ndsRendererHardwareEntryIdent(entry);
    if (ident != NULL)
    {
        /* The words a dynamic slot keeps. Every reader asks for one of
         * these; anything else is only in the lab's shadow. */
        switch (word)
        {
        case NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_IMAGE_WORD:
            return ident->image;
        case NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TLUT_WORD:
            return ident->tlut_image;
        case NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TEXEL1_WORD:
            return ident->texel1_image;
        case NDS_RENDERER_HW_TEXTURE_KEY_WORD(prim_lod_fraction):
            return ident->prim_lod_fraction;
        default:
            break;
        }
        return 0u;
    }
    record = ndsRendererHardwareEntryStaticRecord(entry);
    if (record == NULL)
    {
        return 0u;
    }
    switch (word)
    {
    case NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_IMAGE_WORD:
        return sNdsRendererHardwareStaticKeyPointers[
            ndsRendererHardwareEntrySlot(entry)][0];
    case NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TLUT_WORD:
        return sNdsRendererHardwareStaticKeyPointers[
            ndsRendererHardwareEntrySlot(entry)][1];
    case NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TEXEL1_WORD:
        return sNdsRendererHardwareStaticKeyPointers[
            ndsRendererHardwareEntrySlot(entry)][2];
    default:
        break;
    }
    return record->key_words[word];
}

/* A static slot's whole key: its record's words with the three runtime
 * pointers. Zero for a dynamic or empty slot. */
static void ndsRendererHardwareEntryCopyStaticKey(
    const NDSRendererHardwareTextureCacheEntry *entry,
    NDSRendererHardwareTextureKey *out)
{
    const NDSBattlePlayableStaticTextureRecord *record =
        ndsRendererHardwareEntryStaticRecord(entry);
    const u32 *pointers;

    if (record == NULL)
    {
        memset(out, 0, sizeof(*out));
        return;
    }
    memcpy(out, record->key_words, sizeof(*out));
    pointers =
        sNdsRendererHardwareStaticKeyPointers[
            ndsRendererHardwareEntrySlot(entry)];
    out->image = pointers[0];
    out->tlut_image = pointers[1];
    out->texel1_image = pointers[2];
}


/* THE ONE PLACE THAT KNOWS A SLOT MAY NOT OWN ITS KEY.
 *
 * Dynamic slots compare their identity: the three pointer words exactly, then
 * both multiply chains of the query (slice 2c; the lab re-checks every hit
 * against its exact shadow). Static slots compare the three runtime pointer
 * words against RAM and the other 56 against ROM -- which is exact, not an
 * approximation, because the prepare built the key by memcpy-ing key_words and
 * then overwriting precisely those three. Word 0, word 4 and word 32 leave
 * three contiguous spans. */
static s32 ndsRendererHardwareEntryKeyEqual(
    const NDSRendererHardwareTextureCacheEntry *entry,
    const NDSRendererHardwareTextureKey *key)
{
    const NDSRendererHardwareTextureIdent *ident;
    const NDSBattlePlayableStaticTextureRecord *record;
    const u32 *pointers;
    const u32 *words;

    if ((entry == NULL) || (key == NULL))
    {
        return FALSE;
    }
    ident = ndsRendererHardwareEntryIdent(entry);
    if (ident != NULL)
    {
        u32 id_a;
        u32 id_b;

        if ((ident->image != key->image) ||
            (ident->tlut_image != key->tlut_image) ||
            (ident->texel1_image != key->texel1_image))
        {
            return FALSE;
        }
        ndsRendererHardwareTextureIdentOf(key, &id_a, &id_b);
        if ((ident->id_a != id_a) || (ident->id_b != id_b))
        {
            return FALSE;
        }
        return TRUE;
    }
    record = ndsRendererHardwareEntryStaticRecord(entry);
    if (record == NULL)
    {
        return FALSE;
    }
    pointers =
        sNdsRendererHardwareStaticKeyPointers[
            ndsRendererHardwareEntrySlot(entry)];
    words = (const u32 *)key;
    if ((words[NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_IMAGE_WORD] !=
         pointers[0]) ||
        (words[NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TLUT_WORD] !=
         pointers[1]) ||
        (words[NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TEXEL1_WORD] !=
         pointers[2]))
    {
        return FALSE;
    }
    return ((memcmp(&record->key_words[1], &words[1],
                    3u * sizeof(u32)) == 0) &&
            (memcmp(&record->key_words[5], &words[5],
                    27u * sizeof(u32)) == 0) &&
            (memcmp(&record->key_words[33], &words[33],
                    26u * sizeof(u32)) == 0)) ? TRUE : FALSE;
}

static void ndsRendererHardwareEntrySetKey(
    NDSRendererHardwareTextureCacheEntry *entry,
    const NDSRendererHardwareTextureKey *key)
{
    NDSRendererHardwareTextureIdent *ident =
        ndsRendererHardwareEntryIdent(entry);

    if ((ident == NULL) || (key == NULL))
    {
        return;
    }
    ndsRendererHardwareTextureIdentOf(key, &ident->id_a, &ident->id_b);
    ident->image = key->image;
    ident->tlut_image = key->tlut_image;
    ident->texel1_image = key->texel1_image;
    ident->refresh = ndsRendererHardwareTextureRefreshClass(key);
    ident->prim_lod_fraction = key->prim_lod_fraction;
}

/* A static slot keeps only the three pointer words; the rest is its record. */
static void ndsRendererHardwareEntrySetStaticKey(
    NDSRendererHardwareTextureCacheEntry *entry,
    const NDSRendererHardwareTextureKey *key)
{
    u32 slot = ndsRendererHardwareEntrySlot(entry);
    u32 *pointers;

    if ((key == NULL) || (slot >= NDS_RENDERER_HW_TEXTURE_STATIC_COUNT))
    {
        return;
    }
    pointers = sNdsRendererHardwareStaticKeyPointers[slot];
    pointers[0] = key->image;
    pointers[1] = key->tlut_image;
    pointers[2] = key->texel1_image;
}

static void ndsRendererHardwareEntryClearKey(
    NDSRendererHardwareTextureCacheEntry *entry)
{
    NDSRendererHardwareTextureIdent *ident =
        ndsRendererHardwareEntryIdent(entry);
    u32 slot;

    if (ident != NULL)
    {
        memset(ident, 0, sizeof(*ident));
        return;
    }
    slot = ndsRendererHardwareEntrySlot(entry);
    if (slot < NDS_RENDERER_HW_TEXTURE_STATIC_COUNT)
    {
        memset(sNdsRendererHardwareStaticKeyPointers[slot], 0,
               sizeof(sNdsRendererHardwareStaticKeyPointers[slot]));
    }
}

static u32 sNdsRendererHardwareTextureCacheNext;
static u32 sNdsRendererHardwareTextureKeyGeneration;
static u32 sNdsRendererHardwareFrameSerial;
static const NDSRendererHardwareTextureCacheEntry
    *sNdsRendererHardwareActiveTextureEntry;

/* Declared here rather than beside the atlas prepare because the static-texture
 * ownership guard below runs long before it and has to recognise the atlas. */
/* Each sheet owns one dedicated libnds texture name for the life of the battle.
 * These names no longer consume the general material cache: that cache is the
 * measured 124-entry table above, while particle/Whispy/Fox native textures
 * are deliberately separate owners. Keep an explicit shipping-set cap so an
 * atlas-coverage change cannot grow the dedicated set unnoticed. */
_Static_assert(NDS_PARTICLE_QUAD_BASE_SHEETS <= 9u,
               "particle atlas sheets would crowd the texture cache");
/* P8 loop V: the animation sheets after the base ones upload only after the
 * countdown has released its atlases, one a frame, into the 57,344 bytes it
 * frees -- seven sheets, so the texture cache keeps the room it had before
 * (the generator's QUAD_ANIMATION_SHEETS_MAX; nds_renderer_particles.c). */
_Static_assert((NDS_PARTICLE_QUAD_ATLAS_SHEETS >=
                NDS_PARTICLE_QUAD_BASE_SHEETS) &&
                   (NDS_PARTICLE_QUAD_ATLAS_SHEETS <=
                    NDS_PARTICLE_QUAD_BASE_SHEETS + 7u),
               "particle animation sheets past the countdown's 57,344 bytes");
/* Main RAM, not the DTCM hot list: one word a sheet, and the sheet count
 * grows (P8 loop V) while the gate ROM's DTCM sits at its 12,288-byte
 * ceiling (linker/nds_hot_text.ld); the ninth sheet's word took it over. */
static int sNdsRendererParticleAtlasName[NDS_PARTICLE_QUAD_ATLAS_SHEETS];
/* 9 -> 10 on 2026-10-04: a sixth quad sheet seats the item flame and smoke
 * at source size. 10 -> 11 the same day: a seventh seats Yoshi's egg
 * explosion and Kirby's inhale wind, their fighter banks' first cells.
 * 11 -> 12 on 2026-10-22: an eighth seats the item explosion's ten frames
 * (texture 29 at 24x24; it held frame 0 for the whole effect). 12 -> 13
 * the same day: a ninth seats SparkleWhite's nine (texture 45, 24x24). */
_Static_assert(NDS_PARTICLE_QUAD_BASE_SHEETS +
                   NDS_WHISPY_NATIVE_TEXTURE_COUNT +
                   NDS_R2_FOX_BLASTER_GLOW_AOT <= 13u,
               "particle native textures would crowd the texture cache");
static int sNdsRendererWhispyNativeName[NDS_WHISPY_NATIVE_TEXTURE_COUNT];
static int sNdsRendererFoxBlasterGlowName;
/* glBindTexture looks each GL name up through libnds's DynamicArray before it
 * writes TEXIMAGE_PARAM and PLTT_BASE.  These three textures are immutable and
 * pinned for the whole scene, so capture the exact two hardware words and the
 * matching libnds active-name state once at upload.  The route-3 lab arm can
 * then perform the identical register transition without a per-particle name
 * lookup; route 4 uses the same words in a packed GXFIFO stream. */
typedef struct NDSRendererWhispyNativeBinding
{
    u32 texture_name;
    u32 texture_format;
    u32 palette_format;
    s32 palette_name;
    u32 valid;
} NDSRendererWhispyNativeBinding;

static NDSRendererWhispyNativeBinding
    sNdsRendererWhispyNativeBinding[
        NDS_WHISPY_NATIVE_TEXTURE_COUNT + NDS_R2_FOX_BLASTER_GLOW_AOT];
/* The common particle atlas is just as immutable as Whispy's texels.  Keep
 * the exact TEXIMAGE_PARAM / PLTT_BASE pair for each sheet so the Phase-2
 * particle packet can switch sheets without calling glBindTexture from the
 * hot path.  Palette variants override only palette_format at submission. */
static NDSRendererWhispyNativeBinding
    sNdsRendererParticleAtlasBinding[NDS_PARTICLE_QUAD_ATLAS_SHEETS];
static u32 sNdsRendererParticleAtlasPrepared;
/* THE WHOLE PALETTE BLOCK, NOT ONE TABLE: NDS_PARTICLE_QUAD_ATLAS_SHEETS
 * tables of NDS_PARTICLE_QUAD_PALETTE_ENTRIES laid end to end, which is how the
 * generator writes it. Each sheet gets its own colours; see the note at the
 * glColorTableEXT call in ndsRendererHardwarePrepareParticleAtlas.
 *
 * ndsRendererHardwarePrepareWhispyNativeTextures borrows this buffer as palette
 * scratch, which stays safe because it runs after all four sheets are uploaded
 * and its own guard still bounds it by NDS_PARTICLE_QUAD_PALETTE_ENTRIES. */
static u16 sNdsRendererParticleAtlasPalette[
    NDS_PARTICLE_QUAD_PALETTE_BYTES / sizeof(u16)];
/* The upload loop indexes sheet * NDS_PARTICLE_QUAD_PALETTE_ENTRIES and the
 * read above takes sizeof(), so a generator that ever emitted a different
 * number of tables than sheets would walk off the end silently rather than
 * fail. Impossible to see on screen; free to catch here. */
_Static_assert(NDS_PARTICLE_QUAD_PALETTE_BYTES ==
                   NDS_PARTICLE_QUAD_ATLAS_SHEETS *
                       NDS_PARTICLE_QUAD_PALETTE_STRIDE_BYTES,
               "particle atlas palette block must hold one table per sheet");
_Static_assert(NDS_PARTICLE_QUAD_PALETTE_STRIDE_BYTES ==
                   NDS_PARTICLE_QUAD_PALETTE_ENTRIES * sizeof(u16),
               "particle atlas palette stride must be its entry count");

static s32 ndsRendererParticleAtlasOwnsName(int name)
{
    u32 sheet;

    for (sheet = 0u; sheet < NDS_PARTICLE_QUAD_ATLAS_SHEETS; sheet++)
    {
        if (sNdsRendererParticleAtlasName[sheet] == name)
        {
            return TRUE;
        }
    }
    for (sheet = 0u; sheet < NDS_WHISPY_NATIVE_TEXTURE_COUNT; sheet++)
    {
        if (sNdsRendererWhispyNativeName[sheet] == name)
        {
            return TRUE;
        }
    }
    if (sNdsRendererFoxBlasterGlowName == name)
    {
        return TRUE;
    }
    return FALSE;
}

static void ndsRendererHardwareRecordBattleStaticTextureHit(
    const NDSRendererHardwareTextureCacheEntry *entry)
{
    u32 record_index;

    if ((sNdsRendererBattleStaticTextureArmed == 0u) ||
        (entry == NULL))
    {
        return;
    }
    /* The particle atlas is a legitimate second owner of battle texture VRAM,
     * not a leak. This guard predates it and asks "is every texture bound
     * during an armed battle a pinned STATIC RECORD" -- true while the static
     * set was the only pinned resident, and false the moment the particle
     * draw binds its atlas, which reported ViolationCount 1 for a texture that
     * is exactly where it should be. Exempted by identity rather than by
     * loosening the test, so a genuinely unowned binding still trips it.
     *
     * The ViolationCount 1 / stage-rebuilds-197 pair this was first aimed at
     * was NOT an ownership question -- it was texture VRAM, and this exemption
     * could never have fixed it. Attributed 2026-08-01 by three tick-HUD ROMs
     * differing only in the particle flags: control and RUNTIME=1 both report
     * 0 and 2, DRAW=1 reports 1 and 197 and aborts at the GO countdown. The
     * prepare ORDER in ndsBattlePrepareSceneTextures is where that is fixed.
     * This stays because it is independently correct: the atlas is a pinned
     * resident with no static record, which is exactly what the test asks
     * about. */
    if ((entry->pinned != 0u) && (entry->static_record_plus1 == 0u) &&
        (sNdsRendererParticleAtlasPrepared != 0u) &&
        (ndsRendererParticleAtlasOwnsName(entry->name) != FALSE))
    {
        return;
    }
    if ((entry->pinned == 0u) || (entry->static_record_plus1 == 0u) ||
        (entry->static_record_plus1 > NDS_RENDERER_HW_TEXTURE_STATIC_COUNT))
    {
        gNdsRendererBattleStaticTextureViolationCount++;
        return;
    }
    record_index = entry->static_record_plus1 - 1u;
    NDS_DIAG(gNdsRendererBattleStaticTexturePinnedHitCount++);
    if (record_index < 32u)
    {
        NDS_DIAG(gNdsRendererBattleStaticTextureSeenMask |= 1u << record_index);
    }
    else
    {
        NDS_DIAG(gNdsRendererBattleStaticTextureSeenMaskHi |=
            1u << (record_index - 32u));
    }
    NDS_DIAG(gNdsRendererBattleStaticTextureOwnerMask |= entry->static_owner_mask);
}
static u16 sNdsRendererHardwareTextureScratch[
    NDS_RENDERER_HW_TEXTURE_MAX_TEXELS];
#if NDS_TICK_HUD
volatile u32 gNdsRendererTextureScratchFillBytesHighWater;
volatile u32 gNdsRendererTextureScratchStaticPayloadBytesHighWater;
volatile u32 gNdsRendererTextureScratchWhispyBytesHighWater;
volatile u32 gNdsRendererTextureScratchFoxGlowBytesHighWater;
volatile u32 gNdsRendererTextureScratchParticleAtlasBytesHighWater;
volatile u32 gNdsRendererTextureScratchDamageSlashBytesHighWater;
volatile u32 gNdsRendererTextureScratchDynamicUploadBytesHighWater;
volatile u32 gNdsRendererTextureRefreshCompactRequestBytesHighWater;
#define NDS_RENDERER_CAPACITY_HIGH_WATER(name, value) \
    do \
    { \
        u32 nds_capacity_value__ = (u32)(value); \
        if (nds_capacity_value__ > (name)) \
        { \
            (name) = nds_capacity_value__; \
        } \
    } while (0)
#else
#define NDS_RENDERER_CAPACITY_HIGH_WATER(name, value) ((void)0)
#endif
/* Sixteen entries is the whole of a GL_RGB16 palette; the static corpus never
 * needs more (generate_battle_playable_static_textures.py falls back to direct
 * colour above that). */
/* The whole palette block, read once per prepare. Per-record reads cost 22
 * extra NitroFS round trips inside the longest pause in the game. */
static u8 sNdsRendererStaticTexturePaletteBlock[
    NDS_BATTLE_STATIC_TEXTURE_PALETTE_BLOCK_MAX_BYTES];
static u16 sNdsRendererStaticTexturePalette[256];

/* Bytes the record's payload span occupies, which is its ENCODING's business
 * and not upload_width x upload_height x 2 any more. 0 means "not a format this
 * uploader knows", which every caller treats as a rejection. */
static u32 ndsRendererStaticTextureSpanBytes(
    const NDSBattlePlayableStaticTextureRecord *record)
{
    u32 texels;

    if (record == NULL)
    {
        return 0u;
    }
    texels = (u32)record->upload_width * (u32)record->upload_height;
    if (record->ds_format == NDS_BATTLE_STATIC_TEXTURE_FORMAT_PAL16)
    {
        return (texels + 1u) >> 1;
    }
    if (record->ds_format == NDS_BATTLE_STATIC_TEXTURE_FORMAT_PAL256)
    {
        return texels;
    }
    if (record->ds_format == NDS_BATTLE_STATIC_TEXTURE_FORMAT_RGBA)
    {
        return texels * sizeof(u16);
    }
    return 0u;
}
#define NDS_RENDERER_HW_TEXTURE_REFRESH_QUEUE_COUNT 2u
#define NDS_RENDERER_HW_TEXTURE_REFRESH_SMALL_TEXELS 2048u
#define NDS_RENDERER_HW_TEXTURE_REFRESH_LARGE_ROWS 64u
typedef struct NDSRendererHardwareTextureRefresh
{
    NDSRendererHardwareTextureCacheEntry *entry;
    const u16 *pixels;
    u32 staged_bytes;
    u32 texture_bytes;
    u32 row_bytes;
    u32 row_count;
    u8 row_map[NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT];
} NDSRendererHardwareTextureRefresh;
static NDSRendererHardwareTextureRefresh
    sNdsRendererHardwareTextureRefreshQueue[
        NDS_RENDERER_HW_TEXTURE_REFRESH_QUEUE_COUNT];
static u32 sNdsRendererHardwareTextureRefreshCount;
static u16 sNdsRendererHardwareTextureRefreshSmall[
    NDS_RENDERER_HW_TEXTURE_REFRESH_SMALL_TEXELS];
static u16 sNdsRendererHardwareTextureRefreshLarge[
    NDS_RENDERER_HW_TEXTURE_MAX_WIDTH *
    NDS_RENDERER_HW_TEXTURE_REFRESH_LARGE_ROWS];
#if NDS_TICK_HUD
volatile u32 gNdsRendererTextureRefreshSmallBytesHighWater;
volatile u32 gNdsRendererTextureRefreshLargeBytesHighWater;
#endif
static u8 sNdsRendererHardwareTexel01Ci4Source0S[
    NDS_RENDERER_HW_TEXTURE_MAX_WIDTH];
static u8 sNdsRendererHardwareTexel01Ci4Source1S[
    NDS_RENDERER_HW_TEXTURE_MAX_WIDTH];
static u8 sNdsRendererHardwareTexel01Ci4Source0T[
    NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT];
static u8 sNdsRendererHardwareTexel01Ci4Source1T[
    NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT];
static u8 sNdsRendererHardwareTexel01Ci4RepresentativeS[
    NDS_RENDERER_HW_TEXTURE_MAX_WIDTH];
static u8 sNdsRendererHardwareTexel01Ci4RepresentativeT[
    NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT];
static u32 sNdsRendererHardwareTexel01Ci4RepresentativeRowsValid;
static u32 sNdsRendererHardwareTexel01Ci4ClassTable[
    NDS_RENDERER_HW_CI4_CLASS_TABLE_COUNT];
#if defined(__arm__)
_Static_assert(
    sizeof(sNdsRendererHardwareTexel01Ci4Source0T) +
    sizeof(sNdsRendererHardwareTexel01Ci4Source1T) +
    sizeof(sNdsRendererHardwareTexel01Ci4RepresentativeS) +
    sizeof(sNdsRendererHardwareTexel01Ci4RepresentativeT) == 512u,
    "CI4 representative maps must stay within 512 bytes");
_Static_assert(sizeof(sNdsRendererHardwareTexel01Ci4ClassTable) == 1024u,
               "CI4 representative class table must stay within 1 KiB");
_Static_assert(
    (NDS_RENDERER_HW_CI4_CLASS_TABLE_COUNT &
     (NDS_RENDERER_HW_CI4_CLASS_TABLE_COUNT - 1u)) == 0u,
    "CI4 representative class table must remain a power of two");
_Static_assert(
    (NDS_RENDERER_HW_TEXTURE_MAX_WIDTH <=
     (NDS_RENDERER_HW_CI4_CLASS_TABLE_COUNT / 2u)) &&
    (NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT <=
     (NDS_RENDERER_HW_CI4_CLASS_TABLE_COUNT / 2u)),
    "CI4 representative class table must remain at most half full");
#endif
static u32 sNdsRendererHardwareTexel01Ci4PairLut[
    NDS_RENDERER_HW_TEXEL01_CI4_LUT_COUNT];
#if defined(__arm__)
_Static_assert(sizeof(sNdsRendererHardwareTexel01Ci4PairLut) == 1024u,
               "phase-masked CI4 pair lookup must stay within 1 KiB");
#endif
static u16 sNdsRendererHardwareTexel01Ci4LutPalette0[16];
static u16 sNdsRendererHardwareTexel01Ci4LutPalette1[16];
static u32 sNdsRendererHardwareTexel01Ci4LutFraction;
static u32 sNdsRendererHardwareTexel01Ci4LutKeyValid;
static NDSRendererHardwareCi4IndexCacheEntry
    sNdsRendererHardwareCi4IndexCache[
        NDS_RENDERER_HW_CI4_INDEX_CACHE_COUNT];
static u32 sNdsRendererHardwareCi4IndexCacheNext;
#if defined(__arm__)
_Static_assert(sizeof(sNdsRendererHardwareCi4IndexCache) == 2080u,
               "CI4 index cache must stay within 2080 bytes");
#endif
static NDSRendererHardwareLightShadeCacheEntry
    sNdsRendererHardwareLightShadeCache[
        NDS_RENDERER_HW_LIGHT_SHADE_CACHE_COUNT];
static u32 sNdsRendererHardwareLightShadeCacheNext;
#if defined(__arm__)
_Static_assert(sizeof(sNdsRendererHardwareLightShadeCache) == 4192u,
               "light shade lookup cache must stay within 4192 bytes");
#endif

static void ndsRendererHardwareEndBatch(void);

static inline s32 ndsRendererHardwareRawVertexFits(
    const NDSRendererInputVertex *vtx)
{
    if (vtx == NULL)
    {
        return FALSE;
    }
    return ((vtx->x >= NDS_RENDERER_HW_RAW_COORD_MIN) &&
            (vtx->x <= NDS_RENDERER_HW_RAW_COORD_MAX) &&
            (vtx->y >= NDS_RENDERER_HW_RAW_COORD_MIN) &&
            (vtx->y <= NDS_RENDERER_HW_RAW_COORD_MAX) &&
            (vtx->z >= NDS_RENDERER_HW_RAW_COORD_MIN) &&
            (vtx->z <= NDS_RENDERER_HW_RAW_COORD_MAX)) ? TRUE : FALSE;
}

static inline u32 ndsRendererHardwareDivideLitDotBy127(u32 dot)
{
    u32 biased = dot + 1u;

    /* The caller clamps dot to 127^2.  Over that complete domain this is
     * exactly floor(dot / 127), which is the reduced source expression
     * floor((dot * 127) / (127 * 127)). */
    return (biased + (biased >> 7)) >> 7;
}

static inline u32 ndsRendererHardwareLitShadeColorLut(
    const NDSRendererInputVertex *vtx,
    const NDSRendererHardwareLightDirection *direction,
    const u32 *rgb_lut)
{
    s32 dot;
    u32 diffuse_numer;

    dot = ((s32)(s8)vtx->r * direction->x) +
        ((s32)(s8)vtx->g * direction->y) +
        ((s32)(s8)vtx->b * direction->z);
    if (dot <= 0)
    {
        diffuse_numer = 0u;
    }
    else if (dot > (127 * 127))
    {
        diffuse_numer = 127u;
    }
    else
    {
        diffuse_numer = ndsRendererHardwareDivideLitDotBy127((u32)dot);
    }
    return rgb_lut[diffuse_numer] | (u32)vtx->a;
}
