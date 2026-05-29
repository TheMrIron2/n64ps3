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

typedef enum OSEvent {
    OS_EVENT_SW1 = 0,
    OS_EVENT_SW2,
    OS_EVENT_CART,
    OS_EVENT_COUNTER,
    OS_EVENT_SP,
    OS_EVENT_SI,
    OS_EVENT_AI,
    OS_EVENT_VI,
    OS_EVENT_PI,
    OS_EVENT_DP,
    OS_EVENT_CPU_BREAK,
    OS_EVENT_SP_BREAK,
    OS_EVENT_FAULT,
    OS_EVENT_THREADSTATUS,
    OS_EVENT_PRENMI,
    OS_NUM_EVENTS
} OSEvent;

void osCreateMesgQueue(OSMesgQueue *queue, OSMesg *storage, s32 count);
s32 osSendMesg(OSMesgQueue *queue, OSMesg message, s32 flags);
s32 osRecvMesg(OSMesgQueue *queue, OSMesg *message, s32 flags);
u64 osGetTime(void);

void osSetEventMesg(OSEvent event, OSMesgQueue *queue, OSMesg message);
void n64ps3_os_raise_event(OSEvent event);

#ifdef __cplusplus
}
#endif

#endif
