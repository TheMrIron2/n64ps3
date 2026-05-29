#ifndef N64PS3_CONFIG_H
#define N64PS3_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct N64ps3Config {
    const char *app_name;
    const char *asset_root;
    const char *save_root;
    unsigned int vi_refresh_hz;
} N64ps3Config;

N64ps3Config n64ps3_config_default(void);

#ifdef __cplusplus
}
#endif

#endif
