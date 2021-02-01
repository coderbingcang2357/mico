#include <stdint.h>
#include <assert.h>
#include <new>
#include "shmmsgqueue.h"
#include "string.h"
#include "thread/mutexguard.h"


ShmMessageQueue::ShmMessageQueue(IMem *shm) : d(0), m_shm(shm)
{
}

bool ShmMessageQueue::init()
{
    d = reinterpret_cast<MsgQueue *>(m_shm->address());
    d->startp = sizeof(*d) + m_shm->address();
    d->end = m_shm->address() + m_shm->length();
    d->head = d->tail = d->startp;
    d->head++; //..
    // ..
    new (&(d->mutex)) MutexWrap;
    new (&(d->notEmptycond)) ConditionWrap;
    new (&(d->notFullCond)) ConditionWrap;
    d->msgsize = 0;
    return true;
}

int ShmMessageQueue::messageSize()
{
    return d->msgsize;
}

bool ShmMessageQueue::hasEnoughMemory(int needBytes)
{
    int freesize = freeSize();
    if (freesize < needBytes + 2) // 2字节是消息长度
        return false;
    else
        return true;
}

int ShmMessageQueue::get(void *buf, int buflen)
{
    MutexGuard guarg(&(d->mutex));

    // empty
    while (d->msgsize <= 0)
    {
        assert(d->tail + 1 == d->head
                || (d->tail + 1 == d->end && d->startp == d->head));
        d->notEmptycond.wait(&(d->mutex));
        //assert(d->tail + 1 == d->head);
        //return 0;
    }
    // empty
    if (d->tail + 1 == d->head
        || (d->tail + 1 == d->end && d->startp == d->head))
    {
        assert(d->msgsize == 0);
        return 0;
    }

    if (d->tail < d->head)
    {
        uint16_t len;
        memcpy(&len, d->tail + 1, sizeof(len));
        assert(len > 0);
        if (len > buflen)
        {
            assert(false);
            return 0;
        }

        d->tail += sizeof(len);
        memcpy(buf, d->tail + 1, len);
        d->tail += len;
        d->msgsize--;
        d->notFullCond.signal();
        return len;
    }
    else //if (d->tail >= d->head)
    {
        uint16_t len;
        char *ptolen = (char *)(&len);
        char *ptotail = d->tail + 1;
        int count = 0;
        for (;;)
        {
            if (ptotail == d->end)
                ptotail = d->startp;

            *ptolen = *ptotail;
            ptolen++;
            ptotail++;
            count++;
            if (count == 2)
                break;
        }
        assert(len > 0);
        if (len > buflen)
        {
            assert(false);
            return 0;
        }
        char *ptobuf = (char *)buf;
        count = 0;
        for (;;)
        {
            if (ptotail == d->end)
                ptotail = d->startp;
            *ptobuf = *ptotail;
            ptobuf++;
            ptotail++;
            count++;
            if (count == len)
                break;
        }
        d->tail = ptotail - 1;
        d->msgsize--;
        d->notFullCond.signal();
        return len;
    }
}

bool ShmMessageQueue::put(void *buf, int len)
{
    MutexGuard guarg(&(d->mutex));

    while (!hasEnoughMemory(len))
    {
        d->notFullCond.wait(&(d->mutex));
        //return false;
    }

    uint16_t len16 = static_cast<uint16_t>(len);
    char tmp[65535];
    memcpy(tmp, &len16, sizeof(len16));
    memcpy(tmp + sizeof(len16), buf, len16);
    len16 += 2;
    if (d->head > d->tail)
    {
        int afterheadlen = d->end - d->head;
        if (afterheadlen >= len16)
        {
            memcpy(d->head, tmp, len16);
            d->head += len16;
            // .
            if (d->head == d->end)
            {
                d->head = d->startp;
                assert(d->head <= d->tail);
            }
            else
            {
                assert(d->head > d->tail);
            }
            //
            d->msgsize++;

            d->notEmptycond.signal();
            return true;
        }
        else
        {
            memcpy(d->head, tmp, afterheadlen);
            d->head = d->startp;
            memcpy(d->head, tmp + afterheadlen, len16 - afterheadlen);
            d->head += (len16 - afterheadlen);
            assert(d->head <= d->tail);
            d->msgsize++;

            d->notEmptycond.signal();
            return true;
        }
    }
    else if (d->head < d->tail)
    {
        memcpy(d->head, tmp, len16);
        assert(d->head <= d->tail);
        d->head += len16;
        d->msgsize++;

        d->notEmptycond.signal();
        return true;
    }
    else
    {
        // d->head == d->tail, full
        assert(false);
        return false;
    }
}

int ShmMessageQueue::freeSize()
{
    // head == tail mean full
    if (d->head == d->tail)
        return 0;

    if (d->head > d->tail)
        return d->end - d->startp - (d->head - d->tail);
    else if (d->head < d->tail)
        return d->tail - d->head;
    else
        return assert(false), 0;
}

