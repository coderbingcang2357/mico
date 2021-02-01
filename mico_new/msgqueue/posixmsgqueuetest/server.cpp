#include <unistd.h>
#include <stdio.h>
#include "../../timer/TimeElapsed.h"
#include "../posixmsgqueue.h"

int main()
{
    MessageQueuePosix queue("/test", 1000, 20 * 1000);
    if (!queue.create())
    {
        printf("create queue failed.\n");
        return 1;
    }
    char data[300];
    int maxretry = 0, retry = 0;
    for (int i = 0; i < 300; i++)
    {
        data[i] = 'a';
    }
    TimeElap t;
    t.start();
    for (int i = 0; i < 100* 10000; i++)
    {
        int tmpretry = 0;
        again:
        if (queue.put((const char *)data, sizeof(data)) == FullQueue)
        {
            retry++;
            tmpretry++;
            usleep(1);
            goto again;
        }
        if (maxretry < tmpretry)
            maxretry = tmpretry;
    }

    printf("\nsend 100 * 10000 ok, time: %ld us, retry: %d, maxretry: %d\n",
                    t.elaps(), retry, maxretry);
    return 0;
}
