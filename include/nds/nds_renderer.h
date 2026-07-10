#ifndef SSB64_NDS_RENDERER_H
#define SSB64_NDS_RENDERER_H

#include <stddef.h>
#include <PR/gbi.h>

#define NDS_RENDERER_BLOCKER_NONE 0u
#define NDS_RENDERER_BLOCKER_BAD_BRANCH 1u
#define NDS_RENDERER_BLOCKER_TOO_DEEP 2u
#define NDS_RENDERER_BLOCKER_BUDGET 3u
#define NDS_RENDERER_BLOCKER_UNSUPPORTED 4u
#define NDS_RENDERER_BLOCKER_NO_VERTICES 5u
#define NDS_RENDERER_BLOCKER_NO_TRIANGLES 6u
#define NDS_RENDERER_BLOCKER_NO_END 7u

#define NDS_RENDERER_RESOLVE_NONE 0u
#define NDS_RENDERER_RESOLVE_SEGMENT 1u

#define NDS_RENDERER_TEXTURE_SETCOMBINE (1u << 0)
#define NDS_RENDERER_TEXTURE_SETTILE (1u << 1)
#define NDS_RENDERER_TEXTURE_TEXTURE (1u << 2)
#define NDS_RENDERER_TEXTURE_SETTILESIZE (1u << 3)
#define NDS_RENDERER_TEXTURE_SETTIMG (1u << 4)
#define NDS_RENDERER_TEXTURE_LOADBLOCK (1u << 5)
#define NDS_RENDERER_TEXTURE_LOADTILE (1u << 6)

#define NDS_RENDERER_TEXTURE_STATE_SEEN (1u << 0)
#define NDS_RENDERER_TEXTURE_STATE_ON (1u << 1)
#define NDS_RENDERER_TEXTURE_STATE_SCALE_S (1u << 2)
#define NDS_RENDERER_TEXTURE_STATE_SCALE_T (1u << 3)

#define NDS_RENDERER_TILE_RENDER_SEEN (1u << 0)
#define NDS_RENDERER_TILE_LOAD_SEEN (1u << 1)
#define NDS_RENDERER_TILE_S_CLAMP (1u << 2)
#define NDS_RENDERER_TILE_S_MIRROR (1u << 3)
#define NDS_RENDERER_TILE_S_MASKED (1u << 4)
#define NDS_RENDERER_TILE_T_CLAMP (1u << 5)
#define NDS_RENDERER_TILE_T_MIRROR (1u << 6)
#define NDS_RENDERER_TILE_T_MASKED (1u << 7)

#define NDS_RENDERER_VERTEX_CACHE_SIZE 32u
#define NDS_RENDERER_TILE_COUNT 8u

#define NDS_RENDERER_GEOM_ZBUFFER 0x00000001u
#define NDS_RENDERER_GEOM_SHADE 0x00000004u
#define NDS_RENDERER_GEOM_CULL_FRONT 0x00000200u
#define NDS_RENDERER_GEOM_CULL_BACK 0x00000400u
#define NDS_RENDERER_GEOM_FOG 0x00010000u
#define NDS_RENDERER_GEOM_LIGHTING 0x00020000u
#define NDS_RENDERER_GEOM_TEXTURE_GEN 0x00040000u
#define NDS_RENDERER_GEOM_TEXTURE_GEN_LINEAR 0x00080000u
#define NDS_RENDERER_GEOM_SHADING_SMOOTH 0x00200000u
#define NDS_RENDERER_GEOM_RESET_MODE \
    (NDS_RENDERER_GEOM_ZBUFFER | NDS_RENDERER_GEOM_SHADE | \
     NDS_RENDERER_GEOM_CULL_BACK | NDS_RENDERER_GEOM_SHADING_SMOOTH)

typedef s32 (*NDSRendererValidateRange)(const Gfx *dl, size_t bytes,
                                        void *user);
typedef const Gfx *(*NDSRendererResolveBranch)(const Gfx *dl,
                                               u32 *resolve_kind,
                                               void *user);
typedef const void *(*NDSRendererResolveData)(const void *ptr, size_t bytes,
                                              void *user);

typedef struct NDSRendererMatrix20p12
{
    s32 m[4][4];
} NDSRendererMatrix20p12;

typedef struct NDSRendererInputVertex
{
    s16 x;
    s16 y;
    s16 z;
    s16 s;
    s16 t;
    u8 r;
    u8 g;
    u8 b;
    u8 a;
} NDSRendererInputVertex;

