#include "n64ps3/gfx.h"
#include "n64ps3/log.h"

static const N64ps3GfxRenderer *g_renderer;

void n64ps3_gfx_register_renderer(const N64ps3GfxRenderer *renderer)
{
    g_renderer = renderer;
}

void n64ps3_gfx_init(void)
{
    if (g_renderer && g_renderer->init) {
        g_renderer->init();
    }
}

void n64ps3_gfx_shutdown(void)
{
    if (g_renderer && g_renderer->shutdown) {
        g_renderer->shutdown();
    }
}

void n64ps3_gfx_begin_frame(void)
{
    if (g_renderer && g_renderer->begin_frame) {
        g_renderer->begin_frame();
    }
}

void n64ps3_gfx_submit_task(const N64ps3GfxTask *task)
{
    if (!task) {
        n64ps3_log(N64PS3_LOG_WARN, "ignored null graphics task");
        return;
    }
    if (g_renderer && g_renderer->submit_task) {
        g_renderer->submit_task(task);
    }
}

void n64ps3_gfx_end_frame(void)
{
    if (g_renderer && g_renderer->end_frame) {
        g_renderer->end_frame();
    }
}
