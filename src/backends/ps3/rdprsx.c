#include "n64ps3/gfx.h"
#include "n64ps3/gfx_f3dex1.h"
#include "n64ps3/log.h"

#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rsx/rsx.h>
#include <sysutil/video.h>

#include "rdprsx_triangle_fpo.h"
#include "rdprsx_triangle_vpo.h"

#define RDPRSX_FRAMEBUFFER_COUNT 2
#define RDPRSX_MAX_TRIANGLES 16384U
#define RDPRSX_MAX_TEXTURES 256U
#define RDPRSX_MAX_TEXTURE_DIMENSION 256U
#define RDPRSX_COMMAND_BUFFER_SIZE 0x80000
#define RDPRSX_HOST_IO_SIZE (32U * 1024U * 1024U)
#define RDPRSX_HOST_ALIGNMENT (1024U * 1024U)

#ifndef N64PS3_RSX_DISABLE_DEPTH
#define N64PS3_RSX_DISABLE_DEPTH 0
#endif

#ifndef N64PS3_RSX_TEXTURE_DIAGNOSTICS
#define N64PS3_RSX_TEXTURE_DIAGNOSTICS 0
#endif

typedef struct RdprsxFramebuffer {
    void *address;
    u32 offset;
    u8 id;
} RdprsxFramebuffer;

typedef struct RdprsxVertex {
    float x;
    float y;
    float z;
    float w;
    float color[4];
    float texcoord[2];
} RdprsxVertex;

typedef struct RdprsxTexture {
    const u8 *source;
    u16 width;
    u16 height;
    u8 wrap_s;
    u8 wrap_t;
    u8 source_bytes_per_texel;
    void *data;
    gcmTexture descriptor;
} RdprsxTexture;

static gcmContextData *s_context;
static void *s_host_io;
static RdprsxFramebuffer s_framebuffers[RDPRSX_FRAMEBUFFER_COUNT];
static void *s_depth_buffer;
static u32 s_depth_offset;
static u32 s_width;
static u32 s_height;
static u32 s_pitch;
static u32 s_current_framebuffer;
static rsxVertexProgram *s_triangle_vertex_program;
static rsxFragmentProgram *s_triangle_fragment_program;
static rsxProgramAttrib *s_triangle_position;
static rsxProgramAttrib *s_triangle_color;
static rsxProgramAttrib *s_triangle_texcoord;
static void *s_triangle_vertex_ucode;
static void *s_triangle_fragment_buffer;
static u32 s_triangle_fragment_offset;
static void *s_triangle_vertices;
static void *s_triangle_indices;
static u32 *s_triangle_modes;
static u8 *s_triangle_textures;
static u8 *s_triangle_blends;
static u8 *s_triangle_filters;
static RdprsxTexture s_textures[RDPRSX_MAX_TEXTURES];
static u32 s_texture_count;
static u32 s_triangle_vertex_offset;
static u32 s_triangle_index_offset;
static u32 s_triangle_count;
static int s_flip_pending;
#if N64PS3_RSX_TEXTURE_DIAGNOSTICS
static u32 s_flip_wait_iterations;
#endif
static int s_ready;

#if N64PS3_RSX_TEXTURE_DIAGNOSTICS
static u32 rdprsx_hash_bytes(u32 hash, const u8 *bytes, u32 size)
{
    u32 index;

    for (index = 0; index < size; index++) {
        hash = (hash ^ bytes[index]) * 16777619U;
    }
    return hash;
}

