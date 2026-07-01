#include <string.h>

#include <nds/nds_gbi_decode.h>
#include <nds/nds_renderer.h>

#define NDS_RENDERER_OP_NOOP 0x00u
#define NDS_RENDERER_OP_VTX 0x01u
#define NDS_RENDERER_OP_MODIFYVTX 0x02u
#define NDS_RENDERER_OP_CULLDL 0x03u
#define NDS_RENDERER_OP_TRI1 0x05u
#define NDS_RENDERER_OP_TRI2 0x06u
#define NDS_RENDERER_OP_TEXTURE 0xd7u
#define NDS_RENDERER_OP_GEOMETRYMODE 0xd9u
#define NDS_RENDERER_OP_MOVEWORD 0xdbu
#define NDS_RENDERER_OP_DL 0xdeu
#define NDS_RENDERER_OP_ENDDL 0xdfu
#define NDS_RENDERER_OP_SETOTHERMODE_H 0xe3u
#define NDS_RENDERER_OP_SETOTHERMODE_L 0xe2u
#define NDS_RENDERER_OP_SETSCISSOR 0xedu
#define NDS_RENDERER_OP_SETCOMBINE 0xfcu
#define NDS_RENDERER_OP_SETCIMG 0xffu
#define NDS_RENDERER_OP_SETFOGCOLOR 0xf8u
#define NDS_RENDERER_OP_SETBLENDCOLOR 0xf9u
#define NDS_RENDERER_OP_SETENVCOLOR 0xfbu
#define NDS_RENDERER_OP_SETPRIMCOLOR 0xfau
#define NDS_RENDERER_OP_SETTIMG 0xfdu
#define NDS_RENDERER_OP_SETTILE 0xf5u
#define NDS_RENDERER_OP_LOADBLOCK 0xf3u
#define NDS_RENDERER_OP_LOADTLUT 0xf0u
#define NDS_RENDERER_OP_SETTILESIZE 0xf2u
#define NDS_RENDERER_OP_RDPSETOTHERMODE 0xefu
#define NDS_RENDERER_OP_RDPPIPESYNC 0xe7u
#define NDS_RENDERER_OP_RDPLOADSYNC 0xe6u
#define NDS_RENDERER_OP_RDPTILESYNC 0xe8u
#define NDS_RENDERER_OP_RDPFULLSYNC 0xe9u

#define NDS_RENDERER_TX_CLAMP 0x2u
#define NDS_RENDERER_TX_MIRROR 0x1u
#define NDS_RENDERER_RENDER_TILE 0u
#define NDS_RENDERER_LOAD_TILE 7u

#define NDS_RENDERER_MAX_VTX 32u

static s32 ndsRendererValidateCommand(const Gfx *dl,
                                      const NDSRendererConfig *config)
{
    uintptr_t addr = (uintptr_t)dl;

    if ((dl == NULL) || ((addr & 0x3u) != 0))
    {
        return FALSE;
    }
    if (config->validate_range == NULL)
    {
        return TRUE;
    }
    return config->validate_range(dl, sizeof(*dl), config->user);
}

static void ndsRendererRecordUnsupported(NDSRendererStats *stats, u32 op)
{
    if (stats->unsupported_opcode == 0)
    {
        stats->unsupported_opcode = op;
    }
    stats->unsupported_command_count++;
}

static void ndsRendererRecordIgnoredOtherMode(NDSRendererStats *stats,
                                              u32 op, u32 w0, u32 w1)
{
    stats->state_command_count++;
    stats->othermode_command_count++;
    stats->ignored_state_command_count++;
    if (stats->first_othermode_opcode == 0)
    {
        stats->first_othermode_opcode = op;
        stats->first_othermode_w0 = w0;
        stats->first_othermode_w1 = w1;
    }
}

static void ndsRendererRecordCull(NDSRendererStats *stats, u32 w0, u32 w1)
{
    stats->cull_command_count++;
    stats->skip_command_count++;
    if ((stats->first_cull_w0 == 0) && (stats->first_cull_w1 == 0))
    {
        stats->first_cull_w0 = w0;
        stats->first_cull_w1 = w1;
    }
}

