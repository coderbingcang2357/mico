#include "serverqueue.h"
#include "connectionservermessage.h"

ServerQueue::~ServerQueue()
{
    lock.lock();
    for (IConnectionServerMessage *m : m_msg)
    {
        delete m;
    }
    m_msg.clear();
    lock.release();
}

void ServerQueue::addMessage(IConnectionServerMessage *msg)
{
    lock.lock();
    m_msg.push_back(msg);
    m_msg.size();
    lock.release();
}

void ServerQueue::getMessage(std::list<IConnectionServerMessage *> *msg)
{
    lock.lock();
    msg->swap(m_msg);
    lock.release();
}

