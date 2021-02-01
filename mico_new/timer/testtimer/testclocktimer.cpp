#include "../clocktimer.h"
#include <unistd.h>
#include <iostream>

class TestClockTimer 
{
public:
    void test(int dsthour)
    {
        int cnt = 0;
        bool isrun = true;
        ClockTimer ct;
        ct.everyHour(dsthour, [&isrun, &cnt, dsthour](){
            isrun = false;
            printf("called: %d\n", dsthour);
        });
        for (;isrun;)
        {
            ct.poll();
            usleep(1000 * 500); // 100ms
        }
    }
};

int main()
{
    TestClockTimer tct;
    for (int i = 24; i >= 0; --i)
    {
        tct.test(i);
    }
    return 0;
}
