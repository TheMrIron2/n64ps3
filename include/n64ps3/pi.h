#ifndef N64PS3_PI_H
#define N64PS3_PI_H

#include "n64ps3/types.h"

#ifdef __cplusplus
extern "C" {
#endif

int n64ps3_pi_mount_rom(const char *path);
int n64ps3_pi_read_rom(u32 rom_offset, void *dst, u32 size);
void n64ps3_pi_unmount_rom(void);

#ifdef __cplusplus
}
#endif

#endif