static void rdprsx_log_texture_inputs(void)
{
    static u32 frame;
    const RdprsxVertex *vertices = (const RdprsxVertex *)s_triangle_vertices;
    u32 position_hash = 2166136261U;
    u32 shade_hash = 2166136261U;
    u32 texcoord_hash = 2166136261U;
    u32 binding_hash = 2166136261U;
    u32 source_hash = 2166136261U;
    u32 upload_hash = 2166136261U;
    u32 textured_triangles = 0;
    u32 triangle;
    u32 texture;

    for (triangle = 0; triangle < s_triangle_count; triangle++) {
        u32 vertex;

        if (s_triangle_textures[triangle] == 0U) {
            continue;
        }
        textured_triangles++;
        binding_hash = (binding_hash ^ s_triangle_textures[triangle]) *
                       16777619U;
        binding_hash = rdprsx_hash_bytes(
            binding_hash, (const u8 *)&s_triangle_modes[triangle],
            sizeof(s_triangle_modes[triangle]));
        for (vertex = 0; vertex < 3U; vertex++) {
            const RdprsxVertex *input = &vertices[triangle * 3U + vertex];

            position_hash = rdprsx_hash_bytes(
                position_hash, (const u8 *)&input->x, sizeof(input->x) * 4U);
            shade_hash = rdprsx_hash_bytes(
                shade_hash, (const u8 *)input->color, sizeof(input->color));
            texcoord_hash = rdprsx_hash_bytes(
                texcoord_hash, (const u8 *)input->texcoord,
                sizeof(input->texcoord));
        }
    }

    for (texture = 1U; texture < s_texture_count; texture++) {
        const RdprsxTexture *entry = &s_textures[texture];
        u32 source_bytes = (u32)entry->width * entry->height *
                           entry->source_bytes_per_texel;
        u32 upload_bytes = entry->descriptor.pitch * entry->descriptor.height;

        source_hash = rdprsx_hash_bytes(source_hash, entry->source,
                                        source_bytes);
        upload_hash = rdprsx_hash_bytes(upload_hash,
                                        (const u8 *)entry->data,
                                        upload_bytes);
    }

    if (frame < 5U || (frame % 120U) == 0U) {
        n64ps3_log(N64PS3_LOG_INFO,
                   "rdprsx texture input frame=%u triangles=%u textured=%u pos=%08x shade=%08x uv=%08x bind=%08x source=%08x upload=%08x textures=%u flip_waits=%u",
                   frame, s_triangle_count, textured_triangles,
                   position_hash, shade_hash, texcoord_hash, binding_hash,
                   source_hash, upload_hash, s_texture_count - 1U,
                   s_flip_wait_iterations);
    }
    frame++;
}
#endif

static void rdprsx_set_render_target(u32 index)
{
    gcmSurface surface;

    memset(&surface, 0, sizeof(surface));
    surface.colorFormat = GCM_SURFACE_X8R8G8B8;
    surface.colorTarget = GCM_SURFACE_TARGET_0;
    surface.colorLocation[0] = GCM_LOCATION_RSX;
    surface.colorOffset[0] = s_framebuffers[index].offset;
    surface.colorPitch[0] = s_pitch;
    surface.colorLocation[1] = GCM_LOCATION_RSX;
    surface.colorLocation[2] = GCM_LOCATION_RSX;
    surface.colorLocation[3] = GCM_LOCATION_RSX;
    surface.colorPitch[1] = 64;
    surface.colorPitch[2] = 64;
    surface.colorPitch[3] = 64;
    /* The title Arwing is viewed at long range with many close panel edges.
     * Keep enough depth precision to avoid collapsing adjacent N64 surfaces. */
    surface.depthFormat = GCM_SURFACE_ZETA_Z24S8;
    surface.depthLocation = GCM_LOCATION_RSX;
    surface.depthOffset = s_depth_offset;
    surface.depthPitch = s_pitch;
    surface.type = GCM_TEXTURE_LINEAR;
    surface.antiAlias = GCM_SURFACE_CENTER_1;
    surface.width = (u16)s_width;
    surface.height = (u16)s_height;
    surface.x = 0;
    surface.y = 0;

    rsxSetSurface(s_context, &surface);
}

static void rdprsx_release(void)
{
    u32 i;

    if (s_context) {
        rsxFinish(s_context, 1);
        s_context = NULL;
    }
    for (i = 0; i < RDPRSX_FRAMEBUFFER_COUNT; i++) {
        if (s_framebuffers[i].address) {
            rsxFree(s_framebuffers[i].address);
            s_framebuffers[i].address = NULL;
        }
    }
    if (s_depth_buffer) {
        rsxFree(s_depth_buffer);
        s_depth_buffer = NULL;
    }
    if (s_triangle_fragment_buffer) {
        rsxFree(s_triangle_fragment_buffer);
        s_triangle_fragment_buffer = NULL;
    }
    if (s_triangle_vertices) {
        rsxFree(s_triangle_vertices);
        s_triangle_vertices = NULL;
    }
    if (s_triangle_indices) {
        rsxFree(s_triangle_indices);
        s_triangle_indices = NULL;
    }
    for (i = 0; i < s_texture_count; i++) {
        if (s_textures[i].data) {
            rsxFree(s_textures[i].data);
            s_textures[i].data = NULL;
        }
    }
    memset(s_textures, 0, sizeof(s_textures));
    s_texture_count = 0;
    free(s_triangle_modes);
    s_triangle_modes = NULL;
    free(s_triangle_textures);
    s_triangle_textures = NULL;
    free(s_triangle_blends);
    s_triangle_blends = NULL;
    free(s_triangle_filters);
    s_triangle_filters = NULL;
    free(s_host_io);
    s_host_io = NULL;
    s_triangle_vertex_program = NULL;
    s_triangle_fragment_program = NULL;
    s_triangle_position = NULL;
    s_triangle_color = NULL;
    s_triangle_texcoord = NULL;
    s_triangle_vertex_ucode = NULL;
    s_triangle_count = 0;
    s_flip_pending = 0;
    s_ready = 0;
}

