#include "n64ps3/n64ps3.h"

int main(void)
{
    N64ps3Config config = n64ps3_config_default();
    config.app_name = "hello_runtime";

    n64ps3_runtime_init(&config);
    n64ps3_log(N64PS3_LOG_INFO, "backend=%s", n64ps3_backend_name());
    n64ps3_runtime_shutdown();
    return 0;
}
