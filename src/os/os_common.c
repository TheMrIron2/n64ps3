#include "n64ps3/os.h"

#include <time.h>

void osCreateMesgQueue(OSMesgQueue *queue, OSMesg *storage, s32 count)
{
    queue->messages = storage;
    queue->capacity = count;
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;
}

s32 osSendMesg(OSMesgQueue *queue, OSMesg message, s32 flags)
{
    (void)flags;
    if (!queue || queue->count >= queue->capacity) {
        return -1;
    }

    queue->messages[queue->tail] = message;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;
    return 0;
}

s32 osRecvMesg(OSMesgQueue *queue, OSMesg *message, s32 flags)
{
    (void)flags;
    if (!queue || queue->count <= 0) {
        return -1;
    }

    if (message) {
        *message = queue->messages[queue->head];
    }
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;
    return 0;
}

u64 osGetTime(void)
{
    return (u64)clock();
}
