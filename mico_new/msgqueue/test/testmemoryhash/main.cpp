#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "../../memoryhash.h"
#include "../../../timer/TimeElapsed.h"

extern int retrycount;
extern int maxretry;

void testStringValue()
{
    static const int ArrSize = 1024 * 1024 * 200;
    static const int testcount = 400000;
    static const int elesize = 300;
    MemoryHash hash;
    char *p = new char[ArrSize]; // 10k
    hash.init(p, ArrSize, elesize);

    char value[elesize];
    memset(value, 'a', elesize - 1);
    value[elesize - 1]  = 0;

    printf("HashInfo: maxele: %d, tbsize: %d\n", hash.maxElement(), hash.slotSize());

    int *keys = new int[testcount];
    int *values = new int[testcount];

    TimeElap elap;
    elap.start();
    //srand(time(0));
    for (int i = 0; i < testcount; i++)
    {
        int key = i;
        keys[i] = key;
        //values[i] = random();
        //bool r = hash.set(key, (char *)(values + i), sizeof(values[i]));
        bool r = hash.set(key, value, elesize);
        assert(r);
    }
    long elp = elap.elaps();
    printf("timeused:%ld us, %ld s, %ld us\n", elp, elp / (1000 * 1000), elp % (1000 * 1000));

    elap.start();
    for (int i = 0; i < testcount; i++)
    {
        int *v = (int *)hash.get(keys[i]);
        assert(v != 0);
        assert(strcmp((char *)v, value) == 0);
        //printf("%d:%d\n", keys[i], *v);
        //assert(*v == values[i]);
    }
    elp = elap.elaps();
    printf("timeused:%ld us,  %ld s, %ld us\n", elp, elp / (1000 * 1000), elp % (1000 * 1000));

    printf("retrycount: %d\n", retrycount);
    printf("maxretry: %d\n", maxretry);

    delete keys;
    delete values;
    delete p;
}

void testIntvalue()
{
    int prevretrycount = retrycount;
    maxretry = 0;

    static const int ArrSize = 1024 * 1024 * 200; // 200M
    static const int testcount = 4400000;
    static const int elesize = sizeof(int);
    MemoryHash hash;
    char *p = new char[ArrSize]; // 
    hash.init(p, ArrSize, elesize);

    char value[elesize];
    memset(value, 'a', elesize - 1);
    value[elesize - 1]  = 0;

    printf("HashInfo: maxele: %d, tbsize: %d\n", hash.maxElement(), hash.slotSize());

    int *keys = new int[testcount];
    int *values = new int[testcount];

    srand(time(0));
    TimeElap elap;
    elap.start();
    for (int i = 0; i < testcount; i++)
    {
        int key = i;
        keys[i] = key;
        values[i] = random();
        bool r = hash.set(key, (char *)(values + i), elesize);
        assert(r);
    }
    long elp = elap.elaps();
    printf("timeused:%ld us, %ld s, %ld us\n", elp, elp / (1000 * 1000), elp % (1000 * 1000));

    elap.start();
    for (int i = 0; i < testcount; i++)
    {
        int *v = (int *)hash.get(keys[i]);
        assert(v != 0);
        assert(*v == values[i]);
    }
    elp = elap.elaps();
    printf("timeused:%ld us,  %ld s, %ld us\n", elp, elp / (1000 * 1000), elp % (1000 * 1000));

    printf("retrycount: %d\n", retrycount - prevretrycount);
    printf("maxretry: %d\n", maxretry);
    delete keys;
    delete values;
    delete p;
}

void testDelete()
{
    MemoryHash hash;
    static const int bufsize = 1024;
    char *p = new char[bufsize];
    static const int elesize = sizeof(int);
    hash.init(p, bufsize, elesize);

    int value = 10;
    hash.set(2, (char *)&value, elesize);
    value = 11;
    hash.set(3, (char *)&value, elesize);
    value = 11;
    hash.set(4, (char *)&value, elesize);

    // del
    bool r = hash.remove(3);
    assert(r);
    r = hash.remove(3);
    assert(!r);
    int *v = (int *)hash.get(3);
    assert(v == 0);

    // add
    r = hash.set(3, (char *)&value, elesize);
    assert(r);
    v = (int *)hash.get(3);
    assert(*v == 11);
}

int main()
{
    testStringValue();
    testIntvalue();
    testDelete();
    return 0;
}
