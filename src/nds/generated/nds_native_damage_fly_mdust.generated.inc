/* DamageFlyMDust immutable source geometry and display-list words.
 * Seven step-selected current images; no fractional TEXID track. */
#include <nds/generated/nds_native_damage_fly_mdust.generated.h>

static const u32 sNdsDamageFlyMDustRootWords[NDS_NATIVE_DAMAGE_FLY_MDUST_ROOT_BYTES / 4u] = {
    0xe7000000u, 0x00000000u,
    0xd9ddffffu, 0x00000000u,
    0xfc121824u, 0xff33ffffu,
    0xde000000u, 0x0e000000u,
    0xf5700000u, 0x07014050u,
    0xe6000000u, 0x00000000u,
    0xf3000000u, 0x073ff100u,
    0xe7000000u, 0x00000000u,
    0xf5701000u, 0x00094250u,
    0x01004008u, 0x32b33286u,
    0x06060402u, 0x00000602u,
    0xe7000000u, 0x00000000u,
    0xd9ffffffu, 0x00220000u,
    0xdf000000u, 0x00000000u,
};

static const u32 sNdsDamageFlyMDustFrameOffsets[7] = {
    0xc178u, 0xb970u, 0xb168u, 0xa960u, 0xa158u, 0x9950u, 0x9148u,
};

static const s16 sNdsDamageFlyMDustVertices[20u] = {
    62, -62, 0, 1024, 0,
    -62, -62, 0, 0, 0,
    -62, 62, 0, 0, 1024,
    62, 62, 0, 1024, 1024,
};

static const u8 sNdsDamageFlyMDustBandMasks[7] = {
    0x8u, 0xfu, 0xfu, 0xfu, 0xfu, 0xfu, 0xfu,
};

static const u16 sNdsDamageFlyMDustTriIndices[6u] = {
    3u, 2u, 1u,
    0u, 3u, 1u,
};

/* Source: one segment-E MObj branch, a stepped IA16 image, then TRI2. */
