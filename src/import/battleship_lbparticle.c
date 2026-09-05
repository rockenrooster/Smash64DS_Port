/* Compile the original BattleShip particle bytecode interpreter and its
 * effect-side owner in place: lb/lbparticle.c plus ef/efparticle.c. The
 * generator model, the opcode decode, the per-frame integrate and the
 * allocation pools are the source's, unmodified.
 *
 * Three things had to be replaced, and only three:
 *
 * 1. Bank residency. efParticleGetLoadBankID DMAs a .scb/.txb pair out of N64
 *    ROM and hands the raw descriptors to lbParticleSetupBankID. There is no
 *    N64 ROM here, so the DS build takes the packed bank from
 *    include/nds/generated/nds_particle_banks.generated.h and populates the
 *    interpreter's own bank arrays directly.
 *
 * 2. Byte order. The bank is byte-identical to the N64 .scb, so every LBScript
 *    header field and every f32 operand inside the bytecode is big-endian, and
 *    lbParticleReadFloatBigEnd is explicitly big-endian-only (lbparticle.c:584).
 *    That reader cannot be interposed -- renaming its definition renames its
 *    call sites with it -- so the normalization happens once at load instead:
 *    ndsParticleNormalizeScript byte-swaps the header and every float operand
 *    in the RAM copy, which makes the source reader correct on ARM by
 *    construction and leaves the interpreter untouched. The walk that finds
 *    those operands is also the bank's validator and its command census.
 *
 * 3. The draw path. lbParticleDrawTextures emits N64 texture rectangles into
 *    gSYTaskmanDLHeads[0]. The DS textured-quad path is a separate gated step,
 *    so this seam counts and returns; nothing is drawn yet. The source emitter
 *    still compiles (as ndsBaseLbParticleDrawTextures) and --gc-sections drops
 *    it, which keeps it available for that step without shipping dead code.
 *
 * Fail-closed rule: a script id whose packed offset is absent must never index
 * another script. Every external constructor validates the id first and returns
 * NULL with a counted rejection; ids reached from inside the bytecode resolve to
 * an inert sentinel script that spawns nothing, because drawing the wrong effect
 * is the exact defect class docs/BUGS.md already logs.
 */
#include "nds_scene_harness_config.h"
#include <nds/nds_scene_manager.h>

/* Take the original lb/lbtypes.h definition of LBTransform in this translation
 * unit. include/gr/ground.h carries a byte-identical copy behind this guard for
 * exactly this case; without it the two definitions collide. */
#define SSB64_NDS_LBTRANSFORM_DECLARED

/* Same reason, same pattern, but nineteen blocks wide rather than one type:
 * decomp/.../ft/ftdef.h and include/ft/fighter.h both declare 725 enumerators
 * (FTKind, FTPlayerKind, the anonymous status/motion/joint blocks, FTMotionEvent)
 * with identical values, and this is the first translation unit to reach both --
 * it compiles decomp sources in place while the decomp header web reaches the
 * port mirror through shadowed names it cannot be redirected away from. Take the
 * decomp copies; the port mirror stands down wherever it is reached from. */
#define SSB64_NDS_FTDEF_MIRROR

/* lb/library.h comes FIRST, ahead of the nds/ headers, and the order is load
 * bearing: nds_startup.h:866 declares ndsSyMallocWouldFit returning `sb32` but
 * does not itself guarantee that type. Every other translation unit in the
 * build happens to reach a decomp type header before it, so the omission never
 * showed; this file is the one that did not, and the result was 826 errors led
 * by "unknown type name 'sb32'" with the rest -- alSoundEffect, the whole
 * fighter.h enum re-declaring, implicit guMtxCatF -- cascading from it.
 *
 * Fixing it here rather than in nds_startup.h keeps the change to the file that
 * is being brought into the build. The better seam is for nds_startup.h to
 * include its own types, and it stays a trap for the next TU that includes it
 * early until it does.
 *
 * sys/audio.h is the same shape one level down: lb/lbcommon.h:43 declares an
 * `alSoundEffect *` field without declaring the type, and sys/audio.h:31 is
 * where that typedef lives. */
#include <sys/audio.h>
#include <lb/library.h>

#include <nds/nds_particle_runtime.h>
#include <nds/nds_firegrind.h>
#include <nds/nds_effects.h>
#include <nds/nds_startup.h>
#include <nds/generated/nds_particle_banks.generated.h>
#include <nds/nds_renderer.h>

/* The DECOMP sc/scene.h, by path, not the port's <sc/scene.h>. INCLUDES puts
 * include/ ahead of the decomp tree, so the angled form silently selects
 * include/sc/scene.h -- which pulls in include/ft/fighter.h, the port's
 * compatibility mirror of ft/ftdef.h and gm/gmdef.h. This translation unit
 * compiles decomp sources in place and already holds the real ftdef.h and
 * gmdef.h via lb/library.h, so the mirror arrives second and re-declares 763
 * enumerators and 62 macros it has no business owning here. Values agree
 * exactly (scripts/check-decomp-header-mirror.py proves it), so this is a
 * duplicate-declaration problem only, and the fix is to stop asking a
 * decomp-source translation unit for a port compatibility header. */
/* union GMStatFlags, which sc/sctypes.h:52 embeds by value. sctypes.h expects
 * it from <gm/generic.h>, but include/gm/generic.h shadows the decomp one and
 * does not carry it, so the field lands as an incomplete type. */
#include "../../decomp/BattleShip-main/decomp/src/gm/gmtypes.h"
#include "../../decomp/BattleShip-main/decomp/src/sc/scene.h"
/* nGRKindPupupu, for the Dream Land test in ndsParticleSceneIsBattle. Same
 * reason as sc/scene.h above: by path, because include/gr/ground.h shadows the
 * decomp gr headers. */
#include "../../decomp/BattleShip-main/decomp/src/gr/grdef.h"
#include <PR/gu.h>
#include <sys/dma.h>
#include <sys/debug.h>
#include <sys/taskman.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

/* Keep the load-bearing include order above. libnds nds/timers.h:255 is the
 * authority for this signature; a local prototype avoids pulling another
 * header web into the BattleShip translation unit. */
extern u32 cpuGetTiming(void);
extern u32 sySchedulerGetTicCount(void);

/* The two functions the decomp leaves to assembly (lbParticleUpdateStruct and
 * lbParticleGeneratorFuncRun) have complete C bodies behind this switch. The
 * port has no MIPS to link, so the C bodies are the implementation. */
#define NON_MATCHING 1

/* Interposed: the DS bank loader replaces the ROM DMA path entirely. */
#define efParticleGetLoadBankID ndsBaseEFParticleGetLoadBankID

/* Interposed: the N64 rectangle emitter is not the DS draw path. */
#define lbParticleDrawTextures ndsBaseLbParticleDrawTextures

/* Interposed: the source pool sizes are for the whole N64 game, and P1 is two
 * fighters on one stage with items off. See NDS_R2_PARTICLE_POOL_* below.
 * Safe to rename here because efParticleInitAll has NO caller inside
 * efparticle.c or lbparticle.c -- every call is from another TU, so the
 * `#define` moves the definition without taking a call site with it. */
#define efParticleInitAll ndsBaseEFParticleInitAll

/* Interposed: every external constructor validates its script id first. */
#define lbParticleMakeScriptID ndsBaseLbParticleMakeScriptID
#define lbParticleMakeCommon ndsBaseLbParticleMakeCommon
#define lbParticleMakePosVel ndsBaseLbParticleMakePosVel
#define lbParticleMakeGenerator ndsBaseLbParticleMakeGenerator

/* lbparticle.c calls lbParticleMakeGenerator (from efParticleMakeGenerator's
 * neighbourhood) before defining it at line 2617, and the interposition renames
 * the call site and the definition together -- so the call gets an implicit
 * `int()` that then conflicts with the real `LBGenerator *(s32, s32)`. The
 * source's own header declares the unrenamed name, which no longer matches. */
LBGenerator *ndsBaseLbParticleMakeGenerator(s32 bank_id, s32 script_id);

#include "../../decomp/BattleShip-main/decomp/src/lb/lbparticle.c"
#include "../../decomp/BattleShip-main/decomp/src/ef/efparticle.c"

#undef efParticleGetLoadBankID
#undef efParticleInitAll
#undef lbParticleDrawTextures
#undef lbParticleMakeScriptID
#undef lbParticleMakeCommon
#undef lbParticleMakePosVel
#undef lbParticleMakeGenerator

/* THE POOLS, SIZED FOR P1 INSTEAD OF FOR THE WHOLE GAME.
 *
 * efparticle.c:28 reserves 112 structs, 24 generators and 80 transforms, and
 * sizeof is 96 / 92 / 192, so the source figure is 28,320 bytes of taskman
 * arena claimed up front. That is not a size nicety here: linked and arena
 * bytes compete, and with the source sizes the battle dies before it starts --
 * the general heap reaches 1,040 bytes free, ifCommonSetMaxNumGObj
 * (ifcommon.c:3156) latches the GObj pool at the count it happens to be at
 * (45), and ifCommonCountdownMakeInterface then dereferences the NULL its 46th
 * request returns. Measured at the crash: StructsMax 0. Not one of the 112 had
 * been used.
 *
 * The N64 numbers cover four players, items, and every stage's hazards
 * simultaneously. P1 is Mario vs one Fox on Dream Land with items off, which is
 * a fraction of the live effect population, and PROJECT_GOAL is explicit that
 * content may be specialized to the configuration.
 *
 * SIZED TO BE MEASURED, NOT GUESSED AT. The first sizing (40/10/24) was a
 * starting point chosen to fit while zero scripts were actually running, and
 * the high-water counters graded it the moment they did. A both-CPU match with
 * the corrected P1 seam list reported:
 *
 *   StructsMax     40 of 40   SATURATED -- every further spawn was dropped
 *   GeneratorsMax   8 of 10   1.25x, tight
 *   TransformsMax   2 of 24   12x, and a transform is 192 B, the dearest unit
 *
 * so the arena was being spent almost entirely on the pool nothing used. The
 * rebalance below is close to free: 64*96 + 12*92 + 12*192 = 9,552 bytes
 * against the old 9,368, +184 for 60% more structs and six times the measured
 * transform mark. Grade these again after any change that adds effects; a
 * saturated pool is a silently missing effect, not an error.
 *
 * REGRADED 2026-08-01 against a single-CPU tick-HUD match with the corrected
 * seam list: StructsMax 41 of 64, GeneratorsMax 8 of 12, TransformsMax 2 of
 * 12. And regrading was not optional -- these bytes are the ONLY slack between
 * this build and a hard failure. `ifCommonSetMaxNumGObj` (ifcommon.c:3156)
 * caps the GObj pool at whatever is active when the general heap drops under
 * 25 KiB free, and past that cap `ifCommonCountdownMakeInterface` dereferences
 * a NULL. Measured with NDS_R2_PARTICLE_DRAW=1: free 23,032, COMMONSMAX
 * latched at 45, data abort in ifCommonTrafficMakeSObj at the GO countdown --
 * a healthy allocator (MALLOCOVF=0) and a dead game. The deficit was 2,568
 * bytes against DRAW's +3,008 of .text.
 *
 * 48/10/6 keeps 17% headroom on structs, 25% on generators and 3x on
 * transforms, and returns 2,872 bytes: (64-48)*96 + (12-10)*92 + (12-6)*192.
 * Overflow is fail-closed and counted (gNdsParticleRejectCount), so a pool
 * that turns out to be tight reports itself instead of aborting.
 *
 * THE MARGIN IS THE THING TO WATCH, not these three numbers. The soak prints
 * GENERALHEAP free on every run now; anything under ~4 KB of headroom over
 * 25,600 means the next feature trips the same cliff.
 *
 * REGRADED 2026-08-02, and A SATURATED COUNTER IS A FLOOR, NOT A MEASUREMENT.
 * Both regrades above were taken from matches that never ran a KO burst, so
 * they graded the wrong population. The first both-CPU soak that reached six
 * KOs reported:
 *
 *   StructsMax     26 of 48   1.8x, fine
 *   GeneratorsMax  10 of 10   SATURATED
 *   TransformsMax   6 of  6   SATURATED
 *   KOBurstAttempt  6, Complete 4, DropMask 2 = NDS_KO_BURST_DROP_XF
 *
 * i.e. two of six KOs drew no burst at all: efManagerDeadExplodeMakeEffect
 * (battleship_efmanager.c:1312) asks for one LBTransform, got NULL, and ejects
 * its particle. That is the owner's "not all vfx are played", measured.
 *
 * Note gNdsParticleRejectCount read 0 through both saturations -- it counts
 * struct rejects only, so it cannot be used as the all-clear for these pools.
 * Read the three Max counters against the three caps.
 *
 * Structs stay at 48 because 26 of 48 is a real measurement. Generators and
 * transforms are not: saturation says "at least this many", so the new caps
 * are sized to be provable rather than to be tight. 24 each is the source's
 * own generator count (efparticle.c:31) and 4x the transform saturation mark,
 * costing 14*92 + 18*192 = 4,744 bytes against a measured general-heap low
 * water of 144,336 -- 118,736 over the cliff before this change, 113,992 after.
 * The source's 80 transforms (efparticle.c:33) would cost 15,360 and is where
 * to go next if 24 saturates too; it covers four players and items, which P1
 * does not have.
 *
 * PREDICTION, to be graded by the next both-CPU soak: DropMask 0, Complete ==
 * Attempt, and both Max counters strictly below 24. If either pins at 24 again
 * the demand is still unmeasured and this comment is still wrong.
 *
 * GRADED 2026-08-03: PASSED, on both scenes, and these caps are now measured
 * rather than floored. The both-CPU soak returned DropMask 0, Attempt 3 ==
 * Complete 3, TransformsMax 13 and GeneratorsMax 11 -- all strictly under 24.
 * probe-results-confetti.ps1, which reaches the heavier scene a battle soak
 * never sees, read gens_used 24 against the 48 that
 * ndsMNVSResultsFuncStartTimed asks for, with both sheets at 192 sized pieces.
 *
 * READ THE PROBE'S SECOND FIELD CAREFULLY. `gens_max` in the CONFETTISLOTS line
 * is gNdsParticleGeneratorsMax -- a HIGH-WATER MARK, not a cap. It printed
 * `gens_used=24 gens_max=24` and that reads exactly like the saturation this
 * comment block warns about, so it was called saturated and generators were
 * bumped to 48 here; the re-probe printed the same two numbers, because the
 * cap the Results scene actually uses comes from the Wanted override at
 * battleship_mnvsresults.c:236 and never from this define. Two equal counters
 * are only saturation when the second one is the bound. The bump is reverted.
 *
 * The old P1 battle therefore kept 48 / 24 / 24. P2-2 changes the contract:
 * the shipping shell can run four simultaneous fighters, and the source's own
 * efParticleInitAll sizes (efparticle.c:30-33) are 112 / 24 / 80 specifically
 * for the full game. Restore those exact capacities for the P2 shell and its
 * dedicated four-fighter stress target; direct-boot P1/perf targets keep the
 * measured smaller pools.
 * Results still uses the Wanted seam below, which currently resolves back to
 * source sizing before its own func_start. */
#if NDS_P2_MENU_SHELL || NDS_P2_FOUR_CPU_STRESS
#define NDS_R2_PARTICLE_POOL_STRUCTS 112
#define NDS_R2_PARTICLE_POOL_GENERATORS 24
#define NDS_R2_PARTICLE_POOL_TRANSFORMS 80
#else
#define NDS_R2_PARTICLE_POOL_STRUCTS 48
#define NDS_R2_PARTICLE_POOL_GENERATORS 24
#define NDS_R2_PARTICLE_POOL_TRANSFORMS 24
#endif

#if NDS_R2_FOX_BLASTER_GLOW_AOT
/* EFCommon script 0x62, decoded from the pinned bank:
 *
 *   MASKT; size 0->150/3; 150->165/1; 165->150/2;
 *   current->45/2; 45->0/3; END
 *
 * lbParticleMakeCommon performs the first update before the caller writes the
 * position, leaving exactly these nine visible half-extents. Store them in Q8
 * so the fixed renderer never executes the interpreter's float divides. The
 * tenth source tick is size zero and is retired here as visually dead. */
#define NDS_FOX_BLASTER_GLOW_POOL_COUNT 4u
#define NDS_FOX_BLASTER_GLOW_VISIBLE_TICKS 9u
#define NDS_FOX_BLASTER_GLOW_TEXTURE_ID 27u
#define NDS_FOX_BLASTER_GLOW_BINDING_SLOT NDS_WHISPY_NATIVE_TEXTURE_COUNT

typedef struct NDSFoxBlasterGlowAOT
{
    Vec3f pos;
    s32 center_q12[3];
    u32 spawn_tick;
} NDSFoxBlasterGlowAOT;

static NDSFoxBlasterGlowAOT
    sNdsFoxBlasterGlowAOT[NDS_FOX_BLASTER_GLOW_POOL_COUNT];
static u32 sNdsFoxBlasterGlowAOTCount;
static const s32 sNdsFoxBlasterGlowSizeQ8[
    NDS_FOX_BLASTER_GLOW_VISIBLE_TICKS] = {
    12800, 25600, 38400, 42240, 40320, 25920, 11520, 7680, 3840
};
static const f32 sNdsFoxBlasterGlowSize[
    NDS_FOX_BLASTER_GLOW_VISIBLE_TICKS] = {
    50.0F, 100.0F, 150.0F, 165.0F, 157.5F,
    101.25F, 45.0F, 30.0F, 15.0F
};

volatile u32 gNdsFoxBlasterGlowAOTSpawnCount;
volatile u32 gNdsFoxBlasterGlowAOTDrawCount;
volatile u32 gNdsFoxBlasterGlowAOTFallbackCount;
volatile u32 gNdsFoxBlasterGlowAOTMissCount;

static void ndsParticleResetFoxBlasterGlowAOT(void)
{
    memset(sNdsFoxBlasterGlowAOT, 0, sizeof(sNdsFoxBlasterGlowAOT));
    sNdsFoxBlasterGlowAOTCount = 0u;
    gNdsFoxBlasterGlowAOTSpawnCount = 0u;
    gNdsFoxBlasterGlowAOTDrawCount = 0u;
    gNdsFoxBlasterGlowAOTFallbackCount = 0u;
    gNdsFoxBlasterGlowAOTMissCount = 0u;
}

sb32 ndsParticleSpawnFoxBlasterGlowAOT(const Vec3f *pos)
{
    s32 center_q12[3];
    u32 now = sySchedulerGetTicCount();
    u32 live = 0u;
    u32 index;

    if ((pos == NULL) ||
        (ndsRendererHardwareFoxBlasterGlowName() == 0u) ||
        (ndsRendererParticlePositionToQ12(pos, center_q12) == FALSE))
    {
        gNdsFoxBlasterGlowAOTFallbackCount++;
        return FALSE;
    }
    /* Compact only at a rare spawn, not every frame. Slot zero is newest,
     * matching lbParticleMakeStruct's head insertion and therefore the source
     * translucent order when two flashes overlap. */
    for (index = 0u; index < sNdsFoxBlasterGlowAOTCount; index++)
    {
        if ((u32)(now - sNdsFoxBlasterGlowAOT[index].spawn_tick) <
            NDS_FOX_BLASTER_GLOW_VISIBLE_TICKS)
        {
            if (live != index)
            {
                sNdsFoxBlasterGlowAOT[live] =
                    sNdsFoxBlasterGlowAOT[index];
            }
            live++;
        }
    }
    if (live >= NDS_FOX_BLASTER_GLOW_POOL_COUNT)
    {
        gNdsFoxBlasterGlowAOTFallbackCount++;
        return FALSE;
    }
    for (index = live; index > 0u; index--)
    {
        sNdsFoxBlasterGlowAOT[index] =
            sNdsFoxBlasterGlowAOT[index - 1u];
    }
    sNdsFoxBlasterGlowAOT[0].pos = *pos;
    sNdsFoxBlasterGlowAOT[0].center_q12[0] = center_q12[0];
    sNdsFoxBlasterGlowAOT[0].center_q12[1] = center_q12[1];
    sNdsFoxBlasterGlowAOT[0].center_q12[2] = center_q12[2];
    sNdsFoxBlasterGlowAOT[0].spawn_tick = now;
    sNdsFoxBlasterGlowAOTCount = live + 1u;
    /* Preserve the source constructor's externally visible tallies/serial even
     * though no 96-byte LBParticle is consumed. */
    dLBParticleCurrentGeneratorID++;
    gNdsParticleScriptStartCount++;
    gNdsFoxBlasterGlowAOTSpawnCount++;
    return TRUE;
}
#endif

/* The pool the NEXT efParticleInitAll should use, or 0 for the battle sizing
 * above. Only the Results scene sets it, because only that scene both needs a
 * bigger pool and has the room: the battle's general-heap low water is 24,404
 * bytes and the 25 KiB cliff under it is what those constants defend, while
 * Results reports 308,316 free.
 *
 * On its own this changes nothing and was reverted once for that reason -- at
 * the source's update_rate the confetti emitters never asked for more than 40
 * structs and a 112-struct pool sat unused with 0 rejects. It only matters
 * alongside NDS_R2_CONFETTI_UPDATE_RATE. */
volatile u32 gNdsParticlePoolStructsWanted;
volatile u32 gNdsParticlePoolGeneratorsWanted;
volatile u32 gNdsParticlePoolTransformsWanted;

#if NDS_R2_WHISPY_NATIVE_AOT
static void ndsWhispyAOTStructFuncRun(GObj *gobj);
static void ndsWhispyAOTGeneratorFuncRun(GObj *gobj);
#endif

