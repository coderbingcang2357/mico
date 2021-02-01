#include <sys/time.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "sessionkeygen.h"
#include "config/configreader.h"
#include "thread/mutexwrap.h"
#include "thread/mutexguard.h"


void genSessionKey(char *buf, int buflen)
{
    assert(buflen == 16);

    uint64_t sessionKeyHigh64 = getRandomNumber();
    uint64_t sessionKeyLoW64 = getRandomNumber();

    memcpy(buf, &sessionKeyHigh64, sizeof(sessionKeyHigh64));
    memcpy(buf + sizeof(sessionKeyHigh64), &sessionKeyLoW64, sizeof(sessionKeyLoW64));
}

void genSessionKey(std::vector<char> *outsessionkey)
{
    char buf[16];
    ::genSessionKey(buf,sizeof(buf));
    *outsessionkey = std::move(std::vector<char>(buf, buf + sizeof(buf)));
}

void genLoginkey(char *buf, int buflen)
{
    return genSessionKey(buf, buflen);
}

uint64_t getRandomNumber()
{
    static MutexWrap getRandnumLock;
    static bool isInit = false;
    MutexGuard g(&getRandnumLock);

    if (!isInit)
    {
        isInit = true;
        srandom((unsigned int) time(0));
    }

    uint64_t ret = random();
    ret <<= 32;
    ret += random();
    return ret;
}

uint64_t genGUID()
{
    static MutexWrap m;
    static int autoinc = 1;
    uint64_t tmp;

    {
        MutexGuard g(&m);
        autoinc++;
        tmp = autoinc;
    }

    // 0xff ff ff ffffffff ff
    uint64_t res = 0;
    Configure *c = Configure::getConfig();
    res = 0xff & c->serverid; // 只取最低字节
    time_t t = time(0);
    t &= 0xffffffff; // 只取低4字节
    t <<= 8;
    res += t; // res

    tmp <<= (32 + 8);
    res += tmp;

    return res;
}

