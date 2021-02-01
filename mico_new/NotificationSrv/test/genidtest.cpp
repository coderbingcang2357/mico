#include <stdio.h>
#include "../notifyidgen.h"

int main()
{
    Notify::genIdInit(1);
        uint64_t v = Notify::genID();
        uint64_t v1 = Notify::genID();
        uint64_t v2 = Notify::genID();
        uint64_t v3 = Notify::genID();
    printf("%lx, %lx, %lx, %lx\n",
        v, v1, v2, v3);
    return 0;
}

