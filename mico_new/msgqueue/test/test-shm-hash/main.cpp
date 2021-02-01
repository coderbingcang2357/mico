#include <stdio.h>
#include <string.h>
#include "../../memoryhash.h"
#include "../../shmwrap.h"
#include <assert.h>

int main(int argc, char **)
{
    static const int shmsize = 1024 * 1024; // 1M
    static const int elesize = sizeof(int);
    ShmWrap shm;
    MemoryHash hash;
    if (!shm.create("/test-shm-hash", shmsize))
    {
        printf("shm create failed.\n");
        return 0;
    }
    hash.init(shm.address(), shm.length(), elesize);

    if (argc < 2)
    {
        printf("set:\n");
        bool r;
        for (int i = 0; i < 20; i++)
        {
            r = hash.set(i, (char *)&i, elesize);
            assert(r);
        }
        printf("set ok\n");
    }
    else
    {
        printf("get:\n");
        for (int i = 0; i < 20; i++)
        {
            int *v = (int *)hash.get(i);
            assert(*v == i);
            printf("get %d\n", *v);
        }
        printf("get ok\n");
    }
    return 0;
}