static u8 rdprsx_map_wrap_mode(u8 n64_mode)
{
    switch (n64_mode & 3U) {
    case 1U:
        return GCM_TEXTURE_MIRRORED_REPEAT;
    case 2U:
        return GCM_TEXTURE_CLAMP_TO_EDGE;
    case 3U:
        return GCM_TEXTURE_MIRROR_ONCE_CLAMP_TO_EDGE;
    default:
        return GCM_TEXTURE_REPEAT;
    }
}

static int rdprsx_init_texture(RdprsxTexture *texture,
                              const u8 *source,
                              u8 source_bytes_per_texel,
                              u16 width,
                              u16 height,
                              u8 wrap_s,
                              u8 wrap_t)
{
    u32 pitch;
    u32 row;

    memset(texture, 0, sizeof(*texture));
    if (!source || (source_bytes_per_texel != 2U &&
                    source_bytes_per_texel != 4U) ||
        width == 0U || height == 0U ||
        width > RDPRSX_MAX_TEXTURE_DIMENSION ||
        height > RDPRSX_MAX_TEXTURE_DIMENSION) {
        return -1;
    }

    pitch = ((u32)width * 4U + 63U) & ~63U;
    texture->data = rsxMemalign(64, pitch * height);
    if (!texture->data) {
        return -1;
    }
    memset(texture->data, 0, pitch * height);
    for (row = 0; row < height; row++) {
        u32 column;
        u8 *destination = (u8 *)texture->data + row * pitch;

        for (column = 0; column < width; column++) {
            const u8 *pixel = source + (row * width + column) *
                                       source_bytes_per_texel;
            u32 red;
            u32 green;
            u32 blue;
            u32 alpha;
            if (source_bytes_per_texel == 2U) {
                u16 rgba = (u16)(((u16)pixel[0] << 8) | pixel[1]);
                u32 red5 = (rgba >> 11) & 31U;
                u32 green5 = (rgba >> 6) & 31U;
                u32 blue5 = (rgba >> 1) & 31U;

                red = (red5 << 3) | (red5 >> 2);
                green = (green5 << 3) | (green5 >> 2);
                blue = (blue5 << 3) | (blue5 >> 2);
                alpha = (rgba & 1U) ? 255U : 0U;
            } else {
                red = pixel[0];
                green = pixel[1];
                blue = pixel[2];
                alpha = pixel[3];
            }
            u32 argb = (alpha << 24) |
                       (red << 16) | (green << 8) | blue;

            ((u32 *)destination)[column] = argb;
        }
    }

    texture->source = source;
    texture->width = width;
    texture->height = height;
    texture->wrap_s = wrap_s;
    texture->wrap_t = wrap_t;
    texture->source_bytes_per_texel = source_bytes_per_texel;
    texture->descriptor.format = GCM_TEXTURE_FORMAT_LIN |
                                 GCM_TEXTURE_FORMAT_A8R8G8B8;
    /* This field is the number of available mip levels in RSX texture
     * registers. A non-mipmapped texture still has its base level (1). */
    texture->descriptor.mipmap = 1;
    texture->descriptor.dimension = GCM_TEXTURE_DIMS_2D;
    texture->descriptor.cubemap = GCM_FALSE;
    texture->descriptor.remap = GCM_TEXTURE_REMAP_MODE(
        GCM_TEXTURE_REMAP_ORDER_XYXY,
        GCM_TEXTURE_REMAP_COLOR_A,
        GCM_TEXTURE_REMAP_COLOR_R,
        GCM_TEXTURE_REMAP_COLOR_G,
        GCM_TEXTURE_REMAP_COLOR_B,
        GCM_TEXTURE_REMAP_TYPE_REMAP,
        GCM_TEXTURE_REMAP_TYPE_REMAP,
        GCM_TEXTURE_REMAP_TYPE_REMAP,
        GCM_TEXTURE_REMAP_TYPE_REMAP);
    texture->descriptor.width = width;
    texture->descriptor.height = height;
    texture->descriptor.depth = 1;
    texture->descriptor.location = GCM_LOCATION_RSX;
    texture->descriptor.pitch = pitch;
    if (rsxAddressToOffset(texture->data, &texture->descriptor.offset) != 0) {
        rsxFree(texture->data);
        memset(texture, 0, sizeof(*texture));
        return -1;
    }
    return 0;
}