typedef struct NDSRendererClipVertex20p12
{
    s32 x;
    s32 y;
    s32 z;
    s32 w;
} NDSRendererClipVertex20p12;

typedef struct NDSRendererVertexCache
{
    NDSRendererInputVertex input_vertices[NDS_RENDERER_VERTEX_CACHE_SIZE];
    NDSRendererClipVertex20p12
        transformed_vertices[NDS_RENDERER_VERTEX_CACHE_SIZE];
    u32 input_valid_mask;
    u32 transformed_valid_mask;
} NDSRendererVertexCache;

typedef struct NDSRendererCommand
{
    const Gfx *dl;
    u32 w0;
    u32 w1;
    u32 op;
    u32 depth;
    u32 list_index;
    const Gfx *raw_branch_dl;
    const Gfx *resolved_branch_dl;
    u32 branch_resolve_kind;
    u32 branch_is_jump;
    const NDSRendererClipVertex20p12 *transformed_vertices;
    u32 transformed_vertex_valid_mask;
    u32 matrix_valid;
} NDSRendererCommand;

typedef struct NDSRendererTileState
{
    u32 set_seen;
    u32 size_seen;
    u32 format;
    u32 size;
    u32 line;
    u32 tmem;
    u32 palette;
    u32 cms;
    u32 cmt;
    u32 masks;
    u32 maskt;
    u32 shifts;
    u32 shiftt;
    u32 uls;
    u32 ult;
    u32 lrs;
    u32 lrt;
    u32 width;
    u32 height;
    u32 flags;
} NDSRendererTileState;

typedef s32 (*NDSRendererCommandCallback)(const NDSRendererCommand *command,
                                          void *user);

typedef enum NDSRendererTextureDataLayout
{
    NDS_RENDERER_TEXTURE_DATA_NATIVE = 0,
    NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED = 1
} NDSRendererTextureDataLayout;

typedef struct NDSRendererConfig
{
    u32 max_depth;
    u32 max_commands;
    u32 max_list_commands;
    const NDSRendererMatrix20p12 *initial_projection;
    const NDSRendererMatrix20p12 *initial_modelview;
    u32 initial_geometry_mode;
    NDSRendererTextureDataLayout texture_data_layout;
    NDSRendererValidateRange validate_range;
    NDSRendererResolveBranch resolve_branch;
    NDSRendererResolveData resolve_data;
    void *user;
} NDSRendererConfig;

