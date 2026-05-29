#ifndef N64PS3_PAD_H
#define N64PS3_PAD_H

#include "n64ps3/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct N64ps3ControllerState {
    u16 buttons;
    s8 stick_x;
    s8 stick_y;
} N64ps3ControllerState;

void n64ps3_pad_init(void);
int n64ps3_pad_read(int port, N64ps3ControllerState *out_state);

#ifdef __cplusplus
}
#endif

#endif
