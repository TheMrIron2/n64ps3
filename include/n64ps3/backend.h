#ifndef N64PS3_BACKEND_H
#define N64PS3_BACKEND_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum N64ps3BackendKind {
    N64PS3_BACKEND_KIND_HOST = 0,
    N64PS3_BACKEND_KIND_PS3 = 1
} N64ps3BackendKind;

const char *n64ps3_backend_name(void);
N64ps3BackendKind n64ps3_backend_kind(void);

#ifdef __cplusplus
}
#endif

#endif
