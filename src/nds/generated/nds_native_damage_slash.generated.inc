/* DamageSlash native packet (generated from EFCommonEffects1).
 * Two source-backed quads; segment-E live material is applied by the executor.
 * Do not hand-edit; regenerate with generate_nds_damage_slash.py. */
#include <nds/generated/nds_native_damage_slash.generated.h>

static const u16 sNdsDamageSlashTriIndices[6] = {
    3u, 2u, 1u,  0u, 3u, 1u,
};

static const u16 sNdsDamageSlashPalette[16] = {
    0x0000u, 0xffffu, 0xfbd2u, 0xfbcfu, 0xfbccu, 0xfbc0u, 0xfb80u, 0xfb20u,
    0xfac0u, 0xfa80u, 0xfa40u, 0xfa20u, 0xf9e0u, 0xf980u, 0xf4c0u, 0xe800u,
};

static const u32 sNdsDamageSlashTextureOffsets[13] = {
    0x7260u, 0x70d8u, 0x6f50u, 0x6dc8u, 0x6c40u, 0x6ab8u, 0x6930u, 0x67a8u,
    0x65a0u, 0x6398u, 0x6190u, 0x5f88u, 0x5d80u,
};

static const u8 sNdsDamageSlashTextureShape[13][5] = {
    { 16u, 48u, 32u, 64u, 1u },
    { 16u, 48u, 32u, 64u, 1u },
    { 16u, 48u, 32u, 64u, 1u },
    { 16u, 48u, 32u, 64u, 1u },
    { 16u, 48u, 32u, 64u, 1u },
    { 16u, 48u, 32u, 64u, 1u },
    { 16u, 48u, 32u, 64u, 1u },
    { 16u, 48u, 32u, 64u, 1u },
    { 32u, 32u, 32u, 32u, 0u },
    { 32u, 32u, 32u, 32u, 0u },
    { 32u, 32u, 32u, 32u, 0u },
    { 32u, 32u, 32u, 32u, 0u },
    { 32u, 32u, 32u, 32u, 0u },
};

static const s16 sNdsDamageSlashVerts0[20] = {
    90, 148, 0, 1024, 1536,
    90, -148, 0, 1023, 0,
    -90, -148, 0, 0, 0,
    -90, 148, 0, 0, 1536,
};

static const s16 sNdsDamageSlashVerts1[20] = {
    90, 91, 0, 0, 1024,
    90, -91, 0, 1023, 1023,
    -90, -91, 0, 1024, 0,
    -90, 91, 0, 0, 0,
};

static void ndsDamageSlashSetup0(NDSRendererStats *stats, const void *palette)
{
    ndsRendererRecordOtherMode(stats, 0xe3u, 0xe3001001u, 0x00008000u);
    ndsRendererRecordSetCombine(stats, 0xfc121624u, 0xff2fffffu);
    ndsRendererRecordSetTile(stats, 0xf5000100u, 0x05000000u);
    ndsRendererRecordSetTile(stats, 0xf5500000u, 0x07018040u);
    ndsRendererRecordSetTile(stats, 0xf5400200u, 0x00098340u);
    ndsRendererRecordSetImage(stats, 0xfd100000u, (u32)(uintptr_t)palette);
    ndsRendererRecordLoadTlut(stats, 0x0503c000u);
    ndsRendererRecordTextureState(stats, 0xd7000002u, 0xffffffffu);
    ndsRendererRecordSetTileSize(stats, 0xf2000000u, 0x0007c0bcu);
}

static void ndsDamageSlashAfterMaterial0(NDSRendererStats *stats)
{
    ndsRendererRecordLoadBlock(stats, 0xf3000000u, 0x070bf800u);
    stats->geometry_mode = (stats->geometry_mode & 0xd9ddfbffu) | 0x00000000u;
    stats->geometry_command_count++;
}

static void ndsDamageSlashFinish0(NDSRendererStats *stats)
{
    stats->geometry_mode = (stats->geometry_mode & 0xd9ffffffu) | 0x00220400u;
    stats->geometry_command_count++;
    ndsRendererRecordOtherMode(stats, 0xe3u, 0xe3001001u, 0x00000000u);
}

/* child 0: palette=NDS_NATIVE_DAMAGE_SLASH_PALETTE0_OFFSET root=0x75a0 */

static void ndsDamageSlashSetup1(NDSRendererStats *stats, const void *palette)
{
    ndsRendererRecordOtherMode(stats, 0xe3u, 0xe3001001u, 0x00008000u);
    ndsRendererRecordSetCombine(stats, 0xfc121624u, 0xff2fffffu);
    ndsRendererRecordSetTile(stats, 0xf5000100u, 0x05000000u);
    ndsRendererRecordSetTile(stats, 0xf5500000u, 0x07014050u);
    ndsRendererRecordSetTile(stats, 0xf5400400u, 0x00094350u);
    ndsRendererRecordSetImage(stats, 0xfd100000u, (u32)(uintptr_t)palette);
    ndsRendererRecordLoadTlut(stats, 0x0503c000u);
    ndsRendererRecordTextureState(stats, 0xd7000002u, 0xffffffffu);
    ndsRendererRecordSetTileSize(stats, 0xf2000000u, 0x0007c07cu);
}

static void ndsDamageSlashAfterMaterial1(NDSRendererStats *stats)
{
    ndsRendererRecordLoadBlock(stats, 0xf3000000u, 0x070ff400u);
    stats->geometry_mode = (stats->geometry_mode & 0xd9ddfbffu) | 0x00000000u;
    stats->geometry_command_count++;
}

static void ndsDamageSlashFinish1(NDSRendererStats *stats)
{
    stats->geometry_mode = (stats->geometry_mode & 0xd9ffffffu) | 0x00220400u;
    stats->geometry_command_count++;
    ndsRendererRecordOtherMode(stats, 0xe3u, 0xe3001001u, 0x00000000u);
}

/* child 1: palette=NDS_NATIVE_DAMAGE_SLASH_PALETTE1_OFFSET root=0x7668 */

/* census: asset=83 roots=0x75a0,0x7668 verts=4+4 tris=2+2 live=typed-MObj */
