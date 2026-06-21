#include <string.h>

#include <nds/renderer_work/nds_rdp_state_work.h>

static NdsRgba8 ndsRgbaFromWord(u32 w1)
{
    NdsRgba8 c;
    c.r = (u8)((w1 >> 24) & 0xFFu);
    c.g = (u8)((w1 >> 16) & 0xFFu);
    c.b = (u8)((w1 >> 8) & 0xFFu);
    c.a = (u8)(w1 & 0xFFu);
    return c;
}

static u16 ndsRgb5551(u8 r, u8 g, u8 b, u8 a)
{
    return (u16)(((a >= 0x80u) ? 0x8000u : 0u) |
                 ((u16)(r >> 3)) |
                 ((u16)(g >> 3) << 5) |
                 ((u16)(b >> 3) << 10));
}

void ndsRdpStateInit(NdsRdpState *state)
{
    if (state == NULL)
    {
        return;
    }
    memset(state, 0, sizeof(*state));
    state->prim_color.r = 0xFFu;
    state->prim_color.g = 0xFFu;
    state->prim_color.b = 0xFFu;
    state->prim_color.a = 0xFFu;
}

void ndsRdpStateApplyCommand(NdsRdpState *state, const Gfx *cmd)
{
    if ((state == NULL) || (cmd == NULL))
    {
        return;
    }
    ndsRdpStateApplyWords(state, cmd->words.w0, cmd->words.w1);
}

