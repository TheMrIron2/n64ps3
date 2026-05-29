#include "n64ps3/os.h"

typedef struct N64ps3EventBinding {
    OSMesgQueue *queue;
    OSMesg message;
    int active;
} N64ps3EventBinding;

static N64ps3EventBinding g_event_bindings[OS_NUM_EVENTS];

void osSetEventMesg(OSEvent event, OSMesgQueue *queue, OSMesg message)
{
    if (event < 0 || event >= OS_NUM_EVENTS) {
        return;
    }

    g_event_bindings[event].queue = queue;
    g_event_bindings[event].message = message;
    g_event_bindings[event].active = queue != 0;
}

void n64ps3_os_raise_event(OSEvent event)
{
    N64ps3EventBinding *binding;

    if (event < 0 || event >= OS_NUM_EVENTS) {
        return;
    }

    binding = &g_event_bindings[event];
    if (!binding->active || !binding->queue) {
        return;
    }

    /*
     * N64 event delivery should not stall the producer. If the queue is full,
     * the event is dropped for now. Later scheduler work can expose diagnostics.
     */
    (void)osSendMesg(binding->queue, binding->message, OS_MESG_NOBLOCK);
}
