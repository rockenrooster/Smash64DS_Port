/* Definitions for the packed efcommon particle bank.
 *
 * The data itself is generated -- scripts/generate_nds_particle_banks.py emits
 * generated/nds_particle_banks.generated.inc, and check-nds-particle-banks.ps1
 * proves it reproduces the source .scb/.txb byte for byte. This file exists
 * only to give that .inc a translation unit, which is the same shape
 * battle_playable_static_textures.c and nds_ifcommon_oam.c already use.
 *
 * It replaces nds_particle_banks_placeholder.c, whose weak empty definitions
 * were correct only while the pack did not exist.
 */
#include <nds/generated/nds_particle_banks.generated.h>

#include "generated/nds_particle_banks.generated.inc"

/* The interpreter indexes gNdsParticleScriptOffsets by SOURCE script id and
 * gNdsParticleTextures by SOURCE texture id, so a table shorter than the id
 * space would read past the end rather than reject. efcommon's texture ids run
 * 0..46 (PARTICLE_BANK_DISCOVERIES.md); battleship_lbparticle.c spells the same
 * 47 as NDS_PARTICLE_TEXTURE_IDS. */
_Static_assert(NDS_PARTICLE_TEXTURE_COUNT == 47u,
               "efcommon texture id space changed");
_Static_assert(NDS_PARTICLE_SCRIPT_REACHABLE_COUNT <= NDS_PARTICLE_SCRIPT_COUNT,
               "more reachable scripts than script ids");

/* The pack is only interchangeable with the source bank while these hold; the
 * generator recomputes them, so a silent asset swap changes the build. */
_Static_assert(NDS_PARTICLE_BANKS_SOURCE_CHECKSUM == 0xa2a1e85fu,
               "efcommon source bank checksum changed");
/* 0x1973edec -> 0x179aea12 on 2026-08-01, deliberately. The SOURCE checksum
 * above is unchanged, which is the half that would mean an asset swap; this one
 * moved because the pack now closes over four more P1 seams -- DustLight,
 * DustHeavy, DustHeavyDouble and MusicNote became live when
 * ndsFTParamMakeSourceEffect started routing the motion-script kinds, and their
 * scripts had never been packed. 87 -> 92 scripts, 31 -> 33 textures. The
 * tripwire did its job: it is the only thing that would have stopped a
 * regenerated pack from shipping unexamined.
 *
 * 0x179aea12 -> 0xd22b30b6 on 2026-08-12, deliberately, and again with the
 * SOURCE checksum unchanged. The fighter fire-damage colanim scripts (ids
 * 12..15) were restored, so their nEFKindFlameLR / nEFKindFlameRandom /
 * nEFKindFlameStatic requests now reach real makers instead of a substitute,
 * and the three Flame seams had never been packed. 92 -> 93 scripts,
 * 33 -> 34 textures. */
_Static_assert(NDS_PARTICLE_BANKS_TABLE_CHECKSUM == 0xd22b30b6u,
               "efcommon packed table checksum changed");
