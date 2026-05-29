#ifndef N64PS3_RUNTIME_H
#define N64PS3_RUNTIME_H

#include "n64ps3/config.h"

#ifdef __cplusplus
extern "C" {
#endif

int n64ps3_runtime_init(const N64ps3Config *config);
void n64ps3_runtime_shutdown(void);
const N64ps3Config *n64ps3_runtime_config(void);

#ifdef __cplusplus
}
#endif

#endif
