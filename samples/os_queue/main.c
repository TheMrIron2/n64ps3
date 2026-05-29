#include "n64ps3/n64ps3.h"

#include <stdio.h>

int main(void)
{
    OSMesg storage[4];
    OSMesgQueue queue;
    int value = 42;
    OSMesg received = 0;

    osCreateMesgQueue(&queue, storage, 4);
    osSendMesg(&queue, &value, OS_MESG_NOBLOCK);
    osRecvMesg(&queue, &received, OS_MESG_NOBLOCK);

    printf("received=%d\n", *(int *)received);
    return 0;
}