static u8 rdprsx_get_texture(const N64ps3GfxF3dex1Vertex *vertex)
{
    u32 i;

    if (!vertex->texture_enabled || !vertex->texture_data ||
        (vertex->texture_bytes_per_texel != 2U &&
         vertex->texture_bytes_per_texel != 4U)) {
        return 0;
    }
    for (i = 1U; i < s_texture_count; i++) {
        const RdprsxTexture *texture = &s_textures[i];

        if (texture->source == vertex->texture_data &&
            texture->width == vertex->texture_width &&
            texture->height == vertex->texture_height &&
            texture->source_bytes_per_texel ==
                vertex->texture_bytes_per_texel &&
            texture->wrap_s == vertex->texture_wrap_s &&
            texture->wrap_t == vertex->texture_wrap_t) {
            return (u8)i;
        }
    }
    if (s_texture_count >= RDPRSX_MAX_TEXTURES) {
        n64ps3_log(N64PS3_LOG_ERROR,
                   "rdprsx: texture table is full (%u entries)",
                   RDPRSX_MAX_TEXTURES);
        return 0;
    }
    if (rdprsx_init_texture(&s_textures[s_texture_count],
                            vertex->texture_data,
                            vertex->texture_bytes_per_texel,
                            vertex->texture_width,
                            vertex->texture_height,
                            vertex->texture_wrap_s,
                            vertex->texture_wrap_t) != 0) {
        n64ps3_log(N64PS3_LOG_ERROR,
                   "rdprsx: failed to create texture %ux%u (%u-byte texels)",
                   vertex->texture_width, vertex->texture_height,
                   vertex->texture_bytes_per_texel);
        return 0;
    }
    return (u8)s_texture_count++;
}

static int rdprsx_init_triangle(void)
{
    static const u8 white_rgba16[2] = { 0xff, 0xff };
    u32 vertex_ucode_size;
    u32 fragment_ucode_size;

    s_triangle_vertex_program = (rsxVertexProgram *)rdprsx_triangle_vpo;
    s_triangle_fragment_program = (rsxFragmentProgram *)rdprsx_triangle_fpo;
    s_triangle_position = rsxVertexProgramGetAttrib(s_triangle_vertex_program,
                                                     "position");
    s_triangle_color = rsxVertexProgramGetAttrib(s_triangle_vertex_program,
                                                  "color");
    s_triangle_texcoord = rsxVertexProgramGetAttrib(s_triangle_vertex_program,
                                                    "texcoord");
    if (!s_triangle_position || !s_triangle_color || !s_triangle_texcoord) {
        n64ps3_log(N64PS3_LOG_ERROR,
                   "rdprsx: texture shader is missing position, color, or texcoord");
        return -1;
    }

    rsxVertexProgramGetUCode(s_triangle_vertex_program, &s_triangle_vertex_ucode,
                             &vertex_ucode_size);
    rsxFragmentProgramGetUCode(s_triangle_fragment_program,
                               &s_triangle_fragment_buffer, &fragment_ucode_size);
    if (!s_triangle_vertex_ucode || vertex_ucode_size == 0 ||
        !s_triangle_fragment_buffer || fragment_ucode_size == 0) {
        n64ps3_log(N64PS3_LOG_ERROR, "rdprsx: triangle shader microcode is empty");
        return -1;
    }

    {
        void *fragment_ucode = s_triangle_fragment_buffer;

        s_triangle_fragment_buffer = rsxMemalign(64, fragment_ucode_size);
        if (!s_triangle_fragment_buffer) {
            n64ps3_log(N64PS3_LOG_ERROR, "rdprsx: fragment shader allocation failed");
            return -1;
        }
        memcpy(s_triangle_fragment_buffer, fragment_ucode, fragment_ucode_size);
    }
    if (rsxAddressToOffset(s_triangle_fragment_buffer,
                          &s_triangle_fragment_offset) != 0) {
        n64ps3_log(N64PS3_LOG_ERROR, "rdprsx: fragment shader offset conversion failed");
        return -1;
    }

    s_triangle_vertices = rsxMemalign(
        64, RDPRSX_MAX_TRIANGLES * 3U * (u32)sizeof(RdprsxVertex));
    s_triangle_indices = rsxMemalign(64, RDPRSX_MAX_TRIANGLES * 3U * (u32)sizeof(u16));
    s_triangle_modes = (u32 *)malloc(RDPRSX_MAX_TRIANGLES * sizeof(u32));
    s_triangle_textures = (u8 *)malloc(RDPRSX_MAX_TRIANGLES * sizeof(u8));
    s_triangle_blends = (u8 *)malloc(RDPRSX_MAX_TRIANGLES * sizeof(u8));
    s_triangle_filters = (u8 *)malloc(RDPRSX_MAX_TRIANGLES * sizeof(u8));
    if (!s_triangle_vertices || !s_triangle_indices || !s_triangle_modes ||
        !s_triangle_textures || !s_triangle_blends || !s_triangle_filters) {
        n64ps3_log(N64PS3_LOG_ERROR, "rdprsx: display-list geometry allocation failed");
        return -1;
    }
    if (rsxAddressToOffset(s_triangle_vertices, &s_triangle_vertex_offset) != 0 ||
        rsxAddressToOffset(s_triangle_indices, &s_triangle_index_offset) != 0) {
        n64ps3_log(N64PS3_LOG_ERROR, "rdprsx: geometry buffer offset conversion failed");
        return -1;
    }

    s_texture_count = 0;
    if (rdprsx_init_texture(&s_textures[0], white_rgba16, 2U,
                            1, 1, 2, 2) != 0) {
        n64ps3_log(N64PS3_LOG_ERROR, "rdprsx: white fallback texture allocation failed");
        return -1;
    }
    s_texture_count = 1;

    return 0;
}

