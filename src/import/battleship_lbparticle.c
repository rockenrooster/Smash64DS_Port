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

/* Same reason, same pattern: decomp/.../ft/ftdef.h defines enum FTKind and so
 * does include/ft/fighter.h, and this is the first translation unit to reach
 * both. Take the decomp one. */
#define SSB64_NDS_FTKIND_DECLARED

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

#include <sc/scene.h>
#include <sys/dma.h>
#include <sys/debug.h>
#include <sys/taskman.h>
#include <string.h>

/* The two functions the decomp leaves to assembly (lbParticleUpdateStruct and
 * lbParticleGeneratorFuncRun) have complete C bodies behind this switch. The
 * port has no MIPS to link, so the C bodies are the implementation. */
#define NON_MATCHING 1

/* Interposed: the DS bank loader replaces the ROM DMA path entirely. */
#define efParticleGetLoadBankID ndsBaseEFParticleGetLoadBankID

/* Interposed: the N64 rectangle emitter is not the DS draw path. */
#define lbParticleDrawTextures ndsBaseLbParticleDrawTextures

/* Interposed: every external constructor validates its script id first. */
#define lbParticleMakeScriptID ndsBaseLbParticleMakeScriptID
#define lbParticleMakeCommon ndsBaseLbParticleMakeCommon
#define lbParticleMakePosVel ndsBaseLbParticleMakePosVel
#define lbParticleMakeGenerator ndsBaseLbParticleMakeGenerator

#include "../../decomp/BattleShip-main/decomp/src/lb/lbparticle.c"
#include "../../decomp/BattleShip-main/decomp/src/ef/efparticle.c"

#undef efParticleGetLoadBankID
#undef lbParticleDrawTextures
#undef lbParticleMakeScriptID
#undef lbParticleMakeCommon
#undef lbParticleMakePosVel
#undef lbParticleMakeGenerator

/* efdisplay.c passes the address of this marker as the efcommon script bank.
 * It is defined in src/import/battleship_efmanager.c; comparing against it is
 * an exact identity test, not a heuristic. */
extern uintptr_t lEFCommonParticleScriptBankLo;

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

/* LBScript's fixed 0x30-byte prefix: four u16 then eleven 4-byte words
 * (lbtypes.h:79-93). */
static void ndsParticleNormalizeHeader(u8 *header)
{
    u32 i;

    for (i = 0u; i < 4u; i++)
    {
        ndsParticleSwap16(&header[i * 2u]);
    }
    for (i = 0u; i < 9u; i++)
    {
        ndsParticleSwap32(&header[8u + (i * 4u)]);
    }
}

/* Walks one script's bytecode exactly as lbParticleUpdateStruct decodes it,
 * byte-swapping every f32 operand in place. Returns FALSE if the stream runs
 * past its limit or carries an opcode the interpreter has no case for, so a
 * malformed script is rejected rather than executed. */
static sb32 ndsParticleNormalizeBytecode(u8 *bytecode, u32 limit,
                                         u32 *commands, u32 *operands)
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
            ndsParticleSwap32(&bytecode[csr]);
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

    for (id = 0u; id < NDS_PARTICLE_SCRIPT_IDS; id++)
    {
        u32 candidate = gNdsParticleScriptOffsets[id];

        if ((candidate != NDS_PARTICLE_UNPACKED_OFFSET) &&
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

    scripts = syTaskmanMalloc(sizeof(*scripts) * NDS_PARTICLE_SCRIPT_IDS, 0x4);
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

    for (id = 0u; id < NDS_PARTICLE_SCRIPT_IDS; id++)
    {
        scripts[id] = (LBScript *)&sNdsParticleInertScript;
    }

    bank = NULL;
    if (bank_bytes >= sizeof(LBScriptHeader))
    {
        bank = syTaskmanMalloc(bank_bytes, 0x8);
        if (bank == NULL)
        {
            gNdsParticleBankLoadResult = NDS_PARTICLE_LOAD_REJECT;
            return FALSE;
        }
        memcpy(bank, gNdsParticleScriptBank, bank_bytes);
    }

    for (id = 0u; id < NDS_PARTICLE_SCRIPT_IDS; id++)
    {
        u32 offset = gNdsParticleScriptOffsets[id];
        u32 limit;
        u32 commands = 0u;
        u32 operands = 0u;
        u8 *header;

        if (offset == NDS_PARTICLE_UNPACKED_OFFSET)
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
        ndsParticleNormalizeHeader(header);
        if (ndsParticleNormalizeBytecode(
                header + sizeof(LBScriptHeader),
                limit - offset - (u32)sizeof(LBScriptHeader),
                &commands, &operands) == FALSE)
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

    sLBParticleScriptBanksNum[bank_id] = NDS_PARTICLE_SCRIPT_IDS;
    sLBParticleTextureBanksNum[bank_id] = NDS_PARTICLE_TEXTURE_IDS;
    sLBParticleScriptBanks[bank_id] = scripts;
    sLBParticleTextureBanks[bank_id] = textures;

    gNdsParticleBankLoadResult = (gNdsParticleBankScriptsPacked != 0u)
                                     ? NDS_PARTICLE_LOAD_PASS
                                     : NDS_PARTICLE_LOAD_REJECT;
    return TRUE;
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

    return bank_id;
}

/* TRUE when this bank/script pair resolves to a real packed script. The source
 * range test is repeated here on purpose: it has to run before the constructor,
 * because the constructor's own miss path reads through the bank array. */
static sb32 ndsParticleScriptIsPacked(s32 bank_id, s32 script_id)
{
    s32 id = bank_id & 7;

    if ((id >= LBPARTICLE_BANKS_NUM_MAX) || (script_id < 0) ||
        (script_id >= sLBParticleScriptBanksNum[id]) ||
        (sLBParticleScriptBanks[id] == NULL))
    {
        return FALSE;
    }
    return (sLBParticleScriptBanks[id][script_id] !=
            (LBScript *)&sNdsParticleInertScript)
               ? TRUE
               : FALSE;
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

/* The DS textured-quad path is the next gated step. Until then this is the
 * observation point for "the interpreter reached the draw seam this frame",
 * and it draws nothing. */
void lbParticleDrawTextures(GObj *gobj)
{
    (void)gobj;
    gNdsParticleDrawSeamCount++;
    ndsParticleRuntimePublishTallies();
}
