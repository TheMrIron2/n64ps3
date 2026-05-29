#ifndef N64PS3_VI_H
#define N64PS3_VI_H

#include "n64ps3/types.h"
#include "n64ps3/os.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct N64ps3ViMode {
    u32 width;
    u32 height;
    u32 refresh_hz;
} N64ps3ViMode;

typedef N64ps3ViMode OSViMode;

void n64ps3_vi_set_mode(const N64ps3ViMode *mode);
void n64ps3_vi_swap_buffer(const void *framebuffer, u32 width, u32 height, u32 stride_bytes);

/*
 * Host-only helper for early validation samples.
 * A later host backend should raise this from a real 50/60 Hz VI tick.
 */
void n64ps3_vi_raise_retrace_for_host(void);

/* libultra-shaped compatibility entry points. */
void osViSetMode(const OSViMode *mode);
void osViBlack(u8 black);
void osViSwapBuffer(void *framebuffer);
void osViSetEvent(OSMesgQueue *queue, OSMesg message, u32 retrace_count);

#ifdef __cplusplus
}
#endif

#endif
