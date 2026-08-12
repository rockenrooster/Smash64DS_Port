#include <nds/nds_fox_gun.h>

#if NDS_R2_FOX_GUN_OVERLAY

/* Fox's blaster, as data.
 *
 * BUGS.md "Fox's pistol model is missing". The gun is model part 13 on joint
 * 17 (FTFOX_BLASTER_HOLD_JOINT); source draws it by pointing the joint DObj's
 * `dl` at a display list in reloc file 315 (ftparam.c:768-785). The DS port
 * cannot do that: the fighter body is a baked primitive stream, and
 * ndsFighterDrawPlanResolve rejects the whole collection when any selected dl
 * resolves to a file whose asset_id is not the fighter model's -- and this dl
 * lives in asset 0x13b, not Fox's 0x139. Assigning joint->dl would push the
 * ENTIRE fighter off the native draw path to make one 22-triangle part appear.
 * Measured, not assumed: artifacts/bugs/2026-08-12_r2-07-cluster/
 * modelpart-chain.log, `MPOWNER asset=0x13b off=1008`.
 *
 * So the gun is an OVERLAY: the fighter body is untouched, and these triangles
 * are submitted separately at joint 17's world matrix while the model-part
 * state says the gun is out.
 *
 * The tables are resolved offline by `scripts/fox_gun_bake.py`, which pins the
 * asset's sha256 and fails closed if the layout moves. Re-derive with
 *
 *     python scripts/fox_gun_bake.py
 *     python scripts/fox_gun_bake.py --check src/nds/nds_fox_gun.c
 *
 * Nothing here is approximated. The display list's two G_VTX batches and
 * eleven G_TRI2 commands resolve to a flat 44-vertex / 22-triangle mesh that
 * never changes, so walking it at runtime would re-derive a constant every
 * frame. The palette is the source's own RGBA5551 with the DS channel order,
 * and the texels are the source's CI4 32x16 byte for byte -- both are already
 * DS formats, so there is no conversion to be wrong about.
 *
 * Positions and texcoords are the source Vtx payload unchanged; the submit
 * applies the DS fixed-point shifts. Normals are the Vtx colour bytes read as
 * signed, which is what a lit N64 vertex carries.
 */

static const NDSFoxGunVertex sNdsFoxGunVertices[] = {
    { {   -54,    36,     9 }, {   512,   227 }, {    0,    0,  127 } },
    { {   -33,   -24,     9 }, {   498,   337 }, {    0,    0,  127 } },
    { {   -21,    54,     9 }, {     0,   216 }, {    0,    0,  127 } },
    { {     0,   -18,     9 }, {    39,   345 }, {    0,    0,  127 } },
    { {     0,   -18,    -9 }, {    39,   345 }, {    0,    0, -128 } },
    { {   -21,    54,    -9 }, {     0,   216 }, {    0,    0, -128 } },
    { {   -54,    36,    -9 }, {   512,   227 }, {    0,    0, -128 } },
    { {   -33,   -24,    -9 }, {   498,   337 }, {    0,    0, -128 } },
    { {    60,   -42,     9 }, {   512,     0 }, {    0,  -10,  127 } },
    { {   -60,   -42,     9 }, {   512,   930 }, {    0,  -10,  127 } },
    { {   -72,    -6,    12 }, {     0,  1024 }, {    0,  -10,  127 } },
    { {    60,    -6,    12 }, {     0,     0 }, {    0,  -10,  127 } },
    { {    60,    -6,    12 }, {     0,     0 }, {    0,  127,    0 } },
    { {   -72,    -6,    12 }, {     0,  1024 }, {    0,  127,    0 } },
    { {   -72,    -6,   -12 }, {     0,  1024 }, {    0,  127,    0 } },
    { {    60,    -6,   -12 }, {     0,     0 }, {    0,  127,    0 } },
    { {   -54,    36,     9 }, {   512,   227 }, { -120,  -42,    0 } },
    { {   -33,   -24,     9 }, {   498,   337 }, { -120,  -42,    0 } },
    { {   -33,   -24,    -9 }, {   498,   337 }, { -120,  -42,    0 } },
    { {   -54,    36,    -9 }, {   512,   227 }, { -120,  -42,    0 } },
    { {    60,   -42,    -9 }, {   512,     0 }, {    0,  -10, -127 } },
    { {    60,    -6,   -12 }, {     0,     0 }, {    0,  -10, -127 } },
    { {   -72,    -6,   -12 }, {     0,  1024 }, {    0,  -10, -127 } },
    { {   -60,   -42,    -9 }, {   512,   930 }, {    0,  -10, -127 } },
    { {    60,   -42,    -9 }, {   512,     0 }, {    0, -128,    0 } },
    { {    60,   -42,     9 }, {   512,     0 }, {    0, -128,    0 } },
    { {   -60,   -42,    -9 }, {   512,   930 }, {    0, -128,    0 } },
    { {   -60,   -42,     9 }, {   512,   930 }, {    0, -128,    0 } },
    { {   -72,    -6,    12 }, {     0,  1024 }, { -121,  -40,    0 } },
    { {   -60,   -42,     9 }, {   512,   930 }, { -121,  -40,    0 } },
    { {   -60,   -42,    -9 }, {   512,   930 }, { -121,  -40,    0 } },
    { {   -72,    -6,   -12 }, {     0,  1024 }, { -121,  -40,    0 } },
    { {    60,    -6,   -12 }, {     0,     0 }, {  127,    0,    0 } },
    { {    60,   -42,    -9 }, {   512,     0 }, {  127,    0,    0 } },
    { {    60,   -42,     9 }, {   512,     0 }, {  127,    0,    0 } },
    { {    60,    -6,    12 }, {     0,     0 }, {  127,    0,    0 } },
    { {   -21,    54,    -9 }, {     0,   216 }, {  122,   35,    0 } },
    { {     0,   -18,    -9 }, {    39,   345 }, {  122,   35,    0 } },
    { {     0,   -18,     9 }, {    39,   345 }, {  122,   35,    0 } },
    { {   -21,    54,     9 }, {     0,   216 }, {  122,   35,    0 } },
    { {   -21,    54,     9 }, {     0,   216 }, {  -61,  112,    0 } },
    { {   -54,    36,     9 }, {   512,   227 }, {  -61,  112,    0 } },
    { {   -54,    36,    -9 }, {   512,   227 }, {  -61,  112,    0 } },
    { {   -21,    54,    -9 }, {     0,   216 }, {  -61,  112,    0 } },
};

