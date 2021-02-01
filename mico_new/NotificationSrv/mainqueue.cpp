#include "msgqueue/msgqueue.h"
#include "mainqueue.h"

static BlockMessageQueue<QueueMessage *> mainqueue;

void postMessage(QueueMessage *msg)
{
    mainqueue.put(msg);
}

QueueMessage *getQueueMessage()
{
    return mainqueue.get();
}
