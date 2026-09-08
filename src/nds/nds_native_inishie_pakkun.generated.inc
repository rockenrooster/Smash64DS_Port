/* Mushroom Kingdom Pakkun native item packet (generated).
 * Source: SHA-pinned file 155 root 0x0B40 + file 107 palette 0x3620.
 * Material hook is live MOBJ_FLAG_ALPHA current-image selection only.
 * Do not hand-edit; regenerate with generate_nds_native_inishie_pakkun.py. */
#define NDS_NATIVE_INISHIE_PAKKUN_ROOT 0x0b40u
#define NDS_NATIVE_INISHIE_PAKKUN_PALETTE_OFFSET 0x3620u
#define NDS_NATIVE_INISHIE_PAKKUN_MOBJ_FLAGS 0x0001u
#define NDS_NATIVE_INISHIE_PAKKUN_VERTEX_COUNT 4u
#define NDS_NATIVE_INISHIE_PAKKUN_TRIANGLE_COUNT 2u
#define NDS_NATIVE_INISHIE_PAKKUN_CORNER_COUNT 6u

static const u16 sNdsNativeInishiePakkunTriIndices[6] =
{
    3u,
    2u,
    1u,
    0u,
    3u,
    1u,
};

static const s16 sNdsNativeInishiePakkunVerts[20] =
{
    180, 180, -24, 1056, 1056,
    180, -240, -24, 1056, 32,
    -180, -240, -24, 32, 32,
    -180, 180, -24, 32, 1056,
};

static const u32 sNdsNativeInishiePakkunVertColors[4] =
{
    0xccccccffu,
    0xccccccffu,
    0xccccccffu,
    0xccccccffu,
};

static const u16 sNdsNativeInishiePakkunFrameOffsets[3] =
{
    0x0968u,
    0x0860u,
    0x0758u,
};
static const u16 sNdsNativeInishiePakkunFrameBytes[3] =
{
    256u,
    264u,
    264u,
};

static void ndsNativeInishiePakkunSetup(
    NDSRendererStats *stats, const void *palette)
{
    stats->geometry_mode = (stats->geometry_mode & 0xd9ddffffu) | 0x00000000u;
    ndsRendererRecordOtherMode(stats, 0xe3u, 0xe3001001u, 0x00008000u);
    ndsRendererRecordOtherMode(stats, 0xe2u, 0xe2001e01u, 0x00000001u);
    ndsRendererRecordOtherMode(stats, 0xe2u, 0xe200001cu, 0x00553078u);
    ndsRendererRecordSetCombine(stats, 0xfc127e24u, 0xfffff3f9u);
    stats->blend_color = 0x00000000u;
    ndsRendererRecordSetTile(stats, 0xf5000100u, 0x05000000u);
    ndsRendererRecordSetTile(stats, 0xf5500000u, 0x07014040u);
    ndsRendererRecordSetTile(stats, 0xf5400200u, 0x00094340u);
    ndsRendererRecordSetImage(stats, 0xfd100000u, (u32)(uintptr_t)palette);
    ndsRendererRecordLoadTlut(stats, 0x0503c000u);
    ndsRendererRecordTextureState(stats, 0xd7000002u, 0xffffffffu);
    ndsRendererRecordSetTileSize(stats, 0xf2003004u, 0x0007f080u);
}

static void ndsNativeInishiePakkunSetupTail(NDSRendererStats *stats)
{
    ndsRendererRecordLoadBlock(stats, 0xf3000000u, 0x0707f800u);
}

static void ndsNativeInishiePakkunFinish(NDSRendererStats *stats)
{
    stats->geometry_mode = (stats->geometry_mode & 0xd9ffffffu) | 0x00220000u;
    ndsRendererRecordOtherMode(stats, 0xe3u, 0xe3001001u, 0x00000000u);
    ndsRendererRecordOtherMode(stats, 0xe2u, 0xe2001e01u, 0x00000000u);
    ndsRendererRecordOtherMode(stats, 0xe2u, 0xe200001cu, 0x00552078u);
}

/* census: dl_words=30 verts=4 tris=2 frames=3 material_flags=0x0001 */
