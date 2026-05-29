#include "n64ps3/pi.h"
#include "n64ps3/log.h"

#include <stdio.h>

static FILE *g_rom;

int n64ps3_pi_mount_rom(const char *path)
{
    n64ps3_pi_unmount_rom();
    g_rom = fopen(path, "rb");
    if (!g_rom) {
        n64ps3_log(N64PS3_LOG_WARN, "failed to mount ROM: %s", path ? path : "(null)");
        return -1;
    }
    return 0;
}

int n64ps3_pi_read_rom(u32 rom_offset, void *dst, u32 size)
{
    if (!g_rom || !dst) {
        return -1;
    }
    if (fseek(g_rom, (long)rom_offset, SEEK_SET) != 0) {
        return -1;
    }
    return fread(dst, 1, size, g_rom) == size ? 0 : -1;
}

void n64ps3_pi_unmount_rom(void)
{
    if (g_rom) {
        fclose(g_rom);
        g_rom = 0;
    }
}