static void ndsRendererRecordTextureState(NDSRendererStats *stats,
                                          u32 w0, u32 w1)
{
    if (stats == NULL)
    {
        return;
    }

    stats->texture_mask |= NDS_RENDERER_TEXTURE_TEXTURE;
    stats->texture_command_count++;
    stats->texture_xparam = (w0 >> 16) & 0xFFu;
    stats->texture_level = (w0 >> 11) & 0x7u;
    stats->texture_tile = (w0 >> 8) & 0x7u;
    stats->texture_on = (w0 >> 1) & 0x7Fu;
    stats->texture_scale_s = (w1 >> 16) & 0xFFFFu;
    stats->texture_scale_t = w1 & 0xFFFFu;
    stats->texture_state_flags = NDS_RENDERER_TEXTURE_STATE_SEEN;
    if (stats->texture_on != 0)
    {
        stats->texture_state_flags |= NDS_RENDERER_TEXTURE_STATE_ON;
    }
    if (stats->texture_scale_s != 0)
    {
        stats->texture_state_flags |= NDS_RENDERER_TEXTURE_STATE_SCALE_S;
    }
    if (stats->texture_scale_t != 0)
    {
        stats->texture_state_flags |= NDS_RENDERER_TEXTURE_STATE_SCALE_T;
    }
}

static void ndsRendererRecordSetTile(NDSRendererStats *stats,
                                     u32 w0, u32 w1)
{
    u32 tile;
    u32 fmt;
    u32 siz;
    u32 line;
    u32 tmem;
    u32 palette;
    u32 cmt;
    u32 maskt;
    u32 shiftt;
    u32 cms;
    u32 masks;
    u32 shifts;

    if (stats == NULL)
    {
        return;
    }

    tile = (w1 >> 24) & 0x7u;
    fmt = (w0 >> 21) & 0x7u;
    siz = (w0 >> 19) & 0x3u;
    line = (w0 >> 9) & 0x1FFu;
    tmem = w0 & 0x1FFu;
    palette = (w1 >> 20) & 0xFu;
    cmt = (w1 >> 18) & 0x3u;
    maskt = (w1 >> 14) & 0xFu;
    shiftt = (w1 >> 10) & 0xFu;
    cms = (w1 >> 8) & 0x3u;
    masks = (w1 >> 4) & 0xFu;
    shifts = w1 & 0xFu;

    stats->texture_mask |= NDS_RENDERER_TEXTURE_SETTILE;
    stats->texture_set_tile_count++;

    if (tile == NDS_RENDERER_RENDER_TILE)
    {
        u32 flags = NDS_RENDERER_TILE_RENDER_SEEN;

        stats->texture_render_tile = tile;
        stats->texture_render_tile_line = line;
        stats->texture_render_tile_tmem = tmem;
        stats->texture_render_tile_palette = palette;
        stats->texture_render_tile_cms = cms;
        stats->texture_render_tile_cmt = cmt;
        stats->texture_render_tile_masks = masks;
        stats->texture_render_tile_maskt = maskt;
        stats->texture_render_tile_shifts = shifts;
        stats->texture_render_tile_shiftt = shiftt;

        if ((cms & NDS_RENDERER_TX_CLAMP) != 0)
        {
            flags |= NDS_RENDERER_TILE_S_CLAMP;
        }
        if ((cms & NDS_RENDERER_TX_MIRROR) != 0)
        {
            flags |= NDS_RENDERER_TILE_S_MIRROR;
        }
        if (masks != 0)
        {
            flags |= NDS_RENDERER_TILE_S_MASKED;
        }
        if ((cmt & NDS_RENDERER_TX_CLAMP) != 0)
        {
            flags |= NDS_RENDERER_TILE_T_CLAMP;
        }
        if ((cmt & NDS_RENDERER_TX_MIRROR) != 0)
        {
            flags |= NDS_RENDERER_TILE_T_MIRROR;
        }
        if (maskt != 0)
        {
            flags |= NDS_RENDERER_TILE_T_MASKED;
        }
        stats->texture_render_tile_flags =
            (stats->texture_render_tile_flags & NDS_RENDERER_TILE_LOAD_SEEN) |
            flags;
    }
    else if (tile == NDS_RENDERER_LOAD_TILE)
    {
        stats->texture_load_tile = tile;
        stats->texture_render_tile_flags |= NDS_RENDERER_TILE_LOAD_SEEN;
    }

    (void)fmt;
    (void)siz;
}