void efParticleInitAll(void)
{
    u32 structs = (gNdsParticlePoolStructsWanted != 0u) ?
        gNdsParticlePoolStructsWanted : NDS_R2_PARTICLE_POOL_STRUCTS;
    u32 generators = (gNdsParticlePoolGeneratorsWanted != 0u) ?
        gNdsParticlePoolGeneratorsWanted : NDS_R2_PARTICLE_POOL_GENERATORS;
    u32 transforms = (gNdsParticlePoolTransformsWanted != 0u) ?
        gNdsParticlePoolTransformsWanted : NDS_R2_PARTICLE_POOL_TRANSFORMS;

    gEFParticleStructsGObj = lbParticleAllocStructs((s32)structs);
    gEFParticleGeneratorsGObj = lbParticleAllocGenerators((s32)generators);
#if NDS_R2_WHISPY_NATIVE_AOT
    /* Swap only the two once-per-frame runners. The GObjs, source free lists,
     * source allocation records, constructors, transforms and ejectors remain
     * BattleShip's. Both wrappers fail back to the original function whenever
     * exact Dream Land script identity or the closed AOT contract is absent. */
    if (gEFParticleStructsGObj != NULL)
    {
        gEFParticleStructsGObj->func_run = ndsWhispyAOTStructFuncRun;
    }
    if (gEFParticleGeneratorsGObj != NULL)
    {
        gEFParticleGeneratorsGObj->func_run = ndsWhispyAOTGeneratorFuncRun;
    }
#endif

    lbParticleAllocTransforms((s32)transforms, sizeof(LBTransform));
    sEFParticleBanksNum = 0;
#if NDS_R2_FOX_BLASTER_GLOW_AOT
    ndsParticleResetFoxBlasterGlowAOT();
#endif
    /* Every call here restarts bank numbering, so two loads on either side of
     * one reset share a slot. That is how efcommon and Dream Land both reported
     * bank 0 on 2026-08-01. Counted rather than assumed. */
    gNdsParticleInitAllCount++;
#if NDS_R2_FIREGRIND_NATIVE
    /* The DS-native FireGrind pool is scene/match-lifetime like the particle
     * pools above; drop every live spark on the same reset so a restart does not
     * inherit sparks from the previous match. */
    ndsFireGrindReset();
#endif
}

/* efdisplay.c passes the address of this marker as the efcommon script bank.
 * It is defined in src/import/battleship_efmanager.c; comparing against it is
 * an exact identity test, not a heuristic. */
extern uintptr_t lEFCommonParticleScriptBankLo;

/* Dream Land's own bank marker. Declared in include/reloc_data.h and defined in
 * src/port/diagnostics.c as intptr_t; only its address is ever used. */
extern intptr_t lGRPupupuParticleScriptBankLo;
#if NDS_P2_STAGE_YOSTER
/* P2-4 Yoster vapor bank marker. Declared in include/reloc_data.h
 * (decomp gr/grcommon/gryoster.h:9-12) and defined in
 * src/import/battleship_gryoster_ground.c; only its address is ever used, and
 * only behind this flag, so the flag-off link never sees it. */
extern intptr_t lGRYosterParticleScriptBankLo;

/* P2-4 Yoster vapor bank payload. Emitted by
 * scripts/generate_nds_particle_banks.py into the (gitignored, build-baked)
 * particle .inc, which is why these externs live here rather than in the
 * generated header another agent owns this cycle. Counts mirror the emission:
 * 1 script (script 0, grYosterCloudVaporMakeEffect,
 * decomp gr/grcommon/gryoster.c), 1 texture (32x32x1), stride 192. */
extern u8 gNdsYosterScriptBank[];
extern const u32 gNdsYosterScriptBankBytes;
extern const u32 gNdsYosterScriptOffsets[1];
extern const u8 gNdsYosterTextureDims[3];
#endif

#if NDS_R2_WHISPY_NATIVE_AOT
#if defined(__arm__)
#define NDS_WHISPY_AOT_FAST_CODE \
    __attribute__((hot, optimize("O3"), target("arm")))
#define NDS_WHISPY_AOT_INLINE_CODE \
    __attribute__((always_inline, hot, optimize("O3"), target("arm")))
#else
#define NDS_WHISPY_AOT_FAST_CODE __attribute__((hot, optimize("O3")))
#define NDS_WHISPY_AOT_INLINE_CODE \
    __attribute__((always_inline, hot, optimize("O3")))
#endif

/* Dream Land's three emitted scripts, compiled into the ARM9 image.
 *
 * Scripts 0/1 remain the source bytecode roots: they create the source
 * LBGenerator objects and attach the source LBTransform exactly as before.
 * Only generator scripts 2/3/4 are closed here. Their source headers are
 * immutable and tiny (416 bytes for the whole bank), so exact bytecode-pointer
 * identity is a stronger selector than a bank number, which can be reused
 * across efParticleInitAll resets.
 *
 * The five orientation values are the source generator's invariant
 * atan2/sin/cos/sqrt result for each header velocity. Hoisting them is the AOT
 * half of this cut. The per-emission random angles use gSYSinTable, the DS-
 * resident source sine table, instead of four ARM9 libm calls. The small table
 * error affects presentation only; RNG call count/order and spawn construction
 * remain source-owned. Route 5 compiles the closed post-construction control
 * states while retaining these same source objects and ownership lists. */
typedef struct NDSWhispyAOTGeneratorDesc
{
    u8 script_id;
    u8 texture_id;
    u16 particle_flags;
    f32 vel_x;
    f32 vel_y;
    f32 vel_z;
    f32 radius;
    f32 angle_span;
    f32 update_rate;
    f32 sin_angle1;
    f32 cos_angle1;
    f32 sin_angle2;
    f32 cos_angle2;
    f32 magnitude;
} NDSWhispyAOTGeneratorDesc;

static const NDSWhispyAOTGeneratorDesc sNdsWhispyAOTGenerators[] = {
    { 2u, 2u, LBPARTICLE_FLAG_GRAVITY,
       60.0F, -5.0F, 0.0F,  5.0F, 0.1745329201F, -0.02F,
      -1.0F, -4.37113883e-8F, 0.9965457916F, 0.0830454156F,
      60.20797348F },
    { 3u, 0u, LBPARTICLE_FLAG_GRAVITY | LBPARTICLE_FLAG_FRICTION,
       60.0F,  0.0F, 0.0F,  0.0F, 0.0F,          -0.10F,
       0.0F,  1.0F,           1.0F,          -4.37113883e-8F,
      60.0F },
    { 4u, 1u, LBPARTICLE_FLAG_FRICTION,
      100.0F,  3.0F, 0.0F, 20.0F, 0.1745329201F, -0.25F,
       1.0F, -4.37113883e-8F, 0.9995502830F, 0.0299864914F,
      100.0449905F },
};

volatile u32 gNdsWhispyAOTGeneratorFastRuns;
volatile u32 gNdsWhispyAOTGeneratorFallbackRuns;
volatile u32 gNdsWhispyAOTGeneratorVisits;
volatile u32 gNdsWhispyAOTGeneratorEmits;
volatile u32 gNdsWhispyAOTTableTrigPairs;
#if NDS_TICK_HUD
volatile u32 gNdsWhispyAOTGeneratorTicks;
#endif
volatile u32 gNdsWhispyAOTStructVisits;
volatile u32 gNdsWhispyAOTStructFastUpdates;
volatile u32 gNdsWhispyAOTStructSourceUpdates;
#if NDS_TICK_HUD
volatile u32 gNdsWhispyAOTStructTicks;
#endif
volatile u32 gNdsWhispyAOTDividesAvoided;
volatile u32 gNdsWhispyAOTRigidDraws;
volatile u32 gNdsWhispyAOTRigidDrawFallbacks;
volatile u32 gNdsWhispyAOTTier2GeneratorMatches;
volatile u32 gNdsWhispyAOTTier2DirectUpdates;
volatile u32 gNdsWhispyAOTTier2FixedTransforms;
volatile u32 gNdsWhispyAOTTier2FixedSubmits;
volatile u32 gNdsWhispyAOTTier2FixedFallbacks;
/* Runtime A/B selector for the lab ROM. All arms keep the identical linked
 * image and cache placement: 0 is source, 1 is the conservative divide/trig
 * cut, 2 is the closed direct-update/fixed-submit kernel, 3 adds pinned binds,
 * 4 sends those same source-ordered fixed vertices/state through one bounded
 * GXFIFO DMA packet per run, 5 also compiles the three closed scripts'
 * wait/loop/blend/end transitions plus source-identical pool retirement, and 6
 * runs that closed update/draw path as one ARM/O3 kernel with pass-local proof
 * tallies and cached exact-script identity. Route 7 additionally removes
 * repeat validation/leg/clamp work from the already-bounded GX packet path.
 * Route 7 is the owner-approved promoted default; build-time overrides retain
 * the source route for controlled A/B and fallback verification. */
volatile u32 gNdsWhispyAOTRoute = 7u;

/* Exact bytecode identity remains the selector, but after one full bank/table
 * proof there is no reason to chase the same three script tables for every
 * particle on every frame. A slot reuse cannot false-positive: the candidate
 * still has to carry the exact immutable source bytecode pointer cached here. */
static u32 sNdsWhispyAOTScriptCacheSlot = ~0u;
static const u8 *sNdsWhispyAOTScriptCache[3];

static sb32 ndsParticleBankIsPupupu(u8 bank_id)
{
    u32 slot = (u32)bank_id & 7u;

    return ((slot < ARRAY_COUNT(sEFParticleScriptBanks)) &&
            (sEFParticleScriptBanks[slot] ==
             (uintptr_t)&lGRPupupuParticleScriptBankLo)) ? TRUE : FALSE;
}

static const NDSWhispyAOTGeneratorDesc *
ndsWhispyAOTDescForBytecode(u8 bank_id, u8 texture_id, const u8 *bytecode)
{
    u32 slot = (u32)bank_id & 7u;
    u32 index;
    u32 script_id;
    LBScript *script;

    /* The exact bank maps emitted scripts 2/3/4 to textures 2/0/1. Use that
     * closed mapping instead of scanning three descriptors for every live
     * particle and every generator pass. The final bytecode-pointer equality
     * remains the runtime proof that this is the pinned source script. */
    switch (texture_id)
    {
    case 2u: index = 0u; break;
    case 0u: index = 1u; break;
    case 1u: index = 2u; break;
    default: return NULL;
    }
    if ((gNdsWhispyAOTRoute >= 6u) &&
        (sNdsWhispyAOTScriptCacheSlot == slot) &&
        (sNdsWhispyAOTScriptCache[index] != NULL) &&
        (sNdsWhispyAOTScriptCache[index] == bytecode))
    {
        return &sNdsWhispyAOTGenerators[index];
    }
    if ((ndsParticleBankIsPupupu(bank_id) == FALSE) ||
        (slot >= ARRAY_COUNT(sLBParticleScriptBanks)))
    {
        return NULL;
    }
    script_id = sNdsWhispyAOTGenerators[index].script_id;
    if (script_id >= (u32)sLBParticleScriptBanksNum[slot])
    {
        return NULL;
    }
    script = sLBParticleScriptBanks[slot][script_id];
    if ((script == NULL) || (script->bytecode != bytecode))
    {
        return NULL;
    }
    if (gNdsWhispyAOTRoute >= 6u)
    {
        if (sNdsWhispyAOTScriptCacheSlot != slot)
        {
            sNdsWhispyAOTScriptCacheSlot = slot;
            memset(sNdsWhispyAOTScriptCache, 0,
                   sizeof(sNdsWhispyAOTScriptCache));
        }
        sNdsWhispyAOTScriptCache[index] = bytecode;
    }
    return &sNdsWhispyAOTGenerators[index];
}

static void ndsWhispyAOTSinCos(f32 angle, f32 *sin_out, f32 *cos_out)
{
    s32 angle_id = SINTABLE_RAD_TO_ID(angle) & 0xFFF;
    s32 sin_value = (s32)gSYSinTable[angle_id & 0x7FF];
    s32 cos_value;

    if ((angle_id & 0x800) != 0)
    {
        sin_value = -sin_value;
    }
    angle_id = (angle_id + 0x400) & 0xFFF;
    cos_value = (s32)gSYSinTable[angle_id & 0x7FF];
    if ((angle_id & 0x800) != 0)
    {
        cos_value = -cos_value;
    }
    *sin_out = (f32)sin_value * (1.0F / 32768.0F);
    *cos_out = (f32)cos_value * (1.0F / 32768.0F);
}

static sb32 ndsWhispyAOTGeneratorMatches(
    const LBGenerator *gn, const NDSWhispyAOTGeneratorDesc *desc)
{
    if ((gn == NULL) || (desc == NULL) ||
        (gn->kind != 0u) || (gn->dobj != NULL))
    {
        return FALSE;
    }
    if (gNdsWhispyAOTRoute >= 2u)
    {
        /* Exact bank + exact bytecode already prove these immutable header
         * fields. Re-comparing eight floats through __aeabi_fcmpeq for every
         * generator on every frame was validation in the hottest possible
         * place. The generated-bank checker pins the source hash instead. */
        gNdsWhispyAOTTier2GeneratorMatches++;
        return TRUE;
    }
    return ((gn->texture_id == desc->texture_id) &&
            (gn->vel.x == desc->vel_x) &&
            (gn->vel.y == desc->vel_y) &&
            (gn->vel.z == desc->vel_z) &&
            (gn->unk_gn_0x38 == desc->radius) &&
            (gn->unk_gn_0x3C == desc->angle_span) &&
            (gn->update_rate == desc->update_rate) &&
            (gn->generator_vars.rotate.base == 0.0F) &&
            (gn->generator_vars.rotate.target == F_CST_DTOR32(360.0F))) ?
        TRUE : FALSE;
}

/* Fast only when EVERY queued generator is one of the three exact Whispy
 * scripts. That all-or-source preflight is what preserves shared RNG ordering:
 * a common hit/KO generator overlapping the wind makes this entire frame call
 * BattleShip's original loop, rather than processing the two lists in a new
 * order and perturbing later gameplay randomness. */
static void ndsWhispyAOTGeneratorFuncRun(GObj *gobj)
{
    LBGenerator *gn;
    LBGenerator *next_gn;
#if NDS_TICK_HUD
    u32 tick_start = cpuGetTiming();
#endif

    if (gNdsWhispyAOTRoute == 0u)
    {
        lbParticleGeneratorFuncRun(gobj);
#if NDS_TICK_HUD
        gNdsWhispyAOTGeneratorTicks += cpuGetTiming() - tick_start;
#endif
        return;
    }
    if (sLBParticleGeneratorsQueued == NULL)
    {
        lbParticleGeneratorFuncRun(gobj);
#if NDS_TICK_HUD
        gNdsWhispyAOTGeneratorTicks += cpuGetTiming() - tick_start;
#endif
        return;
    }
    for (gn = sLBParticleGeneratorsQueued; gn != NULL; gn = gn->next)
    {
        const NDSWhispyAOTGeneratorDesc *desc =
            ndsWhispyAOTDescForBytecode(
                gn->bank_id, (u8)gn->texture_id, gn->bytecode);

        if (ndsWhispyAOTGeneratorMatches(gn, desc) == FALSE)
        {
            gNdsWhispyAOTGeneratorFallbackRuns++;
            lbParticleGeneratorFuncRun(gobj);
#if NDS_TICK_HUD
            gNdsWhispyAOTGeneratorTicks += cpuGetTiming() - tick_start;
#endif
            return;
        }
    }

    gNdsWhispyAOTGeneratorFastRuns++;
    gn = sLBParticleGeneratorsQueued;
    sLBParticleGeneratorsLastProcessed = NULL;

    while (gn != NULL)
    {
        const NDSWhispyAOTGeneratorDesc *desc =
            ndsWhispyAOTDescForBytecode(
                gn->bank_id, (u8)gn->texture_id, gn->bytecode);

        gNdsWhispyAOTGeneratorVisits++;
        if ((gobj->flags & (1u << ((gn->bank_id >> 3) + 0x10))) ||
            (gn->flags & LBPARTICLE_FLAG_PAUSE))
        {
            sLBParticleGeneratorsLastProcessed = gn;
            gn = gn->next;
            continue;
        }

        /* Eligibility proved update_rate is negative, so this is the source's
         * deterministic arm and consumes no RNG until an emission matures. */
        gn->frame -= gn->update_rate;
        if (gn->frame >= 1.0F)
        {
            /* Source computes a first random azimuth and an angular step here.
             * All three Whispy scripts have a nonnegative angle span, so every
             * emission overwrites that azimuth and never reads the step. Keep
             * the RNG advance; delete the dead float and divide. */
            (void)syUtilsRandFloat();
        }
        while (gn->frame >= 1.0F)
        {
            f32 radial_random = syUtilsRandFloat();
            f32 radial = gn->unk_gn_0x38 * radial_random;
            f32 azimuth = gn->generator_vars.rotate.base +
                (syUtilsRandFloat() *
                 (gn->generator_vars.rotate.target -
                  gn->generator_vars.rotate.base));
            f32 spread = radial_random * gn->unk_gn_0x3C;
            f32 sin_azimuth;
            f32 cos_azimuth;
            f32 sin_spread;
            f32 cos_spread;
            f32 radial_x;
            f32 radial_y;
            f32 cone_x;
            f32 cone_y;
            f32 cone_z;
            f32 pos_x;
            f32 pos_y;
            f32 pos_z;
            f32 vel_x;
            f32 vel_y;
            f32 vel_z;

            ndsWhispyAOTSinCos(azimuth, &sin_azimuth, &cos_azimuth);
            ndsWhispyAOTSinCos(spread, &sin_spread, &cos_spread);
            gNdsWhispyAOTTableTrigPairs += 2u;

            radial_x = cos_azimuth * radial;
            radial_y = sin_azimuth * radial;
            cone_x = cos_azimuth * (sin_spread * desc->magnitude);
            cone_y = sin_azimuth * (sin_spread * desc->magnitude);
            cone_z = cos_spread * desc->magnitude;

            pos_x = (radial_x * desc->cos_angle2) + gn->pos.x;
            pos_y = (-radial_x * desc->sin_angle1 * desc->sin_angle2) +
                    (radial_y * desc->cos_angle1) + gn->pos.y;
            pos_z = (-radial_x * desc->cos_angle1 * desc->sin_angle2) -
                    (radial_y * desc->sin_angle1) + gn->pos.z;

            vel_x = (cone_x * desc->cos_angle2) +
                    (cone_z * desc->sin_angle2);
            vel_y = (-cone_x * desc->sin_angle1 * desc->sin_angle2) +
                    (cone_y * desc->cos_angle1) +
                    (cone_z * desc->sin_angle1 * desc->cos_angle2);
            vel_z = (-cone_x * desc->cos_angle1 * desc->sin_angle2) -
                    (cone_y * desc->sin_angle1) +
                    (cone_z * desc->cos_angle1 * desc->cos_angle2);

            /* The source constructor immediately runs the source bytecode once
             * and links the real LBParticle into the real pool. Keeping that
             * call is deliberate: this optimization owns math, not spawns. */
            lbParticleMakeParam(
                gn->bank_id, gn->flags, gn->texture_id, gn->bytecode,
                gn->particle_lifetime, pos_x, pos_y, pos_z,
                vel_x, vel_y, vel_z, gn->size, gn->gravity, gn->friction,
                0u, gn);
            gNdsWhispyAOTGeneratorEmits++;
            gn->frame -= 1.0F;
        }

        if (gn->generator_lifetime != 0u)
        {
            gn->generator_lifetime--;
            if (gn->generator_lifetime == 0u)
            {
                /* Vortex is excluded by the preflight, so this is the source's
                 * ordinary ejection arm verbatim. */
                if (sLBParticleGeneratorsLastProcessed == NULL)
                {
                    sLBParticleGeneratorsQueued = gn->next;
                }
                else
                {
                    sLBParticleGeneratorsLastProcessed->next = gn->next;
                }
                next_gn = gn->next;
                if (gn->xf != NULL)
                {
                    gn->xf->users_num--;
                    if (gn->xf->users_num == 0u)
                    {
                        lbParticleEjectTransform(gn->xf);
                    }
                }
                gn->next = sLBParticleGeneratorsAllocFree;
                sLBParticleGeneratorsAllocFree = gn;
                gn = next_gn;
                gLBParticleGeneratorsUsedNum--;
                continue;
            }
        }
        sLBParticleGeneratorsLastProcessed = gn;
        gn = gn->next;
    }
#if NDS_TICK_HUD
    gNdsWhispyAOTGeneratorTicks += cpuGetTiming() - tick_start;
#endif
}

/* Exact floor(65536 / n), generated once into ARM immediates/rodata. Script 2
 * starts its terminal alpha blend with source length 17; construction already
 * consumes the only other length-17 blend before the AOT runner sees it. */
static const s32 sNdsWhispyBlendReciprocalQ16[18] = {
    0, 65536, 32768, 21845, 16384, 13107, 10922, 9362, 8192,
    7281, 6553, 5957, 5461, 5041, 4681, 4369, 4096, 3855
};
static const f32 sNdsWhispySizeReciprocal[18] = {
    0.0F, 1.0F, 0.5F, 0.3333333333F, 0.25F, 0.2F,
    0.1666666667F, 0.1428571429F, 0.125F, 0.1111111111F,
    0.1F, 0.0909090909F, 0.0833333333F, 0.0769230769F,
    0.0714285714F, 0.0666666667F, 0.0625F, 0.0588235294F
};

static u8 ndsWhispyAOTBlendChannel(u8 current, u8 target, u16 length)
{
    s32 value = ((s32)current << 16) +
        (((s32)target - (s32)current) *
         sNdsWhispyBlendReciprocalQ16[length]);

    return (u8)(value >> 16);
}

static void ndsWhispyAOTApplyBlends(LBParticle *pc,
                                    u16 size_length,
                                    u16 prim_length,
                                    u16 env_length)
{
    if (size_length != 0u)
    {
        pc->size += (pc->size_target - pc->size) *
                    sNdsWhispySizeReciprocal[size_length];
        pc->size_target_length = size_length - 1u;
        gNdsWhispyAOTDividesAvoided++;
    }
    if (prim_length != 0u)
    {
        pc->primcolor.r = ndsWhispyAOTBlendChannel(
            pc->primcolor.r, pc->target_primcolor.r, prim_length);
        pc->primcolor.g = ndsWhispyAOTBlendChannel(
            pc->primcolor.g, pc->target_primcolor.g, prim_length);
        pc->primcolor.b = ndsWhispyAOTBlendChannel(
            pc->primcolor.b, pc->target_primcolor.b, prim_length);
        pc->primcolor.a = ndsWhispyAOTBlendChannel(
            pc->primcolor.a, pc->target_primcolor.a, prim_length);
        pc->primcolor_target_length = prim_length - 1u;
        gNdsWhispyAOTDividesAvoided += 4u;
    }
    if (env_length != 0u)
    {
        pc->envcolor.r = ndsWhispyAOTBlendChannel(
            pc->envcolor.r, pc->target_envcolor.r, env_length);
        pc->envcolor.g = ndsWhispyAOTBlendChannel(
            pc->envcolor.g, pc->target_envcolor.g, env_length);
        pc->envcolor.b = ndsWhispyAOTBlendChannel(
            pc->envcolor.b, pc->target_envcolor.b, env_length);
        pc->envcolor.a = ndsWhispyAOTBlendChannel(
            pc->envcolor.a, pc->target_envcolor.a, env_length);
        pc->envcolor_target_length = env_length - 1u;
        gNdsWhispyAOTDividesAvoided += 4u;
    }
}

