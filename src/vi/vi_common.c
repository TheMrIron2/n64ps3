#include "n64ps3/vi.h"

#include "n64ps3/log.h"

static N64ps3ViMode g_mode = { 320, 240, 60 };
static u8 g_vi_black = 0;
static OSMesgQueue *g_vi_event_queue = 0;
static OSMesg g_vi_event_message = 0;
static u32 g_vi_retrace_count = 1;
static u32 g_vi_retrace_accum = 0;

void n64ps3_vi_set_mode(const N64ps3ViMode *mode)
{
    if (mode) {
        g_mode = *mode;
    }

    n64ps3_log(
        N64PS3_LOG_INFO,
        "VI mode: %ux%u @ %uHz",
        g_mode.width,
        g_mode.height,
        g_mode.refresh_hz
    );
}

void n64ps3_vi_swap_buffer(const void *framebuffer, u32 width, u32 height, u32 stride_bytes)
{
    (void)framebuffer;
    (void)stride_bytes;

    n64ps3_log(N64PS3_LOG_DEBUG, "VI swap: %ux%u", width, height);
}

void n64ps3_vi_raise_retrace_for_host(void)
{
    if (!g_vi_event_queue) {
        n64ps3_os_raise_event(OS_EVENT_VI);
        return;
    }

    g_vi_retrace_accum++;
    if (g_vi_retrace_accum < g_vi_retrace_count) {
        return;
    }

    g_vi_retrace_accum = 0;
    (void)osSendMesg(g_vi_event_queue, g_vi_event_message, OS_MESG_NOBLOCK);
    n64ps3_os_raise_event(OS_EVENT_VI);
}

void osViSetMode(const OSViMode *mode)
{
    n64ps3_vi_set_mode(mode);
}

void osViBlack(u8 black)
{
    g_vi_black = black ? 1u : 0u;
    n64ps3_log(N64PS3_LOG_DEBUG, "VI black: %u", (unsigned)g_vi_black);
}

void osViSwapBuffer(void *framebuffer)
{
    n64ps3_vi_swap_buffer(framebuffer, g_mode.width, g_mode.height, g_mode.width * 2u);
}

void osViSetEvent(OSMesgQueue *queue, OSMesg message, u32 retrace_count)
{
    g_vi_event_queue = queue;
    g_vi_event_message = message;
    g_vi_retrace_count = retrace_count ? retrace_count : 1u;
    g_vi_retrace_accum = 0;

    osSetEventMesg(OS_EVENT_VI, queue, message);
}
