#include "n64ps3/n64ps3.h"

int main(void)
{
    static const unsigned long long fake_display_list[] = {
        0xDEADBEEF00000000ull,
        0x0000000000000000ull
    };

    N64ps3GfxTask task;
    N64ps3Config config = n64ps3_config_default();
    config.app_name = "gfx_task";

    n64ps3_runtime_init(&config);

    task.kind = N64PS3_GFX_TASK_DISPLAY_LIST;
    task.dram_base = 0;
    task.display_list = fake_display_list;
    task.display_list_size_bytes = sizeof(fake_display_list);
    task.ucode = 0;
    task.ucode_size_bytes = 0;
    task.debug_name = "fake display list";

    n64ps3_gfx_begin_frame();
    n64ps3_gfx_submit_task(&task);
    n64ps3_gfx_end_frame();

    n64ps3_runtime_shutdown();
    return 0;
}
