#ifndef POSIX_MSG_QUEUE_H
#define POSIX_MSG_QUEUE_H

#include <mqueue.h>
#include <string>

enum QueueOperatorResult
{
    ErrorQueue = -1,
    SuccessQueue = 0,
    EmptyQueue = 1,
    FullQueue = 2
};

class MessageQueuePosix
{
public:

    MessageQueuePosix(const char *name, int maxQueueSize, int maxMessageSize);
    ~MessageQueuePosix();
    static int remove(char *name);
    const std::string &name() {return m_name; };
    bool create();
    int put(const char *data, int datalen);
    int get(char *buf, int *buflen);
    mqd_t fd() { return m_queuekey; }

private:
    std::string m_name;
    int m_maxQueueSize;
    int m_maxMessageSize;
    mqd_t m_queuekey;
};

#endif