/* Route 6's source-equivalent blend kernel. A colour channel already at its
 * target is mathematically unchanged by BattleShip's blend expression; avoid
 * its multiply/shift/store while retaining the proof counter's source-operation
 * meaning. Whispy's terminal fades change alpha only, so three of four channel
 * operations disappear for the busiest portion of each particle lifetime. */
static inline NDS_WHISPY_AOT_INLINE_CODE u32
ndsWhispyAOTApplyBlendsLean(LBParticle *pc,
                            u16 size_length,
                            u16 prim_length,
                            u16 env_length)
{
    u32 avoided = 0u;

    if (size_length != 0u)
    {
        pc->size += (pc->size_target - pc->size) *
                    sNdsWhispySizeReciprocal[size_length];
        pc->size_target_length = size_length - 1u;
        avoided++;
    }
    if (prim_length != 0u)
    {
#define NDS_WHISPY_BLEND_IF_CHANGED(channel) \
        do { \
            if (pc->primcolor.channel != pc->target_primcolor.channel) \
            { \
                pc->primcolor.channel = ndsWhispyAOTBlendChannel( \
                    pc->primcolor.channel, pc->target_primcolor.channel, \
                    prim_length); \
            } \
        } while (0)
        NDS_WHISPY_BLEND_IF_CHANGED(r);
        NDS_WHISPY_BLEND_IF_CHANGED(g);
        NDS_WHISPY_BLEND_IF_CHANGED(b);
        NDS_WHISPY_BLEND_IF_CHANGED(a);
#undef NDS_WHISPY_BLEND_IF_CHANGED
        pc->primcolor_target_length = prim_length - 1u;
        avoided += 4u;
    }
    if (env_length != 0u)
    {
#define NDS_WHISPY_ENV_BLEND_IF_CHANGED(channel) \
        do { \
            if (pc->envcolor.channel != pc->target_envcolor.channel) \
            { \
                pc->envcolor.channel = ndsWhispyAOTBlendChannel( \
                    pc->envcolor.channel, pc->target_envcolor.channel, \
                    env_length); \
            } \
        } while (0)
        NDS_WHISPY_ENV_BLEND_IF_CHANGED(r);
        NDS_WHISPY_ENV_BLEND_IF_CHANGED(g);
        NDS_WHISPY_ENV_BLEND_IF_CHANGED(b);
        NDS_WHISPY_ENV_BLEND_IF_CHANGED(a);
#undef NDS_WHISPY_ENV_BLEND_IF_CHANGED
        pc->envcolor_target_length = env_length - 1u;
        avoided += 4u;
    }
    return avoided;
}

static void ndsWhispyAOTSetPrimAlphaBlend(LBParticle *pc,
                                         u16 length, u8 alpha)
{
    pc->target_primcolor = pc->primcolor;
    pc->target_primcolor.a = alpha;
    pc->primcolor_target_length = length;
}

/* AOT control states are bytecode cursor offsets after the source constructor's
 * immediate first update. The generated-bank hash pins every command byte.
 * Keeping the source cursor/timer/loop fields current makes the route reversible
 * at any frame and gives route 4 an exact same-ROM control. */
static sb32 ndsWhispyAOTAdvanceScript2(LBParticle *pc)
{
    switch (pc->bytecode_csr)
    {
    case 22u:
        pc->frame_id = 1u;
        pc->bytecode_csr = 24u;
        pc->bytecode_timer = 2u;
        return TRUE;
    case 24u:
        pc->frame_id = 2u;
        pc->bytecode_csr = 26u;
        pc->bytecode_timer = 2u;
        return TRUE;
    case 26u:
        pc->frame_id = 3u;
        pc->bytecode_csr = 28u;
        pc->bytecode_timer = 2u;
        return TRUE;
    case 28u:
        if (pc->loop_count == 0u)
        {
            return FALSE;
        }
        pc->loop_count--;
        pc->frame_id = 0u;
        pc->bytecode_timer = 2u;
        if (pc->loop_count != 0u)
        {
            pc->bytecode_csr = 22u;
        }
        else
        {
            ndsWhispyAOTSetPrimAlphaBlend(pc, 17u, 0u);
            pc->loop_count = 2u;
            pc->loop_ptr = 34u;
            pc->bytecode_csr = 36u;
        }
        return TRUE;
    case 36u:
        pc->frame_id = 1u;
        pc->bytecode_csr = 38u;
        pc->bytecode_timer = 2u;
        return TRUE;
    case 38u:
        pc->frame_id = 2u;
        pc->bytecode_csr = 40u;
        pc->bytecode_timer = 2u;
        return TRUE;
    case 40u:
        pc->frame_id = 3u;
        pc->bytecode_csr = 42u;
        pc->bytecode_timer = 2u;
        return TRUE;
    case 42u:
        if (pc->loop_count == 0u)
        {
            return FALSE;
        }
        pc->loop_count--;
        if (pc->loop_count != 0u)
        {
            pc->frame_id = 0u;
            pc->bytecode_csr = 36u;
            pc->bytecode_timer = 2u;
        }
        else
        {
            pc->lifetime = 1u;
            pc->bytecode_csr = 44u;
            pc->bytecode_timer = 0u;
        }
        return TRUE;
    default:
        return FALSE;
    }
}

static sb32 ndsWhispyAOTAdvanceBytecode(
    LBParticle *pc, const NDSWhispyAOTGeneratorDesc *desc)
{
    switch (desc->script_id)
    {
    case 2u:
        return ndsWhispyAOTAdvanceScript2(pc);
    case 3u:
        if (pc->bytecode_csr == 40u)
        {
            ndsWhispyAOTSetPrimAlphaBlend(pc, 11u, 0u);
            pc->frame_id = 0u;
            pc->bytecode_csr = 45u;
            pc->bytecode_timer = 10u;
            return TRUE;
        }
        if (pc->bytecode_csr == 45u)
        {
            pc->lifetime = 1u;
            pc->bytecode_csr = 46u;
            pc->bytecode_timer = 0u;
            return TRUE;
        }
        return FALSE;
    case 4u:
        if (pc->bytecode_csr == 28u)
        {
            ndsWhispyAOTSetPrimAlphaBlend(pc, 9u, 0u);
            pc->frame_id = 0u;
            pc->bytecode_csr = 33u;
            pc->bytecode_timer = 8u;
            return TRUE;
        }
        if (pc->bytecode_csr == 33u)
        {
            pc->lifetime = 1u;
            pc->bytecode_csr = 34u;
            pc->bytecode_timer = 0u;
            return TRUE;
        }
        return FALSE;
    default:
        return FALSE;
    }
}

/* This is the source lbParticleUpdateStruct lifetime-zero branch with the same
 * list, transform-user, free-list and live-count ownership. Whispy's closed
 * scripts cannot carry VORTEX, but retaining that clause makes the copied seam
 * fail safely if its selector is ever widened. */
static LBParticle *ndsWhispyAOTEjectStruct(
    LBParticle *pc, LBParticle *previous, s32 link)
{
    LBParticle *next = pc->next;

    if (previous == NULL)
    {
        sLBParticleStructsAllocLinks[link] = next;
    }
    else
    {
        previous->next = next;
    }
    if ((pc->gn != NULL) &&
        ((pc->flags & LBPARTICLE_FLAG_VORTEX) != 0u) &&
        (pc->gn->kind == nLBParticleKindVortex))
    {
        pc->gn->generator_vars.vortex.lifetime--;
    }
    if (pc->xf != NULL)
    {
        pc->xf->users_num--;
        if (pc->xf->users_num == 0u)
        {
            lbParticleEjectTransform(pc->xf);
            if ((previous == NULL) &&
                (next != sLBParticleStructsAllocLinks[link]))
            {
                next = sLBParticleStructsAllocLinks[link];
            }
        }
    }
    pc->next = sLBParticleStructsAllocFree;
    sLBParticleStructsAllocFree = pc;
    gLBParticleStructsUsedNum--;
    return next;
}

typedef struct NDSWhispyAOTLeanStats
{
    u32 visits;
    u32 fast_updates;
    u32 source_updates;
    u32 divides_avoided;
    u32 direct_updates;
} NDSWhispyAOTLeanStats;

/* Identical closed ownership as route 5, but its proof values live in the ARM
 * runner's locals until the end of the once-per-frame pass. This removes three
 * to four volatile global read/modify/writes from every live particle update. */
static inline NDS_WHISPY_AOT_INLINE_CODE LBParticle *
ndsWhispyAOTUpdateStructLean(LBParticle *pc, LBParticle *previous,
                             s32 link, NDSWhispyAOTLeanStats *stats)
{
    const NDSWhispyAOTGeneratorDesc *desc =
        ndsWhispyAOTDescForBytecode(
            pc->bank_id, pc->texture_id, pc->bytecode);
    u16 size_length;
    u16 prim_length;
    u16 env_length;

    if (desc == NULL)
    {
        return lbParticleUpdateStruct(pc, previous, link);
    }
    stats->visits++;
    if ((pc->flags & LBPARTICLE_FLAG_PAUSE) != 0u)
    {
        stats->fast_updates++;
        return pc->next;
    }

    size_length = pc->size_target_length;
    prim_length = pc->primcolor_target_length;
    env_length = pc->envcolor_target_length;
    {
        const u16 motion_mask = LBPARTICLE_FLAG_GRAVITY |
                                LBPARTICLE_FLAG_FRICTION |
                                LBPARTICLE_FLAG_VORTEX |
                                LBPARTICLE_FLAG_ATTACH;

        if ((pc->lifetime == 0u) || (pc->bytecode_timer == 0u) ||
            (size_length > 17u) || (prim_length > 17u) ||
            (env_length > 17u) ||
            ((pc->flags & motion_mask) != desc->particle_flags))
        {
            stats->source_updates++;
            return lbParticleUpdateStruct(pc, previous, link);
        }
    }
    if (pc->bytecode_timer == 1u)
    {
        if (ndsWhispyAOTAdvanceBytecode(pc, desc) == FALSE)
        {
            stats->source_updates++;
            return lbParticleUpdateStruct(pc, previous, link);
        }
    }
    else
    {
        pc->bytecode_timer--;
    }

    size_length = pc->size_target_length;
    prim_length = pc->primcolor_target_length;
    env_length = pc->envcolor_target_length;
    stats->divides_avoided += ndsWhispyAOTApplyBlendsLean(
        pc, size_length, prim_length, env_length);
    pc->lifetime--;
    stats->direct_updates++;
    stats->fast_updates++;
    if (pc->lifetime == 0u)
    {
        return ndsWhispyAOTEjectStruct(pc, previous, link);
    }

    if ((pc->flags & LBPARTICLE_FLAG_GRAVITY) != 0u)
    {
        pc->vel.y -= pc->gravity;
    }
    if (((pc->flags & LBPARTICLE_FLAG_FRICTION) != 0u) &&
        (desc->script_id != 4u))
    {
        pc->vel.x *= pc->friction;
        pc->vel.y *= pc->friction;
        pc->vel.z *= pc->friction;
    }
    pc->pos.x += pc->vel.x;
    pc->pos.y += pc->vel.y;
    pc->pos.z += pc->vel.z;
    return pc->next;
}

/* Route 5 owns every post-construction update for exact Whispy scripts 2/3/4:
 * fixed control transitions, reciprocal blends, lifetime, pool retirement and
 * their closed gravity/friction motion. Routes 1-4 remain unchanged below as
 * synchronized controls. */
static LBParticle *ndsWhispyAOTUpdateStruct(
    LBParticle *pc, LBParticle *previous, s32 link)
{
    const NDSWhispyAOTGeneratorDesc *desc =
        ndsWhispyAOTDescForBytecode(
            pc->bank_id, pc->texture_id, pc->bytecode);
    u16 size_length = pc->size_target_length;
    u16 prim_length = pc->primcolor_target_length;
    u16 env_length = pc->envcolor_target_length;
    LBParticle *next;

    if (desc == NULL)
    {
        return lbParticleUpdateStruct(pc, previous, link);
    }
    gNdsWhispyAOTStructVisits++;
    if (gNdsWhispyAOTRoute >= 5u)
    {
        const u16 motion_mask = LBPARTICLE_FLAG_GRAVITY |
                                LBPARTICLE_FLAG_FRICTION |
                                LBPARTICLE_FLAG_VORTEX |
                                LBPARTICLE_FLAG_ATTACH;

        if ((pc->flags & LBPARTICLE_FLAG_PAUSE) != 0u)
        {
            gNdsWhispyAOTStructFastUpdates++;
            return pc->next;
        }
        if ((pc->lifetime == 0u) || (pc->bytecode_timer == 0u) ||
            (size_length > 17u) || (prim_length > 17u) ||
            (env_length > 17u) ||
            ((pc->flags & motion_mask) != desc->particle_flags))
        {
            gNdsWhispyAOTStructSourceUpdates++;
            return lbParticleUpdateStruct(pc, previous, link);
        }
        if (pc->bytecode_timer == 1u)
        {
            if (ndsWhispyAOTAdvanceBytecode(pc, desc) == FALSE)
            {
                gNdsWhispyAOTStructSourceUpdates++;
                return lbParticleUpdateStruct(pc, previous, link);
            }
        }
        else
        {
            pc->bytecode_timer--;
        }

        size_length = pc->size_target_length;
        prim_length = pc->primcolor_target_length;
        env_length = pc->envcolor_target_length;
        ndsWhispyAOTApplyBlends(
            pc, size_length, prim_length, env_length);
        pc->lifetime--;
        gNdsWhispyAOTTier2DirectUpdates++;
        gNdsWhispyAOTStructFastUpdates++;
        if (pc->lifetime == 0u)
        {
            return ndsWhispyAOTEjectStruct(pc, previous, link);
        }

        if ((pc->flags & LBPARTICLE_FLAG_GRAVITY) != 0u)
        {
            pc->vel.y -= pc->gravity;
        }
        if (((pc->flags & LBPARTICLE_FLAG_FRICTION) != 0u) &&
            (desc->script_id != 4u))
        {
            pc->vel.x *= pc->friction;
            pc->vel.y *= pc->friction;
            pc->vel.z *= pc->friction;
        }
        pc->pos.x += pc->vel.x;
        pc->pos.y += pc->vel.y;
        pc->pos.z += pc->vel.z;
        return pc->next;
    }
    if ((pc->flags & LBPARTICLE_FLAG_PAUSE) || (pc->lifetime <= 1u) ||
        (pc->bytecode_timer <= 1u) ||
        (size_length > 16u) || (prim_length > 16u) ||
        (env_length > 16u))
    {
        gNdsWhispyAOTStructSourceUpdates++;
        return lbParticleUpdateStruct(pc, previous, link);
    }

    if (gNdsWhispyAOTRoute >= 2u)
    {
        const u16 motion_mask = LBPARTICLE_FLAG_GRAVITY |
                                LBPARTICLE_FLAG_FRICTION |
                                LBPARTICLE_FLAG_VORTEX |
                                LBPARTICLE_FLAG_ATTACH;

        /* A timer above one cannot enter the bytecode interpreter, and a
         * lifetime above one cannot eject. Exact bank/texture/bytecode identity
         * plus the motion-bit check therefore closes this frame to the small
         * kernel below. Presentation flags (ENV colour, alpha blend, masks)
         * deliberately remain source-owned and may coexist with these bits. */
        if ((pc->flags & motion_mask) == desc->particle_flags)
        {
            pc->bytecode_timer--;
            ndsWhispyAOTApplyBlends(
                pc, size_length, prim_length, env_length);
            pc->lifetime--;

            if ((pc->flags & LBPARTICLE_FLAG_GRAVITY) != 0u)
            {
                pc->vel.y -= pc->gravity;
            }
            if (((pc->flags & LBPARTICLE_FLAG_FRICTION) != 0u) &&
                (desc->script_id != 4u))
            {
                pc->vel.x *= pc->friction;
                pc->vel.y *= pc->friction;
                pc->vel.z *= pc->friction;
            }
            /* Script 4's source friction is exactly 1.0F. Omitting its three
             * identity multiplies is bit-exact for finite source velocities. */
            pc->pos.x += pc->vel.x;
            pc->pos.y += pc->vel.y;
            pc->pos.z += pc->vel.z;
            gNdsWhispyAOTTier2DirectUpdates++;
            gNdsWhispyAOTStructFastUpdates++;
            return pc->next;
        }
        gNdsWhispyAOTStructSourceUpdates++;
        return lbParticleUpdateStruct(pc, previous, link);
    }

    if ((size_length | prim_length | env_length) == 0u)
    {
        gNdsWhispyAOTStructSourceUpdates++;
        return lbParticleUpdateStruct(pc, previous, link);
    }

    pc->size_target_length = 0u;
    pc->primcolor_target_length = 0u;
    pc->envcolor_target_length = 0u;
    next = lbParticleUpdateStruct(pc, previous, link);

    ndsWhispyAOTApplyBlends(pc, size_length, prim_length, env_length);
    gNdsWhispyAOTStructFastUpdates++;
    return next;
}

static NDS_WHISPY_AOT_FAST_CODE void
ndsWhispyAOTStructFuncRunLean(GObj *gobj)
{
    NDSWhispyAOTLeanStats stats = { 0u, 0u, 0u, 0u, 0u };
    u32 flags = gobj->flags;
    s32 link;

    for (link = 0; link < ARRAY_COUNT(sLBParticleStructsAllocLinks);
         link++, flags >>= 1)
    {
        LBParticle *previous;
        LBParticle *current;

        if ((flags & 0x10000u) != 0u)
        {
            continue;
        }
        previous = NULL;
        current = sLBParticleStructsAllocLinks[link];
        while (current != NULL)
        {
            LBParticle *next = ndsWhispyAOTUpdateStructLean(
                current, previous, link, &stats);

            if (current->next == next)
            {
                previous = current;
                current = next;
            }
            else
            {
                current = next;
            }
        }
    }

    gNdsWhispyAOTStructVisits += stats.visits;
    gNdsWhispyAOTStructFastUpdates += stats.fast_updates;
    gNdsWhispyAOTStructSourceUpdates += stats.source_updates;
    gNdsWhispyAOTDividesAvoided += stats.divides_avoided;
    gNdsWhispyAOTTier2DirectUpdates += stats.direct_updates;
}

static void ndsWhispyAOTStructFuncRun(GObj *gobj)
{
    u32 flags = gobj->flags;
    s32 link;
#if NDS_TICK_HUD
    u32 tick_start = cpuGetTiming();
#endif

    if (gNdsWhispyAOTRoute >= 6u)
    {
        ndsWhispyAOTStructFuncRunLean(gobj);
#if NDS_TICK_HUD
        gNdsWhispyAOTStructTicks += cpuGetTiming() - tick_start;
#endif
        return;
    }
    if (gNdsWhispyAOTRoute == 0u)
    {
        lbParticleStructFuncRun(gobj);
#if NDS_TICK_HUD
        gNdsWhispyAOTStructTicks += cpuGetTiming() - tick_start;
#endif
        return;
    }
    for (link = 0; link < ARRAY_COUNT(sLBParticleStructsAllocLinks);
         link++, flags >>= 1)
    {
        LBParticle *previous;
        LBParticle *current;

        if ((flags & 0x10000u) != 0u)
        {
            continue;
        }
        previous = NULL;
        current = sLBParticleStructsAllocLinks[link];
        while (current != NULL)
        {
            LBParticle *next =
                ndsWhispyAOTUpdateStruct(current, previous, link);

            if (current->next == next)
            {
                previous = current;
                current = next;
            }
            else
            {
                current = next;
            }
        }
    }
#if NDS_TICK_HUD
    gNdsWhispyAOTStructTicks += cpuGetTiming() - tick_start;
#endif
}
#endif /* NDS_R2_WHISPY_NATIVE_AOT */

/* efcommon's texture ids run 0..46 (PARTICLE_BANK_DISCOVERIES.md). */
#define NDS_PARTICLE_TEXTURE_IDS 47

#define NDS_PARTICLE_LOAD_UNRUN 0u
#define NDS_PARTICLE_LOAD_PASS 1u
#define NDS_PARTICLE_LOAD_REJECT 2u

volatile u32 gNdsParticleBankLoadResult;
volatile u32 gNdsParticleBankBytes;
volatile u32 gNdsParticleBankScriptsPacked;
volatile u32 gNdsParticleBankScriptsUnpacked;
volatile u32 gNdsParticleBankScriptsRejected;
volatile u32 gNdsParticleBankCommands;
volatile u32 gNdsParticleBankFloatOperands;
volatile u32 gNdsParticleBankTextures;
volatile u32 gNdsParticleBankEFCommonID;
volatile u32 gNdsParticleBankOtherID;
/* 0xff until Dream Land registers, so a particle from any other bank can never
 * match it and pick up the Pupupu atlas stride. Bank ids are small and 0 is a
 * legal one, which is why this is not zero-initialised. */
volatile u32 gNdsParticleBankPupupuID = 0xffu;
/* How many of Dream Land's five scripts survived normalization. 0 means the
 * bank registered empty and Whispy is back to silent leaves. */
volatile u32 gNdsParticlePupupuScriptsPacked;
#if NDS_P2_STAGE_YOSTER
/* P2-4 Yoster. 0 means the vapor bank registered empty and the cloud
 * evaporate effect is silent; the gameplay (timers, collision, sink) is the
 * source's and never consults this. */
volatile u32 gNdsParticleBankYosterID = 0xffu;
volatile u32 gNdsParticleYosterScriptsPacked;
#endif

volatile u32 gNdsParticleScriptStartCount;
volatile u32 gNdsParticleGeneratorStartCount;
volatile u32 gNdsParticleRejectCount;
volatile u32 gNdsParticleDrawSeamCount;

volatile u32 gNdsParticleStructsLive;
volatile u32 gNdsParticleStructsMax;
volatile u32 gNdsParticleGeneratorsLive;
volatile u32 gNdsParticleGeneratorsMax;
volatile u32 gNdsParticleTransformsLive;
volatile u32 gNdsParticleTransformsMax;
volatile u32 gNdsParticleRootSpawnCount;

/* One inert texture stands in wherever the pack has no entry for a source
 * texture id, so sLBParticleTextureBanks[bank][id]->flags is always a real
 * read. 1x1, one frame, one shared palette. */
typedef struct NDSParticleInertTexture
{
    LBTextureHeader header;
    void *data[2];
} NDSParticleInertTexture;

static u8 sNdsParticleInertTexel;

static NDSParticleInertTexture sNdsParticleInertTexture = {
    { 1u, 0, 0, 1, 1, 1u },
    { &sNdsParticleInertTexel, &sNdsParticleInertTexel }
};

/* One inert script stands in for every source script id the pack does not
 * carry, so a script id reached from inside the bytecode can never index a
 * different effect. particle_lifetime 0 makes the particle die on its first
 * update, generator_lifetime 1 makes the generator retire on its first pass,
 * size 0 makes both invisible, and the bytecode is a bare END. Its address is
 * also how ndsParticleScriptIsPacked recognizes an unpacked id. */
