#include <time.h>
#include <iostream>
#include <map>
#include <assert.h>
#include "heartbeattimer.h"
#include "timer/timer.h"
#include "userdata.h"
#include "mainthreadmsgqueue.h"
#include "usertimeoutcommand.h"
#include "devicetimeoutcommand.h"
#include "thread/mutexwrap.h"
#include "thread/mutexguard.h"
#include "util/logwriter.h"

class UserTimeOutCallBack : public Proc
{
public:
    UserTimeOutCallBack(uint64_t id) : m_id(id)
    {
    }

    void run()
    {
        // post a message to main thread 
        UserTimeoutCommand *uc = new UserTimeoutCommand(this->m_id);
        MainQueue::postCommand(uc);
    }

    uint64_t m_id;
};

class DeviceTimeOutCallBack : public Proc
{
public:
    DeviceTimeOutCallBack(uint64_t id) : m_id(id)
    {
    }

    void run()
    {
        // post a message to main thread 
        // postMessage(new DeviceTimeoutCommand(m_userid));
        DeviceTimeoutCommand *dc = new DeviceTimeoutCommand(this->m_id);
        MainQueue::postCommand(dc);
    }

    uint64_t m_id;
};

static std::map<uint64_t, Timer *> alltimers;
static MutexWrap timerMutex;

void registerUserTimer(uint64_t userid)
{
    MutexGuard g(&timerMutex);

    if (alltimers.find(userid) != alltimers.end())
        return;

    Timer *timer= new Timer(5000);
    UserTimeOutCallBack *cb = new UserTimeOutCallBack(userid);
    timer->setTimeoutCallback(cb);
    timer->start();
    alltimers.insert(std::make_pair(userid, timer));
}

void registerDeviceTimer(uint64_t devid)
{
    MutexGuard g(&timerMutex);

    if (alltimers.find(devid) != alltimers.end())
        return;

    Timer *timer = new Timer(5000);
    DeviceTimeOutCallBack *cb = new DeviceTimeOutCallBack(devid);
    timer->setTimeoutCallback(cb);
    timer->start();
    alltimers.insert(std::make_pair(devid, timer));
}

void unregisterTimer(uint64_t id)
{
    MutexGuard g(&timerMutex);

    auto it = alltimers.find(id);
    if (it != alltimers.end())
    {
        it->second->stop();
        delete it->second;
        alltimers.erase(id);
    }
    else
    {
        // assert(false);
    }
}
