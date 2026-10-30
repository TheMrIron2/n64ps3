#include "n64ps3/gfx_f3dex1.h"

#include <math.h>
#include <string.h>

#define F3DEX1_OP_MTX 0x01U
#define F3DEX1_OP_MOVEMEM 0x03U
#define F3DEX1_OP_VTX 0x04U
#define F3DEX1_OP_DL 0x06U
#define F3DEX1_OP_TRI1 0xbfU
#define F3DEX1_OP_TRI2 0xb1U
#define F3DEX1_OP_POPMTX 0xbdU
#define F3DEX1_OP_MOVEWORD 0xbcU
#define F3DEX1_OP_SETOTHERMODE_L 0xb9U
#define F3DEX1_OP_ENDDL 0xb8U
#define F3DEX1_OP_CLEARGEOMETRYMODE 0xb6U
#define F3DEX1_OP_SETGEOMETRYMODE 0xb7U
#define F3DEX1_OP_TEXTURE 0xd7U
#define F3DEX1_OP_LOADSYNC 0xe6U
#define F3DEX1_OP_PIPESYNC 0xe7U
#define F3DEX1_OP_TILESYNC 0xe8U
#define F3DEX1_OP_FILLRECT 0xf6U
#define F3DEX1_OP_SETFILLCOLOR 0xf7U
#define F3DEX1_OP_SETTILESIZE 0xf2U
#define F3DEX1_OP_LOADBLOCK 0xf3U
#define F3DEX1_OP_SETTILE 0xf5U
#define F3DEX1_OP_SETTIMG 0xfdU
#define F3DEX1_MTX_PROJECTION 0x04U
#define F3DEX1_MTX_LOAD 0x02U
#define F3DEX1_MTX_PUSH 0x01U
#define F3DEX1_MTX_FLAGS_MASK 0x07U
#define F3DEX1_MATRIX_BYTES 64U
#define F3DEX1_MOVEWORD_SEGMENT 0x06U
#define F3DEX1_MOVEWORD_NUMLIGHT 0x02U
#define F3DEX1_LIGHTING_MODE N64PS3_GFX_F3DEX1_GEOMETRY_LIGHTING
#define F3DEX1_LIGHT_BASE 0x86U
#define F3DEX1_LIGHT_BYTES 16U
#define F3DEX1_DL_NOPUSH 0x01U
#define F3DEX1_VERTEX_BYTES 16U
#define F3DEX1_RDRAM_ADDRESS_MASK 0x1fffffffU
#define F3DEX1_RDP_FORCE_BLEND 0x00004000U
#define F3DEX1_FILLRECT_SCREEN_WIDTH 320.0f
#define F3DEX1_FILLRECT_SCREEN_HEIGHT 240.0f

typedef struct N64ps3GfxF3dex1Tile {
    u8 format;
    u8 size;
    u8 wrap_s;
    u8 wrap_t;
    u8 mask_s;
    u8 mask_t;
    u8 shift_s;
    u8 shift_t;
    u16 width;
    u16 height;
    u16 uls;
    u16 ult;
} N64ps3GfxF3dex1Tile;

typedef struct N64ps3GfxF3dex1Decoder {
    const N64ps3GfxTask *task;
    N64ps3GfxF3dex1TriangleCallback triangle_callback;
    void *user_data;
    N64ps3GfxF3dex1Stats *stats;
    N64ps3GfxF3dex1Vertex vertices[N64PS3_GFX_F3DEX1_VERTEX_CACHE_SIZE];
    u8 vertex_valid[N64PS3_GFX_F3DEX1_VERTEX_CACHE_SIZE];
    u32 segment_base[16];
    u8 segment_valid[16];
    float modelview[4][4];
    float projection[4][4];
    float modelview_stack[N64PS3_GFX_F3DEX1_MATRIX_STACK_SIZE][4][4];
    float light_directions[N64PS3_GFX_F3DEX1_MAX_LIGHTS][3];
    u8 light_colors[N64PS3_GFX_F3DEX1_MAX_LIGHTS][3];
    u8 ambient_color[3];
    u32 modelview_stack_depth;
    u32 geometry_mode;
    u32 other_mode_l;
    u32 light_count;
    u32 texture_image_address;
    u32 texture_image_format;
    u32 texture_image_size;
    u32 texture_image_width;
    u32 texture_load_bytes;
    u32 fill_color;
    const u8 *texture_data;
    N64ps3GfxF3dex1Tile texture_tiles[8];
    u16 texture_scale_s;
    u16 texture_scale_t;
    u8 texture_tile;
    u8 texture_enabled;
    u8 texture_loaded;
    u8 has_projection;
} N64ps3GfxF3dex1Decoder;

static u32 f3dex1_segment_address(const N64ps3GfxF3dex1Decoder *decoder,
                                 u32 address,
                                 int *valid);
static const u8 *f3dex1_resolve(const N64ps3GfxF3dex1Decoder *decoder,
                                u32 address,
                                u32 required_size,
                                u32 *available_size);

static u16 f3dex1_read_be16(const u8 *bytes)
{
    return (u16)(((u16)bytes[0] << 8) | bytes[1]);
}