typedef struct NDSParticleInertScript
{
    LBScriptHeader header;
    u8 bytecode[4];
} NDSParticleInertScript;

static NDSParticleInertScript sNdsParticleInertScript = {
    { 0u, 0u, 1u, 0u, 0u,
      0.0F, 1.0F, { 0.0F, 0.0F, 0.0F }, 0.0F, 0.0F, 0.0F, 0.0F },
    { LBPARTICLE_OPCODE_END, 0u, 0u, 0u }
};

static u32 ndsParticlePopcount4(u32 bits)
{
    u32 count = 0u;

    while (bits != 0u)
    {
        count += bits & 1u;
        bits >>= 1;
    }
    return count;
}

static void ndsParticleSwap16(u8 *at)
{
    u8 b0 = at[0];

    at[0] = at[1];
    at[1] = b0;
}

static void ndsParticleSwap32(u8 *at)
{
    u8 b0 = at[0];
    u8 b1 = at[1];

    at[0] = at[3];
    at[1] = at[2];
    at[2] = b1;
    at[3] = b0;
}

/* LBScript's fixed 0x30-byte prefix: four u16 then TEN 4-byte words
 * (lbtypes.h:79-93) -- flags, gravity, friction, vel.x/y/z, unk_0x20, unk_0x24,
 * update_rate, size. It shipped as nine, and the tenth is `size` at 0x2C.
 *
 * An unswapped 20.0f is not zero and not obviously wrong: 0x41A00000 read back
 * the other way round is 0x0000A041, a positive DENORMAL of 5.7e-41. It passes
 * lbParticleDrawTextures' `pc->size == 0.0F` test, so the particle is fully
 * alive, submits a real quad, costs no QuadMiss, and draws at a size no pixel
 * can hold. Every counter reads clean and nothing appears on screen. That is
 * the whole of "Results confetti pieces do not look like they are large
 * enough": scripts 108-111 carry size 20.0 in the header and set no size
 * opcode, so all four confetti generators emit invisible pieces. Any script
 * that sets its size in BYTECODE instead -- lbpSetSize, lbpSetSizeLerp -- was
 * always correct, which is why the Whispy dust measured a healthy 260.0 and hid
 * this for as long as it did.
 *
 * The static assert is the real fix. The count was a bare literal that had to
 * agree with a struct in another repository's header, and nothing checked it.
 */
_Static_assert(offsetof(LBScript, bytecode) == (8u + (10u * 4u)),
               "LBScript prefix is not four u16 plus ten 4-byte words");

/* `swap` is FALSE on a re-entry, where the bank is normalized already and the
 * walk is only rebuilding the per-scene tables. Swapping twice would swap back;
 * see the one-shot latch in ndsParticleLoadEFCommonBank. */
static void ndsParticleNormalizeHeader(u8 *header, sb32 swap)
{
    u32 i;

    if (swap == FALSE)
    {
        return;
    }
    for (i = 0u; i < 4u; i++)
    {
        ndsParticleSwap16(&header[i * 2u]);
    }
    for (i = 0u; i < 10u; i++)
    {
        ndsParticleSwap32(&header[8u + (i * 4u)]);
    }
}

/* THE RESULTS CONFETTI IS SPARSE BECAUSE ITS SOURCE PARAMETERS MAKE IT SPARSE,
 * and the owner's "confetti should be blanketing the ENTIRE screen" is against
 * what SSB64 shows, not against a broken port. Measured before touching it, so
 * the specialization is aimed rather than hopeful:
 *
 *   efManagerConfettiMakeEffect makes script 0x70 = 112 TWICE (mnvsresults.c
 *   :3216-3217). 112 is a pure spawner -- bytecode `a5 006c a5 006d a5 006e
 *   a5 006f ff` -- so each call creates the four emitters 108..111, and
 *   probe-results-confetti confirms the pair: slot 0 and slot 4 both live,
 *   4 pieces each, size 20, texture 22, gens_used 8. Nothing is missing.
 *
 *   What is sparse is the arrival rate against the fall. Each emitter carries
 *   update_rate 0.07, and lbparticle.c:2324 advances `frame += rand() * rate`
 *   and emits once per whole unit, so that is about one piece every 29 frames
 *   per emitter. Gravity is 4.0 per frame squared, so a piece spawned at
 *   y = 1000 is already past y = -224 when the probe reads it and off screen
 *   within about thirty frames -- long before its 136-frame lifetime. Arrival
 *   0.14/frame against a residency of ~30 frames is the ~20 pieces on screen.
 *
 * Raising the rate is the one lever that adds pieces without touching the
 * physics, the size, or the spawn positions -- all three of which measured
 * correct. PROJECT_GOAL explicitly allows content to be specialized per
 * configuration, and this is a Results-screen cosmetic with no gameplay term.
 *
 * THIS ALSO EXPLAINS WHY RAISING THE POOL ALONE DID NOTHING: at 0.07 the
 * generators never asked for more than 40 structs, so a 112-struct pool sat
 * unused with 0 rejects. The rate and the pool have to move together, and the
 * Results scene claims the larger pool in battleship_mnvsresults.c.
 *
 * SECOND RAISE, 2026-08-02. 0.42 with a 112-struct pool got the owner from "not
 * visible" to "I see its visible now but doesn't cover the whole scene", and the
 * soak said why: StructsLive 112 of StructsMax 112, SATURATED, so the rate was
 * no longer the limit and raising it alone would have done nothing. Both move
 * together, again, for the same reason they had to the first time.
 *
 * THE COST IS FRAMES, NOT BYTES, AND THE OWNER HAS PRICED IT. Measured:
 *
 *   pool/rate    visible   Results present interval
 *   112 / 0.42        62   mostly 2 VBlanks
 *   192 / 0.63       123   mostly 3
 *   384 / 1.26       244   mostly 4, max 8
 *
 * so this is about one extra VBlank per ~91 visible pieces. Memory never
 * mattered -- 384/48/24 is 45,888 bytes (96/92/192 each) against ~300 KB free in
 * this scene, RejectCount 0, MallocOverflow 0, NO-FREEZE at every setting.
 * I backed this down to 192 on the sacrifice order and the owner overruled it:
 * *"for bug fixing, prioritize correctness over FPS. I'll do a separate FPS
 * pass"*. So 384/1.26 stands as the CORRECTNESS setting and the Results cadence
 * cost above is handed to that pass rather than paid for here.
 *
 * ONE MEASUREMENT TRAP, because it nearly cost a wrong conclusion: those three
 * rows are NOT directly comparable out of a freeze soak. Results interval
 * samples were 713, 2543 and 1595 -- the scene was entered at different points
 * and held for wildly different durations, and its early frames are cheaper than
 * its settled ones. Compare Results arms at an equal SOURCE TIC
 * (capture-results-tic.ps1), never across whole-match soaks.
 *
 * SIZE IS THE LEVER THIS BLOCK NEVER PULLED, and it is free. Everything above
 * buys coverage with PIECES, which is why it kept running into the pool and the
 * present interval. Coverage is pieces x area, and area was left at the source's
 * 20.0 through all three raises -- while the owner's own first wording on this
 * row, quoted at the top of probe-results-confetti.ps1, was "confetti pieces do
 * not look like there are large enough". That dimension was in the report from
 * the start and was never measured or moved.
 *
 * 32.0 is 2.56x the area per piece at exactly the same 384 structs, the same
 * 1.26 rate, the same 24 generators and the same fill the DS was already doing
 * -- a particle quad is two triangles whatever its size, so this costs geometry
 * nothing and costs rasterisation only the extra covered pixels. It is a
 * presentation delta with no source value, like the rate above it, and it is
 * recorded as one rather than dressed up as a fix.
 *
 * GRAVITY WAS CONSIDERED AND REJECTED, so nobody re-derives it: residency looks
 * like waste (~30 frames on screen against a 136-frame lifetime) but the
 * measured split is 244 visible of 384 live, so 64% are already on screen and
 * the ceiling from residency alone is about 1.5x. Buying that costs a 16x
 * gravity cut -- residency goes as 1/sqrt(g) -- which turns falling confetti
 * into hanging confetti. Wrong trade for the gain. */
/* BOTH RAISES ARE OFF, 2026-08-03, BY OWNER DECISION -- and the long block
 * above is kept verbatim because its measurements are all still true. What
 * changed is what they are being used for.
 *
 * Three raises took the rate 0.07 -> 0.42 -> 0.63 -> 1.26 and the pool 8 -> 112
 * -> 192 -> 384, then the size 20 -> 32, each because the owner said the
 * confetti still did not cover the scene. The result was twelve times the
 * source's coverage and the answer was still "doesn't look right", which is the
 * signal that the gap is STRUCTURAL and every number above was tuning around
 * it. The owner's instruction on this row is now "read source, apply source
 * behavior to DS", and asked to choose, they picked finding the structural
 * difference over keeping the tuning.
 *
 * So the specialization is neutralized rather than deleted: the identifiers,
 * the counters and the clamp-upward-only contract stay, set to values the
 * source already exceeds, so nothing patches and gNdsConfettiDensityPatchCount
 * and gNdsConfettiSizePatchCount both read 0. Restoring a raise is one constant
 * if the structural search comes back empty.
 *
 * THE OPEN LEAD, so the next cycle does not start from the same three numbers:
 * the Results CAMERA framing has never been compared against source. Every
 * measurement on this row is world-space -- piece count, piece size, fan
 * extent, fall speed -- and all of them are source-exact. If the DS Results
 * camera sits further back than the N64's, source-exact confetti covers less of
 * the screen and no amount of pieces fixes the framing. That is the one
 * dimension the contract never had. */
#define NDS_R2_CONFETTI_FIRST_SCRIPT 108u
#define NDS_R2_CONFETTI_LAST_SCRIPT 111u
#define NDS_R2_CONFETTI_UPDATE_RATE 0.0F
#define NDS_R2_CONFETTI_PIECE_SIZE 0.0F

/* Counts the emitters this build raised. Four is the whole set; anything else
 * means the bank moved out from under the id range. */
volatile u32 gNdsConfettiDensityPatchCount;
volatile u32 gNdsConfettiSizePatchCount;

static void ndsParticleApplyConfettiDensity(u32 id, LBScript *script)
{
    if ((id < NDS_R2_CONFETTI_FIRST_SCRIPT) ||
        (id > NDS_R2_CONFETTI_LAST_SCRIPT) || (script == NULL))
    {
        return;
    }
    /* Only ever upward, and only from the value the source actually carries.
     * Clamping to a raise means a re-render that changes the script cannot be
     * silently overridden downward by this. */
    if (script->update_rate < NDS_R2_CONFETTI_UPDATE_RATE)
    {
        script->update_rate = NDS_R2_CONFETTI_UPDATE_RATE;
        gNdsConfettiDensityPatchCount++;
    }
    /* Same clamp-upward-only contract as the rate, and counted separately so a
     * bank whose size already exceeds this is distinguishable from one this
     * never reached. probe-results-confetti reports maxsize, so the raise is
     * checkable without a capture. */
    if (script->size < NDS_R2_CONFETTI_PIECE_SIZE)
    {
        script->size = NDS_R2_CONFETTI_PIECE_SIZE;
        gNdsConfettiSizePatchCount++;
    }
}

/* Walks one script's bytecode exactly as lbParticleUpdateStruct decodes it,
 * byte-swapping every f32 operand in place. Returns FALSE if the stream runs
 * past its limit or carries an opcode the interpreter has no case for, so a
 * malformed script is rejected rather than executed. */
static sb32 ndsParticleNormalizeBytecode(u8 *bytecode, u32 limit,
                                         u32 *commands, u32 *operands,
                                         sb32 swap)
{
    u32 csr = 0u;

    while (csr < limit)
    {
        u8 command = bytecode[csr++];
        u32 opcode;
        u32 floats = 0u;
        u32 bytes = 0u;
        u32 ushorts = 0u;
        u32 i;

        (*commands)++;

        if (command < 0x80u)
        {
            /* Wait: low five bits are the frame count, 0x20 extends it with a
             * second byte, 0x40 carries a new texture frame id. */
            if ((command & 0x20u) != 0u)
            {
                bytes++;
            }
            if ((command & 0xC0u) == 0x40u)
            {
                bytes++;
            }
            csr += bytes;
            continue;
        }

        opcode = command & 0xF8u;
        if (opcode > 0x98u)
        {
            opcode = command & 0xF0u;
            if ((opcode != 0xC0u) && (opcode != 0xD0u))
            {
                opcode = command;
            }
        }

        switch (opcode)
        {
        case LBPARTICLE_OPCODE_SETPOS:
        case LBPARTICLE_OPCODE_ADDPOS:
        case LBPARTICLE_OPCODE_SETVEL:
        case LBPARTICLE_OPCODE_ADDVEL:
            floats = ndsParticlePopcount4(command & 7u);
            break;

        case LBPARTICLE_OPCODE_SETSIZELERP:
            ushorts = 1u;
            floats = 1u;
            break;

        case LBPARTICLE_OPCODE_SETFLAG:
        case LBPARTICLE_OPCODE_TRYDEADRAND:
        case LBPARTICLE_OPCODE_SETDISTVEL:
        case LBPARTICLE_OPCODE_SETATTACHID:
        case LBPARTICLE_OPCODE_SETLOOP:
            bytes = 1u;
            break;

        case LBPARTICLE_OPCODE_SETGRAVITY:
        case LBPARTICLE_OPCODE_SETFRICTION:
        case LBPARTICLE_OPCODE_SETVELANGLE:
        case LBPARTICLE_OPCODE_MULVELUFORM:
            floats = 1u;
            break;

        case LBPARTICLE_OPCODE_MAKESCRIPT:
        case LBPARTICLE_OPCODE_MAKEGENERATOR:
        case LBPARTICLE_OPCODE_MAKEID:
        case 0xBCu: /* set frame id + random spread */
            bytes = 2u;
            break;

        case LBPARTICLE_OPCODE_SETLIFERAND:
        case LBPARTICLE_OPCODE_MAKERAND:
        case LBPARTICLE_OPCODE_PRIMBLENDRAND:
        case LBPARTICLE_OPCODE_ENVBLENDRAND:
            bytes = 4u;
            break;

        case LBPARTICLE_OPCODE_ADDVELRAND:
        case LBPARTICLE_OPCODE_MULVELAXIS:
            floats = 3u;
            break;

        case LBPARTICLE_OPCODE_SETSIZERAND:
            ushorts = 1u;
            floats = 2u;
            break;

        case LBPARTICLE_OPCODE_ENVCOLOR:
        case LBPARTICLE_OPCODE_NOMASKST:
        case LBPARTICLE_OPCODE_MASKS:
        case LBPARTICLE_OPCODE_MASKT:
        case LBPARTICLE_OPCODE_MASKST:
        case LBPARTICLE_OPCODE_ALPHABLEND:
        case LBPARTICLE_OPCODE_NODITHER:
        case LBPARTICLE_OPCODE_DITHER:
        case LBPARTICLE_OPCODE_NONOISE:
        case LBPARTICLE_OPCODE_NOISE:
        case LBPARTICLE_OPCODE_LOOP:
        case LBPARTICLE_OPCODE_SETRETURN:
        case LBPARTICLE_OPCODE_RETURN:
            break;

        case LBPARTICLE_OPCODE_ADDDISTVELMAG:
            bytes = 1u;
            floats = 1u;
            break;

        case LBPARTICLE_OPCODE_SETVELMAG:
            floats = 2u;
            break;

        case LBPARTICLE_OPCODE_SETPRIMBLEND:
        case LBPARTICLE_OPCODE_SETENVBLEND:
            ushorts = 1u;
            bytes = ndsParticlePopcount4(command & 0xFu);
            break;

        case LBPARTICLE_OPCODE_DEAD:
        case LBPARTICLE_OPCODE_END:
            return TRUE;

        default:
            return FALSE;
        }

        /* lbParticleReadUShort: one byte, or two when the high bit is set. */
        for (i = 0u; i < ushorts; i++)
        {
            if (csr >= limit)
            {
                return FALSE;
            }
            if ((bytecode[csr++] & 0x80u) != 0u)
            {
                csr++;
            }
        }
        csr += bytes;
        for (i = 0u; i < floats; i++)
        {
            if ((csr + 4u) > limit)
            {
                return FALSE;
            }
            if (swap != FALSE)
            {
                ndsParticleSwap32(&bytecode[csr]);
            }
            csr += 4u;
            (*operands)++;
        }
        if (csr > limit)
        {
            return FALSE;
        }
    }
    /* Ran off the end without DEAD or END. */
    return FALSE;
}

/* Smallest packed offset strictly greater than `offset`, or the bank size. The
 * packed set need not be dense, so the next script in file order is found by
 * scan rather than by id. */
static u32 ndsParticleNextOffset(u32 offset, u32 bank_bytes)
{
    u32 limit = bank_bytes;
    u32 id;

    for (id = 0u; id < NDS_PARTICLE_SCRIPT_COUNT; id++)
    {
        u32 candidate = gNdsParticleScriptOffsets[id];

        if ((candidate != NDS_PARTICLE_SCRIPT_UNREACHABLE) &&
            (candidate > offset) && (candidate < limit))
        {
            limit = candidate;
        }
    }
    return limit;
}

static sb32 ndsParticleLoadEFCommonBank(s32 bank_id)
{
    u32 bank_bytes = gNdsParticleScriptBankBytes;
    u8 *bank;
    LBScript **scripts;
    LBTexture **textures;
    u32 id;

    gNdsParticleBankBytes = bank_bytes;
    gNdsParticleBankTextures = gNdsParticleTextureCount;

    scripts = syTaskmanMalloc(sizeof(*scripts) * NDS_PARTICLE_SCRIPT_COUNT, 0x4);
    textures = syTaskmanMalloc(sizeof(*textures) * NDS_PARTICLE_TEXTURE_IDS,
                               0x4);
    if ((scripts == NULL) || (textures == NULL))
    {
        gNdsParticleBankLoadResult = NDS_PARTICLE_LOAD_REJECT;
        return FALSE;
    }

    for (id = 0u; id < NDS_PARTICLE_TEXTURE_IDS; id++)
    {
        textures[id] = (LBTexture *)&sNdsParticleInertTexture;
    }
    for (id = 0u; (id < gNdsParticleTextureCount) &&
                  (id < NDS_PARTICLE_TEXTURE_IDS); id++)
    {
        /* Only the fields the interpreter reads are populated here. The image
         * and palette pointers stay inert until the gated DS draw step wires
         * gNdsParticleTextureData / gNdsParticlePaletteData. */
        NDSParticleInertTexture *entry =
            syTaskmanMalloc(sizeof(*entry), 0x4);

        if (entry == NULL)
        {
            gNdsParticleBankLoadResult = NDS_PARTICLE_LOAD_REJECT;
            return FALSE;
        }
        *entry = sNdsParticleInertTexture;
        entry->header.width = (s32)gNdsParticleTextures[id].width;
        entry->header.height = (s32)gNdsParticleTextures[id].height;
        textures[id] = (LBTexture *)entry;
    }

    for (id = 0u; id < NDS_PARTICLE_SCRIPT_COUNT; id++)
    {
        scripts[id] = (LBScript *)&sNdsParticleInertScript;
    }

    /* NORMALIZE THE LINKED BANK IN PLACE. There used to be a
     * syTaskmanMalloc(bank_bytes) plus a memcpy here, and the copy cost 10,912
     * bytes of taskman arena for a second image of bytes that were already in
     * RAM -- the linked array is main RAM like everything else on this machine,
     * so the only thing the copy bought was somewhere writable to byte-swap
     * into. Making gNdsParticleScriptBank non-const buys that for nothing, and
     * 10,912 is most of what stood between this runtime and booting: the
     * general heap reached 25,600 free with it and 14,756 without.
     *
     * ONE-SHOT, and that is not optional. The normalizer swaps big-endian
     * fields to little in place, so running it twice swaps them back. The old
     * copy was implicitly one-shot because each scene entry got a fresh buffer;
     * an in-place pass over storage that outlives the scene is not
     * (SwitchPlan 3.12 -- anything surviving a scene boundary is re-derived,
     * never trusted). The latch is file-static and never cleared, because the
     * bank is linked data whose normalized form is correct for every entry. */
    static sb32 sNdsParticleBankNormalized = FALSE;
    sb32 swap;

    bank = NULL;
    swap = FALSE;
    if (bank_bytes >= sizeof(LBScriptHeader))
    {
        bank = gNdsParticleScriptBank;
        swap = (sNdsParticleBankNormalized == FALSE) ? TRUE : FALSE;
        sNdsParticleBankNormalized = TRUE;
        /* Re-entry still walks every script: the arena rewind took the previous
         * entry's scripts[] table with it, so the table is rebuilt even though
         * the bytes behind it are already little-endian. Reset the running
         * totals so they describe this pass rather than accumulating. */
        gNdsParticleBankScriptsPacked = 0u;
        gNdsParticleBankScriptsUnpacked = 0u;
        gNdsParticleBankScriptsRejected = 0u;
        gNdsParticleBankCommands = 0u;
        gNdsParticleBankFloatOperands = 0u;
    }

    for (id = 0u; id < NDS_PARTICLE_SCRIPT_COUNT; id++)
    {
        u32 offset = gNdsParticleScriptOffsets[id];
        u32 limit;
        u32 commands = 0u;
        u32 operands = 0u;
        u8 *header;

        if (offset == NDS_PARTICLE_SCRIPT_UNREACHABLE)
        {
            gNdsParticleBankScriptsUnpacked++;
            continue;
        }
        if ((bank == NULL) || ((offset & 3u) != 0u) ||
            (offset > bank_bytes) ||
            ((bank_bytes - offset) < sizeof(LBScriptHeader)))
        {
            gNdsParticleBankScriptsRejected++;
            continue;
        }
        limit = ndsParticleNextOffset(offset, bank_bytes);
        if (limit < (offset + (u32)sizeof(LBScriptHeader)))
        {
            gNdsParticleBankScriptsRejected++;
            continue;
        }
        header = &bank[offset];
        ndsParticleNormalizeHeader(header, swap);
        ndsParticleApplyConfettiDensity(id, (LBScript *)header);
        if (ndsParticleNormalizeBytecode(
                header + sizeof(LBScriptHeader),
                limit - offset - (u32)sizeof(LBScriptHeader),
                &commands, &operands, swap) == FALSE)
        {
            gNdsParticleBankScriptsRejected++;
            continue;
        }
        if (((LBScript *)header)->texture_id >= NDS_PARTICLE_TEXTURE_IDS)
        {
            gNdsParticleBankScriptsRejected++;
            continue;
        }
        scripts[id] = (LBScript *)header;
        gNdsParticleBankScriptsPacked++;
        gNdsParticleBankCommands += commands;
        gNdsParticleBankFloatOperands += operands;
    }

    sLBParticleScriptBanksNum[bank_id] = NDS_PARTICLE_SCRIPT_COUNT;
    sLBParticleTextureBanksNum[bank_id] = NDS_PARTICLE_TEXTURE_IDS;
    sLBParticleScriptBanks[bank_id] = scripts;
    sLBParticleTextureBanks[bank_id] = textures;

    gNdsParticleBankLoadResult = (gNdsParticleBankScriptsPacked != 0u)
                                     ? NDS_PARTICLE_LOAD_PASS
                                     : NDS_PARTICLE_LOAD_REJECT;
    return TRUE;
}

