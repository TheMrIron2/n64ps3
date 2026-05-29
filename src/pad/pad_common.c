#include "n64ps3/pad.h"

#include <string.h>

void n64ps3_pad_init(void)
{
}

int n64ps3_pad_read(int port, N64ps3ControllerState *out_state)
{
    (void)port;
    if (!out_state) {
        return -1;
    }
    memset(out_state, 0, sizeof(*out_state));
    return 0;
}