void ndsRdpStateApplyWords(NdsRdpState *state, u32 w0, u32 w1)
{
    u32 op;

    if (state == NULL)
    {
        return;
    }

    op = ndsGbiOpcode(w0);
    state->command_count++;

    switch (op)
    {
    case NDS_GBI_OP_SETPRIMCOLOR:
        state->prim_color = ndsRgbaFromWord(w1);
        state->seen_flags |= NDS_RDP_STATE_FLAG_PRIM_COLOR;
        break;

    case NDS_GBI_OP_SETENVCOLOR:
        state->env_color = ndsRgbaFromWord(w1);
        state->seen_flags |= NDS_RDP_STATE_FLAG_ENV_COLOR;
        break;

    case NDS_GBI_OP_SETBLENDCOLOR:
        state->blend_color = ndsRgbaFromWord(w1);
        state->seen_flags |= NDS_RDP_STATE_FLAG_BLEND_COLOR;
        break;

    case NDS_GBI_OP_SETFOGCOLOR:
        state->fog_color = ndsRgbaFromWord(w1);
        state->seen_flags |= NDS_RDP_STATE_FLAG_FOG_COLOR;
        break;

    case NDS_GBI_OP_SETCOMBINE:
        state->combine_w0 = w0;
        state->combine_w1 = w1;
        state->seen_flags |= NDS_RDP_STATE_FLAG_COMBINE;
        break;

    case NDS_GBI_OP_TEXTURE:
        state->texture.scale_s = (u16)((w1 >> 16) & 0xFFFFu);
        state->texture.scale_t = (u16)(w1 & 0xFFFFu);
        state->texture.level = (u8)((w0 >> 11) & 0x7u);
        state->texture.tile = (u8)((w0 >> 8) & 0x7u);
        state->texture.on = (u8)(w0 & 0xFFu);
        state->texture.enabled = (state->texture.on != 0u) ? 1u : 0u;
        state->seen_flags |= NDS_RDP_STATE_FLAG_TEXTURE;
        break;

    case NDS_GBI_OP_SETTIMG:
        state->image.fmt = (u8)((w0 >> 21) & 0x7u);
        state->image.siz = (u8)((w0 >> 19) & 0x3u);
        state->image.width = (u16)((w0 & 0x0FFFu) + 1u);
        state->image.raw_addr = w1;
        state->image.dram = (const void *)(uintptr_t)w1;
        state->seen_flags |= NDS_RDP_STATE_FLAG_TIMG;
        break;

    case NDS_GBI_OP_SETTILE:
    {
        u32 tile = (w1 >> 24) & 0x7u;
        NdsRdpTileState *t = &state->tiles[tile];
        t->fmt = (u8)((w0 >> 21) & 0x7u);
        t->siz = (u8)((w0 >> 19) & 0x3u);
        t->line = (u8)((w0 >> 9) & 0x1FFu);
        t->tmem = (u8)(w0 & 0x1FFu);
        t->tile = (u8)tile;
        t->palette = (u8)((w1 >> 20) & 0xFu);
        t->cmt = (u8)((w1 >> 18) & 0x3u);
        t->maskt = (u8)((w1 >> 14) & 0xFu);
        t->shiftt = (u8)((w1 >> 10) & 0xFu);
        t->cms = (u8)((w1 >> 8) & 0x3u);
        t->masks = (u8)((w1 >> 4) & 0xFu);
        t->shifts = (u8)(w1 & 0xFu);
        state->seen_flags |= NDS_RDP_STATE_FLAG_TILE;
        break;
    }

    case NDS_GBI_OP_LOADBLOCK:
    {
        u32 tile = (w1 >> 24) & 0x7u;
        NdsRdpTileState *t = &state->tiles[tile];
        t->load_uls = (u16)((w0 >> 12) & 0xFFFu);
        t->load_ult = (u16)(w0 & 0xFFFu);
        t->load_lrs = (u16)((w1 >> 12) & 0xFFFu);
        t->load_dxt = (u16)(w1 & 0xFFFu);
        state->seen_flags |= NDS_RDP_STATE_FLAG_LOADBLOCK;
        break;
    }

    case NDS_GBI_OP_SETTILESIZE:
    {
        u32 tile = (w1 >> 24) & 0x7u;
        NdsRdpTileState *t = &state->tiles[tile];
        t->uls = (u16)((w0 >> 12) & 0xFFFu);
        t->ult = (u16)(w0 & 0xFFFu);
        t->lrs = (u16)((w1 >> 12) & 0xFFFu);
        t->lrt = (u16)(w1 & 0xFFFu);
        state->seen_flags |= NDS_RDP_STATE_FLAG_TILESIZE;
        break;
    }

    case NDS_GBI_OP_GEOMETRYMODE:
        state->geometry_mode = w1;
        state->seen_flags |= NDS_RDP_STATE_FLAG_GEOMETRY;
        break;

    case NDS_GBI_OP_SETOTHERMODE_H:
    case NDS_GBI_OP_SETOTHERMODE_L:
    case NDS_GBI_OP_RDPSETOTHERMODE:
        if (op == NDS_GBI_OP_SETOTHERMODE_H)
        {
            state->othermode_h = w1;
        }
        else
        {
            state->othermode_l = w1;
        }
        state->seen_flags |= NDS_RDP_STATE_FLAG_OTHERMODE;
        break;

    case NDS_GBI_OP_RDPPIPESYNC:
    case NDS_GBI_OP_RDPLOADSYNC:
    case NDS_GBI_OP_RDPTILESYNC:
    case NDS_GBI_OP_RDPFULLSYNC:
    case NDS_GBI_OP_DL:
    case NDS_GBI_OP_ENDDL:
    case NDS_GBI_OP_VTX:
    case NDS_GBI_OP_TRI1:
    case NDS_GBI_OP_TRI2:
        break;

    default:
        if (state->unsupported_opcode == 0u)
        {
            state->unsupported_opcode = op;
        }
        state->unsupported_mask |= (op < 32u) ? (1u << op) : 0x80000000u;
        break;
    }
}

u16 ndsRdpStateDebugColor5551(const NdsRdpState *state)
{
    const NdsRgba8 *c;
    if (state == NULL)
    {
        return 0u;
    }
    c = ((state->seen_flags & NDS_RDP_STATE_FLAG_PRIM_COLOR) != 0u)
            ? &state->prim_color
            : &state->env_color;
    return ndsRgb5551(c->r, c->g, c->b, c->a);
}

const NdsRdpTileState *ndsRdpStateActiveTile(const NdsRdpState *state)
{
    if (state == NULL)
    {
        return NULL;
    }
    return &state->tiles[state->texture.tile & 7u];
}