/* Dream Land's bank. Same shape as the common loader above and deliberately a
 * separate function rather than a parameterised one: the two differ in the
 * symbols they read and in nothing else, and folding them would put a pointer
 * indirection on a path that runs once per scene entry to save twenty lines.
 *
 * BUGS.md wind row: without this, `grPupupuWhispyLeavesMakeEffect` (script 0)
 * and `grPupupuWhispyDustMakeEffect` (script 1) hit `ndsParticleRegisterEmptyBank`
 * and failed closed at reject reason 2 -- measured, twice each, on the
 * 2026-08-01 both-CPU soak's reject ring. */
static sb32 ndsParticleLoadPupupuBank(s32 bank_id)
{
    static sb32 sNdsPupupuBankNormalized = FALSE;
    u32 bank_bytes = gNdsPupupuScriptBankBytes;
    LBScript **scripts;
    LBTexture **textures;
    sb32 swap;
    u32 id;
    u32 packed = 0u;

    scripts = syTaskmanMalloc(sizeof(*scripts) * NDS_PUPUPU_SCRIPT_COUNT, 0x4);
    textures = syTaskmanMalloc(sizeof(*textures) * NDS_PUPUPU_TEXTURE_COUNT,
                               0x4);
    if ((scripts == NULL) || (textures == NULL))
    {
        return FALSE;
    }

    for (id = 0u; id < NDS_PUPUPU_TEXTURE_COUNT; id++)
    {
        NDSParticleInertTexture *entry = syTaskmanMalloc(sizeof(*entry), 0x4);

        if (entry == NULL)
        {
            return FALSE;
        }
        *entry = sNdsParticleInertTexture;
        entry->header.width = (s32)gNdsPupupuTextures[id].width;
        entry->header.height = (s32)gNdsPupupuTextures[id].height;
        textures[id] = (LBTexture *)entry;
    }

    /* One-shot for the same reason the common bank's latch is: the normalizer
     * swaps in place over linked storage that outlives the scene, so a second
     * pass would swap it back. */
    swap = (sNdsPupupuBankNormalized == FALSE) ? TRUE : FALSE;
    sNdsPupupuBankNormalized = TRUE;

    for (id = 0u; id < NDS_PUPUPU_SCRIPT_COUNT; id++)
    {
        u32 offset = gNdsPupupuScriptOffsets[id];
        u32 limit;
        u32 commands = 0u;
        u32 operands = 0u;
        u8 *header;

        scripts[id] = (LBScript *)&sNdsParticleInertScript;
        if (((offset & 3u) != 0u) || (offset > bank_bytes) ||
            ((bank_bytes - offset) < sizeof(LBScriptHeader)))
        {
            continue;
        }
        limit = (id + 1u < NDS_PUPUPU_SCRIPT_COUNT)
                    ? gNdsPupupuScriptOffsets[id + 1u]
                    : bank_bytes;
        if (limit < (offset + (u32)sizeof(LBScriptHeader)))
        {
            continue;
        }
        header = &gNdsPupupuScriptBank[offset];
        ndsParticleNormalizeHeader(header, swap);
        if (ndsParticleNormalizeBytecode(
                header + sizeof(LBScriptHeader),
                limit - offset - (u32)sizeof(LBScriptHeader),
                &commands, &operands, swap) == FALSE)
        {
            continue;
        }
        if (((LBScript *)header)->texture_id >= NDS_PUPUPU_TEXTURE_COUNT)
        {
            continue;
        }
        scripts[id] = (LBScript *)header;
        packed++;
    }

    sLBParticleScriptBanksNum[bank_id] = NDS_PUPUPU_SCRIPT_COUNT;
    sLBParticleTextureBanksNum[bank_id] = NDS_PUPUPU_TEXTURE_COUNT;
    sLBParticleScriptBanks[bank_id] = scripts;
    sLBParticleTextureBanks[bank_id] = textures;
    gNdsParticlePupupuScriptsPacked = packed;
    return (packed != 0u) ? TRUE : FALSE;
}

#if NDS_P2_STAGE_YOSTER
/* P2-4 Yoster Island's cloud-vapor bank. Same shape as ndsParticleLoadPupupuBank
 * above and deliberately a separate function for the same reason: the two differ
 * in the symbols they read and in nothing else. One script (script 0,
 * grYosterCloudVaporMakeEffect, decomp gr/grcommon/gryoster.c) drawing one
 * 32x32 single-frame texture; dims come from the emitted u8 triple, not from a
 * header struct the generated header does not carry this cycle. */
static sb32 ndsParticleLoadYosterBank(s32 bank_id)
{
    static sb32 sNdsYosterBankNormalized = FALSE;
    u32 bank_bytes = gNdsYosterScriptBankBytes;
    LBScript **scripts;
    LBTexture **textures;
    sb32 swap;
    u32 packed = 0u;
    NDSParticleInertTexture *entry;

    /* The emission pins exactly one script and one texture; anything else is a
     * generator drift the loader must not silently reinterpret. */
    if ((gNdsYosterScriptOffsets[0] != 8u) ||
        (gNdsYosterTextureDims[0] != 32u) ||
        (gNdsYosterTextureDims[1] != 32u) ||
        (gNdsYosterTextureDims[2] != 1u))
    {
        return FALSE;
    }

    scripts = syTaskmanMalloc(sizeof(*scripts) * 1u, 0x4);
    textures = syTaskmanMalloc(sizeof(*textures) * 1u, 0x4);
    entry = syTaskmanMalloc(sizeof(*entry), 0x4);
    if ((scripts == NULL) || (textures == NULL) || (entry == NULL))
    {
        return FALSE;
    }

    *entry = sNdsParticleInertTexture;
    entry->header.width = 32;
    entry->header.height = 32;
    textures[0] = (LBTexture *)entry;

    /* One-shot for the same reason the Pupupu latch is: the normalizer swaps in
     * place over linked storage that outlives the scene, so a second pass would
     * swap it back. */
    swap = (sNdsYosterBankNormalized == FALSE) ? TRUE : FALSE;
    sNdsYosterBankNormalized = TRUE;

    scripts[0] = (LBScript *)&sNdsParticleInertScript;
    {
        u32 offset = gNdsYosterScriptOffsets[0];
        u32 commands = 0u;
        u32 operands = 0u;
        u8 *header;

        if (((offset & 3u) == 0u) && (offset <= bank_bytes) &&
            ((bank_bytes - offset) >= sizeof(LBScriptHeader)))
        {
            header = &gNdsYosterScriptBank[offset];
            ndsParticleNormalizeHeader(header, swap);
            if ((ndsParticleNormalizeBytecode(
                     header + sizeof(LBScriptHeader),
                     bank_bytes - offset - (u32)sizeof(LBScriptHeader),
                     &commands, &operands, swap) != FALSE) &&
                (((LBScript *)header)->texture_id < 1u))
            {
                scripts[0] = (LBScript *)header;
                packed++;
            }
        }
    }

    sLBParticleScriptBanksNum[bank_id] = 1;
    sLBParticleTextureBanksNum[bank_id] = 1;
    sLBParticleScriptBanks[bank_id] = scripts;
    sLBParticleTextureBanks[bank_id] = textures;
    gNdsParticleYosterScriptsPacked = packed;
    return (packed != 0u) ? TRUE : FALSE;
}
#endif

/* Registers a bank with no scripts at all. Every lookup against it fails the
 * source's own `script_id >= sLBParticleScriptBanksNum[id]` test, so an
 * unpacked bank can never resolve into another bank's scripts. */
static void ndsParticleRegisterEmptyBank(s32 bank_id)
{
    sLBParticleScriptBanksNum[bank_id] = 0;
    sLBParticleTextureBanksNum[bank_id] = 0;
    sLBParticleScriptBanks[bank_id] = NULL;
    sLBParticleTextureBanks[bank_id] = NULL;
}

s32 efParticleGetLoadBankID(uintptr_t scripts_lo, uintptr_t scripts_hi,
                            uintptr_t textures_lo, uintptr_t textures_hi)
{
    s32 bank_id = efParticleGetBankID(scripts_lo);

    (void)scripts_hi;
    (void)textures_lo;
    (void)textures_hi;

    if (bank_id != -1)
    {
        return bank_id;
    }
    if ((sEFParticleBanksNum < 0) ||
        (sEFParticleBanksNum >= ARRAY_COUNT(sEFParticleScriptBanks)) ||
        (sEFParticleBanksNum >= LBPARTICLE_BANKS_NUM_MAX))
    {
        gNdsParticleRejectCount++;
        return -1;
    }
    bank_id = sEFParticleBanksNum;

    if (scripts_lo == (uintptr_t)&lEFCommonParticleScriptBankLo)
    {
        if (ndsParticleLoadEFCommonBank(bank_id) == FALSE)
        {
            ndsParticleRegisterEmptyBank(bank_id);
        }
        gNdsParticleBankEFCommonID = (u32)bank_id;
    }
    else if (scripts_lo == (uintptr_t)&lGRPupupuParticleScriptBankLo)
    {
        /* Dream Land's. Registered by symbol rather than by scene, because the
         * scene test below fires for ANY non-common bank in a Pupupu battle and
         * would happily hand this loader someone else's script ids. */
        if (ndsParticleLoadPupupuBank(bank_id) == FALSE)
        {
            ndsParticleRegisterEmptyBank(bank_id);
        }
        else
        {
            gNdsParticleBankPupupuID = (u32)bank_id;
        }
        gNdsParticleBankOtherID = (u32)bank_id;
        gNdsPupupuGroundDeferredMask |= 1u << 1;
        gNdsPupupuGroundSetupMask |= 1u << 9;
        gNdsPupupuGroundParticleBankID = (u32)bank_id;
    }
#if NDS_P2_STAGE_YOSTER
    else if (scripts_lo == (uintptr_t)&lGRYosterParticleScriptBankLo)
    {
        /* P2-4 Yoster vapor. Registered by symbol for the same reason as
         * Pupupu's above: the scene test below would happily hand this loader
         * someone else's script ids. Flag off, this arm does not exist and the
         * vapor bank lands in the empty-bank arm below (silent vapor). */
        if (ndsParticleLoadYosterBank(bank_id) == FALSE)
        {
            ndsParticleRegisterEmptyBank(bank_id);
        }
        else
        {
            gNdsParticleBankYosterID = (u32)bank_id;
        }
        gNdsParticleBankOtherID = (u32)bank_id;
    }
#endif
    else
    {
        ndsParticleRegisterEmptyBank(bank_id);
        gNdsParticleBankOtherID = (u32)bank_id;
        /* Any fight on Dream Land (BATTLE flag, not the VS kind; 2026-09-05). */
        if ((gNdsSceneManagerCurrIsBattle != 0u) &&
            (gSCManagerBattleState != NULL) &&
            (gSCManagerBattleState->gkind == nGRKindPupupu))
        {
            gNdsPupupuGroundDeferredMask |= 1u << 1;
            gNdsPupupuGroundSetupMask |= 1u << 9;
            gNdsPupupuGroundParticleBankID = (u32)bank_id;
        }
    }

    sEFParticleScriptBanks[bank_id] = scripts_lo;
    sEFParticleBanksNum++;
    gNdsParticleBankRegisterCount++;

    return bank_id;
}

/* WHICH script was refused, not just how many. gNdsParticleRejectCount alone
 * cannot tell "the pack's reachable set is missing a script a real match asks
 * for" from "an unreachable id was correctly failed closed", and those have
 * opposite fixes -- the first is a hole in the derivation, the second is the
 * design working. Same shape as the FGM miss ring, and added for the same
 * reason: the first run with the runtime alive reported 18 rejects and zero
 * script starts, and the count said nothing about why. Reason codes:
 *   1 bank id out of range      3 bank never registered
 *   2 script id out of range    4 id is UNREACHABLE in the pack (fail-closed) */
#define NDS_PARTICLE_REJECT_RING_CAPACITY 12u
volatile u32 gNdsParticleRejectRingCount;
volatile u16 gNdsParticleRejectRingScripts[NDS_PARTICLE_REJECT_RING_CAPACITY];
volatile u8 gNdsParticleRejectRingBanks[NDS_PARTICLE_REJECT_RING_CAPACITY];
volatile u8 gNdsParticleRejectRingReasons[NDS_PARTICLE_REJECT_RING_CAPACITY];
volatile u16 gNdsParticleRejectRingCounts[NDS_PARTICLE_REJECT_RING_CAPACITY];

static void ndsParticleRecordReject(s32 bank_id, s32 script_id, u32 reason)
{
    u32 i;

    for (i = 0u; i < gNdsParticleRejectRingCount; i++)
    {
        if ((gNdsParticleRejectRingScripts[i] == (u16)script_id) &&
            (gNdsParticleRejectRingBanks[i] == (u8)bank_id))
        {
            gNdsParticleRejectRingCounts[i]++;
            return;
        }
    }
    if (gNdsParticleRejectRingCount >= NDS_PARTICLE_REJECT_RING_CAPACITY)
    {
        return;
    }
    i = gNdsParticleRejectRingCount;
    gNdsParticleRejectRingScripts[i] = (u16)script_id;
    gNdsParticleRejectRingBanks[i] = (u8)bank_id;
    gNdsParticleRejectRingReasons[i] = (u8)reason;
    gNdsParticleRejectRingCounts[i] = 1u;
    gNdsParticleRejectRingCount++;
}

/* TRUE when this bank/script pair resolves to a real packed script. The source
 * range test is repeated here on purpose: it has to run before the constructor,
 * because the constructor's own miss path reads through the bank array. */
static sb32 ndsParticleScriptIsPacked(s32 bank_id, s32 script_id)
{
    s32 id = bank_id & 7;

    if (id >= LBPARTICLE_BANKS_NUM_MAX)
    {
        ndsParticleRecordReject(bank_id, script_id, 1u);
        return FALSE;
    }
    if ((script_id < 0) || (script_id >= sLBParticleScriptBanksNum[id]))
    {
        ndsParticleRecordReject(bank_id, script_id, 2u);
        return FALSE;
    }
    if (sLBParticleScriptBanks[id] == NULL)
    {
        ndsParticleRecordReject(bank_id, script_id, 3u);
        return FALSE;
    }
    if (sLBParticleScriptBanks[id][script_id] ==
        (LBScript *)&sNdsParticleInertScript)
    {
        ndsParticleRecordReject(bank_id, script_id, 4u);
        return FALSE;
    }
    return TRUE;
}

LBParticle *lbParticleMakeScriptID(s32 bank_id, s32 script_id)
{
    LBParticle *pc;

    if (ndsParticleScriptIsPacked(bank_id, script_id) == FALSE)
    {
        gNdsParticleRejectCount++;
        return NULL;
    }
    pc = ndsBaseLbParticleMakeScriptID(bank_id, script_id);
    if (pc != NULL)
    {
        gNdsParticleScriptStartCount++;
    }
    return pc;
}

LBParticle *lbParticleMakeCommon(s32 bank_id, s32 script_id)
{
    LBParticle *pc;

    if (ndsParticleScriptIsPacked(bank_id, script_id) == FALSE)
    {
        gNdsParticleRejectCount++;
        return NULL;
    }
    pc = ndsBaseLbParticleMakeCommon(bank_id, script_id);
    if (pc != NULL)
    {
        gNdsParticleScriptStartCount++;
    }
    return pc;
}

LBParticle *lbParticleMakePosVel(s32 bank_id, s32 script_id, f32 pos_x,
                                 f32 pos_y, f32 pos_z, f32 vel_x, f32 vel_y,
                                 f32 vel_z)
{
    LBParticle *pc;

    if (ndsParticleScriptIsPacked(bank_id, script_id) == FALSE)
    {
        gNdsParticleRejectCount++;
        return NULL;
    }
    pc = ndsBaseLbParticleMakePosVel(bank_id, script_id, pos_x, pos_y, pos_z,
                                     vel_x, vel_y, vel_z);
    if (pc != NULL)
    {
        gNdsParticleScriptStartCount++;
    }
    return pc;
}

LBGenerator *lbParticleMakeGenerator(s32 bank_id, s32 script_id)
{
    LBGenerator *gn;

    if (ndsParticleScriptIsPacked(bank_id, script_id) == FALSE)
    {
        gNdsParticleRejectCount++;
        return NULL;
    }
    gn = ndsBaseLbParticleMakeGenerator(bank_id, script_id);
    if (gn != NULL)
    {
        gNdsParticleGeneratorStartCount++;
    }
    return gn;
}

void ndsParticleRuntimePublishTallies(void)
{
    gNdsParticleStructsLive = gLBParticleStructsUsedNum;
    gNdsParticleGeneratorsLive = gLBParticleGeneratorsUsedNum;
    gNdsParticleTransformsLive = gLBParticleTransformsUsedNum;
    gNdsParticleStructsMax = D_ovl0_800D644E;
    gNdsParticleGeneratorsMax = D_ovl0_800D6450;
    gNdsParticleTransformsMax = D_ovl0_800D6452;
    gNdsParticleRootSpawnCount = dLBParticleCurrentGeneratorID;
}

/* THE DS TEXTURED-QUAD DRAW.
 *
 * The source (lb/lbparticle.c:1448) projects each particle to NDC, turns
 * `pc->size` into a screen-space half-extent through the projection's column
 * magnitudes, and emits an axis-aligned N64 texture rectangle. This does the
 * same arithmetic and then emits a camera-facing quad instead, which is the DS
 * hardware's native shape and costs no projection swap: the source's NDC
 * half-extent is `size * |P col| / w`, and a WORLD-space quad of half-extent
 * `size` projects to exactly that under the same matrix. So the billboard is
 * the rectangle, expressed where the geometry engine already is.
 *
 * ONE BIND FOR THE WHOLE FRAME. Every admitted texture lives in one atlas, so
 * the texture never changes across particles and the triangle batch is never
 * broken by them -- which is what makes 41 quads affordable against a gate
 * that has no headroom. The source sorts by image to minimise RDP tile loads;
 * there is nothing here to sort.
 *
 * Fails closed at every step: no atlas name, no draw; a texture with no atlas
 * row, no draw. A particle never draws the wrong image (docs/BUGS.md
 * Coin->Sparkle, Slash->HitNormal). */
volatile u32 gNdsParticleTextureUseMask[2];
volatile u8 gNdsParticleTextureFrameMax[NDS_PARTICLE_TEXTURE_USE_IDS];
volatile u32 gNdsParticleDrawVisibleCount;
volatile u32 gNdsParticleDrawVisibleMax;
volatile u32 gNdsParticleQuadEmitCount;
volatile u32 gNdsParticleQuadEmitMax;
volatile u32 gNdsParticleMirrorSSubmitCount;
volatile u32 gNdsParticleMirrorTSubmitCount;
volatile u32 gNdsParticleMirrorSTSubmitCount;
/* TEMPORARY, BUGS.md row 1. Whispy's dust and leaves are alloc-link 1; these
 * record what its quads are actually handed at the submit. Remove with the row. */
volatile u32 gNdsWhispySubmitOk;
volatile u32 gNdsWhispySubmitFail;
volatile u32 gNdsWhispyDrawClamped;
volatile f32 gNdsWhispyDrawX;
volatile f32 gNdsWhispyDrawY;
volatile f32 gNdsWhispyDrawSize;

/* The confetti spread probe that answered BUGS.md's "move as a unit" row lived
 * here and is removed now that it has: over the results scene, Y spread grew
 * 690.0 -> 2817.4 world units while the centroid fell 1146.0 -> -26.6, which is
 * free independent fall and not a rigid cloud. The numbers are in BUGS.md.
 * If it ever needs rebuilding: accumulate min/max/sum of world_pos per draw-seam
 * call, gate on gNdsVSResultsTickCount so battle effects cannot pollute it, and
 * compare the FIRST spread against the LAST -- spread alone proves nothing,
 * because rigid pieces still sit apart. */

volatile u32 gNdsParticleQuadMissCount;
volatile u32 gNdsParticleInitAllCount;
volatile u32 gNdsParticleBankRegisterCount;
volatile u32 gNdsParticleQuadMissMask[2];
volatile u32 gNdsParticleQuadMissFrameMask;
volatile u32 gNdsParticleQuadStrideCount;
volatile u32 gNdsWhispyNativeTextureDrawCount;
volatile u32 gNdsWhispyNativeTextureMissCount;
volatile u32 gNdsWhispyNativeTextureMask;
volatile u32 gNdsWhispyNativeSourceFrameMask;

/* Atlas row for (texture, frame), or NULL. A linear scan of 31 rows: the table
 * is sorted by (texture, frame) and a frame is looked up once per particle, so
 * at 41 particles a frame this is bounded by ~1,300 compares -- cheaper than
 * the index table it would take to avoid them.
 *
 * NEAREST EARLIER FRAME, not exact match, and that is what lets the generator
 * DECIMATE an animation. `row->frame` is the SOURCE frame index, so a texture
 * packed at source frames 0/3/6/9 answers a request for frame 5 with frame 3 --
 * the animation plays at a reduced rate over its full arc instead of vanishing.
 * PROJECT_GOAL allows reduced animation update rates; a particle that does not
 * draw at all is not an allowed approximation, and until 2026-08-02 that is
 * exactly what an unpacked frame produced.
 *
 * This is a strict superset of the old exact-match behaviour: the equality test
 * still short-circuits first, so every lookup that used to find a row finds the
 * same row. Only the paths that used to return NULL can now return data. A
 * request BELOW a texture's first packed frame still returns NULL -- there is
 * no earlier frame to hold -- and a texture that is not on the sheet at all
 * still returns NULL and still counts a QuadMiss, which is the case that needs
 * atlas space rather than a lookup change. */