static u32 f3dex1_read_be32(const u8 *bytes)
{
    return ((u32)bytes[0] << 24) |
           ((u32)bytes[1] << 16) |
           ((u32)bytes[2] << 8) |
           (u32)bytes[3];
}

static void f3dex1_matrix_identity(float matrix[4][4])
{
    u32 row;
    u32 column;

    for (row = 0; row < 4U; row++) {
        for (column = 0; column < 4U; column++) {
            matrix[row][column] = row == column ? 1.0f : 0.0f;
        }
    }
}

static void f3dex1_matrix_copy(float destination[4][4],
                               float source[4][4])
{
    memcpy(destination, source, sizeof(float) * 16U);
}

static void f3dex1_matrix_multiply(float output[4][4],
                                   float left[4][4],
                                   float right[4][4])
{
    float result[4][4];
    u32 column;
    u32 row;

    /* Follows n64psp's matrix storage convention: first index is input column,
     * second index is output row. The operation is result = left * right. */
    for (column = 0; column < 4U; column++) {
        for (row = 0; row < 4U; row++) {
            result[column][row] =
                left[0][row] * right[column][0] +
                left[1][row] * right[column][1] +
                left[2][row] * right[column][2] +
                left[3][row] * right[column][3];
        }
    }
    f3dex1_matrix_copy(output, result);
}

static void f3dex1_transform_vector(float matrix[4][4],
                                    const float input[4],
                                    float output[4])
{
    u32 row;

    for (row = 0; row < 4U; row++) {
        output[row] = matrix[0][row] * input[0] +
                     matrix[1][row] * input[1] +
                     matrix[2][row] * input[2] +
                     matrix[3][row] * input[3];
    }
}

static void f3dex1_normalize_vector(float vector[3])
{
    float length_squared = vector[0] * vector[0] +
                           vector[1] * vector[1] +
                           vector[2] * vector[2];

    if (length_squared > 0.000001f) {
        float inverse_length = 1.0f / sqrtf(length_squared);

        vector[0] *= inverse_length;
        vector[1] *= inverse_length;
        vector[2] *= inverse_length;
    } else {
        vector[0] = 0.0f;
        vector[1] = 0.0f;
        vector[2] = 0.0f;
    }
}

static void f3dex1_transform_direction(float matrix[4][4],
                                       const float input[3],
                                       float output[3])
{
    float vector[4];
    float transformed[4];

    vector[0] = input[0];
    vector[1] = input[1];
    vector[2] = input[2];
    vector[3] = 0.0f;
    f3dex1_transform_vector(matrix, vector, transformed);
    output[0] = transformed[0];
    output[1] = transformed[1];
    output[2] = transformed[2];
    f3dex1_normalize_vector(output);
}

static u8 f3dex1_clamp_color(float value)
{
    if (value <= 0.0f) {
        return 0;
    }
    if (value >= 255.0f) {
        return 255;
    }
    return (u8)value;
}

static void f3dex1_shade_vertex(N64ps3GfxF3dex1Decoder *decoder,
                                N64ps3GfxF3dex1Vertex *vertex)
{
    float raw_normal[3];
    float normal[3];
    float red = (float)decoder->ambient_color[0];
    float green = (float)decoder->ambient_color[1];
    float blue = (float)decoder->ambient_color[2];
    u32 i;

    if ((decoder->geometry_mode & F3DEX1_LIGHTING_MODE) == 0) {
        return;
    }

    raw_normal[0] = (float)vertex->normal[0];
    raw_normal[1] = (float)vertex->normal[1];
    raw_normal[2] = (float)vertex->normal[2];
    f3dex1_transform_direction(decoder->modelview, raw_normal, normal);

    for (i = 0; i < decoder->light_count; i++) {
        float dot = normal[0] * decoder->light_directions[i][0] +
                    normal[1] * decoder->light_directions[i][1] +
                    normal[2] * decoder->light_directions[i][2];

        if (dot > 0.0f) {
            if (dot > 1.0f) {
                dot = 1.0f;
            }
            red += (float)decoder->light_colors[i][0] * dot;
            green += (float)decoder->light_colors[i][1] * dot;
            blue += (float)decoder->light_colors[i][2] * dot;
        }
    }

    vertex->color[0] = f3dex1_clamp_color(red);
    vertex->color[1] = f3dex1_clamp_color(green);
    vertex->color[2] = f3dex1_clamp_color(blue);
}

