#ifndef N64PS3_GFX_H
#define N64PS3_GFX_H

#include "n64ps3/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum N64ps3GfxTaskKind {
    N64PS3_GFX_TASK_DISPLAY_LIST = 0,
    N64PS3_GFX_TASK_RSP_UCODE = 1
} N64ps3GfxTaskKind;

typedef struct N64ps3GfxTask {
    N64ps3GfxTaskKind kind;
    const void *dram_base;
    const void *display_list;
    u32 display_list_size_bytes;
    const void *ucode;
    u32 ucode_size_bytes;
    const char *debug_name;
} N64ps3GfxTask;

typedef struct N64ps3GfxRenderer {
    const char *name;
    int (*init)(void);
    void (*shutdown)(void);
    void (*begin_frame)(void);
    void (*submit_task)(const N64ps3GfxTask *task);
    void (*end_frame)(void);
} N64ps3GfxRenderer;

void n64ps3_gfx_register_renderer(const N64ps3GfxRenderer *renderer);
void n64ps3_gfx_init(void);
void n64ps3_gfx_shutdown(void);
void n64ps3_gfx_begin_frame(void);
void n64ps3_gfx_submit_task(const N64ps3GfxTask *task);
void n64ps3_gfx_end_frame(void);

const N64ps3GfxRenderer *n64ps3_rdpnull_renderer(void);

#ifdef __cplusplus
}
#endif

#endif