static int rdprsx_collect_triangle(void *user_data,
                                  const N64ps3GfxF3dex1Vertex *a,
                                  const N64ps3GfxF3dex1Vertex *b,
                                  const N64ps3GfxF3dex1Vertex *c)
{
    const N64ps3GfxF3dex1Vertex *source[3];
    RdprsxVertex *vertices = (RdprsxVertex *)s_triangle_vertices;
    u16 *indices = (u16 *)s_triangle_indices;
    u32 first;
    u32 i;

    (void)user_data;
    if (s_triangle_count >= RDPRSX_MAX_TRIANGLES) {
        return -1;
    }

    source[0] = a;
    source[1] = b;
    source[2] = c;
    first = s_triangle_count * 3U;
    for (i = 0; i < 3U; i++) {
        vertices[first + i].x = source[i]->clip_x;
        vertices[first + i].y = source[i]->clip_y;
        vertices[first + i].z = source[i]->clip_z;
        vertices[first + i].w = source[i]->clip_w;
        vertices[first + i].color[0] = (float)source[i]->color[0] / 255.0f;
        vertices[first + i].color[1] = (float)source[i]->color[1] / 255.0f;
        vertices[first + i].color[2] = (float)source[i]->color[2] / 255.0f;
        vertices[first + i].color[3] = (float)source[i]->color[3] / 255.0f;
        if (source[i]->texture_enabled && source[i]->texture_width != 0U &&
            source[i]->texture_height != 0U) {
            vertices[first + i].texcoord[0] =
                ((f32)source[i]->s * (f32)source[i]->texture_scale_s) /
                (65536.0f * 32.0f * (f32)source[i]->texture_width);
            vertices[first + i].texcoord[1] =
                ((f32)source[i]->t * (f32)source[i]->texture_scale_t) /
                (65536.0f * 32.0f * (f32)source[i]->texture_height);
        } else {
            vertices[first + i].texcoord[0] = 0.0f;
            vertices[first + i].texcoord[1] = 0.0f;
        }
        indices[first + i] = (u16)(first + i);
    }
    s_triangle_modes[s_triangle_count] = a->geometry_mode;
    s_triangle_textures[s_triangle_count] = rdprsx_get_texture(a);
    s_triangle_blends[s_triangle_count] = a->blend_enabled;
    s_triangle_filters[s_triangle_count] = a->texture_point_filter;
    s_triangle_count++;
    return 0;
}