static const NDSParticleQuadFrame *ndsParticleQuadFrameFor(u32 texture_id,
                                                           u32 frame)
{
    const NDSParticleQuadFrame *earlier = NULL;
    u32 index;

    for (index = 0u; index < NDS_PARTICLE_QUAD_FRAME_COUNT; index++)
    {
        const NDSParticleQuadFrame *row = &gNdsParticleQuadFrames[index];

        if (row->texture_id == (u8)texture_id)
        {
            if (row->frame == (u8)frame)
            {
                return row;
            }
            if (row->frame > (u8)frame)
            {
                return earlier;
            }
            earlier = row;
        }
        else if (row->texture_id > (u8)texture_id)
        {
            return earlier;
        }
    }
    return earlier;
}

#if NDS_R2_FOX_BLASTER_GLOW_AOT
/* Draw the closed script-0x62 pool in the same link-0 position the source
 * LBParticles occupied. The fast submit receives Q12/Q8 directly. Its full
 * 0..16 T coordinates intentionally exceed the physical 8-row texture: the
 * standalone TEXIMAGE_PARAM has WRAP_T|FLIP_T, so GX reflects rows 0..7 back
 * across the lower half exactly like the N64 script's MASKT command. */
static void ndsParticleDrawFoxBlasterGlowAOT(
    u32 texture_name, const Vec3f *right, const Vec3f *up,
    u32 *visible, u32 *emitted)
{
    u32 now = sySchedulerGetTicCount();
    u32 live = 0u;
    u32 index;

    for (index = 0u; index < sNdsFoxBlasterGlowAOTCount; index++)
    {
        NDSFoxBlasterGlowAOT glow = sNdsFoxBlasterGlowAOT[index];
        u32 age = (u32)(now - glow.spawn_tick);
        s32 submit_result;
        s32 draw_center_q12[3];

        if (age >= NDS_FOX_BLASTER_GLOW_VISIBLE_TICKS)
        {
            continue;
        }
        if (live != index)
        {
            sNdsFoxBlasterGlowAOT[live] = glow;
        }
        live++;
        (*visible)++;
        gNdsParticleTextureUseMask[
            NDS_FOX_BLASTER_GLOW_TEXTURE_ID >> 5] |=
            1u << (NDS_FOX_BLASTER_GLOW_TEXTURE_ID & 31u);
        if (gNdsParticleTextureFrameMax[
                NDS_FOX_BLASTER_GLOW_TEXTURE_ID] == 0u)
        {
            gNdsParticleTextureFrameMax[
                NDS_FOX_BLASTER_GLOW_TEXTURE_ID] = 1u;
        }
        /* OWNER DECISION 2026-08-13 -- the approved draw-only bore offset. The
         * pool entry itself is NOT touched: source writes this position through
         * efManagerFoxBlasterGlowMakeEffect's `pc->pos` and the AOT pool is that
         * position's cache, so the raise is applied to a draw-local copy only.
         * Same world +Y constant as the beam, deliberately: every one of the
         * four source callers passes the WEAPON's own translation
         * (wpfoxblaster.c:61/71/86/121), so the muzzle flash and the impact
         * flash both sit on the beam's centre line and have to move with it.
         *
         * Integer path, so this is one add on the shipping route. */
        draw_center_q12[0] = glow.center_q12[0];
        draw_center_q12[1] =
            (glow.center_q12[1] >
                 (INT_MAX - NDS_FOX_BLASTER_BORE_OFFSET_Y_Q12)) ?
            glow.center_q12[1] :
            (glow.center_q12[1] + NDS_FOX_BLASTER_BORE_OFFSET_Y_Q12);
        draw_center_q12[2] = glow.center_q12[2];
        submit_result = ndsRendererSubmitWhispyNativeQuad(
            texture_name, NDS_FOX_BLASTER_GLOW_BINDING_SLOT,
            NULL, 0.0F, draw_center_q12,
            sNdsFoxBlasterGlowSizeQ8[age],
            0x7fffu, 255u, 0u, 16u, 16u, 3u);
        if (submit_result < 0)
        {
            /* The exact binding/fixed contract should make this unreachable.
             * Keep the same native PAL16+mirror texture through the generic
             * corner builder rather than losing the flash. The float copy is
             * built here rather than above so the shipping route pays no
             * software-float add for a branch it never takes. */
            Vec3f draw_pos = glow.pos;

            draw_pos.y += (f32)NDS_FOX_BLASTER_BORE_OFFSET_Y;
            gNdsFoxBlasterGlowAOTFallbackCount++;
            submit_result = ndsRendererSubmitParticleQuad(
                texture_name, &draw_pos, sNdsFoxBlasterGlowSize[age],
                0x7fffu, 255u, right, up, 0u, 0u, 0u, 16u, 16u);
        }
        if (submit_result > 0)
        {
            (*emitted)++;
            gNdsFoxBlasterGlowAOTDrawCount++;
        }
        else
        {
            gNdsFoxBlasterGlowAOTMissCount++;
        }
    }
    sNdsFoxBlasterGlowAOTCount = live;
}
#endif

#if NDS_R2_PARTICLE_DRAW
/* Every input ndsParticleSetCurrentCamera reads on its xobjs_num != 0 path.
 * `persp_near`/`persp_far` rather than near/far because those two are macros on
 * some toolchains and this struct is not worth the surprise. */
typedef struct NDSParticleCameraKey
{
    const void *cobj;
    s32 xobjs_num;
    /* The XObj loop below branches on each entry's `kind`, so the count alone
     * is not the input -- two cameras with the same xobjs_num and different
     * kinds produce different matrices. Folded rather than stored as an array
     * because the fold is a handful of shifts against the 4x4 float concat it
     * is protecting, and the list is short. */
    u32 kind_fold;
    Vec3f eye;
    Vec3f at;
    Vec3f up;
    f32 fovy;
    f32 aspect;
    f32 persp_near;
    f32 persp_far;
    f32 scale;
} NDSParticleCameraKey;

static u32 ndsParticleCameraKindFold(const CObj *cobj)
{
    u32 fold = 2166136261u;
    u32 i;

    for (i = 0u; i < (u32)cobj->xobjs_num; i++)
    {
        u32 kind = (cobj->xobjs[i] != NULL) ?
            (u32)cobj->xobjs[i]->kind : (u32)nGCMatrixKindNull;

        fold = (fold ^ kind) * 16777619u;
    }
    return fold;
}

/* TWO WAYS, BECAUSE ONE MEASURED A 50% HIT RATE. The c101 single-entry cache
 * read Hit delta == Miss delta == 192 at every one of sixteen ring stops --
 * four calls a frame alternating hit, miss, hit, miss. That is not a cold
 * cache, it is two live cameras evicting each other every call. Round-robin
 * replacement over two ways makes an alternating pair hit after the first two
 * calls; if a third camera ever appears the counters go back to thrashing and
 * say so, which is why they stay. */
#define NDS_PARTICLE_CAMERA_CACHE_WAYS 2

static NDSParticleCameraKey sNdsParticleCameraKey[NDS_PARTICLE_CAMERA_CACHE_WAYS];
static Vec3f sNdsParticleCameraRight[NDS_PARTICLE_CAMERA_CACHE_WAYS];
static Vec3f sNdsParticleCameraUp[NDS_PARTICLE_CAMERA_CACHE_WAYS];
static NDSRendererMatrix20p12
    sNdsParticleCameraProjection[NDS_PARTICLE_CAMERA_CACHE_WAYS];
static NDSRendererMatrix20p12
    sNdsParticleCameraModelview[NDS_PARTICLE_CAMERA_CACHE_WAYS];
static u8 sNdsParticleCameraValid[NDS_PARTICLE_CAMERA_CACHE_WAYS];
static u32 sNdsParticleCameraNextWay;

/* THE ROUTE IS A .data WORD, NOT A #if, and that is a measurement decision
 * rather than a style one. c101's first arm gated this on the preprocessor,
 * which moved 672 bytes of `.main` and cost `FTR` +19,712 P50 -- a bucket this
 * code never calls, and 15x the growth the board's 1.85-cycles-per-added-byte
 * rule entitles 672 bytes to. The delta measured was placement, not mechanism.
 * With the route in `.data` both arms link with byte-identical text and bss and
 * differ in one initialised word, which is the pairing cycle 100 proved out.
 * The section attribute is required: an initialiser of 0 would otherwise land
 * the control arm's copy in `.bss` and reintroduce the very skew it removes. */
volatile u32 gNdsParticleCameraCacheEnabled
    __attribute__((section(".data"))) = NDS_R2_PARTICLE_CAMERA_CACHE;

/* Engagement pair, same contract as the shield's and the fireball's: Hit
 * climbing with Miss near the quad-draw count is the cache working; Miss
 * tracking Hit means the key never matches and the rebuild is still running
 * behind an extra 16 compares, which is SLOWER than the control. A measurement
 * taken without reading these would not know which of those it measured. */
volatile u32 gNdsParticleCameraCacheHitCount;
volatile u32 gNdsParticleCameraCacheMissCount;

/* Bitwise-equal is deliberate. These are copied scalars, not computed ones --
 * a hit requires the identical camera, and NaN cannot appear in an eye vector
 * that already passed the zero-length forward check below. */
static sb32 ndsParticleCameraKeyEqual(const NDSParticleCameraKey *a,
                                      const NDSParticleCameraKey *b)
{
    return ((a->cobj == b->cobj) && (a->xobjs_num == b->xobjs_num) &&
            (a->kind_fold == b->kind_fold) &&
            (a->eye.x == b->eye.x) && (a->eye.y == b->eye.y) &&
            (a->eye.z == b->eye.z) &&
            (a->at.x == b->at.x) && (a->at.y == b->at.y) &&
            (a->at.z == b->at.z) &&
            (a->up.x == b->up.x) && (a->up.y == b->up.y) &&
            (a->up.z == b->up.z) &&
            (a->fovy == b->fovy) && (a->aspect == b->aspect) &&
            (a->persp_near == b->persp_near) &&
            (a->persp_far == b->persp_far) && (a->scale == b->scale))
        ? TRUE : FALSE;
}

/* Build both billboard axes and the DS camera load from one current CObj. The
 * source particle draw does this once before iterating its pieces
 * (lbparticle.c:1486-1649): look-at first, projection second, then one 20.12
 * conversion. Reading the shared camera global here was subtly different
 * because the Results callback does not prepare it, leaving the preceding
 * battle matrix paired with the Results CObj-derived billboard axes. */
static sb32 ndsParticleSetCurrentCamera(Vec3f *right, Vec3f *up)
{
    CObj *cobj = (gGCCurrentCamera != NULL) ?
        CObjGetStruct(gGCCurrentCamera) : NULL;
    Mtx44f projection_f;
    Mtx44f look_at_f;
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    u32 i;
    u32 row;
    u32 col;
    NDSParticleCameraKey key;

    if (cobj == NULL)
    {
        return FALSE;
    }
    /* THIS FUNCTION IS CALLED ONCE PER QUAD AND ITS ANSWER IS FRAME-INVARIANT.
     * All three quad entry points call it, and each call rebuilds a
     * perspective matrix, a look-at basis (three sqrtf), re-runs both per XObj,
     * and finishes with a full 4x4 float guMtxCatF -- 64 multiplies and 48 adds
     * -- for a camera that cannot move between two quads of the same frame.
     *
     * MEASURED, not assumed (2026-08-09, ROM 3B1159ED,
     * artifacts/performance/2026-08-09_mtxcat-callers.json): this function is
     * 81.8% of the guMtxCatF + syMatrixLookAtF class, in three rows of 27.3%
     * that are exactly its three internal call sites, and that class is 24.2%
     * of all __aeabi_fadd + __aeabi_fmul. With its own 2.9% direct share it is
     * about 23% of every float operation in the frame -- the largest single
     * float consumer, and it is port-side code, not decomp's.
     *
     * KEYED ON THE INPUTS, NOT ON A FRAME COUNTER. The obvious key is
     * gNdsHardwareRendererSubmittedFrameCount, but nds_platform.c:3128 only
     * increments it when a frame is actually submitted, so two draw passes can
     * share a value and a moved camera would serve stale. Comparing the inputs
     * that determine the output is correct by construction and still trades ~16
     * scalar compares against three sqrtf and a 4x4 float concat.
     *
     * BIT-EXACT BY CONSTRUCTION: the same float arithmetic produces the cached
     * value; a hit replays it rather than approximating it. The A/B for this
     * change therefore predicts ZERO differing pixels, which is a falsifiable
     * claim rather than a fidelity budget.
     *
     * The xobjs_num == 0 branch is deliberately NOT cached. It has no sqrtf and
     * no concat, it keys on viewport fields this key does not carry, and
     * caching it would buy nothing while widening what the key has to prove. */
    key.cobj = cobj;
    key.xobjs_num = cobj->xobjs_num;
    key.kind_fold = ndsParticleCameraKindFold(cobj);
    key.eye = cobj->vec.eye;
    key.at = cobj->vec.at;
    key.up = cobj->vec.up;
    key.fovy = cobj->projection.persp.fovy;
    key.aspect = cobj->projection.persp.aspect;
    key.persp_near = cobj->projection.persp.near;
    key.persp_far = cobj->projection.persp.far;
    key.scale = cobj->projection.persp.scale;
    if ((gNdsParticleCameraCacheEnabled != 0u) && (cobj->xobjs_num != 0))
    {
        u32 way;

        for (way = 0u; way < NDS_PARTICLE_CAMERA_CACHE_WAYS; way++)
        {
            if ((sNdsParticleCameraValid[way] != 0u) &&
                (ndsParticleCameraKeyEqual(&key, &sNdsParticleCameraKey[way]) !=
                    FALSE))
            {
                *right = sNdsParticleCameraRight[way];
                *up = sNdsParticleCameraUp[way];
                /* Re-issued rather than skipped: the renderer's particle camera
                 * is shared state and something between two quads may have
                 * replaced it. Two 64-byte stores against the rebuild this is
                 * replacing. */
                ndsRendererSetParticleCamera(
                    &sNdsParticleCameraProjection[way],
                    &sNdsParticleCameraModelview[way]);
                gNdsParticleCameraCacheHitCount++;
                return TRUE;
            }
        }
        gNdsParticleCameraCacheMissCount++;
    }
    if (cobj->xobjs_num == 0)
    {
        f32 vscale_x = (f32)cobj->viewport.vp.vscale[0];
        f32 vscale_y = -(f32)cobj->viewport.vp.vscale[1];
        f32 vscale_z = (f32)cobj->viewport.vp.vscale[2];

        if ((vscale_x == 0.0F) || (vscale_y == 0.0F) ||
            (vscale_z == 0.0F))
        {
            return FALSE;
        }
        guMtxIdentF(projection_f);
        projection_f[0][0] = 1.0F / vscale_x;
        projection_f[1][1] = 1.0F / vscale_y;
        projection_f[2][2] = -1.0F / vscale_z;
        projection_f[3][0] =
            -(f32)cobj->viewport.vp.vtrans[0] / vscale_x;
        projection_f[3][1] =
            -(f32)cobj->viewport.vp.vtrans[1] / vscale_y;
        projection_f[3][2] =
            (f32)cobj->viewport.vp.vtrans[2] / vscale_z;
        right->x = 1.0F;
        right->y = 0.0F;
        right->z = 0.0F;
        up->x = 0.0F;
        up->y = 1.0F;
        up->z = 0.0F;
    }
    else
    {
        f32 forward_x = cobj->vec.at.x - cobj->vec.eye.x;
        f32 forward_y = cobj->vec.at.y - cobj->vec.eye.y;
        f32 forward_z = cobj->vec.at.z - cobj->vec.eye.z;
        f32 right_length_sq;
        f32 up_length_sq;

        if ((SQUARE(forward_x) + SQUARE(forward_y) + SQUARE(forward_z)) ==
            0.0F)
        {
            return FALSE;
        }
        /* The source's default clause resets both matrices. Seed those same
         * defaults once so a valid projection-only or look-at-only list also
         * has deterministic ownership, then apply every XObj in source order. */
        syMatrixPerspFastF(projection_f, NULL,
                           cobj->projection.persp.fovy,
                           cobj->projection.persp.aspect,
                           cobj->projection.persp.near,
                           cobj->projection.persp.far,
                           cobj->projection.persp.scale);
        syMatrixLookAtF(&look_at_f,
                        cobj->vec.eye.x, cobj->vec.eye.y,
                        cobj->vec.eye.z, cobj->vec.at.x,
                        cobj->vec.at.y, cobj->vec.at.z,
                        cobj->vec.up.x, cobj->vec.up.y,
                        cobj->vec.up.z);
        for (i = 0u; i < (u32)cobj->xobjs_num; i++)
        {
            u32 kind = (cobj->xobjs[i] != NULL) ?
                cobj->xobjs[i]->kind : nGCMatrixKindNull;

            switch (kind)
            {
            case nGCMatrixKindPerspFastF:
                syMatrixPerspFastF(projection_f, NULL,
                                   cobj->projection.persp.fovy,
                                   cobj->projection.persp.aspect,
                                   cobj->projection.persp.near,
                                   cobj->projection.persp.far,
                                   cobj->projection.persp.scale);
                break;
            case nGCMatrixKindPerspF:
                syMatrixPerspF(projection_f, NULL,
                               cobj->projection.persp.fovy,
                               cobj->projection.persp.aspect,
                               cobj->projection.persp.near,
                               cobj->projection.persp.far,
                               cobj->projection.persp.scale);
                break;
            case nGCMatrixKindOrtho:
                syMatrixOrthoF(&projection_f,
                               cobj->projection.ortho.l,
                               cobj->projection.ortho.r,
                               cobj->projection.ortho.b,
                               cobj->projection.ortho.t,
                               cobj->projection.ortho.n,
                               cobj->projection.ortho.f,
                               cobj->projection.ortho.scale);
                break;
            case 6:
            case 7:
            case 12:
            case 13:
                syMatrixLookAtF(&look_at_f,
                                cobj->vec.eye.x, cobj->vec.eye.y,
                                cobj->vec.eye.z, cobj->vec.at.x,
                                cobj->vec.at.y, cobj->vec.at.z,
                                cobj->vec.up.x, cobj->vec.up.y,
                                cobj->vec.up.z);
                break;
            case 8:
            case 9:
            case 14:
            case 15:
                syMatrixModLookAtF(&look_at_f,
                                   cobj->vec.eye.x, cobj->vec.eye.y,
                                   cobj->vec.eye.z, cobj->vec.at.x,
                                   cobj->vec.at.y, cobj->vec.at.z,
                                   cobj->vec.up.x, 0.0F, 1.0F, 0.0F);
                break;
            case 10:
            case 11:
            case 16:
            case 17:
                syMatrixModLookAtF(&look_at_f,
                                   cobj->vec.eye.x, cobj->vec.eye.y,
                                   cobj->vec.eye.z, cobj->vec.at.x,
                                   cobj->vec.at.y, cobj->vec.at.z,
                                   cobj->vec.up.x, 0.0F, 0.0F, 1.0F);
                break;
            default:
                syMatrixPerspFastF(projection_f, NULL,
                                   cobj->projection.persp.fovy,
                                   cobj->projection.persp.aspect,
                                   cobj->projection.persp.near,
                                   cobj->projection.persp.far,
                                   cobj->projection.persp.scale);
                syMatrixLookAtF(&look_at_f,
                                cobj->vec.eye.x, cobj->vec.eye.y,
                                cobj->vec.eye.z, cobj->vec.at.x,
                                cobj->vec.at.y, cobj->vec.at.z,
                                cobj->vec.up.x, cobj->vec.up.y,
                                cobj->vec.up.z);
                break;
            }
        }
        /* syMatrixLookAtF and syMatrixModLookAtF store final right/up in
         * columns 0/1. Using those columns keeps billboard orientation equal
         * to the final ordered XObj, including roll and forced-up variants. */
        right->x = look_at_f[0][0];
        right->y = look_at_f[1][0];
        right->z = look_at_f[2][0];
        up->x = look_at_f[0][1];
        up->y = look_at_f[1][1];
        up->z = look_at_f[2][1];
        right_length_sq = SQUARE(right->x) + SQUARE(right->y) +
                          SQUARE(right->z);
        up_length_sq = SQUARE(up->x) + SQUARE(up->y) + SQUARE(up->z);
        if (!(right_length_sq > 0.0F) || !(up_length_sq > 0.0F))
        {
            return FALSE;
        }
        guMtxCatF(look_at_f, projection_f, projection_f);
    }
    for (row = 0u; row < 4u; row++)
    {
        for (col = 0u; col < 4u; col++)
        {
            projection.m[row][col] = (row == col) ? 4096 : 0;
            modelview.m[row][col] = (s32)(projection_f[row][col] * 4096.0F);
        }
    }
    ndsRendererSetParticleCamera(&projection, &modelview);
    /* Stored only for the branch the key describes. Every FALSE return above
     * leaves sNdsParticleCameraValid untouched, so a rejected camera can never
     * be served from the cache. */
    if ((gNdsParticleCameraCacheEnabled != 0u) && (cobj->xobjs_num != 0))
    {
        u32 way = sNdsParticleCameraNextWay;

        sNdsParticleCameraKey[way] = key;
        sNdsParticleCameraRight[way] = *right;
        sNdsParticleCameraUp[way] = *up;
        sNdsParticleCameraProjection[way] = projection;
        sNdsParticleCameraModelview[way] = modelview;
        sNdsParticleCameraValid[way] = 1u;
        sNdsParticleCameraNextWay =
            (way + 1u) % NDS_PARTICLE_CAMERA_CACHE_WAYS;
    }
    return TRUE;
}

/* The source emitter multiplies every local particle position by its optional
 * LBTransform before projection (lbparticle.c:1683-1732). The first DS quad
 * path submitted pc->pos directly, which left Whispy, hit, and KO particles at
 * their script-local origin. Reuse the source matrix builder and its
 * Ready/Finished cache contract, then scale/mirror the camera-facing axes by
 * the transformed local X/Y magnitudes. */
