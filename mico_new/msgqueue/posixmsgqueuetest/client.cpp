#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include "../posixmsgqueue.h"
#include "../../timer/TimeElapsed.h"

int main()
{
    MessageQueuePosix queue("/test", 1000, 20 * 1000);
    if (!queue.create())
    {
        printf("create queue failed.\n");
        return 1;
    }
    TimeElap t;
    for (int i = 0; i < 100* 10000; i++)
    {
        int v[10 * 1000];
        int buflen = sizeof(v);
        again:
        if (queue.get((char *)v, &buflen) == EmptyQueue)
        {
            usleep(1);
            goto again;
        }

        if (i == 0)
        {
            t.start();
        }
    }
    printf("\nget 100 * 10000, ok, timeused: %ld us\n", t.elaps());
    return 0;
}
