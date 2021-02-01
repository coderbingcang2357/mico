#include <sys/time.h>
#include <assert.h>
#include "sessionRandomNumber.h"
#include "util/util.h"
namespace
{
static uint32_t g_inc = 0;
static uint32_t g_serverid = 0;
}

void getNodeId(int port, uint32_t *nodeid){
	// 端口配置从8000开始
	*nodeid = port - 8000;
}
void initialRandomnumberGenerator(uint32_t serverid)
{
    srandom(time(0));
    g_serverid = serverid;
}

// [4bit serverid] [28bit random number] [32 bit inc value]
uint64_t genSessionRandomNumber()
{
    uint64_t ret =  g_serverid;
    ret = ret << 60; // 高4位是serverid
    uint64_t rand = random() & 0xfffffff; // 只取28bit
    rand = rand << 32;

    ret += rand;

    ++g_inc;
    ret += g_inc;
    return ret;
}

uint64_t getSessionRandomNumber(char *p, int len)
{
    assert(len == 8);(void)len;
    p[0] ^= 0x19;
    p[1] ^= 0x92;
    p[2] ^= 0x04;
    p[3] ^= 0x01;

    p[4] ^= 0x19;
    p[5] ^= 0x87;
    p[6] ^= 0x03;
    p[7] ^= 0x11;

    return ::ReadUint64(p);
}
