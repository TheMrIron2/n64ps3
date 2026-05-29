#include <ultra64.h>

#include <stdio.h>

int main(void)
{
    OSMesg storage[8];
    OSMesgQueue queue;
    OSMesg received = 0;
    int vi_message_value = 1;
    OSViMode mode = { 320, 240, 60 };

    osCreateMesgQueue(&queue, storage, 8);
    osViSetMode(&mode);
    osViSetEvent(&queue, &vi_message_value, 1);

    n64ps3_vi_raise_retrace_for_host();

    if (osRecvMesg(&queue, &received, OS_MESG_NOBLOCK) != 0) {
        printf("no VI retrace message received\n");
        return 1;
    }

    printf("vi_retrace=%d\n", *(int *)received);
    return 0;
}
