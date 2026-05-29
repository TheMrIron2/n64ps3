#ifndef N64PS3_LOG_H
#define N64PS3_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum N64ps3LogLevel {
    N64PS3_LOG_DEBUG = 0,
    N64PS3_LOG_INFO,
    N64PS3_LOG_WARN,
    N64PS3_LOG_ERROR
} N64ps3LogLevel;

void n64ps3_log(N64ps3LogLevel level, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
