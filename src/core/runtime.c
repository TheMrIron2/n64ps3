#include "n64ps3/runtime.h"
#include "n64ps3/gfx.h"
#include "n64ps3/log.h"

static N64ps3Config g_config;
static int g_initialized;

int n64ps3_runtime_init(const N64ps3Config *config)
{
    if (g_initialized) {
        return 0;
    }

    g_config = config ? *config : n64ps3_config_default();
    n64ps3_log(N64PS3_LOG_INFO, "runtime init: app=%s asset_root=%s save_root=%s vi=%uHz",
               g_config.app_name ? g_config.app_name : "(null)",
               g_config.asset_root ? g_config.asset_root : "(null)",
               g_config.save_root ? g_config.save_root : "(null)",
               g_config.vi_refresh_hz);

    n64ps3_gfx_register_renderer(n64ps3_rdpnull_renderer());
    n64ps3_gfx_init();

    g_initialized = 1;
    return 0;
}

void n64ps3_runtime_shutdown(void)
{
    if (!g_initialized) {
        return;
    }

    n64ps3_gfx_shutdown();
    n64ps3_log(N64PS3_LOG_INFO, "runtime shutdown");
    g_initialized = 0;
}

const N64ps3Config *n64ps3_runtime_config(void)
{
    return g_initialized ? &g_config : 0;
}
