#include "timer.h"
#include "timerserver.h"
#include <stdio.h>

class FuncWrapProc : public Proc
{
public:
    FuncWrapProc(const std::function<void()> &func) : m_fun(func)
    {
    }

    void run() override
    {
        m_fun();
    }
private:
    std::function<void()> m_fun;
};

Timer::Timer(uint32_t ms, TimerServer *ts)
    : _timerid(0), _ms(ms),  _p(0), _timerserver(ts)
{
    if (_timerserver == NULL)
    {
        _timerserver = TimerServer::server();
    }
}

Timer::~Timer()
{
    stop();
    //if (_p)
    //    delete _p;
}

void Timer::setTimeoutCallback(Proc *p)
{
    _p = std::shared_ptr<Proc>(p);
}

void Timer::setTimeoutCallback(const std::function<void()> &p)
{
    _p = std::make_shared<FuncWrapProc>(p);
}

void Timer::setInterval(uint32_t ms)
{
    this->_ms = ms;
}

void Timer::start()
{
    stop();
    _timerid = _timerserver->runEvery(_p, _ms);
    //printf("timerid: %lu\n", _timerid);
}

void Timer::stop()
{
    if (_timerid != 0)
    {
        _timerserver->removeTimer(_timerid);
        _timerid = 0;
    }
}

void Timer::runOnce()
{
    stop();
    _timerid = _timerserver->runOnce(_p, _ms);
    //printf("timerid: %lu\n", _timerid);
}
