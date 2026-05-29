#include "n64ps3/vi.h"
#include "n64ps3/log.h"

static N64ps3ViMode g_mode = { 320, 240, 60 };

void n64ps3_vi_set_mode(const N64ps3ViMode *mode)
{
    if (mode) {
        g_mode = *mode;
    }
    n64ps3_log(N64PS3_LOG_INFO, "VI mode: %ux%u @ %uHz", g_mode.width, g_mode.height, g_mode.refresh_hz);
}

void n64ps3_vi_swap_buffer(const void *framebuffer, u32 width, u32 height, u32 stride_bytes)
{
    (void)framebuffer;
    (void)stride_bytes;
    n64ps3_log(N64PS3_LOG_DEBUG, "VI swap: %ux%u", width, height);
}
