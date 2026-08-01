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
_Static_assert(NDS_PARTICLE_BANKS_TABLE_CHECKSUM == 0x1973edecu,
               "efcommon packed table checksum changed");
