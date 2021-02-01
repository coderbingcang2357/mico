#ifndef TIMER_SERVER__H__
#define TIMER_SERVER__H__

#include <stdint.h>
#include <set>
#include <list>
#include <memory>
#include "thread/mutexwrap.h"
#include "thread/conditionwrap.h"

class Proc;
class TimerInfo;
class Thread;
class TimerThread;

typedef bool (*Comp)(const TimerInfo *, const TimerInfo *);

class TimerServer
{
public:
    TimerServer();
    ~TimerServer();
    uint64_t runEvery(const std::shared_ptr<Proc> &p, uint32_t ms);
    uint64_t runOnce(const std::shared_ptr<Proc> &p, uint32_t ms);
    void removeTimer(uint64_t id);

    int remainTimer() { return _timer.size(); }
    static TimerServer *server();
private:
    void processTimerout(const struct timeval &tv);
    void doTimerRemove();

    std::set<TimerInfo *, Comp> _timer;
    std::set<TimerInfo *> _removed; // save the timer to be deleted
    Thread *thread;
    TimerThread *threadproc;
    MutexWrap timerMutex;
    MutexWrap removedMutex;
    ConditionWrap timerCondition;

    friend class TimerThread;
};

#endif
