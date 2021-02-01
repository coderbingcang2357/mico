#include <stdio.h>
#include <unistd.h>
#include "../wheeltimer.h"

int main(int, char **)
{
    WheelTimerPoller polltimer;
    bool run = true;
    auto t = polltimer.newTimer([](){
        printf("timer out.");
        });
    polltimer.update(t);
    int i = 0;
    int cnt = 0;
    while (run)
    {
        ++i;
        if (i != 3 && cnt < 15)
        {
            polltimer.update(t);
            cnt++;
        }

        sleep(1);
        polltimer.wheel();
        printf("polpolll\n");
    }
    return 0;
}
