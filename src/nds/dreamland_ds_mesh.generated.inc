/*
 * Task 62 — generated Dream Land DS-mesh runtime data.
 *
 * Candidate: c120 (220 positions, 119 source triangles).
 * Compiled host-side by scripts/generate_dreamland_ds_mesh.py from
 * the Task 60 primitive stream + Task 61 encoded stream. The runtime
 * emits this verbatim under NDS_DREAMLAND_DS_MESH; default-off keeps
 * the shipping segment0 path byte-identical.
 *
 * DO NOT EDIT — regenerate with: python scripts/generate_dreamland_ds_mesh.py
 */
#ifndef NDS_DREAMLAND_DS_MESH_GENERATED_INC
#define NDS_DREAMLAND_DS_MESH_GENERATED_INC

#if NDS_DREAMLAND_DS_MESH

/* DS primitive ids (libnds videoGL.h). */
#define NDS_DREAMLAND_DS_PRIM_TRIANGLES       0u
#define NDS_DREAMLAND_DS_PRIM_QUAD            1u
#define NDS_DREAMLAND_DS_PRIM_TRIANGLE_STRIP  2u

#define NDS_DREAMLAND_DS_GROUP_COUNT 71u
#define NDS_DREAMLAND_DS_VERTEX_COUNT 261u

static const u8 sNdsDreamLandDSGroupPrim[71] = {
    1u, 2u, 2u, 2u, 2u, 2u, 2u, 1u, 2u, 1u, 2u, 1u,
    1u, 1u, 1u, 1u, 2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u,
    1u, 1u, 2u, 2u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
    1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 2u, 2u, 1u,
    1u, 2u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
    1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 2u, 2u,
};

static const u16 sNdsDreamLandDSGroupFirstVertex[71] = {
    0u, 4u, 7u, 10u, 13u, 16u, 19u, 22u,
    26u, 29u, 33u, 36u, 40u, 44u, 48u, 52u,
    56u, 59u, 62u, 65u, 68u, 71u, 74u, 77u,
    80u, 84u, 88u, 91u, 94u, 98u, 102u, 106u,
    110u, 114u, 118u, 122u, 126u, 130u, 134u, 138u,
    142u, 146u, 150u, 154u, 158u, 162u, 165u, 168u,
    172u, 176u, 179u, 183u, 187u, 191u, 195u, 199u,
    203u, 207u, 211u, 215u, 219u, 223u, 227u, 231u,
    235u, 239u, 243u, 247u, 251u, 255u, 258u,
};

static const u8 sNdsDreamLandDSGroupVertexCount[71] = {
    4u, 3u, 3u, 3u, 3u, 3u, 3u, 4u, 3u, 4u, 3u, 4u,
    4u, 4u, 4u, 4u, 3u, 3u, 3u, 3u, 3u, 3u, 3u, 3u,
    4u, 4u, 3u, 3u, 4u, 4u, 4u, 4u, 4u, 4u, 4u, 4u,
    4u, 4u, 4u, 4u, 4u, 4u, 4u, 4u, 4u, 3u, 3u, 4u,
    4u, 3u, 4u, 4u, 4u, 4u, 4u, 4u, 4u, 4u, 4u, 4u,
    4u, 4u, 4u, 4u, 4u, 4u, 4u, 4u, 4u, 3u, 3u,
};

/* Per-emission vertex: world/256 XYZ (s16 each). */
static const s16 sNdsDreamLandDSVertexX[261] = {
    -9, -6, 6, 9, 4, -9, -8, -5, -6, -4, 4, 3,
    5, -1, -3, -2, -1, 0, -1, -4, -3, -4, -4, -9,
    -6, -4, -9, -9, -6, 9, 4, 6, 9, 4, 4, 6,
    -13, -9, -9, -8, 13, 9, 9, 8, -9, -3, -2, -8,
    2, 3, 9, 8, 3, -3, -2, 2, -9, -9, -3, 3,
    9, 9, -9, -10, -11, 10, 9, 8, 7, 6, 5, -5,
    -6, -7, 2, 0, -1, -3, -4, -5, 9, 6, 6, 9,
    -9, -6, -6, -9, 9, 8, 6, -6, -8, -9, 4, 4,
    6, 6, 6, 6, 7, 7, 0, 0, 2, 2, -2, -2,
    0, 0, -5, -5, -4, -4, -5, -6, -4, -4, -7, -7,
    -5, -5, 7, 7, 6, 6, 4, 6, 4, 4, 2, 0,
    2, 2, 0, -2, 0, 2, -6, -7, -7, -5, 8, 8,
    9, 9, -9, -9, -8, -8, 9, 9, 9, 9, -9, -9,
    -9, -9, -9, -9, 9, 9, -4, -4, -4, 6, 6, 4,
    -5, -8, 5, 0, 8, -9, -8, 5, 8, 9, -9, 1,
    -5, -4, -2, 2, 1, -5, 1, -4, -5, 1, -2, 4,
    2, 2, 3, 5, 4, 2, 4, 2, 2, 4, 3, 6,
    8, 8, 6, -6, -8, -8, -6, 8, 4, 8, 9, 4,
    0, 5, 8, 0, -5, 0, 5, -4, -8, -8, -5, -8,
    -9, -9, -8, 7, 8, 9, 8, 8, 4, 4, 7, 4,
    0, 0, 4, 0, -4, -4, 0, -8, -7, -4, -4, -9,
    -8, -7, -8, -5, -4, 0, 9, 8, 9,
};