typedef struct NDSRendererStats
{
    u32 blocker;
    u32 command_count;
    u32 vertex_count;
    u32 triangle_count;
    u32 first_opcode;
    u32 unsupported_opcode;
    u32 vertex_command_count;
    u32 triangle_command_count;
    u32 matrix_command_count;
    u32 matrix_load_count;
    u32 matrix_mul_count;
    u32 matrix_projection_count;
    u32 matrix_modelview_count;
    u32 matrix_push_count;
    u32 matrix_pop_count;
    u32 matrix_transform_count;
    u32 matrix_mvp_recalc_count;
    u32 matrix_move_word_count;
    u32 transformed_vertex_count;
    u32 transformed_triangle_count;
    u32 hardware_vertex_count;
    u32 hardware_triangle_count;
    u32 hardware_zbuffer_triangle_count;
    u32 hardware_projected_depth_triangle_count;
    u32 hardware_decal_depth_triangle_count;
    u32 hardware_prim_depth_triangle_count;
    u32 hardware_oracle_triangle_count;
    u32 hardware_oracle_reject_count;
    u32 hardware_matrix_seed_count;
    u32 hardware_texture_bind_count;
    u32 hardware_texture_upload_count;
    u32 hardware_texture_ready_count;
    u32 hardware_texture_reject_count;
    u32 hardware_texture_format;
    u32 hardware_texture_width;
    u32 hardware_texture_height;
    u32 first_transformed_tri_v0;
    u32 first_transformed_tri_v1;
    u32 first_transformed_tri_v2;
    u32 matrix_flags;
    s32 first_transformed_x;
    s32 first_transformed_y;
    s32 first_transformed_z;
    s32 first_transformed_w;
    u32 sync_command_count;
    u32 end_command_count;
    u32 branch_command_count;
    u32 branch_call_count;
    u32 branch_jump_count;
    u32 segment_resolve_count;
    u32 othermode_command_count;
    u32 color_command_count;
    u32 light_color_command_count;
    u32 light_direction_command_count;
    u32 light_fallback_count;
    u32 unsupported_command_count;
    u32 state_command_count;
    u32 skip_command_count;
    u32 render_command_count;
    u32 max_depth_seen;
    u32 cull_command_count;
    u32 ignored_state_command_count;
    u32 first_othermode_opcode;
    u32 first_othermode_w0;
    u32 first_othermode_w1;
    u32 othermode_h;
    u32 othermode_l;
    u32 first_cull_w0;
    u32 first_cull_w1;
    const Gfx *first_branch_dl;
    const Gfx *first_resolved_branch_dl;
    u32 geometry_mode;
    u32 geometry_clear_mask;
    u32 geometry_set_mask;
    u32 geometry_command_count;
    u32 texture_mask;
    u32 texture_load_kind;
    u32 texture_command_count;
    u32 texture_scale_s;
    u32 texture_scale_t;
    u32 texture_level;
    u32 texture_tile;
    u32 texture_on;
    u32 texture_xparam;
    u32 texture_state_flags;
    u32 texture_image;
    u32 texture_format;
    u32 texture_size;
    u32 texture_image_width;
    u32 texture_set_tile_count;
    u32 texture_tlut_image;
    u32 texture_tlut_count;
    u32 texture_tlut_tile;
    u32 texture_render_tile;
    u32 texture_render_tile_format;
    u32 texture_render_tile_size;
    u32 texture_render_tile_line;
    u32 texture_render_tile_tmem;
    u32 texture_render_tile_palette;
    u32 texture_render_tile_cms;
    u32 texture_render_tile_cmt;
    u32 texture_render_tile_masks;
    u32 texture_render_tile_maskt;
    u32 texture_render_tile_shifts;
    u32 texture_render_tile_shiftt;
    u32 texture_render_tile_flags;
    u32 texture_load_tile;
    u32 texture_load_block_uls;
    u32 texture_load_block_ult;
    u32 texture_load_block_lrs;
    u32 texture_load_block_dxt;
    u32 texture_load_texels;
    u32 texture_tile_size_tile;
    u32 texture_tile_size_uls;
    u32 texture_tile_size_ult;
    u32 texture_tile_size_lrs;
    u32 texture_tile_size_lrt;
    u32 texture_tile_width;
    u32 texture_tile_height;
    NDSRendererTileState texture_tiles[NDS_RENDERER_TILE_COUNT];
    u32 texture_combine_w0;
    u32 texture_combine_w1;
    u32 texture_combine_count;
    u32 prim_color;
    u32 env_color;
    u32 blend_color;
    u32 light_color_1;
    u32 light_color_2;
    u32 light_color_mask;
    s32 light_dir_x;
    s32 light_dir_y;
    s32 light_dir_z;
    u32 light_dir_mask;
    u32 prim_depth;
    u32 prim_depth_delta;
    u32 prim_depth_command_count;
    u32 fog_color;
    s32 fog_min;
    s32 fog_max;
    u32 fog_status;
} NDSRendererStats;

s32 ndsRendererMtxCellS16p16(const Mtx *mtx, u32 row, u32 col);
void ndsRendererMtxLoadN64ToDS20p12(const Mtx *src,
                                    NDSRendererMatrix20p12 *dst);
void ndsRendererMtxMul20p12(const NDSRendererMatrix20p12 *lhs,
                            const NDSRendererMatrix20p12 *rhs,
                            NDSRendererMatrix20p12 *out);
void ndsRendererTransformVertex20p12(const NDSRendererMatrix20p12 *mtx,
                                     const NDSRendererInputVertex *vtx,
                                     NDSRendererClipVertex20p12 *out);
void ndsRendererInitStats(NDSRendererStats *stats);
void ndsRendererScanDisplayList(const Gfx *dl,
                                const NDSRendererConfig *config,
                                NDSRendererStats *stats);
void ndsRendererExecuteDisplayList(const Gfx *dl,
                                   const NDSRendererConfig *config,
                                   NDSRendererCommandCallback callback,
                                   void *callback_user,
                                   NDSRendererStats *stats);
void ndsRendererExecuteDisplayListWithVertexCache(
    const Gfx *dl,
    const NDSRendererConfig *config,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    NDSRendererVertexCache *vertex_cache);
void ndsRendererHardwareSetNoOracle(u32 enabled);
u32 ndsRendererHardwareNoOracleEnabled(void);
u32 ndsRendererHardwareConsumeSubmittedFrame(void);

#endif
