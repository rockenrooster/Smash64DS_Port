/* PLACEHOLDER -- the R2-07 DS particle asset pack owns this file.
 *
 * The declarations below are the interface contract between the asset-pack step
 * and the runtime interpreter. Nothing here is packed yet: the placeholder
 * definitions in src/nds/nds_particle_banks_placeholder.c leave the bank empty
 * and every script offset at the fail-closed sentinel, so the runtime rejects
 * every script id and reports it through gNdsParticleRejectCount rather than
 * mis-indexing into a wrong effect.
 *
 * When the generator lands it replaces this header and supplies strong
 * definitions; the placeholder translation unit is weak and drops out. Delete
 * that file once the generated data is in the build.
 */
#ifndef SSB64_NDS_PARTICLE_BANKS_GENERATED_H
#define SSB64_NDS_PARTICLE_BANKS_GENERATED_H

#include <PR/ultratypes.h>

/* efcommon carries 119 scripts and 47 textures (PARTICLE_BANK_DISCOVERIES.md). */
#define NDS_PARTICLE_SCRIPT_IDS 119
#define NDS_PARTICLE_UNPACKED_OFFSET 0xFFFFFFFFu

typedef struct NDSParticleTexture
{
    u16 width;
    u16 height;
    u8 ds_format;
    u8 palette_entries;
    u32 data_offset;
    u32 palette_offset;
} NDSParticleTexture;

/* The efcommon script bank, byte-identical to the source .scb file: the fields
 * inside it are therefore N64 big-endian and the runtime normalizes them once
 * at load. Position-independent -- every internal pointer is a file-relative
 * offset, so no relocData tooling is involved. */
extern const u8 gNdsParticleScriptBank[];
extern const u32 gNdsParticleScriptBankBytes;

/* Indexed by SOURCE script id 0..118; NDS_PARTICLE_UNPACKED_OFFSET where the
 * script is not packed. Each packed entry is the byte offset of that script's
 * LBScript header inside gNdsParticleScriptBank. */
extern const u32 gNdsParticleScriptOffsets[NDS_PARTICLE_SCRIPT_IDS];

/* Indexed by SOURCE texture id; gNdsParticleTextureCount entries. */
extern const NDSParticleTexture gNdsParticleTextures[];
extern const u32 gNdsParticleTextureCount;
extern const u8 gNdsParticleTextureData[];
extern const u8 gNdsParticlePaletteData[];

#endif
