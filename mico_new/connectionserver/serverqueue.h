#ifndef SERVERQUEUE_H__
#define SERVERQUEUE_H__
#include <list>
#include "thread/mutexwrap.h"
#include "thread/mutexguard.h"
class IConnectionServerMessage;

class ServerQueue
{
public:
    ~ServerQueue();
    void addMessage(IConnectionServerMessage *msg);
    void getMessage(std::list<IConnectionServerMessage *> *msg);

private:
    MutexWrap lock;
    std::list<IConnectionServerMessage *> m_msg;
};
#endif