static N64ps3GfxF3dex1Status f3dex1_handle_light(
    N64ps3GfxF3dex1Decoder *decoder,
    u32 w0,
    u32 address)
{
    u32 index = (w0 >> 16) & 0xffU;
    u32 length = w0 & 0xffffU;
    u32 slot;
    int address_valid;
    const u8 *raw;

    if (length < F3DEX1_LIGHT_BYTES || index < F3DEX1_LIGHT_BASE ||
        index > F3DEX1_LIGHT_BASE + N64PS3_GFX_F3DEX1_MAX_LIGHTS * 2U ||
        ((index - F3DEX1_LIGHT_BASE) & 1U) != 0) {
        decoder->stats->unsupported_command_count++;
        return N64PS3_GFX_F3DEX1_OK;
    }

    slot = (index - F3DEX1_LIGHT_BASE) / 2U;
    if (slot > decoder->light_count) {
        decoder->stats->unsupported_command_count++;
        return N64PS3_GFX_F3DEX1_OK;
    }

    address = f3dex1_segment_address(decoder, address, &address_valid);
    if (!address_valid) {
        return N64PS3_GFX_F3DEX1_ADDRESS_UNMAPPED;
    }
    raw = f3dex1_resolve(decoder, address, F3DEX1_LIGHT_BYTES, NULL);
    if (!raw) {
        return N64PS3_GFX_F3DEX1_ADDRESS_UNMAPPED;
    }

    if (slot == decoder->light_count) {
        decoder->ambient_color[0] = raw[0];
        decoder->ambient_color[1] = raw[1];
        decoder->ambient_color[2] = raw[2];
    } else {
        float raw_direction[3];

        decoder->light_colors[slot][0] = raw[0];
        decoder->light_colors[slot][1] = raw[1];
        decoder->light_colors[slot][2] = raw[2];
        raw_direction[0] = (float)(s8)raw[8];
        raw_direction[1] = (float)(s8)raw[9];
        raw_direction[2] = (float)(s8)raw[10];
        f3dex1_transform_direction(decoder->modelview, raw_direction,
                                   decoder->light_directions[slot]);
    }
    decoder->stats->light_command_count++;
    return N64PS3_GFX_F3DEX1_OK;
}

static void f3dex1_handle_num_lights(N64ps3GfxF3dex1Decoder *decoder,
                                     u32 encoded_count)
{
    u32 count = (encoded_count & 0x7fffffffU) >> 5;

    if (count != 0) {
        count--;
    }
    if (count > N64PS3_GFX_F3DEX1_MAX_LIGHTS) {
        count = N64PS3_GFX_F3DEX1_MAX_LIGHTS;
    }
    decoder->light_count = count;
    memset(decoder->light_directions, 0, sizeof(decoder->light_directions));
    memset(decoder->light_colors, 0, sizeof(decoder->light_colors));
    memset(decoder->ambient_color, 0, sizeof(decoder->ambient_color));
    decoder->stats->light_command_count++;
}

static N64ps3GfxF3dex1Status f3dex1_handle_matrix(
    N64ps3GfxF3dex1Decoder *decoder,
    u32 w0,
    u32 address)
{
    float loaded[4][4];
    u32 flags = (w0 >> 16) & 0xffU;
    u32 row;
    u32 column;
    int address_valid;
    const u8 *raw;
    int projection = (flags & F3DEX1_MTX_PROJECTION) != 0;
    int load = (flags & F3DEX1_MTX_LOAD) != 0;

    decoder->stats->matrix_command_count++;
    if ((w0 & 0xffffU) != F3DEX1_MATRIX_BYTES) {
        return N64PS3_GFX_F3DEX1_INVALID_MATRIX;
    }
    if ((flags & ~F3DEX1_MTX_FLAGS_MASK) != 0) {
        return N64PS3_GFX_F3DEX1_INVALID_MATRIX_FLAGS;
    }
    if (!projection && (flags & F3DEX1_MTX_PUSH) != 0) {
        if (decoder->modelview_stack_depth >=
            N64PS3_GFX_F3DEX1_MATRIX_STACK_SIZE) {
            return N64PS3_GFX_F3DEX1_MATRIX_STACK_OVERFLOW;
        }
        f3dex1_matrix_copy(
            decoder->modelview_stack[decoder->modelview_stack_depth],
            decoder->modelview);
        decoder->modelview_stack_depth++;
        decoder->stats->matrix_push_count++;
    }

    address = f3dex1_segment_address(decoder, address, &address_valid);
    if (!address_valid) {
        return N64PS3_GFX_F3DEX1_ADDRESS_UNMAPPED;
    }
    raw = f3dex1_resolve(decoder, address, F3DEX1_MATRIX_BYTES, NULL);
    if (!raw) {
        return N64PS3_GFX_F3DEX1_ADDRESS_UNMAPPED;
    }

    for (row = 0; row < 4U; row++) {
        for (column = 0; column < 4U; column++) {
            u32 index = row * 4U + column;
            s16 integer = (s16)f3dex1_read_be16(raw + index * 2U);
            u16 fraction = f3dex1_read_be16(raw + 32U + index * 2U);

            loaded[row][column] = (float)integer +
                                  (float)fraction / 65536.0f;
        }
    }

    if (projection) {
        if (load) {
            f3dex1_matrix_copy(decoder->projection, loaded);
        } else {
            f3dex1_matrix_multiply(decoder->projection, decoder->projection,
                                   loaded);
        }
        decoder->has_projection = 1;
    } else if (load) {
        f3dex1_matrix_copy(decoder->modelview, loaded);
    } else {
        f3dex1_matrix_multiply(decoder->modelview, decoder->modelview, loaded);
    }
    return N64PS3_GFX_F3DEX1_OK;
}