static const u8 sNdsFoxGunTriangles[][3] = {
    { 31, 30, 29 },
    { 28, 31, 29 },
    { 27, 26, 25 },
    { 26, 24, 25 },
    { 23, 22, 21 },
    { 20, 23, 21 },
    { 19, 18, 17 },
    { 16, 19, 17 },
    { 15, 14, 13 },
    { 12, 15, 13 },
    { 11, 10,  9 },
    {  8, 11,  9 },
    {  7,  6,  5 },
    {  4,  7,  5 },
    {  3,  2,  1 },
    {  2,  0,  1 },
    { 43, 42, 41 },
    { 40, 43, 41 },
    { 39, 38, 37 },
    { 36, 39, 37 },
    { 35, 34, 33 },
    { 32, 35, 33 },
};

static const u16 sNdsFoxGunPalette[] = {
    0x8000, 0xbdef, 0xb44c, 0x95bb, 0x88cc, 0x9176, 0xad6b, 0xd093,
    0x850e, 0x9199, 0x8043, 0x95ee, 0xfbde, 0x94c5, 0x99de, 0x896b,
};

/* CI4 16x32, source order preserved. The extent is derived from the display
 * list's own G_SETTILE/G_SETTILESIZE, not from the byte count -- CI4 makes
 * 16x32 and 32x16 both 256 bytes, and the first bake of this table asserted the
 * transpose. */
