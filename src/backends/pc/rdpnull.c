#include "n64ps3/gfx.h"
#include "n64ps3/gfx_f3dex1.h"
#include "n64ps3/log.h"

static int rdpnull_init(void)
{
    n64ps3_log(N64PS3_LOG_INFO, "rdpnull init");
    return 0;
}

static void rdpnull_shutdown(void)
{
    n64ps3_log(N64PS3_LOG_INFO, "rdpnull shutdown");
}

static void rdpnull_begin_frame(void)
{
    n64ps3_log(N64PS3_LOG_DEBUG, "rdpnull begin frame");
}

static void rdpnull_submit_task(const N64ps3GfxTask *task)
{
    N64ps3GfxF3dex1Stats stats;
    N64ps3GfxF3dex1Status status;

    n64ps3_log(N64PS3_LOG_INFO,
               "rdpnull task: name=%s kind=%d dl=%p dl_size=%u ucode=%p ucode_size=%u",
               task->debug_name ? task->debug_name : "(unnamed)",
               (int)task->kind,
               task->display_list,
               task->display_list_size_bytes,
               task->ucode,
               task->ucode_size_bytes);

    status = n64ps3_gfx_decode_f3dex1(task, NULL, NULL, &stats);
    n64ps3_log(status == N64PS3_GFX_F3DEX1_OK ? N64PS3_LOG_INFO : N64PS3_LOG_WARN,
               "rdpnull F3DEX1: status=%d commands=%u vtx_loads=%u triangles=%u dl_calls=%u fill_rects=%u skipped=%u",
               (int)status,
               stats.command_count,
               stats.vertex_load_count,
               stats.triangle_count,
               stats.display_list_call_count,
               stats.fill_rectangle_count,
               stats.unsupported_command_count);
}

static void rdpnull_end_frame(void)
{
    n64ps3_log(N64PS3_LOG_DEBUG, "rdpnull end frame");
}

const N64ps3GfxRenderer *n64ps3_rdpnull_renderer(void)
{
    static const N64ps3GfxRenderer renderer = {
        "rdpnull",
        rdpnull_init,
        rdpnull_shutdown,
        rdpnull_begin_frame,
        rdpnull_submit_task,
        rdpnull_end_frame
    };
    return &renderer;
}
