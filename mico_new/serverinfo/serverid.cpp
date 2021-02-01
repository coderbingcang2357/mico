#include "serverid.h"

static uint32_t _minserverid = 1;
static uint32_t _maxserverid = 10000;

uint32_t minServerID()
{
    return _minserverid;
}

uint32_t maxServerID()
{
    return _maxserverid;
}

bool isServerID(uint64_t id)
{
    return id >= _minserverid && id < _maxserverid;
}
