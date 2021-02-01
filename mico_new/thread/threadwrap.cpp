#include <stdio.h>
#include "threadwrap.h"

class TProcwrap : public Proc
{
public:
    TProcwrap(const std::function<void()> &f) : m_fun(f) 
    {
    }

    virtual void run() override
    {
        m_fun();
    }
private:
    std::function<void()> m_fun;
};
static void *threadProc(void *d)
{
    Proc *th = static_cast<Proc *>(d);
    th->run();
    return d;
}

class ThreadData
{
public:
    ThreadData(Thread *p)
        : thread(p), isthreadvalid(false)
    {
        
    }
    ~ThreadData()
    {
        delete tp;
    }
    Thread *thread;
    pthread_t tid;
    bool    isthreadvalid;
    Proc *tp;
};

// thread will take owner shitp of tp
Thread::Thread(Proc *tp)
    : d(new ThreadData(this))
{
    d->tp = tp;
    // create thread
    int err = pthread_create(&(d->tid), NULL, threadProc, d->tp);
    if (err != 0)
    {
        perror("Create Thread Failed ");
        d->isthreadvalid = false;
    }
    else
    {
        d->isthreadvalid = true;
    }
}

Thread::Thread(const std::function<void()> &t) : Thread(new TProcwrap(t))
{
}

Thread::~Thread()
{
    void *ret;
    pthread_join(d->tid, &ret);
    delete d;
}

void Proc::run()
{
    // 
    printf("Thread Run.\n");
}

