#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include "../timer.h"
#include "../timerserver.h"
#include "../TimeElapsed.h"

int c = 0;
class TimerOut : public Proc
{
public:
    TimerOut() : first(true), a(0) {}
    void run()
    {
        if (first)
        {
            c++;
            first = false;
        }
    }

    bool first;
    int a;
    Timer *t;
};

void testPerformance();
void testDelete();
int main()
{
    testDelete();
    testPerformance();
    return 0;
}

void testPerformance()
{
    TimerServer ts;

    std::list<Timer *> l;
    TimeElap te;
    te.start();
    for (int i = 0; i < 100*10000; i++)
    {
        Timer *t = new Timer(500, &ts);
        //TimerOut *to = new TimerOut;
        //to->t = t;
        //t->setTimeoutCallback(to);
        t->setTimeoutCallback([t](){c++;
            delete t;
        });
        //t->start();
        t->runOnce();
        //l.push_back(t);
    }
    printf("create time used: %ld\n", te.elaps());

    TimeElap te2;
    te2.start();
    while (c != 100 * 10000)
    {
        usleep(10);
    }
    printf("time used: %ld\n", te2.elaps());
    //sleep(2);

    //TimeElap deletetime;
    //deletetime.start();
    //for (auto it = l.begin(); it != l.end(); ++it)
    //{
    //    (*it)->stop();
    //    delete *it;
    //}
    // 等待删除完成, 因为定时器删除是延时删除
    usleep(500*1000); // 500ms
    while (ts.remainTimer() != 0)
        usleep(10);
    // printf("delet time used: %ld\n", deletetime.elaps());
    assert(ts.remainTimer() == 0);

    printf("ok. remain timer: %d\n", ts.remainTimer());
}

void testDelete()
{
    TimerServer ts;
    Timer *t = new Timer(500, &ts);
    t->setTimeoutCallback([t](){
        printf("time out\n");
        delete t;
        });
    t->runOnce();
    sleep(2);
}