static void f3dex1_transform_vertex(N64ps3GfxF3dex1Decoder *decoder,
                                    N64ps3GfxF3dex1Vertex *vertex)
{
    float input[4];
    float view[4];
    float clip[4];

    input[0] = (float)vertex->x;
    input[1] = (float)vertex->y;
    input[2] = (float)vertex->z;
    input[3] = 1.0f;
    f3dex1_transform_vector(decoder->modelview, input, view);

    if (decoder->has_projection) {
        f3dex1_transform_vector(decoder->projection, view, clip);
        vertex->clip_x = clip[0];
        vertex->clip_y = clip[1];
        vertex->clip_z = clip[2];
        vertex->clip_w = clip[3];
        vertex->clip_valid =
            (clip[3] <= -0.001f || clip[3] >= 0.001f) ? 1U : 0U;
    } else {
        vertex->clip_x = view[0] / 320.0f;
        vertex->clip_y = -view[1] / 240.0f;
        vertex->clip_z = view[2] / 4096.0f;
        vertex->clip_w = 1.0f;
        vertex->clip_valid = 1U;
    }
    decoder->stats->transformed_vertex_count++;
}

static N64ps3GfxF3dex1Status f3dex1_pop_matrix(
    N64ps3GfxF3dex1Decoder *decoder)
{
    if (decoder->modelview_stack_depth == 0) {
        return N64PS3_GFX_F3DEX1_MATRIX_STACK_UNDERFLOW;
    }

    decoder->modelview_stack_depth--;
    f3dex1_matrix_copy(decoder->modelview,
                       decoder->modelview_stack[decoder->modelview_stack_depth]);
    decoder->stats->matrix_pop_count++;
    return N64PS3_GFX_F3DEX1_OK;
}

static u32 f3dex1_segment_address(const N64ps3GfxF3dex1Decoder *decoder,
                                 u32 address,
                                 int *valid)
{
    u32 segment = address >> 24;
    u32 offset = address & 0x00ffffffU;

    *valid = 1;
    if (segment < 16U && decoder->segment_valid[segment]) {
        if (decoder->segment_base[segment] > 0xffffffffU - offset) {
            *valid = 0;
            return 0;
        }
        return decoder->segment_base[segment] + offset;
    }
    return address;
}

static const u8 *f3dex1_resolve(const N64ps3GfxF3dex1Decoder *decoder,
                                u32 address,
                                u32 required_size,
                                u32 *available_size)
{
    const void *resolved = NULL;
    u32 available = 0;

    if (decoder->task->address_resolver) {
        resolved = decoder->task->address_resolver(
            decoder->task->address_resolver_user_data,
            address,
            required_size,
            &available);
    } else if (decoder->task->dram_base && decoder->task->dram_size_bytes) {
        u32 offset = address & F3DEX1_RDRAM_ADDRESS_MASK;

        if (offset <= decoder->task->dram_size_bytes &&
            required_size <= decoder->task->dram_size_bytes - offset) {
            resolved = (const u8 *)decoder->task->dram_base + offset;
            available = decoder->task->dram_size_bytes - offset;
        }
    }

    if (!resolved || available < required_size) {
        return NULL;
    }
    if (available_size) {
        *available_size = available;
    }
    return (const u8 *)resolved;
}

static N64ps3GfxF3dex1Status f3dex1_submit_triangle(
    N64ps3GfxF3dex1Decoder *decoder,
    N64ps3GfxF3dex1Vertex triangle_vertices[3])
{
    if (decoder->triangle_callback &&
        decoder->triangle_callback(decoder->user_data,
                                   &triangle_vertices[0],
                                   &triangle_vertices[1],
                                   &triangle_vertices[2]) != 0) {
        return N64PS3_GFX_F3DEX1_CALLBACK_ABORTED;
    }
    decoder->stats->triangle_count++;
    if (triangle_vertices[0].texture_enabled) {
        decoder->stats->textured_triangle_count++;
    }
    return N64PS3_GFX_F3DEX1_OK;
}

