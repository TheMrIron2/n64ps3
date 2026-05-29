#include "n64ps3/config.h"

N64ps3Config n64ps3_config_default(void)
{
    N64ps3Config config;
    config.app_name = "n64ps3 app";
    config.asset_root = "assets";
    config.save_root = "saves";
    config.vi_refresh_hz = 60;
    return config;
}
