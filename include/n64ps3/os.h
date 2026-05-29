#ifndef N64PS3_OS_H
#define N64PS3_OS_H

#include "n64ps3/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *OSMesg;

typedef struct OSMesgQueue {
    OSMesg *messages;
    s32 capacity;
    s32 count;
    s32 head;
    s32 tail;
} OSMesgQueue;

typedef enum N64ps3MesgFlags {
    OS_MESG_NOBLOCK = 0,
    OS_MESG_BLOCK = 1
} N64ps3MesgFlags;

void osCreateMesgQueue(OSMesgQueue *queue, OSMesg *storage, s32 count);
s32 osSendMesg(OSMesgQueue *queue, OSMesg message, s32 flags);
s32 osRecvMesg(OSMesgQueue *queue, OSMesg *message, s32 flags);
u64 osGetTime(void);

#ifdef __cplusplus
}
#endif

#endif
