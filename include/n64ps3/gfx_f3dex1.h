#ifndef N64PS3_GFX_F3DEX1_H
#define N64PS3_GFX_F3DEX1_H

#include "n64ps3/gfx.h"

#ifdef __cplusplus
extern "C" {
#endif

#define N64PS3_GFX_F3DEX1_VERTEX_CACHE_SIZE 64U
#define N64PS3_GFX_F3DEX1_MAX_COMMANDS 16384U
#define N64PS3_GFX_F3DEX1_MAX_DEPTH 8U
#define N64PS3_GFX_F3DEX1_MATRIX_STACK_SIZE 18U
#define N64PS3_GFX_F3DEX1_MAX_LIGHTS 7U
#define N64PS3_GFX_F3DEX1_GEOMETRY_ZBUFFER 0x00000001U
#define N64PS3_GFX_F3DEX1_GEOMETRY_CULL_FRONT 0x00001000U
#define N64PS3_GFX_F3DEX1_GEOMETRY_CULL_BACK 0x00002000U
#define N64PS3_GFX_F3DEX1_GEOMETRY_LIGHTING 0x00020000U

typedef struct N64ps3GfxF3dex1Vertex {
    s16 x;
    s16 y;
    s16 z;
    u16 flag;
    s16 s;
    s16 t;
    u8 color[4];
    s8 normal[3];
    u32 geometry_mode;
    const u8 *texture_data;
    u8 texture_bytes_per_texel;
    u16 texture_width;
    u16 texture_height;
    u16 texture_scale_s;
    u16 texture_scale_t;
    u16 texture_uls;
    u16 texture_ult;
    u8 texture_wrap_s;
    u8 texture_wrap_t;
    u8 texture_mask_s;
    u8 texture_mask_t;
    u8 texture_shift_s;
    u8 texture_shift_t;
    u8 texture_enabled;
    u8 texture_point_filter;
    u8 blend_enabled;
    float clip_x;
    float clip_y;
    float clip_z;
    float clip_w;
    u8 clip_valid;
} N64ps3GfxF3dex1Vertex;

typedef enum N64ps3GfxF3dex1Status {
    N64PS3_GFX_F3DEX1_OK = 0,
    N64PS3_GFX_F3DEX1_INVALID_ARGUMENT,
    N64PS3_GFX_F3DEX1_UNSUPPORTED_TASK,
    N64PS3_GFX_F3DEX1_ADDRESS_UNMAPPED,
    N64PS3_GFX_F3DEX1_INVALID_VERTEX_LOAD,
    N64PS3_GFX_F3DEX1_INVALID_TRIANGLE,
    N64PS3_GFX_F3DEX1_DEPTH_LIMIT,
    N64PS3_GFX_F3DEX1_COMMAND_LIMIT,
    N64PS3_GFX_F3DEX1_TRUNCATED_LIST,
    N64PS3_GFX_F3DEX1_CALLBACK_ABORTED,
    N64PS3_GFX_F3DEX1_INVALID_MATRIX,
    N64PS3_GFX_F3DEX1_INVALID_MATRIX_FLAGS,
    N64PS3_GFX_F3DEX1_MATRIX_STACK_OVERFLOW,
    N64PS3_GFX_F3DEX1_MATRIX_STACK_UNDERFLOW
} N64ps3GfxF3dex1Status;

typedef struct N64ps3GfxF3dex1Stats {
    u32 command_count;
    u32 vertex_load_count;
    u32 triangle_count;
    u32 display_list_call_count;
    u32 unsupported_command_count;
    u32 matrix_command_count;
    u32 matrix_push_count;
    u32 matrix_pop_count;
    u32 transformed_vertex_count;
    u32 geometry_mode_command_count;
    u32 light_command_count;
    u32 texture_load_count;
    u32 textured_triangle_count;
    u32 fill_color_command_count;
    u32 fill_rectangle_count;
    u32 max_depth;
    u32 unsupported_opcode_counts[256];
} N64ps3GfxF3dex1Stats;

typedef int (*N64ps3GfxF3dex1TriangleCallback)(
    void *user_data,
    const N64ps3GfxF3dex1Vertex *a,
    const N64ps3GfxF3dex1Vertex *b,
    const N64ps3GfxF3dex1Vertex *c);

/* Decodes F3DEX1 display-list flow, MTX/POPMTX, VTX, TRI1/TRI2, MOVEWORD
 * segments, light state, RGBA16/RGBA32 texture blocks, SETOTHERMODE_L FORCE_BL,
 * SETOTHERMODE_H texture filtering, and RDP SETFILLCOLOR/FILLRECT. Fill
 * rectangles use the 320x240 logical viewport
 * and are emitted as two screen-space triangles. Other fixed-width commands
 * are counted and skipped.
 * Vertices passed to the callback are decoder-owned and valid only for the
 * duration of that call; clip coordinates contain the matrix-transformed vertex
 * position, geometry_mode contains the RSP mode, and blend_enabled reflects
 * the active RDP FORCE_BL bit from SETOTHERMODE_L and texture_point_filter
 * reflects G_TF_POINT in SETOTHERMODE_H.
 * When G_LIGHTING is enabled, color contains the computed ambient/directional
 * RGB and the original signed normal bytes are available in normal[]. Texture
 * data points into task memory and remains valid through the callback. Texture
 * coordinates remain in N64 fixed-point units in s/t; tile dimensions, origin,
 * mask, shift, and wrap state describe the active tile.
 */
N64ps3GfxF3dex1Status n64ps3_gfx_decode_f3dex1(
    const N64ps3GfxTask *task,
    N64ps3GfxF3dex1TriangleCallback triangle_callback,
    void *user_data,
    N64ps3GfxF3dex1Stats *stats);

#ifdef __cplusplus
}
#endif

#endif