static N64ps3GfxF3dex1Status f3dex1_emit_triangle(
    N64ps3GfxF3dex1Decoder *decoder,
    u32 a,
    u32 b,
    u32 c)
{
    int indices[3];
    N64ps3GfxF3dex1Vertex triangle_vertices[3];
    u32 i;

    indices[0] = (int)a;
    indices[1] = (int)b;
    indices[2] = (int)c;
    for (i = 0; i < 3U; i++) {
        if ((u32)indices[i] >= N64PS3_GFX_F3DEX1_VERTEX_CACHE_SIZE ||
            !decoder->vertex_valid[indices[i]] ||
            !decoder->vertices[indices[i]].clip_valid) {
            return N64PS3_GFX_F3DEX1_INVALID_TRIANGLE;
        }
        triangle_vertices[i] = decoder->vertices[indices[i]];
        triangle_vertices[i].geometry_mode = decoder->geometry_mode;
        triangle_vertices[i].blend_enabled =
            (u8)((decoder->other_mode_l & F3DEX1_RDP_FORCE_BLEND) != 0U);
        triangle_vertices[i].texture_rgba16 = NULL;
        triangle_vertices[i].texture_enabled = 0;
        if (decoder->texture_enabled && decoder->texture_loaded &&
            decoder->texture_data &&
            decoder->texture_tiles[decoder->texture_tile].width != 0U &&
            decoder->texture_tiles[decoder->texture_tile].height != 0U) {
            const N64ps3GfxF3dex1Tile *tile =
                &decoder->texture_tiles[decoder->texture_tile];

            triangle_vertices[i].texture_rgba16 = decoder->texture_data;
            triangle_vertices[i].texture_width = tile->width;
            triangle_vertices[i].texture_height = tile->height;
            triangle_vertices[i].texture_scale_s = decoder->texture_scale_s;
            triangle_vertices[i].texture_scale_t = decoder->texture_scale_t;
            triangle_vertices[i].texture_uls = tile->uls;
            triangle_vertices[i].texture_ult = tile->ult;
            triangle_vertices[i].texture_wrap_s = tile->wrap_s;
            triangle_vertices[i].texture_wrap_t = tile->wrap_t;
            triangle_vertices[i].texture_mask_s = tile->mask_s;
            triangle_vertices[i].texture_mask_t = tile->mask_t;
            triangle_vertices[i].texture_shift_s = tile->shift_s;
            triangle_vertices[i].texture_shift_t = tile->shift_t;
            triangle_vertices[i].texture_enabled = 1;
        }
    }

    return f3dex1_submit_triangle(decoder, triangle_vertices);
}

static void f3dex1_set_fill_vertex(N64ps3GfxF3dex1Vertex *vertex,
                                   float x,
                                   float y,
                                   u32 fill_color)
{
    u16 rgba5551 = (u16)fill_color;
    u32 red = (rgba5551 >> 11) & 0x1fU;
    u32 green = (rgba5551 >> 6) & 0x1fU;
    u32 blue = (rgba5551 >> 1) & 0x1fU;

    memset(vertex, 0, sizeof(*vertex));
    vertex->x = (s16)x;
    vertex->y = (s16)y;
    vertex->color[0] = (u8)((red << 3) | (red >> 2));
    vertex->color[1] = (u8)((green << 3) | (green >> 2));
    vertex->color[2] = (u8)((blue << 3) | (blue >> 2));
    vertex->color[3] = (rgba5551 & 1U) ? 0xffU : 0U;
    vertex->clip_x = 2.0f * (x / F3DEX1_FILLRECT_SCREEN_WIDTH) - 1.0f;
    vertex->clip_y = 1.0f - 2.0f * (y / F3DEX1_FILLRECT_SCREEN_HEIGHT);
    vertex->clip_z = 0.0f;
    vertex->clip_w = 1.0f;
    vertex->clip_valid = 1U;
}

static N64ps3GfxF3dex1Status f3dex1_emit_fill_rectangle(
    N64ps3GfxF3dex1Decoder *decoder,
    u32 w0,
    u32 w1)
{
    float left = (float)((w1 >> 14) & 0x3ffU);
    float top = (float)((w1 >> 2) & 0x3ffU);
    float right = (float)((w0 >> 14) & 0x3ffU) + 1.0f;
    float bottom = (float)((w0 >> 2) & 0x3ffU) + 1.0f;
    N64ps3GfxF3dex1Vertex corners[4];
    N64ps3GfxF3dex1Vertex triangle[3];
    u32 saved_geometry_mode = decoder->geometry_mode;
    u8 saved_texture_enabled = decoder->texture_enabled;
    N64ps3GfxF3dex1Status status;

    if (right <= left || bottom <= top || left >= F3DEX1_FILLRECT_SCREEN_WIDTH ||
        top >= F3DEX1_FILLRECT_SCREEN_HEIGHT) {
        decoder->stats->fill_rectangle_count++;
        return N64PS3_GFX_F3DEX1_OK;
    }
    if (right > F3DEX1_FILLRECT_SCREEN_WIDTH) {
        right = F3DEX1_FILLRECT_SCREEN_WIDTH;
    }
    if (bottom > F3DEX1_FILLRECT_SCREEN_HEIGHT) {
        bottom = F3DEX1_FILLRECT_SCREEN_HEIGHT;
    }

    f3dex1_set_fill_vertex(&corners[0], left, top, decoder->fill_color);
    f3dex1_set_fill_vertex(&corners[1], right, top, decoder->fill_color);
    f3dex1_set_fill_vertex(&corners[2], right, bottom, decoder->fill_color);
    f3dex1_set_fill_vertex(&corners[3], left, bottom, decoder->fill_color);
    decoder->geometry_mode = 0U;
    decoder->texture_enabled = 0U;

    triangle[0] = corners[0];
    triangle[1] = corners[1];
    triangle[2] = corners[2];
    status = f3dex1_submit_triangle(decoder, triangle);
    if (status == N64PS3_GFX_F3DEX1_OK) {
        triangle[0] = corners[0];
        triangle[1] = corners[2];
        triangle[2] = corners[3];
        status = f3dex1_submit_triangle(decoder, triangle);
    }
    decoder->geometry_mode = saved_geometry_mode;
    decoder->texture_enabled = saved_texture_enabled;
    decoder->stats->fill_rectangle_count++;
    return status;
}

