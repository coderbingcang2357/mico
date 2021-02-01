#include "./clocktimer.h"
#include <time.h>
#include <stdio.h>

ClockTimer::ClockTimer()
{
    m_isFirst.store(true);
    m_hour = -1;
}

void ClockTimer::everyHour(int hour, const std::function<void()> &callback)
{
    m_hour = hour % 24;
    m_callback = callback;
}

void ClockTimer::poll()
{
    time_t t = time(0);
    struct tm curt;
    localtime_r(&t, &curt);
    int dsthour = m_hour.load();
#ifdef __TEST__
    static int hour = 0;
    curt.tm_hour = hour++;
    hour = hour %  24;
#endif
    if (curt.tm_hour == dsthour && m_isFirst)
    {
        printf("cur time: %d\n", curt.tm_hour);
        m_callback();
        m_isFirst = false;
    }
    else if (curt.tm_hour == ((dsthour+1)%24) && !m_isFirst)
    {
        m_isFirst = true;
    }
}

