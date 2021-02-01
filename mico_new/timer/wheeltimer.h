#ifndef _ROLLTIMER__H
#define _ROLLTIMER__H

#include <stdint.h>
#include <functional>
#include <unordered_set>
#include <memory>
#include <vector>

class WheelTimer;

class WheelTimerPoller
{
public:
    WheelTimerPoller(int rollcount = 8);

    std::weak_ptr<WheelTimer> newTimer(const std::function<void()> &c);
    void update(const std::weak_ptr<WheelTimer> &t);
    void wheel();
    void remove(std::weak_ptr<WheelTimer> t);

private:
    std::vector<std::unordered_set<std::shared_ptr<WheelTimer>>> m_roll;
    uint32_t m_pos = 0;
};

#endif
