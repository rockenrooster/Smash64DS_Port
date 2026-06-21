#ifndef SSB64_NDS_RDP_STATE_WORK_H
#define SSB64_NDS_RDP_STATE_WORK_H

#include <PR/ultratypes.h>
#include <PR/gbi.h>
#include <nds/renderer_work/nds_gbi_work.h>

#define NDS_RDP_STATE_FLAG_PRIM_COLOR   (1u << 0)
#define NDS_RDP_STATE_FLAG_ENV_COLOR    (1u << 1)
#define NDS_RDP_STATE_FLAG_BLEND_COLOR  (1u << 2)
#define NDS_RDP_STATE_FLAG_FOG_COLOR    (1u << 3)
#define NDS_RDP_STATE_FLAG_COMBINE      (1u << 4)
#define NDS_RDP_STATE_FLAG_TEXTURE      (1u << 5)
#define NDS_RDP_STATE_FLAG_TIMG         (1u << 6)
#define NDS_RDP_STATE_FLAG_TILE         (1u << 7)
#define NDS_RDP_STATE_FLAG_TILESIZE     (1u << 8)
#define NDS_RDP_STATE_FLAG_LOADBLOCK    (1u << 9)
#define NDS_RDP_STATE_FLAG_GEOMETRY     (1u << 10)
#define NDS_RDP_STATE_FLAG_OTHERMODE    (1u << 11)

#define NDS_RDP_TILE_COUNT 8u

typedef struct NdsRgba8
{
    u8 r, g, b, a;
} NdsRgba8;

typedef struct NdsRdpTileState
{
    u8 fmt;
    u8 siz;
    u8 line;
    u8 tmem;
    u8 tile;
    u8 palette;
    u8 cmt;
    u8 maskt;
    u8 shiftt;
    u8 cms;
    u8 masks;
    u8 shifts;
    u16 uls, ult, lrs, lrt;
    u16 load_uls, load_ult, load_lrs, load_dxt;
} NdsRdpTileState;

typedef struct NdsRdpTextureState
{
    u8 enabled;
    u8 tile;
    u8 level;
    u8 on;
    u16 scale_s;
    u16 scale_t;
} NdsRdpTextureState;

typedef struct NdsRdpImageState
{
    u8 fmt;
    u8 siz;
    u16 width;
    const void *dram;
    u32 raw_addr;
} NdsRdpImageState;

typedef struct NdsRdpState
{
    u32 seen_flags;
    u32 unsupported_mask;
    u32 geometry_mode;
    u32 othermode_h;
    u32 othermode_l;
    u32 combine_w0;
    u32 combine_w1;
    NdsRgba8 prim_color;
    NdsRgba8 env_color;
    NdsRgba8 blend_color;
    NdsRgba8 fog_color;
    NdsRdpTextureState texture;
    NdsRdpImageState image;
    NdsRdpTileState tiles[NDS_RDP_TILE_COUNT];
    u32 command_count;
    u32 unsupported_opcode;
} NdsRdpState;

void ndsRdpStateInit(NdsRdpState *state);
void ndsRdpStateApplyCommand(NdsRdpState *state, const Gfx *cmd);
void ndsRdpStateApplyWords(NdsRdpState *state, u32 w0, u32 w1);
u16 ndsRdpStateDebugColor5551(const NdsRdpState *state);
const NdsRdpTileState *ndsRdpStateActiveTile(const NdsRdpState *state);

#endif