static void rdprsx_draw_triangles(void)
{
    static int logged_vertex_probe;
    f32 scale[4];
    f32 offset[4];
    u32 i;

    scale[0] = (f32)s_width * 0.5f;
    scale[1] = (f32)s_height * -0.5f;
    scale[2] = 0.5f;
    scale[3] = 0.0f;
    offset[0] = (f32)s_width * 0.5f;
    offset[1] = (f32)s_height * 0.5f;
    offset[2] = 0.5f;
    offset[3] = 0.0f;

    rsxSetColorMask(s_context, GCM_COLOR_MASK_B | GCM_COLOR_MASK_G |
                               GCM_COLOR_MASK_R | GCM_COLOR_MASK_A);
    rsxSetColorMaskMrt(s_context, 0);
    rsxSetBlendEquation(s_context, GCM_FUNC_ADD, GCM_FUNC_ADD);
    rsxSetBlendFunc(s_context, GCM_SRC_ALPHA, GCM_ONE_MINUS_SRC_ALPHA,
                    GCM_SRC_ALPHA, GCM_ONE_MINUS_SRC_ALPHA);
    rsxSetViewport(s_context, 0, 0, (u16)s_width, (u16)s_height,
                   0.0f, 1.0f, scale, offset);
    rsxSetScissor(s_context, 0, 0, (u16)s_width, (u16)s_height);
    for (i = 0; i < 8; i++) {
        rsxSetViewportClip(s_context, (u8)i, (u16)s_width, (u16)s_height);
    }
    rsxSetBlendEnable(s_context, GCM_FALSE);

    if (s_triangle_count == 0) {
        return;
    }

    if (!logged_vertex_probe) {
        const RdprsxVertex *first = (const RdprsxVertex *)s_triangle_vertices;

        n64ps3_log(N64PS3_LOG_INFO,
                   "rdprsx: probe attrs position=%u color=%u vertex=(%.3f,%.3f,%.3f,%.3f) rgba=(%.3f,%.3f,%.3f,%.3f)",
                   (u32)s_triangle_position->index,
                   (u32)s_triangle_color->index,
                   first->x, first->y, first->z, first->w,
                   first->color[0], first->color[1], first->color[2],
                   first->color[3]);
        logged_vertex_probe = 1;
    }

    rsxBindVertexArrayAttrib(s_context, (u8)s_triangle_position->index, 0,
                             s_triangle_vertex_offset, sizeof(RdprsxVertex), 4,
                             GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);
    rsxBindVertexArrayAttrib(s_context, (u8)s_triangle_color->index, 0,
                             s_triangle_vertex_offset + 16U,
                             sizeof(RdprsxVertex), 4,
                             GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);
    rsxBindVertexArrayAttrib(s_context, (u8)s_triangle_texcoord->index, 0,
                             s_triangle_vertex_offset + 32U,
                             sizeof(RdprsxVertex), 2,
                             GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);
    rsxLoadVertexProgram(s_context, s_triangle_vertex_program,
                         s_triangle_vertex_ucode);
    rsxSetFragmentProgramControl(s_context, s_triangle_fragment_program,
                                 GCM_TRUE, 0, 0);
    rsxLoadFragmentProgramLocation(s_context, s_triangle_fragment_program,
                                   s_triangle_fragment_offset, GCM_LOCATION_RSX);

    i = 0;
    while (i < s_triangle_count) {
        u32 geometry_mode = s_triangle_modes[i];
        u8 texture_index = s_triangle_textures[i];
        u8 blend_enabled = s_triangle_blends[i];
        u8 point_filter = s_triangle_filters[i];
        u32 batch_end = i + 1U;
        u32 cull_mode = geometry_mode &
            (N64PS3_GFX_F3DEX1_GEOMETRY_CULL_FRONT |
             N64PS3_GFX_F3DEX1_GEOMETRY_CULL_BACK);
        int depth_enabled =
            (geometry_mode & N64PS3_GFX_F3DEX1_GEOMETRY_ZBUFFER) != 0;

#if N64PS3_RSX_DISABLE_DEPTH
        depth_enabled = 0;
#endif

        while (batch_end < s_triangle_count &&
               s_triangle_modes[batch_end] == geometry_mode &&
               s_triangle_textures[batch_end] == texture_index &&
               s_triangle_blends[batch_end] == blend_enabled &&
               s_triangle_filters[batch_end] == point_filter) {
            batch_end++;
        }

        rsxLoadTexture(s_context, 0, &s_textures[texture_index].descriptor);
        rsxTextureControl(s_context, 0, GCM_TRUE, 0, 0,
                          GCM_TEXTURE_MAX_ANISO_1);
        rsxTextureFilter(s_context, 0, 0,
                         point_filter ? GCM_TEXTURE_NEAREST
                                      : GCM_TEXTURE_LINEAR,
                         point_filter ? GCM_TEXTURE_NEAREST
                                      : GCM_TEXTURE_LINEAR,
                         0);
        rsxTextureWrapMode(s_context, 0,
                           rdprsx_map_wrap_mode(s_textures[texture_index].wrap_s),
                           rdprsx_map_wrap_mode(s_textures[texture_index].wrap_t),
                           GCM_TEXTURE_CLAMP_TO_EDGE,
                           GCM_TEXTURE_UNSIGNED_REMAP_NORMAL,
                           GCM_TEXTURE_ZFUNC_LEQUAL, 0);
        rsxInvalidateTextureCache(s_context, GCM_INVALIDATE_TEXTURE);

        rsxSetDepthTestEnable(s_context,
                              depth_enabled ? GCM_TRUE : GCM_FALSE);
        rsxSetBlendEnable(s_context,
                          blend_enabled ? GCM_TRUE : GCM_FALSE);
        rsxSetDepthFunc(s_context, GCM_LEQUAL);
        rsxSetDepthWriteEnable(s_context,
                               depth_enabled ? GCM_TRUE : GCM_FALSE);
        rsxSetCullFaceEnable(s_context, cull_mode != 0 ? GCM_TRUE : GCM_FALSE);
        if (cull_mode == (N64PS3_GFX_F3DEX1_GEOMETRY_CULL_FRONT |
                          N64PS3_GFX_F3DEX1_GEOMETRY_CULL_BACK)) {
            rsxSetCullFace(s_context, GCM_CULL_ALL);
        } else if (cull_mode & N64PS3_GFX_F3DEX1_GEOMETRY_CULL_BACK) {
            rsxSetCullFace(s_context, GCM_CULL_BACK);
        } else if (cull_mode & N64PS3_GFX_F3DEX1_GEOMETRY_CULL_FRONT) {
            rsxSetCullFace(s_context, GCM_CULL_FRONT);
        }
        rsxSetFrontFace(s_context, GCM_FRONTFACE_CCW);
        rsxInvalidateVertexCache(s_context);
        rsxDrawIndexArray(s_context, GCM_TYPE_TRIANGLES,
                          s_triangle_index_offset + i * 3U * sizeof(u16),
                          (batch_end - i) * 3U,
                          GCM_INDEX_TYPE_16B, GCM_LOCATION_RSX);
        i = batch_end;
    }
}

