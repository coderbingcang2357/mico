#include "mainqueue.h"
#include "msgqueue/msgqueue.h"

namespace MainQueue
{

static BlockMessageQueue<RelayInterMessage *> mainqueue_p;

RelayInterMessage *getMsg()
{
    return mainqueue_p.get();
}

void postMsg(RelayInterMessage *msg)
{
    mainqueue_p.put(msg);
}

bool hasMsg()
{
    return mainqueue_p.hasMessage();
}

void quit()
{
    mainqueue_p.quit();
}

} // end space mainqueue