static const u8 sNdsFoxGunTexels[] = {
    0x60, 0xdd, 0xd0, 0x66, 0x60, 0x00, 0x00, 0x06, 0x00, 0x11, 0x10, 0x00, 0x06, 0x11, 0x11, 0x66,
    0x00, 0x00, 0x00, 0x00, 0x06, 0x11, 0x11, 0x66, 0x11, 0x11, 0x11, 0x11, 0x06, 0x11, 0x11, 0x66,
    0x11, 0x11, 0x11, 0x11, 0x00, 0x00, 0x00, 0x00, 0x11, 0x11, 0x11, 0x11, 0x0d, 0x61, 0x11, 0x16,
    0x00, 0x00, 0x00, 0x00, 0x0d, 0xd1, 0x11, 0x66, 0x11, 0x11, 0x11, 0x11, 0x0d, 0xd1, 0x11, 0x66,
    0x11, 0x11, 0x11, 0x11, 0x0d, 0xd1, 0x11, 0x66, 0x11, 0x11, 0x11, 0x11, 0x0d, 0xd1, 0x11, 0x66,
    0x00, 0x00, 0x00, 0x00, 0x0d, 0xd1, 0x11, 0x66, 0x11, 0x11, 0x11, 0x11, 0x0d, 0xd1, 0x11, 0x66,
    0x11, 0x11, 0x11, 0x11, 0x0d, 0xd1, 0x11, 0x66, 0x11, 0x11, 0x11, 0x11, 0x0d, 0xd1, 0x11, 0x66,
    0x00, 0x00, 0x00, 0x00, 0x0d, 0xd1, 0x11, 0x66, 0x11, 0x11, 0x11, 0x11, 0x0d, 0xd1, 0x11, 0x66,
    0x11, 0x11, 0x11, 0x11, 0x0d, 0x61, 0x10, 0x00, 0x11, 0x11, 0x11, 0x11, 0x0d, 0x61, 0x10, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x0d, 0x61, 0x10, 0x00, 0x66, 0x66, 0x66, 0x66, 0x6d, 0x61, 0x10, 0x00,
    0x00, 0x06, 0x66, 0x66, 0x6d, 0x61, 0x10, 0x00, 0x11, 0x10, 0x66, 0x66, 0x6d, 0x61, 0x10, 0x00,
    0x11, 0x11, 0x06, 0x66, 0x6d, 0x61, 0x10, 0x00, 0x11, 0x11, 0x10, 0x66, 0x6d, 0x61, 0x10, 0x00,
    0x11, 0x11, 0x11, 0x06, 0x6d, 0x61, 0x11, 0x66, 0x11, 0x11, 0x11, 0x06, 0x6d, 0x61, 0x11, 0x66,
    0x11, 0x11, 0x11, 0x06, 0x6d, 0x61, 0x11, 0x66, 0x11, 0x11, 0x10, 0x66, 0x6d, 0x61, 0x11, 0x66,
    0x11, 0x11, 0x06, 0x66, 0x6d, 0x61, 0x11, 0x66, 0x11, 0x10, 0x66, 0x66, 0x6d, 0x61, 0x11, 0x66,
    0x00, 0x06, 0x66, 0x66, 0x6d, 0x61, 0x11, 0x66, 0x66, 0x66, 0x66, 0x66, 0x6d, 0x61, 0x11, 0x66,
};

const NDSFoxGunVertex *ndsFoxGunVertices(u32 *count_out)
{
    if (count_out != NULL)
    {
        *count_out = (u32)(sizeof(sNdsFoxGunVertices) /
                           sizeof(sNdsFoxGunVertices[0]));
    }
    return sNdsFoxGunVertices;
}

const u8 (*ndsFoxGunTriangles(u32 *count_out))[3]
{
    if (count_out != NULL)
    {
        *count_out = (u32)(sizeof(sNdsFoxGunTriangles) /
                           sizeof(sNdsFoxGunTriangles[0]));
    }
    return sNdsFoxGunTriangles;
}

const u16 *ndsFoxGunPalette(u32 *count_out)
{
    if (count_out != NULL)
    {
        *count_out = (u32)(sizeof(sNdsFoxGunPalette) /
                           sizeof(sNdsFoxGunPalette[0]));
    }
    return sNdsFoxGunPalette;
}

const u8 *ndsFoxGunTexels(u32 *bytes_out)
{
    if (bytes_out != NULL)
    {
        *bytes_out = (u32)sizeof(sNdsFoxGunTexels);
    }
    return sNdsFoxGunTexels;
}

_Static_assert(sizeof(sNdsFoxGunVertices) /
                   sizeof(sNdsFoxGunVertices[0]) == 44u,
               "Fox gun vertex count changed; re-run scripts/fox_gun_bake.py");
_Static_assert(sizeof(sNdsFoxGunTriangles) /
                   sizeof(sNdsFoxGunTriangles[0]) == 22u,
               "Fox gun triangle count changed; re-run scripts/fox_gun_bake.py");
_Static_assert(sizeof(sNdsFoxGunPalette) /
                   sizeof(sNdsFoxGunPalette[0]) == 16u,
               "Fox gun palette is not the source's 16 entries");
_Static_assert(sizeof(sNdsFoxGunTexels) ==
                   (NDS_FOX_GUN_TEXTURE_WIDTH *
                    NDS_FOX_GUN_TEXTURE_HEIGHT) / 2u,
               "Fox gun texels do not match the declared CI4 extent");

#endif /* NDS_R2_FOX_GUN_OVERLAY */