static int rdprsx_init(void)
{
    videoState state;
    videoResolution resolution;
    videoConfiguration configuration;
    u32 i;

    memset(&state, 0, sizeof(state));
    if (videoGetState(VIDEO_PRIMARY, 0, &state) != 0 ||
        videoGetResolution(state.displayMode.resolution, &resolution) != 0) {
        n64ps3_log(N64PS3_LOG_ERROR, "rdprsx: unable to query current video mode");
        return -1;
    }

    s_width = resolution.width;
    s_height = resolution.height;
    s_pitch = s_width * sizeof(u32);
    memset(&configuration, 0, sizeof(configuration));
    configuration.resolution = state.displayMode.resolution;
    configuration.format = VIDEO_BUFFER_FORMAT_XRGB;
    configuration.aspect = state.displayMode.aspect;
    configuration.pitch = s_pitch;

    s_host_io = memalign(RDPRSX_HOST_ALIGNMENT, RDPRSX_HOST_IO_SIZE);
    if (!s_host_io || rsxInit(&s_context, RDPRSX_COMMAND_BUFFER_SIZE,
                              RDPRSX_HOST_IO_SIZE, s_host_io) != 0 || !s_context) {
        n64ps3_log(N64PS3_LOG_ERROR, "rdprsx: RSX context initialization failed");
        rdprsx_release();
        return -1;
    }

    if (videoConfigure(VIDEO_PRIMARY, &configuration, NULL, 0) != 0) {
        n64ps3_log(N64PS3_LOG_ERROR, "rdprsx: video configuration failed");
        rdprsx_release();
        return -1;
    }

    gcmSetFlipMode(GCM_FLIP_VSYNC);
    gcmResetFlipStatus();
    for (i = 0; i < RDPRSX_FRAMEBUFFER_COUNT; i++) {
        s_framebuffers[i].id = (u8)i;
        s_framebuffers[i].address = rsxMemalign(64, s_pitch * s_height);
        if (!s_framebuffers[i].address ||
            rsxAddressToOffset(s_framebuffers[i].address, &s_framebuffers[i].offset) != 0 ||
            gcmSetDisplayBuffer(s_framebuffers[i].id, s_framebuffers[i].offset,
                                s_pitch, s_width, s_height) != 0) {
            n64ps3_log(N64PS3_LOG_ERROR, "rdprsx: framebuffer %u allocation failed", i);
            rdprsx_release();
            return -1;
        }
    }

    s_depth_buffer = rsxMemalign(64, s_pitch * s_height);
    if (!s_depth_buffer || rsxAddressToOffset(s_depth_buffer, &s_depth_offset) != 0) {
        n64ps3_log(N64PS3_LOG_ERROR, "rdprsx: depth buffer allocation failed");
        rdprsx_release();
        return -1;
    }

    if (rdprsx_init_triangle() != 0) {
        rdprsx_release();
        return -1;
    }

    s_current_framebuffer = 0;
    rdprsx_set_render_target(s_current_framebuffer);
    s_ready = 1;
    n64ps3_log(N64PS3_LOG_INFO, "rdprsx ready: %ux%u, two XRGB framebuffers",
               s_width, s_height);
    return 0;
}