static const s16 sNdsDreamLandDSVertexY[261] = {
    -5, -5, -5, -5, 0, 1, 0, 0, -5, -2, 0, -5,
    -2, -2, -2, -5, -5, -5, -4, -5, -5, -4, -2, -2,
    -5, -5, -2, -5, -5, -2, -2, -5, -5, -2, -5, -5,
    2, -1, 3, 9, 2, -1, 3, 9, 3, -1, 10, 9,
    10, -1, 3, 9, -1, -1, 10, 10, 3, -1, -1, -1,
    -1, 3, 4, 5, 4, 2, 4, 2, 5, 7, 5, 5,
    6, 5, 3, 5, 3, 1, 3, 1, -7, -7, -5, -5,
    -5, -5, -7, -7, -7, -10, -7, -7, -10, -7, -2, -2,
    -2, -2, -2, -2, -2, -2, 1, 1, 1, 1, 1, 0,
    1, 1, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, 1, 1,
    0, 1, 0, 0, 1, 0, -2, -2, -2, -2, -10, -10,
    -7, -7, -7, -7, -10, -10, -7, -7, -5, -5, -5, -7,
    -7, -5, -5, -5, -5, -5, -2, -2, -2, -2, -2, -2,
    -5, -5, -5, -5, -5, -5, -5, -5, -5, -5, -5, -5,
    -5, -5, -5, -5, -5, -5, -5, -5, -5, -5, -5, -5,
    -5, -5, -5, -5, -5, -5, -5, -5, -5, -5, -5, -6,
    -6, -4, -4, -4, -4, -6, -6, -7, -7, -5, -5, -7,
    -7, -5, -5, -5, -5, -7, -5, -7, -7, -5, -5, -7,
    -7, -5, -5, -10, -7, -7, -10, -7, -7, -10, -10, -7,
    -7, -10, -10, -10, -10, -7, -7, -7, -10, -10, -7, -7,
    -10, -10, -7, -5, -7, -7, -5, -7, -7,
};

static const s16 sNdsDreamLandDSVertexZ[261] = {
    1, -4, -4, 1, -4, -1, -8, -4, -4, -3, -3, -3,
    -3, -2, -2, -1, -1, -1, -2, -1, -1, -2, -1, -1,
    0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0,
    -4, -1, -1, -2, -4, -1, -1, -2, -1, 1, -1, -2,
    -1, 1, -1, -2, 1, 1, -1, -1, -1, -1, 1, 1,
    -1, -1, -2, -2, -2, -1, -1, -1, -1, -1, -1, 0,
    0, 0, 0, 0, 0, 1, 1, 1, 1, -4, -4, 1,
    1, -4, -4, 1, 1, 2, -4, -4, 2, 1, 2, 3,
    3, 2, 2, 3, 3, 2, 2, 3, 3, 2, 2, 3,
    3, 2, 3, 2, 2, 3, 3, 3, 3, 3, 3, 2,
    2, 3, 3, 2, 3, 3, 3, 3, 3, 2, 3, 3,
    3, 2, 3, 3, 3, 3, 3, 2, 3, 3, 2, 4,
    4, 1, 4, 1, 2, 4, 4, 1, 1, 4, 1, 1,
    4, 4, 1, 4, 4, 1, 3, 3, 2, 3, 3, 3,
    8, 6, 8, 8, 6, 4, 6, 8, 6, 4, 4, 7,
    6, 7, 8, 6, 5, 6, 7, 5, 6, 5, 4, 7,
    7, 7, 7, 7, 6, 7, 7, 6, 7, 6, 6, 6,
    5, 5, 6, 6, 5, 5, 6, 6, 8, 6, 4, 8,
    8, 8, 6, 8, 8, 8, 8, 8, 6, 6, 8, 6,
    4, 4, 6, 6, 6, 4, 4, 6, 8, 7, 6, 8,
    8, 8, 7, 8, 7, 8, 8, 6, 6, 7, 8, 4,
    4, 6, 6, 8, 8, 8, 4, 6, 4,
};

/* Compensating world matrix: world = stored_coord * 256 + origin. */
#define NDS_DREAMLAND_DS_MATRIX_SCALE_S20P12  1048576
#define NDS_DREAMLAND_DS_MATRIX_ORIGIN_X_S20P12 0
#define NDS_DREAMLAND_DS_MATRIX_ORIGIN_Y_S20P12 5615616
#define NDS_DREAMLAND_DS_MATRIX_ORIGIN_Z_S20P12 -2510848

#define NDS_DREAMLAND_DS_CERTIFICATE  0x3B311291u

#endif /* NDS_DREAMLAND_DS_MESH */
#endif /* NDS_DREAMLAND_DS_MESH_GENERATED_INC */
