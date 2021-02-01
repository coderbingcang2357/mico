#include <assert.h>
#include "wheeltimer.h"

class WheelTimer
{
public:
    ~WheelTimer(){
        if (callback)
        {
            callback();
        }
    }
    std::function<void()> callback = nullptr;
};

WheelTimerPoller::WheelTimerPoller(int rollcount)
{
    m_roll.resize(rollcount);
}

std::weak_ptr<WheelTimer> WheelTimerPoller::newTimer(const std::function<void()> &c)
{
    std::shared_ptr<WheelTimer> t = std::make_shared<WheelTimer>();
    t->callback = c;
    m_roll[m_pos].insert(t);
    return t;
}

void WheelTimerPoller::update(const std::weak_ptr<WheelTimer> &t)
{
    assert(!t.expired());
    if (!t.expired())
    {
        m_roll[m_pos].insert(t.lock());
    }
}

void WheelTimerPoller::remove(std::weak_ptr<WheelTimer> t)
{
    if (!t.expired())
    {
        t.lock()->callback = nullptr;
    }
}

void WheelTimerPoller::wheel()
{
    m_pos++;
    if (m_pos >= m_roll.size())
    {
        m_pos = 0;
    }
    m_roll[m_pos].clear();
}

