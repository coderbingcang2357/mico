#ifndef CLOCKTIMER__H
#define CLOCKTIMER__H
#include <stdint.h>
#include <functional>
#include <atomic>
class ClockTimer
{
public:
    ClockTimer();
    void everyHour(int hour, const std::function<void()> &callback);
    void poll();

private:

    std::atomic<int> m_hour;
    std::function<void()> m_callback;
    std::atomic<bool> m_isFirst;

    friend class TestClockTimer;
};
#endif