static void ndsRendererRecordLoadBlock(NDSRendererStats *stats,
                                       u32 w0, u32 w1)
{
    if (stats == NULL)
    {
        return;
    }

    stats->texture_mask |= NDS_RENDERER_TEXTURE_LOADBLOCK;
    if (stats->texture_load_texels == 0)
    {
        stats->texture_load_tile = (w1 >> 24) & 0x7u;
        stats->texture_load_block_uls = (w0 >> 12) & 0x0FFFu;
        stats->texture_load_block_ult = w0 & 0x0FFFu;
        stats->texture_load_block_lrs = (w1 >> 12) & 0x0FFFu;
        stats->texture_load_block_dxt = w1 & 0x0FFFu;
        stats->texture_load_texels =
            stats->texture_load_block_lrs + 1u;
    }
}

static void ndsRendererRecordSetTileSize(NDSRendererStats *stats,
                                         u32 w0, u32 w1)
{
    u32 uls;
    u32 ult;
    u32 lrs;
    u32 lrt;

    if (stats == NULL)
    {
        return;
    }

    stats->texture_mask |= NDS_RENDERER_TEXTURE_SETTILESIZE;
    if ((stats->texture_tile_width != 0) &&
        (stats->texture_tile_height != 0))
    {
        return;
    }

    uls = (w0 >> 12) & 0x0FFFu;
    ult = w0 & 0x0FFFu;
    lrs = (w1 >> 12) & 0x0FFFu;
    lrt = w1 & 0x0FFFu;

    stats->texture_tile_size_tile = (w1 >> 24) & 0x7u;
    stats->texture_tile_size_uls = uls;
    stats->texture_tile_size_ult = ult;
    stats->texture_tile_size_lrs = lrs;
    stats->texture_tile_size_lrt = lrt;
    if (lrs >= uls)
    {
        stats->texture_tile_width = ((lrs - uls) >> 2) + 1u;
    }
    if (lrt >= ult)
    {
        stats->texture_tile_height = ((lrt - ult) >> 2) + 1u;
    }
}

static void ndsRendererRecordSetImage(NDSRendererStats *stats,
                                      u32 w0, u32 w1)
{
    if (stats == NULL)
    {
        return;
    }

    stats->texture_mask |= NDS_RENDERER_TEXTURE_SETTIMG;
    if (stats->texture_image == 0)
    {
        stats->texture_format = (w0 >> 21) & 0x7u;
        stats->texture_size = (w0 >> 19) & 0x3u;
        stats->texture_image_width = (w0 & 0x0FFFu) + 1u;
        stats->texture_image = w1;
    }
}

static void ndsRendererRecordSetCombine(NDSRendererStats *stats,
                                        u32 w0, u32 w1)
{
    if (stats == NULL)
    {
        return;
    }

    stats->texture_mask |= NDS_RENDERER_TEXTURE_SETCOMBINE;
    stats->texture_combine_count++;
    if (stats->texture_combine_w0 == 0)
    {
        stats->texture_combine_w0 = w0;
        stats->texture_combine_w1 = w1;
    }
}

