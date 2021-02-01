#include <time.h>
#include <stdlib.h>
#include "notifyidgen.h"

namespace
{
static uint32_t g_inc = 0;
static uint32_t g_serverid = 0;
}

void Notify::genIdInit(uint32_t serverid)
{
    srandom(time(0));
    g_serverid = serverid;
}

// [4bit serverid and 最高位是1] [28bit time] [32 bit inc value]
uint64_t Notify::genID()
{
    uint64_t ret =  g_serverid;
    ret = ret << 60; // 高4位是serverid
    ret = ret | (uint64_t(1) << 63); // 最高位设为1
    //uint64_t rand = random() & 0xfffffff; // 只取28bit
    //rand = rand << 32;
    //ret += rand;

    uint64_t t = time(0) & 0xfffffff;
    t = t << 32;
    ret += t;

    ++g_inc;
    ret += g_inc;
    return ret;
}

