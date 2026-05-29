#include "n64ps3/gfx.h"
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
    n64ps3_log(N64PS3_LOG_INFO,
               "rdpnull task: name=%s kind=%d dl=%p dl_size=%u ucode=%p ucode_size=%u",
               task->debug_name ? task->debug_name : "(unnamed)",
               (int)task->kind,
               task->display_list,
               task->display_list_size_bytes,
               task->ucode,
               task->ucode_size_bytes);
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
