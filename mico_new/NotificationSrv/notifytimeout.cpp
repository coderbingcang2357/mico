#include <assert.h>
#include "notifytimeout.h"
#include "timer/timer.h"
#include "mainqueue.h"
#include "queuemessage.h"
#include "thread/mutexguard.h"

class TimerProc : public Proc
{
public:
    TimerProc(NotifyTimeOut *nt, uint64_t notifyid)
            : m_timeout(nt), m_notifyid(notifyid), m_times(0) {}
    void run()
    {
        //printf("timeout.\n");
        // post a message to main thread that a notify is timeout
        m_times++;
        // 5 10 15, 设置新的超时, 这个计算方法是随便写的
        uint32_t newtimeout = (m_times + 1)* 5000;
        m_timeout->setNewInterval(m_notifyid, newtimeout);
        NotifyTimeoutMessage *msg = new NotifyTimeoutMessage(m_notifyid,
                                                            m_times);
        ::postMessage(msg);
    }

private:
    NotifyTimeOut *m_timeout;
    uint64_t m_notifyid;
    int m_times;
};

NotifyTimeOut::~NotifyTimeOut()
{
    for (auto it = m_idtimer.begin(); it != m_idtimer.end(); ++it)
    {
        delete it->second;
    }
    m_idtimer.clear();
}

bool NotifyTimeOut::addTimeout(uint64_t notifyid)
{
    // 已经存在了就不重复添加了
    if (m_idtimer.find(notifyid) != m_idtimer.end())
        return false;

    TimerProc *tp = new TimerProc(this, notifyid);
    Timer *ti = new Timer(5000);
    ti->setTimeoutCallback(tp);
    ti->start();
    m_idtimer.insert(std::make_pair(notifyid, ti));

    return true;
}

bool NotifyTimeOut::removeTimeout(uint64_t notifyid)
{
    MutexGuard guard(&m_lock);
    auto it = m_idtimer.find(notifyid);
    if (it != m_idtimer.end())
    {
        delete it->second;
        m_idtimer.erase(it);
        return true;
    }
    else
    {
        return false;
    }
}

void NotifyTimeOut::setNewInterval(uint64_t notifyid, uint32_t newinterval)
{
    MutexGuard guard(&m_lock);
    auto it = m_idtimer.find(notifyid);
    if (it != m_idtimer.end())
    {
        it->second->setInterval(newinterval);
        it->second->start();
    }
}