static void ndsRendererScanList(const Gfx *dl,
                                const NDSRendererConfig *config,
                                NDSRendererStats *stats,
                                u32 depth,
                                NDSRendererCommandCallback callback,
                                void *callback_user)
{
    u32 i;

    if (stats->blocker != NDS_RENDERER_BLOCKER_NONE)
    {
        return;
    }
    if (depth > config->max_depth)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_TOO_DEEP;
        return;
    }
    if (ndsRendererValidateCommand(dl, config) == FALSE)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
        return;
    }
    if (depth > stats->max_depth_seen)
    {
        stats->max_depth_seen = depth;
    }

    for (i = 0; i < config->max_list_commands; i++, dl++)
    {
        u32 w0;
        u32 w1;
        u32 op;
        NDSRendererCommand command;

        if (ndsRendererValidateCommand(dl, config) == FALSE)
        {
            stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
            return;
        }
        if (stats->command_count >= config->max_commands)
        {
            stats->blocker = NDS_RENDERER_BLOCKER_BUDGET;
            return;
        }

        w0 = dl->words.w0;
        w1 = dl->words.w1;
        op = w0 >> 24;
        memset(&command, 0, sizeof(command));
        command.dl = dl;
        command.w0 = w0;
        command.w1 = w1;
        command.op = op;
        command.depth = depth;
        command.list_index = i;

        if (op == NDS_RENDERER_OP_DL)
        {
            command.raw_branch_dl = (const Gfx *)(uintptr_t)w1;
            command.resolved_branch_dl = command.raw_branch_dl;
            if (config->resolve_branch != NULL)
            {
                command.resolved_branch_dl = config->resolve_branch(
                    command.raw_branch_dl,
                    &command.branch_resolve_kind,
                    config->user);
            }
            command.branch_is_jump =
                ((w0 & (1u << 16)) != 0) ? TRUE : FALSE;
        }

        if (stats->first_opcode == 0)
        {
            stats->first_opcode = op;
        }
        stats->command_count++;

        if ((callback != NULL) &&
            (callback(&command, callback_user) == FALSE))
        {
            ndsRendererRecordUnsupported(stats, op);
            stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
            return;
        }

        switch (op)
        {
        case NDS_RENDERER_OP_NOOP:
            stats->skip_command_count++;
            break;

        case NDS_RENDERER_OP_MODIFYVTX:
            stats->state_command_count++;
            stats->skip_command_count++;
            break;

        case NDS_RENDERER_OP_VTX:
        {
            u32 v0;
            u32 count;

            stats->vertex_command_count++;
            stats->state_command_count++;
            if (ndsGBIDecodeF3DEX2Vtx(w0, NDS_RENDERER_MAX_VTX, &v0,
                                      &count) == FALSE)
            {
                stats->skip_command_count++;
                break;
            }
            if ((v0 + count) > stats->vertex_count)
            {
                stats->vertex_count = v0 + count;
            }
            break;
        }

        case NDS_RENDERER_OP_TRI1:
            stats->triangle_command_count++;
            stats->triangle_count++;
            stats->render_command_count++;
            break;

        case NDS_RENDERER_OP_TRI2:
            stats->triangle_command_count++;
            stats->triangle_count += 2u;
            stats->render_command_count++;
            break;

        case NDS_RENDERER_OP_ENDDL:
            stats->end_command_count++;
            stats->skip_command_count++;
            return;

        case NDS_RENDERER_OP_DL:
        {
            const Gfx *raw_branch = command.raw_branch_dl;
            const Gfx *branch = command.resolved_branch_dl;

            stats->branch_command_count++;
            if (stats->first_branch_dl == NULL)
            {
                stats->first_branch_dl = raw_branch;
            }
            if (stats->first_resolved_branch_dl == NULL)
            {
                stats->first_resolved_branch_dl = branch;
            }
            if (command.branch_resolve_kind == NDS_RENDERER_RESOLVE_SEGMENT)
            {
                stats->segment_resolve_count++;
            }
            if (ndsRendererValidateCommand(branch, config) == FALSE)
            {
                stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
                return;
            }
            if ((w0 & (1u << 16)) != 0)
            {
                stats->branch_jump_count++;
                ndsRendererScanList(branch, config, stats, depth + 1u,
                                    callback, callback_user);
                return;
            }

            stats->branch_call_count++;
            ndsRendererScanList(branch, config, stats, depth + 1u,
                                callback, callback_user);
            if (stats->blocker != NDS_RENDERER_BLOCKER_NONE)
            {
                return;
            }
            break;
        }

        case NDS_RENDERER_OP_TEXTURE:
            ndsRendererRecordTextureState(stats, w0, w1);
            stats->state_command_count++;
            break;

        case NDS_RENDERER_OP_MOVEWORD:
        case NDS_RENDERER_OP_SETSCISSOR:
        case NDS_RENDERER_OP_SETCIMG:
            stats->state_command_count++;
            stats->ignored_state_command_count++;
            break;

        case NDS_RENDERER_OP_GEOMETRYMODE:
            stats->geometry_mode = (stats->geometry_mode & w0) | w1;
            stats->geometry_clear_mask = w0;
            stats->geometry_set_mask = w1;
            stats->geometry_command_count++;
            stats->state_command_count++;
            break;

        case NDS_RENDERER_OP_SETCOMBINE:
            ndsRendererRecordSetCombine(stats, w0, w1);
            stats->state_command_count++;
            break;

        case NDS_RENDERER_OP_SETTIMG:
            ndsRendererRecordSetImage(stats, w0, w1);
            stats->state_command_count++;
            break;

        case NDS_RENDERER_OP_SETTILE:
            ndsRendererRecordSetTile(stats, w0, w1);
            stats->state_command_count++;
            break;

        case NDS_RENDERER_OP_LOADBLOCK:
            ndsRendererRecordLoadBlock(stats, w0, w1);
            stats->state_command_count++;
            break;

        case NDS_RENDERER_OP_LOADTLUT:
            stats->texture_command_count++;
            stats->state_command_count++;
            stats->ignored_state_command_count++;
            break;

        case NDS_RENDERER_OP_SETTILESIZE:
            ndsRendererRecordSetTileSize(stats, w0, w1);
            stats->state_command_count++;
            break;

        case NDS_RENDERER_OP_SETFOGCOLOR:
        case NDS_RENDERER_OP_SETBLENDCOLOR:
        case NDS_RENDERER_OP_SETENVCOLOR:
        case NDS_RENDERER_OP_SETPRIMCOLOR:
            stats->state_command_count++;
            stats->color_command_count++;
            if (op == NDS_RENDERER_OP_SETPRIMCOLOR)
            {
                stats->prim_color = w1;
            }
            break;

        case NDS_RENDERER_OP_RDPPIPESYNC:
        case NDS_RENDERER_OP_RDPLOADSYNC:
        case NDS_RENDERER_OP_RDPTILESYNC:
        case NDS_RENDERER_OP_RDPFULLSYNC:
            stats->skip_command_count++;
            stats->sync_command_count++;
            break;

        case NDS_RENDERER_OP_SETOTHERMODE_H:
        case NDS_RENDERER_OP_SETOTHERMODE_L:
        case NDS_RENDERER_OP_RDPSETOTHERMODE:
            ndsRendererRecordIgnoredOtherMode(stats, op, w0, w1);
            break;

        case NDS_RENDERER_OP_CULLDL:
            ndsRendererRecordCull(stats, w0, w1);
            break;

        default:
            ndsRendererRecordUnsupported(stats, op);
            break;
        }
    }
}

