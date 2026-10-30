#include "n64ps3/n64ps3.h"
#include "n64ps3/gfx_f3dex1.h"

#include <stdio.h>
#include <string.h>

static const u8 s_root_display_list[] = {
    0xbc, 0x00, 0x18, 0x06, 0x80, 0x00, 0x01, 0x00,
    0x01, 0x06, 0x00, 0x40, 0x06, 0x00, 0x00, 0x00,
    0x01, 0x02, 0x00, 0x40, 0x06, 0x00, 0x00, 0x40,
    0x06, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0xc0,
    0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const u8 s_nested_display_list[] = {
    0x04, 0x00, 0x0c, 0x2f, 0x06, 0x00, 0x00, 0x80,
    0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x04,
    0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const u8 s_tri2_display_list[] = {
    0x01, 0x02, 0x00, 0x40, 0x80, 0x00, 0x01, 0x40,
    0x01, 0x03, 0x00, 0x40, 0x80, 0x00, 0x01, 0x00,
    0xbd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x0c, 0x2f, 0x80, 0x00, 0x01, 0x80,
    0xb1, 0x00, 0x02, 0x04, 0x00, 0x00, 0x02, 0x04,
    0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const u8 s_matrix_mul_display_list[] = {
    0x01, 0x02, 0x00, 0x40, 0x80, 0x00, 0x01, 0x40,
    0x01, 0x00, 0x00, 0x40, 0x80, 0x00, 0x02, 0x00,
    0x04, 0x00, 0x0c, 0x2f, 0x80, 0x00, 0x01, 0x80,
    0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x04,
    0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const u8 s_lighting_display_list[] = {
    0xbc, 0x00, 0x04, 0x06, 0x00, 0x00, 0x02, 0x40,
    0xbc, 0x00, 0x00, 0x02, 0x80, 0x00, 0x00, 0x40,
    0x03, 0x86, 0x00, 0x10, 0x01, 0x00, 0x00, 0x00,
    0x03, 0x88, 0x00, 0x10, 0x01, 0x00, 0x00, 0x10,
    0xb7, 0x00, 0x00, 0x00, 0x00, 0x02, 0x20, 0x01,
    0x04, 0x00, 0x0c, 0x2f, 0x00, 0x00, 0x02, 0x80,
    0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x04,
    0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const u8 s_vertices[] = {
    0xa2, 0x40, 0xb5, 0xc8, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
    0x5d, 0xc0, 0xb5, 0xc8, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x5d, 0xc0, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff
};

typedef struct TriangleCapture {
    u32 count;
    s16 first_vertex_x;
    s16 first_vertex_y;
    s16 second_vertex_x;
    s16 second_vertex_y;
    s16 third_vertex_x;
    s16 third_vertex_y;
    float first_clip_x;
    float first_clip_y;
    float second_clip_x;
    float third_clip_y;
    u8 first_color[4];
    s8 first_normal[3];
    u32 first_geometry_mode;
} TriangleCapture;

static u8 s_test_dram[0x300];

static void write_be16(u8 *destination, u16 value)
{
    destination[0] = (u8)(value >> 8);
    destination[1] = (u8)value;
}

static void write_matrix_fixed(u8 *matrix,
                               u32 row,
                               u32 column,
                               s16 integer,
                               u16 fraction)
{
    u32 index = row * 4U + column;

    write_be16(matrix + index * 2U, (u16)integer);
    write_be16(matrix + 32U + index * 2U, fraction);
}

static void make_identity_matrix(u8 *matrix)
{
    u32 axis;

    memset(matrix, 0, 64U);
    for (axis = 0; axis < 4U; axis++) {
        write_matrix_fixed(matrix, axis, axis, 1, 0);
    }
}

static int near_float(float actual, float expected)
{
    float difference = actual - expected;

    if (difference < 0.0f) {
        difference = -difference;
    }
    return difference < 0.0001f;
}

static int count_triangle(void *user_data,
                          const N64ps3GfxF3dex1Vertex *a,
                          const N64ps3GfxF3dex1Vertex *b,
                          const N64ps3GfxF3dex1Vertex *c)
{
    TriangleCapture *capture = (TriangleCapture *)user_data;

    if (capture->count == 0) {
        capture->first_vertex_x = a->x;
        capture->first_vertex_y = a->y;
        capture->second_vertex_x = b->x;
        capture->second_vertex_y = b->y;
        capture->third_vertex_x = c->x;
        capture->third_vertex_y = c->y;
        capture->first_clip_x = a->clip_x;
        capture->first_clip_y = a->clip_y;
        capture->second_clip_x = b->clip_x;
        capture->third_clip_y = c->clip_y;
        memcpy(capture->first_color, a->color, sizeof(capture->first_color));
        memcpy(capture->first_normal, a->normal, sizeof(capture->first_normal));
        capture->first_geometry_mode = a->geometry_mode;
    }
    capture->count++;
    return 0;
}

int main(void)
{
    N64ps3GfxTask task;
    N64ps3GfxTask tri2_task;
    N64ps3GfxTask matrix_mul_task;
    N64ps3GfxTask lighting_task;
    N64ps3Config config = n64ps3_config_default();
    N64ps3GfxF3dex1Stats stats;
    N64ps3GfxF3dex1Status status;
    TriangleCapture capture;
    config.app_name = "gfx_task";

    if (n64ps3_runtime_init(&config) != 0) {
        return 1;
    }

    memset(s_test_dram, 0, sizeof(s_test_dram));
    write_matrix_fixed(s_test_dram + 0x100, 0, 0, 0, 2);
    write_matrix_fixed(s_test_dram + 0x100, 1, 1, 0, 2);
    write_matrix_fixed(s_test_dram + 0x100, 2, 2, 0, 2);
    write_matrix_fixed(s_test_dram + 0x100, 3, 3, 1, 0);
    make_identity_matrix(s_test_dram + 0x140);
    write_matrix_fixed(s_test_dram + 0x140, 3, 0, 1024, 0);
    make_identity_matrix(s_test_dram + 0x180);
    memcpy(s_test_dram + 0x180, s_vertices, sizeof(s_vertices));
    memcpy(s_test_dram + 0x1c0, s_nested_display_list, sizeof(s_nested_display_list));
    make_identity_matrix(s_test_dram + 0x200);
    write_matrix_fixed(s_test_dram + 0x200, 0, 0, 2, 0);
    memset(&task, 0, sizeof(task));

    task.kind = N64PS3_GFX_TASK_DISPLAY_LIST;
    task.dram_base = s_test_dram;
    task.dram_size_bytes = sizeof(s_test_dram);
    task.display_list = s_root_display_list;
    task.display_list_size_bytes = sizeof(s_root_display_list);
    task.debug_name = "synthetic F3DEX1 triangle";

    memset(&capture, 0, sizeof(capture));
    status = n64ps3_gfx_decode_f3dex1(&task, count_triangle,
                                      &capture, &stats);
    if (status != N64PS3_GFX_F3DEX1_OK || stats.command_count != 8 ||
        stats.vertex_load_count != 1 || stats.triangle_count != 1 ||
        stats.display_list_call_count != 1 || stats.unsupported_command_count != 0 ||
        stats.matrix_command_count != 2 || stats.transformed_vertex_count != 3 ||
        stats.max_depth != 1 || capture.count != 1 ||
        capture.first_vertex_x != -24000 || capture.first_vertex_y != -19000 ||
        capture.second_vertex_x != 24000 || capture.second_vertex_y != -19000 ||
        capture.third_vertex_x != 0 || capture.third_vertex_y != 24000 ||
        capture.first_color[0] != 255 || capture.first_color[1] != 255 ||
        capture.first_color[2] != 255 || capture.first_color[3] != 255 ||
        !near_float(capture.first_clip_x, -22976.0f / 32768.0f) ||
        !near_float(capture.first_clip_y, -19000.0f / 32768.0f) ||
        !near_float(capture.second_clip_x, 25024.0f / 32768.0f) ||
        !near_float(capture.third_clip_y, 24000.0f / 32768.0f)) {
        fprintf(stderr,
                "F3DEX1 sample decode failed: status=%d commands=%u loads=%u triangles=%u calls=%u skipped=%u matrices=%u transformed=%u depth=%u callbacks=%u vertices=(%d,%d),(%d,%d),(%d,%d) clip=(%.5f,%.5f),(%.5f,%.5f)\n",
                (int)status, stats.command_count, stats.vertex_load_count,
                stats.triangle_count, stats.display_list_call_count,
                stats.unsupported_command_count, stats.matrix_command_count,
                stats.transformed_vertex_count, stats.max_depth, capture.count,
                capture.first_vertex_x, capture.first_vertex_y,
                capture.second_vertex_x, capture.second_vertex_y,
                capture.third_vertex_x, capture.third_vertex_y,
                capture.first_clip_x, capture.first_clip_y,
                capture.second_clip_x, capture.third_clip_y);
        n64ps3_runtime_shutdown();
        return 1;
    }

    memset(&tri2_task, 0, sizeof(tri2_task));
    tri2_task.kind = N64PS3_GFX_TASK_DISPLAY_LIST;
    tri2_task.dram_base = s_test_dram;
    tri2_task.dram_size_bytes = sizeof(s_test_dram);
    tri2_task.display_list = s_tri2_display_list;
    tri2_task.display_list_size_bytes = sizeof(s_tri2_display_list);
    memset(&capture, 0, sizeof(capture));
    status = n64ps3_gfx_decode_f3dex1(&tri2_task, count_triangle,
                                      &capture, &stats);
    if (status != N64PS3_GFX_F3DEX1_OK || stats.command_count != 6 ||
        stats.vertex_load_count != 1 || stats.triangle_count != 2 ||
        stats.display_list_call_count != 0 || stats.matrix_command_count != 2 ||
        stats.matrix_push_count != 1 || stats.matrix_pop_count != 1 ||
        capture.count != 2 || capture.first_vertex_x != -24000 ||
        capture.third_vertex_y != 24000 ||
        !near_float(capture.first_clip_x, -22976.0f / 320.0f) ||
        !near_float(capture.first_clip_y, 19000.0f / 240.0f)) {
        fprintf(stderr, "F3DEX1 TRI2 sample decode failed: status=%d triangles=%u\n",
                (int)status, stats.triangle_count);
        n64ps3_runtime_shutdown();
        return 1;
    }

    memset(&matrix_mul_task, 0, sizeof(matrix_mul_task));
    matrix_mul_task.kind = N64PS3_GFX_TASK_DISPLAY_LIST;
    matrix_mul_task.dram_base = s_test_dram;
    matrix_mul_task.dram_size_bytes = sizeof(s_test_dram);
    matrix_mul_task.display_list = s_matrix_mul_display_list;
    matrix_mul_task.display_list_size_bytes = sizeof(s_matrix_mul_display_list);
    memset(&capture, 0, sizeof(capture));
    status = n64ps3_gfx_decode_f3dex1(&matrix_mul_task, count_triangle,
                                      &capture, &stats);
    if (status != N64PS3_GFX_F3DEX1_OK || stats.command_count != 5 ||
        stats.matrix_command_count != 2 || stats.vertex_load_count != 1 ||
        stats.transformed_vertex_count != 3 || stats.triangle_count != 1 ||
        capture.count != 1 || capture.first_vertex_x != -24000 ||
        !near_float(capture.first_clip_x, -46976.0f / 320.0f)) {
        fprintf(stderr,
                "F3DEX1 matrix multiply failed: status=%d matrices=%u triangles=%u clip_x=%.5f\n",
                (int)status, stats.matrix_command_count,
                stats.triangle_count, capture.first_clip_x);
        n64ps3_runtime_shutdown();
        return 1;
    }

    memcpy(s_test_dram + 0x280, s_vertices, sizeof(s_vertices));
    s_test_dram[0x240] = 40;
    s_test_dram[0x241] = 50;
    s_test_dram[0x242] = 60;
    s_test_dram[0x248] = 0;
    s_test_dram[0x249] = 0;
    s_test_dram[0x24a] = 127;
    s_test_dram[0x250] = 10;
    s_test_dram[0x251] = 20;
    s_test_dram[0x252] = 30;
    s_test_dram[0x28c] = 0;
    s_test_dram[0x28d] = 0;
    s_test_dram[0x28e] = 127;
    s_test_dram[0x29c] = 0;
    s_test_dram[0x29d] = 0;
    s_test_dram[0x29e] = 127;
    s_test_dram[0x2ac] = 0;
    s_test_dram[0x2ad] = 0;
    s_test_dram[0x2ae] = 127;
    memset(&lighting_task, 0, sizeof(lighting_task));
    lighting_task.kind = N64PS3_GFX_TASK_DISPLAY_LIST;
    lighting_task.dram_base = s_test_dram;
    lighting_task.dram_size_bytes = sizeof(s_test_dram);
    lighting_task.display_list = s_lighting_display_list;
    lighting_task.display_list_size_bytes = sizeof(s_lighting_display_list);
    memset(&capture, 0, sizeof(capture));
    status = n64ps3_gfx_decode_f3dex1(&lighting_task, count_triangle,
                                      &capture, &stats);
    if (status != N64PS3_GFX_F3DEX1_OK || stats.command_count != 8 ||
        stats.vertex_load_count != 1 || stats.triangle_count != 1 ||
        stats.unsupported_command_count != 0 ||
        stats.geometry_mode_command_count != 1 || stats.light_command_count != 3 ||
        capture.first_geometry_mode !=
            (N64PS3_GFX_F3DEX1_GEOMETRY_LIGHTING |
             N64PS3_GFX_F3DEX1_GEOMETRY_CULL_BACK |
             N64PS3_GFX_F3DEX1_GEOMETRY_ZBUFFER) ||
        capture.count != 1 || capture.first_color[0] != 50 ||
        capture.first_color[1] != 70 || capture.first_color[2] != 90 ||
        capture.first_color[3] != 255 || capture.first_normal[0] != 0 ||
        capture.first_normal[1] != 0 || capture.first_normal[2] != 127) {
        fprintf(stderr,
                "F3DEX1 lighting/state failed: status=%d commands=%u lights=%u geometry=%u triangles=%u mode=0x%08x color=%u,%u,%u,%u normal=%d,%d,%d\n",
                (int)status, stats.command_count, stats.light_command_count,
                stats.geometry_mode_command_count, stats.triangle_count,
                capture.first_geometry_mode,
                capture.first_color[0], capture.first_color[1],
                capture.first_color[2], capture.first_color[3],
                capture.first_normal[0], capture.first_normal[1],
                capture.first_normal[2]);
        n64ps3_runtime_shutdown();
        return 1;
    }

    n64ps3_gfx_begin_frame();
    n64ps3_gfx_submit_task(&task);
    n64ps3_gfx_end_frame();

    n64ps3_runtime_shutdown();
    return 0;
}