static void rdprsx_shutdown(void)
{
    rdprsx_release();
    n64ps3_log(N64PS3_LOG_INFO, "rdprsx shutdown");
}

static void rdprsx_wait_for_previous_flip(void)
{
#if N64PS3_RSX_TEXTURE_DIAGNOSTICS
    s_flip_wait_iterations = 0;
#endif
    if (!s_flip_pending) {
        return;
    }

    while (gcmGetFlipStatus() != 0) {
#if N64PS3_RSX_TEXTURE_DIAGNOSTICS
        s_flip_wait_iterations++;
#endif
        usleep(200);
    }
    gcmResetFlipStatus();
    s_flip_pending = 0;
}

static void rdprsx_begin_frame(void)
{
    if (!s_ready) {
        return;
    }

    /* The decoder rewrites the RSX vertex/index buffers below. Wait until the
     * previous frame's draw and flip have consumed them before reusing them. */
    rdprsx_wait_for_previous_flip();

    /* The XRGB scanout surface should present the original black title clear. */
    rsxSetClearColor(s_context, 0xff000000U);
    s_triangle_count = 0;
    rsxSetClearDepthStencil(s_context, 0xffffff00U);
    rsxClearSurface(s_context, GCM_CLEAR_R | GCM_CLEAR_G | GCM_CLEAR_B |
                               GCM_CLEAR_A | GCM_CLEAR_Z);
}

static void rdprsx_submit_task(const N64ps3GfxTask *task)
{
    static int warned_unsupported;
    static int logged_title_decode;
    N64ps3GfxF3dex1Stats stats;
    N64ps3GfxF3dex1Status status;

    status = n64ps3_gfx_decode_f3dex1(task, rdprsx_collect_triangle, NULL, &stats);
    if (status != N64PS3_GFX_F3DEX1_OK) {
        n64ps3_log(N64PS3_LOG_ERROR,
                   "rdprsx: F3DEX1 decode failed: status=%d commands=%u triangles=%u",
                   (int)status, stats.command_count, stats.triangle_count);
        return;
    }
    if (stats.unsupported_command_count && !warned_unsupported) {
        n64ps3_log(N64PS3_LOG_WARN,
                   "rdprsx: skipped %u F3DEX1/RDP state commands; matrices and RDP state are not translated",
                   stats.unsupported_command_count);
        warned_unsupported = 1;
    }
    if (!logged_title_decode && task && task->debug_name &&
        strstr(task->debug_name, "logos")) {
        n64ps3_log(N64PS3_LOG_INFO,
                   "rdprsx title diagnostic: commands=%u triangles=%u textured=%u "
                   "texture_loads=%u unsupported=%u rsx_triangles=%u",
                   stats.command_count, stats.triangle_count,
                   stats.textured_triangle_count, stats.texture_load_count,
                   stats.unsupported_command_count, s_triangle_count);
        logged_title_decode = 1;
    }
}

static void rdprsx_end_frame(void)
{
    s32 flip_status;

    if (!s_ready) {
        return;
    }

#if N64PS3_RSX_TEXTURE_DIAGNOSTICS
    rdprsx_log_texture_inputs();
#endif
    rdprsx_draw_triangles();
    flip_status = gcmSetFlip(s_context, s_framebuffers[s_current_framebuffer].id);
    if (flip_status != 0) {
        n64ps3_log(N64PS3_LOG_ERROR, "rdprsx: gcmSetFlip failed (%d)", flip_status);
        return;
    }
    s_flip_pending = 1;
    rsxFlushBuffer(s_context);
    gcmSetWaitFlip(s_context);
    s_current_framebuffer = (s_current_framebuffer + 1) % RDPRSX_FRAMEBUFFER_COUNT;
    rdprsx_set_render_target(s_current_framebuffer);
}

const N64ps3GfxRenderer *n64ps3_rdprsx_renderer(void)
{
    static const N64ps3GfxRenderer renderer = {
        "rdprsx",
        rdprsx_init,
        rdprsx_shutdown,
        rdprsx_begin_frame,
        rdprsx_submit_task,
        rdprsx_end_frame
    };
    return &renderer;
}
