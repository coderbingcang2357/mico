#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>
#include <memory>
#include <functional>
#include "thread/threadwrap.h"
class TimerServer;

class Timer
{
public:
    Timer(uint32_t ms, TimerServer *ts = NULL);
    ~Timer();
    void setTimeoutCallback(Proc *p);
    void setTimeoutCallback(const std::function<void()> &p);
    void setInterval(uint32_t ms);
    void start();
    void stop();
    void runOnce();
private:
    uint64_t _timerid;
    uint32_t _ms;
    std::shared_ptr<Proc> _p;
    TimerServer *_timerserver;
};

#endif
