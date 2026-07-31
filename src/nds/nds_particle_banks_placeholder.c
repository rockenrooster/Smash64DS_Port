/* Weak placeholder for the R2-07 DS particle asset pack.
 *
 * These definitions exist only so the runtime interpreter compiles and links
 * before the generator step lands, and they are deliberately empty: an empty
 * bank with every script offset at the fail-closed sentinel makes the runtime
 * reject every script id and count the rejection. The generated data supplies
 * strong definitions for the same symbols and wins the link; delete this file
 * once it is in the build.
 */
#include <nds/generated/nds_particle_banks.generated.h>

#define NDS_WEAK __attribute__((weak))

#define NDS_PARTICLE_UNPACKED_1 NDS_PARTICLE_UNPACKED_OFFSET,
#define NDS_PARTICLE_UNPACKED_8                                              \
    NDS_PARTICLE_UNPACKED_1 NDS_PARTICLE_UNPACKED_1 NDS_PARTICLE_UNPACKED_1  \
    NDS_PARTICLE_UNPACKED_1 NDS_PARTICLE_UNPACKED_1 NDS_PARTICLE_UNPACKED_1  \
    NDS_PARTICLE_UNPACKED_1 NDS_PARTICLE_UNPACKED_1
#define NDS_PARTICLE_UNPACKED_56                                             \
    NDS_PARTICLE_UNPACKED_8 NDS_PARTICLE_UNPACKED_8 NDS_PARTICLE_UNPACKED_8  \
    NDS_PARTICLE_UNPACKED_8 NDS_PARTICLE_UNPACKED_8 NDS_PARTICLE_UNPACKED_8  \
    NDS_PARTICLE_UNPACKED_8

NDS_WEAK const u8 gNdsParticleScriptBank[1] = { 0u };
NDS_WEAK const u32 gNdsParticleScriptBankBytes = 0u;

/* 119 = 56 + 56 + 7 */
NDS_WEAK const u32 gNdsParticleScriptOffsets[NDS_PARTICLE_SCRIPT_IDS] = {
    NDS_PARTICLE_UNPACKED_56
    NDS_PARTICLE_UNPACKED_56
    NDS_PARTICLE_UNPACKED_1 NDS_PARTICLE_UNPACKED_1 NDS_PARTICLE_UNPACKED_1
    NDS_PARTICLE_UNPACKED_1 NDS_PARTICLE_UNPACKED_1 NDS_PARTICLE_UNPACKED_1
    NDS_PARTICLE_UNPACKED_OFFSET
};

NDS_WEAK const NDSParticleTexture gNdsParticleTextures[1] = {
    { 0u, 0u, 0u, 0u, 0u, 0u }
};
NDS_WEAK const u32 gNdsParticleTextureCount = 0u;