static N64ps3GfxF3dex1Status f3dex1_decode_list(
    N64ps3GfxF3dex1Decoder *decoder,
    const u8 *list,
    u32 list_size,
    u32 depth)
{
    u32 offset = 0;

    if (depth >= N64PS3_GFX_F3DEX1_MAX_DEPTH) {
        return N64PS3_GFX_F3DEX1_DEPTH_LIMIT;
    }
    if (depth > decoder->stats->max_depth) {
        decoder->stats->max_depth = depth;
    }

    while (offset <= list_size && list_size - offset >= 8U) {
        u32 w0;
        u32 w1;
        u32 opcode;
        const u8 *command = list + offset;

        if (decoder->stats->command_count >= N64PS3_GFX_F3DEX1_MAX_COMMANDS) {
            return N64PS3_GFX_F3DEX1_COMMAND_LIMIT;
        }
        decoder->stats->command_count++;
        w0 = f3dex1_read_be32(command);
        w1 = f3dex1_read_be32(command + 4U);
        opcode = w0 >> 24;
        offset += 8U;

        if (opcode == F3DEX1_OP_ENDDL) {
            return N64PS3_GFX_F3DEX1_OK;
        }

        if (opcode == F3DEX1_OP_MTX) {
            N64ps3GfxF3dex1Status status =
                f3dex1_handle_matrix(decoder, w0, w1);

            if (status != N64PS3_GFX_F3DEX1_OK) {
                return status;
            }
            continue;
        }

        if (opcode == F3DEX1_OP_POPMTX) {
            N64ps3GfxF3dex1Status status = f3dex1_pop_matrix(decoder);

            if (status != N64PS3_GFX_F3DEX1_OK) {
                return status;
            }
            continue;
        }

        if (opcode == F3DEX1_OP_SETGEOMETRYMODE) {
            decoder->geometry_mode |= w1;
            decoder->stats->geometry_mode_command_count++;
            continue;
        }

        if (opcode == F3DEX1_OP_CLEARGEOMETRYMODE) {
            decoder->geometry_mode &= ~w1;
            decoder->stats->geometry_mode_command_count++;
            continue;
        }

        if (opcode == F3DEX1_OP_MOVEWORD) {
            u32 index = w0 & 0xffU;
            u32 offset = (w0 >> 8) & 0xffffU;

            if (index == F3DEX1_MOVEWORD_SEGMENT) {
                u32 segment = offset >> 2;

                if ((offset & 3U) == 0 && segment < 16U) {
                    decoder->segment_base[segment] = w1;
                    decoder->segment_valid[segment] = 1;
                } else {
                    decoder->stats->unsupported_command_count++;
                }
            } else if (index == F3DEX1_MOVEWORD_NUMLIGHT && offset == 0) {
                f3dex1_handle_num_lights(decoder, w1);
            } else {
                decoder->stats->unsupported_command_count++;
            }
            continue;
        }

        if (opcode == F3DEX1_OP_SETOTHERMODE_L) {
            u32 shift = (w0 >> 8) & 0xffU;
            u32 length = w0 & 0xffU;

            if (length != 0U && shift < 32U && length <= 32U - shift) {
                u32 mask = length == 32U
                    ? 0xffffffffU
                    : ((1U << length) - 1U) << shift;

                decoder->other_mode_l =
                    (decoder->other_mode_l & ~mask) | (w1 & mask);
            } else {
                decoder->stats->unsupported_command_count++;
                decoder->stats->unsupported_opcode_counts[opcode]++;
            }
            continue;
        }

        if (opcode == F3DEX1_OP_MOVEMEM) {
            N64ps3GfxF3dex1Status status =
                f3dex1_handle_light(decoder, w0, w1);

            if (status != N64PS3_GFX_F3DEX1_OK) {
                return status;
            }
            continue;
        }

        if (opcode == F3DEX1_OP_TEXTURE) {
            decoder->texture_tile = (u8)((w0 >> 8) & 7U);
            decoder->texture_enabled = (u8)(w0 & 0xffU);
            decoder->texture_scale_s = (u16)(w1 >> 16);
            decoder->texture_scale_t = (u16)w1;
            continue;
        }

        if (opcode == F3DEX1_OP_SETTIMG) {
            decoder->texture_image_format = (w0 >> 21) & 7U;
            decoder->texture_image_size = (w0 >> 19) & 3U;
            decoder->texture_image_width = (w0 & 0xfffU) + 1U;
            decoder->texture_image_address = w1;
            decoder->texture_loaded = 0;
            decoder->texture_data = NULL;
            continue;
        }

        if (opcode == F3DEX1_OP_SETTILE) {
            u32 tile_index = (w1 >> 24) & 7U;
            N64ps3GfxF3dex1Tile *tile = &decoder->texture_tiles[tile_index];

            tile->format = (u8)((w0 >> 21) & 7U);
            tile->size = (u8)((w0 >> 19) & 3U);
            tile->wrap_t = (u8)((w1 >> 18) & 3U);
            tile->mask_t = (u8)((w1 >> 14) & 15U);
            tile->shift_t = (u8)((w1 >> 10) & 15U);
            tile->wrap_s = (u8)((w1 >> 8) & 3U);
            tile->mask_s = (u8)((w1 >> 4) & 15U);
            tile->shift_s = (u8)(w1 & 15U);
            continue;
        }

        if (opcode == F3DEX1_OP_LOADBLOCK) {
            u32 tile_index = (w1 >> 24) & 7U;
            u32 texel_count = ((w1 >> 12) & 0xfffU) + 1U;
            const N64ps3GfxF3dex1Tile *tile =
                &decoder->texture_tiles[tile_index];
            u32 bits_per_texel = 4U << tile->size;
            u32 required_bytes = (texel_count * bits_per_texel + 7U) / 8U;
            u32 address;
            int address_valid;

            if (decoder->texture_image_format == 0U &&
                decoder->texture_image_size == 2U && tile->format == 0U &&
                tile->size == 2U) {
                address = f3dex1_segment_address(
                    decoder, decoder->texture_image_address, &address_valid);
                decoder->texture_data = address_valid
                    ? f3dex1_resolve(decoder, address, required_bytes, NULL)
                    : NULL;
                decoder->texture_loaded = decoder->texture_data != NULL;
                decoder->texture_load_bytes = required_bytes;
                if (decoder->texture_loaded) {
                    decoder->stats->texture_load_count++;
                }
            } else {
                decoder->texture_data = NULL;
                decoder->texture_loaded = 0;
            }
            continue;
        }

        if (opcode == F3DEX1_OP_SETTILESIZE) {
            u32 tile_index = (w1 >> 24) & 7U;
            u32 upper_s = (w0 >> 12) & 0xfffU;
            u32 upper_t = w0 & 0xfffU;
            u32 lower_s = (w1 >> 12) & 0xfffU;
            u32 lower_t = w1 & 0xfffU;
            N64ps3GfxF3dex1Tile *tile = &decoder->texture_tiles[tile_index];

            tile->uls = (u16)upper_s;
            tile->ult = (u16)upper_t;
            tile->width = lower_s >= upper_s
                ? (u16)((lower_s - upper_s) / 4U + 1U) : 0U;
            tile->height = lower_t >= upper_t
                ? (u16)((lower_t - upper_t) / 4U + 1U) : 0U;
            if (tile_index == decoder->texture_tile && decoder->texture_enabled &&
                decoder->texture_loaded &&
                (u32)tile->width * tile->height * 2U > decoder->texture_load_bytes) {
                decoder->texture_loaded = 0;
                decoder->texture_data = NULL;
            }
            continue;
        }

        if (opcode == F3DEX1_OP_LOADSYNC || opcode == F3DEX1_OP_PIPESYNC ||
            opcode == F3DEX1_OP_TILESYNC) {
            continue;
        }

        if (opcode == F3DEX1_OP_SETFILLCOLOR) {
            decoder->fill_color = w1;
            decoder->stats->fill_color_command_count++;
            continue;
        }

        if (opcode == F3DEX1_OP_FILLRECT) {
            N64ps3GfxF3dex1Status status =
                f3dex1_emit_fill_rectangle(decoder, w0, w1);

            if (status != N64PS3_GFX_F3DEX1_OK) {
                return status;
            }
            continue;
        }

        if (opcode == F3DEX1_OP_VTX) {
            u32 first = (w0 >> 17) & 0x7fU;
            u32 count = (w0 >> 10) & 0x3fU;
            u32 byte_length = (w0 & 0x3ffU) + 1U;
            u32 required_size;
            u32 address;
            u32 i;
            int address_valid;
            const u8 *vertex_data;

            if (count == 0 || first + count > N64PS3_GFX_F3DEX1_VERTEX_CACHE_SIZE ||
                byte_length != count * F3DEX1_VERTEX_BYTES) {
                return N64PS3_GFX_F3DEX1_INVALID_VERTEX_LOAD;
            }
            required_size = count * F3DEX1_VERTEX_BYTES;
            address = f3dex1_segment_address(decoder, w1, &address_valid);
            if (!address_valid) {
                return N64PS3_GFX_F3DEX1_ADDRESS_UNMAPPED;
            }
            vertex_data = f3dex1_resolve(decoder, address, required_size, NULL);
            if (!vertex_data) {
                return N64PS3_GFX_F3DEX1_ADDRESS_UNMAPPED;
            }

            for (i = 0; i < count; i++) {
                const u8 *raw = vertex_data + i * F3DEX1_VERTEX_BYTES;
                N64ps3GfxF3dex1Vertex *vertex = &decoder->vertices[first + i];

                vertex->x = (s16)f3dex1_read_be16(raw);
                vertex->y = (s16)f3dex1_read_be16(raw + 2U);
                vertex->z = (s16)f3dex1_read_be16(raw + 4U);
                vertex->flag = f3dex1_read_be16(raw + 6U);
                vertex->s = (s16)f3dex1_read_be16(raw + 8U);
                vertex->t = (s16)f3dex1_read_be16(raw + 10U);
                vertex->color[0] = raw[12];
                vertex->color[1] = raw[13];
                vertex->color[2] = raw[14];
                vertex->color[3] = raw[15];
                vertex->normal[0] = (s8)raw[12];
                vertex->normal[1] = (s8)raw[13];
                vertex->normal[2] = (s8)raw[14];
                f3dex1_shade_vertex(decoder, vertex);
                f3dex1_transform_vertex(decoder, vertex);
                decoder->vertex_valid[first + i] = 1;
            }
            decoder->stats->vertex_load_count++;
            continue;
        }

        if (opcode == F3DEX1_OP_TRI1) {
            u32 a = ((w1 >> 16) & 0xffU) / 2U;
            u32 b = ((w1 >> 8) & 0xffU) / 2U;
            u32 c = (w1 & 0xffU) / 2U;

            if (((w1 >> 16) & 1U) || ((w1 >> 8) & 1U) || (w1 & 1U)) {
                return N64PS3_GFX_F3DEX1_INVALID_TRIANGLE;
            }
            {
                N64ps3GfxF3dex1Status status = f3dex1_emit_triangle(decoder, a, b, c);

                if (status != N64PS3_GFX_F3DEX1_OK) {
                    return status;
                }
            }
            continue;
        }

        if (opcode == F3DEX1_OP_TRI2) {
            u32 a0 = ((w0 >> 16) & 0xffU) / 2U;
            u32 b0 = ((w0 >> 8) & 0xffU) / 2U;
            u32 c0 = (w0 & 0xffU) / 2U;
            u32 a1 = ((w1 >> 16) & 0xffU) / 2U;
            u32 b1 = ((w1 >> 8) & 0xffU) / 2U;
            u32 c1 = (w1 & 0xffU) / 2U;
            N64ps3GfxF3dex1Status status;

            if (((w0 >> 16) & 1U) || ((w0 >> 8) & 1U) || (w0 & 1U) ||
                ((w1 >> 16) & 1U) || ((w1 >> 8) & 1U) || (w1 & 1U)) {
                return N64PS3_GFX_F3DEX1_INVALID_TRIANGLE;
            }
            status = f3dex1_emit_triangle(decoder, a0, b0, c0);
            if (status != N64PS3_GFX_F3DEX1_OK) {
                return status;
            }
            status = f3dex1_emit_triangle(decoder, a1, b1, c1);
            if (status != N64PS3_GFX_F3DEX1_OK) {
                return status;
            }
            continue;
        }

        if (opcode == F3DEX1_OP_DL) {
            u32 address;
            u32 nested_size = 0;
            int address_valid;
            const u8 *nested_list;
            N64ps3GfxF3dex1Status status;
            int no_push = (((w0 >> 16) & 0xffU) == F3DEX1_DL_NOPUSH);

            address = f3dex1_segment_address(decoder, w1, &address_valid);
            if (!address_valid) {
                return N64PS3_GFX_F3DEX1_ADDRESS_UNMAPPED;
            }
            nested_list = f3dex1_resolve(decoder, address, 8U, &nested_size);
            if (!nested_list) {
                return N64PS3_GFX_F3DEX1_ADDRESS_UNMAPPED;
            }
            decoder->stats->display_list_call_count++;
            status = f3dex1_decode_list(decoder, nested_list, nested_size, depth + 1U);
            if (status != N64PS3_GFX_F3DEX1_OK) {
                return status;
            }
            if (no_push) {
                return N64PS3_GFX_F3DEX1_OK;
            }
            continue;
        }

        decoder->stats->unsupported_command_count++;
        decoder->stats->unsupported_opcode_counts[opcode]++;
    }

    return N64PS3_GFX_F3DEX1_TRUNCATED_LIST;
}

