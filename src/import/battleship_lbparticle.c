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
#include <stddef.h>
#include <string.h>

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
 * Structs stay 48 / generators 24 / transforms 24 for the battle scene;
 * Results overrides all three to 384/48/24 for its own frame. */
#define NDS_R2_PARTICLE_POOL_STRUCTS 48
#define NDS_R2_PARTICLE_POOL_GENERATORS 24
#define NDS_R2_PARTICLE_POOL_TRANSFORMS 24

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

    lbParticleAllocTransforms((s32)transforms, sizeof(LBTransform));
    sEFParticleBanksNum = 0;
    /* Every call here restarts bank numbering, so two loads on either side of
     * one reset share a slot. That is how efcommon and Dream Land both reported
     * bank 0 on 2026-08-01. Counted rather than assumed. */
    gNdsParticleInitAllCount++;
}

/* efdisplay.c passes the address of this marker as the efcommon script bank.
 * It is defined in src/import/battleship_efmanager.c; comparing against it is
 * an exact identity test, not a heuristic. */
extern uintptr_t lEFCommonParticleScriptBankLo;

/* Dream Land's own bank marker. Declared in include/reloc_data.h and defined in
 * src/port/diagnostics.c as intptr_t; only its address is ever used. */
extern intptr_t lGRPupupuParticleScriptBankLo;

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
    else
    {
        ndsParticleRegisterEmptyBank(bank_id);
        gNdsParticleBankOtherID = (u32)bank_id;
        if ((gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
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

/* The scene camera pair, for the particle pass's own matrix load. Declared here
 * rather than by including gmcamera.h, which drags the whole camera API into a
 * TU that already textually includes two decomp sources. */
extern Mtx44f gGMCameraMatrix;
extern Mtx44f gGCMatrixPerspF;

volatile u32 gNdsParticleQuadMissCount;
volatile u32 gNdsParticleInitAllCount;
volatile u32 gNdsParticleBankRegisterCount;
volatile u32 gNdsParticleQuadMissMask[2];
volatile u32 gNdsParticleQuadMissFrameMask;
volatile u32 gNdsParticleQuadStrideCount;

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

/* The camera basis, once per pass. `right` and `up` span the plane the
 * billboard lives in, derived from the same CObj eye/at/up the source's own
 * draw reads (lb/lbparticle.c:1487). Returns FALSE for a degenerate camera --
 * a zero-length forward or an up parallel to it -- and the pass then draws
 * nothing rather than emitting NaNs into the geometry engine. */
#if NDS_R2_PARTICLE_DRAW
static sb32 ndsParticleCameraBasis(Vec3f *right, Vec3f *up)
{
    CObj *cobj = CObjGetStruct(gGCCurrentCamera);
    Vec3f forward;
    f32 length;

    if (cobj == NULL)
    {
        return FALSE;
    }
    forward.x = cobj->vec.at.x - cobj->vec.eye.x;
    forward.y = cobj->vec.at.y - cobj->vec.eye.y;
    forward.z = cobj->vec.at.z - cobj->vec.eye.z;
    length = sqrtf(SQUARE(forward.x) + SQUARE(forward.y) + SQUARE(forward.z));
    if (length == 0.0F)
    {
        return FALSE;
    }
    forward.x /= length;
    forward.y /= length;
    forward.z /= length;

    right->x = (forward.y * cobj->vec.up.z) - (forward.z * cobj->vec.up.y);
    right->y = (forward.z * cobj->vec.up.x) - (forward.x * cobj->vec.up.z);
    right->z = (forward.x * cobj->vec.up.y) - (forward.y * cobj->vec.up.x);
    length = sqrtf(SQUARE(right->x) + SQUARE(right->y) + SQUARE(right->z));
    if (length == 0.0F)
    {
        return FALSE;
    }
    right->x /= length;
    right->y /= length;
    right->z /= length;

    /* Re-orthogonalised rather than taken from the CObj: the source's `up` is
     * a hint, not necessarily perpendicular to the view direction. */
    up->x = (right->y * forward.z) - (right->z * forward.y);
    up->y = (right->z * forward.x) - (right->x * forward.z);
    up->z = (right->x * forward.y) - (right->y * forward.x);
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
                                        Vec3f *quad_up)
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
sb32 ndsParticleDrawSourceAssetQuad(u32 texture_id, const Vec3f *pos, f32 size,
                                    u32 color, u8 alpha)
{
    const NDSParticleQuadFrame *row;
    Vec3f right;
    Vec3f up;
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
    if (ndsParticleCameraBasis(&right, &up) == FALSE)
    {
        gNdsSourceAssetQuadMissMask |= 1u << 3;
        return FALSE;
    }
    {
        NDSRendererMatrix20p12 projection;
        NDSRendererMatrix20p12 modelview;
        u32 r;
        u32 c;

        /* Same identity-projection / combined-modelview pair the particle pass
         * loads, and for the same reason: gGMCameraMatrix is ALREADY the
         * view-projection (gmcamera.c:1001), so loading a projection on top of
         * it applies perspective twice and collapses the quad off screen. */
        for (r = 0u; r < 4u; r++)
        {
            for (c = 0u; c < 4u; c++)
            {
                projection.m[r][c] = (r == c) ? 4096 : 0;
                modelview.m[r][c] = (s32)(gGMCameraMatrix[r][c] * 4096.0F);
            }
        }
        ndsRendererSetParticleCamera(&projection, &modelview);
    }
    /* Bind the sheet this cell was packed into -- see the note at the other
     * submit site. atlas_name above only proves the atlas prepared. */
    if (ndsRendererSubmitParticleQuad(
            ndsRendererHardwareParticleAtlasNameForSheet(row->sheet),
            pos, size, color, alpha,
                                      &right, &up, row->x, row->y,
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
    (void)atlas_name;
    gNdsSourceAssetQuadMissMask |= 1u << 5;
    return FALSE;
#endif
}

void lbParticleDrawTextures(GObj *gobj)
{
    Vec3f right;
    Vec3f up;
    u32 visible = 0u;
    u32 emitted = 0u;
    u32 atlas_name;
    u32 link;

    gNdsParticleDrawSeamCount++;
#if NDS_R2_PARTICLE_DRAW
    atlas_name = ndsRendererHardwareParticleAtlasName();
    if ((atlas_name != 0u) && (ndsParticleCameraBasis(&right, &up) == FALSE))
    {
        atlas_name = 0u;
    }
    if (atlas_name != 0u)
    {
        /* Hand the pass the scene camera. Without this the batch renders under
         * whichever object's matrix was loaded last -- measured as
         * PROJECTED_IDENTITY or a stage segment's compose, varying by frame --
         * which drew every effect at the eye or inside a stage segment while
         * its world position was perfectly correct. gGCMatrixPerspF and
         * gGMCameraMatrix are the same pair gmcamera.c:1001 composes for the
         * scene, in 20.12 as the renderer wants them. */
        NDSRendererMatrix20p12 projection;
        NDSRendererMatrix20p12 modelview;
        u32 row;
        u32 col;

        /* gGMCameraMatrix IS ALREADY THE VIEW-PROJECTION. gmcamera.c:1001 is
         * `guMtxCatF(lookAt, gGCMatrixPerspF, gGMCameraMatrix)`, and the source
         * then hands that single matrix to the RSP -- the N64 draws the whole
         * scene with one combined matrix, not a projection/modelview pair.
         *
         * Loading gGCMatrixPerspF as the DS projection ON TOP of it applied the
         * perspective TWICE, which collapsed every particle off screen. That
         * shipped on 2026-08-02 and broke ALL VFX -- worse than the misplacement
         * it was fixing, because a misplaced effect can still be play-tested and
         * an absent one cannot. The tell was already in hand and misread: the
         * confetti probe proved pieces were SUBMITTED with sane world positions
         * and growing spread, and submitted is not the same question as visible.
         * Walk created->alive->sized->IN-CAMERA->submitted, not a prefix of it.
         *
         * So the projection is identity and the combined matrix goes in as the
         * modelview. */
        for (row = 0u; row < 4u; row++)
        {
            for (col = 0u; col < 4u; col++)
            {
                projection.m[row][col] = (row == col) ? 4096 : 0;
                modelview.m[row][col] =
                    (s32)(gGMCameraMatrix[row][col] * 4096.0F);
            }
        }
        ndsRendererSetParticleCamera(&projection, &modelview);
    }
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
        for (pc = sLBParticleStructsAllocLinks[link]; pc != NULL; pc = pc->next)
        {
            const NDSParticleQuadFrame *row;
#if NDS_R2_PARTICLE_DRAW
            Vec3f world_pos;
            Vec3f quad_right;
            Vec3f quad_up;
            u32 color;
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
            gNdsParticleTextureUseMask[id >> 5] |= 1u << (id & 31u);
            /* frame_id + 1, so "never drawn" and "drew frame 0" differ. */
            if ((u32)pc->frame_id + 1u > gNdsParticleTextureFrameMax[id])
            {
                gNdsParticleTextureFrameMax[id] = (u8)(pc->frame_id + 1u);
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
                    id += NDS_PARTICLE_QUAD_PUPUPU_STRIDE;
                    gNdsParticleQuadStrideCount++;
                }
            }
            row = ndsParticleQuadFrameFor(id, pc->frame_id);
            if (row == NULL)
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
            ndsParticleTransformForDraw(pc, &right, &up, &world_pos,
                                        &quad_right, &quad_up);
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
            /* The cell's own sheet, not the pass's. The atlas is four separate
             * 8,192-byte allocations because that is the block size the DS
             * texture allocator has never refused, so which one a quad binds is
             * a property of the cell and travels in the frame row. */
            if (ndsRendererSubmitParticleQuad(
                    ndsRendererHardwareParticleAtlasNameForSheet(row->sheet),
                    &world_pos, pc->size,
                                              color, pc->primcolor.a,
                                              &quad_right, &quad_up,
                                              row->x, row->y,
                                              row->width, row->height) != FALSE)
            {
                emitted++;
                if (link == 1u)
                {
                    gNdsWhispySubmitOk++;
                }
            }
            else if (link == 1u)
            {
                gNdsWhispySubmitFail++;
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
            if (link == 1u)
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
        ndsRendererEndParticleQuads();
    }
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
}