static void ndsParticleTransformForDraw(LBParticle *pc,
                                        const Vec3f *camera_right,
                                        const Vec3f *camera_up,
                                        Vec3f *world_pos,
                                        Vec3f *quad_right,
                                        Vec3f *quad_up
#if NDS_R2_WHISPY_NATIVE_AOT
                                        , sb32 rigid_whispy
#endif
                                        )
{
    LBTransform *xf = pc->xf;

    *world_pos = pc->pos;
    *quad_right = *camera_right;
    *quad_up = *camera_up;
    if (xf == NULL)
    {
        return;
    }
    if (xf->transform_id != dLBParticleCurrentTransformID)
    {
        if (xf->transform_status != nLBTransformStatusFinished)
        {
            syMatrixTraRotRpyRScaF(
                &xf->affine,
                xf->translate.x, xf->translate.y, xf->translate.z,
                xf->rotate.x, xf->rotate.y, xf->rotate.z,
                xf->scale.x, xf->scale.y, xf->scale.z);
        }
        if (xf->transform_status == nLBTransformStatusReady)
        {
            xf->transform_status = nLBTransformStatusFinished;
        }
        xf->transform_id = dLBParticleCurrentTransformID;
    }
    world_pos->x = (xf->affine[0][0] * pc->pos.x) +
                   (xf->affine[1][0] * pc->pos.y) +
                   (xf->affine[2][0] * pc->pos.z) + xf->affine[3][0];
    world_pos->y = (xf->affine[0][1] * pc->pos.x) +
                   (xf->affine[1][1] * pc->pos.y) +
                   (xf->affine[2][1] * pc->pos.z) + xf->affine[3][1];
    world_pos->z = (xf->affine[0][2] * pc->pos.x) +
                   (xf->affine[1][2] * pc->pos.y) +
                   (xf->affine[2][2] * pc->pos.z) + xf->affine[3][2];
#if NDS_R2_WHISPY_NATIVE_AOT
    /* Both Pupupu roots attach a rigid translate + Y rotation with unit scale.
     * The generic path takes two square roots per particle just to recover
     * magnitudes 1 and 1 from that matrix. Preserve its diagonal-sign mirror
     * rule, but let the GX camera matrix own the rest of the transform. */
    if ((rigid_whispy != FALSE) && (gNdsWhispyAOTRoute != 0u))
    {
        if ((xf->scale.x == 1.0F) && (xf->scale.y == 1.0F) &&
            (xf->scale.z == 1.0F))
        {
            if (xf->affine[0][0] < 0.0F)
            {
                quad_right->x = -quad_right->x;
                quad_right->y = -quad_right->y;
                quad_right->z = -quad_right->z;
            }
            if (xf->affine[1][1] < 0.0F)
            {
                quad_up->x = -quad_up->x;
                quad_up->y = -quad_up->y;
                quad_up->z = -quad_up->z;
            }
            gNdsWhispyAOTRigidDraws++;
            return;
        }
        gNdsWhispyAOTRigidDrawFallbacks++;
    }
#endif
    {
        f32 scale_x = sqrtf(SQUARE(xf->affine[0][0]) +
                            SQUARE(xf->affine[0][1]) +
                            SQUARE(xf->affine[0][2]));
        f32 scale_y = sqrtf(SQUARE(xf->affine[1][0]) +
                            SQUARE(xf->affine[1][1]) +
                            SQUARE(xf->affine[1][2]));

        if (xf->affine[0][0] < 0.0F) { scale_x = -scale_x; }
        if (xf->affine[1][1] < 0.0F) { scale_y = -scale_y; }
        quad_right->x *= scale_x;
        quad_right->y *= scale_x;
        quad_right->z *= scale_x;
        quad_up->x *= scale_y;
        quad_up->y *= scale_y;
        quad_up->z *= scale_y;
    }
}

#if NDS_R2_POSITION_PROBE
/* Far-end of the fire-burn position contract. The maker-side probe stores the
 * exact LBParticle pointers returned by the three source Flame makers. Record
 * the first world-space quad centre produced for each pointer after the normal
 * source LBTransform has been applied but before any DS camera/fixed-point
 * conversion. That separates "source maker intentionally offset/moved it" from
 * "the DS renderer moved it" without inferring either from pixels. */
extern uintptr_t gNdsFlameMakerProbeParticle[8];
__attribute__((used)) u32 gNdsFlameDrawProbeSeenMask;
__attribute__((used)) f32 gNdsFlameDrawProbeWorldX[8];
__attribute__((used)) f32 gNdsFlameDrawProbeWorldY[8];
__attribute__((used)) f32 gNdsFlameDrawProbeWorldZ[8];
__attribute__((used)) f32 gNdsFlameDrawProbeLocalX[8];
__attribute__((used)) f32 gNdsFlameDrawProbeLocalY[8];
__attribute__((used)) f32 gNdsFlameDrawProbeLocalZ[8];
__attribute__((used)) f32 gNdsFlameDrawProbeXfX[8];
__attribute__((used)) f32 gNdsFlameDrawProbeXfY[8];
__attribute__((used)) f32 gNdsFlameDrawProbeXfZ[8];
__attribute__((used)) f32 gNdsFlameDrawProbeSize[8];
__attribute__((used)) u32 gNdsFlameDrawProbeTexture[8];
__attribute__((used)) u32 gNdsFlameDrawProbeFrame[8];

static void ndsParticleProbeFlameFirstDraw(LBParticle *pc,
                                           const Vec3f *world_pos)
{
    u32 slot;

    for (slot = 0u; slot < 8u; slot++)
    {
        u32 bit = 1u << slot;

        if ((gNdsFlameMakerProbeParticle[slot] == (uintptr_t)pc) &&
            ((gNdsFlameDrawProbeSeenMask & bit) == 0u))
        {
            gNdsFlameDrawProbeSeenMask |= bit;
            gNdsFlameDrawProbeWorldX[slot] = world_pos->x;
            gNdsFlameDrawProbeWorldY[slot] = world_pos->y;
            gNdsFlameDrawProbeWorldZ[slot] = world_pos->z;
            gNdsFlameDrawProbeLocalX[slot] = pc->pos.x;
            gNdsFlameDrawProbeLocalY[slot] = pc->pos.y;
            gNdsFlameDrawProbeLocalZ[slot] = pc->pos.z;
            if (pc->xf != NULL)
            {
                gNdsFlameDrawProbeXfX[slot] = pc->xf->translate.x;
                gNdsFlameDrawProbeXfY[slot] = pc->xf->translate.y;
                gNdsFlameDrawProbeXfZ[slot] = pc->xf->translate.z;
            }
            gNdsFlameDrawProbeSize[slot] = pc->size;
            gNdsFlameDrawProbeTexture[slot] = (u32)pc->texture_id;
            gNdsFlameDrawProbeFrame[slot] = (u32)pc->frame_id;
            return;
        }
    }
}
#endif

#if NDS_R2_WHISPY_NATIVE_AOT
#define NDS_WHISPY_AOT_XF_CACHE_COUNT 4u
static LBTransform *sNdsWhispyAOTXfCache[NDS_WHISPY_AOT_XF_CACHE_COUNT];
static u8 sNdsWhispyAOTXfMirror[NDS_WHISPY_AOT_XF_CACHE_COUNT];
static u32 sNdsWhispyAOTXfCacheCount;
static u32 sNdsWhispyAOTXfCacheTransformID;

/* The two Whispy roots use only translate + Y rotation + unit scale. Validate
 * that shape once per transform/pass, then evaluate its three useful rows: four
 * multiplies instead of the generic nine, with no per-particle sqrt or camera-
 * basis copies. `mirror_mask` is consumed by the fixed GX submitter. */
static sb32 ndsWhispyAOTTier2TransformForDraw(LBParticle *pc,
                                              Vec3f *world_pos,
                                              u32 *mirror_mask)
{
    LBTransform *xf = pc->xf;
    u32 index;
    u32 mirror = 0u;

    if (xf == NULL)
    {
        *world_pos = pc->pos;
        *mirror_mask = 0u;
        if (gNdsWhispyAOTRoute < 6u)
        {
            gNdsWhispyAOTTier2FixedTransforms++;
        }
        return TRUE;
    }
    if (xf->transform_id != dLBParticleCurrentTransformID)
    {
        if (xf->transform_status != nLBTransformStatusFinished)
        {
            syMatrixTraRotRpyRScaF(
                &xf->affine,
                xf->translate.x, xf->translate.y, xf->translate.z,
                xf->rotate.x, xf->rotate.y, xf->rotate.z,
                xf->scale.x, xf->scale.y, xf->scale.z);
        }
        if (xf->transform_status == nLBTransformStatusReady)
        {
            xf->transform_status = nLBTransformStatusFinished;
        }
        xf->transform_id = dLBParticleCurrentTransformID;
    }

    if (sNdsWhispyAOTXfCacheTransformID != dLBParticleCurrentTransformID)
    {
        sNdsWhispyAOTXfCacheTransformID = dLBParticleCurrentTransformID;
        sNdsWhispyAOTXfCacheCount = 0u;
    }
    for (index = 0u; index < sNdsWhispyAOTXfCacheCount; index++)
    {
        if (sNdsWhispyAOTXfCache[index] == xf)
        {
            mirror = sNdsWhispyAOTXfMirror[index];
            break;
        }
    }
    if (index == sNdsWhispyAOTXfCacheCount)
    {
        if ((xf->scale.x != 1.0F) || (xf->scale.y != 1.0F) ||
            (xf->scale.z != 1.0F) ||
            (xf->affine[0][1] != 0.0F) ||
            (xf->affine[1][0] != 0.0F) ||
            (xf->affine[1][1] != 1.0F) ||
            (xf->affine[1][2] != 0.0F) ||
            (xf->affine[2][1] != 0.0F))
        {
            return FALSE;
        }
        if (xf->affine[0][0] < 0.0F) { mirror |= 1u; }
        if (xf->affine[1][1] < 0.0F) { mirror |= 2u; }
        if (sNdsWhispyAOTXfCacheCount < NDS_WHISPY_AOT_XF_CACHE_COUNT)
        {
            sNdsWhispyAOTXfCache[sNdsWhispyAOTXfCacheCount] = xf;
            sNdsWhispyAOTXfMirror[sNdsWhispyAOTXfCacheCount] = (u8)mirror;
            sNdsWhispyAOTXfCacheCount++;
        }
    }

    world_pos->x = (xf->affine[0][0] * pc->pos.x) +
                   (xf->affine[2][0] * pc->pos.z) + xf->affine[3][0];
    world_pos->y = pc->pos.y + xf->affine[3][1];
    world_pos->z = (xf->affine[0][2] * pc->pos.x) +
                   (xf->affine[2][2] * pc->pos.z) + xf->affine[3][2];
    *mirror_mask = mirror;
    if (gNdsWhispyAOTRoute < 6u)
    {
        gNdsWhispyAOTTier2FixedTransforms++;
    }
    return TRUE;
}

#endif
#endif

volatile u32 gNdsSourceAssetQuadAttempts;
volatile u32 gNdsSourceAssetQuadDrawn;
volatile u32 gNdsSourceAssetQuadMissMask;

/* ONE camera-facing textured quad from the shared sheet, drawn OUTSIDE the
 * particle pass.
 *
 * The shield and the respawn pad are not particles -- they are GObj effects on
 * DL link 18 with their own display procs -- but they are exactly what this
 * path draws: a small alpha-blended source texture that always faces the
 * camera. They were procedural untextured discs, which is what the owner filed
 * against both.
 *
 * SELF-CONTAINED ON PURPOSE. It re-derives the basis, re-sets the camera and
 * closes its own batch rather than joining the particle pass, because the two
 * draws are ordered by the GObj display list and this file cannot know whether
 * lbParticleDrawTextures has already run this frame. Borrowing the pass's state
 * would make the shield correct or a frame stale depending on link order -- the
 * kind of dependency that reads as an intermittent visual bug. The cost is one
 * extra glBegin/glEnd and one atlas bind per shielding fighter, at most three
 * on screen at once against the 599 batch opens a match already does.
 *
 * Fails closed and counts: a missing atlas, an unseated texture or a degenerate
 * camera all leave the effect undrawn rather than drawing it wrong, and set a
 * bit in gNdsSourceAssetQuadMissMask so an absent shield is diagnosable instead
 * of mysterious. */
/* TOWARD THE EYE, not along a cross product. right x up would give the view
 * axis up to a sign that depends on the basis handedness, and getting that sign
 * wrong pushes the quad BEHIND the fighter -- the same bug, silently inverted.
 * eye - pos is the direction to the camera by construction, for any camera, and
 * the perspective divide turns a step along it into less depth. A no-op at bias
 * 0, which is every caller that wants the source position. */
static void ndsParticleBiasTowardEye(const Vec3f *pos, f32 depth_bias,
                                     Vec3f *out)
{
    CObj *cobj;

    *out = *pos;
    if (depth_bias == 0.0F)
    {
        return;
    }
    cobj = (gGCCurrentCamera != NULL) ? CObjGetStruct(gGCCurrentCamera) : NULL;
    if (cobj != NULL)
    {
        f32 dx = cobj->vec.eye.x - pos->x;
        f32 dy = cobj->vec.eye.y - pos->y;
        f32 dz = cobj->vec.eye.z - pos->z;
        f32 len = sqrtf((dx * dx) + (dy * dy) + (dz * dz));

        /* A camera sitting on the quad has no direction to offer, and the
         * divide would be the interesting kind of wrong. */
        if (len > 1.0F)
        {
            out->x += (dx / len) * depth_bias;
            out->y += (dy / len) * depth_bias;
            out->z += (dz / len) * depth_bias;
        }
    }
}

/* ONE CAMERA-FACING QUAD OVER A TEXTURE THIS PASS DOES NOT OWN. Same submit as
 * the atlas path, but the caller supplies the texture name and the whole image
 * is the cell -- for art that cannot live on the shared sheet. The shield is
 * the reason: its combiner needs a per-texel colour ramp, which a sheet whose
 * cells are all white cannot express (see NDS_SHIELD_A5I3_* in the generated
 * header). Fails closed and shares the source-asset miss mask. */
sb32 ndsParticleDrawOwnTextureQuad(u32 texture_name, u32 texture_w,
                                   u32 texture_h, const Vec3f *pos, f32 size,
                                   u32 color, u8 alpha, f32 depth_bias,
                                   f32 roll, sb32 mirror_x)
{
    Vec3f right;
    Vec3f up;
    Vec3f draw_pos;

    gNdsSourceAssetQuadAttempts++;
    if ((pos == NULL) || (size <= 0.0F) || (alpha == 0u) ||
        (texture_name == 0u) || (texture_w == 0u) || (texture_h == 0u))
    {
        gNdsSourceAssetQuadMissMask |= 1u << 0;
        return FALSE;
    }
#if NDS_R2_PARTICLE_DRAW
    if (ndsParticleSetCurrentCamera(&right, &up) == FALSE)
    {
        gNdsSourceAssetQuadMissMask |= 1u << 3;
        return FALSE;
    }
    /* The source spin: the weapon update adds 0.349066 rad (20 deg) to
     * dobj->rotate.vec.f.x every frame, and the 0x47 matrix kind applies it
     * as RotRpyR(rotate.x, rotate.y, 0) -- a roll of the camera-facing quad
     * about the axis pointing at the camera. Replicated with the source's OWN
     * sin table and its exact index/sign convention (lbdef.h syGetSinCosUShort),
     * so the quad spins at the same rate and phase the interpreter path
     * does. Rotating the billboard basis in its own plane is the whole
     * operation; nothing else about the quad changes.
     *
     * The mirror MUST land before the roll, not after: the 0x47 pitch term
     * (rotate.y = +-90 deg from wpMainVelSetModelPitch, wpmain.c:52-57)
     * maps the quad's local Z to -screen-X or +screen-X, which mirrors the
     * texture AND reverses the apparent roll direction. Negating the right
     * basis is that mirror; the roll then spins the mirrored basis exactly
     * as the source's RotRpyR composes the two terms. */
    if (mirror_x != FALSE)
    {
        right.x = -right.x;
        right.y = -right.y;
        right.z = -right.z;
    }
    if (roll != 0.0F)
    {
        s32 roll_id = SINTABLE_RAD_TO_ID(roll) & 0xFFF;
        s32 roll_sin = gSYSinTable[roll_id & 0x7FF];
        s32 roll_cos;
        Vec3f base_right;
        Vec3f base_up;

        if ((roll_id & 0x800) != 0)
        {
            roll_sin = -roll_sin;
        }
        roll_id = (roll_id + 0x400) & 0xFFF;
        roll_cos = gSYSinTable[roll_id & 0x7FF];
        if ((roll_id & 0x800) != 0)
        {
            roll_cos = -roll_cos;
        }
        base_right = right;
        base_up = up;
        /* Q15 table: sin/cos sit in the top 15 bits of the u16. The division
         * is by the same 32768.0F constant the source's own fixed-to-float
         * matrix conversions use, so the angle is reproduced as-is. */
        right.x = (roll_cos * base_right.x + roll_sin * base_up.x) / 32768.0F;
        right.y = (roll_cos * base_right.y + roll_sin * base_up.y) / 32768.0F;
        right.z = (roll_cos * base_right.z + roll_sin * base_up.z) / 32768.0F;
        up.x = (-roll_sin * base_right.x + roll_cos * base_up.x) / 32768.0F;
        up.y = (-roll_sin * base_right.y + roll_cos * base_up.y) / 32768.0F;
        up.z = (-roll_sin * base_right.z + roll_cos * base_up.z) / 32768.0F;
    }
    ndsParticleBiasTowardEye(pos, depth_bias, &draw_pos);
    if (ndsRendererSubmitParticleQuad(texture_name, &draw_pos, size, color,
                                      alpha, &right, &up, 0u, 0u, 0u,
                                      texture_w, texture_h) == FALSE)
    {
        ndsRendererEndParticleQuads();
        gNdsSourceAssetQuadMissMask |= 1u << 4;
        return FALSE;
    }
    ndsRendererEndParticleQuads();
    gNdsSourceAssetQuadDrawn++;
    return TRUE;
#else
    (void)texture_name; (void)texture_w; (void)texture_h; (void)color;
    (void)right; (void)up; (void)draw_pos; (void)depth_bias; (void)roll;
    (void)mirror_x;
    gNdsSourceAssetQuadMissMask |= 1u << 5;
    return FALSE;
#endif
}

sb32 ndsParticleDrawSourceAssetQuad(u32 texture_id, const Vec3f *pos, f32 size,
                                    u32 color, u8 alpha, f32 depth_bias)
{
    const NDSParticleQuadFrame *row;
    Vec3f right;
    Vec3f up;
    Vec3f draw_pos;
    u32 atlas_name;

    gNdsSourceAssetQuadAttempts++;
    if ((pos == NULL) || (size <= 0.0F) || (alpha == 0u))
    {
        gNdsSourceAssetQuadMissMask |= 1u << 0;
        return FALSE;
    }
#if NDS_R2_PARTICLE_DRAW
    atlas_name = ndsRendererHardwareParticleAtlasName();
    if (atlas_name == 0u)
    {
        gNdsSourceAssetQuadMissMask |= 1u << 1;
        return FALSE;
    }
    row = ndsParticleQuadFrameFor(texture_id, 0u);
    if (row == NULL)
    {
        gNdsSourceAssetQuadMissMask |= 1u << 2;
        return FALSE;
    }
    if (ndsParticleSetCurrentCamera(&right, &up) == FALSE)
    {
        gNdsSourceAssetQuadMissMask |= 1u << 3;
        return FALSE;
    }
    ndsParticleBiasTowardEye(pos, depth_bias, &draw_pos);
    /* Bind the sheet this cell was packed into -- see the note at the other
     * submit site. atlas_name above only proves the atlas prepared. */
    if (ndsRendererSubmitParticleQuad(
            ndsRendererHardwareParticleAtlasNameForSheet(row->sheet),
            &draw_pos, size, color, alpha,
                                      &right, &up, 0u, row->x, row->y,
                                      row->width, row->height) == FALSE)
    {
        ndsRendererEndParticleQuads();
        gNdsSourceAssetQuadMissMask |= 1u << 4;
        return FALSE;
    }
    ndsRendererEndParticleQuads();
    gNdsSourceAssetQuadDrawn++;
    return TRUE;
#else
    (void)texture_id; (void)color; (void)row; (void)right; (void)up;
    (void)atlas_name; (void)depth_bias; (void)draw_pos;
    gNdsSourceAssetQuadMissMask |= 1u << 5;
    return FALSE;
#endif
}

#if NDS_R2_FIREGRIND_NATIVE
/* THE NATIVE FIREGRIND DRAW, folded into this pass's open GX batch.
 *
 * Texture 5 is admitted to the DS atlas at exactly one row -- frame 0 (frames
 * 1/2 decimate to it via the nearest-earlier-frame rule), so the source's
 * random per-tick frame pick already drew frame 0 every time. Cache that ONE
 * row's sheet/coords once; a FireGrind spark draw is then a direct pointer
 * dereference, not another 31-row atlas scan.
 *
 * sNdsFireGrindFrameRow is NULL until the first frame the atlas is live;
 * resolved lazily so a build before the atlas is loaded never dereferences a
 * stale pointer. It points into the const gNdsParticleQuadFrames table, which
 * is program-lifetime, so it survives efParticleInitAll/scene resets unaided.
 * The sparks are drawn only from the GENLINK(0) camera pass (see the gate in
 * lbParticleDrawTextures). */
static const NDSParticleQuadFrame *sNdsFireGrindFrameRow;
/* SIMULATE ONCE PER FRAME, FROM THE LINK-0 PASS. The source simulates particles
 * in lbParticleStructFuncRun (a once-per-frame GObj proc), separate from the
 * draw. This port can't inject into that GObj proc cleanly (lbparticle.c is
 * included verbatim and its proc registration is not interposable without a GObj
 * proc-swap), so the sim runs here instead -- but OUTSIDE the GX batch, before
 * the camera/batch setup, and only from the GENLINK(0) pass, gated on
 * gNdsFrameCounter so multi-camera-link calls within one frame advance the pool
 * exactly once. A paused/skipped frame never reaches here, so sparks freeze. */
static u32 sNdsFireGrindLastUpdateFrame;
#endif

