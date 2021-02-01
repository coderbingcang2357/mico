#ifndef NOTIFYTIMEOUT_H
#define NOTIFYTIMEOUT_H

#include <unordered_map>
#include "Message/Notification.h"
#include "thread/mutexwrap.h"

class Timer;
class NotifyTimeOut
{
public:
    NotifyTimeOut(){}
    ~NotifyTimeOut();
    bool addTimeout(uint64_t notifyid);
    bool removeTimeout(uint64_t notifyid);
    void setNewInterval(uint64_t notifyid, uint32_t newinterval);

private:
    std::unordered_map<uint64_t, Timer *> m_idtimer;
    MutexWrap m_lock;
};
#endif