void ndsRendererInitStats(NDSRendererStats *stats)
{
    if (stats != NULL)
    {
        memset(stats, 0, sizeof(*stats));
    }
}

void ndsRendererScanDisplayList(const Gfx *dl,
                                const NDSRendererConfig *config,
                                NDSRendererStats *stats)
{
    if (stats == NULL)
    {
        return;
    }

    if (config == NULL)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
        return;
    }

    ndsRendererScanList(dl, config, stats, 0, NULL, NULL);
    if (stats->blocker != NDS_RENDERER_BLOCKER_NONE)
    {
        return;
    }
    if (stats->unsupported_command_count != 0)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
        return;
    }
    if (stats->vertex_count == 0)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_NO_VERTICES;
        return;
    }
    if (stats->triangle_count == 0)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_NO_TRIANGLES;
        return;
    }
    if (stats->end_command_count == 0)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_NO_END;
        return;
    }
}

void ndsRendererExecuteDisplayList(const Gfx *dl,
                                   const NDSRendererConfig *config,
                                   NDSRendererCommandCallback callback,
                                   void *callback_user,
                                   NDSRendererStats *stats)
{
    if (stats == NULL)
    {
        return;
    }

    if (config == NULL)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
        return;
    }

    ndsRendererScanList(dl, config, stats, 0, callback, callback_user);
    if (stats->blocker != NDS_RENDERER_BLOCKER_NONE)
    {
        return;
    }
    if (stats->unsupported_command_count != 0)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
        return;
    }
    if (stats->end_command_count == 0)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_NO_END;
        return;
    }
}
