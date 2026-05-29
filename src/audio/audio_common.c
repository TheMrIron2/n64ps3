#include "n64ps3/audio.h"
#include "n64ps3/log.h"

void n64ps3_audio_init(const N64ps3AudioSpec *spec)
{
    if (spec) {
        n64ps3_log(N64PS3_LOG_INFO, "audio init: %u Hz, %u channels", spec->sample_rate, spec->channels);
    }
}

void n64ps3_audio_submit_s16(const s16 *samples, u32 frame_count)
{
    (void)samples;
    (void)frame_count;
}

void n64ps3_audio_shutdown(void)
{
}
