#include <unistd.h>
#include <stdio.h>
#include "threadpool.h"

class work : public Proc
{
public:
    work(int i = 0) : m_i(i){}
    void run()
    {
        for (int i = 0; i < 100; i++)
        {
            ;
        }
        //printf("finish %d\n", m_i);
    }
private:
    int m_i;
};

int main()
{
    ThreadPool pool(100000);
    pool.init(4);
    for (int i = 0; i < 100 * 10000; i++)
    {
        work *w = new work(i);
        pool.addWorker(w);
    }

    //while (1)
    //sleep(1);
    printf("===================exit.\n");
    return 0;
}
