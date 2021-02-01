#ifndef THREADWRAP_H
#define THREADWRAP_H

#include <pthread.h>
#include <functional>

class Proc
{
public:
    Proc(){}
    virtual ~Proc(){}
    virtual void run();
};

class ThreadData;
class Thread
{
    Thread(const Thread &);
    Thread &operator = (const Thread &);
public:
    Thread(Proc *tp);
    Thread(const std::function<void()> &t);
    ~Thread();

private:
    ThreadData *d;
};
#endif
