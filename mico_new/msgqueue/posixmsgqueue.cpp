#include <errno.h>
#include <string.h>
#include <mqueue.h>
#include <stdio.h>
#include "posixmsgqueue.h"

MessageQueuePosix::MessageQueuePosix(const char *name,
        int maxQueueSize,
        int maxMessageSize)
    : m_name(name),
    m_maxQueueSize(maxQueueSize),
    m_maxMessageSize(maxMessageSize),
    m_queuekey(-1)
{
}

MessageQueuePosix::~MessageQueuePosix()
{
    if (m_queuekey != -1)
        mq_close(m_queuekey);
}

// success return 0, otherwise return error code
int MessageQueuePosix::remove(char *name)
{
    if (mq_unlink(name) == -1)
    {
        int eo = errno;
        return eo;
    }
    else
    {
        return 0;
    }
}

bool MessageQueuePosix::create()
{
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = m_maxQueueSize;
    attr.mq_msgsize = m_maxMessageSize; // 
    attr.mq_curmsgs = 0;

    mqd_t qkey = mq_open(m_name.c_str(),
            O_RDWR | O_CREAT | O_NONBLOCK ,
            0666,
            &attr);

    if (qkey == -1)
    {
        int err = errno;
        if (errno == EMFILE)
            printf("EMFILE\n");
        else if (errno == ENFILE)
            printf("ENFILE");
        //perror("mqopen failed");
        printf("mqopen failed: %d, %s\n", errno, strerror(err));
        return false;
    }
    else
    {
        m_queuekey = qkey;
        return true;
    }
}

int MessageQueuePosix::put(const char *data, int datalen)
{
    if (mq_send(m_queuekey, data, datalen, 0) == -1)
    {
        if (errno == EAGAIN)
        {
            return FullQueue;
        }
        else
        {
            return ErrorQueue;
        }
    }
    else
    {
        return SuccessQueue;
    }
}

// buflen is inout parameter
// buflen is the sizeof buf, and returns the bytes receive
int MessageQueuePosix::get(char *buf, int *buflen)
{
    int len;
    if ((len = mq_receive(m_queuekey, buf, *buflen, 0)) == -1)
    {
        if (errno == EAGAIN)
        {
            //printf("Empty.\n");
            return EmptyQueue;
        }
        else
        {
            return ErrorQueue;
        }
    }
    else // ok
    {
        *buflen = len;
        return SuccessQueue;
    }
}