void lbParticleDrawTextures(GObj *gobj)
{
    Vec3f right;
    Vec3f up;
    u32 visible = 0u;
    u32 emitted = 0u;
    u32 atlas_name;
    u32 link;
#if NDS_R2_FOX_BLASTER_GLOW_AOT
    u32 fox_blaster_glow_name = 0u;
#endif
#if NDS_R2_WHISPY_NATIVE_AOT
    u32 whispy_native_names[3] = { 0u, 0u, 0u };
    u32 whispy_lean_source_frames = 0u;
    u32 whispy_lean_use_mask = 0u;
    u32 whispy_lean_texture_mask = 0u;
    u32 whispy_lean_stride_count = 0u;
    u32 whispy_lean_fixed_transforms = 0u;
    u32 whispy_lean_fixed_submits = 0u;
    u32 whispy_lean_fixed_fallbacks = 0u;
    u32 whispy_lean_draws = 0u;
    u32 whispy_lean_submit_ok = 0u;
    u32 whispy_lean_submit_fail = 0u;
    u8 whispy_lean_frame_max[3] = { 0u, 0u, 0u };
#endif
#if NDS_TICK_HUD
    /* R2-07 MISC split: the whole lbParticle quad pass, ticks not quads. The
     * emit COUNT already exists (gNdsParticleQuadEmitCount) and is exactly the
     * kind of number that got particles blamed once before at 0.21 quads a
     * frame; this is what it costs.
     *
     * cpuGetTiming is forward-declared at file scope rather than reached by
     * including nds/timers.h; the include-order constraint is documented
     * above. */
    u32 misc_particle_mark = cpuGetTiming();
#endif

    gNdsParticleDrawSeamCount++;
#if NDS_R2_FIREGRIND_NATIVE
    /* SIMULATE FIREGRIND ONCE PER FRAME, from the GENLINK(0) pass. Source
     * FireGrind lives on link 0; this pass runs on multiple particle-camera
     * links per frame, so the GENLINK(0) gate plus the frame-stamp together
     * advance the pool exactly one physics step per frame. Done before the
     * camera/batch setup so the GX submit path only renders, never simulates.
     * gNdsFrameCounter is forward-declared here for the same include-order
     * reason cpuGetTiming is above. */
    if ((gobj->camera_mask & (1u << 0u)) != 0u)
    {
        extern volatile u32 gNdsFrameCounter;

        if (gNdsFrameCounter != sNdsFireGrindLastUpdateFrame)
        {
            sNdsFireGrindLastUpdateFrame = gNdsFrameCounter;
            ndsFireGrindUpdate();
        }
    }
#endif
#if NDS_R2_PARTICLE_DRAW
    atlas_name = ndsRendererHardwareParticleAtlasName();
    if ((atlas_name != 0u) &&
        (ndsParticleSetCurrentCamera(&right, &up) == FALSE))
    {
        atlas_name = 0u;
    }
#if NDS_R2_WHISPY_NATIVE_AOT
    if ((atlas_name != 0u) && (gNdsWhispyAOTRoute >= 2u))
    {
        u32 texture;

        ndsRendererSetWhispyNativeBasis(&right, &up);
        for (texture = 0u; texture < ARRAY_COUNT(whispy_native_names);
             texture++)
        {
            whispy_native_names[texture] =
                ndsRendererHardwareWhispyNativeName(texture);
        }
    }
#endif
#if NDS_R2_FOX_BLASTER_GLOW_AOT
    if (atlas_name != 0u)
    {
        fox_blaster_glow_name =
            ndsRendererHardwareFoxBlasterGlowName();
        if (gNdsWhispyAOTRoute < 2u)
        {
            ndsRendererSetWhispyNativeBasis(&right, &up);
        }
    }
#endif
#else
    /* The census half only. The emit wedged the geometry engine on its first
     * build and is behind NDS_R2_PARTICLE_DRAW until that is understood; the
     * interpreter's measured NO-FREEZE full match must not depend on it. */
    (void)right;
    (void)up;
    atlas_name = 0u;
#endif

    dLBParticleCurrentTransformID++;

    for (link = 0u; link < ARRAY_COUNT(sLBParticleStructsAllocLinks); link++)
    {
        LBParticle *pc;

        if ((gobj->camera_mask & (1 << link)) == 0)
        {
            continue;
        }
#if NDS_R2_FOX_BLASTER_GLOW_AOT
        if ((link == 0u) && (fox_blaster_glow_name != 0u))
        {
            ndsParticleDrawFoxBlasterGlowAOT(
                fox_blaster_glow_name, &right, &up, &visible, &emitted);
        }
#endif
        for (pc = sLBParticleStructsAllocLinks[link]; pc != NULL; pc = pc->next)
        {
            const NDSParticleQuadFrame *row = NULL;
#if NDS_R2_PARTICLE_DRAW
            Vec3f world_pos;
            Vec3f quad_right;
            Vec3f quad_up;
            u32 color;
            u32 texture_name;
            u32 texture_x;
            u32 texture_y;
            u32 texture_width;
            u32 texture_height;
#endif
#if NDS_R2_WHISPY_NATIVE_TEXTURES
            sb32 whispy_native = FALSE;
#endif
            u32 id;

            if (pc->size == 0.0F)
            {
                continue;
            }
            visible++;
            id = pc->texture_id;
            if (id >= NDS_PARTICLE_TEXTURE_USE_IDS)
            {
                continue;
            }
#if NDS_R2_WHISPY_NATIVE_AOT
            if (gNdsWhispyAOTRoute < 6u)
#endif
            {
                gNdsParticleTextureUseMask[id >> 5] |= 1u << (id & 31u);
                /* frame_id + 1, so "never drawn" and "drew frame 0" differ. */
                if ((u32)pc->frame_id + 1u >
                    gNdsParticleTextureFrameMax[id])
                {
                    gNdsParticleTextureFrameMax[id] =
                        (u8)(pc->frame_id + 1u);
                }
            }
            if (atlas_name == 0u)
            {
                continue;
            }
            /* Texture 2 of Dream Land's bank is not texture 2 of the common
             * one, so the atlas key carries the bank. The use mask above
             * deliberately does NOT -- it is a source-id diagnostic and only
             * has 64 bits.
             *
             * THE KEY IS THE SLOT'S SCRIPT POINTER, NOT A LATCHED BANK ID.
             * This first shipped as `(pc->bank_id & 7) == gNdsParticleBankPupupuID`
             * and a 2026-08-01 soak reported BOTH gNdsParticleBankEFCommonID and
             * gNdsParticleBankPupupuID as 0 -- efParticleInitAll resets
             * sEFParticleBanksNum, so two loads either side of a reset land on
             * the same slot -- which strided 128,278 of 128,298 common
             * particles into Dream Land's key space and left 126,621 misses
             * against 1,677 quads. sEFParticleScriptBanks holds the pointer the
             * slot was registered with, so comparing it is an exact identity
             * test that cannot collide however the ids come out. */
            {
                u32 slot = (u32)pc->bank_id & 7u;

                if ((slot < ARRAY_COUNT(sEFParticleScriptBanks)) &&
                    (sEFParticleScriptBanks[slot] ==
                     (uintptr_t)&lGRPupupuParticleScriptBankLo))
                {
#if NDS_R2_WHISPY_NATIVE_TEXTURES
                    whispy_native = TRUE;
#else
                    id += NDS_PARTICLE_QUAD_PUPUPU_STRIDE;
#endif
#if NDS_R2_WHISPY_NATIVE_AOT
                    if (gNdsWhispyAOTRoute >= 6u)
                    {
                        whispy_lean_stride_count++;
                    }
                    else
#endif
                    {
                        gNdsParticleQuadStrideCount++;
                    }
                }
            }
#if NDS_R2_WHISPY_NATIVE_TEXTURES
            if (whispy_native != FALSE)
            {
#if NDS_R2_WHISPY_NATIVE_AOT
                if ((gNdsWhispyAOTRoute >= 6u) &&
                    ((u32)pc->texture_id < 3u))
                {
                    u32 frame_max = (u32)pc->frame_id + 1u;

                    whispy_lean_use_mask |= 1u << pc->texture_id;
                    if (frame_max >
                        whispy_lean_frame_max[pc->texture_id])
                    {
                        whispy_lean_frame_max[pc->texture_id] = (u8)frame_max;
                    }
                }
#endif
                if ((u32)pc->frame_id < 32u)
                {
#if NDS_R2_WHISPY_NATIVE_AOT
                    if (gNdsWhispyAOTRoute >= 6u)
                    {
                        whispy_lean_source_frames |= 1u << pc->frame_id;
                    }
                    else
#endif
                    {
                        gNdsWhispyNativeSourceFrameMask |= 1u << pc->frame_id;
                    }
                }
#if NDS_R2_WHISPY_NATIVE_AOT
                texture_name = ((gNdsWhispyAOTRoute >= 2u) &&
                                ((u32)pc->texture_id <
                                 ARRAY_COUNT(whispy_native_names))) ?
                    whispy_native_names[pc->texture_id] :
                    ndsRendererHardwareWhispyNativeName(pc->texture_id);
#else
                texture_name =
                    ndsRendererHardwareWhispyNativeName(pc->texture_id);
#endif
                if (texture_name == 0u)
                {
                    gNdsWhispyNativeTextureMissCount++;
                    continue;
                }
                texture_x = 0u;
                texture_y = 0u;
                switch (pc->texture_id)
                {
                case 0u:
                    texture_width = NDS_WHISPY_NATIVE_TEXTURE_0_WIDTH;
                    texture_height = NDS_WHISPY_NATIVE_TEXTURE_0_HEIGHT;
                    break;
                case 1u:
                    texture_width = NDS_WHISPY_NATIVE_TEXTURE_1_WIDTH;
                    texture_height = NDS_WHISPY_NATIVE_TEXTURE_1_HEIGHT;
                    break;
                case 2u:
                    /* The source still advances frame_id; this deliberately
                     * draws the one generated DS-native source-frame-0 leaf. */
                    texture_width = NDS_WHISPY_NATIVE_TEXTURE_2_WIDTH;
                    texture_height = NDS_WHISPY_NATIVE_TEXTURE_2_HEIGHT;
                    break;
                default:
                    gNdsWhispyNativeTextureMissCount++;
                    continue;
                }
            }
            else
#endif
            {
#if NDS_R2_WHISPY_NATIVE_AOT && NDS_R2_WHISPY_NATIVE_TEXTURES
                if (gNdsWhispyAOTRoute >= 6u)
                {
                    gNdsParticleTextureUseMask[id >> 5] |=
                        1u << (id & 31u);
                    if ((u32)pc->frame_id + 1u >
                        gNdsParticleTextureFrameMax[id])
                    {
                        gNdsParticleTextureFrameMax[id] =
                            (u8)(pc->frame_id + 1u);
                    }
                }
#endif
                row = ndsParticleQuadFrameFor(id, pc->frame_id);
            }
#if NDS_R2_WHISPY_NATIVE_TEXTURES
            if ((row == NULL) && (whispy_native == FALSE))
#else
            if (row == NULL)
#endif
            {
                /* Admitted set does not carry this one. Draw nothing rather
                 * than the neighbouring atlas cell.
                 *
                 * WHICH one, and at which frame. A bare count cannot separate
                 * the three ways this fires -- an unadmitted texture, a frame
                 * past the packed animation, or the bank stride landing on the
                 * wrong key -- and a 2026-08-01 soak spent two builds on that
                 * ambiguity with 128,295 misses against 1,677 emitted quads.
                 * Source id, pre-stride; the stride has its own counter. */
                gNdsParticleQuadMissCount++;
                gNdsParticleQuadMissMask[pc->texture_id >> 5] |=
                    1u << (pc->texture_id & 31u);
                if ((u32)pc->frame_id < 32u)
                {
                    gNdsParticleQuadMissFrameMask |= 1u << pc->frame_id;
                }
                continue;
            }
#if NDS_R2_PARTICLE_DRAW
            if (row != NULL)
            {
                texture_name =
                    ndsRendererHardwareParticleAtlasNameForSheet(row->sheet);
                texture_x = row->x;
                texture_y = row->y;
                texture_width = row->width;
                texture_height = row->height;
            }
            color = ((u32)(pc->primcolor.r >> 3) & 31u) |
                    (((u32)(pc->primcolor.g >> 3) & 31u) << 5) |
                    (((u32)(pc->primcolor.b >> 3) & 31u) << 10);
            /* primcolor.a IS the particle's fade. lbparticle.c ramps the whole
             * SYColorRGBA toward target_primcolor over primcolor_target_length
             * frames, so dropping the alpha here -- which this did until
             * 2026-08-02 -- leaves every particle fully opaque for its whole
             * life. The owner saw that as "emitted objects turn flat at end of
             * lifetime". BGR555 has no alpha channel on this hardware; it goes
             * through POLYGON_ATTR instead. */
            /* Generic particles bind the cell's sheet. Whispy binds one of
             * three dedicated hardware-native textures prepared with the
             * atlas, and always submits full-texture UVs. */
            {
                s32 submit_result = -1;
#if NDS_R2_WHISPY_NATIVE_AOT
                if ((whispy_native != FALSE) &&
                    (gNdsWhispyAOTRoute >= 2u))
                {
                    u32 mirror_mask;
                    sb32 transform_ok;

                    transform_ok = ndsWhispyAOTTier2TransformForDraw(
                        pc, &world_pos, &mirror_mask);
                    if (transform_ok != FALSE)
                    {
                        if (gNdsWhispyAOTRoute >= 6u)
                        {
                            whispy_lean_fixed_transforms++;
                        }
                        submit_result = ndsRendererSubmitWhispyNativeQuad(
                            texture_name, (u32)pc->texture_id,
                            &world_pos, pc->size, NULL, 0,
                            color, pc->primcolor.a, mirror_mask,
                            texture_width, texture_height,
                            gNdsWhispyAOTRoute);
                        if (submit_result > 0)
                        {
                            if (gNdsWhispyAOTRoute >= 6u)
                            {
                                whispy_lean_fixed_submits++;
                            }
                            else
                            {
                                gNdsWhispyAOTTier2FixedSubmits++;
                            }
                        }
                        else if (submit_result < 0)
                        {
                            if (gNdsWhispyAOTRoute >= 6u)
                            {
                                whispy_lean_fixed_fallbacks++;
                            }
                            else
                            {
                                gNdsWhispyAOTTier2FixedFallbacks++;
                            }
                        }
                    }
                    else
                    {
                        if (gNdsWhispyAOTRoute >= 6u)
                        {
                            whispy_lean_fixed_fallbacks++;
                        }
                        else
                        {
                            gNdsWhispyAOTTier2FixedFallbacks++;
                        }
                    }
                }
#endif
                if (submit_result < 0)
                {
                    u32 source_mirror_mask =
                        ((pc->flags & LBPARTICLE_FLAG_MASKS) ? 1u : 0u) |
                        ((pc->flags & LBPARTICLE_FLAG_MASKT) ? 2u : 0u);

                    ndsParticleTransformForDraw(
                        pc, &right, &up, &world_pos, &quad_right, &quad_up
#if NDS_R2_WHISPY_NATIVE_AOT
                        , ((whispy_native != FALSE) &&
                           (gNdsWhispyAOTRoute == 1u))
#endif
                    );
#if NDS_R2_POSITION_PROBE
                    ndsParticleProbeFlameFirstDraw(pc, &world_pos);
#endif
                    if ((source_mirror_mask & 1u) != 0u)
                    {
                        gNdsParticleMirrorSSubmitCount++;
                    }
                    if ((source_mirror_mask & 2u) != 0u)
                    {
                        gNdsParticleMirrorTSubmitCount++;
                    }
                    if ((source_mirror_mask & 3u) == 3u)
                    {
                        gNdsParticleMirrorSTSubmitCount++;
                    }
                    submit_result = ndsRendererSubmitParticleQuad(
                        texture_name, &world_pos, pc->size,
                        color, pc->primcolor.a, &quad_right, &quad_up,
                        source_mirror_mask,
                        texture_x, texture_y,
                        texture_width, texture_height);
                }
            if (submit_result != FALSE)
            {
                emitted++;
#if NDS_R2_WHISPY_NATIVE_TEXTURES
                if (whispy_native != FALSE)
                {
#if NDS_R2_WHISPY_NATIVE_AOT
                    if (gNdsWhispyAOTRoute >= 6u)
                    {
                        whispy_lean_draws++;
                        whispy_lean_texture_mask |= 1u << pc->texture_id;
                    }
                    else
#endif
                    {
                        gNdsWhispyNativeTextureDrawCount++;
                        gNdsWhispyNativeTextureMask |=
                            1u << pc->texture_id;
                    }
                }
#endif
                if (link == 1u)
                {
#if NDS_R2_WHISPY_NATIVE_AOT
                    if (gNdsWhispyAOTRoute >= 6u)
                    {
                        whispy_lean_submit_ok++;
                    }
                    else
#endif
                    {
                        gNdsWhispySubmitOk++;
                    }
                }
            }
            else if (link == 1u)
            {
#if NDS_R2_WHISPY_NATIVE_AOT
                if (gNdsWhispyAOTRoute >= 6u)
                {
                    whispy_lean_submit_fail++;
                }
                else
#endif
                {
                    gNdsWhispySubmitFail++;
                }
            }
            }
            /* TEMPORARY DIAGNOSTIC, BUGS.md row 1. Remove with that row.
             *
             * world_pos is a STACK LOCAL, and row 4 established the hard way
             * that gdb reads those as 0.0 on this remote while globals are
             * sound -- a whole root cause was published and withdrawn on that.
             * So the last unmeasured step of row 1, what the submit actually
             * receives for Whispy's link-1 particles and whether it accepts
             * them, is recorded here into globals instead of read from outside.
             * Also latches the v16 clamp: nds_renderer.c scales world by 16 and
             * saturates at +/-32767, so |world| > 2047.9 is drawn on the rail
             * rather than where it belongs. */
            if ((link == 1u)
#if NDS_R2_WHISPY_NATIVE_AOT
                && (gNdsWhispyAOTRoute < 6u)
#endif
               )
            {
                gNdsWhispyDrawX = world_pos.x;
                gNdsWhispyDrawY = world_pos.y;
                gNdsWhispyDrawSize = pc->size;
                /* Mirrors the renderer's own count rather than re-deriving a
                 * reach here. The reach is no longer a constant this file can
                 * know: ndsRendererSubmitParticleQuad now picks the vertex
                 * factor per batch and escalates when a quad needs it, so the
                 * only honest clamp count is the one taken at the conversion.
                 * Same variable, so probe-vfx-contracts.ps1 keeps reading it. */
                gNdsWhispyDrawClamped = gNdsParticleWorldClampCount;
            }
#endif
        }
    }
    if (atlas_name != 0u)
    {
#if NDS_R2_FIREGRIND_NATIVE
        /* Draw the native FireGrind pool into the SAME open batch. Source
         * FireGrind is created on particle GENLINK(0), so only submit from the
         * camera pass that owns link 0 -- otherwise a second particle-camera
         * pass draws the sparks again from a different camera, which reads as a
         * stray copy away from the bounce. The sim itself is NOT advanced here;
         * lbParticleStructFuncRun owns that, once per frame. */
        if ((gobj->camera_mask & (1u << 0u)) != 0u)
        {
            /* Texture 5's single admitted atlas row (frame 0; frames 1/2
             * decimate to it) is cached so a spark is one pointer dereference,
             * not another 31-row scan. */
            if (sNdsFireGrindFrameRow == NULL)
            {
                sNdsFireGrindFrameRow = ndsParticleQuadFrameFor(5u, 0u);
            }
            if (sNdsFireGrindFrameRow != NULL)
            {
                u32 fg_count;
                const NDSFireGrindParticle *fg =
                    ndsFireGrindPool(&fg_count);
                u32 fg_i;
                u32 fg_texture_name =
                    ndsRendererHardwareParticleAtlasNameForSheet(
                        sNdsFireGrindFrameRow->sheet);

                for (fg_i = 0u; fg_i < fg_count; fg_i++)
                {
                    if (ndsRendererSubmitParticleQuad(
                            fg_texture_name,
                            &fg[fg_i].pos,
                            ndsFireGrindSize(fg[fg_i].variant),
                            ndsFireGrindColor(fg[fg_i].variant,
                                              fg[fg_i].age),
                            255u,
                            &right, &up,
                            0u,
                            sNdsFireGrindFrameRow->x,
                            sNdsFireGrindFrameRow->y,
                            sNdsFireGrindFrameRow->width,
                            sNdsFireGrindFrameRow->height) != FALSE)
                    {
                        emitted++;
                    }
                }
            }
#if NDS_R2_FIREBALL_MAP_COLL_DEBUG
            /* Draw the captured fireball collision diamond as a world-space
             * outline inside this open batch, for tuning
             * NDS_R2_FIREBALL_MAP_COLL_SCALE. The globals are captured every
             * frame by the fireball's update proc
             * (src/import/battleship_mario_fireball.c). */
            {
                extern u32 gNdsFireballDebugCollActive;
                extern f32 gNdsFireballDebugCollX;
                extern f32 gNdsFireballDebugCollY;
                extern f32 gNdsFireballDebugCollZ;
                extern f32 gNdsFireballDebugCollTop;
                extern f32 gNdsFireballDebugCollCenter;
                extern f32 gNdsFireballDebugCollBottom;
                extern f32 gNdsFireballDebugCollWidth;

                if (gNdsFireballDebugCollActive != 0u)
                {
                    ndsRendererSubmitDebugDiamond(
                        gNdsFireballDebugCollX,
                        gNdsFireballDebugCollY,
                        gNdsFireballDebugCollZ,
                        gNdsFireballDebugCollTop,
                        gNdsFireballDebugCollCenter,
                        gNdsFireballDebugCollBottom,
                        gNdsFireballDebugCollWidth);
                }
            }
#endif
        }
#endif
        ndsRendererEndParticleQuads();
    }
#if NDS_R2_WHISPY_NATIVE_AOT
    if (gNdsWhispyAOTRoute >= 6u)
    {
        u32 texture;

        if (whispy_lean_use_mask != 0u)
        {
            gNdsParticleTextureUseMask[0] |= whispy_lean_use_mask;
        }
        for (texture = 0u; texture < 3u; texture++)
        {
            if (whispy_lean_frame_max[texture] >
                gNdsParticleTextureFrameMax[texture])
            {
                gNdsParticleTextureFrameMax[texture] =
                    whispy_lean_frame_max[texture];
            }
        }
        if (whispy_lean_source_frames != 0u)
        {
            gNdsWhispyNativeSourceFrameMask |= whispy_lean_source_frames;
        }
        if (whispy_lean_texture_mask != 0u)
        {
            gNdsWhispyNativeTextureMask |= whispy_lean_texture_mask;
        }
        gNdsParticleQuadStrideCount += whispy_lean_stride_count;
        gNdsWhispyAOTTier2FixedTransforms += whispy_lean_fixed_transforms;
        gNdsWhispyAOTTier2FixedSubmits += whispy_lean_fixed_submits;
        gNdsWhispyNativeTextureDrawCount += whispy_lean_draws;
        gNdsWhispySubmitOk += whispy_lean_submit_ok;
        if (whispy_lean_fixed_fallbacks != 0u)
        {
            gNdsWhispyAOTTier2FixedFallbacks +=
                whispy_lean_fixed_fallbacks;
        }
        if (whispy_lean_submit_fail != 0u)
        {
            gNdsWhispySubmitFail += whispy_lean_submit_fail;
        }
    }
#endif
    gNdsParticleDrawVisibleCount += visible;
    if (visible > gNdsParticleDrawVisibleMax)
    {
        gNdsParticleDrawVisibleMax = visible;
    }
    gNdsParticleQuadEmitCount += emitted;
    if (emitted > gNdsParticleQuadEmitMax)
    {
        gNdsParticleQuadEmitMax = emitted;
    }
    ndsParticleRuntimePublishTallies();
#if NDS_TICK_HUD
    gNdsMiscParticleDrawTicks += cpuGetTiming() - misc_particle_mark;
#endif
}