N64ps3GfxF3dex1Status n64ps3_gfx_decode_f3dex1(
    const N64ps3GfxTask *task,
    N64ps3GfxF3dex1TriangleCallback triangle_callback,
    void *user_data,
    N64ps3GfxF3dex1Stats *stats)
{
    N64ps3GfxF3dex1Decoder decoder;
    N64ps3GfxF3dex1Stats local_stats;
    N64ps3GfxF3dex1Status status;

    if (stats) {
        memset(stats, 0, sizeof(*stats));
    }
    if (!task || !task->display_list || task->display_list_size_bytes < 8U) {
        return N64PS3_GFX_F3DEX1_INVALID_ARGUMENT;
    }
    if (task->kind != N64PS3_GFX_TASK_DISPLAY_LIST) {
        return N64PS3_GFX_F3DEX1_UNSUPPORTED_TASK;
    }

    memset(&decoder, 0, sizeof(decoder));
    memset(&local_stats, 0, sizeof(local_stats));
    decoder.task = task;
    decoder.triangle_callback = triangle_callback;
    decoder.user_data = user_data;
    decoder.stats = &local_stats;
    f3dex1_matrix_identity(decoder.modelview);
    f3dex1_matrix_identity(decoder.projection);

    status = f3dex1_decode_list(&decoder,
                                (const u8 *)task->display_list,
                                task->display_list_size_bytes,
                                0);
    if (stats) {
        *stats = local_stats;
    }
    return status;
}
