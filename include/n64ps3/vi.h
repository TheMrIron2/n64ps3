#ifndef N64PS3_VI_H
#define N64PS3_VI_H

#include "n64ps3/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct N64ps3ViMode {
    u32 width;
    u32 height;
    u32 refresh_hz;
} N64ps3ViMode;

void n64ps3_vi_set_mode(const N64ps3ViMode *mode);
void n64ps3_vi_swap_buffer(const void *framebuffer, u32 width, u32 height, u32 stride_bytes);

#ifdef __cplusplus
}
#endif

#endif
