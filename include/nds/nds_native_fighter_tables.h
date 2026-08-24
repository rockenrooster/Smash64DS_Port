#ifndef NDS_NATIVE_FIGHTER_TABLES_H
#define NDS_NATIVE_FIGHTER_TABLES_H

/* The generated native-fighter table ELEMENT types, and nothing else.
 *
 * These five structs were local to `src/nds/nds_renderer.c` while the renderer
 * was their only reader. Board row P2-3r4 gives them a second one: a P2-3
 * owner's tables are emitted as a struct image, compiled standalone, and
 * objcopied into a NitroFS payload so they stop costing the ARM9 binary (and
 * therefore the taskman arena) one byte for one byte. The image and the
 * renderer must agree on the layout EXACTLY, so there is deliberately one
 * definition rather than two that look alike.
 *
 * Only the ungated element types live here. `NDSNativePreparedDenseVertex` does
 * not: it is mutable draw scratch whose shape depends on build config, it is
 * never part of an image, and it stays where its gates are.
 *
 * `u8`/`u16`/`s16`/`u32` come from the port's own types; a translation unit
 * including this header is expected to have them already.
 */

typedef struct NDSNativeStateDelta
{
    u32 w0;
    u32 w1;
    u8 effect;
    u8 reserved[3];
} NDSNativeStateDelta;

typedef struct NDSNativeVertexAction
{
    u8 kind;
    u8 command_index;
    u8 index;
    u8 count;
    u32 source_offset;
    s16 s;
    s16 t;
} NDSNativeVertexAction;

typedef struct NDSNativeDenseVertex
{
    u32 rgba;
    s16 s;
    s16 t;
    u8 matrix_binding;
    u8 cache_slot;
    u16 reserved;
} NDSNativeDenseVertex;

typedef struct NDSNativeRun
{
    u16 first_triangle;
    u8 triangle_count;
    u8 submit_class;
    u32 required_mask;
} NDSNativeRun;

typedef struct NDSNativeEpoch
{
    u16 before_state_first;
    u16 after_state_first;
    u16 first_action;
    u16 first_run;
    u8 before_state_count;
    u8 after_state_count;
    u8 before_sync_count;
    u8 after_sync_count;
    u8 action_count;
    u8 run_count;
    u8 material_slot;
    u8 first_triangle_command_index;
} NDSNativeEpoch;

#endif /* NDS_NATIVE_FIGHTER_TABLES_H */
