#ifndef SHMMSG_QUEUE_H
#define SHMMSG_QUEUE_H

#include "thread/mutexwrap.h"
#include "thread/conditionwrap.h"
#include "imem.h"
class MsgQueue;
class ShmWrap;
class MsgQueue
{
public:
    int msgsize;
    MutexWrap mutex;
    ConditionWrap notEmptycond;
    ConditionWrap notFullCond;
    char *startp;
    char *end;
    char *head;
    char *tail;
};

class ShmMessageQueue
{
public:
    ShmMessageQueue(IMem *shm);
    bool init();
    int messageSize();
    int get(void *buf, int buflen);
    bool put(void *buf, int len);
private:
    bool hasEnoughMemory(int needBytes);
    int freeSize();

    MsgQueue *d;
    IMem *m_shm;
};
#endif
