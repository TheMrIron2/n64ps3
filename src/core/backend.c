#include "n64ps3/backend.h"

const char *n64ps3_backend_name(void)
{
    return "host";
}

N64ps3BackendKind n64ps3_backend_kind(void)
{
    return N64PS3_BACKEND_KIND_HOST;
}
