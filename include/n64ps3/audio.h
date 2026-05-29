#ifndef N64PS3_AUDIO_H
#define N64PS3_AUDIO_H

#include "n64ps3/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct N64ps3AudioSpec {
    u32 sample_rate;
    u32 channels;
} N64ps3AudioSpec;

void n64ps3_audio_init(const N64ps3AudioSpec *spec);
void n64ps3_audio_submit_s16(const s16 *samples, u32 frame_count);
void n64ps3_audio_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
