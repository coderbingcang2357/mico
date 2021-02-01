#include "connidgen.h"

static uint64_t connid_p = 1;
uint64_t genconnid()
{
    connid_p++;
    if (connid_p == 0)
        connid_p = 1;
    return connid_p;
}
